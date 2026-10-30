# Создание ZIP со Zstandard — 2026-10-24

## Источники и границы

ZIP остаётся официальным handler 7-Zip 26.05:
<https://github.com/ip7z/7zip.git>, commit
`b88c6bc09339de32f7d18bbab900686b244f42ba`.
Перед изменением сверены полные `ZipAddCommon`, `ZipHandlerOut`, `ZipUpdate`,
`ZipHeader`, method dispatch/registry, crypto wrappers, finish/results,
multi-file workers и Qt method/settings consumers. В официальном checkout нет
реализации Zstd encoder; optional branches не предоставляют рабочий encoder.

Алгоритм сжатия — неизменённый официальный libzstd 1.5.7:
<https://github.com/facebook/zstd.git>, commit
`f8745da6ff1ad1e7bab384bd1f9d742439278e99`.
Полные C/header units common/compression, публичные headers и лицензии
перенесены в `src/zstd/` с сохранением внутреннего layout и языка C.
Это тот же набор common/compress, который выбирает официальный CMake при
отключённых decompression/dictBuilder/deprecated. Декодер, CLI и dictionary
builder не импортируются. Внутренние Zstd workers выключены; существующий
ZIP worker pool параллелит файлы. BSD license выбран из исходной dual license.
Полные оригинальные hashes и mapping — в
[manifest libzstd](../zstd-encoder-baseline.json).

COM-адаптер и ZIP dispatch сверены с <https://github.com/mcmilk/7-Zip-zstd.git>,
commit `f7331b3fa14455d78a6e6a91bdd9571c999bb081`:
`Compress/ZstdEncoder.{cpp,h}`, `Archive/Zip/ZipAddCommon.cpp`, `ZipHeader.h`.
[Manifest адаптера](../zip-zstd-adapter-baseline.json) фиксирует полные
оригинальные units, SHA256 и local owners. LGPL notices сохранены.
Перенесены `NZSTD::CEncoder::Code`, streaming read/compress/write loop,
progress и HRESULT error mapping; ZIP method-93 dispatch и convention extract
version 20. Fork decoder/registration, 7z coder property serialization,
fork-only advanced options и его дополнительные compression methods не перенесены.

Адаптер сохраняет официальный ICoder ABI и поддерживает стандартные level,
thread-count и reduce-size properties плюс optional expected-size hint.
Неизвестные properties отвергаются. Каждая повторная операция сбрасывает
Zstd session/parameters; input cursor сохраняется между output flushes.
Проверка progress перед чтением позволяет отменять и короткие операции.
Это адаптации для ZIP worker reuse, ошибок/отмены и ABI официального engine;
сам алгоритм libzstd не изменён. Expected-size используется как hint,
а не как обещание точного размера источника.

`ZipAddCommon.cpp` и `ZipHeader.h` имеют по одному владельцу в `src/CPP/`;
полные original hashes и runtime overlay записаны в основном
[manifest 7-Zip](../upstream-baseline.json). Сохранены Store fallback,
ZIP64, empty entries, CRC, AES/ZipCrypto, paths, comments, updates и cleanup.
Encoder и libzstd включены в статический consumer и оба official native
runtime variants. Snapshot builder сохраняет C/C++ язык дополнительных units
и incremental dependency rebuilds; original-only SFX не получает новый encoder.
Лицензии устанавливаются на всех трёх ОС.

На Windows неизменённые libzstd C units собираются с обычным MSVC `/W3`
вместо `/Wall`, который в 7-Zip предполагает собственные compiler pragmas.
`/WX` сохранён; encoder adapter остаётся под строгими исходными flags.
Порядок StdAfx include сохранён перед остальными headers.

