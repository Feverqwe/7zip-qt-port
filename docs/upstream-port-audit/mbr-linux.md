# MBR: primary/EBR чтение и геометрия устройства

## Источники и source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/Archive/MbrHandler.cpp` скопирован целиком в `src/` под прежним путём.
Оба engines используют runtime overlay из [manifest](../upstream-baseline.json).
Полное форматирование нового C++ unit учитывается отдельно от функциональности.
Сохранены исходные parser/types, CHS presentation, type/property tables,
filesystem probes, extraction и общий stream framework. Перестроен обход
внутри прежнего `ReadTables`; новый parser поверх Qt не добавляется.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`, `block/partitions/msdos.c`, `check.h`,
`include/linux/msdos_partition.h`, `block/partitions/core.c::blk_add_partition`.
До реализации прочитан весь `msdos.c`, включая callers и вложенные subtype
parsers, структуры, primary validation и extended walk; проверена последующая
фильтрация/ограничение геометрии block layer. Со стороны 7-Zip прочитаны весь
handler, `HandlerCont.h`/`.cpp`, extent/limited streams и consumers из
`ArchiveDevice`. После изменений selected read scope повторно сопоставлен.
В primary/EBR итерации Linux C bodies не копировались/не переводились в C++:
исправлен пригодный исходный C++ parser 7-Zip; исходные notices и лицензия
сохранены. Дополнительный перенос subtype readers на C описан ниже.

Перед добавлением geometry bridge сопоставлены `IStreamGetProp` из `IStream.h`
и property implementation `CInFileStream::GetProperty` из `FileStreams.cpp`.
Локальный direct-device adapter публикует уже проверенный размер логического
сектора через стандартный `kpidSectorSize`; остальные свойства остаются empty.
`ReloadProps` сохраняет immutable snapshot открытого разрешённого handle:
переоткрытие устройства или расширение доступа не выполняется. Перенос native
file-information API в этот adapter не требуется.

## Завершённый сценарий

Primary entries могут идти в любом порядке и пересекаться. Неиспользуемая
запись определяется нулевым размером, а не нулевым type; CHS остаётся свойством,
не условием пригодности LBA. Проверяются boot flags всех primary slots,
включая пустые; flags EBR не фильтруют data. LBA/end/size вычисляются в 64 битах.
Out-of-source start пропускается; пересекающий конец span ограничивается
доступными полными секторами, как `blk_add_partition`. Валидные соседи сохраняются.
То же последующее ограничение применено к [APM](apm-linux.md).

Extended types 0x05/0x0F/0x85 проходят один reader. В EBR сначала читаются все
data slots, затем обход продолжается по первой непустой extended link, независимо
от её положения. Data offsets остаются относительными текущему EBR, links —
первому EBR. Сохранены Linux bounds для третьего/четвёртого slots; исходные
declared bounds extended-контейнера используются до EOD clipping. Допускается
немонотонная цепочка. Продолжение итеративное, без роста C++ стека на каждый link.
Прежний предел глубины/числа items 128 больше не обрывает пригодную длинную цепочку.

Сохранены лимиты Linux: 100 links без data и 251 logical entries при 256 slots
и начале logical numbering с 5. Счётчик учитывает пригодные table entries до
block-layer фильтрации, а не только опубликованные spans. Дополнительная archive
защита завершает обход при повторном EBR address: циклическая metadata не
публикует сотни дубликатов до исчерпания quota. Это явное отличие от kernel
walk повреждённой циклической карты, а не native-positive parity case.
Непригодная signature/beyond-source EBR завершает цепочку, сохраняя прочитанное;
ошибки чтения таблицы E_FAIL/E_ABORT прекращают Open и очищают partial items. Повторный Open
снова читает источник. Проверки mount/repair и записи не добавляются.

Для direct-device input используется известная геометрия 512..65536 bytes,
как Linux использует queue logical block size. Bare MBR image не содержит
однозначного размера сектора; без свойства сохранён исходный default 512 и
4Kn filesystem heuristic. Это ограничение информации `IInStream`, а не
доказательство геометрии по отсутствующим on-disk полям. Tail/gap items и
представление extended-container сохраняют archive conventions 7-Zip:
они не объявляются списком созданных Linux block devices. Хвост определяется
максимальной границей, поэтому порядок/пересечение записей его не смещает.

## Проверки

CLI обоих engines проверяет 23 варианта primary/EBR: порядок, overlap, CHS,
нулевой type, пустые slots, clipping/skip, 32-bit end overflow, LBA zero,
три extended types, link-first/OS2 slots, garbage filtering, backwards links,
first-link precedence, cycle, damaged EBR signature, ignored EBR flags,
170 logical partitions и extended bounds за EOD. Проверяются публичные
offset/size, automatic outer-container selection и exact extraction bytes;
malformed primary завершает процесс normal code 2. CLI runtime изолирован
от посторонних plugins, сами shipped binaries/module неизменены.

ABI suite загружает полный runtime и передаёт настоящий `CreateDeviceInput`
через mock handle с обязательным выравниванием. Восемь геометрий 512..65536
проверяются через реальные offsets и прочитанные bytes, а не значения property
сами по себе. Source failure/cancellation после первой прочитанной logical
partition проверяет cleanup и reuse. Существующий direct-device integration
проверяет выбранную filesystem, failure/cancel и ограниченный partition stream
до последнего байта разрешённого диапазона. Его MBR fixture согласован с
4096-byte геометрией device mock вместо прежнего несовпадения единиц LBA.

Native Linux `6.8.0-147-generic`: 28 disposable MBR read-only loop images,
из них 25 с точной геометрией и payload (включая 170 logical entries и hardware
sectors 1024/2048/4096), три invalid primary без partitions. Ещё два APM image
подтвердили EOD clipping/skip с точными bytes. Loop devices отключены,
временные images удалены. Cyclic metadata в kernel не передавалась;
геометрии 8192..65536 подтверждены ABI/mock, не native loop driver.

Focused scope: `mbr`, `mbr_stream`, `archive_device`, `device_folder`, `device_input`,
`gpt`, `gpt_stream`, `apm`, `apm_stream`, `runtime_cache`, `runtime_package`,
`upstream_baseline`, `upstream_inventory`. Full/slow gate и UI appearance
acceptance не требуются для этой невизуальной области; проверяются прямые
потребители изменённого input adapter и handlers.
macOS arm64 13/13 PASS (2.62 s), Linux x64 и Windows x64/MSVC 13/13 PASS,
receipt `20261020T054300Z-d8f6f98a`. Exact baseline verification — 422 exact,
223 adapted, 11 local paths, одна upstream revision. Локальный PATH начинается
с проектного `build/codec-runtime`; Homebrew sevenzip не используется.

## Linux C subtype readers

Дополнительная итерация переносит FreeBSD/NetBSD/OpenBSD disklabels,
Solaris VTOC, Unixware slices и Minix subpartitions из того же Linux baseline.
Поиск signatures/types в полном pinned 7-Zip checkout не обнаружил пригодного
reader для этих labels. Исходные структуры и 336 строк subtype bodies/table
сохранены без изменений в `src/block/partitions/msdos.c` и компилируются как C11.
`nr_sects`, `start_sect` и signature helper также взяты из исходного C unit.
Точный `include/linux/msdos_partition.h` хранится в `third_party/linux/`.
[Source hashes и scope](../msdos-linux-baseline.json) закрепляют оригиналы.

| Linux scope | Перенесённое чтение |
| --- | --- |
| `parse_bsd` / `parse_freebsd` / `parse_netbsd` / `parse_openbsd` | Signature, bounded label count, unused entries, FreeBSD relative-offset convention, parent containment и full-parent duplicate suppression. |
| `parse_solaris_x86` | Sanity/version, 8/16-slice layouts, offsets относительно parent. |
| `parse_unixware` | Обе signatures, все 16 slots с пропуском slice 0/unused labels, абсолютные addresses. |
| `parse_minix` | Secondary MBR signature/first type, четыре Minix slots и абсолютные addresses. |
| `subtypes` dispatch | Все исходные subtype IDs; второй проход только по primary entries после logical EBR partitions. |

`reader.c` и `userspace.h` заменяют page-cache sectors на временный 512-byte
buffer, stream callback, endian services, bounded partition slots и освобождение
buffer. Source callback status хранится в контексте: исходные void parsers
могут прекратить label scan, но E_FAIL/E_ABORT не превращается в успешное Open.
Запрос за физическим концом не выполняет I/O; пригодные соседние labels остаются
доступными. Kernel-only logging не влияет на результат format reader.

При включении ASan освобождение сектора в `parse_bsd` перенесено после
последнего чтения `d_npartitions` для диагностического сообщения. Userspace
adapter освобождает buffer немедленно, поэтому исходное расположение release
оставляло обращение к освобождённой памяти. Структуры, обход и исходный baseline
сохранены; существующие BSD fixtures воспроизводят ошибку с ASan.

7-Zip MBR handler получает C output через узкий adapter. Разные структуры
используют разные sector units: parent geometry масштабируется из известного
logical sector в Linux 512-byte units, а label sectors/slices не переводятся
ошибочно в 4Kn units. Новый byte-offset в исходном item model сохраняет точные
512-byte subdivisions. Extraction и `CExtentsStream` остаются исходными;
последующие clipping/skip правила совпадают с block layer. Parent partitions
сохраняются рядом с subpartitions, как в Linux, без новой иерархии имён.

Read-only `sevenzip_mbr` использует отдельную GPL module boundary, стандартные
archive exports и обычный `Formats` deployment/install на трёх ОС. Общий
`HandlerCont.cpp` компилирует нужный container scope, а `FatHandler.cpp` — свой
исходный header probe без второго FAT handler/registration. Новые compile guards
не меняют обычные сборки этих units. Дублирование parser bodies или RAR code
не добавляется. Dynamic engine использует модуль вместо собственной MBR
registration; standalone engine сохраняет C++ primary/EBR reader. Linux C
не линкуется в standalone/core engine, его subtype extensions относятся
к поставляемому dynamic-module path приложения.

Direct-device partition lease теперь допускает byte ranges, не выровненные
по логическому сектору: Linux labels могут описывать такие subdivisions.
Native read всё ещё выровнен и ограничен полным разрешённым device handle;
внутреннее округление может захватывать соседние bytes аппаратного сектора,
но наружу bounded stream возвращает только выбранный диапазон. Не открывается
новое устройство и не расширяется разрешённый device handle. Bounds, EOF,
жизненный цикл lease и journal policy сохраняются; raw handle не читает
невыровненные запросы.

CLI dynamic-module matrix проверяет 27 subtype variants, в том числе ignored
invalid labels, BSD relative/absolute/unused/containment/count boundaries,
Solaris 8/16/version, Unixware count/slice rules, Minix secondary-table gating,
combined primary labels и совместное logical EBR/subtype numbering. Проверяются
публичные spans, exact bytes и неизменность источника. ABI проверяет все шесть
flavours на 512/4096 геометриях, failures/cancellation/reuse, а полный
`ArchiveDevice` catalog — выбранную невыровненную BSD lease после закрытия
исходного view. Existing device tests проверяют aligned native reads,
byte-exact unaligned views и EOF без публикации соседних bytes.

Native Linux `6.8.0-147-generic` со всеми четырьмя subtype CONFIG options:
33 disposable read-only loop images, 47 exact partition payloads и geometry,
включая все шесть flavours на 4Kn. Loops отключены, fixtures удалены.
macOS arm64 и Linux x64: 17 focused suites PASS; scope предыдущего сценария
расширен `fat`, `fat_module`, `native_rar_memory`, `codec_probe` из-за reuse
compile scopes/registration. Linux/Windows receipt `20261020T060938Z-5adbebc7`:
Windows 16/17, `device_input` timeout 15 s; isolated same-code retry PASS
в receipt `20261020T061452Z-ae338ada`, без изменения timeout. Причина первого
timeout не установлена, повторный failure не воспроизведён. macOS application
target также собран; runtime Formats path проверен CLI/catalog consumers.
Полный install/release gate и UI appearance acceptance не выполнялись.

## Итоговая сверка classification и read scope

Заключительная итерация переносит без изменений `aix_magic_present` и
`is_extended_partition` из того же `msdos.c`. C helper вызывается для root
до проверки 55AA, как в Linux: EBCDIC IBMA и `_LVM` в 512-byte sector 7
не принимаются за DOS table. Linux/Solaris-swap/RAID/LVM/extended type в любом
из четырёх slots сохраняет приоритет MBR, даже при нулевом размере slot.
EFI protective type в любом slot отклоняет MBR в пользу отдельного GPT
handler, включая повреждённый GPT, для которого нет пригодной DOS table.
Обработка соответствует Linux с включённым EFI parser, имеющемуся в проекте.
Callback E_FAIL/E_ABORT не подменяется успешным классификационным fallback;
истинный out-of-device auxiliary address не выполняет чтение.

AIX LVM — отдельный формат, не исходный 7-Zip LVM handler: проверены его
`LABELONE`/`LVM2 001` signatures и структуры Linux `aix.c`. Новый AIX volume
reader не добавляется; classifier предотвращает выдачу AIX за MBR.
FAT superfloppy ветвь Linux возвращает признанный носитель **без partitions**.
Архивный MBR API представляет это отказом MBR (`S_FALSE`), а существующий FAT
module читает сам том. Не добавляется пустая фиктивная MBR hierarchy.
Automatic routing и exact extraction FAT12/16/32 проверены отдельно.

Повторная сверка общего `GetFileSystem` нашла потерю E_ABORT в необязательном
probe. Helper теперь передаёт status обоим consumers. MBR прекращает Open
при отмене read/seek probe, GPT также закрывает partial state; обычный E_FAIL
необязательной эвристики сохраняет прежний unknown-type fallback. Сами
NTFS/exFAT/FAT/UDF probes и original naming policy не заменяются новым parser.
ABI regression проверяет сохранение обычного fallback, E_ABORT, нулевые
published items после отмены и успешное повторное открытие обоих handlers.

| Полный Linux MSDOS read path | Состояние source transfer/adaptation |
| --- | --- |
| Disk geometry, `nr_sects`/`start_sect`, primary validation | Исходный C++ parser 7-Zip актуализирован; known device geometry и byte spans подключены. |
| AIX routing, signature, boot flags, FAT-superfloppy, EFI selection | Linux C AIX helper и EFI filter подключены; различия return model архивного/FAT API объяснены и проверены. |
| `parse_extended` data/link traversal и bounds | Пригодный C++ parser исправлен; native valid chains совпадают, повторный cyclic address завершается без дубликатов. |
| BSD/Solaris/Unixware/Minix и second pass | Исходные C bodies/dispatch полностью подключены к root parents и 512-byte sector services. |
| Block-layer EOD clipping/skip | Применяется к опубликованным spans без out-of-source bytes. |
| UUID/RAID registration, device creation/capacity unlock, logging | Kernel/platform services; не отсутствующие archive read functions. Disk signature/property presentation 7-Zip сохранены. |

Source comparison archive read scope MBR завершён; это не перенос Linux
block-device driver целиком. Standalone сохраняет C++ path, новые Linux C
classification/subtype branches доступны через поставляемый dynamic module.

Заключительная validation: 16 classification variants через CLI, source errors,
Linux-type exemptions и cancellation/reuse через ABI, три FAT superfloppies
с неизменными source bytes и exact payload. Native Linux `6.8.0-147-generic`
подтвердил 19 read-only loop images; FAT12/16/32 не создали partitions,
смонтированы `ro,utf8=1` и отдали по 1044 точных bytes Unicode-файла.
Первый диагностический mount с `iocharset=utf8` отклонён из-за отсутствующего
NLS charset; kernel diagnostic подтвердил причину. Никакие kernel/modules
не устанавливались. Все mounts/loops/temporary images очищены.
macOS arm64 13/13 PASS (7.19 s); Linux x64 и Windows x64/MSVC 13/13 PASS,
receipt `20261020T063827Z-1b8d2928`. Scope: `mbr`, `mbr_stream`, `fat`, `fat_module`,
`gpt`, `gpt_stream`, `archive_device`, `device_input`, `codec_probe`,
`runtime_cache`, `runtime_package`, `upstream_baseline`, `upstream_inventory`.
Full/slow, physical-device и UI acceptance не заявляются.

Общий scope и остальные форматы — в [едином реестре](filesystems-linux.md).
