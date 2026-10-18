# Windows process-pause rollback ownership — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed `CPP/Windows/Thread.h`, `Handle.h`, the FileManager
`ProgressDialog2.cpp` pause handlers and `CProgressSync::CheckStop`, together
with the Qt process-pause service, its backend start/exit/cancel callers and
progress transport. Borrowed source baselines remain unchanged.

Upstream pauses in-process callbacks cooperatively. The existing Qt adapter
also controls an owned external child; there is no upstream external-process
suspend/enumeration body to copy. Its native CThread Suspend/Resume primitives
and three-pass enumeration are retained.

The adapter previously opened a thread handle before allocating its CThread
owner, then suspended the thread before storing its rollback entry in a vector.
Allocation failure could leak the handle or lose ownership of the suspend
increment. The raw snapshot handle also escaped on exception, and SetPaused
bypassed its HRESULT/Resume error path.

The unchanged source CHandle now owns the snapshot. CThread is allocated before
OpenThread, and the rollback entry is registered before Suspend. Allocation
failure returns E_OUTOFMEMORY through the existing Resume rollback, which retains
failed resume entries for retry. Opened threads are also checked against the
owned child, because a snapshot thread ID can be recycled before OpenThread
([Microsoft's ownership explanation](https://devblogs.microsoft.com/oldnewthing/20140822-00/?p=173)).
No source algorithm or progress-command branch is replaced or omitted; the
POSIX process-control path is unchanged.

A Windows-only regression injects allocation failures around the public
SetPaused call, progressing until an uninjected call succeeds. Its disposable
child publishes independent ticks from two threads. Every attempt checks the
HRESULT, fresh ticks after resume/destruction and the parent's handle count.
Before the fix it reproduced escaped exceptions, leaked handles and two cases
with a thread left suspended. The allocator replacement is confined to this
test executable and armed only during the pause call; product internals and
allocation counts are not exposed or pinned.

The application and focused progress, cancellation, preparation, creation,
volume, backend and runner suites passed on macOS arm64 and Linux x64 (9 suites)
and Windows x64 (10, including the allocation regression), using Qt 6.12.
Guest run `20261018T091050Z-129ef57f` reused existing worktrees/builds/caches and
the elevated Windows desktop. An earlier guest run exposed omitted translation
catalog inputs in the runner snapshot; that transport issue was corrected and
the localized consumers passed in the final run. No full gate, slow matrix or
rendered UI comparison was run for this nonvisual change.
