# Ревизия происхождения кода 7zFM

Здесь хранится реестр происхождения кода: upstream-ревизии и исходники,
перенесённые символы, адаптации, пропущенные части и долг переноса.
Правила работы заданы в [AGENTS.md](../AGENTS.md); наблюдаемое поведение
и платформенные отличия — в [сверке с 7zFM](7zfm-parity.md).
Владельцы компонентов и интеграционные границы описаны в
[архитектуре](architecture.md). Эти документы ссылаются на реестр,
не дублируя сведения о происхождении и историю переноса.

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

## Активация общего списка и выбор при наведении (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/UI/FileManager/ListViewDialog.cpp::{OnInit,OnNotify}`: перенесена
оставшаяся ветка CFmSettings.Load/SingleClick → ONECLICKACTIVATE/TRACKSELECT,
сохранён безусловный FULLROWSELECT. Активация делегирует существующему OnEnter:
один столбец подтверждает focused path, Alt или два столбца показывают detail.
Qt itemClicked/itemDoubleClicked заменяют LVN_ITEMACTIVATE/NM_DBLCLK и глобальный
флаг версии common controls; Ctrl/Shift/Meta исключены из активации, поскольку
Qt clicked приходит и для жестов выделения. Клавиатурная активация не изменена.

`qtlistview.cpp/.h` — новая узкая presentation-адаптация TRACKSELECT, для которого
Qt не имеет прямого аналога: dwell 400ms, выбор всей строки без открытия,
остановка при leave/press/modifiers/drag, QPersistentModelIndex защищает от
удаления hovered row, повторная проверка позиции — от scroll/reorder.
Win32 default hover time не запрашивается: фиксированный Qt interval является
явным платформенным отличием. Основная панель table/icon view пока не получила
hover/track-select. Функциональные тела списка/clipboard/delete/detail сохранены.

FmSettingsPath — общий Qt/INI path adapter, перенесён из MainWindow без изменения
политики override/AppConfigLocation/toolbar.ini; такого пути нет в registry upstream.
FoldersHistory передаёт explicit SettingsPath, остальные вызывающие используют
общий adapter. Не добавлена повторная реализация config location в dialog.
Проверки list_activation покрывают single/double, modifiers, Alt/properties detail,
hover on/off, leave и удаление row; прямые consumers — list_view_dialog,
folder_history, properties, settings, fm_settings. Layout не изменён.

