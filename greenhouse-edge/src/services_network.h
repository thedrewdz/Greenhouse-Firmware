#ifndef SERVICES_NETWORK_H
#define SERVICES_NETWORK_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t gh_network_init(void);
esp_err_t gh_network_start(void);
void gh_network_tick(void);
bool gh_network_is_connected(void);
int gh_network_get_rssi(void);

#endif
