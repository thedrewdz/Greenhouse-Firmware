#ifndef RETRY_BACKOFF_H
#define RETRY_BACKOFF_H

#include <stdint.h>

/*
 * Pure, hardware-independent retry-backoff math shared by the WiFi and MQTT
 * connect state machines. Kept free of ESP-IDF dependencies so it can be
 * exercised directly by host unit tests (see tests/host/). Randomness for
 * jitter is injected by the caller rather than sampled here, so the schedule
 * is deterministic and testable.
 */

/*
 * Deterministic exponential backoff with a ceiling.
 * Returns base_ms doubled once per completed failure, saturating at max_ms.
 * completed_failures == 0 yields base_ms.
 */
uint32_t gh_retry_backoff_base_ms(uint32_t base_ms, uint32_t max_ms, uint32_t completed_failures);

/* Symmetric jitter window (+/-) applied to a backoff: 20% of the backoff. */
uint32_t gh_retry_backoff_jitter_window_ms(uint32_t backoff_ms);

/*
 * Applies symmetric jitter to backoff_ms using an externally supplied random
 * draw. Result lies in [backoff_ms - window, backoff_ms + window] where window
 * is gh_retry_backoff_jitter_window_ms(backoff_ms).
 */
uint32_t gh_retry_backoff_apply_jitter(uint32_t backoff_ms, uint32_t random_value);

#endif
