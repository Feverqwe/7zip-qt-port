---
name: sz-setup
description: Initialize or refresh repository-local Sevenzip Desktop Markdown memory and documentation links after checkout or when agent context needs repair.
---

# Set up Sevenzip Desktop context

Run `.agents/skills/sz-setup/scripts/setup.sh` from any working directory.
It resolves the repository root, creates the ignored `.codex/` memory directory,
initializes the memory index only if absent and links tracked project docs.
Existing memory, notes and attachments are preserved. Only task templates are
refreshed from assets; GUI settings and extra navigation pages are not created
or refreshed. It does not install Git hooks or change
Git configuration or global Codex settings.

Follow [the memory protocol](../../../docs/agent-memory.md).
Setup seeds structure, not task history; never replace an existing memory index
with the template.

Start reading at `.codex/memory/README.md`. No GUI is needed.
