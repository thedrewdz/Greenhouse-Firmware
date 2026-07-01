# HIL smoke: fresh-device onboarding through first heartbeat (#17)

Hardware-in-loop smoke test. Unlike the host tests under
[`tests/host/`](../host/README.md), this **must be run on a real ESP32 device**
against a real MQTT broker — it validates the BLE/NimBLE, WiFi, and MQTT paths
end to end, which the host build deliberately does not.

Run this for every release candidate that touches onboarding, connectivity, or
the bootstrap/heartbeat path (per `esp-idf-testing-strategy.md`).

## Prerequisites

- A flashable ESP32 dev board (record the board revision).
- ESP-IDF / PlatformIO toolchain: `pio run -t upload -t monitor` from
  `greenhouse-edge/`.
- An MQTT broker reachable from the device's WiFi (record the `mqtt(s)://` URI).
- A BLE provisioning client able to write the provisioning GATT characteristic
  (e.g. nRF Connect) and subscribe to the status characteristic.
- Known-good provisioning payload (schema_version 1) with the **device's own**
  `device_id` (the `GH-Edge-{id}` suffix), valid `wifi_ssid` / `wifi_password`,
  and the broker URI.

## Procedure & checklist

| # | Step | Expected | Pass/Fail |
|---|---|---|---|
| 1 | Erase flash, upload firmware, open monitor. | Boots; no provisioning config found → enters BLE Provisioning Mode. | |
| 2 | Scan for BLE peripherals. | Device advertises as `GH-Edge-{id}` (id = its MAC, uppercase hex). | |
| 3 | Write an **oversized** value (> characteristic max) to the provisioning characteristic. | Write rejected; status characteristic reports an error; device stays in Provisioning Mode. | |
| 4 | Write an **empty** value. | Write rejected; error status; still provisioning. | |
| 5 | Write a payload with a mismatched `device_id`. | Status error `2002` (device_id mismatch); still provisioning. | |
| 6 | Write the valid provisioning payload. | Status characteristic notifies `success` **before** advertising stops; config persisted to NVS. | |
| 7 | Observe WiFi bring-up. | Device associates and gets an IP within the connect timeout. | |
| 8 | Observe MQTT bring-up. | Client connects to the broker and subscribes to `ghcmd/rd-{id}` and `ghcmd/wr-{id}`. | |
| 9 | Observe first heartbeat. | A message is published to `gh/heartbeat` with the correct `device_id`, `firmware_version`, and a plausible `wifi_rssi`. | |
| 10 | Power-cycle the device. | Loads persisted config from NVS (no re-provisioning) and republishes `gh/heartbeat`. | |

## Record for the release candidate

- Firmware version (from the heartbeat payload / build):
- Board revision:
- Broker URI:
- Date / timestamp (UTC):
- Tester:
- Result (all steps pass?): 
- Notes / anomalies:

## Relationship to the host tests

The host suite already covers the codec parse/validate, backoff schedule, and
the WiFi/MQTT/NVS service state machines against fakes. This HIL smoke is the
only place the **real radios, NimBLE GATT layer, and broker** are exercised, so
keep it in the release checklist even when the host tests are green.
