# Build dependency overlays

The Qt Remote Objects overlay retains the complete pinned vcpkg recipe and MIT
license in [its baseline](../../ports/qtremoteobjects/vcpkg.baseline.json).
It removes the unconditional QtDeclarative dependency; the existing `qml` feature
still requests QtDeclarative and enables Qt Quick through the unchanged recipe.
The port revision is advanced. Qt source code, tools and optional features remain
unchanged. The Widgets application uses the C++ RemoteObjects interface, whose
upstream target links Core and Network; the QML adapter is a separate target
selected when Qt Quick is available. This avoids building QML libraries and tools
for a consumer that does not request them.
The application manifest requests Qt SVG directly to retain SVG icon rendering
after removing the former transitive QML dependency closure.

Verify and review this recipe with the same reference tooling:

```sh
python3 scripts/upstream_baseline.py reference-verify \
  --manifest ports/qtremoteobjects/vcpkg.baseline.json --upstream build/vcpkg
```

For updates, use `reference-review` with this manifest and a proposed vcpkg
revision, then validate the package and its application consumers. The original
complete recipe hashes remain independent of the local adaptation.

The Qt Tools overlay retains the complete Microsoft vcpkg recipe from the
registry revision recorded in [its baseline](../../ports/qttools/vcpkg.baseline.json).
The original recipe files and MIT license have complete SHA256 inventory entries
and local owners. Optional features and their inherited patches remain available;
the local recipe adds the header patch and advances its port revision.

[The Qt baseline](../../ports/qttools/qttools.baseline.json) records the retrievable
Qt Tools repository, full source commit behind v6.11.2, original complete header
hash and licenses. `releasehelper.h` declares `Projects` as `std::vector<Project>`
without including `<vector>`. LLVM 23 exposed this when compiling lrelease;
the adjacent project-description header already includes that standard header.
The local patch supplies the missing direct include. It transfers no translation
algorithm, and leaves callers, types and control flow unchanged.

The recipe remains MIT-licensed. The patched Qt header retains its commercial
or GPL-3.0-only with Qt-GPL-exception-1.0 notice; the applicable texts accompany
the overlay. Qt source archives and dependency versions remain controlled by
vcpkg. This host-tool adaptation adds no Qt Tools implementation to the native
7-Zip engine.

Verify original inventory units with the existing reference tooling, using Git
checkouts containing the commits named in the manifests:

```sh
python3 scripts/upstream_baseline.py reference-verify \
  --manifest ports/qttools/vcpkg.baseline.json --upstream build/vcpkg
python3 scripts/upstream_baseline.py reference-verify \
  --manifest ports/qttools/qttools.baseline.json --upstream build/qttools-upstream
```

For an update, use `reference-review` from the [update procedure](../upstream-update.md)
with each manifest and its proposed repository commit. The Qt review applies the
actual overlay patch to the original and proposed headers in a temporary directory,
reports incompatible context and shows the local adaptation. No second edit
description or adapted-source hash needs to follow a safe refactoring.
Validate the resulting package build and application before advancing either
baseline; retain local fixes and review the inherited recipe features/patches.

Initial validation on 2026-10-27 used macOS arm64 and Homebrew LLVM 23: the
patched Qt Tools package built in Debug and Release, and Sevenzip Desktop built
with static Qt from the manifest. Existing protocol, EROFS reader/module and
language checks passed under ASan/UBSan, followed by the application's isolated
smoke launch. The ordinary shared-Qt preset also configured and passed its
focused EROFS check. Windows/Linux manifest acceptance remains unverified;
the Hyper-V host became unreachable during their dependency builds.
