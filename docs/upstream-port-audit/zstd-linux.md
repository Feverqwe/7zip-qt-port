# Zstandard: frames, skippable metadata и source failures

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/ZstdHandler.cpp` перенесён в `src/`;
`CPP/7zip/Compress/ZstdDecoder.cpp` перемещён из exact baseline в adapted
под исходным upstream-relative path. Сохранены metadata/parser, stream loop,
Code/Read, optional encoding branches и notices. Общий `C/ZstdDec.c` и его
алгоритмы не менялись. [Manifest](../upstream-baseline.json) подключает оба
unit к dynamic/standalone engines; native ZIP использует тот же decoder.

Перед изменениями прочитаны полный handler, C++ decoder/header, wrappers,
registration/detection и ZIP caller. В C decoder сверены frame/block headers,
sizes, dictionary IDs, window allocation/bounds, checksums, frame termination,
skip state и result/unused-input API. Linux framing сравнивался с этими ветвями;
Huffman/FSE/LZ implementation не заменялась. Functional diffs повторно
проверены с оригиналом при одинаковом project formatting.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
Сверены полный `lib/decompress_unzstd.c`, `zstd_decompress_module.c`, frame
header/dictionary/checksum и block/stream dispatch в lib/zstd, соответствующие
headers, initramfs dispatch и зависимости. [Reference hashes](../zstd-linux-baseline.json)
закрепляют kernel Zstd 1.5.7 и xxhash. Эти Linux bodies компилируются только
во временном reference harness; production сохраняет original 7-Zip decoder.

## Исправления

Активирован исходный IsArc probe 7-Zip для standard и всех 16 skippable magic
values. Он подключён к регистрации формата. Теперь `.bin`, начинающийся со
skippable metadata, автоматически открывается как Zstd; payload внутри skip
frame по-прежнему игнорируется. Сам parser и decoder уже поддерживали эти
frames, но прежняя signature-only регистрация не подключала этот read path.

Open начинает новый seekable input с начала, сообщает размер и проверяет
callback даже для короткого файла. Ошибки и отмена очищают partial metadata
и ссылки на source. Parse mode отвергает переполнение суммы declared sizes,
а не публикует обёрнутое значение размера concatenated frames.

Extract учитывает HRESULT PrepareOperation и последнего progress callback.
Code/Read общего decoder возвращают ошибку ReadStream до интерпретации
полученных при ошибке bytes. Поэтому E_FAIL/E_ABORT источника не теряется
за malformed-header/data classification; Read сохраняет причину и для
GetFinishResult. Существующие checksum/data/unsupported diagnostics, размеры,
Code/Read reset и unused-input API сохранены.

## Границы

Это standalone frame reader, native ZIP method 93 и backend integration.
Linux unzstd без внешнего dictionary и 7-Zip отвергают ненулевой dictionary ID;
нулевой ID во всех трёх полях ширины поддерживается. Отдельный kernel API
с caller-supplied DDict не является контрактом unzstd или archive input.
7-Zip сохраняет более широкий window limit; большие allocations не проверялись
runtime. Kernel wrapper comments о старом window limit не используются как
актуальная спецификация: проверены действующие defines/header paths.

Kernel initramfs dispatcher распознаёт обычный Zstd magic; поддержка skip frames
в lib/zstd не доказывает их автоматический выбор в boot parser. Проверенное
archive discovery шире этого dispatch. [Composite initramfs](cpio-linux.md#composite-initramfs--2026-10-20)
подключает Zstd single-frame boundary к общему CPIO state; полный format census
зафиксирован в [едином реестре](filesystems-linux.md).
Эта итерация не заявляет boot/VFS acceptance.

### Window boundary initramfs — 2026-10-22

Повторно сравнены полный `__unzstd`, header parser, `zstd_init_dstream`,
`ZSTD_DCtx_resetParameters` и single-pass/streaming branches
`ZSTD_decompressStream` с `DecodeZstd` в initramfs adapter. Header bounds
определяются действующими `ZSTD_WINDOWLOG_MAX_32=30`/`_64=31` из
`include/linux/zstd_lib.h`; комментарий boot wrapper о 128 MB не определяет
этот read limit. Adapter ошибочно применял 128 MiB как безусловный отказ.

Теперь он использует unsigned bound соответствующей разрядности size_t.
Исходный SDK decoder, memory validation, frame end и consumed boundary сохранены.
Standalone Zstd продолжает свой более широкий SDK scope; изменение относится
только к composite initramfs. Linux streaming context также имеет отдельный
default window limit 27, но complete known-size frame может пройти его
single-pass shortcut. Поэтому header bound и каждый runtime path различаются:
из этого изменения не следует native streaming acceptance любых больших frames.

Новые обычные fixtures имеют маленький известный content size и объявленные
окна 256 MiB, предельное для разрядности и превышающее предел. SDK выделяет
буфер по известному content size; большие allocations тест не создаёт.
До исправления оба positive cases дали E_NOTIMPL через CLI и ABI. После
исправления exact extraction/retained streams проходят, oversized frame
отклоняется. Compiled Linux buffer decoder подтвердил bytes обоих positives
и отказ negative; это не запуск boot streaming wrapper с большим workspace.

macOS `initramfs`, `cpio`, `cpio_stream`, `zstd`, `zstd_stream`,
`source_link_extract`, `upstream_baseline` — 7/7 PASS, 6.65 s. Использованы
прежние incremental build/cache; SDK overlays не менялись. Linux/Windows,
32-bit runtime, slow suites и native boot в эту проверку не входят.
Финальный macOS application target собран с обновлённым module.

## Validation

22 valid fixtures: raw/RLE, empty raw/RLE и zero-output blocks, multiblock,
compressed/Huffman, checksum, unknown size, 1/2/4/8-byte content size, unused
bit, zero dictionary IDs всех ширин, concatenated/empty-first frames,
все skippable IDs и skip-only/between frames. Compressed fixtures созданы
независимым libzstd 1.5.7 producer, level 5; checksum включён отдельно.
Runtime tests не требуют системной библиотеки или Homebrew sevenzip.

CLI проверяет оба shipped engines: automatic discovery, exact bytes при
threads 1/2, test command и неизменность input. 14 malformed fixtures
проверяют magic/reserved bits, block type/size, truncation, content size,
checksum/compressed data, ненулевой dictionary ID, skip length, tail и поздний
повреждённый frame. Ожидается normal exit code 2.

ABI проверяет Open/OpenSeq, sizes/frame counts, parse metadata и overflow,
source/output E_FAIL/E_ABORT, отмену Open/parse/PrepareOperation/final progress,
failed-open cleanup и повторное чтение. Прямые Code/Read проверяют ошибки
источника с нулевым и частичным результатом, ошибочные partial bytes,
GetFinishResult и reuse. Native ZIP method 93 и ArchiveBackend extraction
из Unicode/spaces path с leading skip frames проверяют прямых consumers.

Reference harness компилирует неизменённые kernel lib/zstd/xxhash и streaming
unzstd body с userspace allocation/I/O/header adapters. Buffer API проверяется
отдельно; outer loop использует kernel compressed-frame-size API. Все 72
исхода для 22 valid и 14 malformed fixtures в двух режимах соответствуют
ожиданию; positives дают точный payload. Это compiled source reference,
не kernel boot или composite acceptance. Disposable fixtures/output удаляются;
incremental builds и compiler caches сохраняются.

Focused suites `zstd`, `zstd_stream`, `xz`, `xz_stream`, `bzip2`,
`bzip2_stream`, `lzma`, `lzma_stream`, `gzip`, `gzip_stream`,
`native_folder_opening`, `codec_creation`, `runtime_cache`, `runtime_package`,
`upstream_baseline`, `upstream_inventory`: macOS arm64 16/16 PASS (4,94 с),
Linux x64 16/16 PASS (8,51 с; receipt `20261020T094227Z-a526e1bb`), Windows
x64/MSVC 16/16 PASS (49,22 с; receipt `20261020T094434Z-7c81e19f`).
После переименования локальной переменной для MSVC shadow warning две Zstd
suites также PASS на macOS и Linux с окончательным source fingerprint
(Linux receipt `20261020T094438Z-ad7a7377`). Изменение не меняет поведение.
Baseline verification: 418 exact, 233 adapted, 11 local paths, одна revision.
Full/slow gate и UI appearance не выполнялись.
