# Geometry and layout suite coverage

[Индекс](../testing.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

## Проверка компоновки Qt

Независимые пункты текстового просмотра/правки и тест `file_viewer` удалены:
исходный 7zFM использует внешние приложения. `archive_item_open` сохраняет проверки
системного Open, lifetime извлечённой копии и отказа от запуска после Cancel.
`archive_external_edit` также проверяет просмотр файла размером 17 MiB без
удалённого лимита встроенного preview. `external_editors_ui` проверяет реальные
F3/F4 disk launches через собственный fixture-процесс, focused file при нескольких
selected, source Diff pair/opposite/fallback в normal/flat списках, ошибку запуска,
Editor Browse/Apply/Cancel и normal/compact JSON. `external_launch` проверяет
literal argv с пробелами/Unicode/метасимволами без shell и source parser.
`archive_external_edit` проверяет реальные ZIP F3/F4, Yes/No/Escape-Cancel,
размер и selection после обновления, busy deferral, pending close, смену location
и второй активной панели, failure/cancel и read-only TAR с сохранением правки.
В 48 EN/RU both-pane table/icon standard/alternative случаях rowsRemoved
подтверждает публикацию после failure/Cancel и её отсутствие при worker read-only
refusal. Source marks, selection, provider, inactive peer/focus, redraw и исходные/
временные bytes сохраняются; normal/compact JSON проверяет полученные views.
`source_update_backend` отдельно проверяет optional completion при stale index,
успехе и ранних missing-item/changed-folder/released-provider выходах.
`archive_external_lifetime` проверяет дочерний процесс после выхода родителя,
короткий/долгий неизменённый файл, ошибку запуска и wait error с retained copy.
`archive_external_security` проверяет extraction failure/cancel, password cancel,
ZIP AES и 7z encrypted headers, обновлённые bytes и отказ с неверным паролем.
Fixtures запускают собственный управляемый executable, не пользовательский editor.
Prompt JSON/geometry проверяются при окне 980×620 и 640×400.
`verctrl` проверяет source backup001/002, Edit/Commit и точное округление mtime,
readonly/metadata, literal Diff argv через свой executable, Revert Yes/No/Escape,
identical Edit и timestamp-only Revert, missing backup, одинаковые данные с
другим временем, недоступный backup root и исходный лимит256 MiB. Проверяются
отсутствующие Diff/7vc и multiple selection. Revert geometry/JSON:700×440 и
560×440, default No, скрытые extra buttons; window JSON980×620/640×400.
`archive_mutation` проверяет реальную замену файла в `.7z` и `.zip` и отказ от
небезопасного пути без изменения архива. Windows/Linux runtime не проверен локально.

`ui_state` проверяет 10 слотов избранного из `PanelFolderChange.cpp`: сохранение папки с Unicode и пробелами через клавиатуру, открытие, сохранение внутренней папки архива, повторную загрузку из настроек, блокировку во время загрузки и недоступный архив. Проверка выполнена локально на macOS; сочетания клавиш в Windows и Linux ожидают проверки.

`ui_state` также проверяет числовую сортировку имён `1.txt`, `2.txt`, `10.txt` в обычной папке и архиве по правилу `PanelSort.cpp`.

Команды сортировки проверяются на управляемом списке архива: по типу, дате, размеру и исходному порядку, включая сохранение выбора после обновления. `ui_layout` проверяет, что служебные столбцы типа и порядка скрыты от пользователя. Порядок перечисления файловой системы зависит от ОС и не сравнивается с фиксированным списком.

Для следующих команд 7zFM `ui_state` проверяет создание пустого файла с Unicode и пробелом, отмену диалога без записи, выделение и снятие выделения по маске без учёта регистра, выбор по типу текущего файла и работу маски внутри архива. Отдельный сценарий проверяет удаление записи из диалога истории папок, переход к оставшейся папке и сохранение истории в настройках между окнами. Эти проверки не подтверждают работу сочетаний клавиш и системного оформления диалогов на Windows и Linux.

Для панели команд `ui_layout` проверяет порядок семи действий 7zFM и наличие двух кнопок навигации рядом с адресным полем. `ui_state` запускает общую кнопку «Добавить» в режиме каталога и открытого архива, проверяя выбор соответствующего диалога и блокировку действия во время загрузки. Эти проверки выполнены на macOS; визуальное расположение на Windows и Linux ещё не проверено.

Для углублённого переноса `7zFM` тест `ui_layout` проверяет разделитель групп, цветные значки, отсутствие номеров строк и видимого полного пути, переключение групп панели, размера значков и подписей с восстановлением после нового окна. Он также вызывает контекстное меню заголовка и проверяет сохранение видимости столбца. `ui_state` проверяет состав контекстного меню для выделенного файла, фона каталога и открытого архива, основные поля свойств выбранного файла и переход через выпадающую историю адреса. Тесты хранят настройки в отдельных временных INI-файлах сборки через `SEVENZIP_SETTINGS_PATH`, чтобы не менять настройки пользователя.

Для режимов просмотра `ui_layout` переключает все четыре пункта, проверяет общую модель и выделение, размер значков и восстановление выбора из настроек. Для диалога Extract он проверяет редактируемую историю назначения, доступные режимы путей и перезаписи, пароль и переключатель его видимости. `ui_state` проверяет отмену и передачу параметров извлечения в backend. `extraction_options` проверяет режим без путей, отказ, пропуск и перезапись существующего файла на одноразовых `.7z` и `.zip`, отклонение совпадающих имён, ссылку в назначении и отмену процесса. Компоновка значков и drag-and-drop в реальных файловых менеджерах других ОС требуют отдельной ручной проверки.

`ui_layout` запускает настоящее окно через Qt Test в режиме `offscreen` и проверяет размеры 980×620, 720×480 и 640×400. Тест проверяет видимость панели, адресной строки, таблицы и строки состояния, расположение без наложений и доступность столбцов. Текущие layout-тесты записывают JSON в `build/dev/ui-reports/`; PNG включаются только при `SEVENZIP_CAPTURE_SCREENSHOTS=1`, когда нужен осмотр особенностей отрисовки. Сравнения с эталонными снимками и автоматического сообщения об изменении картинки пока нет.

После изменения видимой части окна или диалога нужно сверить его text specification, normal/compact geometry и текущие JSON-отчёты: группировку, порядок, размеры и обрезание. Для шрифтов, цветов, иконок и необъяснимых дефектов использовать снимок только затронутого экрана по правилам `AGENTS.md`. Исходные Win32 ресурсы подтверждают структуру, но не pixel parity; недоступные экраны и платформы перечислять в результате.

`flat_view` проверяет плоский просмотр вложенных файлов и папок с Unicode и пробелами, совпадающие имена файлов в разных папках архива, сохранение точного пути при выбранном извлечении, сохранение выделения при переключении, переход в папку и обновление архива. Отдельно проверяется независимость настроек плоского просмотра файловой системы и архива. Каталог с 1500 файлами проверяет частичное наполнение списка и отмену обхода до завершения. Перечисление очень большого дерева и поведение платформенных ссылок требуют отдельной проверки.

`nested_archive` проверяет команды Open Inside/Outside из 7zFM: реальное открытие `.zip` внутри `.7z` с Unicode и пробелами через `7zz`, возврат с восстановлением родительской записи и удалением временной копии, блокировку изменения вложенного архива, отмену извлечения и ошибку чтения вложенного архива без потери родительского списка. Системное приложение подменяется локальным обработчиком; визуальное оформление и сочетания клавиш на Windows/Linux остаются для платформенной проверки.

Тот же набор создаёт настоящий TAR с Unicode‑именем, проверяет просмотр и извлечение через backend и недоступность команд изменения. Затем он открывает `.tar.gz`, переходит внутрь TAR и возвращается к родительскому архиву. Остальные добавленные форматы зависят от обработчиков установленного `7zz` и требуют отдельных файловых образцов и платформенной проверки.

`volume_operations` проверяет размеры томов, разделение и обратную сборку Unicode-файла с пробелом, запрет перезаписи, пропущенную часть, отмену с уборкой вывода и элементы диалога. `ui_layout` проверяет доступность команд Split/Combine по выделению. Проверка выполнена локально на macOS; системные диалоги и файловые операции на Windows и Linux требуют отдельных прогонов.

Крупные наборы разделены по области: `ui_state`, `ui_operations`, `ui_navigation`, `ui_archive_states` используют общую управляемую подмену backend; `archive_backend` и `archive_mutation` используют общие локальные фикстуры. Упоминания `ui_state` и `archive_backend` выше описывают исходные сценарии; после разделения соответствующие случаи выполняются в этих тематических наборах.

`archive_proxy` checks the adapted upstream directory tree with explicit and implicit folders, Unicode names, recursive size and packed-size totals, real index expansion, flat traversal and reload cleanup. Existing `flat_view`, `properties`, `ui_state` and nested archive tests verify the Qt integration.

`panel_selection` checks the extracted upstream selection rules without widgets:
case-insensitive masks with Unicode and spaces, matching folder names,
last-dot extensions, names without extensions, dotfiles, folder-only selection,
retaining unmatched selected items, deselection, inversion and missing focus.
Existing `ui_state` mask/type and all/invert/clear scenarios check the Qt adapter
in filesystem and archive views; the shared model retains the current index.

`flat_view` additionally checks selection by an exact filename mask and by type
for extensionless files below dotted parent folders. The Qt selection adapter
passes the base name, as `PanelSelect.cpp::GetItemName()` does, rather than the
relative path displayed in flat mode.

`panel_key` checks numeric keypad mask, all, type and invert commands in all
four view modes, both on disk and in a controlled archive with Unicode names
and spaces. It also checks that ordinary `*`, Ctrl+keypad plus, address-field
input and commands during archive loading do not change selection. Qt key
events are synthesized on macOS; physical keypad mapping on Windows/Linux
remains for platform verification.

`ui_navigation` checks persistent history for internal archive folders with
Unicode and spaces, restoring from the address combo and Alt+F12 dialog,
navigation without reloading the current archive, fallback to a surviving
parent and an unavailable archive. Dialog geometry and JSON at 540×360 and 360×240
are checked as `archive-history-540.json` and `archive-history-360.json`;
PNG is opt-in through `SEVENZIP_CAPTURE_SCREENSHOTS=1`.
The `archive_item_open` scenario also covers window destruction after
visiting an archive folder: address signals are disconnected before Qt's
focus teardown can trigger navigation into a partially destroyed window.

`panel_auto_refresh` проверяет создание и удаление Unicode-файла во внешней
файловой системе, сохранение текущего элемента и прокрутки, выключение и
сохранение настройки, ручное обновление, исчезновение текущей папки и
восстановление после её повторного создания. Таймер пропускает активные
операции и незавершённый flat scan; их отмена и stale batches отдельно
проверены в `flat_view`. На macOS это проверка метаданных, а не Windows
change notification.

`panel_sort` проверяет правила `PanelSort.cpp` (`0766b73`) через Qt-таблицу:
папки перед файлами в обоих направлениях, повторный выбор свойства,
размер и дата по убыванию при первом выборе, щелчок заголовка размера,
сохранение свойства и направления после обновления архива (включая сжатый
размер), обновления каталога и перехода в дочернюю папку и обратно.
Фикстуры содержат Unicode и пробелы; архивный список подаёт управляемый backend.
Снимки `sort-980.png` и `sort-640.png` сохраняются в `build/dev/ui-screenshots/`.

В `panel_sort` дополнительно проверяются плоская сортировка каталогов и
архивов по базовым именам, числовые префиксы одинаковых имён, направление
при равном размере и точное выбранное извлечение после перестановки строк.
Сортировка по расширению включает dotfiles, имена без точки, конечную точку,
многосоставное расширение и папки с точкой. Снимки плоского списка:
`flat-sort-980.png` и `flat-sort-640.png` в том же каталоге снимков.

`volume_operations` также проверяет сводку Combine для пяти частей с Unicode и
пробелами: число, размер в байтах, первые две и последнюю часть и остановку
на первом пропуске. Пустые части допустимы при ненулевой сумме.
`volume_workflow` проверяет отмену диалога без создания результата;
`volume_dialog` сохраняет normal/compact widget JSON. Скриншоты opt-in.

Последовательности размеров Split проверяются в `volume_operations`: повторение
последнего размера, три разных размера, неиспользованный хвост, точные байты
всех частей, отклонение нуля и переполнения, отмена с очисткой вывода. Диалог
принимает `10M 20M` и UInt64-размеры, сохраняет исходный Trim и оставляет
ошибочный ввод открытым для исправления; геометрия проверяется widget JSON.

`panel_key` проверяет Alt+F1/F2 из списка и адреса и Tab из адреса во всех
четырёх режимах просмотра каталога и архива, сохранение текущего пути и
отказ для лишних модификаторов. Shift+F4 проверяется через создание пустого
Unicode-файла, отмену, отсутствие диалога из поля адреса, при чтении архива
и в открытом архиве. Снимки окна и popup адресов при ширине 980 и 640
сохраняются как `address-key-*.png` и `address-popup-*.png`. Физические
клавиши и системные перехваты Alt+F1/F2 на Windows/Linux не проверены.

`settings` проверяет настоящие события мыши с Ctrl, Shift и Meta в режиме
открытия одним щелчком: выделение не меняет папку, обычный щелчок открывает
её. Те же сценарии проверяются при обычном открытии двойным щелчком, включая
доступность Copy после выделения мышью. Проверяются все четыре режима
файловой системы и архива, в фикстурах
есть папки с Unicode и пробелами. Платформенные жесты Windows/Linux требуют
отдельной проверки; локальный прогон использует Qt offscreen на macOS.

`panel_sort` дополнительно проверяет исходный указательный компаратор: ведущие
нули, числа длиннее UInt64, пустые имена, Unicode и ограниченные срезы Qt-строк.

`panel_selection` проверяет ядро масок напрямую: пустой ввод, несколько звёзд,
возврат при поиске совпадения, точное окончание имени, `*.*` для имени без точки,
Unicode без учёта регистра, UTF-16 surrogate pair и срезы без терминатора.

`panel_key::parentKey` проверяет Backspace без модификаторов и с Ctrl/Shift/Alt
во всех четырёх представлениях, в обычной папке и архиве с Unicode-путём.
Тест также проверяет ввод в адресе и блокировку перехода при загрузке; до
исправления все восемь сценариев падали на Ctrl+Backspace. Снимки после
перехода: `build/dev/ui-screenshots/parent-key-980.png` и `parent-key-640.png`.

`volume_operations::combineRollback` проверяет отмену после первого блока,
исчезновение следующей части, увеличение текущей части и занятую цель.
Во всех случаях временный вывод удаляется, существующая цель сохраняется;
изменения частей выполняются синхронно через сигнал прогресса без таймеров.

Новый focused CTest `combine_names` проверяет распознавание и полную сборку
через команду меню для суффиксов без точки, двух/четырёх цифр, Unicode,
имени `.001` и нескольких точек перед номером. Проверяются имя результата,
его байты, сохранность частей и отсутствие временного вывода. Неверные
суффиксы и пропуски отклоняются. До изменения парсер отклонял имена без точки, двухзначные суффиксы
и `.001` без базового имени; дополнительные случаи проверяют имя результата.
Снимки Combine для `.001`: `combine-names-540.png` и
`combine-names-360.png` в `build/dev/ui-screenshots`.

`volume_operations::splitRejectsSourceChange` синхронно уменьшает или увеличивает
источник после первого тома и проверяет отказ с полной уборкой временных
файлов. Проверки последовательности размеров, точного содержимого результата,
существующей цели и отмены покрывают перенесённый `CThreadSplit::ProcessVirt`.

CTest `compression_order` проверяет набор слов LZMA, значение Auto на уровнях
5/9, сохранение выбора и отключение для BZip2. Реальный `7zz` создаёт и
проверяет архивы с 8/48/192; недопустимый размер и отмена не оставляют цель
или stage. До переноса пять сценариев падали. Снимки 760×720 и 640×480:
`compression-order-760.png` / `compression-order-640.png` в `build/dev/ui-screenshots`.

CTest `compression_solid` проверяет границы и промежуточные значения исходного
ряда, сохранение выбора, отключение для ZIP и флажка non-solid. Реальный
архив создаётся и тестируется с блоком 1 МиБ; 3 МиБ отклоняются, отмена
с блоком 32 МиБ убирает временный файл. До переноса оба сценария падали.
Снимки: `compression-solid-760.png` / `compression-solid-640.png`.

`compression_order` также проверяет полные ряды Deflate/Deflate64, Auto на
уровнях 5/7/9, сохранение выбора, Store, реальные ZIP с fb=258/257 и `7zz t`.
Значение 258 для Deflate64 отклоняется до запуска; отмена ZIP не оставляет
архив или stage. Снимки: `compression-Deflate-760.png`,
`compression-Deflate-640.png` и соответствующие `Deflate64` в ui-screenshots.

`compression_order` проверяет ряды PPMd/PPMdZip, Auto на уровнях 1/5/7/9,
сохранение выбора и отключение поля для BZip2. Реальный 7zz создаёт архивы
7z с порядком 2/32 и ZIP с 2/16 и проверяет их командой t. Недопустимые
порядки (9 для разреженного 7z ряда, 17 для ZIP) отклоняются; отмена
убирает цель и stage. Снимки `compression-PPMd-{7z,zip}-{760,640}.png`
сохраняются в `build/dev/ui-screenshots/`.

Focused CTest `compression_dictionary` проверяет малые значения, 3 МиБ,
число вариантов и архитектурный верхний предел, Auto на уровнях 1/5/9,
сохранение выбора между LZMA/LZMA2 и отключение словаря для Store.
Реальные архивы с 64 КиБ, 256 КиБ и 3 МиБ (7z и ZIP LZMA) проходят 7zz t.
128 КиБ отклоняется как отсутствующий в исходном ряду; отмена не оставляет
цель или stage. Снимки `compression-dictionary-760.png` и `-640.png`
сохраняются в build/dev/ui-screenshots. 32-bit предел не проверен локально.

CTest `compression_memory` проверяет ряд PPMd 7z, архитектурный предел,
Auto на уровнях 5/9, сохранение выбора, Store и Auto без явного размера.
Реальный архив с памятью 1 МиБ проходит 7zz t и извлечение с проверкой
байтов Unicode-файла с пробелом. 5 МиБ отклоняется; отмена не оставляет
цель или stage и сохраняет созданный архив. Снимки memory-PPMd-7z-760.png
и memory-PPMd-7z-640.png находятся в build/dev/ui-screenshots.

`compression_memory` дополнительно проверяет отдельный ряд PPMdZip, Auto,
Store и создание ZIP с mem=1 МиБ. После 7zz t проверяются байты извлечённого
файла. Размер 3 МиБ отклоняется для ZIP; отмена сохраняет предыдущий архив
и не оставляет stage. Снимки memory-PPMd-zip-760.png и -640.png.

`compression_memory` покрывает BZip2 в 7z/ZIP: полный ряд, Auto на уровнях
1/3/5/9, сохранение выбора и Store. Архивы с 100 КиБ (7z) и 900 КиБ (ZIP)
проверяются через 7zz t и сравнение извлечённых байтов; 150 КиБ
отклоняется, отмена убирает stage. Снимки memory-BZip2-7z/zip-760/640.png.

`compression_memory` также проверяет фиксированный Auto Deflate/Deflate64
на уровнях 1/3/5/7/9, отключение ручного словаря, создание/извлечение ZIP,
отказ для явного 32/64 КиБ и отмену. Снимки memory-Deflate/Deflate64-zip-760/640.

`compression_solid` покрывает весь список: raw logarithms, Auto/Non/Full,
сохранение Full при смене словаря, отключение для ZIP/Store, исходные
формулы Auto для LZMA2/LZMA/PPMd/BZip2 и границы. Четыре реальные 7z
(Auto, Non-solid, 1 МиБ, Full) содержат два Unicode-файла >1 МиБ:
проверяются число Blocks=1/2/2/1, 7zz t и точные извлечённые байты.
Неверный размер, solid для ZIP/Store и отмена Full не оставляют stage.
Снимки solid-mode-{4294967295,0,64}-{760,640}.png находятся в ui-screenshots.

Focused CTest `compression_resources` использует фиксированные CPU/RAM,
чтобы проверить пределы 2/512/64/1 и ZIP 32/128, CPU×2 ряд, уменьшение
Auto LZMA2/ZIP по памяти и отсутствие уменьшения при неизвестной RAM.
Проверяются численные оценки PPMd/Deflate/Store, сохранение явных потоков,
переход к Auto-only, обновление памяти при смене словаря и UI в трёх режимах.
Пять реальных архивов (LZMA2, PPMd, BZip2, ZIP PPMd, ZIP Deflate) проходят
7zz t и сравнение извлечённых байтов; 513 потоков отклоняются, отмена
удаляет stage. Снимки resources-{PPMd,ZIP,LZMA2}-{760,640}.png.
Побайтовое сравнение подтверждает неизменность всех шести новых reusable
upstream-файлов. Локальная проверка — macOS; остальные платформы требуют CI.

Focused CTest `memory_limit` проверяет перенесённый CMemUse: проценты и
альтернативный p-префикс, байты и K/M/G/T с необязательным B, регистр,
UInt64 maximum, переполнение десятичного ввода и сдвига, ошибочные строки,
сброс ранее разобранного значения, округление и насыщение процентов.
Проверки выполняются через Parse/GetBytes без проверки текста исходника.

Focused CTest `compression_limit` проверяет исходный ряд и его предел,
вставку нестандартных процентов/размеров, Calc_From_Val_Percents с
фиксированной RAM, уменьшение Auto ZIP/LZMA2 и неизвестную RAM.
Проверяются выбор/смена формата/Auto в диалоге, отказ OnOK без закрытия,
последующее успешное подтверждение, реальные 7z и ZIP с лимитом
256 МиБ/80%, 7zz t и точные извлечённые байты Unicode-файла с пробелом.
Предел 1 МиБ отклоняет создание до stage; отмена сохраняет предыдущий
архив и убирает stage. Снимки memory-limit-760.png/-640.png находятся в
build/dev/ui-screenshots. Существующий ui_operations::addToOpenArchiveState
проверяет, что добавление без контролов сжатия продолжает работать.

Focused CTest `compression_preferences` проверяет независимые значения
7z/ZIP при смене формата и повторном создании диалога, сохранение при OK,
отмену после смены обоих форматов, ошибку имени без записи, сброс к Auto,
восстановление нестандартных 37%/320 МиБ, неправильный/переполненный spec,
повторный Load с отсутствующим файлом и сохранность других групп INI.
Тест использует временный Unicode-файл и отдельный settings path;
compression-наборы также получили отдельные SEVENZIP_SETTINGS_PATH,
чтобы не читать и не менять настоящие пользовательские настройки.

Focused CTest `extract_dialog` проверяет исходный порядок overwrite,
OnOK через кнопку и accept(), восстановление режимов/видимости без сохранения
пароля, Cancel, cap истории 16, выбранный и введённый пути, Unicode/пробелы,
Trim и сравнение истории без учёта регистра, legacy history и неверные enum,
сохранность других групп INI. Реальный ZIP проверяет Skip и Overwrite
через результат диалога с точным сравнением байтов. Набор сохраняет снимки
extract-port-640.png и -470.png в build/dev/ui-screenshots. Существующие
extraction_options и ui_archive_states покрывают ошибки/отмену и backend.

Focused CTest `split_destination` проверяет исходный pointer helper:
пустой/корневой/относительный путь, один и два конечных разделителя,
Unicode/пробелы и платформенный backslash. Через GUI проверяются SplitDest
по умолчанию, видимость Edit, Trim/сборка/нормализация назначения, история,
сохранение флажка и Cancel. Реальный ZIP извлекается в новую составную
Unicode-папку с проверкой байтов; занятая файлом цель отклоняется без изменения
файла, Cancel не создаёт папку. Снимки split-dest-{0,1}-{640,470}.png
сохраняются в build/dev/ui-screenshots. Старые GUI-тесты полного назначения
явно отключают новый флажок, как пользователь при выборе полного пути.

`compression_preferences` дополнительно проверяет полный профиль 7z/ZIP:
уровень, метод, словарь, слово, Full solid, потоки и RAM; переключение туда
и обратно, повторное создание диалога, неизвестный метод, неверные численные
значения, Auto sentinel и Cancel. Восстановленный ZIP Deflate level 9/fb=258
создаётся через ArchiveBackend, проходит `7zz t` и извлечение с точным
сравнением байтов Unicode-файла. Снимки compression-preferences-760.png и
-640.png сохраняются в build/dev/ui-screenshots.

`compression_preferences::levelChangeResetsProfile` для 7z/ZIP загружает
профиль PPMd с ручными параметрами, меняет уровень и проверяет исходный
метод/Auto, сохранность лимита RAM, переход к Store и восстановление после
OK. Наборы compression_order/dictionary/memory/solid теперь ожидают сброс
ручных спецификаций при смене уровня; для проверки рядов других методов
после смены уровня метод выбирается снова. Это фиксирует отличие от
прежней самостоятельно написанной политики Qt.

`methodAndDictionaryDependencies` проверяет профиль LZMA2 → LZMA → LZMA2,
сброс чужих параметров, восстановление профиля, Auto solid после смены
словаря, сохранность слова/потоков и специальных solid 0/64. Cancel оставляет
INI неизменным. Снимки Store 7z/ZIP после смены уровня:
`level-reset-{7z,zip}-{760,640}.png` в build/dev/ui-screenshots.

P0-12.5: `source_extract_backend` проверяет исходный `ExtractGUI`/`Extract`
на двух ZIP с одинаковым Unicode-путём и решениями Yes/Cancel, а также на
многотомном `.7z.001` с пропуском следующих частей и точным сравнением байтов.
`nested_archive::opensRealNestedArchive` проверяет Ask и итоговые байты при
извлечении целого вложенного архива через batch и выбранного элемента через
текущую исходную FM-папку. `ui_operations` проверяет root batch, шифрованные
7z/ZIP с неверным и затем верным паролем, текущую папку, отказ до перезаписи
и отсутствие backend overwrite preflight. `extract_dialog` проверяет приоритет
сохранённых и принудительных режимов с паролем; диалог остаётся тем же Qt
представлением исходных групп и combo modes из `ExtractDialog.rc`.

P1-04.1: `source_update_backend` проверяет исходные Agent/ArchiveFolderOut
Delete/Rename/CreateFolder в реальных ZIP/7z, отдельный output, публикацию,
переоткрытие, неявные папки, Unicode/пробелы, неизменные файлы, отказ
небезопасного пути и отмену без изменения оригинала. `ui_operations`
проверяет F7 → F2 inline → Delete с отказом/подтверждением и восстановление
списка; `archive_selection_runtime` проверяет flat basename редактора и
отмену. Эти наборы используют временные файлы, не пользовательские архивы.

P1-04.2: `source_update_backend` проверяет реальные защищённые 7z (header
encryption) и ZIP AES для CreateFolder/Rename/Delete: верный/неверный пароль,
сохранность bytes и mtime, отмену и побайтово исходный архив при ошибке.
`ui_operations` проходит F7, F2 и Delete в защищённом 7z и считает запросы
пароля: один при открытии, без повторов после операций. Прямое сравнение
rendered Windows диалога и runtime Windows/Linux остаются открытыми.

P1-11.1: `volume_mount` подставляет одноразовый том с Unicode/пробелом,
проверяет исходные свойства и переход в `CFSFolder`, исчезновение и
недоступность пути до binding. `root_navigation` проходит Qt root → «Тома» →
системный mount → назад и пишет current-build normal/compact JSON; `fs_folder`
и `panel_path` проверяют соседние native routing paths. Список mounts от
`QStorageInfo` и реальные сетевые ресурсы требуют проверки на каждой ОС.

P1-11.2: `browse_dialog` проверяет Cancel и Unicode/пробелы для folder/file
picker, восстановление исходного FilePath и выбранного фильтра. Прямые
consumers проверяются `copy_dialog`, `extract_dialog`, `volume_operations`,
`compression`, `ui_navigation` и `ui_operations`; modal tests используют
`QT_QPA_PLATFORM=offscreen`, без обращения к пользовательским файлам.
Нативное оформление и Windows super/device path требуют проверки на ОС.

P1-11.3: Unix `network_folder` подставляет сетевой mount с Unicode/пробелами,
проверяет source `CNetFolder` properties/binding и отказ для исчезнувшего
или недоступного пути. `root_navigation` проходит root → «Сеть» → Back,
проверяет доступность Parent и пишет normal/compact widget JSON. Соседние
`volume_mount`/`panel_path` проверяют shared navigation. Unix fixture не
использует live network или credentials.

P3-03.5: Windows target выбирает отдельный native fixture вместо прежнего
required skip. Он создаёт временный локальный SMB share с доступом только
текущего SID, проверяет исходный WNet/CNetFolder → CFSFolder, UNC bytes и
исчезновение ресурса. Нужны запущенные LanmanServer/LanmanWorkstation и token
с правом создания share; внешний сервер, пользовательские credentials и
изменение policy не нужны. Windows Qt6.12 snapshot27c52b66 прошёл этот
native scenario без skips; исходный focused log сохранён отдельно от следующей
menu reference проверки. Это не проверка remote credentials/UI; см. audit.

P3-01: `upstream_baseline` проверяет exact-byte отказ и old→new report для
адаптированного и неизменённого модуля в локальных временных Git fixtures.
`upstream_inventory` сверяет каждый tracked borrowed path с manifest на
каждом CTest прогоне, не требуя checkout upstream. Полная byte-проверка
`python3 scripts/upstream_baseline.py verify --upstream <snapshot>` запускается
отдельно на snapshot точного commit; см. `docs/upstream-update.md`.

`compression_error_policy` covers the source Create validation transaction with
isolated temporary settings/files and the official codec module. Its 48 data
rows (50 Qt results including setup/cleanup) cover EN/RU and allowed/suppressed
questions: ZIP ASCII/AES length, mismatched passwords including disabled controls,
invalid Browse/output path, invalid volume and path-before-volume precedence.
All 32 refusal/recovery rows then create/list/test a real Unicode/spaced ZIP and
check input bytes. Four UI memory refusals remain unconditional, four direct
source memory checks cover known/unknown RAM and explicit/automatic limits, and
eight small-volume No/Cancel rows remain unconditional. Refusal does not publish
a target or persist settings. Standard message geometry produces 56 JSON reports
per OS at requested 480×220/320×160, checking full labels and reachable buttons.
See [source provenance and validation](../upstream-port-audit.md#p3-05--source-create-validation-error-policy-2026-10-07).

`compression_profile_selection` covers saved source profile restoration across
LZMA/LZMA2, PPMd/ZIP PPMd, BZip2 and Deflate, EN/RU: values between/below entries,
large dictionary marker, Auto, full/non-solid, out-of-range solid and unavailable
explicit threads. Eighteen rows reopen after Cancel with byte-identical temporary
settings and check public options/controls. Two further cases verify original
format-name comparison and accepted normalized settings followed by real official
Create/list/test/extract with exact Unicode/spaced input/output bytes. Including
setup/cleanup, the suite has22 Qt results. Normal760×720/compact640×480 probes
produce36 JSON reports, with dictionary/order/solid fully reachable by scrolling.
Large dictionary cases only inspect/reject; they do not allocate those dictionaries.

`compression_method_state` checks source Copy's zero dictionary, unknown-name
classification and14 EN/RU dialog states: 7z/ZIP Store, TAR GNU/POSIX, GZip,
LZMA2 and an unavailable external-name UI fixture. It verifies empty/Auto lists,
enabled states, unknown/Store memory, TAR visibility and recovery to ordinary
7z, with byte-identical settings after Cancel. Normal/compact probes produce28
JSON reports. The external name is a control-state fixture, not a live encoder.
Two real Store workflows accept explicit threads, create/list/test/extract 7z/ZIP
with Unicode/spaced inputs, reject invalid threads and cancel creation without
publishing output; original bytes remain exact. With setup/cleanup,20 Qt results.
`compression_profile_selection` also asserts ZIP's empty solid selector and
interprets its logical all-ones spec without relying on an empty QVariant's zero.

`benchmark_lifecycle` additionally injects failed and empty POSIX affinity queries
through the typed system-query service, checking the original normalized CPU text
and joined cancellation. Windows retains its original native normalization and
explicitly skips these two POSIX fault cases. A presentation-adapter test verifies
queued delivery while attached and successful completion without delivery after
detach, matching source PostMsg_Finish. Existing normal/total metrics, Help/live
language, Restart/Stop/Cancel, memory refusal and engine failure remain covered;
`native_benchmark` exercises the real official engine and source result callbacks.

`compression_event` checks source Solid/thread/memory-limit/dictionary phases:
Auto and explicit/unavailable thread selections, cached memory text, memory
refusal without saving, Cancel/no-write and restart. EN/RU fixtures use the actual
official codec catalog for XZ; fallback-only dialogs are insufficient for this
test. Restored profile options create/list/test/extract real XZ files and preserve
Unicode/space input bytes, including invalid-thread failure and immediate
cancellation without output. Cached-Auto difference/refusal cases require known
RAM and at least four available CPUs and explicitly skip otherwise; the selected
three-platform validation machines satisfy that capability. Normal/compact JSON
and geometry assertions verify reachable thread/memory fields and fitting text.

`compression_timestamp` checks COptionsDialog's source selection and precision
lifecycle: empty/one/multiple capability masks, absent defaults, inserted default,
explicit DOS, custom and unsupported precision, in EN/RU. It asserts the native
ComboBox CB_ERR/no-automatic-selection service, OK versus Cancel, defined access
time, sentinel key deletion, Save/Load and actual keyboard selection/reopening.
Including setup/cleanup it has21 Qt results. Typed handler metadata provides
deterministic unusual masks without adding an encoder. The64 normal/compact
520×400/440×340 reports check precision reachability. `compression_advanced`,
`archive_metadata_ui` and `source_metadata_storage` remain direct real-engine
timestamp/metadata consumers; synthetic masks do not establish format support.

`compression_format` exercises source format priority independently of sorted
presentation: current/reordered native indices, saved/explicit/missing format,
EN/RU, all displayed-to-native mappings and Cancel with unchanged settings.
The24 reports use760×720/640×480 and require format reachability. Additional
public source cases cover sorted Browse filter order, cancellation and changed
filter extension; Hash SHA1/Auto SHA256 filename refresh; accepted ZIP restart,
official create/list/test/extract with exact Unicode/space bytes, invalid-thread
failure and cancellation without output. Source empty/single-entry GetLevelSpec
and TAR profile/restart preserve Auto without a level key. The real source modal
Hash writer produces a SHA1 manifest checked against exact input bytes and name.
Including setup/cleanup:20 Qt results. Windows settings fixtures set their Unicode
override through the wide CRT and assert its exact roundtrip before exercising UI.

`compression_event` now has20 Qt results including six EN/RU 7z/ZIP/XZ level
transitions: method-dependent defaults reset while Parameters retains its current
text; source specs and Cancel/no-write are asserted. `sfx_lifecycle` has22 Qt
results including12 EN/RU Auto/LZMA/LZMA2 valid/unavailable-thread cases. Repeated
SFX on/off preserves method-dependent specs and current parameter text. Existing
real SFX success/extraction/delete/pause/cancellation cases remain in that suite.
