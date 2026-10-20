# Legacy LZ4 и lzop: Linux framing readers

## Источники и компонент

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
Полные `lib/decompress_unlz4.c` и `lib/decompress_unlzo.c` перенесены в `src/`
по исходным путям, остаются C11. Прочитаны wrapper bodies, `parse_header`,
dispatch `lib/decompress.c`, caller `unpack_to_rootfs`, LZ4/LZO public types,
существующие raw decoder и checksum dependencies. Сохранены symbols, block loop,
allocation/cleanup flow и decoder calls. [Baseline](../linux-frames-baseline.json)
закрепляет исходники и адаптированные units. PREBOOT fast-output/kernel-image
ветви не компилируются; выбран bounded flush mode для archive/initramfs input.

В official 7-Zip `9128b80e3a471108678e2b3ec3c985861c8d7b0f` нет LZ4/lzop
wrapper/handler; существующие Linux filesystem block decoders не разбирают
framing. Повторного compression algorithm не добавлено. `LinuxFrameHandler`
использует single-item Open/Extract/properties и dummy/progress pattern
существующего `BrotliHandler`; его fork provenance и LGPL notice сохраняются
в [Brotli audit](brotli.md). `ArchiveExports`, `FindSignature::ReadZeroTail`,
`DummyOutStream`, `StreamUtils`, `ProgressUtils`, PropVariant/string dependencies
переиспользуются из official SDK. `linuxframestream` — узкий C/C++ stream adapter.

Отдельный GPL module регистрирует read-only `Lz4Legacy` и `Lzop` в динамическом
`7z`, каталоге приложения и install/runtime deployment. Notices поставляются
вместе с module. Standalone `7zz` не загружает внешние GPL modules; его scope
этим не расширяется. Контейнер современного LZ4 frame с другим magic также
не выдаётся за Linux legacy format.

## Адаптации границ

Kernel callbacks не имеют opaque context. Здесь I/O services передаются явно
для каждой операции, без общего mutable callback state. C остаётся C;
C linkage сохранён. Ошибки source/output/progress и allocation передаются через
границу, C++ exceptions не выходят в C. Чтение и запись разбиты на bounded chunks,
source ограничен reported size. Consumed count расширен до UInt64: Windows
`long` не подходит для суммарного размера многоблочного потока.

LZ4 сохраняет magic/repeated-magic loop, maximum chunk bound и original safe
block decoder. Zero-size terminator обрабатывается также при callback input,
как в memory-input branch Linux. Он не входит в consumed count, а остаётся
нулевым padding для следующего уровня. Не включена PREBOOT fast decoder ветвь,
предполагающая внешний размер output.

Для lzop initial header read может захватить bytes следующего member. После
успеха seek возвращает источник к reported consumed boundary, поэтому read-ahead
не теряет следующие данные. C API проверен с последующим raw CPIO prefix.
Archive handler поддерживает несколько frames одного format и нулевые промежутки;
ненулевой неизвестный tail получает DataAfterEnd. Повторный Extract начинает
с исходного потока; успешный unpacked size публикуется только после полного успеха.
Проверены отмена внутри padding, повторное использование и source lifetime.

В production используется seekable archive input, как при открытии обычного
файла/вложенного extracted payload. IArchiveOpenSeq/IInArchiveGetStream не
добавлены: seek нужен для восстановления границы после kernel read-ahead;
существующий file-resource workflow извлекает payload во временный файл.

## lzop integrity и дополнительная совместимость

Linux пропускает header checksum и фиксированные четыре bytes перед каждым
block. Последнее корректно лишь для одного D checksum: другие flags могут
привести к shifted stored payload. Поэтому перенесённый loop учитывает порядок
D-Adler/D-CRC/C-Adler/C-CRC и проверяет header/block integrity существующими
Linux Adler/CRC routines. Stored blocks не содержат отдельные C sums.
Поддержаны отсутствие checksum, обе D sums и compressed sums; это дополнение
к kernel boot profile, а не доказательство их корректного чтения исходным wrapper.

Сверены первичные `lzop` `read_header`/`write_header` и `lzo_decompress`:
<https://github.com/mirror/lzop/tree/1941c6fb1c8f5616aa74144fba09a13013f24f45>.
Baseline содержит retrievable commit и hashes полных inventory units;
standalone implementation lzop не копируется. В частности, настоящий header
версии до 0x0940 не имеет version-needed field. Fixed seven-byte skip Linux
заменён выбором реального layout; fixture не имитирует ошибочный kernel layout.

Nonidentity filter, split-block marker и extra-field contracts не реализованы
kernel wrapper. Их отказ сохраняет целостность, вместо выдачи filtered/shifted
payload. Kernel image loading, VFS creation и encoder не переносятся.
Metadata-only flags OS/charset/stdin/stdout/path/multipart/GMT не меняют block
layout и не отвергаются; это отдельно сверено с kernel и проверено fixture.

## Проверки и оставшийся scenario

CLI: 23 valid и 20 malformed вариантов, magic discovery из `.bin`, точные
payloads, stored/RLE/match blocks, несколько members, максимальный LZ4 block,
версии header, checksum combinations, padding, ошибки длины/decoder/checksum/tail,
Unicode/spaces и неизменность source. Отдельно frame→CPIO→Unicode-file через CLI
и ArchiveBackend; тест не выдаёт это за полную грамматику initramfs.
ABI проверяет source/output E_FAIL/E_ABORT, initial/final open cancellation,
активную отмену/zero-tail, failed-open state, repeat extraction, owned source,
непубликацию успешного size после late corruption и точный consumed boundary.

Изолированный C reference компилирует неизменённые kernel wrapper bodies
с userspace memory/flush services и теми же raw Linux decoders. Проверены
25 результатов, включая checksum-ignore policy и четыре ожидаемых ограничения
исходного lzop wrapper: старый header и иные checksum layouts. Для применимых
успехов сверены bytes; malformed out-of-source memory reads не запускаются.
Это compiled source reference, не boot/native-kernel acceptance.

2026-10-20: 12 focused suites PASS на macOS arm64 и Linux x64; после переносимой
правки test macro две новые suites повторно PASS на обеих ОС. Windows x64/MSVC
— 12/12 PASS. Исправление касалось preprocessor directive внутри QVERIFY,
production module на Windows собрался до него. Сохранилось существующее MSVC
warning C4098 в переиспользованном Linux `lz4defs.h`; оно не помешало сборке.
После финальной сверки metadata-only flags обе новые suites повторно PASS
на macOS/Linux/Windows с неизменными лимитами.
Проверены source hashes, SDK baseline и документационные ссылки/whitespace.
Full/slow, native boot и appearance acceptance не выполнялись. Установленный
пользовательский app bundle не заменялся; install rules проверены конфигурацией,
runtime deployment — реальным CLI/backend чтением из `Formats`.

Общий compressed/raw dispatcher, CPIO state на decompressor boundary и hard-link
state через эти переходы остаются открытым integrated initramfs scenario
в [едином реестре](filesystems-linux.md). Эта итерация завершает необходимые
LZ4/lzop readers и их consumers, а не всю цель.
