# Native-worker notification ownership — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed original `CPP/7zip/UI/GUI/HashGUI.cpp` completion and result formatting,
HashCalc callbacks, `CPP/7zip/UI/FileManager/ProgressDialog2.cpp` timer, posted
completion, final-message snapshot and thread join; the existing copy and volume
adapters retain the source FSFolder copy, CopyCoder and MultiOutStream owners.
The Qt side review covers the complete native hash, copy preparation, creation
and volume phase transitions, progress transport, cancellation, backend
destruction and direct consumers. Existing source provenance and dependencies
remain in the corresponding module audits.

The source modal progress dialog retains its thread and result owners through
completion. Qt signals additionally permit synchronous consumers to remove the
backend, cancel/join a phase or start a subsequent operation. There is no usable
Win32 implementation of that QObject lifetime boundary. This change is confined
to that adapter: no hash, copy, compression, volume, publication or cancellation
algorithm from the engine is replaced or omitted.

Confirmed failures and adaptations:

- Hash publication continued after a consumer deleted the backend. Retain a
  QPointer across each output delivery; the detached completed worker still
  owns its result and output data until the delivery returns.
- Hash completion read the shared last result after a reentrant operation
  changed it, dropping the completed operation's summary. Capture the completed
  HRESULT before publication; subsequent calls retain their own backend state.
- Copy progress passed a member QString by reference. A first receiver could
  synchronously cancel/join and clear that member, making a second receiver see
  an empty caption. Retain a local QString for both initial and timer delivery.
- Volume startup inspected members after its initial progress receiver deleted
  the backend. Check retained QObject ownership before inspecting the existing
  worker/generation guards or starting the worker.
- Joined native cancellation continued inspecting backend members after its
  terminal signal deleted the owner, or joined a new hash started from a copy
  completion. Return immediately after finishing the selected native phase.
  Native phases have no live CLI child to reset: copy/hash/creation preparation
  precede it, while volume/deletion follow process-exit cleanup. Retained
  creation metadata is not a concurrently active worker. The shared cancellation
  boundary also checks ownership after dismissing an overwrite prompt and after
  waiting for the process transport.

Regressions use public backend calls/signals, private local inputs and operation
results. Baselines: hash-owner deletion SIGSEGV; missing hash summary and changed
copy caption both failed assertions; copy-to-hash restart synchronously produced
two completions instead of one. Partial macOS ASan confirmed heap-use-after-free
in volume startup and in joined cancellation. It instruments the backend core,
hash/volume/preparation adapters and worker-progress test; Qt, official codecs
and other dependencies remain uninstrumented. Diagnostic optimization inlines
template accessors so uninstrumented debug weak-ODR accessors do not hide loads.
The normal build flags, persistent build outputs and compiler cache are retained.

The application built on macOS, Linux and Windows. Seven focused suites passed
on macOS (6.87 seconds) and Linux (8.42 seconds). Windows passed six suites in
the initial run; archive_creation_lifecycle's symbolic-link fixtures required
the elevated desktop and passed on their focused rerun. Tests and assertions
were retained. Runner evidence: `20261018T113232Z-9c89461f` and
`20261018T113421Z-8165d044`. Final partial macOS ASan worker_progress run:
11 passed, 0 failed, 799 ms.

Process-output, listing and overwrite callbacks have their separate
[ownership audit](process-notifications.md). No UI layout/text change,
slow-suite acceptance or whole-project gate is claimed here.

## Destruction and pipeline continuations — 2026-10-18

The backend destructor reused joined cancellation, which published finished
while the backend's owners were being destroyed. A connected pipeline could
start another operation at that point. Both copy-preparation and hash baselines
actually started a separate next backend during destruction (two failed rows,
3 ms). Starting work on the dying backend itself could also leave a live worker
for member destruction; the regression uses a separate owner to demonstrate the
unwanted continuation safely.

The original CApp::ReleaseApp stops all panel processing before releasing
owners. MainWindow::quiesceOperations already disconnects presentation consumers
before joining work. The backend now blocks its workflow signals at destructor
entry before any join. Explicit cancelAndWait keeps its result notifications
and supported continuation behavior; the existing reentrant cancel/restart
cases remain enabled. Native worker algorithms and resource cleanup are intact.

Application builds and six focused suites passed on macOS (3.24 s), Linux
(7.36 s) and Windows (16.23 s); runner `20261018T121239Z-278677db`.
Both new destruction rows and the existing explicit-cancel/restart, publication,
process-notification and closed-window cases are included. Build trees and
compiler caches were reused; no new sanitizer or full-gate acceptance is claimed.
