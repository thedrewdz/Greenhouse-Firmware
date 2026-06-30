# Local Skill Packs

This folder contains local, implementation-focused skill packs for the Greenhouse ESP32 edge firmware repository.

These packs are tool-neutral: they work for both OpenAI Codex (via `AGENTS.md`) and Claude / Claude Code (via `CLAUDE.md`).

They are **supplemental**. The canonical skills, context, architecture, and contracts live in the Greenhouse Documentation repository:

- https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

Review the relevant canonical skill guidance first, then apply these repository-local packs for ESP32 and ESP-IDF specifics. Local packs must never override canonical policy, architecture, contracts, or terminology.

## Partial Disclosure

Load only the pack that matches the current task. Do not read all packs up front. Each pack states when to use it and when not to.

## Packs

| Pack | Use when |
|---|---|
| `esp32-firmware-architecture.md` | Creating firmware modules, separating hardware from logic, or designing startup/runtime/fault flows. |
| `embedded-oo-coding-standards.md` | Writing or refactoring C/C++ modules, or reviewing code for design and quality. |
| `esp32-application-state-model.md` | Designing the top-level lifecycle state machine, reconciling subsystem state, or projecting state into the heartbeat. |
| `esp32-peripheral-registry-extensibility.md` | Adding support for a new peripheral type, or removing type-specific branches from the application layer. |
| `embedded-resource-budgets.md` | Judging memory/flash/stack impact, choosing static vs dynamic allocation, or checking a design for bloat/over-engineering. |
| `esp-idf-firmware-practices.md` | Creating/refactoring ESP-IDF components, tasks, drivers, or defining file structure. |
| `esp-idf-testing-strategy.md` | Adding tests, or changing command/telemetry/heartbeat, I2C, WiFi, MQTT, or task logic. |
| `esp32-i2c-bus-reliability.md` | Building I2C discovery/slot probing, adding module drivers, or debugging bus faults. |
| `esp32-wifi-mqtt-resilience.md` | Implementing network bootstrap/reconnect, heartbeat/telemetry/command flows, or outage handling. |
| `esp32-ble-provisioning.md` | Working on BLE Provisioning Mode, the GATT onboarding interface, or the onboarding lifecycle. |
| `esp32-nvs-provisioning-persistence.md` | Reading/writing provisioning config in NVS, schema versioning, factory reset, or migration. |

## Gaps

If a documentation, knowledge, or skill gap is identified, do not make things up - bring it to the user's attention to be addressed properly, per `AGENTS.md`.
