# Task: Edge Firmware Standards & Skills Remediation

## Control

- Status: ready-for-triage
- Created: 2026-06-30
- Owner: TBD (maintainer)
- Scope: `greenhouse-edge/` firmware in the `edge` repository
- Source: full code-vs-documentation/skills evaluation (see Findings)

## Purpose

The Phase 1 BLE onboarding slice works and passed its scoped review, but the codebase
diverges from the repository's own documented engineering standards in three first-class
ways and several smaller ones. This task captures everything needed to bring the firmware
up to the bar defined in `AGENTS.md` and `docs/skills/*`, so the divergences are fixed
before the peripheral/command layers are built on top of the current orchestration.

This is repository-internal remediation, not a canonical spec. Canonical contracts
(MQTT topics, device model, heartbeat fields, provisioning payload) remain owned by the
Greenhouse Documentation repository; align with it before changing any wire contract and
do not let this task redefine canonical data shapes.

## Precedence reminder

Per `AGENTS.md` / `CLAUDE.md`: canonical docs win over local skills; skills are supplemental.
Where a work item below touches a wire contract (command ack shape, heartbeat fields,
schema version), confirm against the Documentation repo first; the skill citations here are
the device-side implementation discipline, not the contract authority.

---

## Decision (resolved 2026-06-30) — Option A: migrate to C++

**Chosen: Option A.** The application, service, and device-abstraction layers in
`greenhouse-edge/` migrate to C++ (`.cpp`/`.hpp`), matching the literal `AGENTS.md` Coding
Standards and `embedded-oo-coding-standards.md` mandate. No C-with-discipline shortcut and no
ADR-justified deviation for these layers. C remains acceptable only for a genuine low-level
driver/HAL if one is later added under this repo.

Scope nuance (owner): C-style procedural code is only justified for a **trivial peripheral
unit that is effectively a single `main.cpp`** and does not warrant interface/DI overhead.
That case belongs to the **Peripherals repository**, not this edge firmware — so within
`greenhouse-edge/`, Option A applies across the board.

WI-1 and WI-2 are language-agnostic and can start immediately; WI-3 now proceeds as a C++
migration.

---

## Work items

Severity: **P1** = explicit named-rule violation; **P2** = real gap, narrower or partly phase-deferred.

### WI-1 (P1) — Establish a host-runnable test harness that exercises real firmware code

