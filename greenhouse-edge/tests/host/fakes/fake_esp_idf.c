/*
 * Host-side fake implementation of the narrow ESP-IDF surface the Greenhouse
 * edge services depend on. Lets the real service .c files link and run natively
 * so their state machines (WiFi/MQTT connect + retry, NVS A/B persistence) can
 * be unit-tested. Deterministic and single-threaded -- for host tests only.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "fakes.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "nvs_flash.h"

/* Event base singletons (identity compared by pointer). */
static const char s_wifi_event_base_name[] = "WIFI_EVENT";
static const char s_ip_event_base_name[] = "IP_EVENT";
const char *const WIFI_EVENT_BASE_NAME = s_wifi_event_base_name;
const char *const IP_EVENT_BASE_NAME = s_ip_event_base_name;

/* ------------------------------------------------------------------ clock */
static int64_t s_clock_us;

int64_t esp_timer_get_time(void) { return s_clock_us; }
void gh_fake_clock_set_ms(uint64_t ms) { s_clock_us = (int64_t)(ms * 1000ULL); }
void gh_fake_clock_advance_ms(uint64_t ms) { s_clock_us += (int64_t)(ms * 1000ULL); }

/* ----------------------------------------------------------------- random */
static uint32_t s_random_value;
uint32_t esp_random(void) { return s_random_value; }
void gh_fake_random_set(uint32_t value) { s_random_value = value; }

/* ------------------------------------------------------------ event loop */
#define MAX_HANDLERS 8
typedef struct {
    esp_event_base_t base;
    int32_t id;
    esp_event_handler_t handler;
    void *arg;
    bool used;
} handler_entry_t;
static handler_entry_t s_handlers[MAX_HANDLERS];

esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }

esp_err_t esp_event_handler_register(esp_event_base_t base, int32_t event_id,
                                     esp_event_handler_t handler, void *arg) {
    for (int i = 0; i < MAX_HANDLERS; ++i) {
        if (!s_handlers[i].used) {
            s_handlers[i].base = base;
            s_handlers[i].id = event_id;
            s_handlers[i].handler = handler;
            s_handlers[i].arg = arg;
            s_handlers[i].used = true;
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

void gh_fake_event_post(esp_event_base_t base, int32_t event_id, void *event_data) {
    for (int i = 0; i < MAX_HANDLERS; ++i) {
        if (s_handlers[i].used && s_handlers[i].base == base &&
            (s_handlers[i].id == ESP_EVENT_ANY_ID || s_handlers[i].id == event_id)) {
            s_handlers[i].handler(s_handlers[i].arg, base, event_id, event_data);
        }
    }
}

/* ------------------------------------------------------------------- netif */
esp_err_t esp_netif_init(void) { return ESP_OK; }
void *esp_netif_create_default_wifi_sta(void) { return (void *)1; }

/* -------------------------------------------------------------------- wifi */
static esp_err_t s_wifi_connect_result = ESP_OK;
static int s_wifi_rssi = -50;
static int s_wifi_connect_count;

void gh_fake_wifi_set_connect_result(esp_err_t result) { s_wifi_connect_result = result; }
void gh_fake_wifi_set_rssi(int rssi) { s_wifi_rssi = rssi; }
int gh_fake_wifi_connect_count(void) { return s_wifi_connect_count; }

esp_err_t esp_wifi_init(const wifi_init_config_t *config) { (void)config; return ESP_OK; }
esp_err_t esp_wifi_set_mode(wifi_mode_t mode) { (void)mode; return ESP_OK; }
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {
    (void)interface; (void)conf; return ESP_OK;
}
esp_err_t esp_wifi_start(void) { return ESP_OK; }
esp_err_t esp_wifi_stop(void) { return ESP_OK; }
esp_err_t esp_wifi_connect(void) { s_wifi_connect_count++; return s_wifi_connect_result; }
esp_err_t esp_wifi_disconnect(void) { return ESP_OK; }
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info) {
    if (ap_info == NULL) { return ESP_FAIL; }
    ap_info->rssi = s_wifi_rssi;
    return ESP_OK;
}

/* ------------------------------------------------------------ event group */
static struct gh_fake_event_group { EventBits_t bits; } s_event_group;

EventGroupHandle_t xEventGroupCreate(void) {
    s_event_group.bits = 0;
    return &s_event_group;
}
EventBits_t xEventGroupSetBits(EventGroupHandle_t group, EventBits_t bits) {
    group->bits |= bits; return group->bits;
}
EventBits_t xEventGroupClearBits(EventGroupHandle_t group, EventBits_t bits) {
    group->bits &= ~bits; return group->bits;
}
EventBits_t xEventGroupGetBits(EventGroupHandle_t group) { return group->bits; }

/* -------------------------------------------------------------------- mqtt */
static struct gh_fake_mqtt_client { int alive; } s_mqtt_client;
static esp_event_handler_t s_mqtt_handler;
static void *s_mqtt_handler_arg;
static esp_err_t s_mqtt_reconnect_result = ESP_OK;
static int s_mqtt_publish_result = 1; /* >= 0 means success msg_id */
static int s_mqtt_reconnect_count;
static int s_mqtt_publish_count;
static int s_mqtt_subscribe_count;

void gh_fake_mqtt_set_reconnect_result(esp_err_t result) { s_mqtt_reconnect_result = result; }
void gh_fake_mqtt_set_publish_result(int msg_id) { s_mqtt_publish_result = msg_id; }
int gh_fake_mqtt_reconnect_count(void) { return s_mqtt_reconnect_count; }
int gh_fake_mqtt_publish_count(void) { return s_mqtt_publish_count; }
int gh_fake_mqtt_subscribe_count(void) { return s_mqtt_subscribe_count; }

void gh_fake_mqtt_post_event(int32_t event_id, esp_mqtt_event_t *event) {
    if (s_mqtt_handler != NULL) {
        s_mqtt_handler(s_mqtt_handler_arg, "MQTT_EVENTS", event_id, event);
    }
}

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config) {
    (void)config;
    s_mqtt_client.alive = 1;
    return &s_mqtt_client;
}
esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, int32_t event_id,
                                         esp_event_handler_t handler, void *handler_args) {
    (void)client; (void)event_id;
    s_mqtt_handler = handler;
    s_mqtt_handler_arg = handler_args;
    return ESP_OK;
}
esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client) { (void)client; return ESP_OK; }
esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client) { (void)client; return ESP_OK; }
esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client) {
    (void)client;
    s_mqtt_handler = NULL;
    s_mqtt_handler_arg = NULL;
    return ESP_OK;
}
esp_err_t esp_mqtt_client_reconnect(esp_mqtt_client_handle_t client) {
    (void)client;
    s_mqtt_reconnect_count++;
    return s_mqtt_reconnect_result;
}
esp_err_t esp_mqtt_client_disconnect(esp_mqtt_client_handle_t client) { (void)client; return ESP_OK; }
int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos) {
    (void)client; (void)topic; (void)qos;
    s_mqtt_subscribe_count++;
    return 1;
}
int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic,
                            const char *data, int len, int qos, int retain) {
    (void)client; (void)topic; (void)data; (void)len; (void)qos; (void)retain;
    s_mqtt_publish_count++;
    return s_mqtt_publish_result;
}

