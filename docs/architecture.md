# Architecture

This document describes component ownership, data flow and integration boundaries.
Source provenance, baselines, adaptations and migration debt belong in the
[source-port audit](upstream-port-audit.md). Supported workflows and differences
from Windows 7zFM belong in the [parity record](7zfm-parity.md). Working rules
are defined in [AGENTS.md](../AGENTS.md).

## Repository boundary

`sevenzip-desktop` contains application sources, Qt presentation, platform
integration, tests and build tooling. Application modules live under
`src/CPP/`; reusable dependencies live under `third_party/7zip/`.
`../7zip-upstream` is a read-only source checkout, not a runtime dependency.

Archive operations go through `ArchiveBackend`, which launches the official
`7zz` executable. Widgets do not construct process arguments or implement
archive formats. A future direct engine adapter must preserve this boundary.

```text
Qt Widgets (window, file list, dialogs)
          |
ArchiveBackend interface (operations, progress, errors)
          |
QProcess (official 7zz executable)
```

## Window and panel state

`MainWindow` owns navigation, the current filesystem or archive location,
selection and command availability. Its filesystem, loading, viewing and
operating states determine which actions are enabled. Dialogs collect input;
the window dispatches confirmed requests and handles asynchronous results.

The implementation is divided by responsibility:

| Files | Responsibility |
| --- | --- |
| `mainwindow.cpp` | Construction and scanner initialization |
| `mainwindow_menus.cpp`, `mainwindow_panel.cpp` | Actions, toolbar, address and file views |
| `mainwindow_filelist.cpp`, `mainwindow_archivelist.cpp` | Listing, navigation and command state |
| `mainwindow_archiveopening.cpp` | Archive opening and nested archive lifetime |
| `mainwindow_archivecommands.cpp`, `mainwindow_backendevents.cpp` | Archive requests and result handling |
| `mainwindow_fileoperations.cpp`, `mainwindow_filedelete.cpp`, `mainwindow_volumes.cpp` | Filesystem and volume operation coordination |
| `mainwindow_selection.cpp`, `mainwindow_preferences.cpp`, `mainwindow_dialogs.cpp` | Selection adapter, settings and dialog integration |

`CFolderHistory` and `CFastFolders` own persistent navigation data.
`CPanel::FoldersHistory`, `SetBookmark` and `OpenBookmark` connect that data to
navigation. The address combo is a projection of history. `ViewSettings`
converts string vectors at the QSettings boundary; `MainWindow` supplies the
complete displayed path to the navigation adapter.

`CProxyArc` owns the archive directory tree, entry indices and recursive size
totals. `CAgentFolder` supplies normal and flat traversal. `MainWindow` converts
these items into Qt rows and expands selected folders through the same tree.
Full entry paths remain in item data independently of display order.

`FilesystemScanner` enumerates disk entries in bounded timer batches without
following directory symlinks. A generation counter discards stale batches
after navigation or mode changes. Enumeration and final sorting run on the GUI
thread. Table and icon/list views share a model and selection model.

`CPanelSelection` consumes a snapshot of names, folder flags and selection
states; `mainwindow_selection.cpp` publishes the result to Qt in one update.
`PanelSort.cpp` owns name comparison, while `filetable.cpp` adapts sorting and
drag data to Qt. `PanelKey.cpp` dispatches list and address keys through the
window event filter. Mouse and range selection remain owned by Qt.

Nested archive navigation keeps the parent listing, folder, password and
temporary directory on a stack. Returning to the parent or a listing failure
restores that state. Temporary locations are excluded from persistent history
and favorites. External-open copies remain owned by the window until it closes.
Address signals are disconnected before window teardown.

## Dialogs and preferences

`SettingsDialog` presents list options; `MainWindow` applies them and supplies
filesystem icons through `QFileIconProvider`. `PropertiesDialog` supplies a
metadata snapshot to `CPanel::Properties`, which owns enumeration and totals.
`CListViewDialog` presents the result through the shared Qt list adapter;
the dialog never reads the file table or backend itself.
`CEditDialog` presents full list values; `FileViewerDialog` owns file viewing/editing.

