# SquashFS: metadata, streams и Linux-декодеры

Scope входит в [общую проверку форматов](filesystems-linux.md). Исправлено
существующее чтение в обоих engines; приложение также получает Linux LZ4,
современный LZO и проверки compressor options через отдельный format-модуль.
Полная актуализация SquashFS read features по Linux ещё продолжается.

## Большие raw index tables, 2026-10-22

Сверены Linux `cache.c:squashfs_read_table`, raw branch
`block.c:squashfs_read_data`/`copy_bio_to_actor`, page actor и callers в
`id.c`, `fragment.c`, `export.c`, `xattr_id.c`; baseline Linux
`58161ead29e50d70a317ea26b472e0c7508e69da`, retrieval links ниже.
Локальная защита от пересечения длины с compression flag запрещала любые
raw tables от 16 MiB, хотя у этих таблиц нет encoded compression-size field.
В Linux исходный вызов передаёт всю длину через `length | flag`; это также
не представляет произвольную большую длину без потери бита. Возможность
native allocation большой contiguous table является отдельным ограничением.

Сохранены исходные C allocation, page actor, raw read path и cleanup.
Archive adapter читает таблицу последовательными page-sized requests через
тот же `squashfs_read_data`, без aggregate compression-flag cap. Source range
проверяется до allocation; округление числа страниц не переполняет int.
Каждый запрос должен вернуть точную длину: при ошибке/коротком чтении вся
незавершённая таблица освобождается и не публикуется. Отмена остаётся
фатальной, обычная недоступность optional table сохраняет независимые данные
через существующий механизм неполноты. Записей в source, repair/replay нет.

Public C API regression использует RAM-only fragment index размером
32 MiB + 8 bytes и проверяет доступ к поздней записи, ошибку I/O посередине
таблицы с сохранением независимого UID и фатальную отмену. До исправления
поздняя запись возвращала EIO; после — точные block/size. Fixture изолирует
index bootstrap/lookup, а не доказывает корректность всех неиспользуемых
объявленных fragments или native mount огромного filesystem. Peak allocation
около 64 MiB, файл на диске не создаётся; native mount здесь не проверялся.

На macOS три backend suites прошли за 6,68 с, UI/source suites — за 7,33 с
(Cocoa вне sandbox). Все шесть прошли на Linux/Windows за 9,52/47,26 с,
receipt `20261022T045619Z-d04e5d63`. Это проверка large-table reader и его
прямых consumers; общий read-side аудит SquashFS остаётся открытым.

## Потоковое перечисление xattr names и доступный префикс, 2026-10-22

Повторно сверены полные Linux `xattr.c` list/get/namespace branches,
`xattr_id.c`, metadata cursors и SDK consumers owned attributes, errors,
alternate streams и Close. Baselines: Linux
`58161ead29e50d70a317ea26b472e0c7508e69da`, SDK
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`; retrieval links ниже.
Linux get конкретного xattr не требует успешного list всего inode.
Archive adapter сначала полностью перечислял имена, поэтому отказ хвоста
скрывал доступный префикс. Общий 64 KiB buffer limit также скрывал все
атрибуты исправного inode с большим списком допустимых имён.

В перенесённый исходный list loop добавлен узкий name callback после полного
чтения entry; namespace selection, metadata fields, skip и error branches
сохранены в единственном parser. Исходный `squashfs_listxattr` вызывает то же
тело без callback, сохраняя свой strict list contract. Archive visitor
использует buffer одного допустимого имени и передаёт каждое завершённое
имя в прежний get/owned-value flow. Второго xattr parser нет. Reentrant get
использует тот же cache, а local list cursor сохраняет позицию следующей записи.
Не требуется общий buffer всего списка или повторный проход для его размера.
Ограничения отдельного имени/значения, source bounds, namespace handling и
path validation сохранены. Retained catalog масштабируется с реальными
атрибутами; временный name buffer не зависит от их суммарного числа.

При локальной ошибке list сохраняются уже прочитанные значения, незавершённое
имя не публикуется. Ошибки отдельных значений по-прежнему не скрывают другие
доступные значения. Итоговая ошибка сохраняет прежние диагностику неполноты,
явное согласие на выбранное открытие и видимый badge. Abort/OOM/callback
failures остаются фатальными; Close очищает состояние, возвращённые потоки
владеют bytes. Исправный большой список не помечается неполным только из-за
VFS syscall list limit. Source access остаётся read-only, repair/replay нет.

ABI regression до исправления воспроизвёл потерю prefix streams при доступных
данных файла. После исправления проверяются binary/security/empty values,
hardlink projection, симптомы неполноты, Abort/OOM после доступного префикса
и retained streams. Другой control имеет более 64 KiB допустимых names:
значения за прежней границей доступны без ложной диагностики. CLI копирует
конкретные атрибуты и обычный payload. Native Linux `6.8.0-147-generic`
подтвердил оба controls (`20261022T044219Z-7ecde162`): list syscall возвращает
EIO/E2BIG соответственно, но отдельные values и оба файла читаются точно.
Архивное перечисление доступного префикса является адаптацией полезного
read-only доступа, а не partial-success контрактом Linux listxattr syscall.
Synthetic loop,ro,nosuid,nodev,noexec images не изменились, mounts/temp очищены,
helper удалён. Native kernel и pinned source — отдельные evidence.

Пять backend/source suites на macOS прошли за 8,62 с, существующий UI consumer
consent/badge для неполных attributes — за 12,56 с вне sandbox. Все шесть
прошли на Linux/Windows за 10,92/38,35 с
(`20261022T044315Z-09f5d6c0`). Incremental macOS app build и diff check прошли.
Full/slow suites, физические устройства и visual parity не проверялись.
Это завершение перечисления names/доступного префикса, не закрытие общего
аудита остальных read-side scenarios и форматов.

## Границы необходимых чтений при открытии, 2026-10-22

Сверены Linux `super.c:squashfs_fill_super` и superblock layout, полные SDK
`Open2`, `CHeader::Parse`/version branches и physical-size publication,
`CArc` archive-tail detection/`CheckZerosTail`, nested-open properties и
прямые filesystem consumers. Baselines: Linux
`58161ead29e50d70a317ea26b472e0c7508e69da`, SDK
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`; retrieval links ниже.
Linux читает v4 superblock длиной 96 bytes и ограничивает filesystem reads
через `bytes_used`; проверка нулевого archive padding не нужна для mount.

SDK adapter запрашивал 119 bytes legacy superblock также для v4, затрагивая
начало первого file payload. ABI regression воспроизвёл отказ всего Open
при E_ABORT на этих ненужных bytes. Теперь читается общий 96-byte prefix,
а остаток исходного legacy header запрашивается только для versions 1–3
с прежним endian interpretation. Field parser, version/size checks и методы
декодирования сохранены. Ошибка первого файла проявляется при его чтении,
другой доступный файл можно прочитать независимо.

Исходная проверка нулевого выравнивания сохранена как необязательное уточнение
физического размера. Она выполняется только при наличии полного padding
в известном input size; без padding не нужен даже Seek к концу ФС.
При обычном отказе Seek/Read, unsupported source operation или коротком
результате остаётся размер из superblock. E_ABORT и E_OUTOFMEMORY передаются
вызывающему коду. Только полностью проверенные нули увеличивают physical size;
ненулевой либо непрочитанный хвост не объявляется частью архива.
Это применяется к обоим engines и их общему optional-padding code.

Нечитаемый padding не означает неполную ФС: данные и metadata внутри
`bytes_used` не потеряны, поэтому новый limited mode/согласие не вводится.
Известная неполнота реальных filesystem records по-прежнему использует прежние
diagnostics/согласие/badge. Generic archive tail reporting сохранён;
`CheckZerosTail` является отдельной явно включаемой archive-search опцией,
которая не включается по умолчанию или partition properties MBR/GPT/APM.
Source bounds, реальные ошибки data/metadata и cancellation не ослаблены.

До исправления ABI regressions выявили семь отказов header/padding paths.
После исправления проверяются Seek/Read E_FAIL/E_NOTIMPL/S_FALSE, фатальные
Abort/OOM, отсутствие probe за available input и при aligned bytes_used,
physical-size fallback, отсутствие ложной неполноты и точные file bytes.
CLI проверяет padded/unpadded/short/nonzero/aligned variants на обоих engines.
Native Linux `6.8.0-147-generic` подтвердил минимальный для этой конфигурации
device-block padding 1024B, ненулевой хвост и aligned bytes_used
(`20261022T042615Z-388ad898`). Синтетические loop,ro,nosuid,nodev,noexec images
не изменились; mounts/temp очищены, helper удалён. Kernel block-device minimum
не является требованием archive byte stream: более короткий input проверяется
через ABI/CLI. Native отказов I/O не инжектировали; это отдельная ABI evidence.

