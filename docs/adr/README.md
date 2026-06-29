# Architecture Decision Records (Local)

This folder holds **repository-local** ADRs for the Greenhouse ESP32 edge firmware: decisions specific to this firmware that do not belong in the shared canonical docs.

Canonical platform context, architecture, and contracts live in the Greenhouse Documentation repository:

- https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

Local ADRs are supplemental. They must not override canonical policy, architecture, contracts, or terminology. On conflict, canonical guidance wins and the local ADR is updated or re-scoped.

## ADR Loading Rules (Partial Disclosure)

- Do not read every ADR up front.
- Use the digest below to find ADRs relevant to the area you are touching, then read only those in full.
- When adding or updating a local ADR, follow the ADR workflow in `AGENTS.md`: read the canonical entry point first, align terms and assumptions with canonical docs, and keep the ADR focused on a repository-specific decision.

## ADR Digest

| ADR | Status | Date | Decision (summary) | Touches |
|---|---|---|---|---|
| `0001-ble-first-onboarding.md` | accepted | 2026-06-02 | Phase 1 Edge Unit onboarding defaults to BLE, with wired onboarding kept only as a recovery/manufacturing fallback. | Provisioning Mode, BLE onboarding, provisioning payload contract, startup flow |

## Adding an ADR

- Number sequentially: `NNNN-short-kebab-title.md`.
- Include `Status:` (proposed / accepted / superseded) and `Date:` near the top.
- State the decision, the considered options, and the consequences.
- Reference the canonical specs/docs the decision depends on.
- Add a row to the digest above so partial disclosure keeps working.
