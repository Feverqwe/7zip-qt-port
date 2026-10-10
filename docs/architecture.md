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

This document describes component ownership, data flow and integration boundaries.

Archive opening enters the original CArchiveFolderManager::OpenFolderFile and
CAgent::Open through QtArchiveFolderManager. A per-opening IQtAgentOpenServices
lease supplies the retained codec catalog and ArchiveBackend service in place of
global loading. CArchiveLink::Open delegates to that service; native Open3 runs
once, while file-only listing supplies the existing metadata facade for Agent's
proxy. The facade and physical native link have separate roles and share the
resource/catalog lifetime. Factory binding scopes its progress pointers; the
returned source owner exposes IInFolderArchive and retains diagnostic metadata
even for a recognized non-open archive. Stream spooling remains resource-owned.

Agent LoadItems refuses a closed archive link before inspecting its proxy.
ReOpen invalidates the old proxies; after success callers bind a fresh root,
and after failure the original closed-link check still prevents LoadItems from
touching invalidated storage. ReadItems allocates the source path/tree proxy,
applies the source RAM limit, enumerates descriptors and loads items in that
order. Written property counts govern ReadItems/GetPropertyInfo even when the
provider returns S_FALSE or failure; unwritten counts initialize to zero.
GetNumberOfProperties retains its original HRESULT contract. A scoped cleanup
releases partial proxies on descriptor/load error, Cancel or exception so a
joined worker can retry binding safely.

Native archive item streams pass through the original CAgentFolder::GetStream
index mapping. CFolderArchiveProperties advertises the service only for a native
provider with proven item-order mapping. The narrow qtarchiveitemstream adapter
retains QtArchiveFileResource while delegating reads and optional seeks; native
stream interfaces release before their module/catalog/input owner. It preserves
the native output stream independently of the returned HRESULT. Explicit Close
or ReOpen keeps the upstream invalidation contract.
OpenItemInArchive owns timer suppression during native binding/extraction and
hands off restored flags to asynchronous row publication at each of its three
original successful refresh branches. OpenItem releases its outer timer guard
before dispatch; EditItem enters the same guarded nested owner.

Column-menu mutations belong to PanelItems::ShowColumnsContextMenu, with the
source AddColumn/DeleteColumn vector bodies. Qt supplies the selected catalog
index and popup transport, then projects visibility and appends newly shown
sections. Hidden Qt logical sections remain allocated; saveColumns counts only
visible sections when publishing source order. Hiding the sorted column changes
the source sort metadata and header indicator without sorting rows until Refresh.
Header popup/move/resize callbacks project their owning pane temporarily, and
popup exit restores the previous widget focus.
InitColumns clears source marks on every call, including unchanged catalogs,
and retains the original equal-column early return and DeleteColumn/AddColumn
rebuild. Name-first source logical indices are separate from header visual order.
Its original CIntArr permutation validation is extracted as GetColumnOrder in
PanelItems; configureColumns consumes it after creating Qt sections. Qt rows and
widget selection are published/restored by their existing owners.
FolderTypeID selects the source ViewSettings record. Descriptor Type/Name are
fresh provider metadata and participate in CPropColumns equality; saved records
contain PropID/visibility/width, not stale types or captions. The
[real-provider workflow](testing.md) checks these existing boundaries without
introducing another settings or schema owner.
Schema enumeration consumes the written generic/raw counts without an extra
HRESULT gate, matching upstream; individual property-info errors and generic
IsDir remain skipped. Both counts retain zero initialization for an unwritten
output. The same provider/type is re-enumerated, and Qt hides its reserved Name
section when no generic descriptor exists. Raw Name owns a separate section;
empty schemas retain rows/selection/status with no invented visible columns and
Qt's zero-height empty header. Native rendering of that empty state remains
unverified.
Label-only selection paint resolves source visible logical subitem zero through
the section map; it remains visible when reserved Name is hidden. Standalone
tables without that map retain physical column zero as their fallback.
Icon/list views use the same source logical-zero mapping after schema publication
and menu mutation. Their shared model forwards the existing physical-zero row
icon to that mapped primary cell; no extra provider or icon query is introduced.
Callback text uses the source index from successful row publication and the
owning descriptor/provider. It does not re-enumerate the item count before
GetItemText; the property/raw-property result owns its error/empty text.
FileTable retains only the published provider's non-owning identity for these
guards; callbacks query the current CPanel owner. A widget must not retain an
extra COM owner that keeps native archive streams open after ReleaseFolder,
because CloseOneLevel deletes the nested spool before publishing parent rows.
Load-time count validation, notification/provider guards, row deletion and
generation checks remain in their existing owners.
SaveListViewInfo receives the owning header projection in source visible logical
order, updates that visible snapshot and serializes the original visible/hidden
settings flow. It does not rewrite the property catalog. Menu re-addition uses
the catalog Width, as native AddColumn does; a resize retained until the next
InitColumns instead supplies the saved width for that initialization/reopening.
FileTable records the source catalog index for every published section. Generic
and raw entries with the same ID retain separate descriptors; raw or repeated
reserved IDs receive additional sections. Cells, alignment and timestamp refresh
resolve that owning descriptor. Initial publication matches type/raw/name and
occurrence; a separate section-to-visible-index map then retains source logical
order across native first-ID deletions and appended descriptor copies. Standalone adapters
without a publication map retain their previous ID binding. Namespace rows now
apply the same source alignment as filesystem/archive rows. Column menus enable
all entries except the protected first entry. Qt hides the section for the native
deleted index, shifts subsequent mappings and reuses an unmapped catalog section
or creates an additional section for each appended copy. New callback cells retain
their row source index and selection; each copy persists its own width/order.
Sorting uses source SetSortRawStatus's first catalog-ID flag rather than the
clicked descriptor's raw flag; published raw metadata remains immutable. The
FileTable header inherits Qt gestures and completes the native click callback
after Qt's automatic mouse sort. OnColumnClick delegates to the original source
toggle; projection then restores its direction on the clicked section. Explicit
programmatic sorts keep their requested direction. Sort actions map by PropID,
including raw Size sections; existing standalone comparison adapters without a
published catalog retain their explicitly supplied flags. Broader schema and
native acceptance remain in the port audit.

Copy/Move destination defaults belong to CApp::OnCopy: it reads the destination
panel's source GetFsPath and applies the retained filesystem-path reducer only
for one panel. QtAppPresentation supplies execution/presentation transport, with
no independent default-folder policy. Extract's separate dialog helper remains
in MainWindow. Source destination classification and directory/path diagnostics
also remain in App/Panel. Execution carries captured indices through filesystem
GetFilePaths, archive extraction and CopyFrom; source GetItemRelPath2 names and
existing QtFilesystemPath decoding own their boundaries. The accepted interarchive
Move supplies the same captured indices to the existing Delete owner after
publication, preserving its confirmation/guards and backend rollback. Remaining
completion reconciliation is tracked in the port audit.

