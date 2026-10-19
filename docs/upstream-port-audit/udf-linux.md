# UDF: чтение и актуализация по Linux

## Источники и выбранный сценарий

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Адаптированы `CPP/7zip/Archive/Udf/UdfIn.cpp`, `UdfIn.h`, `UdfHandler.cpp`
под исходными путями в `src/`. Оба engine получают одни runtime overlays;
[baseline](../upstream-baseline.json) фиксирует происхождение и зависимости.
Неизменённые `UdfHandler.h`, `StdAfx.h`, `Common/MyMap.cpp`/`.h` сохранены в
`third_party/7zip`; CRC16, buffers и stream helpers переиспользуются.
Уведомления и лицензия
7-Zip сохранены. Форматирование C++ учитывается отдельно от функционального diff.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
[Reference manifest](../udf-linux-baseline.json) фиксирует snapshot `fs/udf`;
он не означает перенос или проверку всех перечисленных файлов.
Для этой итерации сравнены `udf_map_block`, `__udf_get_block`, `inode_bmap`,
`udf_current_aext`, `udf_next_aext`, inline validation в `udf_read_inode`,
`udf_get_fileshortad`, `udf_get_filelongad` в `directory.c`,
соответствующие `ecma_167.h` allocation/FE/EFE структуры и partition mapper.
Linux определяет значения recorded/unrecorded descriptors, неявные holes и EOF.
GPL bodies в engine не копируются: используется существующий 7-Zip
`CExtentsStream`, включая его готовый zero-fill режим.

До реализации сравнены полные тела 7-Zip `CItem`, short/long/inline AD parsing,
`CheckExtent`, `CheckItemExtents`, `Read`, `ReadLad`, `ReadFromFile`,
`ReadItem`, metadata partition bootstrap и physical-size consumer в `Open`,
а также свойства, `GetStream` и `Extract`. Сохранены структура модуля,
классы, callbacks, progress, CopyCoder, directory traversal и registration.

## Изменения чтения

- Оба типа unrecorded extents дают нули; backing bytes allocated hole не
  выдаются пользователю, а unallocated position не используется как адрес.
  Короткий список AD также оставляет неявный нулевой хвост до размера файла,
  как unmapped blocks в Linux.
  Нулевая raw length завершает список short/long AD; последующие bytes не
  восстанавливают recorded data после terminator.
- Последний обычный extent ограничивается information length. Полный размер
  выделения и последующие descriptors не заставляют читать за EOF; проверка
  partition bounds применяется к реально читаемой части. Partition reference
  активных descriptors проверяется также для holes.
- Directory buffer читается только до information length. Ранее простое
  ослабление общей проверки равенства размеров позволило бы записать полный
  allocation extent в меньший buffer. Short reads и holes в каталогах не
  превращаются в успешно разобранное частичное дерево.
- Inline allocation сохраняет строгое равенство information length, как
  Linux. Данные вне lengthAllocDescs не становятся payload. Эту ветвь
  проверяют вместе с обычными allocation variants, включая inline directory.
- Physical-size scan использует те же активные длины, уменьшает остаток файла
  и пропускает holes. Sparse offsets и unused tail не расширяют physical size.
- Bootstrap metadata partition сохраняет полную проверку выделенного extent,
  поскольку этот caller создаёт partition mapping из всего descriptor.
  Zero-fill policy обычных файлов на него не распространяется.

После изменений повторно сравнены изменённые bodies и все callers проверок
размеров/extents. `Extract` использует `GetStream`; zero-fill, seek, source
ownership и межпоточная независимость остаются у существующего stream helper.
Новый parser или декодер для этих ветвей не добавлен.

## Проверки и оставшаяся работа

Fixtures основаны на disposable образе Ubuntu genisoimage 1.1.11. `udf`
проверяет оба engine: short/long AD, оба explicit hole вида, implicit holes,
EOF внутри allocation, чтение активной части на границе partition, unused tail,
inline file/directory и directory allocation padding. Отдельные повреждения
проверяют отказ для активного адреса вне partition, плохого partition reference,
невалидного continuation target, inline length mismatch и metadata bootstrap
с hole/выходящим за partition allocation.

`udf_stream` проверяет точные bytes и EOF для всех положительных вариантов,
seek, чередующиеся потоки, чтение после Close/reopen/release handler,
передачу source I/O failure и отмены, а также list/selected extraction и
`extractSourceEntry` через ArchiveBackend. Тесты используют публичный ABI,
не читают исходники и не закрепляют внутренний layout.

Все 17 положительных fixtures совпали по payload bytes с read-only loop mounts
нативного Linux `6.8.0-146-generic`; mounts и временные образы удалены.
Это runtime evidence для указанного ядра, а не выполнение pinned reference.

Пять focused suites (`udf`, `udf_stream`, `upstream_baseline`, `runtime_cache`,
`runtime_package`) прошли на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12. Remote receipt: `20261019T202259Z-37da3f7a`. После узкого изменения
отступов повторены macOS checks; функциональный код после проверок не менялся.
macOS app собрана; actual CMake install прочитал все 17 вариантов, staging удалён.
Использованы постоянные incremental builds и compiler caches. Full/slow suites,
UI и чтение физических устройств в этой итерации не проверялись.

## Продолжения allocation descriptors

Отдельный `sevenzip_udf` format-модуль передаёт short/long AD enumeration
исходным Linux C-функциям. Скопированы полные, неизменённые тела
`inode.c::udf_current_aext`, `udf_next_aext` и
`directory.c::udf_get_fileshortad`, `udf_get_filelongad`.
Предел consecutive indirections сохранён. `udf_i.h` сохраняет исходный
`extent_position`, `udfend.h` — `lelb_to_cpu`; полный `ecma_167.h` перенесён
без изменений. В `udfdecl.h` сохранён `udf_file_entry_alloc_offset`.
Остальные тела этих Linux units пока не перенесены: они относятся к остальным
UDF read paths или VFS/write services. Новая C ABI glue только связывает
enumeration с имеющимися extents, information length и callbacks.

