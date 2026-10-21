# HFS/HFS+: read modules, forks и metadata

Работа входит в [общий scope актуализации по Linux](filesystems-linux.md).
Исправлено существующее чтение HFS+/HFSX и их HFS wrappers; добавлены Linux
Unicode read conversion и разрешение hardlinks через backing inode.
Актуализированы permission defaults, catalog/thread validation и FinderInfo.
Подключены Linux timestamp rollover и metadata специальных inode.
Добавлено чтение plain classic HFS через Linux C read paths в том же GPL-модуле.
Read-side source scope семейства HFS сверён по pinned Linux; границы archive/
platform adaptation и runtime evidence отделены от полноты переноса исходников.

## Источники и source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сравнены полные `CPP/7zip/Archive/HfsHandler.cpp`, `HfsHandler.h`,
`CPP/7zip/Common/LimitedStreams.cpp`/`.h`, используемые stream helpers и ABI.
Адаптированные `.cpp` находятся под исходными путями в `src/`, включены
в [baseline](../upstream-baseline.json) и overlays обоих поставляемых engines.
У `LimitedStreams.cpp` один владелец; прежняя exact-копия удалена, CMake
потребители используют адаптацию с исходным заголовком. Полное форматирование
C++ учитывается отдельно от функционального сравнения.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
[Manifest](../hfs-linux-baseline.json) фиксирует reference snapshot HFS/HFS+
и общий `include/linux/hfs_common.h`; наличие файла в snapshot не означает
полного переноса или проверки всех его ветвей.
Для выбранного сценария прочитаны полные read bodies и необходимые callers/types:
`fs/hfsplus/extents.c` (extent lookup/cache/get_block),
`wrapper.c:hfsplus_read_wrapper`, `btree.c:hfs_btree_open`,
`bnode.c` (node read/find/create), `bfind.c` (поиск и проверка catalog records),
`inode.c:hfsplus_inode_read_fork`/`hfsplus_cat_read_inode`,
`dir.c:hfsplus_lookup`/readdir, disk-name conversion в `unicode.c`, inline
чтение `xattr.c:__hfsplus_getxattr`, определения общего on-disk header.

Первое исправление fork mapping использовало Linux как эталон без копирования
GPL bodies. Для Unicode/hardlinks теперь переносится оригинальный C-код
`fs/hfsplus/unicode.c`: `case_fold`, `hfsplus_strcasecmp`/`hfsplus_strcmp`,
`hfsplus_compose_lookup`, Hangul composition, compatibility mapping,
`hfsplus_uni2asc` и его regular/xattr wrappers. Алгоритмы, ветвления и lookup
таблицы сохранены; `tables.c` отличается только include. Decompose table
сохранена вместе с исходным модулем таблиц; inverse conversion и dentry
hash/compare VFS services не копируются. `inline` у экспортируемых wrappers
убран для portable C11 linkage; исправлена знаковость одного pointer conversion.

Из `dir.c:hfsplus_lookup` перенесён hardlink loop: type/creator, наличие private
directory, проверка create date, lookup `iNode%d` и возврат к исходному inode
при цепочке ссылок. VFS structs, B-tree service calls и dentry publication
заменены узким adapter к уже прочитанному 7-Zip catalog. Оригинальные условия
и однократный переход сохранены. Catalog lookup и распознавание private directory
используют исходные disk names и Linux casefold/binary comparator с учётом HFSX.
Write/delete/rename и inode
publication не переносятся; archive adapter возвращает исходный catalog index.

Дополнительно перенесены read bodies `inode.c:hfsplus_get_perms`,
`bfind.c:hfsplus_brec_read_cat` и `xattr.c:hfsplus_getxattr_finder_info`.
Сравнены их callers, catalog thread lookup в `catalog.c:hfsplus_find_cat`,
типы в общем on-disk header, defaults в `options.c` и FinderInfo list/get paths.
VFS inode/B-tree read services заменены уже прочитанными bounded catalog records;
проверки типов, размеров, permission branches и 32-byte payload сохранены.
Для архивного чтения приняты стабильные defaults UID/GID `0`, umask `0022`;
mount credential overrides не предоставляются. Mutable VFS inode flags
не публикуются: исходные BSD flags доступны как item characteristics.
FinderInfo берётся из catalog, включая приоритет над одноимённым attribute-tree
record; ненулевой payload перечисляется как alternate stream. Это соответствует
Linux special xattr read/list, с исходным archive namespace без префикса `osx.`.
Дополнительная проверка thread ID/parent/name/type выполняется сразу для всего
перечисленного каталога. Linux ищет thread при обращении к inode; eager отказ
в `Open` и проверка соответствия типа — адаптация архивного интерфейса.

