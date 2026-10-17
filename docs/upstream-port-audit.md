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

- [Выход через меню macOS](upstream-port-audit/macos-quit-menu.md)
  — системная роль существующей команды выхода.

- [Проверка геометрии остальных диалогов](upstream-port-audit/dialog-layout-review.md)
  — native macOS EN/RU, resource-to-widget contracts и коррекция подписи Zone.Id.

- [Язык главного меню](upstream-port-audit/menu-language.md)
  — исходный перевод переключателя двух панелей при запуске и смене языка.

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
