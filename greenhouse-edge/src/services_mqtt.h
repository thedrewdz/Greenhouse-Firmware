#ifndef SERVICES_MQTT_H
#define SERVICES_MQTT_H

#include <stdbool.h>

#include "esp_err.h"

typedef void (*gh_mqtt_command_cb_t)(const char *topic, const char *payload);

esp_err_t gh_mqtt_init(const char *device_id, const char *broker_uri, gh_mqtt_command_cb_t command_cb);
esp_err_t gh_mqtt_start(void);
void gh_mqtt_tick(bool wifi_connected);
bool gh_mqtt_is_connected(void);
bool gh_mqtt_bootstrap_failed(void);
void gh_mqtt_note_bootstrap_publish_failure(void);
esp_err_t gh_mqtt_publish(const char *topic, const char *payload, int qos, int retain);

#endif
