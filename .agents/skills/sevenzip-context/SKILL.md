---
name: sevenzip-context
description: Find and maintain Sevenzip Desktop project knowledge, architecture decisions, and Obsidian-backed notes when a task needs project context or a durable documentation update.
---

# Sevenzip Desktop context

Use `README.md` for current status, `docs/architecture.md` for system boundaries, `docs/testing.md` for test expectations, and `docs/7zfm-parity.md` for the remaining parity work. Search relevant code and tests before relying on prose. Read `AGENTS.md` for the primary source-port goal. Treat `../7zip-upstream` as the unmodified source to copy from, not merely a behavior reference, and cite its commit when a conclusion depends on it. Use `docs/upstream-port-audit.md` for source provenance and migration debt; do not describe independently written behavior-compatible logic as copied code.

Update tracked docs when behavior or architecture changes. Write local investigation or handoff notes in `.codex/notes/` only when the user asks to preserve them or a future agent needs a concrete handoff. Do not duplicate maintained docs there. The generated Obsidian vault is refreshed with `$sevenzip-setup`.
