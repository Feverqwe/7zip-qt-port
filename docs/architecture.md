# Architecture

This document describes component ownership, data flow and integration boundaries.
Source provenance, baselines, adaptations and migration debt belong in the
[source-port audit](upstream-port-audit.md). Supported workflows and differences
from Windows 7zFM belong in the [parity record](7zfm-parity.md). Working rules
are defined in [AGENTS.md](../AGENTS.md).

Native CApp::ExecuteCommand is the common entry for current menus, toolbar,
shared context actions, panel activation and keyboard commands. MyLoadMenu owns
the copied dispatch and CFileMenu capability policy. QtFileManagerCommands binds
actions and projects one capability snapshot per refresh; every execution takes
a fresh snapshot before effects. It adds only current transport restrictions and
delegates operations to existing owners. QAction enabled state is presentation.
The panel context popup feeds resource-derived File actions into native
`CPanel::CreateFileMenu`/`CFileMenu::Load` with `programMenu=false`; Qt renders
the returned order and enabled state. OS shell and 7-Zip extension groups stay
disabled until their separate platform command owners are ported.
Native CApp owns toolbar flags, persistence ordering and focused-folder title;
QtAppPresentation supplies toolbar reload/layout services. Dialog.cpp retains
the source dialog-unit size predicate with Qt font/work-area services. Complete
model, two-panel, mutation and shell workflows retain their separate plan owners.
For virtual root navigation, CRootFolder and CPanel own names, item binding and
parent resolution. `mainwindow_root.cpp` only renders the bound `IFolderFolder`
into Qt cells with source indices; the address is empty at the native root.
Qt Back history marks this empty prefix explicitly. Typed relative paths and
Parent call the native binding/parent owner, including the POSIX `/` to root
transition. Read-only root command policy prevents filesystem mutations.
For ordinary filesystem rows, CFSFolder provides source item indices, names,
flat prefixes, comment and file-identity properties. Qt only presents the bound
values and normalizes visible Unicode; its existing icon and timestamp display
remain pending separate source UI owners. `CPanel::InitColumns` now owns the
folder capability catalog and `CListViewInfo` save/restore; `FileTable` maps Qt
columns back to source property IDs and raw flags. `MainWindow` snapshots the
header and presents source-formatted cells but does not sort parsed text. Hidden
path/index cells remain transport-only. FilesystemFolderLoader runs source
enumeration off the GUI thread and FilesystemScanner batches Qt row publication.
The P0-05.2 large-scan fixture checks ordered source indices, event-loop turns
between bounded 32-row GUI batches, cancellation and rejection of stale batches.

CProgressDialog in mirrored ProgressDialog2 owns source presentation state and
statistics/resize/title/message/pause bodies. qtprogresscontrols supplies only Qt
widgets, native-style presentation APIs, font units, monotonic ticks and clipboard.
qtprogresswait supplies only the portable monotonic startup request/completion
wait boundary; ProgressDialog2 owns native worker creation/events and the source
modal lifecycle.
The source dialog owns original taskbar state/target/value/cleanup call sites;
guarded Qt targets become HWND only at the native Windows boundary.
qttaskbarprogress_mac owns Cocoa native Dock objects behind an opaque interface. It exposes
only native state/value/initialization services, without statistics or operations.
CQtLauncherProgress is the separate native DBus entry owner. It accepts an explicit
connection and desktop ID, publishes deltas/replay, exports native Query and clears
its entry before unregistering. Linux session/identity selection remains a native
adapter boundary; successful transport does not establish visible desktop support.
qttaskbarprogress_linux selects the actual session and Qt desktop identity, validates
GUI ownership and delegates protocol I/O. main.cpp sets the authoritative
desktopidentity basename; installation/association ownership remains P3-05 packaging.
MainWindow now uses the source CProgressDialog for all current archive, filesystem
and volume operations. ArchiveProgressTransport drives real 7zz pause and child
priority; QtCallbackProgressTransport forwards confirmed cancellation to file
and volume workers, disables Pause while those workers lack a pause capability,
and applies the native process background policy. The source dialog owns the
statistics, messages and final-message lifecycle. Background failure rolls back
source state and reports through CProgressSync.
P0-10.1 work now connects source CProgressThreadVirt and UpdateCallbackGUI2/
UpdateCallback100 to Add in an open archive through ArchiveProgressOperation.
This Qt integration owner bridges backend signals to the native worker, queues
GUI process requests and retains the worker until process completion/cleanup.
Password presentation runs on the GUI owner thread; accepted empty passwords are
sent as an empty line to the waiting engine. Start exceptions return to the
source worker's original exception/result composition. The source callback's
COM methods and native containers remain authoritative.

