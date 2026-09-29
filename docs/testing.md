# Regression strategy

`panel_property` exercises the original PanelMenu typed formatting through a
public IFolderFolder fixture: known/custom catalog names, UI2/UI4/UI8 sizes and
counts, maximum values, CRC, boolean, nanosecond timestamps, absent/empty/zero
values, failed property descriptors/reads, error flags and expected-type warning.
Aggregate counts use the same source formatter. Normal/compact typed-property
dialog reports verify list/header/footer visibility and separation; full values
remain intact while display text applies ListViewDialog truncation/LF rules.

`archive_error_properties` verifies original OpenArchive error-property type
rules (absent versus defined zero, UI4/UI8 narrowing, invalid-type exception),
ExtractCallback flag ordering, encrypted-header password hints and unknown-bit
hex fallback. Resource-generator coverage includes multiple STRINGTABLE inputs
and rejects an invalid later table before overwriting the combined output.

`property_name` checks the original resource/custom-name/numeric-ID priority,
localized source IDs, unchanged English fallback, Unicode custom names, maximum
IDs and resource replacement/append semantics. `resource_strings_generator`
checks source STRINGTABLE conversion in Unicode/space paths, repeatability and
failure before replacing valid output when syntax is unsupported/incomplete.

`property_classification` verifies the source PanelListNotify numeric property
contract: byte sizes, counts and offsets use grouped decimal formatting;
timestamps, CRC, inode/address IDs and error flags do not. It complements
`property_formatting` and `panel_copy_info`; PanelMenu integration is separate.

`property_formatting` checks compiled upstream PropVariantConv/PropIDUtils with
empty, unsigned maximum, signed, boolean and Unicode properties; CRC, attributes,
inode/address formatting; UTC nanosecond precision and zero-time suppression.
Wire-format reparse fixtures verify Unicode targets, every truncated prefix,
reserved-field warnings, invalid offsets and unknown tags. Invalid security data
checks source error/unsupported output. These test portable byte parsers, not
Windows filesystem link/ACL workflows or rendered property dialogs.

`panel_runtime` covers source-folder integration across filesystem/archive
transitions in normal/flat and four views: independent source indices after
sorting, duplicate Unicode basenames, clearing cross-owner selection, immutable
metadata until reload, absent/zero sizes, and retired filesystem workers that
must not publish after archive navigation. It observes public scanner signals and
Qt model values without exposing window internals.
Its archive Move cases verify normal/flat source directory totals versus absence,
missing file sizes, source names and Cancel in four views, with normal/compact
Copy dialog reports. `panel_folder` checks the original operated-index fallback
for selected focus and excludes the parent sentinel; no selection yields no items.

`filesystem_scanner` checks native worker ownership and Qt delivery: ordered
source-index batches, progress, retained folder metadata after scanner cancellation,
restarts during native load/presentation rejecting old generations, pre-start
cancel, active teardown, deletion from callbacks and invalid-directory failure.
`flat_view` exercises live navigation/cancellation at normal/compact sizes and
writes `filesystem-native-scan-*`/`filesystem-presentation-*` widget reports.
`panel_selected_state` checks native filesystem size/absent directory size in
normal/flat runtime; POSIX directory cycles remain single link entries and file
links show their own length rather than target size. It writes normal/compact
`filesystem-runtime-*` reports. Refresh/mode changes preserve independently
selected/current Unicode names (including decomposable characters) in four views.

`file_find` checks the upstream portable enumeration/path services with local
Unicode/non-BMP files and folders, metadata, excluded dots, missing-directory
error versus EOF, source dot resolution and current directory. POSIX symlink
no-follow/follow metadata is covered locally; Windows reparse acceptance remains
required. `filesystem_operations` and `file_transfer` cover existing consumers.
`fs_folder` also cancels native loading before enumeration and during recursion,
injects a progress allocation exception, and verifies no partial/stale records,
ordered count progress and a successful later reload.
`fs_folder` checks normal/flat reload, Unicode, base-relative prefixes, typed
file size/time/attributes versus uncalculated directory size, source type/path/
catalog descriptors, invalid indices and failure clearing stale rows. It also
checks name/index/flat binding, ancestor/root, child flat defaults, missing/file
rejection and WasChanged identity. Windows root-to-FSDrives/rebind is conditional
and still requires execution on Windows.
Its packed-size cases verify normal/flat typed values, cached results after file
removal, missing-before-first-read fallback to loaded size, directory packed zero
versus absent uncalculated Size, and stale-record removal on reload. Windows
native compressed-allocation behavior requires Windows execution; POSIX uses
the explicit source logical-size fallback.
The POSIX source-folder link regression verifies flat no-follow traversal and
typed link metadata. Windows reparse traversal requires Windows runtime acceptance;
QFile::link on Windows creates shortcuts and cannot validate directory junctions.

`folder_contract` validates the unchanged upstream IFolderFolder ABI against a
local mock: interface identity/rejection, reference ownership/destruction,
Unicode property copy/clear/attach/detach, caller-owned property-info BSTR and
missing parent. It exercises portable MyCom/PropVariant dependencies without
opening a real filesystem folder or archive.
`panel_folder` checks the Qt metadata adapter and source panel consumers: typed
Unicode/non-BMP names/prefix, snapshot ownership after caller mutation/release,
missing versus zero size, original numeric-type rejection, missing metadata and
list/selection clearing on release. `panel_copy_info`, `properties` and
`panel_selected_state` validate direct consumers.
Its source-index regression checks reordered Qt rows, a different unselected
current item, typed size/path summary, refresh after another order, missing-row
index gaps and a fixture parent sentinel excluded from operated indices.
`panel_selected_state` checks unknown versus defined-zero archive size/packed
metadata through the runtime in normal/flat and four views. It clears the caller
input and writes current-build reports at normal/compact window sizes, while
checking central-widget visibility.
`archive_mutation` destroys an active backend during helper listing and partial
creation: shutdown must not call completion handlers, leave a temporary archive,
or alter the source file. This reproduces the implicit-destructor lifetime bug
before the explicit QProcess shutdown fix.
`agent_folder` validates the source _proxy folder contract: name/index/ancestor/
root binding, base-relative flat prefixes, duplicate names, Unicode, typed
metadata delegation, folder totals/type, rejected file/missing/invalid bindings
and unbound load failure. Proxy fixtures explicitly outlive their folders until
the borrowed overload is used. Additional owned-Agent cases release input/provider,
root and explicit agent references before descendant/ancestor traversal, and
check empty archive, invalid initialization and immutable repeat-Init rejection.
`location_folder` covers filesystem/archive factories, normalized Unicode binding,
failure output clearing, retained child metadata after input/root release, empty
archive and unknown versus defined zero size/packed size. `archive_proxy` checks
unknown-only and mixed directory totals using the source GetSize conversion.
`archive_proxy`, `flat_view` and
`ui_navigation` validate existing enumeration/window consumers.