На macOS прошли пять backend/source suites за 6,96 с и четыре UI consumers
за 38,81 с; UI повторены вне sandbox после отсутствия Cocoa screens при первом
запуске. На Linux все девять прошли за 19,07 с
(`20261022T042657Z-a2f9927c`). На Windows восемь прошли в том же batch;
существующий backend codec-discovery case в ABI suite получил timeout.
Повтор только этого suite без изменений кода/таймаутов прошёл за 9,26 с
(`20261022T042939Z-379371c6`); остальные прошедшие suites не повторялись.
Intermittent discovery timing не считается доказанной регрессией handler.
Incremental macOS app build и diff check прошли. Full/slow, физические
устройства и visual parity не проверялись.

## Завершение compressed block на первом stream, 2026-10-22

Сверены полные pinned Linux `zlib_wrapper.c`, `xz_wrapper.c`,
`zstd_wrapper.c`, `block.c` и decompressor setup/options с SDK `Decompress`,
Zlib checksum wrapper, `C/XzDec.c` ST/MT/default/one-buffer interfaces,
`C/ZstdDec.c` frame status/result reporting и metadata/data consumers.
Linux baseline — `58161ead29e50d70a317ea26b472e0c7508e69da`, SDK —
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`; retrieval links ниже.
Linux wrappers завершаются на первом stream/frame end. Portable adapter
требовал полного расходования packed input либо продолжал следующие streams,
из-за чего доступные Linux данные могли отклоняться или смешиваться с хвостом.

GPL v4 adapter теперь завершает Zlib/XZ/Zstd на первом проверенном stream.
Сохраняются checksums, header/Index/footer checks, output bounds и ошибки
незавершённого либо повреждённого первого stream; последующие bytes не
декодируются в его результат. LZ4/LZO сохраняют исходные full-input contracts.
Legacy/core-only SquashFS framing не расширялся. Нового ограничения открытия,
режима неподтверждённых данных или обхода source/path bounds не введено.

В SDK нет готового вызова, останавливающего полное XZ-декодирование после
первого verified footer: существующие full/partial interfaces продолжают
padding и следующие streams, а random-block interface не проверяет stream
Index/footer. Поэтому полные `C/XzDec.c` и `C/Xz.h` перемещены в `src/C/`;
единственный parser получил узкий one-stream entry point и возврат после
существующей footer check. Default public entry points вызывают то же тело
с прежним продолжением. Disk parsing, filters, checks, ST/MT branches не
переписаны. C язык, notices и upstream paths сохранены; manifests/overlays
обоих engines и все локальные consumers используют одну реализацию.

CLI/ABI regressions покрывают Zlib/XZ/Zstd data и inode metadata с произвольным
хвостом и вторым stream, independent Unicode sibling и retained stream.
XZ дополнительно проверяется с несколькими blocks, включая zero-output
blocks. Повреждение или обрыв первого stream дают ошибку без ложного payload.
Native Linux `6.8.0-147-generic` подтвердил все 14 положительных controls
(`20261022T041203Z-98c85159`), loop,ro,nosuid,nodev,noexec; source hashes
сохранены, mounts/temp очищены, одноразовый helper удалён. Native kernel
и pinned source остаются разными evidence. Zstd data fixture использует
RLE frame, помещающийся в expected output capacity native direct reader.

Тринадцать focused CLI/ABI/C API/shared-consumer/source suites прошли на
macOS/Linux за 6,06/11,96 с. После compile-time изоляции single-stream branch
от core-only MSVC constant-condition warning повторены затронутые SquashFS
три suites на macOS/Linux за 6,45/6,00 с; Linux receipt
`20261022T041441Z-bdfecd7e`. На Windows все тринадцать прошли за 74,02 с
(`20261022T041439Z-5cad7a16`). Source baseline verification и incremental
macOS app build прошли. ZIP writer validation теперь использует подготовленный
official runtime проекта, без зависимости от установленного через PATH `7zz`.
Full/slow suites, физические устройства и native UI не проверялись.

## Представление данных в block cache, 2026-10-22

Повторно сверены Linux `block.c:squashfs_read_data`, `file.c` regular/fragment
read branches, полные `file_cache.c`, `file_direct.c`, cache lookup/fill и
SDK `ReadBlock`, `ClearCache`, decoder и returned-stream consumers.
Baselines: Linux `58161ead29e50d70a317ea26b472e0c7508e69da`, SDK
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`; retrieval links ниже.
Linux раздельно обслуживает data и fragment caches либо читает data прямо
в page cache. Portable handler использует общий decoded block cache для
data и fragments; его исходный ключ включал только source address и длину.

Воспроизведена успешная выдача неверного хвоста файла в обоих engines:
data block и fragment ссылаются на одинаковый packed range, но задают
противоположные compression flags. После первого чтения повторно выдавалось
другое представление тех же source bytes. При смене представления источник также
ошибочно не получал повторный запрос. Исправлен ключ существующего SDK cache:
добавлен compression flag, сбрасываемый вместе с cache и публикуемый только
после успешного чтения/декодирования. Parser, decoder bodies и stream control
flow сохранены. Изменение применяется и к GPL-модулю, и к standalone engine.
Одинаковое корректное представление по-прежнему может использовать cache.

ABI regression проверяет оба направления переключения, точный payload,
E_ABORT и E_FAIL при повторном source read после уже выданного полного блока.
CLI regression до исправления воспроизвёл четыре неверных результата:
два направления в каждом engine; после исправления проверяет точные данные
и независимый Unicode sibling. Native Linux `6.8.0-147-generic` подтвердил
оба файла в обоих controls (`20261022T040006Z-705eacee`), loop,ro,nosuid,nodev,
noexec. Source hashes неизменны, mounts/temp удалены, helper удалён после
проверки. Native kernel и pinned source — отдельные evidence.
Нового запрета открытия или согласия на эти читаемые данные не введено;
source access остаётся read-only, ошибки источника не скрываются cache hit.

После incremental пересборки обоих engines пять focused CLI/ABI/C API/source
suites прошли на macOS/Linux/Windows за 6,13/6,94/36,50 с; runner receipt
`20261022T040039Z-95b83af0`. Incremental macOS app build прошла.
Full/slow suites, физические устройства и native UI не проверялись.

## Метаданные специальных inode, 2026-10-22

Сверены полные Linux `inode.c:squashfs_read_inode` branches для BLKDEV,
CHRDEV, FIFO, SOCKET и четырёх extended variants, их disk structs,
`include/linux/kdev_t.h:new_decode_dev`, SDK `Parse4`, properties и synthetic
xattr consumers. Baselines остаются Linux
`58161ead29e50d70a317ea26b472e0c7508e69da` и SDK
`9128b80e3a471108678e2b3ec3c985861c8d7b0f` с retrieval links ниже.
SDK содержал исходное чтение `RDev` в закомментированном виде: запись
распознавалась, но major/minor не публиковались.

В существующем v4 parser восстановлено чтение этого поля. GPL format-модуль
выдаёт стандартные ABI properties Device Major/Minor только для четырёх
device types. Для преобразования повторно используется общий Linux kdev_t
header: добавлены исходные `MKDEV` и полное тело `new_decode_dev`, с portable
32-bit типами вместо kernel `dev_t`/`u32` и обычным `static inline` вместо
kernel attribute. Encoders, VFS printing и остальные helpers не требуются.
Header закреплён также в SquashFS manifest; HFS использует прежние macros.
UDF теперь получает `MKDEV` из этого единственного header вместо дубликата
в host declarations; его caller передаёт те же unsigned 32-bit значения.
FIFO/socket properties, mode/link count и extended xattr parsing сохранены.
Synthetic xattr streams не наследуют device properties родительского inode.
Core-only и legacy device publication не расширялись.

