# Проверка и актуализация форматов по Linux

Запрос от 2026-10-19: проверить существующие файловые системы, представленные
в основном дереве Linux, исправить расхождения чтения и добавить read-only EROFS.
Каждая завершённая итерация оформляется отдельным коммитом.

Уточнение пользователя от 2026-10-19 расширяет задачу: включить также другие
форматы проекта, для которых в ядре Linux есть реализация. Scope больше не
ограничивается файловыми системами. Для дополнительных форматов требуется
подтвердить фактический parser/decoder в обоих деревьях; общие названия или
наличие соответствующей userspace-программы не доказывают поддержку в ядре.

Дальнейшее уточнение пользователя: отсутствующие возможности формата, которые
есть в эталонной версии ядра, требуется реализовать. Перечень различий — scope
актуализации, а не основание считать модуль завершённым с ограничениями.
Для файловых систем это включает отсутствующие read features; kernel-only
VFS/platform services и запись не подменяют работу над чтением формата.

## Эталон и границы

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da` от 2026-10-17.
Это также воспроизводимый baseline существующего [exFAT-переноса](exfat-linux.md).
Зеркало для получения отдельных файлов:
<https://github.com/torvalds/linux/tree/58161ead29e50d70a317ea26b472e0c7508e69da>.

7-Zip: <https://github.com/ip7z/7zip.git>,
commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Обработчики встроены в поставляемый динамический engine и standalone engine;
их исходники берутся из read-only sibling checkout при подготовке runtime.
Локальные исправления должны попадать в оба engine, сохранять исходную структуру
модулей и учитываться в provenance/cache runtime. Изменение sibling запрещено.

Linux определяет корректность интерпретации дисковых структур и чтения данных.
Его VFS, запись, журналирование, mount, восстановление повреждённых томов,
управление устройствами и производительность page cache не входят в задачу.
Отказ от неподдерживаемой возможности должен быть явным; успешное извлечение
не должно возвращать неверные данные. Отличия поддержки возможностей
фиксируются отдельно от ошибок в заявленном read scope.

## Последовательность интегрированных итераций

| Область engine | Эталон Linux | Работа |
| --- | --- | --- |
| exFAT/TexFAT | `fs/exfat` | Повторное сравнение C-переноса, metadata/chain/stream границ и ошибок; исправления и существующие ABI/backend consumers. |
| FAT12/16/32 | `fs/fat` | Boot geometry, FAT и длинные имена, цепочки и извлечение. |
| ext2/3/4 | `fs/ext2`, `fs/ext4` | Superblock/features, inode, directory, extents/indirect/inline data и sparse reads. |
| NTFS | `fs/ntfs3` | Boot/MFT, attributes/runlists, directories, sparse/compressed file reads. |
| HFS/HFS+ | `fs/hfs`, `fs/hfsplus` | Volume/B-tree, extents, имена, data/resource forks. |
| cramfs, SquashFS | `fs/cramfs`, `fs/squashfs` | Metadata, directories, blocks/fragments, decompressor boundaries. |
| ISO9660, UDF | `fs/isofs`, `fs/udf` | Descriptors, names/extensions, extents, sparse и embedded data. |
| EROFS (новый модуль) | `fs/erofs` | Сначала полный source comparison read path и зависимостей, затем C-перенос, 7-Zip ABI, deployment и end-to-end чтение. |

APFS не входит в scope: соответствующего драйвера нет в основном дереве Linux.
GPT/MBR/APM проверяются как дополнительные форматы разметки дисков.
Наличие обработчика в engine не считается доказательством проверки по Linux.

Дополнительный scope уточняется по полному списку `7z i` поставляемого engine:
разметка GPT/MBR/APM, CPIO/initramfs, gzip/bzip2/LZMA/XZ/Zstd, загрузочные
образы ELF/PE/COFF и остальные фактически совпадающие parsers/decoders.
Проверка дополнительных форматов ещё не завершена; ограничения отдельных
подформатов Linux и 7-Zip должны быть указаны при source comparison.

Для каждой строки сначала сравниваются полные тела, заголовки, callers и
значимые ветви выбранного read scenario. Все найденные source-visible пробелы
этого scenario исправляются вместе либо получают конкретное обоснование
ограничения. Затем выполняются повторное сравнение и focused runtime-проверки
через публичные engine/ArchiveBackend интерфейсы на локальных fixtures.
Новые regressions должны выявлять конкретное неверное чтение или обработку
ошибки; проверки текста исходников не применяются.

## Свидетельства

### ISO9660: logical blocks, XAR и extent streams

[Source comparison и исправления](isofs-linux.md): оба engines читают
512/1024/2048-byte geometry, XAR и multi-section children; mapping ограничен
source/volume, streams независимы, повреждения и отмена передаются явно.
Отдельный модуль подключает Linux C Rock Ridge/SUSP: CE/NM/SL/PX/PN/TF,
relocation, inode metadata, RR-vs-Joliet выбор, Linux zisofs decompression,
High Sierra и Linux recording dates.
Joliet/RR fallback и normal/Acorn/raw mapping подключены к Linux C readers;
глубокий обход, очистка дерева и выбор вложенного файла проверены вместе.
Собственные inode каталогов определяют metadata и Level 3 секции, включая root;
родительские имена сохраняются независимо от inode metadata.
Все Linux NLS charset modules подключены к Joliet, включая aliases и
многобайтовые преобразования. Read-path options, legacy-size `cruft`, выбор
descriptor sector и необязательные descriptor dates актуализированы.
RR producer layouts и оставшиеся read-side consumers сверены с pinned source;
исправлены directory-name CE context и malformed TF fractions. Runtime
проверки и отличия archive projection перечислены в ISO audit.
UDF ещё не проверен; остальные строки общей задачи не закрываются этим результатом.

### HFS+/HFSX: fork mapping и metadata reads

[Source comparison и исправления](hfs-linux.md): overflow continuation между
leaves, preallocation/EOF, границы metadata и forks, boot blocks/wrappers,
циклы каталога, независимые streams и ошибки I/O. Добавлены Linux C Unicode
conversion/casefold и hardlink lookup в отдельном GPL-модуле; backing forks,
xattrs и inode metadata проходят ABI/backend. Подключены Linux permission defaults,
проверки catalog/thread records и catalog FinderInfo streams, включая hardlinks.
Linux inode projection также передаёт timestamp rollover и special-device metadata.
Добавлен Linux C reader plain classic HFS: MDB/B-tree, data/resource и catalog
overflow forks, имена, lock/default metadata и type/creator streams.
Дальнейший metadata/read audit остаётся в scope актуализации.

### NTFS: classic data reads и LZNT1

[Сравнение и интегрированная актуализация](ntfs-linux.md): boot/MFT и
атрибуты, sparse/initialized data, физические run границы, независимые streams,
nonresident reparse payload и Linux C LZNT1 в отдельном GPL-модуле приложения.
Динамический модуль также читает WOF LZX/XPRESS, 32/64-битные ADS offsets и
bootstrap MFT attribute lists через оригинальные Linux C readers.
Оставшийся NTFS metadata/read audit и остальные форматы сохраняются в scope.

### SquashFS: metadata, fragments и независимые streams

[Сравнение и исправление существующего чтения](squashfs-linux.md) охватывает
оба engines: корректные границы metadata/data, пустые тома без fragments,
независимость потоков и передачу ошибок. Актуализация также
подключает Linux C LZ4/LZO, compressor options, index/xattr metadata и
inode/link properties. Source scope и границы standalone/archive ABI описаны
в linked audit; runtime evidence не заменяет проверку остальных строк.

### ext2/3/4: sparse, metadata и inline data

[Сравненный scope и исправления](ext-linux.md): classic sparse indirect paths,
ограничения extent/каталогов, fast symlink с EA и передача ошибок потоков в обоих
engines. Динамический движок также подключает Linux C-проверки metadata,
полные inline data/каталоги, META_BG, BIGALLOC, современные descriptors и
fs-verity data verification. Полная актуализация ext4 не завершена: fscrypt/key
service, trust policy и оставшиеся
feature/creator-OS read semantics остаются в общем scope.

### FAT12/16/32: BPB, имена и размер файла

[Сравненные paths и интегрированные изменения](fat-linux.md): сняты лишние
ограничения BPB, восстановление LFN и чтение выделенного хвоста приведены
к проверенному Linux поведению; сохранены проверки читаемой части цепочки.
Оба поставляемых engine используют один адаптированный исходник. Следующая
интеграция подключает Linux C-функции DOS 1.x, NLS и дат в отдельный FAT-модуль
приложения; source scope, проверки и границы standalone описаны в linked audit.

### EROFS: read-only модуль

[Переданный C scope, интеграция и границы проверок](erofs-linux.md).
Подключены mapping/metadata paths, четыре декодера и format-модуль. Проверены
образы официального mkfs, ABI/backend, повреждения, отмена, время жизни потоков
и дополнительные устройства на трёх ОС. Эта итерация не закрывает остальные
файловые системы и дополнительные форматы из общего scope.

### cramfs: sparse end-pointer blocks

[Сравнение всего модуля, исправление и оставшаяся актуализация](cramfs-linux.md).
Нулевые extents читаются обоими поставляемыми engines. Полный модуль ещё не
был завершён в `b3c78fd`. Следующая интегрированная актуализация переносит
Linux C read path в отдельный format-модуль и закрывает расширенные указатели,
prefixed superblock и metadata branches; её фактический scope и проверенные
платформы записаны в linked audit. Это не закрывает остальные строки таблицы.

### Начальный scope

Начальная итерация: определены фактические обработчики поставляемого
`Bundles/Format7zF/Arc_gcc.mak` и локальные модули, закреплён Linux baseline,
проверено место сборки engine. Функциональная проверка перечисленных
обработчиков и EROFS ещё не завершена. Эта запись задаёт scope проверки,
а не заявляет совместимость или полную передачу Linux drivers.

### exFAT: отмена cached FAT traversal

[Сравненный scope и regression](exfat-linux.md#cached-fat-cancellation-2026-10-19).
Пропущенная проверка отмены — адаптация archive callback, а не изменение
дискового алгоритма Linux. Существующие границы TexFAT и omitted scope сохранены.
Проверка этого scenario не означает завершения всей таблицы выше.

Проверки 2026-10-19: инкрементальная сборка модуля и consumers;
`exfat`/`exfat_backend` PASS на macOS arm64/Qt 6.12 (2,07 с), Linux x64
и Windows x64/MSVC/Qt 6.12. Remote receipt:
`20261019T054117Z-8147988f`. Проверены экспортируемый ABI, реальный process
engine и ArchiveBackend на generated images, включая прежние success/error/
TexFAT/Unicode сценарии. Общие/slow suites, физические носители и UI в эту
проверку не входят. Инкрементальные builds и compiler caches сохранены;
одноразовых task fixtures после успешных запусков не осталось. Посторонние
старые Windows runner-tmp fixtures не удалялись: принадлежность не установлена.