OnCopy saves original source/destination CSelectedState after the modal and
retains its post-operation title/selection/notify/focus body in CApp. The Qt
presentation marks terminal execution and waits for the existing operation/scan
owner to become idle before invoking this continuation once. It restores the
captured source projection first; ReleaseApp discards the continuation. Ordinary
two-panel filesystem Copy preserves source rows/current item, while Move,
copyToSame and a shared single panel keep their required source publication.
The existing providers still own row loading; RefreshListCtrl invokes the
attached load/publication adapter before its extracted source selection tail.
Qt render completion calls RestoreLoadedSelection directly to avoid recursive
reload. Source requests retain their saved state while operation guards defer
publication; the actual renderer consumes it. The Copy continuation runs after
its existing Qt publication and uses the same extracted selection tail, without
retrying an already completed load or concealing its failure.
Filesystem-to-archive Copy/Move saves destination state before CopyFrom and
publishes that same bound provider through RefreshListCtrl after success.
ArchiveFolderOut already owns engine reopening and proxy restoration; the Qt
completion no longer opens another provider. Failure/Cancel retain the existing
unconditional destination OnReload and single-use source continuation.
Interarchive Copy/Move uses the same pre-update destination snapshot and source
RefreshListCtrl after successful publication. Its accepted Copy/Delete/rollback
composition remains: a source Delete refusal, error or cancellation restores the
destination backup, then physically reopens the restored file using that saved
selection. Before replacing bytes, the backend invokes a close service supplied
by the Agent-owning caller. It delegates to source CAgent::Close through a
narrow CAgentFolder bridge, releasing both the physical input stream
and native reader. Folder path/proxy state survives until the existing ReOpen.
Otherwise Windows reader sharing can refuse the rollback publication.
That rollback reopen replaces the provider deliberately. Neither
successful CopyFrom nor its row publication needs a second engine reopen.
Failed physical reopen binds the destination's return filesystem folder, rather
than publishing stale archive metadata. The existing refresh owner reports
non-E_ABORT reopen errors; a failed backup restore additionally reports rollback
failure. Source Copy completion still restores source focus and clears its marks.
The recovery fixture covers this boundary and reopening/retrying a repaired
private destination without changing the product's rollback implementation.
For ordinary Copy/Move, the Qt continuation also publishes the captured
destination panel when output is elsewhere. Consumers mark an already requested
destination refresh, avoiding duplicate publication. OnReload uses the existing
filesystem/archive/namespace publication owners; completion waits for their scan
owners before restoring the original saved states. Namespace reload invokes the
existing native-provider renderer instead of returning without loading rows.

The Refresh command and Ctrl+R invoke original CApp::RefreshView/OnReload for
every provider. Archive commands reload the bound CAgentFolder rather than
reopening the physical archive. Drop uses the original CopyFromNoAsk success
RefreshListCtrl; cancellation and failure return before that reload. CopyFromNoAsk
passes the reached success phase and saved state to ExecuteDrop, which
publishes after the synchronous timer/notify guards unwind. This prevents a
deferred scan from retaining disabled outer flags. The existing pending list-focus
owner also completes Drop's source focus request after publication. Direct source
callers without that output retain immediate Refresh; its HRESULT stays ignored.
The drag controller no longer reopens all visible archive panels after a gesture or Drop.
Filesystem changes remain observable through the source panel timer. Other
operation transport refreshCurrent callers remain a separate review boundary.

CPanel::LoadListItems extracts the original RefreshListCtrl load/property stage:
clear old rows/marks, retain timer/notify guards, set flat mode, load the folder,
initialize columns, validate/cache directory properties and query deleted-item
archive flags. Namespace, archive and small filesystem renderers call it before
publication. Worker-loaded filesystem folders skip only the already completed
flat-mode/load calls. Qt renders the resulting source columns without reloading
them; row directory flags use CPanel's validated cache. Resetting the same archive
folder after load would discard that cache and is no longer part of publication.
The original local numItems is retained across split Qt phases in an optional
loaded-count value. Load consumes the written GetNumberOfItems count regardless
of its HRESULT; unwritten outputs start at zero in this adapter. Namespace
publication and selection restoration reuse that value instead of querying the
provider again. Release/rebind invalidate it; a new load starts with the known
empty count so failed loads do not query metadata during mark projection.
Pre-populated Qt view attachment before any load retains its checked-count boundary;
source Get_ItemIndices_All keeps its separate original HRESULT policy.
PanelSelect::UpdateSelection temporarily disables only item-change notification
while applying source marks to Qt selection. Provider text/draw callbacks remain
available; an existing outer CDisableNotify retains its suppression. Alternative
mark colors are projected separately through Qt background roles.
External-file update transports an optional source CSelectedState to its Qt
completion. OnOpenItemChanged sets it only after reaching the original
unconditional RefreshListCtrl call; update success remains a separate boolean.
The caller publishes after operation guards unwind, including failure/Cancel,
and restores selection only after successful loading. Changed-folder/missing-item
and worker read-only exits do not request publication.
Inactive-owner publication uses withPanelProjection. The adapter retains the
window's focused control across modal progress and restores it after enabling
terminal UI; Qt may otherwise clear remembered focus when disabling an address.
The original drop-highlight row/name belong to CPanel, shared with PanelDrag's
target-path/type consumers. Both extracted Refresh phases clear them before
loading/restoration errors. Qt can retain published rows during restoration;
the list adapter also clears their drop paint role before resetting the source
fields. Drag highlighting uses that same adapter without changing selection.
Programmatic panel activation temporarily suppresses both source notify callbacks
and Qt focus projection while controls are enabled. This prevents a synchronous
focus return to the old pane from changing CApp's target halfway through loading.
Guards restore before the final command/title refresh.
The source icon phase is extracted into CPanel::LoadItemIcon/GetItemIcon.
Deleted-item foreground remains in source PanelItems::IsItem_Deleted and the
adapted PanelListNotify::OnCustomDraw policy, enabled by its original default
and archive-level flag. ArchiveTableItem asks the owning source-index/provider
through ForegroundRole only while notifications are allowed and provider
identity matches. Qt paint catches provider exceptions and uses the palette;
the public source property body retains its exception policy. FileRowDelegate
also projects the foreground into HighlightedText, since Qt otherwise overrides
it for selected rows. Table and icon views share that model/delegate boundary;
existing alternative marks/background and normal items retain their own state.
LoadListItems retains its slow-folder interface-query policy and initializes
indices without walking icon properties. Qt asks for each icon inside existing
bounded row publication. Positive system indices, attribute fallback, archive
attribute masking and directory-bit correction remain source policy. Windows
shell indices are converted through the small image list and QImage::fromHICON;
portable providers use the existing Qt cache, whose index0 is a default file icon.
CFSFolder exposes the original system-icon interface; POSIX adapts only the
platform lookup, using its existing GetWinAttrib conversion. This UI folder
library now links the icon service on every platform; workers still load items
without requesting icons. Filesystem/archive publishers return the icon HRESULT,
cancel further failed publication and clear rows through their existing owners.
QtFolderProjection completes once with the result and rejects replaced/cancelled
generations. Root retains its existing portable namespace presentation; native
Windows drive/network/stream rows use the bridged source icon.
Selection restoration also follows the original RefreshListCtrl name/prefix
walk: request the borrowed fast name before the existing property fallback,
reuse that name in matching, and request a prefix only in flat mode. A NULL fast
prefix uses the original property/BSTR branch and exception policy. GetFlatMode
selects the retained disk/archive flag at the Qt integration boundary. Refresh
returns S_OK immediately without a provider, after clearing drop paint/target,
before retained load errors, pending selection, guards, sorting or status. A
late callback after provider release cannot treat retained Qt rows as a folder.
The snapshot constructor and archive transport use real QtPanelFolder providers;
their selection restoration retains the same source walk. Sorted Qt rows still
map through source indices; displayed name
formatting is independent of saved native names and marks.
After matching rows, RefreshListCtrl uses the original notification guard's
SetMemMode_Enable/Restore before restoring focus. This permits Qt's focus
callbacks to request uncached source text and update command availability.
Failed loads return before that successful-publication phase; an outer guard
continues to protect the failed provider.
The retained completion phase also calls SetSortRawStatus and source list sorting
between named focus and positional fallback, then publishes Refresh_StatusBar
after final selection. The existing source four-part calculation now invokes a
weak per-panel Qt status adapter as well as returning its value. The adapter
updates the owning pane directly without focus projection. Action/language
refreshes use the same publisher; widget text assignment has one owner.
Agent exposes the original IFolderCompare and both proxy comparators, including
raw byte ordering, calculated folder statistics and natural flat prefixes.
Panel sorting passes the raw flag only for the selected property; name/prefix
tie breaks keep the original non-raw flag. The file metadata adapter queries
its existing raw interface into CArc at initialization, without synthesizing a
tree. Native ConfigureArc retains the codec's raw/tree capabilities. Original
Agent temporary prefix strings remain confined to GUI sorting. Its wide-name
calls share PanelSort's existing UTF-16 comparison through a Qt conversion
overload; no independent archive comparator is added.
Sort commands delegate through the existing command service directly to source
CPanel::SortItemsWithPropID. Its original toggle/default property policy,
SetSortRawStatus and focused-row EnsureVisible replace MainWindow's independent
mode switch. The default-property switch is extracted in the same source module
for Qt header roles. QtPanelList translates the property/raw flag through the
owning view's column resolver and sorts QTableView. Qt's indicator notification
supplies its explicit direction to the adapted OnColumnClick; it also restores
raw status and focused visibility. The header adapter projects its owning panel,
preserving peer/focus/menu state for inactive-pane sorting. Qt retains column
mapping, indicator, mode-action projection and persistence.
Original remaining property/sort/redraw/HRESULT presentation branches still
require the remaining complete-function review recorded in the port audit.
Namespace/archive renderers and the bound-folder renderer now return their
synchronous load HRESULT. The reload callback restores saved selection only on
success and returns failure to the retained CPanel::OnReload error owner.
Namespace and filesystem Refresh use that original owner; archive manual refresh
keeps its existing reopen transport pending the remaining reconciliation.
Namespace publication cancels the old projection and publishes the bound address
before loading, unless the timer flag suppresses that update. Load/property/icon
failures clear partial rows and share the successful terminal path for restoring
sorting and notification guards before leaving the Qt operation state. Thus a
Parent reload failure leaves both panes usable for recovery without rebinding
the source provider or changing the original ignored reload-result policy.
Archive failure still finishes its Qt state after synchronous loading, with source
guards restored before ending the operation. Remaining terminal branches stay
part of the complete-function review. CPanel retains the load-stage HRESULT across
Qt row publication. Its source-signature RefreshListCtrl returns that failure
before restoring selection, including the original OnCopy continuation. Empty
row selection snapshots avoid querying a provider whose count already failed.
Replacing/releasing the folder resets the retained result.
Synchronous filesystem reload keeps its already-bound CFSFolder and returns the
load result through showDirectory/startFilesystemListing to OnReload. Selection
restoration is skipped on failure. Other filesystem navigation retains binding;
large/flat asynchronous listing returns pending success. A worker failure enters
the same load-stage RINOK before properties/selection, retaining the bound
provider and result. Its source message appears before ending async publication;
retained scan guards cover that modal phase. Copy completion waits until the
temporary panel projection is restored before changing source focus/title.
Missing-folder reload also runs the source load stage, clearing native items and
marks rather than only Qt rows. CFSFolder uses original Windows Next(fi), which
ends enumeration on errors; the POSIX adapter mirrors that end-of-walk behavior.
Direct CFSFolder::Init still rejects a missing path. The existing missing-folder
status and auto-refresh snapshot support recovery when the directory returns.
CPanel::BindToPath resolves a typed missing descendant to its closest existing
ancestor. The Qt address caller uses the same original outer-quote preprocessing
as BindToPathAndRefresh, via the extracted PrepareBindPath helper; only Windows
strips those quotes. Source binding still owns fallback and archive-chain reuse.
Native quote execution and the remaining complete-function review stay open.

