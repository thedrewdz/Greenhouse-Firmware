/*
 * Host unit-test entry point.
 *
 * Links and runs the real firmware modules from ../../src (codec + retry
 * backoff) natively via Unity. Each suite lives in its own translation unit and
 * exposes a run_* function that this entry point sequences inside a single
 * Unity session.
 */
#include "unity.h"

void run_codec_provisioning_suite(void);
void run_codec_serialization_suite(void);
void run_retry_backoff_suite(void);

/* Unity requires these; the modules under test are stateless, so both no-op. */
void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    run_codec_provisioning_suite();
    run_codec_serialization_suite();
    run_retry_backoff_suite();
    return UNITY_END();
}
