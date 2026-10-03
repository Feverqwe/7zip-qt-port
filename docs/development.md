# Development toolchain

Native console SFX can be built from the recorded official baseline with
`./scripts/build-sfx.sh [upstream-checkout] [artifact-directory]` on macOS/Linux.
Defaults are the read-only sibling checkout and `build/sfx`. The script extracts
an immutable temporary snapshot, builds upstream SFXCon unchanged and writes
`7zCon.sfx`, its license and a SHA-256/source/host/version JSON record. It refuses
to replace an unowned module artifact. This runtime preparation belongs to the
in-progress P2-02; application SFX consumers are not yet accepted. Windows uses
the official GUI `7z.sfx` runtime; this script does not create a Windows module.

For Windows, use `scripts/build-sfx.ps1 -Architecture arm64` (or `x64`/`x86`)
in a matching Visual Studio developer PowerShell. It requires Git, Windows tar,
MSVC/nmake, linker and resource compiler, snapshots the same recorded revision,
and builds unchanged `Bundles/SFXWin/makefile` with `PLATFORM`. Optional
`-UpstreamCheckout` and `-OutputDirectory` select the read-only source and output.
It writes `7z.sfx`, `7z.sfx.json` and `7z-LICENSE.txt`, refuses unowned/reparse-point
artifacts, and never builds in the sibling checkout. The sidecar producer must
match the Windows builder. Actual unchanged SFXWin compilation passes on
Windows11 ARM64 with MSVC14.44 and SDK10.0.26100.0; native Qt SFX consumer
acceptance is still pending. Tools are installed in the local VM but require its
scoped Visual Studio developer environment rather than the default SSH PATH.
`build.sh` requests process-local English MSVC diagnostics (`VSLANG=1033`)
unless supplied by the caller. It also switches its console to UTF-8 and restores
the previous code page on exit so CMake/Ninja parse `/showIncludes` reliably
even when English compiler resources are not installed.
An existing cache configured with a damaged localized prefix needs rebuilding
from a clean build directory; changing the language alone cannot repair stored
header dependencies. This setting does not change application language.

External codecs require the official dynamic console client, not standalone
`7zz`. `./scripts/build-codec-runtime.sh [upstream-checkout] [artifact-directory]`
builds unchanged `UI/Console` and `Bundles/Format7zF` from the pinned source
snapshot into `build/codec-runtime` on macOS/Linux. It produces `7z`, `7z.so`,
license and source/version/SHA records, and refuses unowned artifacts. The Debug
app uses this prepared runtime when present; deployed builds look beside the app
or in `Resources/7zip`. `SEVENZIP_ENGINE_PATH` explicitly selects an engine;
a missing explicit path reports an error. Standalone `7zz` remains the fallback.
Codecs/Formats discovery follows original LoadCodecs from the confirmed deployment;
native modules must agree with the dynamic engine's reported libraries. P2-02 is
still in progress; remaining plugin consumers are not accepted.
On Windows, `scripts/build-codec-runtime.ps1 -Architecture arm64` (or `x64`/`x86`)
uses a matching developer environment and the same optional source/output arguments
as the SFX builder. It compiles unchanged Console/Format7zF/Alone2 makefiles and
produces `7z.exe`, `7z.dll`, `7zz.exe`, license and runtime manifest. All three
compile on Windows11 ARM64; both clients report26.03. Do not install test-only
codec fixtures in a production runtime.

## Dependencies

- C and C++17 compiler, CMake 3.21+, Ninja, Qt 6 Widgets, Git, clang-format
- Python 3.9+ for CTest UI-reference checks (standard library only); clang/GCC
  additionally for extracting resources, not for comparing stored references
- Official `7zz`, or the prepared dynamic `7z`/module runtime for external codecs
- Qt 6 DBus for the Linux launcher service; Unix launcher regression tests also
  need `dbus-daemon` to create their own private bus. macOS builds with testing
  enabled require Qt DBus; Cocoa-only production builds do not.

On macOS with Homebrew:

```sh
brew install cmake ninja qtbase sevenzip clang-format
```

On Linux, install CMake, Ninja, Qt 6 base development files, clang-format,
pkg-config and GLib/GIO development files (`libglib2.0-dev` on Ubuntu). The native
association adapter links `gio-unix-2.0`. On Windows, use a Qt 6 Widgets kit plus
CMake, Ninja, and a C++17 compiler; CI uses MSYS2 UCRT64. Point
`CMAKE_PREFIX_PATH` to Qt when CMake cannot locate it.

