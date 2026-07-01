#ifndef GH_HOST_FAKES_H
#define GH_HOST_FAKES_H

#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"
#include "mqtt_client.h"

/*
 * Test-control surface for the fake ESP-IDF layer (fake_esp_idf.c).
 * Call gh_fake_reset_all() in each test's setUp to get a clean slate.
 */

void gh_fake_reset_all(void);

/* --- Fake clock (esp_timer) --- */
void gh_fake_clock_set_ms(uint64_t ms);
void gh_fake_clock_advance_ms(uint64_t ms);

/* --- Fake randomness (esp_random) --- */
void gh_fake_random_set(uint32_t value);

/* --- Fake event loop (esp_event) --- */
/* Dispatch an event to registered handlers (matches base + id/ANY). */
void gh_fake_event_post(esp_event_base_t base, int32_t event_id, void *event_data);

/* --- Fake WiFi (esp_wifi) --- */
void gh_fake_wifi_set_connect_result(esp_err_t result);
void gh_fake_wifi_set_rssi(int rssi);
int gh_fake_wifi_connect_count(void);

/* --- Fake MQTT (mqtt_client) --- */
/* Deliver an MQTT event to the client's registered handler. */
void gh_fake_mqtt_post_event(int32_t event_id, esp_mqtt_event_t *event);
void gh_fake_mqtt_set_reconnect_result(esp_err_t result);
void gh_fake_mqtt_set_publish_result(int msg_id);
int gh_fake_mqtt_reconnect_count(void);
int gh_fake_mqtt_publish_count(void);
int gh_fake_mqtt_subscribe_count(void);

/* --- Fake NVS --- */
/* After this call, set/commit operations on `namespace_name` return `err`. */
void gh_fake_nvs_fail_writes(const char *namespace_name, esp_err_t err);
void gh_fake_nvs_clear_failures(void);

#endif
