---
name: sevenzip-context
description: Find and maintain Sevenzip Desktop project knowledge, architecture decisions, and Obsidian-backed notes when a task needs project context or a durable documentation update.
---

# Sevenzip Desktop context

Use `README.md` for current status, `docs/architecture.md` for system boundaries, `docs/testing.md` for test expectations, and `docs/roadmap.md` for milestones. Search relevant code and tests before relying on prose. Treat `../7zip-upstream` as reference source and cite its commit when a conclusion depends on it.

Update tracked docs when behavior or architecture changes. Write local investigation or handoff notes in `.codex/notes/` only when the user asks to preserve them or a future agent needs a concrete handoff. Do not duplicate maintained docs there. The generated Obsidian vault is refreshed with `$sevenzip-setup`.
