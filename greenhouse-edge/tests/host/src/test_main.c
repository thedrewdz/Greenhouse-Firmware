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
void run_service_mqtt_suite(void);
void run_service_network_suite(void);
void run_service_provisioning_nvs_suite(void);

/* Unity requires these. Pure-codec/backoff tests are stateless; the fake-service
 * suites reset their own fake + service state at the start of each test. */
void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    run_codec_provisioning_suite();
    run_codec_serialization_suite();
    run_retry_backoff_suite();
    run_service_mqtt_suite();
    run_service_network_suite();
    run_service_provisioning_nvs_suite();
    return UNITY_END();
}
