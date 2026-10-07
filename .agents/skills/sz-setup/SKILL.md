---
name: sz-setup
description: Initialize or refresh the repository-local Sevenzip Desktop Obsidian vault, task memory, and development context after checkout or when project tooling needs repair.
---

# Set up Sevenzip Desktop context

Run `.agents/skills/sz-setup/scripts/setup.sh` from any working directory.
It resolves the repository root, creates the ignored `.codex/` Obsidian vault,
initializes the memory index only if absent and links tracked project docs.
Existing memory, notes and attachments are preserved. Navigation pages,
templates and the four bundled vault settings files are refreshed from assets;
workspace/graph settings are untouched. It does not install Git hooks or change
Git configuration or global Codex settings.

Follow [the memory protocol](../../../docs/agent-memory.md).
Setup seeds structure, not task history; never replace an existing memory index
with the template.

After setup, open `<repo>/.codex` as an Obsidian vault. A new Codex task may be needed for newly added repository skills to appear in skill discovery.
