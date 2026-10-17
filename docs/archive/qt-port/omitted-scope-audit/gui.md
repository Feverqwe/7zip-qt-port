# Omitted source scope review — gui

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

[Индекс](../omitted-scope-audit.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

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
The [error-policy audit](../upstream-port-audit.md#p3-05--source-gui-early-error-policy-2026-10-07)
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
are covered by the [consumer audit](../upstream-port-audit.md#p3-05--source-gui-path-and-history-consumers-2026-10-07);
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

The [validation audit](../upstream-port-audit.md#p3-05--source-create-validation-error-policy-2026-10-07)
owns actual RED, recovery, geometry and same-source platform evidence. Whole
CompressDialog acceptance remains open. Its source profile-to-combo default/clamp
selection is subsequently reconciled in the
[profile audit](../upstream-port-audit.md#p3-05--source-compression-profile-selection-2026-10-07),
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
[method-state audit](../upstream-port-audit.md#p3-05--compression-method-and-control-states-2026-10-07)
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
[Benchmark audit](../upstream-port-audit.md#p3-05--benchmark-source-and-platform-lifecycle-2026-10-07).

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
[event audit](../upstream-port-audit.md#p3-05--compress-event-and-memory-phases-2026-10-08).

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
[timestamp audit](../upstream-port-audit.md#p3-05--options-timestamp-control-lifecycle-2026-10-08).

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
to the [format audit](../upstream-port-audit.md#p3-05--format-selection-browse-and-hash-names-2026-10-08).
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
indexed in the [control lifecycle audit](../upstream-port-audit.md#p3-05--compress-control-specific-event-lifecycle-2026-10-08).
