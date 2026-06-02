# Skill: ESP32 Firmware Architecture

## Purpose

Guide coding agents to implement maintainable ESP32 firmware with clear layering, testable interfaces, and stable message boundaries.

## Use This Skill When

- Creating new firmware modules for Edge Units.
- Refactoring mixed hardware and business logic into separate layers.
- Designing startup, runtime, and fault handling flows.

## Do Not Use This Skill When

- The task is purely documentation formatting.
- The task is only cloud or dashboard application code.

## Architectural Rules

- Keep firmware layered:
  - application layer: orchestration, state machine, command handling
  - service layer: wifi, mqtt, heartbeat, time, ota
  - device abstraction layer: slot modules, sensor and actuator interfaces
  - hardware drivers: i2c, gpio, adc, pwm, spi, uart
- Application layer must not call low-level driver APIs directly.
- Use constructor-based dependency injection where practical, even in embedded C++.
- Keep message schema translation in one place, not spread across drivers.

## Startup and Runtime Pattern

1. Boot and initialize board services.
2. Discover I2C slot modules.
3. Start WiFi and MQTT clients with retry strategy.
4. Publish discovery heartbeat.
5. Enter operational loop with non-blocking scheduling.
6. On failure, degrade safely and continue heartbeat where possible.

## Non-Blocking Rules

- Avoid long blocking delays in the main loop.
- Prefer time-slice scheduling using elapsed time checks.
- Wrap reconnect behavior with bounded backoff and jitter.

## Quality Gate

- No cross-layer leakage from app logic into drivers.
- No duplicate message parsing logic in multiple modules.
- Fault handling is explicit for WiFi down, MQTT down, and I2C read or write failures.
- Modules compile independently and expose small interfaces.
