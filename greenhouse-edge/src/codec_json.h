#ifndef CODEC_JSON_H
#define CODEC_JSON_H

#include <stdbool.h>

#include "app_types.h"

bool gh_codec_parse_command_topic(const char *topic, const char *device_id, bool *is_write);
bool gh_codec_parse_command_payload(const char *payload, gh_command_t *out_cmd, gh_error_code_t *out_error);
char *gh_codec_build_response_payload(const gh_response_t *response);
char *gh_codec_build_heartbeat_payload(const gh_heartbeat_t *heartbeat);
void gh_codec_free_payload(char *payload);

#endif
