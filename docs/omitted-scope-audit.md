# Решения по исходным модулям

Завершённый перенос сверял полные тела, callers и зависимости исходных UI-модулей.
[Ledger и тематические карты](archive/qt-port/omitted-scope-audit.md) сохраняют
решения по переносу, платформенным адаптациям и неприменимым веткам.
[Итоговое закрытие](archive/qt-port/upstream-port-audit/p3-05-source-closure-2026-10-17.md)
и [приёмка](ux-port-audit.md) задают границы этого свидетельства.

| Область | Карта исходного scope |
| --- | --- |
| Общий реестр и зависимости | [Ledger](archive/qt-port/omitted-scope-audit/ledger.md) |
| FileManager, панели и платформенные services | [FileManager](archive/qt-port/omitted-scope-audit/filemanager.md) |
| Archive providers | [Agent](archive/qt-port/omitted-scope-audit/agent.md) |
| Создание/извлечение и GUI entrypoints | [GUI](archive/qt-port/omitted-scope-audit/gui.md) |
| Диалоги и callbacks | [Dialogs/callbacks](archive/qt-port/omitted-scope-audit/dialogs-callbacks.md) |
| Общие архивные операции | [Common](archive/qt-port/omitted-scope-audit/common.md) |

При изменении компонента сопоставлять эти решения с текущим кодом и upstream.
Исторический статус отдельной ветки не открывает новую задачу. Новые решения
и изменения scope записывать в [source audit](upstream-port-audit.md),
не создавая второй план или счётчик долга.
