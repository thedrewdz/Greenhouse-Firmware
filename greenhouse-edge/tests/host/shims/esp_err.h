#ifndef GH_HOST_SHIM_ESP_ERR_H
#define GH_HOST_SHIM_ESP_ERR_H

/*
 * Minimal host-build stand-in for ESP-IDF's <esp_err.h>.
 *
 * The pure firmware modules under test (codec, provisioning config types) only
 * need the esp_err_t type and a couple of well-known codes to satisfy their
 * public headers -- they never call the ESP error-handling runtime. This shim
 * lets those headers compile natively without pulling in ESP-IDF. It is used
 * only by the host test build; on-target builds use the real ESP-IDF header.
 */

typedef int esp_err_t;

#define ESP_OK          0
#define ESP_FAIL        (-1)
#define ESP_ERR_NVS_NOT_FOUND   0x1102
#define ESP_ERR_INVALID_ARG     0x102
#define ESP_ERR_INVALID_STATE   0x103

#endif
