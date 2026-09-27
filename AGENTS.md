# Project agent guide

Sevenzip Desktop is a C++17 / Qt 6 Widgets desktop UI inspired by the Windows 7-Zip File Manager. The application targets Windows, macOS, and Linux. The upstream 7-Zip source is a sibling Git checkout at `../7zip-upstream`, not a submodule or vendored directory.

## Scope and ownership

- You may read, add, edit, and remove files in this repository as needed to complete the user's development task. Keep code, docs, build scripts, and tests in sync. Do not stop after a draft when the requested behavior can be built and checked locally.
- Treat `../7zip-upstream` as reference source. Do not modify it or copy its source into this repository unless the user explicitly asks for an upstream change.
- Keep archive operations behind a backend boundary. Start with the official `7zz` executable; isolate process invocation and parsing from widgets so a native integration remains possible.
- Preserve the classic 7-Zip workflow and terminology. Adapt platform-specific controls to Windows, macOS, and Linux. Do not imply disabled or unimplemented commands work.

## Where to look

- `README.md`: current project status and quick start.
- `docs/architecture.md`: UI/backend boundary and upstream relationship; read for integration changes.
- `docs/development.md`: dependencies, build, run, and verification; read for toolchain changes.
- `docs/roadmap.md`: agreed milestones; update when completing or changing a milestone.
- `../7zip-upstream/DOC/License.txt`: upstream license details; consult before packaging or distribution changes.

## Working loop

1. Inspect the relevant code and the current Git status. Preserve unrelated user changes.
2. Implement the requested slice, including meaningful tests when behavior warrants them.
3. Run the affected build and checks. Fix failures introduced by the change. If a platform or dependency is unavailable, report that specifically.
4. Update docs when behavior, commands, dependencies, or architecture change. Give the user a concise summary and verification results.

Use `./scripts/build.sh dev` and `ctest --preset dev --output-on-failure` as the standard local workflow. Format changed C++ files with `clang-format` when available. `scripts/doctor.sh` checks local prerequisites. Do not publish or update upstream unless the user's task calls for it.
