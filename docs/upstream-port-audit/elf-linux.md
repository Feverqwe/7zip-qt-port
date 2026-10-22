# ELF: headers, tables, core numbering и independent streams

## Итоговая read-side сверка — 2026-10-23

Read-side source audit ELF закрыт для archive projection ниже на закреплённом
Linux commit. Полная сверка ниже дополнена повторной проверкой текущих header/
table bounds, section-zero numbering, file/virtual extents и stream consumers;
все семь reference hashes совпали. Execution/relocation и Linux consumer-specific
policy не подменяют generic byte inspection. Production ELF code не менялся;
обе ELF suites прошли macOS проверку вместе с EFI/PE (8/8, 2.74 s).
Остальные runtime/platform ограничения перечислены ниже.

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/ElfHandler.cpp` скопирован под исходным путём в `src/`.
Сохранены CHeader/CSegment/CSection, class/endian readers, выбор sections либо
segments, names, properties, Extract и AllowTail. [Manifest](../upstream-baseline.json)
подключает unit к обоим shipped engines. Notices и LGPL source ownership
сохранены; Linux C bodies не добавляются в production.

До изменений прочитан весь handler, headers/types, OpenArchive callers,
CopyCoder, LimitedStreams и существующий extent stream consumer. После изменений
functional diff повторно сравнен с оригиналом при одинаковом форматировании.
Новый GetStream использует исходный CExtentsStream и существующую shared-source
cursor adaptation; независимый byte reader не создавался.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
Сверены header/PHDR fetch, PT_LOAD/empty-segment bounds и core numbering в
`fs/binfmt_elf.c`, FDPIC/compat counterparts, весь header/program-table reader
`kernel/kexec_elf.c`, section/name validation и соответствующие callers в
`kernel/module/main.c`, ELF headers. [Source hashes](../elf-linux-baseline.json)
закрепляют эти inventory units. Проверка read-side bodies не означает перенос
всех execution, relocation или VFS services Linux.

## Исправления и актуализация

Reader принимает ignored EI_PAD, невыровненные table offsets и использует
фактический размер header соответствующего класса. e_ehsize не определяет
границу чтения: binfmt_elf его игнорирует. 32-битный header больше не требует
лишних 12 bytes после своих 52 bytes. Неиспользуемые table-entry-size fields
не ограничивают чтение; размеры реально читаемых records проверяются.
Удалены необоснованные ограничения offset 2^60 и section EntSize, при этом
сохраняются class/data/version checks и проверки доступных file spans.

PN_XNUM разрешается через исходный section-zero parser: Linux core dumper
публикует реальный count в sh_info. Связанные extended section count/name index
также читаются из section zero. Counts расширены до 32 bits, а aggregate header
size — до 64 bits. Table sizes проверяются до allocation/read; payload extents,
PT_LOAD file/memory sizes и адресные переполнения не могут обернуться в ложные
положительные ranges. Section-name table проверяется по type, границам и NUL;
при её отсутствии публикуются числовые names допустимого string type.

NULL/NOBITS не содержат file payload. Их offsets не увеличивают physical size
и не вызывают Seek при empty extraction. NOBITS сохраняет действительный
virtual size; Extract/GetStream возвращают именно file bytes, а не memory image.
sh_info публикуется как Info Section только для relocation/INFO_LINK records:
symbol-table metadata не превращается в имя случайного section.

Open начинает чтение с начала, учитывает callback до разбора и в больших tables,
передаёт source HRESULT, очищает partial arrays/metadata при failure/cancellation.
Новый IInArchiveGetStream выдаёт независимые seekable readers, владеющие source
и mapping; они работают после Close/Reopen и освобождения handler. Original
copy extraction, callbacks и output HRESULT сохраняются. Неверные item indices
и переполнение общего extraction size возвращают ошибку.

## Контексты Linux и archive projection

У Linux нет единственного одинакового policy для всех ELF consumers. Kexec
строже binfmt_elf в отношении e_ehsize и unused segment fields; module loader
требует корректную section-name table, тогда как executable loader может вообще
не читать sections. Archive reader сохраняет generic class/endian inspection,
REL/DYN/EXEC/CORE и bounds всего опубликованного file payload. Отсутствующая
name table допускает числовые names; повреждённая table вызывает ошибку.
Он не получает kernel architecture, task-address-space или relocation policy.

PN_XNUM — совместимость с Linux-produced cores, а не утверждение, что Linux
exec loader запускает core files или расширенные PHDR arrays. Extended section
numbering дополнительно проверяется независимым readelf. Бесконечные memory
regions, BSS allocation, GNU executable permission policy и symbol relocation
не заменяют archive byte reads.

AllowTail и PreArc selection сохранены из 7-Zip. Явный выбор ELF читает образ
с overlay; automatic selection резервирует такой случай для вложенного архива.
Это отличается от простого execution loader, игнорирующего trailing bytes.

## Связанный discovery regression

Расширенный набор воспроизвёл ошибочное распознавание GPT как empty LZMA86.
Для необычных LZMA properties bounded probe прежде считал нулевой output
доказательством формата, даже когда decoder не прочитал data. Теперь только
этот auto-probe требует EOS для empty output. Explicit Open/Extract сохраняют
прежний finish mode и совместимость; [LZMA audit](lzma-linux.md) содержит follow-up.
Существующий GPT behavior regression проверяет оба engines; новый source-text
или implementation-presence test не добавлялся.

## Validation

24 CLI positives на обоих engines: ELF32/64 LE/BE, ignored padding/e_ehsize,
unaligned/unused table geometry, minimal header, zero/NULL segments, sections,
NOBITS, numeric names, large EntSize, symbol Info и explicit overlay. Exact
selected payload, automatic full-image recognition, test command и source
immutability проверяются отдельно. 22 malformed cases покрывают header/table
sizes, payload truncation, address/offset overflow, memory size, names и
extended-numbering errors; ожидается normal exit code 2.

ABI дополнительно проверяет 65 536 core PHDRs, 65 538 sections с extended names,
retained/interleaved streams и source release, virtual 2 EiB table offset без
создания большого файла, BSS sizes/EOF, source/output E_FAIL/E_ABORT, initial/
active table cancellation, cleanup/reuse и symbol metadata classification.
ArchiveBackend извлекает section из Unicode/spaces `.bin` path.

Временный C harness компилирует неизменённые header/program reader bodies kexec
и generic section/name validators module loader с userspace header/allocation
adapters. 46 context-specific outcomes соответствуют source policies, включая
различия e_ehsize, unused records, memory sizes и absent names. Доступные payloads
дают exact bytes. Это проверки конкретных readers, не всего kexec/module loading.
Native Linux execution и readelf служат отдельными evidence; references не
подменяют runtime. Disposable fixtures/output удаляются, builds и ccache
переиспользуются. Full/slow и UI appearance gate в этой итерации не запускались.

Focused validation: 16 suites прошли на macOS (4.02 s), Linux (8.51 s) и
Windows (33.45 s), remote receipt `20261020T104000Z-1c46b27c`. Набор включает
ELF/ABI, LZMA/ABI, GPT/ABI, Base64/ABI, XZ/ABI и непосредственные runtime,
discovery и baseline consumers. На Linux действительно выполнены пять native
x86-64 executables (exit 42), а readelf подтвердил оба extended counts;
соответствующие четыре CLI tests прошли без skips.

После последней правки symbol metadata оба ELF suites повторно прошли на macOS
(3.48 s) и Linux; Windows ABI также прошёл, но CLI получил единичный 10 s timeout
при запуске malformed xnum-missing fixture (`20261020T104636Z-007642cb`). Причина
тайм-аута не установлена. Повтор Windows CLI на неизменённом коде и с теми же
лимитами прошёл (26.40 s, `20261020T104950Z-1de3e17a`). Baseline verification:
418 exact / 235 adapted / 11 local units, одна upstream revision.