`app_state` checks independent CAppState instances, combined history/favorites
Save/Read with Unicode and sparse slots, switching settings paths, normalization,
empty state and preservation of unrelated settings. `folder_history`, `bookmarks`,
`ui_navigation` and `ui_state` cover its window consumers.
`panel_focus` checks independent application/panel focus state, top-level focus
restoration to list/address in all four window view modes, and destroyed/rebound
presentation handles. `panel_key` covers address Tab and Alt+F1/F2 consumers.
`panel_selected_state` checks source snapshot defaults, sorted Unicode relative
names, independent current/selected items, normal/flat filesystem refresh across
four view modes, changed row ordering and fallback when the current file is
removed. Archive cases cover four modes, normal/flat relative paths, duplicate
basenames, a removed folder and failure/cancel snapshot reset. Mode changes
retain current separately from selected items. `file_viewer` checks those two
items separately after successful, failed and cancelled replacement.
These are nonvisual state checks; geometry contracts are unchanged.

`fm_settings` covers CFmSettings defaults and INI keys, invalid values,
unchanged/confirmed/cancelled SettingsPage, preservation of omitted flags,
and normal/compact geometry reports. `settings` exercises window consumers
across disk/archive and all four view modes.
`list_activation` covers settings-driven single/double-click in history,
modified selection clicks, Alt/full-property detail, hover selection on/off,
leaving the list and deleting a hovered row. Fixtures use a temporary INI.

`compression_limit` now owns a fresh temporary settings file on each suite run;
its default assertions do not depend on INI data left by previous runs.

`compression_encryption` checks source password boundaries, method defaults and
format transitions, Show Password hide/restore, header availability, profile/flag
restart, password absence in INI, failed confirmation and Cancel. Real 7z with
Unicode password/hidden names, ZIP ZipCrypto, ZIP AES (99 characters) and an
unencrypted ZIP are extracted for exact Unicode-file bytes; wrong password
cannot list hidden 7z names. Backend rejects AES at 100 characters, ZIP Unicode,
unknown methods and unsupported combinations before staging; cancellation removes
output and temporary files. Geometry covers 760×720/640×480 and both show states,
including long history. Reports: `encryption-*.json` in `build/dev/ui-reports/`.

`compression_path` covers extension replacement (hidden names, dotted directories),
path prefix/fallback/Trim, NUL rejection, history selection, twenty-item cap and
case-insensitive duplicates, settings reset, Cancel and validation failure.
A real Unicode-named archive is created through dialog/backend, tested with 7zz
and extracted for byte comparison. Combo geometry at 760×720 and 640×480 writes
`archive-path-*.json` in `build/dev/ui-reports/`.

`folder_history` checks the upstream 100-path cap, case-insensitive duplicates,
Unicode including supplementary characters, ignored MVP settings, empty persisted
history, multiselection deletion/copy, permission to delete, OK/Cancel and
normal/compact list geometry. Reports are `folder-history-{540,360}.json`.
`ui_navigation` covers persistent archive folders through this history adapter.
`bookmarks` checks CFastFolders sparse slots/empty values, FolderShortcuts
persistence, ignored MVP keys, complete archive paths, missing internal
folders and unavailable archives. `ui_state` retains the keyboard/persistence
workflow checks for favorites.

`password_dialog` checks the source-ported password prompt: initial value/show
state, Unicode and spaces, show/hide without text loss, OK/Enter and Cancel,
normal/compact geometry and report redaction. `ui_parity_password` regenerates
its reports via a fixture and compares all five controls against PasswordDialog.rc.
`ui_archive_states` checks the prompt's backend integration and password lifetime.
`ui_geometry` also rejects a stacked Compress layout and checks that the archive
form precedes the source picker; encryption remains to the right at both sizes.

`ui_parity_tool` tests resource extraction and rejects broken/ambiguous widget
reports, stale baselines and invented evidence symbols. `ui_parity_copy` uses a
CTest fixture to regenerate Copy/Move reports with `copy_dialog`, then checks
their types, selected state properties and resource-derived spatial relations.
See [upstream UI comparison](ui-reference.md) for commands and coverage limits.

`combo_dialog` checks initial Value with ordered Strings, literal prompt text,
raw/empty text via accept/button, Cancel without committing Value,
Dlg_CreateFolder default/Cancel/output, and geometry at 360/260 px for
folder/file/mask prompts. `ui_state` covers creating/canceling folder and file
and selection masks, while `panel_key` covers keyboard consumers. JSON reports
are generated for all six geometry states.

`volume_dialog` проверяет исходные presets, подтверждение через кнопку и
accept(), ошибочный/переполненный размер, Cancel, сводку 2/3/5 частей и
геометрию Split/Combine при нормальном и компактном размере с JSON-отчётами.
`volume_workflow` выполняет Split последовательностью 10/20 и обратный
Combine через команды окна с новыми Unicode-папками, проверяет точные байты,
отказ/подтверждение 100 частей, отмену dialog и ошибку занятого назначения.
`volume_operations` сохраняет worker-проверки успеха, ошибки и отмены;
`combine_names` проверяет имя результата, `copy_dialog` — общего потребителя.

