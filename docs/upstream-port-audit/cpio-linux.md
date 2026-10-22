# CPIO и составной initramfs

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
`CPP/7zip/Archive/CpioHandler.cpp` перенесён целиком под исходным путём в `src/`
и включён в runtime overlays обоих engines. Сохранены parser, структуры,
properties, legacy binary/odc readers, extraction/copy/checksum и registration.
Полное форматирование скопированного unit отделено от функциональной сверки
с форматированным оригиналом. [Manifest](../upstream-baseline.json) закрепляет
единственный upstream baseline; notices и лицензия engine сохранены.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
До изменений прочитаны целиком CPIO state machine, hard-link hash и
`unpack_to_rootfs` из `init/initramfs.c`, описание buffer format и dispatch
`lib/decompress.c`. [Source hashes](../cpio-linux-baseline.json) закрепляют
проверенные оригиналы. Со стороны 7-Zip прочитаны полный handler, consumers
`kpidINode`/`kpidStreamId` в extract callback, ограниченные/extent streams,
copy coder и zero-tail helper. После изменений повторно сравнены затронутые
ветви и callers. Использован существующий C++ parser 7-Zip; Linux C bodies
в production не копируются и в C++ не переводятся.

## Исправленный read scenario

Raw newc/crc принимает дополнительные NUL в объявленной длине имени до
PATH_MAX, необязательный trailer и отсутствие конечного alignment padding
после полного последнего payload. Распознавание и reader согласованы для
старых regular-file записей с ненулевым rdev. Неиспользуемый checksum newc
не мешает чтению, как в Linux. Допускаются смешанные newc/crc records,
произвольные нулевые промежутки с выравниванием следующих заголовков и
несколько raw members после `TRAILER!!!`.

Hard links объединяются по device/inode/type только для участвующих в группе
записей с nlink >= 2; directories и symlinks не подменяют regular-file данные.
Последняя непустая запись задаёт содержимое всех aliases, независимо от
её положения. Число сохранённых aliases может быть меньше host nlink.
Trailer сбрасывает группу: независимо созданные members с одинаковыми inode
сохраняют собственные данные. Без trailer группа сохраняется. Проверка CRC
охватывает также сохранённые записи, чьи данные заменены последующим alias,
и пустые записи с неверной суммой.

Открытие при source HRESULT failure или отмене очищает частичные items;
повторное открытие читает новый источник. Нулевые промежутки проверяют отмену
порциями. Entry streams используют существующий extent reader с собственным
положением и удержанием источника; interleaved reads, Close/reopen и release
handler не перенаправляют уже выданный поток на другой payload.

## Границы raw-итерации

