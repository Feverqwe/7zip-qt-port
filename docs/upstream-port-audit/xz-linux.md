# XZ: streams, Index, random access и ошибки источника

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/XzHandler.cpp` скопирован в `src/`; reusable
`C/XzIn.c` и `CPP/7zip/Compress/XzDecoder.cpp` перемещены из exact baseline
в adapted под теми же upstream-relative paths. Сохранены notices, язык C,
parser, block cache, decoder, metadata, encoding/update и module boundaries.
[Manifest](../upstream-baseline.json) включает все три unit в overlays обоих
shipped engines; локальный ZIP consumer использует тот же adapted decoder.

До изменений сверены полные handler, backward/index reader и C++ decoder с
headers, wrappers, progress/cancellation и ZIP caller. В C unpacker проверены
block headers, filters/checks, Index/footer/padding, random-block и ST/MT
result paths; алгоритмы LZMA2/BCJ не заменялись. После изменений повторно
проверены functional diffs с оригиналами при одинаковом форматировании.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
Сверены `lib/decompress_unxz.c`, `lib/xz/xz_dec_stream.c`, headers и dispatch:
stream/block headers, VLI, filter properties, LZMA2 boundaries, block padding,
CRC32, Index hash/records, footer и single-stream termination. BCJ dispatch
проверен для x86, PowerPC, ARM, Thumb, SPARC, ARM64 и RISC-V.
[Reference hashes](../xz-linux-baseline.json) закрепляют исходники и зависимости
compiled reference. Linux bodies в production не добавляются.

## Исправления

Backward reader больше не ограничивает допустимый нулевой stream padding
64 КиБ. Сохраняется исходное сканирование, добавлены progress/cancellation
по ходу поиска и защита достижения начала файла. Поэтому длинный padding
теперь допускает построение block map и seekable чтение, а не только forward
extraction. Open учитывает исходную позицию, ошибки Seek/Read и callback;
неудача или отмена очищает опубликованное состояние.

Выданный stream владеет snapshot block map и ссылкой на исходный input.
Он остаётся доступным после Close, открытия другого архива и освобождения
handler. Interleaved readers сохраняют собственные позиции и block caches.
Валидный пустой XZ также возвращает EOF stream через IInArchiveGetStream.
Сохраняются исходные ограничения памяти для random-access cache.

Random-block Read возвращает HRESULT источника до обработки ошибочных bytes
и не публикует cache при ошибке. Общий C++ decoder также сохраняет source
HRESULT независимо от C error classification. Это существенно для E_ABORT:
wrapper переводит его в progress error, и прежнее условие только для READ
теряло причину сбоя. Extract учитывает последний progress callback и передаёт
фатальные HRESULT; malformed/unsupported format diagnostics сохраняются.

## Границы совместимости

Linux reference принимает checks NONE/CRC32 и один LZMA2 либо BCJ+LZMA2.
CRC64/SHA256 и Delta остаются дополнительными возможностями 7-Zip. Kernel
ANY_CHECK может пропускать неизвестные checks с warning; boot wrapper не
считает этот warning успешным завершением. IA64 и дополнительные filter
properties/chains 7-Zip сохранены без новой acceptance этих расширений.

Standalone XZ требует padding кратный четырём. Внешний initramfs parser Linux
пропускает произвольные NUL gaps; это отдельный контракт. Concatenated XZ
проверен здесь, а composite initramfs с общим CPIO state остаётся открытым в
[едином реестре](filesystems-linux.md). Kernel boot/VFS не проверялись.

## Validation

20 valid CLI fixtures на обоих engines: checks, все семь Linux BCJ filters,
Delta, empty/concatenated streams, explicit block sizes, multiblock,
zero-output blocks, incompressible LZMA2 и padding до 128 КиБ. Проверяются
automatic discovery из `.bin`, exact extraction с threads 1/2, test command
и неизменность входа. 16 malformed fixtures покрывают signatures, header/
block/Index/footer CRC, flags/filter/dictionary/padding, Index sizes,
truncation и tail; все возвращают normal exit code 2.

ABI проверяет retained/interleaved streams и block boundaries, empty EOF,
backward Seek/Read E_FAIL/E_ABORT, zero/partial forward source errors,
output errors, CRC/Index mismatch без выдачи данных, отмену Open/Extract,
cleanup/reuse. Дополнительные consumers — native ZIP method 95 и
ArchiveBackend extraction из Unicode/spaces path с длинным padding.
До исправления общего decoder regression воспроизвёл потерю E_ABORT.

Временный userspace C harness компилирует исходные Linux unxz и lib/xz с
allocation/I/O adapters и всеми семью BCJ. Все 36 исходов соответствуют
эталону: 17 positives дают exact bytes, CRC64/SHA256/Delta отвергаются,
malformed input отвергается кроме однобайтного NUL gap, допустимого внешним
initramfs parser. Совместимые filters дополнительно проверены Python lzma.
Это compiled decoder reference, не kernel boot или composite acceptance.
Disposable fixtures/output удаляются; incremental builds и ccache сохраняются.

Focused suites `xz`, `xz_stream`, `bzip2`, `bzip2_stream`, `lzma`,
`lzma_stream`, `gzip`, `gzip_stream`, `native_folder_opening`, `codec_creation`,
`runtime_cache`, `runtime_package`, `upstream_baseline`, `upstream_inventory`:
macOS arm64 14/14 PASS, Linux x64 14/14 PASS (receipt
`20261020T092053Z-2bba185c`). На Windows x64/MSVC в этом наборе прошли
12 suites; XZ CLI превысил 45-секундный timeout, backend discovery сообщил
failure. Повтор только двух XZ suites без изменения production-кода и timeout
прошёл за 5,23 с (receipt `20261020T092636Z-8ee7c739`). Причина первого
runtime сбоя не установлена. Первоначальная ошибка include нового теста
исправлена до этих проверок. Baseline verification: 419 exact, 231 adapted,
11 local paths, одна revision. Full/slow gate и UI appearance не запускались.
