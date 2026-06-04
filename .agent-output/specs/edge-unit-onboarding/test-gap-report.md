# Test Gap Report: Edge Unit Onboarding

## Inputs

- Canonical spec: `specs/edge-unit-onboarding/spec.md` from Greenhouse Documentation.
- Implementation diff: `origin/main...HEAD` on branch `spec/edge-unit-onboarding`.
- Existing tests: no repository test source files found; only generated `.pio` build output exists.
- Local strategy reference: `docs/skills/esp-idf-testing-strategy.md`.
- Review feedback artifacts: `.agent-output/specs/edge-unit-onboarding/review-report.md` and review-fix commit `346cd3c`.

## Review Fix Re-Evaluation

- Review finding 1, bootstrap retry backoff starts at 2s: addressed by computing retry delay from `s_retry_count - 1` when at least one failed attempt has completed. This aligns the intended five-attempt budget with four inter-attempt waits of approximately `1s`, `2s`, `4s`, and `8s`, subject to the existing plus/minus 20 percent jitter.
- Review finding 2, re-entry to Provisioning Mode leaves WiFi/MQTT runtime state active: addressed at the implementation level. `enter_provisioning_mode()` now calls `gh_mqtt_stop_reset()` and `gh_network_stop_reset()` before advertising BLE, and `gh_mqtt_init()` recreates the MQTT client for a new Bootstrap MQTT Endpoint.
- Test status for both fixes: build-verified and code-reviewed, but not behavior-verified by automated host tests or HIL timing tests.

## Acceptance Criteria Mapping

- New Edge Unit can be onboarded with no wired connection.
  - Implementation evidence: unprovisioned boot enters BLE Provisioning Mode and starts the onboarding service.
  - Test status: gap. Requires HIL BLE scan/connect/write/read smoke test on an erased or fresh ESP32.
- Main Unit can provision WiFi and MQTT endpoint over BLE to a fresh Edge Unit.
  - Implementation evidence: BLE writable payload characteristic calls the provisioning callback; parser accepts `wifi_ssid`, `wifi_password`, `mqtt_broker_uri`, `device_id`, `schema_version`, and optional `heartbeat_interval_ms`.
  - Test status: gap. Needs BLE GATT host/HIL test for valid write, status read/notify, and MTU/payload-size behavior.
- Edge Unit publishes first heartbeat after successful onboarding.
  - Implementation evidence: accepted payload is saved, BLE advertising stops, WiFi/MQTT bootstrap starts, first MQTT connection schedules immediate heartbeat publish to `gh/heartbeat`.
  - Test status: gap. Needs broker-backed HIL smoke test proving first heartbeat publish after BLE provisioning.
- Invalid onboarding payloads are rejected with explicit error codes.
  - Implementation evidence: parser maps unsupported schema, device ID mismatch, empty SSID, and invalid MQTT URI to canonical `200x` status codes.
  - Test status: gap. Needs table-driven unit tests for every canonical rejection case and exact one-error response behavior.
- Rebooted, already-provisioned Edge Unit skips Provisioning Mode and proceeds to normal startup.
  - Implementation evidence: boot loads NVS provisioning config and starts WiFi/MQTT bootstrap when required keys exist.
  - Test status: gap. Needs NVS-backed test or HIL reboot smoke test confirming no BLE provisioning advertisement during normal startup.
- Edge Unit loads persisted WiFi and MQTT configuration from NVS on boot and can overwrite persisted values when a newer valid payload is received.
  - Implementation evidence: NVS load path and save path exist for WiFi SSID, password, MQTT URI, and heartbeat interval.
  - Test status: gap. Needs persistence tests for load, save, overwrite, omitted heartbeat default, and failed-write retention.

## New Or Updated Tests

- None added in this pass.
- Reason: the repository does not currently contain a source test harness or host-mock structure. The firmware build was run as the available executable gate, but build success is not behavioral test evidence.

## Verification Performed

- `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge`.
- Result: passed.
- Noted warning: PlatformIO board profile expects 4MB flash while detected flash is 2MB; linked firmware uses 1,163,633 bytes of a 2,031,616 byte app partition.

## Coverage Gaps

- No automated unit tests for `gh_codec_parse_provisioning_payload` success/default behavior or canonical rejection/error-code mapping.
- No automated tests for BLE GATT status response serialization, payload size rejection, or status notification behavior.
- No automated tests for WiFi/MQTT bootstrap retry budget, 10s/5s attempt timeout handling, backoff schedule, or plus/minus 20 percent jitter bounds.
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
