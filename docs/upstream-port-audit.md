# Происхождение перенесённого кода

Машиночитаемый [baseline](upstream-baseline.json) хранит исходный репозиторий,
полные ревизии и принадлежность заимствованных файлов. Проверка и обновление —
в [upstream update](upstream-update.md); требования к source port —
в [AGENTS.md](../AGENTS.md#source-port-requirements).

## Дополнительные форматы

[Read-only Brotli](upstream-port-audit/brotli.md) — Google C decoder,
адаптация format handler, границы распаковки `.br` и проверки.

[Read-only exFAT из Linux](upstream-port-audit/exfat-linux.md) — исходная ревизия,
границы C-переноса, адаптации потокового ввода и проверки отдельного модуля.

## Базовый аудит завершённого переноса

[Индекс Qt-портирования](archive/qt-port/upstream-port-audit.md) сохраняет
copied/adapted/omitted scope, зависимости, причины адаптаций и датированные
проверки. Архивирование фиксирует завершённую работу; эти сведения остаются
основанием для ревью изменений соответствующих модулей.

- [Решения по bodies/callers](omitted-scope-audit.md).
- [Закрытие исходного scope](archive/qt-port/upstream-port-audit/p3-05-source-closure-2026-10-17.md).
- [Последующая коррекция modal owner](archive/qt-port/upstream-port-audit/p3-03-owner-publication-2026-10-17.md).
- [Итоговая приёмка и границы свидетельств](ux-port-audit.md).

Искать старое обоснование по upstream-пути или символу:

```sh
rg -n --no-ignore -F 'CAgent::Open' docs/archive/qt-port/upstream-port-audit/ docs/archive/qt-port/omitted-scope-audit/
```

## Новые изменения

- [Состояние нового архивного запроса](upstream-port-audit/qt-request-state.md)
  — независимый результат отказа и повторное извлечение после отмены.

- [Отложенные события файлового списка](upstream-port-audit/qt-list-events.md)
  — отмена hover при блокировке и завершение Shift-выделения до следующего нажатия.

- [Управление асинхронными фазами](upstream-port-audit/qt-phase-controls.md)
  — сохранение Pause при смене CLI-процесса или worker.

- [Режимы файлового списка](upstream-port-audit/small-icons-layout.md)
  — Small/Large Icons, размеры по шрифту, активная прокрутка и задержка наведения.

- [Фоновый приоритет завершившегося процесса](upstream-port-audit/background-process-exit.md)
  — повторная проверка архива после включения фонового режима.

- [Видимость платформенных настроек](upstream-port-audit/options-platform-visibility.md)
  — скрытие неподдерживаемых действий с сохранением исходных значений и Apply/Cancel.

- [Относительные ссылки справки](upstream-port-audit/help-relative-links.md)
  — переходы из Add к описаниям Method и SFX внутри корня справки.

- [Тема встроенной справки](upstream-port-audit/help-theme.md)
  — адаптация цветов исходных страниц к палитре приложения.

- [Стабильные колонки Add](upstream-port-audit/add-dialog-stable-columns.md)
  — смена параметров без сжатия окна и перемещения остальных полей.

- [Открытие перетащенного архива](upstream-port-audit/archive-drop-open.md)
  — запрошенное отличие от исходного действия упаковки внешних файлов.

- [Выход через меню macOS](upstream-port-audit/macos-quit-menu.md)
  — системная роль существующей команды выхода.

- [Проверка геометрии остальных диалогов](upstream-port-audit/dialog-layout-review.md)
  — native macOS EN/RU, resource-to-widget contracts и коррекция подписи Zone.Id.

- [Язык главного меню](upstream-port-audit/menu-language.md)
  — исходный перевод переключателя двух панелей при запуске и смене языка.

## Проверка границ Qt-адаптации — 2026-10-18

Исходная точка проверки — `5c57c65`, upstream baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` из
<https://github.com/ip7z/7zip.git>. Проверка началась с последних исправлений
режимов списка, выхода коротких процессов и доступности Quit, затем охватила
следующие границы. Сопоставлялись полные затронутые обработчики, их callers,
типы состояния и прямые Qt consumers; базовый аудит переноса выше сохраняет
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
[аудите событий списка](upstream-port-audit/qt-list-events.md).

События и lifetime: просмотрены QtAppOperationState, отложенные callbacks
списка, FilesystemScanner/QtFolderProjection и close/destruction workflow.
Сверка source FM close/ReleaseApp с Qt quiesce подтверждает порядок join
workers до освобождения provider/temporary inputs, отключение consumers
и отказ устаревших queued deliveries. После исправления событий списка
дополнительного подтверждённого дефекта в просмотренных lifetime-путях нет.

Архивный transport: startup/FailedToStart/exit, completion, cancellation,
multi-stage continuations и controls сопоставлены с source progress/result
owners и их callbacks. Найденные пробелы и узкие Qt-адаптации описаны в
[аудите управления фазами](upstream-port-audit/qt-phase-controls.md) и
[аудите состояния запроса](upstream-port-audit/qt-request-state.md).
Проверка охватывает границы transport; архивные форматы и алгоритмы сохраняют
официальных владельцев.

Настройки и диалоги: source OptionsDialog, PropertyPage apply/reset semantics
и применение значений шести страниц сопоставлены с QtFmOptionsDialog,
сервисами ассоциаций и применением настроек к обоим спискам. Просмотрены
Apply/Cancel, Settings validation refusal, продолжение Apply других страниц
после association error, language notification и сохранение значений скрытых
платформенных настроек. Нового подтверждённого дефекта этой границы нет;
[платформенная видимость](upstream-port-audit/options-platform-visibility.md)
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
[Геометрическое свидетельство](upstream-port-audit/options-platform-visibility.md#переносимость-геометрической-проверки--2026-10-18)
разделяет реальные bounds и запрошенные размеры. Отдельные focused receipts
для controls и событий списка связаны выше. Кэши и постоянные build trees
сохранены. Проверка названных границ завершена; это ограниченная source/behavior
проверка, сохраняющая описанные в базовом аудите платформенные ограничения.

- [Понятные подписи открытия](upstream-port-audit/open-menu-captions.md)
  — уточнение названий Qt-меню с сохранением расположения и исходных команд.

- [Разметка дополнительных параметров архива](upstream-port-audit/compress-options-layout.md)
  — выравнивание флажков, перенос подписи и исходные строки NTFS.

- [Выбор архива через меню File](upstream-port-audit/open-archive-picker.md)
  — запрошенное расширение интерфейса и повторное использование открытия архива.

- [Относительные пути с `.` при распаковке](upstream-port-audit/dot-extraction-paths.md)
  — исправление предварительного отказа для CPIO payload и повторное использование
  исходной коррекции путей.

При изменении перенесённого модуля добавлять сюда ссылку на отдельную запись
в `docs/upstream-port-audit/`. Указывать дату, retrievable source repository,
полный commit, пути/символы, copied/adapted/omitted scope, зависимости и причины
адаптаций. Для обновления upstream сохранять прежний baseline и локальные fixes.
Runtime-проверки записывать с ревизией, ОС и фактически проверенным scope,
отдельно от полноты переноса исходников. Изменившееся поведение описывать
в [parity](7zfm-parity.md), границы компонентов — в [архитектуре](architecture.md).
