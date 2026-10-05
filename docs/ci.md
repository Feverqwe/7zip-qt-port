# CI builds and runtime artifacts

`.github/workflows/check.yml` builds Release applications on hosted Ubuntu 24.04
x64, Windows 2022 x64/MSVC and macOS 14 arm64. Qt is pinned to 6.12.0, following
htopQt's build workflow. Pushes, pull requests and manual dispatches build with
`BUILD_TESTING=OFF`; CI does not run formatting, CTest or native acceptance.
Local validation still uses [the full gate](development.md#local-commands), and
interactive Windows/Linux acceptance uses [Hyper-V runners](hyperv-runners.md).
No self-hosted guest or administrator desktop is needed for CI compilation.

The official 7-Zip checkout stays pinned to
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
<https://github.com/ip7z/7zip.git>. Existing runtime/SFX builders compile private
snapshots. Both checkout actions disable credential persistence.
Windows Qt installation retains htopQt's aqtinstall 3.3.0 adapter for Qt 6.12's
compiler-specific repository layout. Python 3.12 and Ninja 1.12.1 are selected
for that installer; hosted OS/compiler packages can change with runner images.

`scripts/package.py` archives a fresh CMake installation, never the checkout or
build tree. Artifacts contain the application, retained entry aliases, official
engine/module/SFX, Qt runtime dependencies and notices. Headers, static/import
libraries, debug symbols, test plugins, source archives and build metadata are
excluded. macOS contains a single `.app` in ZIP; Linux has a runtime directory
in tar.gz; Windows has a runtime directory in ZIP. Only ICO/SVG image plugins
are retained for embedded resources/theme icons. macOS ZIPs omit resource-fork
metadata alongside the application. Qt deploy tools resolve the
linked libraries and plugins. System libraries remain platform dependencies.
Windows packages copy the imported VC CRT DLL dependency closure from the
installed redistributable, with its notices, instead of shipping a CRT installer.
Only the resulting `build/packages/sevenzip-desktop-*.zip` and `*.tar.gz` are
uploaded, with seven-day retention. Source inputs, caches and test logs are not
uploaded. There is no GitHub Release publication job or write permission.

Matching Qt source archives are checksum-verified private inputs for extracting
license texts only; they are never copied into runtime artifacts. The ICU notice
is included when ICU is deployed. The Qt/ICU pins match htopQt's corresponding
license inputs. macOS binaries are thinned to the native architecture and stripped,
then signed inside out using an ad-hoc signature, including the bundled 7-Zip
engine/module/SFX. SFX checksum metadata is refreshed after these transformations.
Frameworks and the app are signed last; `codesign --verify --deep --strict` must
pass before ZIP creation. This follows htopQt's signing flow. Developer ID,
notarization and Windows Authenticode are not configured.

`./scripts/release.sh patch` (also `minor`, `major` or an explicit larger version)
transfers htopQt's release helper: require a clean tree, change the CMake version,
commit it, create an annotated `vX.Y.Z` tag, and atomically push branch and tag to
`origin`. `--remote` selects another remote; `--dry-run` checks without changing
files, commits or tags. `--check-tag vX.Y.Z` checks the version locally and in CI.
Running this helper publishes Git refs; importing it does not run a release.
CI builds downloadable artifacts after the push and does not publish a Release.

`actionlint .github/workflows/check.yml` checks workflow syntax and shell code.
A local build or static check does not establish a passing GitHub run or validate
other platforms. Native/rendered acceptance remains separate from compilation.
