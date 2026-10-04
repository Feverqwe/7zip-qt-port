---
name: sz-context
description: Read and maintain Sevenzip Desktop context and memory between tasks, including upstream investigations, task handoffs, and durable documentation updates.
---

# Sevenzip Desktop context

Read `.codex/memory/README.md` when present, then only the indexed notes relevant
to the request. Follow [the memory protocol](../../../docs/agent-memory.md) for
verification, saving useful handoffs and index maintenance; the template is
`.codex/templates/handoff.md`. Missing memory does not block work.

Use [the documentation map](../../../docs/README.md) to locate tracked facts.
Check conclusions against current code/tests and upstream provenance, following
`AGENTS.md`. Promote durable findings to the owning document and link from
memory; do not create another status or migration-debt registry.

Use `$sz-setup` only when the local vault needs initialization or refresh.