Read projection `inode.c:hfsplus_cat_read_inode` сохраняет permission/link-count
и special-inode ветви; чтение fork и dates уже выполняет существующий handler,
а VFS operation tables и публикация inode не копируются. Device numbers читаются
как raw internal `dev_t` из `permissions.dev`, как в `init_special_inode`;
используются исходные `MINORBITS`/`MAJOR`/`MINOR` из `include/linux/kdev_t.h`.
Disk encoder `new_encode_dev` здесь неприменим. В archive properties major/minor
публикуются только для character/block devices, не для FIFO/socket.
`hfsplus_fs.h:__hfsp_mt2ut` перенесён в C с исходным unsigned rollover:
Linux read range — 1970–2106, disk zero означает 2040 год. C++ adapter переводит
секунды в существующий FILETIME ABI и сохраняет metadata backing inode у aliases.
Создание файла/тома остаётся archive extension: Linux VFS не возвращает birth time;
исходная local-time трактовка volume creation не заменяется VFS-семантикой.
Новые C helpers подключены только к GPL-модулю; standalone сохраняет исходную
датировку 7-Zip и не публикует новые device properties.

Этот Linux C компилируется только в отдельном GPL-2.0-only format module
`sevenzip_hfs`, без unRAR. Его LGPL closure — исходные ArchiveExports,
Common/Windows/stream helpers и handler, zlib/Deflate и BSD LZFSE/LZVN decoder.
Исходный `HfsHandler.h` переиспользуется без изменений. Модуль и GPL/LGPL/BSD
notices устанавливаются с приложением; registration HFS основного external-codec
engine отключена, чтобы единственным владельцем формата был модуль.
Standalone сохраняет исходный scope HFS+/HFSX/wrappers и явно отказывается от hardlink stubs,
вместо успешного возврата пустого файла; GPL bodies с ним не линкуются.

### Plain classic HFS

Исходный `HfsHandler.cpp:Open2` явно отказывает plain HFS; пригодного classic
reader в 7-Zip нет. До интеграции сравнены Linux `fs/hfs` MDB/B-tree, bnode/brec/
bfind, extent, catalog, inode, name conversion, directory и xattr read bodies
со всеми вызываемыми типами. `include/linux/hfs_common.h` перенесён exact;
portable C types/packing обеспечиваются узким header adapter.
Переданы полные read search bodies из `bfind.c`, record helpers из `brec.c`,
catalog/extent comparators и исходная `string.c` case-order table с `hfs_strcmp`.
Пути и символы сохранены; cast для pointer arithmetic нужен MSVC C.

`mdb.c:hfs_mdb_get`, `btree.c:hfs_btree_open`, `bnode.c:hfs_bnode_find`,
`inode.c:hfs_inode_read_fork`/`hfs_read_inode` и extent read/cache/mapping
адаптированы к byte transport и существующим bounded archive streams.
Page/folio services, inode/hash caches и locks заменены локальными buffers
на время `Open`; B-tree search, высоты/offset checks и predecessor lookup
extent records сохранены. Добавлены bounds source/volume/record, проверка
полного allocation mapping, leaf order/count/links и циклов родителей.
Catalog directory threads проверяются через перенесённые
`hfs_cat_find_brec`/`hfs_cat_validate_found_cnid`; file threads остаются optional.
Mapping включает data/resource forks и overflow extents самого catalog file.
Неприменимый к classic kernel bootstrap overflow самого extent-tree не добавляется:
Linux также требует его mapping в трёх исходных extents.

Read projection сохраняет Linux defaults file/dir masks `0133`/`0022`, lock bit,
inode identity и link count. UID/GID и timezone defaults фиксированы на `0`;
archive clock использует общий unsigned rollover, соответствующий
`hfs_fs.h:__hfs_m_to_utime` при UTC. Creation остаётся archive extension.
`attr.c:__hfs_getxattr` сопоставлен с публикацией точных четырёх bytes
`hfs.type`/`hfs.creator` как alternate streams. Resource fork публикуется
через исходный archive namespace `:rsrc`.

`trans.c:hfs_mac2asc` сохраняет default byte-preserving branch и disk slash →
colon. Каждый byte проецируется в один UTF-16 code unit; portable path builder
затем заменяет separators на `_`. Mount-selected NLS/credentials/timezone,
session/partition selection, загрузка/обновление allocator bitmap и alternate-MDB
services не переносятся: fork mapping не обращается к bitmap, запись отсутствует.
Это archive/platform boundaries; выбор раздела остаётся у
существующих partition handlers. Сырые default Linux names проверены отдельно
от Unicode presentation. Этот перенос не заявляет Mac Roman conversion.

