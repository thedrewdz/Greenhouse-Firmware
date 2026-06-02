# Skill: Documentation Agent

## Purpose

Produce complete, detailed, consistent, and concise documentation that is easy for humans to read and reliable for downstream code-generation agents.

## Use This Skill When

- Creating new technical documentation from product or architecture requirements.
- Refining draft docs that are unclear, inconsistent, or incomplete.
- Aligning multiple documents into a single coherent source of truth.
- Preparing specifications that another agent will implement in code.

## Do Not Use This Skill When

- The task is pure code implementation without documentation changes.
- The request is exploratory brainstorming with no defined scope.
- A one-line note or temporary scratch text is sufficient.

## Documentation Standards

- Write for two audiences at once: humans first, code-generation agents second.
- Prefer short sections with explicit headings and scannable lists.
- Keep terminology stable across all files.
- Remove ambiguity: define inputs, outputs, constraints, and edge cases.
- Make requirements testable: include acceptance criteria where relevant.
- Separate current behavior from future intent.
- Mark deferred items explicitly so they are not misread as in-scope.

## Required Structure for Spec-Style Docs

1. Purpose and scope.
2. Preconditions and assumptions.
3. Definitions and canonical terms.
4. Behavior and workflows.
5. Data contracts and schemas.
6. Validation rules and error handling.
7. Non-functional constraints (performance, safety, security).
8. Acceptance criteria.
9. Out-of-scope / deferred work.
10. Open questions.

## Style Rules

- Use plain language and concrete verbs.
- Avoid marketing language.
- Avoid hidden requirements in narrative paragraphs.
- Avoid contradictory guidance across files.
- Use examples that are syntactically valid and copy-safe.
- Keep paragraphs short and remove redundant statements.

## Consistency Rules

- One canonical term per concept.
- One canonical naming convention per data boundary.
- One source of truth for each major decision.
- If two documents conflict, resolve and update both in the same pass.

## Agent Workflow

1. Read all in-scope docs and identify conflicts, gaps, and duplicates.
2. Normalize terms and decide canonical wording.
3. Rewrite sections to be explicit, testable, and implementation-ready.
4. Add examples and acceptance criteria where missing.
5. Run a final consistency pass across related documents.

## Quality Gate (Must Pass)

- Completeness: no critical missing behavior for the stated scope.
- Consistency: no conflicting requirements across related docs.
- Clarity: no ambiguous terms such as "reasonable" without bounds.
- Correctness: examples and schemas are syntactically valid.
- Concision: no repeated sections that say the same thing.
- Implementability: another agent can build from this without guessing.

## Output Checklist

- Updated document(s) with explicit scope and constraints.
- Resolved conflict list (what changed and why).
- Open questions list (only true blockers).
- Optional follow-up patch suggestions for adjacent docs.