Compatibility layer предоставляет owned block buffers и минимальные inode/sb
services. Логические адреса возвращаются существующему partition mapper 7-Zip,
а ошибки/отмена callbacks сохраняют HRESULT. Portable `linux/types.h` виден
только C translation units UDF-модуля; C++ и системные Linux headers используют
свои типы. Уведомления, GPL-2.0 и лицензия 7-Zip устанавливаются вместе с модулем.
GPL C не связан с core engine или unRAR codecs. Динамический engine передаёт
регистрацию UDF этому модулю; standalone сохраняет прежний flat-AD reader.
Таким образом новые continuation capabilities относятся к приложению и
динамическому engine; standalone parity в этой области не заявляется.

Сравнены полные исходные C bodies, callers, endian/disk types, существующие
`ReadItem`, directory/metadata consumers и `CMap32` с зависимостями. Повторная
сверка после переноса подтвердила неизменённость четырёх C bodies; адаптации
ограничены block/stream services и archive enumeration boundary. AED headers
обрабатываются с теми же length/buffer checks, что и Linux reader; отдельная
новая CRC/tag policy для AED не изобреталась.

Девять новых положительных fixtures проверяют FE+AED data, short/long chains,
граничную глубину, directory continuation, второй физический partition,
terminator и implicit tail в AED. Все совпали с read-only native Linux
`6.8.0-146-generic`. Повреждения проверяют cycle, превышение глубины, адрес и
partition reference вне диапазона, переполнение/выход AD length за block.
ABI также проверяет source failure и отмену во время Open, пустой опубликованный
каталог после ошибки и успешное повторное Open; retained streams и backend
selected/source-entry extraction охватывают новые allocation variants.
Backend получает UDF через штатный native codec catalog и проверяет read-only
capability; CLI-тесты загружают изолированную копию только проверяемого модуля.

Пять focused suites прошли на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12: receipt `20261019T203449Z-3d8c7921`. Расширенные catalog/backend
проверки затем прошли на тех же трёх ОС, remote receipt
`20261019T203621Z-3eb58fef`. macOS app собрана; actual CMake install прочитал
все 26 положительных вариантов и сохранил точное содержимое двух UDF notices.
Staging, native mounts и одноразовые образы удалены. Постоянные build trees и
compiler caches сохранены; full/slow/UI/physical-device acceptance не выполнялась.

## Indirect ICB и strategy 4096

Передан resolution prefix `inode.c::udf_read_inode`: начальные bounds,
FE/EFE/USE tag selection, strategy 4/4096, optional IE, reread, предел hierarchy
и проверка AD kind. Метаданные и данные выбранного FE/EFE затем разбираются
существующим `ReadItem`. VFS/inode projection после этого prefix не копировалась;
она остаётся отдельным read scope. Удалены только неиспользуемые declarations
VFS-части и EFE alias; ветви выбранного prefix сохранены. Возврат owned FE
buffer заменяет переход к остальной VFS-инициализации.

Полные `misc.c::udf_read_tagged`, `udf_read_ptagged`, `udf_tag_checksum` и
`lib/crc/crc-itu-t.c`/`include/linux/crc-itu-t.h` передают исходные checks и
CRC table/loop. Изменены kernel includes/exports и ширина tagged-read address
для существующего portable partition token; тела checks и CRC неизменны.
Все Linux notices и C language сохранены. Новые CRC units входят в тот же
изолированный GPL-модуль; core/standalone не связываются с ними.

Сравнение до и после переноса включает complete selected prefix, helpers,
types, callers и consumers `ReadItem`, `ReadLinuxExtents`, metadata bootstrap,
archive flags и physical-size scan. `super.c::udf_find_metadata_inode_efe`,
`partition.c::udf_try_read_meta` определяют требование short AD и фиксированный
backing partition metadata map. Результат resolution используется для short
AD обычных inode, а metadata сохраняет свой исходный backing map, даже когда
ICB самого metadata file находится в другом разделе.

Linux сохраняет прежний FE при отсутствии пригодной optional IE. У archive
adapter это не теряет отмену или ошибку обязательного target; failed/canceled
Open не публикует каталог. Успешный fallback восстанавливает прежний
UnexpectedEnd state и ограничивает physical footprint backing source.
Повторная строгая C++ tag-проверка не отменяет принятые Linux ICB tags.

Источник `super.c::udf_check_anchor_block`, `udf_scan_anchors`,
`udf_load_sequence` подтверждает: пригодный primary anchor достаточен и для
open media. Поэтому динамический модуль не выставляет ошибку отсутствия end
anchor после успешной основной проверки. Необязательный старый tail scan
больше не отменяет fallback при повторном I/O отказе; E_ABORT сохраняется.
Standalone оставляет прежнюю политику. Воспроизведённый pre-fix ABI failure
и последующий PASS подтверждают чувствительность regression для этого caller.

20 новых положительных вариантов проверяют FE/EFE, inline, root/child directory,
1024 перехода, cross-partition short AD, продолжение AD после ICB, metadata
и fallback для missing/terminal/empty/corrupt IE. Все совпали с read-only
нативным Linux `6.8.0-146-generic`; mounts/images удалены. Metadata fixture
загружает зависимый partition descriptor раньше metadata map, как требует
порядок initialization в Linux. Повреждённые цепочки проверяют цикл, превышение
глубины, неверные refs/targets, required tag/checksum/CRC/version/length,
unsupported strategy/AD kind и long AD metadata. Отдельные ABI проверки
передают I/O failure и отмену в optional IE, проверяют отсутствие ложных
archive error flags после fallback и повторное Open. CLI проверяет metadata
выбранного inode; backend/catalog consumers охватывают новые variants.
`udf_lookup`, `udf_load_fileset` и resolution prefix также подтверждают чтение
полного logical block независимо от extent length родительской ссылки.
Динамический модуль больше не требует ровно block-sized parent extent;
короткая, увеличенная и нулевая root extent length проверены native и через ABI.

