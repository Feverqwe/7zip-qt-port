# ZIP: официальный компонент и независимые эталоны — 2026-10-24

После rebase на `master` (`c88700e1`) полное обновление 26.05, Shrink и runtime
исправления уже находятся в baseline; selective overrides не добавляются.
Ветка сохраняет выбор эталонов и дополнительные 10/13-bit Shrink regressions.
Ниже записан исходный scope переноса до интеграции общего обновления в `master`;
текущая общая baseline описана в [аудите 26.05](upstream-26.05.md).

Выбранный подход и границы дальнейшей актуализации:
[ZIP plan](../zip-refresh-plan.md). Поддержка всего набора возможностей APPNOTE
не заявляется. Эта итерация обновляет ZIP dependency closure до изменений
официального 26.05, сохраняя локальные адаптации общих декодеров.

## Источник и полнота переноса

Repository: <https://github.com/ip7z/7zip.git>.
Исходная baseline: `9128b80e3a471108678e2b3ec3c985861c8d7b0f` (26.04).
Предложенная ZIP revision: `b88c6bc09339de32f7d18bbab900686b244f42ba` (26.05).
Отдельная копия официального репозитория находится внутри task worktree;
соседний upstream checkout не менялся.

До реализации выполнен существующий `upstream_baseline.py review`, сверены
полный diff ZIP reader/writer/update и codec/crypto dependencies.
`CPP/7zip/Archive/Zip/` не изменился. Единственные изменённые ZIP codecs —
`CPP/7zip/Compress/ShrinkDecoder.cpp` и `.h`; изменения ZDecoder относятся к .Z.
Новые зависимости Shrink — существующие `C/CpuArch.h` и
`CPP/7zip/Common/StreamUtils.h`; Alloc, StreamUtils, MyCom и ICoder не менялись.
Изменения общего MyString — форма записи пустого цикла; MyVector добавляет
неиспользуемый новым decoder API. Их обновление этому переносу не требуется.

Оба Shrink units перенесены целиком и побайтно в
`third_party/7zip/CPP/7zip/Compress/`. Сохранены `NShrink::CDecoder`, его Code,
SetFinishMode/GetInStreamProcessedSize, таблицы и helper allocation class.
Прочитаны полный decoder, finish/padding checks, variable bit widths,
leaf clearing, orphan/cyclic bounds, buffered I/O, progress/cancellation;
caller `ZipHandler.cpp` уже выбирает method 1 и включает полный finish mode.
Qt/platform adaptations и omissions внутри импортированных units отсутствуют.
Лицензия — LGPL-2.1-or-later, исходные notices и
[License.txt](../../third_party/7zip/License.txt) сохранены.

Полные commits, SHA256 оригинальных units и mapping на local owners записаны
в `overrides`/`original_sha256` существующего
[manifest](../upstream-baseline.json). Остальные units остаются на 26.04;
версия движка не выдаётся за полное обновление 26.05.
Повторный verify/review выполняется существующим инструментом по
[процедуре](../upstream-update.md); исходные bytes остаются baseline независимо
от последующих локальных адаптаций.

Локальное дополнение snapshot builder подключает exact overrides к обоим
официальным native engine variants и их source provenance. Fingerprint runtime
теперь учитывает override revisions и bytes, чтобы тёплый cache не возвращал
старый decoder. Без этого частичное обновление работало бы только в статическом
ZIP consumer. Исходный handler, codecs и recovery не заменяются другой библиотекой.

Из прежней незакоммиченной работы main checkout выбраны только официальные
Shrink units и focused ZIP regression; общие обновления 26.05, QCOW и другие
незавершённые изменения не перенесены. Ни один общий decoder, ранее сверенный
с Linux, не заменён upstream-оригиналом поверх локальных исправлений.

## Проверки

Source manifest verify PASS: 417 exact, 267 adapted, 12 local owners,
две явно записанные ревизии. Повторная сверка импортированных units с Git blobs
и SHA256 выполнена существующим verify.

macOS arm64 / Qt 6.12: официальный native module и sidecar `7z` собраны
через `prepare-runtime.py --codec-only`; `sevenzip_desktop` также собран.
Сверка runtime provenance подтверждает оба новых Shrink SHA256 в engine snapshot.
Existing compiler cache сохранён; build outputs принадлежат task worktree.

Focused CTest 5/5 PASS: `zip_shrink`, `native_zip_writer`,
`archive_recovery_backend`, `upstream_baseline`, `runtime_cache`.
Shrink regression переиспользован из подготовленной работы main и дополнен
проверками увеличения разрядности до 10/13 бит. Он проверяет точные decoded
bytes, dictionary references/leaf clearing, границы буферов, неверный padding,
усечение, orphan reference, начальный control и отмену через progress.
Каждый archive fixture также извлекается официальным sidecar с Unicode/пробелами
и проверкой неизменности исходного ZIP. Nonzero padding обнаруживает ошибку,
которую прежний decoder не проверял. Пользовательские архивы не используются.
Tooling regressions проверяют результат partial source overlay, отклонение
повреждённого provenance и инвалидацию runtime cache с сохранением outputs.

Изменения платформонезависимы; Linux/Windows не запускались. Full gate,
slow suites, независимые библиотеки и rendered UI не запускались.
Temporary fixtures очищаются существующими helpers; compiler cache и incremental
build trees сохранены. Source completeness двух импортированных units
не означает исчерпывающей поддержки всех ZIP extensions или приёмки всех
архивных сценариев. Дополнительные функции требуют конкретного сценария
и отдельного сравнения с подходящей спецификацией/реализацией.
