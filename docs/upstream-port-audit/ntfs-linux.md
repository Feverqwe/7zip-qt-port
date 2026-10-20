# NTFS: read paths по Linux NTFS3

Итерация входит в [общий scope форматов](filesystems-linux.md). Исправлено
существующее чтение boot/MFT, атрибутов, sparse и compressed streams.
Полная актуализация NTFS по Linux ещё не завершена.

## Исходники и границы переноса

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`, полный
`CPP/7zip/Archive/NtfsHandler.cpp`. Единственная адаптация находится по тому же
пути под `src/`; [baseline](../upstream-baseline.json) включает её в оба runtime
overlays. Сохранены классы, MFT enumeration, сборка base/extension records,
имена/ADS, recovery deleted records, security metadata, исходные stream/cache
и extraction flow. Полное C++ форматирование учитывается отдельно от
функционального сравнения; macro class boundary сохранена.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
[Reference manifest](../ntfs-linux-baseline.json) закрепляет полный snapshot
`fs/ntfs3`, включая library dependencies. Сравнены полные тела выбранного
classic read scenario и их callers/types: `super.c:ntfs_init_from_boot`,
`record.c:mi_enum_attr`, `fsntfs.c:ntfs_fix_post_read`,
`run.c:run_unpack`, `inode.c:ntfs_read_folio`,
`frecord.c:ni_read_frame`, определения в `ntfs.h`/`ntfs_fs.h` и LZNT decoder.
Это не означает полного переноса всего reference snapshot.

Из `fs/ntfs3/lznt.c` перенесены оригинальные C bodies `decompress_chunk`,
`decompress_lznt`, `parse_pair`, `s_max_off` и размер chunk. Их алгоритмы
сохранены; kernel includes заменены узким portable type/memory adapter.
Compression/match-search state, compressor и allocator LZNT не копируются:
модуль read-only. Нормализация положительного короткого decode с нулевым
хвостом соответствует `ni_read_frame`.

Linux C остаётся в отдельном GPL-2.0-only format-модуле `sevenzip_ntfs`.
Его LGPL 7-Zip closure: archive exports, portable stream buffers/utilities,
progress/properties, CopyCoder, DummyOutStream, MethodProps и Common/Windows
portable helpers. Локальная MethodProps-адаптация сохраняет собственный mixed
baseline. Модуль не содержит unRAR. Он устанавливается вместе с engine и
лицензиями; NTFS registration основного external-codec engine отключена,
чтобы фактическим владельцем формата был модуль. Standalone сохраняет
оригинальный C++ LZNT decoder без GPL C linkage; остальные исправления
обработчика действуют в обоих engines.

## Исправленный сценарий

Boot проверяет NTFS OEM и геометрию; не требует несущественных DOS полей,
boot jump и AA55. Размеры MFT/index используют signed low byte; MFT fixups
всегда применяются к 512-байтовым секторам независимо от logical sector size.
Недействительный primary boot допускает чтение последней 512-байтовой копии,
когда NTFS handler уже выбран. Отдельное обнаружение формата только по backup
signature не добавлялось. Ошибки seek/read и отмена сохраняют HRESULT.

Проверяются минимальные размеры атрибутов, nonresident flag, границы имени и
value/run data, initialized/data/allocated size, порядок активных атрибутов и
границы record/fixups. Sparse packed-size учитывается и при compression unit
0. Физические runs проверяются целиком внутри тома, включая signed LCN delta,
переполнение и недостаточную длину mapping pairs.

Sparse runs читаются как нули независимо от наличия compressed frames;
initialized tail не раскрывает выделенные данные. Сжатие определяется по
attribute flags, а не по оставшемуся unit sparse-атрибута. Linux LZNT1
обрабатывает частичные chunks/frames; ошибка decode не публикует cache tag.
Исходная salvage-ветвь удалённых файлов остаётся на C++ decoder.

Каждое физическое чтение восстанавливает source seek position. Потоки
сохраняют собственные mappings, buffers и source owner после Close, ReOpen
и освобождения архива. Неуспешный seek/read допускает повторение; неожиданный
EOF и неполная extraction не считаются успехом. Повреждение данных и
unsupported method имеют разные operation results.

Nonresident reparse attributes разрешаются через обычный bounded stream после
сборки extension records; archive raw property содержит payload, а не mapping
pairs. Некорректные reparse данные неактивной recovery-записи не мешают чтению
активных файлов. EFS возвращает явный unsupported result, как Linux;
ciphertext не выдаётся за файл.

## WOF и bootstrap MFT attribute lists

Сравнены полные read bodies `attrib.c:attr_wof_frame_info`,
`frecord.c:decompress_lzx_xpress`, `ni_parse_reparse`, WOF-ветви `ni_read_frame`,
`inode.c:ntfs_read_mft`, `frecord.c:ni_enum_attr_ex`/`ni_load_mi`/`ni_find_attr`,
`record.c:mi_read`, `attrlist.c:ntfs_load_attr_list`/`al_enumerate` и
типы/dependencies. Оригинальный 7-Zip не содержит WOF reader; новый C++ код
ограничен stream ownership и подключением этих Linux readers к существующим
CMftRec/CAttr. Соседний upstream не меняется.

Полные `lib/decompress_common.c`, `lib/lzx_decompress.c` и
`lib/xpress_decompress.c` скопированы без изменений. Алгоритмы и helper bodies
соответствующих headers сохранены; kernel includes заменены portable types,
unaligned little-endian services и allocator. Оригинальные `ntfs.h` disk types,
`le_size` и `fsntfs.c:WOF_NAME` переданы выбранными read units по исходным путям.
`al_enumerate` и `decompress_lzx_xpress` сохраняют полные C bodies.

WOF reader поддерживает XPRESS4K/8K/16K и LZX32K, compressed/stored frames,
resident/nonresident ADS, короткий последний frame и initialized tail.
32/64-битная offset table читается оригинальным `attr_wof_frame_info` через
private page cache. Archive adapter представляет оба вида ADS обычным
seekable stream и использует paged/nonresident ветвь C reader; прямой доступ
к kernel resident pointer не нужен. Kernel run lookup делегирован заранее
проверенной карте CAttr, locking заменён владением каждого stream, а page I/O
использует исходный HRESULT-aware callback. Перед сложением offsets добавлены
проверки monotonic/range/overflow, включая 64-битные значения до приведения
к размеру frame. Последний frame ограничивается фактическим размером ADS,
как в `ni_read_frame`.

Поток владеет decoder contexts, входным буфером и cached frame. Source errors
и отмена сохраняются; decoder error не публикует частичный output. При замене
frame cache tag инвалидируется до записи, также в classic LZNT cache: ошибка
другого frame не оставляет старый tag над уже изменённым буфером. Неизвестные
WOF algorithms/providers явно отклоняются, полностью uninitialized данные
читаются нулями. Сырые ADS остаются отдельными архивными streams.

MFT bootstrap повторяет порядок `ntfs_read_mft`/`ni_enum_attr_ex`: загружает
resident/nonresident список обычным CAttr stream, перечисляет его Linux C
reader и расширяет доступную MFT mapping по DATA entries. Extension record
читается только из уже известной карты (для первых трёх records сохранён Linux
bootstrap seed). Проверяются record/sequence/base reference, attribute ID и
VCN; gaps, недоступная ссылка и некорректный list не становятся нулевыми данными.
После bootstrap полный MFT читает исходный 7-Zip stream/parser; обычная сборка
base/extension records сохранена. Kernel inode/VFS cache и запись не копируются.

Эти GPL read features подключены к динамическому модулю. Standalone `7zz`
сохраняет исходный classic scope и явно отказывает при чтении WOF или MFT,
требующего дополнительных bootstrap extents. Linux C не линкуется с unRAR.

Process transport явно задаёт штатный `-sns[-]` из
`ArchiveCommandLine.cpp`/`Extract.cpp`. Выбор обычного файла не добавляет
невыбранные ADS: Windows запрещает прямую запись служебного `WofCompressedData`.
Для явного выбора потоков сохраняются только проверенные ADS entries, а
stdout extraction обычного файла возвращает один декодированный payload.
Отдельный raw ADS читается тем же upstream transport при явном запросе.
Изменён только выбор archive entries, формат и OS stream APIs не дублируются.

## Проверки

Fixture создана официальными `mkntfs`/`ntfscp` ntfs-3g 2022.10.3; источник и
команды находятся [рядом с образом](../../tests/fixtures/ntfs/README.md).
Regressions проверяют реальные bytes/ошибки, не текст реализации:

- обоими engines: resident/Unicode, raw, sparse/head hole, initialized tail,
  compressed data, logical sector 4 KiB, boot fields и backup copy;
- GPL ABI: Linux short-frame zero fill, interleaved/lifetime streams,
  seek/read failures, retry, decode-cache failure, отрицательные/overflow seeks,
  nonresident reparse payload, отмена открытия/извлечения, failure cleanup;
- WOF: официальные wimlib frames всех четырёх codecs, stored/E8/short frames,
  оба ADS storage kinds, initialized tail, source error/retry и cache collision;
  выборочное чтение файла свыше 4 GiB проверяет 64-битную table и переход страницы;
- MFT: физически перемещённые extents, resident/nonresident lists, неверные
  sequence/ID/ссылки/entries и отмена bootstrap;
- ArchiveBackend: list и selected extraction с Unicode/spaces, sparse/short
  compression, ошибочный mapping, readonly catalog и неизменность source image;
- native Linux NTFS3 `6.8.0-146-generic`: девять read scenarios (включая
  nonresident symlink), три ошибочных read paths, invalid initialized size и
  EFS; исходные images не изменяются. Штатный `linux-modules-extra` установлен
  для текущего ядра runner. Backup-boot variant этим старым kernel не открыт;
  fallback проверен по закреплённому source и обоим engines, без заявления
  native acceptance для этой ветви.

Для WOF frames использован официальный libwim 1.14.4, не runtime dependency.
Native NTFS3 дополнительно подтвердил оба MFT-list variants, четыре WOF codecs,
resident ADS каждого codec, initialized tail, uninitialized WOF без ADS и
selective read файла свыше 4 GiB — тринадцать scenarios; mounts read-only,
source images неизменны.

2026-10-19: `ntfs`, `ntfs_stream`, `upstream_baseline`, `upstream_inventory`,
`runtime_package`, `windows_package_verification` прошли на macOS ARM64,
Linux x64 и Windows x64. После финальных локальных исправлений повторены
`ntfs`/`ntfs_stream` на трёх ОС. Сборки используют существующие incremental
outputs и ccache. Полный gate и slow suites не запускались: изменения
ограничены NTFS, format deployment и provenance; затронутые consumers покрыты
focused проверками. macOS app target собран; install smoke подтвердил загрузку
модуля, единственную NTFS registration, short-frame extraction и notices.
Native runtime evidence относится к указанному Linux kernel; эталоном source
comparison остаётся закреплённый commit, не версия runner kernel.

WOF/MFT актуализация от 2026-10-19: NTFS CLI/ABI/backend suites и прямые
consumers `archive_backend`, `source_extract_backend`, `archive_external_security`,
`archive_selection_runtime` прошли на macOS ARM64, Linux x64 и Windows x64.
Первый Windows backend run обнаружил автоматическое включение служебного ADS;
исправленный transport проверен обычным извлечением и отдельным decoded/raw
stdout extraction. Linux focused build после OOM повторён с одним job без
очистки outputs/cache. macOS app target и install smoke подтвердили загрузку
установленного модуля, WOF LZX/XPRESS и fragmented MFT. Одноразовый install
staging удалён. Full/slow gate не требовался: NTFS и прямые transport consumers
покрыты адресно; physical devices и UI appearance не проверялись.

## EA и WSL inode metadata — 2026-10-20

До реализации полностью сравнены `fs/ntfs3/xattr.c` read functions
`unpacked_ea_size`, `find_ea`, `ntfs_read_ea`, `ntfs_get_ea`,
`ntfs_get_wsl_perm`; их callers в `inode.c:ntfs_read_mft`, EA/AttrDef disk
types в `ntfs.h`, `$AttrDef` loader из `super.c`, attribute/run readers и
существующий полный SDK handler с extension merge, stream и property consumers.
Используются те же полные Linux/SDK revisions выше. Пять перечисленных C
function bodies перенесены без изменений под исходным путём `src/fs/ntfs3/xattr.c`;
новый unit добавлен в существующий [Linux baseline](../ntfs-linux-baseline.json).
Сохранены Paragon Software notice и GPL SPDX. C остаётся C11.

Dynamic NTFS module читает unnamed EA_INFO/EA после объединения base/extension
records; resident и nonresident EA используют прежний bounded CAttr stream и
полную карту extents. Нельзя заменить nonresident EA mapping pairs содержимым
атрибута. Максимальный размер берётся из реального `$AttrDef`: сохранены исходные
проверки таблицы и ordered type/max-size selection loop в исходном `super.c` unit.
Полный VFS loader
`ntfs_fill_super` не копируется: открытие системного файла выполняет SDK reader.
EA disk structures, flexible-array offsets и little-endian conversions сохранены;
C11 static-assert messages — только compiler adaptation.

Kernel inode/attribute lookup, allocation, locking, run I/O и dirty-state services
заменены узким `ntfseacompat` adapter. Он предоставляет заранее собранные SDK
attributes и проверенную карту данных; повторного EA parser в C++ нет.
`ntfsea` — C linkage boundary и eager validation adapter; Linux read functions
проверяют объявленную длину, весь EA chain и размеры каждого имени/value.
Повреждённый активный EA останавливает archive Open; dirty-state adapter
сохраняет ошибку, но не меняет исходный том. Linux mount может откладывать такую
диагностику до xattr access или оставить default permissions. Source/seek ошибки
и cancellation проходят через C boundary с исходным HRESULT, частичные items
очищаются; temporary metadata streams и callback не остаются у открытого архива.

Полный корректный набор `$LXUID`/`$LXGID`/`$LXMOD` публикует числовые owner IDs
и POSIX mode. Неполный набор или неверная длина значения не публикуют частичную
тройку, как `ntfs_get_wsl_perm`. Сохранены 16-bit inode mode и optional `$LXDEV`
semantics, в том числе проверка точной длины device value. DOS readonly снимает
write bits после WSL override, как `inode.c`; DOS directory bit, directory и
inode-type projection согласованы с полученным mode. Numeric IDs представляют
диск, без переноса host
user namespace или kernel mount defaults.

Character/block devices, FIFO/socket и symlink без поддерживаемого reparse target
не выдают unnamed DATA как обычный file stream; эти inode остаются metadata-only
с явным unsupported result на чтение. Поддерживаемые reparse links теперь
читаются через link scenario ниже. Device major/minor соответствуют NTFS3 internal dev_t layout.
Named ADS остаются независимыми data streams, без присвоения им типа специального
base inode. Existing NT reparse/security projection, WOF, classic compression,
recovery и public extraction flow сохранены. Standalone `7zz` сохраняет прежний
scope metadata; Linux EA implementation линкуется только с отдельным GPL plugin.

Write-side EA/WSL persistence, ACL mutation, xattr syscall frontends, kernel
permission enforcement и host owner mapping не переносятся в readonly archive
reader. Неиспользуемый `ntfs_list_ea` не копируется: выбранный public scenario —
чтение inode permissions/type/device, а не новый archive xattr-enumeration API.

Regression проверяет resident/nonresident/fragmented EA, EA в extension record,
directory metadata, stale DOS directory bit, readonly override,
incomplete/invalid-width values, zero и
explicit next-size records, 16-bit mode и четыре special inode types. Девять
malformed cases покрывают record/info/attribute spans, `$AttrDef` limit и
неизвестный inode mode. ABI сравнивает properties и exact regular-file bytes,
retained streams, явный отказ выдачи special payload, source E_FAIL/E_ABORT
внутри fragmented EA, cancellation и успешное повторное открытие.
ArchiveBackend извлекает файл из образа с fragmented EA; CLI сохраняет source
bytes и error contracts. Эти tests проверяют поведение, не source text.

Десять focused suites PASS на macOS arm64 (10,34 с), Linux x64 (24,43 с) и
Windows x64/MSVC (116,72 с); receipt `20261020T142812Z-2331707a`. Выбор включает
обе NTFS suites, archive/source-extract/security/selection consumers,
runtime cache/package и source provenance. После добавления directory fixture
обе NTFS suites повторены отдельно. Финальные suites после согласования DOS
directory bit — PASS на macOS/Linux/Windows; remote receipt
`20261020T143927Z-59c4358f`. Incremental builds и ccache сохранены;
full/slow gate, UI appearance и физические устройства не проверялись.
После размещения `$AttrDef` C loader в исходном `super.c` unit обе suites
повторно PASS на macOS/Linux/Windows; receipt `20261020T144233Z-7124830d`.

Native read-only loop reference на Linux NTFS3 `6.8.0-147-generic` подтвердил
19 valid images: UID/GID/mode, incomplete-value fallback, readonly override,
directory и special inode types, resident/nonresident/fragmented/extension EA,
точные обычные file bytes и неизменность images. Для чтения файлов с заданными
WSL owners использован privileged reader; сами permission bits не менялись.
После смены runner kernel потребовался штатный Ubuntu пакет
`linux-modules-extra-6.8.0-147-generic` для текущего ядра; загрузка NTFS3 выполнена
без reboot. Images/mountpoints удалены. Финальный receipt `20261020T143833Z-2f6a013b`.

Device major/minor проверены по закреплённым C bodies и ABI fixtures; native
runner вернул 0:0 вместо stored dev_t. В исходном
[Linux v6.8 read helper](https://github.com/torvalds/linux/blob/v6.8/fs/ntfs3/xattr.c)
есть неверная длина `sizeof("$$LXDEV")`, отсутствующая в pinned reference.
Это согласуется с наблюдаемым старым-kernel результатом; точное наличие
backport fix в Ubuntu binary этим не утверждается. Модуль сохраняет исправленное
чтение текущего эталона, а не воспроизводит наблюдённый нулевой rdev старого ядра.
Malformed EA не передавались старому kernel как safety validator; их проверяют
перенесённые актуальные bounds checks и production regressions.

## Indexed security descriptors — 2026-10-20

Исходный SDK `ParseSecuritySDS` требует линейного порядка security IDs,
зеркальных записей и загрузки всего `$SDS` в пределах прежнего memory limit.
Linux NTFS3 ищет ID через `$SII` и читает только выбранный descriptor.
Dynamic GPL module теперь использует этот Linux read path. Standalone `7zz`
сохраняет прежний SDK security scanner; общий C++ handler остаётся единственным
авторитетным исходником для обоих overlays.

До реализации сравнены полные `fsntfs.c:ntfs_get_security_by_id`,
`ntfs_fix_post_read`, `sid_length`, `is_acl_valid`, `is_sd_valid`,
`ntfs_security_init`; `index.c` lookup/init/read/finder и index-header checks;
`ntfs.h`/`ntfs_fs.h` disk types; callers в inode/xattr, boot geometry и полный
SDK parse/extension/stream/raw-property flow. Использованы полные revisions,
указанные выше. В исходные C units `fsntfs.c` и `index.c` перенесены выбранные
функции вместе с именами индексов, disk structures и finder dependencies.
Paragon notices и GPL SPDX сохранены, C компилируется как C11.

Полные тела `cmp_uint`, `index_buf_check`, `fnd_clear`, `fnd_push`,
`hdr_find_e`, `indx_init`, `indx_get_root`, `indx_find`, `ntfs_fix_post_read`,
`sid_length`, `is_acl_valid`, `is_sd_valid` совпадают с эталоном. В
`index_hdr_check` добавлены нижняя граница первого entry после header и
проверка наличия полного следующего entry header перед чтением его полей;
`indx_read_ra` отклоняет VBN с переполнением byte offset;
`ntfs_get_security_by_id` проверяет полный размер SII view перед чтением
`SECURITY_HDR`. `get_cmp_func` специализирован только для UINT `$SII`.
Остальные collations, directory index consumers, `$SDH` write/dedup lookup,
insert/delete/rebalance и создание security indexes не переносятся этим
readonly scenario.

`ntfssecuritycompat` заменяет kernel allocation, inode/attribute lookup,
locking, run I/O и buffer-head services. Логические streams и полная
проверенная карта extents принадлежат SDK. C++ сохраняет `$SII` attributes
через base/extension merge и передаёт resident root, `$INDEX_ALLOCATION` и
`$SDS` узкому C API. C lookup не сканирует зеркало или весь `$SDS` и не
ограничивает logical stream прежним 512 MiB. Root/index geometry проверяется
в пределах поддерживаемой NTFS3/SDK geometry. При fixup mismatch Linux
восстанавливает sector tails; adapter сохраняет исправления только в частном
буфере, без записи на том.

C boundary вызывает Linux descriptor validator после безопасной проверки
четырёх относительных offsets. Повреждённый descriptor активного inode
останавливает archive Open; Linux xattr frontend лишь предупреждает о таком
descriptor и может вернуть bytes. Это явно более строгий archive integrity
contract, без переноса kernel permission enforcement. Не найденный или
reserved ID не создаёт raw property; повреждённая metadata удалённого inode
не мешает recovery остальных items. Read/seek errors и отмена сохраняют
HRESULT через C callback. Descriptor cache принадлежит открытому архиву;
временные index readers и callbacks освобождаются до завершения Open.
NT4 resident security и key/trust services этим сценарием не изменяются.

Новая focused ABI suite проверяет exact raw descriptor bytes, обычный file
payload и retained streams после Close; Unicode/spaces образ проходит реальное
ArchiveBackend extraction. Fixtures покрывают resident/root keys, allocated
trees, расширенный поиск, маленькие index blocks, fragmented allocation,
extension attributes, переставленный physical `$SDS`, повреждённое зеркало,
отсутствующее зеркало, sparse stream с logical offset 4 GiB, missing/reserved
IDs, fixup recovery и чтение удалённого файла при повреждённом descriptor.
Восемнадцать malformed cases проверяют spans, отсутствующий terminal, key/view
границы, SID/ACL, SDS range/header, VBN/fixup и циклы. Отдельные ABI cases
проверяют E_FAIL/E_ABORT, cancellation внутри index I/O и успешное reopen.
Fixtures детерминированы и не используют пользовательские образы.

Локальные 11 focused suites PASS на macOS arm64/Qt 6.12 (10,72 с):
`ntfs`, `ntfs_stream`, `ntfs_security`, archive/backend extraction/security/
selection consumers, runtime cache/package и source provenance. Проверены
прежние EA/WOF consumers после объединения little-endian services.
Incremental builds и compiler caches сохранены; full/slow gate, физические
устройства и UI appearance не проверялись. Linux/Windows runtime и native
kernel comparison для этой итерации пока не подтверждены.

## Reparse links и dedup read restrictions — 2026-10-20

Сравнены полные Linux `frecord.c:ni_parse_reparse`,
`inode.c:ntfs_readlink_hlp`, `ntfs_translate_junction`, `ntfs_get_link`,
reparse/EA branches `ntfs_read_mft`, `dir.c:ntfs_utf16_to_nls`, NLS conversion,
`file.c:check_read_restriction`, disk types и SDK raw-property/GetStream/Extract
callers. Полные revisions прежние. Изначально handler сохранял raw reparse,
но стандартное `kpidSymLink` не публиковал, а unnamed DATA отдавал независимо
от reparse inode type. Это позволяло выдать посторонний file payload вместо
цели ссылки или служебные bytes deduplicated-файла вместо явного отказа.

В исходные `frecord.c`, `inode.c`, `dir.c` перенесены выбранные полные C bodies,
заголовки reparse/GUID/WOF types и необходимые services. `ntfs_translate_junction`
совпадает с Linux; в `ni_parse_reparse` GNU `typeof` заменён именованным C
struct type для MSVC, без изменения ветвлений. WOF provider/header selection
динамического handler теперь вызывает этот же classifier вместо отдельной
C++ проверки. `ntfs_utf16_to_nls` сохраняет тело с явным portable 16-bit
`fat_wchar_t`; общий перенесённый `fs/nls/nls_base.c` используется повторно.
NLS source/header dependency добавлены в существующий baseline. C остаётся
C11, Paragon/NLS notices и GPL licenses сохранены.

`ntfs_readlink_hlp` сохраняет выбор PrintName, terminal-NUL handling, Unicode
conversion, замену backslash на slash, cloud fallback и junction translation.
Добавлены integer span checks до создания указателя PrintName и вместо
pointer arithmetic за границей объекта; последнее UTF-16 слово читается
unaligned-safe. Явный `u8 *` cast устраняет compiler warning. NLS boundary
копирует UTF-16 в выровненный буфер. Размеры reparse/page output ограничены
NTFS3-compatible пределами; malformed offsets/lengths не читают чужую память.

Archive adapter предоставляет приватные inode/attribute/dentry contexts,
allocation и уже прочитанный bounded SDK reparse stream после extension merge.
Kernel inode cache, mount/NLS selection, VFS get_link delayed-call ownership
не копируются; UTF-8 и ownership возвращённого stream задаёт archive API.
Для junction dentry path — путь записи относительно корня образа, включая
recovery folders. Kernel предположение о том же mounted device сохранено:
drive prefix снимается и строится относительный target. Source writes нет.

После WSL mode override dynamic module классифицирует inode и кэширует target
для каждого item, чтобы junction depth соответствовал его пути. Link не
остаётся directory из-за DOS/MFT flags. Public properties показывают target,
mode и размер UTF-8 target; GetStream и Extract используют owned target bytes,
а не unnamed DATA. Archive size измеряется фактическими UTF-8 bytes;
kernel inode estimate `PrintNameLength / 2` не используется как длина extraction.
Target stream остаётся пригоден после Close. Named ADS
сохраняют обычное содержимое и не наследуют raw reparse/link projection.
Отсутствующий/повреждённый target и неизвестный Microsoft name-surrogate дают
явный unsupported read. Deduplicated unnamed reads возвращают E_NOTIMPL, как
Linux `check_read_restriction`; декодер dedup в Linux отсутствует и не выдумывается.
Callback cancellation и allocation error в target loading передаются через Open.
Standalone сохраняет прежнюю link/classification projection; unnamed-only
выбор reparse attribute исправлен в общем handler.

Fixtures проверяют PrintName отдельно от SubstituteName, Unicode/surrogate
pairs, odd byte offset, terminal NUL, backslashes, junction/root/recovery paths,
resident/nonresident/fragmented/extension payloads, stale directory flags,
readonly mode, cloud fallback с WSL mode, пользовательский tag и named ADS.
Отдельные cases проверяют unknown/malformed links и dedup refusal. ABI сравнивает
properties, exact target bytes и retained streams; CLI проверяет selected
Extract/Test и source immutability. ArchiveBackend распознаёт link и читает
target через публичный selected-source flow. Native filesystem link creation
проверяется на POSIX; Windows case явно пропускается без privileged token.

Whole-image Test исходного creator fixture отдельно выявил отказ `$Boot`:
его mapping pairs содержат zero LCN delta. Результат воспроизведён в обоих
engines без новых reparse mutations; текущий Linux `run_unpack` также явно
отклоняет zero delta. Проверка link payload поэтому выбирает конкретный item;
эта итерация не заявляет принятие всех системных streams creator image.

Одиннадцать focused suites PASS на macOS arm64/Qt 6.12 (14,12 с): обе прежние
NTFS suites и новая security suite, archive/backend extraction/security/selection
consumers, runtime cache/package и source provenance. В `ntfs_stream` прошли
106 cases, в `ntfs_security` — 38; CLI suite проверяет selected Extract/Test и
POSIX link creation. Incremental builds и compiler caches сохранены; full/slow
gate, физические устройства и UI appearance не проверялись. Linux/Windows
runtime и native-kernel reference этой итерации пока не подтверждены:
удалённая синхронизация ожидает явного разрешения пользователя после отказа
automatic approval review. Это ограничение проверки, не доказательство parity.

## Открытый read scope

Выбранные classic/WOF/MFT bootstrap scenarios подключены и проверены.
Остальные metadata/read features NTFS должны пройти оставшийся source audit;
эта итерация не закрывает остальные файловые системы и общие форматы goal.
В частности, default 64 KiB branch Linux WOF classifier пока не подключена
к archive WOF frame reader; известные XPRESS4/8/16 и LZX32 scenarios выше
не доказывают поддержку этой дополнительной ветви.
VFS/page cache, mount services, запись/journal replay и EFS key service не
переносятся в archive handler; Linux сам не расшифровывает EFS при чтении.
