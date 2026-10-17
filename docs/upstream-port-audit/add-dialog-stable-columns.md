# Stable Add columns — 2026-10-17

Source reference: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed desktop `CPP/7zip/UI/GUI/CompressDialog.rc`, shared GuiCommon.rc,
`CompressDialog.cpp::OnCommand`, SetMemUseCombo, PrintMemUsage/SetMemoryUsage,
format/method changes and encryption visibility, with the Qt CreateArchiveDialog
construction, source DTO consumer, language and dependent-control adapters.
The source uses fixed gSize/g4xs/g1x/g1xs rectangles: selection handlers update
values/enabled/visible states without resizing columns or closing hidden gaps.

Qt's changing combo size hints and minimum widths had redistributed the columns;
hidden QFormLayout rows also moved the controls below them. Native reproduction
showed the compact English left column changing from 309 to 291 logical pixels,
and a Russian window growing from 651 to 652 pixels during selection changes.

Only the Qt presentation changes. Combo hints are independent of current items;
translated/native column minima are captured during setup and refreshed for
language/font changes. Normal manual resizing remains available. The memory
limit stays anchored at the right of its row, with memory text wrapping in the
remaining space and a reserved two-line value row. Nested forms fill their
column rather than retaining a content-dependent field width. Visible slots
reserve the source areas for hidden memory and
encrypt-header controls; a box layout retains their size because QFormLayout
itself collapses hidden rows. Existing capability/visibility decisions remain.
Dependencies are existing Qt Widgets layouts and language services. No source
algorithm, archive operation, caller or module is copied anew, replaced or
omitted. The review is scoped to this presentation boundary.

The regression selects extreme/restored values in seven compression controls,
then switches ZIP/TAR/XZ/GZip/WIM/7z on a disposable Unicode/spaces-capable file
fixture. It checks window and column sizes, remaining field rectangles and label
fit in EN/RU normal/compact states. It failed before the width correction and
also exposed collapsed rows and the moving memory-limit field during refinement.

Validation uses the existing incremental dev/release trees and compiler cache.
Checked platform: macOS arm64 / Qt 6.12.0, working tree based on
`31e94aace49d59c31916e5e6479135f912707ae9` plus this change. Focused scope:
source_compress_dialog, ui_geometry, compression_method_state,
compression_memory, language_ui and compression_encryption. Native producers
add selection-transition, language/pending-input and default-screen geometry;
evidence is retained in `build/dev/add-column-review/`.

Final focused CTest: 6/6 PASS (4.84s). Native selection regression: 6/6 PASS,
including 80 transition JSON reports with constant window/column/field geometry
within each EN/RU normal/compact case, fitting labels and selected combo text.
Native default-screen geometry: 3/3 PASS. The native language/pending-input check
passed on an isolated retry (3/3); one interactive capture run had unexpectedly
changed editable test input, whose cause was not established. The original
failure and passing retry output are retained; the offscreen language suite also
passed. Current normal/compact screenshots were inspected. Incremental release
build and local package installation passed; `dist/Sevenzip.app` was refreshed.

The sizing helpers stay with existing form construction; this fix introduces no
second workflow responsibility. Broader extraction of the existing large form
owner is deferred to avoid mixing a construction refactor with the behavior fix.

Source handlers are unchanged; runtime checks do not claim a new whole-port
audit, Windows/Linux execution or rendered original 7zFM pixel parity. No full
or slow suite is needed for the local presentation change.