Новый C boundary владеет transient reader state и передаёт errors/cancellation
в COM callback. Handler переиспользует один `BuildRefs`, extent stream,
extraction и retained inline-attribute streams для обоих семейств. Allocation
size хранится в bytes, чтобы classic 1536-byte blocks не превращались в shift.
Все новые C bodies входят только в `sevenzip_hfs`; standalone явно не принимает
plain classic HFS, его прежнее чтение HFS+/HFSX/wrappers проверяется отдельно.

Сохранены исходные классы, eager leaf enumeration,
каталог, alternate streams, decmpfs decoder и extraction callbacks. Полный
kernel B-tree index traversal не переносится: archive handler перечисляет leaves,
а не предоставляет VFS lookup. Проверки высоты, backlinks, offsets и header
относятся к читаемым nodes; это не обещание полной проверки всех index/map nodes.
Зависимости — исходные portable Common/Windows helpers, StreamObjects/Utils,
LimitedStreams и существующие zlib/LZVN decoders; новый decoder не добавляется.

NLS encoding — platform boundary: C converter выдаёт UTF-16 units в исходный
7-Zip string model, сохраняя surrogate pairs. Linux composition и NUL → U+2400
применяются к regular names; xattr conversion следует отдельной kernel ветви.
Недопустимые длины и NUL в xattr name отклоняются вместо усечения строки.
Для переносимого archive path slash/colon/backslash в компоненте заменяются
`_` исходным path builder, чтобы имя не превращалось в ADS syntax. Это
presentation adaptation; Linux VFS использует `:` для disk slash. Metadata
private directory остаётся видимой как в исходном archive handler; Linux VFS
скрывает её из readdir.

## Исправленное поведение

- Continuation extents одного fork сохраняются при переходе между leaves;
  суммы блоков не заворачиваются, порядок file IDs проверяется.
- Логический EOF отделён от allocation size. Допустимы preallocation, отдельные
  extents после EOF и пустой файл с выделенными блоками. Metadata readers читают
  логический размер и требуют полного mapping вместо неинициализированного хвоста.
- Data/resource/attribute forks проверяют весь mapping относительно тома
  и источника, включая неиспользуемый allocation tail. Извлечение и `GetStream`
  используют один исходный extent stream.
- Ненулевые boot blocks допустимы; HFS wrapper распознаёт также HFSX.
  Embedded extent и размер вложенного тома проверяются до чтения каталога.
- Повреждённые leaf/header структуры, несогласованные root depth/последний leaf/
  число записей и циклы родителей каталога отклоняются;
  ошибочный или отменённый `Open` очищает опубликованный каталог.
- Потоки forks читаются независимо при чередовании обращений и сохраняют
  источник после `Close/ReOpen`/освобождения handler. Inline attributes возвращают
  самостоятельную копию через публичный `GetStream`.
- Общий `CExtentsStream` восстанавливает физическую позицию перед чтением,
  сообщает ранний EOF как ошибку и не допускает переполнения signed seek.
  Другие классы из `LimitedStreams` функционально не изменены.
- Hardlink names сохраняют собственный путь и получают data/resource forks,
  inline attributes, compression header, timestamps, owner/group и inode
  identity backing file. Missing/corrupt backing inode сообщает DataError,
  без успешного пустого файла; kernel date/chain guards сохраняются.
- Публичные properties сообщают UID/GID, inode/link count и symlink target;
  symlink payload читается через тот же bounded fork stream.
- Нулевой disk mode получает Linux defaults, несовместимый тип file/directory
  отклоняется. Права и BSD flags hardlink aliases берутся у backing inode.
- Catalog FinderInfo доступен как независимый 32-byte attribute stream для
  файлов, каталогов и hardlink aliases; его bytes сохраняются после `Close`.
- Некорректные размеры/типы catalog records и несогласованные thread records
  отклоняются с очисткой опубликованного каталога.
- Даты после rollover 2040 года читаются в Linux эпохе; нулевые access/modify/
  change timestamps возвращают дату, а не отсутствующее свойство. При извлечении
  сохраняется mtime вплоть до конца Linux диапазона в 2106 году.
- Character/block devices сообщают исходные major/minor. Regular file link count
  и единичные ссылки специальных inode определяет перенесённый Linux read path.

## Inline attribute records

