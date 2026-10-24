# Persistent developer verification workflow (2026-10-17)

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

## Source boundary

This is project-owned Python/Bash/CMake integration, not archive behavior or a
replacement 7-Zip implementation. Official 7-Zip has no Qt/CMake/Hyper-V focused
verification runner to reuse. The existing helper's provenance and full
commits remain in [third-party notices](../../../../THIRD_PARTY_NOTICES.md#tooling-notice).
No additional external tooling implementation was copied in this change.

Runtime preparation still uses official source from
`https://github.com/ip7z/7zip.git` at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, selected by the existing baseline.
Existing builders use `CPP/7zip/UI/Console`, `CPP/7zip/Bundles/Format7zF`,
`CPP/7zip/Bundles/Alone2`, Unix `CPP/7zip/Bundles/SFXCon` or Windows
`CPP/7zip/Bundles/SFXWin`, their original makefile dependencies, `C/7zVersion.h`
and `DOC/License.txt`. The Unix builder now also publishes Alone2's `7zz` and
hash in `runtime.json`, replacing the runner's separate standalone build.
Makefiles, algorithms, C/C++ language boundaries and notices are unchanged;
no new upstream source is copied or omitted. The sibling checkout is read-only.

`prepare-runtime.py` adds only a verified artifact cache around those builders.
The pinned bundle lives in a runner-owned bare object store. Cache identity
includes source, builder code, compiler/toolchain environment and artifact hashes;
failure cannot publish a cache stamp.

## Workflow boundary

The user's 2026-10-17 instruction replaces per-invocation guest snapshots with
one `current` worktree per selected platform. Rsync synchronizes current tracked
and nonignored new project source, deletes removed source, uses low-level
compression and checksum comparison, and protects build/runtime/control outputs.
The transfer is a temporary local source copy, not a second build tree.
Ownership, an atomic lock and verified completion guard sync/build/log collection;
unknown remote execution keeps its lock. See [the contract](../../../hyperv-runners.md#persistent-worktree).

`verify.py` selects scenario/regex suites through `ctest.sh`, expands CTest
fixtures and maps generated test-command executable paths to actual CMake
targets before building. Script tests need no guessed executable target.
Empty selections fail and slow suites keep the explicit opt-in boundary.
Ninja checks CMake input/glob dependencies before selection; changed wrapper,
preset, options/environment or an externally edited cache forces configuration.
Reports include phase timing, actual compiler launchers and ccache counters.

`SharedWorkflowObjects.cmake` compiles existing `qtpanelcontextmenu.cpp`,
`layout_inspection.cpp`, `fixtures/widget_dialogs.cpp` and `panel_load_folder.cpp`
once each. Bodies and public behavior are unchanged; the modules do not consume
test-specific definitions. Consumers retain their link closures, generated Qt
code and public test fixtures. This follows the existing shared-object boundary;
it adds no new product class, UI layout or parity claim.

## Verification

Focused checks cover the tooling and direct consumers of each shared module,
plus application link/startup. No full or opt-in slow gate is claimed.

2026-10-17, local macOS and shared Linux/Windows x64 Qt 6.12.0 runners,
`d4591d15aa57943455125f6d9e314ae8f6b7a764` plus worktree changes:

- macOS: 13 focused suites (tooling plus root/address/panel, shared prompts,
  drag/load, Options and application startup) passed. After the final test-only
  refinements, all five tooling suites passed again. Application link passed.
  The local official runtime/SFX cache was prepared from pinned source in
  16.76s; the next preparation hit in 0.02s. Its native-library, capabilities,
  shared-prompt and application-startup consumers passed separately (4/4).
- Linux/Windows: final shared source manifest
  `96960ed709a96965a2e344db18c3daf6393a2eae2ad004fe082d4166bd524080`
  is recorded in local run `20261017T102220Z-e91d61bd`. Both platforms passed
  ten suites: shared prompts, drag, panel load, Options, application startup and
  five tooling suites. Windows used the logged-on ordinary token with normal
  process priority; no elevated acceptance is claimed.
- The behavioral fixtures exercise an unbuilt CLI project, required setup
  producer, omitted unrelated target, unchanged binary reuse, avoided redundant
  configuration, CMake test-registration changes, plan/result separation and
  empty/slow selections. Runner/cache fixtures cover actual directory rsync,
  deletion/protection, unchanged transfer, Unicode/space/metacharacters,
  interrupted lock retention, cache corruption/invalidation and failed rebuilds.
  No source/template string or call-order assertions are used.
- Repeat-run phase seconds: Linux sync 0.482, configure 0.147, build 0.081,
  tests 11.581; Windows sync 1.044, configure 0.575, build 0.485, tests 65.985.
  These are focused-run observations, not a full-gate benchmark. Both runtime
  caches hit; actual C/C++ launchers point to ccache. Initial project compiler
  statistics reported every call cacheable on both guests. Windows's separate
  MSVC probe demonstrated one miss then one direct cache hit.
- `git diff --check` and shell syntax checks passed. No full, slow, Release,
  packaging or rendered-reference acceptance is inferred. Temporary fixtures
  and local transfer copies were removed; existing build/cache outputs and
  current review/failure logs were retained.

Production source and navigation-test changes already present in the worktree
were preserved. This workflow task did not edit their behavior, stage files,
commit, publish or update upstream.
