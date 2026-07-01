#ifndef GH_HOST_SHIM_ESP_ERR_H
#define GH_HOST_SHIM_ESP_ERR_H

/*
 * Minimal host-build stand-in for ESP-IDF's <esp_err.h>.
 *
 * The firmware modules under test only need the esp_err_t type, a set of
 * well-known codes, and ESP_ERROR_CHECK -- they never call the ESP error
 * runtime. This shim lets those headers/sources compile natively without
 * ESP-IDF. Used only by the host test build; on-target builds use real IDF.
 *
 * ESP_ERROR_CHECK here evaluates its argument and ignores the result: the fake
 * ESP-IDF layer returns ESP_OK from every init call the services wrap in it,
 * so no abort semantics are needed for the host tests.
 */

typedef int esp_err_t;

#define ESP_OK          0
#define ESP_FAIL        (-1)

#define ESP_ERR_NO_MEM          0x101
#define ESP_ERR_INVALID_ARG     0x102
#define ESP_ERR_INVALID_STATE   0x103
#define ESP_ERR_INVALID_SIZE    0x104
#define ESP_ERR_NOT_FOUND       0x105

/* NVS (ESP_ERR_NVS_BASE == 0x1100) */
#define ESP_ERR_NVS_BASE                0x1100
#define ESP_ERR_NVS_NOT_FOUND           0x1102
#define ESP_ERR_NVS_NO_FREE_PAGES       0x110D
#define ESP_ERR_NVS_NEW_VERSION_FOUND   0x1110

#define ESP_ERROR_CHECK(x)                 \
    do {                                   \
        esp_err_t _gh_err = (x);           \
        (void)_gh_err;                     \
    } while (0)

#endif
