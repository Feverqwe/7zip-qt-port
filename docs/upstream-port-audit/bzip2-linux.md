# Bzip2: end padding, integrity и cancellation

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/Bz2Handler.cpp` перенесён под исходным путём в `src/`
и включён в overlays обоих shipped engines. Сохранены parser/probe,
properties, OpenSeq, extraction, concatenated-member handling и update.
[Manifest](../upstream-baseline.json) закрепляет ownership, revision и runtime
fingerprint. Полное форматирование скопированного unit отделено от проверки
функциональных изменений с форматированным оригиналом.

До изменений прочитаны полный handler, весь `BZip2Decoder.cpp`/`.h`, CRC и
format constants: signatures, origin/map, selectors, Huffman lengths/build,
RUNA/RUNB/MTF, BWT, output RLE/randomisation, CRC, source/output HRESULT,
Code/Read и scout-thread lifecycle. Проверены единственный setter MinorError,
все его consumers и progress/error branches handler. После изменений
повторно сравнены полные functional diffs и callers. Общий decoder и его
алгоритмы не меняются; usable исходная реализация 7-Zip сохраняется.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
Прочитан весь `lib/decompress_bunzip2.c`: bit reader, tables/selectors,
Huffman/MTF/RLE, inverse BWT, bounds, CRC, stream end, memory и fill/flush;
также dispatch из `lib/decompress.c` и вызов из `init/initramfs.c`.
[Reference hashes](../bzip2-linux-baseline.json) закрепляют оригиналы и CRC
polynomial header. Linux C bodies не копируются в production и не переводятся
в C++; notices и лицензии исходного engine сохранены.

## Результат сверки

Оба reader принимают BZh levels 1..9, block/end signatures, groups 2..6,
Huffman lengths 1..20, selectors и byte maps. У 7-Zip сохранены partial dummy
trees и чтение дополнительных неиспользуемых selector records; его предел
используемых selectors достаточен для максимального decoded BWT block.
Linux хранит все 32767 selector records. Прямой fixture с максимальным count
и дополнительными unused selectors подтверждает одинаковое чтение payload.
Origin и block bounds, RLE expansion, block CRC и rotate/XOR stream CRC
сверены и сохраняют исходные проверки 7-Zip.

После end CRC Linux игнорирует остаточные биты до byte boundary. Общий
decoder 7-Zip также декодирует payload и проверяет CRC, но устанавливает
MinorError для ненулевых остаточных bits. Единственный соответствующий
archive consumer прежде превращал этот флаг в DataError. Теперь handler
игнорирует именно этот признак unused padding; CRC, malformed data,
unexpected end и data-after-end errors по-прежнему завершают операцию ошибкой.
Это работает и перед следующим concatenated member.

Open начинает чтение нового seekable источника с начала и учитывает callback
до публикации stream/state. Failure/cancellation оставляет состояние закрытым;
повторное открытие работает с новым источником. Extract возвращает HRESULT
последнего progress callback вместо игнорирования. Для короткого потока этот
callback может быть единственным: прежде Cancel приводил к operation OK.
Результат исправлен без изменения decoder/thread framework.

## Границы

Linux отвергает obsolete randomized blocks; соответствующая дополнительная
совместимость исходного 7-Zip сохранена. Новый randomized producer/runtime
fixture не добавлялся, и новая native acceptance этого расширения не заявляется.
Single-thread boot decoder Linux не заменяет существующий multithread decoder
архива; обе ветви 7-Zip проверяются отдельно.

Это законченная итерация standalone bzip2 read scenario. Composite initramfs,
границы отдельного decompressor и общий CPIO hard-link state остаются
в [едином реестре](filesystems-linux.md). Простой concatenated bzip2 output
не доказывает соответствие всей грамматике initramfs. Запись файловых систем,
boot/VFS и реальные носители здесь не проверяются.

## Validation

18 valid CLI cases на обоих engines: девять levels, empty/concatenated streams,
ненулевой end padding, maximal selector count, multiblock, длинный RLE run и
all-byte alphabet. Проверяются automatic recognition из `.bin`, exact payload
при threads 1/2, test command и неизменность источника. 13 malformed cases
завершаются normal code 2: signatures/levels, origin/groups/selectors,
CRC, truncation, tail и поздний повреждённый member. До исправления CLI
воспроизвёл четыре failures для двух padding cases на обоих engines.

ABI проверяет Open/OpenSeq, size/pack/member metadata, exact extraction,
source/output E_FAIL/E_ABORT, отмену короткой операции и активного multiblock
scout, failed-open cleanup и reuse. До исправления короткий Extract игнорировал
E_ABORT последнего callback, а Open игнорировал callback вообще. Прямой
ArchiveBackend consumer извлекает nameless entry с ненулевым end padding из
Unicode/spaces path; выбор использует опубликованное имя listing.

Временный C reference harness компилирует неизменённые Linux bunzip2 bodies
с userspace allocation/I/O adapters. Все 18 valid fixtures дали точные bytes,
а 13 malformed были отвергнуты. Дополнительно standard-library bz2 reader
подтвердил все positive payloads. Это compiled parser reference, не native
kernel boot acceptance. Disposable input/output trees удалены; небольшие
source/diff/failure/result evidence сохранены локально. Проверки переиспользуют
incremental builds и ccache.

Focused suites `bzip2`, `bzip2_stream`, `lzma`, `lzma_stream`, `gzip`,
`gzip_stream`, `native_folder_opening`, `codec_creation`, `runtime_cache`,
`runtime_package`, `upstream_baseline`, `upstream_inventory`: macOS arm64,
Linux x64 и Windows x64/MSVC — 12/12 PASS. Remote receipt
`20261020T084040Z-386ba0d0`. После уточнения braces/comment без изменения
поведения обе bzip2 suites также повторно PASS на macOS; remote набор использует
окончательный unit. Exact baseline verification: 421 exact, 228 adapted,
11 local paths, одна revision. Full/slow gate и UI appearance в этой итерации
одного format handler не выполнялись.
