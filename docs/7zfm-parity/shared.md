# Сверка с Windows 7zFM — shared

[Индекс](../7zfm-parity.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

### Rendered Add/Extract reference (2026-10-15)

Original pinned Windows7zG EN/RU initial views are now inspected against current
609bf25 native Qt layouts. Grouping/order/labels are retained; standard Qt widget
metrics/borders/icons and compact scrolling differ. Original desktop dialogs are
fixed-size; their _2 templates are CE-only, and forced resize is not a valid
desktop compact reference. Native Qt normal/compact consumers pass2/2 on each OS;
Mac uses JSON only. This does not claim default-value/pixel parity or full native
input acceptance. [Concrete reference and scope](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--rendered-addextract-and-native-layout-consumers-2026-10-15).

### Rename validation error (2026-10-15)

Filesystem label validation now uses original OnEndLabelEdit's shared Critical
HRESULT message and7-Zip caption, then rejects the edit without a reload. The
independent generic Qt warning was removed from that caller. Archive labels also
delegate to source validation and Agent RenameItem; safe relative paths with
additional parent levels use its original roots through the CLI adapter.
Backend path safety still refuses parent/absolute archive names. Original-name,
selection and peer-state restoration remain. Cocoa native message boxes retain
the existing platform caption omission; native pixels remain unverified.
[Source/fixture receipt](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-05--source-rename-validation-and-cocoa-modal-fixtures-2026-10-15).

### Parent keyboard focus (2026-10-15)

Original `CPanel::OpenParentFolder` guards notifications/timer processing while
refreshing rows; it leaves the Win32 list enabled and focused. The Qt caller
temporarily disables views during publication, so it must restore the previous
keyboard owner. List focus uses the existing publication/input guard and yields
to newer user interaction; address focus retains its actual Qt proxy without
activating a window. No source navigation/provider algorithm changes.
The old Linux Release package needed a list click between Parent and opening a
physical bookmark; a settling delay alone did not help. Automated current-code
focus checks are separate from physical input. Rebuilt Linux Release2a20be0
now passes physical XTest Save/Open0–9 with Parent before each Open and no extra
list click; bytes are preserved and normal exit succeeds. Other OS physical
and whole native acceptance remain open.

### Windows physical bookmark zero (2026-10-15)

On the current runner desktop, original pinned7zFM also cannot save bookmark0
through physical RightCtrl+Shift+0. RightCtrl+Shift+1 and Alt+Shift+0 positive
controls save the expected path in that same focused native list. The Qt port
has the same unavailable chord, with no digit0 WM_KEYDOWN in its earlier trace;
native-notification dispatch0–9 passes separately. This records observable
reference parity on that desktop, without claiming the OS interception cause,
a working physical zero chord or N/A. System shortcuts/preferences are unchanged.
[Original comparison](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--original-windows-physical-bookmark-zero-comparison-2026-10-15).

### Native Add control geometry and traversal (2026-10-15)

The source two-column CompressDialog keeps its eight-dialog-unit outer margin
and common compression combo widths under Cocoa as well as Windows/Linux.
Qt layout-item extents no longer clip complete combo/group frames; explicit
button StrongFocus retains the original Tab stops without changing macOS system
preferences. Compact lower controls remain vertically scrollable. Native file
pickers still use platform dialogs; their unresolved Cocoa automation is not a
product workflow change. Current geometry checks do not claim rendered pixel
parity. [Source and validation receipt](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--native-add-geometry-tab-traversal-and-display-fixtures-2026-10-15).


The shared Qt transient-message status bar now disables its default resize grip,
matching the existing per-panel bars and original Panel.cpp::OnCreate status
style without SBARS_SIZEGRIP. Window-border resizing and message delivery remain
available. Current EN/RU normal/compact one/two/restored-pane geometry checks
cover this structure; rendered/native acceptance remains separate.
Creating a Qt panel also leaves the shared transient message intact. The former
unconditional Russian ready caption overwrote diagnostics on the first switch
to two panels; source CApp::CreateOnePanel/CPanel::OnCreate has no such message.

Linux codec metadata uses the existing immutable Qt temporary image, unlike
the original native in-place module loading. The selected temporary directory
must allow writes and loading shared libraries. With `noexec /tmp`, a private
process-scoped `TMPDIR` retains successful archive Test and automatic cleanup;
default noexec refusal is recorded in the
[isolated runtime receipt](../upstream-port-audit.md#p3-04--isolated-linux-runtime-and-temporary-directory-contract-2026-10-10).

PanelItems selection restoration now passes source SelectFocused unchanged in
both selection modes. LVS_SINGLESEL limits selected rows; native ListView does
not select a focused row on its own. Alternative marks stay independent: an
unselected focused item does not become operated after Refresh, and original
focused-size/date status stays empty when no row is selected. A providerless
Qt model cannot supply source item count/marks; real snapshot providers retain
attachment callbacks across load/rebind. This source-state correction leaves
the Qt single-selection control adaptation in place. Native control semantics
and public UI consumers are distinct from rendered Windows/physical input
acceptance; see the [receipt](../upstream-port-audit.md#p3-05--panelitems-selection-and-provider-lifecycle-2026-10-10).

Windows drag now uses the original native IDataObject reverse protocol and
IDropSource/DoDragDrop on the windows Qt platform. Qt supplies incoming native
MIME access, widget hit testing and popup presentation; headless and POSIX paths
keep their Qt transport. Source right-or-middle menu admission, source button
flags, temporary-source root and wait precedence are restored. The middle menu
has original Copy/Move/Add/Cancel order. Shared source/temp ownership prevents
late callbacks into closed panels; explicit target WasCanceled retains the
existing Qt selection-on-Cancel fix. A reusable Qt platform MIME may retain the
last native owner until replacement/shutdown. Native COM/archive and actual Qt
platform checks are recorded in the
[protocol receipt](../upstream-port-audit.md#p3-05--original-native-paneldrag-protocol-and-qt-consumers-2026-10-10),
including an independent Qt receiver process exchanging the native formats
through COM proxy.
They do not replace physical pointer/Explorer acceptance or rendered Windows
comparison.

Default drag effect now follows source folder-kind/root admission: AltStreams
and empty/relative FS paths prefer Copy; explicit Control/Shift remain source
policy. Windows uses the original all-source case-insensitive drive/UNC prefix
comparison. POSIX uses actual mounted-volume identity after those same guards.
Qt header mutation persistence, popup-scoped COM cleanup and guarded status
delivery replace native child-window lifecycle/messages; their exact owners
are in the [Panel core map](../omitted-scope-audit.md#panel-control-and-view-state-disposition-2026-10-10).
Three-OS consumer checks and existing status elision are recorded in the
[receipt](../upstream-port-audit.md#p3-05--source-default-drop-effect-and-panel-core-disposition-2026-10-10).
Physical/OLE receiver and rendered acceptance remain separate.

Panel address keys and view state now follow original Panel.cpp: Control-W
(also with Shift) queues close from the address edit, Tab/Backtab returns to its
list and F9 retains the address draft while changing pane visibility. Source
ChangeFlatMode changes Arc/Disk flags by parentFolders and owns refresh; a flat
toggle in Computer/Drives survives View activation and entry into a filesystem
folder. Qt replaces native styles/messages with existing control projection,
popup focus and joined cancellation of the prior folder loader. Normal/compact
and platform evidence belongs to the [receipt](../upstream-port-audit.md#p3-05--panel-address-keys-and-source-view-state-2026-10-10);
physical input and rendered Windows parity remain separate acceptance work.
Untested native Alt/system-key modifier combinations retain that separate
platform acceptance requirement. Compact Windows status parts retain existing
QtStatusLabel elision and full-text tooltip.

## FM platform dispatch and toolbar text (P3-05, 2026-10-10)

Toolbar сохраняет исходные короткие подписи и tooltip из App.cpp::SetButtonText
при смене языка и доступности QAction. Caption общей QAction в меню остаётся
меню-подписью; Qt владеет временем жизни tooltip вместо статических Win32 buffers.
Исходный GetRamSize вызывается один раз первым шагом production startup;
PanelItemOpen использует общий неизменяемый результат FM. Исходные threshold,
fallback и process lifetime восстановлены; существующая защита excessive shift
сохранена. Прямые source-panel consumers без entry могут лениво создать тот же
process cache. Файловый транспорт вложенных архивов по-прежнему materializes
stream на диск; конечный IsVirtual не доказывает выбор RAM-ветки.
Qt Windows platform context владеет OLE для QApplication; offscreen не доказывает
нативный COM/drag transport. Все ветви FM.cpp сопоставлены с владельцами в
[disposition](../omitted-scope-audit.md#fm-desktop-bodies-and-platform-dispatch-2026-10-10);
native/rendered/platform/package и итоговая приёмка остаются отдельными.

## Source View-menu activation (P3-05, 2026-10-10)

Родительское меню «Вид» при открытии выполняет исходный OnMenuActivating:
показывает текущую дату, пять примеров точности времени и UTC, состояние вида,
сортировки, двух панелей, плоского вида, toolbar и автообновления. Сортировка
по другим свойствам использует исходный fallback «Имя». До исправления дата
и примеры заполнялись только при открытии дочернего меню времени. Qt сохраняет
объекты QAction и их команды, блокируя сигналы при отражении состояния;
пересоздание HMENU заменено обновлением существующих действий.

`view_menu` проверяет обе панели, EN/RU, команды точности/UTC и сохранение,
публичное выделение после сортировки CTime, normal/compact geometry главного
окна и двух popup. `panel_columns` активирует тот же родительский обработчик.
Ресурсы и widget JSON устанавливают структуру Qt; rendered Windows parity и
интерактивная desktop-приёмка остаются отдельными обязательными проверками.
[Source receipt](../upstream-port-audit.md#p3-05--source-view-menu-activation-2026-10-10).

## Source startup platform defaults (P3-05, 2026-10-10)

При исходном условии малого экрана подписи toolbar по умолчанию отключаются;
сохранённые labels/icons имеют приоритет. Исходный расчёт200×200 dialog units
использует Qt font metrics и available work area вместо Win32 system-font/
work-area APIs. Layout и порядок toolbar сохранены. Искусственно крупный шрифт
проверяет классификацию/состояние, без rendered parity. Windows symbolic-link
поддержка и process token privileges инициализируются при startup до Copy и
первого Link dialog. Недоступная привилегия сохраняет отказ операции; права
account не выдаются. [Receipt](../upstream-port-audit.md#p3-05--startup-platform-consumers-and-toolbar-defaults-2026-10-10).

## Source Windows namespace icons (P3-05, 2026-10-10)

Computer/Documents/Network снова получают исходные Windows CSIDL image-list
indices. Список корневой папки и строки раскрытого адреса используют эти
системные изображения через существующий QIcon-мост; закрытый адрес Computer/
Network использует тот же источник. Прежний безусловный−1 заменял их общим
значком или QStyle. На POSIX остаётся платформенный сервис значков. Порядок,
геометрия, translated names и navigation policy сохраняются. QIcon также
содержит оба исходных размера small/large из системных image lists. Проверка
native данных и QIcon не устанавливает rendered/HiDPI parity всего Windows 7zFM;
[source/validation receipt](../upstream-port-audit.md#p3-05--filemanager-services-and-native-namespace-icons-2026-10-10).

## Large-page startup and Settings policy (P3-05, 2026-10-10)

Windows startup again follows source SetMemoryLock: attempt the account right,
then enable the process privilege only for the saved option and risk level zero.
Settings Apply immediately enables/disables the current process privilege and
saves the requested option even if token adjustment fails; Cancel leaves it
unchanged. Original source behavior keeps the startup diagnostic flag separate
from later Settings changes. Backend uses the original GUI launch risk check
before adding `-slp`; the engine keeps its own parser checks. POSIX keeps this
Windows-only option disabled. No control layout or wording changed.
This establishes the source policy, not successful large-page allocation on
every machine or rendered Windows parity. [Receipt](../upstream-port-audit.md#p3-05--startup-settings-and-large-page-engine-policy-2026-10-10).

## Source native casing before numeric sort (P3-05, 2026-10-10)

Сортировка имён снова использует исходный MyCharUpper и его платформенный
сервис регистра. Для подтверждённой пары U+1F80/U+1F88 одинаковый буквенный
префикс позволяет сравнить file2 перед file10, как в Windows CharUpperW;
прежний QChar оставлял строчную форму отдельной группой. Алгоритм числовых
серий и Windows UTF-16 единицы сохранены; Agent, filesystem и fallback панели
используют один компаратор. Это проверка поведения, без изменения layout или
утверждения о rendered Windows parity. [Source receipt](../upstream-port-audit.md#p3-05--core-copycrc-sort-and-results-reconciliation-2026-10-10).

## Temporary browser source keyboard (P3-05, 2026-10-10)

Delete Temporary Files снова обрабатывает исходные Ctrl+R и Ctrl+F3/F5/F6:
обновление и сортировку по имени/времени/размеру с переключением направления
и сохранением выделения. Исходный OnKeyDown также владеет Backspace,
Ctrl+A и Delete. Qt передаёт ему клавиши списка; Enter остаётся активацией,
обычный F5 — ранее документированным Qt Refresh. Порядок и геометрия контролов
не меняются. Исходный wait cursor включается при подсчёте свыше1000 объектов
и восстанавливает предыдущий курсор после обновления. Реестр owned-temp,
защита active/external, отказ от открытия
ссылок и исходные подтверждение/отмена/первая ошибка удаления сохранены.
Полная disposition и ограничения: [source receipt](../upstream-port-audit.md#p3-05--temporary-browser-source-lifecycle-reconciliation-2026-10-10).

## Source long-path browser (P3-05, 2026-10-10)

Обычный выбор файла/папки остаётся системным QFileDialog. Для длинного
начального пути сохранён исходный CBrowseDialog с Parent/Create Folder,
Name/Modified/Size, строкой пути, фильтром и OK/Cancel. Windows super/device
prefix тоже выбирает его. Source sorting удерживает parent/каталоги перед
файлами; Backspace, Ctrl+R, F7 и Enter вызывают исходные обработчики.
Cancel сохраняет FilePath/filter, выбор папки очищает выход до вызова,
а raw input разрешает исходный GetFullPath без trim.

Отдельный исходный CommonDialog owner обслуживает обычный picker. Как и
исходные OFN flags, он допускает новое имя в Open и не добавляет overwrite
prompt в Save: дальнейший Open/Update остаётся ответственностью caller.
Cancel не публикует путь/filter, cwd процесса сохраняется.

Qt не возвращает CommDlgExtendedError, поэтому длинный file path выбирает
fallback заранее на reference Win32 границе 260 вместо native-error retry.
На POSIX браузер использует исходный enumerator, включая переход по directory
links; Win32 drive/super branches сохранены. Qt layouts, platform icons,
accessible button names, полный путь в подсказке и локализованные стандартные
кнопки заменяют presentation/services. Структура проверена по точным
BrowseDialog.rc/Res; EN/RU geometry — 560×640 и 420×400. Rendered Windows
reference и native desktop acceptance этим не установлены.

## Source startup placement (P3-05.16, 2026-10-10)

Windows STARTUPINFO снова передаёт исходный show command в InitInstance.
Normal/Show/Default учитывают сохранённый maximized flag; явные Maximize/
Minimize/Restore/no-activate имеют исходный приоритет. Hide оставляет окно
скрытым. Qt заменяет native placement и no-activation через QWidget; настоящий
focus/taskbar/rendered parity остаётся непроверенным. Закрытие никогда не
показанного окна завершает цикл, как исходный WM_DESTROY/PostQuitMessage.
Qt сохраняет видимые association окна и явную политику автозавершения.
[Receipt](../upstream-port-audit.md#p3-05--launch-show-command-and-hidden-window-close-2026-10-10).

CSplitterPos сохраняет исходный fixed-point ratio, границы и округление при
resize, F9 и сохранении. Ручка имеет исходную ширину4; Qt управляет захватом мыши
и размещением дочерних widgets. Минимальная ширина содержимого Qt может превышать
исходные120, чтобы адрес и список оставались видны. Rendered reference и итоговая
native приёмка остаются открыты.

Сохранённый normal rectangle и maximized flag теперь применяет исходный
FM.cpp::InitInstance. Проверяются закрытие и повторный запуск, одна/две панели,
фокус и разделитель. Qt-конструктор остаётся скрытым до show вызывающего кода;
INI хранит логические координаты клиентской области Qt. Это необходимая
адаптация Win32 frame/workspace placement, без импорта системного registry.
Все13 window-placement cases и8 geometry JSON проходят на каждой из3ОС.
Два Windows external-process startup failure воспроизводятся и на прежнем коде.
Последующая [диагностика service desktop](../upstream-port-audit.md#p3-03--windows-service-desktop-visibility-diagnosis-2026-10-10)
показывает тот же false от IsWindowVisible у отдельного Win32 окна с WS_VISIBLE:
desktop служебного сеанса невидим. Проверки на активном desktop, native rendered
и итоговая приёмка остаются открыты; assertions и deadlines сохранены.
[Receipt](../upstream-port-audit.md#p3-0516--source-initinstance-window-placement-2026-10-10).

## Source Copy history (P3-05, 2026-10-10)

История Copy/Move снова использует исходные UStringVector и IsEqualTo_NoCase,
порядок удаления совпадений и добавления в начало; предел20 сохраняется в CApp.
Qt case folding удалял отдельную исходную строку K при добавлении K. Теперь обе
строки сохраняются после записи, Cancel и повторного открытия source Copy
dialog. В QStringList значения переходят только на границе Qt-представления.
После закрытия popup Qt пересчитывает доступность общих QAction: временный
запрет уведомлений панели больше не должен оставлять «Настройки» отключёнными.
Проверки и ограничение native Windows session0:
[receipt](../upstream-port-audit.md#p3-05--viewsettings-and-source-copy-history-2026-10-10).

## Source Comment publication (P3-05, 2026-10-10)

Исходный void CPanel::ChangeComment снова владеет сохранением выбора, ошибкой
Set Comment Error и RefreshListCtrl внутри защиты уведомлений. Qt сохраняет
joined worker и тихую отмену; существующий source callback выполняет загрузку
и публикацию. Cancel не обновляет список; отказ загрузки не восстанавливает
старые строки. Общий source HRESULT helper сохраняет видимый заголовок macOS.
Текущие проверки: [receipt](../upstream-port-audit.md#p3-05--paneloperations-disposition-and-source-comment-lifecycle-2026-10-10).

## Password source state and process caller (P3-05, 2026-10-10)

PasswordDialog follows the original UString state and raw read/toggle/accept
flow. An accepted empty asynchronous process password no longer cancels the
operation; rejection still cancels. Source opening already accepted empty.
Normal/compact widget reports cover Unicode/spaces/empty and redact the value;
source order, labels and geometry are unchanged. See the
[validation receipt](../upstream-port-audit.md#p3-05--password-state-and-process-callers-2026-10-10).
Qt widget/runtime evidence does not establish rendered Windows 7zFM parity.

## Source panel context menu (P3-05, 2026-10-10)

Table, icon и header передают событие своему CPanel::OnContextMenu. Исходный
owner сохраняет точный keyboard sentinel, empty origin, focus/refusal, Cancel
и plugin/file dispatch. Закрытие Qt popup больше не направляет команду соседней
панели через восстановление её фокуса; общий QAction не выполняет её дважды.
Qt использует центр visual item и собственное ограничение popup экраном вместо
Win32 LVIR_ICON/TrackPopupMenu. Структура и порядок исходные; rendered Windows
pixel parity не проверена. Provenance и актуальные checks — в
[receipt](../upstream-port-audit.md#p3-05--source-panel-context-menu-lifecycle-2026-10-10).

2026-10-15: реальный System submenu pinned Windows 7zFM сопоставлен с текущим
Qt bridge:20 entries, text/order/states/icons совпадают, обе отрисовки просмотрены
при96DPI/Segoe UI. QWindows11Style сохраняет собственные интервалы строк и
разделителей и более узкую ширину popup; pixel equality не заявлена.
[Runtime receipt](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--current-rendered-windows-system-menu-2026-10-15).

## Options: ошибки Apply (P3-05, 2026-10-10)

Ошибка ассоциации на странице System показывается один раз, затем остальные
страницы сохраняются в исходном порядке. Она не блокирует OK и не переключает
вкладку. Невалидный лимит RAM сохраняет исходный PSNRET_INVALID: Options остаётся
открытым на Settings с введённым значением. Ошибка RAM использует исходный
MyMessages, включая подавление вопросов, Critical и заголовок «7-Zip».
Qt на macOS скрывает заголовок QMessageBox; размеры и порядок кнопок задаёт ОС.
Source/disposition/checks — в
[receipt](../upstream-port-audit.md#p3-05--six-options-pages-and-apply-errors-2026-10-10).

## Source selection commands (P3-05, 2026-10-10)

Select/Deselect по маске проходит полный CPanel::SelectSpec: исходные заголовки,
prompt, начальная `*`, отмена и применение маски. Qt отображает общий Combo;
MainWindow больше не содержит отдельного сценария диалога. Правила wildcard,
выделения по типу, All/Invert и alternative marks остаются исходными.
Проверены владельцы обеих панелей и неизменность файлов; source scope и проверки
описаны в [receipt](../upstream-port-audit.md#p3-05--source-selection-command-lifecycle-2026-10-10).
Rendered Windows parity остаётся отдельной проверкой.

## Source Rename activation (P3-05, 2026-10-10)

F2/Rename редактирует строку с фокусом, независимо от количества выбранных
элементов и alternative marks. Исходные RenameFile/OnBeginLabelEdit выбирают
цель и запрещают parent/read-only edit; Qt отображает редактор table/icon
в соответствующей панели. Escape оставляет имена и байты без изменений.
При Rename symbolic link меняется имя самой ссылки; её target и байты
сохраняются. Проверки включают два native filesystem случая.
Операция, ошибки и восстановление выбора остаются в прежнем исходном
OnEndLabelEdit и его reload consumer. Проверки и provenance — в
[receipt](../upstream-port-audit.md#p3-05--source-rename-activation-scenario-2026-10-10).
Эти проверки не устанавливают rendered Windows parity.

## Source Delete lifecycle (P3-05, 2026-10-10)

Delete сохраняет исходный ранний выход после No/Cancel. Yes является кнопкой
по умолчанию, как в исходном MB_YESNOCANCEL; Enter подтверждает удаление.
Явные No/Cancel сохраняют файлы и прежний список. После принятой операции
CPanel обновляет bound provider и восстанавливает selection даже при ошибке
или отмене worker. Qt больше не выбирает отдельную политику refresh по типу
папки и не добавляет второе общее сообщение после исходной ошибки progress.
Отложенный scan принимает восстановленные source guards после возврата команды.
Существующий Qt Trash adapter использует исходное подтверждение и captioned
HRESULT при ошибке, затем обновляет частичный результат. Native desktop Trash
и rendered Windows reference остаются отдельной проверкой. Archive Move
с explicit indices сохраняет отложенную публикацию до завершения rollback.
Provenance и границы проверки — в [source receipt](../upstream-port-audit.md#p3-05--source-delete-lifecycle-2026-10-10).

## Source Create File/Folder (P3-05, 2026-10-10)

Обе команды проходят исходные CPanel::CreateFile/CreateFolder: guards, диалог,
валидация, операция и восстановление состояния. CreateFile использует исходные
language IDs вместо постоянных русских строк. После вложенного имени фокус
переходит на первый компонент пути; alternative selection сохраняет прежние
marks независимо от единственного Qt focus selection. После ошибки CreateFolder
обновляет список, CreateFile возвращается до refresh, как upstream. Ошибки,
отмена и существующие байты проверяются через реальные filesystem consumers.
Для отложенного чтения большого каталога Qt scan принимает восстановленное
состояние исходных guards после выхода из синхронного Create. До завершения
scan callbacks отключены, затем текст и команды снова доступны.
Qt dialog construction/exec и native message boxes остаются presentation
адаптациями; macOS не показывает window caption у progress message boxes.
Captioned CreateFile error сохраняет caption как heading на macOS.
Normal/compact JSON не доказывает rendered Windows parity. Provenance и
проверки — в [source receipt](../upstream-port-audit.md#p3-05--source-create-filefolder-scenario-2026-10-10).

Этот документ описывает сценарии и отличия от Windows 7zFM. Разделы по ID
и датам сохраняют границы проверенного поведения; их порядок не определяет
текущий статус. Для выбора работы использовать [чеклист](../porting-checklist.md),
для provenance — [source audit](../upstream-port-audit.md), для открытых UX-коррекций —
[UX audit](../ux-port-audit.md). Нужный сценарий искать по команде, символу или ID,
затем читать соответствующий раздел и ссылки.

### Source Split/Combine (P3-05, 2026-10-08)

Команды снова выбирают каталог второй файловой панели по CApp::Split/Combine;
при одной панели остаётся каталог выбранного файла, включая flat-prefix.
Размеры сохраняют UInt64 и исходный UString::Trim (space/LF/tab): ведущие NBSP
и CR вызывают ошибку с возможностью исправления. Combine берёт непрерывный
префикс до первого отсутствующего файла или каталога, допускает пустые части
при ненулевой общей длине и не включает файлы после пропуска. Самостоятельный
лимит 10000 частей снят; подтверждение от 100 частей остаётся исходным.
2026-10-10: неверный размер снова вызывает исходный модальный Critical/7-Zip/OK;
прежняя встроенная строка ошибки удалена. OK/Escape закрывает только сообщение,
сырой ввод сохраняется для retry. Qt определяет размер message box и порядок
кнопок; macOS игнорирует его заголовок. [Карта диалогов](../omitted-scope-audit.md#primitive-input-and-read-only-dialog-bodies-2026-10-10).
Исходные NIO/preallocation и numbering сохранены; staging, запрет перезаписи,
проверки изменения входов и откат при отмене остаются адаптациями интеграции.
На Windows исходный NIO открывает активный источник с FILE_SHARE_READ: запись
в него отклоняется ERROR_SHARING_VIOLATION. POSIX допускает изменение; финальная
проверка размера откатывает вывод. Изменение уже прочитанной и закрытой части
Combine также вызывает откат на всех ОС.
Inline validation, Qt native browse и недоступная Pause не меняются. Geometry
JSON подтверждает Qt layout; rendered Windows visual parity этим не установлена.
Provenance/checks — в [audit](../upstream-port-audit.md#p3-05--complete-source-splitcombine-scenario-2026-10-08).

### Source filesystem sorting (P3-05, 2026-10-08)

Файловая панель теперь использует исходный CFSFolder::CompareItems. Комментарии
сравниваются вместе со скрытым суффиксом wchar4, поэтому одинаковый видимый
текст может иметь разный порядок. Comparator читает исходный кеш без загрузки
комментариев или packed size; после reload кеш комментариев обновляет запрос
отображаемого свойства. Неподдерживаемое свойство даёт исходный переход к
сортировке по имени. UI layout и тексты не изменены. Точные адаптации, regression
и границы приёмки — в [source audit](../upstream-port-audit.md#p3-05--source-filesystem-comparator-2026-10-08).

### Source GUI path/history consumers (P3-05, 2026-10-07)

Create/Extract теперь сохраняют исходные правила UString::Trim: убирают
краевые пробелы, LF и tab, сохраняя NBSP в имени архива, папки и истории.
CompressDialog использует исходное разрешение полного пути и отказ при
неразрешимом переходе выше корня; Extract добавляет системный разделитель.
Общая история обоих диалогов использует исходное IsEqualTo_NoCase, включая
его отличие от Qt case folding для K/Kelvin. Embedded NUL в Create по-прежнему
отклоняется на границе Qt-текста. Новые controls или тексты не добавлены.

Четыре EN/RU × SplitDest сценария проходят Create/list/Extract через официальный
движок с Unicode/NBSP, точными бинарными байтами, сохранённой историей и Escape
без записи настроек в обоих диалогах. JSON проверяет видимость и исходную
группировку при normal/compact размерах; Windows использует native Qt fonts/style.
Это не rendered reference оригинального Windows 7zFM и не приёмка физического
desktop-ввода. [Source scope и проверки](../upstream-port-audit.md#p3-05--source-gui-path-and-history-consumers-2026-10-07).

### Source Agent provider lifecycle (P3-05, 2026-10-07)

Directory CRC now reaches the list and selected Properties through the original
path-proxy property branch. It uses the source additive UInt32 aggregate, so it
is not a checksum of concatenated directory bytes. Unknown file CRC can leave
the directory value undefined. Explicit-record failures retain source Error
text in the list and omit only the failed item property; the separate folder
summary can still show its cached aggregate. Original written-value/local
variant policy is preserved. This changes property data without adding controls.
See the [property projection audit](../upstream-port-audit.md#p3-05--source-archive-directory-property-projection-2026-10-07).

Closed archive folders refuse loading before reading their proxies, including
after a failed ReOpen. Successful repair binds a fresh root and child provider;
callers must discard the old folders. Cancellation during initial proxy binding
publishes no folder and permits a retry through the same manager. Descriptor
and load failures release partial proxies for that retry. These source lifecycle
rules introduce no dialog or layout change. Exact adaptations and platform
coverage are in the [lifecycle audit](../upstream-port-audit.md#p3-05--source-agent-provider-lifecycle-and-metadata-binding-2026-10-07).

### Source Agent opening/factory (P3-05, 2026-10-07)

Archive-folder opening now reaches the original Agent/factory workflow.
An unknown forced format returns source S_FALSE before listing; a recognized
malformed archive can return a diagnostic folder with the same result, so the
existing error UI can read NonOpen_ErrorInfo. Root binding receives the caller's
progress/Cancel interfaces. The returned owner retains streams and the exact
codec catalog after the manager is released or the backend catalog is replaced.
File-only transport still spools nested input and services process work through
ArchiveBackend. No dialog text, layout or visibility changes are introduced.
Current platform acceptance is recorded in the
[source audit](../upstream-port-audit.md#p3-05--source-agent-opening-and-archive-folder-factory-2026-10-07);
native GUI/rendered and whole-plan acceptance remain open.

### Native zone Copy and bound filesystem publication (P3-05.15, 2026-10-07)

F5/F6 retain the source registry-zone default: None preserves the stored
Zone.Identifier, All applies the parent-container zone, and Office applies it
to Office names. Named alternate streams remain source engine output. The
separate Extract dialog owns its explicit metadata options. Copy/Move refreshes
the existing filesystem provider, including normal/flat worker loading; its
native notification owner remains on the GUI thread. Both panes may finish
their own scans before the original saved selection and source focus continuation.
Root ADS initialization failure again returns the original E_INVALIDARG.
The dated port audit owns bounded platform/geometry evidence. Rendered Windows
and whole-plan acceptance remain separate.

### Native bound archive Drop (P3-05.15, 2026-10-07)

CopyFromNoAsk retains success-only Refresh and source list focus. Actual writer
failure/Cancel and confirmation No/Cancel leave the list/provider and archive
bytes intact. A later native attribute publication failure does not reverse a
successful write: the list becomes empty/enabled and Ctrl+R reloads the same
provider. Qt publishes the reached source phase after synchronous guards unwind
and completes source list focus after asynchronous rows settle. Original native
writer-failure diagnostics remain in source progress and CopyFromNoAsk; no extra
Qt warning or physical reopen is added. Eight representative2/301-row cases
cover both panes/languages/views/selection modes, actual retry and subsequent
filesystem OnTimer. The bounded native component and current normal/compact
geometry pass on macOS, Linux and Windows; evidence is in the dated audit.
Rendered Windows and whole-plan acceptance remain separate.

### Native interarchive rollback file lifetime (P3-03, 2026-10-07)

The existing Copy/Delete/backup extension closes the destination's source
archive streams before restoring backup bytes, then uses the existing physical
reopen and saved selection. This permits Windows replacement under native
reader sharing. Source deletion refusal/error/Cancel still retains source bytes;
failed reopen still returns to the destination's filesystem folder. The narrow
Close bridge reuses CAgent::Close; source copy and deletion algorithms are unchanged.
The native Windows two-case RED and subsequent bounded three-OS verification
are tracked in the dated port audit; this is not whole-platform acceptance.

### Changed nested archive Parent (P3-05.15, 2026-10-07)

For disk-extracted native children, Parent retains the original Yes/No/Cancel
policy. No and Cancel both
discard the changed child and leave the outer archive unchanged. Yes updates
the outer item; writer failure/Cancel retains the modified spool and reports
the source update error, then still returns to the parent. Its final Refresh
result is ignored. Empty/enabled failed publication recovers on Ctrl+R using
the same outer provider. Real native writer retry preserves the old recovery
copy while deleting the new successful spool. Actual F2/native Rename creates
the changed child and retry. Five representative2/301-row cases cover both
panes, languages, views and selection modes, child focus, archive/peer bytes
and later filesystem OnTimer. Geometry reports do not prove rendered parity.
Qt callback publication now keeps a non-owning provider identity, so original
ReleaseFolder closes native archive streams before Parent deletes its spool.
The prior extra COM owner caused actual Windows cleanup failures. Source
algorithms and cleanup order remain unchanged. The five Windows cleanup failures
now pass, as do the affected native consumers on macOS and Linux; detailed
revision, geometry and gate evidence is in the dated port audit.

### Native Rename error ownership (P3-05.15, 2026-10-07)

Native F2 Rename now keeps the source progress dialog as the sole operation-error
owner; Qt no longer adds a generic second warning after the worker. Early invalid
input still rejects the editor with its validation message. Original new/old
focus, saved marks and unconditional posted reload after success/error/Cancel
are preserved. Reload publication errors remain ignored; explicit Ctrl+R recovers
the same provider. Real native writer retry, archive/peer bytes and guard release
are covered at2/301 rows in both panes/languages/views/selection modes. Normal/
compact reports establish control geometry only; native Windows/Linux and rendered
Windows reference acceptance remain open.
Qt's periodic two-panel traversal now honors source notification/status guards
before activation and status reads, preserving native WM_TIMER's guarded access
during archive proxy reopen. This fixes an actual native Rename crash.

### Native external editor update (P3-05.15, 2026-10-07)

The native editor-completion consumer preserves original OnOpenItemChanged's
unconditional ignored-result Refresh after writer success/failure/Cancel.
An ignored native property publication error does not turn a successful update
into an external-update failure: the actual payload changes and its editor copy
is deleted. Writer failure/Cancel preserve archive bytes and the edited copy,
then actual F4 retry succeeds. Source marks, owning provider, peer focus and later
OnTimer are checked in both panes/languages/views/selection modes with2/301 rows.
Current evidence uses macOS; native Windows/Linux and rendered reference remain
mandatory. This iteration adds consumer verification without changing production
algorithms, layout or control flow.

### Native nested opening (P3-05.15, 2026-10-07)

Original Agent::GetStream index mapping and OpenItemInArchive's preferred stream,
bounded memory and disk paths are retained. Qt adds only the native module/input
lease and timer-guard handoff needed by deferred row publication. All three
success branches ignore RefreshListCtrl results as upstream does. Actual native
Cancel Yes retains the outer folder and removes owned temporaries; No/Cancel
continue opening, followed by normal Parent cleanup. TAR explicit directory
records may provide empty streams; implicit non-leaf directories keep the
original no-stream policy. The resource proves structure and control flow;
macOS tests do not establish rendered Windows or three-platform acceptance.

## Аудит диалогов (2026-09-28)

Сверка выполнена с upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
Qt на macOS, checkout до изменений `66ecb5e78bede4a9573918afb1af95f9439db4f7`.
Прочитаны AGENTS.md, профильные skills, ресурсы и обработчики. Проектный
`check_ui_parity.py extract` извлёк Copy, Combo, Split, ListView, Edit, Extract
и Compress вместе с shared include/хешами в `build/dev/ui-reports/audit-*.reference.json`.
Compress extraction также содержит IDD_COMPRESS_OPTIONS; этот дополнительный
диалог пока не перенесён. Перед исправлениями все 28 выбранных CTest прошли:
успех локальных тестов не означает полного соответствия upstream.

| Диалог / эталон | Проверенный результат и оставшееся расхождение |
| --- | --- |
| Copy/Move, CopyDialog.rc/.cpp | Ресурсный контракт проходит: шесть controls, Copy/Move, 440×250 и 340×220. Адаптации: scrolling summary вместо static, Qt footer и Unicode ellipsis. |
| Имя/маска/создание папки, ComboDialog.rc/.cpp | Prompt → editable combo → OK/Cancel; начальное Value, raw OK, Cancel и геометрия 360×150/260×150 проверены. Пока нет отдельного ресурсного контракта. |
| Split/Combine, SplitDialog.rc/.cpp и PanelSplitFile.cpp | Destination/Browse → volume size → footer; error/normal и 2/3/5 частей, normal/compact проверены. Inline error заменяет MessageBox, папка выбирается native Qt dialog. |
| История/свойства, ListViewDialog.rc/.cpp | Один/два столбца, header, footer, Copy/Delete/Enter/detail и 540×360/360×240 проверены. Перенесена settings-driven one-click/hover ветка через Qt adapter; набор свойств ограничен snapshot. |
| Полное значение, EditDialog.rc/.cpp | Read-only multiline, без wrap, обе прокрутки, Enter в edit, Close; 480×320/320×240 проверены. |
| Extract, GUI/ExtractDialog.rc/.cpp | Destination, optional folder name, modes слева и password справа; name shown/hidden, 640×360/470×330, история/сохранение/отмена проверены. Ask открывает исходный OverwriteDialog с Yes/No/All/AutoRename/Cancel. Целый root, включая вложенный или зашифрованный архив, проходит source `ExtractGUI`/`Extract`; выбранные элементы и текущая папка — source FM callback. `7zz` поставляет байты через узкий адаптер. Явная запись каталога в «Без путей» остаётся пустой папкой по исходному callback. Отсутствуют абсолютные пути, ElimDup, NtSecurity и Help. |
| Create/Add, GUI/CompressDialog.rc/.cpp | Исправлены stacked layout и порядок основной формы: compression слева, encryption справа, source picker после archive/options. 760×720/640×480, 7z/ZIP, show/hide confirmation и reachability проверены. Это частичный порт: нет update/path modes, volumes, SFX, shared/delete options, parameters, CompressOptionsDialog и Help; Add не меняет compression options. |
| Запрос пароля, PasswordDialog.rc/.cpp | QInputDialog заменён source port CPasswordDialog: восстановлен Show Password и исходный ReadControls/SetTextSpec/OnOK. Контракт проверяет все пять controls в masked/shown при 360×170/260×170; Qt integration проверяет reuse/reset/cancel. |
| Настройки, OptionsDialog.cpp, SettingsPage2.rc и EditPage2.rc | CSettingsPage показывает семь list flags в исходном порядке, с AlternativeSelection из P0-07.3. Standalone 340×300/460×300 JSON сохранён; partial property sheet с Editor/Settings и общим OK/Cancel/Apply проверен при 640×390/520×350. Viewer/Editor/Diff сохраняют WasChanged/Browse flow. Memory controls и четыре недоступные страницы остаются обязательным scope; rendered Windows parity unverified. |
| Прогресс/сообщения, ProgressDialog2.rc/2a.rc/.cpp, MessagesDialog.rc/.cpp | Текущие архивные, файловые и томовые операции показывают исходный ProgressDialog со статистикой, ошибками, подтверждением отмены и Background. Pause реально управляет 7zz; для файловых/томовых workers отключена. Source MB_YESNOCANCEL и success MB_OK сохранены без значка, ошибка MB_ICONERROR — Critical; стандартный Qt message box сам подбирает размер, macOS игнорирует его заголовок. MessagesDialog показывает исходные две колонки и Close, а Create сохраняет модальное время жизни stack-owned сообщений PanelDrag. Close/Enter возвращают IDOK, Escape — IDCANCEL. Независимый File-menu journal удалён в P2-09. Геометрия 760×450/640×380 проверена на macOS; Windows rendered parity не подтверждена. |
| Информация об архиве / F3/F4 | Archive Info остаётся отдельной QLabel-формой. Disk/archive F3/F4 используют source внешний viewer/editor; ограниченный UTF-8 диалог оставлен отдельной дополнительной командой. Archive known/child-process lifetime и changed-file update подключены в P1-08.2. |

Copy и Password проверяются ресурсными контрактами с current-build fixture.
Текущая коррекция Overwrite восстанавливает компактный footer справа, короткий
Cancel под Auto Rename, верхнее выравнивание file details и font-relative
начальный размер. Qt сохраняет адаптацию native metrics/minimum sizes и
responsive layout; скрытие extra actions не сдвигает Yes/No/Cancel.
Исходные шесть решений и default No сохранены. Rendered Windows reference и
точный проверенный scope — в [receipt P3-06](../upstream-port-audit/p3-06-2026-10-15.md).
После исправлений `./scripts/check.sh` прошёл на macOS: formatting, сборка
приложения и все 51 CTest. Свежая extraction Copy полностью совпала с сохранённым
reference; оба resource comparator завершились без ошибок, по четыре отчёта.
Для остальных сопоставлены resources/source и свежие geometry JSON существующих
тестов; это не автоматическая проверка каждого RC control/state. Pixel parity,
tab order, Windows/Linux runtime и полный набор ветвей не проверены. Backend
недостающих режимов, прогресс и полные настройки требуют отдельных source ports;
декоративные неподключённые controls не добавлялись.

Избранное хранит исходный CFastFolders (FolderShortcuts): десять UI-слотов,
пустые промежуточные записи и полный путь панели. CPanel::SetBookmark и
OpenBookmark подключены к source-address Qt navigateTo. Исчезнувшая папка внутри
уже открытого архива теперь выбирает существующего родителя, как история,
без повторного list. Вложенные архивы сохраняют стабильный virtual prefix;
материализованные engine paths не входят в избранное. Поведение после закрытия
цепочки описано в P3-05.14 выше. Старые настройки MVP history/paths и favorites/*
игнорируются; fallback не используется.

История папок использует исходный CFolderHistory: до 100 путей, последний
первым, удаление всех дублей без учёта регистра. Это заменяет предел 20
самостоятельного Qt MVP. Диалог следует
ListViewDialog.rc: список и OK/Cancel; Delete удаляет выделенные строки,
Ctrl+A выделяет все, Ctrl+C/Ctrl+Insert копируют полные пути. На macOS также
доступен Cmd. Enter/двойной щелчок подтверждают путь; Alt+Enter показывает текст.
Удаление сохраняется только после OK, Cancel оставляет историю неизменной.
Одноколоночный список проверяется при 540×360 и 360×240 по JSON/геометрии;
Windows rendering недоступен, визуальная идентичность не подтверждена.
Настройка одного щелчка действует и в общем списке истории/свойств.
Один щелчок подтверждает путь истории; Alt+щелчок и щелчок свойства показывают
полный текст. Ctrl/Shift/Meta сохраняют выбор без открытия. Наведение в течение
400ms выбирает всю строку без активации; leave/нажатие/модификаторы прекращают
ожидание. Qt использует фиксированный interval вместо системного Win32 hover time.
Основная файловая table/icon панель теперь использует тот же адаптированный
hover/track-select: 400 ms, без открытия, с отменой при Ctrl/Shift, drag,
leave и смене строки после scroll/removal. В 7zFM это системный Win32
track-select interval, а Qt использует фиксированное ожидание.
ShowDots добавляет исходный `..` в файловый и архивный список, но не в
виртуальный root. Reload сохраняет исходный выбор родительской строки при
fallback focus0 и восстанавливает её по явному имени `..`. Native marks/operated
indices не включают sentinel. Qt command availability допускает Open/Enter и
Open Outside для выбранного родителя через исходные handlers. Проверены обе
панели/EN-RU/standard-alternative/hidden dots/small-worker-flat consumers;
native/rendered acceptance остаётся открытой.
В альтернативном выборе Refresh сбрасывает начало Shift-группы на первую
строку, независимо от восстановленного фокуса. Последующий Shift-click следует
исходному диапазону; `..` исключён из marks/operated indices. Проверены прямое
восстановление, menu Refresh и таймер для обеих панелей, EN/RU, table/icons и
показанной/скрытой строки `..` с сохранением файлов и соседней панели.
FullRow в табличном виде переключает окраску всей строки
либо только имени. На macOS/Linux ShowSystemMenu добавляет действие открытия
каталога в системном файловом менеджере; это уже, чем Windows shell menu.
Полная сверка контекстного меню остаётся P0-09. Альтернативное выделение
использует исходную модель отметок P0-07.3. Визуальная проверка двух состояний FullRow выполнена на текущем Qt
build; Windows rendered reference и Windows/Linux runtime не проверены.

Команды маски/типа/всех/инверсии теперь используют исходный индексный вектор
`CPanel::SelectSpec/SelectByType/SelectAll/InvertSelection`, включая выбор папок
по типу, последнюю точку имени и исключение строки `..`. Qt показывает
`CComboDialog` и переносит его принятую маску в исходный цикл; Cancel не меняет
выделение и фокус. Общая Qt selection model связывает все четыре режима списка.
Ctrl-click, Shift-click, Shift-arrow и Insert теперь используют исходные
`OnLeftClick`, `OnArrowWithShift`, `OnShiftSelectMessage`, `OnInsert` и отдельный
вектор отметок. Qt оставляет одну строку фокуса и показывает отмеченные строки
красным RGB(255,192,192), как upstream custom draw; table/icon делят модель.
Клавиатурная навигация значков следует геометрии Qt, поэтому стрелка вправо
переходит к соседнему значку, а в подробном/списочном виде вниз — к следующей
строке. Отметки сохраняются при смене вида и обновлении списка. Точные цвета,
отступы и пиксельное сходство с Windows не подтверждены без Windows runtime.
На macOS исходная desktop policy сопоставления масок без учёта регистра
восстановлена: Qt определяет `TARGET_OS_IPHONE=0`, и это больше не включает
iPhone-specific чувствительность к регистру.

Полное значение свойства и Alt+Enter истории открывают просмотр текста;
компоновка и взаимодействие описаны в [контракте](../ui-layout-contracts.md#full-text-detail).

F5/F6 показывает перенесённую `CPanel::GetItemsInfoString`: число выбранных
папок/файлов, известные размеры, текущий путь и первые пять относительных
имён с многоточием при большем выделении. Неизвестный размер не подменяется
нулём: папки файловой системы до расчёта размера показывают только число.
Папки архива получают размеры через CAgentFolder: в normal это CProxyArc,
а в flat размер папки отсутствует по исходному GetProperty. Числа группируются пробелами
по оригинальному ConvertSizeToString из `PanelListNotify.cpp`.

Copy/Move summary reads the active source folder through original operated
indices and GetItemsInfoString. Missing sizes stay unknown, defined zero stays
defined, and flat archive folder totals remain absent. Cancel dispatches no operation.
Source summary structure was checked in 16 normal/compact reports on macOS;
Windows/Linux runtime and rendered Windows parity remain unverified.

Диалог F5/F6 перенесён из `CopyDialog.cpp/.h`: назначения выбираются из
редактируемой истории, Browse нормализует конечный разделитель, Cancel
оставляет Value без изменений. История CopyHistory читает только исходный
ключ; MVP copy/history игнорируется. Совпадения удаляются исходным
IsEqualTo_NoCase, предел —20; [текущая запись](../upstream-port-audit.md#p3-05--viewsettings-and-source-copy-history-2026-10-10)
уточняет отличие source upper-case от прежнего Qt case folding.
Список выделения прокручивается и не переносит строки; это адаптация
SS_LEFTNOWORDWRAP для длинных имён. Структура проверена по `CopyDialog.rc`
и геометрии Qt; пиксельная сверка с Windows и проверка Windows/Linux не выполнены.

Копирование дерева файловой системы использует перенесённые `CopyFolder`
и read/write ветку `CopyFile_Ask` из `FSFolderCopy.cpp` (`0766b73`).
В Qt занятые имена вызывают исходный OverwriteDialog с Да/Нет/для всех,
автоматическим переименованием и отменой. Решение для всех хранится лишь на
время операции. Qt worker ожидает ответ без обращения к widget, а при отмене
запрос закрывается. Заменённый объект держится как backup до успешного
завершения всей операции; при ошибке откатываются уже опубликованные объекты.
Совпадающие папки сливаются через исходный `CopyFolder`; для вложенных
занятых файлов исходный `AskWrite` показывает OverwriteDialog. Слияние
собирается во временной папке, поэтому отказ или отмена до удаления сохраняют
прежнюю цель. F6 удаляет лишь записанные файлы, а пропущенные остаются в
источнике. Ссылки и несовпадающие типы file/directory запрещены. P1-02.1
добавил staged fallback для F6 при отказе rename между томами.

## Следующие крупные блоки

Создание папки использует перенесённый `Dlg_CreateFolder` и общий
`CComboDialog`, с исходным default «Новая папка». Создание файла и маска
выделения используют ту же форму. OK возвращает сырой текст, Cancel не
подтверждает изменение; ограничения имени проверяет операция. Компоненты
следуют ComboDialog.rc, геометрия проверена в нормальном/компактном размере;
точное визуальное совпадение Windows остаётся непроверенным.

Split перенесён в `CSplitDialog` с исходным порядком destination/volume
и именем источника в title. Combine использует общий `CCopyDialog`, как
`CApp::Combine`, с исходной сводкой в прокручиваемой области. Назначение
может быть новой папкой: создаётся после OK и дополнительного подтверждения
Split для >=100 частей. Отмена dialog/подтверждения папку не создаёт.
Ошибка размера остаётся inline вместо MessageBox. Структура и компактная
геометрия проверяются JSON/тестами; rendered Windows reference отсутствует,
pixel parity не подтверждена. Ограничение 10000 частей и отказ занятых имён
сохраняются как интеграционные отличия.

Создание папки/файла и Rename используют исходные `CPanel` проверки имени,
`CorrectFsPath` и `CThreadFolderOperations` из `PanelOperations.cpp`/
`BrowseDialog.cpp` (`0766b73`), а файловые действия — `FSFolder.cpp`.
F2 редактирует имя прямо в строке или значке во всех четырёх режимах списка;
Enter подтверждает, Escape отменяет, не открывая объект. Папка принимает составной путь, файл — путь с уже
существующими родительскими папками. Исходный Windows path normalizer сохранён;
на POSIX `CorrectFsPath` возвращает ввод без преобразования, как upstream.
Qt-таблица перезагружается после результата и восстанавливает выбор; при
создании вложенной папки выбирается её первый компонент в текущем списке.
Создание по-прежнему использует Qt-адаптацию `ComboDialog` с порядком полей
`ComboDialog.rc`; normal/compact геометрия проверяется контрактом.
Обычный Delete передаёт объекты в системную корзину через
`QFile::moveToTrash`; Shift+Delete вызывает исходный `CThreadFolderOperations`
и постоянное `FSFolder::Delete`. Подтверждение `PanelOperations.cpp` различает
один файл, папку и несколько объектов. Это явная Qt-адаптация: в исходном
Windows пути подтверждение корзины показывает Shell, а у Qt native Trash
такого диалога нет. Исходный worker поддерживает отмену при обходе дерева и
сохраняет частичный результат. Панель обновляется после успеха, ошибки и
отмены. Вложенные symlink удаляются сами, их цели не обходятся;
непосредственно выбранные ссылки отклоняются. Диалоги ввода имени остаются
Qt-адаптацией. Рендеренный Windows-референс inline-редактирования и
подтверждений недоступен; pixel parity не подтверждена.

1. Для P0-08.1 перенесены disk/disk F9 и отдельные представления; mixed archive и назначения межпанельных операций из `App.cpp` остаются P0-08.2/P1-01.
2. Доработать раскладку режимов просмотра, сортировку по свойствам и динамические столбцы по `PanelItems.cpp`, `PanelSort.cpp` и `resource.rc`; для очень больших деревьев вынести обход и сортировку из потока UI.
3. Переносить команды File и контекстного меню с их состояниями доступности из `PanelMenu.cpp` и `MyLoadMenu.cpp`. Для Open Inside/Outside и обратной записи после внешнего редактирования определить безопасную временную распаковку и обновление архива.
4. Расширить постоянную историю адресов, диалоги свойств и настроек. Платформенные отличия системных меню и интеграции проверять отдельно на Windows, macOS и Linux.

Функциональные правила `CPanel::SelectSpec`, `SelectByType`, `SelectAll` и
`InvertSelection` из `PanelSelect.cpp` (0766b73) выделены в `CPanelSelection`
с теми же именами методов. Маска применяется и к папкам; выбор по типу папки
выбирает папки, а тип файла определяется последней точкой и не выбирает папки.
Имена без точки образуют отдельную группу; выбор вне группы сохраняется.
В плоском просмотре маска и тип используют базовое имя (`GetItemName()`),
а не отображаемый относительный путь: точки в родительских папках не меняют
тип файла, точная маска имени работает на любой глубине.
`mainwindow_selection.cpp` владеет диалогом, чтением текущего индекса и
применением результата к общей модели таблицы и списка значков. Диапазоны
мышью/клавиатурой обрабатывает Qt; альтернативный `_mySelectMode` 7zFM пока
отсутствует.

`PanelKey.cpp::OnKeyDown()` сохраняет исходные ветви
`VK_ADD`, `VK_SUBTRACT`, `VK_MULTIPLY`. Цифровые
`+`/`−` вызывают маску, Shift — выбор всех, Alt — выбор по типу,
`*` — инверсию. Приоритет Alt над Shift и подавление Ctrl+Plus следуют
upstream. Адаптация Qt использует `KeypadModifier`, не перехватывает ввод
в адресной строке и ограничивает команды их текущей доступностью.
Таблица и список значков используют один обработчик; фактические события
цифровых клавиатур Windows/Linux ещё требуют платформенной проверки.

`mainwindow_panelkey.cpp::handlePanelKey()` объединяет Qt-адаптер исходного
`CPanel::OnKeyDown()` и `Panel.cpp::CMyComboBoxEdit::OnMessage()`.
Shift+F4 вызывает существующую CreateFile только при фокусе списка и
доступности команды. Alt+F1/F2 из списка или адреса раскрывают историю,
как `App.cpp::CPanelCallbackImp::SetFocusToPath()`; обе клавиши используют
активную панель и в disk/disk P0-08.1; раздельный выбор левой/правой цели
остаётся P0-08.2. Tab из поля
адреса возвращает фокус в активный режим просмотра без навигации.
Win32 ComboBox заменён редактируемым Qt ComboBox и его popup; модификаторы
проверяются точно, чтобы не перехватывать другие сочетания.

В режиме открытия одним щелчком Qt-жесты Ctrl/Shift для множественного
выделения (и Meta как платформенный модификатор) не активируют запись.
`mainwindow_panel.cpp::activateSingleClickedRow()` отделяет выбор от открытия
для таблицы и списка значков. Upstream включает `LVS_EX_ONECLICKACTIVATE`
в `App.cpp::SetListSettings()` и получает активацию через Win32
`LVN_ITEMACTIVATE` (`PanelListNotify.cpp`); Qt вместо этого получает каждый
щелчок через `clicked`, поэтому явно исключает жесты выделения.
Это адаптация Qt, а не перенос Win32 уведомлений. Поведение hover/track-select
`LVS_EX_TRACKSELECT` пока не перенесено. Описание исходных стилей:
[Microsoft List-View Styles](https://learn.microsoft.com/en-us/windows/win32/controls/extended-list-view-styles).

В списке значков Qt теперь выделяет полные строки общей модели, как таблица:
мышь с Ctrl/Shift формирует те же выбранные пути и состояния команд.
Раньше выделялась только колонка имени, невидимая для `selectedRows()`;
проверки, устанавливавшие выбор через таблицу, не обнаруживали этот разрыв.

`PanelKey.cpp::OnKeyDown()` обрабатывает `VK_BACK` без проверки модификаторов.
Qt-адаптер теперь следует этому правилу при фокусе списка, во всех четырёх
режимах. Поле адреса сохраняет редактирование текста; во время загрузки
и операций переход отключён состоянием команды. Win32 уведомления заменены
Qt event filter, а исходный обработчик сохранён по пути
`src/CPP/7zip/UI/FileManager/PanelKey.cpp`.

Ctrl+C и Ctrl+Insert копируют имена реально отмеченных source indices через
CRLF, включая альтернативный выбор; пустой выбор даёт пустой текст без
подстановки фокусной ячейки. Ctrl+X/V и Shift+Insert сохраняют исходные no-op.
Ctrl+A/C/N/R/W/Z и Ctrl+1–4 учитывают Ctrl даже с дополнительными Alt/Shift;
Ctrl+W откладывает Close, как исходный PostMessage. Ctrl+PgDn открывает
сфокусированный объект внутри панели, Ctrl+PgUp переходит к родителю.
Эти ветви CMyListView::OnMessage выполняются до PanelKey::OnKeyDown;
комментарий EnterToFocused в последнем не отменяет обработку list window.
Qt вызывает исходные ветви через существующие Open Inside/Parent services
с публикацией панели и сохранением lifetime. Ctrl+F3–F7 сохраняют исходный
sort switch. Поля адреса и
редактор имени сохраняют собственные текстовые shortcuts. Проверки и точные
платформенные границы — в audit § source keyboard dispatcher.

Combine последовательно читает части и проверяет полную запись, изменение
размера источника и ошибки завершения записи. Отмена доступна до первого
чтения и во время работы. Временный результат публикуется только при успехе;
ошибка или отмена убирает его.

Имя результата Combine берётся из `CApp::Combine()` (`0766b73`): от
`UnchangedPart` убираются все завершающие точки; для пустого имени используется
`file`. Этот блок выделен в `CVolSeqName::GetCombinedName()` и вызывается UI.
Доступность Combine и обнаружение частей используют один `ParseName`, без
дополнительного ограничения регулярным выражением `.001`. Проверка пропусков,
символьных ссылок и непустых частей остаётся адаптацией Qt.

Split использует заданную последовательность размеров и повторяет последний
размер для оставшихся частей. Изменение размера источника и ошибки записи
завершают операцию с откатом временных файлов. Предварительное выделение
места под том не выполняется; файл записывается последовательно.

LZMA/LZMA2 сохраняет явный выбор порядка при смене уровня; автоматический
выбор не передаёт `fb` движку. Исходный числовой ряд, defaults и общий
владелец `CCompressDialog::SetOrder2()` записаны в
[source audit](../upstream-port-audit.md#qt-layout-diagnostics).

Числовой ряд `CCompressDialog::SetSolidBlockSize2` перенесён из `0766b73`
в исходный `GUI/CompressDialog.cpp`: степени 2^20–2^36. Qt хранит размеры
в МиБ вместо логарифмов; UI и backend используют один источник. Сохраняется
явное значение при смене уровня, для ZIP/Store блок отключён. Подписи
показывают исходные MB/GB. Расчёт Auto и Full Solid перенесены последующей итерацией; Auto оставляет выбор движку.

Диалог сжатия ZIP теперь включает размер слова Deflate и Deflate64 по
`SetOrder2` upstream: 8–258 / 8–257, Auto 32/64/128 для уровней ниже 7,
7 и 9. Явный выбор сохраняется при смене уровня; Store отключает поле.
Qt сохраняет Auto как 0 и оставляет значение движку, вместо Win32 sentinel -1.
Расположение поля остаётся прежним; проверены снимки 760×720 и 640×480
на macOS offscreen. Windows/Linux и физический native UI не проверены.

PPMd в 7z и ZIP теперь позволяет выбрать порядок по соответствующим веткам
SetOrder2: 2–32 (исходный разреженный ряд) и 2–16. Auto показывает порядок
в зависимости от уровня; при явном выборе движок получает `o`, для LZMA и
Deflate сохраняется `fb`. Словарь/память PPMd пока не доступны для явного
выбора. Диалоги проверены по снимкам 760×720 и 640×480 на macOS offscreen;
native UI Windows/Linux остаётся непроверенным.

Словарь LZMA/LZMA2 следует SetDictionary2: 64 КиБ, 256 КиБ, 1 МиБ,
2 МиБ и исходный ряд степеней двойки с промежуточными значениями до
3840 МиБ на 64-bit (до 64 МиБ на 32-bit). Auto показывает исходную
формулу для уровня и архитектуры, а значение выбирает движок. Пользовательский лимит RAM добавлен последующей итерацией; автоматические ограничения
потоков и отказ по памяти добавлены последующей итерацией.
Размеры отображаются в исходных KB/MB по Combo_AddDict2; снимки 760×720 и
640×480 проверены на macOS offscreen. Windows/Linux и 32-bit не проверены.

PPMd для 7z теперь предоставляет исходный ряд памяти от 1 МиБ до 1024 МиБ
на 64-bit (512 МиБ на 32-bit); Auto показывает 16 МиБ при уровне 5,
256 МиБ при уровне 9. Поле остаётся Dictionary size перед Word size,
как в CompressDialog.rc; подписи сохраняют исходные MB. Выбор передаётся
7zz через mem=<bytes>b. Память ZIP PPMd и автоматический лимит RAM добавлены последующими итерациями.
Снимки memory-PPMd-7z-760/640 проверены на macOS offscreen; другие ОС
и 32-bit остаются непроверенными.

Явная память ZIP PPMd теперь доступна: 1,2,4,8,16,32,64,128,256 МиБ,
Auto=1<<(level+19), как в отдельной ветке PPMdZip upstream. Ряд не включает
3 МиБ из 7z PPMd. Поле стоит перед размером слова по CompressDialog.rc.
Снимки memory-PPMd-zip-760/640 просмотрены на macOS offscreen; Windows/Linux
и аппаратные ограничения памяти не проверены.

BZip2 в 7z и ZIP теперь предоставляет Dictionary size 100–900 КиБ
с шагом 100 КиБ. Auto показывает 100 КиБ на уровне 1, 500 на уровне 3,
900 при уровне >=5, как в исходной ветке. Размер слова остаётся отключён.
Расположение контролов сверено с CompressDialog.rc; снимки
memory-BZip2-{7z,zip}-{760,640} просмотрены на macOS offscreen.
Windows/Linux и RAM limits не проверены.

Deflate/Deflate64 показывают фиксированный автоматический словарь 32/64 КиБ
во всех уровнях ZIP; ручной выбор отключён, как в ветке upstream с одной
строкой Auto. Снимки memory-Deflate/Deflate64-zip-760/640 проверены.

Управление solid-блоком теперь использует один combo по CompressDialog.rc:
Auto с вычисленным размером, Non-solid, 1 МиБ–64 ГиБ, Full solid.
Отдельный Qt checkbox убран. Auto меняется при выборе метода/словаря/уровня;
Full и Non-solid сохраняются при смене словаря. Данные и параметры
операции следуют SetSolidBlockSize2/OnOK; Auto не задаёт -ms, Full передаёт
UInt64(-1) байт. ZIP и Store отключают список. Снимки всех трёх режимов
при 760×720 и 640×480 проверены на macOS offscreen; Windows/Linux не проверены.

Потоки CPU теперь следуют SetNumThreads2: ряд ограничен методом и числом
аппаратных CPU×2, Auto показывает число потоков с учётом памяти ZIP/LZMA2.
Для 7z PPMd/Store доступен только Auto(1); ZIP использует отдельный предел
32/128 по архитектуре. Справа показано процессное/аппаратное число CPU.
Под строкой потоков стоят Memory usage for Compressing / Decompressing
по порядку CompressDialog.rc. Оценка — алгоритм 7zFM, а не измерение RSS
процесса. Превышение исходного 80% RAM отклоняется до запуска. Выбор
другого лимита памяти и Registry пока отсутствует. Формы PPMd, ZIP и LZMA2
просмотрены при 760×720 и 640×480 на macOS offscreen; Windows/Linux,
32-bit и CPU groups не проверены.

Лимит памяти Add to Archive следует SetMemUseCombo/AddMemComboItem upstream
0766b73: Auto=80%, 10–100% с шагом 10 и ряд размеров 256/384/512/768 МиБ
и далее до 16 ТиБ на 64-bit (до 4 ГиБ на 32-bit). Исходные формулы и
архитектурный предел сохранены. Поле расположено в строке памяти для сжатия
перед памятью для распаковки, как в CompressDialog.rc; Qt выравнивает
оценку и список горизонтально, RAM/предел в байтах доступны в подсказке.
Предел пересчитывает Auto-потоки ZIP/LZMA2, явные потоки сохраняются.
Отказ при нехватке памяти оставляет диалог открытым; backend повторяет
проверку до создания stage и передаёт memuse движку. Добавление в
существующий архив сохраняет прежние параметры, как до этой итерации,
и не показывает новые контролы. Лимит хранится отдельно для форматов и сохраняется при OK между
запусками (CFormatOptions; Qt INI вместо Windows Registry). Отмена и
ошибка валидации сохраняют прежние настройки. Нестандартные значения
из сохранённого spec вставляются в исходный ряд, неверный spec даёт Auto.
Снимки memory-limit-760.png и -640.png проверены на macOS offscreen.
Windows/Linux и 32-bit требуют отдельных проверок.

Диалог извлечения теперь использует перенесённые OnInit/OnOK CExtractDialog
(0766b73). Порядок перезаписи исправлен на исходный: отказ (адаптация Ask),
перезапись, пропуск, переименование извлекаемого, переименование существующего.
История ограничена 16 элементами исходным кодом, при подтверждении выбранный
путь перемещается в начало, сравнение уникальности без учёта регистра.
Путь назначения получает конечный разделитель, как в 7zFM. Режимы и флажок
показа пароля сохраняются при OK; Cancel не меняет INI, пароль не сохраняется.
SplitDest добавлен последующей итерацией. Absolute paths, интерактивный
Ask, ElimDup и NtSecurity пока недоступны. Qt controls сохраняют две колонки ExtractDialog.rc; визуальные
снимки extract-port-640.png и -470.png проверяются на macOS offscreen.

SplitDest диалога Extract перенесён из OnInit/OnOK 0766b73. По умолчанию
включён: родительская папка в combo, последний компонент (включая конечный
разделитель) в Edit. Корень и одиночный относительный путь не дублируются.
Поле имени стоит перед Path mode слева, Password справа, как в rc.
Отключение флажка скрывает Edit и использует только папку combo; изменение
сохраняется при OK. Добавлены Qt tooltip и accessibleName пустому исходному
checkbox для доступности. Снимки split-dest-{0,1}-{640,470}.png просмотрены
на macOS offscreen. Windows/Linux и native folder picker не проверены.

### Шифрование Add to Archive

Пустой пароль создаёт архив без шифрования. Для 7z метод — AES-256;
шифрование имён включается отдельным флажком. Для ZIP исходный список —
ZipCrypto по умолчанию и AES-256; последний метод ZIP сохраняется при OK.
Show Password показывает первое поле и скрывает повтор с его подписью;
после отключения повтор снова виден и проверяется. Флажки показа и шифрования
имён сохраняются, пароль и повтор никогда не сохраняются. Cancel и ошибка
не записывают изменения. При смене 7z→ZIP header checkbox отключается,
при возвращении его выбор сохраняется.

ZIP принимает только символы 0x20–0x7F, как исходный IsAsciiString; для AES
максимум 99. Предел проверяется и диалогом, и backend до запуска операции.
7z допускает Unicode-пароли. Передача пароля 7zz остаётся через stdin.
Контролы password/reenter/show/method/headers проверены геометрией в обоих
состояниях показа при 760×720 и 640×480, включая длинный путь в истории.
Qt сохраняет вертикальную форму; Windows rendering и Windows/Linux runtime
ещё не проверены.

### Путь архива и история Add to Archive

Поле «Архив» — редактируемый список с полным именем и расширением, как
IDC_COMPRESS_ARCHIVE. При смене формата последнее расширение заменяется;
ведущая точка скрытого имени не удаляется. При OK сохраняются полный путь
и архиватор; следующий диалог открывается с последним форматом. История
ограничена 20, текущая цель идёт первой, дубли сравниваются без учёта регистра.
Выбор записи восстанавливает формат и папку. Cancel и ошибка не сохраняют
изменения; пароль в историю не попадает.

Qt сохраняет отдельное поле папки и source picker. Путь в combo может быть
абсолютным или относительным к папке; для backend допустимы только .7z/.zip
с выбранным форматом, существующим родителем и новой целью. Диалог добавления
в открытый архив не изменяет историю создания. Геометрия combo проверена на
macOS offscreen при 760×720 и 640×480; Windows/Linux и отрисованный Windows
эталон не проверены.

### Профили параметров Add to Archive

Доступные параметры сжатия сохраняются отдельно для 7z и ZIP при OK и
восстанавливаются при смене формата и повторном открытии: уровень, метод,
словарь/память, слово/порядок, solid, потоки и лимит RAM. Cancel и отказ
валидации не записывают профиль. Auto не превращается в явное вычисленное
значение. Неподдерживаемый сохранённый метод выбирает первый доступный;
его словарь/слово/solid/потоки не переносятся на другой метод.
QSettings заменяет Windows Registry. История и последний формат перенесены;
настройки времени ещё не перенесены.

При смене уровня сжатия прежние метод, словарь, слово/порядок, solid и
потоки сбрасываются к исходным значениям по умолчанию, как в
IDC_COMPRESS_LEVEL оригинального 7zFM. Лимит памяти сохраняется. Уровень
«Без сжатия» отключает зависимые контролы; возвращение к другому уровню
пересчитывает их Auto. Это заменяет прежнее удержание ручных значений Qt.

Смена метода больше не переносит ручные размеры другого метода. Возврат
к методу сохранённого профиля восстанавливает его параметры. При смене
словаря обычный ручной solid сбрасывается к Auto; Non-solid и Full остаются,
как в upstream. Ручные потоки сохраняются при смене словаря и лимита RAM.

Состояния диалога проверены на Qt/macOS при 760×720 и 640×480; порядок
level/method/dictionary/order/solid/threads/memory сопоставлен с
`CPP/7zip/UI/GUI/CompressDialog.rc`. Отрисованный Windows-эталон недоступен:
это проверка структуры и читаемости Qt, визуальная идентичность не подтверждена.