Source progress sets MainTitle to7-Zip. The Qt dialog can remain alive after
completion for queued close/error presentation, so the transport disconnects its
main-window title target before deferred destruction. It cannot overwrite the
application title restored by OnCopy or another completed operation.

Copy/Move availability follows original CFileMenu without requiring marked
rows; CApp's OperSmart determines the request, including the empty-folder no-op.
Busy/provider/read-only transport gates remain centralized. OnCopy releases its
stack timer guards in reverse order using the existing idempotent source guard
before QtAppPresentation retains timer/notify guards for execution. Async owners
hold them through completion; synchronous native archive consumers release them
after engine work/rollback and before panel projection and refresh, which may
acquire a scan owner. This avoids restoring a stale disabled timer snapshot or
blocking destination publication. Full row-load and native completion parity
remain under review.

Root-command routing follows original CApp::OpenRootFolder and
CPanel::OpenDrivesFolder, which own archive release, CFSDrives creation and source
refresh. The existing provider owns native drives/devices or POSIX mounts.
MainWindow's explicit RootLocation distinguishes that command destination from
the original root namespace used by Parent/empty history markers. Its filesystem
history retains nonempty source namespace prefixes and Back rebinds them through
CPanel. Root command, slash/backslash and keypad slash share one dispatcher;
widgets implement no drive enumeration or path algorithm. After guarded source
focus/selection restoration, the namespace renderer refreshes centralized command
availability so a same-row selection does not leave stale disabled actions.

Address dropdown paths and hierarchy belong to CPanel's retained
PanelFolderChange bodies and per-panel ComboBoxPaths. QtPanelAddressCombo supplies
the combo control, indentation painting, cached Qt icons, logical-drive/mount
service and queued list focus. It delegates binding/publication to the existing
MainWindow navigation boundary. Stored source paths bypass the typed-relative
address and legacy displayed-archive separator conversion; typed Enter keeps
those existing presentation adaptations. PanelKey routes Enter/Escape through
the original edit handlers once. FolderHistory remains the source history owner
and its dialog; command availability reads that history, rather than dropdown
row count. No widget builds an archive operation or its command-line syntax.

The queued address focus service checks the current application focus widget,
or the window's retained widget while application focus is temporarily null.
The retained widget preserves newer panel/control intent through popup/window
activation transitions. This adapter records list focus without activating an
inactive production window; source binding still owns the request/result.

LoadFullPathAndShow owns current-path icon classification, including the source
archive-link/virtual-file boundary and extension fallback. Qt supplies path and
namespace QIcons through the existing CExtToIconMap index space; native shell
indices are never mixed with those Qt indices. A leading QLineEdit action replaces
ComboBoxEx's edit-item image. Disk, archive and namespace renderers call this
source stage for their owning panel. Its explicit display-path/history arguments
preserve the existing Qt archive separator; icon classification uses the native
source prefix. Archive/namespace renderers record stable source prefixes through
CFolderHistory separately from displayed header text, including nested virtual
locations. Bookmarks store the focused panel's source prefix in CFastFolders.
Both saved-location consumers use source-address binding. Their narrow Qt
compatibility conversion removes only a former display separator following a
real archive file, preserving literal spaces in source components and stored data.
The Qt platform path cache shares the source extension-cache capacity bound.

Filesystem item comments belong to the retained CFSFolder provider. SaveComments
serializes its original CPairsStorage and writes descript.ion through upstream
file I/O; SetProperty retains source validation, trim/delete and cache transitions.
The provider implements the existing IQtFolderPropertyCapabilities readiness
boundary so CPanel's shared command state enables Comment. Source ChangeComment,
shared ComboDialog and existing property worker own interaction, selection and
completion. Widgets supply no description-file parser or serializer. Original
SetProperty ignores SaveComments failure; reload reveals the unsaved result.
Original void ChangeComment owns its selected-state capture and final
RefreshListCtrl inside source timer/notification guards. Early returns, including
dialog Cancel, do not load. The existing reload-presentation service loads before
selection restoration; failed loading does not restore old rows. The Qt consumer
supplies source progress lifetime and the same scan handoff as Create; it does
not set blocking Operation state around source publication. The source
HRESULT/caption helper presents property errors before refresh.

`cmake/SharedPortObjects.cmake` assigns cohesive production source groups to
object-library owners after all app/test targets, source-derived registrations
and startup fixtures have been declared. Backend, compression presentation,
MainWindow integration, native menu presentation, source compression/Agent/
selection/Split, file transfer, filesystem scanning and volume operations each
compile once. GUI and FM entry bodies have shared object owners; Windows reuses
the GUI entry in its MAPI host, and Unix reuses FM entry in its public-widget
startup probe. Consumers retain their link libraries, resources and test translation
unit definitions; each QObject header has one generated moc owner. Incomplete
source groups are configure errors rather than silently mixed implementations.
Borrowed C sources keep their C compiler and required engine/module variants.

