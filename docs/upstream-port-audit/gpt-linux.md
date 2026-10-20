# GPT: validated tables и partition streams

## Источники и source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
`CPP/7zip/Archive/GptHandler.cpp` скопирован целиком в `src/` под исходным
путём; `HandlerCont.cpp` перемещён из `third_party/7zip` в `src/` как адаптированный
модуль. Их прежние symbols, class/module boundaries, partition properties,
type/name presentation и extraction framework сохранены. Baseline и runtime
overlays фиксируются в [manifest](../upstream-baseline.json). Оба engine получают
одни изменения; sibling upstream остаётся read-only. Полное форматирование
двух перемещённых/скопированных C++ units учитывается отдельно от функциональности.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`,
`block/partitions/efi.c`, `efi.h`, `check.h`. До реализации сравнены полные
`is_gpt_valid`, `is_pte_valid`, `is_pmbr_valid`, allocation/read helpers,
`find_valid_gpt`, `compare_gpts`, `efi_partition`, структуры и callers.
Со стороны 7-Zip сравнены весь GPT handler, `HandlerCont.h`/`.cpp`, ограниченные
и extent streams, registration/probe, image-extension consumer и VHD footer,
property/GetStream consumer для regression fixture. После изменений scope
сопоставлен повторно. Linux C bodies не копируются: usable parser — исходный
C++ 7-Zip; исправлены его проверки и повторно используется тот же parser для
primary/backup. Notices/лицензия 7-Zip сохранены; новый GPL bridge не требуется.

## Реализованный сценарий

Header/table reader выделен из исходного `Open2`, без замены формата новым
parser. Header CRC охватывает validated размер 92..logical-sector-size;
проверяются signature, current LBA, порядок и disk bounds usable range,
ненулевая таблица, bounded table address/size и entry-array CRC. Table span
проверяется до allocation/seek, читаются только реальные bytes массива.
Непригодная запись за пределами диска пропускается, сохраняя пригодные соседние
разделы, как `is_pte_valid`. Обратный диапазон по-прежнему отклоняется:
archive stream не представляет unsigned wraparound как размер раздела.
Alternate-LBA вне образа не дискредитирует пригодный primary и не раздувает
archive physical size. Failed open очищает опубликованные entries и buffer.

Поддержаны logical sectors 512/1024/2048/4096; signature больше не требует
конкретного revision, который Linux также не проверяет. Автоматический probe
и вложенные image-extension consumers согласованы с этими geometry и protective
MBR. Сохранены исходные archive quotas и поддержка 128..4096-byte entries;
Linux принимает только 128-byte entries. Это сохранённая дополнительная
возможность 7-Zip, а не обещание native Linux compatibility для larger entries.

Пригодный primary имеет приоритет, даже если backup отличается или повреждён.
При повреждённом primary archive reader с protective MBR проверяет end-of-image
backup тем же CRC/bounds reader. Это **явная API adaptation**: в Linux fallback
при invalid primary требует `force_gpt`; default kernel не обязан пробовать
backup. Архивный интерфейс не предоставляет kernel command line, поэтому
такое восстановление разрешено для image с protective MBR. Native force branch
не трактуется как безусловная default policy. Driver-specific
`alternative_gpt_sector` не подменяется эвристикой для обычного `IInStream`.
Source E_FAIL/E_ABORT прерывает чтение, не заменяется успешным fallback.

Заключительная [MBR classification сверка](mbr-linux.md) исправила также
E_ABORT общего filesystem probe: GPT прекращает отменённое открытие и очищает
опубликованные entries, сохраняя прежний optional unknown-type fallback для
обычной неудачи probe. ABI проверяет отмену Windows-partition probe и reuse;
read-only table parser и backup policy не меняются.

ABI regression воспроизвёл ошибку исходного `CHandlerCont::GetStream`:
два `CLimitedInStream` доверяли каждый своему кэшу общей физической позиции,
и чередование чтений смещало данные. Container теперь использует готовый
`CExtentsStream` с одним непрерывным extent; его существующий reader каждый раз
восстанавливает позицию source. Extraction использует прежний CopyCoder path.
Новый disk parser, private test access или второй потоковый алгоритм не добавлены.
Изменение действует также на остальные `CHandlerCont` containers; выбор более
широкой validation связан именно с этим общим consumer.

## Проверки и границы

Native Linux `6.8.0-147-generic` проверил 26 disposable read-only loop images:
10 вариантов дали ожидаемые partition start/size и точный payload; 12 invalid
таблиц не дали GPT partitions; четыре invalid-primary/valid-backup подтвердили
отсутствие default fallback. Эти четыре archive cases проверяются по source
force branch и объявленной adaptation, а не выдаются за native default parity.
Reversed ranges и retained larger entries в native kernel не передавались.
Loops отключены, images удалены.

CLI проверяет оба engine, forced/automatic raw-image recognition, Unicode names,
payload/source immutability, normal code 2, primary precedence и извлекаемое имя
внутреннего GPT из fixed VHD. ABI проверяет independent/interleaved/retained
streams, EOF, source failures, отмену, partial-open cleanup и повторное открытие;
`ArchiveBackend` извлекает выбранный partition из 2048-byte-sector образа.
Focused acceptance: macOS arm64, Linux x64 и Windows x64/MSVC — десять suites
`gpt`, `gpt_stream`, `archive_device`, `device_folder`, `native_rar_memory`,
`codec_probe`, `upstream_baseline`, `upstream_inventory`, `runtime_cache`,
`runtime_package`. Remote receipt `20261020T050233Z-6142d899`: Linux 10/10,
Windows 9/10 с timeout CLI matrix из-за повторной загрузки полного plugin set.
CLI suite затем изолированно копирует неизменённые shipped engine/module без
посторонних plugins; receipt `20261020T050508Z-e07ce4a7`: обе ОС PASS,
Windows 6.32 s. ABI/backend suite сохраняет полный runtime. Exact baseline
verification: 422 exact, 221 adapted, 11 local units, одна upstream revision.
Локальные проверки используют `build/codec-runtime` в начале `PATH`, чтобы
системный `7zz` другой версии не подменял prepared engine; Homebrew-пакет
для этих проверок не требуется.

Из-за общего `CHandlerCont` дополнительно выполнены полная default build
и default CTest на macOS: 367/373 PASS. Шесть UI failures (`ui_layout`,
`compression_encryption`, `panel_creation_path`, `panel_activation`, `panel_key`,
`ui_parity_system`, включая два SIGSEGV) воспроизведены адресно после временного
возврата обоих исходных 7-Zip units; текущими изменениями они не вызваны.
После восстановления адаптированных units runtime и affected targets
пересобраны, десять focused suites повторно PASS. Сам `check.sh` остановился
на форматировании неизменённого `src/erofsreader.h`; общий gate не объявляется
успешным. Slow suites и UI appearance acceptance не выполнялись.

Проверки ограничены fixtures; реальные носители, firmware/device callbacks,
mount/repair и запись не добавляются. Общая задача остальных shared formats
сохраняется в [едином реестре](filesystems-linux.md).