Пять focused suites (`udf`, `udf_stream`, `upstream_baseline`, `runtime_cache`,
`runtime_package`) прошли на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12. Последний remote receipt: `20261019T210032Z-3c93313f`.
Actual macOS CMake install прочитал все 46 положительных вариантов и установил
оба UDF notices с точным содержимым. Staging удалён; сохранены incremental
build trees/compiler caches и небольшое свидетельство воспроизведённого
tail-scan failure. После проверок нормализованы только пустые строки EOF
новых C-файлов. Full/slow/UI/physical-device acceptance не выполнялась.

## Sparable partitions и составное отображение

Полный `super.c::udf_load_sparable_map` перенесён с исходными checks packet size,
числа таблиц, tag/CRC, identifier и размера entries в buffer. Единственная
типовая адаптация body — `const char *` для pointer-sign comparison.
`partition.c::udf_get_pblock_spar15` сохраняет исходный выбор первой пригодной
таблицы, обход entries/sentinels и physical fallback. Добавлены checked additions
для combined block/offset, mapped location и fallback root: 32-bit wrap не
должен превращать повреждённый адрес в успешное чтение чужих блоков. Эти узкие
archive-boundary проверки являются отличием от pinned malformed-input policy;
два regressions защищают mapped-target и fallback-root wrap.

Полный `osta_udf.h` сохранён без изменений. C state владеет проверенными table
buffers и освобождается через module deleter; callback context нужен только при
загрузке. Mapping после Open не хранит указатели на stack progress callbacks.
Уведомления и лицензии остаются в существующем GPL UDF-модуле, исходные C
функции не связаны с core/unRAR. Standalone оставляет прежний read scope.

Существующий `Read` и `GetStream` используют общий physical-span boundary:
extent дробится на packet boundaries, а соседние physical spans объединяются
при подготовке extents для существующего `CExtentsStream`. Logical EOF, holes
и retained streams сохраняются.
Physical-size scan проходит те же spans. Metadata map сохраняет offset и
associated backing reference, чтобы compound mapping доходил до sparable map,
вместо потери remapping при прежнем плоском прибавлении physical root.
Исходные `udf_try_read_meta`, partition mapper и metadata-load callers сравнены
до изменения; single metadata extent и mirror support остаются отдельным scope.

15 новых positive fixtures проверяют packet/extent boundary, обратный physical
порядок, fallback tables, first-valid precedence, отсутствие/повреждение таблиц,
полный table buffer, допустимые Linux reserved/declared-size fields, sparse,
ICB, directory records через два packets и metadata на sparable backing.
Исходные блоки содержат poison bytes там, где обязателен remapping. Отдельные
отказы покрывают неверные packet/table fields, source bounds, sentinel и wrap.
ABI проверяет loader I/O fallback, сохранение однократного E_ABORT при переборе
optional tables, mapped-data I/O failure/retry,
точные bytes/EOF и чтение streams после Close/reopen/release. Backend/catalog
consumers используют reverse mapping, directory и metadata cases.

14 variants совпали с read-only native Linux `6.8.0-146-generic`; mounts/images
удалены. `sp-table-overrun` оставлен проверкой pinned code/module: guest не
выполняет современный bounds check. В upstream v6.8 `udf_load_sparable_map`
использует `sizeof(*st) + reallocationTableLen`, тогда как pinned revision
проверяет полный `struct_size` entries. Наблюдаемый native mount failure
согласуется с этим различием; parity PASS для данного malformed input не
заявляется. Ссылка для отдельного старого source evidence:
<https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux/+/refs/tags/v6.8/fs/udf/super.c>.

Пять focused suites прошли на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12: remote receipt `20261019T212357Z-14847466`. После локальных уточнений
physical sentinel range и сохранения cancellation повторены `udf`/`udf_stream`
на тех же трёх ОС: receipt `20261019T212834Z-ac1db147`.
Actual macOS CMake install прочитал все 61 positive variants, включая pinned
table-bound fallback, и сохранил оба UDF notices. Staging удалён; постоянные
build trees и compiler caches сохранены. Full/slow/UI/physical-device
acceptance не выполнялась. Это завершённый sparable scope, а не закрытие
VAT или всего UDF read audit.

## VAT15/20: поиск таблицы и чтение через backing partition

Из того же pinned Linux revision перенесены полные `super.c::udf_find_vat_block`,
`udf_load_vat`, `partition.c::udf_get_pblock`, `udf_get_pblock_virt15` и
`udf_get_pblock_virt20`. До реализации сравнены их callers при загрузке volume,
`udf_iget_special`, полный `udf_read_inode`, `udf_bread`, `inode_bmap` и
`udf_map_block`, структуры VAT/partition maps и соответствующие 7-Zip consumers.
После изменений тела сравнены повторно. VAT20 dispatch подключён через исходный
alias VAT15 mapper. Исходные notices сохранены; C остаётся C в GPL-модуле.

Сохранены поиск последней доступной FE и трёх предыдущих блоков, выбор первой
пригодной inode, разные VAT15 trailer/VAT20 header offsets, bounds числа entries,
inline/external ветви, recursion guard и передача extent offset в backing map.
Повреждённые данные уже выбранного нового VAT не заставляют выбирать старую
таблицу. Inode неизвестного типа пропускается при поиске; cancellation сохраняется
при повторных попытках, включая однократный E_FAIL progress callback.
Resolved VAT inode может принадлежать другому partition reference; C services
сохраняют полный размер таблицы partition maps, а terminal mapping передаётся
соответствующему backing reader. Device/session discovery не переносилась:
для обычного образа last recorded и device last равны последнему полному блоку файла.
Отдельная CD-ROM/session геометрия остаётся в открытом discovery scope.