`filesystem_operations` проверяет перенесённые CreateFolder/CreateFile/Rename/
Delete: составной и абсолютный путь, Unicode/пробелы, занятое имя без потери
данных, Rename в родительской папке плоского вида, пустые/скрытые записи,
частичное удаление с ошибкой следующего объекта, отмену до запуска, между
объектами и внутри дерева. Unix cases проверяют вложенные ссылки на папку,
файл и отсутствующую цель без изменения внешнего файла. `ui_operations`
проверяет подтверждение/отказ и обновление панели после worker; `ui_state`
покрывает создание, `file_transfer` — прямого потребителя FSFolder.h.

`copy_dialog` checks OK via both accept() and the button, Cancel, history order,
case-insensitive deduplication, INI migration and explicit empty history,
native-folder-dialog adaptation (normalization and canceled browsing), and
Copy/Move geometry at 440×250 and 340×220. A forty-item Unicode selection
exercises the upstream eleven-line summary and scrolling. Current-build JSON
is in `build/dev/ui-reports/`.
`ui_operations` also checks the persisted `CopyHistory` after actual F5 copying.

`panel_copy_info` checks copied decimal formatting through UInt64 maximum,
mixed file/folder size properties, undefined-size propagation, defined zero,
selection order, relative Unicode names, five-name cutoff and full-selection
totals. `ui_operations` checks the displayed F5 filesystem file and F6 archive
folder summaries, including its proxy size and trailing folder separator.

`project_memory_setup` runs setup in an isolated temporary Git repository,
checks memory/template initialization, then verifies that a populated index,
task note, and attachment survive a second setup unchanged. It requires `sh`
and Git; CMake registers it where `sh` is available. It never touches the real
vault or global Git settings. Run directly with
`sh tests/project_memory_setup_test.sh` or through focused CTest.

`file_transfer` exercises the upstream recursive copy branch with Unicode and
space-containing names, hidden files, empty directories, cumulative byte
progress and occupied destinations. Cancellation during a later source rolls
back both its partial tree and an earlier published file. Existing cases cover
single-file cancellation, nested symlink rejection for copy/move and self-copy.

`extraction_options` checks both rename-on-conflict modes against disposable 7z and ZIP archives with a Unicode filename and spaces. It verifies that the original filename has the requested contents and both versions survive under separate names. `ui_layout` checks that the Extract dialog exposes both modes. Their actual filename suffixes are chosen by `7zz`. `archive_backend` checks creation of a nested Unicode destination and rejection of a linked ancestor.

The regression gate is `./scripts/check.sh`. It checks C++ formatting, builds the development preset, and runs CTest. Run it manually according to `AGENTS.md`; there is no project pre-commit hook, and setup does not install one. Local results do not establish verification on other platforms.

During implementation, build affected targets and run focused tests for changed behavior and direct consumers, for example `ctest --preset dev -R '^compression_preferences$' --output-on-failure`. Expand coverage for shared-component changes or failures. Run the full gate once before completing code, test, or build-tooling changes; rerun affected checks after localized edits and repeat the full gate only for broad changes or unresolved regression concerns. Do not repeat a passing full gate on unchanged code. Documentation and agent-instruction edits need only `git diff --check`.

For layout, visible-text, control-visibility, and resizing changes, start with [view specifications and geometry reports](ui-layout-contracts.md). Run focused checks at normal and compact sizes and review current-build JSON. Screenshots are a fallback for rendering details these checks cannot resolve, or at the user's request. Identify the upstream resource/source or Windows screenshot used for comparison; geometric/structural checks do not establish pixel-level Windows parity. Nonvisual changes need no appearance inspection.

`settings` checks that cancelling Options leaves the file list unchanged, accepting its list options persists them, and a click opens a Unicode folder when single-click mode is enabled. System icon appearance still needs manual checks on each platform.
`compression` checks the upstream level masks (0–9 for 7z; 0, 1, 3, 5, 7, 9 for ZIP), ZIP PPMd archive creation, and that changing the 7z/ZIP format updates available methods, password visibility and confirmation controls, and that mismatched passwords prevent submission. It checks method-dependent dictionary, word-size and solid-block controls, creates disposable archives with these options, lists their Unicode filenames and checks the reported method from `7zz l -slt`. Invalid or incompatible options must fail before creating a target. A compact creation dialog must keep confirmation buttons visible and allow scrolling to the password fields. These checks run locally on macOS; the options and dialog layout still need Windows and Linux verification.
`properties` checks mixed filesystem selection, archive packed sizes, recursive
folder counts, the upstream sum of every selected row (including a selected
descendant), grouped sizes and omitted dates. It writes normal/compact
geometry reports. `list_view_dialog` checks two-column display normalization,
truncation without loss of the stored value, full-value clipboard/detail and
aligned deletion. History/navigation tests cover the shared QTreeWidget adapter.
`edit_dialog` checks Unicode/literal markup, immutable text, Enter/Close and
both scroll directions at normal/compact sizes, with JSON geometry reports.
`file_viewer` checks that Enter or Open sends a Unicode filesystem path to the system file handler, extracts one archive entry before opening it, keeps that temporary copy until window destruction, and does not launch a handler after extraction cancellation. Its file-handler dependency is injected so the test never launches a desktop application.

## What to test

`checksum` verifies a real SHA-256 digest for a Unicode filename with spaces, invalid-method rejection, cancellation of a controlled process, menu availability, and the same selected file hashed from a disposable archive through temporary extraction. The focused test runs locally on macOS; Windows and Linux menu layout and process output require platform checks.

- UI wiring: command availability, selected files, dialog inputs, and user-visible error handling.
- Backend: construct arguments without a shell, parse output and errors, handle progress and cancellation.
- Archive integration: make tiny disposable archives with `7zz`, then verify listing, extraction, creation, wrong passwords, corrupted data, Unicode filenames, spaces, and unsafe paths as each feature is implemented.
- Platform behavior: paths, permissions, encodings, and process launch on each supported OS.

Use CTest for tests and keep fixtures under `tests/`. Use temporary directories for generated archives. A test must assert observable behavior. `archive_backend` tests `.7z` and `.zip` creation, listing, selected and full extraction, Unicode, spaces, nested folders, invalid archives, unsafe paths, missing `7zz` and cancellation. `ui_state` checks command availability and table contents for reopening, an empty archive, a real process launch failure, and cancellation during loading and extraction. It also navigates a temporary filesystem with Unicode and spaces through the address bar, double click and Back, opens an archive from that list, and checks the return path. `application_starts` is only a Qt startup check.

