#ifndef GH_HOST_FAKE_ESP_TIMER_H
#define GH_HOST_FAKE_ESP_TIMER_H

#include <stdint.h>

/* Fake monotonic clock in microseconds, driven by the test via fake helpers. */
int64_t esp_timer_get_time(void);

#endif
