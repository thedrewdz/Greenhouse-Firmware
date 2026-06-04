#include "app.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "app_types.h"
#include "codec_json.h"
#include "runtime_config.h"
#include "services_ble_onboarding.h"
#include "services_mqtt.h"
#include "services_network.h"
#include "services_provisioning_config.h"

static const char *TAG = "gh_app";

static char s_device_id[GH_DEVICE_ID_MAX_LEN];
static uint32_t s_message_id = 1U;
static uint64_t s_next_heartbeat_at_us = 0ULL;
static uint64_t s_heartbeat_interval_us = ((uint64_t)GH_HEARTBEAT_INTERVAL_MS * 1000ULL);
static bool s_last_mqtt_connected;
static bool s_first_heartbeat_published;
static bool s_network_initialized;
static bool s_mqtt_started;
static bool s_provisioning_mode;
static bool s_waiting_for_network_logged;

static void on_provisioning_payload(const char *payload, gh_provisioning_status_t *out_status, void *ctx);

static void read_device_id(char *device_id, size_t device_id_len) {
    uint8_t mac[6];

    (void)esp_read_mac(mac, ESP_MAC_WIFI_STA);
    (void)snprintf(
        device_id,
        device_id_len,
        "%02X%02X%02X%02X%02X%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]);
}

static void on_command_received(const char *topic, const char *payload) {
    bool is_write = false;
    gh_command_t cmd;
    gh_error_code_t err = GH_ERR_NONE;

    (void)memset(&cmd, 0, sizeof(cmd));

    if (!gh_codec_parse_command_topic(topic, s_device_id, &is_write)) {
        ESP_LOGW(TAG, "Ignoring message on non-command topic: %s", topic);
        return;
    }

    if (!gh_codec_parse_command_payload(payload, &cmd, &err)) {
        ESP_LOGW(TAG, "Invalid command payload err=%d payload=%s", (int)err, payload);
        return;
    }

    cmd.is_write = is_write;
    ESP_LOGI(
        TAG,
        "Command received id=%" PRIu32 " slot=%d state=%s value=%.2f write=%d",
        cmd.id,
        cmd.slot_id,
        cmd.state,
        cmd.value,
        cmd.is_write ? 1 : 0);
}

static void publish_heartbeat(void) {
    gh_heartbeat_t heartbeat;
    char *payload;
    esp_err_t err;

    (void)memset(&heartbeat, 0, sizeof(heartbeat));
    heartbeat.id = s_message_id++;
    (void)strncpy(heartbeat.device_id, s_device_id, sizeof(heartbeat.device_id) - 1U);
    (void)strncpy(heartbeat.hardware_revision, "A", sizeof(heartbeat.hardware_revision) - 1U);
    (void)strncpy(heartbeat.firmware_version, "0.1.0", sizeof(heartbeat.firmware_version) - 1U);
    heartbeat.uptime_seconds = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    heartbeat.wifi_rssi = gh_network_get_rssi();
    heartbeat.slot_count = 0;

    payload = gh_codec_build_heartbeat_payload(&heartbeat);
    if (payload == NULL) {
        ESP_LOGE(TAG, "Heartbeat encode failed");
        return;
    }

    err = gh_mqtt_publish("gh/heartbeat", payload, 1, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Heartbeat publish failed err=0x%x", (unsigned int)err);
        if (!s_first_heartbeat_published) {
            gh_mqtt_note_bootstrap_publish_failure();
        }
    } else {
        s_first_heartbeat_published = true;
        ESP_LOGI(TAG, "Published heartbeat to gh/heartbeat payload=%s", payload);
    }

    gh_codec_free_payload(payload);
}

static esp_err_t start_network_bootstrap(const gh_provisioning_config_t *provisioning_config) {
    esp_err_t err;

    if (!s_network_initialized) {
        err = gh_network_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Network init failed err=0x%x", (unsigned int)err);
            return err;
        }
        s_network_initialized = true;
    }

    s_heartbeat_interval_us = ((uint64_t)provisioning_config->heartbeat_interval_ms * 1000ULL);
    s_first_heartbeat_published = false;
    s_last_mqtt_connected = false;
    s_mqtt_started = false;
    s_waiting_for_network_logged = false;
    s_next_heartbeat_at_us = esp_timer_get_time();

    err = gh_network_start(provisioning_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Network start failed err=0x%x", (unsigned int)err);
        return err;
    }

    err = gh_mqtt_init(s_device_id, provisioning_config->mqtt_broker_uri, on_command_received);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT init failed err=0x%x", (unsigned int)err);
        return err;
    }

    s_provisioning_mode = false;
    return ESP_OK;
}

