#ifndef SERVICES_PROVISIONING_CONFIG_H
#define SERVICES_PROVISIONING_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define GH_WIFI_SSID_MAX_LEN 33
#define GH_WIFI_PASSWORD_MAX_LEN 65
#define GH_MQTT_BROKER_URI_MAX_LEN 129

#define GH_PROVISIONING_SCHEMA_VERSION 1
#define GH_PROVISIONING_RESULT_MAX_LEN 8
#define GH_PROVISIONING_ERROR_MESSAGE_MAX_LEN 64

typedef struct {
    char wifi_ssid[GH_WIFI_SSID_MAX_LEN];
    char wifi_password[GH_WIFI_PASSWORD_MAX_LEN];
    char mqtt_broker_uri[GH_MQTT_BROKER_URI_MAX_LEN];
    uint32_t heartbeat_interval_ms;
} gh_provisioning_config_t;

typedef enum {
    GH_PROVISIONING_STATUS_SUCCESS = 0,
    GH_PROVISIONING_STATUS_UNSUPPORTED_SCHEMA_VERSION = 2001,
    GH_PROVISIONING_STATUS_DEVICE_ID_MISMATCH = 2002,
    GH_PROVISIONING_STATUS_WIFI_SSID_EMPTY = 2003,
    GH_PROVISIONING_STATUS_MQTT_BROKER_URI_INVALID = 2004,
    GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR = 2099,
} gh_provisioning_status_code_t;

typedef struct {
    uint32_t schema_version;
    char device_id[13];
    gh_provisioning_config_t config;
    bool has_heartbeat_interval_ms;
} gh_provisioning_payload_t;

typedef struct {
    char result[GH_PROVISIONING_RESULT_MAX_LEN];
    gh_provisioning_status_code_t error_code;
    char error_message[GH_PROVISIONING_ERROR_MESSAGE_MAX_LEN];
} gh_provisioning_status_t;

esp_err_t gh_provisioning_config_init(void);
bool gh_provisioning_config_load(gh_provisioning_config_t *out_config);
esp_err_t gh_provisioning_config_save(const gh_provisioning_config_t *config);

#endif
