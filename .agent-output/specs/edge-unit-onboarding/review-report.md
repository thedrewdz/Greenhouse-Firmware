# Review Report: Edge Unit Onboarding

## Inputs Reviewed

- Canonical spec: `specs/edge-unit-onboarding/spec.md` from Greenhouse Documentation.
- Canonical docs: `AGENTS.md`, `CONTEXT.md`, `architecture.md`, `device-model.md`, `mqtt-topics.md`, `vision.md`.
- Local docs: `AGENTS.md`, `.github/copilot-instructions.md`, `docs/adr/0001-ble-first-onboarding.md`.
- Code diff: `main...HEAD` on branch `spec/edge-unit-onboarding` at `2f59feb`.
- Test artifacts: `.agent-output/specs/edge-unit-onboarding/test-gap-report.md`.
- Verification commands:
  - `python -m unittest discover -s greenhouse-edge\tests -v`
  - `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge`

## Blocking Findings

1. Successful BLE provisioning can stop the onboarding service before the success status is delivered.
   - Impacted files: `greenhouse-edge/src/app.c`, `greenhouse-edge/src/services_ble_onboarding.c`.
   - Evidence: `on_provisioning_payload()` calls `gh_ble_onboarding_stop()` before writing the success result into `out_status`. The BLE write handler only calls `notify_status()` after the callback returns. Stopping onboarding disables advertising and stops active advertising, so a successful write can transition away from BLE before the Main Unit has a reliable read/notify opportunity for the required status response.
   - Risk: violates the spec response contract requiring a success/error BLE status response. The Main Unit may see a write complete or disconnect without the canonical `{ result, error_code, error_message }` success payload, making onboarding completion ambiguous.
   - Recommended fix: defer `gh_ble_onboarding_stop()` until after the status response has been set and delivered, or have the BLE service own the transition by notifying/read-stabilizing the status first and then asynchronously stopping advertising/session state.

2. Provisioning persistence can partially overwrite the previous valid configuration before returning an error.
   - Impacted file: `greenhouse-edge/src/services_provisioning_config.c`.
   - Evidence: `gh_provisioning_config_save()` writes `wifi_ssid`, then `wifi_password`, then `mqtt_broker_uri`, then `heartbeat_interval_ms` into the existing namespace before `nvs_commit()`. If a later `nvs_set_*` call fails after one or more earlier calls succeeded, the function returns an error but has still staged changes against the same live keys.
   - Risk: violates the spec requirement that persistence be one logical configuration update and that the Edge Unit retain the last known valid configuration on write failure. Depending on NVS behavior and failure point, a later boot can observe a mixed old/new WiFi or MQTT configuration.
   - Recommended fix: write to a shadow namespace/record with a version or validity marker, commit only the complete candidate, then atomically promote/select it on load. At minimum, add a fake-NVS unit test that proves failed later writes cannot change the loaded config.

## Non-Blocking Findings

- The retry backoff and clean re-entry findings from the prior review are addressed in the current branch. WiFi and MQTT now schedule from completed failures and reset runtime state before BLE advertising.

## Architecture Boundary Concerns

- No Main Unit UI, cloud-first, or desktop/server implementation details were introduced.
- MQTT topic naming and JSON field casing remain aligned with canonical MQTT docs.
- BLE UUIDs remain a documentation coordination risk already captured in `doc-feedback.md`.

## Never Events (Auto-Blocking)

- None observed.

## Guardrail Update Required

- Existing `doc-feedback.md` already captures missing Spec Control/status handling, BLE GATT UUID contract gaps, and NVS atomicity ambiguity. No additional guardrail item is required from this review.

## Test Gaps

- The added Python tests are useful regression guards, but several are source-shape tests or a Python reimplementation of the parser rather than execution of firmware code.
- No automated ESP-IDF/fake tests prove BLE success status delivery before stop/disconnect.
- No fake-NVS or HIL tests prove last-known-valid retention on partial write failure.
- No HIL smoke evidence confirms fresh-device BLE onboarding through first `gh/heartbeat` publish.

## Documentation Feedback Items

- Use `.agent-output/specs/edge-unit-onboarding/doc-feedback.md` items for missing Spec Control/status handling, BLE GATT UUID contract, NVS atomicity expectations, implementation artifact gating, and spec-branch workflow guidance.

## Recommended Next Handoff

- Status recommendation: `ready-for-implementation`.
- Next role: implementation agent to fix BLE status delivery ordering and NVS last-known-valid persistence, then test agent to add focused firmware/fake-service coverage and HIL onboarding smoke evidence.
