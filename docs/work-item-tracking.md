# Work Item Tracking

How work is captured and tracked across the Greenhouse repositories. This is supplemental
operational guidance; if the Greenhouse Documentation repository defines canonical process,
that wins and this should be aligned to (or promoted into) it.

## Principle

All trackable work — bugs, features, updates, tech-debt, standards gaps — is a **GitHub
issue**. One work item = one issue. Work that lives only in chat, a commit message, or a
local doc is invisible and never gets prioritized. If you identify or are asked to do
non-trivial work, file the issue first (or as you start it), in the repository that owns it.

## Single prioritized view

All issues across the Greenhouse repositories flow into one user-level project board,
**Greenhouse Delivery** — https://github.com/users/thedrewdz/projects/1

- A per-repo workflow (`.github/workflows/add-to-project.yml`) adds every newly-opened issue
  to the board automatically. Do **not** rely on manual board adds.
- **Priority = manual rank**: an item's position in the board's `Backlog` view is the source
  of truth for "what's next" (top = highest). The board's **Priority** field (P0–P3) is
  *severity metadata* for filtering, not the ordering key.
- The **Type** field (Bug / Feature / Update / Tech-debt / Standards) classifies the work.

## Filing an issue (recipe)

Create the issue in the owning repo and apply a type label:

```bash
gh issue create -R thedrewdz/<repo> \
  --title "<concise imperative title>" \
  --label <bug|enhancement|tech-debt|standards> \
  --body "<context, why it matters, and acceptance criteria; link related issues/docs>"
```

The add-to-project workflow boards it. Triage then sets the board **Type** field, ranks it by
dragging in the `Backlog` view, and (optionally) sets the **Priority** severity. Prefer a body
that is self-contained: the standard/spec it relates to, current state, the required change,
and acceptance criteria.

## Epics and sub-issues

For multi-part efforts, create a tracking **epic** issue and attach the parts as native
**sub-issues** (the board's *Sub-issues progress* field then tracks completion). Keep the
detailed design/evidence dossier under `.agent-output/` and link it from the epic.

## Rules of thumb

- Do not begin non-trivial work that lacks an issue.
- If you discover new work mid-task, file a follow-up issue rather than silently expanding scope.
- Close issues through PRs (`Closes #N`) so the board's *Done* automation fires on merge.
- Never put secrets in issue titles or bodies.