По дополнительному вопросу пользователя проверены кандидаты на повторное
использование библиотеки: SDK Zstd decoder обслуживает ZIP, standalone .zst,
QCOW, SquashFS, EROFS и initramfs. Замена его алгоритма на libzstd decoder
потребует отдельного сравнения partial reads, consumed boundaries, frame metadata,
лимитов памяти и failure/cancellation/recovery поведения всех этих consumers.
В этой итерации decoder сохранён; равенство возможностей после такой замены
не заявляется. ZIP container/crypto/update остаются вне назначения libzstd.

## Интерфейс и совместимость

ZIP получил явный метод ZSTD; Deflate остаётся исходным default.
Другие форматы не получают этот метод автоматически. Прежние уровни ZIP
передаются libzstd; настройки словаря и длины слова для нового метода отключены.
Оценки памяти берутся из libzstd; автоограничение ZIP workers сохраняется.
Это расширение относительно Windows 7zFM, а не заявление о UI parity.
Файл остаётся ZIP, записывается method 93 из
[PKWARE APPNOTE](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT).
Распаковщику требуется поддержка этого метода.

## Повторяемая source-проверка и обновление

Использовать отдельные checkout соответствующих repositories с указанными
commits; соседний checkout проекта оставлять read-only.

```sh
python3 scripts/upstream_baseline.py verify --upstream <7zip-checkout>
python3 scripts/upstream_baseline.py reference-verify --manifest docs/zstd-encoder-baseline.json --upstream <zstd-checkout>
python3 scripts/upstream_baseline.py reference-verify --manifest docs/zip-zstd-adapter-baseline.json --upstream <7zip-zstd-checkout>
python3 scripts/upstream_baseline.py reference-review --manifest docs/zstd-encoder-baseline.json --upstream <zstd-checkout> --new-revision <full-commit> --output <report>
python3 scripts/upstream_baseline.py reference-review --manifest docs/zip-zstd-adapter-baseline.json --upstream <7zip-zstd-checkout> --new-revision <full-commit> --output <report>
```

Reference review сопоставляет original baseline, proposed revision и local
owners. Для libzstd local owners должны оставаться побайтными originals;
для adapter review отдельно сохранять перечисленные omissions/fixes.
Не обновлять manifest до проверки зависимостей и runtime consumers.

## Проверки

Source verify: 415 exact, 269 adapted, 14 local 7-Zip owners; reference verify
подтвердил 58 libzstd и 5 adapter units. Нативный диагностический архив успешно
создан методом 93 и проверен official sidecar. Все imported libzstd owners
также побайтно сверены с original SHA256. Повторный нормализованный diff
ZIP writer/header отделил форматирование от добавленных Zstd branches;
другие branches не изменены. Runtime provenance включает текущий encoder
и полный набор libzstd C/header dependencies.

macOS arm64 / Qt 6.12: 8/8 focused suites PASS — `zip_zstd`,
`native_zip_writer`, `codec_creation_ui`, `compression_method_state`,
`compression_memory`, `archive_recovery_backend`, `upstream_baseline`,
`runtime_cache`. После узких include/build-boundary исправлений повторены
`zip_zstd`/`upstream_baseline` (2/2), затем расширенный `zip_zstd` (1/1).
Первые native creation отказы внутри sandbox затрагивали и все прежние
форматы; вне sandbox эти же операции проходят. Новый тест исправлен для
штатного порядка open-before-extract; продуктовый gate чтения сохранён.

Linux x64 / Qt 6.12: те же 8/8 PASS,
batch `20261024T084600Z-57a9ccf6`; после добавления static-writer и mixed
Deflate/Zstd update cases `zip_zstd` повторно PASS,
batch `20261024T090142Z-b32242c7`.

Windows 11 x64 / MSVC / Qt 6.12: те же 8/8 PASS,
batch `20261024T085419Z-34dc965f`; расширенный `zip_zstd` повторно PASS,
batch `20261024T090242Z-b1924568`. Первые сборки выявили порядок StdAfx include
и несовместимость `/Wall` с unchanged external C. Исправления ограничены
адаптером и compiler boundary; алгоритм libzstd не менялся. Codegen diagnostics
не отключались для всего engine, `/WX` сохранён.

