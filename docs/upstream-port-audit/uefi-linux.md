# EFI capsules: общий header, Quark и archive streams

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/UefiHandler.cpp` скопирован под исходным путём в `src/`.
Прочитаны header/item types, registrations UEFIc/UEFIf, все volume/section,
Intel ME, LZH/LZMA, naming, property и extraction bodies. Сверены OpenArchive
extension/signature ordering, copy/progress helpers и ownership buffer streams.
После изменений повторно проверен functional diff при одинаковом форматировании.
[Manifest](../upstream-baseline.json) подключает unit к обоим shipped engines.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
Сверены `drivers/firmware/efi/capsule-loader.c`: setup/write/submit/release и
allocation/error states; `capsule.c`: supported/query, size/flags, update и
scatter/gather transport; типы `include/linux/efi.h`; Quark header,
qrk_capsule_setup_info и его CPU-selected caller в
`arch/x86/platform/efi/quirks.c`. [Source hashes](../uefi-linux-baseline.json)
закрепляют эти units. Наличие firmware-management structs само по себе не
означает, что Linux декодирует FMP contents: loader передаёт opaque capsule
прошивке. Kernel firmware-volume/FFS parser этой сверкой не заявляется.

Сохранён исходный C++ reader 7-Zip, включая LZH/LZMA algorithms и зависимости.
В SDK нет generic/Quark archive reader: header whitelist отвергал другие GUID,
а kernel C transport зависит от pages/physical addresses, CPU и EFI services.
Узкая новая адаптация читает ABI fields существующими Get32/header helpers и
добавляет file offsets к готовому stream mapper. Это не перенос C kernel bodies
в C++ и не независимый firmware/crypto parser. Linux GPL code используется
только в reference harness, в production не добавляется.

## Актуализация чтения

Общий EFI capsule header читается от 28 bytes; opaque GUID, в том числе FMP,
больше не требует одного из трёх SDK GUID. Header/image/body spans проверяются
до доступа к payload. Legacy headers 80/32 bytes сохраняют свои body offsets;
их дополнительные fields не читаются из короткого input. Произвольные header
padding и пустой/короткий payload больше не требуют 80-byte image или FV body.

Opaque payload доступен как `payload.bin` через исходный extent stream.
Он читается по запросу без allocation полного capsule. То же применяется к
capsule, превышающему прежний buffer budget, и legacy split fragments: это
байты конкретного fragment, а не сборка обновления. Известные FV/Intel ME
contents в пределах SDK budget продолжают проходить исходный полный parser.
Ошибки распознанного содержимого не превращаются в успешные неверные child bytes.
Большой generic capsule сохраняет byte access, включая FV payload без его
буферизации/разворачивания; лимиты расширенного SDK FV parser не объявляются
ограничением общего capsule header reader.

Quark mapping распознаёт signature и фиксированный 1024-byte security header,
проверяет отсутствие chain и читает вложенный EFI header. Version/modulesize и
другие firmware security fields не получают выдуманную validation policy:
Linux quirk также использует signature/header size/next-header. Физический
размер и source offsets считаются в 64 bits, включая wrapper плюс 32-bit image.
Archive reading не зависит от CPU, на котором выполняется приложение.

Capsule GUID и flags публикуются в archive metadata. Сохранены прежние три
signatures, добавлена Quark signature; `scap cap capsule` позволяют открытие
generic GUID по расширению. Известные signatures работают также в `.bin`.
Другие GUID доступны по расширению или explicit UEFIc selection; глобальный
signature scan не угадывает формат по любым 16 bytes.

Open учитывает initial/chunk/parse callbacks, передаёт source HRESULT и очищает
partial state при error/exception/cancellation. UEFIf использует те же bounded
reads и callbacks. Intel ME region bounds считаются относительно body, включая
полный region span: неверный region не публикуется как усечённый файл.
GetProperty/GetStream/selected Extract проверяют indices. Test mode действительно
читает stream, поэтому lazy payload I/O failure не выдаётся за успешный test.

Opaque streams владеют source/mapping и имеют независимые cursors. Buffered FV,
ME и decoded-section streams используют существующий owned-buffer helper:
удержание handler не сохраняло прежние `_bufs` после Close/Reopen. Выданные
streams теперь переживают эти операции и освобождение handler/source.

## Границы Linux и archive projection

EFI firmware query определяет поддержку GUID, max size и reset policy при
обновлении системы. Linux отвергает unsupported flags и превышение firmware
max; archive inspection сохраняет эти fields и читает bytes независимо от
возможности конкретной машины установить обновление. Ограничение firmware max
относится к EFI image, без Quark wrapper. Никакие firmware calls, update, reboot,
authentication или key policy не выполняются archive reader.

Linux transport не проверяет FV/ME layout и некоторые невалидные header/body
spans до firmware call; archive reader проверяет границы публикуемых bytes и
развёрнутых children. Эти контексты не подменяют друг друга. Quark runtime quirk
выбирается только на соответствующем x86 CPU; archive ABI даёт переносимый
просмотр его file layout. Это не утверждение о native update на остальных ОС.
Сохранён SDK budget для расширенного FV/section decoding; LZH body не изменён,
но отдельный LZH producer fixture в этой итерации не проверялся.

## Validation

18 CLI positives на обоих engines: generic/FMP/zero GUID, padding/flags,
empty/short payload, три SDK capsule layouts, split fragment, Quark/raw/FV,
FV/LZMA/ME capsules и standalone FV/LZMA. Проверяются exact selected bytes,
automatic extension/signature discovery, test command и неизменность source.
13 malformed inputs покрывают short/header/image/body spans, legacy layouts,
Quark truncation/chain, FV checksum/size и ME region overrun; normal error code 2.

ABI проверяет 12 retained/interleaved stream variants, в том числе decoded
buffer, EOF и virtual sources с near-4-GiB header, crossing-4-GiB Quark extent
и 1-GiB padding перед FV. Большие физические файлы не создаются. Дополнительно:
initial/source/output E_FAIL/E_ABORT, lazy test-mode source failure, отмена до
чтения, во время chunk read и после полного чтения при разборе partial tree,
cleanup/reuse и backend extraction из Unicode/spaces `.cap` path.

Reference-only C harness компилирует неизменённые selected Linux common setup,
supported-query и Quark bodies вместе с исходными header/context layouts.
Allocation/logging/CPU/query adapters моделируют userspace environment; firmware
query не обращается к реальной машине. 43 context-specific outcomes проверяют
header/flags/sizes, partial input, Quark physical skip/total и firmware max.
Это не full upload, signature, firmware-update или boot acceptance.

Focused 17 suites прошли на macOS (3.94 s), Linux (8.36 s) и Windows/MSVC
(37.33 s), receipt `20261020T113108Z-ec1e4b2b`: UEFI/ABI, PE/ABI, ELF/ABI,
LZMA/ABI, GPT/ABI и непосредственные native opening/codec/runtime/baseline
consumers. После добавления large-capsule projection и signature-only discovery
cases оба UEFI suites повторно прошли на macOS (3.19 s), Linux (1.19 s) и
Windows (5.23 s), receipt `20261020T113256Z-87d4237e`.
Baseline verification: 418 exact / 237 adapted / 11 local units, одна revision.
Full/slow и appearance gates не запускались; firmware и физические носители не
использовались. Disposable fixtures удалены, incremental builds и ccache сохранены.
