/*
 * Host fake-NVS tests for the real provisioning-config persistence
 * (src/services_provisioning_config.c) against the in-memory fake NVS.
 *
 * Covers #16: A/B candidate-write-then-promote, and that a failed candidate
 * write or a failed promotion leaves the previously active slot intact
 * (last-known-valid / power-loss retention).
 */
#include <string.h>

#include "unity.h"

#include "fakes.h"
#include "services_provisioning_config.h"

#define META_NS "gh_prov_meta"
#define SLOT_A_NS "gh_prov_a"
#define SLOT_B_NS "gh_prov_b"

static gh_provisioning_config_t make_config(const char *ssid, const char *uri, uint32_t hb) {
    gh_provisioning_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    strcpy(cfg.wifi_ssid, ssid);
    strcpy(cfg.wifi_password, "pw");
    strcpy(cfg.mqtt_broker_uri, uri);
    cfg.heartbeat_interval_ms = hb;
    return cfg;
}

static esp_err_t save_config(const char *ssid, const char *uri, uint32_t hb) {
    gh_provisioning_config_t cfg = make_config(ssid, uri, hb);
    return gh_provisioning_config_save(&cfg);
}

static void fresh_nvs(void) {
    gh_fake_reset_all();
    TEST_ASSERT_EQUAL_INT(ESP_OK, gh_provisioning_config_init());
}

static void test_load_fails_on_fresh_device(void) {
    gh_provisioning_config_t out;
    fresh_nvs();
    TEST_ASSERT_FALSE(gh_provisioning_config_load(&out));
}

static void test_save_then_load_roundtrip(void) {
    gh_provisioning_config_t out;
    fresh_nvs();

    TEST_ASSERT_EQUAL_INT(ESP_OK, save_config("Net1", "mqtt://h1", 30000));
    TEST_ASSERT_TRUE(gh_provisioning_config_load(&out));
    TEST_ASSERT_EQUAL_STRING("Net1", out.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("mqtt://h1", out.mqtt_broker_uri);
    TEST_ASSERT_EQUAL_UINT32(30000U, out.heartbeat_interval_ms);
}

static void test_second_save_alternates_slot_and_wins(void) {
    gh_provisioning_config_t out;
    fresh_nvs();

    /* First save lands in slot B (candidate of default active A) and is promoted. */
    TEST_ASSERT_EQUAL_INT(ESP_OK, save_config("Net1", "mqtt://h1", 30000));
    /* Second save must go to the *other* slot and become the active one. */
    TEST_ASSERT_EQUAL_INT(ESP_OK, save_config("Net2", "mqtt://h2", 45000));

    TEST_ASSERT_TRUE(gh_provisioning_config_load(&out));
    TEST_ASSERT_EQUAL_STRING("Net2", out.wifi_ssid);
    TEST_ASSERT_EQUAL_UINT32(45000U, out.heartbeat_interval_ms);
}

static void test_failed_candidate_write_retains_previous(void) {
    gh_provisioning_config_t out;
    fresh_nvs();

    /* Establish a known-good active config (lands in slot B). */
    TEST_ASSERT_EQUAL_INT(ESP_OK, save_config("Good", "mqtt://good", 30000));

    /* Next candidate is slot A; force its writes to fail. */
    gh_fake_nvs_fail_writes(SLOT_A_NS, ESP_FAIL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, save_config("Bad", "mqtt://bad", 60000));
    gh_fake_nvs_clear_failures();

    /* The previously active slot must be untouched. */
    TEST_ASSERT_TRUE(gh_provisioning_config_load(&out));
    TEST_ASSERT_EQUAL_STRING("Good", out.wifi_ssid);
    TEST_ASSERT_EQUAL_UINT32(30000U, out.heartbeat_interval_ms);
}

static void test_failed_promotion_retains_previous(void) {
    gh_provisioning_config_t out;
    fresh_nvs();

    /* Known-good active config (slot B). */
    TEST_ASSERT_EQUAL_INT(ESP_OK, save_config("Good", "mqtt://good", 30000));

    /* Allow the candidate (slot A) write to succeed but fail the meta promotion. */
    gh_fake_nvs_fail_writes(META_NS, ESP_FAIL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, save_config("Bad", "mqtt://bad", 60000));
    gh_fake_nvs_clear_failures();

    /* Active pointer never advanced -> previous config still loads. */
    TEST_ASSERT_TRUE(gh_provisioning_config_load(&out));
    TEST_ASSERT_EQUAL_STRING("Good", out.wifi_ssid);
    TEST_ASSERT_EQUAL_UINT32(30000U, out.heartbeat_interval_ms);
}

static void test_save_rejects_invalid_config(void) {
    fresh_nvs();
    /* Empty SSID / empty broker are rejected before touching NVS. */
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG, save_config("", "mqtt://h", 30000));
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG, save_config("Net", "", 30000));
}

void run_service_provisioning_nvs_suite(void) {
    RUN_TEST(test_load_fails_on_fresh_device);
    RUN_TEST(test_save_then_load_roundtrip);
    RUN_TEST(test_second_save_alternates_slot_and_wins);
    RUN_TEST(test_failed_candidate_write_retains_previous);
    RUN_TEST(test_failed_promotion_retains_previous);
    RUN_TEST(test_save_rejects_invalid_config);
}
