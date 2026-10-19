# Sevenzip

A cross-platform port of the Windows 7-Zip File Manager for Windows, macOS and
Linux. The interface uses Qt 6 Widgets, and archive operations use the official
7-Zip engine. This is an independent project and is not an official 7-Zip release.

This is an AI-driven project: AI coding agents play a central role in development,
guided by human direction, review and testing.

[Download](https://github.com/Feverqwe/7zip-qt-port/releases) ·
[Report a bug](https://github.com/Feverqwe/7zip-qt-port/issues) ·
[Contributing](CONTRIBUTING.md)

## Installation

Release archives are built for Windows x64, Linux x64 and macOS arm64.
Extract the entire archive, keeping its directories and libraries together.
On macOS, open `Sevenzip.app`; on Windows, run `bin/7zFM.exe`; on Linux,
run `bin/7zFM`. macOS 14.4 or later is required.

Packages do not have Windows Authenticode signatures or Apple Developer ID
signing and notarization. macOS packages use an ad-hoc signature. Checksums
are published alongside the archives in `SHA256SUMS.txt`.

## Project status

The application supports dual panels, filesystem and archive navigation, nested
archives, history, favorites, list views, sorting and selection. Features include
file copy, move, create, rename and delete operations, split/combine, properties,
external viewers, editors and diff tools, checksums, benchmarks and settings.

File → Open device adds experimental read-only browsing and copying from external
ext2/ext3/ext4 devices without a filesystem driver or full disk image.
See [direct device access](docs/direct-device-reading.md) for usage and limits.

Archive creation and updates support compression settings, encryption, volumes
and self-extracting archives. Available formats and operations depend on the
selected engine. Extraction supports password and overwrite prompts; an error
or cancellation may leave files that have already been extracted. Updating a
nested archive requires write support throughout its containing archive chain.
Platform metadata, links, alternate data streams and file associations have
additional limitations.

The project is in an early release stage. See the
[acceptance scope](docs/ux-port-audit.md) for validation boundaries and the
[7zFM comparison](docs/7zfm-parity.md) for supported workflows and limitations.

## Building from source

Clone the project and the official upstream sources:

```sh
git clone https://github.com/Feverqwe/7zip-qt-port.git
cd 7zip-qt-port
git clone https://github.com/ip7z/7zip.git ../7zip-upstream
```

You need C and C++17 compilers, CMake, Ninja, Qt 6 Widgets, `clang-format`
and the official `7zz`. macOS builds use the official Qt SDK and target macOS
14.4 or later. Follow the [dependency setup](docs/development.md#dependencies)
for your platform.

From the repository root:

```sh
./scripts/doctor.sh
./scripts/check.sh
```

`check.sh` checks C++ formatting, builds the application and runs the default
test suite. On macOS, open `build/dev/Sevenzip.app`, or launch it from a terminal:

```sh
build/dev/Sevenzip.app/Contents/MacOS/Sevenzip /path/to/archive.7z
```

To build a Release package without test executables:

```sh
./scripts/build-app.sh
```

On macOS, the resulting application is `dist/Sevenzip.app`.
Intermediate build files remain in `build/`.

Put `7zz` on `PATH`; standard Homebrew locations are also checked on macOS.
See the [development guide](docs/development.md) for platform dependencies,
codec and self-extracting runtime preparation, engine selection and focused
build commands. Local builds and tests do not require access to the maintainer's
infrastructure.

## Documentation

The [documentation map](docs/README.md) links development instructions, source
provenance, architecture, UI specifications and testing. Repository conventions
are in [AGENTS.md](AGENTS.md), with task-specific workflows in `.agents/skills/`.

Local agent memory is optional. Run
`sh .agents/skills/sz-setup/scripts/setup.sh` to initialize the Git-ignored
`.codex/` directory with documentation links. Running it again preserves existing
notes. See the [memory protocol](docs/agent-memory.md) for reading and handoffs.

## Licenses

This repository contains code under several licenses. [MIT](LICENSE) applies
to original project code unless another license is specified. Ported 7-Zip code
retains its own licenses; the exFAT module and tools adapted from htopQt use
GPL-2.0-or-later. Qt and other dependencies retain their own terms.
See [third-party notices](THIRD_PARTY_NOTICES.md).