Следующая сверка полных `xattr.c:__hfsplus_getxattr`, list/get callers,
`attributes.c:hfsplus_find_attr`/`hfsplus_attr_bin_cmp_key` и disk types
обнаружила расхождения с `HfsHandler.cpp:LoadAttrs`. Inline value теперь читает
перенесённый C `__hfsplus_getxattr`: 16-битная length, предел Linux и правила
размера output buffer. Reserved bytes не участвуют в длине и не требуют нуля.
Большие inline values вне kernel limit сохраняют существующий SDK read path
с его 32-битной длиной и строгими header/bounds checks. Это расширение SDK,
как decmpfs, а не заявленная возможность Linux xattr API.
Ненулевой `startBlock` не скрывает inline attribute: Linux comparator использует
CNID и name. Fork/extent attributes остаются прежним расширением SDK; Linux
сам не читает их values. Неизвестный record type не принимается за inline value.

Adapter получает уже найденный bounded record вместо inode/B-tree lookup и
slab allocation. Проверка границ header/value относительно record дополняет
Linux checks, чтобы archive reader не копировал соседние metadata bytes.
Disk layout берётся из единственного exact `include/linux/hfs_common.h`, который
теперь переиспользуют оба семейства; локальные дубликаты Unicode structs удалены.
VFS namespace/capability policy, запись и mount services не переносятся.
Standalone сохраняет прежний LGPL reader; GPL helper подключён только к модулю.

CLI проверяет reserved/padding/startBlock, empty/maximal/large values и отказы для
short/unknown records. Старый модуль воспроизвёл три ошибочных отказа.
ABI дополнительно проверяет независимость и lifetime таких attribute streams,
backend — реальные SourceEntry bytes.

Focused6 (`hfs`, `hfs_stream`, runtime packaging, extraction security и два
provenance suites) прошли на macOS ARM64, Linux x64 и Windows x64/MSVC.
Linux receipt `20261020T163041Z-f8ab4173`, Windows
`20261020T163013Z-e6573722`. Финальное сохранение больших SDK values проверено
всеми шестью suites на macOS (14,91 с) и четырьмя HFS/provenance suites на
обоих runners (`20261020T163222Z-4bc7f41a`). Непосредственно затронутые C++
участки отформатированы; incremental build trees и compiler caches сохранены.

Native Linux hfsplus `6.8.0-147-generic` прочитал точные bytes пяти вариантов
(reserved/padding/startBlock/empty/kernel limit), вернул EIO для неизвестного
типа и значения выше kernel limit: `20261020T163011Z-3aef9b8d`. Последнее
модуль продолжает читать как SDK extension. Все семь read-only images сохранили
SHA256; temporary images/mounts и одноразовый review script удалены. Short record
не передавался старому kernel; archive boundary проверяет его собственные bounds.
Версия running kernel отличается от pinned source baseline. Full/slow gate,
UI и физические устройства не проверялись; общий HFS read audit остаётся открытым.

## Page-backed symlink targets — 2026-10-21

До изменений сравнены полные `inode.c:hfsplus_symlink_inode_operations`,
`hfsplus_cat_read_inode`, fork mapping, `fs/namei.c:__page_get_link`,
`page_get_link`/`page_put_link`, `include/linux/namei.h:nd_terminate_link`,
delayed-call/error-pointer types и SDK property/stream/extraction consumers.
Все 58 source/license hashes расширенного HFS manifest проверены.
Linux читает target из первой страницы, ограничивает его `PAGE_SIZE - 1` и
останавливается на первом NUL. Прежняя archive property отклоняла любой NUL,
пустой fork и размер больше 64 KiB, а остальные targets читала целиком.

Исходное тело `page_get_link` перенесено без изменения в C `src/fs/namei.c`;
повторная source-сверка подтвердила все его ветви. Оно переиспользует уже
существующий authoritative `nd_terminate_link` и exact delayed-call header.
`__page_get_link` — узкий boundary к заранее прочитанной owned странице:
VFS page cache/RCU, highmem и folio reference services не копируются и не
эмулируются. Archive owner обнуляет страницу и заполняет её существующим SDK
fork stream; source failure/abort возвращаются до вызова C helper. Новый parser
fork или extent mapping не добавляется. Error-pointer representation использует
`uintptr_t` на LLP64. Notices сохранены, новые C bodies линкуются только с
отдельным GPL HFS-модулем; standalone сохраняет прежнюю symlink projection.

Portable page service фиксирован на 4 KiB, как проверенное native x64 ядро.
Другие Linux PAGE_SIZE configurations этой проверкой не установлены.
Target property теперь возвращает пустую строку, NUL prefix или первые
4095 bytes. Unicode conversion остаётся прежним SDK boundary. Public raw fork
stream сохраняет все исходные bytes, размер, independent/retained lifetime;
это отдельная archive projection от Linux `readlink`. ArchiveBackend получает
нормализованную target metadata, но сохраняет запрет извлечения archive links:
ограничение не ослаблялось ради нового reader.

