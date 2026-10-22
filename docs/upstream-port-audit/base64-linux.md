# Base64: alphabets, canonical tails и handler lifecycle

## Итоговая read-side сверка — 2026-10-23

Read-side source audit Base64 закрыт для archive projection ниже на закреплённой
Linux revision. Повторно сравнены полные `base64_decode`, alphabet tables и
tail/padding branches с текущими `Base64Code`, probe, in-place decoder,
Open/Extract и их callers. Все шесть reference hashes manifest совпали.

Все три alphabets, padded/unpadded groups, canonical tail bits и error paths
подключены к исходному SDK reader. Caller-selected kernel variant заменён
последовательным определением совместимого alphabet из archive input.
Whitespace envelope, extension policy, cached archive operation и отдельная
DMG/XML overload остаются явно описанными adapters/границами. Kernel encoder
и host fscrypt/Ceph services не входят в read-only archive contract.

Новых source-visible read gaps не обнаружено; код не менялся. Прежние CLI/ABI
и compiled-reference checks приведены ниже. После изменения общего archive
discovery Base64 CLI обоих engines повторно прошёл macOS validation `fb09caf3`;
это не расширяет перечисленные ниже платформенные/runtime свидетельства.

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/Base64Handler.cpp` скопирован в `src/` с исходными
путём, symbols, table, decoding loop, buffer-growth strategy и notices.
[Manifest](../upstream-baseline.json) включает unit в overlays обоих engines.
До изменений прочитан весь handler, registration/OpenArchive selection,
stream/progress helpers и вызов отдельной Base64ToBin overload из DmgHandler.
После изменений повторно проверен functional diff с форматированным оригиналом.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
Полностью сверены `lib/base64.c`, header и semantic KUnit vectors; подтверждены
реальные read callers: fscrypt no-key names используют URL-safe alphabet,
Ceph encrypted names/symlinks — IMAP. Это фактический kernel decoder формата,
а не совпадение с userspace tool. [Source hashes](../base64-linux-baseline.json)
закрепляют reader и соответствующие callers.

## Актуализация

Linux поддерживает standard, URL-safe и IMAP alphabets, с padding и без него.
Исходный archive handler понимал только standard, а последние две/три unpadded
цифры считал unexpected end. Теперь он читает все три alphabets и принимает
канонический unpadded tail. Смешение несовместимых alphabets отвергается;
совпадающие символы имеют одинаковое значение и не требуют нового UI option.

Исходный алгоритм 7-Zip сохраняется. Добавлен только narrow alphabet-selection
adapter и проверки конечных bits/EOF в существующих ветвях. Usable SDK decoder
уже существует, поэтому Linux GPL body не заменяет его. У SDK нет variant
selection для этого handler; kernel API выбирает вариант аргументом caller,
а archive interface требует определить совместимый alphabet из самого input.
Padding и unused tail bits проверяются по Linux: ненулевые четыре/два bits
не должны давать успешную операцию. Мусор после padding за пределами initial
probe также теперь даёт DataError.

Open не разыменовывает optional null callback, начинает новый input с начала,
проверяет отмену и очищает partial state после failure/exception. Пустой input
и допустимая text whitespace envelope дают пустой результат. Закрытый handler
отвергает Extract. Сохраняются исходные cached extraction, output HRESULT и
final progress/cancellation.

## Границы

Сохраняются extension/explicit-selection policy и исходный text-probe heuristic
7-Zip. Headerless Base64 не сканируется произвольно внутри binary files.
У нулевого input нет signature: общий engine opener отклоняет его automatic
selection, но явный выбор Base64 и прямой ABI Open читают его. Это отдельно
проверяется, без изменения общего OpenArchive для остальных форматов.

Kernel decoder принимает raw alphabet bytes; line folding/whitespace — archive
адаптация исходного 7-Zip. Reference harness удаляет только поддерживаемые
archive whitespace перед вызовом kernel decoder. IMAP здесь означает alphabet,
а не полный modified UTF-7 mailbox protocol. Никаких host fscrypt keys или
Ceph services не подключается.

Отдельная string overload Base64ToBin для XML внутри DMG сохранена без
функциональных изменений: новый alphabet adapter применяется только к archive
probe и in-place archive decoder. Алгоритм XML/DMG этим не переопределяется.

## Validation

25 valid fixtures покрывают все alphabets, padding/no-padding и три remainder
lengths, RFC short vectors, empty/whitespace, folded lines и input больше buffer
growth boundary. CLI обоих shipped engines проверяет extension recognition
для непустых `.b64`, explicit empty input, exact bytes, test command и source
immutability. 14 malformed fixtures покрывают incomplete groups, noncanonical
tail bits, invalid padding/bytes, смешанные alphabets, late bad bits и garbage
после padding. Ожидается normal exit code 2.

ABI проверяет null callback Open, ненулевую исходную позицию, освобождение
source после Open, metadata/bytes, source/output E_FAIL/E_ABORT, начальную и
активную отмену Open, extraction cancellation, cleanup/reuse и direct-operation
DataError для late malformed tails. ArchiveBackend извлекает IMAP unpadded
entry из Unicode/spaces `.b64` path, используя опубликованное listing name.
Source-visible null-callback dereference не запускался до исправления ради
демонстрации уже установленного crash.

Userspace C harness компилирует неизменённый pinned Linux `lib/base64.c` с
минимальными header/I/O adapters. 25 positives дали точные bytes в выбранном
alphabet; все 14 malformed inputs отвергнуты каждым из трёх вариантов —
67 ожидаемых исходов. Это compiled decoder reference, не kernel/VFS acceptance.
Disposable fixtures/output удаляются; существующие builds и ccache сохранены.

Focused suites `base64`, `base64_stream`, `zstd`, `zstd_stream`, `xz`,
`xz_stream`, `native_folder_opening`, `codec_creation`, `runtime_cache`,
`runtime_package`, `upstream_baseline`, `upstream_inventory`: macOS arm64
12/12 PASS (6,08 с), Linux x64 12/12 PASS (7,05 с), Windows x64/MSVC
12/12 PASS (24,43 с); remote receipt `20261020T100552Z-4a5731b1`.
После уточнения имён alphabet masks и probe scope без изменения поведения
две Base64 suites повторно PASS на всех трёх ОС, remote receipt
`20261020T100801Z-22594c0b`. Baseline verification: 418 exact, 234 adapted,
11 local paths, одна revision. Full/slow gate и UI appearance не запускались.
