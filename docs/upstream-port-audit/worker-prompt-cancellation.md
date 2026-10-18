# Cancellation while a filesystem worker awaits overwrite — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed FileManager `ExtractCallback.cpp::AskOverwrite/AskWrite`,
`FSFolderCopy.cpp` callback dispatch, `ProgressDialog2.cpp::CheckStop`, and their
Qt filesystem transaction, prompt, progress and window-shutdown consumers.
Existing borrowed-module baselines remain unchanged.

Upstream opens its overwrite dialog synchronously on the operation's callback
path. The Qt filesystem worker instead sends a prompt to the GUI and waits on
a condition variable. Window shutdown cancels and joins that worker before
releasing its progress snapshot. This scheduling boundary has no reusable
upstream QWaitCondition implementation.

Cancellation previously updated its atomic stop flag and woke the condition
without acquiring the mutex used by the wait predicate. A cancellation between
the predicate check and entry into wait could lose its wake, leaving shutdown
blocked in join. The adapter now serializes cancellation and wake with that
predicate. The GUI dismiss signal remains outside the lock so synchronous
consumers can submit an answer or cancel again safely. Source copy, overwrite
decisions, transaction publication and rollback are unchanged; no source body,
dependency or branch is replaced or omitted.

The existing source/target preservation test now also delivers Cancel and
No-to-all through the GUI queue, alongside its direct before-wait cases. It
checks completion, result classification, unchanged destination bytes and
retained sources. The narrow lost-wake interleaving was established from the
mutex/predicate boundary; ordinary scheduling is not a deterministic reproducer
of that interleaving.

Ten focused transfer, language, worker progress, checkpoint, scanner, Copy-to-same,
volume and application/window lifecycle suites passed on macOS arm64, Linux x64
and Windows x64 / Qt 6.12. Application builds use the existing incremental trees
and compiler caches. Linux run `20261018T075744Z-27ac2824` and elevated interactive
Windows run `20261018T075744Z-4c6b3f3e` passed all ten selected suites. No full gate,
opt-in slow suite or visual comparison was run for this nonvisual change.