Шесть новых images покрывают пустой target, NUL после prefix и в первом byte,
границу 4095 bytes, полный 4096-byte page и 65537-byte fork. Fixtures выделяют
реальные свободные extents и обновляют allocation bitmap и обе volume headers.
ABI проверяет target, source E_FAIL/E_ABORT и retry, полный raw fork после Close.
Official dynamic CLI проверяет точную target metadata; ArchiveBackend — listing,
обычные file/stream consumers и отказ создать archive link без output artifact.
Standalone не заявляет эти Linux read semantics. Windows backend test
нормализует native path separators перед сравнением имени.

`hfs`/`hfs_stream` PASS на macOS arm64/Qt 6.12 (22,38 с) и Linux x64/Qt 6.12
(23,23 с), receipt `20261021T021722Z-053e1abe`; Windows x64/MSVC/Qt 6.12
прошёл CLI suite, но новый backend assertion ожидал POSIX separator.
После исправления только тестового сравнения финальный `hfs_stream` PASS
на всех трёх ОС: macOS 21,90 с, Linux 15,22 с, Windows 26,78 с, receipt
`20261021T021913Z-03dc8324`. Source/ABI/CLI checks и security policy не
менялись после предыдущего запуска. Оба SDK runtime подготовлены с общим
исходником; standalone не получил GPL C dependencies.

Native hfsplus `6.8.0-147-generic` подтвердил все шесть точных `readlink` targets
через read-only loop mounts, receipt `20261021T021831Z-f40fbf21`: 0/5/0/4095/
4095/4095 bytes. Images остались неизменными, mounts и temporary fixtures
удалены. Версия running kernel дополняет сравнение pinned source. Incremental
builds и compiler caches сохранены; full/slow suites, UI, physical devices и
новый install runtime не проверялись. Остальной HFS read audit остаётся открытым.

## Volume discovery, wrappers и headers — 2026-10-21

До изменений сравнены полные `wrapper.c:hfsplus_read_mdb`,
`hfsplus_read_wrapper` и его transport/session callers, version/read-only
ветви `super.c:hfsplus_fill_super`, raw/shared types и SDK recognition/Open,
physical-size, fork-stream и extraction consumers. Прежний SDK путь обрабатывал
один wrapper, не проверял SLOCK/SPARED и secondary header и требовал пары
H+(4)/HX(5), хотя Linux допускает версии 4–5 для обеих сигнатур.

Оригинальное тело `hfsplus_read_mdb` передано без изменения в C
`src/fs/hfsplus/wrapper.c`; signature принимает byte pointer вместо GNU void
arithmetic, а static linkage снята для узкого C boundary. Endian/unaligned
services заменены portable adapters; constants и packed disk types остаются
authoritative shared Linux header. Отдельный `src/fs/hfsplus/super.c` сохраняет
version guard из `hfsplus_fill_super` с теми же constants и failure branch;
VFS setup/cleanup не копируются. Все 58 baseline hashes и полный переданный
MDB body повторно сверены. Новые C dependencies входят только в GPL-модуль.

SDK transport повторяет исходный Linux reread до финального HFS+/HFSX header,
накапливает относительное начало каждого embedded volume и сохраняет внешнюю
physical-size projection. Существующие full-fork readers используют итоговый
offset. Archive bounds дополнительно требуют прогресса offset, полного header,
попадания вложенного extent/allocation area в источник и enclosing extent;
нулевой шаг не запускает бесконечный перечитанный wrapper. Progress callback
проверяется между уровнями, ошибки/отмена идут через прежний Open cleanup.
Plain classic HFS сохраняет свой отдельный reader у начала выбранного образа.

Secondary signature проверяется после discovery, как в Linux; версия secondary
не подменяет primary version и не является условием отказа. Для wrapper выбирается
его объявленный extent; для bare archive — объявленный HFS volume extent.
Последнее сохраняет archive images с данными за концом файловой системы и
отличается от размера целого block device при VFS mount. Поддерживаемая primary
version проверяется C guard, HFSX определяется сигнатурой. Dynamic registration
и IsArc используют те же правила, поэтому H+(5)/HX(4) доступны и автоматическому
открытию. Standalone сохраняет прежние SDK пары/одиночный wrapper; session,
partition discovery, mount/write services не переносятся в этот adapter.

