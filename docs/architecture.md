# Architecture

This document describes component ownership, data flow and integration boundaries.
Source provenance, baselines, adaptations and migration debt belong in the
[source-port audit](upstream-port-audit.md). Supported workflows and differences
from Windows 7zFM belong in the [parity record](7zfm-parity.md). Working rules
are defined in [AGENTS.md](../AGENTS.md).

## Repository boundary

PropertyName retains source resource/custom/numeric lookup. The non-Z7_LANG
LangUtils resource fallbacks call the adapted ResourceString service in
sevenzip_panel. Native Win32 LoadString storage is replaced by a build-generated
table from unchanged upstream PropertyName.rc plus fixed Russian presentation
captions. Generation rejects unsupported syntax; generated headers stay in the
build directory. Full source CLang loading/default-language/selection is P1-14,
and Properties consumers still require PanelMenu integration and live binding.
The same generated resource bundle includes unchanged Extract.rc. Source
OpenArchive/ExtractCallback error-property helpers are compiled in sevenzip_panel
for presentation consumers; they do not introduce engine opening/extraction in
widgets or move ArchiveBackend's operation boundary.
PanelMenu now calls the original typed property helper for every successfully
read item property and for numeric aggregate totals. sevenzip_panel links the
existing native property formatter owner sevenzip_filesystem; no duplicate
formatting implementation is added. The window's Properties snapshot caller and
its temporary filesystem packed-size visibility remain live-binding migration
scope, while raw/archive-level/shell capabilities retain their mandatory owners.
CreateArchiveFolder configures the existing QtPanelFolder with an explicit
ArchiveTransport catalog (Path/Size/PackSize), using source IDs/type descriptors
and immutable presence-aware metadata. Custom presentation callers keep the
default legacy catalog. Owned CAgentFolder applies original catalog expansion/
remapping and ReadItems scans ThereIsPathProp before proxy publication.
Directory counts come from the source proxy, while transport file counts remain
absent. Failed catalog queries do not publish an owner. External-proxy
compatibility delegation remains mandatory lifecycle migration; transport
descriptors do not imply full engine metadata.

Portable filesystem enumeration and times use unchanged upstream FileFind and
TimeUtils. Adapted FileName retains the source path bodies and uses the single
FileDir owner, whose GetCurrentDir has source Windows/POSIX bodies. These services
are prerequisites for CFSFolder binding. Recursive scanning now uses the source
folder in a Qt worker; normal window listing calls that same folder synchronously.
`CFSFolder` owns FString/CFileInfo metadata and exposes the source read-only
enumeration/property subset with flat mode. Mutations convert to Qt at their
service boundary. Source folder binding and parent navigation use native services;
Windows drive roots return the read-only CFSDrives logical-drive owner, while
POSIX `/` returns null parent. Native Windows notification uses IFolderWasChanged.
Unchanged PropVariantConv and PropIDUtils provide the native variant/time/attribute
and raw-data formatters. The adapted FileLink owner retains upstream parser
bodies and POSIX link services. Windows native link operation branches remain
disabled until their FileDir service prerequisites are ported; no UI operation
interface is advertised by this formatter dependency.
Optional property/operations interfaces remain incomplete. Both window modes bind
their source folder to CPanel before publishing matching source item indices.
Packed size uses the original cached FSFolder property branch and Windows native
compressed-size service. On POSIX that service reports unavailable, retaining the
original logical-size fallback. Directory Size remains absent until calculated;
its source packed-size fallback is defined zero. Full native allocation/link/
stream metadata remains migration work.

`sevenzip-desktop` contains application sources, Qt presentation, platform
integration, tests and build tooling. Application modules live under
`src/CPP/`; reusable dependencies live under `third_party/7zip/`.
`../7zip-upstream` is a read-only source checkout, not a runtime dependency.

Archive operations go through `ArchiveBackend`, which launches the official
`7zz` executable. Widgets do not construct process arguments or implement
archive formats. A future direct engine adapter must preserve this boundary.
Backend destruction disconnects its process callbacks and stops the owned 7zz
before releasing buffers; an active mutation's temporary output is removed after
process exit. Shutdown does not dispatch operation completion into a closing UI.

The unchanged upstream IFolder/IStream/IProgress declarations, MyCom ownership
and PropVariant implementation are available for folder adapters. ClassDefs
defines their GUIDs once; consumers use declarations. The window binds the
archive and filesystem subsets.
Their default reference counting assumes GUI-thread ownership. Archive operations
continue through ArchiveBackend.
`CPanel::_folder` uses the unchanged upstream IFolderFolder/CMyComPtr contract.
`QtPanelFolder` adapts the remaining read-only Properties snapshots and the 7zz
input metadata provider; it exposes typed properties and owns the snapshot. It implements
no filesystem/archive navigation. Source property/name/path helpers consume the
interface; complete parent-stack ownership remains in migration. Active filesystem
folders use CFSFolder and archive folders use CAgentFolder. `ReleaseFolder` clears the bound list and caches before replacing
the interface. Complete source notify/lifecycle guards remain open.
`CAgentFolder` exposes the _proxy read-only IFolderFolder binding subset and
IFolderSetFlatMode. Child/parent folders share the typed metadata provider by
COM ownership. A partial source `CAgent` now owns an immutable 7zz listing proxy;
bound children/parents retain its IUnknown identity. The source engine owner
interface is not advertised. Window enumeration binds an owned folder from its
root and publishes its source indices; nested Qt parent frames retain the same
root. The former window proxy value and borrowed stack traversal are removed.
The borrowed overload remains in focused legacy tests. Raw/alternate-stream/operation and
full engine catalog contracts remain open; unsupported interfaces are absent.
`sevenzip_location` provides Qt input factories that use source CFSFolder path
services or source CAgent ownership with an immutable typed metadata snapshot.
Unknown metadata sentinels remain VT_EMPTY and do not wrap source directory
totals. The window uses both factories; outer archive links remain following
iterations. Archive size/packed and filesystem size columns consume typed
variants; absent values remain empty and known zero remains visible.
QtPanelList carries source item parameters separately from sorted row positions.
CPanel selection bits and property indices use source parameters; focus uses a
presentation row. Legacy snapshots build a derived param-to-row map at Save/
Refresh, while bound folders supply source name/prefix.
Copy/Move summaries now use that active panel's bound folder and source operated
indices. Qt selection is synchronized through SaveSelectedState until source
OnNotify is connected; Get_ItemIndices_Operated retains the source selected-focus
fallback and parent exclusion. The current display prefix is supplied explicitly
by Qt until LoadFullPath/parent-stack migration. No row metadata copy is used.

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