For stage 3.2, `ui_state` checks direct and implicit archive folders, an explicit empty folder, a long Unicode filename, file double-click behavior, Up and Back, numeric column sorting, multiple selection and selected extraction paths after sorting. It also checks reopening, cancellation, failure and empty archives after folder navigation. The actual missing-`7zz` process test remains in `ui_state`; the folder-state cases use a controlled backend for deterministic signal timing.

For stage 3.3, `ui_state` sends Enter, Backspace, and the platform's Select All key sequence to the file table. It checks keyboard traversal of folders and archives with Unicode and spaces, unsupported files, selection, disabled actions while loading or extracting, and restored command availability after reopening, a real `7zz` launch failure, cancellation, and an empty archive. The Open action retains Qt's platform-specific standard Open shortcut; its file dialog is not exercised by the keyboard test.

For stage 4.1, `ui_state` exercises the single creation dialog with a Unicode file and folder, cancellation before submission, command availability during creation and after cancellation, failure, reopening and an empty archive. `archive_backend` checks that cancellation after writing partial output removes the temporary file, a failed `7zz` launch leaves no target, retry succeeds, and an existing target is preserved. The existing `.7z` and `.zip` round trips verify successful creation and listing with Unicode paths and spaces. These checks run locally on macOS; Windows and Linux remain unverified.

For stage 4.2, `archive_backend` updates disposable `.7z` and `.zip` archives with a nested Unicode folder containing a 5 MiB file, then lists and extracts the result. It adds to an empty ZIP, rejects a matching name, and checks that failed `7zz` launch, simulated write failure and cancellation leave the original bytes intact and remove temporary copies. `ui_state` checks the distinct add dialog, empty archive availability, disabled commands during update and reread, and recovery after cancellation, failure and reopening. The Qt test saves an add-dialog screenshot. These checks run locally on macOS; Windows and Linux remain unverified.

For stage 5, `archive_backend` creates disposable `.7z` and `.zip` files with a Unicode filename and space, checks reported format, physical size, file count and original size, runs `7zz t` without writing extracted files, and verifies failure on corrupted archives. It checks selected extraction succeeds when an unselected file conflicts at the destination, and cancellation during a controlled test process. Existing tests cover missing destination, occupied files, unsafe archive paths, failed `7zz` launch and recovery. `ui_state` checks the extraction destination dialog, cancellation, Test and Info availability through reopening, an empty archive and active operations. These checks ran on macOS; Windows and Linux remain unverified.

For stage 6, `archive_backend` creates disposable encrypted `.7z` and `.zip` archives with a Unicode filename and test-only passwords (Unicode for `.7z`, ASCII for `.zip`). It verifies encrypted listing, password prompting, wrong-password errors, successful test and extraction, absence of partial output after a wrong extraction password, rejection of Unicode ZIP AES passwords, and absence of the password in forwarded output. An encrypted creation launch failure leaves no target or temporary file and does not expose the password. `ui_state` checks the creation dialog's mode choices and masked input, cancellation of a password prompt, password reuse during an open archive, clearing it on reopening, disabled add during protected viewing, and empty-archive command availability. Existing state tests cover failed `7zz` launch and cancellation during operations. `./scripts/check.sh` ran on macOS; Windows and Linux stdin and Unicode behavior need separate verification.

For stage 7, `ui_state` exercises filesystem Copy, Move, Delete and Rename with Unicode and spaces, dialog cancellation, occupied destinations and a local-file drop. It checks archive Copy selection, folder Delete expansion, file and folder Rename, local-file drop into an archive, disabled commands while a process runs, rereading after success, and command availability through reopening, a real launch failure, cancellation and an empty archive. `archive_mutation` mutates disposable `.7z` and `.zip` archives, verifies rename collision, explicit and implicit folder renaming and deletion, and checks that a failed `7zz` launch or cancellation leaves the original bytes and no temporary copy. The drag tests synthesize Qt events; interaction with Finder, Explorer or a Linux file manager and actual drag-out remain unverified. Tests run on macOS; Windows and Linux remain unverified.

The Copy/Move dialog layout test checks the editable destination history, folder browse control, selected Unicode filename, and distinct Copy/Move titles. The stage 7 state test submits and cancels this dialog while checking the resulting files and occupied destination errors.

`ui_operations` checks F6 from a top-level archive with a selected folder containing a Unicode filename: extraction receives exact entry paths, failure and cancellation do not start deletion, and successful extraction starts deletion before the archive is relisted. It also creates a disposable real `.7z`, moves a Unicode file through the dialog, and verifies its bytes at the destination and its disappearance from the archive list. Windows and Linux dialog behavior remains for platform checks.

`file_transfer` checks filesystem F5/F6 worker behavior with nested Unicode paths, occupied destinations, a destination inside the source, and cancellation cleanup. `ui_operations` waits for F5/F6 completion and checks that commands are disabled while the transfer runs. These checks run locally on macOS; Windows and Linux remain for CI.

`nested_archive` also opens a real ZIP renamed to `.bin` directly and inside a parent archive, then checks that a nonarchive file with the same extension is rejected and the window returns to its original directory.

For the additional main-window commands, `ui_state` checks cancelled and successful folder creation with a Unicode name, selection inversion and clearing, filesystem refresh after an external file appears, and archive refresh with a retained internal folder and selection. It also verifies fallback when that folder disappears, disables commands during archive refresh, and restores the filesystem state after cancellation or failure. The controlled backend makes the archive timing deterministic; local macOS tests do not establish Windows or Linux shortcut behavior.

For stage 1.2, `archive_backend` also runs a disposable helper process that fragments percent and filename output across writes, ends with an unterminated error, supports cancellation, and emits a 3000-entry listing while a timer checks event-loop responsiveness. `ui_state` checks the visible progress label, nonmodal dialog, accessible last-operation log, failure text, and replacement of the previous log. These tests do not establish performance for archives larger than the fixture or verify Windows and Linux.

