# P3-03 — Native archive drag cancellation fixtures (2026-10-16)

This changes only `tests/drag_drop_ui_tests.cpp`; product inputs remaina2254e5.
Baseline: https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Original
`CPP/7zip/UI/FileManager/PanelDrag.cpp::CDropSource::QueryContinueDrag`, its
effect/button state, lazy `CDataObject::CopyFromPanelTo_Folder` and callers were
compared with the adapted bodies, `DoNativePanelDrag` and
`QtPanelDragTransport::DoDragDrop`. No source body/dependency/algorithm changes.

The earlier Linux broad-native run times out in this fixture. Its synthetic
archive drag had no cancellation driver: offscreen returns IgnoreAction itself,
whereas native XCB waits in its drag loop. A scoped timer now observes the owned
QDrag and an increased thread loop level, then invokes public QDrag::cancel.
The table/icon cases require that this native cancellation actually occurs;
archive extraction, refusal to close during extraction, active-root protection,
selection/provider identity, original bytes and cleanup assertions remain.
The timer cannot cancel extraction before the QDrag is published.

Windows native OLE has a different source-defined boundary: no OS mouse button
is held by this synthetic fixture. QueryContinueDrag sees no button/effect and
cancels before requesting lazy archive bytes. The former Qt pre-extraction
expectation fails both table/icon rows. Final native OLE assertions instead
require no extraction, no extraction-time close refusal/protection and no roots.
The offscreen transport retains all original pre-extraction assertions. This
does not inject physical input or prove a successful external OLE drop; existing
direct native-target and platform workflow receipts retain their own scope.

Final default/native suite passes17Qt/0fail/0skip on Linux and Windows, with
nine existing geometry JSON reports and no PNG per OS. Final source file hashes
match snapshots Linux `20261016T172049Z-b986b048` and Windows
`20261016T171840Z-da98da1f`; source/configuration guards pass. macOS default
suite passes17Qt, and its scoped three archive-controller native cases pass5Qt.
The required unsandboxed full gate passes324/324 in350.91s,5191Qt/0fail/66skip;
the subsequent localized OLE-expectation change passes affected macOS checks.
No deadline or slow classification changes. The preceding taskbar handshake
failure remains historical/unexplained; one passing gate is not a cold fix.

Linux's first final follow-up fails before tests with ENOSPC. After active/latest3
guards,332 reproducible top-level binaries/static libraries from old successful
full run9b195e54 are removed; source/runtime/SFX/CTest/configuration/logs remain.
Free space rises737,280→2,543,030,272bytes. The fresh final follow-up passes.
No disk, toolchain, upstream, security or unrelated data changes occur.

Ignored evidence root `build/review/final-availability-8e9343b-20261014` retains
Mac v31/v32 receipts, actual failed/passing guest v31/v32 receipts, NOSPC log,
source hashes and `linux-old-full-products-cleanup.json`. This closes the scoped
XCB fixture blocker; it does not reclassify the earlier230/235 broad-native run
as235/235. Current broad acceptance and P3-03/05/06 parents remain open.
