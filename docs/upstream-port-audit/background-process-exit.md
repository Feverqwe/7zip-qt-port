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
Windows/Linux runtime validation is outside this change's checked scope.