Узкие адаптации malformed-input policy: checked terminal address addition,
защита scanner от sector underflow, минимальная длина VAT20 header, верхняя
граница размера таблицы и выравнивание header на LE32 cell. Inline cell читается
через `memcpy`; external byte offset нормализован без unsigned вычитания
`blocksize - start_offset`, которое небезопасно для многоблочного header в
pinned implementation. Для обычного выровненного header результаты совпадают
с исходным вычислением. Невыровненный header отклоняется, в том числе inline;
Linux inline policy здесь мягче. Эти отличия не выдаются за native parity.

`src/fs/udf/vat.c` предоставляет inode/stream callbacks и владеет выбранным
VAT handle; новых filesystem parsers в нём нет. `UdfLinux.cpp` связывает
существующий FE/AD reader с этими services. Hidden VAT role сохраняет FE/EFE,
strategy resolution и inline size validation. Поддержаны non-device inode types
из Linux switch без обхода directory contents. Block/character special inode
требуют ещё не перенесённой проверки device extended attribute и пока
отклоняются; полный special-inode/EA audit остаётся открытым.

Существующий 7-Zip stream builder перемещён из `CHandler::GetStream` в
`CInArchive::GetItemStream`, чтобы VAT использовал те же bounded extents,
source ownership и seek services. Таблица не буферизуется целиком: external
cells читаются по блокам через готовый stream. Sparse VAT data отклоняются,
поскольку Linux `udf_bread` не возвращает из holes записанные VAT cells.
Lazy data reader не вызывает сохранённый Open progress callback. Его handle
освобождается вместе с partition map, а выданные пользователю streams уже
владеют physical extents и исходным IInStream независимо от handler.
Read, stream preparation и physical-size scan сохраняют исходный extent base
и передают накопленный offset отдельно, включая compound sparable backing.
Standalone engine по-прежнему не получает GPL VAT code.

Одиннадцать новых positive fixtures проверяют VAT15/20 inline и external data,
поздние entries, многоблочный header, поиск предыдущей inode, неизвестный
тип новой inode, cross-partition ICB/VAT backing, sparse file и base/offset semantics.
Compound VAT+sparable case содержит запрещённые соседние VAT cells: packet split
должен продвигать offset исходного extent, а не искать следующую cell.
Исходные data/metadata blocks содержат poison bytes. Десять malformed cases покрывают отсутствие
VAT, короткие/невыравненные/выходящие за EOF headers, отсутствующие cells,
sentinel/out-of-partition mapping, новый повреждённый VAT перед старым исправным
и hole в данных таблицы. ABI дополнительно проверяет optional inode I/O fallback,
однократную отмену, обязательный VAT-data I/O failure/retry, mapped payload
failure/retry, retained streams и ArchiveBackend consumers.

Десять обычных variants совпали по payload с read-only native Linux
`6.8.0-146-generic`; mounts и образы удалены. Многоблочный header проверен
через модуль, а не небезопасную ветвь native kernel; runtime parity для него
не заявляется.

Пять focused suites (`udf`, `udf_stream`, `upstream_baseline`, `runtime_cache`,
`runtime_package`) прошли на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12; receipt `20261019T214824Z-4f2f0a1a`. После локальных уточнений hidden
inode selection, progress errors и cross/compound mapping повторены
`udf`/`udf_stream` на всех трёх ОС: final receipt
`20261019T215541Z-67785abe`. Actual macOS CMake install прочитал все 72 positive
UDF variants и установил обе лицензии с точным исходным содержимым. Staging
удалён; постоянные build trees и compiler caches сохранены. Full/slow/UI и
physical-device acceptance не выполнялась. Runtime validation относится к
описанному read scope, а не ко всем UDF возможностям.

## Metadata partition и mirror

Из pinned Linux `58161ead29e50d70a317ea26b472e0c7508e69da` перенесены полные
`super.c::udf_find_metadata_inode_efe`, `udf_load_metadata_files`,
`partition.c::udf_try_read_meta`, `udf_get_pblock_meta25`, `inode.c::inode_bmap`
и четыре extent-cache helpers (`__udf_clear_extent_cache`, `udf_clear_extent_cache`,
`udf_read_extent_cache`, `udf_update_extent_cache`). Полные bodies, callers,
`udf_iget_special`, `udf_bread`, AD traversal, структуры и bitmap/read-only ветви
сравнены до реализации и повторно после неё. `udf_sb.h` сохраняет read-side
projection partition types, `udf_i.h` — исходные extent/cache структуры.
Их происхождение добавлено в существующий [manifest](../udf-linux-baseline.json).
VFS inode/page-cache ownership и write-side поля не переносятся в архивный ABI.

Исходный loader выбирает основной metadata inode либо mirror при невозможности
загрузить основной, требует short AD и допускает недоступный bitmap в read-only
режиме. Mapper использует `inode_bmap` для fragmented allocations и chains;
если основное mapping невозможно, лениво загружает mirror, сохраняя исходный
флаг однократной попытки. Ошибка чтения уже отображённого payload или его tag
не заменяется данными mirror: fallback относится к mapping, как в Linux.
Associated physical/sparable reference сохраняется независимо от того, куда
ICB перенаправил сам metadata inode. Передача base/offset остаётся исходной,
включая offset за границей выбранного metadata AD и составное sparable mapping.

Новый `metadata.c` предоставляет owned state/callback services; original C
использует существующее чтение FE/EFE и AD через тот же integration boundary,
что VAT. Общие inode/stream services перенесены из `vat.c` в `reader.c`, без
второй реализации. Hidden metadata role сохраняет raw allocation list и его
actual kind; information length не обрезает AD list до передачи `inode_bmap`.
Extended attributes удаляются из buffer view корректировкой его начала; EFE
allocation offset сохраняется. `CPartitionMap` владеет C state и callback
context; отдельная identity partition не смешивает inode keys разных maps.
Старое прибавление одного metadata extent остаётся только у standalone engine.

