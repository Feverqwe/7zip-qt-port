# Architecture

## Reading guide

Start with the affected boundary: [repository](#repository-boundary),
[window/panel state](#window-and-panel-state),
[dialogs/preferences](#dialogs-and-preferences),
[filesystem/volume workers](#filesystem-and-volume-workers),
[archive backend/safety](#archive-backend-and-safety) or
[diagnostics](#diagnostics). Component notes below add source ownership and
lifetime details; locate the relevant class/path with `rg -n` before reading.
Current acceptance belongs to the [checklist](porting-checklist.md), and dated
source/platform evidence to the [audit](upstream-port-audit.md).

[Component notes](architecture/component-notes.md) описывают конкретные классы и lifetime; искать нужный символ с `rg -n`, затем читать его абзацы.

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
QPlainTextEdit journal view. The independent File-menu journal command and its
recent-operation collector were removed during the P2-09 source UI audit.
Errors remain in original Progress/Messages. MessagesDialog Create is modal for
the source PanelDrag stack-owned diagnostics; Close/Enter and Escape return the
original native IDs. MemDialog validation uses the same suppressible MyMessages
owner as other source early errors, while the source extraction callback owns
limit/action persistence and answer flags. On GUI-thread completion, the source final
transition is delivered before command re-enablement so a late final prompt
cannot interrupt the next operation.
Original GetTimeString is the native duration formatter for that presentation;
source progress/update resource tables use the same generated resource owner.
`QtDialogUnits` owns the native-font conversion used by Progress, Messages and
Overwrite presentation. It extracts the existing Progress/Messages alphabet-width
and rounded line-height mapping; original formatting, operation state and resize
handlers retain their owners. Win32 `CDialog::MapRect` requires a dialog HWND and
cannot supply this Qt font service. Overwrite maps its resource initial frame
through this adapter, allowing Qt minimum sizes to accommodate native controls.
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
CPanel through its source Properties entry; metadata is
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
`7zz` executable or deployed dynamic `7z` client. Widgets do not construct process arguments or implement
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
indices. Source OnItemChanged consumes owning-model selection deltas, including
inactive panes, behind notification/alternative-mode guards. Table construction
attaches that callback before command/status listeners. SaveSelectedState reads
updated source marks during the same Qt signal without rebuilding them from the
widget. Qt attachment seeds
a pre-populated model once; source loading allocates marks before deferred row
publication. Get_ItemIndices_Operated retains the source selected-focus
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
The entire module builds once in `sevenzip_system_icons`, shared by panel
presentation and the Windows FSDrives icon interface. This avoids a reverse
filesystem-to-panel dependency. Unix filesystem consumers retain their Core-only
link boundary; Windows filesystem providers also link the shared icon target.
The unchanged Windows FileSystem module belongs to sevenzip_common because
native FileIO/FileFind device-size queries call MyGetDiskFreeSpace. This keeps
common-only DLL consumers linked without depending on the filesystem UI owner.

`CApp` now owns `CAppState` and two independent `CPanel` instances. The window
binds the visible panel's list/address widgets through QPointer presentation
handles; `CPanel` owns remembered focus and `CApp` delegates focus restoration.
The UI can show both panels and restores their location/focus state; other
folder providers and full shell integration remain in migration below. Top-level focus filtering begins after panel construction
and stops before window member teardown.
After native CopyFrom replaces an archive proxy, the destination's old Qt rows
remain until source RefreshListCtrl loads and publishes the new catalog. The
filesystem-to-archive and interarchive publication adapters retain source
CDisableNotify while activating that destination, including rollback reopening.
Command availability must not query the new proxy through old row indices during
this switch. RefreshListCtrl then owns its existing load/publication guards and
saved selection; this does not suppress metadata errors or replace source sorting.
`CSelectedState` and CPanel's selected-status vector own filesystem refresh
selection and current-item snapshots. `QtPanelList` adapts the shared table/icon
model and selection model; it supplies relative names and control operations.
`CApp::SetListSettings` applies source `CFmSettings` to the active panel.
`CSettingsPage` supplies the original list-option order; the Qt presentation
maps ShowDots to a source-sentinel parent row, FullRow to a table paint delegate,
and single-click track-select to one `QtFileListHover` per table/icon viewport.
RefreshListCtrl retains the original explicit parent-name and fallback row0
focus/selection policy. The sentinel stays outside native marked/operated items.
Central command availability uses the source saved state's undefined parent
FocusedName plus selected Qt row to permit Open/Open Outside. The original
OpenSelectedItems then inserts the parent sentinel and dispatches OpenFolder or
OpenFolderExternal; the Qt gate does not implement another parent navigation.
The hover adapter changes selection after dwell without invoking navigation.
List activation goes through the source `CPanel::OnNotifyActivateItems`.
Qt decodes mouse/key modifiers and prevents shortcut preemption; it owns no
second modifier policy. `QtFileManagerCommands::ActivateItems` applies operation,
notification and reentrancy guards, then shares `RunNativeOpen` with menu Open
for selection snapshots, history and provider/state projection. Source Properties
and OpenSelectedItems retain their original operation owners.
AlternativeSelection uses the source vector and adapters detailed below.
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
The source CSelectedState::CalledFromTimer controls address publication through
the Qt reload renderer. Timer reload skips setting the address and rebuilding
its current-path image/history, retaining the edit cursor/selection. Manual
refresh and navigation publish the current path. This policy is passed through
filesystem, namespace and archive renderers before asynchronous publication;
pending selection still transports the original source state.
Source-ported `CAppState` groups `CFolderHistory` and `CFastFolders` and reads
both before the window builds menus or the address combo. Each window owns an
independent instance on the GUI thread; QSettings paths are explicit adapters.
Confirmed history/bookmark edits still persist their individual collection
immediately. Full `CApp`/`CPanel` lifecycle ownership remains migration work.
`CFolderHistory` and `CFastFolders` own persistent navigation data.
`CPanel::FoldersHistory`, `SetBookmark` and `OpenBookmark` connect that data to
navigation. The address combo projects the source current-path hierarchy. `ViewSettings`
converts string vectors at the QSettings boundary; `MainWindow` supplies the
complete displayed path to the navigation adapter.

`CProxyArc` owns the archive directory tree, entry indices and recursive size
totals. `CAgentFolder` supplies normal and flat traversal. `MainWindow` converts
these items into Qt rows and expands selected folders through the same tree.
Full entry paths remain in item data independently of display order.

`FilesystemFolderLoader` runs the original CFSFolder normal or recursive enumeration in a
worker without following POSIX directory symlinks or Windows directory reparse
points. `FilesystemScanner` waits for
completion before taking its COM owner, then presents source-index batches of
at most 32 rows on GUI timers. Count progress and atomic cancel remain available
during native
loading; cancelled workers are joined before destruction. A generation counter
discards stale results/batches after navigation or mode changes. Final sorting
runs on the GUI thread. The loader accepts an existing quiescent CFSFolder;
Init and COM ownership changes remain on the GUI thread, including release on
failure/cancel after joining. Reusing a folder still owned by a running worker
is refused without blocking navigation. Worker callbacks are cleared on every
exit. Table and icon/list views share a model/selection model.
QFile::decodeName supplies native POSIX-to-Qt path conversion (macOS NFC); the
source folder retains native bytes. CPanel compares source names when restoring
selection; Qt display names retain Qt normalization. Normal loading uses the same
folder, source metadata and indices. Uncalculated directory sizes remain absent;
POSIX links expose their own lstat metadata. `CPanel::InitColumns` owns the
source property catalog and per-folder-type column settings; `MainWindow`
projects it into the shared Qt header. P1-10.2 retains technical listing bytes
separately from the trimmed journal, projects actual named display fields and
typed UInt64/CRC, and exposes available checksum/SHA raw bytes through
qtpanelfolderraw.cpp. Agent source IFolderProperties/IFolderArcProps own folder
and archive-level enumeration; QtArchiveFileResource supplies actual header
metadata. No parent/name/ADS graph or unavailable large raw bytes are invented;
CProxyArc2 dispatch now retains original Agent tree/ADS binding and raw parent
properties behind native item-order proof. Native tree updates call the official
IOutArchive with original Agent callbacks through ArchiveBackend; ordinary CLI
mutation plans do not flatten tree/stream parent indexes. Windows filesystem
AltStreamsFolder is a source namespace rendered in the same Qt panel, with
CPanel workers for copy/move/delete/create/rename. Unix filesystem ADS remains
unsupported; archive ADS remains a portable source namespace.
`CNetFolder` owns the network namespace:
Windows uses upstream `NWindows::NNet`, while POSIX lists already mounted
network shares through `qtvolumemounts` and binds them to source `CFSFolder`.
Qt command availability enables Parent/Back for the virtual child folder.
Windows RootFolder's original filesystem-first binding falls back to CNetFolder
for server/container addresses. Qt's directory classification does not choose
the source provider: normal/flat listing first binds through CPanel and dispatches
namespace results through renderBoundFolder before accessing CFSFolder state.
Central Copy availability requires the source operations interface, retaining
CApp::OnCopy's unsupported-provider boundary.
`CPanel::_timestampLevel` and `CApp` carry the source time-level state. Qt View
menu actions use the original command IDs and `MyLoadMenu` dispatch; unchanged
`PropIDUtils`/`PropVariantConv` format typed `FILETIME` cells. `MainWindow`
redraws those cells on level/UTC changes and persists the choice through
QSettings; sort comparison continues to read typed folder properties.
Filesystem/archive metadata text follows the original callback display policy:
`ArchiveTableItem` asks its table's owning `CPanel::GetItemText` on display/edit
demand and caches only formatted text. Name uses this callback for DisplayRole
while EditRole retains the editor's raw value; Rename completion reads that role.
The source fast-name branch replaces RLO, compresses long space runs and marks a
Windows trailing space. Agent restores its original name/prefix/size interface;
FS restores the same conditional Unicode-FString interface on Windows. POSIX FS
keeps the original generic property path. Generic failed name/prefix properties
use the source cell's "Error: " result, with its original CR/LF display conversion.
Qt retains the existing NFC display boundary and flat-prefix composition; raw
operation names/indices remain in their original roles. A name callback with
notifications suppressed returns cached text or an empty string before its first
request. The fast-name buffer has room for the entire source name and terminator,
replacing only the Win32 callback's caller-supplied capacity.
`FileTable::textFolder` retains the shown
folder identity; source notification guards prevent callbacks during proxy
replacement. Column changes do not enumerate properties. Time/statistics changes
invalidate cached cells, including hidden statistics; numerical interaction roles
remain synchronized. Window teardown detaches the callback before CApp is destroyed.

`CPanel::SelectSpec/SelectByType/SelectAll/InvertSelection` own the upstream
indexed status vector and selection rules. `mainwindow_selection.cpp` keeps the
Qt mask dialog and synchronizes current Qt selection before invoking those
source methods; `QtPanelList` applies source indices back to the shared
table/icon selection model while keeping the focused row and parent sentinel.
In alternative selection, that source vector owns marks independently of the
single focused Qt row. `PanelSelect.cpp` owns Ctrl/Shift mouse ranges,
Shift-arrow and Insert; source `PanelKey.cpp::CPanel::OnKeyDown` owns list keys.
`mainwindow_panelkey.cpp` decodes Qt/native modifiers, claims conflicting
shortcuts and adapts posted Shift-selection timing to its owning panel.
`qtnativepanelkeys.cpp` is the narrow Qt GUI private-API boundary for native
digit decoding. Windows virtual digits survive shifted symbols; other native
QPA plugins resolve candidates through their existing keyboard-layout mapper.
Keypad digits retain their separate upstream meaning. Ordered right-Control
notifications preserve modifier state when Qt dispatch follows native key-up;
Windows extended scan codes use Qt's `0xE000` prefix. Source `OnKeyDown` keeps
bookmark priority over view-mode actions. See the matching SDK/runtime
requirement in [development](development.md).
The window's clicked signals supply mouse notifications.
`QtPanelList` paints marks through the shared model, so table
and icon views show the same state. Source operated indices drive commands
even when only one Qt focus row is selected.
Refresh resets the original group anchor to row zero before restoring marks
and focus; it does not derive that anchor from the restored current row. The
next Shift-click uses PanelSelect's original range and parent exclusion, while
Ctrl-click establishes its next anchor. Direct restoration and menu/timer
publication share this source phase.
`PanelSort.cpp` owns source typed-property, name/prefix and item-index sorting;
`filetable.cpp` passes bound source indices and property IDs to the panel and
adapts descending folder grouping and drag data to Qt. The window event filter
routes list keys through Panel.cpp's CMyListView WM_KEYDOWN branches before
source OnKeyDown, and address keys into combo-edit services. OnListViewKeyDown
adapts those Ctrl+PgDn/PgUp branches to the existing CApp command services;
Open Inside retains RunNativeOpen's source capture and Qt publication, while
Parent retains the existing goParent continuation. No second archive-opening
algorithm is introduced.
Source PanelMenu::EditCopy enumerates selected source indices and copies raw
names separated by CRLF, without focused-cell or operated-item fallback.
EditCut/EditPaste retain upstream's active no-op. Presentation commands share
CApp's command/capability owner; F9 and sort retain direct source owners.

Nested archive navigation keeps the parent listing, folder, password and
temporary directory on a stack. Returning to the parent or a listing failure
restores that state. Materialized engine locations are excluded from persistent history
and favorites. External-open copies remain owned by the window until it closes.
Address signals are disconnected before window teardown.

## Dialogs and preferences

`QtFmOptionsDialog` in mirrored OptionsDialog.cpp supplies the native six-page
property sheet in source order, initially showing System. All six pages are
available; System delegates registration to the native services described above.
CMenuPage preserves source list/state/apply bodies; policy flags feed the original
Explorer QueryContextMenu/InvokeCommand for FM menus and the Windows shell DLL.
Zone propagation goes through PanelCopy, Agent and Windows ArchiveExtractCallback.
Native registration is enabled only when its adjacent Windows DLL exists;
Unix keeps applicable in-app policies while disabling Windows-only controls.
CEditPage retains source modified-state and Browse/OnApply,
while CSettingsPage can be embedded or shown standalone. Apply/OK commit source
FM INI values; Cancel discards only pending changes.
System association errors are presented without vetoing later pages or OK, as
source PSNRET_NOERROR requires. Settings invalid input still vetoes acceptance
and selects Settings. Its error uses the retained MyMessages policy.
Source CSettingsPage owns RAM limit validation and capability-gated large pages;
QtMemoryLimitSpin adapts only edit/arrows. IQtMemoryLockServices adapts the original
FM startup and Settings token/LSA calls to unchanged native Windows services.
FM reuses the single ArchiveCommandLine process-mode flag consumed by Bench;
Settings preserves its separate source Apply behavior. Backend transports `-slp`
through the original CompressCall risk policy to the Windows engine process.
CFoldersPage
owns NWorkDir::CInfo; source WorkDir in sevenzip_filesystem serves both FM update
temporary files and the UpdateGUI Create working-dir block, transported via `-w`.
`settingsdialog.h` aliases the container. `MainWindow` applies committed list flags
and source RefreshAllPanels immediately and supplies
filesystem icons through `QFileIconProvider`. `CPanel::Properties` owns enumeration
and totals through the source folder interface; its Qt presentation service
receives the source-generated property rows.
CLangPage owns the source language list/info/apply state. LangUtils is the native
CLang lookup/reload owner, with source registry helpers on the explicit FM INI
adapter. Immutable English fallback comes from source STRINGTABLE/en.ttt; bundled
assets use pinned official release provenance and native-file staging. Language
loading, LangUtils and ResourceString belong to sevenzip_filesystem alongside
CLang and file I/O. These bodies use Core; Unix filesystem/transaction consumers
resolve local message context without linking Widgets. Windows also links the
shared icon module for the original FSDrives interface. Qt widget bindings remain in
sevenzip_panel. The source standard-button table also stays in LangUtils;
IQtLangDialogControls exposes text services without a Widgets dependency.
QtLangButtonBox in sevenzip_panel maps native control IDs and reapplies source
captions after Qt's LanguageChange reset. Source dialog consumers use it;
native dialogs and the Options property-sheet footer keep their platform owner.
FileTransferOperation publishes a typed completion kind before
its result signal; MainWindow suppresses fully rolled-back cancellation and
retains partial-move failures independently of translated message text. Qt text
bindings and mainwindow_language.cpp supply presentation updates only, retaining
existing QAction objects, editable fields, table rows and column state. Successful
Language Apply sends Qt LanguageChange before the existing list-settings callback;
invalid Settings input keeps the language pending. Current dialog and local
operation-message consumers are integrated by accepted P1-14.2; new capability
owners must reuse these presentation services and retain native language IDs.
The mirrored GUI/CompressDialog module owns the copied memory-error and size/
automatic display bodies. Its narrow public formatter declaration serves both
compression UI and backend preflight, avoiding a second message implementation;
the existing RAM calculation and engine boundary remain unchanged. Qt adapters
convert native UString/AString output for presentation only. Real decoder RAM
callbacks remain a separate required capability in P1-18.
The same source module owns FindRegistryFormat_Always, SaveOptionsInMem and
OnOK's memory refusal. Qt reads controls into the original CInfo spec snapshot,
converts Auto=0 to native all-ones specs, and passes the previous format during
format notifications. Source code owns profile creation and the large-dictionary
registry marker; the constructor's missing-Params guard preserves loaded options.
Source OnOK returns its memory-error text for Qt to display before settings save;
the existing unknown-RAM/no-explicit-limit adaptation remains unchanged.
QtBindResourceText preserves the native static-control RC fallback independently
of LangString's dynamic-message fallback. Language bindings update presentation
without reinitializing inputs; composite CPU/RAM rows use Qt focus proxies to
their real combo controls. Original ambiguous Add mnemonics retain Qt platform
behavior and unconditional Tab access. No native parser/lookup API was changed
to support empty captions: source language blank lines skip IDs.
`CListViewDialog` presents the result through the shared Qt list adapter;
the dialog never reads the file table or backend itself.
`CListViewDialog` reads CFmSettings for single-click activation and delegates
hover selection through CListView to the same QtFileListHover adapter used by
the main table/icon views and temporary-file browser. One persistent-index/
400ms timer implementation owns dwell and cancels it on press, leave, wheel,
hide, disabling or drag suspension; hovering does not confirm a path. The view
owns the adapter's QObject lifetime. Source dialogs retain activation, clipboard,
deletion and confirmation; the shared adapter never calls those actions.

Disk F3 first follows source CPanel::EditItem's operated-folder CalcItemFullSize
branch. Source FSFolder owns recursive statistics; InvalidateList projects fresh
properties through mainwindow_propertyinvalidation.cpp without LoadItems/rebind.
It updates source display values/numeric size data and status/actions while
preserving selected/focused rows. Large-tree asynchronous work remains P2-10.
For files, disk F3/F4 dispatch CPanel::EditItem using focused native indices; CApp::DiffFiles
owns pair/opposite-panel/fallback path choice. IQtExternalFileServices supplies
process creation with parsed program/base parameters and literal path argv;
QtAssociatedFileServices detaches disk launches. Archive launches return a
waitable IQtProcessHandle through QtNativeProcessServices; the source
CChildProcesses/MyThreadFunction own discovery, waiting and temporary-file
decisions. OS services supply snapshots, identity and process handles only.
QtExternalGuiCall defers source prompts/updates until the GUI operation owner is
idle, cancels pending delivery before panel teardown and keeps delivered records
alive through modal completion. MainWindow's external-update runner owns source
progress and refreshes the edited panel/selection while retaining the active panel.
Source password callbacks reach the writer through ArchiveBackend; protected ZIP
file updates enable CLI encryption explicitly, without password argv. POSIX
launch groups preserve ordinary child lineage after orphan reparenting; deliberate
daemon/session separation can prevent discovery. File viewing/editing uses
original `CPanel::EditItem` and external viewer/editor settings for F3/F4.
`FmSettingsPath` supplies the shared INI location outside MainWindow; callers
may supply an explicit path to history dialogs.
`CEditDialog` presents read-only full list values; it is not a file editor.

`CApp::VerCtrl` owns source file comparison, numbered backup history and
Edit/Commit/Revert/Diff. `QtVerCtrlFileServices` projects unavailable POSIX Win32
file metadata/methods through upstream FileIO/time conversion; Windows uses
original types directly. Source `CFileMenu` determines dynamic VerCtrl command
visibility and names from Diff plus `FM/7vc` and selected file attributes. Qt
binds those native IDs and refreshes selection/availability after execution.
The source advanced `7vc` setting has no setup UI; it remains an INI key.

`CPasswordDialog` owns the archive password prompt and show/hide state, preserving
upstream ReadControls/SetTextSpec/OnOK and its public `UString Password`.
QLineEdit converts only at that widget boundary; the source callbacks receive
the portable string directly. The asynchronous `MainWindow` process prompt
converts accepted raw text to QString for ArchiveBackend, including an empty
value, and cancels only rejection. Source modal operations own their callback
prompt instead. Passwords are held in operation/archive state, never saved to
settings; source opening/extraction callbacks persist accepted ShowPassword only.

`sevenzip_panel` owns shared `CComboDialog`, `CCopyDialog`, `CSplitDialog` and
panel summary functions. Callers validate operation input; dialogs commit
values only after confirmation. Folder snapshots supply summary metadata;
archive folder sizes come from `CProxyArc`. `ViewSettings` persists Copy/Move
destination history through QSettings. Its original UStringVector interfaces and
IsEqualTo_NoCase/Delete/Insert algorithm own comparison and ordering; CApp retains
the20-entry limit. QStringList is used only for settings storage and the Copy
dialog presentation boundary. Source-distinct K/K history values survive reopen.
Source FM.cpp::InitInstance owns Position/Panels admission, creation ordering and
normal/maximized show policy. IQtFileManagerInitServices supplies widget geometry,
creation and placement transport; MainWindow construction remains hidden until
the entry caller calls show. QWidget logical client coordinates/state replace
Win32 frame/workspace coordinates and g_Maximized. The existing close owner
saves normalGeometry/isMaximized; P3-05.16 tracks its current platform acceptance.
FM.cpp also retains CSplitterPos's fixed-point ratio and coordinate bounds.
QtFileManagerSplitter adapts initial layout, handle capture, resize and child
visibility to that policy; startup/F9/close share its state. Qt child minimum
sizes remain a presentation constraint, and unchanged layout requests preserve
effective user input.
QtPanelContextMenu destroys its popup/CallScope before refreshing cached QAction
availability, so native focus events under temporary source guards cannot leave
commands disabled after Cancel. Source menu admission remains authoritative.

`CExtractDialog` owns extraction input and confirmation. `NExtract::CInfo`
stores destination history and mode preferences in the Extraction INI group.
Toolbar Extract/Test dispatch through `CApp` and `CPanel`. Inside an archive,
Extract delegates to the source `OnCopy` workflow. A filesystem panel passes
the operated archive paths to `CompressCall2` through `QtShellArchiveServices`;
`ExtractGUI` supplies the single-archive caption and consumes the confirmed
dialog values. The source extraction owners choose paths and overwrite outcomes;
`ArchiveBackend` supplies native engine services, validation and cancellation.
`MainWindow` retains operation state and restores the originating pane and focus.
The explicit `extractTo` integration API retains its validated source/transport
adapter. Passwords are never persisted.

Compression and archive-path rules live in `CPP/7zip/UI/GUI/CompressDialog.cpp`.
The partial `CCompressDialog` owns name/extension transitions, path resolution
and archive history confirmation; `CreateArchiveDialog` supplies combo/directory
snapshots. `NCompression::CInfo` persists ArcPaths and ArcType.
The Add presentation uses `QtDialogScreenServices` for source dialog-unit outer
margins. CMake shares that existing service through `sevenzip_dialog_screen`
with both main-window and standalone Add consumers. Native widget rectangles,
explicit form-field growth and button StrongFocus preserve source control
exposure, equal combo widths and Tab traversal across platform styles.
Both dialog owners use the source UString::Trim and FileName/FileDir path
services. CompressDialog's GetFinalPath_Smart/GetFullPathAndSplit preserve
source resolution and refusal; ExtractDialog uses NormalizeDirPathPrefix.
The single AddUniqueString implementation in ExtractDialog supplies the source
UString::IsEqualTo_NoCase comparison to both histories. QString adapts widget
and settings storage; compression's embedded-NUL guard remains at that boundary.
`createarchivedialog_compression.cpp` binds them to Qt controls;
`createarchivedialog_resources.cpp` supplies CPU/RAM information and display;
`createarchivedialog_preferences.cpp` handles profile selection and persistence.
`createarchivedialog_encryption.cpp` binds source-ported method/password rules to
Qt controls. `CCompressDialog::CheckPassword` is shared with ArchiveBackend;
the backend validates encryption settings before staging and owns 7zz arguments.
`NCompression::CFormatOptions` and `CInfo` hold per-format profiles through
QSettings. `CMemUse` parses memory limits. `CompressionOptions` inherits the
original NCompressDialog::CInfo fields as the sole request values; the wrapper
adds only format/SFX path transport and initializes native Level to the existing
public default5. Dict64/Order/NumThreads retain the original unsigned(-1) auto
sentinels, and Method remains UString with GetMethodSpec's empty automatic choice.
sourceInfo adds archive/dialog context without converting a second parameter set.
The backend repeats validation before starting the operation and feeds the
original property parser; Qt converts strings only at control/helper boundaries.

`createarchivedialog_advanced.cpp` binds the original COptionsDialog to an
in-memory format profile and exposes the native parameter field and summary.
`qtcompressoptionscontrols.*` owns Qt presentation; the source precision and
capability bodies remain in GUI/CompressDialog.cpp. Native UpdateGUI property
helpers produce CProperty records, and ArchiveBackend alone projects them to
engine argv. Options Cancel discards its profile copy; only confirmed Add saves
INI. Lifecycle flags cross the same native request boundary. Preserve-atime and
shared writing are effective only on Windows; the global PreserveATime BoolPair
persists without becoming effective on POSIX. Delete is nonpersistent and uses
the native action-set check. Real decoder RAM requests require P1-18.

`archivebackend_creation.cpp` coordinates prepare/delete jobs around CLI creation.
`QtArchiveCreationLifecycle` owns native enumeration, snapshots, pause/cancel and
queued progress off the GUI thread. `QtArchiveDeleteInputs` invokes the copied
UpdateArchive deletion phase after final publication; native Add captures original
processed flags and invokes it after MoveToOriginal. EnumDirItems/SortUtils belong
to sevenzip_filesystem, so creation does not depend on the archive UI provider.
Native latest-time selection includes retained archive items and callback times;
QtArchiveFileResource applies it to CWorkDirTempFile before publication. CLI create
uses the original -stl service, and CMultiOutStream receives its completed mtime.

`createarchivedialog_volumes.cpp` binds the original volume parser and small-size
confirmation to the Add form. Confirmed native `CRecordVector<UInt64>` sizes cross
the `ArchiveBackend` boundary. `archivebackend_volumes.cpp` owns target checks
and lifecycle; `QtArchiveVolumeOutput` copies completed official engine bytes
through the original `CMultiOutStream` and transactionally publishes numbered
files. It supports pause/cancel and removes its own partial outputs. This CLI
integration needs a full temporary archive plus staged volumes; widgets never
construct split commands. Existing multivolume updates are rejected.

Open-archive Test dispatch uses native `CPanel::TestArchives` and `CopyTo(testMode)`
when the production extract service exists. Native smart selection and saved
archive-link password determine the request; `QtArchiveFileResource` supplies
official engine bytes to the source test callbacks. Split `.001` keeps automatic
outer handler detection instead of forcing its inner codec. Source batch Extract
uses VolumePaths to skip repeated numbered inputs. Filesystem batch TestArchives
is not yet exposed by this command.
The retained native CArchiveLink owns VolumePaths and VolumesSize. Batch opening
copies that exact accounting into its source link; it does not reconstruct volume
names from the process listing when a native reader exists. Native volume/nested
state also determines outer-stream detection and read-only update availability.
Process volume counts and numbered-file fallback remain only for absent native
readers; an untrusted count cannot disable a proven ordinary archive writer.

The independent `FileViewerDialog`, its 16 MiB preview limit, menu aliases and
MainWindow extraction/update wrappers were retired in P2-09. Original
`PanelItemOpen.cpp` owns archive viewing/editing, temporary-file lifetime and
update prompts through the existing narrow process/backend services.

## Filesystem and volume workers

`sevenzip_filesystem` owns `CFSFolder` mutations and the filesystem path snapshot.
Its original IFolderCompare supplies filesystem sort semantics directly to
CPanel, including raw comment-cache and metadata-request phases. The pure
natural-name portion of the authoritative PanelSort.cpp compiles into
`sevenzip_name_sort` with SEVENZIP_NAME_SORT_ONLY; the panel target compiles the
remaining CPanel portion. This shared QtCore/common owner avoids a dependency
cycle or duplicate comparator between filesystem and panel libraries.
`FileDir.cpp` owns recursive removal without traversing symbolic links.
The mirrored `CPanel::DeleteItems` owns confirmation, source progress-worker
lifecycle and refresh after an accepted deletion, including partial error or
operation cancellation. No/Cancel exits before refresh. `IQtTrashService`
delegates ordinary Delete to the desktop's Trash/Recycle Bin. Qt command
services hand restored source state to a retained asynchronous scan guard;
the window publishes source refresh without a second deletion workflow/error.
The explicit-index interarchive Move extension defers source publication until
its rollback transaction settles and releases operation guards. A test-only
callback driver remains in `tests/`. See the [source receipt](upstream-port-audit.md#p3-05--source-delete-lifecycle-2026-10-10).

`FileTransferOperation` owns the joined worker, immutable transfer inputs,
cancellation, overwrite interaction and public result/progress signals.
Its stack-owned `QtFileTransferTransaction` stages, publishes and rolls back
filesystem targets, with separate target-preflight and completion phases.
A private friendship lets the transaction use that existing worker state and
emit its signals; no second cancellation/result owner is introduced.
`CFSFolder::CopyTo`,
`CopyFileSystemItems`, `CCopyState`, `CopyFolder` and `CopyFile_Ask` in
`FSFolderCopy.cpp` own source copy/move control flow and callbacks;
`CCopyStateIO::MyCopyFile` is the portable byte-transfer branch.
`CFileTransferCopyCallback` adapts AskWrite/progress to the transaction without
owning widgets. Move tries rename and a separate link-validation traversal. When
source-to-stage rename fails, the worker copies through source `CopyFolder`/
`CopyFile_Ask` and defers source removal until every staged target is
published. A narrow filesystem adapter supplies source rename and deletion
steps for deterministic cross-volume tests. Cancellation before deletion rolls
back staged targets; deletion failure after partial commit keeps published
bytes and reports a partial result.

An occupied target requests a source `NOverwriteAnswer` through a queued
window callback; the worker waits on a condition and cancellation wakes it.
The source `COverwriteDialog` maps native button IDs before answering. The
transaction holds replaced files/directories as backups until all selected
sources publish, and restores them on precommit cancellation or failure.
AutoRename
uses the unchanged upstream `FilePathAutoRename` implementation. Current
directory conflicts merge through the same source CopyFolder body. The worker
copies the old directory into a stage, applies per-file AskWrite decisions,
then publishes the merged tree. For Move, only accepted source files are
removed; skipped files remain. Cancellation before deletion restores the old
tree, while deletion failure keeps published bytes and reports a partial move.

`PanelOperations.cpp` owns complete source CreateFile/CreateFolder workflows,
including dialog entry, validation, error/refresh policy and selection snapshots.
Qt command services call those owners; the window's existing reload callback
publishes the source CSelectedState. Filesystem creation stays in the original
IFolderOperations provider; CreateFolder uses its original progress worker and
CreateFile its original direct call. See the [source receipt](upstream-port-audit.md#p3-05--source-create-filefolder-scenario-2026-10-10).
When a native synchronous Create starts a deferred Qt scan, command services
hand its restored processing state to QtAppOperationState after source return.
The retained scan guard blocks callbacks until completion and then restores
that state; it does not retain the temporary native disabled snapshot.

The same module owns source RenameFile/OnBeginLabelEdit activation: focus chooses
the editor target, while marks belong to operated-item commands. QtPanelList's
EditLabel callback binds the owning pane and invokes its table/icon editor;
MainWindow does not independently choose an operated item for Rename. The source
begin-edit guard rejects parent/read-only targets. Existing OnEndLabelEdit and
queued reload publication retain rename validation, operation and selection state.

`PanelSelect.cpp` owns the complete mask-dialog lifecycle and original selection
loops. MainWindow admits selection commands and synchronizes/projects Qt state;
it does not construct a separate mask dialog. The explicit-mask integration
entry and source SelectSpec(bool) share one wildcard loop. QtPanelList supplies
focus, selected state, alternative mark roles and post-movement notification
timing. See the [complete source disposition](omitted-scope-audit.md#panelselect-complete-bodycaller-disposition-2026-10-10).

`PanelMenu.cpp::OnContextMenu` owns header/list admission, keyboard/empty/focus
branches, popup Cancel and numeric plugin/file routing. Each Qt table/icon/header
callback captures its pane. `QtPanelContextMenu` supplies control coordinates,
scoped popup/projection/shell lifetime and Track results; it blocks shared
QAction signals until source dispatch and suppresses peer FocusIn retargeting
while the popup closes. The existing application CallScope spans popup creation,
dispatch and destruction so Close cannot release its borrowed source providers.
The temporary panel projection then restores its peer.
The [complete PanelMenu disposition](omitted-scope-audit.md#panelmenu-complete-bodycaller-disposition-2026-10-10)
records native properties and plugin dependencies separately.

`PanelSplitFile.cpp` owns CApp::Split/Combine, source selection/default peer
destination, confirmation, destination preparation, contiguous part discovery,
naming/summary and native CThreadSplit/Combine transfer loops/preallocation.
`IQtVolumeServices` launches immutable native plans through `VolumeOperation`'s
joined QThread. `QtVolumeProgress` uses original CProgressSync and `QtVolumeIO`
adapts only the Windows/POSIX FileIO method names/types. QtVolumeTransaction
owns staged outputs, no-overwrite publication and rollback; VolumeOperation
retains input-change/symlink guards. MainWindow owns progress presentation and
joins the worker before releasing its progress snapshot. See the [source
receipt](upstream-port-audit.md#p3-05--complete-source-splitcombine-scenario-2026-10-08).

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

F5 from an archive uses the shared `CCopyDialog` and calls the source
`CPanel::CopyTo` through `CAgentFolder::CopyTo`, which selects `kCurPaths` for
the current folder. `ExtractionOptions::CurrentPaths` makes the backend
destination guard check the actual relative output path. F6 uses that copy
stage before the existing `ArchiveBackend::remove` stage because upstream
`CAgentFolder::CopyTo(moveMode)` returns `E_NOTIMPL`. An accepted transfer
refreshes the source panel and a matching filesystem destination panel.
For disk→archive, the opposite bound `CPanel::CopyFrom` calls source
`CAgentFolder::CopyFrom`/`CAgent::DoOperation`; source `UpdatePair` and
`UpdateProduce` choose the archive update plan. `QtArchiveFileResource` stages
callback bytes and timestamps, and `ArchiveBackend::updateSourceAdd` asks
`7zz` to write a separate output. Native `CommonUpdateOperation` publishes
the result and only then removes disk sources for F6. The UI refreshes both
panels after success. P1-01.3 now routes normal F5/F6 selection, destination
dialog and panel classification through mirrored `CApp::OnCopy`; the Qt
presentation adapter dispatches into filesystem, archive-source extraction
and archive-destination operations. Archive→archive uses source `CopyTo` to a
temporary folder and target `CopyFrom`; Qt presents the F6 delete confirmation
and refreshes both panels. `ArchiveBackend::saveArchiveTransferDestination`
provides a leased `QtArchiveTransferBackup` spanning target publication and source
Delete. It retains the existing snapshot filename/permission service and restores
bytes through original `CInFileStream`/`NCompress::CopyStream`, with a narrow
`QSaveFile` output adapter for atomic publication. The widget requests restoration
after a source Delete refusal and handles its result; it no longer owns a byte
copy loop. `ArchiveBackend` restores per-archive listed
entries/encryption for the active panel before Test/Extract/mutation. The
Shift+F5/F6 `copyToSame` branch and filesystem copy internals remain open
through P1-01.3/P1-02.

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

BenchmarkDialog retains the source worker, callbacks, synchronization and metric
calculations. QtBenchmarkServices bridges the original Bench/RAM/affinity APIs;
the POSIX OnInit branch copies original System.h count normalization. The Qt
message adapter queues to the owning QObject and treats detached completion as
the original successful no-op. Source normal/exceptional paths join workers
before that QObject is destroyed. No widget or service owns a benchmark algorithm.

Compression profile selection stays in the source CompressDialog module's
SetDictionary2/SetOrder2/SetSolidBlockSize2 loops. Their existing value snapshots
also return the selected index; QString/Qt combo insertion applies this index
without a second nearest-value calculation. MethodChanged passes stored values
only when source IsMethodEqualTo matches, retaining source Auto sentinels and
thresholds. Thread profiles retain source exact-match/no-selection behavior.

The dialog forwards CCompressDialog's numeric method ID to these calculations,
including -1 and external IDs. Source snapshots distinguish an unknown dictionary
from Copy's valid zero and retain whether an Auto item exists. Empty source lists
stay empty; Qt enables multi-choice controls by their item count. Memory rows use
the format's source MemUse flag, with QWidget containers solely for joint
visibility. Backend Store validation uses the same original no-method/thread
policy; archive properties and execution remain behind ArchiveBackend.

Create confirmation follows source CompressDialog::OnOK phases: password checks
(including stored text in disabled controls), memory refusal, SaveOptionsInMem,
final path resolution, volume checks, then accepted settings/output publication.
SaveOptionsInMem changes the dialog's transient profile only; refusal never writes
the INI. Source MyMessages owns suppression for password/path/volume errors,
with a narrow QWidget/resource presentation adapter. Direct source memory errors
and small-volume Yes/No/Cancel remain unconditional. The shared source memory
check applies its fallback limit even when RAM detection is unavailable; backend
request validation uses that same owner. See the
[validation audit](upstream-port-audit.md#p3-05--source-create-validation-error-policy-2026-10-07).

`tests/layout_inspection.h/.cpp` inspects public widget geometry and writes
JSON without editable field values. It is not linked into the application.
See [layout contracts](ui-layout-contracts.md) and [testing](testing.md) for
inspection procedures and coverage; [UI reference tooling](ui-reference.md)
defines resource extraction and structural comparison.
