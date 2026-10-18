# Volume-writer allocation ownership and record state — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed complete `CPP/7zip/Common/MultiOutStream.h`, its initialization,
creation/reopen/close/destruction and write paths in `MultiOutStream.cpp`, and
the native `UI/Common/Update.cpp` and QtArchiveVolumeOutput consumers. Stream,
COM and native file owners retain their existing source implementations.

Both production consumers allocate the writer and assign its Prefix before
calling Init. Prefix assignment can throw on allocation failure. The destructor
then reaches Destruct with an empty Streams vector but an uninitialized
NumListItems value. Reused allocation storage reproduced a failed cleanup after
an injected name-allocation failure.

CreateNewStream also created its file before allocating the owning Streams
record. An exception during registration left an untracked temporary file.
The same function updated RealSize on a local record after copying it into
Streams. The retained record could then skip a required truncation: preallocating
several volumes and shrinking to zero left bytes in the final first volume.

The constructor now calls the existing source InitLinkedList method. Registration
precedes file creation; a failed or throwing create removes the unlinked record
without deleting any unowned collision path. After successful creation the
original linked-list publication runs, and SetSize2 updates the retained record.
Init retains its initialization and size checks; no source operation or branch
is omitted. The adapted header moved from third_party into
`src/CPP/7zip/Common`, alongside the adapted implementation, with one
authoritative copy recorded in the baseline. Its FileStreams dependency uses
the existing include root after that move. Borrowed formatting is retained.

The standalone regression checks three observable failures: cleanup after a
failed Prefix assignment, files left after allocation failures during a
two-volume write, and stale data after preallocation followed by truncation.
The write case also preserves an occupied temporary name and its bytes. Reused
storage exposes the partial-construction defect. Tests use public stream methods,
without inspecting private fields or pinning allocation counts. All three failures
were reproduced before their fixes on macOS; the final focused suite passes there.

Related analyzer warnings were also reviewed: CVolStream's COM member releases
the local file owner on early CreateNewStream returns, and Init rejects a zero
final volume size before Write can divide by it. These warnings did not require
changes to the streaming algorithm.

The final application build and eight writer/volume/update/SFX/lifecycle suites
passed on macOS arm64, Linux x64 and Windows x64 / Qt 6.12 (guest run
`20261018T095007Z-7f90abf1`, identical guest source digest
`0e4a0ee3a67602ef26ab556fe001a0897eb9bb583c2bf929906f724f6a14cb18`).
On macOS, SFX discovery initially failed; the focused rerun with the project's
official runtime on PATH passed. The additional metadata-storage suite passed
on macOS/Linux and failed Options geometry on Windows before archive writes,
as in the earlier constructor-only snapshot; that UI scenario is not accepted.
The upstream baseline verifier passed after the header move. No full gate,
opt-in slow matrix or rendered UI comparison has been run for this focused change.
