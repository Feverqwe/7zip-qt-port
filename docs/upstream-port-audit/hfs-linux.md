# HFS/HFS+: read modules, forks и metadata

Работа входит в [общий scope актуализации по Linux](filesystems-linux.md).
Исправлено существующее чтение HFS+/HFSX и их HFS wrappers; добавлены Linux
Unicode read conversion и разрешение hardlinks через backing inode.
Актуализированы permission defaults, catalog/thread validation и FinderInfo.
Подключены Linux timestamp rollover и metadata специальных inode.
Добавлено чтение plain classic HFS через Linux C read paths в том же GPL-модуле.
Полная актуализация семейства HFS ещё не завершена.

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

Остаются дальнейшее сравнение read branches и системных metadata services;
plain classic HFS теперь читается модулем приложения.
Apple decmpfs сохраняется как расширение 7-Zip, которое Linux hfsplus не
декодирует. Fork/extent xattrs также не читаются Linux xattr API; отдельное
отсутствие kernel read feature из этого не выводится. Запись, journal recovery,
mount services и проверка всех allocation/index maps не заменяют read-сценарии.