Extent cache сохраняет исходный lookup/update/clear control flow. Host buffer
services предоставляют `get_bh`/`brelse` ownership; последний cached buffer
освобождается вместе с inode. Kernel spinlocks заменены границей сериализованных
archive calls; дополнительная thread-safety одного `IInArchive` не заявляется.
Progress pointer очищается при выходе из Open, включая исключения. Позднее
чтение AD или mirror использует source stream и не обращается к уничтоженному
Open callback. Однократные E_ABORT/E_FAIL progress failures сохраняются при
optional primary/mirror/bitmap retries; обычная optional inode I/O ошибка
может привести к успешному mirror fallback.

Типовые адаптации C bodies: `loff_t` → `int64_t`, C11 initializer `{0}`, явный
portable ternary вместо GNU `?:` и внешняя linkage cache-clear для inode deleter.
На этом этапе typed AD нулевой длины отклонялся в metadata `inode_bmap`.
В исходном цикле такой AD не увеличивает byte count,
а чередование с continuation может обходить лимит consecutive indirections.
Этот случай проверяется через module CLI с timeout; native kernel его не
запускает. Ограничение затем заменено общим cycle guard ниже: конечные typed-zero
records снова поддержаны. Общие
checked physical additions остаются адаптацией предыдущих итераций.

15 дополнительных положительных случаев покрывают fragmented FE/EFE metadata,
AD после extended attributes, bootstrap/lazy mirror, исправный primary при
повреждённом mirror, доступный/недоступный bitmap, mapping за information length,
short AD chain, base/offset и sparable composition. Два из них уточняют прежнюю
избыточную проверку unused metadata map: Linux не отвергает весь архив заранее
из-за hole или unused allocation за partition bounds. Прежние строгие standalone
checks сохранены отдельными поведенческими проверками. 11 отрицательных случаев
проверяют отсутствующие/неверные inode, unsupported allocation kind, unmapped
data, повреждённые tags, cycles, source bounds и arithmetic wrap.

Нативный Linux `6.8.0-146-generic` подтвердил все 15 положительных вариантов
и отказы для десяти повреждённых образов. Typed-zero cycle исключён из native
run по причине выше; PASS для него относится к module regression. Mounts и
образы удалены. ABI проверяет I/O fallback, однократные E_ABORT/E_FAIL в loader
и AD mapping, отсутствие progress callbacks после Open, payload failure/retry,
Close/reopen/release и сохранённые streams. Backend/catalog consumers покрывают
fragmented, mirror, continuation и sparable scenarios.

Пять focused suites (`udf`, `udf_stream`, `upstream_baseline`, `runtime_cache`,
`runtime_package`) прошли на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12. Финальный remote receipt: `20261019T221324Z-e95dc361`.
Actual macOS CMake install прочитал все 87 положительных UDF variants,
сохранил source bytes и установил обе лицензии с точным исходным содержимым.
Staging, loop mounts и одноразовые образы удалены; использованы прежние
incremental build trees и compiler caches. Full/slow/UI/physical-device
acceptance не выполнялась. Проверка этой итерации закрывает описанный metadata
read scope, а не весь UDF audit или общую задачу форматов Linux.

## Завершение AD traversal и read policies

Повторно сравнены полные Linux `udf_next_aext`, `udf_current_aext`, `inode_bmap`,
short/long AD accessors, `udf_read_tagged`/`udf_read_ptagged` и их actual callers,
а также исходный flat AD parser 7-Zip. Linux ограничивает consecutive continuation
records внутри одного `udf_next_aext`; после возврата обычного AD счётчик
сбрасывается. В общем archive enumerator прежний огромный extent budget не
предотвращал практически неограниченный обход mixed chains. Готового общего
traversal guard в этих исходных bodies нет; flat 7-Zip reader вообще не обходит
такие chains. Поэтому добавлен узкий guard в существующий host-service boundary,
без нового AD decoder или замены алгоритмов Linux.

`reader.c::sevenzip_udf_next_aext` вызывает исходный `udf_next_aext` и выполняет
constant-space cycle detection по cursor AD, включая partition reference, logical
block, offset и различие initial/AED buffer. Guard используется общим enumerator
и `inode_bmap`, сохраняющим исходный extent cache. Память guard не растёт с числом
AD, и дополнительных I/O он не выполняет. Source comparison после изменения
подтверждает, что остальные ветви `inode_bmap` и исходные AD accessors сохранены.
Ранее добавленный blanket отказ для typed-zero metadata AD убран: конечные
нулевые records, включая длинные последовательности, читаются как в Linux.

Обход по-прежнему останавливается на logical EOF; неиспользованный cyclic tail
не исследуется. Detected cycles возвращают обычную archive error и не выдаются
за native malformed-input parity. Metadata mapping failure может восстановиться
через исправный mirror; source I/O failure и однократная отмена/ошибка progress
не превращаются в успех guard. AED tag fields по-прежнему не валидируются заново:
в Linux этот caller использует raw `sb_bread`, а длина AD проверяется по buffer.
Это отличается от обязательного tagged FE/EFE/IE чтения. Частичный short/long AD
tail завершает список и оставляет implicit hole; source bounds остаются прежними.

12 дополнительных конечных variants совпали по bytes с read-only native Linux
`6.8.0-146-generic`: short/long typed-zero AD, последовательность 64 таких records,
chains, active EOF, AED tag policy, partial tails, одинаковые cursor coordinates
в разных partitions и metadata с конечными zero records без пригодного mirror.
Mounts и образы удалены. Отдельный module-only вариант подтверждает восстановление
через mirror при mixed-zero cycle основного metadata. Семь ordinary-file cycles
(short/long, один/два nodes, prefix, записанные bytes и holes) должны завершаться
штатным exit code archive error, без timeout, crash или out-of-memory.
Циклические variants в native kernel не запускались.