Positive fixtures покрывают nested HFS+/HFSX, 1536-byte wrapper allocation blocks,
изменённую secondary version и обе ранее отвергавшиеся пары primary signature/
version. Negative controls проверяют SLOCK/SPARED, secondary signature, primary
версии вне диапазона, нулевой шаг и обрезанный enclosing extent. ABI проверяет
Open cleanup/cancellation и retained independent forks, CLI — точные bytes.
Штатный `-tHFS:s0` отключает поиск внутреннего архива для этих controls:
успешное восстановление inner HFS+ не выдаётся за принятие wrapper. Источник
параметра сверён в SDK `OpenArchive.cpp:ParseTypeParams`/MaxStartOffset consumer.
ArchiveBackend отдельно проверяет обычное automatic listing/extraction
nested volume и H+(5)/HX(4), включая Unicode/alternate streams и source retention.

Финальные `hfs`/`hfs_stream` PASS на macOS arm64/Qt 6.12 (26,43 с), Linux x64
(28,70 с) и Windows x64/MSVC/Qt 6.12 (44,41 с), receipt
`20261021T023218Z-75ab163e`. Оба SDK runtime подготовлены с общим исходником;
GPL C остаётся только в format-модуле. Ранний Windows batch до последних
header/version изменений получил 10-second timeout в прежнем negative fork
control; причина не установлена. Финальный batch прошёл с прежними timeout
и compiler-cache настройками.

Native hfsplus `6.8.0-147-generic` подтвердил одиннадцать read-only controls,
receipt `20261021T023428Z-a8c1451a`: шесть вариантов прочитали точные bytes
двух обычных файлов, пять неверных flags/signature/version отклонены mount.
Images остались неизменными; mounts, temporary fixtures и review helper
удалены. Нулевой шаг и обрезанный parent extent проверены только через archive
ABI/CLI; циклический reread не передавался старому kernel. Версия running kernel
дополняет pinned source comparison, а не заменяет его.

Это selected read-discovery scope, не перенос всего `hfsplus_fill_super` или
завершение общего HFS read audit. Incremental builds и compiler caches сохранены;
full/slow suites, UI appearance, physical devices и новый installed runtime
не входят в эту итерацию.

## Проверки и оставшийся scope

Локальный fixture создан Apple `hdiutil` из сгенерированных данных;
[происхождение](../../tests/fixtures/hfs/README.md). Проверяются bytes обычных
файлов, Unicode-имени, resource fork и inline attribute в обоих engines,
preallocation/overflow leaves, boot blocks, HFSX/wrappers и ошибки metadata.
ABI проверяет чередование потоков, lifetime, I/O failures, EOF, seek boundaries,
отмену и повторное открытие после ошибки.
Через `ArchiveBackend` проверяются listing, выбранное извлечение, SourceEntry
для resource/inline streams и Unicode-файла, read-only возможности формата
и отказ от повреждённого fork.

Focused suites прошли на macOS ARM64, Linux x64 и Windows x64: HFS CLI/ABI/backend,
provenance и прямые потребители общего extent stream (FAT, ext, cramfs,
SquashFS и существующие extraction backend suites). Последние изменения
HFS дополнительно проверены его suites и provenance на трёх ОС. Unicode/hardlink
модуль прошёл CLI/ABI/backend, runtime packaging, codec creation и extraction
security suites на тех же платформах. Проверяется как отдельное deployment
формата, так и immutable runtime copy, используемая приложением.
Приложение собрано на macOS. Полный gate, slow suites, UI и физические
устройства в эту проверку не входили.

Linux VFS `hfsplus`, kernel `6.8.0-146-generic`: восемь корректных вариантов
прочитаны через read-only loop mount; bytes обоих файлов и inline attribute
совпали, образы остались неизменны. Это runtime evidence установленного ядра,
отдельное от source comparison закреплённой более новой ревизии. Resource fork
проверен через engines и ABI, отдельная VFS-проверка его bytes не заявляется.

Unicode/hardlink сценарии проверяются также на отдельном native-created
fixture с тремя hardlink names, decomposed Latin/Hangul, surrogate pair,
symlink и наследуемыми forks/xattr. CLI/ABI/backend проверяют composition,
NUL suffix, casefold lookup, date/chain/missing/bounds cases и 32-bit metadata.
Десять вариантов, включая HFSX binary tree и case-only private-directory name,
дополнительно сопоставлены с Linux VFS: обычные bytes,
inode/link count/UID/GID, symlink и Unicode xattr совпали. Kernel NLS показывает
emoji как `??`, а lookup возвращённого имени не проходит; для него VFS bytes
не заявляются. Сохранение исходного emoji и его bytes проверено через engine/ABI.
Это обоснование UTF-16 adapter, а не обход ошибки decoder.
Zlib и native-produced LZVN decmpfs payloads дополнительно читаются через
CLI и ArchiveBackend/SourceEntry: перенос в отдельный модуль сохраняет исходный
decoder и чтение сжатого backing inode. Это 7-Zip extension, отдельное от
Linux VFS evidence. Сборка и реальный install runtime на macOS проверены
извлечением hardlinks, resource fork и Unicode/emoji файлов.

