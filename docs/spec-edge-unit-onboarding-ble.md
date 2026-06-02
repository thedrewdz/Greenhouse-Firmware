# Edge Unit Onboarding Spec: BLE First (Phase 1)

## Purpose and Scope

Define the first-time onboarding flow for new Edge Units using BLE as the default provisioning channel.

Decision record: docs/adr/0001-ble-first-onboarding.md

This spec covers:

- BLE-based first-time provisioning
- exchange of WiFi credentials and bootstrap MQTT endpoint
- transition from unprovisioned to operational heartbeat state

This spec does not cover production cryptographic key infrastructure, OTA enrollment, or cloud-based onboarding.

## Preconditions and Assumptions

- Main Control Unit hardware baseline is Raspberry Pi 4B with BLE support.
- Edge Unit hardware baseline is ESP32 with BLE support.
- Main Unit and Edge Unit are within BLE range during onboarding.
- Main Unit runs an onboarding service capable of BLE scan, pairing, and provisioning payload delivery.

## Canonical Terms

- Edge Unit: ESP32 node that publishes heartbeat and telemetry after onboarding.
- Onboarding: first-time setup workflow for an unprovisioned Edge Unit.
- Provisioning Mode: temporary BLE-advertising mode for onboarding.
- Bootstrap MQTT Endpoint: broker URI value required before first MQTT connection.

## Behavior and Workflow

1. Edge Unit boots and checks for provisioning data.
2. If provisioning data is missing, Edge Unit enters Provisioning Mode.
3. Main Unit scans for unprovisioned Edge Units over BLE.
4. Main Unit establishes BLE session with selected Edge Unit.
5. Main Unit sends provisioning payload.
6. Edge Unit validates payload and stores values.
7. Edge Unit exits Provisioning Mode and starts normal network bootstrap.
8. Edge Unit connects to WiFi and MQTT.
9. Edge Unit publishes first heartbeat to gh/heartbeat.
10. Main Unit marks onboarding as complete.

Failure behavior:

- If validation fails, Edge Unit remains in Provisioning Mode and reports reason over BLE response.
- If WiFi or MQTT connect fails after valid provisioning, Edge Unit keeps retrying with bounded backoff and remains recoverable.

## Data Contract and Schema

Provisioning payload shape (BLE application payload):

```json
{
  "schema_version": 1,
  "device_id": "1ADD5912AF61",
  "wifi_ssid": "ExampleWiFi",
  "wifi_password": "ExamplePassword",
  "mqtt_broker_uri": "mqtt://192.168.1.50",
  "heartbeat_interval_ms": 30000
}
```

Field requirements:

- schema_version: required integer, must be 1 in Phase 1.
- device_id: required string, must match Edge Unit hardware identity.
- wifi_ssid: required string.
- wifi_password: required string, may be empty only for open networks.
- mqtt_broker_uri: required string in URI format.
- heartbeat_interval_ms: optional integer, defaults to 30000 if omitted.

## Validation Rules and Error Handling

Edge Unit must reject onboarding payload when:

- schema_version is unsupported
- device_id does not match local hardware identity
- wifi_ssid is empty
- mqtt_broker_uri is missing or malformed

Edge Unit response contract (BLE status response):

- result: success or error
- error_code: stable integer when result is error
- error_message: short diagnostic string for local UI

## Non-Functional Constraints

- Provisioning Mode must be bounded and low-power friendly.
- Retries for WiFi and MQTT remain non-blocking and bounded with jitter.
- Onboarding should complete within 60 seconds under normal LAN conditions.
- Edge Unit must never require static IP assumptions for broker reachability.

## Acceptance Criteria

- New Edge Unit can be onboarded with no wired connection.
- Main Unit can provision WiFi and MQTT endpoint over BLE to a fresh Edge Unit.
- Edge Unit publishes first heartbeat after successful onboarding.
- Invalid onboarding payloads are rejected with explicit error codes.
- Rebooted, already-provisioned Edge Unit skips Provisioning Mode and proceeds to normal startup.

## Out-of-Scope / Deferred Work

- Full production PKI and certificate lifecycle.
- Cloud relay onboarding.
- BLE Mesh onboarding.
- Multi-main-unit arbitration.

## Open Questions

- Should mqtt_broker_uri remain explicit or move to Main Unit hostname discovery after initial onboarding?
- What is the exact factory-reset gesture and timeout policy for re-entering Provisioning Mode?
- Should heartbeat_interval_ms be editable by user onboarding flow in Phase 1 UI?
