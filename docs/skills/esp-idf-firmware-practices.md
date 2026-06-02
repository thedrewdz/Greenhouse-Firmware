# Skill: ESP-IDF Firmware Practices

## Purpose

Guide coding agents to implement ESP32 firmware using ESP-IDF conventions, FreeRTOS-safe patterns, and maintainable C or C++ project structure.

## Use This Skill When

- Creating or refactoring ESP-IDF components.
- Implementing tasks, drivers, and service orchestration.
- Defining project and component-level file structure.

## File and Module Structure Rules

- Keep declarations and definitions separate.
- For C++ modules:
  - headers: `.hpp` (or `.h` if project standard chooses C-style headers)
  - implementation: `.cpp`
- For C modules:
  - headers: `.h`
  - implementation: `.c`
- Do not place non-trivial function implementations directly in headers.
- Keep one primary class or module per header and source pair.
- Use include guards or `#pragma once` consistently.

## ESP-IDF Component Rules

- Organize code into ESP-IDF components with clear ownership boundaries.
- Keep each component focused, for example:
  - connectivity
  - mqtt transport
  - slot discovery
  - sensor or actuator drivers
  - telemetry codec
- Expose minimal public headers from each component.
- Keep private helpers in internal source files.

## FreeRTOS and Concurrency Rules

- Keep task responsibilities single-purpose.
- Avoid sharing mutable state across tasks without synchronization.
- Use queues, event groups, or task notifications instead of ad hoc polling between tasks.
- Keep ISR work minimal and defer heavy processing to tasks.
- Never block high-priority tasks with long critical sections.

## Error and Resource Management Rules

- Check and handle `esp_err_t` return values.
- Normalize low-level errors to project-defined stable error codes at module boundaries.
- Pair resource acquire and release paths explicitly.
- Avoid dynamic allocation in hot paths where static or pooled memory is practical.

## Networking and MQTT Rules

- Treat WiFi and MQTT as separate state machines.
- Reconnect with bounded exponential backoff plus jitter.
- Re-subscribe topics after MQTT reconnect.
- Keep payload parsing and validation centralized in codec modules.

## Build and Configuration Rules

- Keep Kconfig options scoped to the owning component.
- Avoid magic numbers; prefer `sdkconfig` or component config constants.
- Keep defaults safe for unattended operation in greenhouse environments.

## Acceptance Checklist

- File structure uses explicit header and source separation.
- Component public APIs are minimal and documented.
- Task interactions are synchronized and race-safe.
- All critical `esp_err_t` returns are handled.
- No business logic is hidden in ISR context.
