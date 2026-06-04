#ifndef CODEC_JSON_H
#define CODEC_JSON_H

#include <stdbool.h>

#include "app_types.h"
#include "services_provisioning_config.h"

bool gh_codec_parse_command_topic(const char *topic, const char *device_id, bool *is_write);
bool gh_codec_parse_command_payload(const char *payload, gh_command_t *out_cmd, gh_error_code_t *out_error);
bool gh_codec_parse_provisioning_payload(
    const char *payload,
    const char *local_device_id,
    gh_provisioning_payload_t *out_payload,
    gh_provisioning_status_t *out_status);
char *gh_codec_build_response_payload(const gh_response_t *response);
char *gh_codec_build_heartbeat_payload(const gh_heartbeat_t *heartbeat);
char *gh_codec_build_provisioning_status_payload(const gh_provisioning_status_t *status);
void gh_codec_free_payload(char *payload);

#endif