ArchiveBackend owns CProcessPause and child priority/PID lifetime, including reset
after exit. External engine pause uses native process control rather than blocking
the bridge in cooperative CheckStop: completion is still observed while paused.
Source sync's nonblocking path setter publishes external paths; a transport-only
percentage fallback updates the bar/title without fabricating byte/file counters.
Actual byte/count progress retains the original source calculation. Source pause
failure restores sync state; background child failure restores the parent policy
and reports through source messages. Backend startup propagates active parent
background policy to its new owned engine. This component is
accepted for Add/drop; the other current consumers use the shared source dialog
through the P0-10.2 GUI transport.
The icon ID service embeds original FM.ico with its native frames. processpriority
owns native process policy and owned-child PID adaptation for future 7zz transport;
widgets do not enumerate threads or construct process API requests. Source
7zz child propagation is supplied by ArchiveProgressOperation/ArchiveBackend. No native engine logic is
implemented by widgets.

P0-04.1 WIP also attaches one retained QtArchiveFolderManager and
QtAssociatedFileServices to the focused native panel. MainWindow owns those
services before CApp in member lifetime order; panels retain their manager.
The association adapter invokes the existing injectable QUrl opener on the GUI
thread and displays plain-text launch errors. It supplies no waitable process
handle; the copied native no-handle temp worker waits on a panel-scoped
CExitEventLauncher and joins before panel member release. Unchanged files are
deleted at owner exit, changed files are retained. This does not substitute for
the mandatory known/child-process/editor service or complete window-close owner.
MainWindow root/nested/address/parent navigation now uses the native panel parent
stack. Its duplicate ArchiveParent/temp stack is removed. The presenter reads the
current CAgentFolder and engine resource; the resource stores ArchiveInfo beside
its metadata transport. Native ReOpen/GetPathParts restore ordinary folder paths
through a joined modal bridge. CSelectedState carries deferred parent focus into
Qt row presentation, then is consumed; provider count changes suppress stale row
notifications. CApp now owns Create/CreateOnePanel/Save/ReleaseApp and its
per-panel native focus callbacks. QtAppPresentation builds menus/toolbars and
per-panel Qt controls, and migrates legacy preferences only when original
per-panel keys are absent. CPanel owns source binding/creation defaults and
processing state. MainWindow renders the bound filesystem provider without
releasing/rebinding it, quiesces its current workers, then saves and releases
the app owner. ReleaseApp disables all panels before any ReleasePanel call.
P0-08.1 projects each panel's Qt controls, location, view and sort through
`mainwindow_twopanel.cpp`; QSplitter replaces Win32 `MoveSubWindows` geometry.
The two CPanel instances remain authoritative for folder and selection. The
single scanner cancels a pending flat scan on focus transfer and restarts it
when that panel is active again. Disk auto-refresh polls both visible panels,
and command availability uses only the active projection. P0-08.2 extends this
projection to mixed disk/archive and archive/archive, restores the CWindowInfo
panel count/focus and QSplitter coordinate, and uses the opposite
CPanel::GetFsPath as the Copy/Move/Extract dialog default. An uncreated hidden
panel keeps its persisted path across close. Full cross-panel transfer remains
P1-01.
P0-02.3 adds CApp-owned QtAppOperationState retaining native timer/notify guards
through completion, and native FM WM_CLOSE through narrow Qt window services.
Close stops callbacks and joins workers before folder release, saves native app/
window records and becomes terminal; modal/live callers reject reentrant close.
Owner/lifecycle acceptance passed P0-02.3; cross-panel transfer and drop routing
remain mandatory P1-01/P1-16 work.

