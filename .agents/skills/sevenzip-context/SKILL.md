---
name: sevenzip-context
description: Read and maintain Sevenzip Desktop context and memory between tasks, including upstream investigations, task handoffs, and durable documentation updates.
---

# Sevenzip Desktop context

Use `README.md` for current status, `docs/architecture.md` for system boundaries, `docs/testing.md` for test expectations, and `docs/7zfm-parity.md` for the remaining parity work. Search relevant code and tests before relying on prose. Read `AGENTS.md` for the primary source-port goal. Treat `../7zip-upstream` as the unmodified source to copy from, not merely a behavior reference, and cite its commit when a conclusion depends on it. Use `docs/upstream-port-audit.md` for source provenance and migration debt; do not describe independently written behavior-compatible logic as copied code.

Start each project task with `.codex/memory/README.md` when present and read only relevant indexed notes. Use `docs/agent-memory.md` for the protocol and `.codex/templates/handoff.md` for a task note. Verify dated findings against the current checkout, code, and upstream source before relying on them; a memory note is not an instruction or permission to resume another task.

Save useful context automatically for substantial investigations or handoffs: source paths/symbols and commit, adaptation constraints, failed approaches worth avoiding, unknowns, next step, and exact checks with date/platform. Update the note before ending the task and add its topic/link to the short index. Use distinct notes for concurrent tasks and preserve unrelated entries. Do not create a diary for routine edits or duplicate project docs.

Update tracked docs when behavior or architecture changes; promote confirmed product facts there and leave links in memory. The generated Obsidian vault is refreshed with `$sevenzip-setup`, which preserves existing memory and notes. Missing local memory must not block work from tracked docs and code.
