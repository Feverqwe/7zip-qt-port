# P0-01 — historical source inventory

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Archived on 2026-09-30 from docs/upstream-port-audit.md at project commit
`31cb3bb44dcf1ad1b05f9a362db59e12429b3c17`. The original records below are
preserved verbatim, including all module tables, decisions, upstream revisions,
dependencies, omitted symbols, project baselines and historical checks.

This is the 2026-09-28 inventory snapshot, not current implementation status or
a work queue. Later module-specific source records supersede its dated omissions.
Current [audit](../upstream-port-audit.md#p0-01--accepted-source-inventory) and
[checklist](../porting-checklist.md#p0-01--инвентаризация-исходного-переноса)
retain the accepted scope and distinguish inventory from functional completion.

## P0-01.4 — reconciliation и закрытие inventory (2026-09-28)

Project baseline `415513c`; upstream — [ip7z/7zip exact commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Таблицы P0-01.1–.3 ниже — актуальный исходный реестр; более ранние тематические
записи после него сохраняют историю частичных портов, не заменяют omissions.
Все 112 unique paths имеют owner, source scope/dependencies и decision:
30 mirrored modules reviewed against bodies, 82 missing UI modules require
source reuse with local presentation/platform/engine adapters. Inactive
comment-only/definition/build files identified individually. No mandatory
unported function was closed as done or N/A. Headers/resources belong to their
modules; dependency closure is re-reviewed and expanded by each functional owner.

| Область | Upstream UI .cpp | Mirrored UI | Missing UI |
| --- | ---: | ---: | ---: |
| FileManager | 67 | 22 | 45 |
| Agent | 7 | 2 | 5 |
| GUI | 10 | 2 | 8 |
| Common | 25 | 1 | 24 |
| Всего | 109 | 27 | 82 |

Три supporting mirrored paths — CPP/Common/Wildcard.cpp,
CPP/Windows/FileDir.cpp, CPP/7zip/Common/MethodProps.cpp. Coverage проверен
равенством sets всех 109 UI paths плюс всех 30 local src/CPP paths, без duplicate
rows/лишних source paths. Owner IDs относятся к 44 пакетам плана; slash shorthand
внутри группы не создаёт новых IDs. Требующие более поздней capability ветки
остаются у соответствующих owners, включая SFX legacy, VerCtrl, device copying,
email integration, checksum handler и alternate command entrypoints.

Сейчас необходимы: upstream string/vector/property/censor reuse, portable folder
contracts и narrow process/filesystem/platform services; snapshots у ArchiveBackend
не заменяют application bodies. Qt/std replacements в borrowed modules остаются
migration debt, названной в строках. Upstream module boundaries authoritative;
не создавать рядом вторую реализацию. P0-02 следующий по приоритету/зависимостям.

Проверки 2026-09-28 macOS: `git diff --check`; data-processing assertions для
112 unique owner rows, точного coverage и counts; повторная побайтовая сверка
32 vendored source/header + License.txt с `git show <full revision>:<path>`;
upstream HEAD exact baseline, status clean. Все passed. Docs-only, runtime/UX
unchanged, build/CTest/layout не требуются и не запускались. Windows/Linux и
rendered Windows visual parity не проверены. P0-01 done как inventory; 1/44
packages complete, 43 remaining. Это не source/workflow/final acceptance.

## P0-01.1 — реестр первых 16 зеркальных модулей (2026-09-28)

Проверенный project baseline: `a5923d4913f487b46a36c8637165e2a74aa85b1d`,
чистая рабочая копия. Для **каждой** строки оригинал — соответствующий путь
относительно корня [ip7z/7zip на полном commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Локальная реализация находится в `src/<путь>`. Ни одна строка не означает
полный перенос файла. Прочитаны локальные bodies/headers, соответствующие
оригинальные функциональные bodies и непосредственные зависимости.
Formatting отличается отдельно от функциональных изменений.

В таблицах FM = `CPP/7zip/UI/FileManager`, Agent = `CPP/7zip/UI/Agent`.
«Остальное» означает все bodies исходного файла за вычетом явно перечисленного
перенесённого scope; названные группы уточняют владельцев, а не разрешают
пропустить неназванную функцию. Headers входят в scope своих `.cpp`.

| Путь / owner оставшегося scope | Скопированная основа и адаптация | Пропущенные символы/ветки и непосредственные зависимости |
| --- | --- | --- |
| FM/App.cpp — P0-02, P0-08, P1-01 | `AddUniqueStringToHead`, `CFolderHistory::{Normalize,AddString}`, `AddSizeValue`, `AddValuePair1/2`, `AddPropValueToSum`, `CPanel::GetItemsInfoString`: сохранены циклы, sentinel неизвестного размера, лимиты 100/history и summary. QString/русские captions и snapshot заменяют Lang/IFolder/PROPVARIANT. Локальный `CPanel` constructor — glue. | Весь `CApp` и `CPanelCallbackImp`: Create/CreateOnePanel, SetListSettings, toolbar, Save/ReleaseApp, переключение/фокус панелей, OnCopy, OnSetSameFolder/OnSetSubFolder, title/notify. `App.h`, `AppState.h`, `Panel.h`, CopyDialog, LangUtils, RegistryUtils, Win32 controls/synchronization. `CAppState` пока отсутствует; удаление lock допустимо только при GUI-thread ownership. |
| FM/PanelFolderChange.cpp — P0-04 | `SetBookmark`, `OpenBookmark`, `FoldersHistory`: исходные чтение/запись slots и обратное восстановление списка после OK; явные state/settingsPath, Qt dialog и возврат выбранного пути вместо BindToPathAndRefresh. | ReleaseFolder, SetNewFolder, SetToRootFolder, BindToPath/BindToPathAndRefresh, GetFolderPath, LoadFullPath/LoadFullPathAndShow, combo events, GetParentDirPrefix/OpenParentFolder, CloseOneLevel/CloseOpenFolders, OpenRootFolder/OpenDrivesFolder/OpenFolder/OpenAltStreams. Dependencies: RootFolder, FSFolder, FileFolderPluginOpen, OpenCallback, CFolderLink/parent stack, selected state, combo/icon services. Навигация MainWindow — ещё самостоятельная реализация. |
| FM/PanelItems.cpp — P0-06, P0-07, P1-08 | Только `GetItem_UInt64Prop` и хвост `GetItemSize`: parent sentinel и throw при ошибке сохранены; switch четырёх snapshot properties заменяет generic variant. `IsItem_Folder/GetItemRelPath` в локальном Panel.h — snapshot glue. | Fast-path IFolderGetItemName, все прочие свойства; InitColumns и helpers, Add/DeleteColumn, RefreshListCtrl variants, SaveSelectedState/GetSelectedNames, Get_ItemIndices_* (Selected/Operated/All/OperSmart), open/edit, item name/prefix/path access, SaveListViewInfo. Dependencies: IFolder, PropVariantConv, PropID, PropertyName, ExtractingFilePath, Sort, ViewSettings, RootFolder. |
| FM/PanelMenu.cpp — P1-10, P0-09, P2-09 | `AddListAscii/AddSeparator/AddPropertyPair`, ограниченный `AddPropertyString`, single-item enumeration и multi-item totals из `Properties`. Snapshot заменяет COM, числа через IntToString, размер через ConvertSizeToString. | `Properties`: raw/security/hex, folder properties, archive levels/errors и native filesystem properties; остальное — InvokeSystemCommand, EditCut/Copy/Paste, shell context menu, CreateSystem/File/SevenZipMenu, IsThereReadOnlyFolder/CheckBeforeUpdate, InvokePluginCommand/OnContextMenu. Dependencies: ListViewDialog, PropertyName/FormatUtils, PropVariantConv, IFolderArcProps, Windows Menu/COM и Explorer integration. |
| FM/PanelKey.cpp — P0-03, P0-07, P0-08 | `MainWindow::handlePanelKey` — Qt adapter выбранных условий `OnKeyDown` и combo edit: Backspace, Shift+F4, bookmarks Alt+digits, Enter, Alt+F1/F2. Это **не** полный source port OnKeyDown: dispatch через QAction/handleSelectionKey принадлежит MainWindow. | Оригинальные OnKeyDown/FindVKeyPropIDPair и таблица сортировки не скопированы целиком. Остались RightCtrl bookmarks, Tab/F9 panel callbacks, Ctrl+F3–F7, Copy/Move same-panel modifiers, clipboard, alternative selection, same/subfolder arrows, другие command branches. Dependencies: CPanel/CApp callbacks, HelpUtils, PropID, list selection/focus. |
| FM/PanelSelect.cpp — P0-07 | `CPanelSelection::{SelectSpec,SelectByType,SelectAll,InvertSelection}` сохраняют loops и folder/no-extension/extension ветви исходного CPanel. QString/std::vector и Items snapshot заменили upstream strings/status vector; dialog и UpdateSelection вынесены в Qt callers. | OnShiftSelectMessage/OnArrowWithShift, OnInsert, UpdateSelection, KillSelection, OnLeftClick и alternative-selection state. OnUpWithShift/OnDownWithShift в baseline закомментированы, не обязательные активные bodies. Dependencies: Wildcard, ComboDialog, LangUtils, CPanel real-index mapping и list redraw. |
| FM/PanelSort.cpp — P0-06 | `CompareFileNames_ForFolderList` pointer loop скопирован; char16_t сохраняет Windows UTF-16, QChar upper — platform adapter. `GetExtensionPtr` адаптирован. Qt `CompareItems` сохраняет folders-first/name/prefix/index tie-break intent, но заменяет оригинальный property dispatch: это частичная адаптация с долгом. | CompareFileNames_Le16, SetSortRawStatus, CompareItems2 raw/reparse/IFolderCompare/variant ветви, generic трёхпроходный CompareItems, SortItemsWithPropID/OnColumnClick. Закомментированный SortItems не активная ветвь. Dependencies: CpuArch, PropVariant, PropID, FileLink/reparse, CPanel columns/direction. |
| FM/ViewSettings.cpp — P0-06, P0-08, P0-09 | Save/ReadFolderHistory, Save/ReadFastFolders, Save/ReadCopyHistory, Save/ReadStringList и AddUniqueStringToHeadOfList: исходные keys и порядок. QSettings INI заменяет Registry; UStringVector сохранён для history/favorites, QStringList для copy history. | CListViewInfo::Save/Read, CWindowInfo::Save/Read, QueryBuf, Save/ReadToolbarsMask, CListMode::Save/Read, Save/ReadPanelPath и integer helpers. Dependencies: MyString/MyTypes, Registry, Synchronization, binary column/window records. Собственные настройки MainWindow не подтверждают перенос этих bodies. |
| FM/FSFolder.cpp — P0-05, P1-03, P1-10, P2-05/06 | GetAbsPath, CreateFolder/CreateFile/Rename/Delete сохранены из mutation methods; Qt filesystem, bool/ErrorMessage и callbacks заменяют services/HRESULT/IProgress. GetRelPath адаптирует исходный inline helper; constructor/AddPath — snapshot glue. Guards indices, selected-link rejection и recursive cancellation — местная политика. | Init, CFsFolderStat::Enumerate, LoadSubItems/LoadItems, comments, item/property/raw/name/size/prefix getters, ReadFileInfo/ReadChangeTime, CompareItems, BindToFolder*/BindToParentFolder, WasChanged/Clone, Calc/GetItemFullSize, SetProperty, GetSystemIconIndex, SetFlatMode/SetShowNtfsStreamsMode. Dependencies: FileFind/FileIO/FileName/FileDir, IFolder/IArchive, TextPairs, PropVariant, metadata services. Enumeration в Qt MainWindow ещё migration debt. |
| FM/FSFolderCopy.cpp — P1-02, P0-11, P2-06 | `CCopyStateIO::MyCopyFile` 64KiB read/write/error/progress loop; CallProgress, IsDestChild, CombinePath; CopyFile_Ask только read/write branch, CopyFolder только recursive copy skeleton. Qt NewOnly/staging, flush/time, unreadable/link guards вместо native services. | AskWrite/skip/rename/overwrite, MoveMode и source delete orchestration, native CopyFile_NT/CopyFile_Sys/MoveFile_Sys и CopyProgressRoutine, CFSFolder::CopyTo/CopyFrom, totals/CFsFolderStat, ADS/direct destination, error callbacks. Dependencies: FilePathAutoRename, IFolderOperationsExtractCallback, FileDir/Find/IO/DLL/ErrorMsg. Transaction/rollback owner сейчас filesystemtransfer.cpp; это integration, не перенос omitted bodies. |
| FM/PanelSplitFile.cpp — P1-06, P0-10 | CVolSeqName::{SetNumDigits,ParseName,GetNextName}, CThreadSplit/Combine::ProcessVirt loops; AddInfoFileName и summary/output-name blocks из CApp::Combine представлены GetInfo/GetCombinedName. QFile, prevalidated stage paths, bool/error и callback вместо NIO/HRESULT/CProgressSync; flush/source-change guards добавлены. | CApp::Split/Combine как owners selection/destination/dialog/progress/refresh не перенесены целиком. CPreAllocOutFile/preallocation и progress file-count/path/title не перенесены. Dependencies: App/Panel, Copy/SplitDialog, SplitUtils, Format/LangUtils, NIO, ProgressDialog2. Qt worker/transaction остаются adapters; full caller review в P1-06. |
| FM/SplitUtils.cpp — P1-06 | Все три активные функции ParseVolumeSizes/AddVolumeItems/GetNumberOfVolumes и k_Sizes перенесены: parse/overflow/units/comment и repeat-last-size control flow сохранены. std::wstring/std::vector вместо UString/CRecordVector; QComboBox вместо Win32 combo. | Активных функций не пропущено; container replacement требует миграции или обоснования. Dependencies: StringToInt и MyTypes (vendored), MyCharLower_Ascii заменён локальным ASCII expression. Это полный functional scope данного `.cpp`, не завершение volume workflow. |
| Agent/Agent.cpp — P1-10, P0-04, P1-04, P2-06 | Только CAgentFolder::{LoadFolder,LoadItems,GetNumberOfItems} ветви CProxyArc; GetItem и constructor — glue real-index mapping. std::vector вместо upstream vector. | _proxy2/alt-stream traversal и archive-open guard; весь CAgent lifecycle/codecs/open/reopen/close/read/extract/update capability; CAgentFolder name/prefix/size/property/raw/sort/bind/stream/real-indices/extract и property enumeration. Dependencies: MyCom, PropVariant, LoadCodecs/OpenArchive/UpdateAction, IFolder/IFolderArchive, AgentProxy. COM transport заменяется backend, bodies от этого не становятся N/A. |
| Agent/AgentProxy.cpp — P1-10, P2-06, P2-10 | CProxyArc::FindSubDir overloads, MergeSubDirs/AddDir, AddRealIndices/GetRealIndex, GetDirPath_as_Prefix* и CalculateSizes traversal; Load tree/long-path skeleton. QString/std::vector/std::deque и ArchiveEntry заменяют owned names/vectors/IInArchive. FindDir — local navigation helper. | CalculateSizes CRC/definedness; Load raw/default-path/errors/progress/memory limit, AllocStringAndCopy/FreeFiles, GetDirPathParts_isChanged, GetRealIndices_Unsorted; все CProxyArc2/CProxyDir2/SetDirPrefix_as_LONG_PATH. Dependencies: OpenArchive/CArc, IArchive/raw props, ItemName, PropVariantConv, Wildcard::CompareFileNames. Qt case comparison заменяет upstream comparator и требует review; container substitutions — debt. |
| CPP/Common/Wildcard.cpp — P0-07, P0-12, P2-07 | EnhancedMaskTest/DoesWildcardMatchName pointer body и SplitPathToParts_Smart; char16_t/Qt wrapper, QChar upper, OS separator adapter. g_CaseSensitive принудительно false для Windows-style selection, в upstream глобальная platform policy. | IsPath1PrefixedByPath2, CompareFileNames/path comparators, SplitPathToParts, wildcard detection, NCensor::CItem/CCensorNode/CCensor (add/check/include/exclude/prefix/drive handling). Dependencies: MyString/MyVector, StringConvert, platform path macros. Маска выбора не заменяет filesystem censor; не вводить второй authoritative upstream wildcard module. |
| CPP/Windows/FileDir.cpp — P1-03, P1-02, P1-12, P2-05/07 | DeleteFileAlways и RemoveDirWithSubItems из Windows ветви: read-only, reparse guard, recursion и error aggregation skeleton. QFile/QDir services, callbacks cancel, no-follow directory-link removal; POSIX policy local. | Остальное: directory/current/full/temp path, CreateComplexDir, move/progress/hardlink, time/attribute/ownership и CTempFile/CTempDir, Win32 long-path и upstream POSIX implementations. Dependencies: FileFind/FileIO/FileName/TimeUtils, StringConvert, ErrorMsg, native platform APIs. В upstream уже есть POSIX bodies — сначала проверить reuse, не переписывать их автоматически в Qt. |

### Типы, границы и следующие prerequisites

`MyString`, `MyVector`, `MyTypes`, `StringToInt`, `IntToString` уже доступны из
`third_party/7zip`; их полный byte/provenance review относится к P0-01.2.
Использование QString в control/service boundary необходимо для Qt. Замена
внутренних upstream vectors/strings в Agent, selection и SplitUtils не доказана
как необходимость: это migration debt соответствующего owner, не новый стандарт.
`std::deque` сейчас сохраняет ссылки на CProxyDir при AddDir; при возврате
upstream containers надо сохранить исходную reference stability и control flow.
Property snapshots нужны на границе `7zz`, но их ограниченные поля не заменяют
generic property pipeline. `IFolder`/variant/COM зависимости надо разложить на
portable application contracts и transport adapters; Win32 controls заменить Qt.
Не копировать engine в widgets и не превращать engine-facing ветки в N/A.

Текущая файловая инвентаризация подтверждает исходные 30 mirrored `.cpp`
(27 UI + 3 supporting), отсутствующих UI — 82. FM/BrowseDialog.cpp и
FM/PanelListNotify.cpp входят в оставшиеся 14 файлов P0-01.2; их имена
уточнены в чеклисте, без изменения знаменателя. Окончательная сверка
ownership — P0-01.4. P0-01.1 фиксирует ровно 16 уникальных путей таблицы выше.

Проверки: 2026-09-28, macOS, docs-only, `git diff --check` и сверка наличия
16 путей в обеих рабочих копиях; upstream HEAD совпадает с baseline, upstream
рабочее дерево чистое. Build/CTest/layout не запускались: code/UX не менялись.
Runtime Windows/Linux и pixel parity этим аудитом не подтверждены.

## P0-01.2 — остальные 14 модулей и portable dependencies (2026-09-28)

Project baseline: `cfd02a5` после P0-01.1, чистая рабочая копия.
Для всех путей ниже exact source — [ip7z/7zip,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
локально `src/<путь>`. FM/GUI/Common здесь означают соответствующие
`CPP/7zip/UI/` каталоги. Scope omissions: все bodies кроме явно перечисленных
перенесённых portions; имена ниже уточняют обязательные оставшиеся ветви.

| Путь / owner оставшегося scope | Copied/adapted symbols | Omitted scope, dependencies и причины |
| --- | --- | --- |
| FM/BrowseDialog.cpp — P1-11 | Только `Dlg_CreateFolder`: clear → CComboDialog → Cancel/OK → raw Value. Qt HWND/dialog result и русские Lang captions. | Весь CBrowseDialog (Init/Reload/parent/path/events/sort/create/enter/FinishOnOK), CBrowseInfo::BrowseForFile, MyBrowseForFolder и CorrectFsPath/path cleanup. Dependencies: FileFind/Dir/Name, RootFolder/drives, ComboDialog, native browse wrappers/Lang, filters. Native QFileDialog сейчас заменяет presentation, но не доказывает перенос validation/navigation bodies. |
| FM/PanelListNotify.cpp — P0-06, P0-07, P0-09 | Оба `ConvertSizeToString` и INT_TO_STR_SPEC: исходный integer/grouping алгоритм, внешний результат QString вместо UString. | IsSizeProp, SetItemText (все generic/raw/metadata formatting), OnItemChanged/OnNotifyActivateItems/OnNotifyList, OnCustomDraw, Refresh_StatusBar; GetCpuTicks diagnostic branch не включён. Dependencies: Panel, PropVariantConv, FormatUtils/PropertyName, FileLink/security, list notifications. Основной panel hover и status pipeline ещё не портированы. |
| FM/ComboDialog.cpp — P1-03, P0-07, P1-14 | OnInit/OnOK функциональная последовательность; raw Value и Strings, Qt layout/focus/selected edit заменяют controls/NormalizeSize/OnSize. Constructor/accept/value — glue. | LangSetDlgItems отсутствует (P1-14); Win32 OnSize заменён layout, не отдельный algorithm debt. Sorted code в оригинале закомментирован. Dependencies: ComboDialog.h/.rc, Control/Static, LangUtils, upstream strings заменены QStringList только на dialog boundary. |
| FM/CopyDialog.cpp — P1-01, P1-14 | OnInit, OnButtonSetPath, OnOK и browse-button dispatch: history/value/info, normalize separator и reset selection. Qt signals/layout/native browse заменяют Win32 OnButtonClicked/OnSize. | Lang, UNDER_CE initialization workaround не перенесены; CE branch отдельно не доказывает неприменимость остальных callers. Local sources-summary constructor — glue, основной summary уже в App.cpp. Dependencies: CopyDialog.h/.rc, BrowseDialog::MyBrowseForFolder, FileName::NormalizeDirPathPrefix, LangUtils. Full routing/overwrite не принадлежит dialog и остаётся P1-01/P0-11. |
| FM/SplitDialog.cpp — P1-06, P1-14 | OnInit/OnButtonSetPath/OnOK: filename title, first preset, path, trim/parse/empty check; Qt signals/layout/native browse и inline error вместо Win32 message. Добавлен signed-qint64 bound. | Lang; original resize/control wrappers заменены presentation. Dependencies: SplitDialog.h/.rc, SplitUtils, BrowseDialog, FileName, CopyDialogRes, resourceGui. UString/CRecordVector result сохранён частично (VolumeSizes), QString/std conversion parser — debt SplitUtils, не оправдание новой parser реализации. |
| FM/PasswordDialog.cpp — P0-12, P1-14 | Все functional handlers ReadControls/SetTextSpec/OnInit/OnButtonClicked/OnOK, включая read-before-echo-toggle. QString и Qt controls/result вместо Win32/Lang. | Lang отсутствует; generic CDialog fallback заменён signals. Dependencies: PasswordDialog.h/.rc, LangUtils, edit/control wrappers. Read password dialog не закрывает callback retry/Ask/encrypted mutation scopes. |
| FM/ListViewDialog.cpp — P1-10, P1-14 | ListView_GetSelected, OnInit, CopyToClipboard/ShowItemInfo/DeleteItems/OnEnter/OnOK, активные keyboard/activation branches OnNotify. UString/CUIntVector сохранены; qtlistview, Qt clipboard/signals/filter/layout заменяют services/OnSize. | Lang; version-specific g_LVN_ITEMACTIVATE_Support заменён Qt click policy, Cmd keys/modifier guards и 400ms hover — local presentation. Dependencies: ListViewDialog.h/.rc, EditDialog, RegistryUtils/CFmSettings, CListView adapter. Generic/raw property provider относится к P1-10, не к этому списку. |
| FM/EditDialog.cpp — P1-10, P1-14 | OnInit Title/Text; layout вместо OnSize. Resource styles read-only/multiline/no-wrap/scroll/Enter сохранены через Qt. eventFilter — ES_WANTRETURN adapter. | Lang отсутствует. Dependencies: EditDialog.h/.rc, GuiCommon.rc, ViewSettings ToQString. Это detail dialog; внешний editor/viewer из P1-08 — иной workflow. |
| FM/RegistryUtils.cpp — P0-07, P0-08, P1-13/14, P2-07 | CFmSettings::Load/Save, 7 keys/default false и порядок; SaveOption/ReadOption services адаптированы QSettings bool/UInt32 preserving invalid default. FmSettingsPath — local config-location glue. | Save/ReadRegLang, editor/viewer/diff/version-control, Save/ReadLockMemoryEnable, Save/ReadFlatView, Save/Read_ShowDeleted, HKLM helpers и прочие registry functions. Dependencies: Registry, StringConvert, MyString, synchronization/services; persistence adapter необходим между ОС, portable fields/logic сохранять. |
| FM/SettingsPage.cpp — P0-07, P1-12, P2-07 | Части OnInit/OnApply/OnButtonClicked для ShowRealFileIcons/ShowGrid/SingleClick, init/changed flags. Qt dialog вместо property page; snapshot omitted flags сохраняется при OK. | ShowDots/FullRow/AlternativeSelection/ShowSystemMenu, large-pages capability/privilege, extraction limit spin/range/validation/save, EnableSpin, OnCommand, OnNotifyHelp и Options Apply container. Dependencies: SettingsPage.h, SettingsPage.rc/SettingsPage2.rc, RegistryUtils, ZipRegistry, MemoryLock/System/ErrorMsg/StringToInt/HelpUtils. Controls не считать готовыми только потому, что есть CFmSettings поля. |
| GUI/CompressDialog.cpp — P1-05, P1-07, P2-01/02 | Static format/method tables; GetStaticFormatIndex adapted by format name. Numeric portions SetDictionary2/SetOrder2/GetOrderMode/SetSolidBlockSize2/SetNumThreads2; Get_Lzma2_ChunkSize, GetMemoryUsage_Threads_Dict_DecompMem, Add_Size/AddMemUsage/AddMemSize/AddMemComboItem/SetMemUseCombo/Get_MemUse_Bytes. Path portions GetExtDotPos/SetArchiveName*/ArcPath_WasChanged/SetArcPathFields/GetFinalPath_Smart; FindRegistryFormat/SetEncryptionMethod/GetEncryptionMethodSpec. IsAsciiString and password block OnOK extracted as CheckPassword; history/encrypt persistence and solid conversion blocks OnOK extracted separately. | Full CCompressDialog::OnInit/OnOK/OnCommand/FormatChanged/SetMethods/SetLevel2/SetMethod2, registry selection within numeric setters, SaveOptionsInMem, GetMethodSpec*, SFX/KeepName/hash filename, volume/shared/delete/update/path modes, memory-blocking prompt, OnHelp, parameters; **all COptionsDialog** time/links/security/precision handlers. Dependencies: CArcInfoEx/LoadCodecs, ZipRegistry, MethodProps, System affinity/RAM, SplitUtils, BrowseDialog/FileName, Lang/Help, CompressDialog.rc/CompressOptionsDialog.rc. Qt preferences widgets live in compressiondialog*.cpp; pure functions moved out of original class are existing structure debt, not a model for new ports. Commented ZSTD portions in baseline are not active missing branches; external/user codecs remain mandatory capability review. |
| GUI/ExtractDialog.cpp — P0-12, P2-07, P1-14/15 | AddComboItems/AddUniqueString; parts OnInit/OnOK for modes, force flags, password visibility, 16-entry history, SplitDest, trim/normalization; UpdatePasswordControl/OnButtonSetPath and signals replacing OnButtonClicked. QString/Qt layout/native browse/settings path services. | Absolute-path choice hidden, Ask transport absent (currently maps safe no-overwrite), GetBoolsVal/CheckButton_TwoBools/GetButton_Bools and ElimDup/NtSecurity, full initial Password/ArcPath title pipeline, OnHelp; SFX/no-registry conditional workflows not ported. Dependencies: ExtractDialog.h/.rc, ExtractMode, ZipRegistry, Wildcard::SplitPathToParts_Smart, FileName/Dir, Browse/Lang/Help. `options/destination/settingsPath` — integration helpers, not upstream bodies. |
| Common/ZipRegistry.cpp — P1-07/12, P2-07/09 | ParseMemUse/CMemUse::Parse and inline GetBytes/Clear; partial NCompression::CInfo::Save/Load and NExtract::CInfo::Save/Load. Original key names, numeric absent sentinel, force flags and format iteration retained; QSettings replaces Registry/lock, Qt strings at persistence boundary. | Compression Level/global bool pairs/Options/time precision/MAC/link/security fields; Extract ElimDup/Security and Read/Save_ShowPassword/Read/Save_LimitGB; NWorkDir::CInfo::Save/Load; CContextMenuInfo::Save/Load; omitted Key_*bool pair helpers. MemLimit_Save/Load are commented in baseline, not active missing symbols. Dependencies: Registry/Synchronization, ExtractMode, MethodProps, StringToInt. Legacy history/extract fallback is local migration; QSettings bool conversion differs from strict registry reads and needs review when extending fields. |
| CPP/7zip/Common/MethodProps.cpp — P1-07, P2-01 | Only Calc_From_Val_Percents: body copied including saturation/overflow; MyTypes interface. | StringToBool, parameter/dict/log-size parsing, CProps/CCoderProps/CMethodProps and coder property conversions/memory helpers, property-name lookup; dependencies: StringToInt/MyString/MyVector, PropVariant, ICoder/PropID. Process engine owns compression; advanced application property validation still requires source review. |

### Vendored closure и UI references

На 2026-09-28 проверены **33 файла** (32 source/header + License.txt): каждый
побайтово совпал с `git show <full-baseline>:<path>`, License.txt — с
`DOC/License.txt`. README.md — локальная документация, не copied source.
Из них sevenzip_common собирает StringToInt, MyString, IntToString, MyWindows,
StringConvert, UTFConvert и Windows/System; headers/defines входят через
`third_party/7zip/CPP`. PropID/ExtractMode, CpuArch/MyBuffer и portable type
closure не следует считать отдельными завершёнными UX-функциями.
README vendor inventory обновлён до полного фактического набора; исходные notices
и license не изменены. При будущем расширении closure проверять optional/platform
includes и callers, не заменять portable bodies из-за Win32 имени каталога.

Прочитаны reference `.rc` для Combo/Copy/Split/Password/ListView/Edit,
SettingsPage2, Extract/Compress и include CompressOptionsDialog; соответствующие
Qt handlers сверены с функциональным source. Это source/resource audit,
без visual change: geometry/JSON/screenshot и runtime checks не повторялись.
Русские captions вместо Lang, native Browse, inline Split errors и scrolling Copy
summary — существующие adapters, указанные в parity. Отсутствие controls
Options/Help/absolute/security и functional callers остаётся обязательной работой.
Windows rendered reference не использован; pixel parity не подтверждена.

Checks: 2026-09-28 macOS docs-only — byte comparison всех 33 vendored файлов,
unique-path coverage 16 + 14 = 30 и `git diff --check`; build/CTest не нужны.
P0-01 parent остаётся partial до absent-module review и полной coverage сверки.

## P0-01.3a — отсутствующие FM owners и callbacks (2026-09-28)

Project baseline: `d03de90`. Для всех 12 путей ниже FM =
`CPP/7zip/UI/FileManager`; exact source — [ip7z/7zip на
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Эти `.cpp` отсутствуют в `src/`: copied scope пуст, **все активные bodies
остаются открытыми**. Проверены исходные bodies, includes, headers и callers;
таблица определяет перенос/адаптацию, а не заявляет завершённый workflow.
Названные symbols — опорные точки review; неназванные bodies не исключены.

| Путь / owner | Обязательный функциональный scope | Dependencies, решение и необходимые adapters |
| --- | --- | --- |
| FM/FM.cpp — P0-02/03/08/09, P1-14, P3-04 | Startup/command-line/start path, InitInstance/window lifecycle, ExecuteCommand, SaveWindowInfo, splitter ratio/clamp и CApp::MoveSubWindows. | App/AppState/Panel, MyLoadMenu, ViewSettings, Lang/Registry/StringUtils, Windows security/system/alloc. Перенести application flow; HWND/message loop/accelerators/DPI/window metrics заменить Qt и platform services. DLL security/privilege branches проверять по ОС отдельно; Win32 entrypoint не основание исключить startup и persistence. |
| FM/Panel.cpp — P0-02/03/04/05/08/09, P1-05/06, P2-03/06 | CPanel Create/ReleasePanel/OnCreate/OnDestroy, list/combo messages, focus, folder-type/path queries, view/flat/streams modes; AddToArchive, ExtractArchives, CThreadTest::ProcessVirt/TestArchives и GetFilePaths. | App/Panel/RootFolder/FSFolder, IFolderArchive, ArchiveName/CompressCall, ExtractCallback, PropVariant, resources. Сохранить state и operation orchestration; notifications/controls заменяются Qt, IFolder transport и engine calls — ArchiveBackend adapters. Наличие самостоятельных MainWindow actions не переносит эти bodies. |
| FM/MyLoadMenu.cpp — P0-03/09, P1-13/14/15, P2-01/03/04/09 | MyLoadMenu/translation, CFileMenu::Load, OnMenuActivating availability/check state, ExecuteFileCommand и OnMenuCommand: view/sort/select/bookmarks/panels/operations/tools/help routing. | App/Panel, RegistryUtils/LangUtils/HelpUtils, BrowseDialog2/AboutDialog, CompressCall, PropID, resource/menu IDs. Перенести исходные branch/command tables в owner, QAction/QMenu заменить HMENU services; один dispatch и availability для keys/toolbar/menu. Shell-only entries требуют platform capability, не исключения всего меню. |
| FM/OpenCallback.cpp — P0-04/10/12, P1-09, P2-02 | COpenArchiveCallback CheckBreak/SetTotal/SetCompleted, password request/cache и открытие progress; отмена и PasswordWasAsked state. | ArchiveOpenCallback/IOpenCallbackUI, PasswordDialog, ProgressDialog2; OpenCallback.h выбирает ProgressDialog при Z7_SFX. Сохранить callback state/return semantics; BSTR/window/thread boundary адаптировать через backend и GUI-thread requests. Process API не оправдывает потерю password/cancel paths. |
| FM/ProgressDialog2.cpp — P0-10, P2-10 | CProgressSync totals/status/ratio/messages/stop/pause; CProgressDialog stats/range, elapsed/remaining/speed, pause/background priority, cancel-confirm с восстановлением pause и completion race, Create/OnExternalCloseMessage/ProcessWasFinished; CProgressThreadVirt lifecycle/exception/final errors, HResultToMessage. | ProgressDialog2.h/.rc/Res, MessagesDialog/ListView, Lang/IntToString, Thread/Synchronization/ErrorMsg, taskbar COM. Copy/update/test/extract callers используют этот модуль. Скопировать sync/lifecycle/statistical bodies; Qt UI/queued completion и process control — narrow adapters. Pause/priority нуждаются в реальном backend/platform support; fake enabled controls недопустимы. |
| FM/ProgressDialog.cpp — P2-02, P3-05 | Legacy CProgressSync/CProgressDialog totals, pause/stop, timer, Create/thread-close и cancellation flow. | ProgressDialog.h/.rc, Thread/Synchronization, Lang; build evidence: SFXSetup/makefile собирает ProgressDialog.obj, ExtractCallbackSfx.h включает его; FM/FM.mak, GUI/makefile и SFXWin/makefile выбирают ProgressDialog2.obj. Решение отложено до SFX entrypoint review: сохранить применимые bodies либо доказать использование официального standalone runtime. **Не N/A** по одному отсутствию у нас. |
| FM/MessagesDialog.cpp — P0-10, P1-14 | CMessagesDialog OnInit/OnSize: multiline messages → list, numbering/columns/widths; dialog completion. | MessagesDialog.h/.rc, LangUtils, Control/ListView, resourceGui. Qt presentation adapter, общий provider сообщений/ownership из ProgressDialog2. Существующая выдача одной строки ошибки не заменяет список сообщений. |
| FM/OverwriteDialog.cpp — P0-11/12, P1-01, P1-14 | COverwriteDialog OnInit/OnDestroy/OnButtonClicked, ReduceString/SetFileInfoControl/SetItemIcon, file name/size/time/icon presentation, Yes/No/YesToAll/NoToAll/AutoRename/Cancel. | OverwriteDialog.h/.rc/Res, FileFind/PropVariantConv/ResourceString, Lang/IntToString и native icon services. Перенести решения и comparison preparation; Qt controls/icons/metrics адаптировать, полный result contract передать FS и extract callbacks. |
| FM/PanelCopy.cpp — P1-01/04/09/16, P2-03/06 | CPanelCopyThread::ProcessVirt dispatch FS copy / archive extract/test / CopyTo, ShowFinalResults, CPanel::CopyTo/CopyFrom/CopyFromNoAsk, zone stream inheritance, password/progress/messages и selection/focus refresh. | FSFolder/IFolderOperations, ExtractCallback/UpdateCallback100, HashGUI, ZipRegistry, parent CFolderLink, Thread. Сохранить orchestration и branch order; portable folder operation interface и backend/process adapter вместо COM/threads, Win32 focus/notify guards — Qt owner. Hash folder Move E_NOTIMPL и error/cancel distinction сохранить. |
| FM/PanelOperations.cpp — P1-03/04, P0-10/11 | CThreadFolderOperations ProcessVirt/DoOperation, DeleteItems internal/recycle paths, begin/end inline rename, CorrectFsPath, CreateFolder/CreateFile/RenameFile/ChangeComment, selected state restoration. | Panel/FSFolder, UpdateCallback100, ComboDialog/Dlg_CreateFolder, PropVariant/FileName/Wildcard, shell recycle API. Перенести validation/update guards/refresh bodies; Qt editing/confirmation/recycle services заменить Win32, mutations остаются provider/backend. Create refresh even on error и comment capability/error ветви обязательны. |
| FM/UpdateCallback100.cpp — P0-10/12, P1-04/05/09 | CUpdateCallback100Imp Init, totals/completed/ratio, messages/operation result/error counts, CryptoGetTextPassword2 with AskPassword/defined state. | UpdateCallback100.h, IFolderArchive/IProgress/ICrypto, ProgressDialog2, PasswordDialog/Lang. Сохранить callback semantics; HRESULT/BSTR/process progress/password prompt — adapters. Это не engine implementation и не N/A из-за 7zz integration. |
| FM/ExtractCallback.cpp — P0-10/11/12, P1-08/09, P2-06/07/10 | CExtractCallbackImp open/extract progress/results/error flags, AskOverwrite/AskWrite conflict modes и auto-rename, password, stream/operation callbacks и RequestMemoryUse; CVirtFileSystem Write/FlushToDisk memory→disk, metadata/ADS/zone restore. | ExtractCallback.h, ProgressDialog2/Overwrite/Password/Mem dialogs, ArchiveOpenCallback/OpenArchive, ExtractingFilePath/FilePathAutoRename/StreamUtils, FileFind/Dir/IO/PropVariant, ZipRegistry/PropertyName/Lang. Сохранить policy/return branches; engine stream transport через backend, filesystem/memory/GUI services узкими adapters. Different directory/file conflict, remembered memory choice, unknown error bits и encrypted-header diagnostics остаются обязательными. |

Проверки: 2026-09-28 macOS, docs-only, `git diff --check`; 12 уникальных
путей существуют в exact upstream и отсутствуют в local mirrored tree;
суммарный реестр покрывает 42 уникальных модуля (30 mirrored + 12 missing).
Runtime/build/layout не менялись и не проверялись. Platform-specific решения
и полный dependency closure остаются открытыми у owners и в P0-01.4/P3-05.

## P0-01.3b1 — отсутствующие FM Options, Language и service dialogs (2026-09-28)

Project baseline `9eced86`; exact upstream для каждой строки —
[ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
FM = `CPP/7zip/UI/FileManager`. Все 12 `.cpp` отсутствуют локально:
copied/adapted scope пуст, все активные bodies обязательны до переноса или
доказанного platform/entrypoint решения. Проверены bodies, соответствующие
dialog headers/resources (включая Edit/Folders/MenuPage2.rc) и callers.

| Путь / owner | Исходные symbols и обязательный scope | Dependencies и решение адаптации |
| --- | --- | --- |
| FM/OptionsDialog.cpp — P1-12/13/14, P2-08/09 | OptionsDialog: шесть страниц System/Menu/Folders/Edit/Settings/Language в исходном порядке; property-sheet result, reload menus/toolbars/lang items/list settings и refresh всех panels после Apply. | App, MyLoadMenu, все page headers/resources, PropertyPage/DialogSize. Перенести container/control flow; Qt tabs/Apply/Cancel presentation и settings transaction вместо Win32 property sheet. Одиночный текущий Settings dialog не покрывает контейнер. |
| FM/EditPage.cpp — P1-08/13 | CEditPage OnInit/OnApply/OnCommand/OnButtonClicked, Edit_BrowseForFile: три viewer/editor/diff команды, changed flags, SplitCmdLineSmart, executable browse. | RegistryUtils, BrowseDialog, PanelItemOpen::SplitCmdLineSmart, Help/Lang, EditPage2.rc. Сохранить initialization и save-only-changed flow; executable filter/platform command launch адаптировать для ОС, без запуска из widget parser. Закомментированное восстановление старых parameters после Browse не активный scope. |
| FM/FoldersPage.cpp — P1-12 | CFoldersPage OnInit/GetWorkMode/MyEnableControls/GetWorkDir/ModifiedEvent/OnApply и browse: system/current/specified temp path, removable-only, dirty/init state. | NWorkDir::CInfo/ZipRegistry, BrowseDialog, Help/Lang, FoldersPage2.rc. Перенести decision/persistence flow; Qt radio/path controls и portable removable-volume service. WasChanged закомментирован, не активная обязательная функция. |
| FM/LangPage.cpp — P1-14 | CLangPage OnInit: enumerate/load/sort языки, default marks, compare sorted ID lists с английским baseline, missing/extra lines; OnApply/OnCommand/ShowLangInfo, AddVectorToString limits. | Common/Lang CLang, FileFind/ResourceString, LangUtils/RegistryUtils, LangPage.rc. Перенести loops/types и choice state; Qt combo/text, file enumeration и resources adapters. Информация о неполном переводе и bad-file errors не исключаются как presentation-only. |
| FM/LangUtils.cpp — P1-14 | LangOpen/GetLangDirPrefix/LoadLangOneTime/ReloadLang/OpenDefaultLang, kLangPairs/kLangs/FindShortNames, Lang_GetShortNames_for_DefaultLang, LangString overloads, AddLangString, dialog/window caption/colon helpers. | Common/Lang, DLL module dir, Synchronization, Window/ResourceString, RegistryUtils. Portable parser/retrieval/fallback bodies сохранить; Qt control/resource ID map и OS locale adapter вместо LANGID/Win32. Registry user override/path-separator guard и English '-' sentinel сохранить. Global mutable store требует явного ownership/thread contract. |
| FM/AboutDialog.cpp — P1-15, P2-01, P3-04 | CAboutDialog OnInit/OnHelp/OnButtonClicked: codecs load errors, version/CPU/date, homepage/help. | LoadCodecs/CCodecs, MyVersion/CpuArch, Lang/Help, AboutDialog.rc/logo/copyright. Перенести initialization; Qt dialog и URL open service, отдельно показывать version проекта/engine без неверного upstream attribution. External-codecs error branch остаётся с owner capabilities. |
| FM/HelpUtils.cpp — P1-15, P3-04 | ShowHelpWindow topic routing: module-relative 7-zip.chm, HtmlHelp либо hh.exe process; UNDER_CE/MinGW branch пуст. | DLL/StringConvert, HtmlHelp или ProcessUtils/FileDir/FileName; callers из всех Options pages, Compress/Extract/Benchmark/Browse/About/menu. Qt/OS help service нужен на macOS/Linux с теми же topic destinations и shipped help resources. Пустая upstream MinGW ветка не разрешает оставить F1 без результата в продукте. |
| FM/LinkDialog.cpp — P2-05, P0-08 | CLinkDialog init/current target/type, GetSymLink reproduce/validate reparse data, OnButton_Link hard/sym-file/sym-dir/junction/WSL create/delete checks; CApp::Link selection/destination/refresh. | FileDir/Find/IO/Name, CReparseAttr/FillLinkData, App/Panel, Browse/Lang, LinkDialog.rc. Сохранить flow; Qt presentation и portable hard/symlink services, Windows-specific junction/WSL/reparse capability. Не переписывать reparse encoding в widget; не скрывать unsupported platforms как полную готовность функции. |
| FM/MemDialog.cpp — P1-07, P2-07 | CMemDialog AddInfoMessage_To_String/OnInit/EnableSpin/OnButtonClicked/OnContinue: required/limit/RAM info, allow/skip default, save next limit/remember current action, numeric validation. | StringToInt, Windows/System portable RAM API, ErrorMsg/MyMessages, Lang/ExtractRes, MemDialog.rc; caller ExtractCallback::RequestMemoryUse. Перенести policy/ranges/parse; Qt spin/edit/radio/errors, callback result transport. Commented AddAction/SkipFile не активные branches; SkipArc/Cancel обязательны. |
| FM/MenuPage.cpp — P1-13, P2-07/09 | CMenuPage OnInit/OnApply/event changed flags: two-bitness shell handlers, cascaded/icons/ElimDup/zone, kMenuItems checkbox→flags, recheck registry state после apply errors. | Explorer ContextMenuFlags/RegistryContextMenu/resources, ZipRegistry/CContextMenuInfo, DLL/FileFind/ErrorMsg, Lang/Format/Help, MenuPage2.rc. Qt page/registry service adapters; Windows handler branch separately capability-gated. ElimDup и zone влияют на extraction callers и не N/A вместе с shell DLL. |
| FM/FormatUtils.cpp — P0-09/10, P1-10/14 | NumberToString и оба MyFormatNew: upstream UInt64 conversion и replacement всех '{0}' placeholders. | IntToString/MyString, LangUtils. Portable bodies можно vendor/reuse через единственный mirrored module; QString conversion только boundary, не самостоятельный replacement algorithm. |
| FM/PropertyName.cpp — P0-06/09, P1-10/14 | GetNameOfProperty: translated 1000+propID для ID<1000, supplied name fallback, decimal ID fallback. | IntToString, LangUtils, PropertyName.h/PropID; callers PanelMenu/ListNotify/ExtractCallback/Browse. Перенести body и property IDs, Qt string boundary; hardcoded current labels не подтверждают generic pipeline. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check` и 54 unique paths
(30 mirrored + 24 missing), все 12 новых путей существуют в upstream и
отсутствуют локально. Приложение/UX не менялись; build/CTest/layout не требуются.
Windows/Linux runtime и rendered visual parity этим review не подтверждены.

## P0-01.3b2 — отсутствующие FM providers и platform support (2026-09-28)

Project baseline `ca5030a`; FM = `CPP/7zip/UI/FileManager`, для каждой строки
exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Все 12 `.cpp` отсутствуют в local mirror, copied scope пуст. Прочитаны
bodies, headers и FM.mak/build callers; все активные bodies остаются открытыми.
Пустые/comment-only единицы ниже отличаются от неперенесённой функции:
этот вывод не закрывает их functional parent packages.

| Путь / owner | Исходный scope и omissions | Dependencies и решение |
| --- | --- | --- |
| FM/RootFolder.cpp — P0-04/05, P1-11, P2-06 | CRootFolder Init/LoadItems/GetProperty/BindToFolder overloads/parent/properties/icons, AreEqualNames и special folder helpers; Windows Computer/Documents/Network/Volumes и non-Windows single root branch. | FSFolder/FSDrives/NetFolder/AltStreamsFolder, MyCom/MyString/PropVariant/PropID, FileName/DLL/ShlObj/Lang/SysIcon. Скопировать routing/name/path bodies; Qt/platform special dirs/icons и folder transport adapters. Non-Windows root branch уже существует: inspect/reuse, не invent отдельную navigation модель. |
| FM/FSDrives.cpp — P1-11, P2-01/05, P3-05 | CFSDrives LoadItems/properties/bind, CDriveInfo device name, AddExt/GetFileSize, CopyFileSpec/CopyTo physical/logical volume image reading с AskWrite/progress. Остальные mutation methods возвращают E_NOTIMPL. | FileSystem/Find/IO/Name/Dir, Alloc/Defs/PropVariant, FSFolder/Lang/SysIcon, IFolderOperationsExtractCallback; FM.mak non-CE и Z7_DEVICE_FILE. Сохранить portable orchestration, platform volume enumeration/device IO adapter. Device imaging scope явно решить в acceptance, не потерять его внутри Drives audit. Исходные unsupported mutations не превращать в активные команды. |
| FM/NetFolder.cpp — P1-11, P2-10 | CNetFolder Init/resource lookup, LoadItems enumeration с access-denied interactive connection retry, provider/local/comment properties, share→FSFolder или nested network bind/parent/icons. Name-based bind E_NOTIMPL. | Windows/Net CResourceW/CEnum/GetResourceInformation/AddConnection2/GetResourceParent, PropVariant, FSFolder/SysIcon. Сохранить resource traversal и return paths; Windows network service и portable mounted-share capability adapters. Qt filesystem list не эквивалент network containers; credentials не сохранять в app state. |
| FM/AltStreamsFolder.cpp — P2-06, P1-03, P0-11 | CAltStreamsFolder Init/LoadItems/property/sort/parent/clone/change detection, CreateFile/Rename/Delete/CopyTo, CopyStream/UpdateFile/GetFsParentPrefixSize; native stream rename fallback copy-delete. CopyFrom/CopyFromFile/CreateFolder/SetProperty E_NOTIMPL; commented stream-extraction/CopyFrom не активные bodies. | FileFind CStreamEnumerator/change notifications, FileIO NtSetInformationFile, FileDir/FileName/Wildcard, FSFolderCopy CCopyStateIO, ExtractingFilePath/IFolderArchive, SysIcon/PropVariant. Windows filesystem ADS capability отдельна от archive streams. Сохранить guards/AskWrite/totals/error semantics, platform stream/watcher/IO adapters; macOS/Linux applicability решить по capability, не blanket N/A всего P2-06. |
| FM/FilePlugins.cpp — P2-01/02/08 | CExtDatabase::FindExt/Read: CArchiveFolderManager extensions/icon lookup, split list/merge duplicate extensions. External CPluginInfo loading loop закомментирован. | Agent/ArchiveFolder manager, PluginLoader, StringUtils/SplitString, MyCom/BSTR. Перенести active extension database bodies, metadata supplied by backend/capability service; DLL loader adapter отдельно. Комментарии не подтверждают работающие внешние handlers. |
| FM/RegistryAssociations.cpp — P2-08 | NRegistryAssoc CShellExtInfo::ReadFromRegistry/IsIt7Zip, key path helpers, AddShellExtensionInfo/DeleteShellExtensionInfo, icon index parse и open command registration. | Registry/StringConvert/StringToInt/IntToString; SystemPage callers. Windows service должен сохранить key/hive/error semantics; macOS/Linux association providers отдельные platform adapters с документированной difference. При переносе не выполнять регистрацию на машине агента: тестировать isolated registry/service fixture. |
| FM/EnumFormatEtc.cpp — P1-16, P2-09 | CEnumFormatEtc deep-copy/lifetime, Next/Skip/Reset/Clone и CreateEnumFormatEtc validation. Active caller PanelDrag::EnumFormatEtc. | IEnumFORMATETC/FORMATETC/DVTARGETDEVICE, CoTaskMemAlloc/Free, MyCom2/IDecl. Windows OLE service adapter; QMimeData formats projection для portable drag. Native iterator bodies на Windows не терять, clone position/ownership semantics сохранить. Применимость portable iterator body решить в P1-16, не объявлять drag/drop N/A. |
| FM/ClassDefs.cpp — P2-01/02/09, P3-04 | GUID definition translation unit: MyInitGuid + Agent.h/MyWindowsNew.h/Explorer MyExplorerCommand.h; runtime algorithms отсутствуют. | MyWindows/COM interface declarations, FM.mak ClassDefs.obj. Build adapter для Windows interfaces; duplicate GUID definitions исключить. Решение о включении зависит от ported interfaces, не functional replacement. |
| FM/ProgramLocation.cpp — P3-04/05 | Только StdAfx include; ProgramLocation.h содержит guard, активных functions/fields нет. | FM.mak ProgramLocation.obj сохраняет пустую unit. Не создавать новый location algorithm под этим именем; module-dir behavior реально находится в DLL/Help/Lang/Plugin services. Empty unit не закрывает resource location requirement. |
| FM/RegistryPlugins.cpp — P2-02, P3-05 | Активных bodies нет: ReadPluginInfo/ReadFileFolderPluginInfoList и header types находятся в comments. FM.dsp перечисляет файл, FM.mak его не включает; похожие calls в FilePlugins/FileFolderPluginOpen тоже commented. | RegistryPlugins.h/MyString и исторические DLL/plugin declarations. Не активировать старый закомментированный loader автоматически. Обязательные external handler/codecs capabilities остаются у Agent/LoadCodecs/PluginLoader и P2-02; comment-only inventory decision не их N/A. |
| FM/StdAfx.cpp — P3-04/05 | Только include StdAfx.h: compiler warning/Windows-version commentary и Common/Common.h, активных algorithms нет. | PCH/compiler integration adapter; mirrored sources сохраняют original includes, возможно portable common header reuse. Не считать platform macros функциональным port; compare header scope при каждом dependency update. |
| FM/StringUtils.cpp — P0-02, P1-08, P2-01 | SplitStringToTwoStrings quote-state loop: removes quotes, splits first unquoted space; JoinStrings commented. | MyString и StringUtils.h (SplitString inline), FM startup argv parsing callers, FilePlugins metadata split. Portable body копировать с original types/control flow; OS argv boundary отдельно, не заменять shell parser style expression. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 66 unique paths
(30 mirrored + 36 missing). Все новые upstream paths существуют, local copies
отсутствуют. Code/UX не менялись, build/CTest/layout не запускались;
Windows/Linux и device/network/ADS runtime остаются unverified.

## P0-01.3b3 — отсутствующие FM open, drag, hash и integration workflows (2026-09-28)

Project baseline `6ce4cac`; FM = `CPP/7zip/UI/FileManager`, exact source для
каждого пути — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Все девять `.cpp` отсутствуют локально, copied scope пуст, все активные bodies
остаются открытыми. Проверены functional bodies перечисленных групп, headers,
includes/resources/callers. Это inventory review; при реализации каждого owner
потребуется полный source comparison его scope и dependency closure.

| Путь / owner | Обязательный source scope | Dependencies и adapter decision |
| --- | --- | --- |
| FM/FileFolderPluginOpen.cpp — P0-04/10/12, P1-06/09, P2-01/02 | CFfpOpen::OpenFileFolderPlugin, CThreadArchiveOpen::Process, filename split, GetFolderLevels/GetFolderError: open path/stream with forced format, password inheritance, volume callback lifetime, per-level/non-open errors. | Agent/CArchiveFolderManager, ArchiveOpenCallback, OpenCallback/ProgressDialog2, PluginLoader, PropVariant/PropertyName/Format/Lang, FileName/Thread. Сохранить lifecycle/error/format decisions; IFolderManager transport и Thread/GUI через backend adapters. Историческая external plugin preference loop commented, не evidence работающего loader. |
| FM/PanelItemOpen.cpp — P0-04/12, P1-08/09/15, P2-06/10, P3-02 | OpenAsArc* и parent links, OpenParentArchiveFolder/OnOpenItemChanged, OpenItem/EditItem/OpenItemInArchive: direct stream → bounded memory → temp disk fallback; external viewer/editor/diff, process/child tracking, changed-temp update prompt, readonly/errors, exit/temp lifetime; IsVirus_Message и extension routing. | App/Panel CFolderLink/CTempFileInfo, CFfpOpen/CopyTo/CopyFromFile/UpdateCallback100/CVirtFileSystem, FileStreams/StreamObjects, ProcessUtils/ToolHelp, FileName/PropVariant/ExtractingFilePath/Registry/Lang. Скопировать flow и security checks; Qt signals/prompts/process services и backend nested extraction/update. Windows process snapshot/handle waits адаптировать по ОС, отсутствие handle не равно завершённой работе. Commented CBufSeqOutStream_WithFile/GetTime/ReadZoneFile/DeleteOldTempFiles не активные bodies; actual temp deletion browser — BrowseDialog2. |
| FM/PanelDrag.cpp — P1-16, P1-01/09, P2-09/10 | CDataObject SetData/GetData/QueryGetData/EnumFormatEtc ownership/transfer metadata и delayed extraction; CDropSource QueryContinueDrag/GiveFeedback; CPanel::OnDrag; CDropTarget enter/over/leave/drop, PositionCursor/GetEffect/GetTargetPath/source handshake; CompressDropFiles/right-button menu, CApp drag focus state. | IDataObject/IDropSource/IDropTarget/FORMATETC/CF_HDROP/MemoryGlobal/Shell/EnumFormatEtc, App/Panel, ArchiveName/CompressCall/ExtractingFilePath/Messages/Lang. Functional command/effect/temp/wait decisions сохраняются; Qt QDrag/QMimeData и platform OLE adapters заменяют transport. Internal FS copy, archive update, external→Add, same-drive modifiers, Cancel и target/source operation ownership не сводить к URL-drop handler. |
| FM/PanelCrc.cpp — P2-03, P0-10, P2-10 | CDirEnumerator traversal, CThreadCrc ProcessVirt two-pass scan/hash, progress/error accounting, result display once, CApp CalculateCrc2/CalculateCrc filesystem versus archive stream dispatch. | FileFind/IO/Name, MyBuffer/LoadCodecs/CHashBundle/HashGUI, ProgressDialog2, App/Panel/CopyTo. Перенести traversal/orchestration/statistics; hashing engine остаётся official engine/backend, callbacks/UI/services adapters. Flat mode recursion, archive hashMethods, first-file name, errors и Cancel учитывать. |
| FM/BrowseDialog2.cpp — P1-15, P2-10, P3-02 | CBrowseEnumerator bounded recursive totals/no reparse traversal; CBrowseDialog2 Reload/sort/selection/parent/keyboard/context/properties/delete, temp-name filters, prefix restriction/confirmation, MyBrowseForTempFolder; shared StartApplication/StartApplicationDontWait. | BrowseDialog2.h/.rc, RegistrySettings/Lang/Help/Format/PropertyName/PropIDUtils/SysIcon, FileFind/Dir/Name, ProcessUtils/Shell/PropVariant. Перенести active Delete Temporary Files workflow; Qt list/layout/menu/focus, platform shell launch/process/temp services. OnCommand фильтра commented; disabled fixed temp filter не feature debt. Prefix/path/link guards должны пройти security regression; screenshot не доказывает их. |
| FM/SystemPage.cpp — P2-08, P1-12 | CModifiedExtInfo/CAssoc state, CSystemPage Init/ChangeState/OnApply/RefreshListItem/AddIcon/events/keys; current/all-users association cycle, save changed only, error revert/reload and SHChangeNotify. | SystemPage.h/.rc, FilePlugins/RegistryAssociations, Registry hives, DLL/ErrorMsg/ShlObj, Lang/Help, image/list/property-page controls. Сохранить state/selection/error algorithms; Qt page/columns/icons и per-platform association services. Executable open command must name our app через service; tests не меняют реальные associations. |
| FM/SysIconUtils.cpp — P0-05/06/09, P1-11, P2-10 | Shell_GetFileInfo* wrappers/CSIDL/image lists; CExtToIconMap GetIconIndex, sorted attribute/extension binary searches, bounded caches/compressed maps/numeric split-extension shortcut. | SysIconUtils.h, FileDir, ShlObj/COM memory/native shell icons, MyString/MyVector. Portable cache/search bodies сохранить; OS icon lookup/image ownership adapters, Qt QIcon boundary. Реальные per-file icons/settings должны учитывать slow services и big-list limits; generic icons не подтверждают завершённый scope. |
| FM/TextPairs.cpp — P0-05, P1-03/04/10 | CPairsStorage parse/save/sort/binary lookup/add/replace/delete/GetValue, quoted ID/BOM/CR handling. | TextPairs.h/MyString/MyVector, FSFolder comment storage callers. Полностью portable bodies копировать/reuse, не заменять parser Qt text split; сохранить one authoritative module. Unicode/space IDs, duplicate update и malformed lines покрыть consumers tests. |
| FM/VerCtrl.cpp — P1-08, P3-05 | CApp::VerCtrl Edit/Commit/Revert/Diff active menu commands: bounded file read/data/time compare, numbered backups, readonly/timestamp adjustments, overwrite confirm and diff launch. | App/Panel, RegistryUtils VerCtrlPath, FileFind/IO/Dir/Name/StringToInt, OverwriteDialog; MyLoadMenu active IDM_VER_* routing. Это исходный local version-control workflow, не Git integration. Перенести filesystem/process/UI adapters; user-facing scope уточнить в P1-08/P3-05, без N/A из-за отсутствия отдельной строки плана. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 75 unique audited
paths (30 mirrored + 45 missing FM), полное покрытие всех upstream FM `.cpp`.
P0-01.3b1–b3 покрывают 33 оставшихся missing FM; P0-01.3a — первые 12.
Code/UX не менялись, build/CTest/layout не запускались. Runtime Windows/Linux,
OLE drag/shell/device и rendered Windows parity не проверены.

## P0-01.3c1 — отсутствующие Agent modules (2026-09-28)

Project baseline `3e914ce`. Agent = `CPP/7zip/UI/Agent`; exact source для
всех пяти строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied scope пуст; все активные bodies остаются обязательными. Проверены
bodies и interfaces из Agent.h/IFolderArchive/UpdateCallbackAgent.h, FM callers
и AGENT_OBJS. Portable functional policy должна сохраняться при замене COM
и in-process engine transport на ArchiveBackend/официальный 7zz.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Agent/ArchiveFolder.cpp — P1-01, P0-12, P2-06/07 | CAgentFolder SetZoneIdMode/SetZoneIdFile/CopyTo: reject archive move-out, interface callback, cur/no paths/no paths alt-stream selection, Extract с Ask. | Agent/Proxy2, ArchiveExtractCallback, ExtractMode/IFolderArchive; PanelCopy caller. Сохранить path/zone/overwrite decisions, transport адаптировать в backend; filesystem extraction adapter не заменяет policy. Commented SetReplaceAltStreamCharsMode не активная функция. |
| Agent/ArchiveFolderOpen.cpp — P0-04/12, P1-06/11, P2-01/02/08 | CCodecIcons LoadIcons/FindIconIndex, CArchiveFolderManager LoadFormats/OpenFolderFile/GetExtensions/GetIconPath: lazy formats, parsed icon pairs, non-open-error folder, progress refs и bind-to-root lifetime. | LoadGlobalCodecs/CCodecs libraries, Agent/OpenArchive, StringToInt/ResourceString/DLL, IFolderManager/BSTR. Resource/DLL icons и engine-open transport адаптировать; active external codecs metadata остаётся. Commented FindFormat/GetTypes/CreateFolderFile не активные bodies, не замена актуальной capability модели. |
| Agent/AgentOut.cpp — P1-04/05/09, P2-02/06, P3-02 | CAgent SetFolder/SetFiles, EnumerateArchiveItems/2, DoOperation/2 action-set→directory/archive pairs→UpdateProduce, properties/SFX prefix, processed empty-file tracking; CommonUpdate/DeleteItems/CreateFolder/RenameItem/CommentItem/UpdateOneFile/SetProperties. | AgentProxy/2, EnumDirItems/UpdatePair/UpdateProduce/UpdateCallback/UpdateAction, Wildcard/FileName/TimeUtils/ItemNameUtils, FileStreams/CopyCoder/IFolderArchive. Functional pair/path/rename/comment policy переносить; compression/UpdateItems выполняет official engine. Backend должен выразить исходные capabilities и mutations, не исполнять engine API в widgets. Не заменять сохранение неизменных entries перебором заново написанного algorithm. |
| Agent/ArchiveFolderOut.cpp — P1-04/09, P1-02, P2-06, P0-10 | CAgentFolder GetPathParts/CommonUpdateOperation и mutation wrappers: CanUpdate/hash guards, original prefix/tail stream, temp archive, KeepMode/Close, MoveToOriginal, MoveArc callbacks/Before_ArcReopen, processed source deletion only after success, ReOpen и restore Proxy/Proxy2/alt-folder path. | WorkDir/CWorkDirTempFile, FileStreams/LimitedStreams/CopyCoder, AgentOut/UpdateCallbackAgent/IFolderArchive_Update_MoveArc, FileDir/Find. Сохранить transaction/return/reopen order, cancellation during replace и error recovery; process/temp/filesystem/GUI services adapters. CreateFile E_NOTIMPL; SetProperty только BSTR comment ZIP; не объявлять unsupported generic mutations готовыми. |
| Agent/UpdateCallbackAgent.cpp — P1-04/05/09, P0-10/12 | CUpdateCallbackAgent SetCallback, item/totals/ratio, OpenFileError S_FALSE versus ReadingFileError failure, GetStream/update/extract result/delete operation reports, lazy crypto interfaces/password. | IUpdateCallbackUI/Common UpdateCallback, IFolderArchive callback/2/crypto/compress-progress, ErrorMsg/IntToString. Сохранить result/callback distinctions и password-defined semantics, process progress/event/error adapter. WriteSfx/CheckBreak здесь upstream no-op; отмена приходит через вызываемые callbacks и не становится optional. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 80 unique paths
(30 mirrored + 50 missing), полный coverage всех Agent `.cpp` в baseline.
Runtime/source implementation не менялись; build/CTest/layout не запускались,
Windows/Linux и engine callback transport пока не проверены.

## P0-01.3c2 — отсутствующие GUI modules (2026-09-28)

Project baseline `f6e1411`. GUI = `CPP/7zip/UI/GUI`; exact source всех
восьми строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Проверены source bodies, headers, BenchmarkDialog.rc и GUI/makefile.
Copied scope пуст; функциональные bodies ниже остаются обязательными.
GUI/makefile собирает отдельный 7zG entrypoint, а не FM: его process startup
адаптируется к Qt executable, общие command/error/workflow semantics не N/A.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| GUI/BenchmarkDialog.cpp — P2-04, P0-10, P2-10 | Benchmark/ParseNumberString, CBenchmarkDialog initialization/memory limits/dictionary/thread/pass selection, Start/Restart/Stop/Cancel, result formatting/aggregation, UpdateGui, CThreadBenchmark::Process, CBenchCallback/2 и CFreqCallback. | Common Bench/COneMethodInfo/CTotalBenchRes, system RAM/CPU/affinity, locks/thread/timer/messages, Lang/Help. Сохранить pass loop, first-pass frequency, error/cancel/join/restart order, formulas и capped history; engine Bench через official 7zz adapter. Qt layout по BenchmarkDialog.rc: compress/decompress current/resulting, usage/speed/rating, totals и log; TotalMode отдельный text view. Windows resource не доказывает rendered parity. |
| GUI/ExtractGUI.cpp — P0-12, P1-01/03, P2-02/06/07 | ExtractGUI и CThreadExtracting::ProcessVirt: output-path validation/normalization, dialog options transfer, callback initialization, Extract result/stats/hash report и messageWasDisplayed. | ExtractDialog/ExtractCallback/ArchiveExtractCallback/Extract/HashGUI, FileDir/Find/Name/Thread, ProgressDialog2. Сохранить forced/dialog option precedence, E_ABORT и distinction finished/messages; filesystem, Qt modal/progress и engine process adapters. Z7_SFX conditional scope требует отдельной P2-02/P3-05 сверки. |
| GUI/HashGUI.cpp — P1-14, P0-10/12 | CHashCallbackGUI scan/open/process/error callbacks, ProcessVirt→HashCalc, AfterLastFile/AddHashBundleRes, HashCalcGUI и ShowHashResults. | HashCalc/CHashBundle, ProgressDialog2/Thread, ListViewDialog/Lang/FormatUtils. Сохранить digest/data/name/stream statistics, S_FALSE file errors и отсутствие final report при E_ABORT; backend hash engine и Qt two-column result/progress adapters. |
| GUI/UpdateCallbackGUI.cpp — P0-10/12, P1-04/05/09 | CUpdateCallbackGUI totals/ratio/scan/archive/item/extract reports, OpenFileError/ReadingFileError, FailedFiles, crypto state/password prompt, deleting-after-compress, MoveArc и WriteSfx. | Common UpdateCallback/IUpdateCallbackUI, OpenResult_GUI/ErrorMsg/FormatUtils, ProgressDialog2/PasswordDialog. Сохранить S_FALSE open-error versus S_OK read-error reporting, counters, password-defined/asked state и cancellation; process callback/event adapter. Upstream no-op hooks не отменяют требования lifecycle. |
| GUI/UpdateCallbackGUI2.cpp — P0-10, P1-04/09, P2-02 | Init/SetOperation_Base, ShowAskPasswordDialog, MoveArc_Start/Progress/Finish и wrappers. | Lang operation tables/FormatUtils, progress Sync/WaitCreating/CheckStop и PasswordDialog. Сохранить ожидание создания перед password prompt, E_ABORT, `< 1<<57` overflow branch процента, repeat-percent CheckStop и clearing status; Qt signals/sync/process adapters. |
| GUI/UpdateGUI.cpp — P0-12, P1-04/05, P2-02/06 | ParseProp_Time_BoolPair/ParseProperties, SplitOptionsToStrings/IsThereMethodOverride/ParseAndAddPropertires, SetOutProperties, g_UpdateMode_Pairs, ShowDialog, UpdateGUI/CThreadUpdating::ProcessVirt. | CompressDialog/CCompressionInfo, UpdateArchive/UpdateOptions/UpdateAction/WorkDir, LoadCodecs, command properties/FileName/ModuleDir/Registry. Сохранить action-set mapping, one-command/one-file/capability guards, dialog round trip, method override/property replacement/time/security/link/password/volume/shared/delete/SFX decisions; Qt/platform adapters и official engine transport в backend. |
| GUI/GUI.cpp — P0-04/12, P1-01/03/04/05/14, P2-04, P3-05 | Main2 command-line phases/codecs/formats/password, benchmark/extract/update/hash dispatch, archive enumeration, failed-file/error exit codes; WinMain exception boundaries. | ArchiveCommandLine/LoadCodecs/ExtractGUI/UpdateGUI/HashGUI/Benchmark, FM callbacks/Lang, Windows DLL security/common-controls/OLE. Preserve applicable command routing/options/errors with Qt startup adapter; standalone 7zG invocation/exit behavior needs explicit acceptance scope, cannot erase shared functional bodies as presentation. |
| GUI/StdAfx.cpp — P0-02 | Only include StdAfx.h; no functional body. | Header delegates FM/StdAfx.h and MSVC warning pragma. Build/PCH adapter, no functional port claimed. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 88 unique paths
(30 mirrored + 58 missing), полный Agent/GUI coverage в exact baseline.
Runtime/UI не менялись; build/CTest/layout не запускались, rendered Windows
reference и Windows/Linux runtime не проверены. Benchmark и остальные workflow
остаются открытыми в своих functional packages.

## P0-01.3d1 — Common names, paths, update pairs и temp policy (2026-09-28)

Project baseline `0c70e3b`. Common = `CPP/7zip/UI/Common`; exact source всех
десяти строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные bodies, связанные interfaces и callers в FM/Agent/GUI/Common.
Copied scope пуст. Здесь есть portable policy, которую нельзя заменить
самостоятельным Qt/std algorithm или считать перенесённой вместе с `7zz`.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveName.cpp — P1-04/14, P1-08 | CreateArchiveName: file/parent-folder base name, hash keep-name, single-dot stripping, valid filesystem name, collision suffixes для 7z/zip/tar/wim/sha256 и baseName. | MyString/MyVector, Wildcard/StringToInt, C Sort HeapSort, FileDir/Find/Name, ExtractingFilePath; Panel/PanelDrag callers. Сохранить suffix scan/sort и `_2` gap selection; narrow file-info/parent/root adapters для платформ. Commented tar special-case не активная policy. |
| Common/DefaultName.cpp — P0-04, P1-01 | GetDefaultName3/2: recognized extension/addSubExtension, fallback last dot, `~` при отсутствии extension, TrimRight. | UString, OpenArchive::DefaultName callers. Portable unchanged body candidate; не заменять QFileInfo completeBaseName. |
| Common/ExtractingFilePath.cpp — P0-12, P1-01/06/11/13, P2-07 | ReplaceIncorrectChars/Correct_AltStream_Name/IsSupportedName/CorrectUnsupportedName/Correct_PathPart/Get_Correct_FsFile_Name/Correct_FsPath/MakePathFromParts; platform trail replacement defaults, Windows reserved names, ADS postfix/RLO, empty/dot parts и absolute prefixes. | UString/UStringVector, Wildcard/FileName; ArchiveExtractCallback, Extract, FM temp/drag/items и AltStreamsFolder callers. Retain platform branches and control flow; safety validation in backend supplements this correction policy. Correct_FsPath alone не доказывает containment или symlink safety. Header's obsolete `[]` comment не actual replacement: body uses `_`. |
| Common/SetProperties.cpp — P0-12, P1-04/05, P2-06 | ParseNumberString, CPropPropetiesVector, SetProperties: empty/no-interface success, trailing +/- boolean when no value, uint32/uint64/string conversion, stable names/values lifetime. | MyCom/MyString/StringToInt/PropVariant/ISetProperties; Update/OpenArchive callers. Preserve property typing before process serialization; replace only in-process COM dispatch with backend official-engine adapter. |
| Common/SortUtils.cpp — P0-05, P1-04/05/14 | CompareStrings/SortFileNames: upstream filename comparison, sorted index vector, empty-input return. | Wildcard CompareFileNames/CUIntVector; EnumDirItems/UpdatePair. Portable source candidate; no Qt locale comparator replacement. |
| Common/TempFiles.cpp — P0-10, P1-04/05/09, P2-02/06 | CTempFiles::Clear and header destructor: reverse-order cleanup, NeedDeleteFiles controls deletion but paths always consumed. | FStringVector/FileDir::DeleteFileAlways; Update owns lifetime. Filesystem adapter only; retain success-path ownership transfer and error cleanup policy, inspect caller changes in d3. |
| Common/UpdateAction.cpp — P1-04/05/09, P0-12 | Add/Update/Fresh/Sync/Delete seven-state action tables; header CActionSet::IsEqualTo/NeedScanning. | UpdateAction NPairState/NPairAction, AgentOut/UpdateGUI/Update/command parser. Portable source candidate; preserve exact tables and scan policy when building official-engine options. |
| Common/UpdatePair.cpp — P1-04/05, P2-06/07, P0-12 | MyCompareTime, CompareArcItemsBase/CompareArcItems, ThrowError/GetUpdatePairInfoList: sorted merge, duplicate/censor collision checks, directory distinction, time/size states, ADS HostIndex. | DirItem/CDirItems/CArcItem/CFiTime/CArcTime, TimeUtils, IArchive, SortUtils/Wildcard/upstream vectors; AgentOut/Update callers. Preserve DOS/Unix saturation, precision clipping and sub-100ns comparisons. Engine boundary не повод переписать pair algorithm; timestamp/filesystem inputs use narrow adapters. |
| Common/UpdateProduce.cpp — P1-04/05/09, P0-12 | UpdateProduce and CUpdatePair2 helpers: ignore/delete notification, copy/compress collision guards, orphan ADS rule, anti-item props, IsSameTime, ReserveDown. | UpdatePair/UpdateAction/IUpdateProduceCallback, AgentOut/Update callers. Retain NewData/NewProps/UseArcProps and action flow; engine executes resulting mutations. ShowDeleteFile return is not propagated by this body: do not invent an upstream failure contract. |
| Common/WorkDir.cpp — P1-09, P2-06, P0-10/11 | GetWorkDir current/specified/system and Windows removable-only policy; CWorkDirTempFile::CreateTempFile/MoveToOriginal, release output before replace. | ZipRegistry CInfo, FileName/System/Dir, CTempFile/FileStreams; GUI UpdateGUI/Agent ArchiveFolderOut. Settings/filesystem/drive-type adapters; preserve basename/prefix normalization, original target and failure HRESULT. Non-Windows removable-only behavior requires explicit platform parity decision, not silent rewrite. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 98 unique paths
(30 mirrored + 68 missing), все десять source files существуют в baseline.
Runtime/UX не менялись; build/CTest/layout не запускались. Mandatory bodies и
platform/engine adapters остаются открытыми до functional ports и regression checks.

## P0-01.3d2a — Common open callbacks и property conversion (2026-09-28)

Project baseline `9aad76e`. Common = `CPP/7zip/UI/Common`; exact source обеих
строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Проверены bodies, interfaces и consumers. Copied scope пуст; это открытые
functional modules, включая code внутри platform/crypto/SFX conditionals.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveOpenCallback.cpp — P0-04/10/12, P1-03, P2-01/02/08 | COpenCallbackImp Init2/subarchive/properties/progress/password/GetStream; CMultiStreams linked LRU Insert/Remove/Close/PrepareToOpenNew/EnsureOpen; CInFileStreamVol Read/Seek/GetSize/destructor и AtCloseFile. | FileFind/Name/System, FileStreams/StreamUtils, MyCom/PropVariant/IArchive/IPassword, ArchiveExtractCallback::IsSafePath; OpenArchive lifetime. Preserve reopen callback precedence, PasswordWasAsked, subarchive S_FALSE, safe-volume-path/wildcard guards, bounded open descriptors and saved-position reopen, volume names/sizes/used flags. Official engine owns in-process streams through 7zz boundary; retain application callbacks/policy with narrow adapter and explicitly review engine-owned scope. IProgress SetCompleted checks break even when byte totals ignored. No blanket N/A for COM presentation. |
| Common/PropIDUtils.cpp — P0-05/12, P1-11/13, P2-07 | ConvertPosixAttribToString/ConvertWinAttribToString, ConvertPropertyToShortString2/ConvertPropertyToString2; SID/owner/ACL helpers and ConvertNtSecureToString, Windows CheckNtSecure, reparse-tag table/ConvertNtReparseToString. | CpuArch endian helpers, IntToString/StringConvert/PropVariantConv/PropID, FileIO::CReparseAttr; PanelListNotify/PanelMenu/BrowseDialog2, GUI ExtractGUI, ArchiveExtractCallback consumers. Preserve precision/ns100/time-zero handling, CRC/inode/address and attributes formatting, descriptor size/offset validation and reparse fallback hex. Copy portable conversion bodies, adapt typed backend property input and platform services; Qt text output only. Security display parses archive bytes across platforms; native ACL application remains Windows-specific. Commented Data_To_Hex/device/ACE-detail bodies are omitted upstream code, not active requirements. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 100 unique paths
(30 mirrored + 70 missing), оба exact-source paths существуют.
Runtime/UX не менялись; build/CTest/layout не запускались. Volume transport,
property/security behavior и Windows/Linux проверки остаются functional work.

## P0-01.3d2b — Common command parser (2026-09-28)

Project baseline `6d0d8a9`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный `.cpp`, options/parser header и GUI/Console consumers.
Copied scope пуст. GUI/GUI.cpp и Console/Main.cpp вызывают Parse1/Parse2;
локального CArcCmdLineParser пока нет. Standalone CLI scope нужно согласовать
с acceptance P3-05; общая archive command policy обязательна независимо от него.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveCommandLine.cpp — P0-04/12, P1-01/03/04/05/09/14, P2-01/02/06/07, P3-05 | CArcCommand group/path helpers, switch tables/ParseArchiveCommand, CNameOption/AddNameToCensor/listfile/non-switch/rename/switch wildcard helpers, Windows ParseMapWithPaths/CEventSetEnd, ParseUpdateCommandString/2/SetAddCommandOptions/SetMethodOptions, FindCharset/SetBoolPair/ParseSizeString, CArcCmdLineParser Parse1/Parse2, non-Windows module prefix helpers. | CommandLineParser/Wildcard/ListFileUtils/StringToInt/StringConvert/IntToString/MyException, EnumDirItems/UpdateAction/Update/Extract/Hash options, FileDir/Name/System/ErrorMsg/PropVariantConv, Windows mapping/events/privileges, Alloc large pages. Preserve recursive/matching/mark rules, default mask and `--` boundary, rename-pair validation, action-state restrictions/multiple archives, volume last-zero rejection, method properties, charset and overflow checks. Preserve extraction forced overwrite/path precedence, separate archive censor, absolute-mask rejection, ADS/link/security/owner/zone/time/shared flags, stdout/data-channel conflicts and update/delete/SFX/email guards. Adapt OS process/terminal/mapping/module services narrowly; settings/widgets pass typed options through ArchiveBackend, engine serialization belongs there. Global case/trail/time/large-page state needs scoped integration review without changing parser algorithms. Commented hashgen/GetUpdatePairActionType/Linux huge-page branch are inactive, not proof of omitted active feature completion. |

Parse1 содержит реальные process side effects (affinity/privileges/large pages),
а не только parsing: их нельзя без review выполнять в GUI-потоке при каждом
построении команды. Backend process adapter должен сохранять необходимые engine
options и отделять process startup services. Windows mapping transport имеет
size/marker/termination checks и event RAII; portable transport требуется review.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 101 unique paths
(30 mirrored + 71 missing), exact source path существует. Runtime/UX не менялись;
build/CTest/layout не запускались. Parser integration и platform behavior открыты.

## P0-01.3d2c1 — Common codecs/capabilities (2026-09-28)

Project baseline `6ab0a55`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный source, CArcInfoEx/CCodecs interfaces и FM/Agent/GUI/Common
consumers. Copied scope пуст. Codec implementation остаётся official engine;
это не отменяет port metadata/capability/loading policy и явный adapter audit.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/LoadCodecs.cpp — P0-04/12, P1-04/05/11/14, P2-01/02/04/06/07, P3-05 | RegisterArc, CArcInfoEx FindExtension/AddExts, ParseSignatures, typed handler/method property helpers; CCodecs LoadCodecs/LoadFormats/LoadDll/LoadDllsFromFolder/Load/CloseLibs, IsSupportedDll, registry/module lookup, format name/extension/type-chain lookup, external codec/hasher interfaces and Get_CodecsInfoUser_Vector/error formatting. | RegisterArc/RegisterCodec/ICoder/IArchive, upstream MyCom/strings/buffers/vectors/StringToInt/StringConvert/PropVariant, DLL/FileFind/FileIO/ErrorMsg/Registry, external module ABI and optional large pages. Preserve sorted format order, paired/add extensions, flags/time flags/signature offsets, old-interface fallback, update support, dotted type/`*` lookup and exe exclusion. Retain typed/default/error semantics, incompatible-library reporting, unused-library cleanup and SetCodecs(NULL) before unload. Backend supplies official-engine capabilities/metadata; Qt widgets must not independently guess capability from extensions. In-process decoder/encoder/hasher factories and DLL exports belong engine transport adaptation; preserve original source scope and document exactly which remain delegated to 7zz. External handler support remains required review, not N/A. |

Header scope включает CreateArchiveHandler/CreateInArchive/CreateOutArchive,
CArcInfoEx flag/time helpers и compare: это часть dependency closure даже если
factory выполняет engine. Commented local SplitString body не используется:
active AddExts вызывает supporting SplitString; не копировать старую реализацию
ради устранения зависимости. Windows registry fallback и non-Windows `7z.so`
baseline не означают автоматически подходящую packaged Qt lookup policy.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 102 unique paths
(30 mirrored + 72 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Capability adapters и platform loading открыты.

## P0-01.3d2c2 — Common archive opening/nesting (2026-09-28)

Project baseline `4546ae9`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный source и COpenOptions/COpenType/CArc/CArchiveLink interfaces,
проверены consumers. Copied scope пуст. Source включает функциональный parser
handler внутри UI/Common: его расположение не делает algorithm Qt presentation.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/OpenArchive.cpp — P0-04/12, P1-01/03/06/11, P2-01/02/07/08, P3-05 | NArchive::NParser CParseItem/CHandler sorted regions/unknown gaps/properties/extract/streams; Archive_GetItemBoolProp/IsItem_* и typed archive helpers; CArc GetItem_PathToParent/Path/DefaultPath/Path2/GetItem/Size/MTime; signature/order helpers, offset/extract-to-open callbacks, CArcErrorInfo/ReadBasicProps/PrepareToOpen/CheckZerosTail/OpenArchiveSpec/OpenStream2/OpenStream/OpenStreamOrFile/ReOpen; CArchiveLink KeepMode/Close/Release/Open/2/3/ReOpen; ParseComplexSize/ParseTypeParams/ParseType/ParseOpenTypes. | LoadCodecs/DefaultName/SetProperties/ArchiveOpenCallback/DirItem/IArchive, MyCom/ComTry/strings/vectors/Wildcard/UTFConvert/StringToInt/CpuArch, FileDir/FileStreams/LimitedStreams/StreamUtils/ProgressUtils/CopyCoder. Preserve typed property failures/defaults, raw UTF16/path escape conversion, tree/ADS parent handling/deleted prefixes and time precision fallback. Preserve format exclusions/forced type/extension/signature/backward/UDF order, scan limits/callback offsets, SFX and tail/error distinctions, nested main-subfile order/depth bound, used-volume tracking and Open_Finished, reverse close and single-level reopen guard. Official engine handles archive streams/decoder/parser execution through narrow backend transport; copied application path/type/lifecycle policy remains required. Widgets must not recreate signature search or access in-process archive APIs. |

OpenStream2 имеет отдельные main-extension, start-archive и sliding-buffer
signature-scan phases; parser собирает sorted regions/unknown gaps и может
вернуть parser archive вместо обычного archive. Engine-owned delegation этих
тел требует точного scope/acceptance в P3-05; отсутствие Qt parser view не N/A.
OpenArchiveSpec может test-extract для определения physical size. ReOpen
preserves global offset и ограничивает вложенность; Open2 учитывает только
использованные volumes. Source port должен сохранить эти distinctions, включая
S_FALSE versus HRESULT failure, а не одинаковое сообщение «не архив».
Commented old Set_ErrorsText/IsHashType/path reconstruction не активные bodies.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 103 unique paths
(30 mirrored + 73 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Open/reopen/parser transport и platform parity
остаются открытыми до functional implementation и regression checks.

## P0-01.3d3a — Common scan/extract orchestration (2026-09-28)

Project baseline `682128d`. Exact source обоих Common paths — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные bodies, headers и consumers; copied scope пуст. Файловые
services адаптируются отдельно от upstream censor/scan/operation policy.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/EnumDirItems.cpp — P0-05/10/12, P1-04/05/11/14/16, P2-01/06/07/10 | FindFile_KeepDots; CDirItems AddDirFileInfo/AddError/ScanProgress/GetPrefixesPath/GetPhyPath/GetLogPath/ReserveDown/AddPrefix/DeleteLastPrefix/constructor/AddSecurityItem/EnumerateOneDir/EnumerateDir/EnumerateItems2/SetLinkInfo/FillFixedReparse/FillDeviceSizes; EnumerateDirItems/_Spec/EnumerateForItem/EnumerateAltStreams/CanUseFsDirect/IsVirtualFsFolder/EnumerateItems/EnumerateDirItemsAndSort; ConvertToLongName/s and CMessagePathException constructors. | DirItem/EnumDirItems headers, Wildcard/SortUtils/upstream strings/vectors, FileDir/IO/Name/Find/SecurityUtils/UTFConvert, POSIX owner APIs and Windows reparse/ADS; AgentOut/Update/HashCalc/GUI/CompressCall2. Preserve physical/logical prefixes, include/exclude recursion/direct-name optimization, dot names, folder/file/ADS stats, scan error callback propagation and periodic progress. Retain link traversal/stream suppression, security buffer/dedup policy, Windows link target reduction, POSIX device-size/owner mapping, long-name censor merge and duplicate sorted full-path errors. Narrow platform metadata/enumeration adapter, worker execution for expensive scans; no independent QDirIterator policy rewrite. |
| Common/Extract.cpp — P0-12, P1-01/03/14, P2-01/02/07/08/10 | SetErrorMessage/DecompressArchive/Find_FileName_InSortedVector/Extract: archive sizes and batch loop, Open_Strict/OpenResult, hash-handler guard, volume skip/total correction, output-dir substitution/PE basename/ElimDup, indices/censor/ADS filtering, hard-link prepare, extraction callback close/result precedence and statistics. | Extract options/IExtractCallbackUI, OpenArchive/ArchiveExtractCallback/ExtractingFilePath/HashCalc/LoadCodecs, Wildcard/FileFind/Dir/Name/PropVariantConv; GUI ExtractGUI/CompressCall2. Preserve E_ABORT early exit versus continue-on-open-error, no-files success callback, test+CRC mode, reset progress after Open, close failure only replaces S_OK, multivolume duplicate avoidance and Win zone inheritance. ArchiveBackend official-engine process replaces Extract/Open stream dispatch; copied orchestration/path/callback policy stays. Output containment validation supplements this code and belongs backend. |

Extract uses `CompareFileNames` both for sorting and binary volume-path search;
its documented past case-sensitivity fix is relevant regression evidence.
EnumDirItems distinguishes SymLinks mode from following links, and its recursion
decisions depend on explicit censor subnodes. Preserve these cases before any
platform service adaptation; identical root/folder display alone proves nothing.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 105 unique paths
(30 mirrored + 75 missing), оба exact-source files существуют. Runtime/UI не
менялись; build/CTest/layout не запускались. Scan/extract adapters и platform
behavior остаются открытыми до functional ports и regression checks.

## P0-01.3d3b — Common extraction callback (2026-09-28)

Project baseline `9289748`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитан полный source, проверены callback header/lifecycle helpers и связи с
Extract/ArchiveOpenCallback/Agent. Copied scope пуст. Это обязательная functional
policy, включая работу с filesystem; engine transport требует narrow adaptation.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveExtractCallback.cpp — P0-10/12, P1-01/03/11/13/14, P2-01/02/07/10 | COutStreamWithHash, privileges/office-zone helpers, CHardLinkNode/PrepareHardLinks; CArchiveExtractCallback Init/progress/CreateComplexDirectory/time/error/property helpers; CLinkLevelsInfo/IsSafePath/CensorNode_CheckPath/2; ReadLink/owner/Read_fi_Props/CorrectPathParts/CreateFolders/CheckExistFile/GetExtractStream/GetItem/GetStream/PrepareOperation/CloseFile; link filesystem guards/CreateHardLink2/SetLink/SetLink2/DeleteLinkFileAlways_or_RemoveEmptyDir, CLinkInfo parsing/normalization; CloseReparseAndFile/attributes/security/SetOperationResult/ReportExtractResult/password/hash disk-stream/RequestMemoryUse; CDirPathSortPair/CFiTimesCAM/SetPostLinks/SetDirsTimes/CloseArc. | OpenArchive/ExtractingFilePath/PropIDUtils/HashCalc/FilePathAutoRename/ItemNameUtils, MyCom/ComTry/upstream strings/vectors/UTFConvert/Wildcard, Alloc/CpuArch, FileDir/Find/Name/IO/Time/PropVariantConv/SecurityUtils/streams. Preserve metadata types/defaults, inode+stream-id hard-link grouping, overwrite answers and mode changes, incoming/existing auto-rename and renamed-parent ADS mapping, path-prefix removal modes, split handling, link-data limit, preallocation/truncation and close HRESULT precedence. Preserve deferred links/empty placeholders, dangerous-level/path-through-link guards, archive-root-relative link normalization, Windows/WSL/POSIX translation, zone-before-timestamps, owner/ACL/attribute ordering, hash/stat reporting and memory limit callback flags. Backend engine/filesystem/event adapters own execution; Qt dialogs supply decisions, never duplicate algorithms. |

CloseArc выполняет CloseReparseAndFile → SetPostLinks → SetDirsTimes, сохраняя
первую ошибку и clearing arc; closer header отвечает за завершение даже при
early return. CheckExistFile отличает skip/failed deletion return S_OK от
cancel E_ABORT/rename E_FAIL; эти distinctions нельзя потерять в общем статусе.
Link safety — не только lexical sanitization: SetLink2 проверяет виртуальный
target и существующие filesystem links по dangerous-level policy. Обязательный
backend containment слой нужно сверить с ней, а не заново написать вместо тела.
Active GetStream отвергает split output как unsupported, хотя GetExtractStream
содержит split branches; scope обоих фиксируется без заявления реализации.
Commented MyCopyFile/copy-link и delayed-symlink legacy не активные requirements.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 106 unique paths
(30 mirrored + 76 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Callback/process integration и filesystem
success/failure/cancel/Unicode/malicious-path regression остаются обязательными.

## P0-01.3d3c — Common update transaction (2026-09-28)

Project baseline `1a5f850`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный source и Update.h options/callback interfaces; consumers —
GUI/UpdateGUI и Common/CompressCall2. Copied scope пуст; compression transport
остаётся official engine, transaction/command policy требует source port.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/Update.cpp — P0-10/12, P1-04/05/09, P2-01/02/06/07/10, P3-01/05 | CUpdateErrorInfo helpers, CMultiOutStream_Bunch cleanup/deletion control, CArchivePath parse/final/volume/temp paths, CUpdateOptions InitFormatIndex/SetArcPath, CUpdateProduceCallbackImp, CRenamePair Prepare/CompareTwoNames/GetNewPath, Compress, Censor_AreAllAllowed/Censor_CheckPath/EnumerateInArchiveItems, move-progress proxy и UpdateArchive including Windows MAPI branches. | UpdateAction/Pair/Produce/Callback/DirItem/EnumDirItems/OpenArchive/SetProperties/TempFiles/LoadCodecs, MyCom/strings/vectors/Wildcard, FileDir/Find/Name/Time/DLL/PropVariantConv, FileStreams/LimitedStreams/MultiOutStream/CopyCoder/MAPI. Preserve smart/exact/add naming, format capability/type/readonly/tail/multivolume guards, rename first-match/ADS/anti props, action pair production/stats, scan decisions and parent metadata. Preserve unique-new temp creation, SFX module lookup/copy, existing prefix/tail preservation, latest-time tracking, UpdateItems→all-inputs-closed→flush/close→processed status order. Backend process/filesystem/platform adapters replace transport/services; widgets pass typed options and callbacks. |

UpdateArchive после success закрывает input archive, отключает deletion output
volumes/temp и только затем replaces original with temp. При move failure/cancel
после удаления original сохраняет temp; при new-archive cancel может включить
cleanup. MoveArc_Finish может вернуть delayed E_ABORT; не превращать его в
безусловный success и не удалять sources после него. DeleteAfterCompressing
ограничен одним compress-action command, использует processed status и повторную
проверку size/ctime/mtime/link before delete; folders удаляются в depth order,
только empty. Это обязательная safety policy, а не удобство Qt filesystem API.
Windows email workflow вызывает MAPI_DIALOG с attachment lifetime/current-dir
restorer; portable integration/acceptance P3-01/05 остаётся open, не N/A.
Известное source FIXME self-inclusion сравнивает paths без canonicalization:
backend safety adaptation требует отдельного regression и documented deviation.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 107 unique paths
(30 mirrored + 77 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Transaction success/failure/cancel, prefix,
volumes/SFX и delete-after regression остаются обязательными functional checks.

## P0-01.3d3d — Common update callback (2026-09-28)

Project baseline `5063a41`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные UpdateCallback.cpp/.h; consumer Update.cpp проверен в d3c.
Copied scope пуст; все активные callback paths остаются обязательными.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/UpdateCallback.cpp — P0-10/12, P1-04/05/09, P2-01/02/06/07/10, P3-05 | CArchiveUpdateCallback constructor, progress/ratio, GetUpdateItemInfo/GetRootProp/GetParent/raw props, GetProperty, UpdateProcessedItemStatus/GetStream2/GetStream, operation/extract reporting, volume streams, password callbacks, InFileStream_On_Error/On_Destroy; Windows GetRelativePath. | UpdatePair/Produce, DirItems/OpenArchive, MyCom/ComTry/strings/vectors/UTF/Wildcard, FileStreams/StreamObjects/ItemNameUtils, FileDir/Name/PropVariant/SecurityUtils/Synchronization. Preserve arc-vs-disk property precedence, renamed names/comments, anti item properties and empty file streams, root/security/reparse/owner/device metadata, hardlink identity map, per-mode stdin and preserve-atime handling. Backend owns typed operation events/streams and engine transport; Qt handles callback presentation only. |

GetStream2 checks cancellation before opening and reports open errors through UI;
retry without preserve-atime precedes final failure. StopAfterOpenError and resource
exhaustion (Windows ERROR_NO_SYSTEM_RESOURCES / POSIX EMFILE) must terminate even
when UI accepts continuing. Reparse data uses memory streams; processed status
is set for supplied file/reparse streams, not as proof of transaction success.
Update.cpp additionally gates deletion on successful completion and metadata.
Open-file index/path registration precedes callback attachment for exception
safety; destruction removes registration without throwing and accumulates latest
mtime under lock. Preserve lifetime and thread safety across engine integration.
Reporting uses archive server indices through CArc, not ArcItems indices (v26.03);
out-archive extract-result reporting returns E_FAIL. Volumes repeat the last size
and create new numbered files; password defined-state remains distinct from empty
password. Commented arc-property/report-finished interfaces and debug branches
are inactive baseline code, not missing mandatory features.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 108 unique paths
(30 mirrored + 78 missing); exact source path exists. Runtime/UI unchanged,
build/CTest/layout не запускались. Stream failure/cancel/lifetime, metadata,
password and update deletion regressions remain required during functional port.

## P0-01.3d4a — Common benchmark integration (2026-09-28)

Project baseline `153e222`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный Bench.cpp (5036 lines), Bench.h и GUI/BenchmarkDialog call sites;
Console/BenchCon delegates to Bench. Copied scope пуст; GUI source scope записан
в P0-01.3c2. Benchmark algorithms are upstream source, not a Qt replacement.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/Bench.cpp — P2-04, P2-10, P3-05 | CBaseRandomGenerator/CBenchRandomGenerator, benchmark/CRC streams, platform clocks/user time, CBenchInfoCalc/ProgressStatus/ProgressInfo, CBenchInfo rating/usage/speed, CBenchProps, CAffinityMode, CEncoderInfo Generate/Encode/Decode, MethodBench, thread flushers, GetBenchMemoryUsage, CRC/hash/frequency benches, method/hash tables, print helpers, CTotalBenchRes, TotalBench/TotalBench_Hash and Bench property/mode/iteration orchestration. | Bench.h/CreateCoder/MethodProps/StreamObjects/StreamUtils/Property; C CRC/CpuArch/RotateDefs; Common buffers/strings/Wildcard; Windows portable Thread/Synchronization/SystemInfo/FileFind/FileIO and POSIX timing. Preserve deterministic generated data/salts, CRC verification and size/finish checks, coder properties/password cache warmup, filter versus coder paths, affinity/groups and LZMA encoder/decoder thread pairing, complexity and weighted totals. Engine codec/hash factories remain official; backend owns execution and typed results, Qt owns controls/callback display. |

CBenchInfo NumIterations means threads during progress, iterations after Code;
do not multiply partial and final values identically. Preserve 16-bit usage scale,
minimum dictionary complexity, divide-by-zero guards and saturation. Ready/start
barriers exclude generation from measurements; RAII flushers start in exit mode
and join created threads on failure. Hash buffer freeing occurs outside measured
interval. Keep cancellation/error propagation and final-result flags; E_NOTIMPL
is skipped only in total method/hash suites, not blanket benchmark success.
GUI callback selects old LZMA benchmark; text callback supports hash/filter
bandwidth, wildcard totals, file input, dictionary/thread sweeps and CPU frequency
phases. Memory-driven dictionary reduction and limits are functional policy.
Current 7zz transport needs a narrow engine execution/result adapter retaining
these bodies and GUI formulas; exact callback/transport capability remains open
for P2-04, not an authorization to reproduce them from console text. Platform
clock/CPU/group/large-page services require scoped adapters, no GUI global changes.
Commented debug/alternative hash-update/CRC routines are inactive baseline paths.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 109 unique paths
(30 mirrored + 79 missing), exact source exists. No runtime/UI changes or
build/CTest/layout runs. P2-04 still requires functional and geometry checks;
Windows/macOS/Linux native benchmark parity remains unverified.

## P0-01.3d4b — Common hash calculation and checksum handler (2026-09-28)

Project baseline `8a1f3e2`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные HashCalc.cpp/.h; GUI/HashGUI callbacks/worker и Extract IHashCalc
integration установлены. Copied scope пуст, codec/hash primitives — official engine.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/HashCalc.cpp — P2-03, P2-01, P0-04/12, P1-04/05/09, P3-05 | CHashBundle methods/init/update/final, CHasherState digest sums/formatting, HashCalc scan/read/callback orchestration, HashHexToString, filename escape/tag/line helpers, NHash::CHashPair parsing, CHandler properties/Open/Close/Extract/UpdateItems/SetProperties, method inference and Codecs_AddHashArcHandler. | CreateCoder/MethodProps/DirItem/EnumDirItems/IFileExtractCallback/LoadCodecs, FileStreams/ProgressUtils/StreamObjects/StreamUtils/ItemNameUtils/IArchive, UTF/strings/DynLimBuf, C Alloc/CpuArch. Preserve sorted unique method IDs/default CRC32/wildcard expansion, digest limit, four digest groups/extra carry bytes, directory/file/ADS totals, name hashing/path separator normalization, scan policy/reparse streams/preserve-atime/share-for-write. Backend owns streams/codec transport/censor/callbacks; Qt owns progress and result presentation. |

HashHexToString reverses uppercase byte order for <=8-byte digests, preserves
lowercase order for longer digests; summed digests append extra bytes when group
has multiple members. Final hashes 16-byte type prefix, digest and two-byte path
units; preserve source algorithm rather than invent a cross-platform encoding.
HashCalc opens skip only on callback S_FALSE, updates totals for file growth,
propagates read/cancel errors and records scan errors in bundle. Its direct
checksum-file generation branch is commented; active writer is CHandler::UpdateItems.
Handler accepts GNU/BSD/tag/cksum/PGP and zero/CRLF lines, backslash escape policy,
filename/size/method inference; PGP checksum metadata is not signature verification.
CanUpdate disables opened PGP/cksum mutation; keep checks and output flags.
Extract requires external input callback, validates digest and optional stored
size, reports unsupported U/^ modes/methods separately from CRCError, and may copy
external input to an output stream. Path resolution must reuse extraction safety
boundary; checksum entries cannot authorize arbitrary filesystem reads/writes.
Update preserves old FullLine or regenerates renamed properties, inherits flags
unless explicitly set, bounds output and writes after complete generation.
GetStream S_FALSE sets needWrite=false but the baseline still emits a digest line;
do not silently change that flow during copying. Any corrective adaptation needs
its own regression/deviation record. Handler registration is application checksum
integration, not a new hash or compression format implementation. Qt adapters
must retain this scope even when 7zz provides hash primitives. Commented report
interfaces/debug/default-method alternatives are inactive, active branches open.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 110 unique paths
(30 mirrored + 80 missing), exact source exists. Runtime/UI unchanged;
build/CTest/layout not run. Known digests, sums/names/ADS, escaped Unicode,
malicious checksum paths, errors/cancel and handler lifecycle remain mandatory
functional tests; native parity across three platforms remains unverified.

## P0-01.3d4c — Common command entrypoints and inventory coverage (2026-09-28)

Project baseline `b35a90a`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны оба полных .cpp и общий CompressCall.h (CompressCall2.h отсутствует);
FM Panel/MyLoadMenu/PanelDrag и Explorer ContextMenu callers, FM/Explorer makefile.
Copied scope пуст; это альтернативные реализации одного интерфейса, не два
владельца одного алгоритма в проекте. FM/Explorer makefiles выбирают CompressCall;
Compile scope второго варианта отдельно подтверждается при его functional port.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/CompressCall.cpp — P0-03/10/12, P1-04/05/16, P2-03/04/09, P3-05 | GetQuotedString/error helpers, Call7zGui, AddLagePagesSwitch, CRandNameGenerator/CreateMap, CompressFiles, ExtractGroupCommand/ExtractArchives/TestArchives/CalcChecksum/Benchmark. | CompressCall.h, Random/strings/MyCom, Windows DLL/ErrorMsg/FileDir/FileMapping/MemoryLock/ProcessUtils/Synchronization, FM RegistryUtils. Preserve add/exact naming, format/email/show-dialog options, grouped archive/hash inputs, ElimDup/zone, hash-file versus digest command, total benchmark dispatch and process/event wait semantics. Adapt Windows mapping/event handshake and 7zG.exe module discovery to typed backend operation/lifetime transport; command syntax remains ArchiveBackend-owned. |
| Common/CompressCall2.cpp — P0-03/10/12, P1-04/05, P2-03/04, P3-05 | GetQuotedString/error helpers, ThrowException_if_Error, CREATE_CODECS/LOAD_EXTERNAL_CODECS, CompressFiles, ExtractGroupCommand/ExtractArchives/TestArchives/CalcChecksum/Benchmark. | Shared CompressCall.h; MyException/EnumDirItems/LangUtils, GUI BenchmarkDialog/ExtractGUI/UpdateGUI/HashGUI/ExtractRes, CCodecs/hash handler/censor/update/extract options. Preserve InitFormatIndex/SetArcPath validation, no-wildcard input censor, archive enumeration/sort/full paths, MultiArcMode, callback failed-file/IsOK checks, already-displayed errors and silent E_ABORT. Replace HWND/messagebox with Qt parent/error presenter and backend execution; preserve option/control flow bodies. |

CreateMap writes initial zero plus nul-terminated UTF-16 names and adds map size/
event to include switch. Non-waiting caller still waits for either process exit
or reader event before releasing mapping; portable listfile/IPC must retain
equivalent input lifetime and filename semantics. Call7zGui returns launch/wait
success without inspecting child exit status; backend must separately report
operation failures rather than claim completed archive from process creation.
Large-page registry/risk decision is platform policy; no unconditional enable.
CompressCall2 outer !Z7_EXTERNAL_CODECS guard makes its inner external-codecs
macro branch inactive in this baseline. waitFinish is unused in direct GUI
variant; do not fabricate asynchronous completion equivalence. Both variants
route checksum-file creation through hash Update workflow; digest-only path is
HashCalcGUI/command h. Benchmark total mode sets method wildcard. Read/source
audit does not authorize sending email or installing shell integration.
Choose one authoritative adapted entrypoint per workflow; retain relevant policy
from both variants locally behind process/platform adapters. No active mandatory
command is N/A merely because Windows 7zG/IPC is unavailable on macOS/Linux.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`; 112 unique exact-source
paths = all 109 UI modules + 3 supporting modules, all 30 local paths covered,
82 missing UI modules assigned audit decisions. No runtime/UI changes or
build/CTest/layout runs. Parent P0-01 awaits .4 reconciliation; functional ports
and native three-platform checks remain open.
