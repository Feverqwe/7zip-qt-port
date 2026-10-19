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

Существующие адаптации Agent и QtArchiveFileResource рассчитаны на file-only
transport и требуют отдельного подключения устройства; копирование новых
парсеров или дублирование алгоритмов там не требуется. POSIX device size,
выравнивание, разрешения и обнаружение отсутствуют в этом первом компоненте.
Windows source device I/O уже существует, но не означает готовность нового UX.

## Текущее состояние

Реализуется [первая версия](../direct-device-reading-plan.md).
Первый компонент предоставляет stream-level discovery и native Ext-reader;
пользовательский UI ещё не подключён. Добавлен native access macOS/Linux;
платформенный адаптер Windows ещё не реализован.
Это не завершённый пользовательский сценарий.

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
