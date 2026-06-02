# Agent Handoff

This file is for local, time-bound session state only.

Durable policy, canonical context, architecture, MQTT contracts, ADRs, and skill guidance live in the Greenhouse Documentation repository:

- https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

## Current Workspace State

- Path: `D:\Code\Greenhouse\esp32-main`
- Repository purpose: ESP32 Edge Unit firmware.
- Current branch: `main`
- Last completed commit before this handoff update: `cc43de4 Move durable docs to documentation repo`
- Local documentation has been removed from this repository except:
  - `README.md`
  - `AGENTS.md`
  - `.github/copilot-instructions.md`
  - `agent-handoff.md`

## Current Progress Snapshot

- Implemented the start of the Edge Unit BLE-first onboarding flow.
- Removed `greenhouse-edge/platformio.local.ini` and removed build-time WiFi/MQTT credential injection from `platformio.ini` and `runtime_config.h`.
- Startup now initializes NVS, loads provisioning data from namespace `gh_prov`, and uses NVS values for:
  - `wifi_ssid`
  - `wifi_pass`
  - `mqtt_uri`
  - optional `hb_ms`
- If required WiFi and Bootstrap MQTT Endpoint values are present, the firmware starts WiFi and then MQTT using the existing non-blocking retry flow.
- If provisioning values are missing, the firmware enters BLE Provisioning Mode.
- Added BLE onboarding advertising using NimBLE:
  - custom 128-bit Greenhouse onboarding service UUID in the primary advertisement
  - device name `GH-Edge-{device_id}` in the scan response
  - inferred NimBLE address type for advertising
- Added a custom 2 MB-friendly partition table because enabling BLE made the firmware exceed the default 1 MB app partition.
- PlatformIO upload to the connected ESP32 succeeded on `COM3`.
- BLE smoke test passed: the device was visible to the user as `GH-Edge-10061CB5C3C4`.

## Open Questions

- BLE provisioning payload receive/write flow is not implemented yet; current onboarding work only starts the unprovisioned advertising path.
- The ESP32 board profile still reports a 4 MB expected flash size while upload detects 2 MB. The custom partition table is sized for the detected 2 MB flash.

## Next Actions

- Implement the BLE GATT provisioning payload contract from the Greenhouse Documentation onboarding spec.
- Validate payload fields, persist accepted provisioning data to NVS as one logical configuration update, and transition to WiFi/MQTT startup after successful provisioning.
- Consider adding a factory reset or forced re-entry path for Provisioning Mode when the documentation decision is settled.

## Resume Prompt

```text
Read AGENTS.md, agent-handoff.md, and the Greenhouse Documentation README, then continue ESP32 Edge Unit firmware work using the relevant external onboarding, device model, and MQTT documentation.
```