## Local commands

```sh
sh .agents/skills/sz-setup/scripts/setup.sh
./scripts/doctor.sh
./scripts/check.sh
```

`check.sh` checks formatting, builds with the development CMake preset, and runs CTest. Both `check.sh` and `ctest.sh` run up to four tests in parallel by default. CTest resource locks serialize tests that write to the shared UI report or screenshot directories. Set `SEVENZIP_CTEST_JOBS=1` for a sequential run or another positive number to change concurrency; `CTEST_PARALLEL_LEVEL` and an explicit `--parallel` argument to `ctest.sh` also take precedence. CTest reports failed suites through `--output-on-failure` and continues running the remaining suites. For a release build, run `./scripts/build.sh release` and `./scripts/ctest.sh --preset release --output-on-failure`.

Qt Test suites run with `-silent`, so a failed suite reports its failed cases without listing the cases that passed. `ctest.sh` filters CTest's `Start` and successful `Passed` lines as they arrive; failed suite status, diagnostics, and the final summary remain visible. Each retained line is flushed immediately, and a failed CTest still returns a nonzero exit status.

`check.sh` exits before any work when `CODEX_SANDBOX` identifies an active sandbox. The complete suite needs an unsandboxed run: DBus socket creation and native process-priority tests fail with `Operation not permitted` in the Codex sandbox. Focused tests can still be run there when their required system calls are permitted.

`build.sh` uses CMake `--log-level=WARNING`, discards configuration stdout, and builds with Ninja `--quiet`: successful configuration and build progress lines are hidden, while warnings and errors remain visible as they occur. `ctest.sh` streams CTest output directly, including failure details when `--output-on-failure` is passed. When `CODEX_SESSION_ID` is set, only the formatting phase of `check.sh` buffers output; on failure it shows the last 30 log lines and the path to the full temporary log. Set `SEVENZIP_SCRIPT_OUTPUT=verbose` to see that formatting output as it runs. `ctest.sh` passes its arguments directly to CTest after setting the default parallel level; agents use it for both focused and full test runs.

If `ccache` is installed, `build.sh` automatically uses it for C, C++ and Objective-C++ compilation, including builds started by `check.sh`. Install it with `brew install ccache` on macOS or your package manager on Linux. It speeds up recompilation after a clean build or branch switch; Ninja already skips unchanged files, and the full CTest suite still runs on every `check.sh` invocation.
On macOS the script defaults `CCACHE_DIR` to `/private/tmp/sevenzip-desktop-ccache`, which is writable in the local sandbox and reuses the existing cache. Set `CCACHE_DIR` before running the script to use another directory.

During implementation, build only affected targets with `./scripts/build.sh dev <target> [target ...]` and run relevant cases with `./scripts/ctest.sh --preset dev -R '<test-name-regex>' --output-on-failure`. The build script configures the preset and applies platform toolchain workarounds; without targets it builds everything. Run the full `check.sh` once before completing code, test, or build-tooling changes; do not repeat a passing gate on unchanged code. After a localized edit, rerun affected checks; repeat the full gate for broad changes or unresolved regression concerns. Documentation-only and agent-instruction edits need `git diff --check`.

Run the built app on macOS from `build/dev/sevenzip_desktop.app/Contents/MacOS/sevenzip_desktop`, on Linux from `build/dev/sevenzip_desktop`, or on Windows from `build/dev/sevenzip_desktop.exe`. Build outputs are ignored. The development preset exports `compile_commands.json` for clangd and editor navigation.

Some Apple Command Line Tools installations contain C++ standard headers inside the macOS SDK but omit them from Clang's default include path. `scripts/build.sh` detects this and sets `CPLUS_INCLUDE_PATH` only for that build process; it does not alter system files.

## Automation

Use the existing shared x64 Hyper-V guests on Windows host `192.168.1.14`:

```sh
python3 scripts/hyperv-runner.py probe linux
python3 scripts/hyperv-runner.py probe windows
python3 scripts/hyperv-runner.py check linux
python3 scripts/hyperv-runner.py check windows
```

Add `--preset release` to run Debug's full gate followed by the Release build
and CTest. The helper uses the global `runners` skill's authenticated readiness
command, starts only the requested guest if needed, and discovers its current
DHCP address. Host storage stays on `D:\runner`; project snapshots and logs
use separate Sevenzip guest directories. See [Hyper-V runners](hyperv-runners.md)
for toolchains, pinned engine preparation, snapshot scope and validation results.
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
