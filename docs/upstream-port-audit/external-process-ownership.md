# External-process registration and allocation failures — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, reviewed commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. The existing PanelItemOpen port
and CSnapshotProcess provenance also retain their initial baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Reviewed complete source snapshot/child-process/wait/launch owners in
`CPP/7zip/UI/FileManager/PanelItemOpen.cpp`, native StartApplication branches
in BrowseDialog2, and AutoPtr, Handle and ProcessUtils dependencies. The
corresponding QtNativeProcessServices, QtAssociatedFileServices, archive editing
caller, worker entry and temporary-copy leases were traced through cleanup.

The Windows adapter had replaced the source snapshot's scoped owner with a raw
Toolhelp handle across allocating operations. OpenProcess also acquired its OS
handle before allocating the interface owner. Both now reuse the original
CHandle/CProcess RAII types. The snapshot enumeration and process identity
algorithm remain unchanged. A separate launch boundary allocates the interface
owner before native CreateProcess or the original association launcher runs;
a valid native process is transferred even if the launcher reports an error.
The source native launch implementations remain authoritative.

POSIX creation previously read allocating metadata after posix_spawn before
owning the child. An allocation failure left a child without a reaper. The
adapter prepares its path before launch and records child ownership immediately
after success. Metadata allocation failure returns E_OUTOFMEMORY with the valid
process owner, allowing the caller to retain the externally used file. Reaper
resources are reserved before spawn; close only hands the PID to that retained
state, without allocating a thread or waiting for the editor to exit. POSIX
process identity, wait and orphan-reaping services have no usable Win32 source
implementation; their existing narrow adapter remains in place.

The source child tracker now retains each received handle with CMyUniquePtr
through registration, and reserves both parallel arrays before publishing a
pair. Allocating folder/path metadata is prepared before launching an editor.
A scoped output guard also covers exceptions inside launch/error reporting;
transfer clears the output before handing ownership to the child tracker.
The queued GUI bridge now returns exceptions from its callback or readiness
predicate to the waiting worker and completes the semaphore handoff even on
failure. The native worker boundary catches exceptions, unwinds its owners and
preserves an unproven externally protected copy rather than allowing an exception to
escape the OS thread. Child discovery, wait policy, update prompts, archive
publication and normal cancellation retain their source algorithms and branches.
No source workflow or dependency is omitted.

Regressions observe real Windows handle counts across failed snapshot/open
allocations, and POSIX process ownership/reaping after a failure immediately
following a real spawn. A pipe-controlled child also verifies that releasing its
owner remains safe with allocation failure armed and returns before child exit.
The POSIX adoption leak was reproduced on macOS before the fix; reaper creation
inside a noexcept destructor was established from source without deliberately
terminating a parent and leaving its private child behind.
Public archive-editing fixtures also fail a name query, a worker snapshot, or a
launch after returning a handle; they verify released handles, a retained
protected copy and unchanged archive bytes. These checks do not expose private
trackers, read source text or pin allocation counts. A separate real cross-thread
GUI-bridge suite verifies callback/predicate exceptions and cancellation before
readiness; exceptions must not escape the GUI loop or strand the worker.

Focused application builds and process/GUI-worker regressions passed on macOS,
Linux and Windows. Windows also passed all four native association scenarios;
the POSIX-only reaper refinement was rebuilt and rechecked on macOS (3/3,
3.55 seconds) and Linux (3/3, 3.49 seconds; runner
`20261018T110904Z-b65fddac`). The earlier Windows check
`20261018T105902Z-fdcdaf37` passed its eight selected scenarios; the later
attempt did not start on Windows because another run owned the worktree.
No Windows production branch changed between those checks.

This is focused ownership evidence, not full-suite or UI parity acceptance.
The earlier broad run still has separate translation/UI failures and a Windows
native-shell shutdown crash after its functional assertions pass. Those remain
outside this process-registration fix. Slow suites were not run.
