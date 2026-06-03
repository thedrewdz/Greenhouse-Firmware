# AGENTS

## Purpose

This repository contains ESP32 edge firmware for the Greenhouse platform.

This is not the Main Unit application repository.

## First Action

Before taking any other action in this repository, agents must read the central documentation entry point:

- https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

Use the dedicated Greenhouse Documentation repository for durable project documentation, canonical context, architecture, MQTT contracts, ADRs, and skill guidance.

## Scope Boundaries

- Keep work focused on embedded firmware concerns.
- Do not introduce Main Unit UI, cloud-first assumptions, or desktop/server implementation details.
- Keep architecture local-first and MQTT-centered for Edge Unit communication.

## Instruction Precedence

Use this precedence order when instructions overlap:

1. AGENTS.md (this file)
2. .github/copilot-instructions.md
3. Greenhouse Documentation repository instructions and docs
4. Local repository docs under docs/ (supplemental only)

If guidance conflicts, follow the highest-precedence source.

## Local Docs And Skill Pattern

- Treat all local docs in this repository under docs/ as supplemental repository guidance.
- Do not let local docs override canonical policy, architecture, contracts, or terminology from the Greenhouse Documentation repository.
- If a documentation, knowledge or skill gap is identified, don't make things up, instead bring it to the user's attension to be addressed properly.
- Local documentation authored in this repository would typically be in the form of ADRs located under docs/adr/.
- When adding or updating a local ADR:
	1. Read the central documentation entry point first: https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md
	2. Align terms and assumptions with canonical docs before writing local ADR content.
	3. Keep local ADRs focused on repository-specific decisions that do not belong in shared canonical docs.
	4. If canonical and local guidance differ, follow canonical guidance and update or scope the ADR accordingly.

## Coding Standards

- Use object-oriented design with clear module boundaries.
- Program to small interfaces and use dependency injection when a dependency crosses a module or hardware boundary.
- Keep declarations in headers and implementations in source files.
- C++ files: .hpp or .h declarations with .cpp implementations.
- C files: .h declarations with .c implementations.
- Isolate hardware access behind HAL/driver interfaces.
- Prefer composition over deep inheritance.
- Keep failure paths explicit with stable error codes.

## Firmware Runtime Rules

- Keep loop behavior non-blocking.
- Use bounded retries with backoff and jitter for WiFi and MQTT reconnect.
- Keep I2C fault isolation per slot so one bad module does not take down the node.
- Preserve canonical MQTT payload contracts and topic naming from the Greenhouse Documentation repository.
- Maintain actuator fail-safe defaults during degraded, disconnected, or unprovisioned network states.

## Quality Gates

Before finalizing changes, verify:

1. Class and module responsibilities are single-purpose.
2. Interfaces are small and testable.
3. WiFi, MQTT, and I2C fault paths are explicitly handled.
4. Changes remain aligned with the device model and MQTT topic contracts in the Greenhouse Documentation repository.
5. No Main Unit specific implementation detail leaks into firmware code.
6. Tests are added or updated at the correct layer (unit, host-mock, or HIL).
