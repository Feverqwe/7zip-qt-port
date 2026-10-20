# Проверка и актуализация форматов по Linux

Запрос от 2026-10-19: проверить существующие файловые системы, представленные
в основном дереве Linux, исправить расхождения чтения и добавить read-only EROFS.
Каждая завершённая итерация оформляется отдельным коммитом.

Уточнение пользователя от 2026-10-19 расширяет задачу: включить также другие
форматы проекта, для которых в ядре Linux есть реализация. Scope больше не
ограничивается файловыми системами. Для дополнительных форматов требуется
подтвердить фактический parser/decoder в обоих деревьях; общие названия или
наличие соответствующей userspace-программы не доказывают поддержку в ядре.

Дальнейшее уточнение пользователя: отсутствующие возможности формата, которые
есть в эталонной версии ядра, требуется реализовать. Перечень различий — scope
актуализации, а не основание считать модуль завершённым с ограничениями.
Для файловых систем это включает отсутствующие read features; kernel-only
VFS/platform services и запись не подменяют работу над чтением формата.

## Эталон и границы

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da` от 2026-10-17.
Это также воспроизводимый baseline существующего [exFAT-переноса](exfat-linux.md).
Зеркало для получения отдельных файлов:
<https://github.com/torvalds/linux/tree/58161ead29e50d70a317ea26b472e0c7508e69da>.

7-Zip: <https://github.com/ip7z/7zip.git>,
commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Обработчики встроены в поставляемый динамический engine и standalone engine;
их исходники берутся из read-only sibling checkout при подготовке runtime.
Локальные исправления должны попадать в оба engine, сохранять исходную структуру
модулей и учитываться в provenance/cache runtime. Изменение sibling запрещено.

Linux определяет корректность интерпретации дисковых структур и чтения данных.
Его VFS, запись, журналирование, mount, восстановление повреждённых томов,
управление устройствами и производительность page cache не входят в задачу.
Отказ от неподдерживаемой возможности должен быть явным; успешное извлечение
не должно возвращать неверные данные. Отличия поддержки возможностей
фиксируются отдельно от ошибок в заявленном read scope.

## Последовательность интегрированных итераций

| Область engine | Эталон Linux | Работа |
| --- | --- | --- |
| exFAT/TexFAT | `fs/exfat` | Повторное сравнение C-переноса, metadata/chain/stream границ и ошибок; исправления и существующие ABI/backend consumers. |
| FAT12/16/32 | `fs/fat` | Boot geometry, FAT и длинные имена, цепочки и извлечение. |
| ext2/3/4 | `fs/ext2`, `fs/ext4` | Superblock/features, inode, directory, extents/indirect/inline data и sparse reads. |
| NTFS | `fs/ntfs3` | Boot/MFT, attributes/runlists, directories, sparse/compressed file reads. |
| HFS/HFS+ | `fs/hfs`, `fs/hfsplus` | Volume/B-tree, extents, имена, data/resource forks. |
| cramfs, SquashFS | `fs/cramfs`, `fs/squashfs` | Metadata, directories, blocks/fragments, decompressor boundaries. |
| ISO9660, UDF | `fs/isofs`, `fs/udf` | Descriptors, names/extensions, extents, sparse и embedded data. |
| EROFS (новый модуль) | `fs/erofs` | Сначала полный source comparison read path и зависимостей, затем C-перенос, 7-Zip ABI, deployment и end-to-end чтение. |
| Base64 | `lib/base64.c`, fscrypt/Ceph callers | Standard/URL-safe/IMAP alphabets, padding/tail validation и archive handler lifecycle. |
| ELF | `fs/binfmt_elf.c`, `fs/binfmt_elf_fdpic.c`, kexec/module readers | Headers, table/payload spans, Linux core numbering, metadata и archive streams; execution services не входят в byte reader. |
| PE | `crypto/asymmetric_keys/verify_pefile.c`, kexec callers | Headers, certificate/file spans и byte streams; общий COFF consumer проверяется вместе, kernel trust/execution — отдельные contracts. |
| EFI capsules | `drivers/firmware/efi/capsule-loader.c`, `capsule.c`, x86 EFI quirks | Common header/payload и Quark mapping; UEFIf проверяется как общий SDK consumer, firmware updates не выполняются. |
| Intel HEX | AS102/IQS5xx ASCII firmware readers | Extended address/EOF/separators, errors/streams и явный IQS5xx map profile; binary ihex_binrec — другой format. |
| TAR | Marvell CPT `load_tar_archive` | [Сверка reader и streams](tar-linux.md), physical alignment служебных записей, GNU/PAX/sparse consumers и update. |
| Legacy LZ4, lzop | `lib/decompress_unlz4.c`, `decompress_unlzo.c` | [Framing readers](linux-frames.md), bounded C callbacks, consumed boundary, checksum и nested CPIO consumers. |

APFS не входит в scope: соответствующего драйвера нет в основном дереве Linux.
GPT/MBR/APM проверяются как дополнительные форматы разметки дисков.
Наличие обработчика в engine не считается доказательством проверки по Linux.

Дополнительный scope уточняется по полному списку `7z i` поставляемого engine:
разметка GPT/MBR/APM, CPIO/initramfs, gzip/bzip2/LZMA/XZ/Zstd, Base64, загрузочные
образы ELF/PE/COFF и остальные фактически совпадающие parsers/decoders.
Проверка дополнительных форматов ещё не завершена; ограничения отдельных
подформатов Linux и 7-Zip должны быть указаны при source comparison.

Для каждой строки сначала сравниваются полные тела, заголовки, callers и
значимые ветви выбранного read scenario. Все найденные source-visible пробелы
этого scenario исправляются вместе либо получают конкретное обоснование
ограничения. Затем выполняются повторное сравнение и focused runtime-проверки
через публичные engine/ArchiveBackend интерфейсы на локальных fixtures.
Новые regressions должны выявлять конкретное неверное чтение или обработку
ошибки; проверки текста исходников не применяются.

## Census поставляемых форматов — 2026-10-20

Повторно получены полные `7z i` и `7zz i` текущего runtime: 66 и 61 format
строки соответственно, включая console Hash. Разница — EROFS/exFAT/Brotli/Lz4Legacy/Lzop
modules; заменяющие filesystem plugins не создают дополнительные names.
Ниже классифицирован весь union этих списков, а не только первоначальные FS.
Это датированное source evidence, не фиксированный публичный список приложения.

[Census baseline](../formats-linux-baseline.json) закрепляет полный source
archive Linux, retrieval URL/hash и ключевые candidate/negative-comparison units.
В существующий reference cache получены и сверены 65 697 C/header/assembly/Rust/
linker/Kbuild units всех kernel source directories. Tools/scripts/docs не
выдавались за in-kernel readers. Case-sensitive variants Linux сохранены отдельно
с исходными path/hash mappings: macOS filesystem не может хранить все такие
имена в одном directory. Поиск включал их содержимое. Source inventory и весь
format correctness audit — разные свидетельства.

| Форматы из union | Результат source census |
| --- | --- |
| CramFS, EROFS, Ext, FAT, HFS, Iso, NTFS, SquashFS, Udf, exFAT | Общие disk formats: Linux `fs/*`; индивидуальные audits выше владеют read completeness. |
| APM, GPT, MBR | Общие partition readers `block/partitions`; включая вложенные MBR subformats, а не отдельные registry names. |
| Cpio, tar | `init/initramfs.c` и Marvell CPT `load_tar_archive`; [TAR read audit](tar-linux.md) включает declared-size mode и SDK extensions. Composite initramfs остаётся отдельным сценарием. |
| ELF, PE, UEFIc | Настоящие executable/capsule readers; byte inspection отделён от execution, trust и firmware services. |
| Base64, IHex | `lib/base64.c` и AS102/IQS5xx ASCII readers; binary ihex helpers не использованы как ложное доказательство. |
| bzip2, gzip, lzma, xz, zstd | Общие framed streams/decoders `lib/decompress_*`, XZ/Zstd libraries; composite initramfs остаётся открытым. |
| Lz4Legacy, Lzop | Linux boot wrappers `lib/decompress_unlz4.c`, `lib/decompress_unlzo.c`; добавлены отдельным GPL module с [read audit](linux-frames.md). |
| COFF, TE, UEFIf | COFF fields внутри Linux PE не доказывают standalone COFF reader. EFI capsule transport не разбирает TE/FV. SDK units проверены как общие consumers; отдельного in-kernel reader этих containers не найдено. |
| 7z, Cab, Chm, Hxs, Nsis, SWFc, Dmg, wim, zip | In-kernel container parser не найден. Совпадающие contained compression streams учитываются у actual consumers: DEFLATE/zlib, LZMA/XZ, NTFS3 LZX/XPRESS. Это не поддержка соответствующего container Linux. |
| Ar, Arj, Compound, FLV, LP, Lzh, MachO, MsLZ, Mub, Ppmd, Rar, Rar5, Rpm, SWF, Sparse, Split, Xar | In-kernel reader не найден по source bodies, names/magics и dispatchers. Android device-mapper tables не читают LP metadata или Android sparse images; firmware blobs и archive/linker host tools не считаются такими readers. |
| APFS, QCOW, VDI, VHD, VHDX, VMDK | APFS driver отсутствует в mainline. Block/virt frontends работают с sectors, а backing container декодирует host. UML COW имеет собственные MOOO headers, это не QCOW/VHD/etc. |
| Z, brotli, lzma86, Hash | Standalone counterparts не найдены. PPP BSD LZW использует MSB packet coding и иной state contract, а не Unix `.Z`; x86 BCJ внутри XZ не означает `.lzma86` header. Kernel hashes/checksums и binary OpenPGP-inspired metadata не являются Hash text-manifest reader. |

Таким образом, подтверждены 27 общих format names; остальные строки покрыты
отдельными dependency/container/subformat различиями выше. Это завершает
первичный census archive-format surface, но не заявляет завершения всех
индивидуальных read audits. Появление новых доказанных kernel readers или
изменение pinned sources требует пересмотра соответствующей classification.

Дополнительные decoder boundaries также сверены: LZ4/LZO raw blocks уже
представлены filesystem modules; legacy LZ4/lzop framing перенесён отдельным
компонентом для оставшегося composite initramfs scenario. Linux 842
не представлен decoder проекта. Gzip marker 0x1f9e есть в dispatch table, но
actual gunzip требует 0x1f8b: одно имя в dispatch не доказывает read support.
Проверки XZ включают его BCJ/Delta consumers; отдельные криптопримитивы не
добавляются как архивные форматы без соответствующего binary/text grammar.

## Свидетельства

### Intel HEX: ASCII records и vendor profile

[Сверка Intel HEX](ihex-linux.md): реальный AS102/IQS5xx text reader, исправления
probe/address/EOF/separators, cleanup/cancellation и independent cached streams.
Явный IQS5xx property сохраняет особую settings checksum policy и map assembly;
default checksum validation остаётся строгой. Общий census ниже отделяет этот
формат от binary ihex helpers и включает подтверждённый TAR reader.

### EFI capsules: generic header, Quark и streams

[Сверка EFI capsule](uefi-linux.md): GUID-agnostic/short payload reads, большой
header без буферизации, Quark wrapper, source/region bounds, cancellation и
независимые retained streams. Сохранены исходные FV/LZMA/ME consumers.
Firmware services и общий format census не закрываются этой итерацией.

### PE: headers, bounds и resource streams

[Сверка PE](pe-linux.md): поздние headers, 64-bit file spans, debug iteration,
certificate/symbol bounds, cleanup/cancellation и retained section/resource
streams. Общий COFF consumer, оба engines и backend проверены вместе.
TE и оставшийся census требуют отдельных source evidence.

### ELF: headers, core numbering и streams

[Сверка ELF](elf-linux.md): class/endian header reads, ignored fields, bounds,
extended counts/names, BSS/NULL metadata, independent retained streams и ошибки
Open/Extract. Связанный GPT discovery regression исправлен в LZMA probe.
ELF execution и archive inspection имеют отдельные границы; оставшийся census
не закрывается этой итерацией.

### Base64: современные alphabets и lifecycle

[Сверка Base64](base64-linux.md): URL-safe/IMAP, canonical padded/unpadded tails,
optional callback, error cleanup и cached extraction. Фактический список форматов
получен у обоих engines; оставшиеся filesystem read features сохраняются в scope.

### Zstandard: frames, discovery и decoder errors

[Сверка Zstd](zstd-linux.md): автоматическое открытие leading skippable frames,
размеры и cleanup Open, parse overflow, source HRESULT в Code/Read и
PrepareOperation/final progress cancellation. Проверены оба engines,
native ZIP и backend; composite initramfs и census остаются открытыми.

### XZ: streams, Index и random access

[Сверка XZ](xz-linux.md): длинный padding, пустой EOF stream, независимое
время жизни выданных streams, source HRESULT и Open/Extract cancellation.
Проверены Linux BCJ filters, integrity, оба engines, native ZIP и backend.
Composite initramfs и оставшийся census форматов остаются открытыми.

### Bzip2: end padding, CRC и cancellation

[Сверка bzip2](bzip2-linux.md): unused end bits больше не превращаются в
DataError, учитываются Open и последний Extract callback. CRC и общий
decoder сохранены; проверены levels, selectors, blocks, streams и I/O errors.
Composite initramfs этим не закрывается.

### LZMA: headers, dictionary и ошибки чтения

[Сверка LZMA](lzma-linux.md): arbitrary dictionary fields, 64-bit declared size,
пустой первый member, bounded probe и source HRESULT в общем decoder.
Подключены оба shipped engines и локальный ZIP consumer; LZMA86 сохранён.
Это не завершение composite initramfs или оставшихся compression formats.

### Gzip: probe, metadata и member extraction

[Сверка gzip](gzip-linux.md): XFL, ignored DEFLATE stored bits, длинные
header strings, progress/cancellation и cleanup Open/OpenSeq. Сохранены
исходные decoder, concatenated-member reader и CRC/ISIZE diagnostics 7-Zip.
Standalone gzip не закрывает composite initramfs или остальные decompressors.

### CPIO: raw newc/crc members

[Сверка CPIO](cpio-linux.md): NUL-padded names, optional trailer, смешанные
raw members, hard links с данными в любом положении и сбросом по trailer,
checksum и independent streams. Использован исходный handler 7-Zip.
Composite initramfs с compressed/raw members и общим hard-link state остаётся
открытым; эта итерация не завершает initramfs/decompressor scope.

### MBR primary/EBR и block-layer границы

[Сверка MBR](mbr-linux.md): primary ordering/overlap/CHS, extended types,
data/link offsets, bounded iterative chains и 64-bit/EOD geometry. Direct-device
adapter передаёт известный размер сектора; APM также согласован с EOD clipping.
Вложенные BSD/Solaris/Unixware/Minix readers перенесены на C в отдельный GPL
module; byte-granular device leases сохраняют их 512-byte subdivisions на 4Kn.
Native Linux и focused consumers проверяют эти сценарии. AIX/FAT/EFI
classification callers также сверены, C helper подключён, optional-probe
cancellation исправлена в MBR/GPT. Archive read source scope MBR завершён;
kernel/platform services и отдельный AIX volume format не подменяют этот scope.

### Apple Partition Map: geometry и map reader

[Сверка APM](apm-linux.md): block sizes до 32768, reserved fields, first-entry
count, end signature и безопасные partition spans; оба engine и backend consumer.
Сохранён отдельный legacy installation-image mode 7-Zip. Native Linux и
focused три ОС проверяют read scope; MBR и остальные shared formats ещё открыты.

### GPT: tables, geometry и partition streams

[Сверка и исправления GPT](gpt-linux.md): validated header/table spans и CRC,
usable ranges, out-of-disk entries, четыре размера сектора, primary precedence
и backup recovery с явной archive adaptation относительно Linux `force_gpt`.
Оба engine и вложенный image consumer используют обновлённый исходный 7-Zip
handler; общие container streams переиспользуют готовый extent reader для
независимых позиций. MBR/APM и остальные дополнительные форматы этим не закрыты.

### ISO9660: logical blocks, XAR и extent streams

[Source comparison и исправления](isofs-linux.md): оба engines читают
512/1024/2048-byte geometry, XAR и multi-section children; mapping ограничен
source/volume, streams независимы, повреждения и отмена передаются явно.
Отдельный модуль подключает Linux C Rock Ridge/SUSP: CE/NM/SL/PX/PN/TF,
relocation, inode metadata, RR-vs-Joliet выбор, Linux zisofs decompression,
High Sierra и Linux recording dates.
Joliet/RR fallback и normal/Acorn/raw mapping подключены к Linux C readers;
глубокий обход, очистка дерева и выбор вложенного файла проверены вместе.
Собственные inode каталогов определяют metadata и Level 3 секции, включая root;
родительские имена сохраняются независимо от inode metadata.
Все Linux NLS charset modules подключены к Joliet, включая aliases и
многобайтовые преобразования. Read-path options, legacy-size `cruft`, выбор
descriptor sector и необязательные descriptor dates актуализированы.
RR producer layouts и оставшиеся read-side consumers сверены с pinned source;
исправлены directory-name CE context и malformed TF fractions. Runtime
проверки и отличия archive projection перечислены в ISO audit.
[UDF allocation/EOF](udf-linux.md): short/long explicit и implicit sparse,
логический EOF, bounded directory reads и metadata bootstrap проверены вместе
для обоих engines. [Итоговая source-сверка UDF read scope](udf-linux.md#итоговая-сверка-read-side-source-scope--2026-10-20)
отделяет закрытые disk readers от kernel/platform services и runtime границ.
Остальные строки общей задачи остаются открытыми. Динамический UDF-модуль
подключает исходные Linux C allocation
continuations, включая short/long chains, directory data и межраздельные адреса;
indirect ICB/strategy 4096 выбирают актуальную запись inode через Linux C reader,
сохраняя fallback и read-only metadata backing semantics. Sparable loader/mapper,
packet spans и compound metadata backing подключены в том же GPL-модуле.
VAT15/20 loader и mapper перенесены в модуль и подключены к общим streams;
проверены inline/external таблицы, предыдущие записи и offset semantics.
Metadata/mirror loader и inode-backed mapping также перенесены: fragmented
short AD, chains, primary/mirror fallback, readonly bitmap policy и extent cache.
Общий AD traversal ограничивает циклы без запрета конечных typed-zero records;
EOF и AED tag/read policies сверены с Linux.
Volume discovery, block geometry, резервные anchors/VDS и prevailing descriptors
подключены к исходному Linux C reader; проверены tagged fileset и LVID read revision.
CS0 names/dstrings, default UTF-8, CRC mangling и symlink path components
подключены к исходным Linux C converters. Read-side body `udf_read_inode`,
permissions/owners/links, специальные inode/device EA и исходное преобразование
timestamps также подключены. Linux FID iterator читает каталоги по блокам,
включая fragmented/AED и межраздельные переходы. Разбор partition-map table
и выбор первой подходящей карты PD перенесены из Linux; проверены неизвестные,
повторные и непривязанные карты, а также таблицы с большим числом разделов.
Политика physical-block inode cache и hidden-role consistency перенесена через
исходный `__udf_iget`; USE доступен как metadata-only inode и hidden metadata
backing. Read-only PD allocation-info validation и последовательная загрузка
разделов перенесены через Linux C bodies. Глубокий archive traversal и cycle
guards сверены; C iterator возвращается до спуска к дочернему inode, а inode
parser не удерживается в рекурсивной цепочке. Read-side source scope проверен
с явными archive/API адаптациями; все функции kernel driver не заявляются.

### HFS+/HFSX: fork mapping и metadata reads

[Source comparison и исправления](hfs-linux.md): overflow continuation между
leaves, preallocation/EOF, границы metadata и forks, boot blocks/wrappers,
циклы каталога, независимые streams и ошибки I/O. Добавлены Linux C Unicode
conversion/casefold и hardlink lookup в отдельном GPL-модуле; backing forks,
xattrs и inode metadata проходят ABI/backend. Подключены Linux permission defaults,
проверки catalog/thread records и catalog FinderInfo streams, включая hardlinks.
Linux inode projection также передаёт timestamp rollover и special-device metadata.
Добавлен Linux C reader plain classic HFS: MDB/B-tree, data/resource и catalog
overflow forks, имена, lock/default metadata и type/creator streams.
Дальнейший metadata/read audit остаётся в scope актуализации.

### NTFS: classic data reads и LZNT1

[Сравнение и интегрированная актуализация](ntfs-linux.md): boot/MFT и
атрибуты, sparse/initialized data, физические run границы, независимые streams,
nonresident reparse payload и Linux C LZNT1 в отдельном GPL-модуле приложения.
Динамический модуль также читает WOF LZX/XPRESS, 32/64-битные ADS offsets и
bootstrap MFT attribute lists через оригинальные Linux C readers.
Оставшийся NTFS metadata/read audit и остальные форматы сохраняются в scope.

### SquashFS: metadata, fragments и независимые streams

[Сравнение и исправление существующего чтения](squashfs-linux.md) охватывает
оба engines: корректные границы metadata/data, пустые тома без fragments,
независимость потоков и передачу ошибок. Актуализация также
подключает Linux C LZ4/LZO, compressor options, index/xattr metadata и
inode/link properties. Source scope и границы standalone/archive ABI описаны
в linked audit; runtime evidence не заменяет проверку остальных строк.

### ext2/3/4: sparse, metadata и inline data

[Сравненный scope и исправления](ext-linux.md): classic sparse indirect paths,
ограничения extent/каталогов, fast symlink с EA и передача ошибок потоков в обоих
engines. Динамический движок также подключает Linux C-проверки metadata,
полные inline data/каталоги, META_BG, BIGALLOC, современные descriptors и
fs-verity data verification. Полная актуализация ext4 не завершена: fscrypt/key
service, trust policy и оставшиеся
feature/creator-OS read semantics остаются в общем scope.

### FAT12/16/32: BPB, имена и размер файла

[Сравненные paths и интегрированные изменения](fat-linux.md): сняты лишние
ограничения BPB, восстановление LFN и чтение выделенного хвоста приведены
к проверенному Linux поведению; сохранены проверки читаемой части цепочки.
Оба поставляемых engine используют один адаптированный исходник. Следующая
интеграция подключает Linux C-функции DOS 1.x, NLS и дат в отдельный FAT-модуль
приложения; source scope, проверки и границы standalone описаны в linked audit.

### EROFS: read-only модуль

[Переданный C scope, интеграция и границы проверок](erofs-linux.md).
Подключены mapping/metadata paths, четыре декодера и format-модуль. Проверены
образы официального mkfs, ABI/backend, повреждения, отмена, время жизни потоков
и дополнительные устройства на трёх ОС. Эта итерация не закрывает остальные
файловые системы и дополнительные форматы из общего scope.

### cramfs: sparse end-pointer blocks

[Сравнение всего модуля, исправление и оставшаяся актуализация](cramfs-linux.md).
Нулевые extents читаются обоими поставляемыми engines. Полный модуль ещё не
был завершён в `b3c78fd`. Следующая интегрированная актуализация переносит
Linux C read path в отдельный format-модуль и закрывает расширенные указатели,
prefixed superblock и metadata branches; её фактический scope и проверенные
платформы записаны в linked audit. Это не закрывает остальные строки таблицы.

### Начальный scope

Начальная итерация: определены фактические обработчики поставляемого
`Bundles/Format7zF/Arc_gcc.mak` и локальные модули, закреплён Linux baseline,
проверено место сборки engine. Функциональная проверка перечисленных
обработчиков и EROFS ещё не завершена. Эта запись задаёт scope проверки,
а не заявляет совместимость или полную передачу Linux drivers.

### exFAT: отмена cached FAT traversal

[Сравненный scope и regression](exfat-linux.md#cached-fat-cancellation-2026-10-19).
Пропущенная проверка отмены — адаптация archive callback, а не изменение
дискового алгоритма Linux. Существующие границы TexFAT и omitted scope сохранены.
Проверка этого scenario не означает завершения всей таблицы выше.

Проверки 2026-10-19: инкрементальная сборка модуля и consumers;
`exfat`/`exfat_backend` PASS на macOS arm64/Qt 6.12 (2,07 с), Linux x64
и Windows x64/MSVC/Qt 6.12. Remote receipt:
`20261019T054117Z-8147988f`. Проверены экспортируемый ABI, реальный process
engine и ArchiveBackend на generated images, включая прежние success/error/
TexFAT/Unicode сценарии. Общие/slow suites, физические носители и UI в эту
проверку не входят. Инкрементальные builds и compiler caches сохранены;
одноразовых task fixtures после успешных запусков не осталось. Посторонние
старые Windows runner-tmp fixtures не удалялись: принадлежность не установлена.
