# AGENTS

## Purpose

This repository contains ESP32 edge firmware for the Greenhouse platform.

This is not the control-unit application repository.

## Scope Boundaries

- Keep work focused on embedded firmware concerns.
- Do not introduce control-unit UI, cloud-first assumptions, or desktop/server implementation details.
- Keep architecture local-first and MQTT-centered for node communication.

## Instruction Precedence

Use this precedence order when instructions overlap:

1. AGENTS.md (this file)
2. .github/copilot-instructions.md
3. docs/skills/*.md
4. docs/agent-handoff.md (session state only)

If guidance conflicts, follow the highest-precedence source.

## Always-Read Files

- docs/skills/README.md
- CONTEXT.md
- docs/device-model.md
- docs/mqtt-topics.md
- docs/architecture.md

Read ADRs relevant to the changed area when they exist:

- docs/adr/*.md

When resuming previous work, also read:

- docs/agent-handoff.md

## Skills Catalog

Use the following skills based on task type.

| Skill | Path | Use When |
|---|---|---|
| Documentation | docs/skills/documentation.md | Writing or updating implementation-ready docs, contracts, and acceptance criteria |
| Firmware Architecture | docs/skills/esp32-firmware-architecture.md | Defining module boundaries, startup flow, runtime orchestration |
| I2C Bus Reliability | docs/skills/esp32-i2c-bus-reliability.md | Designing shared-bus access, fault isolation, probe/recovery behavior |
| WiFi and MQTT Resilience | docs/skills/esp32-wifi-mqtt-resilience.md | Reconnect logic, retries/backoff/jitter, offline buffering and command handling |
| ESP-IDF Firmware Practices | docs/skills/esp-idf-firmware-practices.md | ESP-IDF project structure, component boundaries, FreeRTOS-safe patterns |
| ESP-IDF Testing Strategy | docs/skills/esp-idf-testing-strategy.md | Unit tests, host-mock tests, HIL smoke plans and verification gates |
| Embedded OO Standards | docs/skills/embedded-oo-coding-standards.md | Interface-first OO design, dependency boundaries, maintainability review |

## Domain Docs Workflow

Use this repo's domain docs pattern during planning and implementation.

Structure:

- Single-context glossary at CONTEXT.md
- Decision records under docs/adr/

Rules:

- Keep CONTEXT.md as glossary-only domain language.
- Do not store implementation details in CONTEXT.md.
- When a term is resolved, update CONTEXT.md in the same session.
- Add an ADR only for decisions that are hard to reverse, surprising without context, and based on real trade-offs.
- Keep ADRs concise and use sequential numbering.

## Coding Standards

- Use object-oriented design with clear module boundaries.
- Program to small interfaces and use dependency injection where practical.
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
- Preserve canonical MQTT payload contracts and topic naming from docs.
- Maintain actuator fail-safe defaults during uncertain network state.

## Quality Gates

Before finalizing changes, verify:

1. Class and module responsibilities are single-purpose.
2. Interfaces are small and testable.
3. WiFi, MQTT, and I2C fault paths are explicitly handled.
4. Changes remain aligned with docs/device-model.md and docs/mqtt-topics.md.
5. No control-unit specific implementation detail leaks into firmware code.
6. Tests are added or updated at the correct layer (unit, host-mock, or HIL).

## Handoff Policy

- docs/agent-handoff.md is for time-bound state only: progress snapshot, next steps, open questions, and resume prompt.
- Do not duplicate long-lived policy in docs/agent-handoff.md.
- Update docs/agent-handoff.md at the end of substantial work sessions.

## Maintenance Cadence

Use this update pattern to keep instructions consistent over time.

Per substantial work session:

- Update docs/agent-handoff.md with current progress, next actions, and open questions.
- Update CONTEXT.md when domain terminology is clarified or renamed.
- Keep entries factual and concise.
- Remove stale or completed next-step items when they are no longer relevant.

Only when policy or standards change:

- Update AGENTS.md for durable guidance, precedence, quality gates, and skills usage.
- Keep .github/copilot-instructions.md minimal and aligned to AGENTS.md.
- If a new skill is added or renamed, update both docs/skills/README.md and the Skills Catalog table in AGENTS.md.
- Add or update ADRs in docs/adr/ for qualifying architectural decisions.

Quick consistency check after policy edits:

1. AGENTS.md and .github/copilot-instructions.md do not conflict.
2. docs/agent-handoff.md contains session state only.
3. Skill names and file paths match docs/skills/README.md.