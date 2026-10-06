# Omitted source scope review

This review records module bodies and caller/dependency boundaries for P3-05.
It does not establish whole-module, platform or final acceptance from filenames.
The historical [P0-01 registry](history/p0-01-source-inventory.md) remains intact;
later functional ownership is recorded in the [port audit](upstream-port-audit.md).

Source: [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
full revision `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
The maintained per-module [manifest](upstream-baseline.json) owns reproducible
local provenance; this document does not replace its exact/adapted distinction.

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
failure and recovery. Missing-folder adaptation, async terminal and other
post-Copy failure behavior still require reconciliation;
this remains a partial integration rather than complete RefreshListCtrl closure.

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
| CApp::CreateOnePanel, Create, SwitchOnOffOnePanel | Saved path, callback binding, panel ID, state/settings/list mode/width/flat-mode ordering, main-panel creation order and source Create/binding retained. Qt presentation replaces child/menu/toolbar/window handles. The adaptation restores NumPanels on failed second-panel creation; original increments before returning an HRESULT failure. Lifecycle audits/tests own this deliberate failure-state correction. |
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
| CloseOneLevel, CloseOpenFolders | Original release/library transfer, parent-archive return, stack deletion and flat-mode restoration retained. Actual nested opening/materialization/update consumers are owned by PanelItemOpen and ArchiveBackend, not independently reimplemented here. |
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

The ADS failure branch differs: original RootFolder::BindToFolder converts any
failed CAltStreamsFolder::Init to E_INVALIDARG; adaptation uses RINOK and exposes
the native HRESULT. Existing positive root-stream tests and direct provider
refusal do not prove this root-level error contract. A Windows regression must
cover missing/deleted Unicode/space base paths, cleared result pointer and
unchanged control-file bytes before restoring the original branch.

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

P3-05 remains open. The selected ClassDefs/native SDK boundary is recorded above;
every claimed full source body and resource,
desktop-versus-CE branch, standalone console/GUI consumer and custom Qt algorithm
must be reconciled against its owning functional audit. The source entrypoint gap
found by this review is accepted in the bounded P3-05.1 caller component.
Engine/Qt packaging, notices and own-code licensing belong to P3-04; final
rendered/native UX acceptance belongs to P3-06.
