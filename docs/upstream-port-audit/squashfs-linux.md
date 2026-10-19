# SquashFS: metadata, streams и Linux-декодеры

Scope входит в [общую проверку форматов](filesystems-linux.md). Исправлено
существующее чтение в обоих engines; приложение также получает Linux LZ4,
современный LZO и проверки compressor options через отдельный format-модуль.
Полная актуализация SquashFS read features по Linux ещё продолжается.

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
`fs/squashfs`, LZO и переиспользуемого LZ4. Manifest различает adapted/exact
units и полный reference snapshot. Linux C-код остаётся в отдельном GPL-модуле,
который не линкуется с unRAR и не добавляется в основной LGPL/unRAR engine.

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

## Linux C decompressor module

В `sevenzip_squashfs` переданы полные тела `decompressor.c` (lookup,
`get_comp_opts`, setup), `decompressor_single.c` (create/destroy/decompress),
`lz4_wrapper.c` и `lzo_wrapper.c` (options/init/free/decompress), а также
`lib/lzo/lzo1x_decompress_safe.c`, включая versioned RLE и overrun branches.
LZO scalar branches используются на всех ОС; архитектурные ускорения kernel
не нужны для корректного чтения. Существующий Linux LZ4 из EROFS используется
повторно из единственного исходника `src/lib/lz4`, с его исходной лицензией.
`xz_wrapper.c` сохраняет полные option structs и `squashfs_xz_comp_opts`.
`block.c` сохраняет полные `copy_bio_to_actor` и `squashfs_read_data`, включая
metadata header, bounds и недоступность decoder при bootstrap options.
Block-device bio submission/cache заменены bounded archive source service;
новый parser metadata header в host layer не вводится.

Для XZ/Zlib/Zstd сохранены пригодные bounded реализации 7-Zip и их portable
C dependencies. Linux framework проверяет их options: для XZ — dictionary
shape, для LZ4 — обязательную legacy version 1; Zlib/LZO/Zstd игнорируют
содержимое options согласно Linux. Bootstrap block читается до создания
decoder и должен быть uncompressed. Linux LZMA compression ID остаётся
unsupported; v1–3 legacy reader сохраняется отдельно.
Эти 7-Zip decoders работают с ограниченным полным output buffer, без
Linux workspace/page-cache allocation policy. Проверка options не заявляет
идентичность kernel memory allocation и отказов по размеру workspace.

`page_actor.c` содержит полные intermediate-buffer functions; direct VFS
page-cache branch опущена. `page_actor.h` сохранён. Узкий C host layer
предоставляет bounded source callback, одноэлементный bio, allocator и
contiguous-buffer page actors. Страница actor выбрана 64 KiB для переносимого
buffer service; размер SquashFS metadata остаётся 8 KiB, filesystem block
может быть меньше страницы. Mutex service не нужен для приватного decoder
каждого последовательного stream. В original C сохранены lock calls.
GNU void-pointer arithmetic заменена арифметикой `char *`, empty initializer
— `{0}`, GNU omitted ternary в actor — стандартной C формой.
Неиспользуемое nested flexible-array поле ldir inode скрыто для MSVC;
размер фиксированного дискового prefix и читаемые offsets не меняются.

Основной динамический engine передаёт регистрацию SquashFS отдельному
format-модулю; standalone `7zz` сохраняет исходные codec dependencies и явно
отказывает на LZ4/modern LZO. Приложение использует динамический engine,
catalog/probe и установленный `Formats` module. CMake deploy и Unix/Windows
install включают модуль и GPL/BSD/7-Zip notices. Обработка GetStream/Close
сохраняет собственный C decoder у каждого stream; callback источника нужен
только при setup и не хранится после него.

## Открытая актуализация

Полная проверка оставшихся index/xattr metadata,
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

Интеграция C-декодеров, 2026-10-19: focused5 `squashfs`, `squashfs_stream`,
`codec_probe`, `upstream_baseline`, `upstream_inventory` PASS на macOS и Linux;
Windows PASS после адаптации nested flexible array. Receipts:
Linux `20261019T102300Z-7b21f906`, Windows `20261019T102401Z-e8b1d446`.
Добавленный standalone refusal regression и итоговый read/ABI/backend scope
повторно PASS на обоих runners: `20261019T102624Z-b99384f9`.
Все шесть compact creator images из `tests/fixtures/squashfs` читаются на
трёх ОС; producer и flags записаны в fixture manifest. LZ4 HC с 4 KiB блоками,
XZ с заданным dictionary, sparse/hardlinks/fragments и Unicode проверяются
по фактическим байтам. ABI и ArchiveBackend проверяют LZ4/LZO RLE/Zlib,
отмену, ошибки и lifetime. На Linux дополнительно проходят свежие producer
images и native readonly mount: восемь valid images и три invalid options.
Установленный macOS runtime извлекает LZ4-HC/4K, XZ и LZO. Install smoke
payload и native mount fixtures удалены; incremental build/cache сохранены.
После передачи полных `block.c` read bodies и защиты C callback от C++
исключений обе SquashFS suites повторно PASS на macOS/Linux/Windows;
final remote receipt `20261019T103336Z-9b9eae7a`. Повторное source comparison
подтвердило неизменное тело `squashfs_read_data`, кроме `{0}` initializer.
