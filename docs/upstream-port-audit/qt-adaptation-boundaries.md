# Qt adaptation boundary review — 2026-10-18

Исходная точка проверки — `5c57c65`, upstream baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` из
<https://github.com/ip7z/7zip.git>. Проверка началась с последних исправлений
режимов списка, выхода коротких процессов и доступности Quit, затем охватила
следующие границы. Сопоставлялись полные затронутые обработчики, их callers,
типы состояния и прямые Qt consumers; [базовый аудит переноса](../upstream-port-audit.md) сохраняет
учёт прежних omissions и платформенных решений.

Команды: source `MyLoadMenu`/`PanelMenu`/`FM` и `CApp` сопоставлены с
QtFileManagerCommands и проекцией QAction. Просмотрены повторная проверка
доступности перед dispatch, capabilities текущего provider/selection,
защита Open/Operation/Scan, восстановление после внешнего CallScope,
toggle rollback и закрытие с modal/source guards. Нового подтверждённого
пробела в этой границе не установлено.

Навигация и список: source BindToPath, OpenParentFolder, SetNewFolder,
OnSetSameFolder/OnSetSubFolder и восстановление выделения сопоставлены с
двумя Qt-представлениями, per-panel projection, asynchronous folder publication
и отменой прежних producers. Проверены generation/lifetime guards,
сохранение owning panel при смене фокуса, source indices и общий selection
model. Исправления доставки hover/Shift описаны в
[аудите событий списка](qt-list-events.md).

События и lifetime: просмотрены QtAppOperationState, отложенные callbacks
списка, FilesystemScanner/QtFolderProjection и close/destruction workflow.
Сверка source FM close/ReleaseApp с Qt quiesce подтверждает порядок join
workers до освобождения provider/temporary inputs, отключение consumers
и отказ устаревших queued deliveries. После исправления событий списка
дополнительного подтверждённого дефекта в просмотренных lifetime-путях нет.

Архивный transport: startup/FailedToStart/exit, completion, cancellation,
multi-stage continuations и controls сопоставлены с source progress/result
owners и их callbacks. Найденные пробелы и узкие Qt-адаптации описаны в
[аудите управления фазами](qt-phase-controls.md) и
[аудите состояния запроса](qt-request-state.md).
Проверка охватывает границы transport; архивные форматы и алгоритмы сохраняют
официальных владельцев.

Настройки и диалоги: source OptionsDialog, PropertyPage apply/reset semantics
и применение значений шести страниц сопоставлены с QtFmOptionsDialog,
сервисами ассоциаций и применением настроек к обоим спискам. Просмотрены
Apply/Cancel, Settings validation refusal, продолжение Apply других страниц
после association error, language notification и сохранение значений скрытых
платформенных настроек. Нового подтверждённого дефекта этой границы нет;
[платформенная видимость](options-platform-visibility.md)
сохраняет прежние необходимые отличия.

Полнота этой source-проверки относится к названным границам и сценариям.
Новых omissions либо disconnected consumers в них после исправлений не
выявлено. Runtime-свидетельства каждого изменения записаны в связанных
аудитах; focused PASS подтверждает соответствующее поведение на указанных ОС.
Рендеренное сравнение с Windows 7zFM, физический ввод и полный/slow gate
в эту проверку не входят.

Итог runtime-проверки: приложение и выбранные targets собраны incremental;
19 focused suites команд/настроек/навигации/lifetime/backend прошли на macOS
arm64 и Linux x64 / Qt 6.12. Windows x64 прошёл 18 из них в том же batch
`20261017T214943Z-5b5325cd`; последний `options_dialog` прошёл после коррекции
хрупкого test-only resize expectation в `20261017T215830Z-4e302ed8`.
[Геометрическое свидетельство](options-platform-visibility.md#переносимость-геометрической-проверки--2026-10-18)
разделяет реальные bounds и запрошенные размеры. Отдельные focused receipts
для controls и событий списка связаны выше. Кэши и постоянные build trees
сохранены. Проверка названных границ завершена; это ограниченная source/behavior
проверка, сохраняющая описанные в базовом аудите платформенные ограничения.

## Проверка предупреждений о владении — 2026-10-18

Дополнительная проверка выполнена после `217c2b0`, по upstream
`9128b80e3a471108678e2b3ec3c985861c8d7b0f` из того же репозитория.
Сопоставлены полные тела Rar5Handler и EnumDirItems с единым форматированием:
у первого различаются notices/includes, у второго также перевод диагностик.
Алгоритмы и их зависимости для этой проверки не изменялись.

Предупреждение анализатора в RAR5 не учитывает связь Create и Code:
оба caller сначала проверяют успешный Create для того же item, а IsEncrypted
и создание filterStream используют одну crypto extra record. COM-владелец
filterStream сохраняет raw filterStreamSpec до завершения распаковки.
Инициализацию указателя для подавления предупреждения не добавляли.

Для EnumDirItems проверены построение/удаление prefix chains, shrink vectors
и consumers в AgentOut, Update, HashCalc и QtArchiveCreationLifecycle.
Успешно опубликованный item ссылается на существующий prefix либо использует
корневой индекс; пустая ветвь удаляет свой prefix без оставшихся items.
Исключение при добавлении prefix прерывает перечисление до использования
результата. Wildcard::CheckPath сохраняет границы индексов через разность длин;
исходные vectors владеют строками на протяжении проверки. Подтверждённого
нарушения владения в этих предупреждениях не установлено.

Отдельно просмотрены QObject-владельцы menu action groups, Help shortcut,
queued callable contexts, отложенные rename/focus/hover события, leased temp
registry и native/Qt file-transfer callbacks. Родители, retained payloads,
контекст доставки и close/join сохраняют соответствующие объекты; уничтожение
окна отсоединяет consumers до освобождения provider. Это проверка названных
границ, а не доказательство безопасности произвольного удаления QObject
из любого пользовательского callback.

Без новых production-правок прошли существующие owned_temp,
owned_temp_publication, archive_creation_lifecycle, native_rar_memory и
source_delete: macOS arm64 5/5 (7,52 с), Linux x64 5/5 (9,68 с), Windows x64
5/5 (45,83 с), Qt 6.12. Guest run: `20261018T124841Z-7bdfff4c`;
Windows link fixtures выполнялись с elevated desktop. Постоянные сборки и
compiler caches сохранены. RAR-проверки подтверждают свои compressed/memory
сценарии; шифрованный RAR здесь проверен по исходникам, без нового runtime
свидетельства. Инъекция отказов, минимальный пример Qt, full/slow gate и
новая визуальная приёмка не выполнялись. Общий аудит проекта продолжается.

## Пакетная распаковка и ожидающие GUI-вызовы

Дополнительно просмотрены полные QtArchiveBatchOpen/QtArchiveBatchExtract,
QtArchiveFolderManager и QtShellArchiveServices с их заголовками, исходные
ExtractGUI/Common Extract и граница join в ProgressDialog2. Сравнение полных
ExtractGUI и Extract с upstream `9128b80e3a471108678e2b3ec3c985861c8d7b0f`
после общего форматирования сохраняет source extraction loop, COM-владельцев,
фильтрацию и учёт томов; отличия — Qt presentation, диагностики и уже учтённые
open/progress/path-policy services. Новых omissions в проверяемом сценарии нет.

Ссылки opener на параметры распаковки, native progress и output policy живут
до завершения исходного worker. Native-open callback отсоединяется scope guard
после Open; retained resource/catalog держат handler и его зависимости.
QtExternalGuiCall, callback dialogs/password и их callers просмотрены вместе
с pending/running/cancelled состояниями: ожидающий worker сохраняет captured
данные до результата либо отменяет ещё не начатую доставку. Таймеры используют
QObject context. Нового подтверждённого дефекта в этих границах не установлено.

Существующие source_extract_backend, archive_open_options и external_gui_call
прошли на macOS arm64 (3/3, 1,24 с), Linux x64 (3/3, 1,21 с), Windows x64
(3/3, 3,79 с), Qt 6.12; guest run `20261018T133638Z-d15df4e8`.
Проверки включают пакетную распаковку, отмену перезаписи и возврат исключения
из GUI в ожидающий worker; прежние результаты других сценариев ими не заменяются.
Сборки incremental и compiler caches сохранены. Новые тесты, инъекция отказов,
реальные пользовательские архивы, full/slow gate и UI appearance acceptance
для этой source-проверки не требовались и не выполнялись.

## Локальные файловые и вспомогательные адаптеры

Полностью просмотрены VerCtrl.cpp и QtVerCtrlFileServices с заголовком.
Сравнение с upstream `9128b80e3a471108678e2b3ec3c985861c8d7b0f` после общего
форматирования сохраняет исходные чтение, сравнение, backup/history,
commit/revert и ветви отказа. Отличия ограничены ранее учтёнными файловыми
сервисами POSIX, настройками и Qt-диагностикой. CByteBuffer владеет прочитанными
байтами; файловые объекты локальны, а переданные временные FILETIME копируются
выходным NIO-владельцем. Нового дефекта времени жизни не установлено.
Существующий verctrl проверен на трёх ОС в составе batch
`20261018T134937Z-6240522b`; общие результаты и ограничения приведены в
[проверке потоков и свойств](archive-close-reopen.md#item-streams-and-property-consumers--follow-up-review).

Дополнительно просмотрены полные небольшие QtNativePanelKeys,
QtDialogScreenServices и QtCallbackProgressTransport с их заголовками:
keyboard candidates используются в пределах выражения, show-фильтр принадлежит
окну и снимается до отложенного удаления, cancel callable принадлежит transport.
ArchiveCodecImage и ArchiveSfxModule используют локальные файловые объекты,
owning строки/контейнеры и retained временную директорию при частично выполненном
копировании. Эти source-проверки не выявили нового подтверждённого нарушения
владения и не добавляют runtime-приёмку иных сценариев этих компонентов.

## Модели списка и обновлённый анализ

Повторно просмотрены полные FileTable/ArchiveTableItem, QtPanelList,
QtFileListHover, QtPanelAddressCombo, QtFolderProjection и QtAppOperationState
с заголовками, публикацией колонок/строк и source consumers PanelListNotify,
PanelSort и PanelSelect. Отложенный hover хранит persistent index и проверяет
его против текущего представления; публикация удерживает folder и прекращает
старое поколение после callback. Таблица получает source-текст в рамках
актуального panel/provider, сохраняя owning QVariant/QString. Адресные таймеры
имеют QObject context; отложенный Rename повторно проверяет исходную папку и
индекс. Нового подтверждённого дефекта в проверенных путях не установлено.

panel_columns, panel_name_text, list_activation, address_combo и
panel_address_keys прошли на macOS arm64 (5/5, 11,24 с), Linux x64
(5/5, 16,53 с) и Windows x64 (5/5, 28,43 с), Qt 6.12, guest run
`20261018T135402Z-8253eb6e`. Использованы существующие incremental targets и
compiler caches. Новых тестов, production-правок или appearance changes нет.

Clang-анализ обновлён для 288 production translation units из текущей macOS
compile database с её SDK/флагами и существующим ccache. Ошибок разбора нет;
результат содержит те же 47 диагностик в 31 единице, что прежний проход, без
новых diagnostic identities. Перемещение MyVector.h учтено при сравнении.
Полные plist-multi-file трассы сохранены также для предупреждений в заголовках:
обычный plist не включает межфайловые пути. Во время основного прохода менялись
два implementation-файла меню; они вместе с межфайловыми диагностическими
единицами перепроверены отдельно (19 units, без ошибок и изменений исходников
во время повторной проверки). Предупреждения по-прежнему оцениваются по
контрактам и callers, а не считаются подтверждёнными ошибками автоматически.

Это обновление macOS diagnostic evidence и проверка названных модельных границ.
Windows/Linux-only translation units не входят в эту compile database;
профильные runtime PASS не доказывают их полного статического анализа.
Новые инъекции отказов, минимальный пример Qt, полный gate, slow suites и
рендеренное сравнение с оригинальным 7zFM не выполнялись. Общая проверка
полноты аудита проекта остаётся незавершённой.

## Согласование областей аудита времени жизни

Проверка использует существующий
[компонентный реестр P2-11](../archive/qt-port/upstream-port-audit/p2-11.md#functionalcomponent-review-register).
Его историческая приёмка сама по себе не подтверждает текущую ревизию.
Последующие проверки владельцев соотносятся с его областями следующим образом:

| Области реестра | Проверенные границы и текущее свидетельство |
| --- | --- |
| P, H — окно, панели, представления | Публикация папки/строк, поколения, индексы и hover рассмотрены выше; [Rename](deferred-rename-owner.md) удерживает исходного владельца, [смена языка после Close](closed-language-event.md) прекращает обращение к освобождённой панели. |
| C, S, K — параметры и создание | Состояние source-диалогов, профили и consumers проверены вместе с DTO; [каталог кодеков](codec-thread-ownership.md) удерживается native workers, [volume writer](multiout-lifetime.md) и Qt-публикация томов имеют владельцев до передачи результата. |
| U, V, R — native open/update/transfer | [Close/ReOpen и свойства](archive-close-reopen.md), пакетная распаковка выше, retained catalog/link и callbacks просмотрены до join; заимствованные имена не переживают исходного handler. |
| J — backend и transport | [Process/listing/overwrite notifications](process-notifications.md), [внешние процессы](external-process-ownership.md), [фазы](qt-phase-controls.md) и [состояние запроса](qt-request-state.md) проверены с отменой, завершением и повторным запуском. |
| W, F — workers и файловые операции | [Ожидание перезаписи](worker-prompt-cancellation.md), [Pause](process-pause-ownership.md), [транзакционный перенос](file-transfer-ownership.md), checkpoints и shutdown проверены вместе с их непосредственными consumers. |
| B — общие зависимости | Общие source object groups сохраняют авторитетные реализации; [object vector](object-vector-ownership.md) и общий COM-владелец кодеков проверены как зависимости нескольких компонентов. |

Платформенные владельцы проверялись отдельно от macOS compile database:
[Shell/GIO/native error buffers](platform-result-ownership.md),
[native drag](native-drag-session.md), [shell menus](shell-menu-ownership.md),
[COM server locks](native-shell-server-lock.md) и [POSIX files](linux-file-close.md).
Просмотрены также Windows association adapter, ADS folder, shell presentation,
AppKit completion и Linux launcher/taskbar consumers; их runtime-ограничения
остаются в соответствующих свидетельствах. Отсутствие platform-only модуля
в macOS анализе не считается его статической проверкой.

После появления новых сочетаний клавиш дополнительно проанализированы десять
новых/изменённых production translation units с текущими macOS SDK/флагами.
Ошибок разбора и изменений файлов во время прохода нет. Три предупреждения
совпадают с ранее разобранными Qt callable/QActionGroup owners; новых нет.
Полные тела нового shortcut adapter и его panel/editor/close consumers
просмотрены: действия сохраняют исходных QObject-владельцев, callbacks имеют
контекст окна. Нового подтверждённого дефекта времени жизни не установлено.

Это согласование source-проверок по областям, а не доказательство отсутствия
всех ошибок. Общая runtime-проверка и разбор её сбоев продолжаются; slow suites,
полное sanitizer-покрытие зависимостей и визуальное сравнение с оригинальным
Windows 7zFM этим аудитом не установлены.

## Проверки Add/Extract после изменения представления

Общий gate выявил проверки прежнего двухколоночного вида и постоянно открытых
полей. Они сопоставлены с текущими конструкторами, исходными CompressDialog /
ExtractDialog resources и обработчиками upstream
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`, а также с
[согласованным редизайном](ui-redesign.md). Производственные алгоритмы здесь
не менялись. Проверки расширенного состояния теперь явно открывают Advanced и
пароль; сохраняются проверки полной видимости, текста, порядка, параметров и
результатов операций. На POSIX проверяется согласованное скрытие неподдерживаемых
Windows-настроек. Прокрутка в source_compress_dialog раскрывает весь combo,
а не только его input-method cursor rectangle.

compression, codec_creation_ui, compression_order, compression_dictionary,
compression_method_state, compression_memory, compression_limit,
compression_solid, sfx_creation, source_extract_advanced, dialog_path_rules,
source_compress_dialog, ui_geometry и language_focus прошли адресные проверки
на macOS arm64, Linux x64 и Windows x64 / Qt 6.12. Linux run
`20261018T143721Z-ea996e90`, Windows run `20261018T144745Z-441ad471`.
Остальные сбои этих batch не считаются приёмкой и разбираются отдельно.
Использованы существующие incremental targets и compiler caches.

Windows дополнительно прошёл native_gui_workflow и source_metadata_storage.
Native cancellation-сценарий раскрывает настройки через UI Automation перед
проверкой доступности полей, затем закрывает форму и проверяет неизменность
частных входов/отсутствие результата. WIM-проверка допускает расширение окна
по native font minima, сохраняя полную видимость контролов, Options/Add
Cancel/OK/restart и фактическое сохранение ADS/security в архиве. Геометрические
проверки и native accessibility не устанавливают rendered pixel parity с
оригинальным Windows 7zFM. Сбои остальных областей общего gate ещё не закрыты.
