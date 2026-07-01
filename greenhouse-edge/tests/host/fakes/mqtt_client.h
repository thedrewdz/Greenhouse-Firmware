#ifndef GH_HOST_FAKE_MQTT_CLIENT_H
#define GH_HOST_FAKE_MQTT_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

/* Minimal esp-mqtt surface used by services_mqtt.c, faked for host tests. */

typedef struct gh_fake_mqtt_client *esp_mqtt_client_handle_t;

typedef enum {
    MQTT_EVENT_ANY = -1,
    MQTT_EVENT_ERROR = 0,
    MQTT_EVENT_CONNECTED = 1,
    MQTT_EVENT_DISCONNECTED = 2,
    MQTT_EVENT_SUBSCRIBED = 3,
    MQTT_EVENT_DATA = 6,
} esp_mqtt_event_id_t;

typedef struct {
    char *topic;
    int topic_len;
    char *data;
    int data_len;
} esp_mqtt_event_t;

typedef esp_mqtt_event_t *esp_mqtt_event_handle_t;

typedef struct {
    struct {
        struct {
            const char *uri;
        } address;
    } broker;
    struct {
        bool disable_auto_reconnect;
    } network;
} esp_mqtt_client_config_t;

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t *config);
esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, int32_t event_id,
                                         esp_event_handler_t handler, void *handler_args);
esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_reconnect(esp_mqtt_client_handle_t client);
esp_err_t esp_mqtt_client_disconnect(esp_mqtt_client_handle_t client);
int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos);
int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic,
                            const char *data, int len, int qos, int retain);

#endif
