#include "services_ble_onboarding.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "gh_ble_onboard";
static const char *DEVICE_NAME_PREFIX = "GH-Edge-";

static char s_device_name[32];
static bool s_started;
static uint8_t s_own_addr_type;

static const ble_uuid128_t s_onboarding_service_uuid =
    BLE_UUID128_INIT(0x47, 0x48, 0x45, 0x44, 0x47, 0x45, 0x2d, 0x4f, 0x4e, 0x42, 0x4f, 0x41, 0x52, 0x44, 0x01, 0x00);

static int on_gap_event(struct ble_gap_event *event, void *arg);

static void start_advertising(void) {
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    struct ble_gap_adv_params adv_params;
    int rc;

    (void)memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = &s_onboarding_service_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set BLE advertising fields rc=%d", rc);
        return;
    }

    (void)memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.name = (const uint8_t *)s_device_name;
    rsp_fields.name_len = (uint8_t)strlen(s_device_name);
    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set BLE scan response fields rc=%d", rc);
        return;
    }

    (void)memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, on_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start BLE advertising rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "BLE Provisioning Mode advertising as %s", s_device_name);
}

static int on_gap_event(struct ble_gap_event *event, void *arg) {
    (void)arg;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "BLE onboarding client connected");
            } else {
                ESP_LOGW(TAG, "BLE onboarding connect failed status=%d", event->connect.status);
                start_advertising();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE onboarding client disconnected");
            start_advertising();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            start_advertising();
            break;

        default:
            break;
    }

    return 0;
}

static void on_ble_sync(void) {
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);

    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to infer BLE address rc=%d", rc);
        return;
    }

    start_advertising();
}

static void ble_host_task(void *param) {
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t gh_ble_onboarding_start(const char *device_id) {
    esp_err_t err;

    if (device_id == NULL || device_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_started) {
        return ESP_OK;
    }

    (void)snprintf(s_device_name, sizeof(s_device_name), "%s%.12s", DEVICE_NAME_PREFIX, device_id);

    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NimBLE host err=0x%x", (unsigned int)err);
        return err;
    }

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(s_device_name);

    ble_hs_cfg.sync_cb = on_ble_sync;
    nimble_port_freertos_init(ble_host_task);

    s_started = true;
    return ESP_OK;
}