mainwindow_selectionpaths.cpp is the Qt boundary for source operated indices
and archive paths. Expanded extraction requests use real indices and typed Path;
logical rename/view/edit/hash-result targets use source full prefixes and names.
Filesystem selected paths use source GetItemFullPath with the native folder's
normalized prefix and OS conversion at the Qt boundary. Copy/Move/Delete/Rename,
hash, split/combine, View/Edit and Create input share this owner. Native
LoadFullPath supplies the prefix for current root/nested/address/parent consumers.
Filesystem CreateFolder/CreateFile and inline Rename now call the mirrored
PanelOperations.cpp worker and IFolderOperations; Qt owns only the combo editor,
table editor, refresh and visible selection. BrowseDialog.cpp owns source path
correction. Delete/recycle routing remains P1-03.2.
Source IsItem_Folder owns single-file eligibility. ArchiveBackend retains all
operation transport and temporary-file handling. Remaining filesystem consumers,
complete external-open and command/navigation acceptance remain mandatory.

ArchiveBackend::cancelAndWait is the transport quiescence boundary for destruction:
stop process writes before a window releases temporary source/destination paths.
MainWindow disconnects backend/scanner/file/volume callbacks before invoking it; backend
completion retains operation-specific cancellation cleanup. Ordinary cancel remains
asynchronous. Source CApp/panel/parent-stack release is connected; complete
operation-state and window-close consumers passed P0-02.3 acceptance.

The owned-provider/selected-consumer lifecycle is a prerequisite for native stack
integration. Broader runtime acceptance stays open until original parent/path
owners replace presentation-state decisions and complete all focused navigation
consumers; archiveParents_ has been removed. Native stack work uses accepted providers and bounded
lifecycle, avoiding a dependency on a runtime parent that itself needs that stack.

Panel.h owns original CTempFileInfo/CFolderLink records, native metadata, explicit
temporary cleanup and parent/interface/library references. Their change detector
uses unchanged upstream Compare_FiTime for native FILETIME/timespec. The runtime
parent stack and close/update/navigation consumers are connected by P0-04.1;
complete CApp release and external-process/editor owners remain mandatory.

CPanel owns original processing flags and nested RAII guards. Qt selection and
current-cell callbacks query the source notification flag before updating commands;
source RefreshListCtrl guards its intermediate selection changes. Full native
timer/notify/status dispatch and all-panel release integration remain mandatory.

CPanel relative/full path helpers and current prefix storage use original
UString. Source folder/type/device/super-drive classification owns their path
branches; Qt conversions are confined to setters and presentation. Ownerless
row path fallback is removed. Complete LoadFullPath/outer-stack prefix setup
remains open; a caller using GetItemFullPath must supply the native source folder
prefix until that integration. Filesystem/single-item path consumers and command
availability are separate subsequent migration scopes.

## Repository boundary

FormatUtils owns the complete original native number/placeholder helpers.
ResourceString's generated bundle includes the original FileManager STRINGTABLE;
mixed resource files explicitly opt into table extraction without interpreting
Win32 menus/accelerators/bitmaps or included dialogs. Actual command/dialog ports
retain their feature owners, and full source language loading remains P1-14.

sevenzip_common owns original native CLibrary loading/free and procedure lookup.
It also builds unchanged native C Threads and ErrorMsg, exposing upstream
Thread/Synchronization wrappers with one authoritative implementation and the
platform Threads::Threads link. Source progress/update consumers reuse these
owners; their modal UI/operation integration remains subsequent mandatory scope.

ProgressDialog2 owns complete original CProgressSync counters, native critical
section, pause/stop and error accumulation. Its POSIX pause sleep is a narrow Qt
platform service. CProgressDialog/CProgressThreadVirt modal/cancel/finish and Add
were connected in P0-10.1. P0-10.2 connects the remaining current archive,
filesystem and volume consumers, with source MessagesDialog replacing the custom
QPlainTextEdit journal view. A single recent-operation UStringVector retains
diagnostics for the File menu action. On GUI-thread completion, the source final
transition is delivered before command re-enablement so a late final prompt
cannot interrupt the next operation.
Original GetTimeString is the native duration formatter for that presentation;
source progress/update resource tables use the same generated resource owner.
POSIX parent-link transfer/copy guard uses the original Windows method bodies;
there is one adapted DLL owner. This is a parent-stack lifetime dependency, not
in-process archive engine integration. Module-path/resource services remain
mandatory separate ports; ArchiveBackend retains all archive transport.

