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

## Итерация 2: Add и Extract

Источник: тот же официальный repository/commit. До правки сопоставлены исходные
`GUI/CompressDialog.cpp/.h/.rc`, `ExtractDialog.cpp/.h/.rc`, их OnInit/OnOK,
FormatChanged, password/encryption и OnCommand-ветви с текущими адаптерами.
Проверены PresentSource/sourceInfo, UpdateGUI ShowDialog, ExtractGUI invocation,
переключение профилей, memory/volume validation и вложенный COptionsDialog.
Зависимости/существующие adaptations — в исходном baseline-аудите этих модулей.

Сохранены исходные расчёты, codec capability owner, метод/словарь/потоки/память,
имена архивов, история путей, параметры времени/метаданных и транзакция вложенных
параметров. Win32-контролы по-прежнему заменяются Qt; новых архивных алгоритмов
и таблиц возможностей нет. Повторно проверены подключения исходных валидаций
и итоговых DTO после изменения компоновки.

Новая общая часть — QtDisclosure: только раскрытие/сводка, без изменения значений
и enabled-state содержимого. Add переносит настройки в одну форму с постоянно
видимыми format/level и сворачиваемыми дополнительными параметрами. Extract
аналогично группирует путь/перезапись/пароль/дополнительные настройки. Опциональный
пароль — согласованная UI-адаптация: переданный пароль раскрывается автоматически;
выключение исключает его из запроса, не стирая черновик. Поддержка шифрования
остаётся у исходного GetFormatInfo; capability EncryptHeadersIsAllowed отделена
от текущего UI enable-state. Исходная password validation получает активные
значения либо пустую пару при явном выключении защиты.

QtLangButtonBox поддерживает локализуемую подпись основного действия, сохраняя
исходную кнопку/роль/сигналы и повторный перевод. Для Add/Extract это Create archive
и Extract. Новые EN-строки и RU-переводы находятся в контексте ArchiveRedesign.
Сводки не содержат паролей и произвольного текста дополнительных параметров.

Пересмотрены проверки, закреплявшие старые колонки и постоянную видимость всех
полей: теперь проверяются раскрытие, доступность через прокрутку, читаемость
и прежние результаты операций. Добавлены EN/RU regressions на сохранение опций
при сворачивании и сохранение/исключение password draft. Проверки исходных
профилей, ошибок, сохранения и фактических архивов сохранены.

Размер существующих presentation-модулей рассмотрен: формы остаются едиными
сценариями построения UI, а алгоритмы и зависимости уже находятся в исходных
модулях и отдельных adapters. Новая общая часть выделена отдельно; универсальный
движок форм и новый владелец настроек не вводились.

Проверки macOS: сборка приложения и затронутых targets PASS; focused CTest
`source_compress_dialog`, `extract_dialog`, `compression_encryption`,
`compression_advanced`, `language_standard_buttons`, `language_dialog`,
`compression_event`, `compression_profile_selection`, `compression_error_policy`,
`compression_resources`, `source_archive_browse` — 11/11 PASS.
Нативные Cocoa EN/RU Add/Extract и compact RU Add в light/dark — PASS;
проверены текущие widget JSON и снимки в `build/dev/ui-redesign-review`.
Исправлена начальная высота Add: пересчёт height-for-width выполняется после
инвалидации layout, чтобы сводка помещалась сразу после открытия.
Заголовок раскрытия рисуется средствами текущего Qt style без постоянной панели
кнопки; размер стрелки ограничен отдельно от области нажатия.
Windows/Linux, VoiceOver и новая sheet-модальность в этой итерации не проверены.
Вложенный COptionsDialog сохраняет прежнее представление и исходную транзакцию;
его отдельный редизайн и остальные окна остаются в scope следующих итераций.

## Итерация 3: изменение размеров двух панелей

Источник: https://github.com/ip7z/7zip.git,
commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сопоставлены полные тела `UI/FileManager/FM.cpp::CSplitterPos`, WM_CREATE,
WM_LBUTTONDOWN/UP, WM_MOUSEMOVE, WM_SIZE, MoveSubWindows и SaveWindowInfo
с портом FM.cpp, QtFileManagerSplitter и текущими create/toggle/close adapters.
Проверены CApp::SwitchOnOffOnePanel и Qt-публикация видимости панелей.

