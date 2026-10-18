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
