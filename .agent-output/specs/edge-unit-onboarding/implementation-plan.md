# Implementation Plan: Edge Unit Onboarding

## Scope

- Implement Phase 1 BLE-first Edge Unit onboarding in ESP32 firmware.
- Accept WiFi credentials, Bootstrap MQTT Endpoint, device identity, schema version, and optional heartbeat interval over BLE.
- Validate provisioning payloads and return explicit BLE status responses using the canonical `200x` error codes.
- Persist accepted provisioning data to ESP32 NVS as one logical configuration update.
- Bootstrap WiFi and MQTT from persisted provisioning data.
- Publish the first heartbeat to `gh/heartbeat` after successful onboarding.
- Re-enter BLE Provisioning Mode when WiFi, MQTT, or first-heartbeat bootstrap retry budget is exhausted.

## Inputs

- `specs/edge-unit-onboarding/spec.md` from Greenhouse Documentation.
- Repository `AGENTS.md`.
- Repository `.github/copilot-instructions.md`.
- Canonical `device-model.md`.
- Canonical `mqtt-topics.md`.
- Local ADR `docs/adr/0001-ble-first-onboarding.md`.
- Local skill `docs/skills/esp32-wifi-mqtt-resilience.md`.
- Local skill `docs/skills/esp-idf-firmware-practices.md`.

## Proposed Changes

- Application layer: update `greenhouse-edge/src/app.c` to orchestrate provisioning state, accepted-payload handling, network bootstrap, first heartbeat completion, and re-entry to Provisioning Mode on bootstrap failure.
- Codec layer: update `greenhouse-edge/src/codec_json.c` and `greenhouse-edge/src/codec_json.h` to parse provisioning JSON and build BLE status JSON while keeping payload contracts out of transport services.
- Provisioning storage service: update `greenhouse-edge/src/services_provisioning_config.c` and `.h` to persist accepted WiFi, MQTT, and heartbeat interval values to NVS.
- BLE service layer: update `greenhouse-edge/src/services_ble_onboarding.c` and `.h` to expose the onboarding GATT service, writable provisioning payload characteristic, readable/notifiable status characteristic, and stop/re-advertise behavior.
- WiFi service layer: update `greenhouse-edge/src/services_network.c` and `.h` to track bounded bootstrap attempts, per-attempt timeout, exponential backoff, and jitter.
- MQTT service layer: update `greenhouse-edge/src/services_mqtt.c` and `.h` to track bounded bootstrap attempts, per-attempt timeout, exponential backoff, jitter, and first-heartbeat publish failure as MQTT bootstrap failure.
- Runtime configuration: update `greenhouse-edge/src/runtime_config.h` with bootstrap retry budget and attempt timeout defaults.

## Review Fix Pass

- Branch gate: confirmed current branch is `spec/edge-unit-onboarding` and `git pull --ff-only` reported already up to date before changes.
- Review finding 1: fix WiFi and MQTT bootstrap retry delay scheduling so the four inter-attempt delays for a five-attempt budget are `1s`, `2s`, `4s`, and `8s` with existing plus/minus 20 percent jitter.
- Review finding 2: add explicit WiFi and MQTT stop/reset paths and call them before entering BLE Provisioning Mode after bootstrap failure, so a later valid provisioning payload starts from clean service state and applies the new MQTT endpoint.
- Verification update: rerun `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge` after the review fixes.

## Risks and Mitigations

- Risk: BLE client payloads may exceed single-write limits depending on client MTU.
  Mitigation: bound payload size to 512 bytes and keep Phase 1 schema compact; future work can add prepared-write or chunking if Main Unit BLE client requires it.
- Risk: Persisting new config could partially update NVS if one write fails.
  Mitigation: write all keys then commit once so successful persistence is treated as one logical update.
- Risk: WiFi/MQTT retry behavior could block the main loop.
  Mitigation: keep retries time-based and non-blocking in service `tick` functions.
- Risk: Existing already-provisioned devices should not enter BLE mode on reboot.
  Mitigation: preserve boot-time NVS load path and only enter Provisioning Mode when required values are missing or bootstrap failure budget is exhausted.
- Risk: MQTT endpoint validation could reject valid local broker URIs without DNS dots.
  Mitigation: validate accepted URI schemes and non-empty authority rather than requiring dotted hostnames.

## Verification Plan

- Run `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge`.
- Confirm build succeeds with the ESP-IDF/NimBLE APIs used by the BLE GATT implementation.
- Review changed modules for canonical boundary alignment: app orchestration, codec parsing/serialization, BLE transport, NVS persistence, WiFi service, MQTT service.
- Hardware smoke test still recommended: erase NVS or use a fresh device, confirm `GH-Edge-{device_id}` advertises, write a valid provisioning payload, read/subscribe status response, confirm WiFi/MQTT connect, and confirm first heartbeat on `gh/heartbeat`.

## Handoff Notes

- Implemented behavior compiles successfully.
- Review fix pass completed on branch `spec/edge-unit-onboarding` after `git pull --ff-only` reported already up to date.
- Blocking review finding 1 addressed: WiFi and MQTT retry scheduling now derives backoff from completed failures minus one, preserving the canonical `1s`, `2s`, `4s`, `8s` inter-attempt schedule for a five-attempt budget while retaining jitter.
- Blocking review finding 2 addressed: WiFi and MQTT services now expose stop/reset APIs, MQTT init recreates the client for a new Bootstrap MQTT Endpoint, and app re-entry to BLE Provisioning Mode resets WiFi/MQTT runtime state before advertising.
- Verification command result after review fixes: `C:\Users\Andrew\.platformio\penv\Scripts\pio.exe run` from `greenhouse-edge` completed with `[SUCCESS]`.
- PlatformIO still reports the pre-existing flash-size warning: board profile expects 4MB and detects 2MB; the custom partition table remains compatible with the detected 2MB flash.
- No host/unit test harness exists in this repository for BLE GATT, NVS, WiFi, or MQTT service behavior, so verification is currently build-level plus recommended hardware smoke testing.
- Spec status was not updated because the fetched canonical spec does not include a Spec Control block in the implementation repository; this is captured in `doc-feedback.md`.