PropertyName retains source resource/custom/numeric lookup. The non-Z7_LANG
LangUtils resource fallbacks call the adapted ResourceString service in
sevenzip_panel. Native Win32 LoadString storage is replaced by a build-generated
table from unchanged upstream PropertyName.rc plus fixed Russian presentation
captions. Generation rejects unsupported syntax; generated headers stay in the
build directory. Full source CLang loading/default-language/selection is P1-14;
selected-item Properties consumes the source PanelMenu and live folder binding.
The same generated resource bundle includes unchanged Extract.rc. Source
OpenArchive/ExtractCallback error-property helpers are compiled in sevenzip_panel
for presentation consumers; they do not introduce engine opening/extraction in
widgets or move ArchiveBackend's operation boundary.
PanelMenu now calls the original typed property helper for every successfully
read item property and for numeric aggregate totals. sevenzip_panel links the
existing native property formatter owner sevenzip_filesystem; no duplicate
formatting implementation is added. Window Properties passes its bound focused
CPanel and source operated indices directly to PropertiesDialog; metadata is
not rebuilt from rows/QFileInfo or another proxy. Packed aggregate visibility
now follows the original unconditional branch. Raw/archive-level/shell
capabilities retain their mandatory owners.
CreateArchiveFolder configures the existing QtPanelFolder with an explicit
ArchiveTransport catalog (Path/Size/PackSize), using source IDs/type descriptors
and immutable presence-aware metadata. Custom presentation callers keep the
default legacy catalog. Owned CAgentFolder applies original catalog expansion/
remapping and ReadItems scans ThereIsPathProp before proxy publication.
Directory counts come from the source proxy, while transport file counts remain
absent. Failed catalog queries do not publish an owner. External-proxy
compatibility delegation is removed; every initialized folder retains its agent.
Transport descriptors do not imply full engine metadata.
Archive selected-path requests synchronize source operated indices and delegate
their mapping to CAgentFolder::GetRealIndices. A narrow read-only transport
accessor supplies immutable full Path values by archive index; window path/
directory/proxy presentation roles and entries_ are not this consumer's metadata
source. QStringList conversion and overlap deduplication are integration glue.
The current 7zz flat directory expansion flag remains explicit until source
Extract integration. Selected single-item/filesystem/hash-result paths now use
source helpers; focused open/navigation and full context/parent paths remain
mandatory migration work.
Archive transport converts the local timestamp text emitted by 7zz List.cpp to
native FILETIME with declared fractional precision; seven fractional digits survive
the round trip. Invalid and missing timestamps return S_OK/VT_EMPTY, as native
codecs require. Custom presentation snapshots retain their separate text catalog.
The bridge is needed because the engine boundary exposes listing text; full native
codec property/raw catalogs remain mandatory P0-03/P0-04 work.

Portable filesystem enumeration and times use unchanged upstream FileFind and
TimeUtils. Adapted FileName retains the source path bodies and uses the single
FileDir owner, whose GetCurrentDir, native DeleteFileAlways and RemoveDir retain
source Windows/POSIX bodies. The Qt deletion path wrapper delegates to the native
function; Windows readonly handling uses original SetFileAttrib. These services
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
`QtPanelFolder` adapts the 7zz input metadata provider and legacy selection/test
presentation fixtures; window Properties uses its bound source folder directly.
The adapter exposes properties and owns immutable input metadata. It implements
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
The test-only external-proxy constructor/overload is removed; all initialized
folders retain a CAgent owner, including the focused test fixtures.
Raw/alternate-stream/operation and
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
fallback and parent exclusion. Native LoadFullPath supplies the current prefix.
No row metadata copy is used for functional selection decisions.

```text
Qt Widgets (window, file list, dialogs)
          |
ArchiveBackend interface (operations, progress, errors)
          |
QProcess (official 7zz executable)
```