`sevenzip_panel` owns shared `CComboDialog`, `CCopyDialog`, `CSplitDialog` and
panel summary functions. Callers validate operation input; dialogs commit
values only after confirmation. Folder snapshots supply summary metadata;
archive folder sizes come from `CProxyArc`. `ViewSettings` persists Copy/Move
destination history through QSettings.

`CExtractDialog` owns extraction input and confirmation. `NExtract::CInfo`
stores destination history and mode preferences in the Extraction INI group.
`MainWindow` consumes the confirmed result; `ArchiveBackend` validates it,
creates a missing destination and selects process arguments. Passwords are
never persisted.

Compression rules live in `CPP/7zip/UI/GUI/CompressDialog.cpp`.
`createarchivedialog_compression.cpp` binds them to Qt controls;
`createarchivedialog_resources.cpp` supplies CPU/RAM information and display;
`createarchivedialog_preferences.cpp` handles profile selection and persistence.
`NCompression::CFormatOptions` and `CInfo` hold per-format profiles through
QSettings. `CMemUse` parses memory limits. `CompressionOptions` carries confirmed
values to the backend, which repeats validation before starting the operation.

`FileViewerDialog` reads bounded UTF-8 text and saves through `QSaveFile` after
checking for external changes. `MainWindow` owns extraction and the subsequent
archive-update request, keeping the temporary source alive until completion.

## Filesystem and volume workers

`sevenzip_filesystem` owns `CFSFolder` mutations and the filesystem path snapshot.
`FileDir.cpp` owns recursive removal without traversing symbolic links.
`FileDeleteOperation` runs deletion on a worker; the window connects progress,
cancellation and refresh. Destruction cancels and joins the worker.

`FileTransferOperation` owns copying/moving on a worker and the transaction
wrapper that stages, publishes and rolls back targets. `CCopyState`,
`CopyFolder` and `CopyFile_Ask` in `FSFolderCopy.cpp` own recursive copying;
`CCopyStateIO::MyCopyFile` owns byte transfer. Progress callbacks have no widget
dependency. Move uses rename and a separate link-validation traversal.

`VolumeOperation` owns Split/Combine path validation, temporary outputs,
publication and rollback. `PanelSplitFile.cpp` owns naming and the
`CThreadSplit::ProcessVirt` / `CThreadCombine::ProcessVirt` transfer loops.
The window coordinates confirmation and destination preparation; the worker
validates Combine parts again before writing.

## Archive backend and safety

`archivebackend.h` defines entries, metadata and asynchronous operations.
Companion implementations own process lifecycle, arguments, listing parsing,
extraction validation and mutations. `QProcess` receives structured arguments
without a shell. Output is assembled into complete lines and forwarded as
progress, current-file detail and journal messages. The window owns the
nonmodal progress dialog and operation journal.

Extraction requires a successful listing and validates the selected paths
(or all entries for full extraction), destination ancestors, links, portable
names, Unicode/case collisions and file/directory conflicts. No-path extraction
also checks flattened names. Existing output files are checked against the
chosen overwrite mode. Overwrite writes directly to the destination; failure
or cancellation can leave changed files. Preflight checks cannot prevent
concurrent external changes between validation and process writes.

Creation writes a temporary peer archive and publishes it after success.
Mutations copy the existing archive, update that copy and replace the original
through a backup/rollback sequence. Failed rollback retains the backup.
Preparing the archive copy currently runs on the GUI thread. Archive Move to
the filesystem is coordinated by `MainWindow`: extraction must succeed before
deletion begins. CRC uses `ArchiveBackend::hash`; archive entries are staged
through the extraction adapter first.

Passwords remain in memory while the archive is open and travel through
process stdin as UTF-8. They are excluded from process arguments, persistence
and forwarded output. Encrypted extraction requires a successful archive test
before writing output. Backend validation owns format and encryption
restrictions; UI availability reflects those capabilities.

## Diagnostics

`tests/layout_inspection.h/.cpp` inspects public widget geometry and writes
JSON without editable field values. It is not linked into the application.
See [layout contracts](ui-layout-contracts.md) and [testing](testing.md) for
inspection procedures and coverage; [UI reference tooling](ui-reference.md)
defines resource extraction and structural comparison.
