#include "retry_backoff.h"

uint32_t gh_retry_backoff_base_ms(uint32_t base_ms, uint32_t max_ms, uint32_t completed_failures) {
    uint32_t backoff = base_ms;
    uint32_t i;

    for (i = 0; i < completed_failures && backoff < max_ms; ++i) {
        backoff *= 2U;
        if (backoff > max_ms) {
            backoff = max_ms;
        }
    }

    return backoff;
}

uint32_t gh_retry_backoff_jitter_window_ms(uint32_t backoff_ms) {
    return backoff_ms / 5U;
}

uint32_t gh_retry_backoff_apply_jitter(uint32_t backoff_ms, uint32_t random_value) {
    int32_t jitter_window = (int32_t)gh_retry_backoff_jitter_window_ms(backoff_ms);
    int32_t jitter = (int32_t)(random_value % (uint32_t)((jitter_window * 2) + 1)) - jitter_window;
    return (uint32_t)((int32_t)backoff_ms + jitter);
}
