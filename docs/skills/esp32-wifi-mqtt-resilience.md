# Skill: ESP32 WiFi and MQTT Resilience

## Purpose

Guide coding agents to implement resilient WiFi and MQTT behavior for local-first greenhouse peripheral firmware.

## Use This Skill When

- Implementing network bootstrap and reconnect logic.
- Building heartbeat, telemetry publish, and command subscribe flows.
- Handling controller or broker outages without manual recovery.

## Connectivity Rules

- Treat WiFi and MQTT as independent state machines.
- Reconnect with exponential backoff plus jitter.
- Keep retry loops non-blocking.
- Resume subscriptions after MQTT reconnect.

## Messaging Rules

- Keep one encoder and decoder for canonical JSON payloads.
- Validate required fields before acting on write commands.
- Always publish an ack or error response for handled commands.
- Use monotonic message ids per device runtime session.

## Offline Behavior

- Continue local sensing and actuator safety logic when MQTT is unavailable.
- Buffer only bounded telemetry windows to avoid memory exhaustion.
- Publish a fresh heartbeat immediately after reconnect.

## Security and Safety Baseline

- Do not execute malformed commands.
- Reject commands targeting unknown slots.
- Keep actuator default state fail-safe during network uncertainty.

## Acceptance Checklist

- Device recovers from WiFi loss without reboot.
- Device recovers from broker restart and resubscribes automatically.
- Command handling remains idempotent for duplicate message delivery.
- Heartbeat includes current RSSI, slot state, and fault codes after reconnect.
