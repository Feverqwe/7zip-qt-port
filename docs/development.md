# Development toolchain

## Dependencies

- C++17 compiler, CMake 3.21+, Ninja, Qt 6 Widgets, Git, clang-format
- Python 3.9+ for CTest UI-reference checks (standard library only); clang/GCC
  additionally for extracting resources, not for comparing stored references
- Official `7zz` executable for archive functionality once implemented

On macOS with Homebrew:

```sh
brew install cmake ninja qtbase sevenzip clang-format
```

On Linux, install CMake, Ninja, Qt 6 base development files, and clang-format from the distribution. On Windows, use a Qt 6 Widgets kit plus CMake, Ninja, and a C++17 compiler; CI uses MSYS2 UCRT64. Point `CMAKE_PREFIX_PATH` to Qt when CMake cannot locate it.

## Local commands

```sh
sh .agents/skills/sevenzip-setup/scripts/setup.sh
./scripts/doctor.sh
./scripts/check.sh
```

`check.sh` checks formatting, builds with the development CMake preset, and runs CTest. For a release build, run `./scripts/build.sh release` and `ctest --preset release --output-on-failure`.

During implementation, build only affected targets with `./scripts/build.sh dev <target> [target ...]` and run relevant cases with `ctest --preset dev -R '<test-name-regex>' --output-on-failure`. The build script configures the preset and applies platform toolchain workarounds; without targets it builds everything. Run the full `check.sh` once before completing code, test, or build-tooling changes; do not repeat a passing gate on unchanged code. After a localized edit, rerun affected checks; repeat the full gate for broad changes or unresolved regression concerns. Documentation-only and agent-instruction edits need `git diff --check`.

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
