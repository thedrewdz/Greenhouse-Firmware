#ifndef GH_HOST_FAKE_ESP_NETIF_H
#define GH_HOST_FAKE_ESP_NETIF_H

#include <stdint.h>

#include "esp_err.h"
#include "esp_event.h"

/* IP event base + ids used by services_network.c */
extern const char *const IP_EVENT_BASE_NAME;
#define IP_EVENT (IP_EVENT_BASE_NAME)

enum {
    IP_EVENT_STA_GOT_IP = 0,
};

esp_err_t esp_netif_init(void);
void *esp_netif_create_default_wifi_sta(void);

#endif
