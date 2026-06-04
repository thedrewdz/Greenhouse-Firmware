#include "services_provisioning_config.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "runtime_config.h"

static const char *TAG = "gh_provisioning";
static const char *NVS_NAMESPACE = "gh_prov";
static const char *KEY_WIFI_SSID = "wifi_ssid";
static const char *KEY_WIFI_PASSWORD = "wifi_pass";
static const char *KEY_MQTT_BROKER_URI = "mqtt_uri";
static const char *KEY_HEARTBEAT_MS = "hb_ms";

static bool read_string(nvs_handle_t handle, const char *key, char *dest, size_t dest_len, bool required) {
    size_t len = dest_len;
    esp_err_t err;

    if (dest == NULL || dest_len == 0U) {
        return false;
    }

    dest[0] = '\0';
    err = nvs_get_str(handle, key, dest, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND && !required) {
        return true;
    }

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Provisioning key %s missing or invalid err=0x%x", key, (unsigned int)err);
        return false;
    }

    return dest[0] != '\0' || !required;
}

esp_err_t gh_provisioning_config_init(void) {
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    return err;
}

bool gh_provisioning_config_load(gh_provisioning_config_t *out_config) {
    nvs_handle_t handle;
    esp_err_t err;
    uint32_t heartbeat_interval_ms = GH_HEARTBEAT_INTERVAL_MS;

    if (out_config == NULL) {
        return false;
    }

    (void)memset(out_config, 0, sizeof(*out_config));

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Provisioning config not found in NVS");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open provisioning config err=0x%x", (unsigned int)err);
        return false;
    }

    if (!read_string(handle, KEY_WIFI_SSID, out_config->wifi_ssid, sizeof(out_config->wifi_ssid), true) ||
        !read_string(handle, KEY_WIFI_PASSWORD, out_config->wifi_password, sizeof(out_config->wifi_password), false) ||
        !read_string(handle, KEY_MQTT_BROKER_URI, out_config->mqtt_broker_uri, sizeof(out_config->mqtt_broker_uri), true)) {
        nvs_close(handle);
        return false;
    }

    err = nvs_get_u32(handle, KEY_HEARTBEAT_MS, &heartbeat_interval_ms);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Invalid heartbeat interval in NVS err=0x%x", (unsigned int)err);
        nvs_close(handle);
        return false;
    }

    out_config->heartbeat_interval_ms = heartbeat_interval_ms;
    nvs_close(handle);

    ESP_LOGI(TAG, "Loaded provisioning config from NVS for SSID=%s", out_config->wifi_ssid);
    return true;
}

esp_err_t gh_provisioning_config_save(const gh_provisioning_config_t *config) {
    nvs_handle_t handle;
    esp_err_t err;

    if (config == NULL || config->wifi_ssid[0] == '\0' || config->mqtt_broker_uri[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open provisioning config for write err=0x%x", (unsigned int)err);
        return err;
    }

    err = nvs_set_str(handle, KEY_WIFI_SSID, config->wifi_ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, KEY_WIFI_PASSWORD, config->wifi_password);
    }
    if (err == ESP_OK) {
        err = nvs_set_str(handle, KEY_MQTT_BROKER_URI, config->mqtt_broker_uri);
    }
    if (err == ESP_OK) {
        err = nvs_set_u32(handle, KEY_HEARTBEAT_MS, config->heartbeat_interval_ms);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist provisioning config err=0x%x", (unsigned int)err);
        return err;
    }

    ESP_LOGI(TAG, "Persisted provisioning config for SSID=%s", config->wifi_ssid);
    return ESP_OK;
}
