# Omitted source scope review — dialogs-callbacks

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

[Индекс](../omitted-scope-audit.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

## Primitive input and read-only dialog bodies (2026-10-10)

Complete original/current ComboDialog.cpp/.h, CopyDialog.cpp/.h,
SplitDialog.cpp/.h, EditDialog.cpp/.h and OptionsDialog.cpp were compared at
https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, port parent48866c8. The four
desktop resources, shared dialog macros, source parser/path/control services
and current CApp/PanelSelect/ListView/settings consumers were reviewed too.

| Original body/interface | Retained source policy and narrow Qt boundary |
| --- | --- |
| Combo OnInit/OnOK, header Title/Static/Value/Strings | Original title/prompt/raw value/history insertion and accepted-only Value publication remain. Qt restores initial edit text after insertion because its first addItem selects row0. Editable NoInsert combo, label buddy, focus/select-all, standard buttons and layouts replace native control/modal/OnSize services. Sorted-style code is commented upstream and remains inactive. SelectSpec and Create/Comment callers own mask/name/path validation. |
| Copy OnInit/OnButtonClicked/OnButtonSetPath/OnOK | Original history, title/prompt/info, raw accepted Value, source MyBrowseForFolder refusal, NormalizeDirPathPrefix and selection reset remain. Qt supplies controls/Browse/layout and a read-only scrolling plain-text summary instead of SS_LEFTNOWORDWRAP. Constructor conversion of supplied filenames is integration input; CApp's source selection/destination/history/CopyTo policy is separate. The CE initial-path insertion is outside desktop scope. |
| Split OnInit/OnButtonClicked/OnButtonSetPath/OnOK | Original file-title suffix, path, nine presets/initial index, Browse normalization, UString::Trim, ParseVolumeSizes/VolumeSizes and accept-or-return flow remain. Invalid volume restores the original direct MB_ICONERROR/7-Zip/OK modal message, including source text, rather than the independent inline error label. It leaves raw input and parent dialog open, allows correction, and creates no output. The source parser retains partial values on failure; no new size parser or archive/volume writer is added. |
| Edit OnInit/OnSize, Title/Text header | Original read-only multiline, horizontal/vertical scrolling, raw full text and Close remain. QPlainTextEdit/layout replace native attachment/rectangles; ES_WANTRETURN is preserved by consuming Enter instead of triggering Close. ListView ShowItemInfo still supplies full untruncated values and retains original Windows CRLF conversion. |
| OptionsDialog page construction, MyPropertySheet and accepted application refresh | Original System/Menu/Folders/Edit/Settings/Language order, initial page0, source page state/Apply and CApp SetListSettings/RefreshAllPanels remain. QtFmOptionsDialog owns tab/modal/Help/modified/close services and typed Applied delivery; language notifications refresh the existing menu/toolbar/panel bindings. Its existing Apply callback makes the source application refresh available after Apply/OK. The [complete six-page/body/property-sheet map](filemanager.md#six-options-pages-and-apply-error-policy-2026-10-10) owns page error/veto/continuation policy. Commented SysIconsWereChanged/PostMessage alternatives and CE resources remain inactive/outside desktop presentation. |

This replaces the old inline Split-error adaptation in the UI contract. Native
QMessageBox still determines its own geometry and Windows button order;
macOS ignores its supplied title. All four source dialog owners retain C++ and
their paths/notices; no borrowed dependency or baseline change is introduced.
Existing portable strings/vectors/path/parser/language/folder interfaces remain
authoritative. Qt widgets do not perform archive/volume operations: CApp's
accepted Split/Combine worker and ArchiveBackend/official-engine boundaries
remain separate. Current checks/geometry and acceptance limits are recorded
in the [receipt](../upstream-port-audit.md#p3-05--input-dialog-source-and-split-error-policy-2026-10-10).

## ProgressDialog2 complete body disposition (2026-10-10)

Complete original/current FileManager/ProgressDialog2.cpp/.h were compared at
port parent fc4012e with https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`: all1483 original and1316 adapted
implementation lines, inline declarations, ProgressDialog2.rc/2a.rc/Res.h and
GuiCommon.rc desktop macros. Normalized comparison separates formatting from
functional platform boundaries. The authoritative adapted C++ owner remains
under its original path; reusable resource/type dependencies remain vendored.

| Original active body/interface family | Retained owner and platform boundary |
| --- | --- |
| CProgressSync constructor, stop/pause access, CheckStop, Clear_Stop_Status, ScanProgress, total/current/ratio/title/status/path/error setters | Original locked state, undefined sentinels, cancellation HRESULT and100ms multi-producer pause polling remain. The existing opt-in single-worker wake uses the upstream synchronization event; QtWorkerProgress's publication lock prevents cancelled output publication without replacing source progress state. |
| GetTimeString, ReduceString, ConvertSizeToString, GetChangedString, GetPower32/64, MyMultAndDiv, GetNumDigits | Original formatting, truncation, integer scaling and overflow behavior remain, including byte-based speed during files-progress. No Qt number/statistics algorithm replaces these bodies. |
| CProgressDialog construction/destruction, AddToTitle, OnInit/OnSize, EnableErrorsControls | Same initial source fields, control IDs/groups/order, language strings, icon branch, progress/list setup and rectangle calculation. Qt font-DLU/control/move/work-area services replace HWND calls. NormalizeSize delegates to the authoritative mirrored CDialog source body through one-shot native first-Show services; source policy supplies no minimum size. [Common geometry scope](../upstream-port-audit/p3-06-modal-geometry-2026-10-17.md) includes all callers and modal chrome preservation. Source title ownership is released before queued backend completion restores the application title. |
| SetProgressRange/Pos, ShowSize, UpdateStatInfo, SetTitleText | Original undefined-total, range reduction, cached repaint, elapsed/pause accounting, remaining/speed/ratio, path splitting and percentage title flow remain. CQtProgressBar translates PBM range/position, including native clamping; Qt timers/resize/show events deliver original handlers. |
| SetPauseText/OnPauseButton, SetPriorityText/OnPriorityButton, SetTaskbarProgressState | Same state transition/captions and taskbar precedence. ArchiveProgressTransport supplies actual process pause/priority; callback workers explicitly lack pause. Windows retains ITaskbarList3 and upstream process helpers. Darwin uses process priority and an AppKit Dock badge; Linux uses thread scheduler rollback and the existing launcher DBus adapter. Failed priority delivery preserves state and records a source Sync error. |
| AddMessageDirect/AddMessage/UpdateMessagesDialog, ListView_GetSelected/CopyToClipboard, OnNotify | Same numbered first lines, unnumbered continuations, source newline/selection/all-row clipboard policy and message counter. Qt list/clipboard/key delivery replaces Win32 presentation; macOS additionally accepts Meta. |
| OnOK/OnCancel, OnButtonClicked, OnExternalCloseMessage, OnMessage, CheckNeedClose | Same source no-op OK, YES/NO/CANCEL/default/escape cancellation, temporary pause and restoration, finish deferred through an open question, final-error/list/Close and success-message transitions. Qt accept/reject/queued delivery replace native messages. Two invented Qt icons were removed: original MB_YESNOCANCEL and success MB_OK have no icon flags; MB_ICONERROR remains Critical. |
| Create, ProcessWasFinished, MyThreadFunction, CProgressThreadVirt::Create/Process, AddMessageToString/HResultToMessage | Original creation delay, WaitMode, worker result/exception/error composition, event sequencing, joined lifetime and GUI finalization remain. Windows uses native source handles/waits; POSIX QtProgressStartWait supplies two-latch wake/deadline with finished-first precedence and joins the upstream CThread. ShowAfterMessages is only an upstream declaration, with no implemented body/caller to recover. |

All Qt progress controls, wait, taskbar, process-priority, archive/callback
transports and volume/worker progress adapters were read with their headers.
Direct source consumers include UpdateGUI/ExtractGUI/HashGUI, PanelCopy,
PanelCrc, PanelOperations and PanelSplitFile; callbacks assign their progress
owner before joined Create. MainWindow begin/worker/completion and
ArchiveProgressOperation retain GUI-thread backend delivery through
ArchiveBackend and the official engine. An exceptional launch cancels/joins
the child before releasing its progress owner; completion is still observed
during pause. QtVolumeProgress delegates source setters and checkpoint return
values; it does not supply a new split/combine algorithm.

CMake compiles the same source separately as Sync-only and presentation-only
objects, linking one authoritative Sync implementation to each consumer.
Native Windows taskbar/COM/thread/process resources, portable strings/vectors/
synchronization/error/language helpers and existing Qt platform adapters remain
the dependencies. The AppKit bridge remains Objective-C++ for the native API;
original C dependencies retain C compilation/linkage. No new borrowed file,
dependency, baseline or language change follows. CE dialogs/priority/taskbar
branches are outside the desktop target; original inactive experiments remain
inactive. Selected official SFX modules build their own unchanged sources.
The [receipt](../upstream-port-audit.md#p3-05--progressdialog2-source-and-message-policy-2026-10-10)
owns validation and remaining native/rendered/package/final acceptance limits.

## Extraction and opening callback body disposition (2026-10-10)

Complete original/current FileManager/ExtractCallback.cpp/.h and
OpenCallback.cpp/.h were compared at port c601400 with source
https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Formatting-normalized complete
comparisons separate unchanged algorithms from the adaptations below. The
1272-line original ExtractCallback and its1161-line adapted owner retain their
original C++ module paths; all four manifest owners remain authoritative.

| Original active body/interface family | Retained owner and actual integration boundary |
| --- | --- |
| CExtractCallbackImp constructor/destructor, Init, IsOK, COM QI/interfaces | Original defaults, language initialization, archive/message error counters, stream/hash/operation interfaces and portable UString/BSTR state remain. SettingsPath and callback-owned DisableUserQuestions replace implicit registry/global delivery. GUI.cpp explicitly forwards parsed YesToAll; ordinary FM callbacks default to questions enabled. |
| SetNumFiles/SetTotal/SetCompleted/SetRatioInfo, Open_CheckBreak/SetTotal/SetCompleted/Finished | Same source Sync totals, ratio, MultiArc suppression, undefined totals, current files and CheckStop policy. Open_SetCompleted's unused byte input remains as in source; no fabricated progress is added. |
| PrepareOperation, SetCurrentFilePath2/SetCurrentFilePath, BeforeOpen, ThereAreNoFiles | Same mode-label selection, file/folder and pending-file counts, archive title/path and cancellation ordering. BeforeOpen resets archive-name diagnostics; the no-files callback retains S_OK. |
| AddError_Message/ShowArcPath, MessageError overloads, ShowMessage, Add_ArchiveName_Error | Source once-per-archive name delivery and ThereAreMessageErrors flag remain; Sync owns message storage. No independent widget journal or error accumulator replaces them. |
| SetExtractErrorMessage, SetOperationResult, ReportExtractResult | Source operation-result/language-ID mapping, old-language fallback, encrypted guess versus definite wrong-password text, unknown numeric result, file-path clearing and file/folder counts remain. |
| k_ErrorFlagsIds, AddNewLineString, GetOpenArcErrorMessage, ErrorInfo_Print, GetBracedType, OpenResult_GUI/OpenResult, ExtractResult | Source known/unknown error-bit formatting, warnings, nested levels, detected/forced format, encrypted-open text, NumArchiveErrors and final newline policy remain. ExtractResult preserves direct E_ABORT/disk-full return and handled-error S_OK; callers must still inspect IsOK. |
| AskOverwrite/AskWrite | Original button-answer switch, Cancel/E_ABORT, file/folder conflict distinctions, Skip/Ask/YesAll/NoAll/AutoRename state, original AutoRenamePath and deletion error policy remain. RunCallbackDialog supplies joined GUI-thread construction/modal/destruction. FiTime_To_FILETIME converts destination timestamps on POSIX; it does not alter overwrite policy. |
| SetPassword/CryptoGetTextPassword/Open_CryptoGetTextPassword | Original asked/defined cache, accepted-only raw password and ShowPassword persistence, BSTR conversion and E_ABORT remain. qtcallbackpassword owns GUI-thread modal transport; explicit settings path replaces registry scope. Empty accepted text is not rejected or trimmed. |
| UseExtractToStream, GetTime/GetItemBoolProp, GetStream7, PrepareOperation7, SetOperationResult8 | Source dir/mode/AltStream refusal, expected-size/property checks, virtual-file index/name/colon/Zone state, hash stream attachment/release/finalization and stats remain. Original ignored GetItemBoolProp result and inactive extract/test experiments are not silently rewritten. |
| RequestMemoryUse | Source rounded limits, persisted/forced precedence, immediate allow, report/no-question/no-error paths, remembered skip/allow, Save/Remember, Stop/E_ABORT and archive/file diagnostics remain. RunCallbackDialog marshals the body because even report text uses the existing Qt memory presenter; it adds no memory policy. |
| CGrowBuf and CVirtFile/CVirtFileSystem header state, CloseMemFile, Write/FlushToDisk | Same allocation growth/budget, memory-to-disk switch, partial-write/result handling, name correction, main-file creation failure versus ignored additional-stream failure, close ordering, Zone filtering and attribute restoration. POSIX converts FILETIME into source CFiTime for SetTime; Windows retains read-only base-file ADS handling. Colon names on POSIX are ordinary files, not native NTFS streams. |
| COpenArchiveCallback complete methods/header | Original progress stop/total/current-files/current-bytes, password cache and StartProgressDialog flow remain. QPointer<QWidget>/qintptr replace HWND/INT_PTR presentation types and SettingsPath makes persistence explicit. IOpenCallbackUI stays the same portable interface; commented COM/locking/subarchive alternatives remain inactive. |

Direct consumers reviewed: PanelCopy callback/options/hash/password lifetime;
GUI Main2 and ExtractGUI worker/init/join/result; CompressCall2 and
QtShellArchiveServices; QtArchiveBatchExtract; FileFolderPluginOpen,
QtArchiveFolderManager and QtArchiveReopen. PanelItemOpen consumes the virtual
file interface through its existing CopyTo route; its external-process/source
disposition remains a separate component and is not closed here.

Dependencies retain the original C allocator/C linkage, COM/string/variant and
folder/archive/progress/hash interfaces, FileStreams/StreamUtils/FilePathAutoRename,
ExtractingFilePath, OpenArchive/ArchiveOpenCallback, FileFind/FileDir/time helpers,
ZipRegistry, LangUtils/PropertyName/FormatUtils and the four existing prompt
owners. Their resource/layout receipts and Common/Agent audits remain separate.
Original inactive debug/password/stat alternatives stay inactive. Qt desktop
builds do not claim Z7_SFX/Z7_NO_CRYPTO/CE configurations; selected official SFX
bundles compile their own unchanged source. ProgressDialog2's whole presentation
and native/rendered/physical/package/final scope are not inferred from this map.
No functional source, dependency, baseline or UI change follows from the review.
The [receipt](../upstream-port-audit.md#p3-05--extraction-and-opening-callback-disposition-2026-10-10)
owns current checks and the remaining acceptance limits.