ABI проверяет прежние stream/EOF/ownership contracts на всех положительных
вариантах; отдельные проверки передают I/O failure, однократные E_ABORT/E_FAIL
после частичного обхода и последующий успешный reopen. Backend consumers покрывают
finite chains, metadata zero records и damaged-primary mirror fallback.

Пять focused suites (`udf`, `udf_stream`, `upstream_baseline`, `runtime_cache`,
`runtime_package`) прошли на macOS arm64 и Linux x64; remote receipt
`20261019T222545Z-8cfa0a4a`. Windows x64/MSVC прошёл четыре suites в том же run;
`udf_stream` получил одиночный codec-discovery timeout в прежнем первом backend
сценарии до UDF операции, при 128 успешных проверках. Адресный повтор всего
`udf_stream` без изменения кода, build tree или deadline прошёл: receipt
`20261019T222826Z-cbf7cacf`. Это повторная проверка после наблюдаемого timeout,
а не исправление discovery или доказательство его причины.
Actual macOS CMake install прочитал все 100 положительных UDF variants,
сохранил source bytes и установил обе лицензии с точным исходным содержимым.
Staging и временные native images удалены; incremental builds и compiler caches
сохранены. Full/slow/UI/physical-device acceptance не выполнялась.

Полный UDF read audit остаётся открытым. В частности, ещё требуются
names/NLS, symlinks, специальные
inode и metadata/read policies. Проверка этой итерации не означает завершение
всех UDF возможностей или общей задачи форматов Linux. VFS, запись,
journal/mount ownership и page cache остаются вне archive read scope.

## Volume discovery, geometry и prevailing descriptors

С pinned Linux snapshot до реализации сравнены полные bodies и callers
`identify_vsd`, `udf_check_vsd`, `udf_verify_domain_identifier`,
`udf_load_fileset`/`udf_find_fileset`, `udf_lvid_valid`,
`udf_load_logicalvolint`, `handle_partition_descriptor`,
`get_volume_descriptor_record`, `udf_process_sequence`, `udf_load_sequence`,
`udf_check_anchor_block`, `udf_scan_anchors`, `udf_load_vrs`, `udf_sb_lvidiu`,
а также geometry/revision checks в `udf_fill_super` и `lowlevel.c`.
Эти 15 discovery/selection/helper bodies перенесены в `src/fs/udf/super.c`;
их структуры, constants, buffer ownership и зависимости включены в тот же
изолированный GPL format module. `lowlevel.c` сохраняет только regular-file
ветви last session/block: archive stream не предоставляет CD-ROM ioctls.
Default regular-file geometry перебирает 512/1024/2048/4096 bytes; VRS probe
используется также при автоматическом определении формата без расширения.
Резервные anchors, main/reserve sequences, VDP chains, prevailing LVD/PD,
LVID chains/revision и fileset tag/domain проходят через исходный C reader.

Полные Linux `udf_load_pvoldesc`, `udf_load_logicalvol`, `udf_load_partdesc`
изучены, но не скопированы целиком: их VFS publication, write allocation state
и NLS conversion не являются готовым archive API. Narrow adapters передают
выбранные validated descriptors единственному исходному 7-Zip parser в
`UdfIn.cpp`; `UdfDiscovery.cpp` соединяет его с C discovery. Legacy engine
сохраняет прежний decoder под условной компиляцией. Неиспользованные PD
игнорируются, активные partition bounds проверяются; geometry определяется
реальным чтением, а не противоречащим ему LVD field. Fileset читается полным
блоком с проверкой tag location, как в Linux. Выбранный last recorded block
передаётся также VAT loader. Hardware multisession и пользовательские mount
overrides session/anchor/lastblock/novrs не экспонируются stream interface.

После переноса повторно сравнены все 15 bodies. Адаптации: portable int64_t
для VSD byte offsets, запись выбранного anchor и копирование validated fileset
для archive projection, platform allocation/buffer services. LVID validation
дополнительно требует объявленное пространство для implementation-use revision
fields. Частичная длина integrity extent не переполняется при вычитании блока;
невалидный последний LVID освобождается, предыдущий пригодный сохраняется,
следующий extent копируется до освобождения buffer. Эти malformed-input fixes
сознательно строже/безопаснее исходной ветви и не заявляются native parity.
VFS mount/write accounting не перенесён. Прочие discovery branches сохранены;
обычные ошибки optional reads допускают fallback, однократные cancellation и
progress failures сохраняются. При полном I/O failure возвращается source error.

33 новых положительных discovery variants и 12 отрицательных проверены через
read-only native Linux `6.8.0-146-generic`: четыре block geometries, обе 4K VRS
strides, резервные/скорректированные anchors, main/reserve VDS, bounded VDP,
prevailing/tied/older records, рост PD table, fileset tag/length policies,
LVID chains и read revision. Mounts и images удалены. Три дополнительных
malformed LVID variants проверяются только модулем из-за исходных bounds и
ownership ошибок. CLI покрывает также auto-detection без format hint;
ABI проверяет bytes/EOF, interleaved streams, Close/reopen ownership, source
failures, однократные progress E_ABORT/E_FAIL и успешное повторное открытие.
Backend consumers включают шесть discovery scenarios.

Пять focused suites (`udf`, `udf_stream`, `upstream_baseline`, `runtime_cache`,
`runtime_package`) прошли на macOS arm64, Linux x64 и Windows x64/MSVC;
remote receipt `20261019T231108Z-75bbd86a`. Использованы прежние incremental
build trees и compiler caches. Full/slow/UI/physical-device acceptance
не выполнялась; source-port scope и runtime evidence указаны отдельно выше.

Actual macOS CMake install прочитал все 136 положительных UDF variants,
сохранил source bytes и установил обе лицензии с точным исходным содержимым.
Staging удалён. Полный UDF audit остаётся открытым: специальные inode/device EA
и оставшиеся metadata/read policies требуют
отдельной source comparison. Эта итерация закрывает default file-stream
discovery, а не physical-CD acceptance или все UDF возможности.