/* --------------------------------------------------------------------- nvs */
#define MAX_NS 8
#define MAX_ENTRIES 16
#define NS_NAME_LEN 24
#define KEY_LEN 24
#define VAL_LEN 160

typedef enum { E_NONE = 0, E_STR, E_U32, E_U8 } entry_type_t;
typedef struct {
    char key[KEY_LEN];
    entry_type_t type;
    char str[VAL_LEN];
    uint32_t u32;
    uint8_t u8;
    bool used;
} nvs_entry_t;
typedef struct {
    char name[NS_NAME_LEN];
    bool created;
    nvs_entry_t entries[MAX_ENTRIES];
} nvs_ns_t;
static nvs_ns_t s_ns[MAX_NS];

typedef struct { char name[NS_NAME_LEN]; esp_err_t err; bool used; } nvs_fail_t;
static nvs_fail_t s_fail[MAX_NS];

static int find_ns(const char *name) {
    for (int i = 0; i < MAX_NS; ++i) {
        if (s_ns[i].created && strcmp(s_ns[i].name, name) == 0) { return i; }
    }
    return -1;
}
static int create_ns(const char *name) {
    for (int i = 0; i < MAX_NS; ++i) {
        if (!s_ns[i].created) {
            memset(&s_ns[i], 0, sizeof(s_ns[i]));
            (void)snprintf(s_ns[i].name, sizeof(s_ns[i].name), "%s", name);
            s_ns[i].created = true;
            return i;
        }
    }
    return -1;
}
static esp_err_t ns_write_failure(const char *name) {
    for (int i = 0; i < MAX_NS; ++i) {
        if (s_fail[i].used && strcmp(s_fail[i].name, name) == 0) { return s_fail[i].err; }
    }
    return ESP_OK;
}
static nvs_entry_t *find_entry(nvs_ns_t *ns, const char *key) {
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (ns->entries[i].used && strcmp(ns->entries[i].key, key) == 0) { return &ns->entries[i]; }
    }
    return NULL;
}
static nvs_entry_t *find_or_add_entry(nvs_ns_t *ns, const char *key) {
    nvs_entry_t *e = find_entry(ns, key);
    if (e != NULL) { return e; }
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (!ns->entries[i].used) {
            ns->entries[i].used = true;
            (void)snprintf(ns->entries[i].key, sizeof(ns->entries[i].key), "%s", key);
            return &ns->entries[i];
        }
    }
    return NULL;
}

void gh_fake_nvs_fail_writes(const char *namespace_name, esp_err_t err) {
    for (int i = 0; i < MAX_NS; ++i) {
        if (!s_fail[i].used) {
            (void)snprintf(s_fail[i].name, sizeof(s_fail[i].name), "%s", namespace_name);
            s_fail[i].err = err;
            s_fail[i].used = true;
            return;
        }
    }
}
void gh_fake_nvs_clear_failures(void) { memset(s_fail, 0, sizeof(s_fail)); }

