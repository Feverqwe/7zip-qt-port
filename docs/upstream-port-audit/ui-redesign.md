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

## Итерация 5: File/View/context меню

Источник: https://github.com/ip7z/7zip.git,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сопоставлены resource.rc::IDM_MENU, MyLoadMenu.cpp::CFileMenu::Load,
OnMenuActivating (File/View, включая timestamp examples/UTC),
PanelMenu.cpp::CreateFileMenu/OnContextMenu и вызов plugin command с текущими
портированными телами, QtProgramFileMenu, QtPanelContextMenu и построением меню.
Проверены source filtering, read-only/hash/ADS/small-screen ветви, diff/version
control, hash submenu, plugin ownership и захват вызвавшей меню панели.

Исходные функциональные тела не изменены. Новый QtGroupFileCommands работает
после исходной projection: перемещает существующие QAction в Open as/Create/
Advanced либо context More. Контекстные QAction остаются отдельными копиями,
возвращающими прежний ID через QMenu::exec; центральной диспетчеризацией по-прежнему
владеет CApp. Все hash-алгоритмы сохранены, неизвестные plugin/extension строки
включаются в редкую группу. Системные подменю перемещены целиком с сохранением
внутреннего порядка и прежних callback/owner объектов. Вложенные группы
присоединены к существующему disposable-row lifecycle; исходный плоский
SourceActions остаётся инвентарём для filtering и контекстных копий.

View получает Presentation/Sort by, прежние action groups и checked-state
сохраняются. Подпись Time format устанавливается presentation adapter после
исходного OnMenuActivating; сами актуальные примеры времени, точность и UTC
не менялись. Добавлены восемь EN/RU подписей. Новые назначения клавиш не входят
в эту итерацию. Размер helper рассмотрен: классификация и размещение относятся
к одному представлению, функциональная политика остаётся в CFileMenu.

Повторное сравнение scope: все известные ветви исходного отбора сохранены;
нет новой capability-таблицы и переопределения enabled-state. Старые проверки
плоского расположения заменены поиском по дереву и реальным выбором подменю.
source_context_menu создаёт файл через More в правильной панели для table/icons,
EN/RU; View выбирается через Presentation без смены режима соседней панели.
Повторное открытие, stale plugin actions, source paths/providers и данные
сохраняют прежние проверки. В прямом shell_update consumer обновлён сценарий
раскрытия Add/password, оставшийся от прежней формы; SFX/password/volumes/Cancel
по-прежнему проверяются результатом операции. Failure cleanup закрывает форму,
чтобы ошибка проверки не запускала повторяющийся modal responder.

macOS: приложение и affected targets собраны. main_shell, panel_shell,
source_context_menu, shell_menu_window, file_menu_policy, view_menu, language_ui
PASS; shell_update_window после обновления consumer PASS отдельно (10,50 s).
Первый shell-menu запуск использовал Homebrew 7zz 26.03 с модулем 26.04;
для связанных проверок PATH направлен на существующий согласованный
`build/codec-runtime` 26.04, без пересоздания runtime/cache/build.
Cocoa File/context/View и новые подменю проверены EN/RU normal/compact в
light/dark; реальные nested Create/View команды PASS. Native shell-menu
normal EN / compact RU и stale rebuild PASS отдельно в текущей системной теме.
Проверены текущие widget JSON и снимки в `build/dev/ui-redesign-review`.
Windows/Linux native integration, физические раскладки и VoiceOver остаются
непроверенными; новые shortcuts и остальные семейства окон ещё предстоят.

## Уточнение: шеврон дополнительных параметров

По замечанию пользователя исправлен общий QtDisclosure в Add/Extract. Ранее
native arrow primitive в малом прямоугольнике давал слишком мелкий значок.
Теперь контур шеврона масштабируется по шрифту и поворачивается в общей области;
подпись рисуется с метриками кнопки. Сохранены checkable-кнопка, клавиатурный
фокус, RTL, disabled palette и прежнее управление видимостью содержимого.
Это уточнение presentation-адаптации итерации 2: новых upstream-ветвей нет,
архивные параметры и обработчики не изменены.

