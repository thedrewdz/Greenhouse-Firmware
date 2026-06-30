# Skill: ESP32 Application State Model

## Purpose

Guide coding agents to implement a single, explicit Edge Unit application state machine and consistent runtime state tracking, so the device's top-level behavior is deterministic and its heartbeat reflects reality.

This is the application-layer orchestration referenced by `esp32-firmware-architecture.md`. It unifies the independent subsystem state machines (WiFi/MQTT, slot modules) under one authoritative lifecycle. The device lifecycle phases, onboarding flow, and heartbeat contents are owned by the Greenhouse Documentation repository; read those first, then apply this device-side state-modeling discipline.

- Canonical entry point: https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

## Use This Skill When

- Implementing or refactoring the Edge Unit's top-level orchestration loop.
- Adding or changing lifecycle states (for example provisioning, degraded operation, recovery, OTA).
- Reconciling subsystem state (WiFi, MQTT, slots) into one coherent device state.
- Projecting current state into the heartbeat payload.

## Do Not Use This Skill When

- The task is subsystem-internal reconnect mechanics (use `esp32-wifi-mqtt-resilience.md`).
- The task is I2C fault detection detail (use `esp32-i2c-bus-reliability.md`).
- The task is defining the heartbeat payload contract or lifecycle vocabulary (those are canonical; raise gaps against the Greenhouse Documentation repository).

## State Machine Rules

- Maintain one authoritative application state machine that owns the top-level lifecycle. Enumerate states explicitly (for example: Unprovisioned, Provisioning, Booting, Connecting, Operational, Degraded, Recovering, Fault). Do not represent lifecycle through ad hoc booleans scattered across modules.
- Keep subsystem state machines (WiFi, MQTT, slot modules) independent and authoritative for their own concern. They report their state upward; the application state is a deterministic function of those subsystem states, never a parallel guess that can drift.
- Centralize transitions. Transitions are driven by explicit events (subsystem connected/lost, command received, configuration applied, fault raised). Do not duplicate transition logic across modules.
- Keep the state model small and deterministic. Model only states the firmware actually behaves differently in; do not add states you never act on (see `embedded-resource-budgets.md`).

## Behavior-Per-State Rules

- Define what each state permits. For example: publish telemetry only when Operational; keep heartbeat flowing whenever the transport allows; hold actuators in their safe state during Degraded, Recovering, and Fault.
- Distinguish Degraded from Fault:
  - Degraded = partial capability (one slot down, or MQTT down while WiFi is up). Continue operating safely and keep reporting.
  - Fault = unsafe or unrecoverable-in-place. Enter safe state and attempt bounded recovery (reconnect, watchdog) without manual intervention.
- Recovery is automatic and bounded: tolerate WiFi/MQTT outages, Main Unit restarts, and power interruptions per the canonical offline-recovery expectations, using bounded retries with backoff.

## Runtime vs Persisted State Rules

- In Phase 1 the Main Unit is the source of truth for configuration; Edge Units do not persist long-term configuration. Keep operational/runtime state (connection status, slot fault history, current readings) in RAM.
- Do not persist transient connection state. Only provisioning data is persisted, and that is owned by `esp32-nvs-provisioning-persistence.md`.

## Heartbeat Projection Rules

- The heartbeat must reflect current application state together with subsystem, slot, and fault state — consistently. A read of telemetry and the heartbeat's slot state must not disagree.
- Implement the projection from internal state to heartbeat payload as one pure function so it is unit-testable and cannot drift between code paths. Keep its output aligned with the canonical heartbeat contents.

## Validation Checklist

- Exactly one authoritative application state machine exists; lifecycle is not encoded in scattered booleans.
- Application state is derived from subsystem states deterministically, with no parallel/duplicated tracking.
- Transitions are centralized and event-driven; behavior per state is explicit.
- Actuators hold safe state in Degraded, Recovering, and Fault; recovery is automatic and bounded.
- No transient runtime state is persisted; only provisioning data is.
- Heartbeat is produced by a single pure projection function and is consistent with reported telemetry.
- State transitions and heartbeat projection have unit tests (per `esp-idf-testing-strategy.md`).
