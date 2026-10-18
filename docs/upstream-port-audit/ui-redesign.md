# Редизайн интерфейса

Область и последовательность — [согласованный план](../ui-redesign-plan.md).
Это намеренная адаптация представления; полный исходный функциональный scope
и baseline сохраняются в [основном аудите](../upstream-port-audit.md).

## Итерация 1: короткая форма и диагностический отчёт

Дата: 18 октября 2026. Исходник: https://github.com/ip7z/7zip.git,
commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.

Сравнены полные `CPP/7zip/UI/FileManager/ComboDialog.cpp/.h/.rc` и
`MessagesDialog.cpp/.h/.rc` с портом; просмотрены подключения из
PanelOperations (создание/комментарий), PanelSelect (маска), BrowseDialog
(создание папки) и PanelDrag (stack-owned vector сообщений).
Зависимости: исходные UString/векторы, LangUtils, list-view adapter,
Qt font-DLU/screen services и button-box/localization adapters.

Сохранены Combo OnInit/OnOK, порядок history strings и необработанный Value;
Messages AddMessageDirect/AddMessage, включая разбиение строк, OnInit,
modal Create, результаты IDOK/IDCANCEL и начальный Close focus.
Новые функциональные алгоритмы не добавлялись, исходные ветви не исключались.
Изменяется только представление: общая компоновка содержимого и footer,
проектные отступы, нативные размеры кнопок; высота короткой формы по содержимому.
Ручная Win32-подобная геометрия Messages OnSize заменена Qt layout, сам путь
уведомления остаётся. Общий helper не владеет значениями, переводами или действиями.

Повторное сравнение изменённого scope: Combo OnOK и обе функции добавления
сообщений не изменены; виджеты и footer подключены к прежним обработчикам,
Create по-прежнему удерживает исходный modal lifetime. Новых ветвей операций нет.

Проверки macOS: сборка пяти focused targets; CTest `combo_dialog`,
`source_prompt_presentation`, `extract_prompt`, `language_dialog`,
`language_standard_buttons` — 5/5 PASS. После включения существующего review
entry point повторён `combo_dialog` — PASS. Нативный Cocoa запуск geometry/focus
Combo и EN/RU Messages в light/dark — PASS; просмотрены текущие widget JSON
и изображения длинного prompt/диагностики. Reports: `build/dev/ui-redesign-review`.
Первый sandboxed Cocoa запуск не получил экран; unsandboxed повтор прошёл.
Windows/Linux и native sheet/VoiceOver на этом этапе не проверены.
Итерация не означает готовность остальных окон или всего редизайна.
