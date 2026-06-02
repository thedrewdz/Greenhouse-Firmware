# Skills Index

This folder contains reusable skills for coding and documentation agents working on ESP32 greenhouse peripheral firmware.

## Available Skills

- documentation.md
  - Produces consistent implementation-ready documentation.
- esp32-firmware-architecture.md
  - Enforces layered firmware architecture and startup/runtime patterns.
- esp32-i2c-bus-reliability.md
  - Covers shared-bus I2C design, recovery, and fault reporting.
- esp32-wifi-mqtt-resilience.md
  - Covers non-blocking reconnect, command handling, and offline behavior.
- esp-idf-firmware-practices.md
  - Enforces ESP-IDF component structure, FreeRTOS safety, and header/source separation.
- esp-idf-testing-strategy.md
  - Defines unit, host-mock, and hardware-in-loop testing strategy for ESP-IDF firmware.
- embedded-oo-coding-standards.md
  - Enforces object-oriented design and code quality standards.

## Recommended Usage Order

1. Start with documentation.md for scope and contract clarity.
2. Apply esp32-firmware-architecture.md for module boundaries.
3. Apply esp32-i2c-bus-reliability.md for slot and bus behavior.
4. Apply esp32-wifi-mqtt-resilience.md for network and messaging behavior.
5. Apply esp-idf-firmware-practices.md for ESP-IDF component and RTOS conventions.
6. Apply esp-idf-testing-strategy.md for test planning and verification gates.
7. Apply embedded-oo-coding-standards.md as a review gate on all code changes.

## Domain Docs Convention

Agents should follow this repo's domain-doc layout:

- CONTEXT.md: canonical glossary for firmware domain language
- docs/adr/: architecture decision records using sequential numbering

Working rules:

1. Read CONTEXT.md before naming modules, concepts, or boundaries.
2. Keep CONTEXT.md free of implementation details.
3. When terminology is clarified, update CONTEXT.md in the same session.
4. Record hard-to-reverse architectural trade-offs in docs/adr/.
