# Сверка интерфейса с оригинальным 7zFM

## Current final acceptance matrix (2026-10-10)

Матрица различает текущие native launcher и delivery components и прежние full/native receipts.
Checked означает только названную
проверку; partial/unverified не дают итоговой приёмки P3-06.
[Текущий launcher, логи, входы и ограничения](upstream-port-audit.md#p3-05--native-associated-launcher-and-process-lifetime-2026-10-10);
[предыдущий полный Linux gate и версии](upstream-port-audit.md#p3-03--current-full-linux-gate-after-status-corrections-2026-10-10).

| Область приёмки | macOS arm64 | Linux x64 | Windows x64 |
| --- | --- | --- | --- |
| Default format/build/CTest | checked: current321/321,5075Qtpass/0fail/56Windows-NTFS skips | partial: current consumers7/7,119Qtpass/0fail/0skip; prior full318/318 | partial: current native/consumer focused11/11,134Qtpass/0fail/0skip; прежние list-settings exposure failures сохранены, current full не принят |
| Исходные модули, headers/resources/callers | partial: [109 source receipts](omitted-scope-audit.md#109-unit-source-receipt-ledger-2026-10-10); identified launcher/continuation gap corrected, whole workflow acceptance open | partial: общий source scope тот же; POSIX no-handle consumers checked | partial: original native launcher and real handle/disk/browser/archive consumers checked; whole acceptance open |
| EN/RU, normal/compact, команды и workflow | partial: автоматические contracts/regressions проходят; полный нативный обход не принят | partial: offscreen contracts/regressions проходят; физический ввод не принят | partial: выполненные focused consumers/JSON; service desktop не доказывает показ окон |
| Нативные dialogs/shortcuts/icons/drag/registration/FS | partial: историческая ручная Finder-проверка принята отдельно; текущий полный native обход не проверен | partial: Unix FS проверен; native desktop/physical drag не проверены | partial: прежние scoped native receipts сохранены; текущий interactive desktop недоступен |
| Наблюдение текущего приложения через Computer Use | unverified: доступ к Sevenzip Desktop не одобрен | unverified: не выполнялось | unverified: нет активного runner logon |
| Сверка с отрисованным Windows 7zFM того же pin | unverified: нет полного rendered reference | unverified: нет полного rendered reference | partial: исторический System menu; полный текущий rendered audit не принят |
| Runtime packages и чистая ОС | partial: current8d56d8d Release/relocation15/15, notices/signature,11 Mach-O import review; clean OS unverified | partial: current8d56d8d Release/relocation14/14; isolated userspace15/15 без SDK/host Qt, actual image hashes checked; отдельный desktop/kernel unverified | partial: current8d56d8d Release/relocation14/14, CRT/notices,26 PE import review/system resolution; clean OS unverified |
| Cold/stable Windows, итоговая трёхплатформенная приёмка | partial: общий gate остаётся открытым | partial: успешный Linux gate не заменяет Windows | unverified: повторения и full gate требуют активного desktop |
| GitHub CI | N/A по явному решению пользователя сохранить удаление workflow | N/A по тому же решению | N/A по тому же решению |

Slow suites исключены из default gates по AGENTS.md и не объявляются принятыми.
Текущий source style строк состояния и geometry consumers — в
[P3-06 receipt](upstream-port-audit.md#p3-06--source-status-bar-style-and-current-geometry-consumers-2026-10-10).
Текущие артефакты, dependency/rootfs proofs и точные границы relocation-проверки — в
[P3-04 receipt](upstream-port-audit.md#p3-04--delivery-after-native-launcher-restoration-2026-10-10).
Linux userspace/rootfs, отсутствие host Qt, missing-module и TMPDIR проверены
в [isolated runtime receipt](upstream-port-audit.md#p3-04--isolated-linux-runtime-and-temporary-directory-contract-2026-10-10).
Подробная методика macOS/Windows load commands и системных импортов — в
[binary dependency receipt](upstream-port-audit.md#p3-04--current-macos-and-windows-binary-dependency-closure-2026-10-10).
Открытые обязательства остаются в P3-03/04/05/06;43/47 не изменено. Историческая
успешная проверка не превращается в проверку нового бинарника или другой ОС.

Уточнение пользователя 2026-10-03: исправить во всём приложении подобные
самостоятельные допущения, как выбор источников внутри Add to Archive.
Это обязательная часть текущего портирования и итоговой P3-06 приёмки.
Прежняя запись «Qt adaptation» сама по себе не оправдывает иной workflow.

Эталон исходников: `https://github.com/ip7z/7zip.git`, полный commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Для поведения проверяются
обработчики и callers; для отрисовки — настоящий Windows 7zFM того же commit.
Требования доступности, масштабирования и платформенные сервисы фиксируются
с конкретной причиной. Они не разрешают добавлять новые шаги, поля, команды
и подтверждения ради удобства реализации.

| Область | Исходный владелец | Найденное отличие / необходимая проверка | Статус |
| --- | --- | --- | --- |
| Native Copy terminal phases | App.cpp::OnCopy; PanelCopy.cpp; CPanel::MessageBox_Error_HRESULT | Premature refresh consumed pending Move publication while guards blocked loading. Keep pending publications until guard release and present writer result first. Replace independent Qt Warning with original Critical/OK owner; retain both source diagnostic phases and macOS caption omission. | Six stale-Move and four wrong-icon old-product RED cases; final focused2/2 on three OS and32reports/OS. Full/native snapshot precedes localized diagnostic follow-up. Physical/rendered/current-package/final scope stays open. [Receipt](upstream-port-audit.md#p3-05--native-copy-terminal-phases-and-retained-guards-2026-10-10). |
| Ошибка обновления после Copy/Move | App.cpp::OnCopy; PanelItems.cpp::RefreshListCtrl/OnReload | Порт подменял исходный final RefreshListCtrl вызовом OnReload и показывал лишнюю ошибку загрузки. Восстановлены сохранённое состояние и игнорирование reload HRESULT, включая worker; явный Refresh и ошибки записи/отката сохраняют диагностику. | Mac full320/320; Linux15/15, Windows15/16 с последующим исправленным localized1/1 на обоих гостях;120reports. Native/rendered/current-package/final приёмка открыта. [Receipt](upstream-port-audit.md#p3-05--copy-publication-caller-and-result-policy-2026-10-10). |
| Нативный протокол drag и средняя кнопка | PanelDrag.cpp::CDataObject/CDropSource/CDropTarget, CompressDropFiles; EnumFormatEtc.cpp | Межпроцессные Windows-форматы были заменены только in-process carrier; отсутствовали performed-effect и средняя кнопка, временный корень был drive-relative. Исходный protocol восстановлен; Qt получает входящий IDataObject через публичный application-owned converter. WasCanceled сохраняет принятую защиту выделения при Cancel. | Mac full320/320; focused16/16 Mac/Linux,19/19 Windows;153reports, исходный порядок popup и видимые действия. Physical pointer/Explorer/rendered приёмка остаётся обязательной. [Receipt](upstream-port-audit.md#p3-05--original-native-paneldrag-protocol-and-qt-consumers-2026-10-10). |
| Эффект перетаскивания по умолчанию | Panel.cpp::GetDriveOrNetworkPrefix; PanelDrag.cpp::IsItSameDrive/GetEffect | AltStreams, пустой/относительный FS-путь ошибочно предпочитали Move при положительном ответе Qt volume service. Восстановлены исходные проверки и Windows prefix loop; POSIX заменяет только определение тома. Остальные lifecycle/status/control ветви Panel имеют конкретную карту Qt owners. | Mac full319/319; focused14/14 на Mac/Linux/Windows. Native ADS consumer проходит Windows. Физический ввод/OLE receiver/rendered приёмка и whole PanelDrag остаются открыты; [receipt](upstream-port-audit.md#p3-05--source-default-drop-effect-and-panel-core-disposition-2026-10-10). |
| Адресные клавиши и состояние плоского вида | Panel.cpp::CMyComboBoxEdit::OnMessage, SetListViewMode, ChangeFlatMode; App.h | Control-W из адреса не закрывал окно; плоский вид в Drives менял только QAction и терялся при View activation/переходе в папку. Исходные обработчики и состояние восстановлены; Qt заменяет сообщения, стили и отмену загрузчика. Tab/Backtab сохраняет draft и фокусирует собственный список. | Mac full318/318 и focused22/22; Linux22/22, Windows20/22 с прежними popup-exposure и pre-dispatch SendInput failures. Новые cases проходят;120normal/compact JSON проверены,24compact Windows status elisions учтены. Native/rendered приёмка открыта. [Receipt](upstream-port-audit.md#p3-05--panel-address-keys-and-source-view-state-2026-10-10) |
| Открытие меню «Вид» и примеры времени | MyLoadMenu.cpp::OnMenuActivating; resource.rc View; App/Panel getters | Дата и пять примеров времени заполнялись только при открытии дочернего popup. Родительское View теперь выполняет исходный обработчик, отражая view/sort/flat/toolbar/UTC без выполнения команд. QAction сохраняют identity и bindings. | Actual RED4 → GREEN; обе панели/EN-RU, precision/UTC persistence, CTime fallback и normal/compact main/View/time JSON. Native desktop/rendered Windows приёмка остаётся открытой. [Receipt](upstream-port-audit.md#p3-05--source-view-menu-activation-2026-10-10) |
| Большие страницы: запуск и Apply | FM.cpp::SetMemoryLock; SettingsPage::OnApply; CompressCall::AddLagePagesSwitch | Восстановлены исходные native privilege calls, save-on-failure и риск до `-slp`; POSIX option остаётся disabled. Layout/text не изменены. | Actual RED; MemoryLock11Qt PASS3OS. Mac full314/314, Linux13/13; Windows12/13 с двумя прежними external startup-window сбоями.60JSON без неожиданного clipping, Benchmark сохраняет проверенную прокрутку. Права runner не изменены; native allocation/rendered/final acceptance открыты. [Receipt](upstream-port-audit.md#p3-05--startup-settings-and-large-page-engine-policy-2026-10-10) |
| Окно: сохранённое положение и максимизация | FileManager/FM.cpp::InitInstance/SaveWindowInfo; ViewSettings.cpp::CWindowInfo | Исходный InitInstance применяет rectangle/maximized и Panels через узкие Qt services. Закрытие и повторный запуск проверяют normal/maximized, панели, фокус и разделитель; Qt сохраняет logical client coordinates. | Window placement13Qt/8JSON PASS на каждой из3ОС; Mac full312/312, Linux9/9. Windows8/9: два external-process startup сбоя повторяются на прежнем коде, причина/native rendered/final приёмка открыты. [Receipt](upstream-port-audit.md#p3-0516--source-initinstance-window-placement-2026-10-10) |
| Разделитель: исходная координата и пропорция resize/F9 | FM.cpp::CSplitterPos, WM_CREATE/WM_SIZE/MoveSubWindows/SaveWindowInfo | Исходные границы, fixed-point ratio и округление, ручка4; Qt переносит события мыши/resize и сохраняет ограничения содержимого. | Actual RED saved300→299/298; window19Qt PASS на3ОС,30JSON без clipping. Mac full313/313, Linux16/16; Windows14/16 с прежними external-startup/exposure сбоями, подтверждёнными на предыдущем коде. Native/rendered acceptance открыта. [Receipt](upstream-port-audit.md#p3-0516--source-splitter-resize-and-persistence-2026-10-10) |
| Copy/Move: история назначения и доступность после popup | FileManager/ViewSettings.cpp::AddUniqueStringToHeadOfList; App.cpp::OnCopy; PanelMenu.cpp::OnContextMenu | Qt case folding удалял исходно отдельную строку K при добавлении K. Восстановлены source UStringVector/comparison и Qt boundary; Cancel/accept/reopen проверяются через реальные source dialogs. Завершение popup восстанавливает доступность QAction после временных source guards. | macOS full311/311 и Linux11/11 PASS; Windows10/11, native list-settings требует активного рабочего стола. [Receipt](upstream-port-audit.md#p3-05--viewsettings-and-source-copy-history-2026-10-10); rendered/global приёмка отдельно |
| Add: выбор источников | FileManager/Panel.cpp::AddToArchive; GUI/CompressDialog.rc | Picker удалён из всех ветвей диалога. Исходный CPanel::AddToArchive перенесён целиком с typed CompressFiles services: источники из панели, пустое выделение и архивная панель дают исходные ошибки, native ShowDialog/UpdateArchive создаёт/обновляет output. Семь форматов, параметры, metadata/password/Cancel проверены; unknown-extension update использует исходный two-panel Copy. Drag Add также идёт через source CompressCall2, с native success/error/Cancel. | macOS focused16/16PASS14.41s; свежие full/Windows/Linux проверки идут |
| Add: компоновка и дополнительные поля | GUI/CompressDialog.rc/.cpp | Format и compression слева; Update/Path и Options/Encryption справа; volumes/parameters слева; лишняя Compression group, отдельные Save in/preview и Create/Add вместо OK удалены. Folder label над Archive+`...`. Encryption method теперь в одной строке с combo, decompression memory выровнено вправо по настоящему Windows runtime. Normal/compact geometry и текущие 14 Add/14 Copy JSON просмотрены; labels не обрезаны, compact нижние поля доступны прокруткой. AddFilter/OnButtonSetArchive перенесены; порядок фильтров, aliases, extension/Cancel проверены. Устаревшая constructor-only existingTarget ветвь с блокировкой полей ещё подлежит удалению. | macOS focused19/19PASS14.93s; текущие platform/full проверки и legacy branch открыты |
| Add: размер окна | GUI/CompressDialog.rc; настоящий Windows 7zFM | В Qt оставался размер760×720 от прежнего picker. Размер теперь следует содержимому и доступной области экрана. Повторный настоящий Windows runtime v9:642×578, OK/Cancel/Help полностью доступны; local640×571 JSON и default/frame regression проверены. | Windows13/13PASS70.86s и runtime проверены; snapshot предшествует следующему Add source port |
| Add: значения памяти | GUI/CompressDialog.cpp::PrintMemUsage; CompressDialog.rc | Сравнение настоящего 7zFM и Qt на Windows выявило отсутствие limit/RAM в видимой строке и label сбоку вместо сверху. Исходный PrintMemUsage перенесён с Qt text return; ветви и новые EN/RU reports проверены. Новый снимок запущенного приложения показывает полную строку. | macOS full227/227; Windows/Linux focused13/13, global acceptance открыта |
| Add: Delete и update mode | GUI/CompressDialog.cpp::OnInit/OnOK; Common/Update.cpp::CheckDeleteAfterCompressingActionSet | Самостоятельное отключение checkbox для Freshen/Update/Sync удалено. Исходная форма принимает intent, source worker проверяет совместимость после OK. Успех/Cancel/отказ с сохранением input проверены. | macOS full227/227; Windows/Linux focused13/13, global acceptance открыта |
| File/menu/toolbar/context | FileManager/MyLoadMenu.cpp, App.cpp, PanelMenu.cpp, resource.rc; Explorer/ContextMenu.cpp | Independent Open/Create/Add-to-open/Extract-selected QAction, extension IDs/dispatch и Ctrl+O picker удалены. Source toolbar/Explorer Add, two-panel Copy update, selected Copy To/F5 и Open/Open Inside сохранены. Archive popup больше не добавляет отдельную группу Qt Add/Extract/Test/Info. Program-menu source lifetime и Info→Properties подключены. Journal и built-in Text View/Edit, их collector/dispatch/FileViewerDialog также удалены; source F3/F4 и external process/update prompts сохранены. | Указанные aliases удалены; main_shell проверяет отсутствие действий. Полная native/rendered/P2-09/P3-06 приёмка остаётся открытой |
| Extract и prompts | GUI/ExtractDialog.rc/.cpp, FileManager/OverwriteDialog.rc/.cpp | Проверить исходный набор полей, defaults, скрываемые состояния, overwrite questions и кнопки. Проверки безопасного назначения остаются за backend. | Открыто |
| Copy/Move, две панели | FileManager/CopyDialog.rc/.cpp, App.cpp, PanelCopy.cpp | Проверить последовательность выбора источника/цели, defaults, prompts и команды в архивном namespace; сохранить исходный способ добавления в архив. | Открыто |
| Navigation/list/views/selection | FileManager/Panel.cpp, PanelKey.cpp, PanelSelect.cpp, PanelItems.cpp, PanelListNotify.cpp | Проверить focused versus selected, пустую папку, адрес, root/parent, shortcuts, columns и transitions во всех режимах. | Открыто |
| Options и дочерние страницы | FileManager/OptionsDialog.cpp, Windows/Control/PropertyPage.cpp и *Page.rc/.cpp | Исправлена самостоятельная стартовая Settings page: исходный property sheet начинает с System. Шесть source pages, Apply/OK/Cancel/restart, обе панели и EN/RU normal/compact проверены в P1-12.2. Qt Tab cycle сохраняет pending input и доступный фокус; Windows может посетить footer перед полями. Полная runtime-сверка полей/контролов с оригиналом остаётся обязательной. | P1-12.2 accepted; macOS full234/234, Windows8/8, Linux8/8, 72 current page JSON reviewed; rendered/global audit открыт |
| Standard dialog captions | FileManager/LangUtils.cpp::LangSetDlgItems, GuiCommon.rc и dialog OnInit callers | Исправлены English Cancel при выбранном RU и сброс Memory Continue в OK после смены языка. Исходная таблица standard IDs подключена к двенадцати dialog consumers; roles/signals сохраняются. Qt native dialogs и Options property-sheet имеют отдельного платформенного владельца. | P3-05.9 accepted button scope:27Qt/0skip и по48 EN/RU normal/compact footer reports на3OS; rendered/global acceptance и source colon helpers .10 открыты |
| Progress/messages/password | FileManager/ProgressDialog2.rc/.cpp, MessagesDialog.rc/.cpp, PasswordDialog.rc/.cpp | Проверить дополнительные тексты/кнопки, pause/cancel/close, повторные ошибки и modal lifetime. Полный ProgressDialog2 проверен2026-10-10; отмена/успех восстановлены без добавленных Qt-значков, ошибка сохраняет Critical. [Карта исходных тел](omitted-scope-audit.md#progressdialog2-complete-body-disposition-2026-10-10); native/rendered приёмка остаётся. | Открыто |
| Properties/Comment/Link/Split/Combine/Benchmark | Соответствующие FileManager и GUI .rc/.cpp | Проверить параметры вызова, набор контролов, условные состояния, action semantics и Help. Split invalid-volume восстановлен как исходный modal Critical/OK2026-10-10; прежняя inline-адаптация удалена. [Карта диалогов](omitted-scope-audit.md#primitive-input-and-read-only-dialog-bodies-2026-10-10); native/rendered приёмка остаётся. | Открыто |
| Help/About и OS integration | FileManager/AboutDialog.rc/.cpp, HelpUtils.cpp, SystemPage.cpp, Explorer | Проверить самостоятельные экраны/команды и необходимые различия native платформ; не подменять исходный workflow новой формой. | Открыто |

Source keyboard reconciliation restores the complete active PanelKey::OnKeyDown
dispatcher and PanelMenu clipboard bodies. Ctrl+C/Ctrl+Insert copy actual
selected raw names in source-index order with CRLF, Ctrl+Shift+A follows Select
All and Ctrl+W posts Close. Modified shortcuts, bookmark precedence, guards,
both panes/four views, EN/RU normal/compact and editor launches are covered;
the inactive source Ctrl+PgDn branch remains a consumed no-op despite its menu
caption. This closes the identified independent keyboard dispatcher gap,
not the whole Navigation row or rendered Windows parity. Native Windows
keyboard execution remains pending desktop login; see
[source/evidence](upstream-port-audit.md#p3-05--source-keyboard-dispatcher-2026-10-07).

Native changed-archive Parent for disk-extracted children now has bounded consumer verification against
PanelItemOpen::OpenParentArchiveFolder and PanelFolderChange::CloseOneLevel/
OpenParentFolder: No and Cancel discard/exit, failed or cancelled Yes preserves
the modified spool while still returning to the parent, and final Refresh
errors remain ignored. Five native representative2/301-row cases check recovery, real writer
retry, child focus, peer bytes/guards and normal/compact controls. This closes
that consumer gap, not the full Navigation/Copy/Move or rendered UX rows above;
see the [dated audit](upstream-port-audit.md#p3-0515--changed-native-nested-archive-parent-2026-10-07).

Конкретный пропуск Root/drive-view command зафиксирован в
[P3-05.13 source audit](upstream-port-audit.md#p3-0513--source-root-command-target-implementation-validation-active-2026-10-06)
и text contract: проверка непустого списка и selection marks не доказывает
исходную цель команды. Исходный переход и consumers теперь интегрированы;
проверки provider/prefix/Parent/Back/peer state добавлены. Native/rendered
приёмка остаётся обязательной, текущие свидетельства — в source audit.

Interarchive Copy/Move теперь сохраняет provider при успешной публикации и
pre-update selection при физическом rollback. Native ZIP/7z и transport
ошибка/Cancel/Delete No/Cancel/error/engine cancellation проверяются отдельным
[сценарием P3-05.15](upstream-port-audit.md#interarchive-copy-publication-and-rollback-2026-10-07).
Отдельный interarchive_recovery проверяет failed rollback/reopen, fallback folder,
предупреждения и повторный F5 после восстановления временного назначения; обе
панели EN/RU table/icon/standard-alternative в normal/compact состояниях.
Native metadata/platform и полная UX-приёмка остаются открытыми.

Source OnCreate теперь начинает публикацию до открытия следующей панели;
позднее создание peer использует тот же owner без дополнительного Qt render.
Reference — полное программное построение list/header/status в upstream
Panel.cpp::OnCreate и App::Create/SwitchOnOffOnePanel. Initial/late creation,
2/301-row FS/transport/native ZIP/7z, оба view/selection режима и EN/RU
normal/compact проверяются в panel_creation_publication. Это не pixel/rendered
Windows и не полная native renderer-failure приёмка.
Saved FS0/main archive1 дополнительно завершает первое чтение внутри второго
Bind; source-provider counts должны соответствовать видимым строкам обеих панелей.
Отдельный panel_creation_path проверяет requested versus saved main path,
включая ordinary startup справа, и terminal focus после двух301-row scans.
Явный ввод в peer или новый public archive request имеет приоритет над ещё
не завершённым initial focus. Оба режима/языка/selection и normal/compact
проверяются без утверждения rendered Windows parity.
После modal opening выбранный адресный контрол сохраняет фокус; API fixture
доказывает выбор peer настоящим mouse input, title и physical focus до запроса.
Немедленное раскрытие сохранённого peer до первого focus callback сохраняет
его адрес/строки; отложенный initial focus нужен только ещё читающему стартовому
списку. Синхронный source Create уже устанавливает фокус.
Source FM.cpp::MoveSubWindows задаёт panel0 слева/panel1 справа. Вставка Qt по
индексу сохраняет этот порядок при archive-first и late peer creation; обе
normal/compact матрицы теперь проверяют положение панелей явно.

Полная сверка App.cpp выделяет
[P3-05.15](upstream-port-audit.md#p3-0515--source-copymove-destination-and-continuation-reconciliation-required-scope-2026-10-06):
восстановить исходный выбор/классификацию destination и Copy/Move continuation,
пустая цель peer и однопанельный reducer уже восстановлены с F5/F6/Cancel/EN-RU
normal/compact consumers. Classification/refusals,
path+HRESULT diagnostics и captured execution теперь восстановлены; staged
options возвращены к исходным defaults, native metadata evidence ещё требуется.
Menu/F5/F6 без marks теперь доходят до source OperSmart; EN/RU normal/compact
summary/Cancel проверены. Timer guard handoff исправлен для одной/двух панелей;
Source post-operation saved states/title/notify/focus body теперь выполняется
после terminal execution и публикации, с сохранением peer selection и снятием
ordinary source marks; deferred progress больше не сбрасывает title. Оба source
roles/EN-RU и normal/compact panel JSON проверены. Full RefreshListCtrl load
semantics/native completion и остальные source branches ещё обязательны.
Unconditional destination publication теперь восстановлена для цели вне peer,
с retained selection и ожиданием bounded/worker load; namespace OnReload также
публикует provider rows. Success/Move/dialog Cancel/overwrite Cancel и resulting
normal/compact geometry входят в focused consumers; полная source-load и
native/platform приёмка остаются открытыми.
Source load/property stage теперь общий для namespace/archive/FS renderers,
включая удаление старых строк до смены provider indices и отказ при неверном
directory type. Existing flat/selection/Copy consumers и namespace error-state
geometry проверяют эту границу; это не закрывает remaining RefreshListCtrl
presentation и native/HRESULT/timer scope.
Синхронные namespace/archive HRESULT больше не теряются renderer callback.
Namespace Refresh использует исходный OnReload с исходным error message;
EN/RU explicit/timer/command/failure/Cancel/recovery и60 OS-managed message
reports проверяют эту границу. Post-Copy selection stage теперь сохраняет
исходный early return после load failure; обе source roles/Copy/Move проверяют
ошибки, Cancel и повторную загрузку без исключения из count provider.
Синхронный FS reload больше не теряет provider через повторную привязку;
filesystem Refresh идёт через source OnReload. EN/RU обе роли и реальный FS
peer Copy/Move проверяют Cancel/ошибку и восстановление, с48 message probes.
Реальная worker error теперь сохраняет HRESULT/provider и показывает source
message до завершения async publication. Временная peer projection больше не
получает source state при завершении Copy/Move; title/focus и peer recovery
проверены, включая normal/compact resulting-panel reports.
Missing-folder reload больше не оставляет скрытые native items/marks после
очистки Qt rows: source enumeration и восстановление проверены с обеими
selection modes, EN/RU и обеими панелями, с32 empty-view reports.
Missing-address descendants теперь проверены через source closest ancestor с
actual address/title/focus и unchanged peer/bytes;32 panel reports покрывают
оба размера. Qt transport получает исходное Windows outer-quote handling через
общий helper, с native-only conditional routes для последующей проверки.
Исходный CalledFromTimer теперь сохраняет адресный draft, cursor/selection и
focus при обновлении списка; manual Refresh публикует текущий путь. Проверки
обоих roles/EN-RU охватывают малый/worker/flat FS и bound root/ZIP consumers.
Родительская строка теперь получает исходный fallback selection и явный
name-based focus. Open/Enter/Open Outside сохраняют доступность для выбранного
`..` через source handlers; native marks/operated indices исключают sentinel.
Hidden-row/standard-alternative/small-worker-flat checks охватывают обе панели
и EN/RU, с192 normal/compact reports и private control bytes.
Source row-icon policy также подключена к FS/archive/namespace consumers:
slow-folder query, positive-index validity, исходные атрибуты/mask/directory bit.
Lookup остаётся в bounded GUI publication; failure/Cancel сохраняет HRESULT и
очищает failed rows. Real FS/ZIP small/301 on/off и normal/compact geometry имеют
focused coverage; native Windows shell appearance и global acceptance открыты.
Source name callback также подключён: исходные RLO/space/empty/Windows-marker
branches, generic cell errors и suppression. Raw operation/editor names остаются
отдельными от display text; Rename completion читает editor value. Public callback
и real ZIP Rename/Cancel/refusal/bytes покрыты с14 normal/compact name reports;
native marker rendering и full acceptance остаются открытыми.
Уточнение2026-10-10: native ListView probe подтвердил, что LVS_SINGLESEL не
выделяет строку при восстановлении одного фокуса. Qt теперь сохраняет исходный
SelectFocused и отдельные marks; без selected row focused metadata остаётся
пустой. Providerless model rows не создают source marks. Public restore/status
и manual/timer Reload consumers проверяют этот контракт; native/rendered
приёмка остаётся отдельной. [Evidence](upstream-port-audit.md#p3-05--panelitems-selection-and-provider-lifecycle-2026-10-10).

Полная [карта PanelItems](omitted-scope-audit.md#panelitems-complete-body-map-2026-10-06)
выявила ещё незавершённые исходные потребители: красный foreground удалённых
архивных элементов (IsItem_Deleted / OnCustomDraw, default marking enabled)
теперь восстановлен через исходный getter/policy и Qt model/delegate; прежний
archive-level flag получает потребителя. Table/icon/selection/guard/reload/rebind
и current red-pixel checks пройдены на macOS. Мутации header column menu
теперь принадлежат исходному ShowColumnsContextMenu: показ добавляет колонку
в конец, скрытие сортируемой колонки меняет metadata без немедленной сортировки.
EN/RU обе панели, active/inactive owner, Cancel, move/resize/Refresh и focus
проверены; Qt сохраняет скрытые logical sections и пишет компактный visible order.
Текущие native checks и rendered Windows reference обязательны.
InitColumns также исправила принудительное возвращение Name в начало: source
логический индекс сохраняется отдельно от visual order, который теперь переживает
Refresh/reopen в обеих панелях EN/RU. Прямой вызов очищает source marks до
enumeration, сохраняя Qt rows/selection до следующего restoration consumer.
Focused и normal/compact JSON проверки пройдены на macOS. Повторная сверка
SaveListViewInfo выявила независимую запись Qt width/order в `_columns`: она
удалена. Hide/show снова берёт исходную catalog width; resize без hide/show
сохраняется для следующего InitColumns/reopening. Прежняя регрессия173px после
hide/show закрепляла Qt-допущение и исправлена; catalog/visible/saved metadata
проверены отдельно.
Raw/generic publication также исправлена: Qt больше не смешивает первый PropID
с точным descriptor исходной колонки и не вытесняет generic Size raw-колонкой.
Namespace alignment теперь следует исходному GetColumnAlign. EN/RU обе панели,
неактивный provider/peer, RawProp errors/Cancel/recovery и16 JSON проверены на Mac.
Duplicate-ID menu теперь следует исходным first-visible-ID delete и append-new-copy:
повторные пункты доступны, новые секции сохраняют callback/source indices, отдельные
ширины и selection/peer focus. EN/RU обе панели и32 normal/compact JSON проверены
на Mac. Duplicate-ID sort теперь использует первый catalog raw-флаг и исходный
toggle по PropID через generic/raw/append секции. Qt сигнал применяется после
автоматического mouse sort; индикатор остаётся на нажатой секции, Size action
привязана к PropID.16 EN/RU обе панели с empty raw, selection/peer/Refresh и64
normal/compact JSON проверены на Mac. Текущие native/rendered проверки обязательны.
Schema/error consumers также восстановили отсутствие лишнего Name при raw-only/
no-Name/empty catalog и повторное перечисление одного provider/type. Source
потребляет записанные count при S_FALSE/E_FAIL и пропускает individual info errors/
IsDir; existing initialized-zero адаптация обрабатывает незаписанный выход.
12 EN/RU обе панели с menu Cancel, selection/focus/peer/bytes/recovery и72 JSON
проверены на Mac. Qt zero-height empty header записан как platform presentation;
native/rendered приёмка остаётся обязательной.
Label-only highlight теперь следует первому source logical subitem при raw Name
и no-Name; фактическая delegate paint-регрессия ловит прежнее невидимое выделение.
Icon/list projection теперь следует тому же source subitem для текста и значка,
включая menu hide/re-add raw Name и append-порядок. Lifecycle охватывает четыре
режима, EN/RU обе панели, peer focus/selection и normal/compact geometry.
Callback publication также удаляет лишний count gate перед source GetItemText:
валидные опубликованные cells сохраняются при позднем count HRESULT. Property
error/empty text, notify/provider guards и rebind/selection/peer остаются проверяемыми
consumers;32 detail/icon recovery/rebind reports покрывают EN/RU обе панели на Mac.
Сквозной FS/real ZIP/7z/schema/reopen consumer теперь проверяет source defaults,
per-type widths/order/visibility и отдельные Type/Name changes с сохранением
selection/peer focus. Normal/compact EN/RU обе панели проверены на macOS;
[evidence](upstream-port-audit.md#source-real-provider-column-workflow-2026-10-07).
Refresh written-count policy теперь совпадает с оригиналом: count HRESULT не
заменяет валидные rows ошибкой, включая Parent/post-Copy/Move. Qt хранит один
numItems между load/publication/selection и задаёт ноль для unwritten output.
EN/RU обе панели, detail/icon, Refresh/timer/recovery и bytes покрыты текущими
consumers; [evidence](upstream-port-audit.md#source-refresh-loaded-count-consumers-2026-10-07).
Enter/double-click теперь используют original OnNotifyActivateItems и все
Ctrl/Shift/Alt сочетания, включая Properties/Outside, в table/icon обеих панелей.
EN/RU real FS/ZIP, guards, Cancel Properties, folder-error/recovery и normal/compact
reports покрывают macOS; native Windows shell sheets/current3OS/rendered остаются
обязательными. [Evidence](upstream-port-audit.md#source-item-activation-notification-consumers-2026-10-07).
Source selected-state notifications также теперь обновляют owning-pane marks
немедленно, включая inactive peer, через original OnItemChanged. Snapshot больше
не перезаписывает marks при guard; deletion/alternative guards, source indices,
Reload/peer/bytes проверены в EN/RU обеих ролях с normal/compact reports.
[Evidence](upstream-port-audit.md#source-selection-notification-and-snapshot-consumers-2026-10-07).
Сверка тел не подтверждает текущую отрисовку или завершение всего модуля.
Остальная full/post-Copy error и
native/platform приёмка остаётся открытой.
Принятый архивный Move/rollback сохраняется. Existing
Copy/Shift checks не закрывают остальные branches; новый text contract обязателен.

Сверка полного PanelFolderChange также выявила обязательный
[P3-05.14](upstream-port-audit.md#p3-0514--persisted-source-location-consumers-required-scope-2026-10-06):
namespace/nested Save ограничения, отказ доступной drive-команды и обработка
сохранённых путей при обнаружении расходились с исходными consumers. Source
virtual prefix, доступность/handler и saved-location binding восстановлены;
старые Qt display entries, история, обе панели и original nested reuse/cold
fallback имеют public coverage. Mac implementation gate и16EN/RU reports
проверены; current native/platform/rendered приёмка остаётся открытой.

Устаревшая `existingTarget` constructor-only ветвь Add удалена: поля path/format/
browse/volumes/encryption больше не блокируются ради отдельного update workflow.
All-target build и12 affected suites PASS9.66s на macOS. Также удалены
самостоятельные требования существующей папки назначения и preflight источников:
исходные scan/UpdateArchive отвечают за ошибки и создание папки после OK.
Регрессия новой папки падает до исправления; последующие проверки ещё идут.
Actual Windows v10 Add просмотрен рядом с pinned original:642×546, без picker,
footer полностью доступен, encryption method same-row и memory right-alignment
подтверждены отрисовкой. Source SFX уже подключён к retained validated module;
обычный/password/split executable и Cancel проходят локально. Новая сборка
ещё проверяется на Windows/Linux. Открыты automatic-method `*` и исходное
очищение списка методов при Store; их нельзя считать допустимыми Qt adaptations.
Artifact/provenance — 7zfm-parity.

Исходный `App.cpp::g_StandardButtons` задаёт Info как `IDM_PROPERTIES`.
`QtFileManagerCommands::AttachFileActions` теперь привязывает её к исходному ID.
ArchiveInformation extension и самостоятельный summary screen удалены, tests
мигрированы к исходной modal Properties. Item/empty-archive/EN→RU→EN/backend
counts и actual filesystem Info во всех view/flat режимах проходят локально;
five direct consumers PASS3.16s. Native Windows System Properties и свежая
платформенная проверка этого последующего изменения ещё обязательны.

2026-10-04: Add selection/header/focus corrections validated separately by
macOS full230/230PASS93.41s and snapshot07e2aeb5 Windows29/29PASS123.49s /
Linux29/29PASS38.34s. Actual default Qt v12 PNG compared with the pinned original:
no source picker, native path display, selected filename, SFX/automatic method
and visible footer. Remaining spacing/control-width/Archive-caption differences
are still open; full visual correspondence is not claimed.

Program File now uses original OnMenuActivating/CreateFileMenu and CApp plugin
invoke/release. Reopening, same-ID stale QAction and panel-switch checks pass,
as do program Open success/refusal/Cancel and program Add success/error/Cancel.
Four EN/RU normal/compact root and child JSON inspected, no clipped rows or
visible aliases. Legacy alias QAction construction/workflows still need removal;
this partial result does not close the File/menu/toolbar/context row.

2026-10-04: Journal и built-in text view/edit QAction, extension IDs и dispatch
удалены. Удалены их независимые накопитель сообщений, FileViewerDialog и
MainWindow preview/extract/update wrappers. F3/F4 используют исходный EditItem;
Progress/Messages продолжают показывать ошибки, Cancel не создаёт второго окна.
Native Open lifetime/Cancel перенесены в archive_item_open; source external edit
success/failure/Cancel coverage сохранена. Текущие проверки ещё идут; это не
закрывает глобальную строку File/menu/toolbar/context.

Каждая строка закрывается только после исправления соответствующих consumers,
регрессий, текущих EN/RU normal/compact reports и сравнения с доступным Windows
runtime. Различия, которые ещё не исследованы, не считаются допустимыми или
исправленными. Счёт принятых пакетов не увеличивается от этой инвентаризации.