## Настройки файловых списков (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/UI/FileManager/RegistryUtils.cpp/.h::CFmSettings::{Load,Save}`
перенесены в исходные пути `src/`: семь полей, defaults=false, имена ключей,
порядок чтения/записи и сохранение default при некорректном значении сохранены.
QSettings INI group FM заменяет HKEY_CURRENT_USER FM; путь передаётся явно.
ReadOption принимает bool/UInt32 и проверяет !=0 как UINT32ToBool вместо registry DWORD. Остальные функции
RegistryUtils (editor/language/flat view/large pages) не перенесены.
Старые самостоятельные ключи list/* не читаются и не мигрируются.

`SettingsPage.cpp/.h::{OnInit,OnApply,OnButtonClicked}` перенесены частично:
ShowRealFileIcons/ShowGrid/SingleClick, флаги init/changed, чтение при
инициализации и запись только изменённой страницы при подтверждении.
Qt QDialog/check boxes/signals заменяют CPropertyPage, CheckButton и notifications;
OnApply сохраняет snapshot пропущенных полей вместо чтения отсутствующих controls.
Cancel ничего не сохраняет. Отдельный Apply и OptionsDialog с шестью страницами
отсутствуют. Не перенесены controls/ветки ShowDots, FullRow, AlternativeSelection,
ShowSystemMenu, large pages, extraction RAM, help и Lang. Порядок трёх controls
взят из SettingsPage2.rc; самостоятельная settingsdialog.cpp удалена,
header оставлен как alias. MainWindow адаптирует поддерживаемые флаги к Qt.
CFmSettings принадлежит sevenzip_common, CSettingsPage — sevenzip_panel.
Notice/лицензия сохранены; форматирование адаптаций — clang-format.

Проверки: fm_settings (defaults, invalid INI, round trip, cancel/confirm,
сохранение пропущенных flags, отсутствие записи без изменений, геометрия),
settings (применение и один щелчок в четырёх режимах disk/archive).
Геометрия/JSON проверяют структуру Qt; Windows rendering не проверен.

## Запрос пароля и группировка Compress после сверки диалогов (2026-09-28)

Baseline: официальный [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

`CPP/7zip/UI/FileManager/PasswordDialog.cpp/.h/.rc`: перенесены
`CPasswordDialog::{ReadControls,SetTextSpec,OnInit,OnButtonClicked,OnOK}`
в исходные относительные пути `src/CPP/7zip/UI/FileManager/PasswordDialog.*`.
Сохранён порядок чтения Password/ShowPassword, обновления password char/text,
инициализации checkbox и чтения при OK. Win32 controls, resource attachment,
Create/parent HWND и routing buttonID заменены Qt controls/layouts, OnInit/exec
и отдельным соединением clicked. UString заменён QString на границе формы.
Lang/иконки/Win32 базовые обработчики не перенесены; подписи русские, кнопки
стандартные Qt. Модуль принадлежит sevenzip_panel, не backend.

QInputDialog в `mainwindow_backendevents.cpp` заменён этим диалогом. Прежние
правила cancel/empty request и lifetime пароля сохраняются; пароль/ShowPassword
не сохраняются в INI. Отмена после переключения ShowPassword отбрасывает
весь результат формы, хотя upstream ReadControls обновляет её публичные поля.
Generated reference содержит полный commit и хеши resource/shared include/source;
password contract сопоставляет все пять controls. Это структурная проверка,
не проверка Windows rendering. LGPL notice сохранён; лицензия —
`third_party/7zip/License.txt`.

`CPP/7zip/UI/GUI/CompressDialog.rc::IDD_COMPRESS`: исправлено Qt presentation
расположение поддерживаемых controls — compression слева, encryption справа.
Ресурс размещает password labels над edit; Qt encryption form теперь делает то же.
Дополнительный source picker следует за основной формой, чтобы не вытеснять
archive/options при открытии. Qt scroll area, wrapping длинных compression rows,
отдельное destination field, группировка и отсутствие неподдерживаемых controls
остаются явно ограниченным представлением исходного ресурса. Новые функциональные
алгоритмы Compress не добавлены, provenance уже перенесённых методов не меняется.

Проверки: password_dialog, ui_parity_password, ui_archive_states,
compression_encryption, ui_geometry; последний проверяет оба столбца на двух
размерах и в 7z/ZIP. Полная сверка и оставшийся долг —
[аудит диалогов](7zfm-parity.md#аудит-диалогов-2026-09-28).

## Шифрование и пароль Add to Archive (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/GUI/CompressDialog.cpp/.h/.rc`,
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h` и `GUI/UpdateGUI.cpp::SetOutProperties`.

В `src/CPP/7zip/UI/GUI/CompressDialog.cpp/.h` перенесены
`CCompressDialog::{SetEncryptionMethod,GetEncryptionMethodSpec,FindRegistryFormat}`
и `IsAsciiString`. Qt preferences делегируют поиск тому же исходному helper.
Исходный password-блок `OnOK` выделен в `CheckPassword`, чтобы UI и backend
использовали один алгоритм. Сохранены UString, порядок проверок ZIP ASCII
(0x20–0x7F), AES >99, сравнение повторного пароля только при скрытом пароле,
7z AES-only, ZIP ZipCrypto/AES, восстановление aes-prefix из CFormatOptions,
default-index и RemoveChar('-'). Qt QStringList/index заменяют ComboBox;
FindRegistryFormat использует snapshot CInfo вместо контролов.

`createarchivedialog_encryption.cpp` — Qt presentation/glue для
`OnInit`, `UpdatePasswordControl`, соответствующих частей `FormatChanged` и
`SaveOptionsInMem`: повторное поле и label скрываются при показе, значение
сохраняется; отдельный флажок EncryptHeaders доступен только для 7z.
Исходный порядок password/reenter/show/method/headers взят из CompressDialog.rc.
QFormLayout заменяет Win32 controls. Формирование таблиц/валидация остаются в
исходном GUI-модуле. CFormatOptions.EncryptionMethod и CInfo.ShowPassword/
EncryptHeaders сохраняются через QSettings вместо Registry; пароль не хранится.

Backend повторяет перенесённый password-блок до stage. `UpdateGUI::SetOutProperties`
передаёт em только для непустого spec; адаптер 7zz делает то же через -mem,
поэтому пустой ZIP spec даёт исходный ZipCrypto вместо прежнего принудительного
AES. Header encryption передаётся существующим -mhe. Валидация доступных em и
ZIP/header combinations — интеграционная проверка двух поддерживаемых форматов.
Исходные UI error messages заменены локализованными Qt-сообщениями; возврат error
enum заменяет ShowErrorMessageHwndRes, остальной OnOK остаётся частичным портом.
Нет password initialization из существующего архива, SFX/внешних кодеков и
других частей полного CompressDialog. Форматирование — clang-format.

История длинных путей больше не увеличивает minimum width всей формы:
AdjustToMinimumContentsLengthWithIcon сохраняет редактируемую строку и полный
текст в dropdown. Это необходимая Qt-адаптация resize, обнаруженная геометрией.

## Путь архива, история и архиватор (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/GUI/CompressDialog.cpp/.h` и
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h`; адаптированные владельцы сохраняют пути в `src/`.

`CCompressDialog::{SetArchiveName,SetArchiveName2,GetFinalPath_Smart,
SetArcPathFields,ArcPath_WasChanged}` и `GetExtDotPos` перенесены для 7z/ZIP:
сохранены поиск точки после разделителя, исключение ведущей точки,
удаление последнего расширения при смене формата, Trim, fallback StartDirPrefix,
разбор выбранной истории и ветка `always`. Qt snapshot текста заменяет Win32
ComboBox; QString — тексты этой границы, UString сохранён для исходных
операций расширения; QDir/QFileInfo — адаптеры
NName::GetFullPath/GetFullPathAndSplit. Проверка NUL необходима на границе native API.
Метаданные двух доступных кодеков заменяют CArcInfoEx; оба имеют KeepName=false.
SFX, KeepName=true, OriginalFileName, hash handlers, внешние кодеки и
OnButtonSetArchive пока не перенесены.

`CCompressDialog::OnOK` содержит только исходный блок истории/ArcType: сначала
новый полный путь, затем прежние уникальные пути до 20. Чтение контролов и Save
остаются в Qt-обвязке. Локальный AddUniqueString сохраняет алгоритм одноимённого
helper из ExtractDialog.cpp без зависимости rules/backend от виджета извлечения.
Полный OnOK ещё не перенесён. `NCompression::CInfo::{Save,Load}` получили исходные
ArcPaths/ArcType, defaults и ключи ArcHistory/Archiver; QSettings INI заменяет
Registry, остальные группы сохраняются.

`createarchivedialog.cpp` остаётся presentation/integration glue с неперенесёнными
частями формы. Combo показывает полное имя с расширением; activated заменяет
CBN_SELCHANGE/PostMessage. Backend определяет формат по суффиксу, поэтому Qt
подтверждает только совпадающие с выбранным форматом .7z/.zip, существующего
родителя и незанятую цель; upstream допускает более общий путь.
Форматирование — обязательный clang-format.

## Свойства и двухколоночный список (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Пути ниже относительно `CPP/7zip/UI/FileManager/`; адаптированные файлы — в `src/`.

| Исходник и символы | Заимствование и адаптация |
| --- | --- |
| `EditDialog.cpp/.h/.rc::CEditDialog` | Перенесён весь модуль: UString Title/Text и OnInit; QPlainTextEdit/layout заменяют Win32 attachment/OnSize, Qt-локализация — LangSetDlgItems, event filter — ES_WANTRETURN. ShowItemInfo сохраняет обе ветки и Windows-only LF→CRLF при отсутствии CR. |
| `PanelMenu.cpp::{AddListAscii,AddSeparator,AddPropertyPair,CPanel::Properties}` | Перенесены перечисление свойств и сводка single/multiple selection с UStringVector и UInt64 totals. CPanelItem/CPanelProperty заменяют COM metadata снимком доступных полей, Qt — LangString и создание modal. Числа форматируют ConvertSizeToString/ConvertUInt64ToString. |
| `PanelItems.cpp::{GetItem_UInt64Prop,GetItemSize}` | Сохранены parent-index, пустое свойство, ошибка 2723400 и делегирование GetItemSize; snapshot заменяет COM/variant. |
| `ListViewDialog.cpp/.h::{OnInit,CopyToClipboard,ShowItemInfo,DeleteItems,OnEnter}` | Перенесены NumColumns/Values, усечение display copy, raw copy/detail и параллельное удаление Strings/Values. `qtlistview.h` адаптирует CListView к QTreeWidget; полный текст показывает перенесённый CEditDialog. Лимит 1024 wchar_t зависит от ширины wchar_t платформы. |

Не перенесены raw/security/flags/errors, полный ConvertPropertyToString2,
IFolderProperties/IFolderArcProps levels, non-open level, IFolderGetItemName
fast path, остальные команды PanelMenu/PanelItems, single-click/hover и
Win32 notifications. Qt-список заменяет filesystem shell properties.
`third_party/7zip/CPP/7zip/PropID.h` скопирован без изменений из того же baseline;
адаптированные файлы форматируются clang-format.

## История папок и общий список (2026-09-28)

Следующая итерация переносит `AppState.h::CFastFolders::{SetString,GetString,
Save,Read}` и `PanelFolderChange.cpp::CPanel::{SetBookmark,OpenBookmark}`
из того же полного baseline ниже. Сохранены sparse vector, заполнение пустыми
строками до индекса и отсутствие изменения state при GetString вне списка.
Qt adapter передаёт полный отображаемый prefix; OpenBookmark возвращает его
для общего navigateTo вместо Win32 BindToPathAndRefresh. Save/Read используют
`ViewSettings.cpp/.h::{SaveFastFolders,ReadFastFolders}`, ключ FolderShortcuts.
Критическая секция исключена по той же причине GUI-thread ownership.
Двухполевая структура Bookmark и самостоятельное ветвление открытия удалены.
Исчезнувшая внутренняя папка открывает ближайший существующий parent через
общую навигацию, включая уже открытый архив без повторного чтения.
MVP-настройки history/paths и favorites/* больше не читаются по указанию
пользователя; миграционного fallback нет. Остальной CAppState пока не перенесён.

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Из `CPP/7zip/UI/FileManager/AppState.h` перенесён `CFolderHistory`;
из `App.cpp` — `AddUniqueStringToHead`, `Normalize`, `AddString` с исходным
пределом 100 и сравнением `UString::IsEqualTo_NoCase`. Имена, вектор,
порядок удаления дублей и вставки сохранены. State принадлежит GUI thread,
поэтому Win32 critical section отсутствует; Save/Read принимают путь INI.
`ViewSettings.cpp/.h::{SaveFolderHistory,ReadFolderHistory}` сохраняют исходный
ключ FolderHistory; настройки прежнего Qt MVP игнорируются.

Из `ListViewDialog.cpp/.h/.rc` перенесена одноколоночная ветка:
OnInit, ListView_GetSelected, DeleteItems, CopyToClipboard, ShowItemInfo,
OnEnter, OnOK и клавиши OnNotify. QListWidget/event filter/layout/clipboard
заменяют Win32 controls/OnSize/notifications. Сохранены множественный выбор,
удаление первого выбранного до исчерпания выбора, флаг изменений, индекс
фокуса и платформенный перевод строки clipboard. Meta добавлен для macOS.
ShowItemInfo использует перенесённый CEditDialog.
NumColumns/Values и усечение значений перенесены следующей
итерацией выше; Settings single-click/hover пока пропущен.
Qt выбирает surviving current row после удаления; Win32 focus восстановить
буквально нельзя. Отдельная кнопка удаления MVP убрана по исходному rc.

`PanelFolderChange.cpp::CPanel::FoldersHistory` сохраняет ветки OK/Cancel,
обратную вставку отредактированного списка и выбор focused item. State/parent
передаются аргументами вместо _appState/GetParent, путь возвращается Qt adapter
вместо BindToPathAndRefresh. Остальные функции модуля пока не перенесены.
MainWindow хранит history отдельно от combo; combo — только presentation.

Неизменённые зависимости взяты из того же baseline в `third_party/7zip/`:
`CPP/Common/{MyString.h,MyString.cpp,IntToString.h,IntToString.cpp,
StringConvert.h,StringConvert.cpp,UTFConvert.h,UTFConvert.cpp,MyBuffer.h,
MyWindows.cpp}` и `C/CpuArch.h`. Они сохраняют upstream notices; остальные
зависимости MyVector/MyTypes/Common/7zTypes уже присутствовали.
MyWindows обеспечивает BSTR на POSIX, Windows использует oleaut32.
Преобразование QString↔UString ограничено Qt/settings boundary и учитывает
разную ширину wchar_t. Форматирование адаптированных файлов — clang-format,
vendored sources не переформатированы.

Воспроизводимый baseline новых итераций FSFolder/FileDir, Split/Combine и
Combo/Browse: официальный репозиторий
[`ip7z/7zip` на commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные пути, перенесённые symbols, пропущенные части и адаптации указаны
в разделах ниже. Для обновления каждого частичного модуля сравнивать этот
snapshot с новой ревизией, включая зависимые/пропущенные ветки; форматирование
рассматривать отдельно от functional diff. Соседний checkout остаётся read-only.

## Инструмент сверки UI (2026-09-28)

Инструмент UI-сверки описан в [docs/ui-reference.md](ui-reference.md).
Сохранённый Copy baseline извлечён из той же полной ревизии официального
`ip7z/7zip`: `CopyDialog.rc`, `CopyDialogRes.h`, `CPP/7zip/GuiCommon.rc` и
`CopyDialog.cpp::{OnInit, OnSize, OnButtonClicked, OnButtonSetPath, OnOK}`.
JSON сохраняет ресурсные данные и короткие навигационные выдержки, не
функциональную реализацию. Python-парсер/сверка — новый локальный tooling;
новых заимствованных алгоритмов или изменений Qt presentation в этой задаче нет.
Контракт покрывает шесть контролов и выбранные Copy/Move состояния; полный
source-port audit существующих модулей остаётся в соответствующих разделах.

## Общий ввод имени/маски и создание папки (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ComboDialog.cpp/.h/.rc::{CComboDialog, OnInit, OnOK,
OnSize}` и `BrowseDialog.cpp/.h::Dlg_CreateFolder`.
Адаптированные реализации сохраняют исходные пути и symbols. Прежний
самостоятельный combodialog.cpp удалён; combodialog.h — compatibility alias.

Перенесены Title/Static/Value/Strings, порядок заполнения и получение сырого
Value в OnOK. Qt-конструктор задаёт поля до OnInit; QDialog, QComboBox,
QLabel/layouts заменяют CModalDialog/Win32 controls/OnSize. Qt AddItem выбирает
первую запись, тогда как исходный AddString не меняет введённый Value:
после наполнения восстанавливается исходный edit text. accept проходит через
OnOK; getters — Qt compatibility glue. QStringList используется только на
presentation boundary; никакого нового functional parser не добавлено.

Dlg_CreateFolder скопирован с очисткой destName перед dialog, исходными
Title/Static/default Value, возвратом false при Cancel и Value при OK.
QWidget заменяет HWND, русские строки — LangString. CreateFolder теперь
использует эту функцию, как PanelOperations.cpp; создание файла и SelectSpec
переходят на исходный CComboDialog через alias. Остальная BrowseDialog,
CorrectFsPath, inline Rename и complex-name validation остаются долгом.
Notices/LGPL сохранены; clang-format обязателен. sevenzip_panel компилирует
общий dialog один раз и включает AUTOMOC для Qt meta-object.

## Split dialog и подготовка Combine (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/SplitDialog.cpp/.h/.rc`,
`PanelSplitFile.cpp::{CApp::Split, CApp::Combine, AddInfoFileName}` и
`App.cpp::AddValuePair2`. `CSplitDialog` сохранён в исходном относительном пути;
самостоятельные volumedialog.cpp/.h удалены.

Перенесены OnInit (имя в title, Path, AddVolumeItems, выбор первого размера),
OnOK (Path, Trim, ParseVolumeSizes, отказ пустого/ошибочного ряда),
OnButtonSetPath (browse, NormalizeDirPathPrefix, сброс индекса). VolumeSizes
использует исходный CRecordVector<UInt64>; временный std::vector нужен для
текущего адаптированного SplitUtils parser, Qt-worker принимает QList<qint64>.
Предел qint64 сохраняет допустимые смещения QFile. QComboBox/QDialog/layouts
заменяют CModalDialog/Win32 controls/OnSize; native QFileDialog заменяет
MyBrowseForFolder. Inline label заменяет modal error, accept вызывает OnOK.
Qt-конструктор передаёт публичные FilePath/Path перед OnInit.

Блок сводки CApp::Combine скопирован в CThreadCombine::GetInfo в том же
PanelSplitFile.cpp: сохранены AddInfoFileName, AddValuePair2, первые два имени,
условие многоточия и последний файл. Извлечение метода необходимо для
разделения Qt orchestration и исходной логики при отсутствии полной CApp.
AddValuePair2 экспортирован из существующего App.cpp, как в upstream.
CCopyDialog получил конструктор для Title/Static/Value/Info: Combine теперь
использует исходный общий диалог вместо отдельного Qt dialog.

mainwindow_volumes.cpp переносит подготовительные ветки CApp::Split:
повторную проверку источника после dialog, сравнение размера первого тома,
GetNumberOfVolumes и подтверждение >=100. Оба сценария нормализуют назначение
и вызывают CreateComplexDir (QDir::mkpath) после подтверждения. Сохранены
адаптерные ограничения на выбранные symlink, занятые имена, 10000 частей,
транзакционную публикацию и откат. Проверка пропусков и пустых частей остаётся
существующей integration adaptation; discovery loop не объявляется новым
source port. Полный CApp, взаимодействие двух панелей и Win32 progress — долг.
Форматирование — clang-format; notices/LGPL сохранены.

## Мутации файловой системы и рабочий поток удаления (2026-09-28)

Источник: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/FSFolder.cpp::{GetRelPath, GetAbsPath, CreateFolder,
CreateFile, Rename, Delete}`, `FSFolder.h::{CDirItem, CFSFolder}`,
`CPP/Windows/FileDir.cpp::{DeleteFileAlways, RemoveDirWithSubItems}`.
Адаптированные файлы сохранены по исходным путям; notices и LGPL сохранены.

Перенесены ветки mkdir → CreateComplexDir, Create_NEW, Rename с родительским
префиксом, индексный цикл Delete с SetTotal/SetCompleted и рекурсивный обход
RemoveDirWithSubItems. Сохранён исходный порядок файлов, накопление isError
в рекурсивном обходе и частичный результат удаления. Files/Folders и indices
используют исходные CObjectVector/CRecordVector. Остальные методы CFSFolder
не объявляются перенесёнными.

Необходимые изменения: QFile/QDir/QFileInfo заменяют NIO/Win32 filesystem
services; QString — платформенные пути, bool/ErrorMessage — HRESULT и last
error, callbacks — IProgress. AddPath и конструктор формируют снимок панели
Qt, включая Parent для плоского вида; это новая интеграционная обвязка.
Win32 FILE_ATTRIBUTE_READONLY заменён Qt permissions только на Windows:
на Unix удаление зависит от прав родительской папки. QFile удаляет сам symlink;
ветка HasReparsePoint сохраняет запрет обхода его цели. Qt enumeration не
имеет Next(error): проверяется читаемость папки, а rmdir выявляет оставшиеся
записи. Сохраняется существующая политика отказа для непосредственно
выбранного symlink и незамещения dangling symlink при Rename.

FileDeleteOperation и mainwindow_filedelete.cpp — Qt worker/presentation glue:
исходный прогресс i преобразуется в число i+1 для QProgressDialog, отмена
проверяется до удаления каждого объекта и в рекурсии, завершение всегда
обновляет панель, деструктор ожидает поток. Дополнительный CheckBreak нужен,
потому что исходный RemoveDirWithSubItems не принимает IProgress и иначе
большое дерево не реагировало бы на отмену. UI сохраняет прежнее ограничение
создания одним именем; CFSFolder поддерживает исходную ветку составного пути.
Корзина/SHFileOperation, alternate streams и остальные FSFolder методы — долг.
Форматирование — обязательный clang-format.

## Qt layout diagnostics

Reference inspected: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/ExtractDialog.rc` (`IDD_EXTRACT`) and
`CPP/7zip/UI/GUI/CompressDialog.rc` (`IDD_COMPRESS`).
`docs/ui-layout-contracts.md` records control grouping/order and current Qt
adaptations. `tests/layout_inspection.cpp` is new test-only Qt integration glue:
runtime QObject-tree inspection, ancestor-clipped QWidget geometry, and JSON
reports are specific to Qt, so no upstream implementation is copied for this
helper. It does not replace upstream dialog functional logic or establish
pixel-level parity. `ui_geometry` checks reachability, footer geometry, and
Extract column ordering on disposable fixtures.

Источник сравнения: `../7zip-upstream`, commit `0766b73`, прежде всего
`CPP/7zip/UI/FileManager/`. Эта таблица охватывает все производственные файлы
`src/` на 28 сентября 2026 года. «Собственный» означает, что реализация написана
в этом репозитории по поведению 7zFM, а не перенесена из исходного файла.

Цель и обязательные правила переноса заданы в `AGENTS.md`: исходная реализация
копируется с сохранением имени файла, относительного пути, символов и структуры
модуля. UI-представление адаптируется к Qt; остальная логика меняется только там,
где этого требуют платформенные или интеграционные зависимости. Это относится
и к функциональному коду внутри Win32/COM UI-модулей. Собственная реализация
с совпадающим поведением остаётся долгом переноса.

Адаптированные исходники приложения размещаются в `src/<путь-upstream>`,
неизменённые общие зависимости — в `third_party/7zip/<путь-upstream>`.
Для каждого переноса фиксируются commit, исходные пути и символы,
скопированные и адаптированные части, а также причины каждого существенного
отклонения. Одних совпадающих имён недостаточно для признания прямого переноса.
Сохраняются исходные уведомления и применимая лицензия; текст лицензии для уже
перенесённых фрагментов лежит в `third_party/7zip/License.txt` из того же checkout.
Архивные операции сейчас остаются за `ArchiveBackend` и официальным `7zz`.

| Наши файлы | Соответствующий исходник upstream | Текущее состояние |
| --- | --- | --- |
| `src/CPP/7zip/UI/FileManager/PanelSort.cpp`, `.h` | `CPP/7zip/UI/FileManager/PanelSort.cpp::CompareFileNames_ForFolderList`, `CompareItems` | Сравнение имён и порядок «папки перед файлами → свойство → базовое имя → префикс → индекс загрузки» перенесены с сохранением имён; Указательный компаратор скопирован напрямую с заменой Windows `wchar_t` на `char16_t`; перегрузка `QStringView` преобразует вход на границе. Qt item data заменяет свойства `IFolder`. Направление, начальный порядок свойств и расширение по последней точке адаптированы по `PanelSort.cpp`, `FSFolder.cpp` и `Agent.cpp` того же commit. Остальная сортировка по сырым и форматным свойствам пока не перенесена. |
| `src/CPP/Common/Wildcard.cpp`, `.h` | `CPP/Common/Wildcard.cpp::DoesWildcardMatchName`, `EnhancedMaskTest` | Указательный `EnhancedMaskTest` скопирован с заменой Windows `wchar_t` на `char16_t`; перегрузка `QStringView` преобразует вход, Qt выполняет `MyCharUpper`. Для выбора в панели применяется нечувствительное к регистру сравнение 7zFM. |
| `src/CPP/7zip/UI/FileManager/PanelSelect.cpp`, `.h`, `src/mainwindow_selection.cpp` | `CPP/7zip/UI/FileManager/PanelSelect.cpp::SelectSpec`, `SelectByType`, `SelectAll`, `InvertSelection` | Функциональная часть `CPanel` выделена в `CPanelSelection`: методы, порядок ветвей и вектор выбора сохранены. Компонент не зависит от виджетов; `mainwindow_selection.cpp` адаптирует `ComboDialog`, фокус и Qt selection model, публикуя выбор одним обновлением. Альтернативный режим выбора upstream и обработчики мыши/диапазона остаются за Qt и не перенесены. |
| `third_party/7zip/CPP/Common/StringToInt.cpp`, `.h` и их заголовки `Common*.h`, `My*.h`, `NewHandler.h`, `StdAfx.h`, `C/7zTypes.h`, `7zWindows.h`, `Compiler.h`, `Precomp.h` | Те же относительные пути upstream | Побайтовые копии без изменения алгоритмов и форматирования. Собирается `sevenzip_common`; Qt-код использует оригинальный `ConvertStringToUInt64`. |
| `src/CPP/7zip/UI/FileManager/SplitUtils.cpp`, `.h` | `CPP/7zip/UI/FileManager/SplitUtils.cpp` | Перенесены `ParseVolumeSizes`, `GetNumberOfVolumes`, список `k_Sizes` и `AddVolumeItems`. Алгоритмы сохранены; `UString` / `CRecordVector` заменены стандартными контейнерами, Win32 combo — `QComboBox`. Worker применяет последовательность размеров и повторяет последний; диапазон ограничен `qint64`. |
| `src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp`, `.h` | `CPP/7zip/UI/FileManager/PanelSplitFile.cpp::CVolSeqName`, `CThreadCombine::ProcessVirt`, `CThreadSplit::ProcessVirt` | Алгоритм нумерации и циклы Combine/Split перенесены с сохранением имён; `QString` / `QFile` / callback заменяют строки, NIO и progress sync. Проверки изменений входа и flush — платформенная адаптация. |
| `src/CPP/7zip/UI/GUI/CompressDialog.cpp`, `.h` | `CPP/7zip/UI/GUI/CompressDialog.cpp::CFormatInfo`, `g_Formats`, `g_*Methods`, `GetStaticFormatIndex`, `SetMethod2`, `SetDictionary2`, `SetOrder2`, `SetSolidBlockSize2`, `SetNumThreads2`, `GetMemoryUsage_Threads_Dict_DecompMem`, `OnOK` | Таблицы форматов, методов, уровней и флагов взяты из upstream. Qt-преобразование возвращает имена методов с исходным фильтром 7z. Один источник используется UI и backend; SFX и внешние кодеки не представлены в текущем UI. |
| `src/CPP/7zip/UI/Agent/AgentProxy.cpp`, `.h`, `Agent.cpp`, `Agent.h` | `CPP/7zip/UI/Agent/AgentProxy.cpp::CProxyArc`, `CProxyDir`, `Agent.cpp::CAgentFolder::LoadFolder`, `LoadItems`, `GetNumberOfItems` | Перенесены бинарный поиск и отложенное слияние подпапок, дерево явных/неявных папок, итеративный подсчёт размеров, раскрытие выбора в реальные индексы и плоский обход. Qt-строки и стандартные контейнеры заменяют upstream-контейнеры; данные `ArchiveBackend` — COM-свойства. CRC, сырые свойства, `CProxyArc2` и alternate streams не представлены процессным листингом. |
| `src/main.cpp`, `mainwindow.cpp`, `mainwindow.h`, `mainwindow_menus.cpp`, `mainwindow_panel.cpp` | `FM.cpp`, `App.cpp`, `App.h`, `Panel.cpp`, `Panel.h`, `PanelItems.cpp`, `resource.rc` | Qt-оболочка одной панели. Конструктор, сборка меню и сборка панели разделены; перенос правил меню и компонентов панели ещё не завершён. |
| `src/mainwindow_backendevents.cpp` | `PanelCopy.cpp`, `PanelCrc.cpp`, `PanelItemOpen.cpp`, `OpenCallback.cpp`, `ExtractCallback.cpp` | Адаптер асинхронных сигналов `ArchiveBackend`: переходы между этапами хеширования/перемещения, просмотр извлечённого файла и восстановление состояния. Он необходим для процессного backend; функциональные сценарии ещё требуют сверки с исходными callback и panel-модулями. |
| `src/mainwindow_filelist.cpp` | `PanelFolderChange.cpp`, `PanelItems.cpp`, `PanelOperations.cpp` | Собственные переходы по файловой системе, наполнение строк и создание. Нужен перенос по исходным функциональным модулям. Выбор по маске и типу вынесен в `PanelSelect.cpp`. |
| `src/mainwindow_archivelist.cpp` | `PanelItems.cpp`, `PanelSort.cpp`, `PanelMenu.cpp` | Построение дерева, раскрытие выбранных папок и плоский обход делегированы перенесённым `CProxyArc` / `CAgentFolder`. Строки таблицы адаптируют их данные к Qt. Собственные правила доступности команд ещё требуют переноса. |
| `src/mainwindow_archiveopening.cpp` | `PanelFolderChange.cpp`, `PanelItemOpen.cpp` | Собственная цепочка вложенных архивов с временными файлами и backend. Нужно сверить стек папок и открытие по содержимому. |
| `src/mainwindow_archivecommands.cpp` | `PanelCrc.cpp`, `PanelOperations.cpp`, `PanelCopy.cpp`, `PanelItemOpen.cpp` | Собственная связка действий с `ArchiveBackend`; выбор и правила команд требуют сверки. |
| `src/mainwindow_fileoperations.cpp`, `mainwindow_filedelete.cpp` | `PanelOperations.cpp`, `PanelCopy.cpp`, `FSFolder.cpp` | Qt-обвязка подтверждения, рабочего потока и команд. Создание, переименование и удаление делегированы перенесённым методам CFSFolder; остальные правила панели остаются долгом. |
| `src/CPP/7zip/UI/FileManager/PanelKey.cpp` | `PanelKey.cpp::OnKeyDown`, `Panel.cpp::CMyComboBoxEdit::OnMessage`, `App.cpp::CPanelCallbackImp::SetFocusToPath` | Qt-диспетчер клавиш списка и адреса: Shift+F4, Alt+F1/F2, Tab, закладки и делегирование команд выделения. Однопанельный режим сохраняет upstream-правило общей цели Alt+F1/F2. |
| `src/mainwindow_preferences.cpp` | `PanelMenu.cpp`, `PanelSort.cpp`, `PanelFolderChange.cpp`, `App.cpp`, `PanelKey.cpp` | Собственный код меню, сортировки, истории, закладок и событий. Это несколько upstream-модулей в одном файле. |
| `src/CPP/7zip/UI/FileManager/PanelMenu.cpp`, `PanelItems.cpp`, `src/mainwindow_dialogs.cpp`, `propertiesdialog.cpp`, `propertiesdialog.h` | `PanelMenu.cpp::Properties`, `ListViewDialog.cpp`, `PanelItems.cpp::GetItemSize/GetItem_UInt64Prop`, `UI/Agent/AgentProxy.cpp::CalculateSizes` | Перенесены цикл перечисления доступных snapshot properties, сводка выбранных объектов и двухколоночный ListViewDialog. Qt-обвязка только собирает snapshot; размеры папок — CProxyArc. Форматные/сырые свойства и остальные PanelMenu/PanelItems branches отсутствуют. |
| `src/mainwindow_volumes.cpp`, `volumeoperations.cpp/.h`, `src/CPP/7zip/UI/FileManager/SplitDialog.cpp/.h` | `PanelSplitFile.cpp`, `SplitDialog.cpp/.h/.rc`, `CopyDialog.cpp` | CSplitDialog OnInit/OnOK/browse перенесены, Combine использует общий CCopyDialog и исходную сводку. Нумерация, parser, число томов и worker-циклы перенесены ранее; транзакции, проверка путей и discovery остаются Qt-адаптацией. |
| `src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp`, `FSFolder.h` | `CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CCopyStateIO::MyCopyFile`, `CCopyState`, `CopyFolder`, `CopyFile_Ask`, `IsDestChild` | Цикл чтения/записи, накопленный прогресс, рекурсивная copy-ветка и проверка вложенного назначения перенесены с сохранением имён. `QFile`, `QDir` и callbacks заменяют платформенные операции; stage создаётся с `NewOnly`, ошибки записи проверяются `flush`, сохраняется дата. Транзакционная обвязка, move и интерактивный overwrite остаются адаптацией/долгом. |
| `src/filetable.cpp`, `filetable.h` | `PanelItems.cpp`, `PanelSort.cpp`, `PanelDrag.cpp` | Qt-адаптер таблицы и drag-out. Функция сравнения вынесена в зеркальный `PanelSort.cpp`. |
| `src/filesystemscanner.cpp`, `filesystemscanner.h` | `FSFolder.cpp::LoadSubItems`, `Panel.cpp::ChangeFlatMode` | Собственный порционный обход через Qt. Нужна сверка фильтров, ссылок и отмены. |
| `src/filetransferoperation.cpp`, `filetransferoperation.h` | `FSFolderCopy.cpp`, `PanelCopy.cpp` | Qt-worker с временными файлами и откатом. Копирование дерева/байтов и проверка вложенного назначения делегированы перенесённым `CopyFolder`, `CopyFile_Ask`, `CCopyStateIO` / `IsDestChild`. Перемещение, предварительная проверка дерева для rename и транзакционное завершение остаются адаптацией. |
| `src/fileviewerdialog.cpp`, `fileviewerdialog.h` | `PanelItemOpen.cpp`, `EditDialog.cpp`, `EditPage.cpp` | Собственный Qt-редактор и интеграция с backend; в upstream есть отдельные режимы редактора и внешнего открытия. |
| `src/createarchivedialog.cpp`, `createarchivedialog_compression.cpp`, `createarchivedialog_resources.cpp`, `createarchivedialog.h` | `CPP/7zip/UI/GUI/CompressDialog.cpp`, `CompressDialog.rc`, `FileManager/PanelCopy.cpp` | Qt-форма выбора источников и сжатия. Правила уровней (`SetLevel2`) и методы берутся из перенесённого `GUI/CompressDialog.cpp`; Ветки размера слова LZMA/LZMA2, Deflate/Deflate64 и порядка PPMd/PPMdZip перенесены в `SetOrder2`; Словари доступных методов, Auto/Non/Full solid, потоки и оценки памяти перенесены; путь/история, шифрование, пользовательский лимит памяти и профили перенесены последующими итерациями выше; выбор источников и остальные поля полного OnOK остаются долгом. |
| `src/CPP/7zip/UI/GUI/ExtractDialog.cpp`, `.h`, `src/CPP/7zip/UI/Common/ZipRegistry.*` | `CPP/7zip/UI/GUI/ExtractDialog.cpp::CExtractDialog::OnInit`, `OnOK`, `AddComboItems`, `AddUniqueString`; `ZipRegistry.*::NExtract::CInfo` | Исходные таблицы режимов, история и сохранение режимов/показа пароля перенесены. Qt заменяет presentation; `src/extractdialog.h` — compatibility alias, прежняя самостоятельная реализация удалена. SplitDest и SplitPathToParts_Smart перенесены следующей итерацией; ElimDup и NtSecurity остаются долгом. |
| `src/CPP/7zip/UI/FileManager/CopyDialog.*`, `ViewSettings.*`, `App.cpp`, `Panel.h`, `PanelListNotify.cpp`, `src/copydialog.h` | `FileManager/CopyDialog.cpp`, `.h`, `.rc`; `ViewSettings.cpp::ReadCopyHistory`, `SaveCopyHistory`, `AddUniqueStringToHeadOfList`; `App.cpp::Copy`, `CPanel::GetItemsInfoString`; `PanelListNotify.cpp::ConvertSizeToString` | Перенесены CCopyDialog, история, сводка выделения и форматирование чисел. Qt layouts заменяют OnSize, QSettings — Registry, snapshot свойств — COM; старый header содержит только alias. Остальные App/Panel/ListNotify функции пока не перенесены. |
| `src/CPP/7zip/UI/FileManager/ComboDialog.cpp/.h`, `BrowseDialog.cpp/.h`, `src/combodialog.h` | `FileManager/ComboDialog.cpp/.h/.rc`, `BrowseDialog.cpp::Dlg_CreateFolder` | Перенесены OnInit/OnOK и создание папки через общий dialog; Qt заменяет presentation, header alias сохраняет существующих потребителей. Остальная BrowseDialog не перенесена. |
| `src/CPP/7zip/UI/FileManager/SettingsPage.*`, `RegistryUtils.*`, `settingsdialog.h` | `FileManager/SettingsPage.cpp/.h`, `RegistryUtils.cpp/.h`, `SettingsPage2.rc` | Частичный source port страницы и CFmSettings, подробно выше; OptionsDialog и остальные controls остаются долгом. |
| `src/archiveformats.h` | `FileManager/FilePlugins.cpp`, `PanelFolderChange.cpp`, `PanelItemOpen.cpp` | Собственный статический список расширений для эвристики двойного щелчка и фильтра диалога. В upstream расширения приходят от установленных обработчиков; движок сам определяет формат при явном открытии. |
| `src/archivebackend.cpp`, `archivebackend.h`, `archivebackend_listing.cpp`, `archivebackend_mutations.cpp`, `archivebackend_validation.cpp` | `CPP/7zip/UI/Console/Main.cpp`, `CPP/7zip/UI/Common/OpenArchive.cpp`, `Update.cpp` и движок `CPP/7zip/Archive/` | Адаптер процесса `7zz`, парсер его вывода, проверка путей и транзакционное обновление. Не является копией движка; переносить код форматов сюда нельзя. |

Вывод ревизии: строки со статусом «собственный» обозначают долг переноса, а не
принятую конечную архитектуру. Переименование файлов само по себе его не
исправит. Каждый такой блок требует сверки исходных функций, зависимостей и
тестов, переноса переносимой логики и Qt-адаптации платформенных вызовов.

Сравнение имён `PanelSort.cpp::CompareFileNames_ForFolderList` теперь использует
указательный алгоритм напрямую из upstream `0766b73`. Единственная замена
в теле алгоритма — `wchar_t` на `char16_t`, чтобы сохранить UTF-16 Windows
на macOS/Linux. `MyCharUpper` использует Qt; отдельная перегрузка преобразует
`QStringView` в строки с завершающим нулём, включая срезы без терминатора.

Цикл объединения `CThreadCombine::ProcessVirt()` перенесён из upstream
`PanelSplitFile.cpp` (`0766b73`) в тот же относительный путь. Сохранены порядок
частей, чтение до EOF блоками 1 МиБ, проверка полной записи и прогресс после
каждого блока. `QFile` и callback заменяют NIO/HRESULT/CProgressSync. Адаптация
добавляет проверку изменения размера, `flush` и отмену до первого чтения.
`VolumeOperation` владеет проверкой путей, временным именем, публикацией
и откатом; Split делегирует чтение/запись перенесённому `CThreadSplit::ProcessVirt`.

Правила первой части и имени результата Combine используют исходный
`CVolSeqName::ParseName` и перенесённый блок `CApp::Combine` в
`CVolSeqName::GetCombinedName`. Снято собственное ограничение `.001`;
поддерживаются `part01`, `part.01`, `.001` и числовые имена.

Цикл `CThreadSplit::ProcessVirt()` перенесён из `PanelSplitFile.cpp` (`0766b73`)
в тот же модуль: блоки 1 МиБ, ограничение чтения остатком тома, повтор последнего
размера, переход к следующему файлу и прогресс через 4 МиБ либо на границе тома.
Qt-адаптер передаёт список временных путей вместо `VolBasePath`, проверяет размер
источника и сообщает ошибки `flush`. Предварительное выделение длины из
`CPreAllocOutFile` не переносится: Qt пишет временный файл последовательно.
`VolumeOperation::copyBytes` удалён; транзакционная публикация и откат остаются
в `VolumeOperation`.

Ветка LZMA/LZMA2 `CCompressDialog::SetOrder2()` перенесена из upstream
`0766b73` в `src/CPP/7zip/UI/GUI/CompressDialog.cpp`. Сохранены генерация
8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 273 и автоматическое значение
32 для уровней ниже 7, иначе 64. UI и backend используют одну функцию.
Qt-часть формы сжатия выделена в `createarchivedialog_compression.cpp`;
явный выбор сохраняется при смене уровня, автоматический не передаёт `fb`
движку. Ветки порядка Deflate/PPMd подключены последующими итерациями, описанными ниже.

Числовой ряд `CCompressDialog::SetSolidBlockSize2` перенесён из `0766b73`
в исходный `GUI/CompressDialog.cpp`: степени 2^20–2^36. Qt хранит размеры
в МиБ вместо логарифмов; UI и backend используют один источник. Сохраняется
явное значение при смене уровня, для ZIP/Store блок отключён. Подписи
показывают МиБ/ГиБ. Расчёт автоматического размера и отдельный Full Solid
из upstream пока не перенесены; Auto оставляет выбор движку.

Скопированы проверки `fi.Solid_()`/нулевого уровня и числовой цикл `i = 20..36`.
Вместо вставки значений в Win32 `m_Solid` функция возвращает данные для Qt combo
и проверки `ArchiveBackend`; преобразование логарифмов в МиБ требуется текущим
полем `CompressionOptions::solidBlockMiB`. Registry, Add_Size, расчёт Auto,
Non Solid и Full Solid из этой функции не включены в данный перенос.
Форматирование изменено только по обязательному clang-format gate.

### Итерация: порядок Deflate / Deflate64

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetOrder2`.
Скопированы обе ветки Deflate: условия Auto 32/64/128, цикл и замена
последнего значения на 258/257. Сохранены имена метода, порядок вычислений
и границы; clang-format меняет только оформление. Win32 AddOrder и выбор
Registry заменены возвратом данных и текущим выбором Qt, как в ранее
перенесённой ветке LZMA. `GetMethodID` — узкий адаптер между строковым
`CompressionOptions` и исходными ID из таблицы форматов; upstream получает
ID из item data Win32 combo. Backend использует тот же ряд для проверки и
передаёт существующий параметр `fb` движку. Ветки порядка PPMd перенесены следующей итерацией.

### Итерация: порядок PPMd / PPMdZip

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetOrder2`,
`GetOrderMode`, `CPP/7zip/UI/GUI/UpdateGUI.cpp::SetOutProperties`.
Скопированы ветки PPMd (ряд 2,3,4,5,6,7,8,10,12,14,16,20,24,28,32 и Auto
4/6/16/32) и PPMdZip (2–16, Auto level+3), включая исходные циклы,
условия и формулы. Как в остальных ветках, AddOrder/Registry заменены
выдачей данных и текущим выбором Qt. `GetOrderMode` скопирован с передачей
ID вместо чтения Win32 combo; добавлен пустой default для clang -Wswitch.
Backend адаптирует исходный выбор `di.OrderMode ? "o" : "fb"` из
SetOutProperties к существующей строке аргумента QProcess. COM property
vector не используется при процессной границе с 7zz; функция целиком пока
не перенесена. Размер памяти PPMd и его dictionary branch остаются долгом:
UI отключает словарь, движок использует свой Auto. Подпись «Размер слова»
сохранена как в ресурсе upstream. Только clang-format меняет оформление.

### Итерация: словарь LZMA / LZMA2

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2`,
`Combo_AddDict2`. Скопированы формула Auto, цикл i=30..62, пропуски малых
значений, ограничение kLzmaMaxDictSize (15<<28) и архитектурный предел
kLzmaMaxDictSize_Up. Сохранены типы UInt32/size_t, условия и порядок
вычислений. Qt получает список вместо Win32 combo; Registry и выбор
ближайшего сохранённого значения заменены сохранением доступного текущего
выбора (как в остальных опциях). Пересчёт сохранённого Registry dictionary
>=3.75 GiB не нужен: отсутствуют Registry и произвольные размеры в Qt combo.
Перенесены проверки кратности и выбор единиц из Combo_AddDict2; форматирование
строки выполняет QString, подписи локализованы в КиБ/МиБ. UI и backend
используют один исходный ряд. CompressionOptions хранит байты (quint64)
вместо МиБ, чтобы не терять 64/256 КиБ; адаптер 7zz добавляет суффикс b.
Auto=0 не передаёт словарь движку. Другие dictionary branches, RAM limits,
расчёт памяти и Registry остаются долгом. clang-format меняет оформление.

### Итерация: память PPMd в 7z

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kPPMd)
и `CPP/7zip/UI/GUI/UpdateGUI.cpp::SetOutProperties`.
Скопированы Auto=1<<(level+19), константы kPpmd_Default_4g /
kPpmd_MaxDictSize_Up, цикл i=38..62, пропуск 1.5 МиБ, ограничение размера
и break по архитектуре. Формулы, типы и порядок вычислений сохранены;
Win32 AddDict2/Registry заменены выдачей значений и текущим выбором Qt.
Ряд не достигает границы 4g на поддерживаемых 32/64-bit, поэтому размер
для подписи совпадает с реальным размером. Backend переносит выбор
OrderMode ? "mem" : "d" в существующий QProcess аргумент; COM property
vector остаётся вне процессной границы. Qt включает словарь по наличию
исходных вариантов; backend проверяет тот же список без собственного
ограничения на LZMA. RAM limits, Registry и PPMdZip пока не перенесены.
Обязательный clang-format меняет только оформление исходных циклов.

### Итерация: память PPMdZip

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kPPMdZip).
Скопированы Auto=1<<(level+19) и цикл i=20..28 с UInt32 dict=1<<i.
AddDict/Registry заменены возвратом данных и Qt selection; исходные
формулы, границы и типы сохранены. Backend уже использует GetOrderMode
и mem=<bytes>b; одинаковая подпись PPMd разрешается в kPPMdZip по таблице
ZIP. RAM limits и выбор Registry остаются долгом. Форматирование — clang-format.

### Итерация: словарь BZip2

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kBZip2).
Скопированы ветки уровня >=5 / >=3 / ниже и значения 900/500/100<<10,
цикл i=1..9 и UInt32 dict=(i*100)<<10. Условия, типы и формулы сохранены;
Win32 AddDict и сохранённый Registry default заменены Qt combo данными
и сохранением доступного текущего значения. Backend использует исходный
список и существующий d=<bytes>b. Исходные значения в KiB сохранены,
хотя BZip2 движок нормализует размер до блоков по 100000 байт.
Это не собственный набор размеров. Форматирование — clang-format.

В этой же итерации завершены dictionary branches доступных методов:
скопирована ветка kDeflate/kDeflate64 с фиксированным Auto 1<<15 / 1<<16.
AddDict2(Auto) заменён одной строкой Qt; явный словарь не доступен,
backend отклоняет его, поскольку исходный список не содержит ручных вариантов.
Ветка kCopy не подключается: метод Copy не экспонируется нашей таблицей
7z-методов, Store сохраняет отдельное отключение контролов. Остальные
неподдерживаемые форматы и закомментированный ZSTD не включены.

### Итерация: управление solid-блоками

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::Get_Lzma2_ChunkSize`, `Add_Size`,
`CCompressDialog::SetSolidBlockSize2`, solid-блок `OnOK`,
`CompressDialog.h::NCompressDialog::CInfo` и `UpdateGUI.cpp::SetOutProperties`.
Скопированы Get_Lzma2_ChunkSize целиком, выбор единиц Add_Size, расчёт
Auto solid (включая нормализацию BZip2 и пределы 16 МиБ/4 ГиБ/16 ГиБ),
ряд логарифмов 20..36, Non Solid=0 и Full Solid=64, а также преобразование
OnOK в байты с UInt64(-1) для Full. Win32 insertion/Registry заменены
данными Qt combo и текущим выбором; названия, формулы, типы, условия
и порядок вычислений сохранены. Add_Size использует QString для строки
и локализованные единицы, без собственного алгоритма размеров.
Только solid-поля CInfo скопированы: CompressionOptions наследует этот
запись-адаптер, остальные поля CInfo ещё не перенесены. Qt задаёт
SolidIsSpecified по доступности поля перед вызовом перенесённого OnOK,
как FormatChanged upstream. Backend проверяет байты через тот же ряд
и преобразование, переносит s=<bytes>b из SetOutProperties в 7zz -ms.
Прежние bool solid / solidBlockMiB и отдельный checkbox удалены:
единственный список теперь следует исходному порядку контролов.
Полный OnOK, Registry и XZ UI остаются долгом; исходная XZ ветка Auto
сохранена, но формат создания остаётся ограничен 7z/ZIP.
Форматирование — обязательный clang-format.

### Итерация: потоки и расход памяти

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetNumThreads2`,
`GetMemoryUsage_Threads_Dict_DecompMem`, `AddMemUsage`, RAM-блок `OnInit`
и проверка памяти `OnOK`; `CompressDialog.h::GetDict2`;
`CPP/7zip/Common/MethodProps.cpp::Calc_From_Val_Percents`;
`CPP/Windows/System.cpp`, `.h`.

System.cpp/.h, Windows/StdAfx.h, Common/Defs.h, Common/MyVector.h и C/Threads.h
скопированы побайтово в third_party/7zip с исходными путями и notices.
Переиспользованы CProcessAffinity и GetRamSize, включая Windows processor
groups, POSIX affinity, sysctl/sysinfo: они уже переносимы, новые Qt/sysconf
аналоги не написаны. POSIX CProcessAffinity не имеет Windows метода
Get_and_return_NumProcessThreads_and_SysThreads; нормализация num1/num2
скопирована из его тела в узкий адаптер GetSystemInfo. RAM-блок OnInit
сохранён с 32-bit пределом 7<<28, минимумом 64 МиБ и исходными 80%.
Calc_From_Val_Percents скопирован целиком в зеркальный Common/MethodProps.cpp;
COM/строковые свойства остальных функций MethodProps пока не нужны
процессной границе, поэтому адаптированный заголовок содержит только
используемый интерфейс и исходные UInt64/UInt32, без новых вычислений.

SetNumThreads2 сохраняет исходные пределы LZMA=2/LZMA2=512/BZip2=64,
Auto-only для 7z PPMd/Copy, ZIP=32/128 по архитектуре, CPU*2 предел списка,
а также циклы уменьшения Auto для ZIP и LZMA2 по памяти. Registry и
Win32 вставка заменены текущим Qt выбором и возвратом данных. Скопирована
полная активная функция оценки памяти со всеми доступными ветками и
исходными арифметикой/условиями/порядком; вместо методов виджета её
параметры содержат format, level, method, dictionary и blockSizeLog.
AddMemUsage сохраняет единицы MB/GB/TB и округление, QString заменяет
UString insertion. Пустой default устраняет enum -Wswitch без смены поведения.

Qt-представление ресурсов вынесено в createarchivedialog_resources.cpp:
GetDict2 адаптирует исходное переключение Spec/Auto к combo=0; системные
данные читаются один раз на диалог. Spinbox 0..256 заменён combo Auto/ряд,
показаны процессные/аппаратные CPU и оценки сжатия/распаковки по порядку
CompressDialog.rc. Backend получает тот же набор через порт, проверяет
явные потоки, применяет условие memUsage>limit из OnOK перед запуском
и сохраняет -mmt только для явного выбора. При неизвестной RAM отказ
не применяется (адаптация недоступного системного значения); Auto по
памяти отключён, как в исходном SetNumThreads2. Пользовательский RAM
selector, Registry, ZSTD и UI XZ остаются долгом. Форматирование — clang-format.

### Итерация: исходный разбор лимита памяти

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::NCompression::CMemUse`,
`ZipRegistry.cpp::ParseMemUse`, `CMemUse::Parse` и
`CPP/Common/MyString.h::MyCharLower_Ascii(wchar_t)`.
Структура, Clear/GetBytes, весь указательный парсер и ASCII helper
скопированы с сохранением имён, типов, ветвей и проверки переполнения.
Адаптация: Parse принимает QString и передаёт нуль-терминированную
std::wstring исходному wchar_t парсеру. Это позволяет использовать уже
скопированный ConvertStringToUInt64 на всех ОС, без нового Qt-парсера.
CMemUse использует ранее перенесённый Calc_From_Val_Percents, включая
насыщение и округление. Остальной ZipRegistry (Windows Registry,
критические секции и настройки форматов) не нужен для этого ядра и
остаётся долгом; не добавлены платформенные Registry зависимости.
Форматирование — обязательный clang-format. Подключение пользовательского
предела к диалогу и backend выполняется следующей итерацией.

### Итерация: пользовательский лимит памяти от диалога до движка

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::AddMemSize`, `AddMemComboItem`,
`SetMemUseCombo`, `Get_MemUse_Bytes`, RAM-блок `OnInit`, память в `OnOK`,
`SetNumThreads2`; `CompressDialog.h::CInfo::MemUsage`;
`UpdateGUI.cpp::SetOutProperties` (ветка memuse).
Скопированы форматирование размеров, Auto/процентные/байтовые элементы,
оба цикла выбора и вставка нестандартного текущего значения, GetBytes
и fallback Auto. Алгоритмы, типы, пределы и порядок ветвей сохранены.
Вместо UString и Win32 combo функции возвращают Qt-строки/данные;
Registry CFormatOptions заменён параметром spec, операции SetCurSel
удалены из ядра — текущий выбор принадлежит Qt. Show/Enable по MemUse_
не нужен: оба экспонируемых формата имеют этот flag. Для байтов mu.Val
эквивалентен исходному GetBytes(_ramSize_Reduced). CSystemInfo получил
исходное _ramSize_Reduced после минимального размера 64 МиБ.

SetNumThreads2 теперь вызывает перенесённый Get_MemUse_Bytes вместо
фиксированного RamUsageAuto. При смене solid-блока Qt пересчитывает
Auto-потоки; backend восстанавливает log из проверяемого CInfo, чтобы
оценка соответствовала выбранному solid. Это адаптер байтовой process
границы, не новая формула оценки памяти. OnOK сохраняет проверку
memUsage!=UInt64(-1), затем memUsage>limit; Qt заменяет MessageBoxError.
Проверка применяется лишь к созданию: диалог добавления не экспонирует
сжатие и backend сохраняет существующие параметры.

CInfo передаёт исходную CMemUse, backend повторяет проверку и переносит
ветку SetOutProperties: проценты в memuse=<value>%, байты в memuse=<value>b,
Qt QProcess добавляет префикс -m. Неизвестная RAM сохраняет ранее описанную
адаптацию Auto без отказа; явный предел проверяется. Оценка/RAM в подсказке
вместо исходной длинной строки со slash — адаптация ширины Qt-формы.
Windows Registry, сохранение по формату и остальные поля CInfo остаются
долгом. Форматирование — обязательный clang-format.

### Итерация: сохранение лимита памяти по формату

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::CFormatOptions::{FormatID,MemUse}`,
`CInfo::Formats`, `ZipRegistry.cpp::CInfo::{Load,Save}` (пределы памяти);
`CompressDialog.cpp::FindRegistryFormat`, `FindRegistryFormat_Always`,
`Get_FormatOptions`, `SaveOptionsInMem`, `SetMemUseCombo`, FormatChanged
и Save в OnOK. Поля остальных compression options не объявляются
перенесёнными: эта итерация ограничена памятью.

Скопированы поиск формата без учёта регистра, поиск-or-add, доступ по индексу,
сохранение MemUse предыдущего формата до его смены и формирование исходного
выбранного индекса при вставке нестандартного spec. Имена, порядок ветвей,
возвращаемые индексы и CObjectVector сохранены; используется уже побайтово
скопированный Common/MyVector.h. Qt presentation-зависимые UString/
CSysString заменены QString, Get_ArcInfoEx/Info.FormatIndex — текущим
и предыдущим именем Qt combo. Тела принадлежат одному Qt адаптеру
createarchivedialog_preferences.cpp; нет второго алгоритма поиска форматов.

В ZipRegistry.cpp скопированы Clear при Load, обход formatIDs,
создание CFormatOptions, чтение MemUse и Formats.Add; Save сохраняет
исходный обход FOR_VECTOR и заменяет только группу options. Win32
Registry open/create/GetRegString/SetRegString заменены QSettings INI
в выделенной группе Compression/Options существующего toolbar.ini.
Имена MemUse32/MemUse64 сохраняют исходное разделение архитектур.
Win32 CS_LOCK не нужен: загрузка/сохранение вызываются только в GUI thread.
Методы принимают путь вместо глобального registry key, что обеспечивает
детерминированные фикстуры. Другие группы настроек не затрагиваются.
Load/Save переносят только память/FormatID; остальные поля исходного CInfo
и CFormatOptions остаются долгом. Save вызывается после всех проверок OK;
Cancel и неуспешная валидация не записывают изменения. Форматирование —
обязательный clang-format.

### Итерация: исходные режимы и история диалога извлечения

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/ExtractDialog.cpp::{kPathModeButtonsVals,kOverwriteButtonsVals,
AddComboItems,AddUniqueString,CExtractDialog::OnInit,OnOK}`, `.h`;
`CPP/7zip/UI/Common/ZipRegistry.*::NExtract::CInfo::{Load,Save}`;
`CPP/Windows/FileName.cpp::NormalizeDirPathPrefix`.
`UI/Common/ExtractMode.h` скопирован побайтово в third_party/7zip.

Класс CExtractDialog и адаптированные тела находятся в зеркальных GUI путях.
Исходные таблицы, исходный порядок overwrite Ask/Overwrite/Skip/Rename/
RenameExisting, циклы AddComboItems и AddUniqueString, предел 16,
обработка выбранного/введённого пути, исключение выбранного или последнего
элемента, Trim, NormalizeDirPathPrefix, порядок ветвей смены режимов и
показа пароля, очистка/пересборка истории и сохранение в OnOK скопированы.
Сохраняются CObjectVector, enum и CBoolPair upstream. Win32 controls,
строки локализации и сообщения заменены Qt; UI constructor — presentation
адаптация существующей формы по ExtractDialog.rc. Исходная нормализация
пути перенесена локальным QString helper: пустой путь, проверка последнего
разделителя, добавление разделителя. Qt принимает оба разделителя на Windows.

Qt editable combo может удерживать индекс при редактировании текста:
узкий адаптер переводит этот случай в CB_ERR до исходной ветки OnOK.
После addItem восстанавливается typed text, поскольку Qt автоматически
выбирает первый элемент. Backend API переводит исходные enum в свои
ExtractionOptions; параметры 7zz остаются за ArchiveBackend. Absolute
paths не добавляются в combo (граница backend их не поддерживает); kAsk
адаптирован к прежнему отказу при конфликте вместо интерактивного callback.

Load/Save переносят только пути, режимы/Force и ShowPassword из NExtract
CInfo; QSettings заменяет Registry, GUI-thread вызовы не требуют CS_LOCK.
Сохраняются оригинальные имена ключей в Extraction; старый history/extract
читается только как миграционный fallback. Пароль не записывается.
MainWindow лишился самостоятельного алгоритма истории, старый cpp удалён,
header содержит только alias ExtractDialog=CExtractDialog. Неиспользуемые
SFX, Lang, Help, NtSecurity, ElimDup, SplitDest пока остаются долгом.
Обязательный clang-format изменяет оформление скопированного тела.

### Итерация: SplitDest и исходное разделение пути

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/Common/Wildcard.cpp::SplitPathToParts_Smart`,
`CPP/7zip/UI/GUI/ExtractDialog.cpp::CExtractDialog::{OnInit,OnOK,
OnButtonClicked}` (ветки IDX_EXTRACT_NAME_ENABLE / SplitDest),
`UI/Common/ZipRegistry.*::NExtract::CInfo::SplitDest`.

Указательный обход SplitPathToParts_Smart скопирован в существующий
зеркальный Common/Wildcard.cpp; имя, start/p, пропуск одного конечного
разделителя и обратный поиск сохранены. QString/QChar заменяют UString/
wchar_t, left/mid заменяют SetFrom/присваивание p. IsPathSepar сохраняет
исходное разделение платформ: на Windows slash/backslash, POSIX slash.
Структура borrowed Wildcard не реорганизуется; это единственный helper.

Скопированы ветки OnInit: при SplitDest.Checked исходный путь разделяется,
пустой prefix заменяется name (корень и одиночное относительное имя), иначе
name попадает в отдельный Edit. OnOK сохраняет исходные Trim, +=pathName,
NormalizeDirPathPrefix, сравнение SplitDest со старым значением и Def=true
при изменении. Значение по умолчанию true и Key_Get_BoolPair_true перенесены
в QSettings Load/Save NExtract::CInfo; только изменённый bool сохраняется.
Name field показывается/скрывается Qt toggled вместо Win32 OnButtonClicked.
Отмена не записывает флажок/историю, поле имени само не сохраняется:
в истории хранится составной конечный путь, как в upstream.

Qt presentation повторяет checkbox/Edit перед Path mode слева, Password
справа по ExtractDialog.rc. У исходного пустого checkbox добавлены tooltip
и accessibleName; Edit получил accessibleName/placeholder. Qt getter
до OK собирает preview из тех же полей, после OK возвращает DirPath.
Параметры движка не меняются: backend проверяет составное назначение
существующей защитой путей/ссылок и создаёт его. NtSecurity, ElimDup,
интерактивный Ask остаются долгом. Форматирование — clang-format.

## Полный профиль доступных параметров сжатия (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::NCompression::CFormatOptions`,
`ZipRegistry.cpp::CInfo::Save/Load`, `Key_Set_UInt32/Key_Get_UInt32`;
`CPP/7zip/UI/GUI/CompressDialog.cpp::SaveOptionsInMem`, `FormatChanged`,
`SetLevel2`, registry branches `SetMethod2`, `SetDictionary2`, `SetOrder2`,
`SetSolidBlockSize2`, `SetNumThreads2`.

Скопированы поля Level/Dictionary/Order/BlockLogSize/NumThreads/Method,
инициализация и ResetForLevelChange/Reset_BlockLogSize. Save/Load сохраняют
исходный порядок полей и DWORD sentinel `UInt32(-1)` (отсутствующее значение).
Win32 Registry заменён узким QSettings INI адаптером; UString Method заменён
QString на границе Qt. У SaveOptionsInMem сохранены ветви преобразования
UInt64 dictionary в DWORD, включая `UInt32(-2)` для 4 GiB. Нулевые Qt Auto
словаря/слова/потоков преобразуются в исходный sentinel. Solid хранит исходный
логарифм, в том числе 0/64/Auto. Пока параметры времени, произвольные options
и encryption method CFormatOptions не подключены.

В существующем Qt presentation adapter createarchivedialog_preferences.cpp
FormatChanged восстанавливает level → method → dictionary/order → solid →
threads, с исходной проверкой совпадения метода перед восстановлением его
параметров. QSignalBlocker заменяет отсутствие Win32 CBN_SELCHANGE при
программном заполнении. Недоступные значения возвращаются к Auto; выбор
большого словаря ограничивается текущим исходным рядом. SetLevel2 теперь
использует исходные ветви persisted level (default 5, invalid >9 →9) и
nearest-selection цикл. Обработчики смены уровня/метода/словаря перенесены
следующей итерацией. Пароли в профиль не входят.

## Смена уровня сжатия (2026-09-28)

Источник того же commit: `CompressDialog.cpp::OnCommand`, ветка
`IDC_COMPRESS_LEVEL`, `CompressDialog.h::SetMethod/MethodChanged`,
`ZipRegistry.h::CFormatOptions::ResetForLevelChange`.
В Qt-обработчик скопирован вызов ResetForLevelChange перед заполнением
метода и его зависимых контролов. Исходный reset очищает Method и присваивает
sentinel Level/Dictionary/Order/BlockLogSize/NumThreads; MemUse сохраняется.
Программное заполнение выполняет уже адаптированный FormatChanged, с
записью текущего выбранного Level в in-memory профиль перед заполнением:
это необходимо, поскольку Qt adapter заполняет также level combo, тогда
как исходная ветка CBN_SELCHANGE использует уже выбранный Win32 level.
Расчёты solid/threads/memory выполняются в исходном порядке зависимостей.
SFX/checksum ветки не применимы к текущим поддерживаемым 7z/ZIP.
Уровень 0 выключает Qt-контролы; список методов оставлен видимым и
недоступным (upstream очищает его). Пароль/лимит памяти не сбрасываются.

В той же итерации перенесены ветви `IDC_COMPRESS_METHOD`,
`IDC_COMPRESS_DICTIONARY`, `IDC_COMPRESS_MEM_USE`. Qt MethodChanged
заполняет словарь и слово в исходном порядке, затем solid/threads/memory
по вызывающему обработчику upstream. Параметры восстанавливаются только
при совпадении Method с сохранённым профилем; другие методы получают Auto.
При смене словаря скопированы SaveOptionsInMem, проверка
`blockSizeLog != kSolidLog_NoSolid && != kSolidLog_FullSolid` и
Reset_BlockLogSize: обычный размер solid сбрасывается к Auto, специальные
0/64 сохраняются. SaveOptionsInMem при смене лимита сохраняет explicit threads
перед их пересчётом. Оба сохранения меняют только in-memory профиль до OK.

## Рекурсивное копирование файловой системы (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CCopyState::CallProgress`,
`CopyFile_Ask` (UseReadWriteMode), `CombinePath`, `CopyFolder`.
В том же зеркальном файле сохранены порядок CallProgress → IsDestChild →
создание папки → перечисление → рекурсивная ветка/CopyFile_Ask, имена
srcPath2/destPath2 и накопление StartPos после MyCopyFile.
Самостоятельный обход copyEntry и обвязка copyFile удалены из worker.

CCopyState вынесен в существующий FSFolder.h и namespace NFsFolder для
доступа Qt-worker; из CProgressInfo нужен только StartPos. COM callbacks
заменены функциями Progress/CurrentFile, HRESULT/RINOK — bool и проверкой
результата; ошибки передаются в ErrorMessage. QString/QFileInfo/QDir заменяют
FString/CFileInfo/CEnumerator и платформенные операции. Qt entryInfoList
не предоставляет ошибку Next; перед обходом проверяется доступность папки.
Скрытые и системные записи включены, ссылки отклоняются, stage создаётся
только в отсутствующей папке (mkdir вместо CreateComplexDir).

AskWrite адаптирован как отказ при конфликте до открытия файла, а не
интерактивный запрос. Неиспользуемые skip/TotalSize и Win32 CopyFileEx ветки
не подключены. MoveMode/MoveFile_Sys/RemoveDir остаются у транзакционного
worker: он использует rename и откат, копирование с удалением источника
нарушило бы текущую гарантию сохранности при ошибке. Межтомный перенос
остаётся неподдержанным. Mandatory clang-format меняет оформление.

## Диалог копирования/перемещения и история (2026-09-28)

Источник того же commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/CopyDialog.cpp`, `.h`, `.rc`,
`ViewSettings.cpp::SaveStringList`, `ReadStringList`, `SaveCopyHistory`,
`ReadCopyHistory`, `AddUniqueStringToHeadOfList`, `App.cpp::CApp::Copy`
(обновление истории назначения). Исходный класс CCopyDialog и поля
Title/Static/Value/Info/Strings сохранены в зеркальном пути. OnInit заполняет
историю перед Value и устанавливает Info; OnOK получает Value только при OK.
OnButtonSetPath сохраняет cancel → NormalizeDirPathPrefix → SetCurSel(-1)
→ SetText. Qt QFileDialog заменяет MyBrowseForFolder, slash-проверка после
fromNativeSeparators — платформенный NormalizeDirPathPrefix.

Win32 Attach/SetItemText/NormalizeSize/OnSize заменены Qt-контролами и layouts
с исходным порядком label → path/browse → info → OK/Cancel. Вместо Static
SS_LEFTNOWORDWRAP используется read-only QPlainTextEdit без переноса строк:
прокрутка сохраняет доступ к длинным именам и большому выделению. Входной
список имён и конструктор — существующая Qt интеграция, а не перенос полного
CPanel::GetItemsInfoString (property totals ещё не подключены). Исходный
kCopyDialog_NumInfoLines сохранён для будущей сводки.

Save/ReadStringList используют QSettings вместо Registry и explicit settings
path вместо HKEY; вызовы идут в GUI-thread, lock не нужен. Имя CopyHistory
сохранено; старый copy/history не читается и не изменяется (fallback удалён
в итерации истории/избранного по указанию пользователя). Исходный цикл
удаляет все совпадения без учёта регистра и вставляет новый путь в начало;
App copy workflow ограничивает список 20 записями и сохраняет после принятого
существующего назначения. Проверка/создание назначения и backend остались
на прежних границах; произвольное переименование цели/новая папка из CApp::Copy
пока не перенесены. Самостоятельный copydialog.cpp удалён; header — alias.
Форматирование изменено обязательным clang-format.

## Сводка выделения для Copy/Move (2026-09-28)

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/App.cpp::CPanel::GetItemsInfoString`,
`AddSizeValue`, `AddValuePair1`, `AddValuePair2`, `AddPropValueToSum`;
`PanelListNotify.cpp::ConvertSizeToString` (обе перегрузки, INT_TO_STR_SPEC);
`Panel.h::GetItemsInfoString`, `CopyDialog.h::kCopyDialog_NumInfoLines`.
Скопированы исходные циклы и ветви: суммы папок/файлов раздельно, неизвестное
значение UInt64(-1) распространяется без дальнейшего счёта, общий размер
выводится только при двух ненулевых определённых суммах, затем путь и первые
11−6 имён с разделителем для папок и многоточием для остатка. Сохраняются
имена CPanel, методов и _currentFolderPrefix. Форматирование UInt32/UInt64
скопировано целиком с промежуточным Byte[32], wchar_t[32] и группами по три
цифры; QString::fromWCharArray заменяет UString только на выходе.

В зеркальном Panel.h портирована только часть CPanel, владеющая сводкой.
IFolderFolder заменён snapshot CObjectVector<CPanelItem> с RelPath/IsFolder/
SizeDefined/Size; upstream CRecordVector<UInt32> используется без изменений.
AddPropValueToSum получает тот же индекс и свойство Size без COM/PROPVARIANT;
языковые resource строки заменены русскими Qt строками. QString заменяет
UString для UI текста, noexcept — throw(). Конструктор snapshot и optional
Info аргумент CCopyDialog — узкая интеграция: исходный App устанавливает Info
до Create, а Qt создаёт виджеты в конструкторе.

MainWindow::selectedCopyInfo только собирает свойства выбранных строк:
относительное имя из представления, размеры файлов из UserRole, папок архива
из CProxyArc. Для папок диска SizeDefined=false соответствует FSFolder.cpp
kpidSize до FolderStat_Defined. Missing-size свойства архивных файлов уже
нормализуются текущим backend в 0; различение отсутствующего Size остаётся
долгом parser/metadata, без изменения границы ArchiveBackend в этой итерации.
Сводка не устраняет пересечение выбранных папок/файлов: как upstream, суммы
относятся к выбранным строкам. `sevenzip_panel` собирает эти UI модули один
раз, не добавляя QWidget-зависимость к sevenzip_common. clang-format меняет
оформление. Предыдущий конструктор имён оставлен как compatibility fallback;
рабочие F5/F6 теперь всегда получают перенесённую сводку.
