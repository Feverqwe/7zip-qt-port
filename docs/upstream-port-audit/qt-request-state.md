# State of a new archive request — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed FileManager `ProgressDialog2.h/.cpp` (`CProgressThreadVirt` result
initialization/Process and `CProgressSync` stop state), `ExtractCallback.cpp`
initialization and overwrite decisions, together with persistent ArchiveBackend
request entry points, process startup/completion, cancellation, early validation,
overwrite preparation and the HRESULT consumer in RunArchiveBackendRequest.
Upstream operation owners provide a fresh result/stop state for each request;
the Qt transport reuses its backend across requests. There is no upstream
persistent-QProcess request initializer to copy.

Seven entry points could refuse a request before process startup without
replacing the preceding request's HRESULT. Fourteen regressions reproduced
S_OK after a previous success or E_ABORT after a previous cancellation for
Create, Extract, Test, Add, Delete, Rename and Replace. They now initialize
their result before validation. The mutation entry points reject a busy
backend before changing request state or restoring listing metadata.
Empty Delete remains a no-op.

Extraction had a related retained-state error: overwrite preparation precedes
process startup, which was the point that cleared cancellation. After a
cancelled Test, accepting Yes in the next Extract overwrite prompt returned
"Operation canceled" and left the old file. A real ZIP regression reproduced
this. A newly accepted extraction now clears the prior cancellation before
overwrite preparation; cancellation of the current prompt is still honored.

Existing source-extract/source-update/hash request owners already initialize
their results. They were reviewed with listing, multi-stage completion and
failure/publication paths to keep the correction at the Qt request boundary.
No archive validation, mutation, overwrite algorithm, upstream body, dependency
or command branch is copied, replaced or omitted by this change.

The pre-fix failures were reproduced on macOS arm64 / Qt 6.12, working tree
based on `41a6c47`. Both regressions exercise operation signals/results and
unchanged or correctly replaced local fixture bytes. The existing overwrite
suite also retains Cancel/Yes-to-all/No-to-all and encrypted queue checks.

Incremental application and test builds passed. A final set of 19 focused
suites passed on macOS and Linux x64 / Qt 6.12. Run
`20261017T214943Z-5b5325cd` passed all changed backend suites on interactive
Windows x64 as well: `archive_backend`, `archive_mutation`, `extraction_options`,
`archive_creation_lifecycle`, `source_update_backend` and `archive_progress`.
The broader Windows run passed 18/19; its sole failure was an unrelated Options
resize expectation, being investigated separately. Linux/Windows used the
persistent `current` worktree and compiler caches; Windows symlink fixtures
used the existing elevated task mode. No full/slow gate was run.
