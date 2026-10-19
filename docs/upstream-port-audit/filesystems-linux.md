# Проверка чтения файловых систем по Linux

Запрос от 2026-10-19: проверить существующие файловые системы, представленные
в основном дереве Linux, исправить расхождения чтения и добавить read-only EROFS.
Каждая завершённая итерация оформляется отдельным коммитом.

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
GPT/MBR/APM, LVM и контейнеры дисков не являются файловыми системами.
Наличие обработчика в engine не считается доказательством проверки по Linux.

Для каждой строки сначала сравниваются полные тела, заголовки, callers и
значимые ветви выбранного read scenario. Все найденные source-visible пробелы
этого scenario исправляются вместе либо получают конкретное обоснование
ограничения. Затем выполняются повторное сравнение и focused runtime-проверки
через публичные engine/ArchiveBackend интерфейсы на локальных fixtures.
Новые regressions должны выявлять конкретное неверное чтение или обработку
ошибки; проверки текста исходников не применяются.

## Свидетельства

Начальная итерация: определены фактические обработчики поставляемого
`Bundles/Format7zF/Arc_gcc.mak` и локальные модули, закреплён Linux baseline,
проверено место сборки engine. Функциональная проверка перечисленных
обработчиков и EROFS ещё не завершена. Эта запись задаёт scope проверки,
а не заявляет совместимость или полную передачу Linux drivers.
