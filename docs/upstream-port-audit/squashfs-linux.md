# SquashFS: metadata, streams и Linux-декодеры

Scope входит в [общую проверку форматов](filesystems-linux.md). Исправлено
существующее чтение в обоих engines; приложение также получает Linux LZ4,
современный LZO и проверки compressor options через отдельный format-модуль.
Полная актуализация SquashFS read features по Linux ещё продолжается.

## Частично доступные xattr: read-only политика, 2026-10-21

Повторно сравнены `SquashfsHandler.cpp:Open2`, callback/error translation,
alternate streams и их потребители с `fs/squashfs/inode.c:squashfs_read_inode`,
`xattr_id.c:squashfs_xattr_lookup`, `xattr.c:squashfs_listxattr` и
`squashfs_xattr_get` того же Linux commit, указанного ниже. Linux читает
значения атрибутов отдельно от содержимого файлов; прежний eager archive
adapter отказывал всему открытию при ошибке одного значения.

GPL-модуль сохраняет читаемые данные файлов и полностью полученные значения
атрибутов. Если список имён доступен, сбор значений продолжается после
недоступного значения. Неполный набор диагностируется у владельца, включая
его жёсткие ссылки, и через `Read-only limitations`. Общая Qt-служба запрашивает
согласие на выбранное открытие и сохраняет индикатор ограниченного режима.
Недоступное значение не публикуется как пустое или проверенное. Отмена,
ошибка callback и нехватка памяти остаются фатальными; Close/reopen сбрасывает
диагностику. Ограничения размеров и адресов сохранены, записи/replay нет.

Это адаптация archive lifetime/enumeration в существующем host glue
`src/squashfsmetadata.c` и исходном SDK handler. Тела Linux xattr readers,
их C-язык и GPL-изоляция сохранены; standalone engine без GPL-модуля
сохраняет прежний scope. Изменение не доказывает полную устойчивость к
повреждениям SquashFS: начальная загрузка таблиц, eager inode/directory
traversal и export lookup ещё требуют отдельного продолжения аудита.

Синтетический образ с недопустимой ссылкой одного out-of-line xattr
проверен через native Linux `6.8.0-147-generic`, read-only loop mount:
данные `alpha`, его hardlink и Unicode-файла совпали побайтно; inline и
следующий независимый user xattr доступны, повреждённый trusted xattr
возвращает EIO при чтении с правами root. Образ не изменился, mount и
временные данные удалены. Receipt: `20261021T141525Z-f90232ec`.
Этот runtime reference старше закреплённого source reference.

Регрессии проверяют публичные ABI/CLI: недоступный список/большое значение,
повреждённую ссылку, последующие читаемые значения, основные данные,
диагностику владельцев, отмену/OOM и повторное открытие. Отдельный
`filesystem_squashfs_read_ui` проверяет реальное открытие, EN/RU принятие
и отказ, видимый badge, запрет изменения и копирование читаемого файла.
Итоговые восемь focused suites (`squashfs`, `squashfs_stream`,
`squashfs_metadata`, `filesystem_squashfs_read_ui`, `filesystem_fat_read_ui`,
`language`, `upstream_baseline`, `upstream_inventory`) прошли на macOS,
Linux и Windows. Remote receipt: `20261021T141701Z-b4e8ce1f`.
На macOS проверены Cocoa widgets и EN/RU JSON нормальной/компактной геометрии;
на гостях — Qt offscreen. Это проверка сценария ограниченного чтения,
а не утверждение pixel parity. Приложение собрано инкрементально на macOS.
Полные и slow suites не запускались; временный native helper удалён и
его удаление синхронизировано на обоих гостях. Build trees и кэши сохранены.

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

## Linux metadata, xattrs и inode properties