`zip_zstd` проверяет реальные native create/update/list/open/extract,
serial reuse и parallel ZIP workers, AES-256/ZipCrypto и неверный пароль,
Unicode/пробелы, пустые и incompressible entries, Store fallback, сохранение
нетронутых записей и исходников, отмену обновления без изменения архива.
Статический ZIP writer также создаёт архивы method 93 с точным round trip.
Отдельный coder regression проверяет source/output E_FAIL, mid-stream E_ABORT
и успешное повторное использование после ошибок, включая пустой frame.
Mixed-archive case обновляет Deflate ZIP с записью новых Zstd entries,
сохраняя нетронутые Deflate records. Fixtures синтетические и автоматически
удаляются; пользовательские архивы не используются.

UI: EN/RU, normal 760×720 и compact 640×480, текущие widget JSON подтверждают
видимость метода, выключенные dictionary/order и доступный footer. Проверен
реальный результат UI selection — ZIP method 93, сохранён Deflate default,
ZSTD не появляется у 7z. Проверки на runners — offscreen, не rendered Windows
pixel parity. Новых UI strings нет.

`sevenzip_desktop` успешно собран на всех трёх ОС; remote app builds —
batch `20261024T090604Z-592f1365`. Official native module и независимый sidecar
также собраны с новым encoder. Incremental outputs и compiler caches сохранены.
Системный libarchive/tar текущей macOS отказал method 93 как unsupported;
это установленная граница совместимости, а не ошибка созданного архива.
Полный gate, slow suites и release/package acceptance не запускались:
выбранные focused suites покрывают изменённые consumers и build boundary.

## Сравнение decoder SDK и libzstd — 2026-10-24

Проверено после rebase, Desktop `6e97d6d5`, официальный SDK 26.05
`b88c6bc09339de32f7d18bbab900686b244f42ba` и libzstd 1.5.7
`f8745da6ff1ad1e7bab384bd1f9d742439278e99`.
Функциональный код приложения не изменён. Сопоставлены полные Code/Read/
Prepare/GetFinishResult/ReadUnused bodies SDK, C state/result API, все прямые
consumers, публичные libzstd buffer/stream APIs, reset, frame boundaries,
header limits и errors. Прочитаны соответствующие decoder branches libzstd.
Семь полных decoder C/header units закреплены как `reference_only` в
[существующем manifest](../zstd-encoder-baseline.json), с SHA256 оригиналов;
в product source/dependencies они не импортированы. Их verify/review используют
те же команды reference tooling, что и encoder.

### Результат сравнения

**Прямая замена не сохраняет всё текущее поведение.** Для исправных inputs
совместимость хорошая; для повреждённых входов уже измерены различия partial
output. Для замены алгоритма нужны сохранённые adapters и отдельная приёмка.

| Consumer | Текущий контракт | Что потребуется от libzstd adapter |
| --- | --- | --- |
| ZIP | Code, полный finish mode, точные sizes/consumed bytes, HRESULT и checksum result | Потоковый API, source/output/progress callbacks, finish и error mapping; сохранить recovery output |
| Standalone .zst | Open/OpenSeq, concatenated/skippable frames, metadata, pull Read и ReadUnused, partial result | Сохранить форматный parser и агрегированную статистику; новый decoder не заменяет handler |
| QCOW | Один frame, точный размер cluster, sector padding после frame, отдельное время жизни stream | Найти границу первого frame, декодировать только его, проверить полный размер; не подавать padding как следующий frame |
| SquashFS SDK/GPL | Bounded blocks; варианты различаются остановкой на frame и допустимостью хвоста | Явные exact-input/single-frame режимы, сохранённые variant owners и output bounds |
| EROFS | Полный decode либо prefix read; window не больше filesystem dictionary, bounded output | Prefix streaming mode и отдельная проверка frame window; single-pass в короткий output не заменяет partial mode |
| Initramfs | Один frame и точное consumed для следующей записи, cancellation, ограничение окна | Остановиться на frame boundary, вернуть остаток входа, сохранить progress/I/O/limit checks |