Регрессии проверяют через ABI все восемь special types, zero/high/maximal
major/minor, mode/link count, xattr payloads и отсутствие device numbers у
обычных файлов, symlinks и xattr streams. CLI проверяет листинг и извлечение
обычных данных и атрибутов; файлы устройств на хосте не создаются.
Native Linux `6.8.0-147-generic` подтвердил `stat` восьми special inode,
обычный payload и symlink на синтетическом 4 KiB loop,ro,nosuid,nodev,noexec
образе (`20261022T035159Z-c286a98d`). Сами device entries не открывались;
source hash неизменен, mount/temp data очищены, helper удалён.
Это metadata read feature, без source writes, repair, replay или нового
ограничения полезного read-only доступа. Native kernel и pinned source
составляют отдельные свидетельства.

Итоговые восемь focused suites (SquashFS CLI/ABI/C API, HFS ABI, UDF CLI/ABI
и source contracts) прошли на macOS/Linux/Windows за 44,02/52,13/110,44 с;
runner receipt `20261022T035426Z-b1cc2734`. Оба engines пересобраны из текущих
исходников перед acceptance; incremental app build на macOS прошла.
Full/slow suites, физические устройства и native UI не проверялись.

## Потоковое чтение inode metadata, 2026-10-22

Сверены Linux `inode.c:squashfs_read_inode`, `cache.c:squashfs_read_metadata`
и copy/cache contracts, SDK `CNode::Parse4`, canonical lookup, recovery,
properties, packed-size и stream consumers. Baselines: Linux
`58161ead29e50d70a317ea26b472e0c7508e69da`, SDK
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`; retrieval links ниже.
Linux VFS inode construction не является готовым archive ABI. Сохранён
исходный SDK field parser; узкий availability hook получает продолжение
record через перенесённый C `squashfs_read_metadata`. Variable directory
indexes разбираются исходным циклом без повторного прохода каждого prefix.

GPL v4 adapter больше не сохраняет общий decoded inode table и UInt32
offsets в нём. Canonical references и endpoints содержат physical metadata
block и decoded offset; отрицательный cache относится к конкретной ссылке.
MTime хранится в parsed node, symlink payload принадлежит node и сохраняет
все байты, включая NUL. Display property сохраняет прежнее представление,
а raw stream возвращает полный payload и переживает Close обработчика.
Все прежние raw-table consumers сверены: properties, symlink streams,
regular streams и packed sizes. Legacy/core paths сохраняют прежние buffers.

Чтение inode использует те же временные range bounds, что directory reader,
с восстановлением ancillary bounds после успеха и ошибки. Packed boundary
проверяется до payload I/O. C cache/copy/read_metadata algorithms не менялись.
Обычное открытие не загружает blocklist или optional directory indexes.
Только уже закэшированный полный single-block inode table может сохраняться
как bounded snapshot для прежних count/unused/duplicate diagnostics; getter
не выполняет I/O. Размер snapshot не превышает одного metadata block.

Полный recovery inventory читается последовательным cursor: regular blocklists
пропускаются исходным NULL-buffer metadata reader, extended-directory indexes
проверяются полностью. Mapping для восстановления публикуется только после
точного count/end и отсутствия неполноты; observed canonical candidates сами
по себе не подтверждают inventory. Пересечения record extents запрещают
такое восстановление, сохраняя независимое canonical чтение доступных данных.
Abort/OOM/unsupported и callback results сохраняются. Ошибки и неполные
результаты используют прежние diagnostics, согласие и видимый режим.
Это соответствует политике полезного read-only доступа без source writes,
repair или replay; согласие не подменяет доказательство recovery mapping.

Transient memory ограничена record, metadata block и skip chunk вместо всей
decoded таблицы. Максимальные variable records ограничены форматом; retained
node/name/link catalog масштабируется с реальными entries. Реальный образ
с decoded inode table больше 4 GiB и slow matrices не проверялись.

Новые ABI/CLI fixtures проверяют blocklist на нескольких metadata blocks,
переход symlink через block boundary, binary/Unicode targets, MTime, потоки
после Close и запрет recovery по ложному inode внутри чужого blocklist.
C API проверяет inode cursor и восстановление ID/directory access после
отклонённого выхода за bounds. Sparse fixture не материализует большой файл:
проверяются несколько коротких чтений, сами образы занимают по 4 KiB.
Native loop,ro Linux `6.8.0-147-generic` подтвердил clean/recovery controls,
данные, MTime, symlink display/raw length и sparse samples; невалидная
canonical ссылка даёт EIO. Number-based recovery остаётся явно обозначенным
расширением reader, а не поведением native kernel. Receipt
`20261022T033608Z-fa32a857`; source hashes не изменились, mounts/temp очищены,
одноразовый helper удалён. Runtime kernel и pinned source — разные evidence.

На macOS ABI/CLI/C API прошли за 6,23 с, семь связанных stream/UI/source suites
за 31,98 с; вместе покрыты девять выбранных suites. На Linux/Windows все девять
прошли за 17,50/68,64 с (`20261022T033838Z-1f9cfdd4`). Incremental macOS app
build прошла. Full/slow acceptance и физические устройства не проверялись.
Эта итерация закрывает aggregate inode-cache scope; полная source-актуализация
SquashFS и общая проверка остальных форматов остаются открытыми.

## Потоковое чтение directory metadata, 2026-10-22

Сверены полные Linux `dir.c:squashfs_readdir`, directory index helpers,
`cache.c:squashfs_read_metadata`/copy/cache contracts и SDK `OpenDir`,
directory field parser, `GetPath`, callers, item storage, synthetic root и
xattr paths. Baselines прежние: Linux
`58161ead29e50d70a317ea26b472e0c7508e69da`, SDK
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`; retrieval links ниже.
Linux читает header, entry и filename по metadata cursor, не сохраняет весь
decoded directory table ради последующего получения имён.

GPL v4 adapter теперь использует исходный C `squashfs_read_metadata` для этих
полей. Сохранены SDK parser/control flow, порядок полей, endian/type/count/
UTF/path checks и алгоритм построения пути. Узкий field hook заменяет backing
buffer; завершённые имена принадлежат portable `CObjectVector<AString>`.
`GetItemName` выносит прежнее получение имени из двух проходов `GetPath` и
выбирает owned name для v4; core/legacy используют прежние raw records.
Synthetic root/alternate-stream names по-прежнему обрабатываются отдельно.
V4 больше не вызывает directory `ReadMetadataRun`, не накапливает decoded
directory bytes/offset maps и не зависит от UInt32 aggregate directory cache.
Retained item/name catalog масштабируется по реально выданным entries;
inode decoded cache и существующие resource limits остаются отдельным scope.

C host API ограничивает cursor directory range, проверяет bounds также для
cache hits и восстанавливает ancillary range после успеха/ошибки. Тот же range
mechanism сохраняет прежние постоянные inode bounds file blocklist reader.
CPP metadata bridge передаёт packed end в исходный `ReadMetadataBlock` до
payload I/O как для directory fields, так и для file creation/block lookup.
Два ABI regressions воспроизвели нежелательный Abort при malformed packed
length, пересекающей соседнюю таблицу. После исправления невозможный block
отклоняется до такого чтения; partial directory/file result остаётся локальным,
packed size неизвестен, доступный file prefix сохраняется.
Сам Linux cache/copy/read_metadata algorithm не переписан. Metadata cache и
field buffer ограничены одним block и максимальной длиной поля независимо
от суммарной длины directory table. Partial field output не публикуется;
полностью прочитанные records до ошибки остаются доступны. Fatal source
Abort/OOM/unsupported и callback results сохранены, checkpoints работают
при чтении и завершении каталога. Реальные неполные каталоги используют
прежние diagnostics/согласие/badge и copy results.

C API regression проверяет directory cursor и восстановление ID access после
отклонённого выхода за directory bounds. ABI/CLI проверяют header/entry/name,
пересекающие короткие смешанные compressed/raw blocks, и отказ в хвосте имени:
доступные prefix entries и вложенные файлы сохраняются, недочитанное имя не
выдаётся. Существующие consumers покрывают shared blocks, unused metadata,
wide physical spans, recovery, xattrs, callbacks, Close/reopen и legacy paths.
Native readonly Linux `6.8.0-147-generic` подтвердил четыре controls:
short/short-bad/shared/unused; receipt `20261022T023056Z-c865eb9c`.
Listing/payloads совпали, source hashes не изменились, mounts/temp data очищены,
одноразовый helper удалён. Native kernel и pinned source — разные evidence.
Большие реальные directory images и slow matrices не запускались.

