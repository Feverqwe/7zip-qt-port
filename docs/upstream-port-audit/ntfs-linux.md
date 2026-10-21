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
Active DATA/reparse/EA selection перед extension merge уточнён Linux lookup;
последующие разделы фиксируют границы этих изменений отдельно от deleted recovery.

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
reader. Публикация EA values добавлена следующей связанной итерацией ниже.

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

## EA names и values — 2026-10-20

Сравнены полные `xattr.c:ntfs_list_ea`, `ntfs_get_ea`, `ntfs_listxattr`,
`ntfs_getxattr`, их namespace callers, `ntfs_read_ea`/`find_ea` и disk types
с существующим EA/WSL adapter и SDK stream/property/extraction consumers.
Dynamic module теперь публикует произвольные EA values, включая пустые,
resident/nonresident данные и EA из extension MFT records. WSL inode metadata
сохраняет прежний getter и условия полного набора permissions.

Перенесённый C `ntfs_list_ea` использует тот же валидирующий loader; вместо
буфера VFS names callback получает bounded name/value. Это позволяет читать
список один раз, без повторного source I/O на каждый value. Отдельный C++ adapter
копирует values в resident archive streams; это не новый EA parser.
Повторяющиеся точные имена дают первое значение, как Linux `find_ea`/get.
Пустое EA name завершает enumeration согласно kernel list path.
Source/read errors, отмена и allocation failures сохраняют HRESULT; C callback
не выпускает C++ exceptions. Per-value progress проверяет отмену и для resident EA.

Archive representation отделяет EAs от ADS префиксом `$EA.`. Небезопасные и
не-ASCII bytes имени экранируются как `%HH`; коллизии с ADS или другим EA,
включая регистр, получают числовой suffix. Raw ADS сохраняют свои names/bytes.
Это переносимое presentation boundary, а не NTFS disk stream name или Linux
xattr namespace. Owned streams используют прежние listing/properties,
GetStream/Extract и SourceEntry consumers, включая lifetime после Close/Release.
Standalone не получает GPL EA code и сохраняет прежний read scope.

Regressions проверяют binary/empty/Unicode/separator/case-only/duplicate names,
resident/fragmented/extension data, ADS collision, CLI listing и exact bytes,
retained streams и реальное backend SourceEntry extraction. Старый модуль
воспроизвёл отсутствие всех новых непустых values. В затронутой проверке отмены
счётчик callback-вызовов заменён запросом отмены после чтения EA fixture data;
результат операции и повторное успешное открытие проверяются без привязки к
порядку progress checkpoints.

Семь focused suites (`ntfs`, `ntfs_stream`, `ntfs_security`, runtime packaging,
extraction security и два provenance suites) PASS на macOS ARM64 (14,97 с),
Linux x64 (receipt `20261020T165626Z-79cf4177`) и Windows x64/MSVC
(70,09 с; `20261020T165521Z-cc5f9e6d`). Host discovery первоначально завершился
SSH timeout до передачи/сборки. Уже настроенные гости были доступны напрямую;
после проверки identities/host keys те же wrappers/current worktrees использовали
временную ignored direct-SSH configuration. Основной runner config и toolchains
не менялись; compiler caches и incremental build trees сохранены.