SDK `CZstdDecInfo` также даёт количество блоков/data/skippable frames, суммарные
размеры, checksum и максимальные declared/allocated windows. libzstd header API
даёт сведения отдельного frame; готового эквивалента всей aggregate statistics
и `CZstdDecResInfo` нет. Существующий upstream parser можно сохранить; не нужно
переписывать ZIP или вводить собственные алгоритмы декодирования.

В `.zst` различаются CRCError, UnexpectedEnd, IsNotArc, DataAfterEnd и
UnsupportedMethod. Коды libzstd позволяют построить mapping, но не являются
готовыми HRESULT или `NOperationResult`. Пустой input дополнительно должен
отличаться от корректного пустого frame. После ошибки libzstd требует reset
decoder state; callbacks и отмена принадлежат адаптеру. Эти механизмы не
исчезнут при замене алгоритма библиотекой.

### Окна и память

Активный предел SDK — 2 GiB, в том числе на 64-bit. Ветка с `MAX_WINDOW_SIZE_LOG=41`
отключена условием `&& 0 == 1`; из неё нельзя выводить поддержку 2 TiB.
libzstd streaming по умолчанию ограничивает окно 128 MiB; на 64-bit параметр
`ZSTD_d_windowLogMax=31` позволяет сохранить предел SDK. Синтетические frames
с 40 bytes payload и объявленными окнами 256 MiB, 1 GiB и 2 GiB проходят SDK
и настроенный stream decoder, но не default stream decoder. Большие allocations
не создавались: известный маленький content size ограничивает actual buffer.
Frame с window log 32 отвергается обоими.

Single-pass и streaming нельзя считать взаимозаменяемыми для limits: libzstd
имеет shortcut для полного known-size frame, который выполняется до streaming
max-window gate. EROFS dictionary и initramfs policy нужно проверять явно через
header API независимо от выбранного пути. На 32-bit SDK source допускает window
log 31, а libzstd ограничивает его 30; runtime 32-bit не проверялся.

Дополнительный 53-byte frame с declared window 2.25 GiB и actual output 40 bytes
подтвердил shortcut: SDK его отвергает, libzstd single-pass принимает; stream
с max log 31 и input по 13 bytes отвергает, а stream с полным input принимает.
Это не большая allocation и не повреждение памяти, но показывает, почему
лимит формата/consumer нельзя обеспечивать одним streaming parameter.

### Поведенческий эксперимент

На macOS arm64 собран diagnostic probe из неизменённых official common/
decompress C units через существующий compiler cache (`ZSTD_DISABLE_ASM`).
Он вызывает только libzstd public APIs и не содержит decoder implementation.
Использованы существующие `tests/formats/zstd/support/zstd_fixture.py`: 22 valid и 14 malformed cases,
плюс empty input, size overflow, пять window boundaries и frame с sector padding —
44 inputs. Для каждого сравнены SDK sidecar extraction, single-pass,
default/max-window stream и остановка после первого frame. Вход/выход —
disposable synthetic fixtures, пользовательских архивов нет.

- Все 22 valid cases дают exact одинаковые bytes в SDK и настроенном stream
  libzstd: raw/RLE, compressed/Huffman, checksum, unknown sizes, zero dictionary
  IDs, concatenated/empty frames и все skippable IDs.
- Все 14 malformed cases отклонены SDK и настроенным stream decoder. Это
  совпадение acceptance, а не равенство diagnostics или recovery data.
- В `body-short` оба stream пути дают 528 bytes и неполный результат; single-pass
  возвращает ошибку без публичного размера пригодного partial output.
