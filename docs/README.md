# Documentation map

The Qt porting plan is complete; its scope and validation evidence are in the
[archive](archive/README.md). Choose documentation by the current task.
Keep each fact in one authoritative document and link to it elsewhere.

## Where to start

Agents start with [AGENTS.md](../AGENTS.md) and the local memory index, if present,
then read the relevant documentation, code and dependencies. Dated validation
evidence applies only to the recorded revision, platform and scenario.

| Task | Primary documentation |
| --- | --- |
| Learn about the application | [README](../README.md) |
| Report a bug, contribute or check licenses | [Contributing](../CONTRIBUTING.md), [security](../SECURITY.md), [licenses](../THIRD_PARTY_NOTICES.md) |
| Set up, build, run or prepare runtime dependencies | [Development](development.md) |
| Update interface translations | [Localization](localization.md) |
| Select checks and clean up temporary artifacts | [Testing](testing.md) |
| Find workflows, source owners and regressions | [Regression matrix](regression-matrix.md) |
| Find a test suite or UI data producer | [Coverage](testing/coverage.md), [geometry](testing/geometry.md) |
| Understand UI/backend ownership and boundaries | [Architecture](architecture.md) |
| Implement direct read-only access to external ext devices | [First-version plan](direct-device-reading-plan.md) |
| Implement the proposed interface simplification and redesign | [Redesign plan](ui-redesign-plan.md) |
| Check behavior and platform differences | [Parity](7zfm-parity.md) |
| Change layout, text, visibility or resizing | [UI contracts](ui-layout-contracts.md) |
| Extract resources and compare them with widgets | [UI reference tooling](ui-reference.md) |
| Capture an original Windows reference | [Windows reference](windows-ui-reference.md) |
| Compare native runtime geometry across three operating systems | [Runtime comparison](ui-runtime-comparison.md) |
| Configure remote hosts and validate Linux/Windows | [Local configuration](runner-configuration.md), [runner workflows](hyperv-runners.md) |
| Build local packages or understand CI/release scope | [Packaging and CI](ci.md) |
| Check source provenance and adaptation decisions | [Source audit](upstream-port-audit.md), [module decisions](omitted-scope-audit.md) |
| Compare or update upstream | [Baseline manifest](upstream-baseline.json), [update procedure](upstream-update.md) |
| Review the completed UX acceptance scope | [Acceptance](ux-port-audit.md) |
| Save an investigation or handoff | [Memory protocol](agent-memory.md) |
| Find a completed plan, past decision or validation record | [Archive](archive/README.md) |

Skills in `.agents/skills/` provide task-specific workflows. `.codex/` stores local
memory; its notes do not replace documentation or define a backlog.
Ordinary `rg` searches skip the documentation archive. For a specific historical
investigation, use `rg --no-ignore` with the relevant archive directory.

## Maintaining documentation

- Update the primary document when its facts change. Keep the README focused on
  purpose, usage and public limitations; put implementation details in the
  relevant technical document.
- Do not duplicate code, configuration lists, counters or run logs. Verify
  commands and test names against the current scripts, CMake files and tests.
- Link new source decisions and validation evidence from the
  [audit](upstream-port-audit.md), recording the date, revision, OS and scope.
  Update the corresponding architecture, parity or UI contract when it changes.
- Preserve unique commits, upstream symbols, omissions, limitations and reasons
  for deviations. Move completed plans and superseded investigations to the
  archive, updating incoming and relative links.
- Read archive entries for a specific symbol or scenario. Their past statuses,
  iteration rules and next steps are not current instructions.