## Window and panel state

`CPanel` owns the native folder and archive parent stack. `MainWindow` dispatches
navigation and presents that location, selection and command availability.
Its filesystem, loading, viewing and
operating states determine which actions are enabled. Dialogs collect input;
the window dispatches confirmed requests and handles asynchronous results.

The implementation is divided by responsibility:

| Files | Responsibility |
| --- | --- |
| `mainwindow.cpp` | Construction and scanner initialization |
| `mainwindow_menus.cpp`, `mainwindow_panel.cpp` | Resource-ordered actions, toolbar, address, file views and per-panel status presentation |
| `mainwindow_filelist.cpp`, `mainwindow_archivelist.cpp` | Listing, navigation and command state |
| `mainwindow_archiveopening.cpp` | Archive opening and nested archive lifetime |
| `mainwindow_archivecommands.cpp`, `mainwindow_backendevents.cpp` | Archive requests and result handling |
| `mainwindow_fileoperations.cpp`, `mainwindow_filedelete.cpp`, `mainwindow_volumes.cpp` | Filesystem and volume operation coordination |
| `mainwindow_selection.cpp`, `mainwindow_preferences.cpp`, `mainwindow_dialogs.cpp` | Selection adapter, settings and dialog integration |

`CPanel::Refresh_StatusBar` in mirrored `PanelListNotify.cpp` computes four
source fields from operated indices and typed folder properties. Each panel
has its own Qt status strip beneath the list; the main-window status bar
carries operation/scanner messages. Mirrored `SysIconUtils.cpp` keeps the source
attribute/extension/split-number cache and asks Qt's platform icon provider
for images. Widgets receive `QIcon` values rather than owning the cache.

`CApp` now owns `CAppState` and two independent `CPanel` instances. The window
binds the visible panel's list/address widgets through QPointer presentation
handles; `CPanel` owns remembered focus and `CApp` delegates focus restoration.
The UI can show both panels and restores their location/focus state; other
folder providers and full shell integration remain in migration below. Top-level focus filtering begins after panel construction
and stops before window member teardown.
`CSelectedState` and CPanel's selected-status vector own filesystem refresh
selection and current-item snapshots. `QtPanelList` adapts the shared table/icon
model and selection model; it supplies relative names and control operations.
`CApp::SetListSettings` applies source `CFmSettings` to the active panel.
`CSettingsPage` supplies the original list-option order; the Qt presentation
maps ShowDots to a source-sentinel parent row, FullRow to a table paint delegate,
and single-click track-select to one `QtFileListHover` per table/icon viewport.
The hover adapter changes selection after dwell without invoking navigation.
AlternativeSelection still needs the source marked-item state (P0-07.2).
Normal refresh restores after loading/sorting; flat refresh holds a pending
snapshot until scanner completion and discards it on navigation. Archive refresh,
flat-mode changes and edit results use the same source state. Pending snapshots
transport that state across asynchronous results; failed refresh clears it during
navigation rollback, and a removed archive folder discards names belonging to
the old location. Failed replacement retains the unchanged displayed state.
The source `CPanel::OnTimer` and selected-state reload policy own filesystem
auto-refresh. `MainWindow` supplies a one-second Qt timer and POSIX metadata
signature adapter because upstream `CFSFolder::WasChanged` only observes native
changes on Windows. A reload restores the top visible path or prior scroll
value; the timer pauses while flat scanning or archive operations are active.
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
completion before taking its COM owner, then presents source-index batches of
at most 32 rows on GUI timers. Count progress and atomic cancel remain available
during native
loading; cancelled workers are joined before destruction. A generation counter
discards stale results/batches after navigation or mode changes. Final sorting
runs on the GUI thread. Table and icon/list views share a model/selection model.
QFile::decodeName supplies native POSIX-to-Qt path conversion (macOS NFC); the
source folder retains native bytes. CPanel compares source names when restoring
selection; Qt display names retain Qt normalization. Normal loading uses the same
folder, source metadata and indices. Uncalculated directory sizes remain absent;
POSIX links expose their own lstat metadata. `CPanel::InitColumns` owns the
source property catalog and per-folder-type column settings; `MainWindow`
projects it into the shared Qt header. The external `7zz` adapter still needs
binary raw property support in P1-10; network namespace capabilities remain
P1-11.
`CPanel::_timestampLevel` and `CApp` carry the source time-level state. Qt View
menu actions use the original command IDs and `MyLoadMenu` dispatch; unchanged
`PropIDUtils`/`PropVariantConv` format typed `FILETIME` cells. `MainWindow`
redraws those cells on level/UTC changes and persists the choice through
QSettings; sort comparison continues to read typed folder properties.

