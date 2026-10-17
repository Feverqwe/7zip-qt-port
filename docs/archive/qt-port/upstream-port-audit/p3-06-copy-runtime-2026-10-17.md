# P3-03/P3-06 — Copy/Move native runtime comparison, 2026-10-17

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

## Source and correction

Reviewed https://github.com/ip7z/7zip.git at full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/CopyDialog.cpp/.h/.rc`, `CopyDialogRes.h`, shared
`CPP/7zip/GuiCommon.rc` and complete `App.cpp::OnCopy` initialization/callers.
The original `CCopyDialog::OnSize` retains the resource Browse width;
`GuiCommon.rc::bxsDots` defines its twenty dialog units. The Qt button instead
inherited the native style's ordinary push-button minimum. At compact size it
occupied81px on Windows and80px on Linux, against the original30px, reducing
the destination field unnecessarily.

The existing adapted `src/CPP/7zip/UI/FileManager/CopyDialog.cpp::OnInit` now
maps that source width through the existing Qt dialog-unit service. This is
one presentation statement in the authoritative source owner. No functional
body, caller, archive operation, module boundary, language, dependency or
baseline revision changes; existing notices remain. No new independent
implementation or borrowed module is introduced. Tested input is
`28b3cd827ee291afa82e39da04b97ed0115b1d11` plus this correction and regression.

## Actual original and current native captures

The protected source-built original has SHA256
`0f00ac1e2b860632a97001440115d477ea8e3e5e7a19e08814cb6a753cd98eed`.
Its retained build receipt identifies the pin above, unmodified functional
bodies/resources and the existing private registry/entry adapters. The capture
run verifies the EXE, engine and language hashes, uses only disposable Unicode/
space fixtures, switches Russian through the original Language page and cancels
each Copy/Move dialog. The owned reference key is removed, process exits0 and
source/destination bytes remain identical. Windows run:
`20261017T110115Z-9ca75be3`, normal-priority Limited interactive desktop.

Eight actual original states cover Copy/Move × EN/RU × initial/compact. They
are application-runtime captures, not resource templates. Initial client is
504×260 physical pixels at96DPI; compact is deliberately340×220. Current Qt
captures use native Windows, Cocoa and XCB on Xvfb/Openbox. The latter verifies
native XCB widgets/window management, not physical Linux console input.
Qt is6.12.0 on all three platforms: macOS arm64, Ubuntu24.04.5 x64 and
Windows11 10.0.26300 x64.

`compare_ui_runtime.py` accepts all24 declared pairs: no missing platform,
mapping, state, text, clipping or spatial-relation error. All six mapped
controls and six declared relations are checked. Existing adaptations remain
Unicode ellipsis, scrolling read-only summary and native footer text/order.
No contract, threshold or adaptation is relaxed. Normalized bounds are retained
as measurements, not an unreviewed pixel-equality threshold.

Compact Browse sizes are33×26 Windows,37×22 XCB and29×32 Cocoa logical pixels,
against30×26 original physical pixels. Cocoa's compact client remains340×232
because of its native minimum. Initial Qt clients are561×320 Windows,
622×280 XCB and483×232 Cocoa. Font mapping, native footer order and the original
static label retaining its width when narrowed explain the retained proportional
differences; these results do not assert identical absolute geometry or pixels.

## Regression and focused validation

The final regression checks the established upstream twenty-DLU UI contract
after actual layout, at9/12/32pt in both languages and operations. Existing
normal/compact fit, grouping, summary, Tab, Enter/raw path and Escape checks
remain. Removing only the correction produces an actual macOS offscreen RED:
9Qt pass/12fail, with80px instead of37/47/123 mapped pixels. Restoring it passes
`copy_dialog` and `ui_parity_copy` again. Earlier experimental aspect-ratio
assertions were invalid across native/offscreen font services and were removed;
their failures are retained. An investigated paint-device API change is fully
reverted; the shared dialog-unit adapter is unchanged.

Final guest run `20261017T110941Z-b1e104c5` and the matching macOS run build
the selected targets, then pass six default suites on each OS:
`copy_dialog`, `ui_parity_copy`, `panel_copy_info`, `app_copy_routing`,
`panel_copy_reload`, `copy_publication_order`; each totals101Qt/0fail/0skip.
The resource checker is included in the six CTest results, not Qt totals.
Native `sourceFrameAndKeyboard` additionally passes14Qt/0fail/0skip and writes
24JSON/0PNG per OS. All captured tracked source inputs remain unchanged and
match the current correction. Baseline verification passes412exact/198adapted/
3local paths at one revision. Focused scope covers the view, its resource
contract, selection summary and direct Copy/routing/publication consumers;
no full or slow gate is repeated for this local presentation correction.

Local evidence:
`build/review/goal-copy-runtime-20261017/` retains original/pre-fix captures,
guest collections, source manifests and failed experiments;
`build/review/goal-copy-runtime-accepted-20261017/` retains the final matrix,
comparison, native reports, default/RED/restored-GREEN logs and archived drivers.
Runtime reports retain actual QPA, DPI, font, language/state and artifact hashes.

This accepts Copy/Move's declared runtime structure/state geometry and the
named keyboard/widget workflows. It does not establish pixel/font/icon parity,
physical input, OS folder panels, the full application walkthrough, Windows
cold-codec stability or final package acceptance. P3-03/P3-04/P3-06 remain open;
the package count remains44/47. Nothing is published or pushed; upstream is
read-only.
