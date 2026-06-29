# Skill: ESP32 NVS Provisioning Persistence

## Purpose

Guide coding agents to persist and load Edge Unit provisioning configuration in ESP32 NVS safely: namespace discipline, schema versioning, fail-safe loads, and atomic configuration updates that survive power loss mid-write.

This pack covers `services_provisioning_config.*`. The field set and schema version are part of the canonical provisioning contract owned by the Greenhouse Documentation repository; read that first, then apply these device-side persistence rules.

- Canonical entry point: https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

## Use This Skill When

- Implementing or changing how provisioning config is read from or written to NVS.
- Adding fields to the persisted configuration or bumping the provisioning schema version.
- Implementing factory reset, re-provisioning, or migration of stored configuration.
- Debugging boot-time config load, slot selection, or corrupted/partial writes.

## Do Not Use This Skill When

- The task is the BLE transport or GATT flow (use `esp32-ble-provisioning.md`).
- The task is operational WiFi/MQTT behavior after config is loaded (use `esp32-wifi-mqtt-resilience.md`).

## Initialization Rules

- Initialize NVS before any provisioning read or write.
- Recover from `ESP_ERR_NVS_NO_FREE_PAGES` and `ESP_ERR_NVS_NEW_VERSION_FOUND` by erasing and re-initializing the partition, then proceeding.
- Do not assume a clean flash; treat a missing config as "unprovisioned," not as an error.

## Namespace and Key Discipline

- Keep one module responsible for all provisioning NVS access; do not scatter `nvs_open` calls across features.
- Use stable, documented namespace and key names. Keep keys short and explicit (for example: `wifi_ssid`, `wifi_pass`, `mqtt_uri`, `hb_ms`).
- Keep configuration data separate from metadata (active-slot pointer, schema/version markers) in their own namespaces.
- Never log secrets; log the SSID for diagnostics but never the WiFi password.

## Atomic Update Rules (A/B Slot Pattern)

- Persist configuration as one logical update so a power loss never leaves a half-written active config.
- Use two config slots plus a metadata pointer to the active slot:
  1. Write the full candidate configuration to the inactive slot and commit.
  2. Only after that commit succeeds, flip the active-slot pointer in metadata and commit.
- If the candidate write fails, leave the active pointer untouched so the previous good config remains in effect.
- Treat the pointer flip as the single commit point of the update.

## Load and Fallback Rules

- On load, read the metadata active-slot pointer, then load that slot.
- If the active slot is unreadable, fall back deterministically (for example to a legacy namespace) rather than booting with partial data.
- Validate required fields on load; a record missing a required field (SSID, MQTT broker URI) is "not provisioned," not a usable config.
- Apply safe defaults for optional values (for example heartbeat interval) instead of failing the whole load.

## Schema Versioning and Migration Rules

- Stamp persisted configuration with a schema version and check it on load.
- When adding or changing fields, bump the schema version and provide an explicit migration or rejection path for older records; never silently misinterpret old layouts.
- Keep migration logic isolated and one-directional (old to new), and keep the legacy-namespace fallback until migration is proven.

## Validation and Save Guards

- Reject saves with empty required fields before touching NVS.
- Normalize NVS errors to explicit results at the module boundary; callers should not interpret raw `esp_err_t` deep in feature code.
- Pair every `nvs_open` with `nvs_close` on all paths, including error paths.

## Validation Checklist

- First boot with empty NVS reports "not provisioned" and triggers Provisioning Mode.
- A successful save is fully readable after reboot, with all required fields intact.
- Power loss simulated between candidate write and pointer flip leaves the previous good config active.
- Active slot corruption falls back to the previous/legacy config rather than booting unprovisioned-by-accident.
- Optional fields absent from storage resolve to documented safe defaults.
- No secret values appear in logs.