- В `checksum-bad` SDK отдаёт 297000 bytes с CRC error; обычный libzstd stream
  с 13-byte input / 64 KiB output chunks отдаёт 262144 bytes с checksum error.
  При input по 1 byte libzstd успевает вернуть все 297000 bytes до ошибки;
  при 1 KiB output и 13-byte input — 296960. Количество доступных bytes зависит
  от границ вызова. Это конкретный риск потери recovery output при прямой замене.
- `late-crc` также сохраняет меньше данных в обычном libzstd stream;
  при несогласованном content size (`size-large`) partial output отличается.
  Нельзя объявлять этот output проверенным или превращать ошибку в успех.
- Для пустого input `ZSTD_decompressDCtx` возвращает 0 как успешный размер,
  тогда как SDK не открывает его как Zstd archive. Нужен прежний no-archive gate.
- После первого frame libzstd возвращает consumed boundary до padding;
  QCOW/initramfs могут использовать этот режим. При декодировании всего input
  тот же padding является ошибкой, как хвост standalone stream.

Текущие `zstd`/`zstd_stream` suites — 2/2 PASS на macOS; они подтверждают
исходные SDK metadata, pull/seq, source/output failure, cancellation и ZIP
consumer contracts. Альтернативного product adapter пока нет: эти suites не
доказывают его соответствие. Диагностические `results.json`, `recovery.json`,
probe source и воспроизводящий driver сохранены в ignored
`build/zstd-decoder-comparison/` (дополнительный window case —
`window-shortcut.json`). Производительность, peak RAM, native 32-bit,
Linux/Windows differential runtime и реальные damaged media не проверялись.

### Рекомендация

