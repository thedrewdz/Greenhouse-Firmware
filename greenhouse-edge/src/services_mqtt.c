#include "services_mqtt.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "mqtt_client.h"

#include "runtime_config.h"

static const char *TAG = "gh_mqtt";

static esp_mqtt_client_handle_t s_client;
static gh_mqtt_command_cb_t s_command_cb;
static bool s_started;
static bool s_connected;
static bool s_reconnect_scheduled;
static uint32_t s_retry_count;
static uint64_t s_next_retry_at_ms;
static char s_device_id[13];
static char s_broker_uri[129];

static uint32_t compute_backoff_ms(uint32_t retry_count) {
    uint32_t backoff = GH_MQTT_RETRY_BASE_MS;
    uint32_t i;

    for (i = 0; i < retry_count && backoff < GH_MQTT_RETRY_MAX_MS; ++i) {
        backoff *= 2U;
        if (backoff > GH_MQTT_RETRY_MAX_MS) {
            backoff = GH_MQTT_RETRY_MAX_MS;
        }
    }

    return backoff + (esp_random() % 300U);
}

static void schedule_reconnect(void) {
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);
    const uint32_t wait_ms = compute_backoff_ms(s_retry_count);

    s_next_retry_at_ms = now_ms + wait_ms;
    s_reconnect_scheduled = true;
    s_retry_count++;

    ESP_LOGW(TAG, "MQTT reconnect scheduled in %lu ms (attempt=%lu)", (unsigned long)wait_ms, (unsigned long)s_retry_count);
}

static char *copy_bounded(const char *src, int len) {
    char *dst;

    if (src == NULL || len <= 0) {
        return NULL;
    }

    dst = (char *)malloc((size_t)len + 1U);
    if (dst == NULL) {
        return NULL;
    }

    (void)memcpy(dst, src, (size_t)len);
    dst[len] = '\0';
    return dst;
}

static void subscribe_command_topics(void) {
    char rd_topic[32];
    char wr_topic[32];

    (void)snprintf(rd_topic, sizeof(rd_topic), "ghcmd/rd-%s", s_device_id);
    (void)snprintf(wr_topic, sizeof(wr_topic), "ghcmd/wr-%s", s_device_id);

    (void)esp_mqtt_client_subscribe(s_client, rd_topic, 1);
    (void)esp_mqtt_client_subscribe(s_client, wr_topic, 1);

    ESP_LOGI(TAG, "Subscribed to %s and %s", rd_topic, wr_topic);
}

static void on_mqtt_event(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    (void)handler_args;
    (void)base;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            s_connected = true;
            s_retry_count = 0;
            s_reconnect_scheduled = false;
            subscribe_command_topics();
            ESP_LOGI(TAG, "MQTT connected");
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;
            schedule_reconnect();
            ESP_LOGW(TAG, "MQTT disconnected");
            break;

        case MQTT_EVENT_DATA:
            if (s_command_cb != NULL) {
                char *topic = copy_bounded(event->topic, event->topic_len);
                char *payload = copy_bounded(event->data, event->data_len);

                if (topic != NULL && payload != NULL) {
                    s_command_cb(topic, payload);
                }

                free(topic);
                free(payload);
            }
            break;

        default:
            break;
    }
}

esp_err_t gh_mqtt_init(const char *device_id, const char *broker_uri, gh_mqtt_command_cb_t command_cb) {
    esp_mqtt_client_config_t cfg = {
        .network.disable_auto_reconnect = true,
    };

    if (device_id == NULL || strlen(device_id) >= sizeof(s_device_id)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (broker_uri == NULL || strlen(broker_uri) == 0U) {
        ESP_LOGE(TAG, "Bootstrap MQTT Endpoint missing from provisioning config");
        return ESP_ERR_INVALID_ARG;
    }

    if (strlen(broker_uri) >= sizeof(s_broker_uri)) {
        ESP_LOGE(TAG, "Bootstrap MQTT Endpoint exceeds supported length");
        return ESP_ERR_INVALID_ARG;
    }

    (void)snprintf(s_device_id, sizeof(s_device_id), "%s", device_id);
    (void)snprintf(s_broker_uri, sizeof(s_broker_uri), "%s", broker_uri);
    cfg.broker.address.uri = s_broker_uri;
    s_command_cb = command_cb;

    s_client = esp_mqtt_client_init(&cfg);
    if (s_client == NULL) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT client configured for broker=%s", broker_uri);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, on_mqtt_event, NULL));
    return ESP_OK;
}

esp_err_t gh_mqtt_start(void) {
    if (s_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_started) {
        return ESP_OK;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));
    s_started = true;
    s_reconnect_scheduled = false;
    s_next_retry_at_ms = 0;
    s_retry_count = 0;

    ESP_LOGI(TAG, "MQTT client start requested");
    return ESP_OK;
}

void gh_mqtt_tick(bool wifi_connected) {
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000ULL);

    if (!s_started || s_connected || !s_reconnect_scheduled || !wifi_connected) {
        return;
    }

    if (now_ms >= s_next_retry_at_ms) {
        esp_err_t err = esp_mqtt_client_reconnect(s_client);
        if (err == ESP_OK) {
            s_reconnect_scheduled = false;
            ESP_LOGI(TAG, "MQTT reconnect attempt started");
        } else {
            ESP_LOGW(TAG, "MQTT reconnect call failed err=0x%x", (unsigned int)err);
            schedule_reconnect();
        }
    }
}

bool gh_mqtt_is_connected(void) {
    return s_connected;
}

esp_err_t gh_mqtt_publish(const char *topic, const char *payload, int qos, int retain) {
    int msg_id;

    if (topic == NULL || payload == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_connected || s_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, qos, retain);
    if (msg_id < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}