For stage 1.3, `archive_backend` feeds disposable listings with parent and drive paths, link metadata, duplicate names, file/directory collisions, and Windows reserved names. It also verifies refusal of missing, linked, and occupied destinations while preserving an existing Unicode filename. The earlier round trips cover successful Unicode paths and spaces. These tests run locally on macOS; Windows and Linux path semantics and filesystem behavior still need their own checks. Concurrent changes between validation and extraction are not covered.

When a bug is found, first capture it with a focused regression test when practical, then fix it. Keep tests deterministic and independent of network access. Report the exact check command and result in the change summary. If a test cannot run on the current host, say why and rely on CI only after its result is available.

## Проверка компоновки Qt

`file_viewer` проверяет редактирование и сохранение UTF-8 файла с Unicode в имени, блокировку двоичного содержимого, F4 из окна файловой системы, F3 и F4 для выбранной записи архива. Управляемый backend проверяет точный путь извлечения, передачу сохранённого текста на замену и удаление временной папки после завершения. `archive_mutation` проверяет реальную замену файла в `.7z` и `.zip` и отказ от небезопасного пути без изменения архива; работа клавиш и диалогов на Windows/Linux остаётся для платформенной проверки.

`ui_state` проверяет 10 слотов избранного из `PanelFolderChange.cpp`: сохранение папки с Unicode и пробелами через клавиатуру, открытие, сохранение внутренней папки архива, повторную загрузку из настроек, блокировку во время загрузки и недоступный архив. Проверка выполнена локально на macOS; сочетания клавиш в Windows и Linux ожидают проверки.

`ui_state` также проверяет числовую сортировку имён `1.txt`, `2.txt`, `10.txt` в обычной папке и архиве по правилу `PanelSort.cpp`.

Команды сортировки проверяются на управляемом списке архива: по типу, дате, размеру и исходному порядку, включая сохранение выбора после обновления. `ui_layout` проверяет, что служебные столбцы типа и порядка скрыты от пользователя. Порядок перечисления файловой системы зависит от ОС и не сравнивается с фиксированным списком.

Для следующих команд 7zFM `ui_state` проверяет создание пустого файла с Unicode и пробелом, отмену диалога без записи, выделение и снятие выделения по маске без учёта регистра, выбор по типу текущего файла и работу маски внутри архива. Отдельный сценарий проверяет удаление записи из диалога истории папок, переход к оставшейся папке и сохранение истории в настройках между окнами. Эти проверки не подтверждают работу сочетаний клавиш и системного оформления диалогов на Windows и Linux.

Для панели команд `ui_layout` проверяет порядок семи действий 7zFM и наличие двух кнопок навигации рядом с адресным полем. `ui_state` запускает общую кнопку «Добавить» в режиме каталога и открытого архива, проверяя выбор соответствующего диалога и блокировку действия во время загрузки. Эти проверки выполнены на macOS; визуальное расположение на Windows и Linux ещё не проверено.

Для углублённого переноса `7zFM` тест `ui_layout` проверяет разделитель групп, цветные значки, отсутствие номеров строк и видимого полного пути, переключение групп панели, размера значков и подписей с восстановлением после нового окна. Он также вызывает контекстное меню заголовка и проверяет сохранение видимости столбца. `ui_state` проверяет состав контекстного меню для выделенного файла, фона каталога и открытого архива, основные поля свойств выбранного файла и переход через выпадающую историю адреса. Тесты хранят настройки в отдельных временных INI-файлах сборки через `SEVENZIP_SETTINGS_PATH`, чтобы не менять настройки пользователя.

Для режимов просмотра `ui_layout` переключает все четыре пункта, проверяет общую модель и выделение, размер значков и восстановление выбора из настроек. Для диалога Extract он проверяет редактируемую историю назначения, доступные режимы путей и перезаписи, пароль и переключатель его видимости. `ui_state` проверяет отмену и передачу параметров извлечения в backend. `extraction_options` проверяет режим без путей, отказ, пропуск и перезапись существующего файла на одноразовых `.7z` и `.zip`, отклонение совпадающих имён, ссылку в назначении и отмену процесса. Компоновка значков и drag-and-drop в реальных файловых менеджерах других ОС требуют отдельной ручной проверки.

`ui_layout` запускает настоящее окно через Qt Test в режиме `offscreen` и проверяет размеры 980×620, 720×480 и 640×400. Тест проверяет видимость панели, адресной строки, таблицы и строки состояния, расположение без наложений и доступность столбцов. При каждом запуске сохраняет PNG в `build/dev/ui-screenshots/` с именем платформы и размера. `ui_state` сохраняет там же снимки открытой папки архива в тех же размерах, диалога создания и журнала. Эти снимки доступны для визуального осмотра. Сравнения с эталонными снимками и автоматического сообщения об изменении картинки пока нет.

После изменения видимой части окна или диалога нужно открыть собранный интерфейс и визуально проверить результат на доступной ОС при обычном и компактном размере. Просмотреть существующие снимки затронутого экрана или сделать новые, если их нет; для условных полей проверить состояния, в которых они появляются и скрываются. Сравнивать группировку и порядок элементов с 7zFM, проверять выравнивание, отступы, ширину подписей и полей, расположение кнопок, обрезание текста и поведение при изменении размера. Снимок надо действительно просмотреть: успешный `ui_layout` подтверждает наличие и часть геометрии элементов, но не качество компоновки. Найденные визуальные дефекты исправлять до завершения задачи; недоступные экраны и платформы перечислять в результате.

`flat_view` проверяет плоский просмотр вложенных файлов и папок с Unicode и пробелами, совпадающие имена файлов в разных папках архива, сохранение точного пути при выбранном извлечении, сохранение выделения при переключении, переход в папку и обновление архива. Отдельно проверяется независимость настроек плоского просмотра файловой системы и архива. Каталог с 1500 файлами проверяет частичное наполнение списка и отмену обхода до завершения. Перечисление очень большого дерева и поведение платформенных ссылок требуют отдельной проверки.

