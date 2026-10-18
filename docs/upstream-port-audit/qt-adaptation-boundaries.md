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
