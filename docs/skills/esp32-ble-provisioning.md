# Skill: ESP32 BLE Provisioning

## Purpose

Guide coding agents to implement and maintain the Edge Unit BLE-first onboarding flow on ESP32 using NimBLE: unprovisioned Provisioning Mode advertising, the GATT provisioning interface, and the receive-validate-persist-transition lifecycle.

This is the firmware side of a contract shared with the Main Unit. The canonical provisioning payload and status contract is owned by the Greenhouse Documentation repository (onboarding spec, device model). Read that contract first; this pack covers how to implement it on the device, not how to define it.

- Canonical entry point: https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md
- Local decision record: `docs/adr/0001-ble-first-onboarding.md`

## Use This Skill When

- Implementing or changing `services_ble_onboarding.*`.
- Adding or modifying GATT characteristics for provisioning payload delivery or status reporting.
- Changing how the device enters or exits Provisioning Mode, or how it transitions to WiFi/MQTT startup after onboarding.
- Debugging advertising, connection, or GATT write/notify behavior during onboarding.

## Do Not Use This Skill When

- The task is normal operational WiFi/MQTT connectivity after provisioning (use `esp32-wifi-mqtt-resilience.md`).
- The task is only persisting accepted provisioning data (use `esp32-nvs-provisioning-persistence.md`).
- The task is defining the provisioning payload contract itself (that is canonical; raise it against the Greenhouse Documentation repository).

## Provisioning Mode Lifecycle

1. On startup, attempt to load persisted provisioning config from NVS.
2. If required values are present, skip BLE and start WiFi then MQTT.
3. If required values are missing, enter Provisioning Mode and start BLE advertising.
4. Accept a provisioning payload over the GATT write characteristic.
5. Validate, persist as one logical configuration update, and report status.
6. On success, stop advertising and transition to WiFi/MQTT startup.
7. On error, keep advertising and report a specific failure status so the Main Unit can retry.

## BLE / NimBLE Rules

- Use the NimBLE host: initialize with `nimble_port_init`, run the host on its own FreeRTOS task via `nimble_port_freertos_init`, and infer the advertising address type on the sync callback.
- Put the 128-bit onboarding service UUID in the primary advertisement; put the device name (`GH-Edge-{device_id}`, device id truncated to 12 chars) in the scan response.
- Keep the GATT layout stable: a primary onboarding service with a write characteristic for the inbound payload and a read+notify characteristic for status.
- Re-arm advertising on disconnect, failed connect, and advertise-complete so the node stays discoverable until provisioned.
- Stop advertising only after a successful, persisted provisioning result.

## GATT Payload Handling Rules

- Bound the inbound payload size; reject empty and oversized writes with an explicit status instead of allocating unbounded buffers.
- Copy the mbuf to a flat, NUL-terminated buffer before parsing, and free it on every path.
- Reject writes to the status characteristic; it is read/notify only.
- Always set and notify a status after handling a write, both on success and on every failure branch.

## Boundary and Decoupling Rules

- Keep the BLE layer transport-only: it advertises, receives bytes, and reports status. It must not parse JSON or write NVS directly.
- Delegate payload handling through a callback (`gh_ble_onboarding_payload_cb_t`) so parsing, validation, and persistence live in their owning modules. This keeps the onboarding service testable and single-responsibility.
- Map all outcomes to the shared provisioning status type, then let the codec render the status JSON.

## Status and Error Reporting Rules

- Report a stable `result` plus a status/error code and a short human-readable message for every handled write.
- Preserve the canonical status code meanings (for example: unsupported schema version, device id mismatch, missing WiFi SSID, invalid MQTT broker URI, internal persistence error). Do not invent overlapping codes; extend the contract through the documentation repository.
- Keep error messages bounded and free of secrets (never echo the WiFi password).

## Security and Safety Baseline

- Treat Provisioning Mode as an unauthenticated entry surface for Phase 1; keep it active only while unprovisioned and shut it down immediately after success.
- Do not persist or act on a payload whose schema version or target device id does not match this device.
- Security hardening (pairing, encryption, payload authentication) is deferred to follow-on specs and ADRs; do not assume it exists, and do not weaken existing validation.

## Validation Checklist

- Unprovisioned boot enters Provisioning Mode and advertises as `GH-Edge-{device_id}`.
- A valid payload is accepted exactly once, persisted, acknowledged with success, and advertising stops.
- Invalid schema version, device id mismatch, missing/invalid required fields, and oversized payloads each produce a specific error status and leave the device advertising.
- Disconnect mid-onboarding returns the device to advertising without reboot.
- No JSON parsing or NVS access leaks into the BLE transport layer.