Metadata/permissions и thread validation проверены CLI/ABI/backend suites на
macOS ARM64, Linux x64 и Windows x64, включая extraction security и provenance
на всех трёх платформах (remote receipt `20261019T133324Z-075995b5`).
Linux VFS дополнительно подтвердил defaults, UID/GID/link count и точные
FinderInfo bytes файлов, каталога и трёх hardlink names на generated image.
Шесть повреждённых вариантов сопоставлены отдельно: неверные file/directory
modes и thread length дали EIO, неверные parent/name — ENOENT.
Несовпадение thread type kernel 6.8 принимает; наш eager integrity check
отклоняет этот случай. Все семь read-only loop images остались неизменны.
Это не проверка принудительного применения immutable/append VFS flags.

Timestamp/special-inode fixtures сопоставлены с Linux VFS отдельно: mtime/atime/
ctime у файла, каталога и трёх hardlink aliases, disk zero, raw dev_t с большими
major/minor и FIFO/socket metadata совпали; оба read-only образа остались неизменны.
Проверка special inode использует только `lstat`, без открытия устройств.
CLI/ABI проверяют эти properties, backend — реальное извлечение с mtime 2106 года.
HFS CLI/ABI/backend, runtime packaging, provenance и extraction security suites
прошли на macOS ARM64, Linux x64 и Windows x64. Remote receipt
`20261019T134311Z-af620460`: Linux 6/6, Windows 5/6; CLI regression на Windows
прошла после исправления CRLF parsing теста (`20261019T134440Z-0d16b3b4`).
Код модуля в повторном запуске не менялся. Приложение собрано на macOS;
полный gate, slow suites и UI в эту итерацию не входили.

Classic HFS проверен на hfsutils-produced fixture с native multi-level catalog:
data/resource bytes, type/creator streams, Unicode projection, независимые и
retained streams, отмена/I/O/ошибочный Open и реальные ArchiveBackend consumers.
Linux VFS `6.8.0-146-generic` подтвердил bytes обычных файлов, type/creator,
правила lock/default mode, UID/GID/mtime и неизменность шести read-only images:
base, data/resource overflow, catalog overflow, 1536-byte blocks, lock и
preallocation/empty forks. VFS lookup `file/rsrc` вернул ENOTDIR; native VFS
resource bytes не заявляются, их проверяют CLI/ABI/backend.
Все семь focused suites (HFS CLI/ABI/backend, runtime/Windows packaging,
provenance и extraction security) прошли на macOS ARM64, Linux x64 и Windows x64;
remote receipt `20261019T141119Z-77ea5199`. Приложение собрано на macOS.
Реальный install runtime на macOS автоматически распознал и извлёк пять
classic variants, включая data/resource/catalog overflow и 1536-byte blocks;
установленные GPL/LGPL/BSD notices проверены. Staging удалён после проверки.
Full/slow suites, UI и физические устройства в эту итерацию не входили.

Plain classic HFS читается модулем приложения. Итоговая сверка read scope и
системных metadata services приведена ниже.
Apple decmpfs сохраняется как расширение 7-Zip, которое Linux hfsplus не
декодирует. Fork/extent xattrs также не читаются Linux xattr API; отдельное
отсутствие kernel read feature из этого не выводится. Запись, journal recovery,
mount services и проверка всех allocation/index maps не заменяют read-сценарии.

## Итоговая сверка read-side source scope — 2026-10-21

Полные inventory units `fs/hfs`, `fs/hfsplus` и shared dependencies из manifest
сопоставлены с текущим reader и его consumers. Финальный проход проверил
bootstrap B-tree/inode, catalog threads и readdir/lookup, extent/cache mapping,
attribute lookup/list/get, resource-fork roles, inode/fileattr projection и
оставшиеся super/ioctl/options ветви. Все 58 pinned source/license hashes
повторно сверены. Source read-scope review HFS/HFS+/HFSX закрыт; это не
завершение общего goal и не заявление о переносе всех функций kernel driver.

Следующая сверка дополняет существующие разделы этого аудита, не создавая
отдельного inventory или очереди работ:

