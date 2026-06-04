# Review Report: Edge Unit Onboarding

## Inputs Reviewed

- Canonical spec: `specs/edge-unit-onboarding/spec.md` from Greenhouse Documentation.
- Canonical docs: `AGENTS.md`, `CONTEXT.md`, `architecture.md`, `device-model.md`, `mqtt-topics.md`, `vision.md`.
- Local docs: `AGENTS.md`, `.github/copilot-instructions.md`, `docs/adr/0001-ble-first-onboarding.md`.
- Code diff: `main...HEAD` on branch `spec/edge-unit-onboarding` at `e55d865`.
- Test artifacts: `.agent-output/specs/edge-unit-onboarding/test-gap-report.md`.
- Verification commands:
  - `python -m unittest discover -s greenhouse-edge\tests -v`
  - `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge`

## Blocking Findings

- None identified in this re-review.

## Prior Blocking Findings Re-Evaluated

1. BLE success status delivery before onboarding stop: resolved.
   - Evidence: `on_provisioning_payload()` no longer calls `gh_ble_onboarding_stop()` directly. The BLE write handler calls `notify_status()` before `gh_ble_onboarding_stop()` when the status is success.
   - Impact: the app can set the canonical success status before the BLE service exits Provisioning Mode.

2. Last-known-valid provisioning persistence on write failure: resolved at the code-review level.
   - Evidence: `gh_provisioning_config_save()` writes the new candidate to the inactive NVS slot first, then promotes the candidate through `active_slot` metadata only after candidate commit succeeds. Legacy `gh_prov` loading remains as a fallback when no active-slot metadata exists.
   - Impact: failed candidate writes or failed promotion should leave the previously active slot selected.

3. Bootstrap retry schedule and clean re-entry: still resolved.
   - Evidence: WiFi and MQTT backoff derive from completed failures, and `enter_provisioning_mode()` resets MQTT and WiFi runtime state before BLE advertising.

## Non-Blocking Findings

- None identified.

## Architecture Boundary Concerns

- No Main Unit UI, cloud-first, or desktop/server implementation details were introduced.
- MQTT topic naming and JSON field casing remain aligned with canonical MQTT docs.
- Codec/transport separation is preserved: JSON parsing/serialization remains in `codec_json`, while BLE/MQTT services stay transport-oriented.
- BLE UUIDs remain a documentation coordination risk already captured in `doc-feedback.md`.

## Never Events (Auto-Blocking)

- None observed.

## Guardrail Update Required

- Existing `doc-feedback.md` already captures missing Spec Control/status handling, BLE GATT UUID contract gaps, and NVS atomicity expectations. No additional guardrail item is required from this review.

## Test Gaps

- Host tests are passing and now guard the two prior review blockers, but several tests remain source-shape checks or a Python reimplementation of parser behavior rather than execution of firmware code.
- No automated ESP-IDF/NimBLE test proves live BLE write/read/notify behavior, payload MTU behavior, or status delivery to a real client.
- No fake-NVS failure-injection test proves last-known-valid retention at every candidate-write and active-slot promotion failure point.
- No broker-backed HIL smoke evidence confirms fresh-device BLE onboarding through first `gh/heartbeat` publish.

## Verification Performed

- `python -m unittest discover -s greenhouse-edge\tests -v`: passed, `Ran 10 tests` and `OK`.
- `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge`: passed.
- Noted warning: PlatformIO still reports the pre-existing flash-size mismatch, expected 4MB and detected 2MB. Firmware links within the custom app partition.

## Documentation Feedback Items

- Use `.agent-output/specs/edge-unit-onboarding/doc-feedback.md` items for missing Spec Control/status handling, BLE GATT UUID contract, NVS atomicity expectations, implementation artifact gating, and spec-branch workflow guidance.

## Recommended Next Handoff

- Status recommendation: passed; ready for maintainer review/merge workflow.
- Suggested hardening after merge: ESP-IDF fake-service tests for codec/NVS/retry failure injection plus one HIL smoke test covering BLE provisioning through first `gh/heartbeat`.
