# P3-03 — Native archive drag cancellation fixtures (2026-10-16)

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

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

## Physical Windows Explorer recipient (2026-10-17)

Product `17fab0f86d7706795f7f5cea62a98c277c75d5ee` now passes actual archive
drag-out Copy and Escape into separate UUID-owned Windows Explorer folders.
Windows11Pro x64, Qt6.12.0/VisualStudio2022 17.14, official engine baseline above;
interactive snapshot `20261017T033718Z-6d4ebc6c`, Session1, Limited,
Elevated=False, Normal priority. No product code or source algorithm changes.
The reused native executable's producer matches all1047 committed product
inputs; executable SHA256
`85f28b83fb04b57c8c96bd3aaa5021b351adf3f063c45de0ac6defd2be9ef640`.

The disposable ZIP contains `Sevenzip payload ü 空.txt`. UI Automation confirms
the source archive address/row and owned Explorer content. Before input,
WindowFromPoint/GetAncestor must identify the exact owned source and destination
windows; all pointer movement, button transitions and Escape use Win32 SendInput.
Copy delivers exact bytes, SHA256
`aa7f8ddaefb9c7e79c35dd5ba8bae91735bc5e8be31e5dfb0e2e922bc36b6e0e`;
Escape leaves its destination empty. Both preserve the source ZIP hash and keep
the source application alive. Completion and cleanup close only fixture-owned
windows/processes; Explorer itself and unrelated windows remain untouched.

Earlier attempts expose runner-console occlusion and stop before input; one
earlier unguarded attempt times out and is not accepted. Showing both owned
windows above the console resolves the fixture prerequisite. An initial passing
run uses a misdecoded fixture name; the explicit UTF-8 replay above verifies
the intended Unicode name and bytes. Source/configuration guards pass.

Evidence in the final availability root:
`windows-final-explorer-drag-v148-evidence/`, including pointer ownership,
Copy/Escape results, exact Unicode payload proof, source/product hash receipts
and application logs. This proves a real external OLE recipient, rather than
the earlier synthetic cancellation alone. It does not prove every native
failure/lifetime state, cold stability, full rendered parity or parent acceptance.