macOS: приложение и два прямых потребителя собраны; focused CTest
`source_compress_dialog` и `extract_dialog` — 2/2 PASS. Cocoa EN/RU в light/dark
прошёл для обеих форм; проверены widget JSON и снимки свёрнутого/раскрытого
состояния в `build/dev/ui-redesign-review`. В существующие сценарии добавлен
только report раскрытого состояния, без фиксации пиксельных размеров тестом.
Windows/Linux и VoiceOver для этой правки не проверены.

Повторное уточнение размера: контур уменьшен относительно cap height подписи,
линия стала тоньше; размер кнопки и положение текста сохранены. Меняется только
отрисовка общего QtDisclosure, обработчики Add/Extract и upstream-поведение
не затронуты. Приложение и оба прямых потребителя собраны; focused
`source_compress_dialog|extract_dialog` — 2/2 PASS. Cocoa EN/RU light/dark —
16/16 test functions PASS; актуальные widget JSON и снимки collapsed/expanded
проверены в прежнем каталоге review. Windows/Linux и VoiceOver для этого
уточнения не проверены.

## Итерация 6: современные сочетания

Источник: https://github.com/ip7z/7zip.git,
commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сопоставлены целиком upstream PanelKey.cpp, CMyListView::OnMessage,
CMyComboBoxEdit::OnMessage, основной menu resource и соответствующие Qt dispatch,
command bindings, guards, context copies, language/toolbar consumers. Повторная
сверка сохранила Shift-F5/F6, выбор, clipboard, keypad, сортировку и оставшиеся
bookmark/path variants вне явно согласованных переназначений.

Назначения сосредоточены в Qt-представлении команд по
[схеме](../ui-shortcuts-design.md). Операционные тела CPanel и исходные command
owners не изменены. Новые сочетания файлового списка проходят существующие
closed/busy/provider/modal guards и проверки доступности; недоступное действие
поглощает своё сочетание, не передавая его прежней команде. Inline editor и
адрес сохраняют собственный ввод, Return и Undo. Фокус пути выбирает текст,
не раскрывая историю. Закрытие использует существующий command 8.
Основное назначение отображается самим QAction; toolbar Info получает подсказку
от Properties, поскольку upstream привязывает обе кнопки к IDM_PROPERTIES.

Намеренные различия относительно Windows 7zFM: Command/Control-Shift архивные
действия; macOS Return — rename, Command-Down — open, Command-Backspace — delete;
Control/Command-N и Z освобождены. Windows/Linux Alt-Up/Left — навигация,
синхронизация остаётся через Alt-Shift. Shift-2 переключает панели, сохраняя
Alt-Shift-2 для записи закладки. Удаление без корзины на macOS сохраняется
через исходное действие с Shift, без нового Command-Shift-Backspace.
Полная схема и остальные варианты описаны в её владельце, без дублирования тут.

