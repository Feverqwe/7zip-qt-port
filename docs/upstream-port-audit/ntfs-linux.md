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

## Открытый read scope

Выбранные classic/WOF/MFT bootstrap scenarios подключены и проверены.
Остальные metadata/read features NTFS должны пройти оставшийся source audit;
эта итерация не закрывает остальные файловые системы и общие форматы goal.
VFS/page cache, mount services, запись/journal replay и EFS key service не
переносятся в archive handler; Linux сам не расшифровывает EFS при чтении.
