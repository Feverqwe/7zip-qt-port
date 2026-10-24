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
