# Прямое read-only чтение устройств

## Источник и границы

Источник: https://github.com/ip7z/7zip.git,
ревизия `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Форматы остаются в официальном динамическом runtime; новый парсер не добавляется.

Просмотрены upstream `CPP/7zip/Archive/GptHandler.cpp` (`CHandler::Open2`,
`Open`, `GetProperty`), `MbrHandler.cpp` (`ReadTables`, `Open`, `GetProperty`),
`ExtHandler.cpp` (`Open2`, `GetStream_Node`, `Extract`, свойства и флаги),
а также `CPP/7zip/Common/LimitedStreams.{h,cpp}` (`CLimitedInStream`,
`CreateLimitedInStream`) и file/stream opening в `UI/Common/OpenArchive`.
Зависимости: официальный codec catalog, COM stream/handler interfaces,
upstream stream bounds, callback progress/cancellation и retained module lease.

`ArchiveDevice` — локальная интеграция между разрешённым OS-потоком и этими
модулями. Она передаёт проверенный размер, предлагает разделы из GPT/MBR,
проверяет их попадание в границы устройства и открывает выбранный раздел
штатным Ext-обработчиком. Автоматический recursive open отключён, чтобы
пользователь мог выбрать раздел. Синтетические записи свободного места MBR
исключаются по отсутствию FileSystem, которое upstream выдаёт только для
реальных разделов. Источник без таблицы разделов передаётся Ext целиком.

Agent и QtArchiveFileResource принимают отдельный прямой stream transport;
обычные архивы продолжают использовать прежний file-only transport. Native I/O
и разрешения перечислены ниже; они сами по себе не означают готовность UX.

## Текущее состояние

Реализована экспериментальная [первая версия](../direct-device-reading-plan.md).
Подключены native access macOS/Linux/Windows, stream-level partition discovery,
Ext-reader и пользовательский выбор устройства/раздела. Просмотр и копирование
проверены на частных образцах. Доступная платформенная приёмка завершена;
реальные носители и системная авторизация остаются непроверенными. Пользователь
подтвердил отсутствие тестового носителя и выбрал проверки только на образцах.
Инструкция и ограничения — в [руководстве](../direct-device-reading.md).

## Agent и прямой поток

Сравнены полные тела и callers `CPP/7zip/UI/Agent/Agent.cpp`
(`Open`, `CanUpdate`, `ReOpen`, `ReadItems`), `ArchiveFolderOpen.cpp`
(`OpenFolderFile`), `ArchiveFolder.cpp` (`CopyTo`, `Extract`),
`ArchiveFolderOut.cpp` (`CommonUpdateOperation`) и
`CPP/7zip/UI/Common/OpenArchive.cpp` (`CArc::ReOpen`, `CArchiveLink::ReOpen`).
Ревизия источника — указанная выше; существующий смешанный baseline адаптированных
Agent-модулей сохраняется, новые upstream-тела не копируются.

Штатные Agent proxy/tree, выбор файлов, извлечение и пути назначения сохраняются.
Qt-граница распознаёт authorized device stream до spool/CLI listing и строит
property transport над тем же сохранённым native handler. Индексы не сопоставляются
по именам: метаданные и извлечение принадлежат одному обработчику. Размер и
read-only состояние берутся из потока и reader, виртуальный display path не
проверяется как обычный файл. При refresh переоткрывается разрешённый поток,
сохраняются исходные retirement/binding proxy и восстановление подпапки.
Признак устройства подключён к существующему `CanUpdate`; команды записи
отклоняются до создания выходного файла. Источник освобождается вместе с lease.

Upstream Ext публикует feature descriptions, но не применяет продуктовую политику
этой версии для неподдерживаемых возможностей. Direct-device open проверяет эти
публичные свойства до публикации reader и отклоняет неизвестные возможности,
шифрование, требование journal recovery и неподдерживаемые варианты размещения
данных. Собственный superblock parser не добавляется; обычное открытие `.img`
не получает это ограничение. Список проверяемых возможностей остаётся в коде.

Актуализация ext по [Linux reference](ext-linux.md) подключает отдельный Ext
format-модуль через штатное обнаружение движка. Direct-device open принимает
реализованные современные metadata/inline layouts; encrypted content и journal
recovery по-прежнему не публикуются как готовый reader. Новые creator fixtures
проверяют извлечение точных байтов из inline, BIGALLOC и META_BG через
авторизованный поток. Следующая связанная итерация подключает fs-verity data
verification и проверяет её тем же device-потоком. Ключи fscrypt и trust policy
остаются в общем scope актуализации ext; разрешение layouts не реализует их.

## Выбор устройства и панель

Перед подключением сравнены upstream `CPP/7zip/UI/FileManager/PanelItemOpen.cpp`
(`OpenAsArc`, stream callers), `PanelFolderChange.cpp` (parent links, bind/close),
`PanelMenu.cpp` (read-only state) и File-menu `resource.rc`/handlers.
Source `OpenAsArc` принимает выбранный поток; его folder links, worker/modal
opening, Parent, выбор и существующий CopyTo остаются владельцами поведения.
Отдельного файлового браузера или второй модели навигации нет.

Оригинальный Windows device namespace не даёт переносимого выбора внешних
носителей с native authorization. Qt добавляет File → Open device, сохраняя
обычный Open archive и список томов. Диалог перечисляет внешние устройства,
блокирует busy rows, затем предлагает разделы или открывает единственный.
Native access и backend inspection выполняются owned worker; UI отменяет запрос
и ждёт завершения, не публикуя поздний источник. Back освобождает источник до
следующего exclusive open; отложенное удаление QThread не удерживает device lease.
Проверка устройства компилируется вместе с reader; лёгким потребителям
метаданных не требуется весь process backend.

Для direct input главное окно использует native extract/test и refresh вместо
file-only listing prerequisites; извлечение всех файлов не переоткрывает display
path как файл. Read-only badge остаётся видимым в статусной строке, включая
просмотр вложенного архива; mutation availability учитывает source parent chain.
Во время незавершённого перечисления текущей панели команда открытия устройства
недоступна, чтобы не менять provider под его асинхронной проекцией.

EN/RU используют Qt Linguist; native access diagnostics также переведены.
Компоновка новая, поэтому rendered Windows 7zFM reference для неё отсутствует.
На macOS используются QDialog window modality и стандартные Qt/Cocoa роли кнопок;
AppKit sheet и pixel parity не заявляются. Контракт —
[direct device selection](../ui-layout-contracts/extraction.md#direct-device-selection).

## Native access macOS

Локальный platform adapter перечисляет внешние whole-media через IOKit и
Disk Arbitration. Upstream Windows FSDrives не предоставляет переносимой
реализации этого сервиса; POSIX device-size snippets в FileIO не активны.
Системное открытие и проверка native device identity/size — необходимая
платформенная граница, а не новая реализация файловой системы.

Устройство повторно сверяется с текущим inventory до и после открытия;
проверяются размер, размер сектора, тип дескриптора и отсутствие смонтированных
томов. Не выполняется принудительное размонтирование. Адаптер не создаёт
согласованный снимок носителя, который изменяет другая программа.

Пользователь подтвердил 2026-10-19: Developer ID отсутствует, функция должна
работать без него. Поэтому постоянный SMAppService/XPC-вариант исключён до
интеграции: Apple указывает проблемы ad hoc signing для такого сервиса.
Используется одноразовый helper через AppleScript administrator privileges,
который Apple рекомендует для ad hoc решений с разовым повышением прав:
[обсуждение signing](https://developer.apple.com/forums/thread/799910),
[варианты privilege escalation](https://developer.apple.com/forums/tags/servicemanagement/?sortBy=oldest&sortOrder=DESC).

Helper принимает выбор устройства, проверяет его самостоятельно и передаёт
O_RDONLY fd через частный Unix socket с проверкой UID сторон. Парсер ФС и
запись извлечённых файлов остаются в непривилегированном процессе.
Нет постоянной службы, изменения автозапуска, Authorization DB или требования
Apple-issued сертификата. Исходный язык native adapters — C++/Objective-C++
для доступа к системным framework; язык заимствованных модулей не меняется.

Новый поток адаптирует произвольные чтения к размеру сектора и использует
original CLimitedInStream для логических границ/seek. У каждого раздела
собственный курсор поверх readAt, чтобы чередование чтений двух открытых
разделов не нарушало cached physical cursor исходного limited stream.
Дескриптор остаётся жив, пока существуют его потоки. QtCore helper включается
в .app и deploy tool обрабатывает его loader paths; штатная упаковка подписывает
его ad hoc вместе с остальным пакетом.

## Native access Linux

Локальный адаптер использует системный UDisks2 ObjectManager для выбора внешних
whole devices; уже существующий upstream FSDrives этой службы не использует.
Отдельного Linux root-helper и разбора форматов в привилегированном процессе нет.
Размер логического сектора читается из kernel sysfs, открытие выполняется
через [Block.OpenDevice](https://storaged.org/doc/udisks2-api/latest/gdbus-org.freedesktop.UDisks2.Block.html)
в режиме `r` с exclusive-open и close-on-exec. Системную авторизацию обслуживает
polkit; noninteractive probe не разрешает показывать запрос прав.

Идентичность объединяет Block/Drive identifiers, device number и времена
обнаружения носителя из [Drive API](https://storaged.org/doc/udisks2-api/latest/gdbus-org.freedesktop.UDisks2.Drive.html).
После получения fd проверяются native block-device type, read-only access mode,
device number, размер и сектор; затем повторно сверяется inventory. Отдельные
разделы, внутренние диски, смонтированные тома и активные kernel holders не
предлагаются как доступный источник прямого чтения. Отмена ожидания не публикует
поздний дескриптор; поздний ответ принадлежит QtDBus и освобождается им.
Системный диалог polkit имеет собственное действие отмены.

Этот путь требует доступного UDisks2 и системного агента авторизации. Отсутствие
службы или отказ policy возвращаются вызывающему коду. Размонтирование, изменение
прав/политик и fallback к root-запуску GUI не выполняются.

## Native access Windows

Просмотрены upstream `CPP/7zip/UI/FileManager/FSDrives.cpp` (inventory,
`BindToFolderSpec`, `CopyTo`), `CPP/Windows/FileIO.{h,cpp}` (`CFileBase::Create`,
`CInFile::Open`, `CalcDeviceSize`, `ReadPart`) и
`CPP/7zip/Common/FileStreams.{h,cpp}` (`CInFileStream::Read`). Их path-based
открытие не предоставляет service boundary для уже разрешённого native handle
или ограниченной elevated session. Локальная граница использует WinAPI,
upstream `CPP/Windows/Handle.h` / `NWindows::CHandle` и общий sector-aligned
device stream. Форматные обработчики и файловый менеджер не копируются в helper.
Новых заимствованных тел в этой итерации нет; существующая baseline выше
остаётся источником переиспользуемых заголовков.

SetupDi и storage IOCTL предоставляют внешние whole disks, размер, сектор,
PnP identity/arrival и счётчик смены съёмного носителя. Смонтированные Windows
тома сопоставляются с physical disks через volume extents; неизвестное состояние
сопоставления запрещает открытие. До и после read-only открытия повторяются
проверки выбора; разделы не размонтируются.

Если обычному процессу не хватает прав, `ShellExecuteEx` с `runas` запускает
одноразовый `SevenzipDeviceReader.exe`. Частный локальный named pipe имеет ACL
текущего пользователя/администраторов/SYSTEM; стороны проверяют kernel peer PID.
Helper самостоятельно проверяет выбранное устройство, открывает его для чтения
и обслуживает только ограниченные выровненные запросы чтения. При закрытии
источника завершается elevated session. В helper нет парсеров архивов/ФС,
назначения извлечения или операций записи. GUI не получает повышение прав.
Helper устанавливается рядом с основным Windows executable.

## Проверки

Образцы в `tests/fixtures/devices` создаются штатными `mke2fs`/`sfdisk` как
обычные файлы, без монтирования. Образцы включают ext4 без таблицы, MBR-раздел
и GPT с несколькими разделами. Suite `archive_device` проверяет выбор раздела,
содержимое Unicode-файла, отказ неподдерживаемого раздела, отмену, ошибку чтения,
время жизни потока и соблюдение границ разрешённого источника.

Нативный доступ к устройствам, права, упаковка помощника и UI этой проверкой
не подтверждаются.

2026-10-19, первая итерация: `archive_device` и `native_archive_opening` PASS
на macOS arm64, Linux x64 и Windows x64. На macOS дополнительно повторён
`archive_device` после ограничения поиска началом потока. Все проверки использовали
существующие incremental build trees и compiler caches. Физические устройства,
системные запросы прав и UI в этой итерации не запускались.

2026-10-19, native access iteration: `archive_device`, `device_input`,
`native_archive_opening` PASS на macOS arm64, Linux x64 и Windows x64;
`device_access` PASS на macOS (O_RDONLY fd read/lifetime, отказ writable fd,
отмена и поддельный/stale выбор без elevation). macOS app/helper собраны;
private CMake install, ad hoc codesign verification и запуск установленного
helper без прав проверены. Helper использует QtCore из установленной .app.
Системный диалог авторизации, successful root fd handoff и реальный raw disk
не проверены; соответствие user workflow этим не установлено.

2026-10-19, Linux native access iteration: `udisks_device`, `device_access`,
`device_input`, `archive_device` PASS на настроенном Linux x64 runner.
Протокольная проверка использует частную D-Bus-шину, а не пользовательскую
system/session bus: внешние/внутренние устройства, partitions, busy/mounted,
смена носителя, policy denial/dismissal, отмена preflight без последующего
запроса прав, отмена pending authorization и отказ
regular-file fd вместо block-device. Kernel sysfs используется только как
источник метаданных сектора; содержимое физических устройств не читается.
Нативный successful OpenDevice/block-fd и реальное окно polkit ещё не проверены.

2026-10-19, Windows native access iteration: helper собран на Windows x64;
`device_access_win`, `device_pipe_win`, `device_input`, `archive_device`,
`native_archive_opening` PASS. После последнего уточнения mounted-state и
cleanup pending I/O повторены два native suites. Проверены Unicode-file read,
параллельные независимые readAt, освобождение handle, отказ write-only handle,
stale selection, отмена до elevation, обмен через частный named pipe,
ошибка/разрыв/неверный размер ответа и отмена pending read. Используются только
обычные временные файлы и pipe; виртуальные диски не создаются и не подключаются.
Реальный raw disk, UAC, privileged peer exchange и установленный Windows package
ещё не проверены. UI остаётся следующим интеграционным компонентом.

2026-10-19, stream-to-Agent integration: `archive_device`, `device_folder`,
`agent_opening`, `agent_lifecycle`, `native_folder_opening` PASS на macOS arm64,
Linux x64 и Windows x64. `device_folder` открывает частный GPT/ext4 fixture
через авторизованный stream, переходит в Unicode-папку и копирует выбранный
файл штатным `IFolderOperations::CopyTo`. Проверяются refresh с восстановлением
папки и retirement старого proxy, отмена/потеря источника при копировании,
отказ удаления, освобождение native lease, отсутствие CLI listing и полного
считывания раздела. Дополнительно проверены отмена/потеря источника при открытии
с успешным повторным открытием. Synthetic feature declarations проверяют отказ
прямого чтения при сохранении исходного поведения обычного образа.
На macOS первичный native-folder запуск использовал несовпадающий system `7zz`;
повтор с подготовленным pinned runtime PASS. Первичный device-folder test выявил
неподключённый device flag в Agent, исправление проверено тем же сценарием.
Отображение UI, нативная авторизация и физические устройства этой итерацией
не подтверждаются; явные ext2/ext3 fixtures ещё не проверены.

2026-10-19, UI integration: одиннадцать focused suites (`device_ui`,
`device_folder`, `archive_device`, `app_copy_routing`, `file_menu_policy`,
`ui_geometry`, `menu_policy`, `archive_metadata_ui`, `language`,
`language_main_dialog`, `language_standard_buttons`) PASS на macOS arm64,
Linux x64 и Windows x64. После локальных уточнений повторены UI/command-policy
consumers, затем финальный `device_ui` на трёх ОС. macOS app собрана.
Проверка выявила и устранила лишнюю зависимость лёгкого menu consumer от
process backend; source inspection factory выделена в reader module.

`device_ui` проходит от пункта меню через выбор GPT-раздела к текущей панели:
Unicode folder/file, выбранное и полное копирование, readonly availability,
refresh и Parent с возвратом в исходный каталог. Отдельно проверяются пустой
список/Refresh, отказ в правах и retry, Escape во время pending authorization,
Return на выбранном разделе и освобождение exclusive source после Back.
Настройки и содержимое — частные временные fixtures; реальная авторизация и
доступ к пользовательским устройствам не вызываются.

Просмотрены текущие normal/compact EN/RU widget JSON chooser (devices/partitions)
и главной панели с read-only badge: группировка, размеры, footer, clipping и
enabled state. macOS использует native Cocoa; sandbox WindowServer refusal
перепроверен вне sandbox, PASS. Linux/Windows используют offscreen Qt: rendered
native appearance, физическая клавиатура/VoiceOver и системные запросы прав
этими проверками не подтверждаются. Screenshots не требовались.

2026-10-19, итоговая проверка образцов и границ: добавлены обычные ext2/ext3
fixtures, созданные `mke2fs` без монтирования; ext3 содержит чистый journal.
`archive_device` проверяет их Unicode extraction, отмену и ошибку чтения вместе
с ext4/MBR/GPT. После последних исправлений `device_input`, `archive_device`,
`device_folder`, `device_ui` PASS на macOS arm64, Linux x64 и Windows x64.
Runner receipt: `20261019T044047Z-43c8e0a0`; локальный UI использует Cocoa.

Повторно сравнены upstream `CLimitedInStream::Read/Seek`, создание ограниченного
потока и GPT sector-size branches; исходные тела не менялись. Платформенная
граница дополнительно отказывает разделам с невыровненными к native sector
границами: логическое ограничение само не предотвращает физическое чтение
соседнего сектора. Второе исправление учитывает начальный sector prefix в лимите
native transfer. Регрессия большого невыровненного чтения воспроизвела отказ
до исправления и затем прошла; отдельный сценарий проверяет отказ неверных
границ до native I/O. Форматные алгоритмы не добавлялись и не изменялись.

Частные install-only пакеты macOS/Windows собраны штатным `package.py` на
`a10ef00`; helper не менялся в последующем исправлении потоков. macOS прошёл
ad hoc `codesign --verify --deep --strict`; установленный helper каждой ОС
запускается с системным PATH без Qt environment и ожидаемо отклоняет запуск
без аргументов. Это проверка loader closure, а не доступа с повышенными правами.
Windows runtime targets также собраны. Первичный link shell extension выдал
LNK1163; идентичный повтор и затем весь выбранный build прошли без удаления
objects, изменения flags или очистки compiler cache. Временные install payloads,
генерация fixtures и разобранный диагностический dump удалены; incremental
builds и caches сохранены.

Покрыты наблюдаемые сценарии выбора, просмотра, копирования, refresh/Parent,
readonly state, границ потока и отмены/ошибок с возможностью повторить операцию.
Обычные архивные consumers проверены в предыдущих focused итерациях. Реальные
raw-device I/O, успешная privileged session/fd передача с системной авторизацией,
UAC/polkit/AppleScript prompts и rendered native Windows/Linux UI не подтверждены.
Новые виртуальные диски не создавались и не подключались; slow suites и полная
release-приёмка в scope этой проверки не входили.

2026-10-19, отказ macOS после пароля: пользователь подтвердил запуск из Finder,
`Operation not permitted` после авторизации и успешное получение доступа после
включения Full Disk Access с последующим повторным вводом пароля. Это ручное
свидетельство пользователя об открытии устройства; чтение/копирование содержимого
им ещё не подтверждено. Пароль администратора и privacy permission независимы.
Helper EPERM/EACCES теперь объясняется в EN/RU с инструкцией включить доступ и
перезапустить приложение. Протокол, привилегии и форматные обработчики не менялись.
При проверке длинного русского сообщения найдена и исправлена коллизия со списком
в компактной геометрии. Native macOS `device_access`, `language`, `device_ui`
PASS; после изменения геометрии повторён `device_ui`. Linux/Windows `device_ui`
и `language` PASS (`20261019T162540Z-b0027ef6`). EN/RU JSON normal/compact
проверяют размещение отказа и успешную повторную попытку; native rendered
Windows/Linux не проверялись. Предыдущие записи о непроверенной авторизации
описывают границы автоматических тестов первоначальной версии.
