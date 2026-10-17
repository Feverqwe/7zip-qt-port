# Reviewing an upstream 7-Zip update

The complete current baseline is [upstream-baseline.json](upstream-baseline.json).
It records the official repository, full default commit and every tracked
borrowed path. `exact` files under `third_party/7zip/` must match the original
bytes; `adapted` files under `src/` retain their original relative path but need
code review; `local` headers have no upstream counterpart at their current
paths. `overrides` records full commits for individual modules if an update is
applied incrementally. The function/branch scope and reasons for local changes
remain in [the port audit](upstream-port-audit.md).

Start from a disposable Git snapshot of the official repository. The old
`../7zip-upstream` checkout is not required, and it must not be modified:

```sh
git clone --no-checkout https://github.com/ip7z/7zip.git /tmp/7zip-update-review
python3 scripts/upstream_baseline.py verify --upstream /tmp/7zip-update-review
```

`verify` rejects missing/stale manifest entries, duplicate source owners,
unknown revisions, new upstream counterparts for local-only paths, and changed
bytes in exact files. It checks the complete tracked inventory.

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
source change. Keep Qt/platform/`7zz` adapters and local fixes; do not replace
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

Early verification and synthetic-review evidence is preserved in
[the tooling archive](archive/qt-port/workflow-evidence.md).