ArchiveCodecProbe owns a joined discovery worker. It captures immutable engine/
module inputs, creates and reaps its local QProcess, parses the unchanged version/
library transport and constructs the existing ArchiveCodecCatalog, including its
private runtime image and original CCodecs loading calls. Native process creation
also runs there: Windows start can block before QProcess::started. The worker
reads atomic cancellation/deadline requests and touches no widgets or owner
QObject state. The probe receives its result on its QObject thread after joining;
ArchiveBackend retains its queued catalog publication. Cancel/deadline discard
the completed catalog. Native DLL entrypoints cannot be interrupted safely, so
cancel and destruction join active work before releasing module leases. A child
is killed/reaped after native creation returns; process waits check requests every
20ms. The unchanged five-second GUI deadline covers process and native loading;
delivery can follow it while native creation or a DLL entrypoint finishes.

Archive update preparation belongs to ArchiveBackend. Its CopyPrepare phase
runs the retained CCopyStateIO::MyCopyFile in QtArchiveCopyPreparation; source
mutation/plan and compatibility Add/Delete/Rename/Replace consumers resume their
existing engine stages only after the worker joins and publishes its private
peer stage. Qt owns scheduling, pause/cancel checkpoints and temporary lifetime;
the original module owns reads/writes, copy metadata and error behavior. A failed
publication cannot remove an existing caller-owned output. Normal local-filesystem
QFile rename supplies exclusive publication; unusual filesystem fallback behavior
is outside verified coverage.

Hash, creation scan/deletion and volume-output callbacks update one latest progress snapshot
under a mutex. A 33ms GUI timer delivers it without per-callback queued events;
generation tokens reject obsolete posts and finished deliveries. Joining a phase
stops its timer and discards remaining transient progress before its terminal
result; retained hash output, errors and source result aggregation are unchanged.
FilesystemFolderLoader likewise keeps an atomic count, polled by the GUI scanner
and delivered once at completion before folder ownership. Source cancellation
checks still execute for every native callback. These are external-worker Qt
transports; source CProgressSync/CProgressDialog remain the dialog owners.
The four hash, creation-lifecycle, volume-output and copy-preparation workers
delegate pause/stop checkpoints to CProgressSync through QtWorkerProgress.
Its Core-only source target separates Sync from panel presentation without
moving original functions. A source auto-reset event supplies immediate wake
for each single checkpoint worker; native dialog callbacks retain their original
polling mode. Copy preparation keeps an atomic cancel/publication mutex in this
adapter, separate from the source pause/stop state.
Filesystem Copy/Move and Split/Combine presentation feeds that existing
CProgressSync directly from worker callbacks. Only its mutex-protected source
state and the callback's local byte accumulator are accessed on the worker;
the original200ms dialog timer owns widget updates. Result and close paths join
before releasing the dialog. Public native-worker signals remain available for
direct cancellation consumers, without per-chunk queued MainWindow updates.

