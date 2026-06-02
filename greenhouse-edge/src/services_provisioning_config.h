#ifndef SERVICES_PROVISIONING_CONFIG_H
#define SERVICES_PROVISIONING_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define GH_WIFI_SSID_MAX_LEN 33
#define GH_WIFI_PASSWORD_MAX_LEN 65
#define GH_MQTT_BROKER_URI_MAX_LEN 129

typedef struct {
    char wifi_ssid[GH_WIFI_SSID_MAX_LEN];
    char wifi_password[GH_WIFI_PASSWORD_MAX_LEN];
    char mqtt_broker_uri[GH_MQTT_BROKER_URI_MAX_LEN];
    uint32_t heartbeat_interval_ms;
} gh_provisioning_config_t;

esp_err_t gh_provisioning_config_init(void);
bool gh_provisioning_config_load(gh_provisioning_config_t *out_config);

#endif
