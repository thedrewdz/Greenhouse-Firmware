/*
 * Compiled tests for the codec builders and command parsers:
 *  - gh_codec_build_provisioning_status_payload  (BLE GATT status serialization)
 *  - gh_codec_build_response_payload             (command ack/error)
 *  - gh_codec_build_heartbeat_payload
 *  - gh_codec_parse_command_payload / gh_codec_parse_command_topic
 *
 * Built JSON is parsed back with cJSON so assertions are field-order independent.
 */
#include <string.h>

#include "unity.h"

#include "cJSON.h"
#include "codec_json.h"

static void test_provisioning_status_success_serialization(void) {
    gh_provisioning_status_t status;
    memset(&status, 0, sizeof(status));
    strcpy(status.result, "success");
    status.error_code = GH_PROVISIONING_STATUS_SUCCESS;
    status.error_message[0] = '\0';

    char *json = gh_codec_build_provisioning_status_payload(&status);
    TEST_ASSERT_NOT_NULL(json);

    cJSON *root = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("success", cJSON_GetObjectItem(root, "result")->valuestring);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetObjectItem(root, "error_code")->valueint);
    TEST_ASSERT_EQUAL_STRING("", cJSON_GetObjectItem(root, "error_message")->valuestring);
    cJSON_Delete(root);
    gh_codec_free_payload(json);
}

static void test_provisioning_status_error_serialization(void) {
    gh_provisioning_status_t status;
    memset(&status, 0, sizeof(status));
    strcpy(status.result, "error");
    status.error_code = GH_PROVISIONING_STATUS_MQTT_BROKER_URI_INVALID;
    strcpy(status.error_message, "mqtt_broker_uri is invalid");

    char *json = gh_codec_build_provisioning_status_payload(&status);
    TEST_ASSERT_NOT_NULL(json);

    cJSON *root = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("error", cJSON_GetObjectItem(root, "result")->valuestring);
    TEST_ASSERT_EQUAL_INT(2004, cJSON_GetObjectItem(root, "error_code")->valueint);
    TEST_ASSERT_EQUAL_STRING("mqtt_broker_uri is invalid",
                             cJSON_GetObjectItem(root, "error_message")->valuestring);
    cJSON_Delete(root);
    gh_codec_free_payload(json);
}

static void test_status_null_returns_null(void) {
    TEST_ASSERT_NULL(gh_codec_build_provisioning_status_payload(NULL));
}

static void test_response_serialization(void) {
    gh_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.id = 42;
    strcpy(resp.device_id, "1ADD5912AF61");
    resp.slot_id = 3;
    resp.value = 1.0;
    strcpy(resp.state, "on");
    resp.error_code = GH_ERR_NONE;

    char *json = gh_codec_build_response_payload(&resp);
    TEST_ASSERT_NOT_NULL(json);

    cJSON *root = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(42, cJSON_GetObjectItem(root, "id")->valueint);
    TEST_ASSERT_EQUAL_STRING("1ADD5912AF61", cJSON_GetObjectItem(root, "device_id")->valuestring);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetObjectItem(root, "slot_id")->valueint);
    TEST_ASSERT_EQUAL_STRING("on", cJSON_GetObjectItem(root, "state")->valuestring);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetObjectItem(root, "error_code")->valueint);
    cJSON_Delete(root);
    gh_codec_free_payload(json);
}

static void test_heartbeat_serialization_has_empty_slot_arrays(void) {
    gh_heartbeat_t hb;
    memset(&hb, 0, sizeof(hb));
    hb.id = 1;
    strcpy(hb.device_id, "1ADD5912AF61");
    strcpy(hb.hardware_revision, "revA");
    strcpy(hb.firmware_version, "0.1.0");
    hb.uptime_seconds = 120;
    hb.wifi_rssi = -55;
    hb.slot_count = 0;

    char *json = gh_codec_build_heartbeat_payload(&hb);
    TEST_ASSERT_NOT_NULL(json);

    cJSON *root = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("1ADD5912AF61", cJSON_GetObjectItem(root, "device_id")->valuestring);
    TEST_ASSERT_EQUAL_INT(-55, cJSON_GetObjectItem(root, "wifi_rssi")->valueint);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetObjectItem(root, "slot_count")->valueint);
    TEST_ASSERT_TRUE(cJSON_IsArray(cJSON_GetObjectItem(root, "slots")));
    TEST_ASSERT_TRUE(cJSON_IsArray(cJSON_GetObjectItem(root, "capabilities")));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(cJSON_GetObjectItem(root, "slots")));
    cJSON_Delete(root);
    gh_codec_free_payload(json);
}

static void test_command_payload_valid(void) {
    gh_command_t cmd;
    gh_error_code_t err;
    bool ok = gh_codec_parse_command_payload(
        "{\"id\":7,\"slot_id\":2,\"state\":\"on\",\"value\":1.5}", &cmd, &err);

    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_INT(GH_ERR_NONE, err);
    TEST_ASSERT_EQUAL_UINT32(7U, cmd.id);
    TEST_ASSERT_EQUAL_INT(2, cmd.slot_id);
    TEST_ASSERT_EQUAL_STRING("on", cmd.state);
    TEST_ASSERT_EQUAL_DOUBLE(1.5, cmd.value);
}

static void test_command_payload_invalid_json(void) {
    gh_command_t cmd;
    gh_error_code_t err;
    TEST_ASSERT_FALSE(gh_codec_parse_command_payload("{oops", &cmd, &err));
    TEST_ASSERT_EQUAL_INT(GH_ERR_INVALID_PAYLOAD, err);
}

static void test_command_payload_missing_field(void) {
    gh_command_t cmd;
    gh_error_code_t err;
    TEST_ASSERT_FALSE(gh_codec_parse_command_payload("{\"id\":7,\"slot_id\":2}", &cmd, &err));
    TEST_ASSERT_EQUAL_INT(GH_ERR_INVALID_PAYLOAD, err);
}

static void test_command_topic_parsing(void) {
    bool is_write = false;
    TEST_ASSERT_TRUE(gh_codec_parse_command_topic("ghcmd/wr-1ADD5912AF61", "1ADD5912AF61", &is_write));
    TEST_ASSERT_TRUE(is_write);

    is_write = true;
    TEST_ASSERT_TRUE(gh_codec_parse_command_topic("ghcmd/rd-1ADD5912AF61", "1ADD5912AF61", &is_write));
    TEST_ASSERT_FALSE(is_write);

    TEST_ASSERT_FALSE(gh_codec_parse_command_topic("ghcmd/wr-OTHER", "1ADD5912AF61", &is_write));
    TEST_ASSERT_FALSE(gh_codec_parse_command_topic("random/topic", "1ADD5912AF61", &is_write));
}

void run_codec_serialization_suite(void) {
    RUN_TEST(test_provisioning_status_success_serialization);
    RUN_TEST(test_provisioning_status_error_serialization);
    RUN_TEST(test_status_null_returns_null);
    RUN_TEST(test_response_serialization);
    RUN_TEST(test_heartbeat_serialization_has_empty_slot_arrays);
    RUN_TEST(test_command_payload_valid);
    RUN_TEST(test_command_payload_invalid_json);
    RUN_TEST(test_command_payload_missing_field);
    RUN_TEST(test_command_topic_parsing);
}
