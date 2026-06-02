#include "codec_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

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

void gh_codec_free_payload(char *payload) {
    cJSON_free(payload);
}
