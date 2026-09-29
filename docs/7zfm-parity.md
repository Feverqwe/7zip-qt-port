# Сверка с Windows 7zFM

Здесь описаны текущие сценарии, команды, ограничения и отличия от Windows
7zFM. Исходники в таблице служат ссылками на эталон поведения;
происхождение реализации, ревизии и технические адаптации находятся в
[аудите переноса](upstream-port-audit.md). Порядок проверки UI описан в
[тестировании](testing.md) и [контрактах компоновки](ui-layout-contracts.md).
Совпадение поведения не подтверждает перенос исходника или проверку на всех ОС.

P0-02.2b3c3b1 compiles original FormatUtils and source caption bundle, preserving
literal {0} substitution and update/error line breaks. Fixed Russian captions are
available to upcoming update dialogs; no displayed dialog/layout changes yet.
Complete modified-nested-file prompt/progress/cancel/update remains mandatory.

P0-02.2b3c3a restores original processing guards around source selection restoration.
Qt selection callbacks defer command refresh during intermediate restore state
and resume afterward. No visible text/layout changes. Full native notification/
timer/status dispatch and nested update/parent-stack workflows remain mandatory.

P0-02.2b3c2 copies original CTempFileInfo/CFolderLink cleanup/change/lifetime
records. Native time comparison retains upstream FILETIME/timespec precision;
cleanup only deletes an explicitly owned file and then its empty parent directory.
No displayed behavior/layout changes. Full root/nested close/pop/update/runtime
integration, memory-archive/password/zone workflows remain mandatory later scope.

P0-02.2b3c1b compiles original CLibrary lifetime/loading and extends POSIX with
original Windows parent-link transfer/copy guard. No displayed behavior changes;
native parent-stack integration follows. Module-path/resource services and
Windows/Linux runtime remain mandatory, unverified local scopes.

P0-02.2b3c1a restores original native single-item cleanup and Windows readonly
attribute handling under the Qt path wrapper. POSIX remove() accepts an empty
directory whereas Windows DeleteFile rejects it; current UI directory commands
still route through their directory owner. No layout/text change. Native parent
stack and complete filesystem/reparse/alternate-stream workflows remain mandatory.

P0-02.2b3b3c3c3 accepts the bounded owned-provider/selected-consumer lifecycle
prerequisite. Full native parent/path/navigation, all-panel release, operation
state and platform acceptance remain open; no runtime/text/layout change is made.

P0-02.2b3b3c3c2 stops the 7zz transport before window temporary paths are released
and disconnects callbacks before cancellation. Window destruction cannot leave
that operation writing into a removed temporary directory or reopen UI through
completion callbacks. No dialog/layout change. Full native panel/parent-stack
release and Windows/Linux runtime remain mandatory.

P0-02.2b3b3c3c1 verifies selected consumers across archive replacement, cancelled
extraction and failed/cancelled listing back to filesystem. Loading disables
selected commands; requests follow the current source owner and temporary hash
extraction cleanup completes. No layout/text change; full native navigation,
outer stack and operation teardown remain mandatory.

P0-02.2b3b3c3b3e verifies selected versus unselected focus and replacement empty-folder
owner at the Create input boundary. No layout/text change. The audit lists every
remaining focused open/navigation, selection restoration, drag/icon and duplicate
context consumer with its mandatory owner; selected-consumer acceptance does not
close those workflows or the source parent/path stack.

P0-02.2b3b3c3b3d routes filesystem selected-operation paths through original
CPanel full-path helpers and the bound native folder prefix. Sorting, flat mode
and preceding Properties cannot substitute paths from presentation rows. Existing
dialogs and cancellation remain unchanged. Create input follows source operated
selection: an unselected focused item is excluded. This consumer integration
does not complete native operation control flow, focused open/navigation or the
source parent/path stack. Windows/Linux runtime remains unverified.

P0-02.2b3b3c3b3c binds archive Rename/View/Edit and logical hash-result targets
to original source names/full prefixes/operated indices. Source folder metadata
also controls single-file eligibility and the existing viewer size limit.
Failure/cancel return to an operable selection without trusting row path/type
roles. Dialogs, control order and displayed text are unchanged. Full native
open/edit/hash workflows, filesystem consumers and parent/path stack stay open;
Windows/Linux runtime and rendered Windows parity remain unverified.

P0-02.2b3b3c3b3b restores original UString panel path helpers and source
folder-type/drive-prefix rules. Relative/full paths come from the bound source
folder, with no ownerless row fallback. Complete source full-prefix stack setup,
actual drive/alternate-stream providers and remaining path consumers stay open;
helper tests do not claim their workflows or cross-platform runtime parity.
Displayed UI is unchanged.

P0-02.2b3b3c3b3a adds original archive item names and full root-relative prefix
helpers in normal/flat modes, including bound nonroot folders. Displayed commands
are unchanged; remaining single-item/hash-result path consumers still require
binding to these helpers. Alternate-stream/proxy2 branches remain mandatory.

P0-02.2b3b3c3b2 routes selected archive extraction/Copy/Move/hash extraction
paths through source operated and real indices plus the owner's full Path
metadata. Display roles cannot substitute paths; requests follow source archive
order and deduplicate overlap at the 7zz boundary. The current selected-folder
expansion policy remains true in flat mode, explicitly differing from native
engine Extract's false flag until mandatory P0-12 integration. Remaining
single-item/filesystem/hash-result consumers and full operation acceptance stay
open. Visible UI is unchanged.

