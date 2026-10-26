# ZIP: актуализация по 7-Zip 26.05 — 2026-10-24

## Defined memory/arithmetic operations — 2026-10-26

ASan/UBSan validation of the C++23 foundation exposed three original optimized
operations with undefined C/C++ behavior. Complete affected functions, caller
contracts and their portable paths were compared with the SDK baseline:

- `GetMatchesSpecN_2` doubles a possibly negative relative tree offset using
  multiplication instead of signed left shift. The same pair address, match
  lengths, traversal and output sequence are retained; it remains C.
- Shrink's little-endian buffered bit load uses the shared alias-safe `GetUi32`
  primitive. The original four-byte lookahead guard, masks and decoder state
  transitions are unchanged.
- RAR5's scalar match-copy loads/stores use `GetUi32/64` and `SetUi32/64`.
  Copy order, chunk sizes, overlap behavior, padding and NEON/SSE paths retain
  the original implementation. The unRAR notice remains intact.

These complete original units now have adapted owners under `src/`; the existing
SDK manifest pins their original hashes and includes them in runtime overlays.
C++ formatting is accounted for separately from those functional substitutions.
Existing compression dictionary/limit, Shrink and native RAR tests reproduce
the pre-fix sanitizer failures. Performance acceptance is separate from these
correctness checks.

## Original refresh scope

После первоначального точечного обновления пользователь запросил
[полный переход на 26.05](upstream-26.05.md). Ниже сохранены scope и результаты
именно первоначальной ZIP-проверки; текущая общая baseline описана там.

Источник: официальный <https://github.com/ip7z/7zip.git>, release 26.05 от
2026-10-21, полный commit `b88c6bc09339de32f7d18bbab900686b244f42ba`.
Исходная ZIP baseline — 26.04, `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сравнение выполнено в отдельном временном Git checkout; sibling upstream
оставлен без изменений. Исходное рабочее дерево содержало несвязанные
изменения QCOW, упаковки, инструментов provenance и drag/drop; они сохранены.

## Полнота переноса

До runtime-проверок рассмотрены изменения 26.04 → 26.05 и текущие owners
`CPP/7zip/Archive/Zip/`: чтение, extraction, writing/update, типы и заголовки.
Эти units не изменились. Из ZIP codecs изменились только
`CPP/7zip/Compress/ShrinkDecoder.{cpp,h}`; crypto, Deflate, Deflate64, Implode,
PPMd, LZMA, XZ, Bzip2 и Zstandard остались на тех же исходных bytes.
Общие изменения `MyString.h` касаются записи пустого тела цикла, `MyVector.h`
добавляет неиспользуемый здесь API; обязательных новых зависимостей нет.

Оба Shrink units скопированы целиком, побайтно, в
`third_party/7zip/CPP/7zip/Compress/`. Перенесены `NShrink::CDecoder`,
`Code`, `SetFinishMode`, `GetInStreamProcessedSize`, исходные таблицы,
буферы и helper allocation class. Новые include dependencies — существующие
`C/CpuArch.h` и `Common/StreamUtils.h`; `Alloc`, COM interfaces и stream helpers
совместимы с текущей baseline и не изменились в 26.05. Notices сохранены;
лицензия LGPL-2.1-or-later, [исходная лицензия](../../third_party/7zip/License.txt).
Qt/platform adaptations и omissions внутри импортированных units отсутствуют.

Повторно сверены полный `CDecoder::Code`, обработка escape/разрядности,
очистка словаря, orphan/cyclic checks, размеры и padding, progress/cancellation,
а также callers `ZipHandler.cpp`: выбор method 1 и `SetFinishMode(true)`.
`sevenzip_zip` уже включает этот decoder; отдельно собранные официальные
`7z` и `7z.so` теперь получают exact revision overrides через существующий
runtime snapshot builder. Это устраняет расхождение между статическим ZIP
consumer и native runtime при частичном обновлении зависимости.

Полные новые commits и SHA256 оригинальных units с mapping на local owners
записаны в [baseline](../upstream-baseline.json): `overrides` и
`original_sha256`. Default revision и версия приложения/движка остаются 26.04;
изменения других форматов в 26.05 этой задачей не переносятся.
Повторная проверка и review следующей версии используют существующие
`scripts/upstream_baseline.py verify/review` по [процедуре](../upstream-update.md).

## Runtime-проверки и границы

macOS arm64: baseline verify PASS; оба импортированных файла и runtime source
provenance совпадают с оригинальными SHA256. Native module и самостоятельный
`7z` пересобраны существующим `prepare-runtime.py --codec-only` с сохранением
incremental outputs и compiler cache.
`sevenzip_desktop` (общий executable для 7zFM/7zG) также успешно собран.

Focused CTest: 6/6 PASS — `zip_shrink`, `native_zip_writer`,
`native_archive_test`, `archive_recovery_backend`, `upstream_baseline`,
`runtime_cache`. `zip_shrink` проверяет точные decoded bytes для literal,
dictionary reference, leaf clearing и переходов границ буферов; ошибки
padding, усечение, orphan reference и initial control; отмену через progress.
Каждый archive fixture также извлекается официальным runtime `7z`, с Unicode
и пробелами в именах, проверкой результата и неизменности source bytes.
Проверка nonzero padding обнаруживает случай, который старый decoder
не проверял. Fixtures синтетические, пользовательские архивы не используются.
Tooling regressions проверяют попадание selective exact update в runtime,
инвалидацию prepared cache и сохранение incremental outputs.

Изменение платформонезависимое; Linux/Windows не запускались. Полный gate,
slow suites и ручная UI-приёмка не требуются для выбранного scope и не запускались.
Временные fixtures автоматически удаляются; существующие build trees и кэши
сохранены. Полнота переноса двух source units и runtime scope указаны отдельно:
это не обновление всего 7-Zip до 26.05 и не исчерпывающая проверка всех ZIP features.