## CS0 namespace и symbolic links

До реализации сравнены полные `unicode.c`, `symlink.c`, соответствующие
`dir.c`/`namei.c` enumeration/lookup branches и inode read/type consumers,
а также 7-Zip `CDString`, `CFileId`, `ReadItem`, paths, comments,
`GetProperty`/`GetStream`/`Extract` и исходный extract callback, читающий
`kpidSymLink`. Из того же pinned Linux commit перенесены полные
`get_utf16_char`, `udf_name_conv_char`, `udf_name_from_CS0`,
`udf_dstrCS0toChar`, `udf_get_filename` в `src/fs/udf/unicode.c` и
`udf_pc_to_char` в `src/fs/udf/symlink.c`. Повторное сравнение всех шести bodies
подтверждает единственную type adaptation: Linux 16-bit `wchar_t` именуется
`fat_wchar_t`, как в существующем NLS module. Notices сохранены; write-side
CS0 encoding и VFS folio/getattr operations не копируются.

Используется прежний исходный `fs/nls/nls_base.c::utf32_to_utf8`; происхождение
зафиксировано в [FAT/NLS audit](fat-linux.md). Общие объявления portable NLS ABI
вынесены из FAT service header в `src/fs/nls/types.h`, без изменения layout
или алгоритмов FAT/ISO. Unicode archive API использует default Linux UTF-8
branch; mount charset overrides и обратная запись имён не экспонируются.
Сохранены обе converter branches, включая имеющийся NLS callback ABI.

Directory names теперь учитывают compression ID 8/16, surrogate pairs,
invalid character runs, slash/NUL replacement, CRC suffix, extension preservation
и Linux `UDF_NAME_LEN=254` bytes. Некорректные CS0 names и hidden FIDs
пропускаются при перечислении, как default Linux `readdir`; допустимые имена
из пробелов сохраняются. Archive enumeration не воспроизводит побочный отказ
native `namei` при поиске последующих entries после malformed CS0 record:
пригодные файлы остаются доступны по опубликованным archive paths.
Fixed dstrings для volume/fileset metadata используют исходную separate
informational conversion, её truncation/replacement и отсутствие filename CRC.

Symbolic links принимаются как отдельный UDF inode type. Narrow stream adapter
сохраняет политику `udf_symlink_filler`: максимум один filesystem block,
inline content либо полное raw чтение первого recorded data block без
синтеза holes. Исходный path-component decoder обрабатывает root/reset,
parent/current, origin и CS0 components; результат публикуется через
`kpidSymLink`, native symlink mode `S_IFLNK | 0777`, текстовый `GetStream`
и размер target в UTF-8 bytes, как native
`readlink`/symlink getattr. Готовые 7-Zip buffer streams и extract callback
обеспечивают ownership и создание ссылок через CLI. Backend сохраняет прежнюю
валидацию links при извлечении; technical-listing adapter дополнительно читает
поле Mode, чтобы не принимать пустой symlink target за обычный файл.
Эта узкая Qt boundary не имеет готового upstream ArchiveEntry parser.
Существующая regression извлечения покрывает пустую ссылку и порядок Mode/Attributes.
Отмены/ошибки source reads и progress
не скрываются; malformed target возвращает archive error.

33 новых положительных fixtures покрывают имена, hidden/invalid entries,
volume dstrings и inline/external/long-AD/AED symlink data. Safe native checks
на Linux `6.8.0-147-generic` подтвердили имена и target bytes; для malformed
CS0 отдельно сравнен `readdir`, без обещания lookup parity. Для dstring fixtures
native mount/data проверены, а текст archive comment проверен через CLI;
kernel volume-label publication не измерялась. Четыре повреждённых symlink
варианта отвергнуты native `readlink`; пять отвергаются модулем.

Пятый вариант, symlink с unallocated hole, при первом native run вызвал
kernel oops в `6.8.0-146-generic::read_cache_page`. Он больше не запускается
в ядре; его module rejection не заявляется native parity. Выделенный test
guest штатно перезагружен, зависший fixture mount освобождён, temporary images
удалены. Загрузилось ранее установленное ядро 6.8.0-147; matching
linux-cloud-tools восстановил KVP/VSS discovery после reboot. Build trees и
compiler caches сохранены. Остальные native checks завершены после recovery.

CLI проверяет точные paths/bytes, комментарии и symlink properties; на macOS
и Linux — actual extraction трёх safe relative links. Windows проверяет
properties/streams, без native link creation, требующего отдельной privilege.
ABI дополнительно проверяет EOF, retained streams после Close/reopen/release,
source errors, однократные progress E_ABORT/E_FAIL именно после symlink data read
и последующее успешное открытие. Backend consumers проверяют Unicode,
CRC-mangled и предельные имена через list/extract/extractSourceEntry.

Десять focused suites (`udf`, `udf_stream`, `fat_module`, `iso`, `iso_stream`,
`archive_backend`, `source_extract_backend`, `upstream_baseline`,
`runtime_cache`, `runtime_package`) прошли на macOS arm64, Linux x64 и Windows
x64/MSVC; remote receipt `20261019T232841Z-36674133`. До дополнения empty-link
consumer также прошли восемь filesystem/runtime suites на трёх платформах;
финальный run включает связанные backend regressions и новый symlink mode.

Actual macOS CMake install прочитал все 169 положительных UDF variants,
сохранил source bytes и установил обе лицензии с точным исходным содержимым.
Staging, native mounts и временные images удалены. Build trees и compiler
caches переиспользованы; full/slow/UI/physical-device acceptance не выполнялась.

Полный UDF audit остаётся открытым: оставшиеся directory/inode/read policies требуют
следующей интегрированной source comparison. Standalone decoder и physical-CD
границы остаются прежними; эта итерация их не закрывает.

