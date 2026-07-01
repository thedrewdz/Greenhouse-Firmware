#ifndef GH_HOST_FAKE_ESP_RANDOM_H
#define GH_HOST_FAKE_ESP_RANDOM_H

#include <stdint.h>

/* Returns a test-controlled value so backoff jitter is deterministic. */
uint32_t esp_random(void);

#endif
