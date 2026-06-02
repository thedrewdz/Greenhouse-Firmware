# Copilot Instructions

## Repository Focus

This repository is for ESP32 peripheral firmware in the Greenhouse platform.

Do not introduce control-unit UI, cloud-first assumptions, or desktop-server implementation details into firmware tasks.

## Required Skills To Apply

For firmware and documentation work, read and follow:

- docs/skills/documentation.md
- docs/skills/esp32-firmware-architecture.md
- docs/skills/esp32-i2c-bus-reliability.md
- docs/skills/esp32-wifi-mqtt-resilience.md
- docs/skills/esp-idf-firmware-practices.md
- docs/skills/esp-idf-testing-strategy.md
- docs/skills/embedded-oo-coding-standards.md

## OO and Code Quality Rules

- Use object-oriented design with clear module boundaries.
- Program to small interfaces and use dependency injection patterns where practical.
- Keep declarations in headers and implementations in source files.
- For C++ use `.hpp` or `.h` plus `.cpp`; for C use `.h` plus `.c`.
- Keep hardware access isolated behind HAL or driver interfaces.
- Prefer composition over deep inheritance.
- Keep retry and timeout logic centralized in service-level components.
- Avoid duplicated parsing and message-mapping logic.
- Keep failure paths explicit with stable error codes and deterministic behavior.

## Firmware Behavior Rules

- Keep loop behavior non-blocking.
- Use bounded retries with backoff and jitter for WiFi and MQTT reconnect.
- Keep I2C fault isolation per slot so one bad module does not take down the node.
- Preserve canonical MQTT payload contracts and topic naming from docs.
- Maintain actuator fail-safe defaults during uncertain network state.

## Review Gate

Before finalizing changes, ensure:

1. Class and module responsibilities are single-purpose.
2. Interfaces are small and testable.
3. WiFi, MQTT, and I2C fault paths are explicitly handled.
4. Changes remain aligned with docs/device-model.md and docs/mqtt-topics.md.
5. No control-unit specific implementation detail leaked into firmware code.
6. Tests are added or updated at the correct layer (unit, host-mock, or HIL).