На macOS прошли ABI/CLI/C API три suites за 6,07 с и шесть остальных direct
consumers за 39,79 с. На Linux/Windows девять suites прошли за 19,76/59,97 с,
receipt `20261022T023251Z-75963aae`. После финальных packed-boundary fixes
повторно прошли затронутые три suites на macOS/Linux/Windows за
5,58/5,16/22,48 с (`20261022T024443Z-df80c193`); остальные consumers не
повторялись. Incremental macOS app build прошла. Full/slow acceptance и
физические устройства не проверялись; source completeness и runtime evidence
учитываются отдельно.

## Необязательные индексы extended directories, 2026-10-22

Сверены полные Linux `inode.c:squashfs_read_inode` (LDIR branch),
`dir.c:get_dir_index_using_offset`/`squashfs_readdir`,
`namei.c:get_dir_index_using_name`/`squashfs_lookup`, metadata/cache dependencies
и SDK `Parse4`, canonical/cache lookup, inventory/recovery, directory traversal
и свойства. Baselines остаются Linux
`58161ead29e50d70a317ea26b472e0c7508e69da` и SDK
`9128b80e3a471108678e2b3ec3c985861c8d7b0f` с retrieval links ниже.
Linux загружает заголовок LDIR отдельно от variable-length index и допускает
последовательное чтение при отказе необязательной подсказки поиска.

GPL v4 canonical path теперь использует полный фиксированный заголовок LDIR
без обязательного разбора индекса; SDK field parser, формат и directory reader
сохранены. Последовательное archive enumeration не использует эти hints.
Обычный cache-only inventory scan не требует индекса только ради глобального
подтверждения: он оставляет inventory недоказанным, а не объявляет его полным.
Для фактического number-based recovery сохраняется полный строгий разбор,
включая variable extent уже закэшированного LDIR. Неполный inventory не
разрешает подменить невалидный canonical reference догадкой по номеру.

Отказ полного inventory не помещает fixed header в negative cache canonical
чтения. Regression воспроизвёл потерю доступного child directory после попытки
recovery другой записи; после исправления этот child и его файлы доступны.
Abort/OOM/unsupported при действительно необходимом чтении inventory остаются
fatal. Проверки inode/header, bounds и путей не ослаблены. Непрочитанный либо
ненужный индекс не создаёт ложный limited mode; известные реальные ограничения
по-прежнему используют существующие согласие, badge и честные copy errors.
Core/legacy paths используют прежний полный record scan.

ABI/CLI проверяют отдельный index block (включая E_ABORT, который не должен
читаться), header/checksum faults, malformed name/count, индекс внутри уже
прочитанного block, hardlinks/Unicode, строгий recovery и independent child
после его отказа. EN/RU UI проверяет открытие/копирование/refresh/reopen без
ненужного предупреждения, сохраняя проверки реального partial mode.
Native readonly Linux `6.8.0-147-generic` подтвердил семь controls, включая
сохранность списка, данных, symlinks/xattrs и доступного child при невалидном
соседнем reference; receipt `20261022T021356Z-d7bc4947`. Образы не изменились,
mounts/temp data очищены, одноразовый helper удалён. Native kernel старше
source baseline, его runtime evidence учитывается отдельно.

Девять focused suites прошли на macOS/Linux/Windows за 39,24/19,94/72,33 с;
remote receipt `20261022T021527Z-e1892190`. После исправления permissions/hint
в новом synthetic child fixture ABI/CLI повторно прошли на macOS за 5,33 с;
product code не менялся. Incremental macOS app build прошла. Full/slow suites
и физические устройства не проверялись; build trees/compiler caches сохранены.

Суммарный namespace decoded cache по-прежнему ограничен UInt32; эта итерация
не заявляет устранения всего metadata-size scope или завершения SquashFS audit.

## Blocklist файлов по запросу, 2026-10-22