P0-02.2b3b3c3b1 adds the original normal/flat real-index mapping and unchanged
HeapSort dependency. Normal directory selection expands descendants; flat
mapping distinguishes explicit archive entries and implicit folders and retains
the source caller's expansion flag. The current 7zz selection caller still
expands selected folders; original engine Extract uses a different flag. This
caller adaptation and source operation port stay explicit, with no complete
Extract or alternate-stream parity claim. Visible UI is unchanged.

P0-02.2b3b3c3a removes the test-only external-proxy compatibility lifecycle.
Initialized archive folders and their children/parents always retain the source
CAgent; catalogs use its source expansion/remapping. Default unbound calls fail
without publishing undefined counts. Runtime presentation is unchanged;
remaining consumer migration and outer/native path-stack scope stay mandatory.

P0-02.2b3b3c2cc verifies source Properties in normal/flat and all four views
through filesystem/archive transitions and Refresh. Native loaded size/cache,
duplicate Unicode names/prefixes, directory counts/flat absence and missing/zero
archive values retain their source semantics. File/directory/summary dialogs
keep visible list/header/footer at normal and compact sizes; Cancel performs no
archive operation. These macOS Qt checks leave the previously recorded complete
metadata, native time parsing, raw/archive-level/shell and lifecycle scopes open.

P0-02.2b3b3c2cb binds window Properties to the current source folder and operated
indices. It displays source catalogs instead of synthesized Type/item-Path rows;
the current-location footer remains the existing full Qt location until
LoadFullPath/outer-stack migration. Filesystem packed size is now shown as source
Properties requires. Loaded size and primed packed cache are preserved after
external file changes; archive display-cell edits cannot change source values.
Unknown sizes/counts remain absent and zero stays defined. Existing supplied
archive Modified text retains fractional digits as explicitly unparsed BSTR
compatibility data; native FILETIME/timezone/precision remains mandatory
P0-03/P0-04. Raw/archive-level/shell Properties remain their mandatory owners.
Normal/compact Qt list structure uses ListViewDialog.cpp/.rc; no rendered Windows
reference or cross-platform runtime claim is made.

P0-02.2b3b3c2ca2 uses the original owned CAgentFolder catalog expansion: Path
becomes Name, descendant count descriptors are added, Prefix appears only in
flat mode and Name is inserted when the transport has no Path. Directory counts
come from the source proxy; file count values unsupported by the transport stay
absent. Catalog errors prevent root publication and allow retry. The legacy
external-proxy compatibility branch remains lifecycle migration, and full
handler metadata remains mandatory. Live window Properties binding follows;
no current displayed dialog or rendered Windows parity is claimed here.

P0-02.2b3b3c2ca1 makes the archive transport provider's Path/Size/PackSize
catalog explicit, with native source types and no fabricated absent values.
This prerequisite changes no displayed dialog; source folder catalog expansion
and live Properties integration follow. Full engine metadata remains mandatory
P0-03/P0-04/P1-10. It does not establish a complete Properties workflow or
Windows rendered/runtime parity.

P0-02.2b3b3c2b3 replaces the Properties BSTR-only formatting restriction with
original PanelMenu typed branches, including defined zero versus absence,
grouped sizes/counts, CRC/boolean/time formatting, error flags and expected-type
warning. Aggregate counts above 999 now use the same source grouping. Original
warning literals remain English pending full language work. Properties window
still receives legacy snapshots; live owner binding and filesystem packed-size
visibility remain mandatory c2c, not N/A. Raw/archive-level/shell capabilities
remain P0-03/P0-04/P1-10/P2-09. The reference is PanelMenu.cpp and
ListViewDialog.cpp/.rc; normal/compact Qt geometry cannot verify rendered Windows
parity without a Windows runtime reference.

P0-02.2b3b3c2b2c adds original archive error-property conversion and message
branches: source bit ordering, encrypted-header password hint, and unknown bits
preserved as hexadecimal. Source ExtractRes.h/Extract.rc IDs/texts back the
current fixed Russian presentation overrides. No displayed dialog changes yet;
PanelMenu integration follows. Full archive open/extract callbacks remain
mandatory operation owners, and rendered Windows parity is unverified.

P0-02.2b3b3c2b2b adds original PropertyName priority and source resource IDs.
The current fixed Russian presentation captions override corresponding native
English resource text; other captions retain exact source English fallback.
This dependency changes no displayed dialog yet. Full language selection/loading
remains mandatory P1-14, and PanelMenu/live Properties integration follows.
The reference is PropertyName.cpp, PropertyNameRes.h/.rc and LangUtils.h at the
pinned source revision, not a rendered Windows reference.

P0-02.2b3b3c2a compiles the original typed property formatters and portable
reparse parsers; it changes no current dialog text or layout. PropertyName/
PanelMenu formatting and live Properties binding remain the next mandatory
substeps. Portable binary fixtures do not establish Windows link/ACL workflow
parity. Windows native link operations retained in FileLink require additional
FileDir services and remain disabled/open; they are not N/A.
Focused 10/10, ASan 1/1 and full gate 65/65 passed on macOS 2026-09-29.
Windows/Linux runtime and Windows native reparse/ACL workflow acceptance remain
unverified; no rendered parity is claimed.

