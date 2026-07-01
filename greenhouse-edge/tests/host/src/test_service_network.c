/*
 * Host/fake-service tests for the real WiFi connect state machine
 * (src/services_network.c) driven against the fake esp_wifi/esp_event/clock
 * layer.
 *
 * Covers #14 (WiFi side): connect-attempt timeout -> reconnect scheduling, and
 * bootstrap retry-budget exhaustion -> bootstrap-failed (the signal app.c uses
 * to re-enter Provisioning Mode).
 */
#include <string.h>

#include "unity.h"

#include "esp_netif.h"
#include "esp_wifi.h"
#include "fakes.h"
#include "runtime_config.h"
#include "services_network.h"
#include "services_provisioning_config.h"

static gh_provisioning_config_t make_config(void) {
    gh_provisioning_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strcpy(cfg.wifi_ssid, "ExampleWiFi");
    strcpy(cfg.wifi_password, "ExamplePassword");
    strcpy(cfg.mqtt_broker_uri, "mqtt://192.168.1.50");
    cfg.heartbeat_interval_ms = GH_HEARTBEAT_INTERVAL_MS;
    return cfg;
}

static void fresh_started_network(void) {
    gh_provisioning_config_t cfg = make_config();
    gh_fake_reset_all();
    gh_network_stop_reset();
    gh_fake_random_set(0); /* deterministic backoff */
    TEST_ASSERT_EQUAL_INT(ESP_OK, gh_network_init());
    TEST_ASSERT_EQUAL_INT(ESP_OK, gh_network_start(&cfg));
    /* The driver emits STA_START once esp_wifi_start() completes. */
    gh_fake_event_post(WIFI_EVENT, WIFI_EVENT_STA_START, NULL);
}

static void test_got_ip_marks_connected_and_reports_rssi(void) {
    fresh_started_network();
    TEST_ASSERT_FALSE(gh_network_is_connected());

    gh_fake_wifi_set_rssi(-42);
    gh_fake_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP, NULL);

    TEST_ASSERT_TRUE(gh_network_is_connected());
    TEST_ASSERT_EQUAL_INT(-42, gh_network_get_rssi());
}

static void test_connect_attempt_starts_on_tick(void) {
    fresh_started_network();
    /* Scheduled immediately after STA_START; first tick issues the connect. */
    gh_network_tick();
    TEST_ASSERT_EQUAL_INT(1, gh_fake_wifi_connect_count());
    TEST_ASSERT_FALSE(gh_network_is_connected());
}

static void test_connect_timeout_schedules_retry(void) {
    fresh_started_network();
    gh_network_tick(); /* connect attempt #1 starts at t=0 */
    TEST_ASSERT_EQUAL_INT(1, gh_fake_wifi_connect_count());

    /* Just under timeout: still in progress. */
    gh_fake_clock_advance_ms(GH_WIFI_CONNECT_TIMEOUT_MS - 1U);
    gh_network_tick();
    TEST_ASSERT_EQUAL_INT(1, gh_fake_wifi_connect_count());

    /* Cross the timeout: attempt abandoned, retry scheduled (not failed yet). */
    gh_fake_clock_advance_ms(1U);
    gh_network_tick();
    TEST_ASSERT_FALSE(gh_network_bootstrap_failed());

    /* Advance past backoff so the next connect fires. */
    gh_fake_clock_advance_ms(GH_WIFI_RETRY_MAX_MS);
    gh_network_tick();
    TEST_ASSERT_TRUE(gh_fake_wifi_connect_count() >= 2);
}

static void test_bootstrap_budget_exhausts_and_raises_failed(void) {
    fresh_started_network();

    for (int i = 0; i < 40 && !gh_network_bootstrap_failed(); ++i) {
        gh_fake_clock_advance_ms(GH_WIFI_RETRY_MAX_MS + GH_WIFI_CONNECT_TIMEOUT_MS);
        gh_network_tick();
    }

    TEST_ASSERT_TRUE(gh_network_bootstrap_failed());
    TEST_ASSERT_FALSE(gh_network_is_connected());
}

void run_service_network_suite(void) {
    RUN_TEST(test_got_ip_marks_connected_and_reports_rssi);
    RUN_TEST(test_connect_attempt_starts_on_tick);
    RUN_TEST(test_connect_timeout_schedules_retry);
    RUN_TEST(test_bootstrap_budget_exhausts_and_raises_failed);
}
