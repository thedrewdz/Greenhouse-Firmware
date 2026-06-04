# Review Report: Edge Unit Onboarding

## Inputs Reviewed

- Canonical spec: `specs/edge-unit-onboarding/spec.md` from Greenhouse Documentation.
- Canonical docs: `AGENTS.md`, `CONTEXT.md`, `architecture.md`, `device-model.md`, `mqtt-topics.md`, `vision.md`.
- Local docs: `AGENTS.md`, `.github/copilot-instructions.md`, `docs/adr/0001-ble-first-onboarding.md`.
- Code diff: `origin/main...HEAD` on branch `spec/edge-unit-onboarding`.
- Test artifacts: `.agent-output/specs/edge-unit-onboarding/test-gap-report.md`.
- Verification command: `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge`.

## Blocking Findings

1. Bootstrap retry backoff starts at 2s instead of the canonical 1s.
   - Impacted files: `greenhouse-edge/src/services_network.c`, `greenhouse-edge/src/services_mqtt.c`.
   - Evidence: both services increment `s_retry_count` before scheduling the delay, and `compute_backoff_ms()` doubles once for `retry_count == 1`. The first failed WiFi/MQTT attempt therefore schedules roughly 2s plus/minus jitter, followed by 4s, 8s, and 16s.
   - Risk: violates the spec-required Phase 1 schedule of 1s, 2s, 4s, 8s for a five-attempt budget and may push normal onboarding outside expected timing.
   - Recommended fix: compute delay from completed failures minus one, or increment retry counters after scheduling, so the four inter-attempt delays are 1s, 2s, 4s, and 8s with plus/minus 20 percent jitter.

2. Re-entering Provisioning Mode after bootstrap failure does not stop or reset WiFi/MQTT runtime state.
   - Impacted files: `greenhouse-edge/src/app.c`, `greenhouse-edge/src/services_network.c`, `greenhouse-edge/src/services_mqtt.c`, `greenhouse-edge/src/services_network.h`, `greenhouse-edge/src/services_mqtt.h`.
   - Evidence: `enter_provisioning_mode()` only starts BLE advertising. The failed WiFi service remains started, the MQTT client remains allocated/started, and `s_started` in MQTT is never cleared. A later accepted provisioning payload calls `gh_network_start()` and `gh_mqtt_init()` over existing service state; then `gh_mqtt_start()` can return `ESP_OK` early because the old client is still marked started.
   - Risk: after a failed first onboarding attempt, the next valid BLE payload may not actually apply the new MQTT endpoint or restart bootstrap cleanly. This breaks the spec recovery rule that retry-budget exhaustion re-enters Provisioning Mode and waits for a new onboarding attempt.
   - Recommended fix: add explicit stop/reset APIs for WiFi and MQTT bootstrap state before entering Provisioning Mode, or make `start_network_bootstrap()` fully reinitialize existing service state and recreate/reconfigure the MQTT client for the new provisioning payload.

## Non-Blocking Findings

- None identified beyond the blocking items and test gaps.

## Architecture Boundary Concerns

- No Main Unit UI, cloud-first, or desktop/server implementation details were introduced.
- MQTT topic naming and payload field casing remain aligned with the canonical contract.
- BLE UUIDs remain a documentation coordination risk already captured in `doc-feedback.md`.

## Never Events (Auto-Blocking)

- None observed.

## Guardrail Update Required

- Existing `doc-feedback.md` already captures missing Spec Control/status handling and BLE GATT UUID documentation gaps. No additional guardrail item is required from this review.

## Test Gaps

- No automated unit tests cover provisioning payload parsing, canonical error-code mapping, retry budgets, backoff/jitter bounds, first-heartbeat publish failure handling, or NVS overwrite/retention behavior.
- No HIL smoke evidence confirms fresh-device BLE onboarding through first `gh/heartbeat` publish.
- Build verification passed, but it is not behavioral evidence for onboarding correctness.

## Documentation Feedback Items

- Use the existing `.agent-output/specs/edge-unit-onboarding/doc-feedback.md` items for missing Spec Control/status handling, BLE GATT UUID contract, NVS atomicity expectations, implementation artifact gating, and spec-branch workflow guidance.

## Recommended Next Handoff

- Status recommendation: `ready-for-implementation`.
- Next role: implementation agent to fix bootstrap retry timing and service reset/re-entry behavior, then test agent to add focused codec/retry tests plus HIL onboarding smoke evidence.
