#include "services_provisioning_config.h"

#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "runtime_config.h"

static const char *TAG = "gh_provisioning";
static const char *NVS_LEGACY_NAMESPACE = "gh_prov";
static const char *NVS_META_NAMESPACE = "gh_prov_meta";
static const char *NVS_SLOT_A_NAMESPACE = "gh_prov_a";
static const char *NVS_SLOT_B_NAMESPACE = "gh_prov_b";
static const char *KEY_ACTIVE_SLOT = "active_slot";
static const char *KEY_WIFI_SSID = "wifi_ssid";
static const char *KEY_WIFI_PASSWORD = "wifi_pass";
static const char *KEY_MQTT_BROKER_URI = "mqtt_uri";
static const char *KEY_HEARTBEAT_MS = "hb_ms";

typedef enum {
    GH_PROVISIONING_SLOT_A = 0,
    GH_PROVISIONING_SLOT_B = 1,
} gh_provisioning_slot_t;

static const char *slot_namespace(gh_provisioning_slot_t slot) {
    return (slot == GH_PROVISIONING_SLOT_A) ? NVS_SLOT_A_NAMESPACE : NVS_SLOT_B_NAMESPACE;
}

static gh_provisioning_slot_t inactive_slot(gh_provisioning_slot_t active_slot) {
    return (active_slot == GH_PROVISIONING_SLOT_A) ? GH_PROVISIONING_SLOT_B : GH_PROVISIONING_SLOT_A;
}

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

static bool load_from_namespace(const char *namespace_name, gh_provisioning_config_t *out_config) {
    nvs_handle_t handle;
    esp_err_t err;
    uint32_t heartbeat_interval_ms = GH_HEARTBEAT_INTERVAL_MS;

    err = nvs_open(namespace_name, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open provisioning namespace %s err=0x%x", namespace_name, (unsigned int)err);
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
        ESP_LOGW(TAG, "Invalid heartbeat interval in namespace %s err=0x%x", namespace_name, (unsigned int)err);
        nvs_close(handle);
        return false;
    }

    out_config->heartbeat_interval_ms = heartbeat_interval_ms;
    nvs_close(handle);
    return true;
}

static bool load_active_slot(gh_provisioning_slot_t *out_active_slot) {
    nvs_handle_t handle;
    esp_err_t err;
    uint8_t active_slot = GH_PROVISIONING_SLOT_A;

    err = nvs_open(NVS_META_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open provisioning metadata err=0x%x", (unsigned int)err);
        return false;
    }

    err = nvs_get_u8(handle, KEY_ACTIVE_SLOT, &active_slot);
    nvs_close(handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Provisioning active slot missing or invalid err=0x%x", (unsigned int)err);
        return false;
    }

    if (active_slot != GH_PROVISIONING_SLOT_A && active_slot != GH_PROVISIONING_SLOT_B) {
        ESP_LOGW(TAG, "Provisioning active slot value is out of range: %u", (unsigned int)active_slot);
        return false;
    }

    *out_active_slot = (gh_provisioning_slot_t)active_slot;
    return true;
}

static esp_err_t save_to_namespace(const char *namespace_name, const gh_provisioning_config_t *config) {
    nvs_handle_t handle;
    esp_err_t err;

    err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open provisioning namespace %s for write err=0x%x", namespace_name, (unsigned int)err);
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
    return err;
}

static esp_err_t promote_active_slot(gh_provisioning_slot_t active_slot) {
    nvs_handle_t handle;
    esp_err_t err;

    err = nvs_open(NVS_META_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open provisioning metadata for write err=0x%x", (unsigned int)err);
        return err;
    }

    err = nvs_set_u8(handle, KEY_ACTIVE_SLOT, (uint8_t)active_slot);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    return err;
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
    gh_provisioning_slot_t active_slot;

    if (out_config == NULL) {
        return false;
    }

    (void)memset(out_config, 0, sizeof(*out_config));

    if (load_active_slot(&active_slot)) {
        if (load_from_namespace(slot_namespace(active_slot), out_config)) {
            ESP_LOGI(TAG, "Loaded provisioning config from NVS slot %u for SSID=%s", (unsigned int)active_slot, out_config->wifi_ssid);
            return true;
        }

        ESP_LOGW(TAG, "Active provisioning slot %u is unreadable", (unsigned int)active_slot);
    }

    if (load_from_namespace(NVS_LEGACY_NAMESPACE, out_config)) {
        ESP_LOGI(TAG, "Loaded legacy provisioning config from NVS for SSID=%s", out_config->wifi_ssid);
        return true;
    }

    ESP_LOGI(TAG, "Provisioning config not found in NVS");
    return false;
}

esp_err_t gh_provisioning_config_save(const gh_provisioning_config_t *config) {
    gh_provisioning_slot_t active_slot = GH_PROVISIONING_SLOT_A;
    gh_provisioning_slot_t candidate_slot;
    esp_err_t err;

    if (config == NULL || config->wifi_ssid[0] == '\0' || config->mqtt_broker_uri[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    (void)load_active_slot(&active_slot);
    candidate_slot = inactive_slot(active_slot);

    err = save_to_namespace(slot_namespace(candidate_slot), config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist provisioning candidate slot %u err=0x%x", (unsigned int)candidate_slot, (unsigned int)err);
        return err;
    }

    err = promote_active_slot(candidate_slot);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to promote provisioning candidate slot %u err=0x%x", (unsigned int)candidate_slot, (unsigned int)err);
        return err;
    }

    ESP_LOGI(TAG, "Persisted provisioning config to slot %u for SSID=%s", (unsigned int)candidate_slot, config->wifi_ssid);
    return ESP_OK;
}
