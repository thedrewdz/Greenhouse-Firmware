# CLAUDE

This file is the Claude / Claude Code entry point for the Greenhouse ESP32 edge firmware repository.

## First Action

Before taking any other action in this repository, read the central documentation entry point:

- https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

The dedicated Greenhouse Documentation repository is canonical for durable project documentation, platform context, architecture, MQTT contracts, ADRs, and skill guidance. This firmware repository is a consumer of that documentation, not a replacement for it. Treat the Greenhouse Documentation repository as authoritative whenever guidance overlaps.

## Canonical Policy

@AGENTS.md

AGENTS.md is the canonical, cross-agent policy for this repository.
All scope boundaries, instruction precedence, coding standards, firmware runtime rules, and quality gates in AGENTS.md apply fully to Claude and Claude Code.

## What This Repository Is

This repository contains ESP32 edge firmware for the Greenhouse platform.
It is not the Main Unit application repository.

Claude Code may write and edit firmware code, tests, and local supplemental docs in this repository.
Claude Code must not introduce Main Unit UI, cloud-first assumptions, or desktop/server implementation details into firmware tasks.

## Incremental Loading and Partial Disclosure

**Do not read all documentation or all skill packs up front.**

This is the required pattern - load only what the task needs:

1. Always read first:
   - The Greenhouse Documentation `README.md` (see First Action) - it routes to canonical context, architecture, the device model, MQTT topic contracts, and the onboarding/configuration specs.

2. From the documentation repository, load only the canonical docs that match the current task (for example: the device model and MQTT topic contracts for messaging work, or the onboarding spec for provisioning work). Do not load unrelated specs or orientation docs.

3. In this repository, load only the local skill packs in `docs/skills/` that match the task (see Local Skill Packs below). Do not load all skill packs at once.

4. Load local ADRs under `docs/adr/` only for the area you are touching.

This pattern preserves context and prevents conflicting guidance from non-applicable docs and skills.

## Instruction Precedence

When instructions overlap, follow this order:

1. `AGENTS.md` (canonical cross-agent policy)
2. `CLAUDE.md` (this file - Claude-specific operational guidance; never overrides AGENTS.md)
3. Greenhouse Documentation repository instructions and docs (canonical project documentation)
4. `docs/skills/*.md` (local skill packs - supplemental only)
5. `docs/adr/*.md` (local ADRs - supplemental, repository-specific decisions only)
6. `agent-handoff.md` (session scratchpad only - never durable guidance)

If guidance conflicts, follow the highest-precedence source. Local docs and skill packs must never override canonical policy, architecture, contracts, or terminology from the Greenhouse Documentation repository.

## Local Skill Packs

Local, implementation-focused skill packs for this repository live under `docs/skills/`.

These are supplemental to the canonical skills in the Greenhouse Documentation repository. Review the relevant canonical skill guidance first, then apply these repository-local packs for ESP32 and ESP-IDF specifics.

Read `docs/skills/README.md` to select the right pack for the task.
Load only the matching pack, not all of them.

Available skill packs:

- `docs/skills/esp32-firmware-architecture.md` - firmware layering, testable interfaces, startup/runtime/fault flows, non-blocking scheduling.
- `docs/skills/embedded-oo-coding-standards.md` - object-oriented design, small interfaces, hardware abstraction, testability for C/C++ firmware.
- `docs/skills/esp-idf-firmware-practices.md` - ESP-IDF component structure, FreeRTOS-safe concurrency, `esp_err_t` handling, build/config rules.
- `docs/skills/esp-idf-testing-strategy.md` - layered testing: Unity unit tests, host-side mock tests, and hardware-in-loop smoke validation.
- `docs/skills/esp32-i2c-bus-reliability.md` - I2C discovery, bounded retries, per-slot fault isolation, and bus recovery.
- `docs/skills/esp32-wifi-mqtt-resilience.md` - WiFi/MQTT as independent state machines, backoff with jitter, resubscribe, and fail-safe offline behavior.
- `docs/skills/esp32-ble-provisioning.md` - NimBLE Provisioning Mode, GATT payload/status interface, and the receive-validate-persist-transition onboarding lifecycle.
- `docs/skills/esp32-nvs-provisioning-persistence.md` - NVS namespace discipline, schema versioning, fail-safe loads, and atomic (A/B slot) configuration updates.

If a documentation, knowledge, or skill gap is identified, do not make things up - bring it to the user's attention to be addressed properly, per AGENTS.md.

## Local ADRs

Repository-specific architectural decisions live under `docs/adr/`.

Read `docs/adr/README.md` first - it is the ADR digest and loading rules. Use it to load only the ADRs relevant to the area you are touching, not all of them.

When adding or updating a local ADR, follow the ADR workflow in AGENTS.md: read the central documentation entry point first, align terms and assumptions with canonical docs, keep ADRs focused on repository-specific decisions, and defer to canonical guidance on conflict.

## Handoff File

`agent-handoff.md` is for local, time-bound session state only.

Do not treat it as durable domain guidance and do not duplicate canonical policy there.

## Tooling Notes for Claude Code

- The Codex/cross-agent policy lives in `AGENTS.md`. Claude Code reads it through the `@AGENTS.md` include above and must not modify it without an explicit instruction to do so.
- `.github/copilot-instructions.md` is the GitHub Copilot bridge; it also defers to `AGENTS.md` and the Greenhouse Documentation repository. Keep it in sync when cross-agent policy changes.
- `docs/skills/` packs are tool-neutral and work for both Codex and Claude Code. When updating skill guidance, update the shared pack rather than duplicating it per tool.
- Firmware runtime rules, coding standards, and quality gates are defined once in `AGENTS.md`; do not duplicate them here.

## Repository Tool Bridges

| Tool | File |
|---|---|
| Claude / Claude Code | `CLAUDE.md` (this file) |
| OpenAI Codex | `AGENTS.md` |
| GitHub Copilot | `.github/copilot-instructions.md` |

All bridges point to `AGENTS.md` as the canonical source of policy, and to the Greenhouse Documentation repository as the canonical source of project documentation.