**Standard:** `esp-idf-testing-strategy.md` (Unity unit tests + host-side mock tests;
"inject dependencies for transport, clock, and storage"; "avoid global singleton state
where test isolation is required"). `embedded-oo-coding-standards.md` testing rules.

**Current state:** The only tests, [test_onboarding_contract.py](../../greenhouse-edge/tests/test_onboarding_contract.py),
either reimplement the parser in Python ([lines 39-82](../../greenhouse-edge/tests/test_onboarding_contract.py#L39-L82))
or assert on the *text* of `.c` files ([lines 107-157](../../greenhouse-edge/tests/test_onboarding_contract.py#L107-L157)).
No test compiles or runs firmware code. `test-gap-report.md` and the prior `review-report.md` both acknowledge this.

**Required:**
- Stand up a host-build test target (ESP-IDF `unity` component for on-target/QEMU unit
  tests, and/or a host `cmake`+native compile of the pure modules) that links and runs the
  actual C from `src/`.
- Port the existing contract assertions to execute `gh_codec_*` directly (delete the Python
  reimplementation; keep Python only as an optional black-box harness if still useful).
- Make codec parse/validate, backoff math, and (after WI-4) heartbeat projection covered by
  tests that call the real functions, with table-driven positive/negative cases.

**Acceptance:**
- `gh_codec_parse_provisioning_payload`, `gh_codec_parse_command_payload`, and the backoff
  computation are exercised by compiled tests, not reimplementations or text matches.
- CI/local command documented; both positive and negative cases per canonical error code.
- No remaining test asserts on source-file text shape.

**Notes:** Start here — it is independently valuable, low-risk, and it is the safety net for
WI-2/WI-3/WI-4. It also forces the first DI seams (clock, transport) into existence.

---

### WI-2 (P1) — Introduce one explicit application state machine

**Standard:** `esp32-application-state-model.md`: "Maintain one authoritative application
state machine… Enumerate states explicitly (Unprovisioned, Provisioning, Booting,
Connecting, Operational, Degraded, Recovering, Fault). **Do not represent lifecycle through
ad hoc booleans scattered across modules.**" Centralized, event-driven transitions.

**Current state:** Lifecycle lives in scattered statics in
[app.c:28-33](../../greenhouse-edge/src/app.c#L28-L33) (`s_provisioning_mode`,
`s_mqtt_started`, `s_network_initialized`, `s_first_heartbeat_published`,
`s_last_mqtt_connected`, `s_waiting_for_network_logged`). No state enum; transition logic is
inline in [gh_app_tick](../../greenhouse-edge/src/app.c#L253-L305).

**Required:**
- Define an explicit `gh_app_state_t` enum modelling only states the firmware behaves
  differently in (do not add states you never act on — see `embedded-resource-budgets.md`).
- Application state is a deterministic function of subsystem states (WiFi/MQTT report
  upward); no parallel/duplicated tracking. Centralize transitions; drive them by events
  (subsystem connected/lost, config applied, bootstrap budget exhausted, fault raised).
- Define behavior-per-state, including actuator fail-safe holding in Degraded/Recovering/Fault
  (`AGENTS.md` Firmware Runtime Rules; `esp32-application-state-model.md`).
- Replace the scattered booleans with the state machine + minimal subsystem-reported flags.

**Acceptance:**
- Exactly one authoritative state machine; lifecycle not encoded in scattered booleans.
- Transitions centralized and unit-tested (via WI-1) for the onboarding/bootstrap/degraded paths.
- Behavior-per-state is explicit and documented in-code.

**Depends on:** WI-1 (so transitions are testable). Should land before WI-3/WI-4.

---

### WI-3 (P1) — Migrate service/application layers to C++ with interfaces + DI (Option A)

**Standard:** `AGENTS.md` Coding Standards + `embedded-oo-coding-standards.md`
(C++ for application/service/device-abstraction layers; small interfaces; DI across
module/hardware boundaries; `.cpp`/`.hpp` vs `.c`/`.h` per `esp-idf-firmware-practices.md`).
`esp32-firmware-architecture.md` (constructor-based DI; app layer must not call drivers directly).

**Current state:** Entire codebase is `.c` (zero `.cpp`/`.hpp`); every module is a
service/application-layer module using file-static singletons and direct cross-module calls
(e.g. [services_mqtt.c:17-29](../../greenhouse-edge/src/services_mqtt.c#L17-L29),
[services_network.c:21-28](../../greenhouse-edge/src/services_network.c#L21-L28)). No opaque
types, no interfaces, no DI. Fails both the letter (C++) and the spirit (OO-in-C escape hatch).

**Required (Option A — C++):**
- Convert the application/service/device-abstraction modules to C++ (`.cpp` implementations,
  `.hpp` headers); keep `extern "C"` shims only where ESP-IDF callbacks require C linkage.
- Express each service as a small C++ interface (transport client, clock/time provider, config
  store) with the concrete implementation injected at the composition root
  (`gh_app_init` / `app_main`) via constructor-based DI.
- Remove file-static singleton state in favor of an owned instance/handle passed in.
- Ensure the application layer programs to interfaces only and never calls a low-level
  driver API directly.
- Mind embedded C++ cost (`embedded-resource-budgets.md`): avoid RTTI/exceptions/iostreams,
  keep virtual dispatch deliberate, prefer fixed buffers; verify flash/RAM margin from the
  build output after migration.

**Acceptance:**
- Service/app/device-abstraction code is C++ with `.cpp`/`.hpp`; C remains only in genuine
  low-level driver/HAL (none today).
- Service dependencies are injected, not reached through globals; app layer has no direct
  driver calls.
- Transport/clock/storage are mockable, and WI-1 tests use those mocks for orchestration tests.
- Flash/RAM margin after migration is known and acceptable.

**Depends on:** WI-1 + WI-2 in place so migration targets a clean shape. Largest item —
split per layer (codec → services → app) to keep each change reviewable and buildable.

---

### WI-4 (P2) — Complete the command path and make the heartbeat a pure projection

**Standard:** `esp32-wifi-mqtt-resilience.md` ("Always publish an ack or error response for
handled commands"; "Reject commands targeting unknown slots"; validate write fields).
`esp32-application-state-model.md` (heartbeat produced by "one pure function so it is
unit-testable and cannot drift"; reflects subsystem/slot/fault state).

**Current state:**
- [`on_command_received`](../../greenhouse-edge/src/app.c#L53-L79) parses then only logs;
  it never executes or acknowledges. `gh_codec_build_response_payload` is **dead code**
  (defined/declared in [codec_json.c:237](../../greenhouse-edge/src/codec_json.c#L237) /
  [codec_json.h:16](../../greenhouse-edge/src/codec_json.h#L16), never called). No
  unknown-slot rejection.
- [`publish_heartbeat`](../../greenhouse-edge/src/app.c#L81-L113) builds the payload inline;
  `slot_count` hardcoded 0, empty `slots`/`capabilities`, no fault codes.

**Required:**
- Implement command handling: validate, act (or no-op safely until the device layer lands),
  and **always publish an ack/error response** on the canonical response topic using the
  existing response codec. Reject unknown slots with the canonical error code.
- Extract heartbeat construction into a single pure projection function
  (`internal state -> heartbeat payload`) that WI-1 can unit-test; wire slot/fault state once
  the device layer exists (WI-6).

**Acceptance:**
- Every handled command yields an ack/error response (verified by a host-mock transport test).
- `gh_codec_build_response_payload` is reachable and covered, or removed if intentionally deferred.
- Heartbeat projection is one pure, unit-tested function consistent with reported state.

**Depends on:** WI-1, WI-2 (state machine feeds the projection). Slot/fault content depends on WI-6.

---

### WI-5 (P2) — Persist and check a provisioning schema version in NVS

**Standard:** `esp32-nvs-provisioning-persistence.md`: "Stamp persisted configuration with a
schema version and check it on load… provide an explicit migration or rejection path for
older records; never silently misinterpret old layouts."

**Current state:** Schema version is validated only on the inbound BLE payload
([codec_json.c:187-192](../../greenhouse-edge/src/codec_json.c#L187-L192)). The persisted
[`gh_provisioning_config_t`](../../greenhouse-edge/src/services_provisioning_config.h#L17-L22)
has no version field, and load ([services_provisioning_config.c:58-89](../../greenhouse-edge/src/services_provisioning_config.c#L58-L89))
performs no version gate. The A/B slot atomicity itself is well done — this is the one missing piece.

**Required:**
- Store the schema version alongside the config (or in the meta namespace) and check it on
  load; on mismatch, take an explicit migration-or-reject path rather than reinterpreting bytes.
- Keep the legacy-namespace fallback until any migration is proven.
- Confirm the canonical NVS atomicity/versioning expectation with the Documentation repo
  (open item in `doc-feedback.md` #3) and align.

**Acceptance:**
- A stored config carries a schema version; load rejects/migrates unknown versions deterministically.
- Power-loss A/B retention behavior preserved; covered by a fake-NVS failure-injection test (WI-1).

---

### WI-6 (P2) — Device-abstraction layer: peripheral registry + I2C reliability (likely its own spec)

**Standard:** `esp32-peripheral-registry-extensibility.md` (capability interfaces +
static `{capability -> factory}` registry; no per-type branches in app layer; unknown
capability => explicit fault module). `esp32-i2c-bus-reliability.md` (one I2C manager,
bounded retries, per-slot fault isolation, bus recovery, slot state in heartbeat).

**Current state:** Entirely absent. `slot_count` stubbed to 0; no I2C code; actuator
fail-safe is therefore untested (no actuators yet).

**Required:** Build the device-abstraction layer per the two skills when the platform reaches
that phase. This unblocks the slot/fault content of WI-4's heartbeat projection.

**Acceptance:** Per the two skills' validation checklists. **Recommend tracking as a
separate spec/task** — listed here for completeness and dependency visibility, not to be
done inside this remediation pass unless explicitly pulled in.

---

### WI-7 (P3) — Structural / minor

- **ESP-IDF components:** `esp-idf-firmware-practices.md` wants components with ownership
  boundaries (connectivity / mqtt transport / codec / …); current build is one component via
  `FILE(GLOB_RECURSE)` in [src/CMakeLists.txt](../../greenhouse-edge/src/CMakeLists.txt#L4).
  Reorganize into components (pairs naturally with WI-3's interface boundaries).
- **Float vs fixed-point:** command `value` is `double`
  ([app_types.h:27](../../greenhouse-edge/src/app_types.h#L27)); `embedded-resource-budgets.md`
  prefers integer/fixed-point where the contract allows (ESP32 FPU is single-precision; `double`
  is soft-float). Revisit only if the canonical contract permits and measurement justifies it.
- **Flash size:** pre-existing PlatformIO 4MB-vs-2MB warning noted in prior reports; confirm the
  partition table / board profile intent.

---

## Recommended sequence

1. **WI-1** (test harness) — safety net + first DI seams.
2. **WI-2** (state machine) — testable via WI-1.
3. **WI-3** (C++ migration + DI, Option A), split per layer (codec → services → app).
4. **WI-4** (command ack + heartbeat projection) and **WI-5** (NVS schema version) — parallelizable.
5. **WI-7** structural items (fold into WI-3 where natural).
6. **WI-6** device layer — separate spec; do last / on phase change.

## Cross-cutting quality gates (apply to every WI)

From `AGENTS.md` Quality Gates + `esp-idf-testing-strategy.md`:
1. Single-purpose modules; small, testable interfaces.
2. WiFi/MQTT/I2C fault paths explicitly handled.
3. Aligned with canonical device model + MQTT contracts; no wire-contract change without
   Documentation-repo alignment.
4. No Main Unit / cloud / desktop detail leaks into firmware.
5. Tests added/updated at the correct layer (unit / host-mock / HIL) for changed behavior,
   and they execute firmware code.
6. Resource budgets respected: no new heap in hot paths/ISRs; bounded buffers; justify each
   new abstraction; read flash/RAM margin from build output.

## Verification

- Host/unit tests (WI-1 target) run and pass, exercising real firmware code.
- `pio run` from `greenhouse-edge/` builds clean (note the known flash-size warning).
- For onboarding/bootstrap-affecting changes, the HIL smoke checklist in
  `esp-idf-testing-strategy.md` is completed for release candidates.

## Out of scope

- Redefining any canonical wire contract (raise against the Documentation repo).
- Security hardening of Provisioning Mode (pairing/encryption) — deferred per
  `esp32-ble-provisioning.md` and follow-on ADRs.
- Building the peripheral/I2C layer (WI-6) unless explicitly pulled in.

## Reference: what is already good (do not regress)

BLE onboarding transport/callback separation; NVS A/B slot promotion with legacy fallback;
WiFi/MQTT as independent non-blocking state machines with bounded backoff + jitter and
resubscribe-on-connect; fresh heartbeat forced on reconnect; codec centralization.