`CPanel::SelectSpec/SelectByType/SelectAll/InvertSelection` own the upstream
indexed status vector and selection rules. `mainwindow_selection.cpp` keeps the
Qt mask dialog and synchronizes current Qt selection before invoking those
source methods; `QtPanelList` applies source indices back to the shared
table/icon selection model while keeping the focused row and parent sentinel.
In alternative selection, that source vector owns marks independently of the
single focused Qt row. `PanelSelect.cpp` owns Ctrl/Shift mouse ranges,
Shift-arrow and Insert; `PanelKey.cpp` and the window's clicked signals adapt
Qt event timing. `QtPanelList` paints marks through the shared model, so table
and icon views show the same state. Source operated indices drive commands
even when only one Qt focus row is selected.
`PanelSort.cpp` owns source typed-property, name/prefix and item-index sorting;
`filetable.cpp` passes bound source indices and property IDs to the panel and
adapts descending folder grouping and drag data to Qt. `PanelKey.cpp` dispatches
list and address keys through the window event filter.

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
filesystem icons through `QFileIconProvider`. `PropertiesDialog` supplies the
current operated indices to its bound `CPanel::Properties`, which owns enumeration
and totals through the source folder interface.
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
creates a missing destination and selects process arguments for non-Ask modes.
For Ask, the source extraction owners choose overwrite outcomes while
`ArchiveBackend` supplies only listing, validation, password checks and file
bytes. Passwords are never persisted.

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
An occupied target requests a source `NOverwriteAnswer` through a queued
window callback; the worker waits on a condition and cancellation wakes it.
The source `COverwriteDialog` maps native button IDs before answering. The
transaction holds replaced files/directories as backups until all selected
sources publish, and restores them on cancellation or failure. AutoRename
uses the unchanged upstream `FilePathAutoRename` implementation. Current
directory conflicts replace or skip the whole directory; source recursive
merge remains a later filesystem port.

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
For an Ask route without a source extract service, the backend fallback emits
each listed file collision to the window's source `COverwriteDialog` before
starting `7zz`. The user decisions
are retained for this extraction. The backend first runs a skip-existing pass
for unoccupied files, then targeted overwrite or auto-rename passes for accepted
conflicts. It uses the documented `-aos`, `-aoa` and `-aou` modes, never parses
the CLI's localized prompt. Each pass remains an official-engine process;
password and cancellation state continue across passes. The source in-process
`CheckExistFile` callback cannot run inside the external `7zz` child, so this
transport has a preflight-to-write race and the same direct-write partial-result
limit as noninteractive extraction.
For selected items or a viewed archive folder, `MainWindow` reuses the backend
destination guard, then calls mirrored `CPanel::CopyTo` with explicit extract
mode. `CAgentFolder::Extract` and `CArchiveExtractCallback::CheckExistFile` own
per-item paths, overwrite decisions and the source dialog. Whole root archives,
including nested archives, enter mirrored `GUI/ExtractGUI.cpp` and
`Common/Extract.cpp`: the source archive loop, wildcard censor, callback close,
result composition and multi-archive progress remain intact. `QtArchiveBatchOpen`
is the file-only `CArchiveLink::Open2` service while the in-process codec opener
is absent. It uses `QtArchiveFolderManager` to list each archive and fills source
`CArc`; for split files it records the additional `.002`... paths and combined
size so the source loop skips those entries. `QtArchiveFileResource` asks
`ArchiveBackend::extractSourceEntry` for bytes through official `7zz`; the
source modal worker owns completion while backend signals are suppressed for the
outer window. Both Ask routes write directly and retain the destination-change
race after validation.

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