Core sections: [repository boundary](#repository-boundary),
[window/panel state](#window-and-panel-state),
[dialogs/preferences](#dialogs-and-preferences),
[filesystem/volume workers](#filesystem-and-volume-workers),
[archive backend](#archive-backend-and-safety), [diagnostics](#diagnostics).
The port-specific entries below record ownership changes at their stated scope;
current package status belongs in [the checklist](porting-checklist.md), and
dated source/validation evidence in [the audit](upstream-port-audit.md).

The Menu page's source controls edit the same explicit options file consumed
by CZipContextMenu. NativeMenuPageServices discovers adjacent original-name
DLLs and maps original registry operations to project-owned per-user Classes
keys; the original optional machine Approved operation retains its failure
policy. Unix exposes applicable in-app command settings while reporting native
DLL integration unavailable. Apply/requery, flags and command generation stay
in the copied MenuPage/RegistryContextMenu/ContextMenu modules.

P2-09 in progress: CPanel::CreateFileMenu now owns plugin release, extended
verbs, archive-folder exclusion and ShowSystemMenu sequencing. CreateSevenZipMenu
owns source path/attribute setup and query IDs; InvokePluginCommand owns numeric
offset routing and success-only selection clearing. QtFileManagerShellMenu renders
the source tree and transports commands to QtShellArchiveServices. CFileMenu's
regular-item projection starts at the already rendered plugin prefix; there is
no second Qt copy of shell command-generation rules. Windows System query/invoke
is connected through the native COM/HMENU presentation bridge. Windows Properties
and source MAPI email use their retained native owners. The installed native
component and System presentation are accepted under P2-09.2; actual Explorer
handler registration and Menu policy remain P2-09.3. Available source
Open/Extract/Test/Hash and direct Add/generation commands use the real
popup and program File menu; unsupported transports stay disabled.

QtProgramFileMenu retains the initial static resource actions separately from
the disposable plugin prefix. Original MyLoadMenu::OnMenuActivating's File
branch rebuilds it through CPanel::CreateFileMenu; Qt renders the resulting
projection. Plugin QActions dispatch through CApp::ExecuteCommand and the source
ExecuteFileCommand invoke/release branch. A generation token rejects deferred
QActions from an earlier menu, and focused-panel identity rejects a retained
selection after a panel switch. Rebuilding removes actions without deleting the
shared toolbar/resource actions. Popup construction uses the retained static
list, never the dynamic program menu. This Qt ownership adapter does not generate
functional shell commands.
Open/Create/Add-to-open/Extract-selected legacy extension actions and their
dispatch/fields are removed, including the independent Open archive picker.
Selection extraction is source Copy To/F5; archive update is two-panel Copy;
creation is source toolbar/Explorer Add with filesystem selection. Archive popup
contains only the source projection, without the extra Qt toolbar-action group.
Journal and built-in text aliases, their collector, viewer and MainWindow
preview/update wrappers are also removed. Original F3/F4 and Progress/Messages
own their workflows.
Toolbar Info now shares source IDM_PROPERTIES with File Properties; the
independent archive summary extension and window have been removed. Original
Properties supplies item and folder/archive sections, including empty archives.
Source CPanel::Properties now dispatches Windows filesystem items to the native
IContextMenu string verb; archive items use its retained collector and source
CListViewDialog. Unix uses that portable source collector for filesystem items.
Native shell HMENU presentation uses the same typed ContextMenu.cpp tree as Qt;
its Win32 adapter owns insertion/submenu transfer and maps the tree's icon tag
to the module's original MenuLogo bitmap. Command generation and invocation stay
in their source owner. Windows builds the same CZipContextMenu owner with its
four original COM interfaces into the source shell DLL. Each default/native
object owns its process transport, including enumerated children; injected
in-app consumers retain their existing lifetime. Source factory/unload and a
host without QCoreApplication are tested. Installed COM activation and GUI/map/
email consumers retain the same source owners; actual Explorer handler
registration remains separate Menu integration scope.
The redundant PropertiesDialog production wrapper has been removed; native
Windows runtime acceptance is recorded under P2-09.2 in the audit.

Windows NDLL::MyGetModuleFileName resolves the module containing its source
implementation, then retains the upstream GetModuleFileName/directory algorithm.
This serves both EXE and shell DLL callers without host-executable substitution
or a mutable module-instance global. POSIX retains the Qt executable-path service.
Windows settings use a fixed project config directory for both app and DLL,
and installed languages are resolved adjacent to the containing module. These
services do not adopt Explorer's application identity or executable directory.

QtArchiveBatchOpen carries source COpenOptions through QtArchiveFolderManager
and ArchiveBackend to NativeArchiveReader. ArchiveCodecCatalog binds its retained
codec owner before invoking the original CArchiveLink::Open3; source excluded
formats, properties and layered types are preserved. The Qt service pointer is
cleared at that boundary to avoid re-entering the transport. GUI extraction also
passes the original ArcType string unchanged to the backend listing transport;
the widgets do not construct or serialize archive command types.

Windows email commands reuse the retained Common/Update.cpp MAPI body through
the same source UpdateGUI service. File Manager presentation derives email
availability from the ordinary source Add capabilities; Unix retains an explicit
unavailable transport. The native process sender's `-seml.` selects the source
temporary attachment lifetime, while the in-process CompressCall2 options keep
their original archive policy. Qt does not implement a second mail workflow.

The full source Common UpdateArchive/Compress orchestration and its TempFiles,
UpdateCallback, UpdatePair and UpdateProduce dependencies belong to
sevenzip_filesystem. sevenzip_location consumes that owner for Agent workflows;
panel-only backend consumers can also link it without depending on Agent/UI
providers. ArchiveBackend::updateSourceArchive retains the catalog through the
call, while ArchiveCodecCatalog confines the original CCodecs pointer to its
factory owner. Source CThreadUpdating/UpdateGUI own callback, native worker,
error paths and modal progress completion. IQtUpdateGUIServices transports the
source update call through ArchiveBackend with its original COpenType vector
and retained catalog; the worker reads no QProcess/widget state. GUI preflight
checks busy/stale catalog state. Direct popup Add to7z/ZIP and SHA256 generation
use this path. Source ShowDialog now owns the options roundtrip and source
SetMethods filters external encoder metadata. Its PresentCompressDialog boundary
now presents a fixed-selection Qt modal using the service's retained catalog,
including external encoder metadata. Generic popup Add is connected. Source
modal success/broken-existing archive/Cancel and selection/refresh are checked;
new layout corrections and native platform gates are in progress. Source-modal
SFX uses the catalog's validated native module path supplied by the service;
the original adjacent-module fallback remains. Native output shares the Unix
executable-bit adapter with CLI output, and explicit SfxMode reaches the source
volume-name owner so suffixless native executables can find their `.7z.001`
payload. Local native creation, encryption, volumes and cancellation pass;
fresh Windows/Linux validation is pending.
The subsequent24-suite native snapshot passed on both Windows and Linux,
including executable SFX fixtures. The later source method selector owns raw
IDs and automatic specs separately from Qt captions; its latest Windows
validation passed in29-suite snapshot07e2aeb5 on Windows/Linux. Actual default
v12 was compared with native7zFM; subsequent program-menu edits need their own
platform checks and do not inherit that result.
The ordinary filesystem Add no longer chooses sources inside its parameters
dialog. The legacy open-archive Add workflow and independent program File-menu
aliases have been removed in favor of the original command projection.
[the mandatory UX audit](ux-port-audit.md) tracks it and other independent
presentation decisions throughout the application.

Source shell kOpen launches another File Manager. The Qt adaptation creates an
independent MainWindow/backend with its own operation/process state and shares
only the immutable codec-module lease. Explicit open type reaches BindToPath;
archive-only entry retains an S_FALSE archive refusal even when BindToPath
successfully binds the containing filesystem folder. Cancellation and failed
operations do not clear the caller's selection. Native synchronous errors are
presented once through their source services; cancellation stays silent.

P2-08 accepted: source SystemPage owns association selection/state/apply;
FilePlugins owns extension merging. ArchiveFolderOpen's CArchiveFolderManager
owns the icon-resource association catalog behind QtArchiveFolderManager, which
retains its immutable codec catalog lease. Archive opening has its existing owner.
IQtAssociationServices performs pre-write revision checks and synchronizes aliases
sharing a native type after its own successful mutation. Native platforms own
Win registry/notifications, Linux GIO desktop MIME services and macOS NSWorkspace
completion/history. Options invokes them only on explicit Apply, with reentrant
dialog actions disabled while a native completion is pending. Tests inject fake
registration boundaries. GUI code neither writes OS registration directly nor
builds archive process arguments for this workflow.

QtAssociationFileOpen retains native application file events until the UI
consumer is connected, then dispatches local paths asynchronously. main.cpp
uses an additional MainWindow and its existing openPath/backend owner for each
native request, so an association activation does not replace an active operation
in another window. macOS document declarations derive from the pinned source
association resource; Alternate rank does not select an OS default application.

P2-07: source Common/Extract owns root elimination;
ArchiveExtractCallback owns path correction, overwrite, attributes and timestamps.
IQtExtractPathPolicy checks source-corrected containment and occupied/link ancestors
before creation. QtArchiveBatchOpen validates each actual listing before the
source loop. ArchiveBackend invokes native extraction after count/path-order
proof, retaining reader/catalog lifetime. Agent borrows metadata/path choices
only while CopyTo's worker is joined. macOS/Linux startup openPath delegates to
existing native navigation; it does not implement a second opener. Windows FM
startup uses FM.cpp's original quoted-string/type parsing and path preparation.
Per-launch QtFileManagerStartup replaces its globals and feeds the original
CApp::Create/BindToPath during MainWindow construction. ArchiveBackend discovery
finishes before an initial archive bind. A typed failure transports the original
WM_CREATE refusal to the Qt entry, preserving silent E_ABORT and InitInstance's
FALSE return without showing a partly constructed File Manager. WinMain's
exception policy remains in FM.cpp under a narrow namespace because the native
Windows entrypoint cannot be overloaded; Qt supplies widgets and its event loop.

Metadata storage choices flow from original CompressDialog bool pairs and
handler capabilities through the Add request into original CDirItems and
CArchiveUpdateCallback. Requested metadata uses native SetProperties/UpdateItems;
ordinary process plans retain their current transport. One validated source
CProperty projection supplies both transports. Native readers retain their exact
catalog for format/capability decisions. Source processed-item statuses and the
no-follow snapshot reach deletion only after publication. Windows native
security/streams are validated through native WIM fixtures. Native link extraction preserves
source deferred creation and safety algorithms; the borrowed destination policy
checks final symbolic/hard-link targets before creation. Preflight permits links
only for a proved native service, with directory links treated as leaves.
File-only transport refuses archive links. POSIX fixtures are checked; Windows
link extraction is checked through native TAR/WIM fixtures. MainWindow scopes preflight to the real Agent
indices used by CopyTo, including every entry in the current folder. Actual
Add/Options/native update/deletion and selected/current-folder extraction are
covered by the focused metadata UI suite on all three platforms. CAgent::ReOpen
refreshes the new native handler's capabilities before source ReadItems, including
the first added ADS. Native Windows ADS output validates stream and bounded base
separately; ordinary colon paths remain refused. Standalone Windows ZIP/7z link
storage is refused before engine start because its directory reparse round-trip
loses metadata. Source-advertised native TAR/WIM capabilities remain authoritative.

P2-05: source LinkDialog/CApp::Link own selection, defaults, path/type/data
validation and original hardlink/reparse command semantics. CQtLinkWindow owns
only Qt controls, language bindings and scrolling. The partial source FM startup
initializes original Windows process privileges through exact MemoryLock.
Shared qtlinkservices belongs to sevenzip_filesystem: raw-link cloning and
no-copy rename reuse FileLink/FileDir operations, with a POSIX rename adapter
that lets callers handle EXDEV without reading target bytes. Source FSFolder
owns Windows raw reparse properties and no-follow deletion; FSFolderCopy owns
leaf-link copy/move, progress and native file/new-folder metadata preservation.
FileTransferOperation retains the joined worker and cancellation;
QtFileTransferTransaction owns staging, publication and rollback.
Destination links remain refused; source links are leaves, including dangling
and cyclic targets. POSIX edit stages a replacement in the same parent before
rename. Archive link storage/extraction and their security policy have separate
backend boundaries and are not enabled by the filesystem Link command.

P2-04: complete source Common/Bench owns generation, timing and rating
calculations in sevenzip_common. Source GUI/BenchmarkDialog owns properties,
memory refusal, native worker synchronization, callbacks, passes and results in
sevenzip_panel. CQtBenchmarkWindow provides only controls, layout, monotonic
clock, timer and queued-message services. ArchiveBackend supplies retained native
codec/hash providers and IQtBenchmarkServices; the production services delegate
to original Bench and SystemInfo GetRamSize. The source Common/CompressCall2
Benchmark entry connects Tools dispatch without global codec/window ownership.
Providers and services outlive the dialog's original worker join. There is no
CLI benchmark output parser or independently implemented rating formula.

P2-03: source PanelCrc owns panel checksum routing and filesystem scan/read;
PanelCopy/ExtractCallback hash the selected archive stream. MainWindow supplies
only the retained backend catalog provider and Qt operation lifetime. Backend
hash/hashMethods use an owned QtArchiveHashOperation thread around complete source
HashCalc; thread adapters forward progress/pause/cancel and never implement hashes.
HashCalc and its directory/link dependencies belong to sevenzip_common, HashGUI
to sevenzip_panel. Exact original digest registration remains the sole algorithm
owner. The complete source Hash reader/writer/verifier is attached to the catalog;
native manifest Test uses ArchiveBackend's typed extraction boundary after source
item-order verification and a local safe-read path guard. Catalog images outlive
all retained IHasher/handler instances. File-only engine streams can use a private
spool at the extraction boundary; application-level checksum staging is removed.

P2-02: ArchiveEngineLocation selects a deployed official dynamic `7z` client
when available, with an explicit engine-path override and standalone `7zz` fallback.
The runtime builder snapshots the pinned original Console and Format7zF makefiles;
neither the engine nor the sibling checkout is rewritten. ArchiveCodecProbe
transports the source Console `Libs` records and version, while CCodecs owns native
metadata/ABI checks and its original Codecs/Formats enumeration. Catalog native
libraries must match the engine deployment. Directory/NeedSetLibCodecs logic is
factored into an explicit-folder adapter. ArchiveCodecImage owns private runtime
copies for each confirmed dynamic catalog. Native handlers use the original
SetCodecs loop and per-instance fallback; child operations use that same runtime.
Discovery and archive jobs exclude one another. Actual native and engine
consumers of an external Formats module without a main DLL pass on macOS.
An external test facade delegates all archive methods to the unchanged official
module but withholds ISetCompressCodecsInfo. Exact-byte decoding through global
SetCodecs survives the creation and release of another confirmed catalog on
macOS. Implicit module selection transports the engine's Libs resolution;
explicit mismatched module requests still fail. Focused consumers pass on macOS,
Windows and Linux; this does not establish whole-app P3 acceptance.
Nested Codecs/Formats assets are copied with the runtime; the source
loader still enumerates only direct modules. A real external facade reads its
private resource after the original is removed. Cyclic directory links refuse
snapshot preparation instead of retaining links to mutable external directories.
Windows ancestry uses original FileIO directory handles and volume/file IDs
because junction aliases are not reliably identified by canonical pathname.
This does not relocate arbitrary dependencies installed outside the deployment.
Source load errors remain nonfatal when working modules exist and reach startup
status and original About's error branch. About reads its retained catalog;
GetCodecsErrorMessage is const-qualified with its functional body unchanged.
Original FileDir/FileName/FileFind/FileIO/TimeUtils and the Qt owned-temp
identity/registry services used by FileDir are foundation units in
sevenzip_common. FileStreams/LoadCodecs/source RAR handler consumers therefore
have the same dependency closure without depending on filesystem UI modules.
`archivebackend_completion.cpp` owns CLI terminal failure/cancellation, ordered
stage continuation and success signal delivery, using the same ArchiveBackend
state as the process transport. Mutation publication/restoration stays in
`archivebackend_mutations.cpp`; listing result projection stays beside its inverse
Console/List parser in `archivebackend_listing.cpp`. These private methods add
no second job, process or mutable archive owner.
MainWindow panel construction remains in mainwindow_panel.cpp; source-column
projection/persistence/context/time redraw belongs to mainwindow_columns.cpp.
mainwindow_refresh.cpp owns Qt refresh/reopen/change observation/viewport and
reload projection. Native CPanel still owns catalog, selection and reload guards;
the constructor registers the existing callback without retaining its workflow body.
`archivebackend_compression.cpp` validates public native compression requests and
projects the original method properties into CLI switches. Both dialog submission
and backend preflight delegate memory refusal to source CCompressDialog::OnOK.
`archivebackend_creation.cpp` owns creation planning and delete-input lifecycle;
`archivebackend_mutations.cpp` owns changes to an existing archive and publication.
`archivebackend_sourceplan.cpp` owns local source callback-plan validation,
KeepName output selection and asynchronous stage orchestration. Other individual
update/mutation transports remain in archivebackend_sourceupdate.cpp. Native
OpenArchive.cpp now retains the full upstream handler detection/parser/nested/
reopen implementation; QtOpenService remains an explicit optional process
integration branch. With a valid codec catalog, QtArchiveFolderManager invokes
ArchiveBackend::openNativeArchive in the source worker before process listing.
ArchiveNativeOpen owns the original CArchiveLink and its exact catalog lease;
the folder resource delegates archive-level properties to the native reader.
CAgent's IFolderArcProps methods enumerate that retained native link, including
its original levels, parent-item properties and final NonOpen diagnostics.
Format/error indices resolve against the reader's own catalog; replacing the
backend catalog cannot reinterpret them. The file-only operation facade retains
its separate index mapping; process-only readers keep the transport metadata
fallback. Native raw interfaces release before the resource's module lease.
For an accepted outer chain with a rejected inner file, process listing uses the
exact accepted handler chain in original ParseOpenTypes order instead of retrying
the failed inner detection. Original opener/format/stream algorithms are unchanged.
Its stable callback proxy detaches the opening UI after Open3, since source
volume streams can retain COpenCallbackImp. Resource/transport Close releases
native handlers and streams before source publication or private-file cleanup.
Item enumeration, update and reopen still use explicit 7zz integration services,
except the source ZIP comment writer. Rar5 metadata constructs the copied native
RAR5/RAR7 reader and registered decoder. After proving identical item index/path
order, the resource routes extraction/Test through ArchiveBackend to original
IInArchive::Extract and source Agent/FM memory callbacks. Native RAR extraction
uses source overwrite modes for root/batch and selected paths; a missing index
contract is an explicit refusal. Process transports do not handle its memory
decisions. Missing-module callers retain process-only read fallback.
Native nested streams retain their original open-type intent (explicit *:r for
empty native types) for process listing/extract/reopen. Process writer readiness
refuses volume sets/native nested chains and native read-only readers; the flags
survive Close so publication/reopen cannot silently enable an unsupported writer.
The installed gzip reader does not expose a seekable decoded stream: TGZ remains
at the gzip item level, as in the original OpenArchive contract.
New creation requests carry a stable native format name in CompressionOptions;
ArchiveBackend resolves/validates it against its immutable catalog and the
source UpdateGUI eligibility routine, then transports native properties to 7zz.
Ordinary Add enters original CPanel::AddToArchive and source CompressCall2/
ShowDialog/UpdateArchive with the selected filesystem paths and retained catalog.
The dialog has a fixed selection and no source picker. Updating a detected
unknown-extension archive uses original two-panel Copy.
CreateArchiveDialog projects eligible formats and original format control flags;
fixed censor paths supply naming inputs and parameter eligibility. Existing
archive update services use native UpdateEnabled capabilities. Single-stream
KeepName replacements use the original input and a separate output to preserve
source default-name identity. With a retained native writer and proven unique
item indexes, update passes the original callback directly to IOutArchive behind
ArchiveBackend. Exact CArc paths include source default names; ambiguous native
mapping makes the folder read-only for update. Source Sync checks bracket the
writer before the original work-dir publisher moves output. The native SFX
path first uses source CArchiveLink::ReOpen to retain its physical
prefix input and ArcStreamOffset; the original publisher copies that prefix and
writes through CTailOutStream. Explicit source ISetProperties forwards through
the backend and takes precedence over optional CInfo projection; the original
DoOperation reset and CommonUpdate no-reset behavior are preserved. The process-only
fallback still uses staged plans: CLI Add replaces referenced files, while Delete
handles only omitted items. A failed native writer never selects that fallback.
Copied mutation staging retains the physical archive basename in its own private
subdirectory, preserving source DefaultName for pathless handlers. Mutation
commands use the source exact archive-name mode (-sae), including extensionless
owned spools. Individual handler contracts determine refusal/empty-stream results.
Source provenance, baselines, adaptations and migration debt belong in the
[source-port audit](upstream-port-audit.md). Supported workflows and differences
from Windows 7zFM belong in the [parity record](7zfm-parity.md). Working rules
are defined in [AGENTS.md](../AGENTS.md).

P2-01: ArchiveBackend owns shared immutable ArchiveCodecCatalog snapshots.
The explicit module/version boundary loads native CCodecs and refuses missing,
incompatible or mismatched metadata. Source owners handle aliases, update flags
and open-type parsing; Qt only projects metadata for consumers. Native objects
must retain their catalog until they are released, including across a failed
replacement. ArchiveCodecProbe asynchronously obtains the installed 7zz version,
finds its official module beside the app/engine or installation library directory,
and checks exact version agreement before queued publication. It owns timeout,
failure and cancellation; no shell or Formats-table guessing is used. The default
MainWindow starts discovery; injected backends retain caller-owned configuration.
The Open dialog projects native extensions, falling back to All files when absent.
Source archive folders retain a metadata lease and use native default-name rules;
menu and transfer readiness follows source read-only state rather than suffixes.
Transactional update transport supports the seven checked native writer formats. With no catalog
loaded the existing typed transport works; an explicitly failed catalog refuses
update readiness. CreateArchiveDialog holds a catalog lease; its formats component
projects native eligible transports and configures source CompressDialog naming
and history indices. Existing archive type is passed from actual listing metadata;
failed metadata disables submission. Source UpdateGUI prepares one-file/name/format
eligibility from native censor paths. Native CreateArchiveName supplies initial suggestions for
metadata-configured callers. Complete writable-format transports and final live
command/error consumers remain P2-01 work.

Native PanelDrag owns source selection, effects, destination/transfer handshake,
source-versus-target CopyTo/CopyFrom/Add execution and completion. CApp owns its
drop target and Begin/End focus state. QtFileManagerDrag owns view hit testing,
highlight, modal presentation and input/event delivery. Its CompressFiles service
passes original PanelDrag arguments to source CompressCall2/UpdateGUI and native
backend update factories, retaining the catalog throughout the modal worker.
It attaches newly created panels as well as initial ones.
QtPanelDragTransport exposes real local-file URLs after native source extraction;
the common temp registry retains unconfirmed external lifetimes. Widgets do not
parse archive commands or implement archive transfers.

Native HelpUtils owns topic construction and source dialog/page callbacks own
their context topics. Qt help services in sevenzip_panel render bundled original
HTML/CSS with internal navigation; explicit web clicks use QDesktopServices.
Native AboutDialog owns source version/date/homepage dispatch; Qt supplies its
modal controls and the separate CMake application-version/license presentation.
Main Help/About and Delete Temporary Files enter through CApp/MyLoadMenu
capability routing; cleanup is modal and available while navigation is allowed.

Core qtownedtempregistry owns creation records and active-session
leases, including durable protection for unconfirmed external lifetimes. Native
FileDir temp owners and copied CTempFileInfo/nested/process records retain those
leases; Qt temp/spool services use the same owner. The registry offers an inactive
object claim with identity revalidation; it does not implement deletion. Native
BrowseDialog2 owns native enumeration, aggregate/props/sort/navigation and
confirmation/per-item deletion/first-failure behavior. Qt presentation renders
its controls and columns; claims protect owned roots including nested navigation.
FM ShowDots/SingleClick/FullRow/Grid are projected into the tree and paint adapter;
QtFileListHover has one shared implementation in sevenzip_panel for both views.
When source CommonUpdateOperation publishes over a private stream spool,
QtArchiveFileResource retains the replacement CTempFile lease after the native
move, before Finish/reopen. The held same-object record follows the move; a
cross-volume copy gets a new identity record only for an already owned spool.
Publication of user archives never grants cleanup ownership.
Linux object identity includes statx birth time when available, distinguishing
an immediately reused inode after a retained edit is manually deleted. The
source FileDir temp naming/creation algorithm remains unchanged. Filesystems
without STATX_BTIME retain the device/inode/type fallback.

Native CApp::ExecuteCommand is the common entry for current menus, toolbar,
shared context actions, panel activation and keyboard commands. MyLoadMenu owns
the copied dispatch and CFileMenu capability policy. QtFileManagerCommands binds
actions and projects one capability snapshot per refresh; every execution takes
a fresh snapshot before effects. It adds only current transport restrictions and
delegates operations to existing owners. QAction enabled state is presentation.
The panel context popup feeds resource-derived File actions into native
`CPanel::CreateFileMenu`/`CFileMenu::Load` with `programMenu=false`; Qt renders
the returned order and enabled state. The P2-09 popup additionally uses the
source shell tree through QtFileManagerShellMenu. On Windows, QtNativeShellMenu
retains the source-created System IContextMenu/HMENU for one generation and
renders the generated tree; CPanel owns PIDL resolution and the original numeric
invocation body. The shared dispatch rejects stale QAction generations. Native
HMENU presentation is isolated in QtNativeShellMenuPresentation: it copies GDI
item/checkmark pixels into owned Qt icons and retains the provider through lazy
submenu initialization. Generation and invocation remain in the source owners.
Native IContextMenu2/3 measure/draw and printable mnemonic messages feed the
Qt presentation boundary, including callback icons and owner-drawn submenus.
Provider state remains alive through row widgets and message filters. GDI
pixels are copied into owned images before native handles are released;
failed measurement disables the affected unrenderable row. Command generation
and invocation remain in CPanel. Incomplete archive shell transports stay disabled.
ZIP item comments follow source CPanel::ChangeComment, Agent::CommentItem and
CAgentFolder::SetProperty. The shared source worker/callback invokes the source
ZIP IOutArchive behind ArchiveBackend::updateNativeComment; widgets never build
archive commands. sevenzip_zip contains the exact original handler and codec/
crypto/stream closure. ArchiveCodecCatalog retains module discovery metadata and
replaces only ZIP factory hooks with this source handler, passing codecs per
instance and retaining the immutable lease. An empty g_ExternalCodecs ABI fallback
is owned by archiveexternalcodecs.cpp, not a mutable global runtime catalog.
The process-backed item proxy can expose native properties and comment writes
only after exact count/order/path validation; mismatches disable the capability.
CommonUpdate closes readers, publishes via the existing work-dir transaction and
reopens/validates the new native reader. Owned nested spools reuse the existing
parent writeback chain. Native callbacks pause/stop through source ProgressSync;
process-backed reopen still uses the shared external transport controls.
Native CApp owns toolbar flags, persistence ordering and focused-folder title;
QtAppPresentation supplies toolbar reload/layout services. Dialog.cpp retains
the source dialog-unit size predicate with Qt font/work-area services. Complete
model, two-panel, mutation and shell workflows retain their separate plan owners.
For virtual root navigation, CRootFolder and CPanel own names, item binding and
parent resolution. `mainwindow_root.cpp` only renders the bound `IFolderFolder`
into Qt cells with source indices; the address is empty at the native root.
On macOS/Linux the root also binds a `CFSDrives` mount view. `qtvolumemounts`
supplies OS mount metadata; `CFSDrives` retains source property/binding ownership
and rechecks a mount before entering it. The Qt list only presents those items.
`BrowseDialog.cpp` owns the source file/folder picker entry points and result
state, including its original private CBrowseDialog navigation/filter/sort/create
and acceptance bodies. `QFileDialog` is the ordinary native picker service;
the source browser handles long initial paths and Windows super/device prefixes.
Qt rows delegate sorting to the source comparator. Portable strings/filter
vectors remain at the dialog boundary; widgets convert only for presentation.
Copy, Split, Extract,
Create Archive and Open Archive call those entries instead of opening their own
picker implementations.
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
Ordinary folders preserve immediate loading up to 256 source items. The original
CFSFolder callback aborts the bounded probe above that count; the same LoadItems
body then runs in the existing worker with flat mode disabled, retaining the
bound provider. Refresh does not replace the filesystem provider or its native
notification owner. Each pane can publish its own scan after Copy/Move; another
pane's scan does not suppress that reload. QtFolderProjection
retains the native archive folder while presenting at most 32 rows or 8ms per
GUI turn, including property/icon creation. Navigation and close invalidate
pending turns before releasing owners. Scan operation state disables commands
until the shared table/icon model is coherent. Native selection snapshots and
post-operation path selection wait for completion and survive panel switches.
Sorting is enabled once with the desired source property/order; its comparator
remains CPanel::CompareItemIndices.
Disk and archive projection preserve RefreshListCtrl's redraw suppression until
sorting and selection finish, using Qt widget update control across batches.
The synchronous source guards end before an idle operation acquires retained
scan guards; otherwise a flat-view change would retain disabled notification
flags after completion. Widget updates resume after final sorting and selection.
Each panel retains its own FilesystemScanner, QtFolderProjection, scan flags,
selection snapshot and viewport state. Changing focus switches the presentation
fields without canceling that panel's publication. Worker signals and archive
batches project into their captured panel, then restore the focused panel's
fields, actions and status. Scan guards cover only panels with pending rows;
Open/Operation still covers both. A synchronous Qt focus change while disabling
a busy list cannot retarget the caller's remaining scan setup: the address in
that same panel receives focus and the transition retains its source context.
Folder synchronization cancels only the destination's previous publication
before rebinding; close cancels and joins both scanners. View-mode switching retains the
captured source panel and control pointers across synchronous Qt focus events;
the new view restores list focus just as upstream changes the same ListView's
style. Normal Windows folders use the source change-notification watcher instead
of duplicate per-file Qt signature reads. A root signature keeps missing-folder
recovery; flat descendants retain the nonrecursive watch's portable fallback.
Open/Operation disables both panel address combos along with their lists,
matching the source modal progress boundary. Otherwise Qt can focus an inactive
address while source guards suppress its panel-focus notification, leaving
presentation focus and command ownership inconsistent after completion. Scan
keeps address navigation available; ending the blocking operation restores it.
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

GUI early errors retain Explorer/MyMessages' process question policy.
ShowErrorMessageQt in that source owner adapts QWidget/QMessageBox Critical/OK
presentation while reading the existing g_DisableUserQuestions flag. UpdateGUI's
missing-writer branch and both ExtractGUI path failures use it; Qt wrappers do
not independently bypass suppression. Original HRESULT/messageWasDisplayed and
worker-entry ordering remain with the source GUI routines.

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
stack. Its duplicate ArchiveParent/temp stack is removed.
P1-09 materializes source-owned nested memory streams at the CTempFileInfo
parent-link path through IQtArchiveStreamOpenServices inside the existing open
worker. CTempFileInfo owns cleanup; the file-only engine resource retains only
the path. Successful materialization captures source FileInfo and clears IsVirtual.
Failed probes remove their owned copy before source FlushToDisk fallback. Source
OpenParentArchiveFolder/OnOpenItemChanged publishes each changed level on return;
IsThereReadOnlyFolder checks the complete parent chain before mutation.
The presenter reads the
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
QtAppPresentation attaches the reload service with each panel's controls, before
source OnCreate calls RefreshListCtrl. Initial publication therefore starts
before the next panel binds; the constructor and panel toggle do not add another
render. An uncreated panel may start publication while its peer is scanning;
existing panels still defer reload during active operations. OnCreate keeps the
original ignored Refresh HRESULT policy. A large filesystem scan publishes its
worker's separately loaded provider; archive row projection keeps its provider.
Native archive publication captures CPanel's scoped OnReload result continuation;
initial creation and OpenAsArc_Index have no error continuation. The row owner
discards cancelled/replaced continuations, and completes sorting, guards and
controls before delivering a deferred HRESULT. Projection depth prevents Copy
completion from preceding error reporting. OpenAsArc_Index releases its native
binding guards before Refresh takes the guards retained by asynchronous rows.
The filesystem OpenItem caller releases its outer guards at the same handoff,
and reacquires timer processing suppression for unsuccessful-open fallback.
CApp::Create captures its original main-panel index separately from the live
focus/projection used by Qt construction and asynchronous callbacks. This keeps an ordinary startup's
requested main path distinct from its saved path, including a right main panel.
QtAppPresentation completes initial focus once the visible window's scans finish;
explicit list/address input or a new public archive request cancels that pending
focus so it cannot override subsequent navigation.
Only a still-scanning initial target needs deferred focus. Synchronous Create
has already focused its list, so later peer creation has no redundant startup
focus that could change projection inside its OnCreate.
Public archive opening also restores the selected address control after modal
progress, using the existing weak-widget focus boundary of external updates.
P0-08.1 projects each panel's Qt controls, location, view and sort through
`mainwindow_twopanel.cpp`; QSplitter replaces Win32 `MoveSubWindows` geometry.
Panel controls are inserted at their source index, preserving panel0 left/panel1
right even when a main archive or initially single right panel is created first.
The two CPanel instances remain authoritative for folder and selection.
Per-panel asynchronous publication follows the lifetime described above and
survives focus transfer. Disk auto-refresh polls both visible panels,
and command availability uses only the active projection. P0-08.2 extends this
projection to mixed disk/archive and archive/archive, restores the CWindowInfo
panel count/focus and QSplitter coordinate, and uses the opposite
CPanel::GetFsPath as the Copy/Move/Extract dialog default. An uncreated hidden
panel keeps its persisted path across close. Full cross-panel transfer remains
P1-01.
`OnCopy` retains source/destination panel references across the modal. Its Qt
consumer restores the captured source's projection before executing Copy/Move:
native Windows modal focus restoration can activate the opposite pane without
changing the original request. The close guard still prevents a released owner
from consuming it.
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
correction.
OnEndLabelEdit retains the source timer guard from handler entry and notify guard
around its Rename worker. Nested GUI events cannot reload the provider or query
it through enabled notifications during the operation. RAII restores the prior
guard state on success/error/cancellation, including an enclosing operation guard.
The Qt editor and completion live in mainwindow_rename.cpp, registered with the
shared widgets object owner. The source handler saves selection before Rename,
sets the new/old focused name and passes that state through the existing pending
refresh channel. Completion loads the bound provider and restores only after
successful publication, including deferred batches. `_dontShowMode` suppresses
callback DisplayRole text, including cached Qt cells, between the operation and
the load/restoration phase; raw roles and editor values remain independent.
Early validation rejection resets the Qt editor without loading and retains
selection, adapting the native FALSE edit result. The source error/Cancel policy
is preserved: CProgressDialog owns completed-worker errors, while the Qt boundary
shows a generic rejection only when no pending refresh state exists. The posted
reload ignores its HRESULT as upstream does; explicit Refresh owns recovery
diagnostics. Native F2 writer/publication/retry evidence is in the port audit;
remaining native ADS/platform acceptance stays separate.
The periodic Qt timer visits both pane projections, unlike native WM_TIMER's
direct CPanel::OnTimer call. It checks visible source notification/status guards
before projection activation, and refreshActions avoids status reads under that
guard. This prevents GUI status from reading an archive proxy during native
worker reopen; the original source guards remain the authoritative state.
Filesystem Delete now follows the mirrored PanelOperations.cpp source
confirmation and worker. `IQtTrashService` is the platform boundary for
ordinary Delete; Shift+Delete uses source `IFolderOperations::Delete` and the
ported progress callback. The independent application delete worker is gone;
its deterministic FSFolder callback driver lives only in `tests/`.
Source IsItem_Folder owns single-file eligibility. ArchiveBackend retains all
operation transport and temporary-file handling. Remaining filesystem consumers,
complete external-open and command/navigation acceptance remain mandatory.

ArchiveBackend::cancelAndWait is the transport quiescence boundary for destruction:
stop process writes before a window releases temporary source/destination paths.
MainWindow disconnects backend/scanner/file/volume callbacks before invoking it; backend
completion retains operation-specific cancellation cleanup. Ordinary cancel remains
asynchronous. Source CApp/panel/parent-stack release is connected; complete
operation-state and window-close consumers passed P0-02.3 acceptance.
MainWindow completion receives an explicit HRESULT from each operation owner:
backend signals use lastResult(), source batch/PanelCopy use their own returned
result, and ArchiveProgressOperation uses the joined worker Result after a
successful modal Create. E_ABORT suppresses error presentation independently
of translated message text. The modal Create result alone does not represent
the worker's operation result.

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
QtMemoryLimitSpin adapts only
edit/arrows. Backend transports `-slp` to the Windows engine process. CFoldersPage
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
Window Position serialization is complete, but its omitted source InitInstance
startup/normal/maximized consumer remains mandatory P3-05.16.
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
