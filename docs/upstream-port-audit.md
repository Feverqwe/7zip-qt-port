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

- [Владение shell-подменю при отказе вставки](upstream-port-audit/shell-menu-ownership.md)
  — передача native/Qt menu tree только после успешной вставки в родителя.

- [Редизайн интерфейса](upstream-port-audit/ui-redesign.md)
  — поэтапная адаптация представления с сохранением исходных владельцев и поведения.

- [Владельцы process/listing/overwrite уведомлений](upstream-port-audit/process-notifications.md)
  — срок жизни Qt-транспорта и данных сигнала при удалении, отмене и повторном запросе.

- [Владельцы внешних процессов при исключении](upstream-port-audit/external-process-ownership.md)
  — OS handles, запуск и регистрация editor worker с сохранением временной копии.

- [Поздняя смена языка после закрытия окна](upstream-port-audit/closed-language-event.md)
  — запрет обращения к освобождённым папкам панели из отложенного события Qt.

- [Владение при исключении копирования object vector](upstream-port-audit/object-vector-ownership.md)
  — освобождение уже скопированных владельцев при отказе очередного элемента.

- [Владение дескриптором после ошибки Linux close](upstream-port-audit/linux-file-close.md)
  — сохранение ошибки, защита повторно использованного дескриптора и очистка томов.

- [Владение файлами и состояние volume writer](upstream-port-audit/multiout-lifetime.md)
  — очистка при отказе выделения памяти и корректное усечение предвыделенных томов.

- [Откат Windows Pause при нехватке памяти](upstream-port-audit/process-pause-ownership.md)
  — владение handles и suspend increments до операций, способных выбросить исключение.

- [Владелец отложенного Rename](upstream-port-audit/deferred-rename-owner.md)
  — привязка завершения редактора к исходной панели, папке и элементу списка.

- [Миграция на Qt Linguist](upstream-port-audit/qt-linguist.md)
  — каталоги Qt-текстов, live bindings и границы незавершённой миграции.

- [Адаптивные цвета интерфейса](upstream-port-audit/adaptive-colors.md)
  — общие семантические цвета, фон справки как у окна и обновление открытых видов.

- [Общий каталог кодеков в native workers](upstream-port-audit/codec-thread-ownership.md)
  — исходный атомарный COM-счётчик для владельца, разделяемого окнами.

- [Отмена файлового worker в ожидании перезаписи](upstream-port-audit/worker-prompt-cancellation.md)
  — согласование отмены и пробуждения при завершении операции и окна.

- [Папка архива после отката Move](upstream-port-audit/archive-close-reopen.md)
  — сохранение пути ADS до закрытия native handler на Windows.

- [Application and upstream versions](upstream-port-audit/application-version.md)
  — release identity includes the original 7-Zip source version in About and build metadata.

- [Independent About identity](upstream-port-audit/about-product-identity.md)
  — Sevenzip presentation with a separate upstream attribution and license section.

- [Доступность команд popup и Drop](upstream-port-audit/popup-command-state.md)
  — сохранение состояния контекстного меню и восстановление Copy после отмены Drop.

- [Обновление upstream до 26.04](upstream-port-audit/upstream-26.04.md)
  — перенос source changes, обновление engine/SFX и ресурсов справки.

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

- [Проверка границ Qt-адаптации](upstream-port-audit/qt-adaptation-boundaries.md)
  — source comparison и focused runtime-свидетельства от 2026-10-18.

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
