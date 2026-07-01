/*
 * Host/fake-service tests for the real MQTT connect state machine
 * (src/services_mqtt.c) driven against the fake esp-mqtt/clock layer.
 *
 * Covers:
 *   #14 - MQTT connect-attempt timeout -> reconnect scheduling.
 *   #15 - first-heartbeat publish failure is attributed to the MQTT stage,
 *         consumes the bootstrap retry budget, and raises bootstrap-failed
 *         once the budget is exhausted (the signal app.c uses to re-enter
 *         Provisioning Mode).
 */
#include "unity.h"

#include "fakes.h"
#include "mqtt_client.h"
#include "runtime_config.h"
#include "services_mqtt.h"

#define DEVICE_ID "1ADD5912AF61"
#define BROKER_URI "mqtt://192.168.1.50"

static void fresh_started_client(void) {
    gh_fake_reset_all();
    gh_mqtt_stop_reset();
    gh_fake_random_set(0); /* deterministic backoff */
    TEST_ASSERT_EQUAL_INT(ESP_OK, gh_mqtt_init(DEVICE_ID, BROKER_URI, NULL));
    TEST_ASSERT_EQUAL_INT(ESP_OK, gh_mqtt_start());
}

static void deliver_connected(void) {
    gh_fake_mqtt_post_event(MQTT_EVENT_CONNECTED, NULL);
}

static void test_tick_is_noop_while_wifi_down(void) {
    fresh_started_client();
    gh_fake_clock_advance_ms(60000);
    gh_mqtt_tick(false); /* wifi not ready */
    TEST_ASSERT_FALSE(gh_mqtt_is_connected());
    TEST_ASSERT_FALSE(gh_mqtt_bootstrap_failed());
    TEST_ASSERT_EQUAL_INT(0, gh_fake_mqtt_reconnect_count());
}

static void test_connect_timeout_schedules_reconnect(void) {
    fresh_started_client();

    /* Just under the 5s connect timeout: still in-progress, no reconnect yet. */
    gh_fake_clock_advance_ms(GH_MQTT_CONNECT_TIMEOUT_MS - 1U);
    gh_mqtt_tick(true);
    TEST_ASSERT_EQUAL_INT(0, gh_fake_mqtt_reconnect_count());

    /* Cross the timeout: the attempt is abandoned and a reconnect is scheduled. */
    gh_fake_clock_advance_ms(1U);
    gh_mqtt_tick(true);
    TEST_ASSERT_FALSE(gh_mqtt_is_connected());
    TEST_ASSERT_FALSE(gh_mqtt_bootstrap_failed());

    /* Advance past the scheduled backoff so the reconnect actually fires. */
    gh_fake_clock_advance_ms(GH_MQTT_RETRY_MAX_MS);
    gh_mqtt_tick(true);
    TEST_ASSERT_TRUE(gh_fake_mqtt_reconnect_count() >= 1);
}

static void test_first_heartbeat_failure_attributed_to_mqtt(void) {
    fresh_started_client();
    deliver_connected();
    TEST_ASSERT_TRUE(gh_mqtt_is_connected());

    /* app.c calls this when the first gh/heartbeat publish fails. */
    gh_mqtt_note_bootstrap_publish_failure();

    /* Attributed to MQTT: connection dropped, reconnect pending, not yet failed. */
    TEST_ASSERT_FALSE(gh_mqtt_is_connected());
    TEST_ASSERT_FALSE(gh_mqtt_bootstrap_failed());
}

static void test_bootstrap_budget_exhausts_and_raises_failed(void) {
    fresh_started_client();
    deliver_connected();

    /* First heartbeat publish fails -> consumes budget, schedules reconnect. */
    gh_mqtt_note_bootstrap_publish_failure();

    /* Drive reconnect attempts that keep timing out until the budget is spent.
     * Each large clock jump lets a scheduled reconnect fire, then time out. */
    for (int i = 0; i < 20 && !gh_mqtt_bootstrap_failed(); ++i) {
        gh_fake_clock_advance_ms(GH_MQTT_RETRY_MAX_MS + GH_MQTT_CONNECT_TIMEOUT_MS);
        gh_mqtt_tick(true);
    }

    TEST_ASSERT_TRUE(gh_mqtt_bootstrap_failed());
    TEST_ASSERT_FALSE(gh_mqtt_is_connected());
}

static void test_reconnect_on_connected_event_resets_budget(void) {
    fresh_started_client();

    /* Time out once so retry_count has advanced. */
    gh_fake_clock_advance_ms(GH_MQTT_CONNECT_TIMEOUT_MS);
    gh_mqtt_tick(true);

    /* A successful connect clears the retry budget and subscribes. */
    deliver_connected();
    TEST_ASSERT_TRUE(gh_mqtt_is_connected());
    TEST_ASSERT_FALSE(gh_mqtt_bootstrap_failed());
    TEST_ASSERT_TRUE(gh_fake_mqtt_subscribe_count() >= 2); /* rd + wr topics */
}

void run_service_mqtt_suite(void) {
    RUN_TEST(test_tick_is_noop_while_wifi_down);
    RUN_TEST(test_connect_timeout_schedules_reconnect);
    RUN_TEST(test_first_heartbeat_failure_attributed_to_mqtt);
    RUN_TEST(test_bootstrap_budget_exhausts_and_raises_failed);
    RUN_TEST(test_reconnect_on_connected_event_resets_budget);
}