P0-02.2b3b3c2b1 replaces FSFolder's packed-size E_NOTIMPL placeholder with the
original cached GetProperty branch and Windows native compressed-size helper.
POSIX retains the original logical-size fallback because that Windows API is
unavailable; this does not establish compressed-allocation parity. Directory
Size is absent until calculated while the source packed fallback is zero.
Current window/dialog layout and visible text are unchanged; live Properties
binding follows. Windows native behavior remains unverified locally.

## Аудит диалогов (2026-09-28)

P0-02.2b3b2b1 (2026-09-29) добавляет source filesystem services, без изменения
видимого workflow. Upstream FileFind сохраняет native enumeration и POSIX
no-follow metadata; FileName — dot/absolute/Windows path bodies. Детерминированные
проверки выполнены на macOS; Windows reparse/drive/UNC runtime acceptance
обязательна и не считается подтверждённой этой проверкой.

P0-02.2b3b2b2 переносит CFSFolder normal/flat enumeration и typed properties.
Ошибка перечисления распространяется наружу и очищает partial/stale snapshot;
исходный bool-only loop мог скрыть ошибку. Runtime окна и видимые controls пока
не меняются. Binding, comments, compressed allocation, link/raw properties
и optional interfaces остаются обязательными.

P0-02.2b3b2b3 переносит binding/parent/notification CFSFolder и read-only logical
drive CFSDrives. POSIX root `/` возвращает null parent, missing/file bind сразу
отклоняется native stat adapter. Windows drive-root возвращает CFSDrives по
исходной ветке; проверка Windows/UNC/device workflows остаётся обязательной.
Physical imaging, icons и COM operations этого модуля ещё не перенесены.
Runtime окна не меняется до P0-02.2b3b3.

P0-02.2b3b2c1 переносит source CAgent proxy ownership/root binding и strong agent
reference у папок. Для текущего 7zz metadata adapter listing immutable: новый
listing получает нового владельца. Open/ReOpen/Close и остальные engine contracts
ещё обязательны. Время жизни descendants/ancestors проверяется после release
root и внешних references; runtime окна пока использует прежнюю lexical ветку.

P0-02.2b3b2c2 добавляет folder factories для следующей runtime integration.
Archive unknown size/packed size остаётся VT_EMPTY, defined zero — числом zero;
source GetSize не добавляет negative sentinel к unsigned directory totals.
Parser presence/type/range и full Properties catalog ещё обязательны P0-03/04.
Видимый workflow не меняется этой фабрикой.

P0-02.2b3b3a переносит GetRealItemIndex через item parameter вместо предположения,
что sorted row равен folder index. Selection/property consumers сохраняют
правильный source item после сортировки, focus остаётся отдельным row cursor.
Layout/text не меняется; fixture parent sentinel проверяется без добавления
dots row в приложение. Full RefreshListCtrl/OnNotify scope ещё обязателен.

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
| Extract, GUI/ExtractDialog.rc/.cpp | Destination, optional folder name, modes слева и password справа; name shown/hidden, 640×360/470×330, история/сохранение/отмена проверены. Отсутствуют абсолютные пути, ElimDup, NtSecurity и Help; Ask адаптирован к отказу при конфликте, а не callback. |
| Create/Add, GUI/CompressDialog.rc/.cpp | Исправлены stacked layout и порядок основной формы: compression слева, encryption справа, source picker после archive/options. 760×720/640×480, 7z/ZIP, show/hide confirmation и reachability проверены. Это частичный порт: нет update/path modes, volumes, SFX, shared/delete options, parameters, CompressOptionsDialog и Help; Add не меняет compression options. |
| Запрос пароля, PasswordDialog.rc/.cpp | QInputDialog заменён source port CPasswordDialog: восстановлен Show Password и исходный ReadControls/SetTextSpec/OnOK. Контракт проверяет все пять controls в masked/shown при 360×170/260×170; Qt integration проверяет reuse/reset/cancel. |
| Настройки, OptionsDialog.cpp и SettingsPage2.rc | Частично перенесены CSettingsPage и CFmSettings: real icons/grid/single-click. Проверены defaults/persistence/confirm/cancel и geometry/JSON на двух размерах. Upstream имеет шесть страниц и дополнительные flags/memory controls; полный порт и ресурсный comparator отсутствуют. |
| Прогресс/сообщения, ProgressDialog2.rc/2a.rc/.cpp, MessagesDialog.rc/.cpp | Qt показывает label, percent и Cancel, сообщения — в журнале. Нет upstream statistics, pause/background, embedded list и подтверждения отмены; journal не имеет Close footer исходного MessagesDialog. Это сохраняющийся долг переноса, не результат ресурсного comparator. |
| Информация об архиве / F3/F4 | Archive Info остаётся отдельной QLabel-формой; F3/F4 — локальный UTF-8 viewer/editor, а upstream запускает настроенное внешнее приложение. Эти окна не подтверждены как порты upstream dialogs. |

