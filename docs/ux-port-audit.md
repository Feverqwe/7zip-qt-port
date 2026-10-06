# Сверка интерфейса с оригинальным 7zFM

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
| Add: выбор источников | FileManager/Panel.cpp::AddToArchive; GUI/CompressDialog.rc | Picker удалён из всех ветвей диалога. Исходный CPanel::AddToArchive перенесён целиком с typed CompressFiles services: источники из панели, пустое выделение и архивная панель дают исходные ошибки, native ShowDialog/UpdateArchive создаёт/обновляет output. Семь форматов, параметры, metadata/password/Cancel проверены; unknown-extension update использует исходный two-panel Copy. Drag Add также идёт через source CompressCall2, с native success/error/Cancel. | macOS focused16/16PASS14.41s; свежие full/Windows/Linux проверки идут |
| Add: компоновка и дополнительные поля | GUI/CompressDialog.rc/.cpp | Format и compression слева; Update/Path и Options/Encryption справа; volumes/parameters слева; лишняя Compression group, отдельные Save in/preview и Create/Add вместо OK удалены. Folder label над Archive+`...`. Encryption method теперь в одной строке с combo, decompression memory выровнено вправо по настоящему Windows runtime. Normal/compact geometry и текущие 14 Add/14 Copy JSON просмотрены; labels не обрезаны, compact нижние поля доступны прокруткой. AddFilter/OnButtonSetArchive перенесены; порядок фильтров, aliases, extension/Cancel проверены. Устаревшая constructor-only existingTarget ветвь с блокировкой полей ещё подлежит удалению. | macOS focused19/19PASS14.93s; текущие platform/full проверки и legacy branch открыты |
| Add: размер окна | GUI/CompressDialog.rc; настоящий Windows 7zFM | В Qt оставался размер760×720 от прежнего picker. Размер теперь следует содержимому и доступной области экрана. Повторный настоящий Windows runtime v9:642×578, OK/Cancel/Help полностью доступны; local640×571 JSON и default/frame regression проверены. | Windows13/13PASS70.86s и runtime проверены; snapshot предшествует следующему Add source port |
| Add: значения памяти | GUI/CompressDialog.cpp::PrintMemUsage; CompressDialog.rc | Сравнение настоящего 7zFM и Qt на Windows выявило отсутствие limit/RAM в видимой строке и label сбоку вместо сверху. Исходный PrintMemUsage перенесён с Qt text return; ветви и новые EN/RU reports проверены. Новый снимок запущенного приложения показывает полную строку. | macOS full227/227; Windows/Linux focused13/13, global acceptance открыта |
| Add: Delete и update mode | GUI/CompressDialog.cpp::OnInit/OnOK; Common/Update.cpp::CheckDeleteAfterCompressingActionSet | Самостоятельное отключение checkbox для Freshen/Update/Sync удалено. Исходная форма принимает intent, source worker проверяет совместимость после OK. Успех/Cancel/отказ с сохранением input проверены. | macOS full227/227; Windows/Linux focused13/13, global acceptance открыта |
| File/menu/toolbar/context | FileManager/MyLoadMenu.cpp, App.cpp, PanelMenu.cpp, resource.rc; Explorer/ContextMenu.cpp | Independent Open/Create/Add-to-open/Extract-selected QAction, extension IDs/dispatch и Ctrl+O picker удалены. Source toolbar/Explorer Add, two-panel Copy update, selected Copy To/F5 и Open/Open Inside сохранены. Archive popup больше не добавляет отдельную группу Qt Add/Extract/Test/Info. Program-menu source lifetime и Info→Properties подключены. Journal/built-in text view/edit aliases и остальные local commands ещё требуют миграции. | Четыре aliases удалены; текущий full/platform gate и оставшиеся consumers открыты |
| Extract и prompts | GUI/ExtractDialog.rc/.cpp, FileManager/OverwriteDialog.rc/.cpp | Проверить исходный набор полей, defaults, скрываемые состояния, overwrite questions и кнопки. Проверки безопасного назначения остаются за backend. | Открыто |
| Copy/Move, две панели | FileManager/CopyDialog.rc/.cpp, App.cpp, PanelCopy.cpp | Проверить последовательность выбора источника/цели, defaults, prompts и команды в архивном namespace; сохранить исходный способ добавления в архив. | Открыто |
| Navigation/list/views/selection | FileManager/Panel.cpp, PanelKey.cpp, PanelSelect.cpp, PanelItems.cpp, PanelListNotify.cpp | Проверить focused versus selected, пустую папку, адрес, root/parent, shortcuts, columns и transitions во всех режимах. | Открыто |
| Options и дочерние страницы | FileManager/OptionsDialog.cpp, Windows/Control/PropertyPage.cpp и *Page.rc/.cpp | Исправлена самостоятельная стартовая Settings page: исходный property sheet начинает с System. Шесть source pages, Apply/OK/Cancel/restart, обе панели и EN/RU normal/compact проверены в P1-12.2. Qt Tab cycle сохраняет pending input и доступный фокус; Windows может посетить footer перед полями. Полная runtime-сверка полей/контролов с оригиналом остаётся обязательной. | P1-12.2 accepted; macOS full234/234, Windows8/8, Linux8/8, 72 current page JSON reviewed; rendered/global audit открыт |
| Standard dialog captions | FileManager/LangUtils.cpp::LangSetDlgItems, GuiCommon.rc и dialog OnInit callers | Исправлены English Cancel при выбранном RU и сброс Memory Continue в OK после смены языка. Исходная таблица standard IDs подключена к двенадцати dialog consumers; roles/signals сохраняются. Qt native dialogs и Options property-sheet имеют отдельного платформенного владельца. | P3-05.9 accepted button scope:27Qt/0skip и по48 EN/RU normal/compact footer reports на3OS; rendered/global acceptance и source colon helpers .10 открыты |
| Progress/messages/password | FileManager/ProgressDialog2.rc/.cpp, MessagesDialog.rc/.cpp, PasswordDialog.rc/.cpp | Проверить дополнительные тексты/кнопки, pause/cancel/close, повторные ошибки и modal lifetime. | Открыто |
| Properties/Comment/Link/Split/Combine/Benchmark | Соответствующие FileManager и GUI .rc/.cpp | Проверить параметры вызова, набор контролов, условные состояния, action semantics и Help. | Открыто |
| Help/About и OS integration | FileManager/AboutDialog.rc/.cpp, HelpUtils.cpp, SystemPage.cpp, Explorer | Проверить самостоятельные экраны/команды и необходимые различия native платформ; не подменять исходный workflow новой формой. | Открыто |

Конкретный пропуск Root/drive-view command зафиксирован в
[P3-05.13 source audit](upstream-port-audit.md#p3-0513--source-root-command-target-implementation-validation-active-2026-10-06)
и text contract: проверка непустого списка и selection marks не доказывает
исходную цель команды. Исходный переход и consumers теперь интегрированы;
проверки provider/prefix/Parent/Back/peer state добавлены. Native/rendered
приёмка остаётся обязательной, текущие свидетельства — в source audit.

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
Duplicate-ID menu/sort workflows ещё требуют сверки и native/rendered проверки.
Сверка тел
не подтверждает текущую отрисовку или завершение всего модуля.
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
