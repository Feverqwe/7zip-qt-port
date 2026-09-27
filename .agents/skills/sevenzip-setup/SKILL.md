---
name: sevenzip-setup
description: Initialize or refresh the repository-local Sevenzip Desktop Obsidian vault, Git hooks, and development context after checkout or when project tooling needs repair.
---

# Set up Sevenzip Desktop context

Run `.agents/skills/sevenzip-setup/scripts/setup.sh` from any working directory. The script resolves its repository root, creates the ignored `.codex/` Obsidian vault, links tracked project docs, preserves existing local notes and attachments, and enables the tracked pre-commit hook in this repository only. It does not change global Codex or Obsidian settings.

After setup, open `<repo>/.codex` as an Obsidian vault. A new Codex task may be needed for newly added repository skills to appear in skill discovery.