`CApp` now owns `CAppState` and two independent `CPanel` instances. The window
binds the visible panel's list/address widgets through QPointer presentation
handles; `CPanel` owns remembered focus and `CApp` delegates focus restoration.
Current UI uses one panel; location and full lifecycle ownership remain
in migration below. Top-level focus filtering begins after panel construction
and stops before window member teardown.
`CSelectedState` and CPanel's selected-status vector own filesystem refresh
selection and current-item snapshots. `QtPanelList` adapts the shared table/icon
model and selection model; it supplies relative names and control operations.
Normal refresh restores after loading/sorting; flat refresh holds a pending
snapshot until scanner completion and discards it on navigation. Archive refresh,
flat-mode changes and edit results use the same source state. Pending snapshots
transport that state across asynchronous results; failed refresh clears it during
navigation rollback, and a removed archive folder discards names belonging to
the old location. Failed replacement retains the unchanged displayed state.
Source-ported `CAppState` groups `CFolderHistory` and `CFastFolders` and reads
both before the window builds menus or the address combo. Each window owns an
independent instance on the GUI thread; QSettings paths are explicit adapters.
Confirmed history/bookmark edits still persist their individual collection
immediately. Full `CApp`/`CPanel` lifecycle ownership remains migration work.
`CFolderHistory` and `CFastFolders` own persistent navigation data.
`CPanel::FoldersHistory`, `SetBookmark` and `OpenBookmark` connect that data to
navigation. The address combo is a projection of history. `ViewSettings`
converts string vectors at the QSettings boundary; `MainWindow` supplies the
complete displayed path to the navigation adapter.

`CProxyArc` owns the archive directory tree, entry indices and recursive size
totals. `CAgentFolder` supplies normal and flat traversal. `MainWindow` converts
these items into Qt rows and expands selected folders through the same tree.
Full entry paths remain in item data independently of display order.

`FilesystemFolderLoader` runs the original CFSFolder recursive enumeration in a
worker without following POSIX directory symlinks or Windows directory reparse
points. `FilesystemScanner` waits for
completion before taking its COM owner, then presents source-index batches on
GUI timers. Count progress and atomic cancel remain available during native
loading; cancelled workers are joined before destruction. A generation counter
discards stale results/batches after navigation or mode changes. Final sorting
runs on the GUI thread. Table and icon/list views share a model/selection model.
QFile::decodeName supplies native POSIX-to-Qt path conversion (macOS NFC); the
source folder retains native bytes. CPanel compares source names when restoring
selection; Qt display names retain Qt normalization. Normal loading uses the same
folder, source metadata and indices. Uncalculated directory sizes remain absent;
POSIX links expose their own lstat metadata. Full property catalogs, source
BindToPath/LoadFullPath and parent-stack ownership remain migration work.

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

`CSettingsPage` presents supported list options and commits changed values
through source-ported `CFmSettings` in the FM INI group. Omitted flags are
preserved but do not affect Qt views yet. `settingsdialog.h` is a compatibility
alias. `MainWindow` applies accepted values and supplies
filesystem icons through `QFileIconProvider`. `PropertiesDialog` supplies a
metadata snapshot to `CPanel::Properties`, which owns enumeration and totals.
`CListViewDialog` presents the result through the shared Qt list adapter;
the dialog never reads the file table or backend itself.
`CListViewDialog` reads CFmSettings for single-click activation and delegates
hover selection to the Qt CListView adapter. Its persistent hover index and
owned timer prevent access to deleted rows; hovering does not confirm a path.
`FmSettingsPath` supplies the shared INI location outside MainWindow; callers
may supply an explicit path to history dialogs.
`CEditDialog` presents full list values; `FileViewerDialog` owns file viewing/editing.

`CPasswordDialog` owns the archive password prompt and show/hide state, preserving
upstream ReadControls/SetTextSpec/OnOK. `MainWindow` submits accepted passwords
to ArchiveBackend and cancels a rejected or empty request. It does not persist
the prompt's password or show state.

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

Compression and archive-path rules live in `CPP/7zip/UI/GUI/CompressDialog.cpp`.
The partial `CCompressDialog` owns name/extension transitions, path resolution
and archive history confirmation; `CreateArchiveDialog` supplies combo/directory
snapshots. `NCompression::CInfo` persists ArcPaths and ArcType.
`createarchivedialog_compression.cpp` binds them to Qt controls;
`createarchivedialog_resources.cpp` supplies CPU/RAM information and display;
`createarchivedialog_preferences.cpp` handles profile selection and persistence.
`createarchivedialog_encryption.cpp` binds source-ported method/password rules to
Qt controls. `CCompressDialog::CheckPassword` is shared with ArchiveBackend;
the backend validates encryption settings before staging and owns 7zz arguments.
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
