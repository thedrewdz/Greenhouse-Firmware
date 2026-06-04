# Test Gap Report: Edge Unit Onboarding

## Inputs

- Canonical spec: `specs/edge-unit-onboarding/spec.md` from Greenhouse Documentation.
- Implementation diff: `origin/main...HEAD` on branch `spec/edge-unit-onboarding`.
- Existing tests after mitigation: `greenhouse-edge/tests/test_onboarding_contract.py`.
- Local strategy reference: `docs/skills/esp-idf-testing-strategy.md`.
- Review feedback artifacts: `.agent-output/specs/edge-unit-onboarding/review-report.md` and review-fix commit `346cd3c`.

## Review Fix Re-Evaluation

- Review finding 1, bootstrap retry backoff starts at 2s: addressed by computing retry delay from `s_retry_count - 1` when at least one failed attempt has completed. This aligns the intended five-attempt budget with four inter-attempt waits of approximately `1s`, `2s`, `4s`, and `8s`, subject to the existing plus/minus 20 percent jitter.
- Review finding 2, re-entry to Provisioning Mode leaves WiFi/MQTT runtime state active: addressed at the implementation level. `enter_provisioning_mode()` now calls `gh_mqtt_stop_reset()` and `gh_network_stop_reset()` before advertising BLE, and `gh_mqtt_init()` recreates the MQTT client for a new Bootstrap MQTT Endpoint.
- Test status for both fixes: partially mitigated by host tests. `greenhouse-edge/tests/test_onboarding_contract.py` verifies canonical retry schedule/jitter bounds, source-level use of completed failures for WiFi/MQTT backoff, reset-before-BLE-advertising ordering, and MQTT client recreation before applying a new Bootstrap MQTT Endpoint. ESP-IDF async event ordering still requires HIL or fake-service tests.

## Acceptance Criteria Mapping

- New Edge Unit can be onboarded with no wired connection.
  - Implementation evidence: unprovisioned boot enters BLE Provisioning Mode and starts the onboarding service.
  - Test status: gap remains. Not possible to fully test in this host-only pass because it requires an erased/fresh ESP32 and BLE scan/connect/write/read smoke validation. Host tests do not exercise NimBLE advertising.
- Main Unit can provision WiFi and MQTT endpoint over BLE to a fresh Edge Unit.
  - Implementation evidence: BLE writable payload characteristic calls the provisioning callback; parser accepts `wifi_ssid`, `wifi_password`, `mqtt_broker_uri`, `device_id`, `schema_version`, and optional `heartbeat_interval_ms`.
  - Test status: partially mitigated. Host tests cover the provisioning payload contract cases, but BLE GATT write/read/notify, MTU, and Main Unit client interoperability still require BLE HIL or an ESP-IDF NimBLE host test harness.
- Edge Unit publishes first heartbeat after successful onboarding.
  - Implementation evidence: accepted payload is saved, BLE advertising stops, WiFi/MQTT bootstrap starts, first MQTT connection schedules immediate heartbeat publish to `gh/heartbeat`.
  - Test status: gap remains. Not possible to prove in this host-only pass because it requires live WiFi, a reachable MQTT broker, and broker-backed assertion on `gh/heartbeat`.
- Invalid onboarding payloads are rejected with explicit error codes.
  - Implementation evidence: parser maps unsupported schema, device ID mismatch, empty SSID, and invalid MQTT URI to canonical `200x` status codes.
  - Test status: partially mitigated. Host tests cover valid payload defaults and canonical rejection codes for unsupported schema, device ID mismatch, empty SSID, missing/malformed MQTT URI, but they do not execute `gh_codec_parse_provisioning_payload()` against ESP-IDF `cJSON`. A future ESP-IDF Unity test should execute the firmware parser directly.
- Rebooted, already-provisioned Edge Unit skips Provisioning Mode and proceeds to normal startup.
  - Implementation evidence: boot loads NVS provisioning config and starts WiFi/MQTT bootstrap when required keys exist.
  - Test status: gap remains. Not possible to verify in this host-only pass because it requires NVS-backed boot state and BLE advertising observation after reboot.
