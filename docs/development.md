# Development toolchain

The native panel keyboard adapter requires the private development headers of
the selected Qt GUI SDK (`Qt6::GuiPrivate`). Install those headers with the same
Qt build as Core/Gui/Widgets; on distribution SDKs this can be a separate private
development package. `qtnativepanelkeys.cpp` uses Qt's native keyboard-layout
mapping instead of maintaining another platform key map. This private API has
no ABI compatibility guarantee: build and deploy against the same Qt module
build, and rebuild the application when changing it. CMake suppresses Qt's generic
private-module warning and rejects a missing matching private SDK. Bundled packages must carry that
same Qt runtime; this adds no separate runtime module.

Native console SFX can be built from the recorded official baseline with
`./scripts/build-sfx.sh [upstream-checkout] [artifact-directory]` on macOS/Linux.
Defaults are the read-only sibling checkout and `build/sfx`. The script extracts
an immutable temporary snapshot, builds upstream SFXCon unchanged and writes
`7zCon.sfx`, its license and a SHA-256/source/host/version JSON record. It refuses
to replace an unowned module artifact. Source scope and acceptance evidence are
in [the port audit](upstream-port-audit.md). Windows uses
the official GUI `7z.sfx` runtime; this script does not create a Windows module.

For Windows, use `scripts/build-sfx.ps1 -Architecture arm64` (or `x64`/`x86`)
in a matching Visual Studio developer PowerShell. It requires Git, Windows tar,
MSVC/nmake, linker and resource compiler, snapshots the same recorded revision,
and builds unchanged `Bundles/SFXWin/makefile` with `PLATFORM`. Optional
`-UpstreamCheckout` and `-OutputDirectory` select the read-only source and output.
It writes `7z.sfx`, `7z.sfx.json` and `7z-LICENSE.txt`, refuses unowned/reparse-point
artifacts, and never builds in the sibling checkout. The sidecar producer must
match the Windows builder. Use a scoped Visual Studio developer environment
rather than relying on the default SSH PATH.
`build.sh` requests process-local English MSVC diagnostics (`VSLANG=1033`)
unless supplied by the caller. It also switches its console to UTF-8 and restores
the previous code page on exit so CMake/Ninja parse `/showIncludes` reliably
even when English compiler resources are not installed.
This setting does not change application language. If stored header dependencies
appear damaged, collect diagnostics before proposing a cache/build-tree repair;
follow [cache retention](../AGENTS.md#validation-scope).

External codecs require the official dynamic console client, not standalone
`7zz`. `./scripts/build-codec-runtime.sh [upstream-checkout] [artifact-directory]`
builds unchanged `UI/Console` and `Bundles/Format7zF` from the pinned source
snapshot into `build/codec-runtime` on macOS/Linux. It produces `7z`, `7zz`, `7z.so`,
license and source/version/SHA records, and refuses unowned artifacts. The Debug
app uses this prepared runtime when present; deployed builds look beside the app
or in `Resources/7zip`. `SEVENZIP_ENGINE_PATH` explicitly selects an engine;
a missing explicit path reports an error. Standalone `7zz` remains the fallback.
Codecs/Formats discovery follows original LoadCodecs from the confirmed deployment;
native modules must agree with the dynamic engine's reported libraries.
The immutable codec image uses Qt's selected temporary directory to copy and
load its module. That directory must permit writes and executable mappings.
On Linux with a `noexec` default temporary filesystem, a process-scoped `TMPDIR`
can select an approved private directory with those capabilities. The isolated
[runtime check](archive/qt-port/upstream-port-audit.md#p3-04--isolated-linux-runtime-and-temporary-directory-contract-2026-10-10)
verifies this override, cleanup and unchanged archives; it changes no system policy.
Native regression fixtures also cache `SEVENZIP_CODEC_MODULE` in CMake. When
preparing a fresh checkout or changing deployments, configure that variable to
the same pinned module, for example `cmake --preset dev
-DSEVENZIP_CODEC_MODULE="$PWD/build/codec-runtime/7z.so"` on macOS/Linux
(`7z.dll` on Windows), before building and running the gate. Copying runtime
artifacts alone does not replace an already cached system-module path.
Completed package evidence and its limits are linked from [acceptance](ux-port-audit.md).
On Windows, `scripts/build-codec-runtime.ps1 -Architecture arm64` (or `x64`/`x86`)
uses a matching developer environment and the same optional source/output arguments
as the SFX builder. It compiles unchanged Console/Format7zF/Alone2 makefiles and
produces `7z.exe`, `7z.dll`, `7zz.exe`, license and runtime manifest.
Do not install test-only codec fixtures in a production runtime. Earlier ARM64
builder samples are retained in [the tooling archive](archive/qt-port/workflow-evidence.md).

Windows CMake installation places the Qt application, original7zFM/7zG entry
aliases, project shell DLL and official `7z.exe`/`7z.dll` runtime together in `bin`.
The prepared standalone `7zz.exe` is a development fallback and is not installed.
`cmake --install build/dev --prefix <private-directory>` deploys Qt runtime,
platform plugins and compiler runtime using Qt's own deployment script. It
first checks the prepared runtime's producer, repository, pinned revision,
architecture and binary SHA256 values. Runtime provenance and7-Zip license are
installed under `share`; installation does not register shell handlers or
write associations. `SEVENZIP_WINDOWS_RUNTIME` can select another prepared
runtime directory at configure time.

`scripts/check-windows-package.sh` builds and installs into the snapshot's
`build/windows-package`, then exercises its actual DLL and GUI helper with
only package/system paths in the child process environment. Run it on the
interactive Windows runner. This private test layout is not a release package
or final license/distribution acceptance; see [packaging scope](ci.md).

## Dependencies

- C and C++17 compiler, CMake 3.21+, Ninja, Qt 6 Widgets, Git, clang-format
- Python 3.9+ for CTest UI-reference checks (standard library only); clang/GCC
  additionally for extracting resources, not for comparing stored references
- Official `7zz`, or the prepared dynamic `7z`/module runtime for external codecs
- Qt 6 DBus for the Linux launcher service; Unix launcher regression tests also
  need `dbus-daemon` to create their own private bus. macOS builds with testing
  enabled require Qt DBus and libdbus alongside `dbus-daemon`; Cocoa-only
  production builds do not. The two private-bus tests set the discovered libdbus
  directory in their own loader environment for official Qt kits.

macOS builds target macOS 14.4+ and use the official shared Qt 6.12.0 kit,
matching the runner version. Install the macOS desktop Qt Base SDK (including
private headers) with the [Qt installer](https://www.qt.io/development/download-qt-installer-oss)
under `build/qt`; the default kit path is `build/qt/6.12.0/macos`.
For an SDK installed elsewhere, set `CMAKE_PREFIX_PATH` to its `macos` directory.
Build and deploy with the same SDK. Homebrew supplies the other local tools:

```sh
brew install cmake ninja sevenzip clang-format ccache
```

On Linux, install CMake, Ninja, Qt 6 base development files, clang-format,
pkg-config and GLib/GIO development files (`libglib2.0-dev` on Ubuntu). The native
association adapter links `gio-unix-2.0`. On Windows, use a Qt 6 Widgets kit plus
CMake, Ninja, and a C++17 compiler. The shared Windows runner uses the
MSVC2022/Qt6.12.0 kit; see [runtime artifacts and CI scope](ci.md). Point
`CMAKE_PREFIX_PATH` to Qt when CMake cannot locate it.

## Local commands

For a focused correction, use one command to configure, build the selected
targets and run their CTest suites (including required fixtures):

```sh
python3 scripts/verify.py --list
python3 scripts/verify.py navigation
python3 scripts/verify.py archive-open
python3 scripts/verify.py options --plan
python3 scripts/verify.py --tests '^address_combo$'
python3 scripts/verify.py navigation --platform all
```

Profiles in `scripts/config/verify-profiles.json` define scenario scope and whether a
Windows desktop is needed. They are starting points: extend the selection for
other affected consumers. `--tests` overrides a profile's CTest regex. Targets
come from generated CMake/CTest registrations, including executable arguments
and fixture producers; their names are not guessed from suite names. An empty
enabled selection fails. Slow suites retain their explicit opt-in requirement.
`--plan` configures and prints the selection without building, testing or
preparing runtime. Verification uses the `dev` preset by default; the `release`
preset disables test programs for application-only builds.

Keep command entrypoints in `scripts/`, shared implementation and build helpers
in `scripts/lib/`, and scenario configuration in `scripts/config/`.

`verify.py` keeps `build/<preset>/verify-result.json` with the selection,
targets, result and time spent on runtime/configuration/selection/build/tests.
It uses `build.sh --configure-only`, then `build.sh --build-only <targets>` to
avoid configuring twice. Both modes retain the build wrapper's toolchain setup;
the latter requires an existing CMake cache. Plan output uses `verify-plan.json`
and preserves the last verification result. The build wrapper checks the
requested options/environment and cache signature; when unchanged, Ninja checks
CMake inputs/globs and regenerates only when needed, before test selection.
Changed wrapper/preset/options/environment or an externally edited CMake cache
triggers normal configuration. Incremental builds retain the same directory.

Runtime preparation uses `python3 scripts/prepare-runtime.py`: the official
builders run only when the pinned upstream revision/repository, builder code,
compiler/toolchain fingerprint or an artifact hash changes. A failed build does
not publish a cache hit. The Unix runtime builder also supplies pinned `7zz`.
`--skip-runtime` on verify uses the existing runtime instead. When the prepared
module exists, `build.sh` configures its exact path, replacing a stale cached
system module. The sibling upstream checkout stays read-only.

Remote verification uses the same command in the single persistent guest
worktree; see [sync, locks and recovery](hyperv-runners.md#persistent-worktree).
There are no per-run source/build directories on the guests. Distinct run IDs
identify local logs and timing records only.

Shared Linux/Windows runners enable `SEVENZIP_COMPACT_TEST_BINARIES=ON`.
The build wrapper forwards this environment setting to the CMake option of the
same name. Linux test executables omit linked DWARF debug sections; MSVC test
executables omit linker PDB/ILK files and remove unreferenced code, while retaining
distinct function addresses. Compiler debug information, Debug assertions and
runtime checks are retained, as are production application symbols. Ordinary
local builds default to OFF. For debugger work, rebuild the affected test with
`SEVENZIP_COMPACT_TEST_BINARIES=OFF ./scripts/build.sh dev <target>` in the same
worktree/toolchain. The option does not delete artifacts from older builds.

Runtime packages use Release builds with testing disabled; local checks remain
unchanged. GitHub Actions builds release packages; see [CI scope](ci.md).
For a local application with its runtime dependencies, run `./scripts/build-app.sh`.
It installs into `dist/` (`dist/Sevenzip.app` on macOS); intermediate build files
remain in `build/`. The local runtime receives the same size optimizations as
the archive payload, including native-architecture stripping and verified ad-hoc
signing on macOS. Its size also depends on the selected Qt kit and its dependencies.
The installation replaces the previous runtime only after preparation succeeds,
so dependencies from an older Qt kit do not remain in `dist/`.
To package an existing Release build, run `python3 scripts/package.py
--qt-source <matching-qtbase.tar.xz> --qt-sha256 <checksum-file>`; add
`--icu-license <license-file>` if the deployed Qt kit uses ICU. The script uses
a fresh CMake installation and writes only runtime archives to `build/packages`.
Source inputs stay private. macOS packages have verified ad-hoc signatures.
The install includes the project's MIT notice under `Notices/sevenzip-desktop`
in the macOS bundle or `share/licenses/sevenzip-desktop/sevenzip-desktop`
on Linux/Windows; engine and dependency notices remain separate.
Windows additionally requires `--vc-runtime <Microsoft.VC143.CRT-directory>`
and `--vc-notices <Visual-Studio-Redist.txt>`; only imported CRT DLLs are copied.
See [CI and release helper](ci.md) for exact artifact scope and release commands.

```sh
sh .agents/skills/sz-setup/scripts/setup.sh
./scripts/doctor.sh
./scripts/build.sh dev <targets>
./scripts/ctest.sh --preset dev -R '<test-name-regex>' --output-on-failure
```

Focused checks are the default for agent work. Select targets and suites using
[testing](testing.md); use `./scripts/check.sh` only when the full-gate criteria
in [AGENTS.md](../AGENTS.md#validation-and-ui-reference) apply. Before each batch,
follow [artifact retention](testing.md#test-artifact-retention). The wrappers do
not automatically prune old runs; the agent performs the scoped cleanup.

`check.sh` checks formatting, builds with the development CMake preset, and runs CTest with long suites excluded. Both `check.sh` and `ctest.sh` run up to four tests in parallel by default. CTest resource locks serialize tests that write to the shared UI report or screenshot directories. Set `SEVENZIP_CTEST_JOBS=1` for a sequential run or another positive number to change concurrency; `CTEST_PARALLEL_LEVEL` and an explicit `--parallel` argument to `ctest.sh` also take precedence. CTest reports failed suites through `--output-on-failure` and continues running the remaining suites. For a release build, run `./scripts/build.sh release` and `./scripts/ctest.sh --preset release --output-on-failure`.

Long suites are excluded by default, including focused `-R` selections;
see [opt-in commands](#long-suites). Agents follow the explicit-request rule
in [AGENTS.md](../AGENTS.md#validation-and-ui-reference).

Qt Test suites run with `-silent`, so a failed suite reports its failed cases without listing the cases that passed. `ctest.sh` filters CTest's `Start` and successful `Passed` lines as they arrive; failed suite status, diagnostics, and the final summary remain visible. Each retained line is flushed immediately, and a failed CTest still returns a nonzero exit status.

`check.sh` exits before any work when `CODEX_SANDBOX` identifies an active sandbox. The complete suite needs an unsandboxed run: DBus socket creation and native process-priority tests fail with `Operation not permitted` in the Codex sandbox. Focused tests can still be run there when their required system calls are permitted.

`build.sh` uses CMake `--log-level=WARNING`, discards configuration stdout, and builds with Ninja `--quiet`: successful configuration and build progress lines are hidden, while warnings and errors remain visible as they occur. `ctest.sh` streams CTest output directly, including failure details when `--output-on-failure` is passed. When `CODEX_SESSION_ID` is set, only the formatting phase of `check.sh` buffers output; on failure it shows the last 30 log lines and the path to the full temporary log. Set `SEVENZIP_SCRIPT_OUTPUT=verbose` to see that formatting output as it runs. `ctest.sh` forwards arguments to CTest after setting the default parallel level and combining caller label exclusions with the default slow-suite exclusion; agents use it for both focused and default gate runs.

The buffered log belongs to the shell that called `sevenzip_setup_output`.
Background progress notifiers cannot finalize or remove that parent's log,
including when a fast command cancels a notifier before its traps are installed.
`agent_output` covers fast-command termination, delayed progress and failure-log
retention across the supported Bash versions.

If `ccache` is installed, `build.sh` automatically uses it for C, C++ and Objective-C++ compilation, including builds started by `check.sh`. Install it with `brew install ccache` on macOS or your package manager on Linux. It speeds up recompilation after a clean build or branch switch; Ninja already skips unchanged files, and the full CTest suite still runs on every `check.sh` invocation.
On macOS the script defaults `CCACHE_DIR` to `/private/tmp/sevenzip-desktop-ccache`, which is writable in the local sandbox and reuses the existing cache. Set `CCACHE_DIR` before running the script to use another directory.
Windows ccache uses CMake 3.25+ with embedded MSVC `/Z7` debug information;
the project runners provide that toolchain. `verify-result.json` records actual
compiler launchers and ccache hit/miss deltas without resetting shared statistics.

Validation reuses the existing incremental build and compiler cache. Full/release
checks do not imply a cold build; clearing or bypassing the cache requires an
explicit user request under [the validation rules](../AGENTS.md#validation-scope).

Build specific targets with `./scripts/build.sh dev <target> [target ...]` and run
focused cases with `./scripts/ctest.sh --preset dev -R '<test-name-regex>' --output-on-failure`.
The build script configures the preset and applies platform toolchain workarounds;
without targets it builds everything. Requirements for focused/full validation,
repeat runs and docs-only changes are in [AGENTS.md](../AGENTS.md#validation-and-ui-reference).

Run the built app on macOS from `build/dev/Sevenzip.app/Contents/MacOS/Sevenzip`, on Linux from `build/dev/sevenzip_desktop`, or on Windows from `build/dev/sevenzip_desktop.exe`. Build outputs are ignored. The development preset exports `compile_commands.json` for clangd and editor navigation.

Some Apple Command Line Tools installations contain C++ standard headers inside the macOS SDK but omit them from Clang's default include path. `scripts/build.sh` detects this and sets `CPLUS_INCLUDE_PATH` only for that build process; it does not alter system files.

## Long suites

The stable list is in [SlowTests.cmake](../cmake/SlowTests.cmake), based on
suites observed above 30 seconds on macOS; other platforms may differ.
Agents run these only on an explicit request for long tests or the exhaustive
suite, following [AGENTS.md](../AGENTS.md#validation-and-ui-reference).

`copy_archive_refresh` is also opt-in: its full native matrix has192 workflows.
The corrected Cocoa fixture completed89 without row failures before the120s
whole-suite cap. Its macOS cap is540s, using twice the extrapolated full time
rounded to a minute; Windows retains its measured360s cap. This changes neither
individual state waits nor the application codec-discovery deadline.
Its new full three-platform replay was explicitly declined on2026-10-17;
earlier receipts retain their original revision and scope. Exclusion is not PASS.

```sh
# Only long suites:
SEVENZIP_INCLUDE_SLOW_TESTS=1 ./scripts/ctest.sh --preset dev -L slow --output-on-failure
# One explicitly requested long suite (build its targets first):
SEVENZIP_INCLUDE_SLOW_TESTS=1 ./scripts/ctest.sh --preset dev -R '^native_rename_2$' --output-on-failure
# Exhaustive local gate:
SEVENZIP_INCLUDE_SLOW_TESTS=1 ./scripts/check.sh
# Exhaustive gate on both existing runners:
SEVENZIP_INCLUDE_SLOW_TESTS=1 python3 scripts/hyperv-runner.py check all --interactive --elevated
```

Remote focused checks must build their targets after source synchronization in
the persistent worktree; use `verify.py` or follow [Hyper-V runners](hyperv-runners.md).
A default gate does not
establish acceptance of excluded slow suites.

## Automation

Remote checks use the hosts and toolchains in
[local runner configuration](runner-configuration.md). These optional commands
require authenticated SSH access; contributors can build and test locally without them:

```sh
python3 scripts/hyperv-runner.py probe linux
python3 scripts/hyperv-runner.py probe windows
python3 scripts/hyperv-runner.py check linux
python3 scripts/hyperv-runner.py check windows
```

Add `--preset release` to run Debug's full gate followed by the Release build
and CTest. The helper uses either the configured SSH connection or an optional
readiness command for VM startup and address discovery. Each guest reuses its
configured Sevenzip `current` worktree. See [Hyper-V runners](hyperv-runners.md)
for toolchains, pinned engine caching, source sync scope and validation results.
This is project integration glue; upstream 7zFM has no runner implementation.

The old `scripts/runner.py` wrapper is retained for explicitly selected local
QEMU/UTM workflows supported by the global skill. Historical ARM64 VM checks
do not validate the current Hyper-V guests; the removed htopQt QEMU VMs must
not be selected as the default route.

Local task memory is initialized by the setup script in `.codex/memory/` and
`.codex/notes/`. Repeated setup preserves the populated index, notes, and
attachments; it refreshes navigation pages and templates. See
[the memory protocol](agent-memory.md) for reading, updating, and transferring
memory between checkouts.

There is no project pre-commit hook. Run `scripts/check.sh` manually according to `AGENTS.md`; setup does not install hooks or modify Git configuration. Local tests run on the current machine; separate platform checks are needed to establish cross-platform results.

## Upstream checkout

The sibling `../7zip-upstream` should point to `https://github.com/ip7z/7zip.git`. Clone it separately when absent:

```sh
git clone https://github.com/ip7z/7zip.git ../7zip-upstream
```

Keep upstream updates separate from UI commits. Record the upstream version or commit in a release note when packaging.
Use [the pinned baseline and update review procedure](upstream-update.md) to
verify the exact borrowed files and compare old/new source before transferring
changes into adapted Qt modules. The original sibling checkout stays read-only.
