# SquashFS: metadata и независимые streams

Scope входит в [общую проверку форматов](filesystems-linux.md). Эта итерация
исправляет существующее чтение SquashFS в обоих поставляемых engines. Она
не завершает актуализацию всех SquashFS read features по Linux.

## Исходники и сравнение

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`, полный
`CPP/7zip/Archive/SquashfsHandler.cpp`. Сохранены модуль, классы, parsers
версий 1–4, table buffers, декодеры, recursive directory traversal,
`CCachedInStream`, чтение блоков и progress/extraction flow.
Единственная авторитетная адаптация находится по тому же пути под `src/`;
runtime overlay и [7-Zip baseline](../upstream-baseline.json) учитывают её
для динамического и standalone engines. Sibling checkout не изменяется.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
[Snapshot manifest](../squashfs-linux-baseline.json) закрепляет полные units
`fs/squashfs`. Это reference snapshot, а не перечень переданных C-функций.
В этой итерации Linux C-код не копируется в LGPL/unRAR engine.

Сравнены полные тела 7-Zip handler, его portable stream/decoder dependencies
и соответствующие read branches Linux:

- `super.c`: версия, block geometry, bytes_used и порядок metadata tables;
- `inode.c`, `dir.c`: base inode, UID/GID, regular/extended file, directory,
  symlink и special inode branches, offsets, counts, names и fragments;
- `block.c:squashfs_read_data`, `cache.c:squashfs_read_metadata`:
  границы metadata/data и переходы между metadata blocks;
- `id.c`, `fragment.c`, `export.c`, `xattr_id.c`: index tables и их границы;
- `decompressor.c`, LZ4/LZO wrappers, compressor-option branches XZ,
  существующие bounded Zlib/XZ/Zstd/LZO implementations 7-Zip.

VFS/page cache, mount services, locking и запись Linux не переносятся
в archive handler. Версии 1–3 и legacy LZMA остаются расширением 7-Zip;
Linux эталон поддерживает v4.0. Форматирование полного C++ файла выполнено
отдельно от функционального сравнения; macro class boundary сохранена.

## Адаптация выбранного read scenario

Открытие учитывает реальную длину входа, v4.0/block-size contract,
отсутствующий fragment index при нулевом числе fragments и пустой root.
Конец directory metadata определяется перед fragment/lookup/ID metadata,
а не по началу массива fragment pointers. Сохраняется исходный eager parser.

Добавлены проверки inode IDs/mode/number, отрицательного размера, fragment
tail, directory counts/names, root type и смещений внутри конкретного
распакованного metadata block. Цикл каталогов отклоняется при открытии.
Compressed metadata ограничена 8 KiB, включая Zlib output; data/fragment
packet проверяется до выделения буфера и чтения, также при ненулевом
смещении файла в общем fragment. Ошибки I/O и отмена сохраняют HRESULT.

Возвращённый stream получает собственные inode, block map, decoder и cache,
используя исходные `CHandler::ReadBlock` и `CCachedInStream`. Он удерживает
источник и не зависит от следующего GetStream или Close/reopen/release
архива. Symlink stream владеет копией своих байтов. Это адаптация archive
ABI lifetime, у которой нет прямого аналога в Linux VFS. Она не заявляет
параллельное чтение одного seekable источника из нескольких threads.

## Открытая актуализация

LZ4 пока отсутствует, LZO требует Linux decoder с modern RLE support.
Полная проверка compressor options, оставшихся index/xattr metadata,
экспорт доступных inode/link properties и остальные source-visible read
различия остаются в scope следующей интеграции. Текущие проверки native
xattr-containing images подтверждают чтение обычных файлов, а не экспорт
произвольных xattrs через archive ABI. Ограничение eager metadata buffers
также не считается новым read feature. Эта запись не превращает пробелы
в окончательные ограничения продукта.

## Проверки

Focused suites `squashfs` и `squashfs_stream` проверяют наблюдаемые результаты
обоих engines: Unicode, raw/Zlib metadata и data, sparse blocks, shared
fragments, пустой root без fragment index, legacy v3 BE/LE, malformed
metadata references, cycles, names/counts, размеры/границы данных,
interleaved seek/read, stream lifetime и HRESULT/cancellation/retry.
На Linux дополнительно создаются образы официальным `mksquashfs` с
gzip/XZ/LZO/Zstd, hardlinks и xattrs. Временные fixtures удаляются автоматически.

2026-10-19: обе suites PASS на macOS arm64/Qt 6.12, Linux x64 и Windows
x64/MSVC/Qt 6.12. Remote receipt: `20261019T101207Z-7e1e0ba6`.
Дополнительный read-only mount oracle на Linux `6.8.0-146-generic` подтвердил
байты трёх synthetic v4 images и четырёх образов `mksquashfs`; это runtime
evidence существующего runner kernel, source reference остаётся закреплённой
ревизией выше. Сортировка имён synthetic fixture исправлена по этому oracle.
После прогона временных SquashFS fixtures на runners не осталось.
Baseline verification: 416 exact, 207 adapted, 6 local; `git diff --check` PASS.
Полная/slow/UI/physical-device приёмка не входит в этот backend scenario;
incremental builds и compiler caches сохранены.