`nested_archive` проверяет команды Open Inside/Outside из 7zFM: реальное открытие `.zip` внутри `.7z` с Unicode и пробелами через `7zz`, возврат с восстановлением родительской записи и удалением временной копии, блокировку изменения вложенного архива, отмену извлечения и ошибку чтения вложенного архива без потери родительского списка. Системное приложение подменяется локальным обработчиком; визуальное оформление и сочетания клавиш на Windows/Linux остаются для платформенной проверки.

Тот же набор создаёт настоящий TAR с Unicode‑именем, проверяет просмотр и извлечение через backend и недоступность команд изменения. Затем он открывает `.tar.gz`, переходит внутрь TAR и возвращается к родительскому архиву. Остальные добавленные форматы зависят от обработчиков установленного `7zz` и требуют отдельных файловых образцов и платформенной проверки.

`volume_operations` проверяет размеры томов, разделение и обратную сборку Unicode-файла с пробелом, запрет перезаписи, пропущенную часть, отмену с уборкой вывода и элементы диалога. `ui_layout` проверяет доступность команд Split/Combine по выделению. Проверка выполнена локально на macOS; системные диалоги и файловые операции на Windows и Linux требуют отдельных прогонов.

Крупные наборы разделены по области: `ui_state`, `ui_operations`, `ui_navigation`, `ui_archive_states` используют общую управляемую подмену backend; `archive_backend` и `archive_mutation` используют общие локальные фикстуры. Упоминания `ui_state` и `archive_backend` выше описывают исходные сценарии; после разделения соответствующие случаи выполняются в этих тематических наборах.

`archive_proxy` checks the adapted upstream directory tree with explicit and implicit folders, Unicode names, recursive size and packed-size totals, real index expansion, flat traversal and reload cleanup. Existing `flat_view`, `properties`, `ui_state` and nested archive tests verify the Qt integration.

`panel_selection` checks the extracted upstream selection rules without widgets:
case-insensitive masks with Unicode and spaces, matching folder names,
last-dot extensions, names without extensions, dotfiles, folder-only selection,
retaining unmatched selected items, deselection, inversion and missing focus.
Existing `ui_state` mask/type and all/invert/clear scenarios check the Qt adapter
in filesystem and archive views; the shared model retains the current index.

`flat_view` additionally checks selection by an exact filename mask and by type
for extensionless files below dotted parent folders. The Qt selection adapter
passes the base name, as `PanelSelect.cpp::GetItemName()` does, rather than the
relative path displayed in flat mode.

`panel_key` checks numeric keypad mask, all, type and invert commands in all
four view modes, both on disk and in a controlled archive with Unicode names
and spaces. It also checks that ordinary `*`, Ctrl+keypad plus, address-field
input and commands during archive loading do not change selection. Qt key
events are synthesized on macOS; physical keypad mapping on Windows/Linux
remains for platform verification.

`ui_navigation` checks persistent history for internal archive folders with
Unicode and spaces, restoring from the address combo and Alt+F12 dialog,
navigation without reloading the current archive, fallback to a surviving
parent and an unavailable archive. Dialog screenshots at 540×360 and 360×240
are saved as `archive-history-540.png` and `archive-history-360.png`.
The existing `file_viewer` scenario also covers window destruction after
visiting an archive folder: address signals are disconnected before Qt's
focus teardown can trigger navigation into a partially destroyed window.

`panel_sort` проверяет правила `PanelSort.cpp` (`0766b73`) через Qt-таблицу:
папки перед файлами в обоих направлениях, повторный выбор свойства,
размер и дата по убыванию при первом выборе, щелчок заголовка размера,
сохранение свойства и направления после обновления архива (включая сжатый
размер), обновления каталога и перехода в дочернюю папку и обратно.
Фикстуры содержат Unicode и пробелы; архивный список подаёт управляемый backend.
Снимки `sort-980.png` и `sort-640.png` сохраняются в `build/dev/ui-screenshots/`.

В `panel_sort` дополнительно проверяются плоская сортировка каталогов и
архивов по базовым именам, числовые префиксы одинаковых имён, направление
при равном размере и точное выбранное извлечение после перестановки строк.
Сортировка по расширению включает dotfiles, имена без точки, конечную точку,
многосоставное расширение и папки с точкой. Снимки плоского списка:
`flat-sort-980.png` и `flat-sort-640.png` в том же каталоге снимков.

`volume_operations` также проверяет сводку Combine для пяти частей с Unicode и
пробелами: число, размер в байтах, первые две и последнюю часть, отказ при
пропуске и отмену диалога без создания результата. Снимки `combine-540.png`
и `combine-360.png` сохраняются в `build/dev/ui-screenshots/`.

Последовательности размеров Split проверяются в `volume_operations`: повторение
последнего размера, три разных размера, неиспользованный хвост, точные байты
всех частей, отклонение нуля и переполнения, отмена с очисткой вывода. Диалог
принимает `10M 20M` и оставляет ошибочный ввод открытым; снимки `split-540.png`
и `split-360.png` сохраняются в том же каталоге.

`panel_key` проверяет Alt+F1/F2 из списка и адреса и Tab из адреса во всех
четырёх режимах просмотра каталога и архива, сохранение текущего пути и
отказ для лишних модификаторов. Shift+F4 проверяется через создание пустого
Unicode-файла, отмену, отсутствие диалога из поля адреса, при чтении архива
и в открытом архиве. Снимки окна и popup адресов при ширине 980 и 640
сохраняются как `address-key-*.png` и `address-popup-*.png`. Физические
клавиши и системные перехваты Alt+F1/F2 на Windows/Linux не проверены.

`settings` проверяет настоящие события мыши с Ctrl, Shift и Meta в режиме
открытия одним щелчком: выделение не меняет папку, обычный щелчок открывает
её. Те же сценарии проверяются при обычном открытии двойным щелчком, включая
доступность Copy после выделения мышью. Проверяются все четыре режима
файловой системы и архива, в фикстурах
есть папки с Unicode и пробелами. Платформенные жесты Windows/Linux требуют
отдельной проверки; локальный прогон использует Qt offscreen на macOS.

