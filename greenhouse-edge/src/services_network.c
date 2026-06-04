#include "services_network.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "runtime_config.h"

static const char *TAG = "gh_network";
static const int WIFI_CONNECTED_BIT = BIT0;

static EventGroupHandle_t s_wifi_event_group;
static bool s_started;
static bool s_connect_scheduled;
static bool s_connect_in_progress;
static bool s_bootstrap_failed;
static uint32_t s_retry_count;
static uint64_t s_next_retry_at_ms;
static uint64_t s_connect_started_at_ms;

static uint32_t compute_backoff_ms(uint32_t retry_count) {
    uint32_t backoff = GH_WIFI_RETRY_BASE_MS;
    uint32_t i;
    int32_t jitter_window;
    int32_t jitter;

    for (i = 0; i < retry_count && backoff < GH_WIFI_RETRY_MAX_MS; ++i) {
        backoff *= 2U;
        if (backoff > GH_WIFI_RETRY_MAX_MS) {
            backoff = GH_WIFI_RETRY_MAX_MS;
        }
    }

    jitter_window = (int32_t)(backoff / 5U);
    jitter = (int32_t)(esp_random() % (uint32_t)((jitter_window * 2) + 1)) - jitter_window;
    return (uint32_t)((int32_t)backoff + jitter);
}

static void schedule_connect_retry(void) {
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    const uint32_t wait_ms = compute_backoff_ms(s_retry_count);

    if (s_retry_count >= GH_BOOTSTRAP_RETRY_BUDGET) {
        s_bootstrap_failed = true;
        s_connect_scheduled = false;
        s_connect_in_progress = false;
        ESP_LOGW(TAG, "WiFi bootstrap retry budget exhausted");
        return;
    }

    s_next_retry_at_ms = now_ms + wait_ms;
    s_connect_scheduled = true;
    s_connect_in_progress = false;

    ESP_LOGW(
        TAG,
        "WiFi reconnect scheduled in %lu ms (completed_attempts=%lu)",
        (unsigned long)wait_ms,
        (unsigned long)s_retry_count);
}

static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)arg;
    (void)event_base;
    (void)event_data;

    if (event_id == WIFI_EVENT_STA_START) {
        s_retry_count = 0;
        s_next_retry_at_ms = 0;
        s_connect_scheduled = true;
        s_connect_in_progress = false;
        s_bootstrap_failed = false;
        return;
    }

    if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        schedule_connect_retry();
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    (void)arg;
    (void)event_base;
    (void)event_data;

    if (event_id == IP_EVENT_STA_GOT_IP) {
        s_connect_scheduled = false;
        s_connect_in_progress = false;
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "WiFi connected and got IP");
    }
}

esp_err_t gh_network_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    (void)esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_ip_event, NULL));

    return ESP_OK;
}

esp_err_t gh_network_start(const gh_provisioning_config_t *config) {
    wifi_config_t wifi_cfg;
    size_t ssid_len;
    size_t password_len;

    if (config == NULL || strlen(config->wifi_ssid) == 0U) {
        ESP_LOGE(TAG, "WiFi SSID missing from provisioning config");
        return ESP_ERR_INVALID_ARG;
    }

    ssid_len = strlen(config->wifi_ssid);
    password_len = strlen(config->wifi_password);
    if (ssid_len > sizeof(wifi_cfg.sta.ssid) || password_len > sizeof(wifi_cfg.sta.password)) {
        ESP_LOGE(TAG, "Provisioned WiFi credentials exceed ESP32 limits");
        return ESP_ERR_INVALID_ARG;
    }

    (void)memset(&wifi_cfg, 0, sizeof(wifi_cfg));
    (void)memcpy(wifi_cfg.sta.ssid, config->wifi_ssid, ssid_len);
    (void)memcpy(wifi_cfg.sta.password, config->wifi_password, password_len);
    wifi_cfg.sta.threshold.authmode =
        (strlen(config->wifi_password) == 0U) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi_cfg.sta.pmf_cfg.capable = true;
    wifi_cfg.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_started = true;
    s_bootstrap_failed = false;
    ESP_LOGI(TAG, "WiFi start requested for SSID=%s", config->wifi_ssid);
    return ESP_OK;
}

void gh_network_tick(void) {
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);

    if (!s_started || s_bootstrap_failed || gh_network_is_connected()) {
        return;
    }

    if (s_connect_in_progress) {
        if ((now_ms - s_connect_started_at_ms) >= GH_WIFI_CONNECT_TIMEOUT_MS) {
            ESP_LOGW(TAG, "WiFi connect attempt timed out");
            s_connect_in_progress = false;
            (void)esp_wifi_disconnect();
            schedule_connect_retry();
        }
        return;
    }

    if (s_connect_scheduled && now_ms >= s_next_retry_at_ms) {
        esp_err_t err = esp_wifi_connect();
        if (err == ESP_OK) {
            s_retry_count++;
            s_connect_scheduled = false;
            s_connect_in_progress = true;
            s_connect_started_at_ms = now_ms;
            ESP_LOGI(TAG, "WiFi connect attempt started");
        } else {
            ESP_LOGW(TAG, "WiFi connect call failed err=0x%x", (unsigned int)err);
            s_retry_count++;
            schedule_connect_retry();
        }
    }
}

bool gh_network_is_connected(void) {
    EventBits_t bits;

    if (s_wifi_event_group == NULL) {
        return false;
    }

    bits = xEventGroupGetBits(s_wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

bool gh_network_bootstrap_failed(void) {
    return s_bootstrap_failed;
}

int gh_network_get_rssi(void) {
    wifi_ap_record_t ap_info;

    if (!gh_network_is_connected()) {
        return 0;
    }

    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        return 0;
    }

    return ap_info.rssi;
}
