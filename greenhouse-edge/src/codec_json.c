#include "codec_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "runtime_config.h"

static void set_provisioning_status(
    gh_provisioning_status_t *status,
    gh_provisioning_status_code_t error_code,
    const char *message) {
    if (status == NULL) {
        return;
    }

    (void)memset(status, 0, sizeof(*status));
    if (error_code == GH_PROVISIONING_STATUS_SUCCESS) {
        (void)strcpy(status->result, "success");
        status->error_code = GH_PROVISIONING_STATUS_SUCCESS;
        status->error_message[0] = '\0';
        return;
    }

    (void)strcpy(status->result, "error");
    status->error_code = error_code;
    if (message != NULL) {
        (void)snprintf(status->error_message, sizeof(status->error_message), "%s", message);
    }
}

static bool parse_uint32_field(const cJSON *root, const char *name, uint32_t *out_value) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsNumber(item) || item->valuedouble < 0) {
        return false;
    }

    *out_value = (uint32_t)item->valuedouble;
    return true;
}

static bool parse_int_field(const cJSON *root, const char *name, int *out_value) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsNumber(item)) {
        return false;
    }

    *out_value = item->valueint;
    return true;
}

static bool parse_double_field(const cJSON *root, const char *name, double *out_value) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsNumber(item)) {
        return false;
    }

    *out_value = item->valuedouble;
    return true;
}

static bool parse_string_field(const cJSON *root, const char *name, char *out_buf, size_t out_len) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }

    if (strlen(item->valuestring) >= out_len) {
        return false;
    }

    (void)strcpy(out_buf, item->valuestring);
    return true;
}

static bool parse_optional_uint32_field(const cJSON *root, const char *name, uint32_t *out_value, bool *out_present) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, name);

    if (item == NULL) {
        *out_present = false;
        return true;
    }

    if (!cJSON_IsNumber(item) || item->valuedouble < 0) {
        return false;
    }

    *out_value = (uint32_t)item->valuedouble;
    *out_present = true;
    return true;
}

static bool is_valid_mqtt_uri(const char *uri) {
    size_t prefix_len;

    if (uri == NULL) {
        return false;
    }

    if (strncmp(uri, "mqtt://", 7) == 0) {
        prefix_len = 7U;
    } else if (strncmp(uri, "mqtts://", 8) == 0) {
        prefix_len = 8U;
    } else {
        return false;
    }

    return uri[prefix_len] != '\0' && uri[prefix_len] != '/';
}

bool gh_codec_parse_command_topic(const char *topic, const char *device_id, bool *is_write) {
    char expected_read_topic[32];
    char expected_write_topic[32];

    if (topic == NULL || device_id == NULL || is_write == NULL) {
        return false;
    }

    (void)snprintf(expected_read_topic, sizeof(expected_read_topic), "ghcmd/rd-%s", device_id);
    (void)snprintf(expected_write_topic, sizeof(expected_write_topic), "ghcmd/wr-%s", device_id);

    if (strcmp(topic, expected_read_topic) == 0) {
        *is_write = false;
        return true;
    }

    if (strcmp(topic, expected_write_topic) == 0) {
        *is_write = true;
        return true;
    }

    return false;
}

bool gh_codec_parse_command_payload(const char *payload, gh_command_t *out_cmd, gh_error_code_t *out_error) {
    cJSON *root;

    if (payload == NULL || out_cmd == NULL || out_error == NULL) {
        return false;
    }

    out_cmd->is_write = false;

    root = cJSON_Parse(payload);
    if (root == NULL) {
        *out_error = GH_ERR_INVALID_PAYLOAD;
        return false;
    }

    if (!parse_uint32_field(root, "id", &out_cmd->id) ||
        !parse_int_field(root, "slot_id", &out_cmd->slot_id) ||
        !parse_string_field(root, "state", out_cmd->state, sizeof(out_cmd->state)) ||
        !parse_double_field(root, "value", &out_cmd->value)) {
        cJSON_Delete(root);
        *out_error = GH_ERR_INVALID_PAYLOAD;
        return false;
    }

    cJSON_Delete(root);
    *out_error = GH_ERR_NONE;
    return true;
}

