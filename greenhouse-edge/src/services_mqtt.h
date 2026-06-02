#ifndef SERVICES_MQTT_H
#define SERVICES_MQTT_H

#include <stdbool.h>

#include "esp_err.h"

typedef void (*gh_mqtt_command_cb_t)(const char *topic, const char *payload);

esp_err_t gh_mqtt_init(const char *device_id, gh_mqtt_command_cb_t command_cb);
esp_err_t gh_mqtt_start(void);
void gh_mqtt_tick(bool wifi_connected);
bool gh_mqtt_is_connected(void);
esp_err_t gh_mqtt_publish(const char *topic, const char *payload, int qos, int retain);

#endif
