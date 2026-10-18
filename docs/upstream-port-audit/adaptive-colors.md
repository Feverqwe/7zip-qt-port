# Адаптивные цвета интерфейса

Дата: 2026-10-18. Запрошенная пользователем Qt presentation адаптация.

Upstream: [ip7z/7zip](https://github.com/ip7z/7zip/tree/9128b80e3a471108678e2b3ec3c985861c8d7b0f),
commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сверены `CPP/7zip/UI/FileManager/PanelListNotify.cpp::OnCustomDraw`,
`PanelSelect.cpp::UpdateSelection`, `AboutDialog.cpp/.h/.rc` и
`HelpUtils.cpp::ShowHelpWindow`, их Qt consumers и исходные цветовые токены
официального CHM CSS. Bodies соответствуют прежнему baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; baseline файлов сохраняется.

Upstream задаёт розовую отметку и красный текст удалённого файла непосредственно
в Win32 custom draw; Help использует HtmlHelp/hh.exe. Пригодной реализации
семантических цветов для Qt и смены палитры в этих модулях нет.
Новый `qtsemanticcolors` ограничен presentation: системная QPalette остаётся
источником фона, текста и выделения. Подложки выводятся из фактического фона,
акценты корректируются для контраста обычного текста. Дополнительные зависимости —
только уже используемые Qt Widgets/Gui и стандартная математическая библиотека.
Код htopQt и MUI не заимствован.

`QtPanelList` передаёт состояние отметки, а `ArchiveTableItem` вычисляет её
BackgroundRole при чтении текущей палитры. Исходные решения о выборе и удалённых
элементах остаются у CPanel. Делегат учитывает фон системного выделения для
цветного текста; таблица и значки используют общий делегат.
About сохраняет ранее согласованную структуру, действия и тексты;
его подложки и ссылки обновляются при PaletteChange.
В списке временных файлов цвет защищённой строки также берётся из текущей
Disabled-палитры при чтении, вместо сохранения устаревающей кисти.

Справка использует Window/WindowText: фон документа, таблиц и viewport совпадает
с окном, подложки команд и заголовков выводятся из него. Цветовые токены
исходного CSS остаются точками подстановки; сами HTML/CSS resources не меняются.
Навигация, история, позиция прокрутки и обработка ссылок сохраняются.
Это продолжает [адаптацию справки](help-theme.md) и
[оформление About](about-product-identity.md).

Нового upstream functional code не копировалось. Алгоритмы выбора файлов,
проверка deleted property и исходные ветви команд не меняются и не опускаются.
Нативные контролы продолжают использовать стиль и палитру Qt; фиксированные
цвета оригинальных пиктограмм не перекрашиваются. Это адаптация цветов,
а не заявление о пиксельном совпадении с Windows 7zFM.

## Проверки

macOS arm64 / Qt 6.12: incremental сборка приложения и targets прямых consumers
успешна. Focused CTest `help_source`, `help_ui`, `panel_deleted_draw`, `panel_key`,
`panel_refresh_completion`, `panel_cell_text`, `temp_cleanup`,
`source_browse_dialog` — PASS (8/8).
Регрессии проверяют смену уже открытой палитры: светлый, тёмный и цветные фоны,
читаемость обычного/выделенного удалённого файла, сохранение отметок,
фон справки как у окна, контраст таблиц/команд/ссылок и обновление About.
В существующих тестах точный розовый RGB заменён проверкой видимости отметки;
source-level deleted policy остаётся отдельно покрыта.

Нативный Cocoa-прогон Help palette/navigation, About live palette и EN/RU
normal/compact identity geometry — PASS (7 QtTest cases с init/cleanup).
Отдельный Cocoa-прогон live marks/deleted-text — PASS (3 с init/cleanup).
Проверены current-build widget JSON справки 760×560/420×300 и About;
нативные отчёты: `build/dev/adaptive-theme-review/`.
Первый Cocoa-запуск в sandbox не имел доступа к экрану; успешные проверки
выполнены вне sandbox. Переключение системных настроек ОС не проверялось:
сценарии меняют QPalette уже открытых нативных окон.
Windows/Linux, full/slow gates и rendered Windows 7zFM reference не проверены.
