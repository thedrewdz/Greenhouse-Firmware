# Heartbeat Spec: Phase 1 Skeleton

## Purpose and Scope

Define the minimal heartbeat behavior for ESP32 Edge firmware before slot discovery and module-state reporting are implemented.

This spec covers:

- heartbeat publish cadence
- required MQTT topic
- required JSON payload fields
- behavior when WiFi or MQTT are unavailable

This spec does not cover slot discovery, per-slot state reporting, command handling side effects, or OTA.

## Preconditions and Assumptions

- Node runs as an ESP32 Edge firmware unit.
- MQTT broker URI, WiFi SSID, and WiFi password are configured in firmware build configuration.
- Firmware uses JSON payloads in Phase 1.
- Device identity is WiFi station MAC rendered as uppercase 12-char hex string with no separators.

## Canonical Terms

- Edge Unit: ESP32 firmware unit publishing telemetry and heartbeat.
- Heartbeat: periodic liveness and state summary payload.
- Canonical Payload: project-standard JSON field names and semantics.

## Behavior and Workflow

1. Boot and initialize runtime services.
2. Start WiFi connection workflow.
3. Start MQTT client when WiFi is connected.
4. Publish heartbeat to topic gh/heartbeat on cadence interval.
5. If WiFi or MQTT is unavailable, skip publish and continue non-blocking retry workflows.
6. Publish the next heartbeat immediately after MQTT reconnect, then continue cadence.

## Data Contract

Topic:

- gh/heartbeat

Payload (skeleton):

```json
{
  "id": 1,
  "device_id": "1ADD5912AF61",
  "hardware_revision": "A",
  "firmware_version": "0.1.0",
  "uptime_seconds": 3,
  "wifi_rssi": -59,
  "slot_count": 0,
  "slots": [],
  "capabilities": []
}
```

Field requirements:

- id: required integer, monotonic per runtime session.
- device_id: required string, 12-char uppercase hex MAC.
- hardware_revision: required string.
- firmware_version: required string.
- uptime_seconds: required integer.
- wifi_rssi: required integer; use 0 when unknown.
- slot_count: required integer; Phase 1 skeleton sets 0.
- slots: required array; Phase 1 skeleton sets empty array.
- capabilities: required array; Phase 1 skeleton sets empty array.

## Validation Rules and Error Handling

- Heartbeat payload encoding failure must be logged and not crash the main loop.
- If MQTT publish fails, firmware logs the failure and retries on the next cadence tick.
- Main loop must remain non-blocking.

## Non-Functional Constraints

- Heartbeat interval target: 30 seconds.
- Loop tick target: 100 ms scheduler interval.
- No unbounded retry loops in foreground path.
- No blocking delays longer than one scheduler tick in app loop.

## Acceptance Criteria

- Device attempts WiFi and MQTT connectivity without blocking app loop.
- While MQTT is connected, firmware publishes valid JSON heartbeat payload to gh/heartbeat every 30 seconds.
- Heartbeat payload includes all required fields with slots and capabilities as empty arrays.
- After MQTT reconnect, a heartbeat is published immediately (without waiting full interval).
- Device continues running when network is unavailable and resumes publish when reconnected.

## Out of Scope / Deferred

- Slot probing and per-slot heartbeat data.
- Capability derivation from discovered modules.
- Command-to-heartbeat state coupling.
- Persistent offline heartbeat queue.

## Open Questions

- Should hardware_revision and firmware_version be compile-time constants or Kconfig values?
- Should heartbeat interval be fixed (30 s) or runtime configurable in Phase 1?
- Should publish QoS be 0 or 1 for heartbeat in Phase 1 default profile?
