# HFS+: fork mapping и чтение metadata

Работа входит в [общий scope актуализации по Linux](filesystems-linux.md).
Эта итерация исправляет существующее чтение HFS+/HFSX и их HFS wrappers.
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

Linux служит эталоном интерпретации формата; GPL bodies в этой итерации
не копируются и не линкуются. Исправляются исходные 7-Zip алгоритмы и их
portable stream integration. Сохранены исходные классы, eager leaf enumeration,
каталог, alternate streams, decmpfs decoder и extraction callbacks. Полный
kernel B-tree index traversal не переносится: archive handler перечисляет leaves,
а не предоставляет VFS lookup. Проверки высоты, backlinks, offsets и header
относятся к читаемым nodes; это не обещание полной проверки всех index/map nodes.
Зависимости — исходные portable Common/Windows helpers, StreamObjects/Utils,
LimitedStreams и существующие zlib/LZVN decoders; новый decoder не добавляется.

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
HFS дополнительно проверены его suites и provenance на трёх ОС.
Приложение собрано на macOS. Полный gate, slow suites, UI и физические
устройства в эту проверку не входили.

Linux VFS `hfsplus`, kernel `6.8.0-146-generic`: восемь корректных вариантов
прочитаны через read-only loop mount; bytes обоих файлов и inline attribute
совпали, образы остались неизменны. Это runtime evidence установленного ядра,
отдельное от source comparison закреплённой более новой ревизии. Resource fork
проверен через engines и ABI, отдельная VFS-проверка его bytes не заявляется.

Остаются актуализация Unicode conversion/normalization, разрешение hardlinks
через private inode directory, расширение metadata и дальнейшее сравнение read
branches. Plain classic HFS исходный handler не читает: поддерживается только
его wrapper с HFS+/HFSX; это отсутствие остаётся в общем scope работы.
Apple decmpfs сохраняется как расширение 7-Zip, которое Linux hfsplus не
декодирует. Fork/extent xattrs также не читаются Linux xattr API; отдельное
отсутствие kernel read feature из этого не выводится. Запись, journal recovery,
mount services и проверка всех allocation/index maps не заменяют read-сценарии.
