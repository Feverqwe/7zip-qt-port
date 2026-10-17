# Local runtime artifacts and CI scope

The GitHub workflow was removed in `96daee8de9f82374a4548e6dff2bd47a6fe5a891`.
On 2026-10-10 the user explicitly chose to preserve that deletion. There is no
active GitHub build, artifact upload or automatic release configuration in this
checkout. Local validation uses [focused checks and a full gate when needed](development.md#local-commands),
and Windows/Linux acceptance uses [Hyper-V runners](hyperv-runners.md).
Release runtime packaging uses `BUILD_TESTING=OFF` and the matching Qt kit;
tests and native acceptance are separate checks.
Local macOS builds default to the official shared Qt 6.12.0 kit and target
macOS 14.4+. This avoids the external ICU/GLib dependency chain of Homebrew Qt;
see [SDK setup](development.md#dependencies).

Runtime/SFX builders compile private snapshots from the official repository and
revision recorded in [the baseline](upstream-baseline.json).
`scripts/install-qt-windows.py` adapts aqtinstall to the selected Qt kit's
compiler-specific repository layout. Pins from the removed workflow are
[historical evidence](archive/qt-port/workflow-evidence.md).

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
The resulting `build/packages/sevenzip-desktop-*.zip` and `*.tar.gz` stay local.
Packaging does not upload artifacts or publish a release.

Matching Qt source archives are checksum-verified private inputs for extracting
license texts only; they are never copied into runtime artifacts. The ICU notice
is included when ICU is deployed. The Qt/ICU pins match htopQt's corresponding
license inputs. macOS binaries are thinned to the native architecture and stripped,
then signed inside out using an ad-hoc signature, including the bundled 7-Zip
engine/module/SFX. SFX checksum metadata is refreshed after these transformations.
Frameworks and the app are signed last; `codesign --verify --deep --strict` must
pass before ZIP creation. Local `build-app.sh` installations use the same runtime
compaction and signing. This follows htopQt's signing flow. Developer ID,
notarization and Windows Authenticode are not configured.

`./scripts/release.sh patch` (also `minor`, `major` or an explicit larger version)
transfers htopQt's release helper: require a clean tree, change the CMake version,
commit it, create an annotated `vX.Y.Z` tag, and atomically push branch and tag to
`origin`. `--remote` selects another remote; `--dry-run` checks without changing
files, commits or tags. `--check-tag vX.Y.Z` checks the version locally.
Running this helper publishes Git refs; importing it does not run a release.

There is no workflow file for actionlint or a GitHub run to validate. Native,
rendered and clean-environment package acceptance remain separate from local
compilation and relocation checks.
