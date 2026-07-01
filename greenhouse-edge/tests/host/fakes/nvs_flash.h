#ifndef GH_HOST_FAKE_NVS_FLASH_H
#define GH_HOST_FAKE_NVS_FLASH_H

#include "esp_err.h"

esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_erase(void);

#endif
