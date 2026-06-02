# Agent Handoff

## Repository Purpose

This repository is for Greenhouse peripheral firmware on ESP32 devices.

Current root README states:
- project name: Greenhouse-Firmware
- purpose: firmware for peripheral units

This is not the .NET control-unit application repository.

## Current Workspace State

- Path: D:\Code\Greenhouse\esp32-main
- Present at root: .git, .gitignore, README.md, LICENSE, docs/
- Firmware source folders are not yet present in this workspace snapshot.
- Documentation has been pruned to firmware-relevant scope.

## Confirmed Technical Decisions

- Firmware framework: ESP-IDF
- MQTT client: esp-mqtt
- JSON codec library: cJSON
- Firmware architecture: layered modules with OO design and interface-first boundaries
- Source organization rule:
   - C++: .hpp or .h for declarations, .cpp for implementations
   - C: .h for declarations, .c for implementations
- Payload strategy:
   - Phase 1 uses JSON via cJSON
   - codec boundary is required to keep transport independent from payload parsing
   - Phase 2 may evaluate compact binary formats only with measured evidence

## Agent Guidance Baseline

Use and follow:

- .github/copilot-instructions.md
- docs/skills/esp32-firmware-architecture.md
- docs/skills/esp32-i2c-bus-reliability.md
- docs/skills/esp32-wifi-mqtt-resilience.md
- docs/skills/esp-idf-firmware-practices.md
- docs/skills/esp-idf-testing-strategy.md
- docs/skills/embedded-oo-coding-standards.md

## Documentation Scope Guidance

Keep this repo's docs focused on firmware concerns:
- ESP32 hardware assumptions
- peripheral node roles (sensor and actuator nodes)
- telemetry payload formats and constraints
- command handling and safety/failsafe behavior
- connectivity, retry, and offline buffering strategy
- OTA update strategy for peripheral firmware
- topic naming and message schema from the firmware perspective

Avoid mixing in control-unit/UI implementation details (Blazor routes, .NET service wiring, desktop/server deployment specifics).

## Practical Resume Checklist

1. Confirm you are in this repo:

```powershell
git rev-parse --show-toplevel
git branch --show-current
git status --short
```

2. Validate docs are aligned to firmware scope:
- docs/agent-handoff.md
- docs/device-model.md
- docs/mqtt-topics.md
- docs/architecture.md

3. Before coding, read the implementation constraints:
- .github/copilot-instructions.md
- docs/skills/README.md

## Immediate Next Work (Code Generation)

1. Initialize ESP-IDF firmware project structure for peripheral node implementation.

2. Create initial module boundaries with header and source separation:
- app: startup orchestration and non-blocking main loop scheduling
- services/network: WiFi connection state machine
- services/messaging: esp-mqtt connection, subscribe, publish
- services/heartbeat: heartbeat cadence and payload composition
- codec/json: cJSON encode and decode plus payload validation
- hal/i2c: shared bus access and module probing
- modules/slots: slot state registry and fault isolation logic

3. Implement first vertical slice:
- boot
- connect WiFi
- connect MQTT
- publish heartbeat to gh/heartbeat
- subscribe to ghcmd/rd-{deviceId} and ghcmd/wr-{deviceId}
- parse commands via codec layer
- publish canonical ack and read responses

4. Add tests in parallel with implementation:
- unit tests for codec parse and validation
- host-mock tests for reconnect and slot-fault isolation behavior
- define a minimal HIL smoke checklist for boot and heartbeat validation

## Open Questions For The Next Pass

- Which exact ESP32 board target is Phase 1 baseline?
- What is the baseline target board and flash/PSRAM profile?
- What telemetry schema versioning strategy should firmware enforce?
- Should nodes support mixed sensor and actuator slots in Phase 1 or later?
- What are hard realtime/safety constraints for actuator command execution?

## First Coding Session Deliverables

- Project skeleton with compile-ready module structure
- Buildable startup path that publishes one valid heartbeat
- Command topic subscriptions wired through codec boundary
- Initial error-code mapping for invalid payload and invalid slot
- Test scaffold with at least one unit test and one host-mock test

## Resume Prompt

Use this prompt when continuing work in this repo:

```text
Read docs/agent-handoff.md, then generate the initial ESP-IDF firmware code skeleton and first heartbeat plus command-handling vertical slice using esp-mqtt and cJSON with strict header/source separation.
```
