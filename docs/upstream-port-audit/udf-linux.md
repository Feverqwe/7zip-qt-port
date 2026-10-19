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

Полный UDF read audit остаётся открытым. В частности, ещё требуются
volume discovery/block geometry,
полная metadata partition/mirror mapping, names/NLS, symlinks, специальные
inode и metadata/read policies. Проверка этой итерации не означает завершение
всех UDF возможностей или общей задачи форматов Linux. VFS, запись,
journal/mount ownership и page cache остаются вне archive read scope.