Переданы полные `id.c` и `fragment.c`, обе read functions `xattr_id.c`,
`export.c:squashfs_inode_lookup` и `squashfs_read_inode_lookup_table`, полный
`xattr.c` с list/get/namespace handlers. `cache.c` сохраняет полные
`squashfs_copy_data`, `squashfs_read_metadata`, `squashfs_read_table`.
VFS/NFS dentry/export operations и kernel cache allocation/wait queues
не относятся к архивному чтению и не копируются. `squashfs_fill_super`
непригоден целиком без VFS; его table setup sequence перенесена в узкую
инициализацию C reader с исходными helpers и порядком xattr/ID/export/fragment.

C metadata reader получает тот же проверенный источник и original 7-Zip
metadata decoder через callback. Одноэлементный cache хранит собственные
распакованные байты, поддерживает переходы через metadata blocks и повтор
после ошибки. Источник и callbacks живут до Close; returned file streams
по-прежнему независимы от metadata reader. Общий bounded bio service вынесен
из decoder adapter и используется обоими C-компонентами.

Динамический handler использует C tables для UID/GID, fragments и directory
boundary. При наличии export table каждый inode reference проверяется через
исходный Linux lookup. Xattrs читаются полностью, включая binary/empty values,
shared out-of-line references и переходы metadata block. C API позволяет
получать names/values; те же функции используются при проверке inode metadata.
User/trusted/security namespaces читаются как байты архива, без привязки к
привилегиям процесса на host OS. Неизвестные типы пропускаются как в Linux.
Публичные xattrs доступны как alternate streams через стандартный archive ABI,
как описано ниже. Установка OS xattrs остаётся отдельной write operation.

Сохранены GNU-to-C portability adaptations. Raw table range проверяется до
выделения памяти; длина не должна пересекаться с кодируемым uncompressed bit.
Отрицательный xattr count отклоняется до signed loop, names/values ограничены
Linux VFS xattr limits. Eager archive open отклоняет повреждённые metadata
целиком; Linux может сообщить ошибку позднее при отдельном xattr/NFS access.
Это отличие момента отказа не подменяет корректные байты файла нулями.

В обоих engines восстановлены реальные inode number/nlink и цель symlink.
Стандартные archive/item inode properties подключают существующий механизм
hardlinks `ArchiveExtractCallback`; новый алгоритм создания ссылок не добавлен.
Ненулевой data descriptor с нулевым payload и нулевой fragment payload теперь
отклоняются, сохраняя настоящий sparse descriptor 0. Так повреждённый пакет
не выдаёт вымышленные нули вместо ошибки Linux.

Проверенный source scope перечислен выше отдельно от runtime evidence.
Eager metadata limits и bounded-buffer decoder allocation policy сохраняются;
другие filesystem/format rows общего аудита продолжаются.

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

Metadata/inode iteration: `squashfs_metadata` дополнительно проверяет names,
empty/binary и shared OOL values, short buffers, invalid IDs, export lookup,
source failure на следующем block и retry через чистый C API без unRAR.
CLI/ABI checks покрывают raw table/index corruption, OOL/count повреждения,
ложные zero packets, реальные hardlink identity и symlink properties.
ArchiveBackend теперь видит ссылку и применяет существующий process-extraction
guard; отказ всего набора и успешный выбор обычного файла проверяются вместе.
Native Linux `6.8.0-146-generic` подтвердил реальные xattrs, включая trusted,
empty и shared OOL values, inode identity/readlink и семь corrupt cases.
Focused6 (три SquashFS suites, codec_probe, upstream_baseline/inventory) PASS
на macOS/Linux/Windows; remote receipt `20261019T105020Z-026dcd9a`.
Последние export lookup и packet/count guards проверены тремя SquashFS suites
на всех трёх ОС: receipt `20261019T105725Z-8db18d7e`, macOS 4,51 с.
Проверка baseline и staged whitespace выполняется перед коммитом. Full/slow,
physical-device и native Windows symlink creation в этот backend scenario
не входят. Build directories и compiler caches сохранены.

## Публикация xattr values — 2026-10-20

