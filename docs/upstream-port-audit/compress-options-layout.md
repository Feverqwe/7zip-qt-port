# Compression Options layout — 2026-10-17

Source: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed complete `CPP/7zip/UI/GUI/CompressOptionsDialog.rc`, shared checkbox
macros in `CPP/7zip/GuiCommon.rc`, `CompressDialog.cpp`'s `COptionsDialog`
initialization, precision/time state and event handlers, its declaration and
Options caller. Compared the corresponding port bodies, Qt controls/language
bindings and Add-dialog consumers. TAR fixture capabilities follow upstream
`CPP/7zip/Archive/Tar/TarRegister.cpp`; its selected method is GNU.

Only Qt presentation changes: explicit-value checkboxes keep their natural
height so Cocoa does not separate the indicator from its colon in a tall row;
the precision label uses available-width wrapping instead of inserted line
breaks; NTFS rows use source row spacing with remaining group space below.
Hidden NTFS rows do not impose additional row minima beyond the reserved group.
Dependencies remain Qt Widgets, existing dialog units and language bindings.
No upstream algorithm, state handler, caller or module was replaced or omitted;
no additional source was copied. Fixed source group areas and capability-driven
visibility remain. This review covers this dialog, not a new whole-port audit.

Validation: macOS arm64 / Qt 6.12.0, working tree based on
`b9d13fbca9f43fe3b9971e0a0ef6545b8fb99ec7` plus this change. Focused suites:
`compress_options_presentation`, `compression_advanced`, `compression_timestamp`,
`source_link_storage`. Native Cocoa producers additionally cover EN/RU,
normal/compact, 7z/ZIP/WIM/TAR and real engine-backed TAR:GNU Cancel/OK.
Current widget JSON and affected screenshots are inspected for exposure,
caption fit, grouping and checkbox alignment. Windows/Linux and rendered
original 7zFM comparison are not validated by these macOS checks.

Final focused CTest: 4/4 PASS (6.81s). Native presentation: 18/18 PASS;
native TAR transaction/geometry: 6/6 PASS. All 22 current Cocoa JSON reports
have exposed visible controls and fitting labels; RU TAR requests resolve to
540×509 and 480×509 due to native control minima. Incremental dev/release builds
PASS; `python3 scripts/package.py --install-only dist` refreshed the local app,
and `codesign --verify --deep --strict dist/Sevenzip.app` passed. No full or slow
suite was run for this presentation-only change.

## Компактное окно по запросу пользователя — 2026-10-18

Повторно сверены исходный `CompressOptionsDialog.rc` и перенесённые
`COptionsDialog::CheckButton_BoolBox/SetPrec/SetTimeMAC/OnInit/OnOK`, обработчики
переключателей и Qt presentation. Baseline и upstream repository выше сохраняются.

Пользовательский скриншот показал ненужный пустой резерв скрытого NTFS и неясные
флажки `:`. Разрешённое отличие от фиксированного Windows шаблона:
начальный размер рассчитывается после capability visibility по содержимому;
скрытая группа NTFS и скрытые временные строки не резервируют место.
Ручной размер сохраняется, дополнительная высота остаётся ниже групп.
Видимый NTFS также занимает высоту своих доступных строк.
Флажки явного значения подписаны Set/Задать с пояснением о переопределении
значения формата по умолчанию; descriptive accessible names сохранены.
Windows-only preserve-access скрыт на POSIX, его сохранённое значение не меняется.

Алгоритмы, BoolPair default/explicit state, supported-format branches,
порядок параметров, Apply/Cancel и исходные символы сохранены.
Нового functional scope и omitted source нет; изменения ограничены Qt layout,
подписями и существующей platform boundary. Прежний fixed-size regression
заменён проверкой компактности видимого содержимого; проверка фокуса и
клавиатурного изменения параметров сохранена.

macOS arm64 / Qt 6.12.0: incremental dev и Release target `sevenzip_desktop`
собраны. Focused CTest 5/5 PASS: `compress_options_presentation`,
`compression_advanced`, `compression_timestamp`, `source_link_storage`,
`language_ui`. Native Cocoa presentation 18/18 PASS для четырёх форматов EN/RU;
18 текущих JSON без clipping видимых controls/labels. Просмотрены affected
screenshots, в том числе initial 7z и WIM RU: нет скрытого верхнего резерва,
группы имеют высоту по строкам, подписи и footer полностью видны.
Смена языка проверяет также видимую подпись переключателей явного значения.
Windows/Linux в этой коррекции не запускались.
