# P3-06 — Cocoa inline Rename focus frame (2026-10-16)

The user reports that inline Rename text fits but its focus outline is clipped
above/below. The source baseline remains https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Original
`CPP/7zip/UI/FileManager/PanelOperations.cpp::RenameFile`, `OnBeginLabelEdit`,
`OnEndLabelEdit`, the panel declarations and Qt `MainWindow::beginRename`
callers remain authoritative. Windows ListView owns its native label-editor
presentation; it provides no reusable Cocoa focus-frame geometry implementation.

This changes only local Qt `filetable.cpp::FileRowDelegate::updateEditorGeometry`.
On Cocoa, QFocusFrame extends four style pixels above/below the frameless
QLineEdit. Previously the editor occupied the complete30px row, giving a38px
ring at y-4 on the first row; the viewport clips it. The delegate now reserves
`PM_FocusFrameVMargin` inside table rows. Current native JSON shows22px editor
height,21px required height and30px ring contained in the row/viewport. Icon
editors and other platforms retain their existing geometry. No upstream module,
algorithm, dependency, validation, commit/cancel or archive transport changes.

The public regression observes actual QFocusFrame bounds, editor minimum height,
Escape preservation and subsequent successful Rename in both panes/table-icon
views, with ordinary/alternative marks and disposable symbolic links. Native
RED v26 fails both table-pane cases; both icon cases pass. The correction passes
the same cases. Final default three suites (`panel_rename_state`,
`archive_rename_window`, `flat_view`) pass53Qt/0fail/0skip on each OS. Scoped
native activation, archive successful/relative Rename and flat-view consumers
pass31Qt/0fail/0skip on each OS,56JSON/0PNG per OS. Guest snapshot
`20261016T165319Z-93b85fc3` matches both changed inputs exactly; source and
restored-configuration guards pass. Windows Highest is used for disposable
symlink fixtures only. Official runtime inputs are rehashed/reused.

The broader Cocoa native v27 attempt retains six pre-operation activation
failures (two unsafe-archive and four selection rows); its geometry activation
cases pass. The final native subset does not establish those omitted rows.
The required unsandboxed full gate checks formatting/build and completes
323/324 in372.62s,5190Qt/1fail/66skip. Its only failure is the unchanged
`taskbar_adapter` child's initial stdout handshake. The preceding main-component
full gate passes324/324 with that fixture unchanged; the intermittent cause
remains unproven. Slow suites remain excluded and no deadline is changed.
This is scoped Rename acceptance, not a passing current whole gate.

Ignored evidence: `build/review/final-availability-8e9343b-20261014/`
`mac-rename-focus-red-v26`, `mac-rename-focus-green-v26`,
`mac-rename-focus-final-v27`, guest `final-rename-focus-native-v28-evidence`;
current macOS receipts are `build/review/final-rename-focus-native-v28`.
Manual confirmation of the new outline and real macOS Delete/collision messages
remains pending. Packages require a refresh. Parent acceptance stays open.