Эталон нового C-переноса: Linux
[`58161ead29e50d70a317ea26b472e0c7508e69da`](https://github.com/torvalds/linux/tree/58161ead29e50d70a317ea26b472e0c7508e69da),
`fs/squashfs/file.c`. Перед изменением и после него сверены полные
`locate_meta_index`, `empty_meta_index`, `release_meta_index`, `read_indexes`,
`calculate_skip`, `fill_meta_index`, `read_blocklist_ptrs`/`read_blocklist`,
их metadata/cache/header dependencies, `inode.c` и SDK parser/stream callers.
SDK baseline остаётся `9128b80e3a471108678e2b3ec3c985861c8d7b0f` из общего аудита.

В `src/fs/squashfs/file.c` перенесены эти тела и исходные notices; файл
компилируется как C в том же отдельном GPL module. Dependency fields inode,
superblock/meta-index и allocation adapter подключены к существующему C host.
Внутренние blocklist entry points получили module prefix при экспорте в host;
последнее чтение descriptor выделено в общий helper для sequential cursor.
Index-slot selection, skip calculation, lookup/growth/error cleanup сохранены.
Host сериализует обращения, поэтому kernel mutex adapters остаются no-op.
Сохраняются bounded metadata cache и собственный index cache потока; размеры
его mapping arrays не зависят от размера файла. Ancillary tables для него
не загружаются. Host lifetime/cache API оставлен вместе с metadata service,
чтобы не дублировать его ownership и bounds checks.

Folio/page-cache/readahead и VFS file operations из второй части `file.c`
не переносятся: SDK cached IInStream, sparse/fragment handling и decoder
выполняют чтение archive bytes. VFS SEEK_HOLE/DATA не входит в IInStream ABI.
GPL v4 canonical inode требует только заголовок regular file; blocklist
больше не копируется целиком в inode cache или offset arrays при GetStream.
Seek/read запрашивают mappings через C reader. Последовательный cursor
использует тот же descriptor helper, избегая повторного scan предыдущих
descriptors. Packed-size property вычисляется отдельно, линейно; ошибка
делает её неизвестной и диагностируется, но не запрещает полезное чтение.
Extraction progress использует encoded-byte counters принадлежащего потоку
reader без предварительного обхода blocklist. Старые offset arrays копируются
только legacy-потокам: новый regression воспроизвёл неверное чтение fragment
после v3 → v4 reopen и теперь проверяет также сохранённый старый поток.
Decoded metadata cache для namespace по-прежнему ограничен UInt32: эта
итерация устраняет хранение blocklist в нём, а не весь оставшийся size scope.

Поздний повреждённый metadata block не скрывает заголовок/размер и читаемое
начало файла. Copy возвращает доступный prefix и DataError; sibling читается
полностью. Abort/OOM/unsupported/source failures сохраняют свои результаты.
Ошибка, ещё не встреченная при обычном открытии, не выдаётся за заранее
диагностированный limited mode; известные ограничения открытия сохраняют
прежние выбор пользователя и badge. Две новые диагностики локализованы.

ABI/CLI/UI проверяют prefix/error и independent sibling, readonly source,
basic/extended headers, sparse/tail reads, seek, поток после Close/reopen и
fatal errors. C API дополнительно проверяет sparse mappings через несколько
index-cache entries и возврат к читаемому началу после ошибки позднего блока
на малом metadata source без большого файла/матрицы. Native readonly Linux
`6.8.0-147-generic` подтвердил clean/header/checksum controls, размер файла,
8192-byte prefix, EIO позднее и healthy sibling; receipt
`20261022T014850Z-2f924233`. Источники побайтно не изменились, mounts/data
очищены, одноразовый helper удалён. Native kernel и закреплённый source
baseline учитываются отдельно.

Девять ordinary focused suites прошли на macOS/Linux/Windows за
36,46/16,38/92,04 с; remote receipt `20261022T014955Z-c093b0fe`.
После локализации и адресных уточнений повторялись только affected consumers:
language/error policy/inode UI — macOS и оба runner
(`20261022T015313Z-ca5d264f`), extraction/stream/read UI — все три ОС
(`20261022T015536Z-e33fdf7f`). Финальные ABI/CLI с regression v3 → v4 прошли
на всех трёх ОС (`20261022T015820Z-d6d3a458`). Incremental macOS app build
прошла. C source comparison и runtime acceptance учитываются отдельно;
full/slow suites и физические устройства не проверялись.

## Inode metadata по запросу, 2026-10-22

Повторно сверены все type branches Linux `inode.c:squashfs_read_inode`,
`namei.c:squashfs_lookup`, `cache.c:squashfs_read_metadata` и SDK `Parse4`,
его callers, root/directory traversal, inode-number recovery и stream/property
consumers. Используются прежние полные Linux/SDK baselines из этого аудита.
Linux читает metadata по canonical inode reference и продолжает запись через
границы metadata blocks, не сканируя предварительно всю таблицу inode.

GPL v4 adapter теперь запрашивает первый block нужного inode и расширяет
contiguous run только при недостатке bytes для исходного SDK parser.
`Parse4` сообщает необходимый размер, сохраняя ветви разбора полей; отдельного
parser нет. Partial decoder output и gaps не соединяются с независимыми
записями. Перекрытие повреждённой записи не запрещает bounded разбор соседнего
canonical reference; полный inventory при этом считается недостоверным.
Старые cache offsets остаются стабильными при расширении run.

Общий scan не вызывает дополнительного I/O, если весь contiguous inventory
уже в cache. Полная загрузка для number-based recovery выполняется только
при фактической необходимости и требует доказанного полного inventory без
конфликтов. Неполные mappings не разрешают догадку по inode number.
Сохраняются partial-read diagnostics, согласие, badge и fatal Abort/OOM/
unsupported decoder/callback results. Убран 32-битный gate на physical span
inode table; cumulative decoded-cache extent пока ограничен UInt32.
Это оставшийся scope, а не заявление полной metadata-size parity.

ABI/CLI проверяют basic/extended records на коротких metadata blocks,
extended directory index и symlink suffix, hardlinks и Unicode payloads.
ABI также ставит E_ABORT на посторонний блок, который не должен читаться,
и использует виртуальный source со span inode table свыше 4 ГиБ.
Native readonly Linux `6.8.0-147-generic` подтвердил пять controls, включая
symlink target и xattr values; receipt `20261022T013349Z-3c3ba876`.
Wide-span sparse image занял 12 КиБ физически. Проверены записанные regions,
size/allocated blocks/mtime источника; gap не хешировался и не сканировался.
Mounts/temp data очищены, одноразовый helper удалён. Native kernel старше
закреплённого source baseline, эти свидетельства учитываются отдельно.

Обычные девять focused suites прошли на Linux/Windows за 16,26/63,40 с,
receipt `20261022T013404Z-1006d4cf`. На macOS семь остальных suites прошли
в первоначальном batch; после исправления отсутствующего `nlink` в extended
directory fixture повторно прошли ABI/CLI suites за 4,72 с. Product code между
этими запусками не менялся. Incremental macOS app build прошла.
Full/slow suites, физические устройства и UI appearance не проверялись.

## Directory metadata ranges и 64-битные physical spans, 2026-10-22

Сверены полные Linux `dir.c`/`namei.c`, `cache.c:squashfs_read_metadata`, SDK
`ReadData`, directory traversal и потребители cached raw offsets. Linux читает
блоки по directory inode reference и длине каталога, а не требует заранее
загрузить весь span directory table. Предыдущая адаптация декодировала table
целиком, включая недостижимые blocks, и ограничивала её physical span 32 битами.

GPL v4 reader теперь загружает bounded run при обращении к каталогу. Если
каталог делит первый block с уже прочитанным каталогом, но продолжается дальше,
его запрос расширяет доступный contiguous run. Append cache сохраняет старые
item offsets; partial output после decoder failure не публикуется, gaps не
склеиваются. Недоступный посторонний block больше не нужен для открытия.
При ошибке нужного блока сохраняются прежние partial directory diagnostics,
согласие и badge; Abort/OOM/unsupported decoder и callbacks остаются fatal.

Physical packed positions и run arithmetic расширены до UInt64. Чтение одного
metadata block по-прежнему ограничено исходным SDK пределом; сами directory
references и decoded-cache offsets проверяются отдельно. Обновлён общий тип
packed-position vector без изменения legacy/core parser flow. Изначальная
загрузка inode table и пределы суммарного decoded cache пока не устранены;
полная поддержка всех metadata sizes Linux не заявляется.

ABI/CLI покрывают совместный block и продолжение каталога, а ABI дополнительно
ставит E_ABORT на недостижимый block. Малый виртуальный source проверяет
compressed directory block, пересекающий 32-битную boundary: gap не выделяется
в памяти, попытка его сканировать немедленно прерывается. Native readonly
Linux `6.8.0-147-generic` подтвердил shared/unused controls и wide physical span
(`20261022T011228Z-2e9bc929`), затем actual cross-boundary block
(`20261022T011443Z-8c057bfc`). Последний sparse image занимал 8 КиБ физически;
файлы совпали побайтно. Проверены записанные regions, size/allocated blocks и
mtime источника после mount; весь sparse gap не хешировался и не сканировался.
Mounts/data удалены, память/кэши runner не менялись.

Девять focused suites на macOS/Linux/Windows прошли за 64,68/15,66/61,74 с,
receipt `20261022T011312Z-7258eafd`. После усиления boundary fixture ABI suite
повторно прошла на macOS/Linux/Windows за 4,89/3,14/6,87 с,
receipt `20261022T011513Z-8c16e790`. Incremental macOS app build также прошла.
Full/slow suites, физические устройства и изменения UI appearance не запускались.

## Зависимости inode вне namespace, 2026-10-22

Сверены Linux `namei.c:squashfs_lookup`, `inode.c:squashfs_iget`/read branches,
`id.c`, `fragment.c` и xattr lookup/read dependencies с SDK inventory scan,
root/directory traversal, number-based recovery, hardlinks и metadata consumers.
Linux запрашивает зависимости при обращении к inode; недостижимый inode сам
по себе не требует чтения своих UID/GID, fragment entry или xattr values.
Прежний archive scan выполнял такие обращения и мог отвергать весь образ
при ошибке данных, не нужных его namespace.

GPL v4 adapter разделяет исходный `Parse4`/reference inventory и загрузку
зависимостей. Полный доступный inventory сохраняется для проверки конфликтов
inode numbers и ограниченного recovery; новый decoder не создаётся. После
разрешения root/directory reference исходные Linux readers загружают нужные
UID/GID, fragments и xattrs. ID entries кэшируются по исходному index;
публикуются только действительно загруженные свойства. Недостижимые записи
не вызывают чтения ancillary payloads и ложного предупреждения об ограничениях.

Сохраняются проверки record bounds/type/count, неопределённые identities при
неполной таблице, конфликтующие numbers и unverified recovery. Достижимые
ошибки по-прежнему дают неизвестные свойства/неполный результат с согласием;
fatal source/callback results останавливают открытие. Shared hardlinks не
инициализируют зависимости повторно. Close очищает состояние, streams
сохраняют собственные mappings/leases. Linux C readers не менялись, legacy/
core-only scope не расширялся.

Три малых fixtures содержат дополнительный корректно разбираемый inode,
отсутствующий в каталоге: с отдельным ownership metadata block, fragment block
или xattr set. ABI устанавливает source E_ABORT на соответствующем ancillary
block и проверяет успешное открытие, точные данные, xattr/UID и отсутствие ложной
read-only limitation. CLI проверяет тот же namespace и все payloads. Native
readonly Linux `6.8.0-147-generic` подтвердил три образа: файлы/hardlinks и
`user.inline` совпали побайтно; образы не изменились, mounts/data удалены.
Receipt `20261022T010427Z-14ac2193`. Девять focused suites на macOS/Linux/Windows
прошли за 69,18/15,55/68,38 с; remote receipt `20261022T010457Z-e2acd07f`.
Incremental macOS app build также прошла. Full/slow suites и физические
устройства не проверялись. Whole-metadata caches и их пределы остаются отдельной незакрытой частью
общей source-сверки; namespace-lazy directory/inode metadata не заявляется.

## Загрузка fragment entries по inode references, 2026-10-22

Сверены полные Linux `fragment.c` loader/lookup, regular/extended regular
branches `inode.c` и SDK `Parse4`, block mapping, properties и retained streams.
Linux загружает fragment index при mount, затем читает конкретную entry при
разборе inode. Прежний archive adapter обходил весь declared fragment count
и выделял массив соответствующего размера, включая неиспользуемые entries;
большой count мог отвергнуть пригодное чтение либо потребовать лишнюю память.

GPL v4 adapter теперь вызывает исходный Linux lookup только для fragment IDs,
встреченных в разобранных regular inodes. Одна полученная entry используется
всеми её потребителями; внутренний компактный индекс сохраняет исходные SDK
block/stream paths. Count/range проверяет Linux reader, missing entry остаётся
недоступной. Прежние partial-prefix errors, packed-size uncertainty, fatal
source/callback results и независимое время жизни streams сохраняются.
Close очищает mapping. Linux C-код и legacy/core-only scope не менялись.

Новые ABI/CLI controls проверяют разные payloads при IDs в разных index blocks,
fragment ID выше прежнего eager-count предела и недоступный неиспользуемый
metadata block. Последний не читается даже при source E_ABORT на этом block.
Проверяются точные хвосты/полные payloads, hardlinks, interleaving и streams
после Close/reopen. Образ с большим ID содержит около 4 МиБ index, а не массив
всех объявленных fragment entries; это обычная focused проверка, не slow suite.

Native Linux `6.8.0-147-generic` подтвердил точные payloads для разных fragment
blocks и неиспользуемого повреждённого блока. Большой index получил ENOMEM при
mount: его native payload acceptance не подтверждён. Runtime failure не
подменён форматом/искусственным отказом reader и не обходился изменением памяти
или кэшей runner. Образы не изменились, mounts/data удалены; receipt
`20261022T005520Z-dc2abf18`. Девять focused suites на macOS/Linux/Windows прошли
за 68,41/14,17/91,39 с; remote receipt `20261022T005546Z-0f658519`.
Incremental macOS app build также прошла; full/slow и физические устройства
не проверялись. Общая сверка продолжается, включая существующие whole-metadata
cache пределы и предварительный разбор inode records вне достижимого namespace.

## Недоступные служебные indexes: ограниченное чтение, 2026-10-21

Сопоставлены полные Linux `super.c:squashfs_fill_super`, `id.c`, `fragment.c`,
`export.c`, `xattr_id.c`, их headers/macros, metadata readers и SDK consumers.
Linux отказывает mount при ошибке обязательного bootstrap, хотя обычные файлы
адресуются через root/directory refs, а NFS export index не участвует в этом
чтении. Read-only archive adapter теперь изолирует ошибки ancillary tables:
это явное отличие от mount acceptance по политике проекта, а не утверждение,
что Linux принимает такие тома.

Исходные Linux table loaders и ID/fragment/xattr readers сохранены. Если
соседняя таблица не позволяет установить точный `next_table`, собственные
declared count/start и границы `bytes_used` ограничивают вызов loader.
Его внутренние проверки refs/order/gaps не заменяются новым parser.
Недоступные таблицы остаются NULL; обращения к ним возвращают ошибку.
Фактические inode/directory spans, decoder bounds и проверка имён сохраняются.
Directory cache ограничен доступными metadata boundaries, либо общим
`bytes_used`, когда более точную границу подтвердить нельзя. Это ограниченное
чтение с явной диагностикой, а не проверка целостности размещения таблиц.

Изолированы xattr ID table, ownership index, export index и fragment index,
включая start вне образа, invalid refs, неправильные counts и combined failure.
Сбой ownership не выдаёт нулевой UID/GID за известный; отсутствующие xattrs
не выдаются за полностью прочитанный набор. Сбой fragment index сохраняет
полные блоки файла, неизвестный packed size и DataError на недоступном хвосте;
файл без fragment dependency читается полностью. Непригодный index при huge
declared fragment count не вызывает allocation по этому count.

Raw I/O bridge сохраняет negative error code, чтобы Abort/OOM/неподдерживаемый
reader не потерялись при продолжении после локального сбоя другой таблицы.
Fatal C result имеет приоритет перед сохранённой ошибкой прежнего I/O.
Пользователь получает конкретное предупреждение о недоступных indexes и
существующие дополнительные причины, отдельное согласие и постоянный badge.
Новая диагностика переведена на русский. GPL module включается на всех трёх
ОС; core-only engine сохраняет прежние dependencies и strict checks.

ABI/CLI проверяют полезные payloads, неизвестные свойства, частичный stream,
здоровые независимые xattrs, Close/reopen, source E_FAIL и fatal source/callback
results для каждой таблицы. EN/RU UI проверяет принятие/отказ, копирование,
повторное согласие, geometry и неизменность исходного образа. Девять focused
suites на macOS/Linux/Windows прошли за 70,34/16,11/75,62 с; remote receipt
`20261021T161045Z-9814b0a1`. После уточнения приоритета fatal C result две
CLI/ABI suites повторно прошли на macOS/Linux/Windows за 4,45/3,74/11,36 с,
receipt `20261021T161401Z-95353adc`. Incremental macOS app build
также прошла. Остальные прошедшие suites не повторялись на неизменённом scope.

Native Linux `6.8.0-147-generic` отверг десять повреждённых bootstrap controls,
а чистый образ вернул точные payloads. Все образы остались неизменны,
mounts/data удалены; receipt `20261021T161011Z-e72c7126`. Runtime kernel старше
закреплённого source reference. Общая сверка SquashFS read scope продолжается;
в частности, существующие общие resource/metadata-size пределы ещё не дают
заявлять поддержку всего диапазона размеров Linux.

## Ошибки отдельных inode metadata blocks, 2026-10-21

Повторно сопоставлены полные Linux `inode.c` type branches,
`cache.c:squashfs_read_metadata` и связанные block/cache services с SDK
`ReadData`, `Parse4`, root/directory refs и потребителями сырых inode bytes.
Linux читает metadata по block/offset и переходит только к следующему
физическому блоку; предварительная успешная загрузка всей inode table ему
не требуется. VFS/cache lifetime не переносится в archive ABI, существующие
SDK field parser и декодеры сохраняются.

GPL v4 reader теперь сохраняет исправные inode metadata runs. После локального
сбоя продолжение использует известную packed boundary либо canonical root
reference; поздние directory references могут загрузить свой bounded run.
Невалидный header не используется для угадывания следующего блока. Повторно
загруженный физический диапазон сохраняет старые raw offsets; canonical inode
reference возвращает уже разобранный node, не создавая ложный конфликт identity.
Один общий metadata-run loader обслуживает inode и directory caches.

`Parse4` ограничен непрерывным успешно декодированным диапазоном. Частичный
output декодера после checksum error не публикуется, а данные после gap не
становятся продолжением предыдущего inode/blocklist. Метаданные и streams
недоступного inode не выдаются за корректные; исправные файлы читаются с
существующим per-opening предупреждением и badge. Неполная таблица сохраняет
непроверенные identities скрытыми. Abort/OOM/недоступный decoder и ошибки
callbacks не превращаются в partial success. Нечитаемый root остаётся
техническим препятствием открытию. Legacy/core-only чтение не расширялось.

Небольшие ABI/CLI fixtures проверяют bad header, Adler checksum после выдачи
output, source E_FAIL, независимые root/sibling refs, отсутствие склейки через
gap, Close/reopen и чистый inode, пересекающий границу двух metadata blocks.
Fatal source/callback controls теперь охватывают обе metadata tables.
EN/RU UI копирует исправный файл из обоих повреждённых образов, проверяя
согласие, badge, недоступность мутаций и неизменность источника.

Четыре native readonly controls на Linux `6.8.0-147-generic` подтвердили точные
байты исправного файла, EIO при чтении файла с повреждённым продолжением inode,
отказ mount при повреждённом root и чтение cross-block inode чистого образа.
Образы не изменились; mounts/data удалены. Receipt:
`20261021T155908Z-c01a660a`. Runtime kernel старше закреплённого source reference.
Семь focused suites на macOS/Linux/Windows прошли за 17,68/10,04/46,90 с;
remote receipt `20261021T155938Z-4ad8cfe1`. Incremental macOS app build также
прошла. Full/slow suites и физические устройства не запускались.
Initial index bootstrap впоследствии изолирован, как описано выше;
полнота SquashFS не заявляется.

## Локальные ошибки inode records, 2026-10-21

Проверены полные Linux `inode.c` branches и их VFS/cache dependencies,
`squashfs_fs_i.h`, `namei.c`, а также SDK `CNode::Parse4`, его потребители,
raw offsets, xattrs, directory anchors и hardlink identity. Linux читает
inode по ссылке из каталога; неизвестный или повреждённый соседний inode
не требует отказа всему открытию. Целиком подключить VFS `squashfs_iget`
к текущему archive ABI нельзя без нового VFS lifetime/type layer.
Поэтому сохраняется один исходный SDK parser; codec и inode field parsing
не реализуются заново.

GPL v4 handler использует тот же `Parse4`/xattr flow через `ReadNode` как при
последовательном разборе, так и для записей по canonical root/directory refs.
После первой неразбираемой записи он не сканирует байты в поисках похожих
headers: используются только адреса, предоставленные root и проверенными
directory entries. Некорректный record остаётся недоступным; исправный sibling
и root после него могут читаться. Ошибка count не задаёт размер выделения
памяти без учёта фактически полученного inode buffer. Неразбираемый root
остаётся реальным ограничением opening, а не подменяется искусственным каталогом.

Для неполной таблицы подавляются непроверенные `INode`/`StreamId` и подбор
record по inode number: отсутствие скрытых конфликтов не доказано. Сохраняются
проверки metadata block/offset, фактического buffer extent, type/mode, record
fields, paths и доступности decoder. Если повреждённая длина одной записи
захватывает соседние, canonical refs могут указывать на отдельно проверяемые
records; это не даёт права читать за пределами inode metadata. Изменение
требует отдельного per-opening согласия и badge; недоступные элементы имеют
неизвестные свойства/ошибку, копирование исправных данных остаётся доступным.
Abort/OOM и callbacks не превращаются в partial success. Legacy/core-only
scope сохраняется.

Поздно обнаруженный directory inode может загрузить собственный bounded
metadata run, даже если первоначальный проход остановился на чужом bad header.
Кэш добавляет только законченные блоки, сохраняет старые item offsets и
границы разрывов. Arena растёт с запасом, но logical extent отделён от capacity:
неинициализированный запас не становится metadata. Это сохраняет оба ранее
проверенных сценария — локальный inode failure и directory block isolation —
при их совместном возникновении.

Семь native readonly Linux `6.8.0-147-generic` controls подтвердили доступность
исправных payloads при invalid/unused inode type, неверном count, завышенной
длине предыдущей записи и combined inode/directory failure. Неподдерживаемый
inode возвращает EINVAL, невалидный root отвергается при mount. Образы не
изменились, mounts/data удалены. Receipt: `20261021T154236Z-95808f13`;
runtime kernel старше закреплённого source reference.

ABI/CLI проверки покрывают payloads, unknown properties/identities, reset,
huge-header resource guard, independent root, late directory re-entry и
отсутствие склейки имен через gap. EN/RU UI проверяет принятие/отказ, копирование
исправного файла, badge и новое согласие после закрытия. Семь focused suites
прошли на macOS за 10,71 с; после исправления fragment type guard повторные
три suites прошли за 8,73 с. На Linux/Windows семь suites прошли за 8,27/45,42 с,
receipt `20261021T154735Z-481c0cd0`. Windows выявил чтение нерелевантного,
неинициализированного fragment field у symlink: проверка fragment availability
теперь ограничена двумя regular-file types, как соответствующие Linux branches.
Здоровый symlink не получает ложную ошибку fragment; это проверяется через ABI.
Отказы декомпрессии всей inode metadata и
initial index bootstrap остаются незакрытыми; полнота SquashFS не заявляется.

## Ошибки отдельных directory metadata blocks, 2026-10-21

Повторно сверены SDK `ReadMetadataBlock`/`Decompress`/`ReadData`, все decoder
output paths и потребители directory buffers с Linux `cache.c:squashfs_read_metadata`,
`block.c:squashfs_read_data`, `dir.c`/`namei.c` и directory inode branches.
Локальная ошибка блока каталога не должна закрывать другие независимо
адресуемые каталоги: Linux читает их по inode references, а прежний archive
adapter требовал успешной предварительной декомпрессии всей directory table.

GPL v4 reader публикует только полностью декодированные metadata blocks.
Ошибка checksum после выдачи декодером части данных не публикует эти данные.
Разрывы сохраняются в mapping, и parser ограничивается непрерывным диапазоном:
байты исправного следующего каталога не становятся продолжением повреждённого.
Продолжение чтения использует только проверенную packed boundary либо anchor
из уже разобранного directory inode; недостоверная длина не используется для
угадывания следующего блока. Расчёт непрерывных диапазонов выполняется один
раз, без повторного сканирования всей таблицы для каждого каталога.

Сохранены оригинальные bounded SDK decoders, COM streams и item parsing.
`ReadMetadataBlock` использует уже существующий direct-buffer output path
decoder для отдельного блока; никаких новых codec implementations нет.
Core-only/legacy scope сохранён. Initial inode table и обязательные index
tables по-прежнему требуют отдельного продолжения аудита. Новая адаптация
не ослабляет source/table bounds, names/path validation, decoder availability
или fatal abort/OOM/callback contract. Используются прежние consent, badge,
directory diagnostics и DataError для неполного результата; source write/replay нет.

Новые deterministic images содержат root, отдельный повреждённый каталог и
исправный Unicode sibling после него. Проверяются bad header, checksum с
частичным decoder output, read I/O failure, directory span через разрыв и
неповреждённый control. Наблюдаемые результаты: исправные payloads доступны,
повреждённые имена не выдумываются/не склеиваются, reset/reopen работает,
abort/OOM/unsupported decoder и non-S_OK callbacks остаются фатальными.
EN/RU UI проверяет реальное ограниченное открытие и копирование из такого образа.

Четыре readonly native Linux `6.8.0-147-generic` controls подтвердили доступность
root/исправного sibling при недоступном соседнем каталоге, в том числе после
ошибки compressed metadata. Receipt: `20261021T150825Z-851450f1`.
Байты payloads и неизменность образов проверены, mounts/data удалены;
runtime kernel старше закреплённого source reference.
Шесть focused suites (`squashfs`, `squashfs_stream`, `squashfs_metadata`,
`filesystem_squashfs_directory_ui`, `upstream_baseline`, `upstream_inventory`)
прошли на macOS и Linux; Windows прошёл те же проверки после исправления
native path separator в новом ABI-тесте вложенного пути. Основной receipt:
`20261021T151051Z-f7cea294`, повтор только затронутого ABI suite на трёх ОС:
`20261021T151239Z-e3934310` (remote). Прочие прошедшие suites не повторялись
после test-only correction. На macOS итоговая шестёрка — 8,17 с.
Временный native helper удалён и удаление синхронизировано на гостях.
Приложение инкрементально собрано на macOS.
Полные/slow suites и физические носители не использовались; incremental
build trees и compiler caches сохранены.

## Частичные каталоги и недоступные inode, 2026-10-21

Сравнены SDK `OpenDir` и потребители item/node mapping, Linux
`dir.c:squashfs_readdir`, `namei.c:squashfs_lookup`,
`inode.c:squashfs_iget` и полный hardlink consumer
`ArchiveExtractCallback.cpp:Archive_Get_HardLinkNode`. Ревизии ниже.
Основной directory parser сохранён, разбор direct entries выделен в
`ReadDirItems` в том же handler; платформа и source language не менялись.

GPL-модуль сохраняет проверенный префикс каталога и продолжает обход исправных
записей при локальной ошибке. Достоверное имя с недоступным inode остаётся
видимым: size, times, IDs и прочие неизвестные свойства не выдумываются,
GetStream возвращает ошибку, извлечение сообщает DataError и продолжает
работу со следующими файлами. Неполные каталоги получают собственную ошибку;
неполный root представлен существующим collision-safe `[ROOT]` container.
Неполнота требует согласия на новое открытие и остаётся видна в badge.
Имена с NUL, slash, dot/dotdot и неверными границами не публикуются.
Проверки recursion/cycles, item limits, root reference и decoder сохраняются.
Отмена/OOM и любой не-S_OK callback result, включая S_FALSE, прерывают открытие,
а не превращаются в partial success.

Дополнительно выяснено, что Linux может открыть запись с повреждённой inode
reference после загрузки исправной hardlink: `iget` использует inode cache.
Archive reader может использовать уже разобранный inode с однозначным номером
и совпадающим типом. Это отдельная явно непроверенная адаптация, не утверждение
достоверности повреждённой связи имени и файла: предупреждение допускает
неверное сопоставление/устаревшие данные, копирование сообщает DataError даже
при полном наборе байтов. Признак распространяется на потомков и xattrs.
При отсутствии или неоднозначности номера такая подстановка не производится.
Непроверенные/неоднозначные `INode` и `StreamId` не экспортируются как verified
identity: upstream hardlink consumer считает inode достаточным для объединения
файлов даже при неопределённом StreamId. Регрессия с двумя разными payloads
проверяет их отдельное извлечение и отсутствие ложного объединения.

Native Linux `6.8.0-147-generic`, receipt `20261021T145043Z-24ca3f1e`:
три synthetic readonly controls подтвердили directory prefix, холодный EIO
повреждённой reference, успешное чтение после priming правильной hardlink и
устойчивый EIO при неизвестном inode number. Исправная hardlink во всех случаях
совпала побайтно. Образы не изменились; mounts/data удалены. Runtime kernel
старше закреплённого source reference; его сборка здесь не проверялась.

Публичные ABI/CLI проверки покрывают partial directory tail, cycle, invalid
name/count, missing/unique/ambiguous inode mapping, неизвестные свойства,
последующее исправное извлечение, Close/reopen и ошибки callback. EN/RU UI
проверяет принятие/отказ, root warning, честный результат copying, нормальную
и компактную геометрию и новый запрос после закрытия. На macOS восемь focused
suites прошли за 24,30 с. Directory metadata blocks рассмотрены выше;
отказы начальной загрузки inode metadata и index bootstrap ещё не закрыты.
Эта итерация не доказывает
полную устойчивость SquashFS к повреждениям.
Те же восемь suites прошли на Linux и Windows, receipt
`20261021T145404Z-57c0a1b5`. После форматирования изменённых C++ ranges
локальные CLI/ABI suites повторены: PASS за 5,33 с; приложение собрано
инкрементально. Cocoa EN/RU JSON проверяет предупреждение о непроверенном
inode mapping; guests используют Qt offscreen. Pixel parity не заявляется.
Native helper удалён и удаление синхронизировано на гостях. Полные/slow suites,
установка и физические источники не использовались, build trees/cache сохранены.

## Недоступные UID/GID и fragments: ограниченное чтение, 2026-10-21

Сопоставлены Linux `id.c:squashfs_get_id`, `fragment.c:squashfs_frag_lookup`
и index loaders, `inode.c:squashfs_new_inode`/regular inode branches,
`super.c` table setup с полными SDK `Open2`, `GetPackSize`, `ReadBlock`,
`GetStream`, `Extract`, `CCachedInStream` и `CCopyCoder`. Ревизии и исходные
репозитории закреплены ниже. Linux VFS требует ownership и fragment lookup
при создании inode; эти проверки не означают, что все байты его основных
блоков технически недоступны archive reader.

GPL handler сохраняет доступные ID и fragment entries, помечая недоступные
вместо отказа всему открытию. Неизвестный UID/GID, включая ссылку вне ID table,
не выдаётся за нулевой или проверенный ID. При неизвестном fragment mapping
размер файла остаётся исходным, packed size неизвестен; полные проверенные
блоки читаются, обращение к хвосту возвращает ошибку. Извлечение сохраняет
полученное начало, сообщает DataError и продолжает обработку исправных файлов.
Returned streams сохраняют этот результат после Close/reopen владельца.

Это явная адаптация ограниченного archive access по Read-only filesystem
access, не перенесённый VFS recovery mode. Согласие запрашивается для каждого
нового открытия; badge и сообщения у владельцев/hardlinks сохраняют
неполноту видимой. Cancel/OOM и ошибки progress callback остаются фатальными.
Unsupported decoder, index bootstrap/order/range validation и проверки
основных data/inode/directory/path boundaries сохранены. Тела Linux C readers
не менялись. Core-only standalone engine и старые версии SquashFS сохраняют
прежний scope. Записи, repair и replay отсутствуют.

Составные per-item сообщения переводятся построчно общей Qt-службой; неизвестный
текст и переводы строк сохраняются. EN/RU проверки включают ownership,
fragment и combined предупреждения, принятие/отказ, повторное открытие,
обычную/компактную геометрию, копирование доступных байтов и диагностику
неполного результата. Публичные ABI/CLI controls покрывают недоступный ID block,
неверные UID/GID references, fragment block/reference, сохранение соседнего
файла, исходного размера, отдельных ID и отсутствие ложного packed size,
одну операцию с неполным и исправным файлом, сохранённый stream и сброс состояния.

Шесть native Linux `6.8.0-147-generic` controls подтвердили границу VFS:
неверные UID/GID references возвращают EINVAL, fragment block/reference — EIO
для затронутых inode; исправный сосед читается точно. При повреждённом ID block
root mount отвергается. Поэтому доступ приложения к известным блокам таких
файлов отдельно проверен archive ABI, а не заявлен как native VFS поведение.
Receipt: `20261021T143244Z-ce04eec0`. Образы неизменны, temporary mounts/data
удалены; runtime kernel старше закреплённого source reference.

На macOS девять focused suites прошли за 17,74 с: SquashFS CLI/ABI/C metadata,
общий read-only UI, FAT partial-copy UI consumer, language, archive errors и
оба upstream checks. Те же девять suites прошли на Linux и Windows, receipt
`20261021T143424Z-2e85a00d`. Приложение собрано инкрементально на macOS;
Cocoa EN/RU JSON подтверждает видимость и вместимость combined предупреждения.
Guests используют Qt offscreen; pixel parity не заявляется. Native helper
удалён и удаление синхронизировано на обеих ОС. Полные/slow suites, установка
и физические носители не использовались; build trees и caches сохранены.
Остальные initial-table и eager inode/directory failures ещё не закрыты.

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
traversal ещё требуют отдельного продолжения аудита. Export lookup
рассмотрен отдельно ниже.

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
boundary. Границы export index проверяются при setup; его payload не является
зависимостью обычного чтения файлов. Xattrs включают binary/empty values,
shared out-of-line references и переходы metadata block. C API позволяет
получать names/values; те же функции используются при проверке inode metadata.
User/trusted/security namespaces читаются как байты архива, без привязки к
привилегиям процесса на host OS. Неизвестные типы пропускаются как в Linux.
Публичные xattrs доступны как alternate streams через стандартный archive ABI,
как описано ниже. Установка OS xattrs остаётся отдельной write operation.

### Независимость от NFS export payload, 2026-10-21

Сверены полные `export.c` и его VFS callers, `namei.c:squashfs_lookup`,
`inode.c:squashfs_iget`, setup в `super.c`, исходный SDK `Open2`/`OpenDir`
и текущие C metadata services. Закреплённые ревизии указаны выше.
Linux использует export inode lookup при работе с NFS filehandles;
обычный поиск получает inode location из directory entries. Наша добавленная
в предыдущем переносе сверка каждого inode с export payload создавала
дополнительный отказ, которого нет в Linux для обычного чтения.

Этот gate удалён из archive opening. Linux index loader и его size/order/range
checks сохранены: первая index location всё ещё участвует в вычислении границ
таблиц. Проверки inode/directory references, payload bounds и path validation
не менялись. Ошибки действительно запрошенных source reads и callbacks
сохраняют прежний контракт. NFS export operations приложение не предоставляет;
неиспользуемый payload не объявляется проверенным и не требует отдельного
режима ограниченного чтения. Native readonly сценарий подтверждает обычную
доступность основных файлов, а не работоспособность NFS filehandles.

Восемь native Linux `6.8.0-147-generic` controls: три повреждения export
payload (неправильная reference, нулевой metadata block, повреждённый
compressed block) при наличии/отсутствии xattrs читаются побайтно,
включая Unicode и hardlink. Два controls с index за пределами образа
отвергаются при mount. Все образы неизменны, temporary mounts/data удалены.
Receipt: `20261021T142144Z-edbb37b7`; runtime kernel старше source reference.

CLI-регрессии проверяют оба поставляемых engines, листинг и извлечение
основных файлов; ABI-регрессия оставляет export metadata недоступной через
IInStream и проверяет основные/alternate streams. Контроль некорректного
index сохраняет отказ. На macOS шесть focused suites (`squashfs`,
`squashfs_stream`, `squashfs_metadata`, `filesystem_squashfs_read_ui`,
`upstream_baseline`, `upstream_inventory`) прошли за 3,71 с.
Те же шесть suites прошли на Linux и Windows, receipt
`20261021T142259Z-f410be08`. Временный native helper удалён и удаление
синхронизировано на обоих гостях; образы/mounts удалены после oracle.
Полные/slow suites и физические носители не использовались, incremental
build trees и compiler caches сохранены. Начальная загрузка прочих metadata
и eager inode/directory failures остаются в открытом scope общего аудита.

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
