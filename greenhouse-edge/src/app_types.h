#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GH_STATE_MAX_LEN 16
#define GH_DEVICE_ID_MAX_LEN 13
#define GH_HW_REV_MAX_LEN 8
#define GH_FW_VER_MAX_LEN 16

typedef enum {
    GH_ERR_NONE = 0,
    GH_ERR_UNKNOWN_COMMAND = 1000,
    GH_ERR_INVALID_PAYLOAD = 1001,
    GH_ERR_INVALID_SLOT = 1002,
    GH_ERR_UNSUPPORTED_CAPABILITY = 1003,
    GH_ERR_INVALID_STATE = 1004,
    GH_ERR_DEVICE_FAULT = 1005
} gh_error_code_t;

typedef struct {
    uint32_t id;
    int slot_id;
    char state[GH_STATE_MAX_LEN];
    double value;
    bool is_write;
} gh_command_t;

typedef struct {
    uint32_t id;
    char device_id[GH_DEVICE_ID_MAX_LEN];
    int slot_id;
    double value;
    char state[GH_STATE_MAX_LEN];
    gh_error_code_t error_code;
} gh_response_t;

typedef struct {
    uint32_t id;
    char device_id[GH_DEVICE_ID_MAX_LEN];
    char hardware_revision[GH_HW_REV_MAX_LEN];
    char firmware_version[GH_FW_VER_MAX_LEN];
    uint32_t uptime_seconds;
    int wifi_rssi;
    uint8_t slot_count;
} gh_heartbeat_t;

#endif