## Inode attributes, special files и timestamps

Повторно сравнены полные Linux `udf_read_inode`, `__udf_iget`,
`udf_alloc_i_data`, `udf_convert_permissions`, `udf_get_extendedattr`,
`udf_disk_stamp_to_time`, mount defaults/credential flags, ECMA FE/EFE/EA
structures и их callers. Сопоставлены исходные 7-Zip `CItem::Parse`, ICB/FID
consumers, hidden VAT/metadata reader, properties, streams и extraction.
Сохранён pinned commit из начала audit; [manifest](../udf-linux-baseline.json)
включает `udftime.c` и общий dependency `kernel/time/time.c`.

В `src/fs/udf/inode.c` дополнена оставшаяся часть исходного `udf_read_inode`;
`udf_alloc_i_data` и `udf_convert_permissions` перенесены целиком. Теперь native
C body проверяет FE/EFE/USE, allocation lengths и inline size, выбирает inode
type, права, владельцев, link count, blocks, timestamps и named-stream metadata.
Прежние независимые C++ type whitelist и требование выравнивания EA по четыре
байта убраны из Linux branch. Typed attributes передаются вместе с validated
FE/EFE единственному 7-Zip archive projection. Обычный zero-link inode
отвергается; hidden VAT/metadata inode получает native fallback link count.
Directory link count увеличивается без прежнего ограничения UInt16 property.

После реализации повторно сравнены полные bodies. Адаптации `udf_read_inode`:
не публикуются VFS inode/file/page-cache operations и highmem settings;
успешный FE buffer удерживается до archive projection, включая USE early return.
Owned `i_data` и buffer освобождаются wrapper после копирования, в том числе
при ошибках. Serialized calls заменяют VFS credential locks; kernel operation
tables, inode-cache publication и write paths не реализуются фиктивными handlers.
`__udf_iget` cache/role consistency остаётся отдельной частью read-policy audit.

Permissions и setuid/setgid/sticky flags преобразуются исходным helper.
Mode, UID/GID, links и device major/minor доступны через стандартные archive
properties. Default Linux profile использует overflow UID/GID 65534 для disk -1;
host sysctl/user namespace и mount overrides не экспонируются archive API.
`udf_get_extendedattr` перенесён в `misc.c`; сохранены EAHD identity/location,
поиск type/subtype, порядок attributes и native internal dev_t normalization.
Невостребованные EA fields не валидируются дополнительно. Защита malformed
границ дополнена: EA header должен помещаться в declared EA, deviceSpec — в
declared attribute; offset scan не переполняется. Два таких варианта проверены
только модулем, без заявления native malformed-input parity. Windows shim
отображает отсутствующий в MSVC `ESTALE` в обычную archive input error.

Block/character devices, FIFO и socket перечисляются с native metadata.
Их AD не интерпретируются как обычное содержимое: нулевой размер даёт пустой
archive stream; ненулевой information length сохраняется в properties, а data
stream возвращает E_NOTIMPL. Host device/FIFO/socket не открывается. Private
metadata inode без обычного file type не получает публичный regular stream;
внутренний hidden metadata backing reader остаётся подключён. Создание device
nodes и native special-file I/O не входят в archive read boundary.

`src/fs/udf/udftime.c` содержит исходный `udf_disk_stamp_to_time` без изменения
body. Переиспользован ранее перенесённый Linux `mktime64`; его include теперь
ограничен portable integer types, вместо зависимости от ISO service header.
Все UDF archive/item time properties используют этот converter: timezone type,
signed/unspecified offset, fractional sanitization, leap second и нормализация
date fields соответствуют Linux. 7-Zip FILETIME adapter ограничивает результат
диапазоном публичного типа и сохраняет microsecond precision. Write-side
`udf_time_to_disk_stamp` не переносится. Оригинальный LGPL-2.0+ notice сохранён;
pinned Linux LGPL-2.0 text устанавливается вместе с прежними GPL/7-Zip notices
на Unix/macOS и Windows. GPL/LGPL C остаётся в отдельном format module.

32 новых положительных variants сверены с read-only native Linux
`6.8.0-147-generic`, включая закрытые права (data check от root), high/overflow
owners, 65535/65536 links, regular type aliases, FE/EFE device EA, subtype/chain,
EAHD tag policy, unaligned EA, hidden zero-link metadata и timestamps.
Десять invalid inode/EA variants отвергнуты native stat и модулем. Отдельный
special inode с ненулевой длиной сравнен через stat без открытия устройства;
модуль проверяет его metadata-only контракт. Mounts и images удалены.

CLI проверяет точные bytes и published properties; ABI — mode/owner/link/device
values, FILETIME quantums, empty и unavailable streams, EOF и ownership после
Close/release. Backend consumers дополняют coverage для type alias, unaligned EA
и rewritten EFE. Прежние cancellation/I/O/mirror checks остаются включены.
Семь focused suites (`udf`, `udf_stream`, `iso`, `iso_stream`,
`upstream_baseline`, `runtime_cache`, `runtime_package`) прошли на macOS arm64,
Linux x64 и Windows x64/MSVC. Linux receipt `20261019T234537Z-ae6f25af`;
Windows после исправления compile error ESTALE — `20261019T234653Z-f8965613`.
Общий ISO consumer включён из-за переиспользования `mktime64`.
Actual macOS CMake install прочитал все 201 положительный UDF variant,
сохранил source bytes и установил три notices с точным исходным содержимым.
Staging удалён; incremental builds/caches сохранены. Full/slow/UI/physical-device
acceptance не выполнялась.

Полный UDF audit не закрыт: directory FID, partition-map и оставшиеся cache/read
policies, включая non-FE inode publication, требуют следующей сверки.
USE обрабатывается C service как free-space descriptor, но не публикуется
7-Zip projection как пользовательский inode. Standalone/VFS/physical-device
границы прежние.
