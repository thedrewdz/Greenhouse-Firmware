#ifndef GH_HOST_FAKE_ESP_EVENT_H
#define GH_HOST_FAKE_ESP_EVENT_H

#include <stdint.h>

#include "esp_err.h"

/* ESP-IDF event-loop surface used by the services, faked for host tests. */

typedef const char *esp_event_base_t;
typedef void (*esp_event_handler_t)(void *arg, esp_event_base_t base, int32_t event_id, void *event_data);

#define ESP_EVENT_ANY_ID (-1)

esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_register(esp_event_base_t base, int32_t event_id,
                                     esp_event_handler_t handler, void *arg);

#endif