Native NTFS3 `6.8.0-147-generic` подтвердил list/get четырёх read-only images
(`20261020T165521Z-76d54250`): exact binary/empty/Unicode/separator/case-only
values, fragmented EA, extension record и первое значение дубликата. Все image
SHA256 остались неизменными; temporary images/mounts и review script удалены.
Обычный ADS не предоставляется native Linux xattr API и проверен через engine/ABI.
Running kernel остаётся отдельным evidence от pinned source baseline.
Full/slow gate, UI и физические устройства не проверялись; общий NTFS read audit
остаётся открытым.

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
четырёх относительных offsets. Первоначальный отказ всего Open при повреждённом
descriptor заменён [изоляцией ошибок security metadata](#security-metadata-failure-isolation--2026-10-21).
Linux xattr frontend лишь предупреждает о таком descriptor и может вернуть
bytes; archive не публикует непроверенный ACL и не переносит kernel permission
enforcement. Не найденный или reserved ID не создаёт raw property.
Отмена и ошибки callback сохраняют HRESULT через C boundary.
Descriptor cache принадлежит открытому архиву;
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
устройства и UI appearance не проверялись. Последующая Linux/Windows и native
kernel проверка приведена в разделе platform acceptance ниже.

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
gate, физические устройства и UI appearance не проверялись. Последующая
cross-platform и native-kernel проверка приведена ниже.

## WOF initialized tail и unsupported formats — 2026-10-20

Полностью сравнены `ni_parse_reparse`, `ni_ext_compress_bits`,
`inode.c:ntfs_read_folio`, `frecord.c:ni_read_folio_cmpr`, `ni_read_frame`,
`decompress_lzx_xpress`, `attrib.c:attr_wof_frame_info`, XPRESS/LZX decoders,
SDK stream construction/Extract и portable WOF adapters. Metadata classifier
назначает неизвестному CompressionFormat 64 KiB, однако read path явно
принимает только 4/8/16/32 KiB и возвращает EOPNOTSUPP для default branch.
Поэтому 64 KiB не является отсутствующей поддержкой чтения относительно Linux:
архивный движок также не должен декодировать эту ветвь как известный XPRESS.

Найдено другое расхождение: eager ADS stream construction отказывала ещё до
чтения initialized data. В Linux полностью неинициализированная folio
обнуляется до вызова compressed reader; отсутствие ADS или ошибка его mapping
не должны блокировать такое чтение. Portable byte stream сохраняет прежнюю
нулевую проекцию после initialized size и теперь открывает ADS при первом
фактическом чтении compressed bytes. Новый узкий ownership adapter сохраняет
source и копию SDK attributes, а mapping выполняет оригинальный
`CMftRec::GetStream`; независимого run parser нет. Неуспешная инициализация
допускает повтор после восстановления source. Streams остаются пригодны после
Close; исключения allocation/services не пересекают C callback boundary.

C classifier возвращает frame bits из исходного Linux `ni_ext_compress_bits`,
без второй таблицы CompressionFormat в C++. Значения 4, high-bit и UINT32_MAX
не смешиваются с признаком повреждённого header. C frame adapter сохраняет
исходный supported-frame switch `ni_read_frame` до ADS lookup; unsupported
read передаётся как E_NOTIMPL. Extract одинаково сообщает UnsupportedMethod
при таком отказе от GetStream или от последующего Read. Source E_FAIL/E_ABORT
сохраняются; S_FALSE остаётся DataError. Полностью uninitialized stream
не требует ни ADS, ни decoder даже при unknown format, как Linux early guard.

Регрессии проверяют нули при missing ADS, неверном physical mapping, encrypted
ADS и unavailable source, затем отказ при чтении initialized bytes. Проверены
unknown formats, source failure/abort и retry, operation results и retained
streams; CLI извлекает целиком нулевой uninitialized file и отклоняет
нечитаемые initialized данные без публикации bytes. Прежний missing-ADS test
проверяет ошибку Read и Extract вместо закрепления eager failure в GetStream.
WOF frames остаются независимым wimlib fixture, новый decoder/producer не нужен.

Сравниваемый zero-tail начинается с полностью неинициализированной 4 KiB
страницы: kernel page, которая содержит initialized bytes повреждённого кадра,
может целиком завершиться ошибкой. Archive API работает с byte streams и не
эмулирует VFS page cache или его гранулярность ошибок. Это не native-kernel
acceptance частично инициализированной страницы.

Одиннадцать focused suites PASS на macOS arm64/Qt 6.12 (12,07 с): NTFS,
security и stream suites, прямые archive/backend extraction/security/selection
consumers, runtime cache/package и provenance. После сужения error mapping
до WOF и удаления неиспользуемой локальной format table повторены пять прямых
NTFS/provenance suites: PASS, 14,25 с; `ntfs_stream` содержит 117 cases,
`ntfs_security` — 38. Сохранены incremental builds и compiler caches.
Последующая Linux/Windows и native-kernel проверка приведена ниже.
Full/slow gate, UI и физические устройства не проверялись.

## Platform acceptance security/reparse/WOF — 2026-10-20

После разрешённой синхронизации текущих исходников одиннадцать focused suites
PASS на Linux x64/Qt 6.12 (42,00 с), receipt `20261020T160746Z-8dd33871`.
Windows x64/MSVC/Qt 6.12 выявил C compiler adaptation: unary `!offsetof` в
static assertion не принимается как constant expression. Эквивалентное
сравнение offset с нулём сохраняет disk layout assertion. WOF/security
compatibility headers также не переопределяют уже предоставленный UCRT `min`.
Алгоритмы и структуры не изменялись. После исправлений все одиннадцать suites
PASS на Windows (78,74 с), receipt `20261020T160908Z-e41153de`.

Финальные C adapters проверены пятью прямыми NTFS/provenance suites на macOS
arm64 (17,34 с) и тремя NTFS suites на Linux (35,85 с), receipt
`20261020T161114Z-215e74a0`. Windows compiler warnings C4005 и ошибка C2057
устранены. Ранее выбранные archive/backend/runtime consumers не менялись.
Windows filesystem link-creation case явно исключён без privileged token;
ABI/CLI target bytes, properties и обычный ArchiveBackend flow проверены.

Native Linux NTFS3 `6.8.0-147-generic` подтвердил 32 generated images через
read-only loop mounts, receipt `20261020T161051Z-063dba33`: 11 security cases
сравнивают exact `system.ntfs_security` bytes, 17 reparse cases — readlink
targets или обычный payload для named-only reparse, четыре WOF cases — нулевые
полностью неинициализированные страницы. Покрыты SII root/trees/fragmented/
extension/small blocks/large search, переставленные SDS records, stale/missing
mirror и sparse SDS offset 4 GiB; Unicode, odd offset, cloud+WSL, resident/
fragmented/extension links и root junction; missing ADS и unknown WOF formats.
Archive recovery-folder junction не имеет native directory entry и проверен
только ABI. Повреждённые SID/index cycles не передавались старому kernel.

Все образы остались неизменными; mounts, images и временный review script
удалены. Native kernel версия отличается от pinned source revision, поэтому
эта проверка дополняет source comparison, а не заменяет его. Incremental build
trees и compiler caches сохранены; full/slow, UI appearance и физические
устройства этим набором не проверялись. Остальной read scope остаётся открытым.

## Standard information и активные MFT records — 2026-10-21

До изменений и после них сравнены полные выбранные тела Linux
`inode.c:ntfs_read_mft`, `ntfs.h:ATTR_STD_INFO/ATTR_STD_INFO5`, enumeration
через `ni_enum_attr_ex` и security consumer `xattr.c:ntfs_getxattr`, а также
SDK `CSiAttr::Parse`, `CAttr::ParseSi`, `CMftRec::ParseRec/MoveAttrsFrom`,
constructors, MFT bootstrap/open и consumers timestamps/attributes/security.
Все 30 source hashes NTFS baseline повторно проверены. Repository и полные
ревизии обоих эталонов приведены выше; новых borrowed C bodies нет.

Shared SDK reader теперь требует resident standard information длиной не менее
48 bytes; security ID читается только из полного 72-byte варианта. Промежуточные
56/64-byte layouts сохраняют basic metadata без ложного security descriptor.
Первый корректный атрибут задаёт metadata; каждый последующий проверяется,
но не перезаписывает время, readonly и security ID. Existing extension merge
переносит standard information при отсутствии её в base record; обязательное
наличие проверяется после merge. Активная MFT record должна объявлять полный
фиксированный размер записи. Проверки общих attribute bounds остаются в SDK.

Это узкое исправление существующего SDK parser по disk contracts Linux,
не передача целого VFS `ntfs_read_mft` и не новая реализация ATTR_LIST.
Оригинальные SDK MFT enumeration, sequence/base-reference merge и streams
сохранены. Recovery удалённых записей — archive adaptation: повреждённая
standard information не скрывает активные файлы и не запрещает восстановление
доступного payload удалённого файла. Linux возвращает ESTALE для такой записи.
Оба поставляемых engines используют один исправленный SDK исходник;
GPL closure и ранее записанные границы standalone не расширены.

Регрессии проверяют точный payload в обоих engines, точные FILETIME и readonly,
raw security descriptor, extension ownership, duplicate attributes, retained
streams после Close, отказ Open без опубликованных items и повторное открытие
после ошибки. Legacy/partial/full layouts и восстановление deleted payload
проверяются через публичный ABI; отсутствующие/короткие/nonresident metadata,
повреждённый второй атрибут и неверный MFT total — через ABI и оба CLI.

Финальные `ntfs`, `ntfs_stream`, `ntfs_security` PASS на macOS arm64/Qt 6.12
(15,29 с) и Linux x64/Qt 6.12 (40,36 с), receipt
`20261021T014050Z-4729815f`. Windows в том же запуске прошёл NTFS/security,
но stream suite получил codec discovery timeout в sparse backend case;
повторная проверка записана ниже. Ранее все три suites прошли обе remote
платформы до добавления проверки MFT total, receipt
`20261021T013833Z-fd7d5068`.
Повторный `ntfs_stream` PASS на Windows x64/MSVC/Qt 6.12 без изменения
кода или timeout, receipt `20261021T014359Z-04f9e516`; причина единичного
codec discovery timeout не установлена.

Native ntfs3 `6.8.0-147-generic` подтвердил шесть корректных и пять повреждённых
образов, receipt `20261021T014356Z-b9f8948c`: exact payload, nanosecond mtime,
readonly mode и `system.ntfs_security`; все пять повреждений отклонены EINVAL.
Образы остались неизменными, mounts и fixtures удалены. Эта версия kernel
дополняет сравнение pinned source, а не заменяет его. Incremental builds и
compiler caches сохранены. Full/slow suites, UI и физические устройства не
проверялись; оставшийся NTFS read scope остаётся открытым.

## Атрибуты файла из DATA — 2026-10-21

Сравнены полные selected read bodies `inode.c:ntfs_read_mft`, его enumeration,
standard-information owner, последующие EA/reparse consumers и
`xattr.c:ntfs_getxattr`; SDK `ParseDataNames`, base/extension merge и публикация
`kpidAttrib`. Все 30 baseline hashes совпадают с указанным выше Linux snapshot.

Оригинальные три set/clear ветви ATTR_DATA перенесены в C `inode.c`, predicates
из `ntfs.h` — в GPL boundary `ntfslinkcompat.h`, selected little-endian flags —
в `ntfs.h`. Узкий вызов передаёт standard attributes и DATA flags; private
service `ATTRIB` дополнен flags, без изменения raw disk layout/parser.
Source language и исходные bit predicates/ветвления сохранены. Существующий
SDK owner выбирает primary атрибуты после extension merge: безымянный DATA обычного
файла, а также nonresident `$BadClus:$Bad` и `$Secure:$SDS`. Resident DATA
`$BadClus`, остальные named ADS и DATA каталогов пропускаются, как в Linux.
Три признака sparse/compressed/encrypted уточняются независимо от устаревших
значений STANDARD_INFORMATION; остальные file attributes сохраняются.

Нормализация действует на активные base records dynamic GPL-модуля; deleted
records сохраняют forensic metadata. Standalone SDK не линкует GPL bodies и
сохраняет прежнюю проекцию этих трёх признаков. EFS decrypt/key service не
добавлялся. Archive enumeration/recovery и Windows-compatible directory bit
остаются адаптациями archive API, а не переносом VFS inode lifecycle.

Девять ABI fixtures проверяют фактически опубликованный `kpidAttrib`, обычные,
resident, sparse/compressed/encrypted DATA, named ADS, directory и оба системных
исключения. Доступное содержимое проверяется целиком, включая retained stream
после Close; существующие suites продолжают проверять unsupported EFS reads.

Финальные `ntfs`, `ntfs_stream`, `ntfs_security` PASS на macOS arm64/Qt 6.12
(15,64 с), Linux x64/Qt 6.12 и Windows x64/MSVC/Qt 6.12, receipt
`20261021T025201Z-ab4e2f10`. Native ntfs3 `6.8.0-147-generic` подтвердил
все девять образов через read-only loop mounts, receipt
`20261021T025132Z-4918e7da`: exact file flags и payload; для directory/system
entries сравнивались три выбранных признака, поскольку Linux directory bit
отличается от Windows archive ABI. Образы остались неизменными, mounts,
fixtures и одноразовый helper удалены; удаление синхронизировано на оба runners.
Native kernel дополняет сравнение pinned source. Incremental builds и compiler
caches сохранены; full/slow suites, UI и физические устройства не проверялись.

## Primary DATA и собственные metadata ADS — 2026-10-21

Последующий source review сопоставил целиком `inode.c:ntfs_init_ads_node`,
`ntfs_iget5_flags`, `ntfs_read_mft` и enumeration `ni_enum_attr_ex`, выделение
inode в `super.c:ntfs_alloc_inode`, `xattr.c:ntfs_getxattr`, SDK sorting,
`NextAttrIndex`, extension merge и `CHandler::GetProperty/GetStream`.
Linux пропускает non-primary attribute-list segments до ATTR_DATA metadata;
их flags не уточняют признаки файла. Dynamic SDK boundary теперь также
пропускает nonresident continuation с ненулевым LowVcn. Stream mapping по-прежнему
собирает все segments и не зависит от metadata filter.

При открытии ADS Linux задаёт три признака по собственному primary DATA.
Dynamic archive `kpidAttrib` для активного ADS вызывает уже перенесённые
оригинальные C ветви с flags соответствующего stream. Остальные Windows file
attributes наследуются от base record согласно archive API; это явно отличается
от lifecycle отдельного kernel ADS inode, который создаётся с нулевым std_fa.
Read payload, размеры, EA/reparse/security properties и standalone SDK scope
не меняются. Deleted recovery сохраняет прежнюю metadata projection.

Пять новых ABI cases проверяют fragmented sparse/ordinary DATA с противоположными
continuation flags, sparse/resident/encrypted ADS, независимость основного файла
и exact payload/retained streams. Encrypted ADS не возвращает ciphertext как
успешно прочитанный файл: проверяется unsupported operation result.

Focused `ntfs`, `ntfs_stream`, `ntfs_security` PASS на macOS arm64/Qt 6.12
(19,65 с), Linux x64/Qt 6.12 и Windows x64/MSVC/Qt 6.12, receipt
`20261021T025648Z-c52e0fe9`. Финальный sync также удалил одноразовый helper
из runner snapshots.

Native ntfs3 `6.8.0-147-generic` подтвердил оба fragmented-образа: exact
`system.ntfs_attrib` и полное содержимое, receipt
`20261021T025620Z-959cba87`. Образы остались неизменными; loop mounts, fixtures
и helper удалены. ADS native acceptance на этом старом kernel не заявляется;
новый ADS read path сопоставлен с pinned Linux source. Все 30 baseline hashes
проверены, три C ветви ADS совпадают с уже перенесённым projection после
замены владельца поля. Incremental builds и caches сохранены; full/slow suites,
UI и физические устройства не проверялись.

## Allocation bitmap и свободное место — 2026-10-21

Сравнены полные selected bodies `super.c:ntfs_statfs/ntfs_fill_super`,
`bitmap.c:wnd_init/wnd_rescan/ntfs_bitmap_weight_le`, `ntfs_fs.h` bitmap helpers,
boot cluster geometry, `inode.c` выбор run owner для `$Bitmap`, `run.c` lookup,
а также SDK `CAttr`, `DataParseExtents`, `CMftRec::GetStream`, archive properties
и lifecycle Clear/Open/Close. Upstream SDK не публикует FreeSpace.

Оригинальный полный C body `ntfs_bitmap_weight_le` скопирован в `bitmap.c`,
`ntfs3_bitmap_size` сохранён в `ntfs_fs.h`. Portable service использует фиксированное
64-bit слово, включая LLP64, исходную little-endian обработку последнего слова
и оригинальные `__sw_hweight32/64` из общего `lib/hweight.c`. Новый boundary
передаёт aligned buffer и число значимых bits; padding не считается allocation.
В существующий NTFS baseline добавлен hash hweight dependency, все 31 hashes
сверены с закреплённым выше Linux snapshot. C body bitmap weight после переноса
совпадает с оригиналом; таблица archive properties переформатирована отдельно
от функциональных изменений.

Dynamic GPL module публикует `kpidFreeSpace` в bytes из `$Bitmap` и boot cluster
count, как read-only `ntfs_statfs`: delayed allocation на неизменяемом archive
отсутствует. Existing SDK stream читает bitmap небольшими aligned windows,
включая fragmented mappings; успешный результат принадлежит текущему Open.
Длина проверяется по quad-aligned Linux bitmap size. Неверный run, отсутствующие
данные, недостаточный initialized range или hole не объявляются свободными
кластерами. I/O errors и E_ABORT сохраняются, неудачная попытка допускает retry.
Close очищает результат. Standalone не линкует GPL helpers и этого свойства
не получает.

VFS bitmap block cache, readahead, allocator extent trees, MFT zone management,
write/journal/replay и delayed allocation не копируются. Метаданные читаются
по запросу свойства: в отличие от mount path Linux, повреждённая bitmap не
блокирует archive recovery обычных файлов. Resident bitmap также доступна через
существующий archive stream; её native mount acceptance не заявляется.

Десять ABI layouts покрывают ordinary/fragmented/resident bitmap, последний
частичный word, padding, последний допустимый cluster, полный том и четыре
повреждённых варианта. Проверяются exact FreeSpace, payload и retained streams
после Close; отдельная проверка — E_FAIL/E_ABORT и retry. CLI сравнивает
публикуемое число на трёх layouts.

Финальные `ntfs`, `ntfs_stream`, `ntfs_security` PASS на macOS arm64/Qt 6.12
(17,74 с), Linux x64/Qt 6.12 и Windows x64/MSVC/Qt 6.12, receipt
`20261021T030703Z-1372708c`. Первый Windows build выявил конфликт нового имени
GetFreeSpace с Win32 macro; helper переименован, затем проверки повторены.
Native ntfs3 `6.8.0-147-generic` подтвердил пять layouts через read-only loop
statvfs: ordinary/padding/last/full/fragmented, exact free/available bytes и
полный payload, receipt `20261021T030459Z-203843e4`. Образы остались неизменными,
mounts, fixtures и helper удалены; удаление синхронизировано на оба runners.
Native kernel дополняет pinned source comparison. Incremental builds/caches
сохранены; full/slow suites, UI и физические устройства не проверялись.

## Volume state и read-only policy — 2026-10-21

До изменений сопоставлены полные `$Volume` ветви
`super.c:ntfs_fill_super`, `fsntfs.c:ntfs_loadlog_and_replay`, read-only
границы `fslog.c:log_replay`, `ntfs.h:VOLUME_INFO`/`is_attr_ext` и SDK
`CVolumeInfo`, MFT/attribute parsing, archive properties и GUI/CLI consumers.
Источник — те же pinned Linux/SDK commits из начала аудита; все 31 source/license
hash повторно сверены. SDK читал лишь version bytes и игнорировал dirty flags.

Оригинальные C layout/constants `VOLUME_INFO`, minimum resident size и
`is_attr_ext` подключены к `src/fs/ntfs3/super.c` через узкую projection function.
Сохранены resident/extended-attribute checks, version/flags reads и dirty branch;
kernel inode lookup/logging/state заменены выбранным SDK attribute и owned
result. Существующий compat `ATTRIB` дополнен полем flags. Все GPL bodies/types
остаются внутри изолированного NTFS format module; standalone SDK не получает
новую runtime dependency. C++ адаптация сохраняет original MFT parser и выбирает
первый безымянный volume-info attribute активной base record `$Volume`;
поздний duplicate или одноимённый named attribute не подменяет состояние.

Dirty state публикуется как read-only limitation с конкретным EN/RU пояснением,
а не ошибкой каждого файла. Если состояние отсутствует или непригодно, оно явно
остаётся неизвестным: reader не выдаёт его за clean. В отличие от kernel mount
acceptance, отсутствие пригодного volume-info metadata не становится отказом
от всех независимо читаемых файлов. Это осознанный archive read-only subset
по политике AGENTS.md. Сохраняются attribute/MFT/source bounds, file mapping,
path checks и реальные per-file errors; согласие не исправляет metadata.

Общий GUI до публикации папки спрашивает согласие с Cancel по умолчанию,
сохраняет заметную отметку и область согласия одного открытия. После Close/Open
состояние определяется заново. CLI/ABI сообщают причины в metadata; корректное
копирование не превращается в фиктивный warning exit code. Стандартная read-only
capability подключает модуль к уже существующим UI consumers. Journal replay,
repair и запись на source не добавлены. Чтение dirty flag не является проверкой
`$LogFile` или доказательством согласованности clean тома.

Первоначальная итерация читала состояние только из base record `$Volume`;
extension/attribute-list metadata оставалась неизвестной с явным выбором limited
read. Следующая сверка ниже подключает этот lookup после загрузки MFT. Полная
source closure NTFS и остальных metadata/read features этим не заявляется.

Regressions проверяют dirty/clean/resize, absent/short/named/compressed metadata,
приоритет первого unnamed attribute, сброс состояния, точные stream bytes и
retained streams после Close. CLI и ArchiveBackend consumers проверяют доступное
чтение без изменения source. GUI EN/RU — accept/cancel, copying, persistent
indicator, refresh и повторный запрос при новом открытии. Детерминированные
сжатые fixture records повторно используются между ABI случаями, без удаления
проверок и без изменения timeout.

Native Linux `6.8.0-147-generic` прочитал clean/dirty/resize synthetic images
через `ntfs3`/`mount -o loop,ro`: exact alpha/Unicode bytes, ST_RDONLY и неизменные
SHA-256 подтверждены. Receipt `20261021T041530Z-1b801c1e`; mounts/images/helper
удалены. Это evidence более старого running kernel 6.8, отдельно от pinned source.

Focused `ntfs`, `ntfs_stream`, `ntfs_security`, `filesystem_read_ui`, `language`
прошли на macOS arm64/Cocoa, Linux x64/offscreen и Windows x64/MSVC/offscreen.
После уточнения диагностического текста affected CLI/ABI/UI/language проверки
повторно прошли на macOS (39,67 с) и Windows; shared EA/security behavior
проверен исходным полным выбранным набором. Linux final receipt
`20261021T041650Z-ffb3ffef` (52,22 с), Windows initial five-suite receipt
`20261021T041534Z-9efb198e` (86,31 с), final affected receipt
`20261021T041825Z-870ca180`. Локальное приложение собрано с конечным текстом.
Cocoa EN/RU JSON dirty/unknown предупреждений просмотрен: 420×201 / 420×249,
причины и кнопки видимы без clipping. Одноразовый native helper удалён с обоих
гостей синхронизацией. Full/slow suites, physical devices, native Windows/Linux
appearance и installed-package acceptance не заявляются; incremental build
trees и compiler caches сохранены.

## `$Volume` attribute-list lookup и label — 2026-10-21

Сопоставлены полные read-side bodies/callers `attrlist.c:ntfs_load_attr_list`,
`al_enumerate`/`al_find_ex`, `frecord.c:ni_find_attr`/`ni_load_mi`,
`record.c:mi_find_attr`/`mi_read`, bootstrap в `inode.c`/`super.c` и
`fsntfs.c:ntfs_read_run_nb_ra`. В SDK проверены MFT bootstrap/enumeration,
attribute parsing, extension merge, metadata properties и extraction consumers.
Используются те же pinned commits; borrowed scope учтён в существующем manifest.

В `src/fs/ntfs3/attrlist.c` передано неизменённое тело `al_find_ex`.
Оригинальные `upcase_unicode_char`/`ntfs_cmp_names` из `upcase.c` подключены как
C dependency с сохранением notices и полных bodies; `le_name` взят из `ntfs.h`.
Kernel cache/inode services заменены уже прочитанным bounded list и owned SDK
MFT records. Consumer ищет только unnamed attributes; полноценная загрузка
`$UpCase`, named namespace lookup, `ntfs_cmp_names_cpu` и VFS name hashing этой
итерацией не добавлены. Нулевой name length использует исходный comparator без
таблицы upcase; не реализован второй decoder/алгоритм сравнения.

SDK собирает volume label/info attributes по их исходным records. После полной
загрузки MFT и до extension merge consumer следует исходному разделению Linux:
при наличии списка выбирает entry через C lookup, затем проверяет record index,
sequence, active FILE/base ownership и instance ID. Ошибка или отсутствие entry
не подменяются stale clean metadata из base record. Без списка выбирается первый
unnamed attribute primary record. Label имеет собственный независимый lookup;
читаемый Unicode label сохраняется даже при неизвестном состоянии тома.

Resident и обычный nonresident attribute list читаются через существующий SDK
attribute stream с прежними source/cluster/mapping bounds. Несогласованная
структура, недоступное чтение списка или неправильная ссылка оставляют состояние
неизвестным и сохраняют явный limited read для доступных файлов. E_ABORT и
callback cancellation завершают Open без опубликованных items. Mapping/file data,
path validation и реальные per-file errors не ослаблены; replay/source writes
не добавлены. Standalone SDK metadata branch не меняется.

### Native bootstrap boundary

Linux загружает `$Volume` до `$LogFile` и полного `$MFT`. Пока run mapping ещё
не задан, `ntfs_read_run_nb_ra` допускает только bootstrap records 0–3. Поэтому
extension metadata в record 16 не проходит kernel mount даже при пригодном
общем `ni_find_attr`/list algorithm; эта граница подтверждена и pinned source,
и `6.8.0-147-generic` diagnostic `ni_load_mi_ex`/`Failed to load $Volume`.
Archive consumer применяет original lookup после уже существующей полной
загрузки MFT. Это явная archive adaptation, более широкая, чем mount acceptance,
а не заявление о поддержке такого bootstrap layout Linux kernel.

Native controls clean и resident list со ссылками на base record успешно
смонтированы read-only: exact alpha/Unicode data, ST_RDONLY и unchanged SHA-256.
Четыре extension-record layouts (clean/dirty/nonresident-list/stale-base override)
ожидаемо отвергнуты на bootstrap; их успешное чтение kernel не заявляется.
Receipt `20261021T043524Z-e42013dd`; диагностический отказ сохранён отдельно в
`20261021T043411Z-3902fda0`. Все mounts/images/одноразовый helper удалены.

Regressions проверяют одиннадцать list layouts: resident/nonresident, base и
extension records, Unicode label, dirty/clean, stale unlisted base metadata,
неверные ID/sequence/owner, отсутствие записи, malformed list и out-of-source
run. CLI/ABI проверяют metadata и exact file bytes; retained streams работают
после Close. Сбой metadata I/O сохраняет обычное чтение и допускает успешный
reopen; E_ABORT и callback cancellation не превращаются в unknown-success.
ArchiveBackend и GUI EN/RU проверяют nonresident-list opening, consent, copying,
refresh, повторное согласие и неизменность source. Это behavioral coverage,
не source-text assertions или проверка внутренних имён.

Focused `ntfs`, `ntfs_stream`, `ntfs_security`, `filesystem_read_ui` прошли на
macOS arm64/Cocoa, Linux x64/offscreen и Windows x64/MSVC/offscreen. Локальный
набор — 18,89 с; дополненные I/O/cancellation cases повторно проверены ABI suite
(17,55 с). Windows receipt `20261021T043304Z-1f040639` (89,90 с), Linux receipt
`20261021T043644Z-1eb1d1c8`. Приложение собрано локально. Переданные C lookup/
comparator bodies повторно сопоставлены с оригиналом без functional differences,
все 31 baseline source/license hashes проверены. Удаление native helper
синхронизировано обоим гостям, Windows cleanup receipt
`20261021T043648Z-5fc34df9` не выдаётся за отдельный test run. Incremental trees
и compiler caches сохранены. Physical devices, installed package, full/slow и
native Windows/Linux appearance не проверялись; общий read audit остаётся открыт.

## Reparse attribute-list authority и частичное чтение

Источник — закреплённые выше Linux и SDK revisions. Сопоставлены полные
`fs/ntfs3/inode.c:ntfs_read_mft`, `frecord.c:ni_parse_reparse`, attribute-list/
record lookup, continuation-run и raw metadata I/O services с SDK ParseRec,
MoveAttrsFrom, GetStream, DataParseExtents и потребителем ArchiveExtractCallback.
Новых заимствованных C bodies нет; прежние selector/classifier и SDK stream/
extent implementations остаются авторитетными. GPL scope ограничен dynamic
format module; standalone SDK branch сохранён.

Typed metadata lookup теперь обслуживает и Volume, и reparse до объединения
extension records. Список определяет выбранные атрибуты: проверяются record,
sequence, owner, instance и VCN; stale unlisted base attribute не подменяет
отсутствующий выбранный. Resident/nonresident lists и полные continuation runs
подключены к существующим SDK потокам. Metadata читает allocated bytes без
DATA encryption/compression или initialized-tail transforms, как Linux metadata
run I/O; исходные flags не меняются. Sparse metadata не синтезируется в нули.
Ограниченный prefix probe устанавливает только тип, а не непроверенную цель.

По политике Read-only filesystem access неизвестный reparse type не запрещает
полезное чтение: при наличии обычного DATA он доступен после явного согласия,
с постоянным индикатором ограничений и свойством Method `NTFS DATA`. Это не
проверенная ссылка или декодированное представление неизвестного формата.
Если header устанавливает известный тип, но payload недоступен, остаётся
per-file error без выдуманного размера/target; обычные файлы и независимые ADS
доступны. Raw DATA и ADS не наследуют reparse/directory role для извлечения:
иначе исходный extractor ошибочно интерпретировал бы их байты как link payload.
Повреждённый Volume-only list не создаёт ложное предупреждение о reparse.
Cancellation прерывает Open, retry очищает прежнее состояние; source остаётся
read-only, bounds/path validation сохранены.

Тринадцать fixtures проверяют selected/stale attrs, resident/nonresident list,
split runs, WOF, metadata flags/valid-size, неверные references и runs,
oversized/missing/sparse tails. CLI/ABI проверяют exact bytes, metadata,
per-file errors, retained streams, I/O failure/reopen и cancellation. Backend
и GUI EN/RU действительно копируют доступный DATA, проверяют отказ/согласие,
повторное открытие, видимый режим и неизменность образа. Native Cocoa warning
JSON проверен на полноту текста EN/RU; guest UI runs — offscreen.

Focused `ntfs`, `ntfs_stream`, `ntfs_security`, `filesystem_read_ui`, `language`
прошли на macOS arm64 (50,84 с), Linux x64 и Windows x64/MSVC;
guest receipt `20261021T052227Z-ba756052`. После уточнения sparse-prefix probe
затронутые `ntfs`/`ntfs_stream` повторно прошли на всех трёх ОС (macOS 19,64 с,
guest receipt `20261021T052708Z-3d272027`). Приложение собрано локально;
incremental trees и compiler caches сохранены.

Native Linux 6.8.0-147 readonly controls — receipt
`20261021T051929Z-f4dad67d`: восемь layouts, exact обычный файл, ST_RDONLY и
неизменный SHA образа. Selected/nonresident/metadata-flags возвращают точную
цель; unlisted и bad-id дают обычный DATA, WOF — decoded bytes. Split continuation
reparse не читается kernel: ntfs_read_mft пропускает continuation entries для
не-MFT, а readlink использует primary mapping. Archive читает полный mapping
через SDK: это явная более широкая archive adaptation, не native parity claim.
Дополнительный sparse-tail control подтверждает известную ссылку с readlink
ошибкой EINVAL при сохранном обычном файле и unchanged image
(`20261021T053237Z-02467241`). Временные mounts/images/helpers удалены.

Source integration выбранного сценария и runtime evidence разделены. Остальные
NTFS read features и все форматы goal этой итерацией не закрываются; physical
devices, installed package, full/slow и native Windows/Linux appearance не
проверялись.

## EA list authority и read-only доступ при ошибках EA

Закреплённые Linux/SDK repositories и revisions приведены выше. Сравнены полные
`xattr.c:ntfs_read_ea`, `ntfs_get_ea`, `ntfs_get_wsl_perm`, callers в
`inode.c:ntfs_read_mft`, attribute-list/record lookup и metadata run I/O,
а также SDK parser/extension merge, EA stream visitor и consumers свойств/копирования.
`find_ea`, `ntfs_read_ea`, `ntfs_get_wsl_perm` повторно сопоставлены с оригиналом:
тела не изменены. Все 31 baseline source/license hashes совпадают. Новых
заимствованных C units нет; прежние GPL/standalone boundaries сохранены.

Active EA_INFO/EA выбираются по исходному list selector до merge extension records.
Общий typed metadata adapter проверяет reference/sequence/owner/instance и VCN;
полные EA continuation runs подключаются к SDK extent reader. Unlisted stale
base/extension EA не перезаписывают выбранные values. Без списка учитываются
атрибуты base record. EA metadata использует raw allocated-byte transport,
включая чтение без DATA encrypted/initialized-tail transforms. Это соответствует
`ntfs_read_ea`/`ntfs_read_run_nb`; codecs DATA остаются в своём reader.

Предыдущее archive-wide отклонение malformed EA, неверного WSL mode или
недоступного EA I/O заменено явным limited opening. В Linux `ntfs_get_wsl_perm`
не требует успеха всех EA reads для обычного DATA inode. Archive предупреждает
об отсутствующих EA/WSL ownership/type metadata, сохраняет пользовательский выбор
одного открытия и видимый режим. Читаемый DATA неизвестного WSL типа обозначается
в свойствах как `NTFS DATA`; другие файлы и ADS остаются доступны. При успешном
EA parsing и неизвестном WSL mode raw EA values сохраняются, непроверенная WSL
проекция не публикуется. Короткий/отсутствующий EA_INFO не включает WSL reader,
как исходный inode path. Проверенный special inode по-прежнему не выдаётся за DATA.

EA metadata и synthesized streams публикуются после полной проверки, поэтому
ошибка второго чтения не оставляет частично добавленных EA streams. Недоступный
`$AttrDef` limit ограничивает EA, сохраняя читаемые файлы: это archive recovery
adaptation, не утверждение об успешном kernel mount с повреждённым `$AttrDef`.
Source read failure превращается в конкретное ограничение; E_ABORT, allocation
failure и ошибки open callback сохраняют исходный failure. Повторное открытие
очищает ограничение и восстанавливает прочитанные metadata. Source writes,
journal replay и обход bounds/path checks отсутствуют.

Девять новых list layouts покрывают selected/stale EA_INFO/EA, resident/nonresident
list, split continuation, metadata flags/valid-size, unlisted attributes и
недоступные references/runs. Девять прежних malformed-EA fixtures теперь
проверяют honest limited opening и exact обычные payloads вместо blanket refusal.
ABI дополнительно проверяет отсутствие ложных UID/mode, raw DATA Method,
retained streams, I/O failure/retry, E_ABORT и callback E_FAIL. При invalid mode
проверяется сохранность самих EA values. Сочетания повреждённых EA с известными
link/WOF/dedup roles сохраняют точный target/decoded payload либо настоящий
UnsupportedMethod, без ложной DATA проекции. GUI EN/RU проверяет consent/cancel,
видимый режим, фактическое копирование обычного файла и затронутого DATA,
повторное открытие и unchanged source. UI geometry/text проверены через
current-build native Cocoa JSON; screenshots/pixel parity не заявляются.

Focused `ntfs`, `ntfs_stream`, `ntfs_security`, `language` прошли локально;
`filesystem_read_ui` повторён с доступом к native Cocoa после sandbox отказа
из-за отсутствия экранов и прошёл за 58,86 с. EN warning 420×214, RU 420×275,
текст полностью видим. Linux прошёл все пять suites, receipt
`20261021T054403Z-781cec04`. В этом же receipt Windows прошёл четыре suites,
но ABI backend dirty-volume case получил codec discovery timeout. Повторный
`ntfs_stream` прошёл на обоих guests без изменения timeout, receipt
`20261021T054644Z-59c43f70`. Финальное уточнение Method для известных reparse
roles и три combined fixtures проверены `ntfs`/`ntfs_stream` на macOS (22,15 с),
Linux и Windows (60,08 с), receipt `20261021T054918Z-8f602a34`.
Приложение собрано локально после последнего изменения.

Native Linux 6.8.0-147, receipt `20261021T054610Z-191213da`: семь readonly
loop controls, exact обычные file bytes и unchanged images. Selected,
nonresident-list, split и metadata-flags layouts дали UID 2067/GID 4097/mode
0100751; unlisted EA не применились. Bad-id оставил обычный alpha читаемым,
но kernel отверг gamma с EINVAL; archive всё ещё может прочитать его DATA.
В bad-sequence старый kernel применил EA, archive строже проверяет sequence
и явно оставляет DATA без утверждения достоверности EA. Это различие
record-validation policy, а не native parity этих metadata. Malformed EA
buffers не передавались старому kernel как safety validator. Временные
mounts/images/helper удалены; incremental trees и compiler caches сохранены.

Остальной NTFS read audit и весь multi-format goal остаются открытыми.
Full/slow suites, physical devices, installed package и native Windows/Linux
appearance этой итерацией не проверялись.

## DATA membership, ADS и частично доступные mappings

Linux/SDK repositories и full revisions закреплены выше. Сопоставлены полные
`inode.c:ntfs_read_mft`/ADS opening, `frecord.c:ni_enum_attr_ex`/`ni_find_attr`,
record/attribute-list lookup, `attrib.c:attr_load_runs_vcn`/`attr_load_runs_range`,
`run.c:run_unpack` и SDK CAttr parser, extension merge, DataParseExtents,
CInStream, WOF consumers, item properties и extraction callbacks. Linux
перечисляет выбранные list entries и загружает нужные mapping segments; прежнее
безусловное SDK объединение active DATA по одному base reference этого не
обеспечивало. Original C enumeration/lookup bodies сохранены, private entry
projection дополнена bounded pointer на исходное имя. Новых kernel C units нет.

Active unnamed DATA и ADS теперь выбираются до merge по type/name/instance,
record index и VCN, с проверкой sequence/base owner. Противоречивый sequence или
owner доступного active extension record допускает только явно непроверенное
DATA-представление, если совпадают имя/instance/VCN и валидны source bounds.
Raw metadata остальных типов не объединяются по такому reference; некорректный
extension больше не превращает failure отдельного DATA в отказ всего образа.
Непроверенные bytes не объявляются достоверными. Имена ADS сопоставляются точно, включая
case-distinct и Unicode/spaces, как enumerating ADS caller Linux; unused named
case-folded VFS lookup не добавлялся. Names с embedded NUL не превращаются в
другой addressable stream: unaddressable list name пропускается с ограничением,
а reference на несовпадающее raw attribute name остаётся ошибкой потока.
Unlisted stale base/extension DATA не заменяет выбранный payload. Без списка
используются base attributes; deleted records сохраняют исходный forensic merge.
Исключённый из списка прежний default DATA остаётся ошибкой потока, а не ложным
пустым файлом; его независимые ADS доступны.
Ранее переданные Volume/reparse/EA selectors используют ту же typed ownership
boundary; оригинальные C bodies и все 31 baseline hashes повторно проверены.

Недоступный primary остаётся видимым ошибочным stream без выдуманного размера;
независимые ADS и другие файлы читаются. При недоступном continuation исходный
SDK parser сохраняет проверенные segments. Unknown mappings имеют отдельный
error marker, а не sparse marker: Read возвращает ошибку при достижении такого
диапазона; Seek допускает чтение доступных участков после него. Compressed unit
с unknown mapping не декодируется и не публикует cache output. Sparse zeros и
initialized tail сохраняют исходную семантику. Полное извлечение с missing bytes
возвращает DataError, даже если часть байтов уже скопирована. Буферы, mappings и
source owner принадлежат выданному stream и переживают Close/reopen.

Если list целиком недоступен или malformed, reader предлагает только локальные
base DATA mappings как явно непроверенное recovery-представление, без blind
extension merge. Это полезный archive read subset, а не утверждение kernel
acceptance или актуальности старых данных. Согласие не восстанавливает missing
bytes и не снимает source bounds/path validation. Error property отмечает
затронутый stream; archive reason требует явного выбора одного открытия и
остаётся видимым при навигации/копировании. Metadata-only list failures без
признаков DATA не порождают дополнительное ложное DATA warning.

Потребители подключены вместе: при недоступном выбранном `$Bitmap` free space
остаётся неизвестным, при недоступном `$Secure:$SDS` не публикуются unverified
security descriptors; обычные файлы остаются читаемыми. Недоступный `$AttrDef`
ограничивает EA через уже проверенный partial-EA path. MFT bootstrap остаётся
обязательным условием enumeration; это не новый reader недоступных MFT records.
Предварительная проверка также подключает полезный bounded partial mode без
списка, когда primary run читается, а продолжения недоступны. Unlisted extensions
при этом не объединяются. Полностью недекодируемые mappings сохраняют lazy
per-file failure, включая `$Boot` zero-delta, отвергаемый SDK и закреплённым
`run_unpack`. Непубликуемый service `$BadClus:$Bad` не создаёт ложное opening
warning. Glue остаётся у существующих SDK CMftRec/CDatabase/stream owners:
вынос его в отдельный reader потребовал бы дублирования private MFT model.

Source language, notices и isolated GPL dynamic-module closure сохранены.
Standalone SDK не получает Linux-specific selection/partial mapping; оба runtime
overlays собираются из одного handler source с прежней macro boundary. Это
подключение существующих SDK stream/map bodies к Linux metadata lookup, а не
независимый DATA или compression decoder.

Двадцать один layout покрывает selected/stale DATA, resident/nonresident list,
split mappings, точные case-distinct и Unicode ADS, absent/invalid primary,
missing first/middle runs, sparse/initialized tails, недоступный compressed unit,
unreadable/malformed list, stale sequence/owner, embedded NUL и зависимые
`$Bitmap`/`$SDS` consumers. CLI проверяет точные bytes, partial stdout + DataError
и unchanged source; для case-distinct ADS использован явный `-ssc`, поскольку
Windows CLI иначе выбирает оба имени своей case-insensitive маской. ABI проверяет
Seek после missing range, unknown size/error properties, lifetime после Close,
I/O recovery/reopen, callback E_FAIL/E_ABORT и отсутствие unverified ACL/free count.
Backend действительно копирует selected primary/Unicode ADS и сообщает failure
при incomplete copy. GUI EN/RU копирует local DATA после явного согласия;
cancel проверен общим service по каждой reader family, acceptance/geometry — по
всем различным reasons без дублирования одинаковых cancel combinations.

DATA/list recovery `ntfs`, `ntfs_stream`, `ntfs_security` прошёл на macOS arm64 (23,57 с),
Linux x64 (61,33 с) и Windows x64/MSVC (84,55 с), receipt
`20261021T062529Z-6af04568`. `language` и native Cocoa `filesystem_read_ui`
прошли локально, GUI — 51,83 с; warning EN 420×214/RU 420×275 полностью видим.
Guest language/GUI и Linux пять suites прошли в `20261021T061112Z-aca82c53`;
Windows ABI/security/language/GUI там прошли, CLI выявил описанную case-mask
ошибку теста. После исправления точного выбора обе DATA suites прошли на обоих
guests в `20261021T061648Z-79f656f6`, затем финальная recovery-семантика проверена
выше. Приложение собрано локально после последнего source change. Source bodies
`al_enumerate`/`al_find_ex` повторно сравнены с оригиналом без изменений.

Native Linux 6.8.0-147, receipt `20261021T063212Z-abd1aadc`: одиннадцать
readonly loop controls, exact обычный alpha и unchanged images во всех случаях.
Selected/nonresident/split DATA совпали с archive payloads. Missing middle
дал EIO при сохранных читаемых первом/последнем ranges; известный sparse range
вернул нули, initialized tail также вернул нули без требуемого mapping.
Stale sequence и оба owner contradictions вернули выбранные DATA bytes: archive
сохраняет эту возможность только как unverified mode с предупреждением.
Unlisted default и bad-id отвергнуты kernel с EINVAL; archive также сообщает
ошибку default stream, сохраняя другие файлы/ADS. Доступ после invalid first
mapping и incomplete compressed unit проверен archive ABI/CLI, не native mount.
Временные mounts/images/helpers удалены. Scoped cleanup завершённых fixtures
на обоих guests — `20261021T063049Z-7cf9463d`, сборки/кэши не затрагивались.

Native controls проверяют unnamed DATA; ADS проверены по pinned source и через
archive ABI/CLI/backend. Это не native acceptance современных ADS interfaces
на старом guest kernel. Остальной NTFS и multi-format read scope остаются
открытыми; full/slow, physical devices, installed package и native Windows/Linux
appearance не проверялись. Incremental builds и compiler caches сохранены.

Дополнительный no-list prefix контроль — `20261021T064539Z-5ef34cee`:
native 6.8 прочитал точный первый run и обычный alpha, readonly image не изменён.
В недостающем tail старое ядро вернуло 4096 нулей при declared size 12288.
Archive намеренно возвращает DataError после доступного prefix, не принимая
unknown mapping за подтверждённый sparse range. Pinned `attr_load_runs_vcn`
проверяет requested VCN против фактического segment range; полный runtime
этого pinned kernel данным старого guest не подтверждается. No-list prefix,
partial-copy failure и retained stream отдельно проверены archive ABI/CLI.

После расширения на no-list prefix те же три suites повторно прошли на macOS
(24,83 с) и Linux (60,56 с), receipt `20261021T064542Z-69264566`. Windows в нём
прошёл CLI/security, ABI suite завершилась по timeout. Адресный повтор
`ntfs_stream` без изменения кода/timeout прошёл за 50,03 с,
`20261021T065127Z-b649399f`; причина единичного timeout не установлена.
Локальное приложение пересобрано после этого source change; language/UI inputs
не менялись. Эти результаты дополняют, а не заменяют source comparison.

## File metadata, names и parent paths — 2026-10-21

Повторно сравнены `inode.c:ntfs_read_mft`, перечисление ATTR_LIST через
`frecord.c:ni_enum_attr_ex` и `attrlist.c:al_enumerate`, а также
`dir.c:ntfs_dir_emit` и типы STANDARD_INFORMATION/FILE_NAME закреплённого
Linux snapshot. Сопоставлены полные затронутые ParseRec, extension merge,
item/path construction и property consumers исходного 7-Zip и текущего порта.
Первый пригодный base STANDARD_INFORMATION сохраняет приоритет: Linux читает
его до переключения на ATTR_LIST. FILE_NAME выбирается по списку, reference,
instance и границам исходных атрибутов вместо объединения старых aliases.
Оригинальные parsers, vectors и Linux C list enumerator переиспользованы.

Отсутствующее/непригодное SI или имя не запрещает независимо читаемый DATA.
Неизвестные времена/permissions не подменяются нулевыми значениями; сохранены
upstream имена `[LOST]`/`[NONAME]-…`. Orphan DOS alias и доступное имя со stale
extension reference остаются явно непроверенным recovery-представлением.
Общий limitation property включает конкретную EN/RU причину и использует
существующие согласие на открытие, постоянный badge и read-only операции.
Отказы повреждённых FILE/MFT records изолированы
[последующим сценарием](#mft-record-failure-isolation--2026-10-21).

Эта итерация сохраняла MFT namespace; перечисление `$I30` добавлено
[следующим сценарием](#directory-indexes-i30--2026-10-21). Циклы родительских связей направляются
в recovery-каталог, конечные пути глубже 256 компонентов сохраняются в пределах
исходного ограничения длины. Это узкая адаптация single-parent archive graph:
upstream CFB traversal работает с другим sibling/child graph, а подходящего
готового helper в NTFS/Ext/HFS handlers нет. Алгоритмы формата/декодирования
не заменялись. Standalone сохраняет исходный metadata acceptance scope.

17 новых детерминированных fixtures проверяют выбранные/устаревшие имена,
resident/nonresident lists, hard links и DOS alias, SI precedence, отсутствие
metadata, повреждённые имена, родителей, циклы и путь из 300 каталогов.
CLI/ABI проверяют exact DATA, metadata properties и retained streams; GUI
копирует файл с непригодным SI после согласия. Образы остаются неизменными.
На macOS arm64 пять focused suites `ntfs`, `ntfs_stream`, `ntfs_security`,
`language`, `filesystem_read_ui` прошли за 21,43 с, включая native Cocoa EN/RU
consent/badge geometry. Linux прошёл те же пять suites в
`20261021T073808Z-07e9c4ac`. Windows прошёл четыре, а `ntfs_stream` завершился
по прежнему timeout при совместном прогоне; адресный повтор без изменения
кода/timeout прошёл в `20261021T074204Z-0fef8bd4`. Причина timeout не установлена.
Новых native-kernel mount checks этого scenario нет;
archive recovery adaptation не заявляется как полная VFS namespace parity.

## Directory indexes `$I30` — 2026-10-21

Сравнены полные `dir.c:ntfs_readdir`, `ntfs_dir_count`, `de_countable_fname`,
`ntfs_dir_emit`, `index.c:indx_init`, `indx_get_root`, `indx_read_ra`,
`indx_used_bit`, `scan_nres_bitmap`, их callers и disk types закреплённого Linux.
Перенесён оригинальный `ntfs_dir_count` с полным обходом root и занятых index
buffers по bitmap. Предикаты FILE_NAME и DOS namespace сохранены; callback
добавлен рядом с подсчётом, а проверка home reference взята из `ntfs_dir_emit`.
Allocated buffers используют уже перенесённые NTFS3 read/fixup/header services.
Дополнены исходные `ntfs.h` disk types; shared `lib/find_bit.c` получил оригинальный
`_find_next_bit` с раскрытым C11 `FIND_NEXT_BIT`, как прежний zero-bit helper.
Manifest расширен этой dependency; проверены все 32 hashes исходного snapshot.

`dir.c` остаётся единственным source owner link conversion и directory bodies.
Две C compilation contexts используют свои узкие inode adapters; GPL directory
object входит только в `sevenzip_ntfs`. Буферы/типизация и имя исходного C-файла
сохранены. Новый C++ код ограничен выбором resident/nonresident `$I30` attributes
по ATTR_LIST, существующими mapping/stream services и публикацией archive names.
Нового decoder/независимого обхода формата нет. В SDK directory-index reader
отсутствует; оригинальная MFT enumeration остаётся восстановительным fallback.

Bitmap читается приватными блоками, без полной материализации. В оригинальном
`scan_nres_bitmap` kernel run lookup, buffer_head и locks заменены проверенным
логическим stream; сохранены block scan, valid/data bounds и `scan_for_used`.
Нули за initialized tail не извлекаются из старых физических bytes. Resident и
nonresident bitmap используют один stream boundary. Ошибки callback/отмена
сохраняют HRESULT и прерывают открытие; ошибка чтения отдельного индекса допускает
независимый MFT/DATA recovery. Reopen очищает прежнее ограничение и восстанавливает
нормальное чтение. Память ограничена index/bitmap блоками, кроме уже существующих
MFT/attribute ownership и archive-name vectors.

Полученные имена объединяются с пригодными MFT именами; несовпадение, stale
extension ownership/sequence, неисправный список, пропущенный target или malformed
key явно помечают metadata как непроверенные. Идентичные пути с разным parent
sequence не публикуются дважды. При непригодном имени в MFT индекс может сохранить
доступный путь; без обоих источников остаётся исходный `[LOST]/[NONAME]-…`.
Недоступный MFT target не превращается в фиктивный пустой файл. Небезопасные
компоненты проходят тот же bounded filename/path validation. System namespace
и deleted recovery сохраняют исходное archive представление. Неадресуемые child
references, self references и fixup correction приватного буфера сообщаются как
непроверенные; child tree не обходится рекурсивно. Disk writes, journal replay,
VFS inode/dentry cache и collated pathname lookup не добавлялись. Роли объектов
берутся из уже прочитанного MFT/reparse/WSL metadata, без дополнительного VFS iget
ради `d_type`. Standalone сохраняет прежний classic reader без GPL linkage.

26 deterministic fixtures проверяют resident/allocated roots, fragmented mapping,
root/continuation attributes в extension record, resident/nonresident bitmap и
его вторую страницу, initialized tail, 1 KiB index blocks, case-distinct links,
повреждения/отсутствие имени или index, stale references, duplicate names и path escape.
CLI/ABI проверяют точные bytes, неизвестные/непроверенные metadata, retained streams,
ошибки I/O, callback E_FAIL/E_ABORT, reuse и unchanged input. GUI действительно
копирует DATA при отсутствующем root index после per-opening consent и сохраняет
видимый badge; EN/RU Cocoa geometry проверяется существующим общим сервисом.

Native Linux 6.8.0-147 подтвердил семь корректных вариантов: те же Unicode/case
имена, payload, hard-link count и read-only mount/device flags. Восьмой контроль
с несовпадающим именем MFT/`$I30` перечисляет index name, но получает ENOENT при
чтении обоих имён; archive намеренно сохраняет читаемый DATA по обоим recovery
именам с явным предупреждением. Все SHA-256 неизменны; mounts/loop devices/images
удалены. Receipt `20261021T081924Z-e2f238cc`. Старое native kernel не доказывает
runtime всей закреплённой версии; malformed keys старому kernel не передавались.

Итоговые пять suites `ntfs_directory`, `ntfs_stream`, `ntfs_security`, `ntfs`,
`filesystem_read_ui` прошли на macOS arm64 (29,30 с), Linux (85,24 с) и Windows
(135,18 с), receipt `20261021T083507Z-3a83c6bc`. До выделения I30 в собственную
suite совместный Windows прогон достиг старого timeout `ntfs_stream`; причина
того timeout не доказана. Новая матрица не расширяет прежнюю suite: fixture/COM
support вынесен без изменения поведения, локальный I30 прогон занял 2,45 с.
Product code между этими прогонами не менялся, timeout не увеличивался.
Native Cocoa подтвердил EN/RU consent/copy и читаемость сообщения (420×214 /
420×262); guest UI проверки не устанавливают pixel parity с Windows reference.
Приложение пересобрано инкрементально, upstream provenance verifier прошёл.
Full/slow suites, установка и физические носители не проверялись.
После финальной сверки одинаковые MFT names также объединены перед публикацией
items: повторяющийся FILE_NAME больше не приводит к двойному извлечению одного
пути. Расширенные `ntfs_directory`/`ntfs` прошли повторно на macOS (8,99 с),
Linux и Windows, receipt `20261021T084317Z-4c707931`; прочие product paths и
предыдущие passing checks не менялись. Этот повтор проверяет локальное уточнение,
а не подменяет пять suites и native reference evidence выше.

## MFT record failure isolation — 2026-10-21

Сравнены полные `record.c:mi_read`, read/validation ветви `mi_enum_attr`,
`inode.c:ntfs_read_mft`, `fsntfs.c:ntfs_fix_post_read` и их types/callers
закреплённого Linux с оригинальными `CMftRec::ParseRec`/`CDatabase::Open` 7-Zip,
текущими DATA/metadata selectors, I30 projection, property и extraction consumers.
Linux загружает и отклоняет конкретный inode; исходный SDK останавливает весь
MFT scan при одной повреждённой записи. При уже проверенной bootstrap mapping
это не является основанием закрывать все независимо читаемые файлы.

Исходный fixed-size/bulk scan сохранён. Ошибка большого чтения повторяется
по целым records внутри той же проверенной карты; unreadable frame не передаётся
parser. Повреждённая запись не сдвигает slot numbers. Error/cancel callback и
E_ABORT/OOM сохраняются; полное отсутствие читаемых records не публикует ложный
успешный пустой архив. Успешный record-sized retry без потери данных не создаёт
фиктивного ограничения. Первичная bootstrap mapping по-прежнему обязательна:
непригодный MFT record 0 или недоступная карта остаются техническим отказом.

Границы record/header/USA, attribute values и physical runs продолжают проверяться.
При повреждённом attribute tail сохраняются уже проверенные attributes только
после полного header/frame validation; пригодный DATA prefix не теряется из-за
последующего неправильного атрибута. Отсутствующий DATA не становится пустым
файлом. При непригодном frame все частично набранные attributes отбрасываются;
имя из I30 может представить доступный slot как unreadable item с неизвестным
размером и конкретной ошибкой. Неадресуемый slot за границами MFT не выдумывается.
Недостоверные ACL из неполного `$Secure` не публикуются, а повреждённый `$Bitmap`
оставляет free-space statistics неизвестной. Independently readable DATA остаётся
доступным с отдельной EN/RU причиной, consent на открытие и постоянным badge.

Оригинальный C `ntfs_fix_post_read` переиспользован через узкий C adapter.
Как `mi_read`, он восстанавливает private-buffer fixups при USN mismatch.
Неполное USA coverage допускается в границах record, как в NTFS3, с явным
unverified indication; нулевой count и выход за buffer не допускаются.
Stored record number не определяет адресацию NTFS3: несовпадение с запрошенным
slot сохраняет чтение с предупреждением. Signature/geometry/run/path validation
не заменены догадками. Journal replay, repair и source writes отсутствуют.
Standalone сохраняет прежние C++ checks; GPL C остаётся в отдельном module.

19 deterministic record fixtures проверяют header/USA/signature/offset/attribute
границы, validated DATA prefix, недоступные root/bitmap/security/extension
records, Unicode, bootstrap и mismatch recovery. Отдельная `ntfs_record` suite
использует общий fixture support; она также проверяет прежние
`standard-record-total`/`invalid-initialized` через наблюдаемое per-file failure,
а не устаревший отказ всего образа. CLI/ABI проверяют точные healthy bytes,
неизвестный размер/error у unreadable items, bounded retry, unexpected EOF,
полностью неудачное чтение, callback E_FAIL/E_ABORT, reuse и retained streams.
Test faults задаются диапазоном input, без проверки исходников/порядка private
вызовов. GUI действительно копирует healthy file после EN/RU consent.

Native Linux 6.8.0-147 подтвердил шесть read-only controls, receipt
`20261021T092111Z-197aa309`: ошибка total в gamma даёт EINVAL только этому
файлу; alpha/Unicode остаются читаемыми. USN mismatch, partial USA coverage,
bootstrap fixup и stored record-number mismatch читают исходные bytes.
Block device и mount read-only; все SHA-256 неизменны; временные mounts,
loop devices и images удалены. Старое native kernel не доказывает runtime всего
закреплённого snapshot; malformed attribute tails ему не передавались.

Семь focused suites `ntfs_record`, `ntfs_directory`, `ntfs_stream`,
`ntfs_security`, `ntfs`, `language`, `filesystem_read_ui` прошли на macOS arm64
(29,61 с), Linux (91,41 с) и Windows (168,60 с), guest receipt
`20261021T092617Z-30acebe3`. После добавления двух USA boundary fixtures и
уточнения текста «недоступны или не проверены» четыре затронутые suites
`ntfs_record`, `ntfs`, `language`, `filesystem_read_ui` прошли на macOS (30,48 с),
Linux (38,14 с) и Windows (107,82 с), receipt `20261021T093341Z-34888a1a`.
Native Cocoa EN/RU подтвердил consent/copy, badge и читаемость окончательного
текста при 420×253 / 420×301; guest UI не устанавливает pixel parity.
Приложение собрано инкрементально; provenance verifier прошёл
(422 exact / 242 adapted / 12 local), все 32 hashes Linux baseline сверены.
Full/slow suites, установка и физические носители не проверялись.

## Security metadata failure isolation — 2026-10-21

Проверка политики `AGENTS.md: Read-only filesystem access` выявила отказ всего
Open из-за повреждённого `$SII` или descriptor при независимо читаемом DATA.
До исправления повторно сравнены полные Linux `ntfs_security_init`,
`ntfs_get_security_by_id`, `is_sd_valid`, index lookup/fixup/read services,
`super.c` mount caller и `xattr.c:ntfs_getxattr`, а также SDK `ParseSecuritySDS`,
текущие attribute selectors, logical stream callbacks, raw-property cache и
GUI open/reopen/consent consumers. Baselines остаются указанными выше.

Linux mount может отказать при непригодном security index; отдельный xattr lookup
не является условием обычного DATA read, а некорректный descriptor frontend лишь
сопровождает предупреждением. Archive eagerly запрашивает права всех items;
перенос отказа lookup на весь Open был дополнительным product gate. Теперь
ошибка конкретного descriptor оставляет независимо проверенные ACL других IDs.
Непригодный root/mapping делает права неизвестными без отказа читаемым файлам.
Missing IDs, malformed descriptors/indexes, read errors и unexpected EOF дают
конкретные archive/item diagnostics и общий consent/badge ограниченного режима.
Reserved IDs не требуют descriptor и не создают фиктивного ограничения.

Проверки index/descriptor spans, SID/ACL, logical/physical bounds и paths
сохранены; оригинальные Linux C lookup/validation bodies не заменены.
Узкая archive adaptation отделяет optional metadata failure от callback failure,
отмены и OOM: последние по-прежнему завершают Open. После Close/reopen отсутствуют
старые descriptors и старые ограничения. Source остаётся read-only, журнал не
воспроизводится, repair не выполняется. Standalone сохраняет прежний SDK scanner;
GPL C остаётся только в отдельном dynamic module.

Private-buffer index fixup recovery и неполное USA coverage больше не позволяют
назвать ACL проверенным: C adapter возвращает отдельную ошибку для такого lookup,
не передавая descriptor bytes. Shared read service сохраняет indication о partial
coverage и для directory consumer. Следующий независимый lookup получает свой
статус; восстановленные buffers не записываются в образ.

Прежняя malformed matrix проверяет доступный payload, отсутствие raw ACL,
непустую диагностику, retained streams и восстановление после открытия исправного
образа. Добавлены partial-fixup и два mixed-ID fixtures: invalid gamma descriptor
не удаляет точные alpha permissions. I/O cases различают E_FAIL/EOF, E_ABORT/OOM
и callback errors. CLI проверяет реальные bytes при испорченном root/descriptor;
EN/RU GUI копирует alpha и gamma после согласия, сохраняет badge, запрашивает
согласие при новом открытии и проверяет неизменность input.

Native Linux 6.8.0-147 прочитал все три payload в control, short-descriptor и
invalid-ACL-revision образах; raw xattr вернул исходные 72/8/72 bytes соответственно.
Archive намеренно не экспортирует последние два malformed ACL, сохраняя DATA.
Block devices/mounts были read-only; SHA-256 всех образов неизменны; временные
mounts/loops/images удалены. Receipt `20261021T094248Z-b5413be3`. Это отдельное
свидетельство старого kernel, не runtime acceptance закреплённого snapshot.

Локально на macOS arm64 прошли `ntfs_stream`, `ntfs_directory`, `ntfs_record`,
`ntfs`, `language`, `filesystem_read_ui`; первая новая mixed-ID проверка выявила,
что исходный control alpha имел 48-byte SI без security ID. Добавлены явные
два-ID fixtures вместо ослабления проверки; финальная `ntfs_security` прошла за
11,52 с. Product code после первого прогона не менялся. Native Cocoa подтвердил
EN/RU consent/copy и отсутствие clipping при 420×214 / 420×275.
Linux прошёл все семь suites за 93,22 с, receipt `20261021T094357Z-24e74799`.
Windows в том же batch прошёл шесть; `ntfs_stream` достиг прежнего 60 с timeout,
причина которого не установлена. Timeout/cache/toolchain settings не менялись.
Изолированный повтор `ntfs_stream` на том же коде прошёл за 52,29 с,
receipt `20261021T094808Z-ad26d523`; он дополняет шесть passing suites batch,
а не скрывает его timeout.
Приложение собрано инкрементально, provenance verifier прошёл
(422 exact / 242 adapted / 12 local); сверены все 32 Linux baseline hashes.
Full/slow suites, установка и физические носители не проверялись.

## Открытый read scope

Известный security-metadata gate исправлен описанным выше сценарием.
Per-opening consent, постоянный limited-access badge, read-only input и честные
per-file errors проверены для рассмотренных MFT/DATA/name/security scenarios;
это не доказывает полное соответствие всех оставшихся read features.

Выбранные classic/WOF/MFT bootstrap scenarios подключены и проверены.
Остальные metadata/read features NTFS должны пройти оставшийся source audit;
эта итерация не закрывает остальные файловые системы и общие форматы goal.
VFS/page cache, mount services, запись/journal replay и EFS key service не
переносятся в archive handler; Linux сам не расшифровывает EFS при чтении.
