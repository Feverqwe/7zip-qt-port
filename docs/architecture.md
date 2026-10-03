# Architecture

This document describes component ownership, data flow and integration boundaries.

P2-05: source LinkDialog/CApp::Link own selection, defaults, path/type/data
validation and original hardlink/reparse command semantics. CQtLinkWindow owns
only Qt controls, language bindings and scrolling. The partial source FM startup
initializes original Windows process privileges through exact MemoryLock.
Shared qtlinkservices belongs to sevenzip_filesystem: raw-link cloning and
no-copy rename reuse FileLink/FileDir operations, with a POSIX rename adapter
that lets callers handle EXDEV without reading target bytes. Source FSFolder
owns Windows raw reparse properties and no-follow deletion; FSFolderCopy owns
leaf-link copy/move, progress and native file/new-folder metadata preservation.
FileTransferOperation retains staging, publication, cancellation and rollback.
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
`archivebackend_sourceplan.cpp` owns local source callback-plan validation,
KeepName output selection and asynchronous stage orchestration. Other individual
update/mutation transports remain in archivebackend_sourceupdate.cpp. Native
OpenArchive.cpp now retains the full upstream handler detection/parser/nested/
reopen implementation; QtOpenService remains an explicit optional process
integration branch. With a valid codec catalog, QtArchiveFolderManager invokes
ArchiveBackend::openNativeArchive in the source worker before process listing.
ArchiveNativeOpen owns the original CArchiveLink and its exact catalog lease;
the folder resource delegates archive-level properties to the native reader.
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
An existing Add target retains its detected format across empty/ineligible
source-picker states and restores the source parameter controls when eligible.
CreateArchiveDialog projects eligible formats and original format control flags;
changing sources refreshes both naming inputs and parameter controls. Existing
archive update services use native UpdateEnabled capabilities. Single-stream
KeepName replacements use the original input and a separate output to preserve
source default-name identity. Other callback plans use staged publication; CLI
Add replaces referenced files, while Delete handles only omitted items.
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
eligibility from native censor paths; additions/removals in the Qt source picker
refresh those inputs. Native CreateArchiveName supplies initial suggestions for
metadata-configured callers. Complete writable-format transports and final live
command/error consumers remain P2-01 work.

Native PanelDrag owns source selection, effects, destination/transfer handshake,
source-versus-target CopyTo/CopyFrom/Add execution and completion. CApp owns its
drop target and Begin/End focus state. QtFileManagerDrag owns view hit testing,
highlight, modal presentation, input/event delivery and the existing backend's
Add progress consumer. It attaches newly created panels as well as initial ones.
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
state; `QFileDialog` is its platform presentation service. Copy, Split, Extract,
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
correction.
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

`QtFmOptionsDialog` in mirrored OptionsDialog.cpp supplies the partial native
six-page property sheet. Folders, Editor, Settings, 7-Zip and Language are available;
System remains disabled. CMenuPage preserves source list/state/apply bodies;
only Zone propagation has a current consumer, through PanelCopy, Agent and the
Windows ArchiveExtractCallback. Shell controls are disabled until P2-09 ports
QueryContextMenu and registration. They do not alter the FM File/context menu.
CEditPage retains source modified-state and Browse/OnApply,
while CSettingsPage can be embedded or shown standalone. Apply/OK commit source
FM INI values; Cancel discards only pending changes. Source CSettingsPage owns RAM
limit validation and capability-gated large pages; QtMemoryLimitSpin adapts only
edit/arrows. Backend transports `-slp` to the Windows engine process. CFoldersPage
owns NWorkDir::CInfo; source WorkDir in sevenzip_filesystem serves both FM update
temporary files and the UpdateGUI Create working-dir block, transported via `-w`.
`settingsdialog.h` aliases the container. `MainWindow` applies committed list flags
and source RefreshAllPanels immediately and supplies
filesystem icons through `QFileIconProvider`. `PropertiesDialog` supplies the
current operated indices to its bound `CPanel::Properties`, which owns enumeration
and totals through the source folder interface.
CLangPage owns the source language list/info/apply state. LangUtils is the native
CLang lookup/reload owner, with source registry helpers on the explicit FM INI
adapter. Immutable English fallback comes from source STRINGTABLE/en.ttt; bundled
assets use pinned official release provenance and native-file staging. Language
loading, LangUtils and ResourceString belong to Core-only sevenzip_filesystem
alongside CLang and file I/O, so filesystem/transaction workers can resolve
local message context without linking Widgets. Qt widget bindings remain in
sevenzip_panel. FileTransferOperation publishes a typed completion kind before
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
QtBindResourceText preserves the native static-control RC fallback independently
of LangString's dynamic-message fallback. Language bindings update presentation
without reinitializing inputs; composite CPU/RAM rows use Qt focus proxies to
their real combo controls. Original ambiguous Add mnemonics retain Qt platform
behavior and unconditional Tab access. No native parser/lookup API was changed
to support empty captions: source language blank lines skip IDs.
`CListViewDialog` presents the result through the shared Qt list adapter;
the dialog never reads the file table or backend itself.
`CListViewDialog` reads CFmSettings for single-click activation and delegates
hover selection to the Qt CListView adapter. Its persistent hover index and
owned timer prevent access to deleted rows; hovering does not confirm a path.

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
daemon/session separation can prevent discovery. The built-in text dialog
has explicit extra menu actions and remains independent of native F3/F4.
`FmSettingsPath` supplies the shared INI location outside MainWindow; callers
may supply an explicit path to history dialogs.
`CEditDialog` presents full list values; `FileViewerDialog` owns file viewing/editing.

`CApp::VerCtrl` owns source file comparison, numbered backup history and
Edit/Commit/Revert/Diff. `QtVerCtrlFileServices` projects unavailable POSIX Win32
file metadata/methods through upstream FileIO/time conversion; Windows uses
original types directly. Source `CFileMenu` determines dynamic VerCtrl command
visibility and names from Diff plus `FM/7vc` and selected file attributes. Qt
binds those native IDs and refreshes selection/availability after execution.
The source advanced `7vc` setting has no setup UI; it remains an INI key.

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

`FileViewerDialog` reads bounded UTF-8 text and saves through `QSaveFile` after
checking for external changes. `MainWindow` owns extraction and the subsequent
archive-update request, keeping the temporary source alive until completion.

## Filesystem and volume workers

`sevenzip_filesystem` owns `CFSFolder` mutations and the filesystem path snapshot.
`FileDir.cpp` owns recursive removal without traversing symbolic links.
The mirrored `CPanel::DeleteItems` owns confirmation and source progress-worker
lifecycle for permanent deletion. `IQtTrashService` delegates ordinary Delete
to the desktop's Trash/Recycle Bin; Qt refreshes and restores source selection
after each result. A test-only callback driver remains in `tests/`.

`FileTransferOperation` owns copying/moving on a worker and the transaction
wrapper that stages, publishes and rolls back targets. `CFSFolder::CopyTo`,
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
temporary folder and target `CopyFrom`; Qt owns F6 delete ordering, byte
rollback and both-panel refresh. `ArchiveBackend` restores per-archive listed
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

`tests/layout_inspection.h/.cpp` inspects public widget geometry and writes
JSON without editable field values. It is not linked into the application.
See [layout contracts](ui-layout-contracts.md) and [testing](testing.md) for
inspection procedures and coverage; [UI reference tooling](ui-reference.md)
defines resource extraction and structural comparison.
