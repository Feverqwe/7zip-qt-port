# Omitted source scope review

This review records module bodies and caller/dependency boundaries for P3-05.
It does not establish whole-module, platform or final acceptance from filenames.
The historical [P0-01 registry](history/p0-01-source-inventory.md) remains intact;
later functional ownership is recorded in the [port audit](upstream-port-audit.md).

Source: [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
full revision `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
The maintained per-module [manifest](upstream-baseline.json) owns reproducible
local provenance; this document does not replace its exact/adapted distinction.

## Source Create File/Folder workflows restored on 2026-10-10

Source RenameFile and OnBeginLabelEdit activation is now retained in the same
module: focus chooses the target independently of operated selection, and Qt's
EditLabel callback is only a control transport. Source policy no longer disables
Rename solely for multiple marks. This does not close remaining PanelOperations
bodies or FileManager family disposition; see the
[activation receipt](upstream-port-audit.md#p3-05--source-rename-activation-scenario-2026-10-10).

The previous PanelOperations helpers kept validation/operation fragments while
MainWindow supplied independent dialogs/errors/selection. Complete source
CreateFile/CreateFolder bodies now own those phases and refresh policy in their
original module, using the existing Qt reload callback. CreateFile's original
direct provider call replaces the extra worker branch. Both normalized bodies
match after dialog boundary substitutions; see the [source receipt](upstream-port-audit.md#p3-05--source-create-filefolder-scenario-2026-10-10).
The deferred Qt scan handoff is integration code because original refresh is
synchronous. It transfers the restored native processing snapshot after Create
returns; it does not replace the source guards or creation algorithms.
Remaining PanelOperations bodies and FileManager family disposition stay open.

## Source Delete lifecycle reconciliation, 2026-10-10

Delete's internal confirmation/worker/refresh phases now remain in the original
PanelOperations owner. No/Cancel exits before refresh; accepted error/Cancel
refreshes inside the source notification scope. MainWindow's independent
filesystem/archive completion and duplicate error path are removed. Existing
QFile trash presentation and explicit-index archive Move transaction seams are
adaptations with concrete platform/transport reasons, not new deletion bodies.
See the [source receipt](upstream-port-audit.md#p3-05--source-delete-lifecycle-2026-10-10).
The bounded checks close the identified Delete lifecycle substitution;
remaining PanelOperations/FileManager full disposition stays open.

## Split/Combine owner and native workers restored on 2026-10-08

Existing mirrored names concealed absent CApp::Split/Combine, substituted native
types/NIO and omitted CPreAllocOutFile. The complete scenario now uses original
application bodies and worker algorithms with narrow Qt progress, I/O API and
publication adapters. Original discovery/summary/output-name blocks have one
shared implementation. The [source receipt](upstream-port-audit.md#p3-05--complete-source-splitcombine-scenario-2026-10-08)
supersedes earlier claims that native preallocation and source discovery were
unusable. This closes those omissions, without closing the FileManager family.

## Filesystem comparator restored on 2026-10-08

Existing FSFolder.cpp/h names concealed an omitted IFolderCompare interface,
QI and CompareItems body. They now retain the source switch and cache phases;
the panel consumes that interface through its existing binding/comparison path.
The authoritative PanelSort.cpp natural-name helper is shared by selective
compilation, avoiding a filesystem/panel cycle. Portable attributes/time services
replace only their Win32 representations. See the [body, dependency and check
receipt](upstream-port-audit.md#p3-05--source-filesystem-comparator-2026-10-08).
This closes that omission, without claiming whole FileManager disposition.

## Missing UI translation units reviewed on 2026-10-05

The original FileManager/Agent/GUI/Common directories contain 67/7/10/25 `.cpp`
files, totaling 109. Current source and vendor paths cover 103 of those names.
The six missing paths below were reviewed in full with their headers and direct
callers. This physical inventory is not a functional coverage measure: remaining
103 bodies, branches and resources still require the P3-05 reconciliation.

| Original path under `CPP/7zip/UI/` | Body and callers | Disposition and dependency boundary |
| --- | --- | --- |
| `FileManager/ProgramLocation.cpp` | Only includes `StdAfx.h`; `ProgramLocation.h` has an empty include guard. FM.mak and GUI/makefile name the object. | No active implementation to port at this revision. Module discovery is supplied by the actual source `Windows/DLL.cpp` owner, not an invented ProgramLocation implementation. |
| `FileManager/RegistryPlugins.cpp` | All old plugin lookup/list bodies are inside comments. Header plugin types/declarations are commented too. | No active plugin registry behavior at this revision. Actual folder/codec discovery belongs to `FilePlugins`, `LoadCodecs` and their accepted consumers; the commented legacy code does not replace those dependencies. |
| `FileManager/StdAfx.cpp` | Only includes `StdAfx.h`. Header includes portable Common.h plus compiler warning policy and explanatory SDK comments. | Upstream precompiled-header build transport, with no runtime body. CMake compiles each source with its required headers/definitions; exact StdAfx/Common/Precomp dependencies remain in the manifest. No claim that compiler macros or platform definitions are unnecessary. |
| `GUI/StdAfx.cpp` | Only includes `StdAfx.h`, whose active contents are warning policy and FileManager/StdAfx.h include. | Same build-transport boundary. GUI.cpp now has production consumers on all supported desktop platforms; omitting this empty PCH translation unit does not omit the GUI dispatcher. |
| `FileManager/EnumFormatEtc.cpp` | `CEnumFormatEtc` owns Next/Skip/Reset/Clone and target-device copies. `PanelDrag.cpp::CDataObject::EnumFormatEtc` supplies one format for DATADIR_GET through `CreateEnumFormatEtc`. | Active Win32 presentation transport, not inactive code. Current source PanelDrag owns data/lifetime/drop policy; `QtPanelDragMime` supplies URLs and `QtPanelDragTransport` supplies QDrag. MIME/native format enumeration is delegated to Qt platform transport, without a second project enumerator. Native receiver proof remains P3-03/P3-06, including blocked Finder evidence; synthetic MIME tests do not close it. |
| `FileManager/ProgressDialog.cpp` | Active legacy CProgressSync pause/stop, progress range/percent, timer, cancellation prompt, external-close/event and thread-join bodies. FM.mak and GUI/makefile instead select ProgressDialog2. SFXSetup/makefile and resource.rc select this legacy dialog. | Explicit installer-bundle boundary: SFXSetup produces `7zS.sfx`, with its separate installer/configuration workflow. The accepted SFX builders select unchanged SFXWin `7z.sfx` on Windows and SFXCon on Unix; SFXWin/resource.rc uses ProgressDialog2. Legacy implementation is not declared inactive or replaced with a homemade equivalent. Custom installer `7zS.sfx` is outside these selected bundles; a future installer feature must reuse its original bodies/dependencies. |

The legacy review includes `ProgressDialog.h`, `ProgressDialogRes.h`, `ProgressDialog.rc` and the
SFXSetup caller/build map. Its class name overlaps the modern progress class;
copying both into one library would create two implementations of the same
symbols. Existing FM/GUI presentation and progress tests apply to ProgressDialog2,
and the original SFX bundle compiles its own selected engine/application sources.

## ClassDefs and native GUID dependencies reviewed on 2026-10-05

Original `FileManager/ClassDefs.cpp` has no runtime algorithm: after StdAfx and
MyWindows it includes MyInitGuid before Agent.h, MyWindowsNew.h and
Explorer/MyExplorerCommand.h. This order selects GUID definitions rather than
external declarations. The adapted upstream-relative ClassDefs remains the
single portable interface owner in `sevenzip_common`: IArchive, ICoder,
IPassword, IFolder, Agent and IFolderArchive retain their original GUID values.
These dependencies are explicit because the adapted Agent header has a narrower
dependency surface. MyInitGuid supplies IID_IUnknown on Unix; Windows uses the
platform libraries as its original comments specify.

The native headers remain exact, live third_party dependencies even though they
are absent from the portable definition unit:

| Dependency and original role | Current caller and definition boundary |
| --- | --- |
| `FileManager/MyWindowsNew.h`: taskbar COM declarations/GUIDs and old SDK fallback | Windows ProgressDialog2.h includes it. CProgressDialog uses CLSID_TaskbarList/IID_ITaskbarList3 in CoCreateInstance and retains its native taskbar operations. Modern MSVC selects system SDK declarations; sevenzip_common publicly links uuid and ole32. Unix selects the existing Qt taskbar adapter. |
| `Explorer/MyExplorerCommand.h`: shell item/command/enumerator declarations and old SDK fallback | Windows ContextMenu.h includes it before the four original COM bases. QueryInterface and EnumSubCommands retain native interfaces. Modern MSVC selects SDK declarations; system GUID definitions come from uuid. Unix excludes Windows COM bases while retaining the source command owner. |
| `Common/MyInitGuid.h`: one definition owner per GUID | The shell DLL's adapted DllExportsExplorer.cpp includes shared interface declarations before MyInitGuid, then refreshes MyWindows' definition macro to define only the project shell CLSID. It does not define a second portable interface set. Its distinct CLSID avoids registering over upstream 7-Zip. |

Current Windows MSVC all-target builds and native progress/shell consumers
provide link/runtime evidence for the selected SDK branch; the port audit's
P3-05.1 records the latest frozen checks and failed whole-gate limits. This review
does not claim execution of old MinGW/old SDK/Windows CE fallback branches or
native taskbar behavior on other operating systems. These retained declarations
do not require a new project GUID implementation. No source/header changes.

## Remaining reconciliation

### Six Options pages and Apply error policy, 2026-10-10

Source https://github.com/ip7z/7zip.git at full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`: complete SettingsPage, FoldersPage,
EditPage, MenuPage, SystemPage and LangPage `.cpp/.h`, desktop `.rc` and shared
`*Page2.rc` resources, OptionsDialog.cpp, Windows/Control/PropertyPage.cpp/.h and
Explorer/MyMessages.cpp/.h. This review covers the six page owners, not the whole
FileManager family or rendered/native OS acceptance.

| Source body/dependency | Disposition |
| --- | --- |
| Settings initialization, seven flags, RAM rounding/range, raw UInt32 validation and changed flags | Retained source state and calculation; Qt controls project native notifications. Large-page privilege belongs to the external engine process via `-slp`; original Common/ArchiveCommandLine enables it there. Invalid RAM uses the retained MyMessages error/question policy and remains PSNRET_INVALID. |
| Folders mode order, enable controls, Browse, GetWorkDir and Apply | Retained source NWorkDir::CInfo, validated mode, original path and modified state. Qt replaces Browse controls; source WorkDir owns actual temporary paths. Removable-drive detection remains the source Windows branch; POSIX preserves its disabled value. |
| Edit control records, init/change/apply and Edit_BrowseForFile | Retained three source Viewer/Editor/Diff records and SplitCmdLineSmart/Trim; Browse replaces the program and discards parameters as upstream. Qt supplies file-picker filters on each OS. |
| Menu shell discovery, reverse registration/requery, menu flags/BoolPairs, Zone, item text and events | Retained source order and algorithms behind typed module/registry services. Qt replaces list/combo notifications. Adjacent Windows DLL and bitness determine shell availability; applicable in-app policies remain usable on Unix. |
| System SetState/GetString, icon choice, mixed-selection cycle, Apply rollback/first error, mouse/key dispatch | Source state algorithms remain authoritative. Native services own registration/revision/alias/consent boundaries; Qt owns list/icons. Association failure is reported once and does not veto the property sheet, matching source PSNRET_NOERROR; later pages still apply. |
| Lang list ordering/locale marks, enumeration, missing/extra merge, metadata limits, selection and Apply | Retained source strings/vectors/parser and algorithms; POSIX enumeration uses the original portable NFind API. Qt replaces combo/static report with a read-only scrollable presentation and sends language-change notifications. |
| Headers/resources/callers | All active fields/controls have source or narrow Qt owners. Options applies System/Menu/Folders/Edit/Settings/Language; Settings invalid alone blocks this sequence. MainWindow uses source SetListSettings/RefreshAllPanels. RegistryUtils/ZipRegistry, WorkDir, LangUtils/HelpUtils and native services retain their separate authoritative modules. |
| Omitted branches | Commented underline/alternate memory UI, old language sort-order assignments and unused editor-parameter reconstruction remain inactive. WinCE compact pages, ANSI/legacy SDK dispatch and native property-sheet handles are outside the selected desktop Qt presentation; no active desktop page algorithm is omitted. |

Qt message boxes retain native sizing/button order; macOS hides their supplied
title. Source captions remain `7-Zip` on systems that display a title. Current
checks and platform limits belong to the
[receipt](upstream-port-audit.md#p3-05--six-options-pages-and-apply-errors-2026-10-10).

### Agent opening/factory body and caller review, 2026-10-07

The original Agent::Open and ArchiveFolderOpen::OpenFolderFile active bodies are
now on the production QtArchiveFolderManager route. FileFolderPluginOpen's
worker still owns modal progress and join; the source factory supplies callback
QI, source Agent lifetime, diagnostic S_FALSE publication and root-binding
progress scope. IInFolderArchive Extract/property interface bodies are restored,
with native/process services confined to ArchiveBackend. Original commented
FindFormat, FolderReOpen and property enumeration remain inactive. The exact
source, adaptations and current validation are recorded in the
[opening audit](upstream-port-audit.md#p3-05--source-agent-opening-and-archive-folder-factory-2026-10-07).
The subsequent lifecycle component restores LoadItems' original link-open
short-circuit and ReadItems' allocation/RAM/descriptor/load ordering. Written
counts are consumed as upstream in ReadItems/GetPropertyInfo; unwritten counts
initialize to zero. Partial-proxy cleanup remains a documented joined-worker
retry adaptation across descriptor/load errors, Cancel and exceptions.
Real ZIP/WIM Close/failed-ReOpen/repair/rebind and factory cancellation consumers
are covered in the [lifecycle audit](upstream-port-audit.md#p3-05--source-agent-provider-lifecycle-and-metadata-binding-2026-10-07).
Remaining Agent update/folder bodies still require full-module disposition;
these bounded components do not close the 109-module review or native/final acceptance.

The complete GetProperty/GetFolderProperty and path/tree comparator review also
found the missing path-proxy directory CRC branch. It is restored from the same
baseline, reusing source proxy aggregates and exact explicit-record/error/local
variant policy. Direct property/formatter/Properties and native ZIP/7z list,
sort/Refresh/selection consumers are recorded in the
[directory-property audit](upstream-port-audit.md#p3-05--source-archive-directory-property-projection-2026-10-07).
This disposition covers these property bodies, not remaining Agent update,
extraction/index/raw-property modules or full-module acceptance.

Source PanelListNotify::OnNotifyActivateItems is now retained with explicit Qt
event modifiers. The owning table/icon Return/double-click adapters replace the
native ComCtl32 notification-version switch and share the existing OpenNative
state/history projection. Parent/multiple-item behavior stays in OpenSelectedItems;
the broader OnNotify/status/drag/rendered/platform review remains open.
See [dated consumer evidence](upstream-port-audit.md#source-item-activation-notification-consumers-2026-10-07).

### PanelKey complete body/caller review, 2026-10-07

The full original `CVKeyPropIDPair`, table, `FindVKeyPropIDPair` and
`CPanel::OnKeyDown` are restored in their original translation unit. All active
cases and their ordering were compared, including right-Ctrl/Alt bookmarks,
F9, modified sort keys, Shift state, F2–F7, unconditional Delete, Insert,
Alt/Shift arrows, the notification-level Ctrl+PgDn branch, keypad selection,
Backspace, Ctrl letters,
view modes and Alt+F12. Presentation methods reach existing CApp commands;
F9/sort and panel callbacks keep their original owners. No archive syntax is
added. Source `PanelMenu::EditCut/EditCopy/EditPaste` is restored: selected
indices/raw names/CRLF and Unicode clipboard; commented shell/file-transfer
branches remain inactive. `Panel.h`, complete `PanelListNotify::LVN_KEYDOWN`
caller, `Panel.cpp::OnNotify` guard and `resource.rc` were reviewed. The resource
only registers F1 and Alt+F12 as active accelerators. Subsequent complete
Panel.cpp review corrects the earlier no-op conclusion: CMyListView::OnMessage
opens internally on Ctrl+PgDn before this notification handler, and handles
Ctrl+PgUp as Parent. Those branches now have a separate Qt list-key service;
PanelKey's commented EnterToFocused remains inactive.

`QtPanelKeyDown` replaces NMLVKEYDOWN/GetKeyState at the source input boundary.
`mainwindow_panelkey.cpp` routes address edits separately, claims conflicting
list shortcuts, decodes native right-primary-modifier notifications and supplies
the existing activation/WM_CHAR transports. Windows also queries original
VK_RCONTROL through GetKeyState; Cocoa maps Command to Qt Control and XCB
supplies its XKB keysym. Qt's native-key transport was inspected in the local
official Qt6.12 sources and macOS SDK constants. Source Tab's false return needs
consumption after its callback to prevent a second Qt focus traversal. Source
posted Shift selection uses a queued callback bound to the original owning
panel; source posted Close uses its QObject lifetime. Ordinary text editors
retain their own shortcuts. Full body disposition is not physical-keyboard,
rendered, three-platform or parent P3-05 acceptance; current evidence is in the
[source keyboard audit](upstream-port-audit.md#p3-05--source-keyboard-dispatcher-2026-10-07).

### RefreshListCtrl phase and caller map, 2026-10-06

Reviewed against the full revision linked above: the active original
`PanelItems.cpp::RefreshListCtrl`, its `SaveFocused`/`OnReload` wrappers,
`Panel.h::DeleteListItems`, `PanelListNotify.cpp::SetItemText`, and the operation
and navigation consumers below. This maps one function and its integration
boundary; it does not close the whole `PanelItems.cpp` module or P3-05.15.

| Original phase | Current authoritative owner and adaptation |
| --- | --- |
| Clear drop target; return for absent folder | Both `LoadListItemsImpl` and restoration clear the panel-owned target and retained Qt paint. Restoration now retains the original absent-folder S_OK before load HRESULT, pending selection, guards, sorting or status. Its independent providerless name/prefix/showDots fallback is removed. `CPanel(prefix, items)` and archive transport construct real `QtPanelFolder` providers; their restoration still uses the source walk. Sixteen initial/released-provider, empty/retained-parent-row, enabled/disabled-guard and stale-load-result scenarios cover no-op restoration followed by provider-backed recovery. |
| Reset text suppression; publish path unless timer | Path/address publication is in `renderBoundFolder`, `renderRootFolder` and `showDirectory`, with `CalledFromTimer` passed through `reloadPanelPresentation`. The source `_dontShowMode` reset is now retained at load/restoration; it suppresses post-Rename source/Qt callback text, including cached cells. `ShouldProcessNotifications` separately gates guarded row construction. Their source/GUI regressions are recorded in the port audit; native provider/platform closure remains separate. |
| Disable redraw/notifications; delete old rows; reset marks/group | `LoadListItemsImpl` owns deletion and source resets under timer/notify guards. `QtPanelList::DeleteListItems` removes model rows. Native `_thereAre_ListView_Items` uses Qt model count; the source `_enableItemChangeNotify` deletion toggle is retained along with the process guard. `mainwindow_archivelist.cpp::refreshActions` controls updates while scanning/projecting. Terminal redraw after success, failure and cancellation remains an integration review requirement. |
| Set flat mode; LoadItems; InitColumns; count and directory validation | `LoadListItemsImpl` retains these source branches. Worker-loaded FS passes its retained HRESULT instead of enumerating twice. Original written-count policy ignores the count HRESULT; a narrow optional retains native numItems across namespace publication/restoration/mark projection. Release/rebind invalidate it; the next load starts with known empty rows/marks so early failure/Cancel does not query metadata during mark projection. Unwritten output starts at zero as a defined Qt adaptation. Directory VT_BOOL/VT_EMPTY validation and cache clearing retain the source failure contract. See the [count consumer evidence](upstream-port-audit.md#source-refresh-loaded-count-consumers-2026-10-07). |
| Deleted-item properties; icon interface/attributes; insert callback rows | Deleted-item metadata and icon-interface selection remain in `LoadListItemsImpl`; `LoadItemIcon` runs the extracted source icon branch during bounded GUI publication. Native callback text is supplied by `ArchiveTableItem::data` and source `GetItemText`; the independent per-cell count/range gate is removed, restoring direct property lookup for successfully published indices. Eight EN/RU both-pane detail/icon count/property-result, notify/provider guard, recovery and actual rebind consumers preserve peer/selection/bytes; load-time validation and generation cancellation remain in their existing owners. Native LVITEM, insertion and system image-list services are Qt/platform adapters, not copied Win32 presentation. |
| Parent row, fast-name/flat-prefix selection, restore notify | GUI publication creates source-indexed rows; restoration walks those rows and retains source name/prefix matching and mark vectors. The source notification restoration precedes focus callbacks. Native parent selection and Qt alternative single-focus adaptation have separate recorded regressions. |
| Named focus; source sort; positional fallback; EnsureVisible; final status | Restoration now retains that order, resets the group anchor before the walk, updates Qt mark paint and publishes the original four-part status through the weak owning-pane callback. Initial renderer/header sorting is a separate Qt publication step; it does not substitute for this source sort phase. |

| Active caller route | Load and completion boundary |
| --- | --- |
| Explicit reload, timer, filesystem/namespace Refresh, post-Copy destination | `RefreshListCtrl_SaveFocused` calls original RefreshListCtrl, which invokes attached `reloadPanelPresentation`; synchronous render HRESULT reaches original `OnReload`. Native archive projection captures the same scoped source result continuation for deferred completion, restoring guards/redraw before reporting and holding projection depth until reporting finishes. Cancelled/replaced generations discard that continuation. Qt publication completion uses the extracted RestoreLoadedSelection tail without recursively reloading. FS workers retain their load result through the source load stage and deferred restoration. `QtAppPresentation::CompleteCopyIfReady` waits for the operation owner before destination reload and source completion. Real filesystem Copy/Move with a native301-row archive peer covers deferred errors, source cleanup/focus and recovery; native writer-failure consumers remain separate. |
| Initial creation and OpenAsArc_Index | Both original callers ignore RefreshListCtrl's result. Their bounded native archive publication now has no OnReload error continuation. OpenAsArc_Index releases binding guards before Refresh retains its own; this prevents asynchronous completion from restoring disabled notification/timer flags. Direct source open, initial creation, retry and native S_FALSE/E_FAIL/E_ABORT are covered; nested OpenItemInArchive and other operation renderers still need their complete caller/guard review. |
| Parent/root/drives/AltStreams, same/subfolder, address binding | Source folder changes clear old rows and save pending selection. `goParent`, `goRoot`, `QtFileManagerCommands::OpenAltStreams`, `QtAppPresentation::RefreshFolder` and the address navigation callback then render the bound provider. `renderBoundFolder` takes pending selection before load and restores it only on successful publication. Namespace load/property/icon failures cancel stale projection and finish Qt sorting/notify/busy state, with address publication before load unless timer. Parent now publishes written count despite metadata HRESULT; actual load failure/Cancel/recovery preserves both panes and the original ignored reload-result policy. Archive/FS navigation failures and remaining void operation result consumers still require review. |
| Comment | `ChangeComment` now hands its source-captured state and final reload-phase flag to `QtFileManagerCommands::Comment`. Early exits/Cancel restore presentation without loading. Accepted operations render, then restore only after successful loading, including deferred publication. Source operation error presentation and ignored reload-result policy remain distinct. The former unconditional outer reload and premature restoration are removed; both-pane EN/RU failure/cancellation/recovery evidence is recorded in the port audit. |
| Rename | Qt `itemChanged` captures raw EditRole/source index and queues `finishFilesystemRename` after the editor callback. Source timer/notify guards cover the operation worker. Original saved names/new-or-old focus/SelectFocused pass through the pending refresh channel; mainwindow_rename.cpp loads before restoring and checks render S_OK. Original `_dontShowMode` controls source formatter/cached Qt DisplayRole until load/restoration. Early validation adapts native FALSE without loading. Completed-worker errors belong only to source progress; the extra Qt warning was removed after actual native RED. Native F2 writer success/Write failure/Cancel, later property publication failures, Ctrl+R and real writer retry cover this consumer. Native ADS/platform and changed-parent/full-body acceptance remain open. |
| Drop into archive / CopyFromNoAsk | `PanelDrag::CompressDropFiles` reaches source `CopyFromNoAsk` for an empty destination folder path. Its success-only Refresh phase and saved state now pass through an optional output to QtFileManagerDrag::ExecuteDrop after source timer/notify guards unwind; default source callers retain immediate Refresh. Qt publishes/restores that one bound provider and completes source list focus after deferred rows. Failure/Cancel/confirmation refusal returns without that phase. Native source update owns archive ReOpen/proxy restoration in ArchiveFolderOut.cpp; no compensating all-visible ReOpen returns. Eight bounded native writer/error/Cancel/refusal, later attribute HRESULT, Ctrl+R and actual retry cases cover guards/focus/bytes/peer/OnTimer. Current source RED, driver diagnostics and three-OS evidence are in the dated port audit; rendered and whole-body acceptance remain separate. |
| External-file update | Source OnOpenItemChanged retains folder/index/name checks, saved selection, operation and unconditional RefreshListCtrl before its update-result branch. Optional CSelectedState now transports that reached phase to Qt; runExternalArchiveUpdate publishes after guards unwind for failure/Cancel as well as success. Worker read-only and source early refusals do not publish. Restore follows only successful render; reload HRESULT retains native ignored diagnostic policy. Native external2/301 consumers now verify actual official writer success/Write failure/source Cancel, later native property publication failure, Ctrl+R and actual F4 writer retry with peer focus/bytes/guards. Source/real editor matrix and provenance are in the port audit. Native rename/drop/changed-parent consumers and platform acceptance remain separate. |
| Manual archive Refresh/Ctrl+R | Original CApp::RefreshView inline body is restored in App.h; Qt dispatch invokes it for all providers. Bound CAgentFolder is loaded/published by OnReload's existing presentation callback, without engine ReOpen. Real ZIP/7z both-pane/action/key/EN-RU/view/selection consumers and nested-folder repeated refresh cover this boundary; native platforms remain open. |
| Filesystem-to-archive Copy/Move | Original OnCopy saves destination state before CopyFrom and refreshes its bound provider. Qt success now uses that pre-update state with RefreshListCtrl, rather than a second ReOpenArchiveFolder/provider replacement. Failure/Cancel retain existing unconditional destination OnReload and source continuation. Native ZIP/7z,301-row publication and transport failure/cancellation consumers are in copy_archive_refresh. Interarchive Move rollback is separate. |
| Interarchive Copy/Move | Successful destination publication uses pre-update saved state and original RefreshListCtrl on the existing provider. Source CopyTo/CopyFrom own staging/update; accepted Move composes source Delete and backup rollback because native CopyTo(moveMode) is E_NOTIMPL. Delete No/Cancel/error/engine cancellation restores exact destination bytes and then physically reopens with pre-update state. Before backup publication, the caller supplies source CAgent::Close through a narrow CAgentFolder bridge, releasing native read sharing without coupling the standalone backend to Agent. interarchive_refresh covers native ZIP/7z,301-row publication, both views/panes and controlled transport phases. interarchive_recovery adds actual native reopen failure/Cancel to restore failure, filesystem fallback and repaired-destination retry; archive_transfer_backup covers refused close/retry. Current platform evidence is in the dated audit; rendered and whole-platform acceptance remain separate. |
| Other operation archive reopen | `refreshCurrent` reopens through `ReOpenArchiveFolder`, then renders/restores. Drag and successful filesystem/interarchive destination completion no longer call it. Physical interarchive rollback deliberately retains this reopen. It does not share the synchronous `OnReload` result consumer for every remaining operation renderer call. Remaining source `PanelItemOpen` continuations and native renderer failure require full-consumer reconciliation. Source OnCreate now owns initial and late-panel publication through the service installed with its controls; constructor/toggle no longer publish again. Its original ignored initial Refresh HRESULT policy is retained. |

Source creation's Qt splitter insertion now follows FM.cpp::MoveSubWindows:
panel0 left/panel1 right regardless of creation order. This geometry service
does not change the retained App creation/binding algorithm.
The native error matrix also invokes actual filesystem OpenItem before
OpenAsArc_Index, handing off both outer and inner guards. Post-recovery
filesystem navigation and OnTimer verify the retained timer state. This does
not cover the separate nested OpenItemInArchive branch. The later
[native nested component](upstream-port-audit.md#p3-0515--native-nested-streams-and-publication-handoff-2026-10-07)
reviews its complete body and both callers, restores original Agent::GetStream,
and verifies all three successful publication handoffs plus actual native Cancel.
Other external-edit/changed-parent/zone and native writer/render failure branches
still require complete reconciliation and platform acceptance; this component
does not close the full module.

The initial map made no functional edits, source copies or manifest changes.
Its Comment row now records the subsequent integrated caller correction. Existing
RED/GREEN and geometry evidence remains bounded to the individual phase fixes
in [the port audit](upstream-port-audit.md#source-refresh-alternative-selection-anchor-2026-10-06).
Next work must exercise the outstanding operation-to-render result consumers,
remaining native drop/changed-parent workflows, terminal redraw and provider boundaries,
then reconcile remaining `PanelItems` bodies/dependencies/resources. A green
whole repository gate alone does not establish those source-body dispositions
or current Windows/Linux/rendered/cold-stable acceptance.

### PanelItems complete body map, 2026-10-06

Compared the complete original `CPP/7zip/UI/FileManager/PanelItems.cpp` at
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17 from https://github.com/ip7z/7zip.git
with the adapted module at project b834b9e. The original has35 active CPanel
bodies plus four column helpers. Commented-out RefreshListCtrl overload and
GetOperatedListViewIndices are excluded; neither supplies missing active logic.
The adapted file has36 CPanel bodies, including extracted load/icon phases and
Qt helpers. These counts are a review index, not functional acceptance.
Subsequent corrections update the dispositions below; the body counts remain
those of the original review snapshot.

| Original bodies | Current owner / disposition |
| --- | --- |
| GetColumnVisible, GetColumnWidth, GetColumnAlign, ItemProperty_Compare_NameFirst | Same module retains visibility/width/name-first policy. Alignment replaces LVCFMT with Qt alignment plus vertical centering. PropertyName and original CPropColumn/CPropColumns supply names/types/order; no second schema. |
| InitColumns | Same module retains folder/raw-property enumeration, individual info failures skipped, descriptor types and saved visibility/order/width, name protection, visible/name-first sorting and sort default. Original mark clear now precedes enumeration on every call; equal-column early return and DeleteColumn/AddColumn rebuild restored. Original CIntArr order validation is extracted as GetColumnOrder for deferred Qt physical publication: source logical Name stays first, visual Name may move elsewhere. Direct unchanged-catalog marks/rows/selection/restore and EN/RU both-pane move/Refresh/reopen/width consumers pass. Publication now gives each catalog entry a separate Qt section and uses its exact descriptor for cells/alignment/visible width/order, including generic/raw CRC and reserved Size overlap. Both-pane EN/RU guard/error/Cancel/Refresh/peer consumers pass. Duplicate-ID menu projection now retains first-visible-ID deletion and appended copies with separate visible logical mappings, callback cells/widths and selection; both-pane EN/RU repeated toggles pass. Overlapping CRC/Size header sorting now uses source first-catalog raw flag and same-PropID toggle through generic/raw/append sections;16 EN/RU both-pane copies/Refresh consumers pass. Written generic/raw count enumeration now retains original ignored-HRESULT loops, with existing initialized-zero outputs; individual info errors/IsDir are skipped. Same-provider raw-Name/no-Name/empty/unwritten/recovery consumers retain source defaults/logical indices and remove phantom Qt Name, with rows/selection/menu/peer preserved. Label-only selection now resolves native visible subitem zero when reserved Name is hidden; actual delegate-image RED4 and focused consumers cover raw Name/no Name. Icon/list projection now resolves the same logical-zero text and existing row icon after schema publication/menu mutations; four view modes cover raw-Name hide/re-add, no-Name/empty/recovery with peer focus/selection retained. Real FS/ZIP/7z defaults, independent per-type width/order/visibility, unchanged equality and separate Type/Name changes now have an integrated EN/RU both-pane workflow; see the [dated evidence](upstream-port-audit.md#source-real-provider-column-workflow-2026-10-07). Provider failures and raw/empty/duplicate schemas retain the focused evidence above; current native/rendered acceptance remains open. |
| DeleteColumn, AddColumn | Original visible-vector bodies now remain in PanelItems and are called by ShowColumnsContextMenu and restored InitColumns rebuild. Win32 LV_COLUMN insertion/removal and column-order-array transport become Qt hide/show/append and deferred source order-array application with compact visible order persistence. Both-pane menu/move/width/Refresh/reopen and direct unchanged-catalog consumers cover these boundaries. Qt now maps native visible indices separately from the catalog and creates additional callback sections for appended copies, preserving first-ID deletion, widths and selected rows. Overlapping CRC/Size header sorting retains source flags/toggles after Qt mouse dispatch and across appended copies. Real-provider defaults/settings and separate Type/Name equality changes now have integrated coverage; current native/rendered acceptance remains required. Path presence alone does not prove whole column closure. |
| RefreshListCtrl() / RefreshListCtrl(state), RefreshListCtrl_SaveFocused, SetFocusedSelectedItem | Retained source restoration and extracted LoadListItems/LoadItemIcon with Qt row publication. The phase/caller map above owns remaining terminal/async/error boundaries. SaveFocused calls RefreshListCtrl, which invokes the attached load/publication callback; source OnReload consumes its HRESULT. Qt render completion calls extracted RestoreLoadedSelection, avoiding recursive reload. Callback-free fixture restoration and deferred operation guards remain explicit boundaries. Pending state transfers synchronous source navigation to Qt publication. |
| GetSelectedNames, SaveSelectedState, Get_ItemIndices_Selected, Get_ItemIndices_Operated, Get_ItemIndices_All, Get_ItemIndices_OperSmart | Same module retains source names/mark vectors, operated focused-row fallback and all-item/provider count. Source OnItemChanged now consumes owning-model selected-state deltas behind notification/deletion/alternative guards, including inactive panes. SaveSelectedState reads marks without rebuilding them; pre-populated Qt attachment seeds once and load prepares marks before deferred publication. Direct InitColumns restoration uses the pre-clear snapshot. Dated source/consumer evidence is linked in the audit. GetFolderItemCount's providerless row-count helper remains outside the newly corrected absent-folder Refresh guard and needs disposition with snapshot/callback consumers. |
| EditItem(bool), OpenFocusedItemAsInternal, OpenSelectedItems | Same module retains folder-size calculation/invalidation, parent refusal for editing, operated-item20 limit, selected-parent insertion and first internal-folder break. Qt commands call these bodies; PanelItemOpen owns actual external/internal opening and process/update continuations. Their complete workflow/platform acceptance is separate. |
| GetItemName(value), GetItemName(output-reference) | Value-returning BSTR/type/error/parent body remains. The output-reference overload is omitted; adapted Refresh/formatter/icon callers use the value body rather than a second property implementation. This omission does not justify removing the generic property fallback. |
| GetItemName_for_Copy, GetItemPrefix, GetItemRelPath, GetItemRelPath2, Add_ItemRelPath2_To_String, GetItemFullPath | Same module retains out-name fallback, extraction name correction, property/fast-name append and Windows drive/device separator conditional. Existing path tests and Copy/Open consumers cover separate subsets; current native device/ADS acceptance remains mandatory. |
| GetItem_BoolProp, IsItem_Folder, IsItem_AltStream, GetItem_UInt64Prop, GetItemSize | Same module retains variant/exception policy, parent values, directory cache and fast size. The cache is populated by the extracted load stage; no independent archive metadata policy replaces it. |
| IsItem_Deleted | Original bool-property/parent body is now restored. PanelListNotify retains `_markDeletedItems` (default true) / OnCustomDraw red-foreground policy; the existing archive-level flag gates it. ForegroundRole asks the owning guarded provider; the shared Qt delegate also carries it into HighlightedText so selected text stays red. Property/type exceptions remain in the source getter, while Qt paint uses the palette after an error. Table/icon/both-pane/reload/rebind/metadata/guard and current rendered-red regressions cover this consumer; real deleted-archive/native/rendered Windows acceptance remains open. |
| SaveListViewInfo | Original saved visible/hidden columns, width/order, sort and equality logic remains, taking the owning Qt projection in source visible logical order instead of GetColumn/LVCOLUMN. Independent mutation of catalog visibility/order/width is removed. MainWindow counts visible sections only; menu re-add applies catalog Width like native AddColumn. Name-to-end/width without hide/show survives Refresh/reopen while source logical Name stays first; direct catalog/visible/persisted metadata and next InitColumns consumers pass. ViewSettings owns settings-path transport. Real FS/ZIP/7z per-type settings and changing Type/Name consumers now pass alongside the raw/duplicate/schema/error suites above; current native/rendered acceptance remains open. |
| OnRightClick, ShowColumnsContextMenu | Win32 notification/cursor/popup transport is replaced by the Qt header menu. Source selected-index mutation, AddColumn/DeleteColumn vector bodies and sorted-column fallback are restored. Qt appends newly shown sections, serializes compact visible order and updates the indicator without immediately sorting rows. Both-pane EN/RU active/inactive menu/Cancel/move/resize/Refresh/focus consumers and existing settings/reopen tests cover this boundary. Runtime popup has no separate dialog RC layout; native/rendered acceptance remains open. |
| OnReload, OnTimer | Same module retains process/auto-refresh/folder/interface/change/result guards and OnReload error owner. The portable changed flag supplements native WasChanged; MainWindow polls only supported filesystem fallback state. Source timer coverage remains subject to current Windows/Linux and native provider acceptance. |

Dependencies reviewed: original/adapted Panel declarations, source selection/sort/
notification/folder-change/open callers; MainWindow columns/selection/renderers,
QtFileManagerCommands, FileTable delegate and QtPanelList; IFolderFolder,
IFolderGetItemName/SystemIconIndex/RawProps/CalcItemFullSize/WasChanged,
IGetFolderArcProps/IFolderArcProps, PropID/PropVariant/PropVariantConv,
ExtractingFilePath, FileName, SysIconUtils, PropertyName and ViewSettings.
Original FM.mak selects PanelItems/PanelListNotify/PropertyName/ViewSettings;
CMake registers their authoritative adapted sources and exact PropertyName.rc.
Original C/Sort and CObjectVector sorting remain portable dependencies, not a
language conversion. Win32 Menu/ListView/PCH and debug/commented USE_EMBED_ITEM,
old virtual-list, NTFS-stream, icon-second-attempt and timing experiments are
presentation/build or inactive branches, not missing active algorithms.

This review adds no code, dependency copy or baseline change. Next available
functional scope includes native deleted-item and column-menu acceptance,
alongside the operation-to-render consumers already listed above.
Neither35 reviewed bodies nor a passing repository gate closes PanelItems,
the109-module ledger, native platforms or final P3-06 acceptance.

### App complete body/caller review, 2026-10-06

Follow-up dependency review: the complete original
PanelItems.cpp::RefreshListCtrl body and SaveFocused/OnReload callers at the
same upstream pin were inspected for the OnCopy continuation. The outside-peer
destination publication and namespace reload gap are now adapted through the
existing Qt owners. Load/columns/property/name/icon/parent/sort/redraw/status
stages remain separate from the retained selection stage; their remaining
whole-body/native/HRESULT/timer acceptance is detailed in the
[port audit](upstream-port-audit.md#unconditional-destination-publication-and-namespace-reload-2026-10-06).
This is a function/dependency review, not another module-wide closure.
Follow-up integration extracts source row/mark reset, flat/load/columns,
directory validation/cache and deleted-item query into PanelItems::LoadListItems.
Namespace/archive/immediate FS and worker-loaded FS consumers share it; Qt
publication retains its existing asynchronous boundary. Original directory
types/errors and delete-before-load are now covered by public regressions.
Remaining complete RefreshListCtrl presentation/timer/HRESULT scope is still
required; the source stage does not close the module review.
Synchronous namespace/archive renderer results now reach the original OnReload
HRESULT error consumer; namespace Refresh delegates to that owner. EN/RU direct/
explicit/timer/command failure/Cancel/recovery consumers cover this seam.
The selection stage retains the load HRESULT across Qt publication and returns
it before querying failed provider state during OnCopy completion. Empty-row
snapshots no longer read a failed count; ReleaseFolder clears the retained
result. Both source roles/Copy/Move exercise load/property/type/count failure,
Cancel, original focus/title/marks and recovery with real output bytes.
Synchronous filesystem reload now retains its native provider and load HRESULT;
filesystem Refresh uses source OnReload. Both roles/EN-RU direct/reload/timer/
command and actual filesystem Copy/Move peers cover controlled Cancel/allocation
failure and recovery. Actual async worker errors now enter the original load
RINOK before properties/selection; the bound provider and HRESULT remain intact.
The source message precedes scan completion. The Qt continuation waits until
temporary peer projection is restored before changing source focus/title.
Both-role EN/RU normal/flat/navigation/Refresh and real Copy/Move peer consumers
check old-generation rejection, provider/result/bytes and recovery.
Missing-folder reload now runs the source load stage to clear native items/marks;
CFSFolder restores original Windows Next(fi) end-of-walk semantics, with POSIX
enumeration errors ending the walk too. Direct missing CFSFolder::Init still fails.
Both-role EN/RU standard/alternative Refresh/timer and restore consumers cover
native count/operated indices, provider/guards and control bytes.
Typed missing descendants use the retained BindToPath closest-ancestor algorithm;
both-role EN/RU absolute/relative/trailing-separator consumers check actual prefix,
title/focus, peer provider/selection and control bytes. Source Windows outer-quote
preprocessing is extracted into PrepareBindPath for both original and Qt callers.
Native Windows quoted-route acceptance and other
post-Copy failure behavior still require reconciliation;
this remains a partial integration rather than complete RefreshListCtrl closure.
CalledFromTimer now reaches Qt filesystem/root/archive address publication;
timer reload skips edit/current-image/history updates as the original source
does, while ordinary reload publishes its path. Original parent-name/fallback
focus and dots selection now reach Qt rows. Central Open/Open Outside availability
recognizes the selected parent via source saved state and leaves navigation with
OpenSelectedItems/OpenFolder/OpenFolderExternal. Remaining row/icon/redraw
and full-module/platform acceptance are separate obligations.

Compared the complete1004-line original `CPP/7zip/UI/FileManager/App.cpp`
with the602-line adapted module at project
`c590f0f1f372f4a236662fac8f490a630d6ef964`. Source is
https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, with sibling HEAD verified.
App.cpp remains an active FM.mak/CMake input. All37 top-level active function
bodies, both SetButtonText overloads, CButtonInfo::GetText, toolbar arrays and
associated App.h inline callers/state interfaces were inspected. This review
does not establish closure of the other108 modules or current runtime parity.

| Original bodies/data | Current authoritative implementation and boundary |
| --- | --- |
| CPanelCallbackImp OnTab, SetFocusToPath, OnCopy, OnSetSameFolder, OnSetSubFolder, PanelWasFocused, DragBegin, DragEnd, RefreshTitle | Original focused/opposite-panel forwarding retained. Qt uses the owning `_app` instead of process-global g_App; SetFocusToPath invokes the panel's Qt focus/dropdown service rather than a Win32 header handle. Source drag, copy and folder-change owners remain separate. |
| CApp::ReloadLangItems, SetListSettings | Selected-count language lookup and settings-to-each-panel flow retained. Qt view/hover/selection adapters replace LVS style bit fields; original alternative selection, dots, icons and System-menu settings remain source state. Current language/address consumers are covered by P3-05.9–.14; this review is not another runtime gate. |
| CApp::CreateOnePanel, Create, SwitchOnOffOnePanel | Saved path, callback binding, panel ID, state/settings/list mode/width/flat-mode ordering, main-panel creation order and source Create/binding retained. Qt presentation replaces child/menu/toolbar/window handles. Create captures the original main choice separately from live Qt focus/projection, retaining ordinary requested main path when panel1 is main and correct source provider during interleaved completion callbacks. Initial Qt focus waits for scans and is cancelled by subsequent explicit input/public archive requests; modal public archive opening restores the selected address control. Source OnCreate owns initial/late publication without constructor/toggle render duplication. The adaptation restores NumPanels on failed second-panel creation; original increments before returning an HRESULT failure. Lifecycle audits/tests own this deliberate failure-state correction. |
| CApp::Save, ReleaseApp | Source state save, outer-parent path for open archive, list modes/flat flags, disabling processing for all panels before releasing any and COM/library teardown ordering retained. Qt Save reads prior list settings and skips uncreated panels to preserve their persistent values; Release first closes the operation owner and detaches command services. These are explicit lifetime/settings adaptations, not byte-identical bodies. |
| CreateToolbar, CButtonInfo::GetText, g_StandardButtons/g_ArchiveButtons, SetButtonText overloads, AddButton, ReloadToolbars | Native toolbar/image-list/bitmap/ANSI versus Unicode control transport omitted. Current Qt presentation constructs the same seven source command/label groups and supplies platform icons/action text, tooltip storage and large/text/visibility settings. Win32 destroy/recreate is replaced by updating owned QActions/widgets; exact bitmap/rendered parity remains P3-06. The original arrays/lookup are data/presentation references, not archive-operation algorithms. |
| SaveToolbarChanges and App.h ReadToolbar/SaveToolbar/SwitchStandardToolbar/SwitchArchiveToolbar/SwitchButtonsLables/SwitchLargeButtons | Original mask bits/defaults, toggle and Save→Reload→Move order retained in App.cpp with explicit settings path and Qt small-screen input. MoveSubWindows delegates Qt splitter/layout transport; original native geometry owner belongs to FM.cpp. CE command bars are outside supported Qt6 desktop targets. |
| Reduce_Path_To_RealFileSystemPath | Original active directory-existence/root-prefix body restored in App.cpp, using existing FileFind/FileName dependencies with qualified namespaces/formatting. OnCopy uses destPanel.GetFsPath and invokes it only for one panel. The independent IQtAppPresentation default hook is removed; separate Extract presentation still uses MainWindow::copyDefaultDestination. Remaining P3-05.15 classification/completion is open. |
| AddSizeValue, AddValuePair1, AddValuePair2, AddPropValueToSum, CPanel::GetItemsInfoString | Source totals/sentinel propagation, language text, folder/file grouping, path/selected-name summary limit and ellipsis retained. QString return/size formatting are Qt string boundaries. Directory suffix now retains Add_PathSepar, with summary consumers expecting the native separator. |
| CApp::OnCopy | Capabilities, focused/OperSmart indices, dialog/history, default/reducer and destination classification/refusal/creation predicates retained. Captured indices/naming and staged defaults reach Qt execution. Timer stack guards unwind before Qt retains timer/notify guards; engine/rollback settles before publication. Source CFileMenu allows no marks and empty no-op. Original post-modal saved states and conditional post-operation title/selection/notify/focus body are retained as a single-use CApp continuation after terminal execution/scan publication. Ordinary two-panel FS Copy preserves source rows/current item; deferred progress cannot reset the restored title. Full RefreshListCtrl loading/unconditional destination refresh, remaining native/HRESULT/ADS-device-zone branches and whole-body acceptance remain mandatory. |
| CApp::OnSetSameFolder, OnSetSubFolder | Original two-panel/provider/parent/close/bind branches retained, with Qt preparation and row publication outside the source flow. Accepted bounded P3-05.4 consumers own actual verification. |
| CApp::OnNotify and tooltip buffers | Win32 TTN_GETDISPINFO/ANSI/wide tooltip transport replaced by QAction-owned localized tooltip strings. The commented GetFocusedPanelIndex experiment is omitted; active original App.h inline LastFocusedPanel getter remains. Commented CheckFolderPath/IsFsPath are not missing active implementations. |
| CApp::RefreshTitle, RefreshTitlePanel | Original prefix/default title/PrevTitle/only-focused-panel suppression retained; QWidget title service and closed-owner guard replace MySetWindowText. |
| AddUniqueStringToHead, CFolderHistory::Normalize/AddString | Original case-insensitive removal/insertion and100-entry limit retained with GUI-thread ownership instead of the native critical-section lock. Settings/language/virtual-location consumers are separate from the collection algorithm. |

Direct interfaces/dependencies reviewed include App.h/AppState/Panel folder
links, selection and timer/notify guards; source folder capability/prefix and
CCopyToOptions; CopyDialog and its resource; FileName/FileDir/Find/PropVariantConv,
FormatUtils, LangUtils, RegistryUtils/ViewSettings and source toolbar/summary IDs.
CopyTo/CopyFrom's ExtractCallback/Agent/FSFolderCopy owners retain their own
functional audits. Active callers are original App.h menu methods, MyLoadMenu,
PanelKey/Panel callbacks, FM create/save/tooltip/layout handling and drag owners.
Current consumers include QtAppPresentation, centralized command services,
mainwindow_twopanel/fileoperations/archive_transfer, FileTransferOperation and
ArchiveBackend publication/rollback. Their relevant bodies were traced rather
than treating ExecuteCopy as an opaque transport.

The mandatory remaining component is P3-05.15. Original OnCopy starts with
destPanel.GetFsPath and applies Reduce_Path_To_RealFileSystemPath only for one
panel; both branches are now restored, removing the previous Qt directory_/
returnDirectory_ fallback when the peer prefix is empty. Original Windows
single-leading-backslash/outside-peer ADS refusals, possibleToUseDestPanel/
destIsFsPath FS/ADS/archive/root/drives distinction and Is_IO_FS_Folder routing
are now retained. The ADS prefix predicate is Windows-only: its upstream portable
implementation is not defined on POSIX, where colon names remain normal names.
Original normalized multi-directory/parent creation and path+HRESULT diagnostics
precede history/execution. Panel.cpp's original formatting helpers and existing
ProgressDialog2 formatter provide those diagnostics through the Qt message box.
Captured indices/GetItemRelPath2 reach filesystem/extraction/CopyFrom consumers;
interarchive Move's accepted Copy/Delete/rollback extension supplies the same
indices to Delete instead of reading changed selection. Staged includeAltStreams
and default zone flags now match source options; native metadata and full original
completion remain open. Generic filesystem safety checks alone do not prove
these source predicates or native workflows.

Original operation options include alternate streams and retain default zone
policy. CopyArchiveToArchive staging now retains includeAltStreams=true and
the constructor's NeedRegistryZone=true; actual native consumers are still
required before metadata acceptance. Original
useFullItemPaths selects GetItemRelPath2 versus GetItemName_for_Copy; source/
destination selections, notify guards, CopyTo/CopyFrom HRESULT/cancel handling,
title/row refresh, KillSelection and focus restoration have Qt continuations
which must preserve the original request while publishing asynchronously.
Accepted archive Move remains a separate plan-required composition: original
Agent CopyTo(moveMode) is E_NOTIMPL, so source Copy/Delete plus transactional
rollback has no usable original Move body to substitute. This review does not
remove that accepted extension or its checked failure/cancellation behavior.

Required implementation must restore usable source destination/classification/
option/control flow in App.cpp, retain narrow asynchronous/platform services and
the accepted transaction owners, and cover default/refusal/FS/archive/ADS/device/
direct/relative targets, focused versus operated items and both panels with
success/failure/Cancel and unchanged control bytes. Existing Copy/Move and
Shift/direct-path evidence stays valid for its bounded scenarios; it does not
close this whole-body reconciliation. The initial review changed no code;
subsequent implementation/validation evidence is recorded in the port audit.

### PanelFolderChange complete body/caller review, 2026-10-06

Reviewed the complete1122-line original
`CPP/7zip/UI/FileManager/PanelFolderChange.cpp` against the857-line adapted
module at project `e314301c908b7b8e4ab8a27d016fe037f05650b8`. Source is
https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; sibling HEAD was verified.
The module is an active FM.mak input; CMake includes its authoritative adapted
translation unit in sevenzip_panel. This is a body/disposition review of this
module, not closure of all109 UI modules or a new runtime verification.

| Original bodies | Current body and explicit boundary |
| --- | --- |
| ReleaseFolder, SetNewFolder, SetToRootFolder | COM release/query order and root construction retained. Qt deletes selection rows through its list adapter and resets pending refresh/cache vectors when releasing. The local comment claiming no complete list-load stage exists is historical: PanelItems now owns RefreshListCtrl, with a separate Qt publication phase. The resets are present adaptation, not evidence that source ownership is missing. |
| DoesNameContainWildcard_SkipRoot, BindToPath | Original archive-link reuse/unwind, deepest existing filesystem ancestor, trailing-separator enumeration, root-prefix handling, Windows wildcard/ADS branches, OpenAsArc and archive/alternate-stream component traversal retained. POSIX enumeration uses existing CDirEntry rather than Windows CFileInfo. Failed per-component folder binding still stops at the existing ancestor as in source; HRESULT behavior must not be replaced with a new path validator. |
| BindToPathAndRefresh | Original timer/notify guards, Windows outer-quote removal, BindToPath result and RefreshListCtrl retained. CApp same/subfolder callers use this body. MainWindow's address and history/bookmark consumers instead supply Qt/native publication; that boundary has the remaining caller debt described below. |
| GetFolderPath, LoadFullPath | Original BSTR property/type check and parent-link ParentFolderPath+RelPath composition retained. The source virtual prefix differs from a materialized archive's engine path; CFolderLink/CTempFileInfo separate VirtualPath, FilePath and ParentFolderPath. |
| GetRealIconIndex_for_DirPath, Find_FileExtension_DotPos_in_path, LoadFullPathAndShow | Source directory attribute/fallback, extension classification, root/device/archive-link branches and RefreshTitle retained. Qt replaces shell image indices and ComboBoxEx edit item with its existing icon cache/leading action; explicit displayed-path/history arguments preserve the earlier presentation policy. Current consumers and unverified native branches belong to P3-05.12/.13. |
| OnNotifyComboBoxEnter, wide OnNotifyComboBoxEndEdit | Source success/focus and Escape/Return flow retained with typed Qt reasons and address services. Source GetUnicodeString is unnecessary at an already-wide UString boundary. The non-Unicode overload is an ANSI control transport duplicate, omitted for Qt6 Unicode controls; this is not an omitted independent navigation algorithm. |
| AddComboBoxItem, OnComboBoxCommand | Source hierarchy, UNC server/root prefix, path parts, archive-link boundaries, separate ComboBoxPaths and selection binding retained. Qt replaces control/icon/drive services and bounds-checks its selected index. POSIX adds its documented Volumes entry/mount service; Windows Documents/drives and device/super-prefix branches remain. CE text indentation/header-refresh messages are unsupported desktop transport, not claimed runtime evidence. |
| OnNotifyComboBox | Win32 notification dispatcher omitted. CPanel::eventFilter records editable-line/list FocusIn and invokes PanelWasFocused; Qt combo's typed end-edit callbacks deliver Return/Escape. The underlying handlers remain in CPanel; the commented Dropdown/Closeup/Selchange experiments are not active missing bodies. |
| FoldersHistory | Original editable list, Cancel return, changed-list reverse insertion and focused path selection retained. Qt modal/settings services replace Create/GetParent; selected path is returned to MainWindow for publication. Existing tests establish list editing/persistence, not all original virtual-location consumers. |
| SetBookmark, OpenBookmark | Current static helpers retain CFastFolders SetString/GetString but inject the stored prefix and return a path to Qt rather than using the bound CPanel/_appState and BindToPathAndRefresh. That adaptation is incomplete: the caller's display-path selection and restrictions differ from source. Required P3-05.14 below; this pair is not a fully reconciled port. |
| GetParentDirPrefix, OpenParentFolder | Original separator/colon/device prefix logic, parent-provider binding, archive close/rebind and focused-name state retained. Source RefreshListCtrl selection is published by existing Qt consumers. |
| CloseOneLevel, CloseOpenFolders | Original release/library transfer, parent-archive return, stack deletion and flat-mode restoration retained. Actual nested opening/materialization/update consumers are owned by PanelItemOpen and ArchiveBackend, not independently reimplemented here. Five native changed-Parent representative2/301-row consumers verify Yes/No/Cancel, writer success/failure/actual Cancel, retained modified spool, ignored publication HRESULT, same-parent recovery and real F2/Parent writer retry; see the dated port audit for validation limits. |
| OpenRootFolder, OpenDrivesFolder, OpenFolder, OpenAltStreams | Original distinct namespace/drive transitions, selection visibility and alternate-stream provider/fallback retained. P3-05.13 restored the missing drive body and active command/character callers. CE uses its retained filesystem-root branch; desktop Windows ADS remains native-only. RootFolder's separate ADS refusal HRESULT gap remains under the recorded root review. |

Reviewed direct dependencies are Panel.h's folder links/guards/selection and
callback services, AppState.h/CFastFolders/CFolderHistory, Common StringConvert
and Wildcard/SplitPathToParts, Windows FileName/FileDir/Find/PropVariant,
PropID and folder interfaces, FSDrives/CE FSFolder, RootFolder name services,
LangUtils, ListViewDialog, ViewSettings and resource IDs. Source callers include
App.h/MyLoadMenu, Panel.cpp notifications/characters, PanelItems refresh and
App.cpp same/subfolder callbacks; Qt callers include address combo, root/disk/
archive renderers, mainwindow_preferences and centralized command availability.
Dependencies' full independent bodies remain with their own audits; reading
their used interfaces here does not close those modules.

The review found another mandatory active consumer gap, P3-05.14. Original
SetBookmark stores `_currentFolderPrefix`, and original OpenBookmark binds that
stored source path, including an empty slot's root binding. Source MyLoadMenu
creates all ten Set and Open entries without the Qt nested/root/empty-slot
prohibitions; App.h forwards to the focused CPanel. Current
MainWindow::saveBookmark permits only Filesystem/Viewing and excludes nested
archives, then saves directory_ or archiveDisplayPath. Drive/network/stream
views are State::Namespace: drive Save can be enabled yet silently return,
while separate root and nested Save are disabled centrally. Virtual archive
prefixes are stable source locations, not materialized FilePath values, so the
temporary-path exclusion does not justify forbidding their virtual bookmarks.
Current empty-slot Open is disabled and its handler returns for an empty path,
whereas source BindToPathAndRefresh receives that path and reaches root namespace.
MainWindow openBookmark/showFoldersHistory use typed-address navigateTo rather
than the source-address mode already used by the hierarchy dropdown. That mode
joins relative input to the current disk directory and decodes the Qt ` / `
separator; it must not reinterpret stored source prefixes or valid literal
space-containing path components. History publication also excludes namespace
and nested paths through the existing displayed-path/history policy. Existing
mock bookmark/list tests do not prove these source consumers. Original binding,
state/availability, persistent source locations, previously saved Qt display
entries, both panels, nested return/restart, input preservation and geometry
must be reconciled together before accepting the new component. No functional
code is changed by this review.
The subsequent P3-05.14 integration reconnects those consumers to source-prefix
storage/history and source-address binding, including original empty-slot and
open-chain/cold fallback semantics. The static state-service boundary is retained
for Qt publication; current validation and platform limits are in the port audit.

Review on 2026-10-05 also found an active two-panel omission. Upstream
`FileManager/PanelKey.cpp::CPanel::OnKeyDown` invokes `OnSetSameFolder` for
Alt-Up and `OnSetSubFolder` for Alt-Left/Right. Upstream `App.cpp` binds the
destination panel to the source prefix or its focused folder, including the
parent/provider branch, then refreshes the destination. At that review the
adapted callbacks delegated to a null Qt presentation lookup and the key adapter
had no Alt-arrow branches. P3-05.4 now ports those original bodies and callers,
with per-panel asynchronous publication and public regressions; current
acceptance is recorded in the [port audit](upstream-port-audit.md#p3-054--samesubfolder-source-callbacks-accepted-callback-scope-2026-10-06).
The RefreshListCtrl selection stage and actual Qt row publication remain
separate presentation phases, now connected for the destination without
canceling its work when source focus is restored.

Review on 2026-10-06 identifies another active body gap, owned by P3-05.5:
original Agent.cpp::GetArcProp resolves kpidErrorType from codec names for both
an opened CArc and the final NonOpen level, and kpidType calls GetTypeOfArc.
At discovery Agent.cpp omitted both error-type branches and used one ArchiveType;
Agent.h::GetTypeOfArc likewise returned that transport type. Native opening now
retains an exact catalog and CArchiveLink, so the old unavailable-codec comments
do not establish an N/A disposition. QtArchiveFileResource::ConfigureArc maps
FormatIndex only when a caller supplies CCodecs; it does not carry native
ErrorInfo/NonOpen metadata. QtArchiveBatchOpen and Agent::Init have different
index-owner boundaries. PanelMenu property levels and FileFolderPluginOpen
warnings consume these properties; source_properties currently checks nonempty
type and synthetic NonOpen flags, not real rejected-format diagnostics. The
required component must review original bodies, native link levels, catalog
lifetime/replacement and process-only fallback together before adapting them;
copying indices between catalogs is not a valid repair. P3-05.5 now adapts the
original property bodies against the retained native link/catalog, including
parent/NonOpen levels, with a process-only fallback. Native interface/module
lifetime and accepted partial-chain transport corrections pass their consumers
on3OS; see [the port audit](upstream-port-audit.md#p3-055--native-archive-property-levels-and-codec-owners-accepted-property-scope-2026-10-06).

Review on 2026-10-06 also confirms an active filesystem comment gap (P3-05.6).
Original FSFolder.cpp SaveComments serializes CTextPairs to UTF-8, preserves
existing file attributes, writes descript.ion and invalidates the read cache;
SetProperty implements kpidComment for direct children, including empty/BSTR,
trim/delete and invalid index/type branches. The adapted SetProperty still
returns E_NOTIMPL for every call and omits SaveComments. Original
PanelOperations.cpp ChangeComment and MyLoadMenu.cpp permit writable filesystem
folders; the current CPanel::CanComment accepts only the archive capability.
The writer, capability and actual command/dialog consumers need an integrated
source port. Existing read-side comment tests do not prove editing. Original
GetPrefix/GetItemFullSize candidates in this review are inside comments and do
not create additional functional gaps. A preliminary qualified-name comparison
only locates candidates; it is not a body-completeness verifier.
P3-05.6 now restores those original writer bodies and the shared capability;
direct property and actual File Comment/Ctrl+Z consumers pass on macOS, including
failure policy, Cancel and normal/compact EN/RU reports. Platform/full evidence
is maintained in [the port audit](upstream-port-audit.md#p3-056--filesystem-description-comments-2026-10-06);
this does not close the remaining body/resource/dependency reconciliation.

Review on 2026-10-06 confirms the original FSDrives device-image branch was
active and mandatory: LoadItems physical enumeration, GetFileSize, CopyTo and
the native FileIO/FileFind Z7_DEVICE_FILE dependency. P3-05.7 now restores those
provider bodies and actual F5 destination consumer; direct6/UI4 old-product RED
becomes native focused GREEN, with full gates pending. See the port audit.
The provider's new operations interface must not enable unsupported consumers:
P3-05.8 connects the original PanelCrc/CApp consumer to operations-capable
filesystem namespaces. Its native old-product regression confirms disabled
logical/device Hash and no-op ADS dispatch; integration now passes ADS and
availability; the later source CInFileStream adapter resolves private VHD
end-of-device errors. Final native affected9/full256 and Mac full249 pass;
Linux final checks and parent acceptance remain pending. Logical-drive Copy stays disabled: original CopyTo
refuses non-device mode and the Qt namespace dispatcher has no such consumer.

Its new current-build EN/RU Copy reports also expose an existing presentation
gap: the Russian label is translated but the standard Cancel footer remains
English. Original LangUtils.cpp::LangSetDlgItems always maps standard control
IDs through kLangPairs (IDOK401/IDCANCEL402 etc.) even when no dialog-specific
IDs are supplied. At discovery CopyDialog constructed Qt standard buttons
without that source language binding. P3-05.9 now accepts the shared standard
mapping and twelve dialog consumers with initial/reload/geometry cases on3OS;
see the port audit. Qt's own translation fallback does not establish parity
with the selected 7zFM language. The device Copy component did not itself
correct this separate presentation gap.

Further LangUtils body/caller review on 2026-10-06 identifies a concrete
caption gap, P3-05.10. Original BenchmarkDialog::OnInit applies
LangSetDlgItems_RemoveColon to IDT_BENCH_SPEED after the ordinary language pass.
The adapted OnInit retains that block behind unused Z7_LANG, while the Qt
resource binding used the translated string unchanged. Discovery macOS EN/RU
normal/compact widget reports show English Speed but Russian «Скорость:»;
the pinned original removes that trailing colon. The original colon,
remove-colon and window helper bodies and actual reload/fallback consumers
are now integrated into initial/reload bindings with original missing-entry RC
fallback. Benchmark, Edit and Progress use those helpers; plain Benchmark/Link/
Progress standard controls use the existing source table. Mac regressions/full
pass, followed by final native consumers/full; Linux and parent acceptance remain open. See the
port audit for bounded evidence. P3-05.9 alone did not close this omission, and
these helper consumers alone did not establish whole-module reconciliation.
The subsequent complete LangUtils.cpp/.h body/dependency review is recorded in
the port audit under P3-05.10: original load/reload/default lookup, Windows locale
table, all caption helpers and ten active window callers have explicit owners;
Qt directory/locale/synchronization/controls and Unicode-only build boundaries
are recorded. This closes that module's body disposition, while final Linux
runtime and reconciliation of the other UI modules remain required.

Review on 2026-10-06 also finds an active RootFolder caller omission, P3-05.11.
Original CRootFolder::BindToFolder(const wchar_t *, ...) normalizes the path,
tries CFSFolder::Init, then creates CNetFolder/Init(name2) on Windows when that
fails for a separator-led path. The adapted branch returned E_NOTIMPL with an
old P1-11 comment. Existing native CNetFolder::Init uses original
GetResourceInformation and retains enumeration/parent services, so an unavailable
implementation is not a reason to omit this caller. Accepted own-share
enumeration/binding did not prove server/container address fallback. The original
branch and actual typed-address/flat/parent/refusal consumers are now integrated
with an owned local native fixture. The regression exposed an unsafe Qt FS cast
and unsupported Copy availability; both use existing source type/interface
boundaries. Final native consumers/geometry pass; whole/final platform and parent
acceptance remain open. See the port audit for scope and preserved RED evidence.

### RootFolder and address-combo consumer review, 2026-10-06

Compared full original/adapted RootFolder.cpp/.h and the original
PanelFolderChange.cpp address handlers, including their live callers. Root
count/name/directory properties, name matching, root/parent properties and
folder binding retain source bodies. Windows keeps four ordered root entries;
POSIX adds mounted volumes/network to its original Computer→`/` entry, as
recorded by P1-11. GetMyDocsPath replaces the SHGetSpecialFolderPath platform
service with QStandardPaths and preserves source path normalization. NT4 dynamic
lookup, ANSI fallback and Windows CE build transport are outside supported Qt6
desktop targets. Root icon indices are currently `-1`; Qt renders generic
namespace icons rather than using source CSIDL helpers. This presentation
boundary is incomplete for the address consumer, not proof of icon parity.

The ADS failure branch originally differed: RootFolder::BindToFolder converts
failed CAltStreamsFolder::Init to E_INVALIDARG, while the adaptation exposed the
native HRESULT. The2026-10-07 integrated namespace/Copy component restores the
original conversion. Its public Windows regression first returned0x80070002;
it now checks a missing Unicode/space base path, null result and no file creation,
then creates the base/named stream and retries with exact base/stream bytes.
The [dated audit](upstream-port-audit.md#p3-0515--native-ads-root-source-zone-copy-and-bound-filesystem-reload-2026-10-07)
owns current platform evidence and the still-open whole source acceptance.

The larger active omission is owned by P3-05.12. Original
PanelFolderChange.cpp::OnComboBoxCommand(CBN_DROPDOWN) constructs the current
path hierarchy with indentation, UNC server prefix, source archive-link
boundaries, Documents/Computer, logical drives and Network. AddComboBoxItem
keeps ComboBoxPaths separate from displayed text; CBN_SELENDOK binds that path
and returns focus to the list. LoadFullPathAndShow supplies the current address,
history and icon; return/escape handlers preserve commit/restore semantics.
RootFolder_GetName_Computer/Network/Documents are active dependencies of these
handlers, not unused source functions that can be dismissed by name search.

At discovery, MainWindow::refreshPathHistory instead put FolderHistory strings in
QComboBox, and activation navigates to itemText; it has no source path hierarchy,
indentation, virtual entries or separate ComboBoxPaths. MainWindow's visited
history and the accepted FoldersHistory dialog remain valid separate consumers,
but they do not replace the original address dropdown. Existing Enter/parent
tests did not exercise CBN_DROPDOWN/SELENDOK. The integrated implementation
now reuses these original bodies and source folder/path owners with Qt combo,
icon, drive/mount and focus services. Disk, ZIP/nested ZIP, both panels, EN/RU,
activation/escape/input preservation and normal/compact JSON have public Mac
coverage; source history stays in FoldersHistory. Source Enter bypass and stale
posted focus were corrected through the original handler and a narrow Qt focus
delivery guard. Native Windows UNC/device consumers, ADS refusal contract and
whole/platform/rendered acceptance still require validation; see the port audit.

The separate LoadFullPathAndShow current-edit-image omission is now implemented
through its original path/root/device/archive-link/extension classification and
GetRealIconIndex_for_DirPath/Find_FileExtension_DotPos_in_path helpers. Original
PanelItems::RefreshListCtrl and address-selection callers establish when this
stage runs; Qt disk/archive/namespace renderers supply the same panel boundary.
QFileIconProvider/Qt standard icons and a leading editor action replace native
shell image-list/ComboBoxEx services. Existing displayed-path formatting and
nested-history exclusion are explicit arguments, while the original native
prefix remains authoritative for classification. CE edit-image omission is kept;
Win32 device/super-prefix branches are retained on Windows. Current platform and
rendered proof remains bounded by the port audit, not this body review.

Stale hierarchy selection now has EN/RU mouse/Return public coverage for original
deepest-existing-folder fallback and posted list focus. A retained window focus
widget repairs the Qt popup activation boundary without overriding newer peer
intent; see the port audit for actual RED, failed broad-null prototype and gates.
The review also found active OpenDrivesFolder missing while its App.h/Panel.cpp
callers routed to the distinct root namespace. P3-05.13 now copies that body and
the original App.h inline caller, integrating command/character consumers with
the existing drive provider. The similar commented PanelKey branch must not be
mistaken for absence of the active Panel.cpp character route. Native/runtime,
remaining body/platform dispositions and final acceptance stay open; the port
audit owns the current validation scope.

The later toolbar review restores the active `CPanel::ExtractArchives` and
filesystem `TestArchives` branches, `CApp` delegates, and their real Qt consumers.
Archive Extract now reaches the original OnCopy; filesystem batch commands reach
CompressCall2/ExtractGUI. ExtractDialog restores original ArcPath, removes its
independent hint/selected-scope restriction and retains source initialization
after caller configuration. The explicit extraction integration API is still a
documented adapter, not an alternative toolbar owner. The corresponding
[source audit](upstream-port-audit.md#p3-05--toolbar-extracttest-source-consumers-2026-10-07)
owns platform and regression evidence; this does not close the whole Panel module.

P3-05 remains open. The selected ClassDefs/native SDK boundary is recorded above;
every claimed full source body and resource,
desktop-versus-CE branch, standalone console/GUI consumer and custom Qt algorithm
must be reconciled against its owning functional audit. The source entrypoint gap
found by this review is accepted in the bounded P3-05.1 caller component.
Engine/Qt packaging, notices and own-code licensing belong to P3-04; final
rendered/native UX acceptance belongs to P3-06.

### Agent update enumeration review, 2026-10-07

Complete original/current AgentOut and its declarations/callers exposed an active
omission: `DoOperation` replaced the original alternate-stream item loop with
E_NOTIMPL, and removed the Windows filesystem-colon-prefix guard. Existing
DirItem/FileFind/EnumDirItems and UpdateCallback already support that Windows
branch. It is now restored, with a narrow Unix callback metadata adapter because
the source FileInfoBase flag is Windows-only. Public source CopyFrom transactions
use the official WIM writer and preserve source directory refusal, failure and
cancellation behavior; see the
[bounded source audit](upstream-port-audit.md#p3-05--source-agent-alternate-stream-updates-2026-10-07).
The seven Agent bodies and associated headers were compared; the following
disposition accounts for wrappers, empty-agent/FAR versus desktop entrypoints
and dependency ownership. Native desktop and final whole-project acceptance
remain open. An absent platform member is not sufficient evidence to omit
supported archived ADS behavior.

### Agent desktop body/header/dependency disposition, 2026-10-07

Compared the seven original/adapted translation units and Agent.h, AgentProxy.h,
IFolderArchive.h and UpdateCallbackAgent.h at the pinned revision. Original
FileManager/FM.mak lists all seven in AGENT_OBJS; 7zip.mak resolves these objects
to UI/Agent source paths. Far/makefile selects the shared six bodies without
ArchiveFolderOpen, establishing a separate consumer boundary rather than making
the archive factory unused. The following is source disposition, not native
desktop/rendered or complete109-module acceptance.

| Module under `CPP/7zip/UI/Agent/` | Active source bodies and current boundary |
| --- | --- |
| `Agent.cpp` | Folder enumeration/flat mapping, property catalogs, path/tree comparators, binding/parent/ADS, raw properties, real-index expansion, stream/extract, Agent open/reopen/close/read/bind and archive-level properties retain source algorithms. Current GetRealIndex is reused by raw projection instead of duplicating its original mapping. Typed engine metadata/open/extract services, public range guards, initialized failure outputs and partial-proxy retry cleanup are explicit adapters. Opening/lifecycle/CRC audits own the identified corrections and their consumers. Original commented EnumProperties stays inactive. |
| `AgentOut.cpp` | SetFolder/SetFiles/SetProperties, archive enumeration, DoOperation planning, CommonUpdate, Delete/CreateFolder/Rename/Comment/UpdateOneFile use source bodies. GUI pathmode, filesystem links/metadata choices, input lifecycle/mtime and the Unix archived-ADS flag are typed integrations with original common update services. Property-value storage has scoped ownership. Original Windows ADS normalization/item marking and directory refusal are restored. FAR output API/empty-agent creation are distinguished below. |
| `AgentProxy.cpp` | Both path and parent-index proxies retain source load, names, directory/ADS mapping, aggregates and ordering. The existing BSTR-lifetime correction moves the local PropVariant outside its branch so its pointer survives until AllocStringAndCopy; it does not replace the algorithm. Headers retain source proxy types. |
| `ArchiveFolder.cpp` | Zone mode/buffer and CopyTo bodies are retained, including original move refusal, callback QI, normal/flat pathmode choice, overwrite mode and Extract forwarding. The application composes Move through the existing source Copy/Delete transaction; it does not add a second Agent Move implementation. Commented SetReplaceAltStreamCharsMode remains inactive. |
| `ArchiveFolderOpen.cpp` | Codec/icon loading, extension/icon lookup and OpenFolderFile retain source decisions, S_FALSE diagnostics and binding progress/lifetime. A retained catalog/open service replaces process-global codec ownership. QtArchiveFolderManager exposes IFolderManager and delegates to this source factory. Windows uses resource modules; Unix uses the compiled Format7zF string table. Commented FindFormat/FolderReOpen/GetTypes/CreateFolderFile stay inactive. |
| `ArchiveFolderOut.cpp` | Original transaction, operation switch, prefix/tail stream, temporary publication, Move callbacks/reopen, processed-source deletion, saved path/ADS folder restoration and error paths are retained. Unix nofollow directory service, owned-temp adoption, output mtime, deleteInputs and GUI action/pathmode bridges are documented adapters. Original unsupported CreateFile has its sole identical E_NOTIMPL body in qtlocationfolder.cpp; no working CreateFile behavior was omitted or invented. |
| `UpdateCallbackAgent.cpp` | Original progress/scan/update/error/stream callbacks and delegation are retained; only include ownership and portable UInt64 spelling differ. Native/process progress transport supplies services through the retained callback interfaces. |

The Agent.h review also accounts for original constructor/default state moved to
member initializers, inline bodies moved out of line, retained folder interfaces
and capability-gated Qt operations, and the new private engine/open-service
lifetime. IFolderArchive.h retains original interface GUIDs/signatures, including
the unused-by-desktop IOutFolderArchive declaration. App-only
IArchiveFolderInternal adds GUI action/pathmode methods after GetAgentFolder;
official codec ABI is kept on IInArchive/IOutArchive, not this Qt UI interface.
No binary compatibility with a separately built original FAR/UI plugin is claimed.

Original IOutFolderArchive is queried by Far/PluginWrite.cpp and PluginDelete.cpp.
PluginWrite's create path constructs an empty CAgent, sets files, and supplies a
codec/index to DoOperation. Agent::CanUpdate explicitly documents this FAR use.
Original DoOperation2 only delegates to DoOperation with g_CodecsObj/-1; the
desktop CommonUpdateOperation adapter calls the same body directly, capturing
requested/processed paths for the accepted input-lifecycle service.
GetErrorMessage's only external original caller is Far.cpp::OpenPlugin.
These omitted FAR-facing wrappers and empty-agent writer creation are not claimed
as implemented APIs. The desktop Add/create owner remains the separate original
CompressCall→GUI/UpdateGUI→Common/UpdateArchive route; restoring a parallel create
algorithm in Agent is not evidence of that desktop workflow. Final whole-project
scope/entrypoint acceptance must retain this distinction explicitly.

Dependencies are the original portable strings/variants/COM and archive
interfaces, OpenArchive/LoadCodecs, ArchiveExtractCallback/Extract, DirItem/
EnumDirItems, UpdatePair/UpdateProduce/UpdateCallback, file streams/CopyCoder,
WorkDir/file services, source folder/progress interfaces and the existing Qt
backend/native-codec resource adapters. CMake has one source owner for each
Agent module; the manifest pins each adapted/exact dependency. Full dependency
and source-language/license dispositions for these common owners remain in
their own audits, not inferred from Agent's successful build.

The new real F5/F6 consumer additionally found a Qt publication omission after
Agent updates: old rows queried a rebuilt proxy during destination activation.
Original OnCopy's notification guard is restored at this switch for FS→archive,
archive→archive and rollback. Its
[source audit](upstream-port-audit.md#p3-05--source-copymove-publication-after-agent-updates-2026-10-07)
owns actual RED, saved-state/rollback/byte/geometry and platform checks. Other
module groups, native desktop input, rendered references and packaging remain
separate required P3 scope.

### GUI dispatcher, workers and update callbacks, 2026-10-07

Compared complete original/current GUI.cpp, ExtractGUI.cpp, UpdateGUI.cpp,
UpdateCallbackGUI.cpp/.h and UpdateCallbackGUI2.cpp/.h, including their original
headers, GUI/makefile/FM.mak owners and Qt entry/service callers. Normalized
read-only comparisons separate formatting from functional adaptations. This
records these five GUI bodies; CompressDialog, ExtractDialog, HashGUI and
BenchmarkDialog still have their own required body/resource reconciliation.

| Module under `CPP/7zip/UI/GUI/` | Source disposition |
| --- | --- |
| `GUI.cpp` | Main2 retains Parse1/question flag/Parse2, format and excluded-format validation, archive enumeration, Benchmark/Extract/Update/Hash options/callbacks/censors and result policy. SevenzipGUI retains WinMain's exception-to-exit-code bodies and Windows DLL-security call. Qt owns QApplication/argv on Unix, language startup and delayed service factory at original codec construction. The retained backend catalog replaces duplicate Load/AddHash and process-global external providers. Original ComCtl32 version/InitCommonControls and old Windows NT/CE entry adaptation are distinct from archive behavior; Qt widget/platform startup replaces those presentation requirements. Native COM/taskbar/desktop and installed helper acceptance remain required, not inferred from the dispatcher or this review. Commented result/debug branches remain inactive. |
| `ExtractGUI.cpp` | Source AddValuePair/AddSizePair, worker Extract dispatch, test/hash result construction, dialog option roundtrip, path validation, callback lifetime/init, progress creation/join and messageWasDisplayed/result ordering remain. Qt adapts dialog fields/exec, native separators, result parenting and presentation. Typed archive opener/progress transport retain backend ownership. Both original incorrect-output paths again call the source MyMessages question policy, through its Qt presentation. Source commented CreateComplexDirectory/hash-init/external-codec blocks stay inactive; selected official SFX bundles build their own unchanged source, not this Qt translation unit with Z7_SFX. |
| `UpdateGUI.cpp` | Source worker/error info, ParseProp_Time_BoolPair/ParseProp/ParseProperties, AddProp helpers, SplitOptionsToStrings/IsThereMethodOverride/ParseAndAddPropertires/SetOutProperties, action-set mappings, ShowDialog and UpdateGUI phases remain. Extracted PrepareFormatSelection/working-dir/property/action-set accessors expose the same source decisions to Qt consumers, with one owner for the underlying routines. QString password/encryption fields, typed modal/codecs/update services and platform SFX path/extension are explicit adapters. The missing No Update Engines branch is restored at its original ShowDialog phase. Original commented legacy cwd, dict-chain, volume-refusal and parent-title blocks are inactive. |
| `UpdateCallbackGUI.cpp` | Every active original method body is unchanged after formatting; only notices/include ownership differ. Scan/open/read error accumulation, cancellation, counts/ratios, operation labels, password flags, processed-file/Move/SFX callbacks retain source ordering/HRESULTs. Finalize/CloseProgress/extra password helpers remain commented. Interface declarations and defaults are retained. |
| `UpdateCallbackGUI2.cpp` | Init, operation lookup, Move status/start/progress/finish bodies retain source language IDs, percentage overflow threshold, rounding and CheckStop behavior. Only ShowAskPasswordDialog uses the existing joined GUI-thread password adapter instead of Win32 Create; E_ABORT and password state updates remain source policy. Header differs only in notices/formatting. |

Original `Explorer/MyMessages.cpp/.h` supplies the error suppression rule:
ShowErrorMessage does nothing when g_DisableUserQuestions is true, otherwise
shows 7-Zip/OK/stop icon. Its native HWND/resource/last-error functions remain
Windows-only. ShowErrorMessageQt is a narrow QWidget/QMessageBox Critical
presentation of that same rule in the same source owner; it adds no second
question flag or operation state. Existing GUI.cpp's native error/exit policy
and in-process CompressCall2's separate caller error behavior are retained.
The [error-policy audit](upstream-port-audit.md#p3-05--source-gui-early-error-policy-2026-10-07)
owns regression, geometry and platform evidence for the two reached omissions.

Dependencies remain source Common strings/variants/CommandLineParser/ListFileUtils,
ArchiveCommandLine, LoadCodecs/OpenArchive, Extract/Update/Hash/Bench workers,
source callbacks/options/progress/language/resources, FileDir/FileName/WorkDir,
and typed Qt backend services. ArchiveCommandLine's previously audited argv
fallback omission and shared ClassDefs GUID owner remain explicit boundaries;
successful builds do not establish all common dependency bodies or the other
GUI dialogs. No upstream resource, license, codec ABI or archive algorithm changes.

### GUI Extract dialog and Hash worker, 2026-10-07

Pinned source https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Complete original/current bodies
and headers were compared, with formatting separated from adaptations.

| Module under `CPP/7zip/UI/GUI/` | Source disposition |
| --- | --- |
| `ExtractDialog.cpp/.h` | Source mode/language tables, AddComboItems/AddUniqueString, BOOL-pair precedence, split/init/history, force/default flags, password toggle, Browse, OnOK and Help ordering retained. UString Trim/FileName normalization and shared original history comparison replace the independent QString services. Qt constructs the desktop ExtractDialog.rc controls, adapts field/settings storage, typed-text restoration, caption at showEvent, language changes, destination preview and backend option projection. Original caller supplies modes/caption; safe Qt defaults do not replace that caller policy. Native icon/position/WM dispatch is QDialog presentation; Unix file-security control remains explicitly disabled. CE/NO_REG/SFX build branches are not newly claimed Qt products; selected official SFX bundles retain their separate source owners. Commented AltStreams/files-mode branches remain inactive. |
| `HashGUI.cpp/.h` | All active worker/callback/hash option, digest/result, progress/cancellation and HRESULT bodies retain original control flow. The complete normalized difference is includes/notices and QWidget parent plus CListViewDialog::OnInit/exec instead of native Create. Original HashCalcGUI S_OK return, thread Result/E_ABORT handling and callback lifetime remain. Hash results use ListViewDialog.rc's existing result presentation owner; this review adds no hash algorithm or result policy. |

Dependencies remain original CHashOptions/Hash/LoadCodecs, progress/thread,
source language/resources, portable strings/variants and FileDir/FileName,
NExtract::CInfo and typed backend/native-codec services. GUI and FM makefiles
and Qt production consumers were inspected. Full common dependency disposition
is still separate required scope. CompressDialog's reviewed path/history bodies
are covered by the [consumer audit](upstream-port-audit.md#p3-05--source-gui-path-and-history-consumers-2026-10-07);
its remaining methods and BenchmarkDialog are not accepted by this record.
Hash runtime uses hash_results/native_hash_workflow; source review and current
Qt geometry do not establish native desktop input or rendered Windows parity.

### GUI Create validation transaction, 2026-10-07

Pinned source https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`: CompressDialog.cpp::OnOK and
OnButtonSetArchive, Explorer/MyMessages.cpp/.h, language/resource dependencies
and current Qt Create/encryption/memory/volume/backend callers reviewed.
Password checks include disabled controls; memory uses the original fallback
limit; SaveOptionsInMem precedes path/volume checks. Resource errors retain the
original LangString/numeric-ID fallback and g_DisableUserQuestions in MyMessages,
with QWidget/QMessageBox as the presentation adapter. Direct memory errors and
small-volume questions remain unconditional. No archive algorithm, duplicate
question flag, validation owner or persisted setting is introduced.

The [validation audit](upstream-port-audit.md#p3-05--source-create-validation-error-policy-2026-10-07)
owns actual RED, recovery, geometry and same-source platform evidence. Whole
CompressDialog acceptance remains open. Its source profile-to-combo default/clamp
selection is subsequently reconciled in the
[profile audit](upstream-port-audit.md#p3-05--source-compression-profile-selection-2026-10-07),
including non-exact stored values, large markers and unavailable explicit threads.
Remaining whole-module headers/control-service/omitted-branch review still includes
Copy dictionary display/effective-zero and unknown-method handling.
BenchmarkDialog/common dependency disposition, native
desktop/rendered reference and package acceptance remain required separately.

### GUI compression profile selection, 2026-10-07

At the same pinned full revision, original SetDictionary2/SetOrder2/
SetSolidBlockSize2/SetNumThreads2, FindRegistryFormat, method-equality and default
selection callers were reviewed. Source dictionary threshold/clamp and selection
conditions are restored inside the existing value-generation loops. Order keeps
its first-entry fallback; solid retains its distinct index condition, full and
non-solid branches. QVariant item insertion is replaced by table append plus
selected index in the existing Qt boundary snapshots. No independent nearest
selector is introduced. Qt MethodChanged forwards saved values only for source
IsMethodEqualTo, and explicit unavailable threads retain no selection. Format
lookup again uses UString::IsEqualTo_NoCase at QString storage boundaries.
Commented ZSTD/dictionary-chain code remains inactive in this source baseline;
Windows rectangle/label resizing remains the existing Qt form-layout adaptation.
The profile audit owns RED/runtime/geometry evidence. This disposition covers
the saved-value branches, without accepting unreviewed whole-module behavior.

### GUI compression method/control states, 2026-10-07

At the same pinned full revision, original SetDictionary2/SetOrder2/
SetSolidBlockSize2/SetNumThreads2, GetDict2/GetDictSpec, GetMethodID/GetMethodSpec,
FormatChanged/EnableMultiCombo, SetMemUseCombo and memory bodies, callers, types
and desktop/CE resources were reviewed. Numeric -1/external IDs now reach source
calculations directly. Copy keeps its single zero dictionary without Auto;
unknown methods retain the all-ones unknown dictionary. Unsupported selectors
remain empty and the MemUse format flag controls the complete memory rows.
Store retains original no-method thread policy through backend validation.
Source loops and algorithms remain authoritative; Qt adapts item insertion,
enabled state and joint visibility. This supersedes the Copy/unknown/control
omissions above. Whole-module final reconciliation, BenchmarkDialog/common
dependencies and native/rendered/package acceptance remain required. The
[method-state audit](upstream-port-audit.md#p3-05--compression-method-and-control-states-2026-10-07)
owns regression, geometry and platform evidence.

### GUI Benchmark complete body/header/resource disposition, 2026-10-07

Pinned https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`: complete BenchmarkDialog.cpp/.h
comparison separates formatting from functional changes; all original active
bodies, desktop/CE/total RC, GuiCommon definitions, Qt controls/services and
GUI.cpp/CompressCall2/QtFileManagerCommands callers were reviewed.

| Source scope | Disposition |
| --- | --- |
| CBenchPassResult/CTotalBenchRes2/CSyncData/CBenchProgressSync | Original result accumulation, flags, limits, critical-section state, text and rating vector remain. Existing CheckBreak lock makes the shared Exit read synchronize with SendExit; it does not change E_ABORT policy. |
| OnInit and dictionary/thread/pass/default/memory selection | Original tables, rounding, limits and RAM refusal retained. Windows uses original System.h normalization. POSIX copies that exact normalization around a typed CProcessAffinity query, including failed/zero results and system>=process counts. The service only queries the original portable API. |
| Metrics/UpdateGui/time/frequency/log/result helpers | Original units, thresholds, aggregate/current/resulting values, formatting and rating-vector removal order remain. Unicode Qt strings and QElapsedTimer replace TCHAR/lstrcat/global tick presentation. |
| Restart/Stop/Cancel/OnMessage/OnTimer/OnCommand | Original restart deferral, SendExit, completion ordering, HRESULT/error reporting, worker Wait_Close and close-after-join remain. QTimer and queued QObject invocation replace Windows messages. Detached completion remains a successful no-op, preserving original PostMsg_Finish's null-window branch. |
| CBenchCallback/CBenchCallback2/CBenchFreqCallback/CThreadBenchmark | Original progress/frequency/encode/decode callbacks, properties, pass loop, exceptions, result fields, dispatch order and finish messages remain. Typed services call the original Bench; the official engine owns calculation. |
| Benchmark entry and destructor | Original property parsing, total-mode selection, defaults, provider assignment, Create and S_OK return remain. QWidget/modal exec replace HWND; optional services have scope covering the joined worker. Exceptional destruction disables posting, requests Exit and joins before QObject/base destruction; queued calls cannot outlive their receiver. |
| Presentation/header/resources | Original names, module path, source language and notices retained. Qt widgets/layout/scrolling, fixed system font, resource text binding, Help and standard buttons replace native controls/rectangles/font. Original CE presentation is not a separately claimed Qt product. PRINT_ITER_TIME and commented debug/legacy parse branches remain inactive. |

Dependencies remain original Bench/MethodProps/CreateCoder/Property, strings and
numeric conversion, MyException, System/SystemInfo/CpuArch, Thread/Synchronization,
language/help/resources and native codec providers. This closes Benchmark's body
disposition; complete common-module reconciliation, CompressDialog final event/
header disposition and native/rendered/package acceptance remain separate required
scope. Actual regression/runtime/geometry evidence belongs to the
[Benchmark audit](upstream-port-audit.md#p3-05--benchmark-source-and-platform-lifecycle-2026-10-07).

### Common ZipRegistry persistence body/header disposition, 2026-10-07

Source: https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h`. Complete active bodies, declarations,
Registry services and Create/Extract/WorkDir/ContextMenu consumers were compared.

| Source scope | Disposition |
| --- | --- |
| Registry key and synchronization services | Explicit application INI path and QSettings replace HKCU/7-Zip/CKey. Source g_CS now serializes complete Load/Save operations, including profile-group replacement, using QMutex. DWORD conversion validates its full range before narrowing; Qt bool text is also accepted because it is the adapter's own serialized representation. |
| NExtract CInfo and password/memory-limit functions | Source mode bounds, forced flags, default-true SplitDest, default-false metadata/password BoolPairs, history and defined-only writes remain. Undefined Extract pairs preserve previous keys. The all-ones memory limit deletes its key. Legacy Qt history remains a one-way read fallback. |
| NCompression CInfo/CFormatOptions | Source Level=5 default/global field, per-format fields, reset rules, history, source traversal and metadata defaults remain. Undefined compression pairs and all-ones per-format DWORDs delete keys; empty profile strings delete keys. Only the owned options group is replaced. INI cannot represent an empty subkey; an entirely default profile has no stored group and FindRegistryFormat_Always supplies its source defaults on use. |
| CMemUse/ParseMemUse | Original parser, overflow checks, suffix/percent rules, clear/default and byte calculation remain; QString-to-wide conversion replaces the native string boundary. |
| NWorkDir/CContextMenuInfo | Original mode/path fallback, removable-only default, context BoolPair defaults, flags and Zone sentinel policies remain. Invalid stored booleans keep their respective source defaults. |
| Inactive branches and storage differences | Commented dictionary-chain, MemLimit_Save/Load and email-mask alternatives remain inactive at this baseline. Registry prefix/open/create, recursive history keys and write-if-changed optimization are storage services; INI groups/string lists replace them. Portable upstream MethodProps/CBoolPair, strings, vectors, ExtractMode and numeric conversion remain dependencies; no archive algorithm is introduced. |

This receipt covers ZipRegistry's persistence owner. Other Common modules and
CompressDialog event/header reconciliation retain their separate required scope.
Regression/platform evidence belongs to the
[settings audit](upstream-port-audit.md#p3-05--source-settings-persistence-2026-10-07).

### Compress event/cache phase disposition, 2026-10-08

At the same full pinned revision, original OnCommand, SetNumThreads2,
SetMemoryUsage/GetMemoryUsage_DecompMem/GetMemoryUsage_Dict_DecompMem, OnOK and
header GetNumThreads2 were compared with their Qt consumers. Solid and explicit
thread events only refresh memory; dictionary and memory-limit events save the
current profile and regenerate threads. The original cached _auto_NumThreads and
getter condition now supply both displayed memory and OK refusal. Qt adapts the
combo read to a source spec, assigning the cache in the source selection phase.
No engine algorithm or NumThreads Auto sentinel changes. Regression, actual-XZ
workflow and platform evidence belong to the
[event audit](upstream-port-audit.md#p3-05--compress-event-and-memory-phases-2026-10-08).

All active COptionsDialog bodies and its header were also compared: BoolBox
defaults/support, timestamp precision flags/default/custom entry, ZIP/TAR time
capabilities, Set-checkbox events, Store_TimeBoxes, OnInit/OnOK/Help remain original
code. Qt language refresh uses AddPrec's existing text builder without changing
data/selection; the declaration is separated to avoid Widgets in Core-only DTO
consumers. Qt control adapters' sentinel/selection disposition follows below.
Remaining Compress/Common/native/rendered/package scope is not closed here.

### COptionsDialog complete control/lifecycle disposition, 2026-10-08

Source https://github.com/ip7z/7zip.git at full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`: complete COptionsDialog bodies in
GUI/CompressDialog.cpp, declaration in CompressDialog.h, CompressOptionsDialog.rc,
CompressDialogRes.h and Windows/Control/ComboBox.h/.cpp. The typed ArcInfo time
flags/defaults in IArchive.h/LoadCodecs.h and Create advanced/profile consumers
were inspected. This completes the COptionsDialog owner, not all CCompressDialog.

| Source scope | Disposition |
| --- | --- |
| Bool1/BoolBox, Store_TimeBoxes, GetComboValue | Original supported/default/defined flags and hidden-value saving remain. Only Qt checkbox/control types replace native handles. |
| AddTimeOption/AddPrec, SetPrec/GetPrecSpec, SetTimeMAC | Original flags/default insertion, custom precision, Auto sentinel, empty selection, ZIP/TAR capabilities and per-format defaults remain. AddPrec's optional text-only path supports language refresh without changing data or selection. |
| Set-checkbox and selection events | Original refresh order and dependent time controls remain; QCheckBox clicked and QComboBox activated dispatch the original handlers. |
| OnInit/OnOK/Help and header state | Original initialization, profile publication and Help topic remain. Functional bodies stay in source CompressDialog.cpp; the source declaration is separated into qtcompressoptionsdialog.h to keep Core-only DTO consumers free of Widgets. Qt's base owns show/accept/reject and window placement. |
| Native ComboBox service | Qt now preserves CB_ADDSTRING's lack of automatic selection and CB_GETITEMDATA's CB_ERR for absent/out-of-range items. Reset, explicit selection, item data and text update remain narrow control services. |
| Resource/platform adaptation | NTFS, format info, Time rows, preserve-atime and footer retain resource order. Qt supplies accessibility names, wrapping, scaling and screen placement. POSIX preserve-atime is disabled because the official engine implementation is Windows-only. Create uses a local source profile transaction and publishes only on Options OK; outer Add Cancel does not save it. |
| Inactive source | Commented default-time button, debug TAR flags, alternate default precision text and preserve-atime summary remain inactive. No alternate timestamp or compression algorithm is introduced. |

Dependencies remain portable source strings/vectors, CBoolPair/MethodProps,
ZipRegistry profiles, ArcInfo metadata, language/property names and Help service.
Regression and platform evidence belong to the
[timestamp audit](upstream-port-audit.md#p3-05--options-timestamp-control-lifecycle-2026-10-08).

### Compress format/Browse/name control disposition, 2026-10-08

At pinned full `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, the OnInit format
block retains original native-order fallback and registry/explicit priority in
CompressDialog.cpp. Qt adapts CBS_SORT with QCollator and item data; projected
display order supplies native GetItemData consumers in OnButtonSetArchive and
ArcPath_WasChanged. PrepareFormatSelection remains the sole eligibility owner.
Native callback/path/filter algorithms are retained, including excluded save
extensions, combined/all-files filters and cancellation. Hash method selection
now calls the original SetArchiveName2(false) after dependent controls, restoring
OnCommand's omitted final branch. Qt callbacks supply text/notification transport;
source naming and method/profile state remain authoritative. Original filename,
KeepName, SFX and explicit/saved-format semantics remain source algorithms.
Original GetComboValue/GetLevelSpec supplies the source DTO and saved profile;
empty/single-entry controls retain all-ones Auto. Standalone CLI effective-level
normalization remains at its separate transport boundary. This prevents emitting
unsupported `x=0` to the unchanged source HashCalc handler. Real source modal SHA1
manifest creation and TAR profile restart cover those consumers.
The platform sorting service has no portable source body: original ComboBox
sends CB_ADDSTRING to the CBS_SORT HWND. System locale supplies collation; this
is not a byte-identical Windows locale or rendered acceptance claim.
Dependencies remain source ArcInfo/LoadCodecs, strings/vectors/numeric helpers,
ZipRegistry, language and Browse services. Full source/runtime evidence belongs
to the [format audit](upstream-port-audit.md#p3-05--format-selection-browse-and-hash-names-2026-10-08).
Remaining Compress/Common/native/rendered/package scope stays mandatory.

### CCompressDialog complete body/header disposition, 2026-10-08

Source https://github.com/ip7z/7zip.git, full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, GUI/CompressDialog.cpp/.h,
CompressDialog.rc/CompressDialogRes.h and native ComboBox/Edit/Dialog services.
Complete original active bodies, inline getters/defaults, callers and current
source/Qt adapters were compared across the linked component receipts. The
separate COptionsDialog disposition above covers that class in the same module.

| Original scope | Source owner and narrow adaptation |
| --- | --- |
| OnInit, format/history/update/path/volume initialization | Native format selection lives in source OnInit; source PrepareFormatSelection supplies eligible handlers. Qt constructs controls and binds registry, CInfo, history and original enum values. RAM/CPU defaults use source GetSystemInfo; Qt supplies window placement, focus, scrolling and language binding. Native first-item and absent-selection semantics are explicit control services. |
| FormatChanged, SetLevel/SetNearestSelectComboBox, SetParams | Original format/profile reset, nearest legal level and dependent-control phases remain in the Qt control binding. Source format tables and numeric calculators own admissibility/defaults. Full format initialization is used for actual format changes; Level/SFX events no longer invoke it and overwrite current input. |
| SetMethods, SetMethod/SetMethod2, method IDs/spec/equality | Source bodies retain static/external method order, encoder/filter/stream eligibility, Auto ID, SFX filtering and keep-method branch. Native control insertion/item data is replaced by QComboBox; source method names/specs remain authoritative. |
| Dictionary/order/solid/thread/memory helpers | Original tables, numeric loops, thresholds, chunk calculation, RAM limits and Auto estimates remain in source CompressDialog.cpp. Qt inserts returned entries and projects registry selection. Source cached Auto-thread phase supplies memory display and OK refusal; specification sentinels remain distinct from effective values. |
| OnCommand, OnButtonClicked, CheckSFXNameChange | Qt signals dispatch original control-specific phases. Format saves the previous profile; Level resets method-dependent defaults; Method refreshes dependent controls and Hash naming; Dictionary and memory-limit regenerate threads; Solid/Threads refresh memory only. SFX uses SetMethod(GetMethodID()), naming and memory, retaining dependent state when the method survives. QSignalBlocker replaces absence of native selection notifications during programmatic insertion. |
| SetArchiveName/2, ArcPath_WasChanged, path splitting/final path and Browse | Source bodies own extension/KeepName/SFX/Hash naming, native-index/filter correspondence and path validation. Qt supplies text, file-picker and platform path services. Qt activated runs after text publication, so the original deferred history-message effect is applied after the format callback without recreating a Win32 posted message. Embedded NUL is refused at the Qt text boundary. |
| Encryption/password/update/volume and OnOK | Source password restrictions, memory refusal, profile saving, path/history and volume validation bodies remain. Qt owns error/confirmation presentation and reads original operation specs. Source UpdateGUI owns property generation and engine validation. Standalone CLI effective-level normalization stays at its transport boundary. Disabled POSIX sharing/NT metadata/preserve-atime and platform SFX extension are explicit adaptations. |
| Metadata/Options, bool pairs and Help | Source Combine/Set_Final_BoolPairs, per-format time state, option-summary and Help topic remain. Qt holds the Options transaction until OK and publishes its source result; outer Add Cancel never saves disk. COptionsDialog has its own complete disposition above. |
| Header/dependencies and inactive scope | Source CInfo fields, native strings/vectors/CBoolPair/ArcInfo/MethodProps and enum domains remain; Qt text/control DTOs and system-field bundles replace presentation handles. Dependencies include ZipRegistry, LoadCodecs, UpdateGUI, Wildcard, FileName/FileDir, System, language/property/help, Browse and SplitUtils. Commented ZSTD/SWFC/dictionary-chain/alternate timestamp/debug PRINT_PARAMS/PRINT_ITER_TIME and CheckVolumeEnable branches remain inactive at this pinned baseline. Desktop resources own structure; CE is not a separate Qt product. |

This closes the body/header disposition of CCompressDialog; its platform/native/
rendered acceptance is not inferred from offscreen controls. Common reconciliation
and final P3 runtime/package acceptance remain required. Component evidence is
indexed in the [control lifecycle audit](upstream-port-audit.md#p3-05--compress-control-specific-event-lifecycle-2026-10-08).

### Common owner and adaptation reconciliation, 2026-10-08

The complete pinned `CPP/7zip/UI/Common` inventory contains25 `.cpp` and30
headers. All55 have exactly one source/vendor owner; none is missing or duplicated.
Six implementation files and15 headers are byte-exact vendor dependencies.
The remaining19 implementations and15 headers were compared against the full
original files with formatting normalized for review. Changed active bodies,
header contracts and their direct consumers were read separately; unchanged
algorithm bodies are established by the complete comparison, not by path names.
ZipRegistry's full persistence disposition above and the earlier functional
component receipts remain authoritative for their changes.

Paths in this table are relative to `CPP/7zip/UI/Common`. Adapted owners are
under `src/`; byte-exact owners are under `third_party/7zip/`.

| Complete file family | Copied/adapted scope and caller/dependency disposition |
| --- | --- |
| ArchiveOpenCallback, DefaultName, PropIDUtils, SetProperties, UpdateAction, UpdateProduce `.cpp/.h`; ArchiveCommandLine.h, ArchiveName.h, DirItem.h, ExitCode.h, ExtractMode.h, Property.h, SortUtils.h, StdAfx.h, UpdatePair.h | All21 files are byte-exact. Original COM/password/volume callbacks, default-name rules, property conversion, action tables and update production remain the sole implementations. OpenArchive/Extract/Update/Agent and GUI callers reuse these types and bodies. StdAfx is header/compiler transport, not an omitted runtime module. |
| ArchiveName.cpp, Bench.cpp/.h, EnumDirItems.cpp/.h, ExtractingFilePath.cpp/.h, SortUtils.cpp, TempFiles.cpp/.h, UpdatePair.cpp | Only include owners, notices and formatting differ. Complete source naming/collision, benchmark, enumeration, path correction, sorting, cleanup and pair-comparison algorithms remain. Create/Panel/drag use CreateArchiveName; Extract callback uses Correct_FsPath/path parts; Update/Agent reuse directory items and pair production. Benchmark's platform services are dependencies of its unchanged algorithm, covered by its lifecycle receipt. |
| ArchiveCommandLine.cpp | Full parser/options body remains. The POSIX argv0/global module-prefix tail is replaced by the single shared Windows/DLL.cpp module-path service; Qt supplies the executable path because the desktop has no console argv0 initializer. Original Windows mapping/large-page/device branches remain selected by their platform definitions. GUI dispatcher uses the original parser; no second command parser is introduced. |
| CompressCall.cpp/.h, CompressCall2.cpp | Complete native process/map/event/quoting and in-process Add/Extract/Test/Hash workflow bodies were compared. Windows shell consumers retain the process implementation; owner HWND becomes null and settings path is explicit. Portable quoting has one owner. Qt shell/panel/drop consumers use typed services in CompressCall2 for catalog, GUI workers, errors and parent windows. Original TRY/return/display policy remains, including CompressFiles' S_OK after its handled exception; the service retains operation error state. Native waitFinish/process lifetime and Qt joined modal lifetime are distinct documented transports. |
| ArchiveExtractCallback.cpp/.h, IFileExtractCallback.h | Source item/path/overwrite/stream/metadata/link algorithms remain. Optional IQtExtractPathPolicy checks the already corrected final path before CreateFolders/output and checks link targets before link creation; it does not rewrite archive names or link algorithms. Complete modified GetExtractStream/CreateHardLink2/SetLink2 and CreateFolders dependency were reviewed. CQtExtractPathPolicy delegates filesystem containment to ArchiveBackend; source multi-archive and shell consumers install scoped policies. Zone helpers are also compiled for POSIX virtual-file callbacks, where a colon names a file rather than an NTFS stream. Original native security/ADS/reparse and POSIX link/mode branches remain. |
| Extract.cpp/.h, OpenArchive.cpp/.h | Full Extract loop and Open/Open2 service branches retain source errors, abort, volume deduplication, totals and nested-chain policy. Optional IQtArchiveOpenService supplies process-backed archives through ArchiveBackend. The retained catalog clears this hook before entering native Open3, avoiding recursion. SetDefaultName is the original OpenStream block shared with process projection; detection/signature/handler algorithms remain unchanged. Native ArchiveOpenCallback, FileStreams/LimitedStreams and source CArc/CArchiveLink types remain dependencies. |
| HashCalc.cpp/.h | Source hash parsing, digest comparison, extraction/test and manifest writer bodies remain. AddDefaultMethod/Extract/UpdateItems use a per-handler CExternalCodecs provider instead of a mutable global pointer; ISetCompressCodecsInfo loads the original provider interface. The catalog owns modules until handlers/streams release; its destructor breaks module SetCodecs links before unloading. No new checksum/parser implementation replaces the original. Source HashGUI and native Hash archive consumers retain the same engine providers. |
| LoadCodecs.cpp/.h | Original loading, metadata, format ordering, errors and factory bodies remain. LoadExternalModules and SetLibCodecs factor the original folder/NeedSetLibCodecs/SetCodecs phases for ArchiveCodecCatalog's immutable runtime snapshot. Load still runs them before Formats.Sort. The catalog checks deployment/version and keeps its image alive through COM release; CloseLibs clears provider links before unload. Source external-codec/export/static/SFX conditionals remain; retained old SDK/CE configurations are not claimed as executed desktop targets. |
| Update.cpp/.h, UpdateCallback.cpp/.h | Complete modified Compress/UpdateArchive and extracted SFX, maximum-MTime and post-success deletion bodies retain original ordering/checks, stream ownership and failure/abort policy. Qt volume output, AgentOut and QtArchiveDeleteInputs call these source bodies. The deletion overload validates its explicit callback/buffer boundary; it preserves source size/time/reparse checks, deepest-first directories and finish callback. Native Unix SFX retains the archive extension before .001 despite having no .exe suffix. POSIX NewDataIsAltStream carries the Agent archive-ADS flag unavailable in POSIX CDirItem; Windows keeps its original field. Original MAPI remains Windows-only; Qt services explicitly refuse unsupported Unix email rather than report success. |
| WorkDir.cpp/.h, ZipRegistry.cpp/.h | WorkDir retains original mode/removable/temp selection, CreateTempFile and MoveToOriginal. Explicit settings path replaces implicit registry location; SetMTime delegates to source COutFileStream and OwnedLease projects the existing temp-file lifetime owner. ArchiveFolderOut applies time before close/publication and adopts only an already-owned spool after success. ZipRegistry's complete body/header/default/undefined/locking disposition is recorded above; there is no second settings implementation. |

Dependency/build review includes original `UI/GUI/makefile`, `UI/Console/Console.mak`,
`UI/FileManager/FM.mak`, `Bundles/Fm/makefile` and `7zip.mak`, plus current CMake
source lists, platform definitions and interface links. Common algorithms compile
as C++; their unchanged `C/` allocation/CPU/hash/thread dependencies still compile
as C. GUI/native shell entrypoint differences do not justify omitting Common
workflow bodies. Original commented/debug/old-code branches remain inactive;
Windows security/device/MAPI, POSIX links/owner/mode and external-codec branches
remain platform-dependent source behavior, not blanket N/A declarations.

This closes Common's file-owner and adaptation disposition at this checkout.
It does not assert execution of every retained conditional branch, native desktop
input, rendered Windows parity or final package acceptance. Agent's desktop
disposition is recorded above. Remaining FileManager bodies and P3-03/04/05/06
acceptance remain open. Current validation and reproducible review evidence are in the
[Common reconciliation audit](upstream-port-audit.md#p3-05--common-owner-and-adaptation-reconciliation-2026-10-08).
