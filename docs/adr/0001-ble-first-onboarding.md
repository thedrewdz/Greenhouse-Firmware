# BLE-first onboarding for Edge Units

Status: accepted  
Date: 2026-06-02

> **Superseded by:** This decision has been consolidated into the documentation repository as
> [ADR 0004 — BLE-first onboarding](https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/adr/0004-ble-first-onboarding.md)
> with updated consequences reflecting WiFi credential storage on the Main Unit and automatic
> `mqtt_broker_uri` derivation. Read the documentation ADR as the canonical record.

Phase 1 onboarding for new Edge Units will default to BLE, with wired onboarding retained only as an optional fallback for recovery and manufacturing workflows.

This decision was made because onboarding must be accessible to non-technical users and should not depend on manual cable handling or static-IP setup, while still preserving a service path when BLE is unavailable.

## Considered options

- BLE-first onboarding with wired fallback
- Wired-first onboarding (I2C/connector) with optional BLE
- Wired-only onboarding

## Consequences

- Firmware must support an explicit unprovisioned Provisioning Mode over BLE before normal WiFi and MQTT startup.
- Main Unit must provide BLE discovery, pairing, payload delivery, and success confirmation after first heartbeat.
- Provisioning payload contracts become a stable interface between Main Unit and Edge Unit implementation.
- Security hardening details remain deferred to implementation-phase specs and follow-on ADRs.

## References

- spec-edge-unit-onboarding-ble.md
- architecture.md
- device-model.md
- mqtt-topics.md


