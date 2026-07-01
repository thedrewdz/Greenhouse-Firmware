#ifndef GH_HOST_FAKE_EVENT_GROUPS_H
#define GH_HOST_FAKE_EVENT_GROUPS_H

#include <stdint.h>

/* Minimal FreeRTOS event-group surface used by services_network.c. */

typedef uint32_t EventBits_t;
typedef struct gh_fake_event_group *EventGroupHandle_t;

#define BIT0 (1U << 0)

EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupSetBits(EventGroupHandle_t group, EventBits_t bits);
EventBits_t xEventGroupClearBits(EventGroupHandle_t group, EventBits_t bits);
EventBits_t xEventGroupGetBits(EventGroupHandle_t group);

#endif
