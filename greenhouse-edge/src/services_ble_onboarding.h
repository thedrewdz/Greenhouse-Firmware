#ifndef SERVICES_BLE_ONBOARDING_H
#define SERVICES_BLE_ONBOARDING_H

#include "esp_err.h"
#include "services_provisioning_config.h"

typedef void (*gh_ble_onboarding_payload_cb_t)(
    const char *payload,
    gh_provisioning_status_t *out_status,
    void *ctx);

esp_err_t gh_ble_onboarding_start(const char *device_id, gh_ble_onboarding_payload_cb_t payload_cb, void *ctx);
void gh_ble_onboarding_stop(void);

#endif
