# Agent Handoff

This file is for time-bound handoff state only.

For stable agent policy, coding rules, and skills selection, use AGENTS.md.

## Current Workspace State

- Path: D:\Code\Greenhouse\esp32-main
- PlatformIO ESP-IDF project now exists at greenhouse-edge/.
- Current scaffold includes:
   - greenhouse-edge/platformio.ini
   - greenhouse-edge/src/main.c
   - greenhouse-edge/src/CMakeLists.txt
- New firmware modules added in this session:
   - greenhouse-edge/src/app.h
   - greenhouse-edge/src/app.c
   - greenhouse-edge/src/app_types.h
   - greenhouse-edge/src/codec_json.h
   - greenhouse-edge/src/codec_json.c
   - greenhouse-edge/src/runtime_config.h
   - greenhouse-edge/src/services_network.h
   - greenhouse-edge/src/services_network.c
   - greenhouse-edge/src/services_mqtt.h
   - greenhouse-edge/src/services_mqtt.c
- New spec added in this session:
   - docs/spec-heartbeat-phase1-skeleton.md
   - docs/spec-edge-unit-onboarding-ble.md
- New ADR added in this session:
   - docs/adr/0001-ble-first-onboarding.md

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
- Onboarding strategy:
   - Phase 1 onboarding is BLE-first
   - Main Unit baseline is Raspberry Pi 4B BLE capability
   - Wired onboarding is optional fallback for recovery and manufacturing

## Current Progress Snapshot

- main entrypoint now calls app orchestrator:
   - gh_app_init()
   - gh_app_tick() on a periodic non-blocking loop (100 ms tick)
- codec module now supports:
   - command topic parsing for ghcmd/rd-{deviceId} and ghcmd/wr-{deviceId}
   - canonical command payload parsing and validation
   - canonical response payload JSON encoding
   - canonical heartbeat payload JSON encoding (with empty slots and capabilities arrays for now)
- app module now supports:
   - device_id derivation from MAC address
   - monotonic runtime message id generation
   - heartbeat payload generation at fixed interval and MQTT publish path
   - immediate heartbeat publish after MQTT reconnect
   - command topic and payload parse for inbound MQTT data (log-only action)
   - MQTT startup gating on WiFi-ready state to avoid pre-network false-negative connect errors
- network service now supports:
   - WiFi STA startup
   - IP event tracking
   - bounded reconnect retry with exponential backoff and jitter
- MQTT service now supports:
   - client startup and command topic subscriptions
   - bounded reconnect attempts with exponential backoff and jitter
   - publish wrapper for application layer
   - inbound message callback delivery to app layer
   - startup behavior that avoids immediate forced reconnect before network-driven MQTT lifecycle events
- Build status:
   - platformio build succeeds for env esp32dev
   - last validated runtime sequence: WiFi ready -> MQTT connected -> heartbeat publish to gh/heartbeat
   - heartbeat messages confirmed as received by Main Unit (Raspberry Pi Mosquitto subscriber on gh/#)
   - startup false-negative mitigated: no pre-network "Host is unreachable" MQTT error after readiness gating

## Known Gaps

- Slot discovery and I2C fault isolation not implemented yet.
- Canonical ack or error response publish path for inbound commands not implemented yet.
- Unit and host-mock tests not added yet.
- Flash size warning still appears in build output (board reports 4 MB while sdkconfig is 2 MB profile).
- BLE onboarding runtime implementation is not yet coded (docs completed first).
- Serial upload port may intermittently disappear on this workstation; reconnecting ESP32 restored successful flashing in the latest validation pass.

## Open Questions For The Next Pass

- Which exact ESP32 board target is Phase 1 baseline?
- What is the baseline flash and PSRAM profile for the chosen board?
- What telemetry schema versioning strategy should firmware enforce?
- Should nodes support mixed sensor and actuator slots in Phase 1 or later?
- What are hard realtime/safety constraints for actuator command execution?

## Next Actions

1. Implement canonical ack and error response publish path for inbound commands.
2. Add slot registry abstraction and invalid_slot validation path.
3. Add unit tests for codec parse and validation rules.
4. Add host-mock tests for reconnect behavior.
5. Decide and align flash profile to remove 2 MB vs 4 MB warning.
6. Implement BLE provisioning mode and onboarding payload exchange per docs/spec-edge-unit-onboarding-ble.md.

## Resume Prompt

Use this prompt when continuing work in this repo:

```text
Read AGENTS.md and docs/agent-handoff.md, then implement command ack/error publish flow for ghcmd traffic (gh/ack and gh/rd), including invalid_slot and invalid_payload handling with stable GH_ERR_* codes.
```
