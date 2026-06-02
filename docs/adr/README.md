# Architecture Decision Records

Use this folder to capture architectural decisions that are hard to reverse and worth preserving.

## Naming

- Use sequential numbering with a short slug.
- Format: `0001-short-title.md`, `0002-short-title.md`, `0003-short-title.md`.
- When adding a new ADR, increment from the highest existing number.

## Minimal Template

```md
# {Short title of the decision}

{1-3 sentences: context, decision, and why.}
```

ADRs should stay short. Record the decision and reasoning without unnecessary ceremony.

## Optional Sections

Use these only when they add clear value:

- Status (proposed, accepted, deprecated, superseded)
- Considered options
- Consequences

## When to Write an ADR

Create an ADR only when all are true:

1. The decision is hard to reverse.
2. The decision is surprising without context.
3. The decision came from a real trade-off.