Raw CPIO entry reading дополнено [composite initramfs](#composite-initramfs--2026-10-20)
ниже: смешанные compressed/raw members и общий hard-link state подключены
через decompressor dispatch. Обычное вложенное открытие `.cpio.gz` само по себе
не доказывает поддержку этой грамматики. [Legacy LZ4/lzop readers](linux-frames.md)
используются тем же dispatcher отдельного GPL module.

Linux распаковывает rootfs через VFS; archive API представляет записи и их
данные. Создание special nodes, chown, boot/mount и replay конечного namespace
при повторении path не являются реализацией readonly archive reader.
Binary LE/BE и odc сохранены как дополнительная совместимость 7-Zip:
initramfs Linux их отвергает. Archive parser сохраняет диагностику оборванных
headers/data, ненулевых name/alignment bytes и trailer с данными; отдельные
lenient kernel branches для malformed input не объявляются format features.
Возможность поиска вложенного архива с ненулевым offset также сохранена;
проверки malformed CPIO фиксируют начало (`-tCpio:s0`) либо используют ABI.

## Validation raw-итерации

CLI обоих engines: 26 valid вариантов, exact payload каждого regular entry,
hard-link extraction на диск, Unicode/spaces, symlink metadata, automatic
recognition и test command; восемь malformed вариантов завершаются code 2.
Legacy binary LE/BE и odc проверяются отдельно от Linux parity. Источники
неизменны; disposable extraction trees удаляются. CLI использует копии shipped
engines/core без посторонних plugins; ABI/backend сохраняет полный runtime.

ABI checks: metadata sizes, independent/interleaved/retained streams,
E_FAIL/E_ABORT при частичном открытии и чтении, zero-tail callback cancellation,
очистка после неуспешного Open и повторное открытие. Прямой consumer
`ArchiveBackend::extractSourceEntry` извлекает Unicode entry с дополнительными
NUL в имени. Сборка использует существующие incremental outputs и ccache.

Дополнительно временный C-harness компилирует исходный Linux state machine
и hard-link routines без изменения их тел, с POSIX file/allocation adapters.
Он подтвердил 23 valid сценария (regular payloads, symlink targets и shared
inode identities) и отказ трёх checksum negatives в disposable каталогах.
Mtime/chown и decompress dispatch в harness отключены;
это проверка compiled reference parser, не native kernel boot acceptance.
Нативная загрузка initramfs и реальные устройства в этой итерации не проверены.

Focused suites `cpio`, `cpio_stream`, `runtime_cache`, `runtime_package`,
`upstream_baseline`, `upstream_inventory`: macOS arm64, Linux x64 и Windows
x64/MSVC — 6/6 PASS. Remote receipt `20261020T073127Z-412fc9f8`.
После расширения fixtures без изменения production handler обе CPIO suites
повторно PASS на всех трёх ОС; remote receipt `20261020T073350Z-c969f54f`.
Exact baseline verification: 422 exact, 224 adapted, 11 local paths, одна
upstream revision. Full/slow gate и UI appearance не требуются изменением
одного format handler и в этой итерации не выполнялись.

## Composite initramfs — 2026-10-20

Прочитаны полные `unpack_to_rootfs`, `flush_buffer`, переходы Start/Reset,
CPIO header/body/alignment branches и hard-link state, dispatch `lib/decompress.c`
и все семь `lib/decompress_*` wrappers закреплённой Linux revision выше.
Со стороны SDK повторно сравнены полный CPIO handler и его extent consumers,
gzip/LZMA header/decoder/footer bodies, BZip2 stream loop, XZ unpacker states
и Zstd C decoder/frame status. Hashes reference units добавлены в существующий
[CPIO baseline](../cpio-linux-baseline.json); decoder baselines остаются у
индивидуальных format audits.

Динамический runtime теперь получает Cpio из отдельного GPL-модуля вместе с
Lz4Legacy/Lzop. В core dynamic engine повторная регистрация отключена;
standalone `7zz` сохраняет raw CPIO и вложенное чтение отдельных compressors.
Новые initramfs возможности предоставляют dynamic `7z` и приложение. Cpio
принимает raw/compressed последовательности с gzip, bzip2, LZMA, XZ, lzop,
legacy LZ4 и Zstd, продолжение после каждого кадра и optional trailers.
Реальный trailer сбрасывает hard-link группу; граница сжатия её сохраняет.

`initramfsreader` — адаптация Linux dispatcher/flush state к archive streams,
а не новый CPIO/compression parser. Исходный CPIO parser SDK остаётся единственным;
его metadata/extraction/hard-link path получает seekable canonical stream.
Каждая декомпрессированная часть отдельно проверяется на complete-record/Reset
boundary до подключения следующей. Частичный header, body или alignment нельзя
дополнить bytes из следующего compressed member. Начальный пустой compressed
member/leading decompressed NUL отвергается, но после полного CPIO record эти
переходы допустимы, как в Linux. Последний полный raw payload может не иметь
alignment padding. Нулевые промежутки и physical alignment учитываются отдельно
от локального alignment внутри compressed part.

`initramfsdecoders` использует существующие SDK decoder bodies, с остановкой
на одном member и возвратом exact consumed position после read-ahead.
Gzip/LZMA single-member entry points добавлены в исходные handler units;
compile guard исключает их обычные registrations из GPL target. BZip2, его
header/constants/MTF dependencies скопированы без изменений из того же SDK
commit по исходным путям; `Z7_ST` исключает неиспользуемую MT service ветвь.
XZ сохраняет SDK index/check/footer validation и существующий one-block stop,
Zstd — C frame-end/result API. LZ4/lzop используют уже перенесённые Linux C wrappers.
Encoder, PREBOOT, VFS node creation и boot services не копируются в этот сценарий.

`initramfscache` — узкий platform-service adapter для временного seekable storage.
SDK InOutTempBuffer предоставляет последовательный вывод, а текущий FileDir
порта зависит от Qt; ни один не является пригодным seekable plugin dependency.
POSIX cache создаётся через mkstemp, сразу unlink, с close-on-exec; Windows
использует private non-inheritable handle с delete-on-close и проверкой файла.
Удержание cache принадлежит source stream: Close/reopen handler не инвалидирует
выданные entry streams. Raw-only путь не создаёт cache. Неверный output path
не записывается из archive metadata. Cancellation/source/disk errors очищают Open.
Progress остаётся в пределах physical input; физический размер исходника
сохранён, а canonical entry offsets/packed sizes не выдаются за compressed offsets.

Расширения `.cpio`, `.initramfs`, `.initrd`, `.img` направляют пробу в Cpio;
обычные `.gz`/`.xz` и другие compressor extensions сохраняют прежнее открытие
внешнего compression format. Для чтения полного initramfs с таким именем
доступен явный `-tCpio`. Неподходящий образ получает S_FALSE и обычный format
fallback. Непредусмотренные corrupt streams не объявляются Linux features;
SDK checksum validation и дополнительные compression contracts сохранены.

CLI regression проверяет полные images, exact extracted files, automatic
recognition, input immutability, shared/trailer-reset hard links, большие
payloads, XZ multiple/empty blocks, смешанный образ со всеми семью compressors
и malformed part boundaries. ABI проверяет independent/retained streams всех
семи methods, source E_FAIL/E_ABORT, initial/mid/final cancellation, bounded
progress, physical metadata, invalid indices, unavailable temporary storage,
повторное открытие и ArchiveBackend extraction смешанного Unicode image.

Compiled reference использует исходные C bodies Linux dispatcher, initramfs FSM,
hard-link routines и всех семи wrappers с POSIX VFS/allocation adapters.
Проверены success/error boundaries и bytes; metadata ownership/time services
адаптированы, native kernel boot не выполнялся. Опасные malformed compressed
memory calls в старых boot wrappers не используются как validator.

Focused macOS arm64 / Linux x64 / Windows x64/MSVC: 15/15 suites PASS
(`cpio`, `cpio_stream`, `initramfs`, обе Linux frame suites и CLI/ABI suites
пяти SDK compressors). Remote receipt `20261020T140937Z-05aae5b6`.
После финальных XZ/raw-boundary fixtures и POSIX close-on-exec две затронутые
suites повторно PASS на всех трёх ОС; receipt `20261020T141242Z-7d284559`.
Итоговый reference: 94 valid images с exact bytes
и 49 invalid boundary cases, все результаты совпадают с исходным Linux FSM.
SDK baseline: 422 exact / 242 adapted / 12 local, одна revision. Full/slow,
native boot, физические устройства и UI appearance в эти проверки не входят.

## Symlink `TRAILER!!!` и итог read-side сверки — 2026-10-22

Повторно сопоставлены `init/initramfs.c:do_header`, `do_name`, `do_symlink`,
hard-link state и `unpack_to_rootfs`/`flush_buffer` с CPIO record consumers,
normalizer и single-member decoder boundaries. Все десять reference hashes
manifest совпали с закреплённым Linux snapshot. Вместе с полной сверкой raw
reader и composite scope выше это закрывает read-side source audit CPIO/initramfs
для archive projection; VFS effects и runtime/platform acceptance отделены ниже.

Обнаружен корректный случай, который reader терял: Linux направляет symlink
в `do_symlink`, поэтому имя `TRAILER!!!` не проходит trailer check `do_name`.
Исходный SDK predicate проверял только имя. Raw reader выдавал Headers Error
и частичный каталог, а composite normalizer отклонял decoded part; hard-link
aliases после такой записи не получали окончательные данные группы.

Оба license variants исходного CPIO handler теперь исключают newc/crc symlinks
из trailer predicate. `ReadInitramfsRecord`, raw enumeration и hard-link grouping
получают одно решение. Новый parser не добавлен; поведение настоящего trailer,
legacy binary/odc, checksum errors и строгая archive validation повреждённого
trailer body сохранены. Recovery/write/VFS services не добавляются.

CLI/ABI regression воспроизвёл отказ до исправления: два raw newc/crc варианта
в обоих engines и symlink в отдельной compressed части для всех семи methods.
Проверяются link metadata, окончательные bytes aliases до/после ссылки,
сохранение группы через compression boundaries и retained streams. CLI composite
checks выбирают regular entries, чтобы не требовать host symlink privileges.
Существующий compiled Linux C reference подтвердил оба raw варианта: exact bytes,
symlink target и общий inode aliases. Disposable reference trees удалены.

Финальные macOS suites `cpio`, `cpio_stream`, `initramfs`, `linux_frames`,
`linux_frames_stream`, `source_link_extract`, `upstream_baseline` — 7/7 PASS
(7.08 s), с существующими incremental builds и compiler cache. Linux/Windows,
slow и native boot не запускались: исправлена общая record classification,
platform adapters не менялись.
Финальный macOS application target собран с обновлёнными readers.
