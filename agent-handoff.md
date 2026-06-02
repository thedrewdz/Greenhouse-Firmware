# Agent Handoff

This file is for local, time-bound session state only.

Durable policy, canonical context, architecture, MQTT contracts, ADRs, and skill guidance live in the Greenhouse Documentation repository:

- https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

## Current Workspace State

- Path: `D:\Code\Greenhouse\esp32-main`
- Repository purpose: ESP32 Edge Unit firmware.
- Current branch: `main`
- Last completed commit: `cc43de4 Move durable docs to documentation repo`
- Local documentation has been removed from this repository except:
  - `README.md`
  - `AGENTS.md`
  - `.github/copilot-instructions.md`
  - `agent-handoff.md`

## Current Progress Snapshot

- Durable Greenhouse documentation was moved to the dedicated Greenhouse Documentation repository.
- Local agent instructions now require agents to read the external documentation README before taking other actions.
- Local terminology was normalized to use `Main Unit`.
- Local ambiguity-handling guidance now references the Plan Interrogation Method from the grill-with-docs Main Unit skill in the Greenhouse Documentation repository.

## Open Questions

- None for the documentation cleanup.

## Next Actions

- Continue firmware work from the current clean documentation baseline.
- Before implementing firmware changes, read the relevant external Greenhouse Documentation files and skills listed from the documentation README.

## Resume Prompt

```text
Read AGENTS.md, agent-handoff.md, and the Greenhouse Documentation README, then continue ESP32 Edge Unit firmware work using the relevant external documentation and skills for the task.
```
