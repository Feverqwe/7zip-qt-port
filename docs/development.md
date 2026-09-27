# Development toolchain

## Dependencies

- C++17 compiler (Apple Clang, GCC, or MSVC)
- CMake 3.21 or newer and Ninja
- Qt 6 with Widgets
- Official `7zz` executable for archive operations when the backend is implemented
- Git
- clang-format for C++ formatting

On macOS with Homebrew, install the local development dependencies with:

```sh
brew install cmake ninja qtbase sevenzip clang-format
```

Qt installed through another distribution may need `CMAKE_PREFIX_PATH` set to its CMake package directory. For Windows, use a Qt 6 Widgets kit plus CMake, Ninja, and MSVC; for Linux, install the equivalent distribution packages and `7zz` from an official 7-Zip build or package.

## Commands

From the repository root:

```sh
./scripts/doctor.sh
./scripts/build.sh dev
ctest --preset dev --output-on-failure
clang-format --dry-run --Werror src/main.cpp
```

The build script uses Homebrew's Qt path when available. On systems with Qt installed elsewhere, set `CMAKE_PREFIX_PATH` first. For a direct macOS configure, use:

```sh
cmake --preset dev -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
```

Run the built app on macOS from `build/dev/sevenzip_desktop.app/Contents/MacOS/sevenzip_desktop`, on Linux from `build/dev/sevenzip_desktop`, or on Windows from `build/dev/sevenzip_desktop.exe`.

`build/` is ignored. `CMAKE_EXPORT_COMPILE_COMMANDS` is enabled in the development preset for editor tooling. CTest currently checks that the Qt application starts in offscreen mode. Add focused tests alongside functional archive code.

Some Apple Command Line Tools installations contain C++ standard headers inside the macOS SDK but omit them from Clang's default include path. `scripts/build.sh` detects this and sets `CPLUS_INCLUDE_PATH` only for that build process. It does not alter system files.

## Upstream checkout

The sibling `../7zip-upstream` should point to `https://github.com/ip7z/7zip.git`. Clone it separately when absent:

```sh
git clone https://github.com/ip7z/7zip.git ../7zip-upstream
```

Keep upstream updates separate from UI commits. Record the upstream version or commit in a release note when packaging.
