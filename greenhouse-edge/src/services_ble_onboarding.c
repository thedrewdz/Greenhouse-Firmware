#include "services_ble_onboarding.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec_json.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "os/os_mbuf.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "gh_ble_onboard";
static const char *DEVICE_NAME_PREFIX = "GH-Edge-";
static const uint16_t MAX_PROVISIONING_PAYLOAD_LEN = 512U;

static char s_device_name[32];
static bool s_started;
static bool s_synced;
static bool s_advertising_enabled;
static uint8_t s_own_addr_type;
static uint16_t s_status_value_handle;
static uint16_t s_active_conn_handle;
static gh_ble_onboarding_payload_cb_t s_payload_cb;
static void *s_payload_cb_ctx;
static gh_provisioning_status_t s_last_status = {
    .result = "error",
    .error_code = GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR,
    .error_message = "no provisioning payload received",
};

static const ble_uuid128_t s_onboarding_service_uuid =
    BLE_UUID128_INIT(0x47, 0x48, 0x45, 0x44, 0x47, 0x45, 0x2d, 0x4f, 0x4e, 0x42, 0x4f, 0x41, 0x52, 0x44, 0x01, 0x00);
static const ble_uuid128_t s_provisioning_payload_uuid =
    BLE_UUID128_INIT(0x47, 0x48, 0x45, 0x44, 0x47, 0x45, 0x2d, 0x4f, 0x4e, 0x42, 0x4f, 0x41, 0x52, 0x44, 0x02, 0x00);
static const ble_uuid128_t s_provisioning_status_uuid =
    BLE_UUID128_INIT(0x47, 0x48, 0x45, 0x44, 0x47, 0x45, 0x2d, 0x4f, 0x4e, 0x42, 0x4f, 0x41, 0x52, 0x44, 0x03, 0x00);

static int on_gap_event(struct ble_gap_event *event, void *arg);
static int on_gatt_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def s_gatt_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_onboarding_service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &s_provisioning_payload_uuid.u,
                .access_cb = on_gatt_access,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = &s_provisioning_status_uuid.u,
                .access_cb = on_gatt_access,
                .val_handle = &s_status_value_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            },
            { 0 },
        },
    },
    { 0 },
};

static void set_status(gh_provisioning_status_t *status, gh_provisioning_status_code_t code, const char *message) {
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

static int append_status_response(struct os_mbuf *om) {
    char *json = gh_codec_build_provisioning_status_payload(&s_last_status);
    int rc;

    if (json == NULL) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    rc = os_mbuf_append(om, json, strlen(json));
    gh_codec_free_payload(json);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static void notify_status(void) {
    if (s_active_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_status_value_handle == 0U) {
        return;
    }

    (void)ble_gatts_chr_updated(s_status_value_handle);
    (void)ble_gattc_notify(s_active_conn_handle, s_status_value_handle);
}

static int on_gatt_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    char *payload;
    uint16_t payload_len = 0;
    int rc;

    (void)arg;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return append_status_response(ctxt->om);
    }

    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (attr_handle == s_status_value_handle) {
        return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    }

    if (OS_MBUF_PKTLEN(ctxt->om) == 0 || OS_MBUF_PKTLEN(ctxt->om) > MAX_PROVISIONING_PAYLOAD_LEN) {
        set_status(&s_last_status, GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, "provisioning payload size is invalid");
        notify_status();
        return 0;
    }

    payload = (char *)malloc((size_t)OS_MBUF_PKTLEN(ctxt->om) + 1U);
    if (payload == NULL) {
        set_status(&s_last_status, GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, "provisioning payload allocation failed");
        notify_status();
        return 0;
    }

    rc = ble_hs_mbuf_to_flat(ctxt->om, payload, OS_MBUF_PKTLEN(ctxt->om), &payload_len);
    if (rc != 0) {
        free(payload);
        set_status(&s_last_status, GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, "provisioning payload read failed");
        notify_status();
        return 0;
    }
    payload[payload_len] = '\0';

    if (s_payload_cb == NULL) {
        set_status(&s_last_status, GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, "provisioning handler unavailable");
    } else {
        s_payload_cb(payload, &s_last_status, s_payload_cb_ctx);
    }

    free(payload);
    ESP_LOGI(TAG, "BLE provisioning payload handled result=%s error=%d", s_last_status.result, (int)s_last_status.error_code);
    notify_status();
    if (s_last_status.error_code == GH_PROVISIONING_STATUS_SUCCESS) {
        gh_ble_onboarding_stop();
    }
    return 0;
}

static void start_advertising(void) {
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    struct ble_gap_adv_params adv_params;
    int rc;

    if (!s_advertising_enabled) {
        return;
    }

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
                s_active_conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "BLE onboarding client connected");
            } else {
                s_active_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                ESP_LOGW(TAG, "BLE onboarding connect failed status=%d", event->connect.status);
                start_advertising();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            s_active_conn_handle = BLE_HS_CONN_HANDLE_NONE;
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

    s_synced = true;
    start_advertising();
}

static void ble_host_task(void *param) {
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t gh_ble_onboarding_start(const char *device_id, gh_ble_onboarding_payload_cb_t payload_cb, void *ctx) {
    esp_err_t err;
    int rc;

    if (device_id == NULL || device_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    (void)snprintf(s_device_name, sizeof(s_device_name), "%s%.12s", DEVICE_NAME_PREFIX, device_id);
    s_payload_cb = payload_cb;
    s_payload_cb_ctx = ctx;
    s_active_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    s_advertising_enabled = true;

    if (s_started) {
        if (s_synced) {
            start_advertising();
        }
        return ESP_OK;
    }

    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NimBLE host err=0x%x", (unsigned int)err);
        return err;
    }

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(s_device_name);
    rc = ble_gatts_count_cfg(s_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT config rc=%d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(s_gatt_services);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services rc=%d", rc);
        return ESP_FAIL;
    }

    ble_hs_cfg.sync_cb = on_ble_sync;
    nimble_port_freertos_init(ble_host_task);

    s_started = true;
    return ESP_OK;
}

void gh_ble_onboarding_stop(void) {
    s_advertising_enabled = false;
    if (!s_started) {
        return;
    }

    if (ble_gap_adv_active()) {
        (void)ble_gap_adv_stop();
    }
}
