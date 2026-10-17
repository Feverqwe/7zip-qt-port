# Open a dropped archive — 2026-10-17

Source reference: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed complete `CDropTarget::Drop` and `CPanel::CompressDropFiles` in
`CPP/7zip/UI/FileManager/PanelDrag.cpp`, their Qt event/operation consumers,
`CCodecs::FindFormatForArchiveName` in `CPP/7zip/UI/Common/LoadCodecs.cpp`,
and the existing `MainWindow::openArchiveAs` / `CPanel::BindToPath` path.
Upstream explicitly chooses AddToArc for external filesystem drops without
testing whether the source is an archive. There is no usable upstream branch
for the requested convenience.

The user explicitly requested a more natural drop action. The Qt drag adapter
now routes a single external regular file with a codec-recognized archive
extension to the existing archive-opening workflow in the destination panel.
The original source drag module remains unchanged. Multiple files, ordinary
files, open archive targets, internal/native 7-Zip transfers and right/middle
button menus retain their source handling. Symlinks and extensionless/unknown
files retain the original Add action. Detection reuses the official codec
catalog's name lookup; actual opening, password prompts and failures retain
the existing source archive-opening owner. A failed opening does not fall
back to compression. Opening reports a copy drop effect to preserve the input.

No upstream module is newly copied, replaced or omitted. Dependencies are the
existing Qt drag transport, backend codec catalog and source archive opening.
Source review covers the new routing and connected consumers; it does not
claim a whole-application audit or native Windows visual parity.

Runtime validation: macOS arm64 / Qt 6.12.0, incremental dev build of `Sevenzip`,
`drag_drop_add_ui_tests`, `drag_drop_ui_tests`, `drag_drop_tests` and
`drag_protocol_tests`. Focused suites passed; after correcting destination-panel
routing, the expanded `drag_drop_add_ui` suite passed with real ZIP opening
(Unicode/spaces, uppercase extension), opening into the inactive peer panel,
damaged ZIP refusal without compression fallback, multiple-file Add cancellation,
and the existing ordinary-file compression/extraction round trip. The peer test
waits for filesystem rows before restoring source-panel focus; earlier attempts
failed during test setup, before the drop. Existing `drag_drop_ui` checks retain
archive-target and middle-button menu coverage. No layout or text changed.
Windows/Linux and physical native drag gestures are unverified; no full or slow
gate is claimed. Temporary fixtures use `QTemporaryDir` and are removed on exit.