- Edge Unit loads persisted WiFi and MQTT configuration from NVS on boot and can overwrite persisted values when a newer valid payload is received.
  - Implementation evidence: NVS load path and save path exist for WiFi SSID, password, MQTT URI, and heartbeat interval.
  - Test status: gap remains for NVS behavior. Host tests cover omitted heartbeat default at the contract level, but NVS load/save/overwrite and failed-write retention require ESP-IDF NVS fake tests or HIL.

## New Or Updated Tests

- Added `greenhouse-edge/tests/test_onboarding_contract.py`.
- Test layer: host-runnable contract/regression tests using Python `unittest`.
- Coverage added:
  - Bootstrap retry budget and canonical backoff schedule: `1s`, `2s`, `4s`, `8s`.
  - Retry jitter bounds: plus/minus 20 percent.
  - Source-level guard that WiFi/MQTT schedule backoff from completed failures, not the next attempt number.
  - Source-level guard that BLE Provisioning Mode re-entry resets MQTT/WiFi before advertising.
  - Source-level guard that MQTT init recreates the client before applying a new Bootstrap MQTT Endpoint.
  - Provisioning payload contract cases for valid/default behavior and canonical `200x` rejection mapping.
- Limitation: these tests do not execute ESP-IDF `cJSON`, NimBLE, NVS, WiFi, or `esp-mqtt` runtime behavior.

## Verification Performed

- `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge`.
- Result: passed.
- `python -m unittest discover -s greenhouse-edge\tests -v` from repository root.
- Result after review remediation: passed, `Ran 10 tests` and `OK`.
- Noted warning: PlatformIO board profile expects 4MB flash while detected flash is 2MB; linked firmware uses 1,163,633 bytes of a 2,031,616 byte app partition.

## Code Review Remediation Coverage

- BLE status delivery ordering: partially mitigated by host source-regression test. The test verifies `on_provisioning_payload()` does not stop BLE directly and that `services_ble_onboarding.c` calls `notify_status()` before stopping onboarding on success.
- NVS last-known-valid retention: partially mitigated by host source-regression test. The test verifies the save path uses inactive-slot candidate write before active-slot promotion and no longer writes live keys directly inside `gh_provisioning_config_save()`.
- Runtime proof still needed: fake-NVS tests should inject failures at each candidate write and metadata promotion point; BLE HIL should confirm write response, status read/notify, and first heartbeat publish through a broker.

## Coverage Gaps

- Direct ESP-IDF unit tests for `gh_codec_parse_provisioning_payload` success/default behavior and canonical rejection/error-code mapping remain missing; host contract tests cover the same cases without executing firmware `cJSON`.
- No automated tests for BLE GATT status response serialization, payload size rejection, or status notification behavior.
- ESP-IDF fake-service or HIL tests for WiFi/MQTT 10s/5s attempt timeout handling remain missing; host tests cover retry budget, backoff schedule, jitter bounds, and reset ordering.
- No automated tests for first-heartbeat publish failure consuming the MQTT bootstrap retry budget.
- No automated tests for NVS persistence overwrite and last-known-valid retention on save failure.
- No HIL smoke evidence for fresh-device BLE onboarding through first `gh/heartbeat` publish.
- No status transition could be recorded in `spec.md` because the canonical spec currently has no Spec Control or Status History block in this implementation repository.

## Residual Risks

- A payload/client interoperability issue could remain hidden because BLE characteristic UUIDs, write mode expectations, MTU behavior, and notification subscription behavior are not yet covered by tests.
- Retry timing and reset/re-entry behavior now look correct in code, but remain unproven under asynchronous ESP-IDF WiFi/MQTT event ordering without host-mock or HIL timing evidence.
- NVS multi-key commit behavior may be acceptable for Phase 1, but crash/power-loss consistency is not proven by this branch.
- The first heartbeat path is only build-verified; it still needs a live broker assertion on `gh/heartbeat`.

## Recommended Next Handoff

- Status recommendation: blocked for `ready-for-review` until critical acceptance criteria have test evidence.
- Next role: implementation/test agent to add a minimal ESP-IDF Unity or host-side test harness for codec and retry policy, then run a small HIL onboarding smoke test.
- Documentation follow-up: update canonical Spec Control handling so downstream agents can record `test-in-progress`, `ready-for-review`, or `blocked` transitions without modifying the documentation repository directly.
