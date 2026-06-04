# Documentation Feedback Items: Edge Unit Onboarding

## Item 1

### Source

- Repo: `esp32-main`
- Branch: `main`
- Commit or diff: local implementation diff
- Agent role: Implementation Agent
- Date: 2026-06-04

### Finding

The implementation-agent workflow requires reading Spec Control status, setting status to `implementation-in-progress`, and later setting status to `ready-for-test`. The canonical `specs/edge-unit-onboarding/spec.md` file fetched for implementation does not include a Spec Control block or Status History section.

### Impact

The implementation agent could not record the required status transitions without inventing a canonical control section in an implementation repository artifact. This creates ambiguity about where status updates should be made and how downstream test/review agents should confirm stage readiness.

### Affected Docs

- `specs/edge-unit-onboarding/spec.md`
- `skills/implementation-agent-skill.md`
- `templates/spec.md`

### Recommended Documentation Change

Add a Spec Control section and Status History section to `specs/edge-unit-onboarding/spec.md`, or update the implementation-agent skill to say that missing Spec Control should be reported in local `doc-feedback.md` without blocking implementation.

Suggested Spec Control shape:

```md
## Spec Control

- Status: ready-for-implementation
- Owner Role: documentation-agent
- Last Updated: YYYY-MM-DD

## Status History

- YYYY-MM-DD: ready-for-implementation - Accepted for firmware implementation.
```

### Prevents

Future implementation agents will know whether they are allowed to proceed and where to record transition to `ready-for-test`.

### Disposition

- [ ] Accepted
- [ ] Rejected
- [x] Deferred

### Final Doc Update

Pending documentation repository update.

## Item 2

### Source

- Repo: `esp32-main`
- Branch: `main`
- Commit or diff: local implementation diff
- Agent role: Implementation Agent
- Date: 2026-06-04

### Finding

The onboarding spec defines the BLE application payload and status response contract, but it does not assign canonical BLE service UUIDs or characteristic UUIDs for the provisioning payload and status response.

### Impact

Firmware had to choose UUIDs locally while preserving the previously advertised onboarding service UUID pattern. A Main Unit BLE client implementation could choose different UUIDs unless the canonical spec records them.

### Affected Docs

- `specs/edge-unit-onboarding/spec.md`

### Recommended Documentation Change

Add a BLE GATT contract subsection that names the Phase 1 service and characteristics:

- Onboarding service UUID.
- Provisioning payload characteristic UUID and properties.
- Provisioning status characteristic UUID and properties.
- Whether write-with-response, write-without-response, notifications, and reads are required or optional.

### Prevents

Prevents Main Unit and Edge Unit implementations from diverging on BLE discovery and characteristic use.

### Disposition

- [ ] Accepted
- [ ] Rejected
- [x] Deferred

### Final Doc Update

Pending documentation repository update.

## Item 3

### Source

- Repo: `esp32-main`
- Branch: `main`
- Commit or diff: local implementation diff
- Agent role: Implementation Agent
- Date: 2026-06-04

### Finding

The spec requires persistence to be applied as one logical configuration update and to retain the last known valid configuration on write failure. It does not specify whether ESP32 NVS multi-key updates require a shadow namespace, version marker, or commit-only semantics for Phase 1.

### Impact

Firmware implemented one logical update by writing all keys and committing once. This is likely sufficient for Phase 1, but the exact crash-consistency expectation is not explicit.

### Affected Docs

- `specs/edge-unit-onboarding/spec.md`

### Recommended Documentation Change

Clarify that Phase 1 may treat a single NVS commit after all key writes as the logical update boundary, or require a stronger two-phase/shadow-record pattern if power-loss consistency during provisioning writes is mandatory.

### Prevents

Prevents reviewers and test agents from applying different expectations for NVS atomicity during onboarding persistence tests.

### Disposition

- [ ] Accepted
- [ ] Rejected
- [x] Deferred

### Final Doc Update

Pending documentation repository update.

## Item 4

### Source

- Repo: `esp32-main`
- Branch: `main`
- Commit or diff: local implementation diff
- Agent role: Implementation Agent
- Date: 2026-06-04

### Finding

The implementation-agent skill lists required output artifacts, but the workflow does not include an early artifact setup/checkpoint that forces `.agent-output/specs/<spec-name>/implementation-plan.md` and `.agent-output/specs/<spec-name>/doc-feedback.md` to be created before or during implementation. In this implementation pass, the firmware work was completed first and the required artifacts were missed until the user pointed it out.

### Impact

Implementation agents may complete code changes without producing the required local dossier artifacts. This weakens handoff quality for test, review, and documentation roles because implementation scope, verification evidence, deviations, and documentation feedback may be absent or reconstructed after the fact.

### Affected Docs

- `skills/implementation-agent-skill.md`

### Recommended Documentation Change

Update the implementation-agent skill workflow to make artifact creation an explicit early gate and final gate. Suggested wording:

```md
Before coding, create or update `.agent-output/specs/<spec-name>/implementation-plan.md` from the template and create `.agent-output/specs/<spec-name>/doc-feedback.md` from the template, even if no feedback is known yet.

Before final response, verify both required files exist and summarize their paths in the handoff.
```

Also add the artifact existence check to the Quality Gate as a concrete file-system check, not only a general required-output statement.

### Prevents

Prevents implementation agents from missing required local artifacts and ensures downstream roles can rely on a consistent `.agent-output/specs/<spec-name>/` dossier.

### Disposition

- [ ] Accepted
- [ ] Rejected
- [x] Deferred

### Final Doc Update

Pending documentation repository update.
