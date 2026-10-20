# Gzip: recognition, длинные headers и завершённое открытие

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/GzHandler.cpp` перенесён под исходным путём в `src/`
и включён в оба runtime overlays. Сохранены CItem, header/footer parser,
DEFLATE decoder, concatenated-member extraction, properties, update и
registration. Форматирование полного скопированного unit сверено отдельно
от функциональных изменений. [Manifest](../upstream-baseline.json) закрепляет
upstream и runtime fingerprints; notices и лицензия engine сохранены.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
Прочитаны полные `lib/decompress.c`, `lib/decompress_inflate.c`, их вызов из
`init/initramfs.c`, а также relevant zlib HEAD/TYPE/STORED/TABLE/bounds branches.
Полный generic Linux inflate и его зависимости ранее сверены и перенесены
в [ISO/zisofs module](isofs-linux.md); исходные hashes сохранены в его manifest.
[Gzip reference hashes](../gzip-linux-baseline.json) закрепляют wrapper и dispatch.
Со стороны 7-Zip сравнены probe, все read bodies CItem, Open/OpenSeq/Close,
Extract, decoder input/progress и прямое вложенное открытие gzip/TAR.
После изменений проверены полный функциональный diff и все read callers.
Существующий usable C++ parser сохранён; Linux C в production не копируется
и не переводится в C++.

## Исправления

Gzip probe принимает произвольный XFL byte, как reader и Linux. XFL описывает
параметры encoder и не меняет декодирование DEFLATE. Probe также игнорирует
unused bits до byte boundary в stored block, как Linux BYTEBITS, сохраняя
LEN/NLEN и остальные проверки структуры. Оба исправления устраняют отказ
автоматического распознавания корректных gzip streams.

FNAME читается до NUL/EOF вместо отдельного предела 4096; probe не отвергает
полный или частично доступный длинный header из-за его длины. Linux gunzip
также не устанавливает такой предел при переданном memory buffer. Аналогично
убран 65536-byte предел FCOMMENT, чтобы optional-field paths исходного handler
работали согласованно; это дополнительная совместимость gzip archive reader,
а не заявленная возможность минимального Linux boot wrapper. Общие ограничения
представления строки и ошибки allocation исходного AString сохранены.

Длинные header strings сообщают progress и проверяют отмену порциями при
Open и Extract. Для Open переиспользован существующий upstream adapter
ICompressProgressInfo → IArchiveOpenCallback. Ошибки и отмена при Open/OpenSeq
очищают источник, metadata и частичное состояние; повторное открытие работает
с новым источником. Проверка data CRC/ISIZE, concatenated streams и исходные
операции обновления не заменяются новой реализацией.

## Границы Linux reference

Минимальный `gunzip` из Linux читает fixed header и FNAME, декодирует raw
DEFLATE и прибавляет восемь bytes footer к consumed size. Он не реализует
обычный gzip reader для FEXTRA/FCOMMENT/FHCRC и не проверяет footer CRC/ISIZE.
Эти дополнительные archive возможности и более строгая диагностика 7-Zip
сохранены; CRC/ISIZE corruption не считается успешным чтением только потому,
что Linux boot wrapper его не проверяет. Reserved flags остаются malformed
для archive API.

`lib/decompress.c` направляет magic `1f 9e` в gunzip, но сам gunzip принимает
только `1f 8b 08`. Следовательно, таблица dispatch не доказывает поддержку
старого magic, и его отказ не является пропущенной read feature.
Сохранены более широкие optional gzip fields и member concatenation 7-Zip.

Это итерация gzip archive reader. Composite initramfs, raw/compressed member
transitions, точные границы каждого decompressor и общий CPIO hard-link state
остаются открытыми в [едином реестре](filesystems-linux.md). Простое объединение
выхода нескольких compressed members не доказывает соответствие initramfs:
Linux требует отдельного завершения CPIO state при выходе каждого decompressor.
Для LZO/LZ4 добавлены [GPL framing readers](linux-frames.md) в dynamic runtime;
они дополняют Linux block decoders файловых систем, но ещё не общий dispatcher.

## Validation

CLI обоих shipped engines: 15 valid вариантов, metadata и automatic recognition
из `.bin`, exact output, test command и source immutability. Покрыты XFL,
stored ignored bits, длинные FNAME/FCOMMENT, optional fields, fixed/dynamic
DEFLATE, пустой и concatenated streams. 12 malformed вариантов завершаются
code 2: magic/method/flags, незавершённые headers, DEFLATE, CRC/ISIZE/footer,
поздний повреждённый member и посторонний tail. Временные runtime copies
исключают посторонние plugins; полный runtime остаётся в ABI/backend consumer.

ABI проверяет публичный probe, Open и OpenSeq, metadata и exact extraction,
source E_FAIL/E_ABORT, отмену внутри длинного header при Open/Extract,
cleanup и повторное открытие. ArchiveBackend извлекает gzip с XFL 255 из
Unicode/spaces path. Дополнительно существующий `native_folder_opening`
проверяет открытие gzip с TAR entry, извлечение его bytes и update service.
Автоматическое рекурсивное открытие TAR через IInArchiveGetStream этим
не заявляется: исходный gzip handler не предоставляет этот интерфейс.

Временный C reference harness использует неизменённое тело Linux gunzip
с userspace allocation/I/O adapters и уже собранными Linux zlib C units.
13 применимых valid fixtures дали точные payloads, включая длинные имена,
stored padding и concatenated members. FCOMMENT и optional-field fixtures
не выдаются за native Linux positives. Harness не является kernel boot
acceptance; disposable файлы удалены. Source, diff и небольшие evidence
сохранены локально; incremental build trees и ccache переиспользуются.

Focused suites `gzip`, `gzip_stream`, `cpio`, `cpio_stream`,
`native_folder_opening`, `runtime_cache`, `runtime_package`, `upstream_baseline`,
`upstream_inventory`: macOS arm64, Linux x64, Windows x64/MSVC — 9/9 PASS.
Окончательный remote receipt: `20261020T080145Z-f9d629d1`; exact baseline
verification — 422 exact, 225 adapted, 11 local paths, одна upstream revision.
В первом Windows прогоне один первоначальный CLI launch превысил 10 s;
без изменения кода и лимита обе gzip suites затем прошли, receipt
`20261020T080049Z-897296cc`. Причина первого timeout не установлена;
окончательный полный focused набор также прошёл с прежним лимитом.
Full/slow gate, UI appearance, реальные устройства и boot не выполнялись
в этой итерации одного format handler.
