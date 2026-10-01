# Development toolchain

## Dependencies

- C and C++17 compiler, CMake 3.21+, Ninja, Qt 6 Widgets, Git, clang-format
- Python 3.9+ for CTest UI-reference checks (standard library only); clang/GCC
  additionally for extracting resources, not for comparing stored references
- Official `7zz` executable for archive functionality once implemented
- Qt 6 DBus for the Linux launcher service; Unix launcher regression tests also
  need `dbus-daemon` to create their own private bus. macOS builds with testing
  enabled require Qt DBus; Cocoa-only production builds do not.

On macOS with Homebrew:

```sh
brew install cmake ninja qtbase sevenzip clang-format
```

On Linux, install CMake, Ninja, Qt 6 base development files, and clang-format from the distribution. On Windows, use a Qt 6 Widgets kit plus CMake, Ninja, and a C++17 compiler; CI uses MSYS2 UCRT64. Point `CMAKE_PREFIX_PATH` to Qt when CMake cannot locate it.

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
