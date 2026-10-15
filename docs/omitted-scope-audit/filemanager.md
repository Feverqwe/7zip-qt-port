# Omitted source scope review — filemanager

[Индекс](../omitted-scope-audit.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

### Native associated launcher and continuation (2026-10-10)

Source https://github.com/ip7z/7zip.git, revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/BrowseDialog2.cpp` lines987–1115 and
`PanelItemOpen.cpp` lines1750–1818. This completes the previously identified
launcher body scope; the existing browser and PanelItemOpen maps below still
own their other bodies and documented platform adaptations.

| Original body/dependency | Current disposition |
| --- | --- |
| StartApplication | Restored in the same adapted BrowseDialog2.cpp. PIDL parsing/fallback, extension-dot handling, directory conversion, original ShellExecuteEx masks, process Attach, hInstApp result and no-association error branch retain original control flow. Qt QWidget replaces HWND only for the existing error presenter; empty UString replaces its NULL error-name. All579 functional tokens match after these two adapters and formatting. |
| StartApplicationDontWait | Original local CProcess/call/destructor body retained in BrowseDialog2.cpp. Disk service and owned browser use the HRESULT-returning body with the same local process lifetime, so the browser can release protection on a proved launch failure. Neither waits nor terminates the launched process. |
| ShellFolder_ParseDisplayName; NShell::CItemIDList | Reuse authoritative PanelMenu.cpp parser and Windows/Shell.cpp/.h PIDL owner, including CoTaskMemFree. No duplicate parser or PIDL implementation. |
| CProcess/CHandle; QtNativeProcessServices | Existing unchanged third_party Windows ProcessUtils/Handle remain owners of OS handle operations. A narrow Adopt transfers a returned CProcess into the existing CWinProcessHandle; its wait/status/path/snapshot implementations remain authoritative. |
| OpenFolderExternal/OpenItem; OpenItemInArchive | Disk/folder callers use the source launcher through QtAssociatedFileServices. Archive caller restores the original failure guard: reject only when both handle is absent and HRESULT failed. A valid handle enters existing CChildProcesses/MyThreadFunction even on failure. No-handle, child search, update, cancel, shutdown and temp-lease policies retain their existing owners. |
| Browser OpenOutside; MainWindow command | Windows default invokes the same source launcher. Optional explicit opener is passed through the window command and dialog; POSIX keeps Qt association because the source Windows Shell/PIDL implementation cannot execute there. The browser closes any process handle, retaining unconfirmed external protection. |

CMake explicitly selects the modern Windows Unicode body; original ANSI/CE
conditionals are retained but not claimed as executed Qt6 desktop targets.
Existing MyCom/StringConvert/Windows DLL/Shell/ProcessUtils dependencies and
shell32/ole32 linkage are reused; no new borrowed module or baseline revision.
Source language and original module organization remain unchanged. The larger
BrowseDialog2 file remains a copied source module; Qt presentation remains in
qttempbrowsepresentation.cpp. No archive syntax/format logic moved into widgets.

Current execution evidence and remaining native/rendered/delivery limits belong
to the [launcher receipt](../upstream-port-audit.md#p3-05--native-associated-launcher-and-process-lifetime-2026-10-10).

### PanelItemOpen body and platform closure (2026-10-10)

Complete original/current bodies, declarations and their process/open/update
callers were reconciled at the same pinned revision. Earlier opening, Parent,
RAM and external-edit receipts remain their execution evidence; the connected
launcher correction above closes the outstanding associated-process admission.

| Original body family | Retained owner and adaptation |
| --- | --- |
| CProcessSnapshot, My_GetProcessFileName, CSnapshotProcess/GetSnapshot | qtexternalprocess retains the OS-only seam: Windows Toolhelp snapshot, process ID/open/status and owned CProcess. QueryFullProcessImageNameW supplies the DOS form instead of device-form/Psapi global loading; CChildProcesses still owns basename/lineage decisions. POSIX has no usable Windows snapshot implementation, so existing native PID identity, reap and launch-group services supply it. |
| CChildProcesses; CTmpProcessInfo/CTmpProcessInfoRelease | Source unique IDs, handles/NeedWait, same-name/parent discovery loop and release behavior remain in PanelItemOpen. Only typed process handles/services, panel/parent pointers and owned-temp leases replace native handles/window messages/global lifetime. |
| MyThreadFunction; CExitEventLauncher::Exit | Original first-pass, bounded wait batch, two-second policy, child search, changed/read-only/update prompt/result, no-handle exit-event wait and final timestamp deletion remain. QElapsedTimer and typed status polling replace OS wait transport; QtExternalGuiCall marshals prompts/update onto the GUI thread. Panel-scoped exit joins its workers rather than detaching after original100ms hard-exit timeout, preventing access after owner release. Proven completion clears external protection; refusal/error/shutdown preserve unconfirmed copies. |
| OpenAsArc/OpenAsArc_Msg/OpenAsArc_Name/OpenAsArc_Index | Source FileInfo/virtual link, password/error, parent-folder/library ownership, flat/deleted state and opening result policies remain. CFfpOpen uses the existing retained manager, settings and owned stream materialization. Previously accepted scoped guard release permits deferred Qt publication; original unconditional Refresh result policy remains. |
| OpenParentArchiveFolder; CThreadCopyFrom; OnOpenItemChanged overloads | Source changed-child prompt, update callback/worker, password, relocated index search, selected state and unconditional Refresh remain. Typed request replaces LPARAM/SendMessage; progress transport and deferred selection publish after GUI guards unwind. Original duplicate-name ambiguity and failed-update retention remain explicit source limitations. |
| kExeExtensions/kStartExtensions/DoItemAlwaysStart; IsVirus_Message | Original extension lists, space/RLO/Windows trailing-dot checks, name sanitization and source warning remain. No alternate executable classifier was introduced. |
| SplitCmdLineSmart/StartAppWithParams/StartEditApplication; EditItem/CApp::DiffFiles | Original command split, settings/default Windows notepad, selected/peer path and error admission remain. Qt process services transport parameters/paths; existing Windows native quoting and POSIX argv replace platform invocation only. Diff's source guards and no-command return remain. |
| OpenFolderExternal/OpenItem/OpenItemInArchive | Source path construction, smart internal/external admission, hash/operation guards, stream probe, RAM limit, virtual filesystem/ZoneId, CopyTo/password/error and disk fallback remain. RAM uses the accepted immutable source record/shift guard. Timer/notification scopes hand off to asynchronous Qt publication; temp leases preserve ownership through launch/thread failure. Native launch now supplies the original process continuation, as detailed above. |

Headers/types, original FileManager makefile dependencies and current CMake owners
were checked. FileFolderPluginOpen, StreamObjects/CBufInStream, ExtractingFilePath,
FileName/PropVariant, source threads/events, UpdateCallback100 and progress remain
authoritative. Thread/process snapshots and GUI marshaling are narrow platform
services, not a second opening/update module. Original commented extension-program
experiments, old zone helper/stream/time implementations and debug output remain
inactive; CE and obsolete Windows SDK paths are not executed Qt6 desktop targets.
Current execution limits remain in the launcher receipt; a complete body disposition
does not establish every native desktop, rendered or final-delivery workflow.

## Native Copy terminal phase reconciliation (2026-10-10)

Complete pinned OnCopy, PanelCopy worker/progress/result and CPanel HRESULT
diagnostic callers retain their source order across Qt execution. Pending source
and peer publication survive until operation guards release; the final diagnostic
uses the existing source owner, with both worker/OnCopy phases preserved.
No new algorithm/module/dependency/language omission is introduced. The controlled
interface regression is separate from actual archive/stream/NTFS consumers.
The [receipt](../upstream-port-audit.md#p3-05--native-copy-terminal-phases-and-retained-guards-2026-10-10)
owns three-OS evidence and remaining native/whole-family/package/final scope.

## App Copy publication caller reconciliation (2026-10-10)

The complete pinned App.cpp::OnCopy565–858 and final load/selection callers were
revisited with all Qt execution/publication routes. CApp retains its original
saved states and RefreshListCtrl caller across the asynchronous boundary;
OnReload's separate error policy is no longer substituted. Source publication
for move/copyToSame, destination publication, guard/scan lifetime and native
worker result policy are reconciled together. Actual writer and accepted Move
rollback/reopen diagnostics remain owned by their existing operation paths.
This supersedes earlier post-Copy expected reload-message assertions, not their
retained body/provenance evidence. No module/dependency/language omission is
introduced; the [receipt](../upstream-port-audit.md#p3-05--copy-publication-caller-and-result-policy-2026-10-10)
owns validation and remaining whole-family/native/final scope.

## PanelDrag protocol body disposition (2026-10-10)

The complete original 3006-line PanelDrag.cpp, declarations, callers and
EnumFormatEtc/MemoryGlobal/Shell dependencies were compared at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
https://github.com/ip7z/7zip.git. The original Windows functional protocol lives
in adapted PanelDrag.cpp, with the native types in NPanelDragNative to coexist
with its typed Qt owner. This map records owners; validation and remaining
native acceptance are recorded in the [port audit](../upstream-port-audit.md).

| Original bodies | Current owner and narrow adaptation |
| --- | --- |
| CDataObject_TransferBase::Init_Program, GetTransfer constructor/Check, SetTransfer Init/Check, CTargetTransferInfo | Original 64-byte base and 112-byte Get/Set wire records, version/ISA/process/size/reserved fields retained for Windows. Explicit conversion bridges the portable target record, whose layout is not the wire ABI. |
| CDataObject constructor, CopyNamesToHGlobal, DuplicateGlobalMem, Medium_CopyFrom | Native registered formats, source Unicode DROPFILES and independent HGLOBAL copies retained. InitNames initializes original pre/final name storage before exposure. Typed names and shared owned-temp lease remain authoritative for Qt/POSIX consumers. |
| CDataObject::SetData, SetData2 | Original validation, UTF-16 target path, transfer checks, performed/logical effects and accepted shell presentation formats retained. TRUE consumes the medium and returns S_OK; FALSE returns the real validation result. CGlobalLock closes original early-validation unlock gaps. Transfer policy calls the existing typed SetTransfer body, avoiding a second policy implementation. |
| CDataObject::GetData, GetDataHere, QueryGetData, EnumFormatEtc, canonical/advisory methods | Original native methods retained; GetTransfer uses the one typed GetTransferFlags owner. Failed extraction is refused before returning CF_HDROP, retaining the port's existing error safety. DATADIR_GET enumerates source CF_HDROP through byte-exact EnumFormatEtc.cpp/.h. |
| CopyFromPanelTo_Folder, CDropSource::QueryContinueDrag/GiveFeedback | Existing source copy/release/control-flow owner retained. Native IDataObject restores OLE capture to the Qt source window before password/progress modal entry. Windows real execution uses native DoDragDrop; injected executors and POSIX retain Qt transport. |
| CPanel::OnDrag | Original admission, selected indices, corrected names, temp owner, callback lifetime, post-drop Copy/Move, messages and selection branches retained. Shared heap ownership replaces stack-only typed storage; a scope guard detaches CPanel on every exit so retained native data cannot call a destroyed panel. Unconfirmed native receivers retain the owned root. Explicit final WasCanceled retains the port's existing selection-on-Cancel adaptation when native OLE returns DROP after a menu Cancel. |
| CDropTarget constructor/ClearState, PositionCursor, RemoveSelection, IsFsFolderPath/GetFolderType, GetTargetPath, IsItSameDrive, GetEffect_ForKeys/GetEffect | Source decisions remain in PanelDrag; Qt supplies hit testing/highlighting and POSIX mounted-volume identity. Windows retains source drive-prefix comparison. Per-window Qt service replaces native RegisterDragDrop/RevokeDragDrop/window ownership. |
| DataObject_GetData_GetTransfer, SendToSource_TargetPath_enable, SendToSource_TransferInfo, SendToSource_UInt32, LoadNames_From_DataObject | Original GetTransfer helper, INIT_FORMATETC_HGLOBAL and Windows/COM.h CStgMedium retain type/size/header validation and medium cleanup. Windows native adapter owns IDataObject, original registered SetTargetFolder/SetTransfer/GetTransfer exchange, medium ownership and source Shell HDROP/IDLIST parsing. Performed-effect sends COPY or NONE, never MOVE; failed TRUE SetData retains caller cleanup. Typed in-process protocol remains the non-native seam. |
| DragEnter/Over/Leave/Drop, NDragMenu, Drag_OnContextMenu | Original source owner retains admission, button/flags precedence, commands, waits, confirmations and delegated-source versus target execution. Middle and right buttons both request the menu; Qt supplies menu presentation and key/point transport. ReadTransferFlags replaces Panel-pointer presence as the native capability test. |
| Is_Path1_Prefixed_by_Path2, IsFolderInTemp, AreThereNamesFromTemp, CompressDropFiles | Source temporary-path and wait precedence retained. Original ROOT_FS_FOLDER is shared with AddToArchive: C:\\ on Windows, `/` for POSIX mounts. ArchiveBackend and bound source providers retain operation ownership. |
| UNDER_CE, old non-Unicode/non-NT CF_HDROP branch, SHOW_DEBUG_DRAG, disabled preferred-effect/file-operation flags | Desktop Qt uses Unicode; CE and non-NT targets are outside this application's platform scope. Debug traces and source-disabled formats have no active workflow. Native performed/logical-effect storage and shell presentation acknowledgements remain, although upstream has no operational consumer for those stored fields. |

QWindowsMimeConverter supplies incoming IDataObject through its public API;
one explicitly QApplication-owned registration serves all windows, retains COM
ownership and returns a private nonce. Offscreen platforms do not register it.
Windows real execution uses native DoDragDrop only with the windows Qt platform;
headless tests keep the portable Qt transport. Repeated MIME conversion replaces
the single retained native owner; MIME destruction or application shutdown releases
it. Window-local native owners release on Leave/Drop/Revoke. A reusable Qt platform
MIME object can retain the last native owner until replacement/shutdown, without
retaining a source panel or accumulating prior transfers.
It exports no token format. Qt's outgoing OLE object accepts only performed-effect
SetData, so it cannot carry the reverse 7-Zip protocol; the native outgoing
source above is required. No Qt platform source is copied into the product.
Actual pointer-driven Explorer/peer Drop and rendered acceptance remain separate
from this source disposition and the automated COM/Qt consumers.

## Panel control and view state disposition (2026-10-10)

Original/current Panel.cpp and Panel.h were read in full at source revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git. Whole-file reading is not whole-module
acceptance. The restored integrated control/view component has these owners:

| Original body or dependency | Disposition |
| --- | --- |
| CMyComboBoxEdit::OnMessage | Retained source class, Alt-F1/F2, Tab, F9 and Control-W branches; QtPanelKeyDown replaces message/key-state transport and receiver-bound queued ExecuteCommand replaces posted close. Qt Return/Escape reach existing PanelComboBox end-edit handlers. |
| ComboBoxEditSubclassProc, SetWindowProc, ANSI/native default procedure, CE edit transport | Qt event filters own control delivery/lifetime; no HWND subclass procedure or ANSI buffer is needed. Qt Backtab is normalized to original VK_TAB. This does not prove physical platform input. |
| CPanel::SetListViewMode and CApp inline caller | Original four-index validation/state retained in Panel.cpp; Qt callback changes existing stack/icon style and reattaches source list focus after container registration. CListMode retains persistence. |
| CPanel::ChangeFlatMode, GetFlatMode and CApp inline caller | Original toggle, parentFolders Arc/Disk branch and RefreshListCtrl_SaveFocused retained. Qt publishes flags/settings, cancels stale presentation and joins the same-folder loader before source reload. Source OpenAsArc/CloseOneLevel assignments remain authoritative. |
| Source OnCreate list style/header edit initialization | Existing source creation/mode/sort/binding owner remains; typed callbacks attach the Qt table/icon/address controls. No independent key/view policy remains in the removed MainWindow branches. |
| Folder-type predicates, FindDir_InOperatedList, error helpers and archive commands | Original type/property admission and operated-item loop remain in Panel.cpp, with source cached/provider folder queries. QMessageBox replaces native MessageBox; HRESULT-to-message and two-line/caption construction retain source flow. AddToArchive, ExtractArchives and TestArchives retain source selection, parent-folder, output-name, archive-interface and error branches through existing CompressCall/CopyTo/backend owners. GetSubFolderNameForExtract2 remains source-owned. Their Copy/CRC/CompressCall receipts and direct archive consumers remain required; commented CThreadTest/size-formatting/NTFS-view experiments remain inactive. |
| CMyListView and PanelListNotify | Existing retained list key and notification owners and their separate body/caller receipts remain authoritative; this component changes the address consumer, not those algorithms. |
| GetDriveOrNetworkPrefix and CDropTarget::IsItSameDrive | Original helper, FS/FSDrives admission, empty-root/source guards and Windows all-source NoCase prefix loop restored. AltStreams remains an eligible filesystem drop target but does not prefer Move by default. POSIX retains the same source admission and substitutes existing QStorageInfo device/root identity only for the Windows prefix comparison, because `/` cannot identify mounts. Public default-effect regressions cover these decisions. This is not whole-PanelDrag disposition. |
| ReleasePanel/destructor and GetParent | Original disable-before-CloseOpenFolders ordering remains; per-panel external workers join before folder cleanup. QtPanelContextMenu/QtNativeShellMenu release popup, native presentation and COM context through scoped RAII, so persistent native context-menu members have no owner here. GetParent prefers the supplied parent, then the attached Qt window/main window. Menu/worker bodies retain their separate receipts. |
| OnDestroy/SaveListViewInfo | Qt header resize/move, source sorting and column-menu mutations call MainWindow::saveColumns with the captured panel projection. Retained PanelItems::SaveListViewInfo owns `_needSaveInfo`, sorted visible/hidden columns, equality and persistence. Changes are saved at mutation rather than requiring a Win32 child WM_DESTROY. View-settings/column consumers cover restart persistence. |
| Post_Refresh_StatusBar and kRefresh_StatusBar | Original post/receive checks use `_processStatusBar`; active callers are key, click, right-click and completed drag notifications in PanelListNotify. Qt delivers source selection/key/drag work before MainWindow::refreshActions. Selection/current-cell connections and refreshActions check ShouldProcessNotifications; source CDisableNotify and permanent disable change notify/status flags together. Async scans also suppress status while rows are incomplete. The existing direct Refresh_StatusBar calculation/presentation remains source-owned; a duplicate WM_USER queue is not required. |
| OnMessage reload/focus/open-change/timer/context branches | Typed reload/selection callbacks, SetFocusToList, external-worker receiver delivery, MainWindow::pollAutoRefresh and retained OnContextMenu replace Win32 message transport. Polling checks operation/scan and both panels' source notification guards before projection. Shift-selection uses the retained handler after Qt moves focus. UNDER_CE header-refresh/softkey and commented WM_DROPFILES remain outside the Qt 6 desktop scope. |
| OnCreate, ChangeWindowSize, OnSize, OnNotifyReBar and OnCommand | Original creation/binding, sort/mode/focus defaults and refresh remain in Panel.cpp. Qt layouts own up/address, table/icon stack and four status parts; QWidget resize replaces HWND moves and RBN_HEIGHTCHANGE. QtPanelAddressCombo, source key handlers, parent command routing and list/header context services replace native child IDs/notifications. Win2000/XP REBARBANDINFO sizes, ANSI procedures, common-control registration and CE controls have no Qt widget consumer. Geometry/rendered acceptance remains distinct. |
| Panel.h inline types/guards and declarations | Original property-column comparison, selected/temp/folder/open records, copy defaults, prefix predicates, timer/notify guards, list deletion notification suppression, inherited-password CopyTo/CopyFsItems and CMyBuffer allocation remain in their source owner. Qt list roles/counts replace LVITEM/handle state; callbacks and per-panel launcher replace HWND/global transport. Copy option path/overwrite/extract/catalog fields are existing integration inputs, not replacement algorithms. Declarations for PanelCopy, ItemOpen, FolderChange, Menu, Items, Select and ListNotify retain those modules' separate body receipts; this core map does not accept them by declaration. |

Direct dependencies include original App/Panel/PanelComboBox/PanelItems/folder
types and resource command IDs; Qt uses existing MainWindow/QtAppPresentation,
QtPanelAddressCombo, projection and FilesystemScanner services. Provenance and
current checks are in the [component receipt](../upstream-port-audit.md#p3-05--panel-address-keys-and-source-view-state-2026-10-10).

## FM desktop bodies and platform dispatch (2026-10-10)

Full original `CPP/7zip/UI/FileManager/FM.cpp` (1223 lines), current adapted
body and direct entry/init/window/splitter/presentation callers were reviewed
at https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. This table completes the body
disposition for that translation unit; it does not accept every runtime platform
or the entire FileManager family. Earlier bounded startup/placement receipts
remain the evidence for their respective consumers.

| Original body/branch | Retained owner or concrete adaptation |
| --- | --- |
| CSplitterPos GetWidth/SetRatioFromPos/SetRatio/SetPosPure/SetPos/SetPosFromRatio | Original fixed-point arithmetic, bounds and minimums remain in FM.cpp. QtFileManagerSplitter supplies client width, handle events and per-window state instead of HWND/GetClientRect/capture globals. Existing resize/F9/restart consumers cover this owner. |
| InitInstance | Original Position/Panels admission, defaults, CApp assignments, Create refusal and saved-maximized/show-command precedence remain. Typed QtFileManagerInitServices supplies QWidget geometry/state. Qt owns WNDCLASS registration, frame, menu/icon construction, cursor and background; the desktop resource/bitmap owners remain unchanged. |
| GetDllVersion and ComCtl activation version setup | Only selects old Win32 LVN_ITEMACTIVATE versus NM_DBLCLK/NM_RETURN delivery. Qt table/icon/keyboard transport calls retained OnNotifyActivateItems directly, with the original modifier policy. No independent activation algorithm or comctl DLL loader is needed for Qt widgets. |
| Set_Wow64 | Only active consumer is MenuPage's other-bitness capability. QtNativeMenuPageServices queries the same IsWow64Process for a 32-bit process; 64-bit availability is explicit. No stale process-global copy is maintained. This is not an x86 runtime-verification claim. |
| IsLargePageSupported/SetMemoryLock/Set_SymLink_Supported | Retained FM bodies and MemoryLock services own version/risk/setting/token policy. Production entry calls memory then link initialization before consumers. POSIX link support has no Windows token service; large-page command is unavailable there. Startup/Settings/engine and pre-Link Copy receipts remain authoritative. |
| WinMain2 RAM initialization | Original NSystem::GetRamSize runs first at production startup. FM.cpp's immutable GetRamInfo record replaces the two write-once source globals; PanelItemOpen reads the shared result. Original fallback/shift decision and existing excessive-shift guard remain. Direct source-panel consumers can initialize the same process record lazily. PanelCopy/Panel.h mentions are comments only. |
| WinMain2 language/small-screen/OLE setup | LoadLangOneTime and CApp::ReloadLangItems remain source-owned. Source IsDialogSizeOK(200,200) reaches ReadToolbar through existing Qt font/work-area services. Native Windows QApplication's Qt platform context owns OleInitialize/OleUninitialize; offscreen fixtures do not establish OLE support. See the current platform receipt. |
| WinMain2 command line and InitInstance return | Active source two-string tokenizer, trim and -t handling remain; Unix receives decoded argv through the typed boundary. WM_CREATE path preparation remains source FileName/FileFind plus CheckFileManagerStartup. Refusal/E_ABORT preserves source exit/message policy. |
| WinMain2 accelerator/message loop and exit | Qt event delivery/QAction shortcuts replace HACCEL/GetMessage/TranslateAccelerator/DispatchMessage. Source command IDs and handlers remain. ArchiveBackend/catalog ownership releases source CCodecs libraries after window/folder/worker lifetime instead of one global FreeGlobalCodecs call. QApplication owns platform-context teardown and loop exit. |
| WinMain/ErrorMessage | Original exception types, messages and return codes remain in NQtFileManager::WinMain. Source DllSecur runs on Windows. Per-window RAII joins workers before the exception owner, replacing the global exit launcher. QMessageBox replaces the native top-level 7-Zip error message. Qt owns instance/window handles. |
| SaveWindowInfo | Original CWindowInfo fields/record serialization remain. Close service obtains Qt normal geometry/maximized state, source panel/current values and source splitter position instead of WINDOWPLACEMENT/IsZoomed. |
| ExecuteCommand/WM_COMMAND | Original toolbar Add/Extract/Test switch and both timer guards remain; other IDs enter original OnMenuCommand. Qt supplies action ID/modifiers instead of LOWORD/HIWORD/control-notification fields. Capability/live-call seams protect asynchronous inputs. |
| WM_INITMENUPOPUP | QMenu aboutToShow replaces the native popup message; original File/menu policy has typed adapters. Remaining View activation reconciliation has its own owner/receipt; this FM review does not silently accept it. |
| WM_CREATE | Source initial splitter/panel/path/Create/open-error/refusal logic is retained across InitInstance, WinMain2 and CApp::Create; Qt splitter and presentation supply width/widget construction. QDrag/MIME transport registers receiving widgets instead of RegisterDragDrop on g_HWND. Native receiver proof remains separate. |
| WM_CLOSE/WM_DESTROY | Retained deactivation, soft external-worker exit, drop revocation, guarded save, ReleaseApp, placement save and hard exit order. Qt additionally joins asynchronous owners before folders are released. Accepted hidden Close uses the previously verified automatic-quit/peer-window bridge. |
| WM_LBUTTONDOWN/UP/MOUSEMOVE/WM_SIZE | Qt splitter captures handle input and forwards source position/ratio transitions; QWidget/layout delivers resize. Native capture globals, LOWORD signed mouse extraction and SIZE_* flags are presentation transport, not duplicated input policies. |
| WM_SETFOCUS | MainWindow's FocusIn event calls original CApp::SetFocusToLastItem; owning panel remembers list/path focus. panel_focus tests cover remembered controls and destroyed/rebound presentations. |
| WM_NOTIFY | Original CApp::OnNotify handles toolbar TTN_GETDISPINFO text only. Its SetButtonText, CButtonInfo and both button tables now remain in App.cpp; Qt button/tooltip ownership replaces static native tooltip buffers and ANSI/Wide notification structures. Panel selection/display/key notifications have their own PanelListNotify transport and are not routed through this FM branch. |
| Window_GetRealHeight/CApp::MoveSubWindows | Qt layout/toolbar/splitter geometry replaces Win32 toolbar AutoSize/window rectangles and explicit child Move calls; original panel-count/visible-owner choice and CSplitterPos remain. Native pixel coordinates are not reused as Qt layout dimensions. |
| NT_CHECK, ANSI/CE and inactive experiments | Legacy ANSI/NT4/WinCE window/control/message-loop branches are outside the supported Qt6 desktop target model; no CE/old-NT support is claimed. DEP/heap/theme/alternative parser/single-instance/menu-unactivate/WM_ACTIVATE/SETTINGCHANGE/DROPFILES blocks are commented upstream and remain inactive, not omitted active behavior. |

Process globals for window/path/format/placement/splitter/application state are
per-window typed state, preserving independent association-window lifetimes.
Language and question-suppression globals retain their already audited source
owners. RAM lifetime and Qt platform services are recorded in parity. The RAM
startup-cache reconciliation restores process lifetime; it does not close
whole-module source acceptance.
The body map covers FM.cpp; native visibility/focus/popup/OLE receivers,
cold stability, packages, remaining family bodies and final P3 acceptance stay
separate. [Current source/platform receipt](../upstream-port-audit.md#p3-05--fm-platform-dispatch-and-source-toolbar-text-2026-10-10).

## MyLoadMenu complete body disposition (2026-10-10)

Complete original/current `CPP/7zip/UI/FileManager/MyLoadMenu.cpp/.h` were
compared at https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, together with the exact
`resource.rc` menu, App/Panel getters and active Qt command/menu consumers.
The existing per-module baseline, C++ language, paths and notices remain.

| Source body or dependency | Disposition |
| --- | --- |
| Menu globals, CopyMenu/CopyPopMenu and destroyer | Qt owns bound QActions and submenu lifetimes. QtProgramFileMenu retains source action references and the static CRC submenu; QtFileMenuProjection removes dynamic plugin items through their owner. Native HMENU copying/destruction is replaced, not duplicated alongside Qt ownership. |
| MyChangeMenu, language pairs and MyLoadMenu | Original popup/ID/position, special Open Inside/Benchmark captions and shortcut-preservation flow remain. Qt English resource lookup replaces LoadMenu reset in place, preserving QAction identity and signal bindings; timestamp examples remain untranslated. |
| CFileMenu::Load | Original screen/read-only/hash/filesystem/selection/stream/version-control admission and separator flow remain. QtFileMenuProjection supplies screen, settings, item and platform services. PanelMenu owns dynamic File/context policy and plugin release. |
| OnMenuActivating File/View | Actual menu identity is checked before dispatch. File rebuild remains source-owned. Restored View body uses original GetSortControlID, focused-panel getters, radio/check flags and current-time precision/UTC loop. Recursive QAction lookup and signal-blocked checks replace CMenu; existing time actions are hidden/reused instead of destroyed. Parent View aboutToShow is the consumer. |
| Favorites activation | Original ten Set and ten Open caption loops, 100/50 shortening and shortcuts remain in MyLoadBookmarks. Qt eagerly refreshes them after state/language changes and construction; full-path tooltip is a presentation addition. FastFolders, persistence and source command dispatch remain authoritative. |
| ExecuteFileCommand and OnMenuCommand | Source command switches, timestamp vector indexing/UTC redraw and bookmark routing remain behind CApp-owned services. Qt availability guards stale callbacks; backend operations and native shell/plugin owners are unchanged. |
| MyBenchmark, About/Help/temp services | Original timer guards and benchmark workflow reside in QtFileManagerCommandWorkflows::MyBenchmark; official engine/backend remains the operation boundary. Existing narrow Qt/platform dialogs and HelpUtils supply native presentation/services. |
| Inactive and platform branches | UNDER_CE command bar and obsolete native menu-mask APIs have no Qt 6 desktop consumer. Commented clipboard, Show Deleted and bookmark experiments remain inactive, not missing live commands. Native HWND/HMENU lifecycle is replaced by Qt. |

This disposes this menu module's bodies and dependencies; it does not close the
remaining FileManager/Panel/FM, native/rendered or package acceptance. Regression,
geometry and platform evidence belongs to the
[source receipt](../upstream-port-audit.md#p3-05--source-view-menu-activation-2026-10-10).

## FM startup platform caller disposition (2026-10-10)

Source https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`: WinMain2's small-screen and
Set_SymLink_Supported policies now reach their startup consumers. CApp::Create
passes its presentation capability to retained ReadToolbar; source Dialog.cpp
200×200 arithmetic uses existing Qt font/work-area services instead of a global
Win32 screen flag. Explicit toolbar masks retain precedence. The production
entry calls existing link initialization after SetMemoryLock, before Copy;
Link dialog retains its idempotent call. Original capability/version/token
bodies and operation/refusal/cleanup owners remain. No account policy is granted.
Remaining FM platform/full-body, native/rendered/package/final scope stays open;
[source/consumer receipt](../upstream-port-audit.md#p3-05--startup-platform-consumers-and-toolbar-defaults-2026-10-10).

## FM launch-show and hidden-close disposition (2026-10-10)

At source https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, FM.cpp WinMain's nCmdShow flows
through WinMain2 to InitInstance. The retained InitInstance body keeps that
policy; QtFileManagerStartup/Run/MainWindow now preserve its real Windows
STARTUPINFO input instead of supplying Normal unconditionally. Qt maps states
and no-activation, leaves Hide unexposed and retains saved-maximized precedence
only for the original Normal/Show/Default branch.

WM_CLOSE cleanup remains source-owned. Qt previously omitted the corresponding
WM_DESTROY quit for never-visible windows; an accepted hidden close now queues
Qt automatic-quit completion while retaining visible association peers and
explicit quit policy. Native class/frame/old-OS presentation remains replaced
by Qt. Native focus/taskbar/rendered/interactive visibility and the remaining
FM platform/full-body review are required separate scope. Provenance and
consumer checks: [receipt](../upstream-port-audit.md#p3-05--launch-show-command-and-hidden-window-close-2026-10-10).

## Filesystem provider bodies and command consumers (2026-10-10)

Source https://github.com/ip7z/7zip.git at full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Complete original/current
FSFolder.cpp/.h, FSFolderCopy.cpp, FSDrives.cpp/.h, NetFolder.cpp/.h and
AltStreamsFolder.cpp/.h were compared, including their native/POSIX services
and PanelOperations, PanelCopy, App::OnCopy and Qt command consumers. These
nine pinned inputs retain their existing per-module baseline, C++ language,
source paths and notices. This review supplements the existing operation,
links, native metadata, devices and network receipts rather than replacing
their platform limits.

| Owner | Retained policy and adaptation boundary |
| --- | --- |
| FSFolder | Original properties/comparator, parent/binding, flat relative paths, descript.ion CPairsStorage, directory statistics, create/delete and Rename command policy retain their source owners. QFile/native stat services expose portable metadata, Qt supplies asynchronous load publication and notifications. Existing no-follow recursive traversal/cancellation protects links and owned operations; it does not replace the archive engine. Windows Rename again invokes original MyMoveFile directly. macOS renamex_np(RENAME_EXCL) supplies its atomic no-replacement boundary while allowing case-only file/directory spelling changes. Linux keeps its existing guarded source service; case-insensitive Linux volumes are unverified. |
| FSFolderCopy | Original destination classification, stat/count/progress, AskWrite/Skip, folder recursion, native Copy/Move and read/write ADS branches remain authoritative. Missing active colon-prefix branches are restored in both CopyTo and CopyFileSystemItems; folders retain the original diagnostic. Native CopyFile_NT/CopyFile_Sys restore main/super-path retry, excluding callback errors; Return_LastError_or_FAIL/GetLastErrorMessage restore the source native diagnostic. CCopyStateIO retains the original64KiB loop, source deletion after successful Move, callback cancellation and INVALID_FILE_ATTRIBUTES suppression for ADS base metadata. Existing exclusive stages, source metadata service, no-follow links and QtFileTransferTransaction rollback remain necessary accepted integration adaptations. Qt translates native path/types and presents callbacks; ordinary archive bytes remain behind ArchiveBackend. |
| FSDrives | Original native drive/device enumeration, binding, parent, property projection, GetFileSize/AddExt and image CopyTo stay active on Windows. POSIX QtVolumeMount supplies readable mounted volumes instead of Win32 drive APIs; it does not emulate device-image operations. Root icon checks open no device. Device/VHD and slow fixtures are excluded from this review's checks. |
| NetFolder | Original Windows resource/authentication/enumeration, nested binding, parent and native icon policy remain. POSIX exposes already-mounted network resources through its existing service; it does not invent SMB discovery or authentication. No live network fixture was used. |
| AltStreamsFolder | Original NTFS enumeration/properties, bind/parent, create/Rename/delete, CopyTo and read/write handling remain. CopyFrom/CopyFromFile are E_NOTIMPL in the pinned source too; the alternative body is commented out. These are source capability limits, not a newly omitted import implementation. POSIX filesystem ADS enumeration remains unavailable; archived ADS has its separate Agent owner. |

Original App::OnCopy permits a stream-prefix destination only when that prefix
is the opened peer ADS folder; the adapter preserves this restriction. Qt now
dispatches that accepted destination through CPanel::CopyTo instead of treating
the colon prefix as a normal filesystem directory. Source password and copy
options are forwarded. Source selection/completion and peer reload continue
through their existing presentation owners. No new 7zz syntax or format logic
is added to a widget.

Legacy ANSI/Win9x dynamic entry points and Windows CE presentation are omitted
for Qt6 desktop targets; Unicode native services remain the Windows boundary.
Inactive commented alternatives remain inactive. This body review does not
establish rendered Windows, installed-package or final P3 acceptance.

## FileManager services and namespace icon consumers (2026-10-10)

Source https://github.com/ip7z/7zip.git, pinned full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, port parent `919c31b`.
Complete original/current bodies and available headers for the following twelve
units were compared with their direct consumers and platform dependencies.
The 23 original cpp/header inputs have unique local owners; pinned Git blobs
and sibling checkout bytes agree. StringUtils remains an exact dependency under
third_party/7zip; adapted C++ owners remain under src/CPP/7zip/UI/FileManager.
Existing notices, source language and per-module baseline are unchanged.

| Original owner | Body, dependency and consumer disposition |
| --- | --- |
| StringUtils.cpp/.h | Original quote-toggle, unquoted-space token splitting and empty-result behavior remain exact. FilePlugins extension parsing and UpdateGUI property splitting consume the same UString/UStringVector body. Original block-commented JoinStrings is inactive. |
| FormatUtils.cpp/.h, PropertyName.cpp/.h | NumberToString uses original ConvertUInt64ToString; MyFormatNew replaces every {0} through UString and the source language overload. Property IDs below1000 use language1000+ID; higher IDs use supplied name or numeric fallback. PanelItems/PanelMenu, callbacks, plugin diagnostics, volume confirmation and deletion prompts retain these owners. Include/type spelling is the adaptation, not a replacement formatter/property policy. |
| RegistryUtils.cpp/.h | Original Viewer/Editor/Diff/7vc/Lang keys, all seven false-default FM options and indexed FlatViewArc retain their policy. Explicit INI paths and FM/Options groups replace HKCU/7-Zip keys; LargePages uses Options. The adapter accepts its own bool text and range-checked DWORD values, leaving defaults on malformed input. FmSettingsPath unifies application/shell identity on Windows. App startup, Options pages, external editing, menus and VerCtrl use that boundary. Commented HKLM/LockMemoryAdd/Underline/ShowDeleted code remains inactive; actual memory privilege/startup policy belongs to FM.cpp, not this storage review. |
| RegistryAssociations.cpp/.h | Original extension/ProgID/default-icon parsing, ASCII 7-Zip ownership test, extension deletion and owned-class deletion, and open-command/icon registration remain on Windows. Portable CShellExtInfo initializes its absent icon index. SystemPage calls typed association services; the Windows platform delegates to these source bodies and SHChangeNotify. Existing revision snapshots reject external races and leave Windows UserChoice untouched. macOS/Linux services have separate native owners; this review does not perform OS association writes. |
| UpdateCallback100.cpp/.h | Source scan/count/total/completed/ratio, operation/error/extract-result, password/no-ask, archive move and Before_ArcReopen cancellation/status bodies retain their original CUpdateCallbackGUI2/ProgressDialog2 delegation and eight COM interfaces. PanelCopy, PanelOperations, PanelItemOpen and ArchiveProgressOperation initialize the callback and assign its progress owner before worker execution, preserving joined lifetime. Qt transports supply presentation/process delivery; archive operations remain behind ArchiveBackend and the official engine. |
| TextPairs.cpp/.h | Original BOM/quote/whitespace/CR parsing, sorting, case-insensitive binary search, insertion/replacement/deletion/value retrieval and CRLF serialization remain. CFSFolder descript.ion load/save and comment property access consume the same CPairsStorage. UTF-8/file/time services are existing filesystem boundaries, not a second comment parser. |
| SplitUtils.cpp/.h | Original whitespace/dash/byte-k-m-g-t parsing, overflow/nonzero checks, preset list and final-volume ceiling count retain their bodies and source vector types. QComboBox addItem replaces only native control insertion. SplitDialog/PanelSplitFile and GUI CompressDialog remain actual consumers. |
| VerCtrl.cpp | Complete source file-data/time/attribute helpers and CApp::VerCtrl Edit/Commit/Revert/Diff bodies remain: filesystem/exact-one admission, configured path conversion,256MiB read bound, version backup numbering, data/time comparison, timestamp rounding, readonly changes and original No-default overwrite. MyLoadMenu controls visibility, ExecuteFileCommand dispatches, Qt refresh preserves selection. qtverctrlfileservices supplies Windows aliases or POSIX stat/FILETIME/source-file/chmod services; original backup algorithm is not recreated there. Commented copy/revert-message alternatives remain inactive. |
| LinkDialog.cpp/.h/.rc | Source reparse read/parse/roundtrip check, init/type/radio/Browse, relative-target computation, regular-file warning, hard/reparse creation/deletion and CApp::Link admission/result/conditional-refresh bodies retain their policy. Qt controls/layout/modal result replace Win32 presentation. qtlinkservices uses source FileFind/FileDir/reparse/link operations; POSIX staged same-directory replacement preserves the old link on failure. Junction/WSL controls remain disabled on POSIX. Windows symlink-version/privilege initialization remains in FM.cpp; no persistent account-right mutation was executed. |
| SysIconUtils.cpp/.h and RootFolder.cpp/.h | Source sorted attribute/extension caches, compressed map, numeric split-extension singleton,65536 limits and basic-attribute fallback remain. Native Unicode path/HRESULT services remain active. The missing CSIDL helper and its Computer/Documents/Network callers are restored from source, including PIDL release and failure−1. RootFolder no longer overwrites Documents/Network indices. Source Shell_Get_SysImageList_smallIcons now supplies both original small/large native images to the existing QIcon bridge, releasing temporary HICONs and caching the combined icon. PanelItems consumes native indices; QtPanelAddressCombo uses the same images for namespace popup rows and closed Computer/Network address. POSIX platform icons remain a presentation service. Obsolete ANSI dispatch and Win32 list-control attachment are presentation/platform omissions; native source image sizes are retained rather than replaced by scaled small images. Whole rendered/HiDPI acceptance remains distinct. |
| RootFolder remaining bodies | Source Init/load/count/name/IsDir/type/path/icon properties, index binding, AreEqualNames, Trim/empty/new-root, translated and English aliases, device/super/ADS dispatch, filesystem-first binding with Windows network fallback, parent-null and folder publication remain. The device-root row also regains its original CSIDL_DRIVES index; testing that row does not open a device. QStandardPaths supplies Documents location instead of legacy dynamic NT4/ANSI/CE shell path lookup. POSIX's existing mounted-volume/network rows extend the original one-row root through their previously audited CFSDrives/CNetFolder services. Native devices/ADS/network behavior retains its separate receipts and fixture limits; this review does not run VHD suites. |

The icon regressions use actual Windows shell PIDL/image-list data as their
independent reference and compare public CPanel row icons, address-control
images and both native image sizes.
This is bounded native data/Qt image verification, not a rendered Windows 7zFM
or physical-input acceptance. The full FileManager startup/platform review,
native/installed/package checks and P3-03/04/05/06 remain required.

## Startup and large-page source closure (2026-10-10)

The previously omitted FM.cpp::SetMemoryLock body and WinMain2 large-page
allocator reset now have active Qt entry callers. SettingsPage::OnApply again
uses original EnablePrivilege_LockMemory before saving. CompressCall's original
AddLagePagesSwitch risk check is shared with ArchiveBackend instead of the
independent capability/setting-only argument policy. Native MemoryLock and
SecurityUtils and the existing ArchiveCommandLine/Bench/C allocator remain the
authoritative dependencies; no second global or memory-allocation algorithm is
added. POSIX capability rejection and Qt 6 desktop-only scope are explicit.
Historical receipts describing these omissions are superseded only for these
branches; the remaining FM/native/packaging acceptance stays open. See the
[source and validation receipt](../upstream-port-audit.md#p3-05--startup-settings-and-large-page-engine-policy-2026-10-10).

## Complete 109-unit physical input inventory (2026-10-10)

The pinned Git-tree inventory was extended from FileManager to Agent, GUI and
Common at port `951a855`, source https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. It includes every top-level file
in those four directories, not just cpp names or CMake's selected objects.

| Original directory | cpp / h / rc | All files | Exact / adapted local owners | Physically absent |
| --- | --- | --- | --- | --- |
| FileManager | 67 / 80 / 29 | 197 | 52 / 106 | 39 |
| Agent | 7 / 5 / 0 | 12 | 0 / 11 | 1 |
| GUI | 10 / 15 / 8 | 38 | 8 / 18 | 12 |
| Common | 25 / 30 / 0 | 55 | 21 / 34 | 0 |
| Total | 109 / 130 / 37 | 302 | 81 / 169 | 52 |

The 276 cpp/h/rc paths have 234 single owners and 42 physical omissions.
The six missing cpp units retain their [explicit body/build dispositions](#missing-ui-translation-units-reviewed-on-2026-10-05).
FileManager's complete absent-file map is [below](#filemanager-headerresource-and-plugin-dependency-inventory-2026-10-10);
the following rows account for all other absent inputs. Common has no missing
physical input; its [complete source reconciliation](common.md#common-owner-and-adaptation-reconciliation-2026-10-08)
owns the relevant body/dependency decisions, rather than this file count.

| Absent original input | Source contents and current boundary |
| --- | --- |
| Agent/StdAfx.h, GUI/StdAfx.cpp/.h | Agent's header supplies the compiler warning pragma and portable Common.h. GUI's header includes FileManager/StdAfx.h; its cpp includes only that header. Current sources include the existing portable common headers directly and CMake supplies compilation policy. No active workflow body or new PCH implementation is omitted. |
| GUI/resource.rc | Standalone version/icon/manifest and included dialog/string resources. Current RunSevenzipGUI invokes original GUI Main2/WinMain dispatch through the shared executable's 7zG alias; it is not an omitted standalone workflow. CMake/Qt supplies executable/resource transport, while the original GUI/dialog owners remain active. Version/product packaging acceptance stays separate. |
| GUI/resource2.rc | Original two-entry STRINGTABLE: IDS_PROGRESS_COMPRESSING3301 and IDS_ARCHIVES_COLON3907. UpdateGUI/UpdateCallbackGUI and ExtractGUI still use those IDs through source LangString. Compiled ResourceString tables are tried first, then the existing original CLang-backed bundled English resource supplies Compressing/Archives:. This is the established resource fallback, not a new caption table or parser. |
| GUI/CompressDialog.rc, GUI/CompressOptionsDialog.rc | Exact desktop/CE controls, compression STRINGTABLE and options include reviewed. Source CCompressDialog and its COptionsDialog retain format/profile/event/default/validation/Apply policies; CreateArchiveDialog and Qt options controls present the reviewed grouping. Original language IDs and bundled English fallback supply error/mode/time captions. The [complete compression](gui.md#ccompressdialog-complete-bodyheader-disposition-2026-10-08) and [options](gui.md#coptionsdialog-complete-controllifecycle-disposition-2026-10-08) receipts own behavior, platform adaptations and actual checks. RC absence is not an acceptance of unreviewed controls. |
| GUI/ExtractDialog.rc | Desktop destination/name/path/overwrite/eliminate-duplication/password/security and OK/Cancel/Help controls, CE alternate layout and commented ADS control reviewed. Source ExtractDialog owns conditional state and acceptance; Qt supplies controls/layouts and its existing text/geometry contract. Source ExtractGUI/ExtractCallback/Common Extract remain the operation owners. The [Extract disposition](gui.md#gui-extract-dialog-and-hash-worker-2026-10-07) remains authoritative. |
| GUI/FM.ico | Byte-identical to the single exact FileManager/FM.ico owner (SHA-256649f8e81cce9d8e3ab9a0d06db31788f5ced3c6ed99a78304a87dba35ecf06cc). The existing Qt resource embeds that original shared icon; a second source copy is unnecessary. This does not prove native executable icon or rendered fidelity. |
| GUI/7zG.exe.manifest | Same long-path/compatibility/native-control/DPI structure as the FM manifest, with GUI assembly identity/description. The 7zG alias copies the same Qt executable, whose embedded longPathAware/Windows10/asInvoker fields were read directly in the [manifest receipt](../upstream-port-audit.md#p3-05--filemanager-resource-and-plugin-dependency-reconciliation-2026-10-10). Original native theme/DPI and current executable identity/packaging remain distinct acceptance boundaries. |
| GUI/makefile, GUI/GUI.dsp, GUI/GUI.dsw | Original nine GUI objects plus PCH, Common/native/C/Explorer/FM dependencies, language/device/external-codec flags and resource.res reviewed with FM.mak/7zip.mak. CMake owns the current libraries and shared dispatcher executable. VC6 workspace, Win32/CE toolchain configurations and C:\\Util output paths are build transport; they do not justify omitting listed functional modules. |

All pinned files and unique local owners were recorded with source/checkout/local
hashes; only legacy dsp/dsw checkout line endings differ from their Git blobs.
The three declared local PanelSelect/PanelSort/PanelSplitFile headers remain
outside the original inventory and expose their source cpp owners; they do not
create additional functional implementations. The complete borrowed manifest
still verifies 410 exact, 198 adapted and three local paths. No source/resource,
UI, build or test change follows from this documentation-only review.

This completes the physical inventory and absent-input map for these 109 units.
It does not close full-body integration, FM startup/platform omissions,
native/rendered/package acceptance or P3-05/P3-06.

## FileManager header/resource and plugin dependency inventory (2026-10-10)

Pinned source: https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. The inventory was repeated from
that Git tree against committed port `0d71043`, rather than inferred from the
sibling checkout or filenames mentioned in earlier receipts. FileManager has
197 top-level files: 67 C++ units, 80 headers, 29 RC files, 14 BMPs, two ICOs,
one application manifest and four build/project files. Of the 176 cpp/h/rc
paths, 142 have exactly one recorded local owner (36 exact, 106 adapted),
and 34 have no physical local copy (five cpp, 11 headers, 18 RC). All 16
bitmap/icon inputs have exact owners; the other five absent files are the
manifest and build/project inputs. No duplicate or unrecorded owner was found.
This is a physical ownership inventory, not acceptance of every functional body.

Every pinned original was compared with the checkout; only FM.dsp/FM.dsw have
checkout CRLF conversion. Their Git blobs define the baseline. The complete
borrowed-file verifier independently passes 410 exact, 198 adapted and three
local paths. The following table accounts for every absent FileManager path.

| Absent original files | Body/resource disposition and actual owner |
| --- | --- |
| ProgramLocation.cpp/.h, RegistryPlugins.cpp/.h, StdAfx.cpp | Empty include/guard, commented registry plugin declarations/bodies and PCH transport respectively. The [translation-unit review](#missing-ui-translation-units-reviewed-on-2026-10-05) records their callers/build boundary. FilePlugins and Windows/DLL remain active owners; the empty header is not a replacement for those modules. |
| EnumFormatEtc.cpp/.h | Byte-exact dependency retained under third_party and compiled for Windows. Native PanelDrag::CDataObject::EnumFormatEtc supplies source DATADIR_GET/CF_HDROP; Next/Skip/Reset/Clone and factory retain their original owner. POSIX Qt platform enumeration remains a presentation adapter. See the complete PanelDrag protocol map above. |
| ProgressDialog.cpp/.h/.rc/ProgressDialogRes.h | Active installer-specific legacy progress/sync/range/cancel/thread-join implementation and its two-control resource. FM.mak selects ProgressDialog2; SFXSetup selects this separate implementation. The [legacy disposition](#missing-ui-translation-units-reviewed-on-2026-10-05) and accepted bundle selection remain authoritative. No modern progress implementation is claimed from these absent files. |
| PluginInterface.h | All plugin interface macros/declarations are inside the original block comment. Actual active interfaces are IFolder/IFolderArchive and codec interfaces; no active API is omitted from this header. |
| PluginLoader.h | Its CreateManager/LoadAndCreateManager inline bodies are active definitions, but both actual FilePlugins/FileFolderPluginOpen loading branches are commented in the original. The live branch constructs CArchiveFolderManager. Current injected IFolderManager replaces that construction at the established ArchiveBackend engine boundary; CLibrary retains actual archive-module lifetime. Original external codec loading remains in LoadCodecs, not this inactive plugin branch. |
| DialogSize.h | BIG_DIALOG_SIZE/SIZED_DIALOG select alternate resources only under UNDER_CE; the desktop branch selects the normal resource. OverwriteDialog, OptionsDialog and ProgressDialog2 use Qt controls/layouts and their existing normal/compact contracts. No Windows CE target or independent dialog-size algorithm is introduced. |
| ComboDialog.rc/ComboDialogRes.h, CopyDialog.rc, SplitDialog.rc | Label/editable-combo/Browse/info/volume fields and OK/Cancel grouping map to the same source ComboDialog/CopyDialog/SplitDialog owners. Combo's numeric RC-only IDs become direct Qt control references. Existing [text/geometry contracts](../ui-layout-contracts.md) and source Create/Copy/Split/Combine receipts own the states; omission of RC compilation does not omit functional dialog bodies. |
| EditDialog.rc/EditDialogRes.h, ListViewDialog.rc/ListViewDialogRes.h | Read-only multiline edit plus Close, and report list plus OK/Cancel, map to source EditDialog/ListViewDialog Qt controls. Direct Qt references replace numeric attachment IDs. Full-value/info/history/result behavior remains in those source modules; current body disposition is [recorded separately](#resulthistory-and-about-complete-disposition-2026-10-10). |
| MemDialog.rc, MessagesDialog.rc, PasswordDialog.rc | Source memory/action/repeat/Continue/Cancel, diagnostics list/Close and password/show/OK/Cancel controls map to the same source Qt dialog owners. Mem/Password generated reference contracts preserve original resource hashes; Messages uses its reviewed text/geometry contract. Their conditional state, result and callback receipts remain authoritative. |
| EditPage.rc/EditPage2.rc, FoldersPage.rc/FoldersPage2.rc, MenuPage.rc/MenuPage2.rc, SettingsPage.rc/SettingsPage2.rc, SystemPage.rc | The outer resources select desktop/CE sizes; their included bodies define Viewer/Editor/Diff, work-folder choices, shell-menu/Zone/items, file-manager/memory settings and association user columns/buttons. Source pages retain the actual initialization/Apply logic; Qt SettingsDialog supplies controls and property-sheet navigation. Existing Options/source page and System resource contracts own the desktop grouping, platform availability and errors. CE-only duplicate sizes and the commented memory/skip controls are not active desktop features. |
| 7zFM.exe.manifest | Original resource.rc embeds Common-Controls v6, Vista–Windows10 compatibility, system DPI and long-path declarations. Qt creates the current executable manifest; a read-only Windows RT_MANIFEST probe and installed Qt generator confirm longPathAware/asInvoker/Windows10 support. Original native control/theme and legacy DPI declarations are presentation boundaries, not archive algorithms. Actual application/desktop/DPI acceptance is not inferred from a test executable; the dated source receipt states the exact probe scope. |
| FM.mak, makefile, FM.dsp, FM.dsw | FM.mak/makefile/7zip.mak identify FM/Agent/Common/GUI/native objects, language/device/external-codec defines and selected resource.res. Current CMake explicitly owns corresponding source libraries, C-versus-C++ languages, Qt controls and resource generators. VC6 workspace/configuration, PCH and machine-specific output paths are build transport. Makefiles remain the dependency map; their absence locally does not authorize dropping active listed bodies. |

### FilePlugins and FileFolderPluginOpen complete disposition

Complete original/current cpp/h bodies were reviewed with PluginLoader,
RegistryPlugins/PluginInterface, IFolder and the actual SystemPage::OnInit and
PanelItemOpen::OpenAsArc consumers. Existing source/engine/opening receipts
remain authoritative for native/process/stream acceptance.

| Original body family | Retained implementation and integration boundary |
| --- | --- |
| CExtDatabase FindExt/Read, CPluginToIcon/CExtPlugins | Original IsEqualTo_NoCase lookup, SplitString, icon defaults and duplicate-extension merge loop retained. Read receives the retained manager instead of constructing another one. SystemPage clears the database before reading it and consumes original extension/icon records. CE-only CAB filtering and commented registry plugin iteration have no desktop branch. |
| CThreadArchiveOpen Process/MyThreadFunction | Original progress closer, OpenFolderFile call, exception-to-E_FAIL and worker entry retained. Owned nested file streams use the existing IQtArchiveStreamOpenServices seam so the original parent-link path sees subsequent changes; the established source stream-copy/materialization receipt owns that necessary file-only adaptation. |
| SplitNameToPureNameAndExtension, CArcLevelInfo/GetFolderLevels/GetBracedType/GetFolderError | Original final-dot split, QueryInterface/property reads, reverse archive levels, type/error/flags formatting and non-open versus opened diagnostics retained. Portable types/propvariants and existing language/resource services supply representation; no second diagnostic composition algorithm is added. |
| CFfpOpen OpenFileFolderPlugin | Original path split, callback creation, cached password, volume callback lifetime, subarchive versus directory initialization, joined progress, S_OK/S_FALSE/error handling, diagnostic/password publication and library/folder transfer retained. QWidget/settings/catalog injection replace HWND/global manager transport. Only S_OK publishes Folder; S_FALSE can retain diagnostic ErrorMessage. PanelItemOpen consumes those exact results and owns its parent link. |
| FindPlugin, association reordering, plugin enumeration/load branches | These bodies are inside original comments. They are not active third-party plugin support and are not substitutes for the mandatory external codec loader. CPluginLibrary has no active LoadAndCreateManager caller in these two source units. |

No code, resource bytes, baseline revision or UI changed in this reconciliation.
The remaining FM startup/platform disposition, full 109-module integration,
native receivers/rendered reference/packages and final acceptance stay open.

## Copy, CRC and sort complete body/caller disposition (2026-10-10)

Complete original/current PanelCopy.cpp, PanelCrc.cpp and PanelSort.cpp were
reviewed with Panel.h/App.h, CApp::OnCopy, typed copy options, folder/Agent/raw
interfaces and Qt progress/list consumers. Pinned source repository/revision
and the casing correction are in the
[source receipt](../upstream-port-audit.md#p3-05--core-copycrc-sort-and-results-reconciliation-2026-10-10).

| Original body family | Retained owner and integration boundary |
| --- | --- |
| CPanelCopyThread ProcessVirt/ShowFinalResults/ProcessWasFinished_GuiVirt | Source hash/test/filesystem-stream/CopyTo branches, callback result/hash publication and once-only final results retained. Typed extract/path/overwrite options route the original IArchiveFolder extraction entrypoint; Qt supplies joined modal/progress transport, not archive algorithms. |
| CPanel CopyTo/CopyFsItems | Source Zone registry decision, external-codec lifetime before Hash, callback/password initialization, progress title, reached result/messages/password cache and title refresh retained. Explicit settings/provider injection replaces FM globals; ArchiveBackend supplies official engine operations. |
| CThreadUpdate, CPanel CopyFrom/CopyFromNoAsk | Original file/path vectors, folder/password/callback state, CopyFrom, E_NOINTERFACE-to-message/E_ABORT and guarded selection/focus phases retained. Existing original CProgressThreadVirt supplies exception/closer/thread/modal join instead of duplicating Win32 thread transport. ActionSet/PathMode use Agent's source update entrypoints; Qt returns the reached selection phase for deferred publication after source guards unwind. |
| CDirEnumerator Init/Next, CThreadCrc ProcessVirt/SetStatus/AddErrorMessage/ShowFinalResults | Original two-pass scan/hash,100-entry scan updates,32KiB buffer,2MiB progress thresholds, counts/errors/digests and once-only results retained. POSIX dirent-to-FileInfo and native bounded Windows CInFileStream adapt I/O; the accepted filesystem link refusal remains explicit. No digest implementation exists in widgets. |
| CApp CalculateCrc2/CalculateCrc | Source operated indices, operations-capable stream-folder delegation, codec/hash selection, flat-mode traversal, source progress and result/title phases retained. Backend catalog supplies the original providers instead of LoadGlobalCodecs; the source-panel flat-mode query and empty-prefix guard adapt the former CApp context. Native device acceptance remains separate. |
| CompareFileNames_ForFolderList, CompareFileNames_Le16, raw-property comparison | Original digit-run/leading-zero and UTF-16 comparison retained. QStringView converts at the boundary; the local char16_t adapter now calls original MyCharUpper(wchar_t), replacing independent QChar casing. Raw NtReparse uses source CReparseShortInfo; invalid records return equality instead of dereferencing a failed parse. |
| CompareItems2/CompareItems, SetSortRawStatus/SortItemsWithPropID/OnColumnClick | Source parent/directory precedence, raw/provider/property branches, three-step property/name/prefix fallback and load-index tie-break remain. Typed CPanel::CompareItemIndices replaces LPARAM callback transport; Qt applies direction outside the comparator so directory/parent precedence remains stable. Header events supply a property ID; source initial size/time direction and raw-column flag remain. |
| Commented raw CRC/read experiments and old SortItems variants | Original inactive examples remain inactive. Qt does not implement an alternate copy/hash/sort algorithm for these comments. |

This records source body and dependency boundaries, not whole FileManager or
native platform/package/final acceptance. Existing operation receipts own the
success/failure/Cancel/publication contracts; the source correction and bounded
current consumers are recorded in the linked receipt.

## Result/history and About complete disposition (2026-10-10)

Complete original/current ListViewDialog.cpp/.h and AboutDialog.cpp/.h, exact
original RC/Res, HelpUtils bodies and actual HashGUI/PanelFolderChange/PanelMenu/
MyLoadMenu/CApp/Qt Help/About callers were reviewed at the same pinned revision.

| Original body family | Retained owner and adaptation |
| --- | --- |
| ListView_GetSelected, CListViewDialog OnInit | Source vector insertion,1024-unit display truncation, CRLF/LF flattening, SelectFirst, columns and StringsWereChanged initialization retained. QTreeWidget/Qt layouts replace native attachment/column/resize transport; FM SingleClick and shared hover replace list styles. |
| CopyToClipboard/ShowItemInfo/DeleteItems | Original untruncated selected names/values, platform line endings, one-item info, parallel Strings/Values deletion and focused selection retained. Qt clipboard and source EditDialog modal replace native services. Property-name language bindings are a UI adaptation. |
| OnEnter/OnNotify/OnOK | Original Alt/two-column info, one-column accept, delete admission, Ctrl+A/C/Insert and accepted focused index retained through Qt events. Native activation-version dispatch becomes typed click/double-click; Meta is the documented macOS keyboard adaptation. Cancel leaves caller history unpublished. |
| CAboutDialog OnInit/OnHelp/OnButtonClicked | Original external-codec error getter, version/date, language IDs, start.htm and homepage switch retained. Scoped catalog injection replaces g_CodecsObj; QMessageBox/QDesktopServices/Qt modal positioning replace Win32/CE services. Local app version/license access are existing explicit additions, not inferred engine versions. |
| HelpUtils ShowHelpWindow | Original module-prefix/7-zip.chm::/topic construction retained. Offline original HTML/CSS and Qt navigation replace HtmlHelp/hh.exe/CE/MingW presentation branches. Page/dialog callbacks retain original topic ownership. |

ListView resources remain reproducible read-only upstream references for the
existing contract; About resources/logo are exact vendored inputs. No rendered
Windows or native-system-picker claim follows from these source reviews.

## FM splitter body/caller disposition (P3-05.16, 2026-10-10)

Original CSplitterPos and its WM_CREATE/WM_SIZE/mouse/SaveWindowInfo/
MoveSubWindows callers now retain source coordinate and fixed-point ratio policy
in FM.cpp. QtFileManagerSplitter replaces native client-rectangle lookup, handle
capture and child moves; startup, F9, resize and close use the same state.
Qt child content minimums can exceed source120; the handle retains source4.
The original vertical toolbar/panel moves become Qt layout services, with panel
ordering retained. This bounded disposition leaves remaining startup-platform
branches and native/rendered acceptance open. See the
[splitter receipt](../upstream-port-audit.md#p3-0516--source-splitter-resize-and-persistence-2026-10-10).

## Temporary browser complete body/caller disposition (2026-10-10)

Pinned repository/revision and validation are in the
[source receipt](../upstream-port-audit.md#p3-05--temporary-browser-source-lifecycle-reconciliation-2026-10-10).
Complete original/current BrowseDialog2 bodies and dependencies were reviewed;
the original private class is declared in its header for the existing extracted
Qt presentation, without a second functional implementation.

The later [109-unit ledger](ledger.md#109-unit-source-receipt-ledger-2026-10-10) identified
the native StartApplication gap. Its correction and caller/handle disposition
are now in the [launcher map](#native-associated-launcher-and-continuation-2026-10-10);
this earlier browser-lifecycle receipt alone did not establish that acceptance.

| Original body family | Retained owner and platform boundary |
| --- | --- |
| CBrowseItem Construct, CBrowseEnumerator Enumerate/NeedInterrupt | Original200-directory/2000-file bounds, root-child counts, one-child metadata, interrupted totals and recursion retained. POSIX enumerator uses no-follow FileInfo; IsOsSymLink maps to HasReparsePoint on Windows. |
| PrintPropsPrefix/Size/MTime/Name/Attrib/PrintProps, PrintFileProps/Show_FileProps_Window | Source properties/formatters and fresh-file lookup retained. Portable FiTime/GetWinAttrib and QMessageBox replace Win32 representations. Existing failed-lookup guard exits after reload invalidates row references. |
| CompareItems, ChangeSorting_and_Reload, Reload overloads, selection helpers | Source directory/parent precedence, six sort fields, stable index tie-break, sort direction and selected/focused names retained. Qt rows delegate to CompareItems; PopulateRows supplies icons, columns and source timestamp/size text. |
| GetParentPath/OpenParentFolder/OnItemEnter/OnOK | Original parent limits and one-selection/Alt-properties/link refusal/Shift-external navigation retained. Owned OS-temp root is the top boundary. Qt Enter event dispatch replaces IDOK/list-focus transport; no default push button accepts Enter. |
| OnKeyDown, OnButtonClicked | Complete active keyboard switch restored. Qt button connections retain source Parent/Delete/Refresh then unconditional list focus; Return is list activation and unmodified F5 remains the recorded Qt convenience. |
| OnDelete | Original prefix guard, selection, one/many confirmation, ten-name summary, per-item deletion, first-failure return and final reload retained. Existing ownership claim revalidates identity/no-link ancestry and active/external protection before each original removal call. |
| OnContextMenu, StartApplication/StartApplicationDontWait/GetQuotedString2 | Source command IDs/order, selected-count admission, link refusal, property/open/delete dispatch retained. QMenu replaces native menu presentation; self-launch uses a detached executable+argument vector. The later launcher map restores original Windows PIDL/ShellExecute bodies, with Qt association limited to POSIX/explicit openers. Browser lease protection remains until proven external completion. |
| OnInit/OnSize/OnMessage/OnNotify, CWaitCursor2, MyBrowseForTempFolder/OnHelp | Qt construction/layout/tree notifications replace HWND/RC/redraw/message transport. Original aggregate count>1000 and CWaitCursor2 Set/destructor phases restored through QApplication override/restore services. Source title, OS temp lookup and fm/temp.htm remain; Qt canonicalizes the OS alias. Exact RC/Res retain grouping/IDs. Drive/non-temp navigation and naming-only7z* admission are superseded by the accepted owned-temp boundary; CE sizing and commented path/filter/debug branches have no new desktop feature claim. |

The original FindNonHexChar_F actually accepts alphanumeric characters; its
eight-character naming convention is not proof of ownership. The registry
supplies membership only, while BrowseDialog2 retains deletion and enumeration.
No whole-family or native rendered acceptance follows from this disposition.

## ViewSettings complete body/caller disposition (2026-10-10)

Complete original/current ViewSettings.cpp/.h reviewed at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git. Shared C/CpuArch endian helpers, MyBuffer and
MyString/Vector types remain source dependencies, with their original linkage.

| Original body family | Source owner and adapter/caller disposition |
| --- | --- |
| CColumnInfo/CListViewInfo constructors, Clear/IsEqual, Save/Read | Original version1, header/column sizes, byte order, sort/direction/visibility/width serialization and validation/control flow retained. QSettings QByteArray replaces REG_BINARY; hex-encoded UTF-8 folder-type IDs escape INI key syntax. PanelItems SaveListViewInfo/InitColumns consume the same records. |
| CWindowInfo Save/Read, QueryBuf | Original20-byte signed rectangle/maximized and12-byte panel/current/splitter records retained; CWindowRect replaces32-bit Win32 RECT fields. MainWindow close saves Qt normal geometry. Source InitInstance now owns position/maximized replay and panel admission through typed Qt services; P3-05.16 platform acceptance remains open. |
| SaveUi32Val/ReadUi32Val, Save/ReadToolbarsMask | QSettings scalar transport replaces private registry helpers, with DWORD range validation and original bit31/default1+4+8 mask. Source App Read/SaveToolbar and Qt legacy-key migration/presentation own consumers. |
| CListMode Init/Save/Read | Original default Report and two packed eight-bit fields retained; source App Create/Save and Qt setViewMode consume them. |
| GetPanelPathName, Save/ReadPanelPath | Original PanelPath+decimal-index naming and missing-value refusal retained. Source CreateOnePanel and Save own path/outer-archive decisions; Qt supplies INI string transport. |
| SaveStringList/ReadStringList and FolderHistory/FastFolders/CopyHistory | Original UStringVector interfaces, clear/read/write wrappers and named keys retained. QSettings QStringList is storage transport only. AppState owns history/bookmarks and App::OnCopy owns Copy history. Duplicate QStringList-only helpers removed. |
| AddUniqueStringToHeadOfList | Original UStringVector/IsEqualTo_NoCase/Delete-all/Insert-front algorithm restored. Source App retains20-entry pruning; Qt converts values only for CCopyDialog presentation. K/K source distinction now survives store/reopen. |
| Registry paths, g_CS, inactive structs/FindColumnWithID | HKCU/CKey become explicit INI paths. Actual consumers run on the GUI thread, so Win32 critical-section state is replaced by that confinement. Commented sample record structs and unused FindColumnWithID have no active upstream body. |

Existing source settings/column/runtime/persisted-location consumers retain
malformed-data, independent folder-type and restart coverage. The source
InitInstance position/maximized consumer and its platform acceptance are tracked
under [P3-05.16](../upstream-port-audit.md#p3-0516--source-initinstance-window-placement-2026-10-10).

## PanelOperations complete body/caller disposition (2026-10-10)

Complete original/current PanelOperations.cpp and Panel.h declarations reviewed
at `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git, with App/MyLoadMenu callers, folder/property
interfaces, Qt operation/reload consumers and dependencies.

| Original body family | Retained owner and adaptation |
| --- | --- |
| EFolderOpType, CThreadFolderOperations ProcessVirt/DoOperation | Source Create/Delete/Rename dispatch, callback/password initialization and joined progress retained. Windows COM stays native; Qt transports progress. Comment adds the source SetProperty call to this joined transport because upstream invokes it synchronously on the UI thread. |
| DeleteItems | Source operated indices/state, internal confirmation, worker and refresh retained. SHFileOperation A/W, pre-NT/CE/MAX_PATH branches become the OS trash adapter, with explicit confirmation/error/partial refresh and symlink refusal in the [Delete receipt](../upstream-port-audit.md#p3-05--source-delete-lifecycle-2026-10-10). Explicit-index Move publication remains its documented transaction boundary. |
| OnBeginLabelEdit, OnEndLabelEdit, RenameFile | Source focused-row/Parent/read-only admission, validation, worker, new-or-old focused name, SelectFocused and dontShowMode retained. Qt supplies editor commit/cancellation and queued reload instead of LV_DISPINFO/PostMsg; see [Rename receipt](../upstream-port-audit.md#p3-05--source-rename-activation-scenario-2026-10-10). |
| IsCorrectFsName, CorrectFsPath | Original final-component dot/dotdot refusal and portable source CorrectFsPath delegation remain. |
| CreateFolder, CreateFile | Complete source validation/dialog/state/worker/error/refresh workflows retained; [Create receipt](../upstream-port-audit.md#p3-05--source-create-filefolder-scenario-2026-10-10) documents Qt controls/provider publication. |
| ChangeComment | Original void lifecycle, focused property/value/title, Cancel return, state/error and guarded RefreshListCtrl restored. Joined property transport and quiet cancellation adapt Qt; the source HRESULT helper replaces duplicate message construction. Qt command no longer owns render/restore. |
| Inactive MessageBoxErrorForUpdate and diagnostics | Original block-commented helper and commented alternate refresh/delete diagnostics have no active callers. They are not missing functional implementations. |

Local CanComment is the existing property-capability/command-state bridge to
MyLoadMenu admission, without comment parsing/writing. Folder providers and
ArchiveBackend retain operation algorithms. Remaining FileManager/platform
acceptance is separate.

This review records module bodies and caller/dependency boundaries for P3-05.
It does not establish whole-module, platform or final acceptance from filenames.

### Browse path-dialog disposition, 2026-10-10

The complete pinned BrowseDialog.cpp/.h/.rc/Res and Combo/Copy headers/bodies
were reviewed with their callers. The active desktop custom CBrowseDialog is
now retained in its source owner: parent/root limits, files, filtering,
sort direction and directory grouping, reload/selection, create-folder errors,
list activation, raw GetFullPath acceptance and Cancel publication. Qt replaces
control construction/events/resize, native icons and ordinary picker services.
POSIX enumeration uses the source FileFind conversion; Win32 drive/super
branches remain. The Win32 260-unit fallback boundary is explicit on all hosts;
Qt's lack of common-dialog extended errors requires preselection for long files.
Portable DTOs and source filter vectors reach every direct production consumer.
Original RC/Res are exact baseline references. The complete CommonDialog.cpp/.h
is restored at its Windows module boundary, with original inherited DTO and
CommonDlg_BrowseForFile filter contract. Qt replaces native OPENFILENAME,
ANSI/SDK/CE transport; source flags permit new Open names and omit a second
Save overwrite prompt, with accepted-only publication and unchanged cwd.
CE layout, HWND/common-dialog transport and commented/debug examples are
accounted for as presentation or
inactive scope. BrowseDialog2's temp ownership/delete flow remains separate.
This supersedes the older fallback deferral, without closing FileManager or
asserting rendered/native Windows acceptance. See the
[source audit](../upstream-port-audit.md#p3-05--source-path-dialogs-and-long-path-browser-2026-10-10).
The historical [P0-01 registry](../history/p0-01-source-inventory.md) remains intact;
later functional ownership is recorded in the [port audit](../upstream-port-audit.md).

Source: [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
full revision `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
The maintained per-module [manifest](../upstream-baseline.json) owns reproducible
local provenance; this document does not replace its exact/adapted distinction.

## Source Create File/Folder workflows restored on 2026-10-10

Source RenameFile and OnBeginLabelEdit activation is now retained in the same
module: focus chooses the target independently of operated selection, and Qt's
EditLabel callback is only a control transport. Source policy no longer disables
Rename solely for multiple marks. This does not close remaining PanelOperations
bodies or FileManager family disposition; see the
[activation receipt](../upstream-port-audit.md#p3-05--source-rename-activation-scenario-2026-10-10).

The previous PanelOperations helpers kept validation/operation fragments while
MainWindow supplied independent dialogs/errors/selection. Complete source
CreateFile/CreateFolder bodies now own those phases and refresh policy in their
original module, using the existing Qt reload callback. CreateFile's original
direct provider call replaces the extra worker branch. Both normalized bodies
match after dialog boundary substitutions; see the [source receipt](../upstream-port-audit.md#p3-05--source-create-filefolder-scenario-2026-10-10).
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
See the [source receipt](../upstream-port-audit.md#p3-05--source-delete-lifecycle-2026-10-10).
The bounded checks close the identified Delete lifecycle substitution;
remaining PanelOperations/FileManager full disposition stays open.

## Split/Combine owner and native workers restored on 2026-10-08

Existing mirrored names concealed absent CApp::Split/Combine, substituted native
types/NIO and omitted CPreAllocOutFile. The complete scenario now uses original
application bodies and worker algorithms with narrow Qt progress, I/O API and
publication adapters. Original discovery/summary/output-name blocks have one
shared implementation. The [source receipt](../upstream-port-audit.md#p3-05--complete-source-splitcombine-scenario-2026-10-08)
supersedes earlier claims that native preallocation and source discovery were
unusable. This closes those omissions, without closing the FileManager family.

## Filesystem comparator restored on 2026-10-08

Existing FSFolder.cpp/h names concealed an omitted IFolderCompare interface,
QI and CompareItems body. They now retain the source switch and cache phases;
the panel consumes that interface through its existing binding/comparison path.
The authoritative PanelSort.cpp natural-name helper is shared by selective
compilation, avoiding a filesystem/panel cycle. Portable attributes/time services
replace only their Win32 representations. See the [body, dependency and check
receipt](../upstream-port-audit.md#p3-05--source-filesystem-comparator-2026-10-08).
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
| `FileManager/EnumFormatEtc.cpp` | `CEnumFormatEtc` owns Next/Skip/Reset/Clone and target-device copies. `PanelDrag.cpp::CDataObject::EnumFormatEtc` supplies one format for DATADIR_GET through `CreateEnumFormatEtc`. | Byte-exact Windows dependency is compiled and used by the restored native source IDataObject. POSIX delegates enumeration to Qt. Automated native consumers and physical peer acceptance have separate receipts; the previously accepted Finder workflow is not reopened. |
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

### Operation prompt bodies and modal lifetime, 2026-10-10

Complete original/current PasswordDialog, OverwriteDialog, MemDialog and
MessagesDialog `.cpp/.h`, all four desktop RC/Res headers and Overwrite's CE
resource were reviewed at the pinned revision above. The source module/language
boundaries and one authoritative algorithm per prompt remain intact.

| Source owner/body | Copied/adapted/omitted scope and dependencies |
| --- | --- |
| PasswordDialog ReadControls, SetTextSpec, OnInit, OnButtonClicked, OnOK; header defaults | Retained read/toggle/read/accept flow and initial hidden-password policy. Qt replaces control attachment, echo mode, event dispatch, modal base, label buddy and resource language binding. Its QString widget boundary converts once into caller-owned UString; it introduces no password parsing or trimming. OpenCallback and ExtractCallback preserve cached password/asked/defined state, accepted-only show-password persistence and E_ABORT on refusal. UpdateCallbackGUI2 uses the same GUI-thread presenter with source success/refusal policy. |
| OverwriteDialog ReduceString, AddSizeValue, SetFileInfoControl text, OnInit, OnButtonClicked; CFileInfo setters/defaults | Retained72-character shortening/trailing-space quoting, byte/KiB/MiB/GiB thresholds, optional time/size text, old/new order, native decision IDs, extra-action visibility and requested No default. QFileIconProvider replaces SHGetFileInfo/HICON ownership; optional second EXE/ICO icon and CE30-character layout are explicit platform presentation omissions. Qt owns positioning/destruction and Cancel transport. MapButtonResult is the same narrow native-to-extraction-answer mapping used by Qt consumers; source AskOverwrite retains its original switch/E_ABORT policy. |
| MemDialog EnableSpin, AddSize_GB, AddInfoMessage_To_String, OnInit, OnButtonClicked, OnContinue; header defaults | Retained RAM rounding, strict allow comparison, source min/max arrows, independently typed Required_GB, hidden Remember, action defaults, parser/range and NeedSave/Remember/SkipArc-before-validation order. Qt widgets/arrows replace edit/up-down and controls; ReloadInfoMessage extracts only existing source text for language changes. Invalid input now reaches the retained MyMessages Critical/7-Zip/question-suppression owner. The dialog still remains open with raw input/unchanged Limit_GB. Inactive action-combo/skip-file/debug branches stay inactive. |
| MessagesDialog AddMessageDirect, AddMessage, OnInit, OnSize; header/RC | Retained zero-based numbering, newline/trailing-empty-row splitting, two columns/autosizing and source list/footer resize equations. Qt replaces list attachment, DLU/font metrics, language notifications and modal dispatch. Create now blocks until dismissal and returns native IDOK/IDCANCEL; the former modeless dispatch was invalid for PanelDrag's stack-owned source caller. Unused modeless append/reset extension is removed with its obsolete journal workflow already removed under P2-09. |

Caller review includes complete OpenCallback.cpp/.h and UpdateCallbackGUI2.cpp,
source/current ExtractCallback AskOverwrite/CryptoGetTextPassword/RequestMemoryUse,
PanelDrag's source/current terminal diagnostics branch, Qt callback dialog/password
thread boundaries and MainWindow's existing process/filesystem prompt consumers.
ProgressDialog2's UpdateMessagesDialog is a separate progress-list owner and does
not call CMessagesDialog. Callback-owned questions/settings and joined GUI-thread
dispatch remain narrow integration seams; whole ExtractCallback/PanelDrag/native
desktop acceptance is not inferred from these prompt bodies. Original Win32
ResourceString/LangUtils, FILETIME/size conversions, upstream COM answer flags and
RAM/error/string helpers retain their existing dependency owners. No archive
algorithm, native service registration or upstream file is changed.

The subsequent [password caller reconciliation](../upstream-port-audit.md#p3-05--password-state-and-process-callers-2026-10-10)
restores the usable original `UString Password` DTO. ReadControls/SetTextSpec
convert at QLineEdit; AskCallbackPassword copies the portable string directly.
The source read/toggle/read/accept sequence, ShowPassword default, RC order,
language IDs, GUI-thread dispatch and native callback state remain authoritative.
The MainWindow process-password slot's extra accepted-empty veto is removed:
its public asynchronous signal path after Test reproduces cancellation before
the fix, whereas opening routes through COpenArchiveCallback and already accepts
empty. Backend providePassword retains its existing raw UTF-8/newline transport.
Reject still cancels. No trimming, validation or password algorithm is added.
Win32 control/message/modal services remain the narrow Qt presentation adaptation;
this closes these two caller/type debts, not whole FileManager acceptance.

See the [validation receipt](../upstream-port-audit.md#p3-05--operation-prompts-and-source-modal-lifetime-2026-10-10).

### PanelMenu complete body/caller disposition, 2026-10-10

Complete original1160-line/current PanelMenu.cpp and its types/callers were
reviewed at pinned upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
The [source receipt](../upstream-port-audit.md#p3-05--source-panel-context-menu-lifecycle-2026-10-10)
owns provenance, validation and current limitations.

| Body family | Retained source and adaptation |
| --- | --- |
| EditCut/EditCopy/EditPaste | Source Cut/Paste are inactive commented shell calls and remain no-ops. Copy retains selected item/path concatenation; QClipboard replaces SetClipboardText. |
| Properties | Original item/raw-property, folder, archive-level and NonOpen property traversal/order remains; Qt displays the source property collector through an explicit-index overload. Native System properties is applicable on Windows, not omitted globally. |
| Shell context creation and invocation | Original native PIDL/Query/Invoke body remains on Windows. HWND/control/menu transport uses winId, CMINVOKECOMMANDINFOEX and native Qt menu services. Portable source plugin numeric routing/error/selection rules are shared. |
| CreateSevenZipMenu/CreateFileMenu | Original selected-path, directory, read-only/hash/FS/ADS policy and CFileMenu load remain. Explorer source trees and source file rows have Qt ownership/rendering services; original engine services execute archive operations. |
| IsReadOnlyFolder/IsThereReadOnlyFolder/CheckBeforeUpdate | Original provider/parent/read-only checks and source error construction remain. Qt message services replace Win32 presentation. |
| OnContextMenu | Complete original header/list/keyboard/empty/focus/Cancel/plugin/file routing is restored. Qt supplies handle identity, visual row coordinates, popup RAII/Track and source dispatch transport. Table/icon/header consumers capture their own pane. |
| Inactive/presentation scope | Commented context hit-test and debug SHOW_DEBUG paths do not define additional active desktop workflows. Native CMenu/CMenuDestroyer, HMENU and screen-coordinate APIs are replaced by scoped Qt services, retaining source branch order. |

This disposition covers PanelMenu; rendered native Windows reference and the
remaining FileManager/package/final acceptance stay separate.

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
[receipt](../upstream-port-audit.md#p3-05--six-options-pages-and-apply-errors-2026-10-10).

### PanelSelect complete body/caller disposition, 2026-10-10

Complete original/current PanelSelect.cpp and Panel.h command declarations were
compared with source App.h, MyLoadMenu dispatch/status, App::SetListSettings,
Create's LVS_SINGLESEL, QtPanelList and the MainWindow selection consumers.
The [source receipt](../upstream-port-audit.md#p3-05--source-selection-command-lifecycle-2026-10-10)
owns current validation and provenance.

| Body family | Authoritative implementation and adaptation |
| --- | --- |
| OnShiftSelectMessage, OnArrowWithShift | Original range/previous-focus/select-mark walk remains. Qt dispatches the post-movement phase after its view moves focus instead of posting the native custom message, and projects changed marks into model roles. |
| OnInsert, OnLeftClick, UpdateSelection | Original index, parent exclusion, toggle, next focus and Shift/Control range rules remain. Qt uses selected/marked model state and invalidation rather than ListView state/custom drawing. It explicitly restores single focus in alternative mode. |
| SelectSpec, SelectByType | Complete mask-dialog lifecycle now belongs to CPanel; the explicit-mask entry shares the original wildcard loop. Type/extension and no-extension loops remain source code. Qt refuses its synthetic parent index before querying provider properties. |
| SelectAll, InvertSelection, KillSelection | Original mark-vector loops and final UpdateSelection remain. Commented legacy single-selected-item inversion and refocusing after KillSelection remain omitted inactive branches. |
| EnsureSelectionVectorSize, SetAlternativeSelection, BeginShiftSelection, IsItemMarked | Narrow Qt integration for prepopulated models, live QAbstractItemView selection-mode changes, key-event phase capture and public mark roles. Source load/OnItemChanged remain authoritative mark initialization/update; native LVS_SINGLESEL/GetStyle/SetStyle are control services. No alternate wildcard or selection algorithm is added. |
| Dependencies and inactive platform scope | Source UString/CRecordVector, Wildcard, language IDs, CComboDialog, source focus/index/mark state and QtPanelList remain shared. Old CE modifier suppression, Win32 notification structs/PostMsg/RedrawItem and commented Up/Down handlers are not separate desktop workflows. |

This maps the complete PanelSelect source scope; whole FileManager/native/
rendered/final acceptance remains separate.

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
[source keyboard audit](../upstream-port-audit.md#p3-05--source-keyboard-dispatcher-2026-10-07).

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
| Set flat mode; LoadItems; InitColumns; count and directory validation | `LoadListItemsImpl` retains these source branches. Worker-loaded FS passes its retained HRESULT instead of enumerating twice. Original written-count policy ignores the count HRESULT; a narrow optional retains native numItems across namespace publication/restoration/mark projection. Release/rebind invalidate it; the next load starts with known empty rows/marks so early failure/Cancel does not query metadata during mark projection. Unwritten output starts at zero as a defined Qt adaptation. Directory VT_BOOL/VT_EMPTY validation and cache clearing retain the source failure contract. See the [count consumer evidence](../upstream-port-audit.md#source-refresh-loaded-count-consumers-2026-10-07). |
| Deleted-item properties; icon interface/attributes; insert callback rows | Deleted-item metadata and icon-interface selection remain in `LoadListItemsImpl`; `LoadItemIcon` runs the extracted source icon branch during bounded GUI publication. Native callback text is supplied by `ArchiveTableItem::data` and source `GetItemText`; the independent per-cell count/range gate is removed, restoring direct property lookup for successfully published indices. Eight EN/RU both-pane detail/icon count/property-result, notify/provider guard, recovery and actual rebind consumers preserve peer/selection/bytes; load-time validation and generation cancellation remain in their existing owners. Native LVITEM, insertion and system image-list services are Qt/platform adapters, not copied Win32 presentation. |
| Parent row, fast-name/flat-prefix selection, restore notify | GUI publication creates source-indexed rows; restoration walks those rows and retains source name/prefix matching and mark vectors. The source notification restoration precedes focus callbacks. Native parent selection and Qt alternative single-focus adaptation have separate recorded regressions. |
| Named focus; source sort; positional fallback; EnsureVisible; final status | Restoration now retains that order, resets the group anchor before the walk, updates Qt mark paint and publishes the original four-part status through the weak owning-pane callback. Initial renderer/header sorting is a separate Qt publication step; it does not substitute for this source sort phase. |

| Active caller route | Load and completion boundary |
| --- | --- |
| Explicit reload, timer, filesystem/namespace Refresh, post-Copy destination | `RefreshListCtrl_SaveFocused` calls original RefreshListCtrl, which invokes attached `reloadPanelPresentation`; synchronous render HRESULT reaches original `OnReload`. Native archive projection captures the same scoped source result continuation for deferred completion, restoring guards/redraw before reporting and holding projection depth until reporting finishes. Cancelled/replaced generations discard that continuation. Qt publication completion uses the extracted RestoreLoadedSelection tail without recursively reloading. FS workers retain their load result through the source load stage and deferred restoration. `QtAppPresentation::CompleteCopyIfReady` waits for the operation owner before destination reload and source completion. Real filesystem Copy/Move with a native301-row archive peer covers deferred errors, source cleanup/focus and recovery; native writer-failure consumers remain separate. |
| Initial creation and OpenAsArc_Index | Both original callers ignore RefreshListCtrl's result. Their bounded native archive publication now has no OnReload error continuation. OpenAsArc_Index releases binding guards before Refresh retains its own; this prevents asynchronous completion from restoring disabled notification/timer flags. Direct source open, initial creation, retry and native S_FALSE/E_FAIL/E_ABORT are covered; nested OpenItemInArchive and other operation renderers still need their complete caller/guard review. |
| Parent/root/drives/AltStreams, same/subfolder, address binding | Source folder changes clear old rows and save pending selection. `goParent`, `goRoot`, `QtFileManagerCommands::OpenAltStreams`, `QtAppPresentation::RefreshFolder` and the address navigation callback then render the bound provider. `renderBoundFolder` takes pending selection before load and restores it only on successful publication. Namespace load/property/icon failures cancel stale projection and finish Qt sorting/notify/busy state, with address publication before load unless timer. Parent now publishes written count despite metadata HRESULT; actual load failure/Cancel/recovery preserves both panes and the original ignored reload-result policy. Archive/FS navigation failures and remaining void operation result consumers still require review. |
| Comment | Original void ChangeComment owns captured selection and guarded RefreshListCtrl; its presentation service loads before restoring, including deferred publication. Early exits/Cancel do not load; failed loads do not restore stale rows. Qt retains source progress lifetime and the source Create scan handoff. The [current receipt](../upstream-port-audit.md#p3-05--paneloperations-disposition-and-source-comment-lifecycle-2026-10-10) supersedes the former reload-flag/output-state signature while preserving Cancel/failure/recovery behavior. |
| Rename | Qt `itemChanged` captures raw EditRole/source index and queues `finishFilesystemRename` after the editor callback. Source timer/notify guards cover the operation worker. Original saved names/new-or-old focus/SelectFocused pass through the pending refresh channel; mainwindow_rename.cpp loads before restoring and checks render S_OK. Original `_dontShowMode` controls source formatter/cached Qt DisplayRole until load/restoration. Early validation adapts native FALSE without loading. Completed-worker errors belong only to source progress; the extra Qt warning was removed after actual native RED. Native F2 writer success/Write failure/Cancel, later property publication failures, Ctrl+R and real writer retry cover this consumer. Native ADS/platform and changed-parent/full-body acceptance remain open. |
| Drop into archive / CopyFromNoAsk | `PanelDrag::CompressDropFiles` reaches source `CopyFromNoAsk` for an empty destination folder path. Its success-only Refresh phase and saved state now pass through an optional output to QtFileManagerDrag::ExecuteDrop after source timer/notify guards unwind; default source callers retain immediate Refresh. Qt publishes/restores that one bound provider and completes source list focus after deferred rows. Failure/Cancel/confirmation refusal returns without that phase. Native source update owns archive ReOpen/proxy restoration in ArchiveFolderOut.cpp; no compensating all-visible ReOpen returns. Eight bounded native writer/error/Cancel/refusal, later attribute HRESULT, Ctrl+R and actual retry cases cover guards/focus/bytes/peer/OnTimer. Current source RED, driver diagnostics and three-OS evidence are in the dated port audit; rendered and whole-body acceptance remain separate. |
| External-file update | Source OnOpenItemChanged retains folder/index/name checks, saved selection, operation and unconditional RefreshListCtrl before its update-result branch. Optional CSelectedState now transports that reached phase to Qt; runExternalArchiveUpdate publishes after guards unwind for failure/Cancel as well as success. Worker read-only and source early refusals do not publish. Restore follows only successful render; reload HRESULT retains native ignored diagnostic policy. Native external2/301 consumers now verify actual official writer success/Write failure/source Cancel, later native property publication failure, Ctrl+R and actual F4 writer retry with peer focus/bytes/guards. Source/real editor matrix and provenance are in the port audit. Native rename/drop/changed-parent consumers and platform acceptance remain separate. |
| Manual archive Refresh/Ctrl+R | Original CApp::RefreshView inline body is restored in App.h; Qt dispatch invokes it for all providers. Bound CAgentFolder is loaded/published by OnReload's existing presentation callback, without engine ReOpen. Real ZIP/7z both-pane/action/key/EN-RU/view/selection consumers and nested-folder repeated refresh cover this boundary; native platforms remain open. |
| Filesystem-to-archive Copy/Move | Original OnCopy saves destination state before CopyFrom and refreshes its bound provider. Qt success now uses that pre-update state with RefreshListCtrl, rather than a second ReOpenArchiveFolder/provider replacement. Failure/Cancel retain existing unconditional destination OnReload and source continuation. Native ZIP/7z,301-row publication and transport failure/cancellation consumers are in copy_archive_refresh. Interarchive Move rollback is separate. |
| Interarchive Copy/Move | Successful destination publication uses pre-update saved state and original RefreshListCtrl on the existing provider. Source CopyTo/CopyFrom own staging/update; accepted Move composes source Delete and backup rollback because native CopyTo(moveMode) is E_NOTIMPL. Delete No/Cancel/error/engine cancellation restores exact destination bytes and then physically reopens with pre-update state. Before backup publication, the caller supplies source CAgent::Close through a narrow CAgentFolder bridge, releasing native read sharing without coupling the standalone backend to Agent. interarchive_refresh covers native ZIP/7z,301-row publication, both views/panes and controlled transport phases. interarchive_recovery adds actual native reopen failure/Cancel to restore failure, filesystem fallback and repaired-destination retry; archive_transfer_backup covers refused close/retry. Current platform evidence is in the dated audit; rendered and whole-platform acceptance remain separate. |
| Other operation archive reopen | `refreshCurrent` reopens through `ReOpenArchiveFolder`, then renders/restores. Drag and successful filesystem/interarchive destination completion no longer call it. Physical interarchive rollback deliberately retains this reopen. It does not share the synchronous `OnReload` result consumer for every remaining operation renderer call. Remaining source `PanelItemOpen` continuations and native renderer failure require full-consumer reconciliation. Source OnCreate now owns initial and late-panel publication through the service installed with its controls; constructor/toggle no longer publish again. Its original ignored initial Refresh HRESULT policy is retained. |

Rename validation follow-up2026-10-15 restores source OnEndLabelEdit's two
E_INVALIDARG message calls before returning, and removes the caller's generic
validation warning. Invalid edits retain the no-reload path and saved selection.
The archive pre-validation branch is removed as well: source Agent RenameItem
owns name/path transformation, with its old/new roots carried to the CLI adapter
and checked by original CRenamePair. This closes the adapter's same-depth
restriction for source callbacks; backend path-safety and whole native/rendered
acceptance remain separate boundaries.
[Source error and native-modal fixture receipt](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-05--source-rename-validation-and-cocoa-modal-fixtures-2026-10-15).

Source creation's Qt splitter insertion now follows FM.cpp::MoveSubWindows:
panel0 left/panel1 right regardless of creation order. This geometry service
does not change the retained App creation/binding algorithm.
The native error matrix also invokes actual filesystem OpenItem before
OpenAsArc_Index, handing off both outer and inner guards. Post-recovery
filesystem navigation and OnTimer verify the retained timer state. This does
not cover the separate nested OpenItemInArchive branch. The later
[native nested component](../upstream-port-audit.md#p3-0515--native-nested-streams-and-publication-handoff-2026-10-07)
reviews its complete body and both callers, restores original Agent::GetStream,
and verifies all three successful publication handoffs plus actual native Cancel.
Other external-edit/changed-parent/zone and native writer/render failure branches
still require complete reconciliation and platform acceptance; this component
does not close the full module.

The initial map made no functional edits, source copies or manifest changes.
Its Comment row now records the subsequent integrated caller correction. Existing
RED/GREEN and geometry evidence remains bounded to the individual phase fixes
in [the port audit](../upstream-port-audit.md#source-refresh-alternative-selection-anchor-2026-10-06).
Next work must exercise the outstanding operation-to-render result consumers,
remaining native drop/changed-parent workflows, terminal redraw and provider boundaries,
then reconcile remaining `PanelItems` bodies/dependencies/resources. A green
whole repository gate alone does not establish those source-body dispositions
or current Windows/Linux/rendered/cold-stable acceptance.

### PanelItems complete body map, 2026-10-06

Reconciled again on2026-10-10 against the complete original1476-line module
at the same full pin and the adapted931-line module at458bfbb. Original35
active bodies/four column helpers, Panel declarations, selection/notification/
sort/folder-change callers, snapshot constructors and synchronous/deferred
MainWindow publication were reviewed. The following correction supersedes the
former unspecified providerless count and forced alternative-focus adaptation:
`GetFolderItemCount` retains the source load count across Qt phases, otherwise
queries the actual provider; with no provider it returns zero, never model rows.
Every snapshot constructor already creates QtPanelFolder. Release/rebind clear
rows, marks and the count; attachment keeps its notification connection for a
later real provider without inventing marks from stale presentation rows.
`RestoreLoadedSelection` passes original `state.SelectFocused` unchanged to
SetFocusedSelectedItem. LVS_SINGLESEL constrains multiplicity but does not
select a merely focused row: the exact native state/mask calls were verified
on Windows11 ListView in both source styles. Source marks, focus and selected
rows remain independent, including original status/operated-index guards.

The remaining extraction boundaries are intentional Qt services: load/caches
before row publication, source-indexed callback rows/parent sentinel, source
restore and sort after publication, and redraw/enabled state through the owning
scan/operation lifecycle. QtFolderProjection retains the provider per batch,
invalidates generations on cancel/rebind and clears its completion before
calling it; MainWindow publishes only the owning pane and restores guards before
deferred result callbacks. The phase/caller map above retains individual source
result policies. GetItemName's unused output-reference overload has no separate
active consumer here; the value overload preserves its property/type exceptions
and supplies the pointer consumed by restoration/icon matching. Win32 callback
messages, redraw, column/control structs, image lists, popups and timers remain
presentation/platform substitutions; debug/commented branches stay omitted.
No archive algorithm, extra upstream dependency, source language or baseline
changes. Body disposition is distinct from native/rendered/final acceptance;
the dated [receipt](../upstream-port-audit.md#p3-05--panelitems-selection-and-provider-lifecycle-2026-10-10)
owns current validation evidence.

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
| InitColumns | Same module retains folder/raw-property enumeration, individual info failures skipped, descriptor types and saved visibility/order/width, name protection, visible/name-first sorting and sort default. Original mark clear now precedes enumeration on every call; equal-column early return and DeleteColumn/AddColumn rebuild restored. Original CIntArr order validation is extracted as GetColumnOrder for deferred Qt physical publication: source logical Name stays first, visual Name may move elsewhere. Direct unchanged-catalog marks/rows/selection/restore and EN/RU both-pane move/Refresh/reopen/width consumers pass. Publication now gives each catalog entry a separate Qt section and uses its exact descriptor for cells/alignment/visible width/order, including generic/raw CRC and reserved Size overlap. Both-pane EN/RU guard/error/Cancel/Refresh/peer consumers pass. Duplicate-ID menu projection now retains first-visible-ID deletion and appended copies with separate visible logical mappings, callback cells/widths and selection; both-pane EN/RU repeated toggles pass. Overlapping CRC/Size header sorting now uses source first-catalog raw flag and same-PropID toggle through generic/raw/append sections;16 EN/RU both-pane copies/Refresh consumers pass. Written generic/raw count enumeration now retains original ignored-HRESULT loops, with existing initialized-zero outputs; individual info errors/IsDir are skipped. Same-provider raw-Name/no-Name/empty/unwritten/recovery consumers retain source defaults/logical indices and remove phantom Qt Name, with rows/selection/menu/peer preserved. Label-only selection now resolves native visible subitem zero when reserved Name is hidden; actual delegate-image RED4 and focused consumers cover raw Name/no Name. Icon/list projection now resolves the same logical-zero text and existing row icon after schema publication/menu mutations; four view modes cover raw-Name hide/re-add, no-Name/empty/recovery with peer focus/selection retained. Real FS/ZIP/7z defaults, independent per-type width/order/visibility, unchanged equality and separate Type/Name changes now have an integrated EN/RU both-pane workflow; see the [dated evidence](../upstream-port-audit.md#source-real-provider-column-workflow-2026-10-07). Provider failures and raw/empty/duplicate schemas retain the focused evidence above; current native/rendered acceptance remains open. |
| DeleteColumn, AddColumn | Original visible-vector bodies now remain in PanelItems and are called by ShowColumnsContextMenu and restored InitColumns rebuild. Win32 LV_COLUMN insertion/removal and column-order-array transport become Qt hide/show/append and deferred source order-array application with compact visible order persistence. Both-pane menu/move/width/Refresh/reopen and direct unchanged-catalog consumers cover these boundaries. Qt now maps native visible indices separately from the catalog and creates additional callback sections for appended copies, preserving first-ID deletion, widths and selected rows. Overlapping CRC/Size header sorting retains source flags/toggles after Qt mouse dispatch and across appended copies. Real-provider defaults/settings and separate Type/Name equality changes now have integrated coverage; current native/rendered acceptance remains required. Path presence alone does not prove whole column closure. |
| RefreshListCtrl() / RefreshListCtrl(state), RefreshListCtrl_SaveFocused, SetFocusedSelectedItem | Retained source restoration and extracted LoadListItems/LoadItemIcon with Qt row publication. The phase/caller map above owns remaining terminal/async/error boundaries. SaveFocused calls RefreshListCtrl, which invokes the attached load/publication callback; source OnReload consumes its HRESULT. Qt render completion calls extracted RestoreLoadedSelection, avoiding recursive reload. Callback-free fixture restoration and deferred operation guards remain explicit boundaries. Pending state transfers synchronous source navigation to Qt publication. |
| GetSelectedNames, SaveSelectedState, Get_ItemIndices_Selected, Get_ItemIndices_Operated, Get_ItemIndices_All, Get_ItemIndices_OperSmart | Same module retains source names/mark vectors, operated focused-row fallback and all-item/provider count. Source OnItemChanged consumes owning-model selected-state deltas behind notification/deletion/alternative guards, including inactive panes. SaveSelectedState reads marks without rebuilding them; pre-populated Qt attachment seeds once from a real provider and load prepares marks before deferred publication. Direct InitColumns restoration uses the pre-clear snapshot. The2026-10-10 lifecycle reconciliation above removes providerless model-row counts and preserves source SelectFocused independently of alternative marks. Dated source/consumer evidence is linked in the audit. |
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
[port audit](../upstream-port-audit.md#unconditional-destination-publication-and-namespace-reload-2026-10-06).
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
acceptance is recorded in the [port audit](../upstream-port-audit.md#p3-054--samesubfolder-source-callbacks-accepted-callback-scope-2026-10-06).
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
on3OS; see [the port audit](../upstream-port-audit.md#p3-055--native-archive-property-levels-and-codec-owners-accepted-property-scope-2026-10-06).

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
is maintained in [the port audit](../upstream-port-audit.md#p3-056--filesystem-description-comments-2026-10-06);
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
The [dated audit](../upstream-port-audit.md#p3-0515--native-ads-root-source-zone-copy-and-bound-filesystem-reload-2026-10-07)
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
[source audit](../upstream-port-audit.md#p3-05--toolbar-extracttest-source-consumers-2026-10-07)
owns platform and regression evidence; this does not close the whole Panel module.

P3-05 remains open. The selected ClassDefs/native SDK boundary is recorded above;
every claimed full source body and resource,
desktop-versus-CE branch, standalone console/GUI consumer and custom Qt algorithm
must be reconciled against its owning functional audit. The source entrypoint gap
found by this review is accepted in the bounded P3-05.1 caller component.
Engine/Qt packaging, notices and own-code licensing belong to P3-04; final
rendered/native UX acceptance belongs to P3-06.
