/*
 * Compiled contract tests for gh_codec_parse_provisioning_payload.
 *
 * Replaces the previous Python reimplementation: these call the real firmware
 * parser and assert on its behaviour and canonical 200x status codes. Positive
 * and negative cases are table-driven.
 */
#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "codec_json.h"
#include "runtime_config.h"
#include "services_provisioning_config.h"

#define LOCAL_DEVICE_ID "1ADD5912AF61"

/* Build a provisioning payload with optional field overrides.
 * Passing NULL for a string omits that field; pass "" to include it empty. */
static void build_payload(char *buf, size_t buf_len,
                          const char *schema_version_json,
                          const char *device_id,
                          const char *wifi_ssid_json,
                          const char *wifi_password_json,
                          const char *mqtt_uri_json,
                          const char *heartbeat_json) {
    char parts[512];
    parts[0] = '\0';
    size_t n = 0;

    n += (size_t)snprintf(parts + n, sizeof(parts) - n, "{");
    if (schema_version_json) n += (size_t)snprintf(parts + n, sizeof(parts) - n, "\"schema_version\":%s,", schema_version_json);
    if (device_id) n += (size_t)snprintf(parts + n, sizeof(parts) - n, "\"device_id\":\"%s\",", device_id);
    if (wifi_ssid_json) n += (size_t)snprintf(parts + n, sizeof(parts) - n, "\"wifi_ssid\":%s,", wifi_ssid_json);
    if (wifi_password_json) n += (size_t)snprintf(parts + n, sizeof(parts) - n, "\"wifi_password\":%s,", wifi_password_json);
    if (mqtt_uri_json) n += (size_t)snprintf(parts + n, sizeof(parts) - n, "\"mqtt_broker_uri\":%s,", mqtt_uri_json);
    if (heartbeat_json) n += (size_t)snprintf(parts + n, sizeof(parts) - n, "\"heartbeat_interval_ms\":%s,", heartbeat_json);
    /* trim trailing comma if present */
    if (n > 1 && parts[n - 1] == ',') { parts[n - 1] = '\0'; n--; }
    (void)snprintf(parts + n, sizeof(parts) - n, "}");

    (void)snprintf(buf, buf_len, "%s", parts);
}