| Reference units | Текущий read consumer и граница |
| --- | --- |
| `hfs/mdb`, `btree`, `bnode`, `bfind`, `brec`, `catalog`, `extent`, `inode` | Переданные C read paths classic HFS: metadata bootstrap/search, data/resource/catalog overflow, record/CNID/thread validation, fork mapping и inode projection. Системные inode/folio/lock services заменены byte transport и owned state. |
| `hfsplus/wrapper`, `super` | Linux MDB/version guards и повторный discovery подключены к SDK transport; secondary signature, enclosing extents и source bounds проверены. Read-only mount defaults/flags не превращаются в write services. Partition/session discovery принадлежит transport/partition handlers. |
| `hfsplus/btree`, `bnode`, `brec`, `bfind` | Сверены bootstrap/header/node/key/record branches. SDK сохраняет authoritative eager leaf enumeration; Linux catalog record validator подключён отдельно. Полный VFS index search и node/map allocator не добавляются как неиспользуемые второй reader или fsck. |
| `hfsplus/catalog`, `dir`, `inode` | Подключены raw-name comparators, hidden-directory/hardlink lookup, bounded thread/record checks, permissions/links/special inode, dates и symlink target. Readdir limits/link counts/inode IDs, создаваемые VFS, не выдаются за отдельные disk records. |
| `hfsplus/extents` | Read mapping и cache consumers сверены с исходным SDK fork/overflow reader. Обе fork роли используют один owned extent stream; нет отдельного resource parser. Сам extent-tree не получает overflow bootstrap, который Linux также отвергает для этого inode. Allocation/truncate/writeback omitted. |
| `hfsplus/attributes`, `xattr`, `xattr_user`, `xattr_trusted`, `xattr_security` | Binary key/lookup/list consumers сверены с indexed archive attributes. Inline values и catalog FinderInfo читают Linux C bodies; namespace wrappers не добавляют иной value decoder. Fork/extent records не поддерживаются самим Linux xattr API; сохранённые SDK extensions отмечены выше. LSM/init-security и set/remove paths не читают дополнительные disk values. |
| `hfsplus/unicode`, `tables`; `hfs/string`, `trans` | Переданы оригинальные read converters/comparators и таблицы. NLS/mount charset, inverse lookup encoding и dentry hashing остаются platform boundary; UTF-16 archive representation и classic default-byte view описаны выше. |
| Остальные `bitmap`, `super`, `sysdep`, `part_tbl`, `ioctl`, `options`, headers/Kconfig/Makefile | Проверены consumers и определения. Allocator bitmap/mutation, caches/locks, mount credentials/timezone/charset, block-device/session/partition selection, blessing, sync/NFS/inode publication и build configuration не являются отсутствующими archive disk readers. Stored BSD flags, classic lock bit и owners уже представлены metadata properties. |
| Shared `hfs_common`, `kdev_t`, `namei`, delayed-call/error-pointer units | Disk layouts, raw device numbers, timestamp rollover и original page-link termination подключены к своим C/SDK consumers. Page-cache/RCU/highmem и native credential enforcement не эмулируются. |

В этой сверке не осталось дополнительной неподключённой disk read feature,
необходимой read-only archive interface. Структура оригинального SDK reader
сохранена; Linux C добавлен там, где требовались отсутствующие decoding branches.
Catalog enumeration вместо pathname lookup, indexed selection вместо dentry
services и alternate streams вместо xattr syscalls — явно заданные interface
adaptations. Читаемые metadata/данные проходят прежние bounds/error/cancellation
paths; borrowed source comparison и runtime acceptance остаются разными
свидетельствами.

Границы не скрываются этим статусом: archive reader проверяет полный доступный
mapping и catalog заранее, отклоняет неоднозначные CNID/parent cycles и может
сообщать headers error при несогласованных counters. Linux обращается к части
этих данных лениво. Это не обещание одинакового forensic recovery для любого
повреждённого дерева. Неиспользуемые allocation/index maps не выдаются за полную
integrity verification. Apple decmpfs и дополнительные SDK attribute/fork
расширения не заявляются как функции Linux.

Текущие focused ABI/CLI/ArchiveBackend сценарии проверены на macOS, Linux и
Windows; native read-only evidence и receipts приведены у соответствующих
изменений. Тесты не доказывают отсутствия всех source omissions сами по себе.
Native reference работает на более старом kernel; другие PAGE_SIZE configurations,
все реальные тома/physical devices, UI appearance, full/slow suites и новый
installed-package прогон не заявляются. Новая итерация меняет только audit/status:
достаточен `git diff --check`, ранее проверенные source/build outputs и compiler
caches сохранены.
