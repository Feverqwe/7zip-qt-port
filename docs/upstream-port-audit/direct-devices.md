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
платформенная авторизация, discovery и UI ещё не подключены.
Это не завершённый пользовательский сценарий.

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
