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
Linux C bodies в этой итерации не копируются/не переводятся в C++: исправляется
пригодный исходный C++ parser 7-Zip; исходные notices и лицензия сохранены.

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

## Открытая часть полного MBR read scope

Это завершение **primary/EBR scenario**, а не всего Linux MSDOS module.
Остались прочитанные, но ещё не перенесённые subtype readers:
FreeBSD/NetBSD/OpenBSD disklabels, Solaris VTOC, Unixware slices и Minix
subpartitions. Поиск соответствующих signatures/types в полном 7-Zip checkout
не обнаружил пригодной реализации. Их нужно переносить из Linux в C с узким
read callback/partition adapter и отдельной GPL module boundary; нельзя
заменять C реализацию независимо написанным C++ parser. AIX delegation и
FAT-superfloppy classification также остаются в source scope callers.
RAID registration, device UUID publication и queue/platform services сами
по себе не являются отсутствующими archive read functions.

Общий scope и остальные форматы — в [едином реестре](filesystems-linux.md).