`panel_sort` дополнительно проверяет исходный указательный компаратор: ведущие
нули, числа длиннее UInt64, пустые имена, Unicode и ограниченные срезы Qt-строк.

`panel_selection` проверяет ядро масок напрямую: пустой ввод, несколько звёзд,
возврат при поиске совпадения, точное окончание имени, `*.*` для имени без точки,
Unicode без учёта регистра, UTF-16 surrogate pair и срезы без терминатора.

`panel_key::parentKey` проверяет Backspace без модификаторов и с Ctrl/Shift/Alt
во всех четырёх представлениях, в обычной папке и архиве с Unicode-путём.
Тест также проверяет ввод в адресе и блокировку перехода при загрузке; до
исправления все восемь сценариев падали на Ctrl+Backspace. Снимки после
перехода: `build/dev/ui-screenshots/parent-key-980.png` и `parent-key-640.png`.

`volume_operations::combineRollback` проверяет отмену после первого блока,
исчезновение следующей части, увеличение текущей части и занятую цель.
Во всех случаях временный вывод удаляется, существующая цель сохраняется;
изменения частей выполняются синхронно через сигнал прогресса без таймеров.

Новый focused CTest `combine_names` проверяет распознавание и полную сборку
через команду меню для суффиксов без точки, двух/четырёх цифр, Unicode,
имени `.001` и нескольких точек перед номером. Проверяются имя результата,
его байты, сохранность частей и отсутствие временного вывода. Неверные
суффиксы и пропуски отклоняются. До изменения парсер отклонял имена без точки, двухзначные суффиксы
и `.001` без базового имени; дополнительные случаи проверяют имя результата.
Снимки Combine для `.001`: `combine-names-540.png` и
`combine-names-360.png` в `build/dev/ui-screenshots`.

`volume_operations::splitRejectsSourceChange` синхронно уменьшает или увеличивает
источник после первого тома и проверяет отказ с полной уборкой временных
файлов. Проверки последовательности размеров, точного содержимого результата,
существующей цели и отмены покрывают перенесённый `CThreadSplit::ProcessVirt`.

CTest `compression_order` проверяет набор слов LZMA, значение Auto на уровнях
5/9, сохранение выбора и отключение для BZip2. Реальный `7zz` создаёт и
проверяет архивы с 8/48/192; недопустимый размер и отмена не оставляют цель
или stage. До переноса пять сценариев падали. Снимки 760×720 и 640×480:
`compression-order-760.png` / `compression-order-640.png` в `build/dev/ui-screenshots`.

CTest `compression_solid` проверяет границы и промежуточные значения исходного
ряда, сохранение выбора, отключение для ZIP и флажка non-solid. Реальный
архив создаётся и тестируется с блоком 1 МиБ; 3 МиБ отклоняются, отмена
с блоком 32 МиБ убирает временный файл. До переноса оба сценария падали.
Снимки: `compression-solid-760.png` / `compression-solid-640.png`.

`compression_order` также проверяет полные ряды Deflate/Deflate64, Auto на
уровнях 5/7/9, сохранение выбора, Store, реальные ZIP с fb=258/257 и `7zz t`.
Значение 258 для Deflate64 отклоняется до запуска; отмена ZIP не оставляет
архив или stage. Снимки: `compression-Deflate-760.png`,
`compression-Deflate-640.png` и соответствующие `Deflate64` в ui-screenshots.

`compression_order` проверяет ряды PPMd/PPMdZip, Auto на уровнях 1/5/7/9,
сохранение выбора и отключение поля для BZip2. Реальный 7zz создаёт архивы
7z с порядком 2/32 и ZIP с 2/16 и проверяет их командой t. Недопустимые
порядки (9 для разреженного 7z ряда, 17 для ZIP) отклоняются; отмена
убирает цель и stage. Снимки `compression-PPMd-{7z,zip}-{760,640}.png`
сохраняются в `build/dev/ui-screenshots/`.

Focused CTest `compression_dictionary` проверяет малые значения, 3 МиБ,
число вариантов и архитектурный верхний предел, Auto на уровнях 1/5/9,
сохранение выбора между LZMA/LZMA2 и отключение словаря для Store.
Реальные архивы с 64 КиБ, 256 КиБ и 3 МиБ (7z и ZIP LZMA) проходят 7zz t.
128 КиБ отклоняется как отсутствующий в исходном ряду; отмена не оставляет
цель или stage. Снимки `compression-dictionary-760.png` и `-640.png`
сохраняются в build/dev/ui-screenshots. 32-bit предел не проверен локально.

CTest `compression_memory` проверяет ряд PPMd 7z, архитектурный предел,
Auto на уровнях 5/9, сохранение выбора, Store и Auto без явного размера.
Реальный архив с памятью 1 МиБ проходит 7zz t и извлечение с проверкой
байтов Unicode-файла с пробелом. 5 МиБ отклоняется; отмена не оставляет
цель или stage и сохраняет созданный архив. Снимки memory-PPMd-7z-760.png
и memory-PPMd-7z-640.png находятся в build/dev/ui-screenshots.

`compression_memory` дополнительно проверяет отдельный ряд PPMdZip, Auto,
Store и создание ZIP с mem=1 МиБ. После 7zz t проверяются байты извлечённого
файла. Размер 3 МиБ отклоняется для ZIP; отмена сохраняет предыдущий архив
и не оставляет stage. Снимки memory-PPMd-zip-760.png и -640.png.

`compression_memory` покрывает BZip2 в 7z/ZIP: полный ряд, Auto на уровнях
1/3/5/9, сохранение выбора и Store. Архивы с 100 КиБ (7z) и 900 КиБ (ZIP)
проверяются через 7zz t и сравнение извлечённых байтов; 150 КиБ
отклоняется, отмена убирает stage. Снимки memory-BZip2-7z/zip-760/640.png.

`compression_memory` также проверяет фиксированный Auto Deflate/Deflate64
на уровнях 1/3/5/7/9, отключение ручного словаря, создание/извлечение ZIP,
отказ для явного 32/64 КиБ и отмену. Снимки memory-Deflate/Deflate64-zip-760/640.