CSplitterPos, его bounds/ratio/rounding и существующее CWindowInfo-хранилище
сохранены без изменений. Дополнение относится к presentation/input:
клавиатурный фокус, Left/Right (Shift — крупнее), Return и двойной щелчок для
равных долей. Этих keyboard/reset handlers в исходном splitter нет;
это прямо согласованная адаптация редизайна, а не новая реализация его расчётов.
Используются QSplitterHandle и системный focus primitive; EN/RU имя и tooltip
проходят через текущий переводчик. Модели и навигация не пересоздаются.

Qt minimum может временно ограничить рассчитанную позицию. При переключении
панелей такая применённая геометрия больше не записывается поверх исходной
предпочтительной пропорции. Явные drag/keyboard изменения используют SetPos.
Повторная сверка scope: исходные расчёты/ветви сохранены, обработчики подключены
к тому же экземпляру splitter, окно сохраняет позицию прежним путём.

macOS: приложение и три focused targets собраны; CTest window_placement,
panel_key, panel_shell — 3/3 PASS. Cocoa keyboard/reset/minimum/toggle и
normal/compact окно проверены в light/dark; pointer drag сохраняет фокус,
выделение, paths и модели обеих панелей. Проверены текущие JSON и RU-снимки
главного окна: обе status strips выровнены, узкие таблицы прокручиваются.
Native проверка выявила различие TabFocus/StrongFocus на macOS: сохранён
StrongFocus для обхода с клавиатуры, pointer press возвращает прежний фокус.
Reports остаются в `build/dev/ui-redesign-review`.
Windows/Linux runtime, физическая клавиатура и VoiceOver пока не проверены.
Toolbar, меню и современные сочетания из этапа 4 остаются следующей работой;
эта итерация не означает завершения главного окна.

## Итерация 4: компактный toolbar главного окна

Источник тот же: https://github.com/ip7z/7zip.git,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
До правки сопоставлены App.cpp::CButtonInfo, g_StandardButtons,
g_ArchiveButtons, оба SetButtonText, AddButton, ReloadToolbars,
SaveToolbarChanges и tooltip-ветви OnNotify с портом App.cpp и текущими
mainwindow_menus/preferences/language adapters. Проверены AttachActions,
AttachFileActions, refreshActions и CopyTo workflow; команды и source IDs
остаются общими для toolbar и меню. Сохранены исходные bitmap assets,
mask/default/visibility/size preferences и их storage owner.

Согласованная presentation-адаптация: текст beside icon только у Add/Extract,
остальные пять кнопок — icon-only. Выключение сохранённого labels-флага скрывает
также эти подписи. Добавлены доступные имена, native shortcut в tooltip и
StrongFocus; autoRaise оставляет hover/focus текущему стилю Qt. Подпись Extract
использует уже существующий ArchiveRedesign-перевод, общий с Extract-диалогом.
Существующее подключение QAction::changed обновляет подписи/подсказки;
второе подключение и новый command dispatcher не добавлены.

Повторная сверка: source button tables, capability/operation handlers и toolbar
state не менялись; все семь прежних QAction остаются теми же экземплярами.
Заменены хрупкие проверки имени ресурса/доступных bitmap размеров/углового
пикселя на проверку отображения значков и доступности всех кнопок на экране.
EN/RU проверка фиксирует согласованное различие двух основных и пяти остальных
действий; language_ui проверяет актуальность tooltip после смены языка,
выделения и shortcut. Keyboard Space вызывает один исходный Copy-диалог
с файлом правой панели и назначением в левой; Cancel не создаёт копию.

macOS: сборка приложения и direct consumers PASS; focused main_shell,
panel_shell, view_menu, language_ui PASS. Cocoa EN/RU normal/compact,
one/two panels и keyboard Copy/Cancel проверены; main_shell и language_ui
повторены в light/dark. Текущие JSON подтверждают видимость всех семи кнопок
при ширине 640, снимки toolbar просмотрены. Reports — прежний
`build/dev/ui-redesign-review`. Временный compile failure в параллельном
file-transfer изменении исправлен его владельцем; повторная сборка прошла.
Windows/Linux, физические раскладки и VoiceOver пока не проверены.
Перегруппировка File/View/context меню, новые основные сочетания и окончательный
выбор современных иконок остаются в дальнейшей области редизайна.