Повторное сравнение полного `xattr.c`, `xattr_id.c`, metadata cache/read callers,
их локальных C services и всех property/stream/extraction consumers handler
выявило незавершённую интеграцию: значения читались при проверке inode, но не
были доступны пользователю. Исходные Linux list/get/namespace/OOL bodies уже
перенесены; новый visitor использует эти же функции и заменяет отдельное
чтение с последующим отбрасыванием bytes. Validation-only C API остаётся
обёрткой того же visitor. Источник, декодирование, metadata-block transitions,
типы namespaces и ошибки остаются у существующих C readers. Пустые значения
передаются отдельно от ошибки; размер фактического getter result проверяется.

Один authoritative handler по прежнему upstream пути публикует owned metadata
streams файлов, каталогов и каждого hardlink name. Общие xattr IDs используют
один набор values. Корень получает виртуальный item с безопасным уникальным
именем. Экранирование bytes/коллизий имён и владение values переиспользуют
адаптер предыдущей EROFS-итерации, выделенный в общий GPL-only source; EROFS
использует тот же код. Этот адаптер не подключён к LGPL/unRAR core. Отдельные
коллизии полных путей, включая реальные filenames с двоеточием, разрешаются
без изменения обычных имён; поиск использует упорядоченный набор путей.

Archive-level/item alternate-stream markers, name/parent/type properties,
owned buffer streams, size/packed size и extraction/progress подключены вместе.
Metadata не наследует inode/stream identity, symlink или directory type своего
владельца. `-sns-` исключает metadata, сохраняя обычные colon filenames; `-sns`
позволяет извлекать выбранные values. Root/directory values имеют обычный
readonly file stream. C callbacks не выпускают C++ исключения через выделенные
C buffers; source errors и cancellation сохраняют HRESULT и очищают Open state.
Standalone `7zz` сохраняет прежний reader без GPL xattr publication; обе версии
по-прежнему извлекают обычные payloads и hardlinks.

Новый fixture создаёт официальный mksquashfs 4.6.1. Временный ext4 EA_INODE
backing позволяет задать большие xattrs независимо от возможностей host volume;
creator читает trusted/security metadata с нужными правами. Образ содержит
binary/empty/Unicode/unsafe names, case collisions, shared 9000-byte values,
корень, каталог, hardlinks и обычные `[ROOT]`/colon filenames. Generator и
manifest сохраняют параметры, hash и native comparison. Нативный Linux
`6.8.0-147-generic` вернул точные 22 значения через read-only mount, подтвердил
общий inode hardlink и неизменный image hash, receipt
`20261020T182610Z-c0f9b97f`. Временные backing/images/mounts и транспортный JSON
удалены. Это установленное ядро, отдельное от закреплённого source baseline.

CLI проверяет все values и обычные bytes, source preservation и stream filtering.
ABI проверяет parent relationship, lifetime после Close/reopen/Release,
cross-block value seeks, empty streams, input E_FAIL/E_ABORT, cancellation после
реального metadata read, output failure, final progress и reuse. ArchiveBackend
проверяет реальные listing и выбранное извлечение больших/root/directory values;
прежние compressor, hardlink/symlink и malformed metadata checks сохранены.
Проверки EROFS включены из-за общего адаптера имён и owned values.

Девять focused suites (`squashfs`, `squashfs_stream`, `squashfs_metadata`,
`erofs`, `erofs_module`, `codec_probe`, `runtime_cache`, `upstream_baseline`,
`upstream_inventory`) PASS на macOS arm64/Qt 6.12 (6,84 с), Linux x64
(10,70 с) и Windows x64/MSVC/Qt 6.12 (45,03 с), receipt
`20261020T183202Z-8d7fd21f`. После финального callback cancellation без
подмены file progress числом inode повторены три SquashFS suites: PASS на
macOS (2,63 с) и обоих runners (1,71/10,92 с), receipt
`20261020T183419Z-578e88e7`. Source-port scope выше и runtime acceptance
различаются; это не доказательство завершения остальных read branches общего
goal. Full/slow, release-package, UI и физические устройства не проверялись.
Incremental builds и compiler caches сохранены; временных task-owned fixtures
после завершённых прогонов не осталось.