`compression_solid` покрывает весь список: raw logarithms, Auto/Non/Full,
сохранение Full при смене словаря, отключение для ZIP/Store, исходные
формулы Auto для LZMA2/LZMA/PPMd/BZip2 и границы. Четыре реальные 7z
(Auto, Non-solid, 1 МиБ, Full) содержат два Unicode-файла >1 МиБ:
проверяются число Blocks=1/2/2/1, 7zz t и точные извлечённые байты.
Неверный размер, solid для ZIP/Store и отмена Full не оставляют stage.
Снимки solid-mode-{4294967295,0,64}-{760,640}.png находятся в ui-screenshots.

Focused CTest `compression_resources` использует фиксированные CPU/RAM,
чтобы проверить пределы 2/512/64/1 и ZIP 32/128, CPU×2 ряд, уменьшение
Auto LZMA2/ZIP по памяти и отсутствие уменьшения при неизвестной RAM.
Проверяются численные оценки PPMd/Deflate/Store, сохранение явных потоков,
переход к Auto-only, обновление памяти при смене словаря и UI в трёх режимах.
Пять реальных архивов (LZMA2, PPMd, BZip2, ZIP PPMd, ZIP Deflate) проходят
7zz t и сравнение извлечённых байтов; 513 потоков отклоняются, отмена
удаляет stage. Снимки resources-{PPMd,ZIP,LZMA2}-{760,640}.png.
Побайтовое сравнение подтверждает неизменность всех шести новых reusable
upstream-файлов. Локальная проверка — macOS; остальные платформы требуют CI.

Focused CTest `memory_limit` проверяет перенесённый CMemUse: проценты и
альтернативный p-префикс, байты и K/M/G/T с необязательным B, регистр,
UInt64 maximum, переполнение десятичного ввода и сдвига, ошибочные строки,
сброс ранее разобранного значения, округление и насыщение процентов.
Проверки выполняются через Parse/GetBytes без проверки текста исходника.

Focused CTest `compression_limit` проверяет исходный ряд и его предел,
вставку нестандартных процентов/размеров, Calc_From_Val_Percents с
фиксированной RAM, уменьшение Auto ZIP/LZMA2 и неизвестную RAM.
Проверяются выбор/смена формата/Auto в диалоге, отказ OnOK без закрытия,
последующее успешное подтверждение, реальные 7z и ZIP с лимитом
256 МиБ/80%, 7zz t и точные извлечённые байты Unicode-файла с пробелом.
Предел 1 МиБ отклоняет создание до stage; отмена сохраняет предыдущий
архив и убирает stage. Снимки memory-limit-760.png/-640.png находятся в
build/dev/ui-screenshots. Существующий ui_operations::addToOpenArchiveState
проверяет, что добавление без контролов сжатия продолжает работать.

Focused CTest `compression_preferences` проверяет независимые значения
7z/ZIP при смене формата и повторном создании диалога, сохранение при OK,
отмену после смены обоих форматов, ошибку имени без записи, сброс к Auto,
восстановление нестандартных 37%/320 МиБ, неправильный/переполненный spec,
повторный Load с отсутствующим файлом и сохранность других групп INI.
Тест использует временный Unicode-файл и отдельный settings path;
compression-наборы также получили отдельные SEVENZIP_SETTINGS_PATH,
чтобы не читать и не менять настоящие пользовательские настройки.

Focused CTest `extract_dialog` проверяет исходный порядок overwrite,
OnOK через кнопку и accept(), восстановление режимов/видимости без сохранения
пароля, Cancel, cap истории 16, выбранный и введённый пути, Unicode/пробелы,
Trim и сравнение истории без учёта регистра, legacy history и неверные enum,
сохранность других групп INI. Реальный ZIP проверяет Skip и Overwrite
через результат диалога с точным сравнением байтов. Набор сохраняет снимки
extract-port-640.png и -470.png в build/dev/ui-screenshots. Существующие
extraction_options и ui_archive_states покрывают ошибки/отмену и backend.

Focused CTest `split_destination` проверяет исходный pointer helper:
пустой/корневой/относительный путь, один и два конечных разделителя,
Unicode/пробелы и платформенный backslash. Через GUI проверяются SplitDest
по умолчанию, видимость Edit, Trim/сборка/нормализация назначения, история,
сохранение флажка и Cancel. Реальный ZIP извлекается в новую составную
Unicode-папку с проверкой байтов; занятая файлом цель отклоняется без изменения
файла, Cancel не создаёт папку. Снимки split-dest-{0,1}-{640,470}.png
сохраняются в build/dev/ui-screenshots. Старые GUI-тесты полного назначения
явно отключают новый флажок, как пользователь при выборе полного пути.

`compression_preferences` дополнительно проверяет полный профиль 7z/ZIP:
уровень, метод, словарь, слово, Full solid, потоки и RAM; переключение туда
и обратно, повторное создание диалога, неизвестный метод, неверные численные
значения, Auto sentinel и Cancel. Восстановленный ZIP Deflate level 9/fb=258
создаётся через ArchiveBackend, проходит `7zz t` и извлечение с точным
сравнением байтов Unicode-файла. Снимки compression-preferences-760.png и
-640.png сохраняются в build/dev/ui-screenshots.

`compression_preferences::levelChangeResetsProfile` для 7z/ZIP загружает
профиль PPMd с ручными параметрами, меняет уровень и проверяет исходный
метод/Auto, сохранность лимита RAM, переход к Store и восстановление после
OK. Наборы compression_order/dictionary/memory/solid теперь ожидают сброс
ручных спецификаций при смене уровня; для проверки рядов других методов
после смены уровня метод выбирается снова. Это фиксирует отличие от
прежней самостоятельно написанной политики Qt.

`methodAndDictionaryDependencies` проверяет профиль LZMA2 → LZMA → LZMA2,
сброс чужих параметров, восстановление профиля, Auto solid после смены
словаря, сохранность слова/потоков и специальных solid 0/64. Cancel оставляет
INI неизменным. Снимки Store 7z/ZIP после смены уровня:
`level-reset-{7z,zip}-{760,640}.png` в build/dev/ui-screenshots.
