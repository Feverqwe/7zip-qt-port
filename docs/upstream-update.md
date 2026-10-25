# Reviewing an upstream 7-Zip update

The complete current baseline is [upstream-baseline.json](upstream-baseline.json).
It records the official repository, full default commit and every
borrowed path. `exact` files under `third_party/7zip/` must match the original
bytes; `adapted` files under `src/` retain their original relative path but need
code review; `local` units have no upstream counterpart at their current
paths. `overrides` records full commits for individual modules if an update is
applied incrementally. The function/branch scope and reasons for local changes
remain in [the port audit](upstream-port-audit.md).
`original_sha256` pins complete original inventory units for new selective imports;
`verify` checks these hashes against each owner's recorded revision. Exact-file
revision overrides also enter the native runtime snapshot and its cache/provenance,
so a selective codec update reaches both static consumers and the standalone engine.

Archive readers with Linux dependencies have one implementation under
`src/gpl/CPP/`, preserving the upstream path below that source root. Both the
application and console load these format modules. The [reader ownership
audit](upstream-port-audit/license-variants.md) records the shared portable support
units. `extracted` maps transferred symbols and local units to their original
handler owner; `original_sha256` pins that complete original inventory unit.
Review these units together with the full reader when updating upstream.

`runtime_adapted` selects adapted engine modules for both dynamic and standalone
runtime builds. Review these local changes during an upstream update as well;
their compiled provenance and prepared-runtime cache include the source hashes.
The builders refresh one owned source snapshot while retaining incremental
compiler outputs. They do not compile in the upstream checkout. GPL reader
sources are excluded from this overlay and built as separate CMake modules;
the snapshot preparer rejects a full GPL reader in `runtime_adapted`.
`runtime_dependencies` adds non-7-Zip C modules and headers needed by these
modules, removing the owning `src/` or `third_party/` prefix in the snapshot.
Colliding destination paths are rejected. Their source revisions and licenses
belong in the existing port audit. Runtime fingerprints and transitive dependencies
include them; generated native rules compile C sources as C while preserving
incremental build outputs. `runtime_format_sources` selects dependencies linked
only into Format7zF, such as probes and HFS compression shared with APFS. The
console executable and original SFX do not link these format support objects.

Start from a disposable Git snapshot of the official repository. The old
`../7zip-upstream` checkout is not required, and it must not be modified:

```sh
git clone --no-checkout https://github.com/ip7z/7zip.git /tmp/7zip-update-review
python3 scripts/upstream_baseline.py verify --upstream /tmp/7zip-update-review
```

`verify` rejects missing/stale manifest entries, duplicate source owners across roots,
unknown revisions, new upstream counterparts for local-only paths, and changed
bytes in exact files. It checks the current working-tree inventory, including
nonignored new sources and excluding deleted files, so source moves can be
reviewed before staging.

Before changing a module, produce an old-to-proposed review:

```sh
python3 scripts/upstream_baseline.py review \
  --upstream /tmp/7zip-update-review --new-revision <full-commit> \
  --output /tmp/7zip-update-report.md
git -C /tmp/7zip-update-review diff <old-commit> <full-commit> -- CPP/7zip/UI/FileManager/NetFolder.cpp
git -C /tmp/7zip-update-review checkout <full-commit>
git diff --no-index /tmp/7zip-update-review/CPP/7zip/UI/FileManager/NetFolder.cpp src/CPP/7zip/UI/FileManager/NetFolder.cpp
```

The report lists changed recorded files and marks exact versus adapted owners.
For each changed adapted module, inspect the upstream diff, current local
deviations, callers and changed dependencies before transferring the relevant
source change. Keep Qt/platform/native-module adapters and local fixes; do not replace
adapted files wholesale. For unchanged vendors, copy exact new bytes only after
reviewing dependent code. Check omitted functions and branches in the audit as
well: a change there can still affect a copied dependency. Update the audit
with old/new full revisions, source paths, retained adaptations, deferred
changes and reason. Change the manifest default revision only if every recorded
module moves together; otherwise add per-path `overrides`. Update parity docs
when the workflow changes, add meaningful behavior regressions and run affected
checks. Use `./scripts/check.sh` only for the broader integration risks defined
in [AGENTS.md](../AGENTS.md#validation-scope). A failed check leaves provenance at the
last validated revision.

Release resources have separate provenance in `resources/7zip/provenance.json`
and `help-provenance.json`. Compare the recorded members with the matching
official release installer as well: translations and CHM help are not covered
by the source manifest. Preserve exact member bytes and update their provenance
after review, including when a member is unchanged in the new release.

Early verification and synthetic-review evidence is preserved in
[the tooling archive](archive/qt-port/workflow-evidence.md).

## External reference baselines

ZIP's libzstd encoder and its 7-Zip-zstd adapter use the reference
verification/review commands with explicit source-to-owner maps. Their pinned
revisions, imported scope and example commands are in the
[ZIP encoder audit](upstream-port-audit/zip-zstd.md).
The same reference commands accept the existing Linux baseline manifests;
explicit adaptation maps additionally enable local-difference reports.