Native command decoding использует Qt QKeyMapper для букв и цифр текущей
раскладки, не event.text(). Нативный Cocoa-прогон обнаружил исключение Qt:
virtual key 0 физической A не выдаёт варианты. Проверены установленный private
header Qt 6.12.0 и [тело QAppleKeyMapper](https://github.com/qt/qtbase/blob/dev/src/gui/platform/darwin/qapplekeymapper.mm).
Узкая macOS-адаптация разрешает только этот нулевой код через текущую системную
Command-layer (TIS/UCKeyTranslate), без таблицы букв и изменения глобальной
раскладки. Это platform input service, нового upstream-алгоритма операции нет.

Регрессии проверяют ровно один Copy/Move с правильной исходной/целевой панелью,
отмену без изменения файлов, Create/Open/Up/path, две панели, недоступную Add,
редактор имени, reserved keys, clipboard и modal/window isolation. Существующие
archive workflows вызываются новыми Add/Extract/Test сочетаниями с прежними
проверками результата. В прямых потребителях обновлены устаревшие назначения;
modal responders ожидают видимого окна. Синтетические modifiers освобождаются
между взаимодействиями, чтобы не загрязнять следующий сценарий.

macOS: приложение и affected targets собраны; 15 focused-наборов PASS.
Cocoa panel_shortcut в light/dark PASS; native Cyrillic logical-key protocol
для Copy, код A=0, main_shell и language_ui PASS. Сохранены текущие widget JSON
и снимки в `build/dev/ui-redesign-review`. Протокольные события не доказывают
физическую EN/RU клавиатуру; реальная смена раскладки, keyboard без F-ряда,
VoiceOver и Linux desktop conflicts остаются отдельной проверкой.

Linux: 15/15 focused-наборов PASS; после финальных input/fixture изменений
пять прямых потребителей PASS. Windows interactive: первый прогон 13/15,
затем исправлены native Shift-2 (единое декодирование виртуальной цифры с
bookmark path) и ожидание готового QMessageBox через общий widget fixture;
пять затронутых наборов PASS. Native Windows-цифры проверены протокольными
событиями, старые physical right-Control 1/8/9 также прошли через SendInput.
Сценарий удаления здесь проверяет Qt widget-подтверждение, не системную панель.
На Cocoa тесту сохранения закладки исправлено сравнение /var и /private/var
как одной реальной папки; 13 вариантов событий для закладок PASS в обеих темах.
Runners используют прежний current и compiler cache; obsolete source-delete
fixtures после timeout не обнаружены. Full/slow suites не запускались.
Финальная версия теста закладок с каноническим fixture path также отдельно
прошла на Linux и Windows после повторной синхронизации и сборки target.

## Итерация 7: короткие формы ввода и подтверждения

Источник: https://github.com/ip7z/7zip.git. Сохранён существующий baseline модулей
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; при работе сопоставлены текущие
тела и ресурсы с checkout `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Область: FileManager CopyDialog, SplitDialog, PasswordDialog, MemDialog,
OverwriteDialog и LinkDialog, их заголовки/Res/RC, GuiCommon.rc и Qt controls.
Проверены callers CApp::OnCopy/Link, PanelSplitFile Combine, callback password,
AskOverwrite и RequestMemoryUse, window/backend consumers. Используемые
SplitUtils, пути, link services, память/formatting и source result IDs сохранены.

Общая компоновка принимает QWidget footer, поэтому стандартный QDialogButtonBox
и существующая сетка ответов используют одни внешние поля/интервал завершения.
Copy/Move/Combine, Split, Password, Memory и Link подключены к ней. Высота
небольших форм следует содержимому; native button hints заменяют жёсткие размеры.
Copy сохраняет не редактируемую прокручиваемую сводку без Tab stop и исходную
историю; Split даёт размеру тома растягиваться с окном. Source acceptance,
проверка размеров томов, browse normalization и password Show/read/commit прежние.
Основные подписи называют результат действия; EN-источники и RU-переводы
находятся в DialogRedesign, роли и существующие сигналы не подменены.

Memory переставляет Allow/Skip перед настройкой будущего лимита; Remember
остаётся самостоятельным выбором ниже. RAM-dependent defaults/range, независимый
текстовый ввод, проверка нуля/ошибок и Continue/Cancel не изменены. Link использует
QDialogButtonBox с тем же исходным handler, без автоматического accept при ошибке;
порядок кнопок теперь нативный, начальный фокус явно возвращён полю источника.
POSIX Junction/WSL остаются disabled по прежнему
контракту; исходные file/link classification и reparse операции не изменены.

Overwrite показывает existing/incoming metadata рядом. Replace/Skip и варианты
для всех возвращают прежние IDYES/IDNO/extra IDs; Auto Rename и Cancel сохранены.
ShowExtraButtons=false снова скрывает пакетные ответы, как upstream, и укладывает
остальные в одну строку. Сохранены ReduceString, byte/time formatting и source
default No. Полная область кнопок учитывается grid на Cocoa, чтобы native optical
margins не давали пересечение. Второй EXE/ICO icon остаётся прежним omission.

После реализации повторно сверены исходные handlers: новых операционных ветвей
и disconnected consumers в выбранной области нет. Большие BuildPresentation/OnInit
рассмотрены: каждый строит одну форму; алгоритмы остаются в исходных владельцах.
Выделен только уже используемый общий layout, универсальный form engine не добавлен.
Строки, геометрические отношения и source mappings обновлены в существующих
контрактах. Хрупкие проверки исходных DLU frames/пропорций заменены проверками
читаемости, порядка, resize и сохранённого результата, без новых тестов констант.

macOS: приложение и affected targets собраны; 17 focused suites PASS, затем
финальный набор с четырьмя ui_parity контрактами — 21/21 PASS. Отдельно просмотрены ошибки
и declared adaptations всех четырёх результатов: по четыре отчёта, без ошибок.
Cocoa EN/RU normal/compact, начальные Password/Memory, Copy/Move/Combine, Split,
Link, Overwrite/default/batch и hot language change прошли в light/dark.
Просмотрены widget JSON и снимки в `build/dev/ui-redesign-review`.
Новая sheet-модальность не вводилась; существующие modal lifetime/owners сохранены.
VoiceOver и rendered original Windows pixel comparison не выполнялись.

Linux: 21/21 focused suites PASS. Windows interactive с обычным токеном: 20/21,
link_dialog прошёл отдельно с повышенным токеном, требуемым существующими
symlink fixtures. Права учётной записи/UAC/developer-mode для этого не менялись.
Последующая правка начального фокуса отдельно прошла в Link и его window consumer
на macOS/Linux/Windows; Cocoa light/dark также PASS. Финальное скрытое состояние
Memory не оставляет spacer перед footer: четыре прямых потребителя PASS на трёх
ОС, нативные состояния с/без Remember проверены в light/dark.
Сохраняются прежние current/build/cache; до прогонов проверены свободное место
и отсутствие старых payload-каталогов (на обоих гостях только current).
Full/slow suites не запускались.

## Итерация 8: вложенные параметры формата

Источник: https://github.com/ip7z/7zip.git,
существующий baseline COptionsDialog — `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`;
сверка с checkout `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Прочитаны CompressOptionsDialog.rc, CompressDialogRes.h/GuiCommon.rc, полные
COptionsDialog Check/Get/Store_TimeBoxes, GetComboValue/AddPrec/SetPrec,
SetTimeMAC, OnInit/OnCommand/OnButtonClicked/OnOK/OnHelp, объявления и callers
основного Add/Qt advanced adapter. Сверены CArcInfoEx time flags, format profile,
TAR method/ZIP precision branches и вложенная транзакция локальной копии profile.

Изменён только Qt presentation adapter. Сведения о текущем формате/методе
подняты над плоскими группами links/metadata и time. Общая компоновка даёт
согласованные margins/footer и Save/Cancel/Help; удалены фиксированные высоты
строк и ширина precision combo. Полная подпись Set time precision находится
над зависимыми label/combo. Остальные Set/value пары, их accessible names,
source IDs и поддержка скрытых значений сохранены. Новые EN/RU подписи хранятся
в существующих application catalogs.

Нет второй таблицы возможностей форматов: source ShowItem/Enable/Check по-прежнему
определяют видимость и доступность. Не менялись precision items/default/custom,
M/C/A/Z-time decisions, metadata bools, locale rebuild, Help/F1 и OnOK. Cancel
не затрагивает Add draft; Save использует прежний nested commit. Метрики берутся
из Qt controls; исходные labels могут переноситься. Extra height уходит под форму,
не распределяется между группами. Существующий keyboard traversal сохраняет
первый доступный source control и Tab/Shift-Tab. Cocoa group backing получает
один логический пиксель внутри content, исключая обрезание на minimum height.

Повторное сравнение после правки подтвердило подключения прежних handlers и
неизменность их тел. Presentation buildControls остаётся одним сценарием
построения формы; общая геометрия выделена в уже существующий dialog layout,
новый functional owner или form engine не добавлялся. Regression resize проверяет,
что заголовок не уезжает вниз при увеличении окна; существующие tests сохраняют
keyboard enable/accept, Cancel, ZIP Unix/Windows/DOS, TAR, реальные metadata/link
архивы и повторное открытие. Не добавлены проверки констант/наличия исходников.

macOS: приложение и affected targets собраны, семь focused suites PASS.
Cocoa EN/RU 7z/ZIP/TAR/WIM initial/compact, keyboard/precision transitions и hot
language change в light/dark PASS; просмотрены widget JSON и снимки в прежнем
`build/dev/ui-redesign-review`. Windows/Linux: те же семь focused suites PASS;
Windows interactive elevated покрывает действительные WIM security/ADS и link
fixtures. На macOS/Linux Windows-only metadata cases корректно исключены.
Runners сохранили current/build/cache, свободное место проверено; старых payload
каталогов нет. Full/slow, VoiceOver, физические клавиатуры и новый sheet behavior
не проверялись. Остальные семейства окон и общая приёмка редизайна ещё в работе.