esp_err_t nvs_flash_init(void) { return ESP_OK; }
esp_err_t nvs_flash_erase(void) { return ESP_OK; }

esp_err_t nvs_open(const char *namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    int idx = find_ns(namespace_name);
    if (idx < 0) {
        if (open_mode == NVS_READONLY) { return ESP_ERR_NVS_NOT_FOUND; }
        idx = create_ns(namespace_name);
        if (idx < 0) { return ESP_FAIL; }
    }
    *out_handle = (nvs_handle_t)(idx + 1);
    return ESP_OK;
}
void nvs_close(nvs_handle_t handle) { (void)handle; }

static nvs_ns_t *ns_from_handle(nvs_handle_t handle) {
    if (handle == 0 || handle > MAX_NS) { return NULL; }
    return &s_ns[handle - 1];
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length) {
    nvs_ns_t *ns = ns_from_handle(handle);
    nvs_entry_t *e;
    if (ns == NULL) { return ESP_FAIL; }
    e = find_entry(ns, key);
    if (e == NULL || e->type != E_STR) { return ESP_ERR_NVS_NOT_FOUND; }
    if (out_value != NULL && length != NULL) {
        if (*length < strlen(e->str) + 1U) { return ESP_ERR_INVALID_SIZE; }
        (void)strcpy(out_value, e->str);
    }
    if (length != NULL) { *length = strlen(e->str) + 1U; }
    return ESP_OK;
}
esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *value) {
    nvs_ns_t *ns = ns_from_handle(handle);
    nvs_entry_t *e;
    esp_err_t fail;
    if (ns == NULL) { return ESP_FAIL; }
    fail = ns_write_failure(ns->name);
    if (fail != ESP_OK) { return fail; }
    e = find_or_add_entry(ns, key);
    if (e == NULL) { return ESP_FAIL; }
    e->type = E_STR;
    (void)snprintf(e->str, sizeof(e->str), "%s", value);
    return ESP_OK;
}
esp_err_t nvs_get_u32(nvs_handle_t handle, const char *key, uint32_t *out_value) {
    nvs_ns_t *ns = ns_from_handle(handle);
    nvs_entry_t *e;
    if (ns == NULL) { return ESP_FAIL; }
    e = find_entry(ns, key);
    if (e == NULL || e->type != E_U32) { return ESP_ERR_NVS_NOT_FOUND; }
    if (out_value != NULL) { *out_value = e->u32; }
    return ESP_OK;
}
esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value) {
    nvs_ns_t *ns = ns_from_handle(handle);
    nvs_entry_t *e;
    esp_err_t fail;
    if (ns == NULL) { return ESP_FAIL; }
    fail = ns_write_failure(ns->name);
    if (fail != ESP_OK) { return fail; }
    e = find_or_add_entry(ns, key);
    if (e == NULL) { return ESP_FAIL; }
    e->type = E_U32;
    e->u32 = value;
    return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value) {
    nvs_ns_t *ns = ns_from_handle(handle);
    nvs_entry_t *e;
    if (ns == NULL) { return ESP_FAIL; }
    e = find_entry(ns, key);
    if (e == NULL || e->type != E_U8) { return ESP_ERR_NVS_NOT_FOUND; }
    if (out_value != NULL) { *out_value = e->u8; }
    return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value) {
    nvs_ns_t *ns = ns_from_handle(handle);
    nvs_entry_t *e;
    esp_err_t fail;
    if (ns == NULL) { return ESP_FAIL; }
    fail = ns_write_failure(ns->name);
    if (fail != ESP_OK) { return fail; }
    e = find_or_add_entry(ns, key);
    if (e == NULL) { return ESP_FAIL; }
    e->type = E_U8;
    e->u8 = value;
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle) {
    nvs_ns_t *ns = ns_from_handle(handle);
    if (ns == NULL) { return ESP_FAIL; }
    return ns_write_failure(ns->name);
}

/* ------------------------------------------------------------------- reset */
void gh_fake_reset_all(void) {
    s_clock_us = 0;
    s_random_value = 0;
    memset(s_handlers, 0, sizeof(s_handlers));
    s_wifi_connect_result = ESP_OK;
    s_wifi_rssi = -50;
    s_wifi_connect_count = 0;
    s_event_group.bits = 0;
    s_mqtt_client.alive = 0;
    s_mqtt_handler = NULL;
    s_mqtt_handler_arg = NULL;
    s_mqtt_reconnect_result = ESP_OK;
    s_mqtt_publish_result = 1;
    s_mqtt_reconnect_count = 0;
    s_mqtt_publish_count = 0;
    s_mqtt_subscribe_count = 0;
    memset(s_ns, 0, sizeof(s_ns));
    memset(s_fail, 0, sizeof(s_fail));
}