static void enter_provisioning_mode(void) {
    esp_err_t err;

    s_provisioning_mode = true;
    ESP_LOGI(TAG, "Entering BLE Provisioning Mode");
    err = gh_ble_onboarding_start(s_device_id, on_provisioning_payload, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "BLE onboarding start failed err=0x%x", (unsigned int)err);
    }
}

static void set_provisioning_status(
    gh_provisioning_status_t *status,
    gh_provisioning_status_code_t code,
    const char *message) {
    (void)memset(status, 0, sizeof(*status));
    if (code == GH_PROVISIONING_STATUS_SUCCESS) {
        (void)strcpy(status->result, "success");
        status->error_code = GH_PROVISIONING_STATUS_SUCCESS;
        return;
    }

    (void)strcpy(status->result, "error");
    status->error_code = code;
    if (message != NULL) {
        (void)snprintf(status->error_message, sizeof(status->error_message), "%s", message);
    }
}

static void on_provisioning_payload(const char *payload, gh_provisioning_status_t *out_status, void *ctx) {
    gh_provisioning_payload_t provisioning_payload;
    esp_err_t err;

    (void)ctx;

    if (!gh_codec_parse_provisioning_payload(payload, s_device_id, &provisioning_payload, out_status)) {
        return;
    }

    err = gh_provisioning_config_save(&provisioning_payload.config);
    if (err != ESP_OK) {
        set_provisioning_status(
            out_status,
            GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR,
            "failed to persist provisioning data");
        return;
    }

    gh_ble_onboarding_stop();

    err = start_network_bootstrap(&provisioning_payload.config);
    if (err != ESP_OK) {
        set_provisioning_status(
            out_status,
            GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR,
            "failed to start network bootstrap");
        enter_provisioning_mode();
        return;
    }

    set_provisioning_status(out_status, GH_PROVISIONING_STATUS_SUCCESS, NULL);
}

void gh_app_init(void) {
    gh_provisioning_config_t provisioning_config;
    esp_err_t err;

    read_device_id(s_device_id, sizeof(s_device_id));
    s_next_heartbeat_at_us = esp_timer_get_time();
    s_heartbeat_interval_us = ((uint64_t)GH_HEARTBEAT_INTERVAL_MS * 1000ULL);
    s_last_mqtt_connected = false;
    s_first_heartbeat_published = false;
    s_network_initialized = false;
    s_mqtt_started = false;
    s_provisioning_mode = false;
    s_waiting_for_network_logged = false;

    ESP_LOGI(TAG, "Greenhouse edge app init, device_id=%s", s_device_id);

    err = gh_provisioning_config_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Provisioning config init failed err=0x%x", (unsigned int)err);
        return;
    }

    if (!gh_provisioning_config_load(&provisioning_config)) {
        ESP_LOGI(TAG, "Provisioning data missing; entering BLE Provisioning Mode");
        enter_provisioning_mode();
        return;
    }

    err = start_network_bootstrap(&provisioning_config);
    if (err != ESP_OK) {
        enter_provisioning_mode();
    }
}

void gh_app_tick(void) {
    const uint64_t now_us = (uint64_t)esp_timer_get_time();
    const bool network_connected = gh_network_is_connected();
    bool mqtt_connected;
    esp_err_t err;

    if (s_provisioning_mode) {
        return;
    }

    gh_network_tick();
    if (!s_first_heartbeat_published && gh_network_bootstrap_failed()) {
        ESP_LOGW(TAG, "WiFi bootstrap retry budget exhausted before first heartbeat");
        enter_provisioning_mode();
        return;
    }

    if (!s_mqtt_started) {
        if (!network_connected) {
            if (!s_waiting_for_network_logged) {
                ESP_LOGI(TAG, "Waiting for WiFi readiness before starting MQTT");
                s_waiting_for_network_logged = true;
            }
        } else {
            err = gh_mqtt_start();
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "MQTT start deferred retry err=0x%x", (unsigned int)err);
            } else {
                s_mqtt_started = true;
                s_waiting_for_network_logged = false;
            }
        }
    }

    gh_mqtt_tick(network_connected);
    if (!s_first_heartbeat_published && gh_mqtt_bootstrap_failed()) {
        ESP_LOGW(TAG, "MQTT bootstrap retry budget exhausted before first heartbeat");
        enter_provisioning_mode();
        return;
    }

    mqtt_connected = gh_mqtt_is_connected();

    if (mqtt_connected && !s_last_mqtt_connected) {
        s_next_heartbeat_at_us = now_us;
    }
    s_last_mqtt_connected = mqtt_connected;

    if (mqtt_connected && now_us >= s_next_heartbeat_at_us) {
        publish_heartbeat();
        s_next_heartbeat_at_us = now_us + s_heartbeat_interval_us;
    }
}
