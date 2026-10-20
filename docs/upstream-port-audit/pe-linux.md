# PE: headers, file spans и retained streams

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/PeHandler.cpp` скопирован под исходным путём в `src/`.
Сохранены PE/COFF header/section readers, resources, debug/certificate/symbol
projection, checksum, properties, Extract, registration и AllowTail/PreArc.
TE находится в той же inventory unit и сохранён без функциональных изменений.
[Manifest](../upstream-baseline.json) подключает unit к обоим shipped engines.
Исходный алгоритм checksum и resource conversion остаётся реализацией 7-Zip.
Независимый PE parser не добавляется; Linux GPL bodies не входят в production.

До изменений прочитаны полный handler и его PE/COFF/TE registrations, callers
OpenArchive/AllowTail, stream ownership, CopyCoder и progress/error dependencies.
После изменений source diff повторно сравнен с SDK при одинаковом форматировании;
изменения форматирования отделены от функциональных. GetStream переиспользует
исходные CExtentsStream, CReferenceBuf и CBufInStream.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
Сверены `crypto/asymmetric_keys/verify_pefile.c`: полные parse_binary,
strip_sig_wrapper, compare_shdrs, digest_pe_contents, digest_pe и signature
entry point; context/header dependencies и kexec verification callers.
[Source hashes](../pe-linux-baseline.json) закрепляют inventory units.
Наличие COFF structures внутри PE не доказывает поддержку standalone COFF object
reader в Linux; COFF проверен как непосредственный consumer общего handler.

## Исправления

e_lfanew больше не ограничен 4 KiB: Linux проверяет доступный file span.
Discovery складывает offset/header size без 32-bit overflow. PE/COFF Open
начинает чтение с начала и проверяет table span до allocation/read. HeadersSize
учитывается в физическом размере, включая образ без sections. Nonempty section,
debug, certificate и symbol/string table spans ограничены source size.
Нулевой raw section не расширяет physical size и не порождает gap по своему
неиспользуемому offset; пустое извлечение не выполняет такой seek.

Физические ends и synthetic section offsets расширены до 64 bits: сумма двух
32-bit disk fields может пересечь 4 GiB. RVA containment debug directory также
считается без wrap. Debug iterator продвигается после каждого record, включая
нулевой: следующая запись больше не теряется. COFF без optional header не читает
его directories, а отсутствующие relocation/exception entries не обнуляют
данные section с VA=0.

Open сообщает progress до разбора, в больших section/resource tables и checksum
loop, передаёт callback/source HRESULT и удаляет partial state при отказе или
exception. Close очищает headers/resource metadata; исходная checksum warning
policy сохраняется. GetProperty/GetStream/selected Extract проверяют indices.

Section streams используют готовый extent reader с независимым cursor и source
ownership. Resource streams без synthetic header теперь тоже владеют копией
данных: ссылка на handler не сохраняла прежний `_buf` после Close/Reopen.
String/version/bitmap streams сохраняют исходный reference-buffer path.
Все эти streams работают после переоткрытия и освобождения handler/source.

## Границы Linux reference

Linux PE verifier читает signed executable container; это другой contract,
чем архивный просмотр unsigned PE, raw sections и ресурсов. -ENODATA означает
отсутствие signature, а не ошибку archive bytes. Проверка trust/keyring, PKCS#7
signature и запуск/relocation image не становятся частью archive extraction.
Извлечение CERTIFICATE не утверждает подлинность сертификата или файла.

Linux verifier ограничен contiguous buffer с unsigned pelen; archive ABI имеет
seekable 64-bit source. Он допускает некоторые header layouts иначе, чем SDK:
не использует opt_hdr_size как границу optional header и прекращает unsigned
parse до section validation. SDK сохраняет собственные directory/table checks
и полезный headers-only inspection. Linux digest reader использует raw sizes;
7-Zip сохраняет useful-byte projection min(nonzero VirtualSize, RawSize),
resource conversion и дополнительные synthetic items. Это file bytes, а не
memory image с BSS allocation.

Сохранены SDK ограничения COFF symbols/string tables и debug-directory heuristic,
для которых в выбранном Linux consumer нет соответствующего reader. AllowTail
и PreArc оставляют automatic overlay selection вложенному архиву; explicit PE
допускает trailing bytes. TE, EFI capsules и оставшийся общий census не считаются
проверенными по Linux только из-за общей terminology или source unit.

## Validation

19 CLI positives на обоих engines: PE32/PE32+, поздний и unaligned PE header,
отсутствующие directories, headers-only, пустой BSS со stale offset, debug после
пустой записи, certificate tables, raw/bitmap/string/version resources,
COFF/relocation, checksum и explicit overlay. Проверяются automatic recognition,
точные selected bytes, test command и неизменность source. 16 malformed cases
проверяют header/table geometry, truncation и переполнения section/certificate/
debug/symbol spans; процесс завершается normal error code 2.

ABI дополнительно проверяет crossing-4-GiB extent без большого физического файла,
interleaved/retained streams всех buffer paths, Close/Reopen/source release,
BSS metadata/EOF и отсутствие неиспользуемого seek, source/output E_FAIL/E_ABORT,
initial/active table/checksum cancellation и cleanup/reuse. Checksum regression
различает корректный и изменённый payload; backend извлекает section из
Unicode/spaces `.bin` path.

Временный C harness компилирует неизменённые Linux parse/strip/compare/digest-input
bodies с userspace allocation/logging adapters и исходным PE header/context.
33 context-specific outcomes соответствуют source policy. Три PE32/PE32+/late
certificate fixtures дают точные canonical digest byte streams, checksum field
и certificate directory исключаются исходным Linux кодом. Hash callback только
записывает input и ограничивает его source buffer; это не crypto/signature или
kernel-boot acceptance. Unsigned early returns не выдаются за validation spans,
которые verifier не успел прочитать.

Focused 16 suites прошли на macOS (3.92 s) и Linux (8.09 s,
`20261020T110423Z-b0e59e09`): PE/ABI, ELF/ABI, LZMA/ABI, GPT/ABI, runtime,
baseline, SFX creation и native archive/folder opening consumers. Строгая
Windows сборка выявила два неявных narrowing conversions для synthetic gaps;
добавлены явные casts в пределах исходных 32-bit offsets. После этой правки
PE suites повторно прошли на macOS (3.70 s) и Linux (1.07 s,
`20261020T110706Z-d18b2774`).

Windows MSVC build и 14 из 16 suites прошли; PE CLI получил единичный timeout
первого malformed-magic process (10 s), SFX suite превысил 60 s
(`20261020T110707Z-03642664`). Причина этих тайм-аутов не установлена;
проверка процессов не обнаружила оставшихся тестовых executables. Повтор только
PE/SFX на неизменённых исходниках и лимитах прошёл (24.58 s,
`20261020T111242Z-6f0f3764`); для исполняющего GUI-модуль SFX выбрана interactive
Windows session. Это не visual/UI parity acceptance.

Baseline verification: 418 exact / 236 adapted / 11 local, одна revision.
Disposable fixtures удаляются; persistent builds и compiler caches сохранены.
Full/slow gates, физические носители и kernel execution не проверялись.