Исторический вывод сравнения ниже относится к прямой замене через frame/stream
API. Его продолжение с сохранением upstream framing — в разделе
[общего encoder/decoder](#общий-libzstd-encoderdecoder--2026-10-24).

Не заменять SDK decoder одним вызовом `ZSTD_decompress`/`ZSTD_decompressStream`.
Для законченной миграции сначала нужен compatibility adapter с доказанными
partial-output, frame/consumed и diagnostic contracts. Наиболее узкий первый
scope для prototype — bounded QCOW/SquashFS blocks; EROFS partial и standalone/
ZIP recovery сложнее. Пока SDK остаётся для других consumers, частичная замена
не удалит его алгоритм и временно добавит второй decoder в сборку.

Это выбор источника сопровождения, а не гарантированное уменьшение кода:
текущие SDK C/header units — около 4240 lines, official libzstd decoder C/headers —
около 7164 без ASM. Общие libzstd units уже есть из encoder; величина adapter
и итоговый binary footprint не измерялись. Дополнительные dictionaries/legacy
modes библиотеки не становятся возможностями приложения без отдельной интеграции.

## Общий libzstd encoder/decoder — 2026-10-24

Feature branch: `feature/zstd-decoder-libzstd`, baseline Desktop `67d50fbc`.
Замена сохраняет upstream `C/ZstdDec.c` framing/state/result API, raw/RLE
partial reads, window/output bounds, XXH64, metadata, skipped/concatenated
frames и consumed/unused bytes. Его единственный owner перемещён в
`src/codecs/zstd/ZstdDec.c`. Полный оригинал SDK 26.05
`b88c6bc09339de32f7d18bbab900686b244f42ba`, repository
<https://github.com/ip7z/7zip.git>, закреплён SHA256 в существующем
[SDK manifest](../upstream-baseline.json). C++ Code/Read и все consumers
остаются upstream adapters. Самостоятельные SDK Huffman/FSE/LZ bodies,
их tables, SIMD helpers и literals allocation удалены из сборки и source tree.

Официальный `ZSTD_decompressBlock` выполняет compressed-block decode;
`ZSTD_insertBlock` сообщает raw/RLE history. Контекст использует переданный
SDK allocator; reset выполняется на каждом frame. Upstream cyclic window и
caller-buffer modes сохранены. Алгоритмов декодирования в glue нет.
Поблочный API помечен libzstd как deprecated и относится к static API;
его использование ограничено version-pinned адаптацией. При обновлении
библиотеки этот boundary нужно сверять; warning отключён только в адаптере
через предусмотренный библиотекой macro, без изменения library sources.

Семь decoder inventory units libzstd 1.5.7
`f8745da6ff1ad1e7bab384bd1f9d742439278e99`, repository
<https://github.com/facebook/zstd.git>, перенесены из `reference_only` в
unchanged dependencies `src/zstd/lib/decompress/`. Их оригинальные SHA256
и mapping — в том же [libzstd manifest](../zstd-encoder-baseline.json).
Common/compress closure остаётся общей; CMake static library используется ZIP,
EROFS, SquashFS и initramfs. Native runtime overlay включает тот же decoder,
поэтому standalone `.zst`, ZIP, QCOW и SDK SquashFS используют его также.
ASM не импортируется; `ZSTD_DISABLE_ASM` задан одинаково для CMake и обоих
runtime builders. CLI, dictionary builder и legacy decoders не импортируются.
Сохранены notices: SDK frame parser BSD-3-Clause, libzstd BSD или GPL;
используется BSD, SDK types/COM adapters сохраняют прежние лицензии.

Прямая замена `ZSTD_decompressStream` не выбрана: функция возвращает ошибку
до публикации последней output position. Bufferless frame API отдельно
проверяет checksum и сохраняет CRC output, но теряет reported output при
несоответствии frame content size; он также сам по себе не сохраняет
truncated raw prefix. Сохранённый SDK parser решает обе границы, а compressed
block API возвращает размер до проверки frame size/checksum в SDK parser.
Ошибка остаётся ошибкой, выданные данные не объявляются проверенными.

Повторяемая source verification/review использует команды выше; SDK:
`python3 scripts/upstream_baseline.py verify --upstream <sdk-checkout>` и
`review --upstream <sdk-checkout> --new-revision <commit> --path src/codecs/zstd/ZstdDec.c --output <report>`.
Новый snapshot сравнивается с полным original и локальным parser, не заменяет
частичную адаптацию полным SDK module.
Текущее отличие parser от исходного SDK воспроизводится отдельно:
`git -C <sdk-checkout> show b88c6bc09339de32f7d18bbab900686b244f42ba:C/ZstdDec.c | diff -u --label SDK-original - src/codecs/zstd/ZstdDec.c`.
Такой diff сохранён в `build/zstd-recovery/source-review.md`: кроме удаления
SDK compressed-block implementation, context/allocator glue и raw/RLE history
registration frame parser не переписывался. Разница форматирования отдельно
ограничена удалением trailing whitespace из 12 пустых строк перенесённого SDK
parser; source algorithm и branches от этого не менялись. Неизменённые libzstd
units не форматировались.

### Проверка сохранения данных

macOS arm64 diagnostic сравнил SDK и адаптер на 36 valid/malformed inputs,
input chunks 1/13/65536, caller-buffer и cyclic-buffer modes: 216/216 совпали
по output bytes, result, unfinished flag, blocks и frames. Дополнительно
24 мутации compressed payload совпали по тем же результатам. Смешанный
raw/RLE/compressed поток 2.77 MB многократно оборачивает 128 KiB window и
совпал с оригиналом в обоих modes. Это focused evidence, не доказательство
эквивалентности для всех повреждений. Diagnostic sources и результаты —
`build/zstd-recovery/`; disposable inputs/outputs автоматически удаляются.

Регрессии проверяют error exit вместе с доступными bytes для checksum-bad,
late-crc, truncated raw body и несогласованного content size; mixed-history
fixture проверяет использование raw/RLE history и window wrapping.

Дополнительный official-encoder probe создал unknown-size поток 2.8 MB
с 128 KiB window и повторениями на расстоянии 70000 bytes. SDK и адаптер
совпали по всем bytes в caller/cyclic modes как при верной checksum, так и
при повреждённой: весь output сохранён, ошибочный результат остаётся CRC.

macOS arm64 / Qt 6.12: приложение собрано, 14/14 focused suites PASS:
`zstd`, `zstd_stream`, `zip_zstd`, `native_zip_writer`, `erofs`, `erofs_reader`,
`erofs_module`, `squashfs`, `squashfs_stream`, `initramfs`, `archive_layer_chain`,
`upstream_baseline`, `upstream_inventory`, `runtime_cache` (61.57s).
Native ZIP требует unsandboxed запуска из-за platform service restrictions.
Первый isolated-worktree запуск обнаружил недостающую MBR runtime dependency
у layer-chain test; она добавлена рядом с существующей Ext dependency.
Последний LastTest.log содержит успешный повтор, устаревшие failure markers
и failed verify receipt удалены. Полные SDK original hashes проверены:
414 exact / 270 adapted / 15 local owners; все 65 libzstd owners побайтно
совпадают с закреплёнными оригиналами. Sibling upstream остаётся read-only;
для проверки/сборки использован отдельный clone точной SDK revision.

До local/remote validation у этой ветки не было старых guest build/fixtures.
Свободное место перед remote build: Linux около 52 GiB, Windows 31.6 GiB;
на macOS 184 GiB. Disposable inputs/outputs создаются с auto-cleanup,
incremental builds и общий compiler cache сохранены. Полный gate и slow suites
не запрашивались и не запускались; UI не менялся.

Linux x64 / GCC / Qt 6.12, runner batch `20261024T103202Z-3f292ca1`:
13/14 suites PASS. Широкая `archive_layer_chain` (54 data rows разных методов)
достигла своего 60-second timeout, поэтому её полная приёмка не заявляется.
Отдельно на том же собранном snapshot выполнены все 12 Zstandard rows:
обычный и extended QCOW × success/cancel/source-failure/read-cancel/automatic/tar;
12/12 PASS (Qt totals 14 с init/cleanup, 12.77s). Лог —
`build/zstd-recovery/linux-zstd-layer-acceptance.log`. Синтетический QCOW,
оставшийся после timeout в task-owned `runner-tmp`, удалён после завершения
прогона; free space после сборки/cleanup — 51 GiB. Runtime/build/cache сохранены.

Windows 11 x64 / MSVC / Qt 6.12: native module/sidecar и все выбранные CMake
consumers собраны. В первом batch `20261024T103202Z-3f292ca1` 10/14 suites PASS;
layer-chain и общий baseline-tool suite достигли 60/120-second timeouts.
Все 12 Zstandard layer-chain rows отдельно PASS (Qt totals 14, 22.97s), лог —
`build/zstd-recovery/windows-zstd-layer-acceptance.log`. EROFS backend discovery
сначала достиг deadline, без изменения product code повторный `erofs_module`
полностью PASS в batch `20261024T105234Z-9f840592`.

Повтор выявил и устранил независимый дефект runtime-cache test isolation:
mock общего `subprocess.run` перехватывал lazy Windows platform probe Python.
Тестовый fixture теперь получает реальные system/machine до builder mock и
фиксирует эти platform probes на время теста; production fingerprint не менялся.
Повтор `runtime_cache` PASS на Windows в том же batch, на Linux —
`20261024T105302Z-afe8a4cf`, на macOS — 0.38s. Это исправление существующей
поведенческой проверки, новых configuration/source-text assertions нет.

Итого Windows: 12 focused suites приняты, плюс 12 Zstandard layer-chain rows;
полные широкие `upstream_baseline` и `archive_layer_chain` остаются с timeout.
Временные fixtures завершённых failed tests в task-owned `runner-tmp` удалены
после проверки отсутствия активных процессов; свободно 28.3 GiB. Build trees,
compiler caches, текущие логи и unresolved timeout evidence сохранены.
После whitespace-only очистки перенесённого parser на macOS повторно собран
native runtime через compiler cache (0.77s); функциональный код не менялся.
