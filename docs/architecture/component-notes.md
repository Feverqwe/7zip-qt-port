# Component ownership and lifetime notes

[Индекс](../architecture.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

PanelDrag owns the original source/target transfer decisions. Windows real drag
executes original IDataObject/IDropSource/DoDragDrop with shared source names and
owned-temp lifetime; the source panel detaches when OnDrag returns. Its incoming
application-owned Qt converter retains native IDataObject through public QWindowsMimeConverter
and the original Shell HDROP/IDLIST parser. SetTargetFolder and transfer wire
records remain in PanelDrag, rather than serialized Qt pointers. The converter
exports no private token format. One QApplication registration serves all windows;
offscreen platforms do not instantiate the Windows converter. Injected Qt
executors, headless platforms and POSIX URL transport
retain the existing portable seam. The complete
[body map](../omitted-scope-audit.md#paneldrag-protocol-body-disposition-2026-10-10)
records native ownership and omitted legacy/debug scope.

This document describes component ownership, data flow and integration boundaries.

FM.cpp owns the process-lifetime RAM snapshot through GetRamInfo, an immutable
record replacing the original two write-once globals. RunSevenzipFileManager
initializes it first, before platform/language/parser services. PanelItemOpen
consumes that record with the retained source limit/fallback and existing shift
guard. Direct source-panel callers initialize the same record on first use.
The platform query remains original NSystem::GetRamSize; ArchiveBackend retains
operation/stream-materialization ownership.

Original App.cpp::SetButtonText owns toolbar caption/tooltip lookup by native
command ID. MainWindow transports that text into QToolButton after language
reload and QAction changes; shared menu QAction captions remain menu-owned.
Qt owns tooltip storage and its platform context owns native Windows OLE.
FM.cpp's complete body/platform dispatch map is in the
[omitted-scope audit](../omitted-scope-audit.md#fm-desktop-bodies-and-platform-dispatch-2026-10-10).

CApp::Create obtains source small-screen capability from IQtAppPresentation
before ReadToolbar. QtAppPresentation uses existing QtDialogScreenServices and
source IsDialogSizeOK(200,200); ReadToolbar owns default/saved-mask policy.
RunSevenzipFileManager calls retained FM symbolic-link initialization after
SetMemoryLock, before archive/filesystem consumers. Link's lazy call stays
idempotent. Source MemoryLock owns token adjustment; link operations and failure
cleanup retain their existing owners.

RunSevenzipFileManager carries Windows STARTUPINFO's launch show command in
QtFileManagerStartup before any progress window. MainWindow passes it to source
InitInstance, whose saved-maximized branch owns precedence; the Qt window
service maps placement/state/no-activation. Hide suppresses the caller's show.
Accepted hidden close queues a Qt automatic-quit completion: lastWindowClosed
requires previous visibility, while source WM_DESTROY posts quit regardless.
QEventLoopLocker uses Qt's visible-window/quit-lock policy, retaining visible
association peers. Modal/live-call refusal stays in source WndProc/services.

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
[real-provider workflow](../testing.md) checks these existing boundaries without
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
captured source projection first; ReleaseApp discards the continuation and its
saved-state publication callback. Ordinary
two-panel filesystem Copy preserves source rows/current item, while Move,
copyToSame and a shared single panel keep their required source publication.
The existing providers still own row loading; RefreshListCtrl invokes the
attached load/publication adapter before its extracted source selection tail.
Qt render completion calls RestoreLoadedSelection directly to avoid recursive
reload. Source requests retain their saved state while operation guards defer
publication; the actual renderer consumes it. The Copy continuation runs after
its existing Qt publication and uses the same extracted selection tail without
retrying an already completed load. CApp's RefreshCopyPanel retains the source
RefreshListCtrl caller and its ignored HRESULT, including asynchronous filesystem
load failures. Qt tracks required source publication for move/copyToSame even
when execution fails or is canceled. Explicit OnReload keeps its own diagnostics;
actual writer and rollback/reopen errors retain their existing owners.
The synchronous native stream/device/ADS route retains pending publication
through writer-result presentation. It cannot mark a blocked reload completed:
ExecuteCopy first ends its operation guards, then its existing completion owner
loads the required source and destination panels.
Filesystem-to-archive Copy/Move saves destination state before CopyFrom and
publishes that same bound provider through RefreshListCtrl after success.
ArchiveFolderOut already owns engine reopening and proxy restoration; the Qt
completion no longer opens another provider. Failure/Cancel retain the existing
unconditional destination RefreshListCtrl and single-use source continuation.
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
destination refresh, avoiding duplicate publication. RefreshListCtrl uses the existing
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
The char16_t boundary delegates casing to original MyCharUpper(wchar_t), using
its native platform policy rather than Qt's Unicode table. It preserves source
numeric-run comparison and is shared by Agent, filesystem and panel fallback
comparators; no second casing or natural-name algorithm exists in these views.
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

Core sections: [repository boundary](../architecture.md#repository-boundary),
[window/panel state](../architecture.md#window-and-panel-state),
[dialogs/preferences](../architecture.md#dialogs-and-preferences),
[filesystem/volume workers](../architecture.md#filesystem-and-volume-workers),
[archive backend](../architecture.md#archive-backend-and-safety), [diagnostics](../architecture.md#diagnostics).
The port-specific entries below record ownership changes at their stated scope;
current package status belongs in [the checklist](../porting-checklist.md), and
dated source/validation evidence in [the audit](../upstream-port-audit.md).

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
[the mandatory UX audit](../ux-port-audit.md) tracks it and other independent
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
[source-port audit](../upstream-port-audit.md). Supported workflows and differences
from Windows 7zFM belong in the [parity record](../7zfm-parity.md). Working rules
are defined in [AGENTS.md](../../AGENTS.md).

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
confirmation/per-item deletion/first-failure behavior. Qt presentation delegates
keyboard policy to source OnKeyDown, including Ctrl+R and Ctrl+F3/F5/F6, before
its Enter/plain-F5 event adaptations. It renders controls and columns; claims
protect owned roots including nested navigation.
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
On Windows, the default association adapter calls original BrowseDialog2's
StartApplication on the GUI thread. Its PIDL/ShellExecuteEx body returns the
original CProcess, which QtNativeProcessServices transfers into its existing
waitable handle owner. MainWindow preserves an empty optional opener so this
native default remains reachable. Explicit QUrl openers and POSIX associations
retain the no-handle service. PanelItemOpen rejects a launch failure only when
no handle was returned, as upstream does; known handles enter the existing
child-process/update worker even if the launch HRESULT reports failure.
The no-handle branch waits on a panel-scoped CExitEventLauncher and joins before
panel member release. Unchanged files are deleted at owner exit, changed files
are retained. The temporary browser receives the same optional opener from its
window; its native default closes the source process handle without waiting and
retains external protection until completion can be established elsewhere.
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
