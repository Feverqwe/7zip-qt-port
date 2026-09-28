# Sevenzip Desktop context map

- `README.md`: project goal, status, and quick start.
- `AGENTS.md`: primary source-port goal and agent working rules.
- `docs/architecture.md`: UI/backend boundary and upstream 7-Zip relationship.
- `docs/development.md`: installation, build, run, and verification.
- `docs/testing.md`: regression strategy.
- `docs/7zfm-parity.md`: implemented and remaining 7zFM behavior.
- `docs/upstream-port-audit.md`: copied/adapted sources, provenance, and custom logic awaiting migration.
- `.agents/skills/`: reusable project workflows.
- `.agents/skills/sevenzip-navigation/`: map Qt, backend, and upstream C++ behavior.
- `.codex/notes/`: local investigations and handoffs; not source of truth.

Follow the source-port goal in `AGENTS.md`: copy upstream implementation with its paths and names, adapt UI to Qt and platform dependencies where needed. Search narrowly. Confirm source provenance against upstream and behavior in code and tests.
