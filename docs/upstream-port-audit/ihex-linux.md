# Intel HEX: ASCII records и IQS5xx profile

## Итоговая read-side сверка — 2026-10-23

Read-side source audit ASCII Intel HEX закрыт на закреплённом Linux commit
для описанной ниже archive projection. Повторно сопоставлены полные
`atohx`/`parse_hex_line`, `iqs5xx_fw_file_parse`, их address/checksum/type/EOF
branches и текущие SDK probe/record assembly, IQS profile и owned streams.
Все пять reference hashes manifest совпали с текущим snapshot.

Generic reader сохраняет extended/start records и полный 32-bit address span;
явный IQS profile передаёт vendor map, overlap, separators и settings exception
без автоматического ослабления generic checksum policy. Более строгий отказ
malformed input и отличие от отдельного binary ihex_binrec объяснены ниже.
Firmware acquisition/upload, I2C/USB и device update/verify не относятся к
архивному чтению и не переносились. Нового пропущенного read feature не найдено.

Эта повторная сверка не меняет production code и не добавляет runtime acceptance.
Применимая CLI/ABI/reference evidence и ограничения платформ указаны ниже.

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/IhexHandler.cpp` скопирован в `src/` по исходному пути.
Прочитаны все bodies и типы, OpenArchive probe/extension/property callers,
buffer exception handling, dynamic buffer и progress/extraction dependencies.
Сохранены исходные record/nibble parser, CBlock assembly, properties и extraction
loop. [Manifest](../upstream-baseline.json) включает unit в оба engines.
Functional diff повторно сверяется при одинаковом форматировании.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
`include/linux/ihex.h` читает отдельное binary representation с big-endian
address/length и alignment; это не ASCII Intel HEX проекта. Однако в kernel
есть и настоящие ASCII readers: `drivers/media/usb/as102/as102_fw.c`
(atohx/parse_hex_line и upload caller) и
`drivers/input/touchscreen/iqs5xx.c` (fw_file_parse, map initialization и caller).
Прочитаны полные selected bodies, constants/types, firmware acquisition и граница
передачи данных устройству; сверены hex_to_bin/hex2bin из `lib/hexdump.c`.
[Source hashes](../ihex-linux-baseline.json) закрепляют inventory units.

В production переиспользуется исходный C++ parser SDK; Linux C bodies не
копируются в C++ и не линкуются с engine. Для IQS5xx в SDK нет готового profile;
узкая адаптация существующего assembly loop задаёт его map/range/checksum policy,
сохраняя общий reader. Hardware upload, I2C/USB и request_firmware не переносились.

## Исправления и актуализация

Probe больше не сравнивает текстовые address digits с binary NUL. Неиспользуемые
address fields extended/start records не ограничивают parsing; AS102 перезаписывает
их перед следующими data records. Linear/segmented/start records теперь проходят
automatic discovery. Empty data records допускаются внутри полного stream с EOF;
короткий неподтверждённый prefix по-прежнему не доказывает формат.

Убрано ограничение 16 separators, поддерживаются CR/LF/space/tab. EOF address
не влияет на file data, включая используемый IQS5xx 0xFFFF. Сохранены generic
checksums, проверки record lengths/types и SDK handling start-address metadata.
Проверяется полная 32-bit data address span: последний byte по 0xFFFFFFFF допустим,
переход за адресное пространство не становится wrapped file mapping.

Open читает от начала, сообщает initial/final и bounded-progress callbacks,
включая длинные separators, сохраняет source HRESULT/CInBufferException и очищает
partial blocks после error/cancellation. Diagnostic error flags сохраняются для
неуспешного format parse. GetProperty/GetStream/selected Extract проверяют indices.
Новый GetStream переиспользует существующий owned-buffer helper SDK: cached data
работают после Close/Reopen и освобождения handler/source, cursors независимы.
Исходные extraction callbacks и output errors сохранены.

## IQS5xx profile

Явный стандартный open property `iqs5xx=on` включает Linux device profile;
CLI пример: `7zz x -tIHex -miqs5xx=on firmware.ihex`.
ISetProperties доступен также через публичный archive ABI. Default mode остаётся
generic Intel HEX: неверный checksum settings record без profile не принимается.

Profile собирает один zero-initialized map для диапазона 0x83C0–0xBFFF.
Data records могут идти в любом порядке, последующие writes заменяют предыдущие
в overlap. Допустимы только data/EOF records и полные data spans внутри map.
Как в Linux, checksum исключение относится к records от settings address 0xBE00;
EOF fields/data игнорируются, separators между records могут содержать vendor
текст и NUL. Другие record types и выход за map явно отклоняются.
Фиксированный диапазон относится только к выбранному profile; generic mode
продолжает поддерживать linear/segmented 32-bit address space.

Это read-only map projection, без устройства, update/verify или автоматического
угадывания vendor profile по повреждённому checksum. Backend regression проверяет
обычный Intel HEX; отдельный UI для выбора vendor profile не добавляется.

## Границы Linux reference

AS102 parser — ограниченный LF-only device consumer, а не общий validator Intel
HEX. Он не проверяет checksum и маскирует digits; archive reader сохраняет
проверки malformed input. Binary ihex_binrec, firmware upload packet framing и
device address policy не выдаются за свойства ASCII archive format.

IQS5xx fw_file_parse содержит checksum-error branch, который делает break,
но затем возвращает 0. Reference воспроизводит этот результат для повреждённого
application record; archive profile возвращает явную ошибку и не отдаёт partial
map. Vendor settings exception при этом сохраняется, включая last-write-wins.
Это отличие при ошибке входных данных, а не потерянная read feature.

## Validation

19 positive fixtures на обоих engines: linear/segmented/start/ignored fields,
case, maximum record, 32-bit boundary, empty data/EOF, long whitespace, отдельные
blocks и пять IQS5xx variants. Проверяются automatic full-image recognition
generic `.bin`, explicit vendor property, exact bytes, test operation и source
immutability. 16 malformed cases покрывают record/checksum/digit/length/type,
truncation/EOF, address wrap, separators и vendor ranges/types; normal exit 2.
Отдельно проверяется, что IQS5xx checksum policy не включается по умолчанию.

ABI: семь retained/interleaved stream cases, адреса decoded blocks, source release,
Close/Reopen, early/late source E_FAIL/E_ABORT, output failure, initial/active/final
cancellation, late malformed record с очисткой partial bytes и сохранением
DataError, reuse и backend Unicode/spaces extraction. Ошибка segmented fixture
(повторное добавление base при генерации) обнаружена VA regression и исправлена;
production shift/assembly в этой ветви не менялся.

Reference-only C harness компилирует неизменённые IQS5xx parser, AS102 atohx/
parse_hex_line и Linux hex conversion bodies с userspace firmware/logging adapters.
15 context-specific outcomes и 30 SDK outcomes прошли; IQS maps и safe LF-only
AS102 payloads совпадают побайтно. AS102 reference inputs ограничены реальным
packet data budget; malformed input не подаётся его небезопасному pointer loop.
Никаких firmware requests к устройству, upload или hardware acceptance нет.

Focused 13 suites прошли на macOS (5.39 s) и Linux (6.16 s,
`20261020T120113Z-d751f42a`): IHex/ABI, Base64/ABI, GPT/ABI и прямые native
opening/codec/runtime/baseline consumers. Windows/MSVC build и 12 suites прошли,
но первый CLI process malformed-magic case превысил 10 s. Причина тайм-аута не
установлена; повтор одного IHex suite на неизменённых code/limits прошёл (1.99 s,
`20261020T120526Z-c4292dfa`).

После добавления late-checksum cleanup regression оба IHex suites повторно
прошли на macOS (2.91 s), Linux (1.02 s) и Windows (3.84 s), receipt
`20261020T120656Z-fe3addc3`. Baseline verification: 418 exact / 238 adapted /
11 local units, одна revision. Full/slow, appearance и hardware gates не
запускались. Disposable fixtures удалены; source cache, incremental builds и
ccache сохранены. Завершённый source transfer archive удалён после сверки и
сохранения immutable source index, включая case variants.