static void test_valid_payload_defaults_heartbeat(void) {
    char payload[512];
    build_payload(payload, sizeof(payload), "1", LOCAL_DEVICE_ID,
                  "\"ExampleWiFi\"", "\"ExamplePassword\"", "\"mqtt://192.168.1.50\"", NULL);

    gh_provisioning_payload_t out;
    gh_provisioning_status_t status;
    bool ok = gh_codec_parse_provisioning_payload(payload, LOCAL_DEVICE_ID, &out, &status);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(GH_PROVISIONING_STATUS_SUCCESS, status.error_code);
    TEST_ASSERT_EQUAL_STRING("success", status.result);
    TEST_ASSERT_EQUAL_STRING("ExampleWiFi", out.config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("ExamplePassword", out.config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("mqtt://192.168.1.50", out.config.mqtt_broker_uri);
    TEST_ASSERT_FALSE(out.has_heartbeat_interval_ms);
    TEST_ASSERT_EQUAL_UINT32(GH_HEARTBEAT_INTERVAL_MS, out.config.heartbeat_interval_ms);
}

static void test_valid_payload_empty_password_and_explicit_heartbeat(void) {
    char payload[512];
    build_payload(payload, sizeof(payload), "1", LOCAL_DEVICE_ID,
                  "\"ExampleWiFi\"", "\"\"", "\"mqtts://broker.example\"", "45000");

    gh_provisioning_payload_t out;
    gh_provisioning_status_t status;
    bool ok = gh_codec_parse_provisioning_payload(payload, LOCAL_DEVICE_ID, &out, &status);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(GH_PROVISIONING_STATUS_SUCCESS, status.error_code);
    TEST_ASSERT_EQUAL_STRING("", out.config.wifi_password);
    TEST_ASSERT_TRUE(out.has_heartbeat_interval_ms);
    TEST_ASSERT_EQUAL_UINT32(45000U, out.config.heartbeat_interval_ms);
}

static void test_mqtts_scheme_accepted(void) {
    char payload[512];
    build_payload(payload, sizeof(payload), "1", LOCAL_DEVICE_ID,
                  "\"ssid\"", "\"pw\"", "\"mqtts://host:8883\"", NULL);

    gh_provisioning_payload_t out;
    gh_provisioning_status_t status;
    TEST_ASSERT_TRUE(gh_codec_parse_provisioning_payload(payload, LOCAL_DEVICE_ID, &out, &status));
    TEST_ASSERT_EQUAL_INT(GH_PROVISIONING_STATUS_SUCCESS, status.error_code);
}

typedef struct {
    const char *name;
    const char *schema_version_json;
    const char *device_id;
    const char *wifi_ssid_json;
    const char *wifi_password_json;
    const char *mqtt_uri_json;
    const char *heartbeat_json;
    gh_provisioning_status_code_t expected;
} reject_case_t;

static void test_invalid_payloads_map_to_canonical_codes(void) {
    static const reject_case_t cases[] = {
        {"unsupported schema", "2", LOCAL_DEVICE_ID, "\"s\"", "\"p\"", "\"mqtt://h\"", NULL,
         GH_PROVISIONING_STATUS_UNSUPPORTED_SCHEMA_VERSION},
        {"missing schema", NULL, LOCAL_DEVICE_ID, "\"s\"", "\"p\"", "\"mqtt://h\"", NULL,
         GH_PROVISIONING_STATUS_UNSUPPORTED_SCHEMA_VERSION},
        {"device mismatch", "1", "FFFFFFFFFFFF", "\"s\"", "\"p\"", "\"mqtt://h\"", NULL,
         GH_PROVISIONING_STATUS_DEVICE_ID_MISMATCH},
        {"empty ssid", "1", LOCAL_DEVICE_ID, "\"\"", "\"p\"", "\"mqtt://h\"", NULL,
         GH_PROVISIONING_STATUS_WIFI_SSID_EMPTY},
        {"missing ssid", "1", LOCAL_DEVICE_ID, NULL, "\"p\"", "\"mqtt://h\"", NULL,
         GH_PROVISIONING_STATUS_WIFI_SSID_EMPTY},
        {"missing password", "1", LOCAL_DEVICE_ID, "\"s\"", NULL, "\"mqtt://h\"", NULL,
         GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR},
        {"missing mqtt uri", "1", LOCAL_DEVICE_ID, "\"s\"", "\"p\"", NULL, NULL,
         GH_PROVISIONING_STATUS_MQTT_BROKER_URI_INVALID},
        {"malformed mqtt uri", "1", LOCAL_DEVICE_ID, "\"s\"", "\"p\"", "\"http://h\"", NULL,
         GH_PROVISIONING_STATUS_MQTT_BROKER_URI_INVALID},
        {"mqtt uri no host", "1", LOCAL_DEVICE_ID, "\"s\"", "\"p\"", "\"mqtt:///\"", NULL,
         GH_PROVISIONING_STATUS_MQTT_BROKER_URI_INVALID},
        {"negative heartbeat", "1", LOCAL_DEVICE_ID, "\"s\"", "\"p\"", "\"mqtt://h\"", "-1",
         GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const reject_case_t *c = &cases[i];
        char payload[512];
        build_payload(payload, sizeof(payload), c->schema_version_json, c->device_id,
                      c->wifi_ssid_json, c->wifi_password_json, c->mqtt_uri_json, c->heartbeat_json);

        gh_provisioning_payload_t out;
        gh_provisioning_status_t status;
        bool ok = gh_codec_parse_provisioning_payload(payload, LOCAL_DEVICE_ID, &out, &status);

        UNITY_TEST_ASSERT(!ok, __LINE__, c->name);
        UNITY_TEST_ASSERT_EQUAL_INT(c->expected, status.error_code, __LINE__, c->name);
        UNITY_TEST_ASSERT_EQUAL_STRING("error", status.result, __LINE__, c->name);
    }
}

static void test_non_json_rejected(void) {
    gh_provisioning_payload_t out;
    gh_provisioning_status_t status;
    bool ok = gh_codec_parse_provisioning_payload("not json{", LOCAL_DEVICE_ID, &out, &status);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(GH_PROVISIONING_STATUS_UNSUPPORTED_SCHEMA_VERSION, status.error_code);
}

static void test_oversized_ssid_rejected(void) {
    /* wifi_ssid buffer is GH_WIFI_SSID_MAX_LEN (33); a 40-char value must be rejected. */
    char payload[512];
    build_payload(payload, sizeof(payload), "1", LOCAL_DEVICE_ID,
                  "\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"", "\"p\"", "\"mqtt://h\"", NULL);

    gh_provisioning_payload_t out;
    gh_provisioning_status_t status;
    bool ok = gh_codec_parse_provisioning_payload(payload, LOCAL_DEVICE_ID, &out, &status);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(GH_PROVISIONING_STATUS_WIFI_SSID_EMPTY, status.error_code);
}

static void test_null_arguments_rejected(void) {
    gh_provisioning_payload_t out;
    gh_provisioning_status_t status;
    bool ok = gh_codec_parse_provisioning_payload(NULL, LOCAL_DEVICE_ID, &out, &status);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_INT(GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, status.error_code);
}

void run_codec_provisioning_suite(void) {
    RUN_TEST(test_valid_payload_defaults_heartbeat);
    RUN_TEST(test_valid_payload_empty_password_and_explicit_heartbeat);
    RUN_TEST(test_mqtts_scheme_accepted);
    RUN_TEST(test_invalid_payloads_map_to_canonical_codes);
    RUN_TEST(test_non_json_rejected);
    RUN_TEST(test_oversized_ssid_rejected);
    RUN_TEST(test_null_arguments_rejected);
}
