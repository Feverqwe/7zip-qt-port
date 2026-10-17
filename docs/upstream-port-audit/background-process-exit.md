# Background priority after child exit — 2026-10-18

Source reference: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed FileManager `ProgressDialog2.cpp::OnPriorityButton`, `Panel::TestArchives`,
Common `CompressCall2.cpp::ExtractGroupCommand` / `TestArchives` and GUI
`ExtractGUI`, together with the Qt archive opener, progress transport and backend.
Upstream changes the current application's priority. The port additionally
propagates that policy to its official-engine child processes through native
platform services; upstream has no QProcess lifecycle implementation to reuse.

A short child can exit before Qt reports its completion. The backend now treats
POSIX ESRCH during that child's priority change as an already completed policy
request. Its normal completion still decides whether the archive operation
succeeded, failed or was cancelled. Other priority errors remain errors. This
changes only the process adapter: no upstream bodies, algorithms or dependencies
are replaced or copied, and no upstream command branches are omitted.

Validation on macOS arm64 / Qt 6.12, working tree based on
`3e6cade1403a10a98cd4810b8036f3bf44f6322b`: the new selected-ZIP regression
reproduced `errno=3 : No such process` after enabling application background
priority between checks. With the fix, three consecutive checks passed in a
native Cocoa run. Incremental app and focused test builds passed; the six suites
`toolbar_archive_workflow`, `archive_progress`, `archive_backend`,
`process_priority`, `progress_priority` and `progress_priority_failure` passed.
The initial fix was checked only on macOS; the follow-up below adds other OS evidence.

Follow-up process-control review (2026-10-18) compared upstream pause/stop and
priority handlers with the port's process startup, completion, cancellation,
stage transitions and native-worker completion guards. The same exit window
also affected pause: a deterministic OS-exit-before-Qt-notification fixture
reproduced ESRCH on macOS and ERROR_NOT_FOUND on Windows. Linux passed that
fixture before the follow-up fix. Cancellation and stage-transition source
review did not establish another defect within this scope.

The backend now shares exit-result handling between pause and priority. POSIX
retains the ESRCH treatment; Windows checks whether the owned child has exited
using the existing upstream `NWindows::CProcess` handle wrapper and a zero-time
native wait. Failure to open an already absent PID is recognized only for
ERROR_INVALID_PARAMETER. Errors on a live or unconfirmed process remain errors.
The platform adapters remain necessary because upstream uses cooperative
in-process pause and current-process priority. No upstream algorithm or command
branch is replaced. The regression covers both successful and failed child
exits, ensuring control requests cannot turn an engine failure into success.

The follow-up also exposed a runner synchronization defect: an edit could predate
an object compiled from the preceding immutable snapshot. Preserving that edit's
source timestamp caused Ninja to reuse the old object despite receiving changed
contents. The source transfer now gives changed files the receiver's current
mtime and continues skipping unchanged contents. An offline rsync/Ninja
regression reproduced stale output before the fix and checks both rebuilt output
and an unchanged second build afterward. Existing build trees and compiler caches
are retained; no clean build or cache invalidation is introduced.

Final follow-up validation, working tree based on `34ea665`: all nine focused
suites passed on macOS arm64 / Qt 6.12, Ubuntu x64 and Windows x64. Scope:
`archive_progress`, `process_pause`, `process_priority`, `progress_priority`,
`progress_cancel`, `archive_backend`, `source_update_backend`,
`toolbar_archive_workflow` and `hyperv_runner`. Remote evidence is retained under
`build/runners/hyperv/20261017T212106Z-4a53c6c4`; both guests reused `current` and
their existing compiler caches. The macOS Release application was rebuilt and
installed into `dist/Sevenzip.app`. This is focused behavior/build validation,
not full-suite or rendered cross-platform UI acceptance.
