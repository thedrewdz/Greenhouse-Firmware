#ifndef GH_HOST_FAKE_ESP_WIFI_H
#define GH_HOST_FAKE_ESP_WIFI_H

#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

/* WiFi event base + ids used by services_network.c */
extern const char *const WIFI_EVENT_BASE_NAME;
#define WIFI_EVENT (WIFI_EVENT_BASE_NAME)

enum {
    WIFI_EVENT_STA_START = 0,
    WIFI_EVENT_STA_DISCONNECTED = 1,
};

typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA = 1,
} wifi_mode_t;

typedef enum {
    WIFI_IF_STA = 0,
} wifi_interface_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WPA2_PSK = 3,
} wifi_auth_mode_t;

typedef struct {
    int unused;
} wifi_init_config_t;

#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){0})

typedef struct {
    wifi_auth_mode_t authmode;
} wifi_scan_threshold_t;

typedef struct {
    int capable;
    int required;
} wifi_pmf_config_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    wifi_scan_threshold_t threshold;
    wifi_pmf_config_t pmf_cfg;
} wifi_sta_config_t;

typedef struct {
    wifi_sta_config_t sta;
} wifi_config_t;

typedef struct {
    int rssi;
} wifi_ap_record_t;

esp_err_t esp_wifi_init(const wifi_init_config_t *config);
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_disconnect(void);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info);

#endif