bool gh_codec_parse_provisioning_payload(
    const char *payload,
    const char *local_device_id,
    gh_provisioning_payload_t *out_payload,
    gh_provisioning_status_t *out_status) {
    cJSON *root;
    gh_provisioning_payload_t parsed;

    if (payload == NULL || local_device_id == NULL || out_payload == NULL || out_status == NULL) {
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, "invalid parser arguments");
        return false;
    }

    (void)memset(&parsed, 0, sizeof(parsed));
    parsed.config.heartbeat_interval_ms = GH_HEARTBEAT_INTERVAL_MS;

    root = cJSON_Parse(payload);
    if (root == NULL) {
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_UNSUPPORTED_SCHEMA_VERSION, "payload is not valid json");
        return false;
    }

    if (!parse_uint32_field(root, "schema_version", &parsed.schema_version) ||
        parsed.schema_version != GH_PROVISIONING_SCHEMA_VERSION) {
        cJSON_Delete(root);
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_UNSUPPORTED_SCHEMA_VERSION, "unsupported schema_version");
        return false;
    }

    if (!parse_string_field(root, "device_id", parsed.device_id, sizeof(parsed.device_id)) ||
        strcmp(parsed.device_id, local_device_id) != 0) {
        cJSON_Delete(root);
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_DEVICE_ID_MISMATCH, "device_id does not match hardware identity");
        return false;
    }

    if (!parse_string_field(root, "wifi_ssid", parsed.config.wifi_ssid, sizeof(parsed.config.wifi_ssid)) ||
        parsed.config.wifi_ssid[0] == '\0') {
        cJSON_Delete(root);
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_WIFI_SSID_EMPTY, "wifi_ssid is required");
        return false;
    }

    if (!parse_string_field(root, "wifi_password", parsed.config.wifi_password, sizeof(parsed.config.wifi_password))) {
        cJSON_Delete(root);
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, "wifi_password is invalid");
        return false;
    }

    if (!parse_string_field(root, "mqtt_broker_uri", parsed.config.mqtt_broker_uri, sizeof(parsed.config.mqtt_broker_uri)) ||
        !is_valid_mqtt_uri(parsed.config.mqtt_broker_uri)) {
        cJSON_Delete(root);
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_MQTT_BROKER_URI_INVALID, "mqtt_broker_uri is invalid");
        return false;
    }

    if (!parse_optional_uint32_field(
            root,
            "heartbeat_interval_ms",
            &parsed.config.heartbeat_interval_ms,
            &parsed.has_heartbeat_interval_ms)) {
        cJSON_Delete(root);
        set_provisioning_status(out_status, GH_PROVISIONING_STATUS_INTERNAL_PERSISTENCE_ERROR, "heartbeat_interval_ms is invalid");
        return false;
    }

    cJSON_Delete(root);
    *out_payload = parsed;
    set_provisioning_status(out_status, GH_PROVISIONING_STATUS_SUCCESS, NULL);
    return true;
}

char *gh_codec_build_response_payload(const gh_response_t *response) {
    cJSON *root;
    char *json;

    if (response == NULL) {
        return NULL;
    }

    root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    if (!cJSON_AddNumberToObject(root, "id", response->id) ||
        !cJSON_AddStringToObject(root, "device_id", response->device_id) ||
        !cJSON_AddNumberToObject(root, "slot_id", response->slot_id) ||
        !cJSON_AddNumberToObject(root, "value", response->value) ||
        !cJSON_AddStringToObject(root, "state", response->state) ||
        !cJSON_AddNumberToObject(root, "error_code", response->error_code)) {
        cJSON_Delete(root);
        return NULL;
    }

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

char *gh_codec_build_heartbeat_payload(const gh_heartbeat_t *heartbeat) {
    cJSON *root;
    cJSON *slots;
    cJSON *capabilities;
    char *json;

    if (heartbeat == NULL) {
        return NULL;
    }

    root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    slots = cJSON_CreateArray();
    capabilities = cJSON_CreateArray();
    if (slots == NULL || capabilities == NULL) {
        cJSON_Delete(slots);
        cJSON_Delete(capabilities);
        cJSON_Delete(root);
        return NULL;
    }

    if (!cJSON_AddNumberToObject(root, "id", heartbeat->id) ||
        !cJSON_AddStringToObject(root, "device_id", heartbeat->device_id) ||
        !cJSON_AddStringToObject(root, "hardware_revision", heartbeat->hardware_revision) ||
        !cJSON_AddStringToObject(root, "firmware_version", heartbeat->firmware_version) ||
        !cJSON_AddNumberToObject(root, "uptime_seconds", heartbeat->uptime_seconds) ||
        !cJSON_AddNumberToObject(root, "wifi_rssi", heartbeat->wifi_rssi) ||
        !cJSON_AddNumberToObject(root, "slot_count", heartbeat->slot_count) ||
        !cJSON_AddItemToObject(root, "slots", slots) ||
        !cJSON_AddItemToObject(root, "capabilities", capabilities)) {
        cJSON_Delete(root);
        return NULL;
    }

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

char *gh_codec_build_provisioning_status_payload(const gh_provisioning_status_t *status) {
    cJSON *root;
    char *json;

    if (status == NULL) {
        return NULL;
    }

    root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    if (!cJSON_AddStringToObject(root, "result", status->result) ||
        !cJSON_AddNumberToObject(root, "error_code", status->error_code) ||
        !cJSON_AddStringToObject(root, "error_message", status->error_message)) {
        cJSON_Delete(root);
        return NULL;
    }

    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

void gh_codec_free_payload(char *payload) {
    cJSON_free(payload);
}
