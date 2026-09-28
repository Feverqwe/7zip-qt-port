---
name: sz-setup
description: Initialize or refresh the repository-local Sevenzip Desktop Obsidian vault, task memory, and development context after checkout or when project tooling needs repair.
---

# Set up Sevenzip Desktop context

Run `.agents/skills/sz-setup/scripts/setup.sh` from any working directory. The script resolves its repository root, creates the ignored `.codex/` Obsidian vault and initializes the memory index only if absent, links tracked project docs, and preserves existing memory, local notes and attachments. It does not install Git hooks or change Git configuration, global Codex, or Obsidian settings.

Memory usage is defined in `docs/agent-memory.md`. Setup seeds structure, not task history; never replace an existing memory index with the template.

After setup, open `<repo>/.codex` as an Obsidian vault. A new Codex task may be needed for newly added repository skills to appear in skill discovery.
