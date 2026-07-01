/*
 * Compiled tests for the pure retry-backoff math shared by the WiFi and MQTT
 * connect state machines (src/retry_backoff.c). Verifies the canonical
 * exponential schedule, ceiling saturation, and 20% symmetric jitter bounds.
 */
#include "unity.h"

#include "retry_backoff.h"
#include "runtime_config.h"

static void test_wifi_schedule_matches_canonical(void) {
    /* compute_backoff_ms(completed_failures): 0->base, doubling each step, cap at max. */
    TEST_ASSERT_EQUAL_UINT32(1000U, gh_retry_backoff_base_ms(GH_WIFI_RETRY_BASE_MS, GH_WIFI_RETRY_MAX_MS, 0));
    TEST_ASSERT_EQUAL_UINT32(2000U, gh_retry_backoff_base_ms(GH_WIFI_RETRY_BASE_MS, GH_WIFI_RETRY_MAX_MS, 1));
    TEST_ASSERT_EQUAL_UINT32(4000U, gh_retry_backoff_base_ms(GH_WIFI_RETRY_BASE_MS, GH_WIFI_RETRY_MAX_MS, 2));
    TEST_ASSERT_EQUAL_UINT32(8000U, gh_retry_backoff_base_ms(GH_WIFI_RETRY_BASE_MS, GH_WIFI_RETRY_MAX_MS, 3));
}

static void test_mqtt_schedule_matches_canonical(void) {
    TEST_ASSERT_EQUAL_UINT32(1000U, gh_retry_backoff_base_ms(GH_MQTT_RETRY_BASE_MS, GH_MQTT_RETRY_MAX_MS, 0));
    TEST_ASSERT_EQUAL_UINT32(2000U, gh_retry_backoff_base_ms(GH_MQTT_RETRY_BASE_MS, GH_MQTT_RETRY_MAX_MS, 1));
    TEST_ASSERT_EQUAL_UINT32(4000U, gh_retry_backoff_base_ms(GH_MQTT_RETRY_BASE_MS, GH_MQTT_RETRY_MAX_MS, 2));
    TEST_ASSERT_EQUAL_UINT32(8000U, gh_retry_backoff_base_ms(GH_MQTT_RETRY_BASE_MS, GH_MQTT_RETRY_MAX_MS, 3));
}

static void test_backoff_saturates_at_ceiling(void) {
    /* 1000 << 5 == 32000 > 30000, so it clamps; and stays clamped for larger counts. */
    TEST_ASSERT_EQUAL_UINT32(30000U, gh_retry_backoff_base_ms(1000U, 30000U, 5));
    TEST_ASSERT_EQUAL_UINT32(30000U, gh_retry_backoff_base_ms(1000U, 30000U, 100));
}

static void test_jitter_window_is_twenty_percent(void) {
    TEST_ASSERT_EQUAL_UINT32(200U, gh_retry_backoff_jitter_window_ms(1000U));
    TEST_ASSERT_EQUAL_UINT32(400U, gh_retry_backoff_jitter_window_ms(2000U));
    TEST_ASSERT_EQUAL_UINT32(1600U, gh_retry_backoff_jitter_window_ms(8000U));
}

static void test_apply_jitter_extremes_and_center(void) {
    /* window(1000) == 200; result = 1000 + (rand % 401) - 200. */
    TEST_ASSERT_EQUAL_UINT32(800U, gh_retry_backoff_apply_jitter(1000U, 0U));      /* min */
    TEST_ASSERT_EQUAL_UINT32(1000U, gh_retry_backoff_apply_jitter(1000U, 200U));   /* center */
    TEST_ASSERT_EQUAL_UINT32(1200U, gh_retry_backoff_apply_jitter(1000U, 400U));   /* max */
    /* modulo wraps: rand 401 behaves like rand 0 */
    TEST_ASSERT_EQUAL_UINT32(800U, gh_retry_backoff_apply_jitter(1000U, 401U));
}

static void test_apply_jitter_stays_within_bounds(void) {
    const uint32_t backoff = 4000U;
    const uint32_t window = gh_retry_backoff_jitter_window_ms(backoff);
    for (uint32_t r = 0; r < 5000U; r += 137U) {
        uint32_t result = gh_retry_backoff_apply_jitter(backoff, r);
        TEST_ASSERT_TRUE(result >= backoff - window);
        TEST_ASSERT_TRUE(result <= backoff + window);
    }
}

void run_retry_backoff_suite(void) {
    RUN_TEST(test_wifi_schedule_matches_canonical);
    RUN_TEST(test_mqtt_schedule_matches_canonical);
    RUN_TEST(test_backoff_saturates_at_ceiling);
    RUN_TEST(test_jitter_window_is_twenty_percent);
    RUN_TEST(test_apply_jitter_extremes_and_center);
    RUN_TEST(test_apply_jitter_stays_within_bounds);
}