Copy и Password проверяются ресурсными контрактами с current-build fixture.
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
OpenBookmark подключены к общему Qt navigateTo. Исчезнувшая папка внутри
уже открытого архива теперь выбирает существующего родителя, как история,
без повторного list. Неперенесённые временные вложенные архивы по-прежнему
не сохраняются в избранное. Старые настройки MVP history/paths и favorites/*
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
В основной файловой панели hover/track-select пока отсутствует.

Полное значение свойства и Alt+Enter истории открывают просмотр текста;
компоновка и взаимодействие описаны в [контракте](ui-layout-contracts.md#full-text-detail).

F5/F6 показывает перенесённую `CPanel::GetItemsInfoString`: число выбранных
папок/файлов, известные размеры, текущий путь и первые пять относительных
имён с многоточием при большем выделении. Неизвестный размер не подменяется
нулём: папки файловой системы до расчёта размера показывают только число.
Папки архива получают размеры через CAgentFolder: в normal это CProxyArc,
а в flat размер папки отсутствует по исходному GetProperty. Числа группируются пробелами
по оригинальному ConvertSizeToString из `PanelListNotify.cpp`.

P0-02.2b3b3c1 removes the Copy/Move row-property snapshot: the active source folder
and original Get_ItemIndices_Operated/GetItemsInfoString provide names and totals.
The prior Qt adapter incorrectly supplied directory totals in flat archive mode;
source VT_EMPTY now leaves that size absent. Missing file sizes remain unknown,
defined zero remains defined, and Cancel dispatches no operation. The Qt display
prefix is temporary glue until source LoadFullPath/parent-stack integration.
Focused 8/8 and full gate 64/64 passed on macOS 2026-09-29. Sixteen normal/compact
source summary reports inspected; Windows/Linux runtime and Windows rendered
parity remain unverified.

Диалог F5/F6 перенесён из `CopyDialog.cpp/.h`: назначения выбираются из
редактируемой истории, Browse нормализует конечный разделитель, Cancel
оставляет Value без изменений. История CopyHistory читает только исходный
ключ; MVP copy/history игнорируется. Совпадения удаляются без учёта регистра,
предел — 20.
Список выделения прокручивается и не переносит строки; это адаптация
SS_LEFTNOWORDWRAP для длинных имён. Структура проверена по `CopyDialog.rc`
и геометрии Qt; пиксельная сверка с Windows и проверка Windows/Linux не выполнены.

Копирование дерева файловой системы использует перенесённые `CopyFolder`
и read/write ветку `CopyFile_Ask` из `FSFolderCopy.cpp` (`0766b73`).
В Qt сохраняются отказ при занятых именах, запрет ссылок и транзакционный
откат при ошибке/отмене. Интерактивный overwrite и межтомный Move не
подключены; перемещение по-прежнему выполняется через rename.

Текущие группировки Create/Extract и адаптации к Qt описаны в
[текстовых спецификациях окон](ui-layout-contracts.md). `ui_geometry`
проверяет порядок, доступность полей при прокрутке и кнопки при нормальном
и компактном размере; JSON-отчёты заменяют обязательные PNG в мигрированных
тестах. Геометрические проверки не подтверждают точное визуальное совпадение
с Windows. Скриншоты используются для неразрешённых вопросов рендеринга.

| Узел 7zFM | Исходник | Состояние Qt и точное расхождение |
| --- | --- | --- |
| Две группы панели Add/Extract/Test и Copy/Move/Delete/Info | `App.cpp`, `g_ArchiveButtons`, `g_StandardButtons`, `ReloadToolbars()` | Порядок, разделитель, цветные символы и подписи перенесены. Символы рисуются средствами Qt; bitmap-ресурсы upstream не включены в проект. |
| Настройки панели: две группы, крупные кнопки, подписи | `App.h`, `ReadToolbar()`, `SaveToolbar()`; `resource.rc`, View → Toolbars | Четыре переключателя в меню «Вид», сохранение в локальном INI через `QSettings`. |
| Заголовок списка и видимые свойства | `PanelItems.cpp`, `GetColumnVisible()`, `ShowColumnsContextMenu()` | Имя, размер, сжатый размер и изменение показаны без служебного полного пути и номеров строк; «Сжатый» скрыт в обычной папке. Видимость доступных столбцов настраивается из заголовка и сохраняется. Набор свойств пока фиксирован, а в 7zFM зависит от папки и формата. |
| Сравнение и сортировка | `PanelSort.cpp`, `CompareFileNames_ForFolderList()`; `resource.rc`, View → Arrange | Сравнение имён перенесено в `src/CPP/7zip/UI/FileManager/PanelSort.cpp`: `2.txt` идёт перед `10.txt`, сравнение без учёта регистра. Qt-таблица вызывает эту функцию из `src/filetable.cpp`. Меню и Ctrl+F3–F7 выбирают имя, тип, дату, размер или исходный порядок. Для архива исходный порядок берётся из загрузки элементов `CAgentFolder`, для обычной папки — из перечисления ОС. Правила `CompareItems()` и `SortItemsWithPropID()` сверены с upstream `0766b73`: папки остаются перед файлами в обоих направлениях, повторный выбор свойства меняет направление, размер/сжатый размер/дата начинают с убывания; свойство и направление сохраняются при обновлении и переходах. В плоском виде сравниваются базовое имя, префикс папки и индекс загрузки, а не отображаемый относительный путь; расширение включает последнюю точку, в том числе у dotfiles и папок (`FSFolder.cpp::GetExtensionPtr`, `Agent.cpp::GetExtension`). Точное сравнение всех свойств и режим сортировки двух панелей 7zFM ещё не перенесены. |
| Контекстное меню списка | `PanelMenu.cpp`, `OnContextMenu()`, `CreateFileMenu()` | Состав зависит от выделения и режима каталога/архива; добавлены команды фона и свойства одного объекта. Нет расширений оболочки Windows, 7-Zip submenu и полного набора операций. |
| Свойства выделения | `PanelMenu.cpp::Properties`, `ListViewDialog.cpp`, `UI/Agent/AgentProxy.cpp::CalculateSizes` | Двухколоночный список свойств, сводка и вложенные counts используют перенесённые методы. Для папок архива размеры берутся из CProxyArc; сводка суммирует все выбранные строки по upstream, включая выбранных потомков. Рекурсивный размер папок файловой системы, форматные и сырые свойства 7zFM пока не включены. |
| Клавиатура списка | `PanelKey.cpp`, `OnKeyDown()` | Enter открывает папку, архив или файл; Backspace, F2–F7, Delete, Ctrl/Cmd+A, Ctrl+N, Alt+F12, Ctrl+F3–F7 и обновление работают в реализованных режимах. F3 показывает текст UTF-8 обычного или архивного файла; F4 редактирует также файл верхнего незашифрованного архива. |
| Главное меню | `resource.rc`; `MyLoadMenu.cpp`, `OnMenuActivating()` | Работают основные «Файл», «Правка», «Вид», «Избранное», включая View/Edit и Open Inside/Outside с ограничениями ниже. Оригинал формирует File динамически и содержит больше команд: Diff, Link и Alternate Streams; также есть Tools и Help. |
| Контрольные суммы | `resource.rc`, `PanelCrc.cpp`, `MyLoadMenu.cpp` | Подменю CRC содержит все 11 методов оригинала. Для выделенных файлов и папок файловой системы запускается `7zz h`; выбранные записи архива сначала извлекаются через backend во временную папку. Вывод и итог доступны в журнале, отмена останавливает текущий этап. В Windows 7zFM отдельное окно итогов и потоковое хеширование архива без временных файлов. |
| Открытие, просмотр и правка файла | `PanelItems.cpp`, `EditItem()`; `PanelItemOpen.cpp`, `StartEditApplication()`/`OpenItemInArchive()` | Enter и двойной щелчок открывают обычный файл системным приложением; архивный файл проходит проверяемое извлечение и открывается из временной папки, сохраняемой до закрытия окна. F3/F4 используют собственный текстовый диалог Qt. F4 сохраняет обычный файл или предлагает обновить выбранный файл незашифрованного верхнего архива через временную копию и backend. Двоичные файлы и объекты больше 16 МиБ доступны через системное открытие, но не через F3/F4. Правка во вложенном или защищённом архиве и обратная запись после внешнего приложения пока недоступны. |
| Открыть внутри / снаружи | `resource.rc`, `MyLoadMenu.cpp`, `PanelItemOpen.cpp::OpenItem()` | Ctrl+PgDn открывает папку или выбранный файл внутри панели, включая вложенные архивы после извлечения во временную папку. Неизвестное расширение при явном открытии проверяет `7zz` по содержимому; обычный двойной щелчок продолжает запускать системное приложение. Цепочка `.tar.gz` открывается последовательным переходом в TAR. Shift+Enter открывает обычный или извлечённый файл системным приложением. Переход назад из корня вложенного архива восстанавливает родительскую папку и выделение. Архивы кроме `.7z`/`.zip` и все вложенные архивы только для чтения. Выбор альтернативного обработчика через `*`/`#` ещё отсутствует. |
| Панели и режимы списка | `App.cpp`, `Panel.cpp`, `SetListViewMode()`; `resource.rc`, View | Крупные/мелкие значки, список и подробности доступны через Ctrl+1–4; режим сохраняется. Все представления делят модель и выделение, поэтому команды действуют на те же записи. Двухпанельный режим ещё отсутствует; раскладка значков Qt может отличаться от Windows ListView. |
| Плоский просмотр | `Panel.cpp`, `ChangeFlatMode()`; `FSFolder.cpp`, `LoadSubItems()`; `resource.rc`, View → Flat View | Вложенные файлы и папки показаны единым списком с относительными путями; режимы файловой системы и архива сохраняются отдельно. Для диска Qt рекурсивно перечисляет записи порциями без прохода по ссылкам; переключение режима или переход в другую папку прерывает обход. Для архива добавляются неявные промежуточные папки. Выделение и операции используют исходные полные пути. Перечисление большого дерева и окончательная сортировка по-прежнему выполняются в потоке UI, но порции ограничены по времени. |
| Адрес и история | `Panel.cpp`, `PanelFolderChange.cpp`, `FoldersHistory()`; `ListViewDialog.rc` | CFolderHistory хранит до 100 каталогов, корней и внутренних папок архивов в INI. Выбор записи восстанавливает папку без повторного чтения уже открытого архива; после нового чтения исчезнувшая подпапка заменяется ближайшей существующей родительской. Временные пути вложенных архивов в историю не записываются. Перенесённый CListViewDialog (Alt+F12) позволяет удалить выделенные записи через Delete, скопировать пути и перейти к текущей; история «Назад» отдельная. Qt показывает границу архива как ` / `; Windows использует разделители пути и системный ListView. |
| Создание файла и выбор по маске/типу | `PanelOperations.cpp`, `CreateFile()`; `PanelSelect.cpp`, `SelectSpec()`/`SelectByType()`; `ComboDialog.rc` | Команда Ctrl+N создаёт пустой файл в обычной папке через редактируемый диалог; в архиве отключена. Правила выбора перенесены в `src/CPP/7zip/UI/FileManager/PanelSelect.cpp`, сопоставление маски — в `src/CPP/Common/Wildcard.cpp`. «Правка» выделяет или снимает выделение по нечувствительной к регистру маске `*`/`?` либо по расширению текущего файла (для папки — все папки). Qt-диалог повторяет поля ComboDialog, но не хранит историю введённых масок. |
| Закладки | `PanelFolderChange.cpp`, `SetBookmark()`/`OpenBookmark()`; `PanelKey.cpp`; `resource.rc`, Favorites | Меню «Избранное» хранит 10 слотов в локальном INI, включая текущую папку архива. Alt+Shift+цифра записывает слот, Alt+цифра открывает его при фокусе таблицы. На Windows 7zFM также поддерживает правый Ctrl с цифрой; здесь используется Alt. |
| Архивные операции | `App.cpp`, `Panel.cpp`, диалоги FileManager | Qt поддерживает создание, добавление в открытый архив, извлечение, тест и основные изменения через `ArchiveBackend`. В Windows сценарии выбора файлов и форматы диалогов шире; здесь действуют ограничения из README. |
| Диалог Add to Archive: сжатие и шифрование | `../GUI/CompressDialog.rc`, `CompressDialog.cpp` | Маски уровней и таблицы методов перенесены из upstream `CompressDialog.cpp`: у 7z уровни 0–9, у ZIP — 0, 1, 3, 5, 7, 9; в ZIP доступен также PPMd. UI и backend используют общие таблицы. Потоки выбираются из исходного ряда с пределами метода/CPU, Auto учитывает память; показаны оценки сжатия и распаковки. Для LZMA/LZMA2 доступны словарь и размер слова, для Deflate/Deflate64 — размер слова, для PPMd — память и порядок, для BZip2 — словарь; для 7z — Auto/Non/Full solid и размер блока. Несовместимые параметры отключаются и отклоняются backend. Пароль вводится дважды; Show Password скрывает повтор. 7z использует AES-256 с отдельным шифрованием имён, ZIP — ZipCrypto/AES-256. Выбор источников остаётся отдельной верхней частью Qt-диалога. Для компактного окна источники и параметры прокручиваются вместе, кнопки подтверждения остаются видимыми. Пока нет томов, режима обновления, режима путей, SFX и произвольных параметров. При добавлении в существующий архив эти параметры не меняются. |
| Диалог Extract | `../GUI/ExtractDialog.rc`, `ExtractDialog.cpp`; `../GUI/Extract.rc` | Перенесены поле назначения с историей из 16 путей, кнопка выбора папки, полные пути или режим без путей, отказ от перезаписи, пропуск, перезапись и оба режима переименования при конфликте, поле пароля с показом. Новая папка назначения создаётся после проверки существующих родителей. Совпадающие имена при извлечении без путей отклоняются до запуска. В upstream доступны также абсолютные пути и запрос решения при конфликте; здесь они ещё не реализованы. Имя подпапки включается отдельным флажком и сохраняется; устранение дублирования корневой папки и восстановление прав пока отсутствуют. |
| Разбить файл / объединить части | `PanelSplitFile.cpp`, `SplitDialog.rc` | Нумерация частей использует перенесённый `CVolSeqName` и `SplitUtils.cpp` (`ParseVolumeSizes`, `GetNumberOfVolumes`, `AddVolumeItems`) в `src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp`. В обычной папке доступны команды для одного файла. Диалог Split содержит папку назначения и редактируемый размер тома; как `CSplitDialog::OnOK()` и `CThreadSplit::ProcessVirt()`, принимает последовательность размеров, применяет их по порядку и повторяет последний для оставшихся частей; Combine принимает первую часть по исходному `CVolSeqName::ParseName` — суффикс `01` с любым числом ведущих нулей, с точкой или без неё, проверяет непрерывность и собирает файл в выбранную папку. Операции идут в рабочем потоке с отменой и временными файлами; файлы с существующими именами не заменяются. Диалог Combine показывает число частей, общий размер в байтах, исходную папку, первые две и последнюю часть, как `CApp::Combine()`. Проверку списка и суммы выполняет `VolumeOperation::inspectParts`; рабочий поток повторно проверяет части перед сборкой. |
| Диалог Copy/Move | `CopyDialog.rc`, `CopyDialog.cpp`; `App.cpp`, `Copy()` | F5/F6 для файловой системы открывают общий диалог с редактируемой историей назначения, кнопкой выбора папки и именами выбранных объектов. Копирование и перемещение выполняются в рабочем потоке с показом текущего файла и отменой; временные результаты убираются при ошибке. F5 из архива открывает диалог извлечения; F6 из верхнего незашифрованного архива использует Copy/Move, извлекает записи, затем удаляет их из архива. История до 20 путей хранится в локальном INI. Перенос между томами, во вложенном или защищённом архиве и операции между панелями пока отсутствуют. |
| Сервис → Настройки | `resource.rc`, `OptionsDialog.cpp`, `SettingsPage2.rc` | Перенесены системные значки файлов, линии сетки и открытие одним щелчком. Диалог применяет параметры при OK и сохраняет в INI; отмена их не меняет. Остальные пункты Settings и страницы System, Menu, Folders, Edit и Language пока не перенесены. |

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

Создание папки/файла, Rename и внутреннее Delete используют исходные методы
`FSFolder.cpp` (`0766b73`). Удаление выполняется в worker, поддерживает отмену
при обходе дерева и сохраняет частичный результат, как внутреннее удаление
7zFM. Панель обновляется после успеха, ошибки и отмены. Вложенные symlink
удаляются сами, их цели не обходятся; непосредственно выбранные ссылки пока
отклоняются. Корзина/системное удаление Windows не перенесены. Диалоги ввода
имени пока остаются Qt-адаптацией, создание через UI принимает одно имя.

1. Перед переносом двухпанельного режима выделить модель панели и навигации из `MainWindow`, затем повторить поведение F9 и операций между панелями из `App.cpp`.
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

`PanelKey.cpp::OnKeyDown()` (`VK_ADD`, `VK_SUBTRACT`, `VK_MULTIPLY`)
перенесён в `mainwindow_selection.cpp::handleSelectionKey()`. Цифровые
`+`/`−` вызывают маску, Shift — выбор всех, Alt — выбор по типу,
`*` — инверсию. Приоритет Alt над Shift и подавление Ctrl+Plus следуют
upstream. Адаптация Qt использует `KeypadModifier`, не перехватывает ввод
в адресной строке и ограничивает команды их текущей доступностью.
Таблица и список значков используют один обработчик; фактические события
цифровых клавиатур Windows/Linux ещё требуют платформенной проверки.

`CPP/7zip/UI/FileManager/PanelKey.cpp::handlePanelKey()` объединяет Qt-адаптер команд
`PanelKey.cpp::OnKeyDown()` и `Panel.cpp::CMyComboBoxEdit::OnMessage()`.
Shift+F4 вызывает существующую CreateFile только при фокусе списка и
доступности команды. Alt+F1/F2 из списка или адреса раскрывают историю,
как `App.cpp::CPanelCallbackImp::SetFocusToPath()`; обе клавиши используют
единственную панель, пока двухпанельный режим не перенесён. Tab из поля
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
и операций переход отключён состоянием команды. Файл адаптера расположен
по исходному пути `src/CPP/7zip/UI/FileManager/PanelKey.cpp`; Win32 уведомления
заменены Qt event filter. Остальные ветви оригинала ещё перенесены не полностью.

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
показывают МиБ/ГиБ. Расчёт Auto и Full Solid перенесены последующей итерацией; Auto оставляет выбор движку.

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
Размеры отображаются в КиБ/МиБ по Combo_AddDict2; снимки 760×720 и
640×480 проверены на macOS offscreen. Windows/Linux и 32-bit не проверены.

PPMd для 7z теперь предоставляет исходный ряд памяти от 1 МиБ до 1024 МиБ
на 64-bit (512 МиБ на 32-bit); Auto показывает 16 МиБ при уровне 5,
256 МиБ при уровне 9. Поле остаётся Dictionary size перед Word size,
как в CompressDialog.rc; подписи локализованы в МиБ. Выбор передаётся
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

## P0-02.1 application navigation data owner (2026-09-28)

History and favorites now live together in upstream CAppState; both are loaded
before menu/address construction. Qt INI storage and immediate confirmed-edit
persistence remain the platform adaptation. GUI-thread ownership replaces
upstream critical sections; no global shared instance. Visible controls, keyboard,
confirmation and cancellation flows are unchanged. This does not close CApp/
CPanel lifecycle or two-panel workflow; Windows/Linux runtime parity unverified.

## P0-02.2a remembered panel focus (2026-09-28)

Source FM.cpp WM_SETFOCUS returns focus through CApp to the last list/address
control remembered by CPanel. Qt uses FocusIn events and current table/icon/list
presentation handles; address Tab routes through the same owner. One visible
panel remains; no layout/control changes. Reference is Panel.cpp focus methods,
CMyListView focus notification and PanelFolderChange combo edit notification.
Windows/Linux native focus behavior and rendered pixel parity remain unverified.

## P0-02.2b1 filesystem refresh focus (2026-09-28)

Reference: PanelItems.cpp SaveSelectedState and RefreshListCtrl selection/focus
portion at 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Filesystem refresh retains
the current item separately from selected files, including flat relative paths.
If the current file disappears, the original row-position fallback chooses a
surviving current item without selecting it. Sorting is already applied by Qt
before this partial restoration; current presentation has a parent toolbar
button and no dots row. Archive refresh and full source list loading remain open.
No layout/text changes; Windows/Linux runtime and rendered parity unverified.

## P0-02.2b2 archive and edit selection (2026-09-28)

Archive refresh and flat-mode changes now preserve current separately from
selected items through the source CSelectedState. Reference: Panel.cpp
ChangeFlatMode and PanelItems.cpp selection bodies at 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
When the current folder disappears, names from it are not selected in the parent.
Reference edit bracket: PanelItemOpen.cpp OnOpenItemChanged captures selection
before updating and restores it afterward. Successful Qt/backend replacement
does so after asynchronous relisting; failure/cancel retains displayed state
because the backend updates a temporary archive. Local edit also uses the same
refresh state. Full source module/lifecycle ports remain open; no layout/text
changes. Windows/Linux runtime and rendered parity unverified.

P0-02.2b3a connects unchanged portable upstream folder/stream/progress interfaces
and property ownership dependencies. It changes no visible workflow or layout;
actual folder binding and navigation/parent-stack bodies remain open. Dependency
tests on macOS do not establish Windows/Linux runtime or rendered parity.

P0-02.2b3b1 replaces the panel property owner with upstream IFolderFolder and
variant property helpers. Copy summary and Properties keep their current visible
values through a read-only Qt metadata adapter; full raw/non-string/archive-level
formatting and actual location binding remain open. Reference: App.cpp summary
and PanelItems/PanelMenu/PanelFolderChange property bodies at the pinned revision.
No text/layout changes; Windows/Linux runtime and rendered parity unverified.

P0-02.2b3b2a ports Agent.cpp _proxy binding and path/property subset behind the
upstream folder interface. Current window enumeration keeps its visible behavior;
long-lived runtime binding waits for owned context. Reference: Agent.cpp folder
bind/parent/prefix/property bodies at the pinned revision. _proxy2, raw/CRC/engine
catalog and operations remain open. No text/layout changes; Windows/Linux runtime
and rendered parity unverified.

P0-02.2b3b3b1 connects owned CAgent/CAgentFolder to the archive window and source
CPanel indices. Archive listing/flat navigation/refresh and nested return share
one source proxy. Source GetProperty VT_EMPTY now leaves size/packed cells empty;
defined zero displays 0, consistent with source PanelListNotify variant formatting.
Grouping, columns and view controls are unchanged. Normal/compact four-view JSON
inspection passed (16 reports); focused 9/9 and full gate 62/62 passed macOS
2026-09-29. Full filesystem runtime, BindToPath/LoadFullPath,
CFolderLink and operation/interface/catalog branches remain mandatory; this
partial owner binding does not establish full module or Windows visual parity.

P0-02.2b3b3b2b replaces the independent recursive Qt iterator with source
CFSFolder loading in a worker. Unlike synchronous source RefreshListCtrl loading,
Qt remains responsive, reports scan counts, then shows rows in bounded batches
after ownership transfer. Cancel/navigation can interrupt either phase. The list
remains disabled until complete; stale results never replace the new location.
Native POSIX names use QFile::decodeName at the Qt boundary, preserving macOS
path/selection behavior without changing source metadata. Progress status now
distinguishes scanned/shown counts; unreadable-folder failure reenables the list.
Four normal/compact progress reports inspected; focused 5/5, ASan 1/1 and full
gate 63/63 passed macOS 2026-09-29. Full CPanel
filesystem binding/normal loading remain mandatory P0-02.2b3b3b2c; Windows/Linux
runtime and Windows rendered parity remain unverified.

P0-02.2b3b3b2c binds normal and flat filesystem runtime to CFSFolder/CPanel and
renders the source item indices and typed sizes; the remaining QDir normal
enumeration is removed. Reference: source FSFolder LoadItems/LoadSubItems and
PanelItems RefreshListCtrl/GetItemName at the pinned revision. Refresh selection
compares source-owned names while Qt display/path conversion retains macOS NFC.
Uncalculated directory size stays blank. POSIX symbolic links now show their own
lstat size/type, whereas the previous QFileInfo view followed target metadata;
explicit path navigation can still resolve a directory link. Flat loading never
follows POSIX directory links. Windows source directory reparse entries keep
their directory flag, but a narrow traversal guard preserves the Qt no-follow
policy and prevents junction cycles. This is an intentional integration difference
from unguarded upstream flat recursion, requiring Windows runtime acceptance.
Columns, control order and layout are unchanged. Full property formatting,
BindToPath/LoadFullPath, parent stack and operation branches remain mandatory.
Focused checks passed 11/11 and full gate 63/63 on macOS 2026-09-29. Four fresh
normal/compact filesystem runtime JSON reports show the path controls, active list
and status bar fully visible. Windows/Linux runtime and Windows rendered parity
remain unverified.

P0-02.2b3b3b3 confirms readonly runtime integration across filesystem/archive
transitions in normal/flat and all four views: source indices survive sorting,
selection does not leak across owners, loaded metadata stays immutable until
reload, and retired workers cannot overwrite an opened archive. No layout or
text changes. Focused 10/10 and full gate 64/64 passed on macOS 2026-09-29;
Windows/Linux runtime remains unverified. Full consumer migration and source
parent/path/interface/operation branches remain mandatory.
