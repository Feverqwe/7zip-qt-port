# P3-03.8 — worker-owned codec discovery deadline, 2026-10-17

## Boundary and source

Repository: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Reviewed the existing `ArchiveCodecProbe::Start/Discover/Complete`, backend
discovery/cancellation/destruction callers, and original
`CPP/7zip/UI/Common/LoadCodecs.cpp::CCodecs::LoadDll` and its native compatibility
query. Native codec loading and archive algorithms remain in those source owners.
Upstream supplies synchronous loading, not this Qt worker/deadline scheduler;
the scheduling policy is local integration code. No copied module, revision,
source language, public engine interface or package dependency changes.

The old GUI `QTimer` was the only writer of the timeout flag. If GUI delivery
was withheld until the worker had completed, the queued completion could stop
the overdue timer and publish a non-timeout result. A precise, monotonic
`QDeadlineTimer` now provides an immutable deadline for the worker request. The worker checks
expiry before/after process execution, during its existing polling loop and
after an uninterruptible native query, before queueing its joined result.
The GUI timer is removed. The default remains5000ms; no application or test
deadline is increased. Cancellation, process ownership, join and GUI-thread
publication remain unchanged. Native calls still cannot be interrupted safely;
an expired result is delivered after they return and the worker is joined.

## Regression and current checks

Evidence root: `build/review/final-availability-8e9343b-20261014`.
`deadlineDoesNotDependOnGuiTimerDelivery` uses an owned delayed version-fixture
process, a50ms probe deadline and withheld GUI event delivery. The corrected
worker may terminate the child before its exit receipt; the test does not
require that receipt after cancellation. It requires one GUI-thread completion,
`E_ABORT`, no published catalog and an inactive probe.

Actual macOS RED on the old implementation:
`mac-codec-deadline-red-LastTest-v67.log`,13Qt pass/1fail; result was `E_FAIL`
instead of required `E_ABORT`. Current macOS focused five suites pass78Qt,
`mac-codec-deadline-green-LastTest-v68.log`. Linux snapshot
`20261016T214221Z-6dd521e9` passes the same78Qt and two native XCB GUI consumers,
41Qt, with source/configuration guards passing. These cover process/native
loading, cancel/deadline/destruction, catalog replacement/version errors,
external handlers and command/creation availability.

Cocoa creation passes17Qt. Its command consumer exposed a separate retired
archive-proxy crash; that source ownership correction and the subsequent full
24Qt Cocoa command PASS are documented in the
[proxy lifetime receipt](p3-05-proxy-lifetime-2026-10-17.md).
Current Windows snapshot `20261016T222256Z-67c427ba` passes all18Qt probe checks,
including the new regression and Windows process-start/heartbeat cases. The
same snapshot still fails actual global-only external-handler discovery at the
unchanged5000ms deadline. Current macOS full gate passes324/324 in264.75s
with source guard passing (`mac-current-full-v83`). No appearance change requires
a new screenshot.

## Acceptance limit

This fixes deadline correctness independently of the unresolved cold stability
requirement. The earlier and current-probe global-only handler timeout is
retained in [timing evidence](p3-03-codec-timing-2026-10-16.md) and the
[long-suite receipt](p3-03-long-acceptance-2026-10-16.md). Passing this regression
does not establish a causal fix for native loading taking more than5000ms.
The narrow deadline regression is verified on all three OS; the independent
cold-loading requirement and final package refresh remain open.
