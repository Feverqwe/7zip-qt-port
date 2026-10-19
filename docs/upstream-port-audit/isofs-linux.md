# ISO9660: descriptors, logical blocks и extent streams

Работа входит в [общую актуализацию по Linux](filesystems-linux.md).
Основной ISO9660 read path исправлен в обоих поставляемых engines.
Отдельный GPL-модуль добавляет оригинальные Linux C Rock Ridge/SUSP readers.
Он также читает High Sierra и zisofs.
Полная актуализация ISO9660 и отдельного UDF ещё не завершена.

## Источники и source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сравнены полные `CPP/7zip/Archive/Iso/IsoIn.cpp`/`.h`, `IsoItem.h`,
`IsoHandler.cpp`, `IsoHandler.h`, `IsoHeader.cpp`/`.h`, `IsoRegister.cpp`,
используемые stream helpers, ABI и регистрация в console/module/standalone.
Изменённые файлы находятся под исходными путями в `src/`; `StdAfx.h`,
`IsoHeader.cpp`/`.h` переиспользуются без изменений в
`third_party/7zip`. [7-Zip baseline](../upstream-baseline.json) фиксирует их
происхождение и runtime overlays. `IsoRegister.cpp` адаптирован: dynamic engine
передаёт регистрацию внешнему модулю, standalone сохраняет исходный handler.
C++ форматирование учитывается отдельно от функциональных изменений.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
[Manifest](../isofs-linux-baseline.json) фиксирует reference snapshot `fs/isofs`
и `include/uapi/linux/iso_fs.h`. Наличие файла в manifest не означает передачу
или проверку всех его ветвей. Для выбранного сценария сравнены `inode.c`:
`isofs_fill_super`, `isofs_get_blocks`, `isofs_read_level3_size`,
`isofs_read_inode`; полные `dir.c`, `isofs.h`, `util.c`, определения дисковых
структур и directory validation/lookup в `namei.c`. Полный `compress.c`,
`zisofs.h`, ZF branch и generic `lib/zlib_inflate` сравнены для zisofs scope.
В первой core-итерации Linux использовался как reference без копирования GPL
bodies в engine. Следующая интеграция переносит C scope в отдельный модуль,
описанный ниже. Новых независимых parsers/decoders не добавлено.

## Переданный read scenario

Сохранены исходные классы, directory records, catalog/ref представление,
свойства, boot parsing, callbacks и CopyCoder. Исправлены их следующие ветви:

- Logical blocks 512/1024/2048 используются для directory/file extents,
  границ записей и padding. Размер XAR прибавляется к extent; счётчики остаются
  64-битными. Descriptor и El Torito sector addresses по-прежнему имеют шаг 2048.
- Неизвестные/foreign volume descriptors пропускаются целиком. Поиск ограничен
  секторами 16–99, как в Linux. LE-часть paired чисел используется даже при
  ошибочной BE-копии, как `isonum_723`/`isonum_733`.
- Исходная группировка multi-section records используется и для чтения
  дочерних каталогов, и для archive refs. Дети всех указанных секций доступны
  через первый ref; отсутствие терминальной записи и невыровненные non-final
  sections не превращаются в успешное частичное извлечение.
- Effective data ranges проверяются по declared volume и source size до выдачи
  потока. Это дополнительная archive boundary: Linux может прочитать данные
  за объявленным размером тома на более крупном backing device. Отказ в таком
  случае не заявляется как точное воспроизведение Linux malformed-volume policy.
- Все file/boot streams используют оригинальный `CExtentsStream`, уже
  адаптированный для независимых курсоров и владения source. `Extract` использует
  тот же mapper. Early EOF даёт DataError; I/O failures и отмена сохраняют HRESULT.
  Streams остаются читаемыми после Close, повторного Open и release handler.
- Interleave gap явно возвращает UnsupportedMethod, вместо неверных сырых байтов.
  Linux также не реализует interleaving; `file_unit_size` без gap принимается.
- Malformed directories и циклы явно отклоняются, вместо успешной публикации
  неполного дерева. Искусственный depth cap снят вместе с рекурсией обхода,
  построения refs и освобождения дерева. Failed/canceled Open
  очищает опубликованный каталог. Частичный последний metadata buffer допустим;
  physical size больше не округляет 512-byte хвост до 2048.

Изменение `IsoIn.h`/`IsoItem.h` меняет layout handler. Native make/nmake rules
7-Zip не отслеживали заголовки: мог сохраниться `IsoRegister` с прежним размером
allocation. Runtime snapshot теперь добавляет транзитивные prerequisites
потребителей адаптированных headers через wrapper makefiles; оригинальные
правила/цели/compiler flags сохраняются. Это входит в интеграцию компонента,
а не отдельный refactor build system. Incremental outputs и ccache сохраняются.
Compiler/flags identity существующих объектов остаётся отдельным runtime debt.

## Rock Ridge/SUSP: Linux C read module

Перенесены `fs/isofs/rock.c`: `check_sp`, `setup_rock_ridge`,
`rock_continue`, `rock_check_overflow`, `get_rock_ridge_filename`,
`parse_rock_ridge_inode_internal`/wrapper, `get_symlink_chunk` и parse loop
`rock_ridge_symlink_read_folio`. Сохранены CE bounds/loop limit, NM accumulation,
SL records/component continuation, PX/PN, short/long TF, ER/SP/XA probing,
CL relocation checks и RE hiding. `rock.h` переиспользован без изменений.
`util.c:iso_date` и `kernel/time/time.c:mktime64` передают Linux timestamp
semantics, включая timezone sanity и long-form fractions. Отрицательный left
shift при sign extension заменён эквивалентным определённым C выражением.
Из `inode.c` перенесены `rootdir_empty` и root-choice часть `isofs_fill_super`;
VFS inode acquisition заменён результатом существующего bounded reader.

7-Zip уже ограничивает и выравнивает System Use area; C boundary получает её
как bounded span. Folio/page-cache acquisition и inode publication заменены
owned buffers; SUSP/XA skip не образует pointer за пределами этого span.
Read services используют stream callbacks. Inode fields остаются read projection;
`isofs_read_inode` defaults/dot-record projection используются для directory inode,
дисковый `iso_directory_record` взят из исходного UAPI header;
VFS operation tables и mutable services не нужны. `CL` проверяет inode identity
по месту directory record, отдельно от data extent, и рекурсивную relocation.
`PL` не участвует в archive hierarchy: native NFS parent/export service не
переносится. Общие read branches и их тела сохранены; wrapper differences
описаны отдельно от скопированных алгоритмов.

`IsoLinux.cpp` — C++ stream/record adapter. Он сохраняет HRESULT/отмену
и восстанавливает физическую позицию metadata buffer после CE/CL callbacks.
Handler публикует UTF-8 RR names, symlink target/stream, UID/GID/links/mode,
device major/minor и access/change/modify times. TF_CREATE трактуется как
change time с приоритетом TF_ATTRIBUTES, как в Linux; birth time не выдумывается.
RR punctuation не проходит ISO version/dot stripping. Служебные `.`/`..` и
скрытые RE records не валидируются как публикуемые children. Relocated children
читаются в исходной hierarchy; физические RE entries скрыты.
Повторные CE reads metadata/name/symlink readers не расходуют исходный budget
directory traversal: общий CE может использоваться тысячами records. Отмена
проверяется при этих reads; CE bounds/loop limit остаются в Linux reader.

При наличии RR выбирается primary descriptor; пустой или повреждённый primary
root допускает переход к корректному Joliet, как `isofs_fill_super`/`rootdir_empty`.
Без ER marker Linux продолжает RR probing, если Joliet не выбран; этот branch
также сохранён. Выбранный primary root должен иметь directory inode mode.
Zisofs ZF metadata распознаётся оригинальной ветвью и подключает описанный ниже
decoded stream. UnsupportedMethod возвращается при попытке читать special
device/FIFO/socket как обычный файл; metadata доступна без обращения к устройствам.

Linux C компилируется как C11 только в `sevenzip_iso`, без unRAR. LGPL closure:
ISO handler/header, ArchiveExports, Common/Windows/stream helpers и CopyCoder.
Модуль устанавливается вместе с GPL/LGPL notices и Linux-syscall-note для
выбранного UAPI type. Standalone `7zz` сохраняет
core fixes и исходный ограниченный RR parser; новые Linux read features
предоставляет dynamic engine приложения, как у остальных GPL format-модулей.

## Zisofs: Linux C decoder

Из того же Linux commit перенесены полные read bodies `compress.c`:
`zisofs_uncompress_block`, `zisofs_fill_pages`, `zisofs_read_folio`.
Buffer-head reads используют bounded 7-Zip extent stream, включая Level 3
multi-section mapping; folio acquisition заменён stream-owned cache до 128 KiB.
Workspace и sink принадлежат потоку вместо kernel-global mutex/workspace.
VFS operation table, module init/exit и shared page cache не переносятся.
`IsoZisofs.cpp` содержит только ABI/ownership/error adapter; Seek использует
исходное тело `CExtentsStream::Seek` с decoded EOF.

Generic Linux zlib inflate компилируется как C: `inflate.c`, `inftrees.c`,
`inffast.c`, их headers и `zlib.h`/`zconf.h`. Первые два тела и headers сохранены
побайтно; в `inffast.c` pointer alignment cast заменён на `uintptr_t` для LLP64.
`zutil.h` заменяет kernel includes и выбирает portable unaligned helper.
Архитектурный s390 DFLTCC, unused blob helper и deflate encoder не нужны.
Manifest фиксирует исходные SHA-256 и разделяет exact/adapted files.
Сохранены notices H. Peter Anvin, Mark Adler и Jean-loup Gailly;
из `zlib.h` выделен устанавливаемый `zlib-LICENSE.txt`.
Все эти C sources подключены только к отдельному GPL `sevenzip_iso` без unRAR.

ZF size/header words/block shift управляют чтением, как в Linux; embedded
zisofs header не становится дополнительным обязательным magic contract.
Sparse blocks, sub-page compression, pointer-table block crossings,
512-byte logical blocks и максимальный shift 17 поддерживаются.
Archive boundary проверяет полную таблицу и непустые packed ranges до чтения;
arithmetic остаётся широкой до проверок. Один private guard page позволяет
проверить trailer даже при точном заполнении последней страницы.
Перед публикацией блока требуется zlib stream end, корректный Adler и точный
decoded size; при ошибке весь cache window инвалидируется. Это намеренно
строже kernel policy, допускающей готовую critical page после поздней ошибки.
Sparse sub-page branch обнуляет только текущий compression block: исходный
Linux branch обнулял оставшуюся страницу вместе с соседними блоками.
Equal sparse pointers не требуют чтения backing bytes.
Packed reads ограничиваются raw extent size, а не decoded size: expanding
zlib blocks могут занимать больше исходных данных. Native 6.8 `isofs_get_blocks`
ошибочно ограничивает такой read по decoded EOF и возвращает EIO.

Handler публикует decoded Size и `zisofs` Method, сохраняет packed Size;
Extract totals и CopyCoder используют decoded size. I/O HRESULT, отмена,
independent cursors и retained-source lifetime сохраняются. Standalone `7zz`
не получает этот GPL decoder; приложение использует dynamic format module.

## High Sierra и recording dates

Из того же pinned `inode.c:isofs_fill_super` передана High Sierra ветвь
идентификации primary descriptor и выбора geometry/root. Она остаётся C
в `isofs_high_sierra`; buffer acquisition и VFS superblock publication
заменены read projection. Из UAPI `iso_fs.h` скопированы исходные
`hs_volume_descriptor`/`hs_primary_descriptor`; kernel-only includes опущены,
форматирование учитывается отдельно. LE часть paired scalars используется
как `isonum_723`/`isonum_733`; существующий LE16 helper носит имя `isonum_721`.
System/volume identifiers и volume set/sequence передаются в существующие
archive properties. Отсутствующие в Linux HS descriptor поля не выдумываются.

Исходный 7-Zip record/extent reader сохранён: флаги берутся из байта перед
ISO flags, следующий reserved byte пропускается. Дата имеет шесть байтов,
timezone равен нулю. XAR, multi-section, logical blocks и lifetime/error
контракты используют тот же mapper. RR/SUSP и El Torito не применяются к HS.
ISO descriptor проверяется первым при перекрывающихся magic, как в Linux.
Вместо единственного fixed signature offset module использует штатный IsArc
callback, распознающий обе descriptor signatures. Имя/CLSID Iso сохранены;
проверяются секторы 16–99, включая primary после чужого descriptor.
Автоопределение проверяется также без расширения файла. Standalone `7zz`
сохраняет исходную регистрацию и не получает GPL HS branch.

Обычные короткие recording dates тоже вызывают исходный Linux `iso_date`:
timezone ±52 quarter-hours применяется, ±53 игнорируется; переполненный день
нормализуется `mktime64`. HS flags и reserved byte не становятся timezone.
Дата возвращается через существующий FILETIME property; decoder остаётся C.
RR TF и long-form date path продолжают использовать тот же Linux helper.
Archive descriptor dates остаются отдельным исходным 7-Zip представлением.

## Joliet, RR fallback и Acorn names

Из того же Linux commit переданы полные `dir.c:isofs_name_translate`,
`get_acorn_filename`, `joliet.c:uni16_to_x8` и `get_joliet_filename`.
Directory iteration, VFS lookup/casefold operations и module registration
не нужны для archive naming. C остаётся C; дисковые spans реконструируются
из уже проверенного directory record, с bounded zero lookahead.
Kernel 16-bit `wchar_t` использует существующий `fat_wchar_t`; перед shared
UTF-16 helper делается aligned copy. Один cast устраняет host signedness warning.

`fs/nls/nls_base.c` и `nls_cp437.c` переиспользуются из существующего FAT port,
с теми же pinned source hashes и единственной реализацией. Сохранены UTF-16
surrogate handling, остановка на NUL, замена непредставимых NLS символов `?`,
`;1` и trailing-period rules для каждого Joliet компонента. Полученный charset
переводится обратно в UTF-8 для Unicode archive ABI. Plain ISO mappings
сохраняют существующий OEM-to-Unicode adapter. RR NM names не проходят ISO
translation; при отсутствии NM используется оригинальный Linux fallback.
Probing state без RR markers сохраняет исходный archive namespace.

В `get_joliet_filename` длина расширена с `unsigned char` до `int`: 274 UTF-8
байта не должны превращаться в 18. Native kernel воспроизводит truncation;
исправленный reader сохраняет полное имя. Реальный host filesystem может
ограничивать длину компонента; archive listing/stream/source-entry не обрезаются.
Пустой результат conversion скрывается, как Linux `len <= 0`.

`IsoHandler.h` адаптирован под штатный `ISetProperties`; исходный interface
macro и module boundary сохранены. Property loop следует исходному NTFS handler.
Open properties `map=n|normal`, `map=a|acorn`, `map=o|off` подключают Linux
normal/Acorn/raw modes; `iocharset` выбирает Joliet charset из Linux NLS,
например `utf8`, `cp437`, `cp1251`, `sjis`, `euc-jp` или `iso8859-15`.
Например, dynamic engine принимает `7z l -mmap=a image.iso`.
Acorn использует исходные ARCHIMEDES marker, mode и file-type rules.
Неверный тип/значение возвращает E_INVALIDARG без изменения прежних options;
пустой SetProperties сбрасывает defaults. Обычный ISO по умолчанию сохраняет
исходное 7-Zip отображение, Joliet/RR используют исправленный path.
Standalone `7zz` не получает GPL readers и этот interface extension.
Прежняя [FAT/NLS dependency boundary](fat-linux.md) переиспользуется:
общие C sources и CP437 остаются authoritative для обоих модулей.

## Полный выбор NLS для Joliet

Сопоставлены полный `fs/nls/Makefile`, Kconfig, charset tables, conversion
functions, alias fields, init/dependency branches и `nls_base.c:find_nls`.
Все charset modules из этого Makefile подключены к ISO: таблицы и `uni2char`/
`char2uni` перенесены под исходными путями как C, вместе с EUC-JP helpers и
зависимостями CP932/KOI8-U. Baseline manifest фиксирует полный reference snapshot;
UCS2 utility service не является charset module и не нужен этому caller.
UTF-8, как исходный `isofs_fill_super`, использует null NLS и оригинальный
UTF-16 converter; surrogate pairs сохраняются этим отдельным read path.

Вместо kernel module loader/registration/refcounts применяется неизменяемый
список linked-in table accessors. Charset и alias сравнения сохранены;
существующая case-insensitive archive property нормализует ASCII spelling.
Неизвестное имя возвращает E_INVALIDARG без изменения прежних settings;
reset возвращает archive UTF-8 default, независимо от CONFIG_NLS_DEFAULT хоста.
`default` использует исходную fallback table из `nls_base.c`.
Kernel `wchar_t` заменён переносимым u16, includes — narrow service header.
Dependency pointers EUC-JP/KOI8-RU получаются из статических accessors;
их init-time case-fold pointer assignments не нужны Joliet conversion.
Ни одного codec или conversion table независимо не реализовано.

Unicode archive ABI получает результат NLS conversion через исходный
`char2uni`, продвигаясь на фактически потреблённое число bytes, затем через
Linux UTF-16/UTF-8 converter. Прежний однобайтовый loop был непригоден для
Shift-JIS/GBK/Korean/Big5/EUC-JP. Output bounds проверяются до публикации имени.
В KOI8-RU исправлено инвертированное условие двух byte overrides в `char2uni`:
прочие bytes, включая ASCII, должны обращаться к KOI8-U. Linux Joliet выдаёт
только encoded bytes через `uni2char`, поэтому этот inverse bug не меняет его
native encoded namespace; archive adapter не воспроизводит повреждение Unicode.

Сохранены особенности исходных tables, включая отсутствие ASCII digits в
ISO-8859-6 и неполную private-use поддержку CP932/EUC-JP. Version stripping
выполняется после conversion, как Linux; ISO-8859-6 может оставить `;?`.
Таблицы Mac сохраняют исходные Unicode copyright/permission notices и явное
уведомление об адаптации. Полный Unicode notice устанавливается рядом с
GPL/LGPL/syscall/zlib notices на всех платформах. GPL code остаётся изолированным
в ISO-модуле; standalone engine не получает NLS property extension.

## Глубокие каталоги и lifetime дерева

Сравнены полные исходные `ReadDir`, `CreateRefs`, `CDir::Clear`, callers и
`CObjectVector` copy/delete/destructor paths. Linux directory/inode lookup
не накладывает archive limit 256 на глубину тома; native openat позволяет
проверить filesystem tree отдельно от лимита длины одного host path.
Исходный 7-Zip record parser выделен без изменения его ветвей; recursive DFS
заменён явными frames с тем же порядком section/group processing и refs.
Active-ancestor extent checks, metadata budget, ошибки и callbacks сохранены.
Это C++ adaptation исходного алгоритма, без нового format parser или VFS emulation.

`CDir::Clear` освобождает subtree снизу вверх без allocations и зависимости
от call stack. Временные owner links устанавливаются при cleanup; частично
построенное дерево не требует уже созданных archive refs/Parent pointers.
Parent инициализирован до копирования записи. Destructor очищает descendants
до вызова исходного `CObjectVector` destructor; пустые child destructors не
образуют рекурсивную цепочку. Старое поле TooDeepDirs сохраняет layout/API,
но произвольный предел не применяется. Оба engines получают эту адаптацию.

Полный install smoke выявил следующую зависимость: исходный
`CPP/Common/Wildcard.cpp:CCensorNode::CheckPathVect` рекурсивно копировал хвост
пути при фильтрации каждого элемента. Профиль 4096-level CLI selection показал
CPU в этой ветви; diagnostic процесс остановлен. Исходные item matching,
exclude-before-child и child-before-parent-include ветви сохранены, а обход
использует frames и offset в неизменном path vector. AddItem и освобождение
subnodes также итеративны. Нового mask matcher не добавлено.

Одна authoritative адаптация Wildcard.cpp/.h используется Qt и обоими native
engines; Qt string/localization boundary обёрнут compile guard. Native path
case policy сохранена. Полные исходные тела и dependencies сравнены с тем же
7-Zip commit; runtime manifest подключает эти два overlays. Отдельные проверки
покрывают ancestor include fallback, глубинное exclude, file/dir flags,
literal/wildcard matching и 4096-component selection/cleanup. ISO CLI regression
теперь извлекает leaf в stdout на глубине 4096 обоими engines с обычным timeout.

На MSVC copy operations CDir объявлены defaulted из-за C5267 при добавлении
destructor. Изолированный transitive-header rebuild fixture также обнаружил
LNK1104 при relink только что выполненного probe.exe; он запускает отдельную
копию executable, сохраняя target/object и compiler cache. Проверка по-прежнему
наблюдает изменение sizeof после правки только транзитивного header.

## Собственный inode каталога и его секции

Сравнены `isofs_normalize_block_and_offset`, полный `isofs_read_inode`,
`isofs_read_level3_size` и mapping loop `isofs_get_blocks` вместе с callers
lookup/readdir и `CL` metadata transfer. Каталог идентифицируется по effective
extent родительской записи и offset 0. Его собственный dot record определяет
размер, data extent, timestamps и RR inode metadata; родительский NM сохраняет
имя. Parent PX больше не превращает wire-directory в файл. Descriptor flags
и descriptor/parent size не подменяют собственный inode, включая root.

Оригинальное тело `inode.c:isofs_read_level3_size` перенесено как C. Сохранены
последовательный scan, padding, сборка пересекающего block record, накопление
размера, next-section identity и cleanup. Bounded archive projection возвращает
section extents/sizes вместо VFS page mapping; существующий C++ reader обходит
их, сохраняя ancestor-cycle и source/volume bounds. High Sierra использует
shifted flags через byte-offset helper вместо отрицательного array index;
byte-pointer arithmetic при сборке record заменяет GNU void-pointer extension
для MSVC без изменения копируемых bytes.
Joliet и HS получают собственные directory defaults/dates через тот же boundary;
RR parser вызывается только при включённом RR state.

Дополнительная archive validation требует dot identifier/directory flag,
выравнивания nonfinal sections и terminal record не позднее 101-й секции.
Linux при превышении своего scan limit может вернуть частичный размер;
archive reader отклоняет такую цепочку. Interleave и некорректный extent
отклоняются до публикации каталога. VFS cache/operation tables, mount permission
overrides и cruft mount option не копируются. GPL code остаётся только в
изолированном module; standalone engine сохраняет прежнее чтение ISO inode.

## Выбор read path, legacy sizes и необязательные даты

Сравнены полный option parse/default/assignment scope `inode.c`, выбор
primary/Joliet и descriptor scan, `isofs_read_inode` до/после RR overrides,
mapping loop `isofs_get_blocks` и filter в `do_isofs_readdir`. Переданы read
options `norock`, `nojoliet`, `nocompress`, `hide`, `showassoc`/`unhide`, `cruft`
и `sbsector`. Property loop использует существующие LGPL MethodProps helpers
`PROPVARIANT_to_bool`/`ParsePropToUInt32` с их полными callers/dependencies.
Их authoritative adaptation сохраняет mixed baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` из того же 7-Zip repository;
StringToInt переиспользуется из текущего exact baseline. Второго bool/number
parser не добавлено. Settings применяются атомарно; пустой SetProperties
сбрасывает defaults следующего Open, а существующий extent map остаётся прежним.

`norock` отключает также старый SUSP fallback handler, а не только Linux PX/NM.
`nojoliet` исключает Joliet из root choice, сохраняя RR preference и fallback
правила. File inode defaults/date projection применяется и без RR/Joliet.
`nocompress` подключает исходную C ZF branch: caller получает хранимые bytes,
включая испорченный zlib payload, без попытки декодировать их как zisofs.

Исходная C flag predicate скрывает associated records по умолчанию; `showassoc`
и его Linux alias `unhide` включают их. Hidden flag сам по себе не скрывает
record, пока не задан `hide`. Predicate вызывается до metadata/name conversion
и обхода child directory, поэтому скрытый повреждённый child не ломает каталог.
Это Linux read defaults; standalone archive handler сохраняет прежнюю выдачу.

`cruft` передаёт исходное `i_size &= 0x00ffffff` перед RR overrides. Полный
declared section size сохраняется для mapping; EOF ограничивает физически
используемый префикс. Один размер используется проверкой source/volume bounds,
созданием stream и расчётом archive physical size. Directory sections аналогично
ограничены собственным inode size; неиспользуемый tail не читается. Строгий
режим без `cruft` сохраняет прежние проверки. `sbsector` сдвигает только начало
descriptor scan (сектор +16…+99); addresses extents/boot catalog остаются
абсолютными. Все вычисления start position — 64-bit, U32 value обязательна.
High Sierra root record читается из выбранного descriptor sector.

Primary определяется по type, а не позиции в vector; неизвестный supplementary
перед ним не подменяет выбранный том. Первый Joliet-family descriptor фиксирует
выбор дерева, включая неизвестный level и `nojoliet`; последующий descriptor
не заменяет его. Joliet без primary допускается, если Joliet включён.
`norock` выбирает Joliet до чтения primary inode/geometry, как `root_found`;
без этого flag невалидная primary geometry остаётся ошибкой. Physical-size
reporting использует выбранный том и фактически читаемые records, игнорируя
невалидный размер неиспользуемого primary. Flags/reserved bytes и identifier
descriptor root не подменяют validation его собственного dot inode.
После фиксации выбора дерева сохраняется bounded поиск El Torito boot record:
это существующая archive extension, которую Linux VFS не предоставляет.
Поздние volume descriptors при этом не парсятся и не меняют выбор дерева.

Kernel не использует descriptor dates для VFS inode times. Необязательные
даты больше не могут отменить открытие корректного дерева: raw fields сохраняются,
пустые и синтаксически повреждённые значения не публикуются как timestamps.
Корректные long-form values используют уже перенесённый `util.c:iso_date`
с GMT sanity, calendar normalization и hundredths. Source-visible strict
ReadDigits gate standalone engine не меняется; C algorithm не переписан.

Device TOC/session discovery, аппаратный mount block size, VFS permission/owner
overrides и casefold lookup остаются OS services; они не читают дополнительные
дисковые features в archive projection. Вместо device session ioctl доступен
явный descriptor sector. Mutable VFS, запись и page cache не переносятся.

## Оставшийся scope

Остаётся проверка остальных RR compatibility branches на производителях и
завершающая сверка read scenario по всем source-visible ветвям. Эти read features остаются задачей,
а не принятой окончательной границей совместимости. UDF ещё не проверен.
Kernel VFS/mount services, запись и page cache не копируются.

El Torito virtual entries и uppercase ISO names сохраняют исходный archive
namespace. Kernel default lowercase projection проверяется отдельно;
boot virtual directory в Linux VFS не существует. Joliet Unicode проверяется
в исходном archive namespace. UI/platform adaptation в этой итерации нет.

## Проверки

Fixtures созданы официальным genisoimage и содержат только generated data;
[происхождение](../../tests/fixtures/iso/README.md). CLI проверяет оба engines;
ABI проверяет независимые/retained streams, ошибки, отмену, невалидные maps
и повторное Open; ArchiveBackend — auto-list, selected extraction и source-entry
с Unicode/spaces. Header-dependency regression компилирует маленький C++ consumer
и проверяет новый размер структуры после изменения только транзитивного header.

Нативное чтение выполнено через read-only loop mounts Linux 6.8.0-146-generic:
plain/Joliet, XAR file/root, multi-section file/directory, unknown/foreign VD,
missing terminator, unit-without-gap и BE mismatch — bytes PASS. Блоки
512/1024 и 512-byte volume tail проверены на вариантах с sector padding:
ядро 6.8 пропускает нули до 2048, тогда как pinned reference `dir.c` использует
размер логического блока. Fixtures с padding 512/1024 проверены engines/ABI,
но не принимаются старым native kernel; это различие версии, а не native PASS.
Образы не изменились, все temporary mounts/images удалены.

Focused `iso`, `iso_stream`, `upstream_baseline`, `upstream_inventory`,
`runtime_cache`, `runtime_package`, `archive_external_security` — 7/7 PASS на
macOS arm64 и Linux/Windows x64, Qt 6.12; Windows — MSVC. Remote receipt:
`20261019T145027Z-c5cca06c`. Native make и nmake header-only rebuild проверены
поведением executable. Full/slow suites, physical CD devices и UI не проверялись;
source scope выше не выводится из runtime PASS.
macOS app target собран; фактический CMake install runtime автоматически
распознал и извлёк plain/Joliet, 512-byte tail, XAR и multi-section file/directory
с точным совпадением bytes. Одноразовый install staging удалён.

Rock Ridge fixtures добавляют native genisoimage `-R` и `-R -J`: Unicode,
239-byte NM, длинную SL target с CE и дерево глубже ISO relocation threshold.
CLI/ABI/backend проверяют data bytes, punctuation и RR-vs-Joliet choice,
retained symlink/relocated streams, PX/PN/TF, ошибки CE span/volume/loops,
SL overflow и CL mapping. Bad/canceled Open очищает каталог. CL regression
различает inode-record identity и совпадающий data extent. Проверяется fallback
при пустом primary, внеобразном root и повреждённой длине имени.

Native Linux 6.8.0-146 read-only mounts: 12/12 generated cases PASS — оба native
образа, long TF, legacy/wide PN, три Joliet fallback, CL same-extent,
RR без ER с/без Joliet и 5001-file shared CE. Сверены
обычные bytes, readlink, UID/GID/mode, device numbers и TF times с fractions.
Special devices не открывались; mounts использовали nodev/nosuid/noexec.
Отдельно проверен отказ root с file inode mode. На прежнем engine до budget fix
shared-CE образ давал exit 2 без данных; Linux VFS читает его, исправленный engine
извлекает payload. Это regression значимого false rejection, не coverage counter.
Temporary images/mounts удалены; входные образы не изменились.

Rock Ridge focused 7/7 suites PASS на macOS arm64, Linux x64 и Windows x64/MSVC.
После финальных root/budget/path уточнений повторены затронутые `iso`/`iso_stream`
— 2/2 PASS на всех трёх ОС, Qt 6.12. Final remote receipt:
`20261019T154035Z-eb1faaf9`; 7-suite receipts: Windows
`20261019T151230Z-788f2467`, Linux `20261019T151023Z-13dd0d99`.
Incremental builds/caches сохранены. Full/slow/UI/physical-device acceptance
не выполнялась. Post-transfer comparison отделяет скопированные Linux bodies,
адаптированные I/O/VFS boundaries и исходную 7-Zip path/extent implementation.

macOS app target собран. Фактический CMake install runtime автоматически
распознал RR/RR+Joliet и извлёк native/derived variants; финальный installed
typed-directory reader проверен на RR+Joliet, RR без ER и 5001-file shared CE.
GPL/LGPL/Linux-syscall-note notices проверены по содержимому. Install staging
и временные images удалены. Deployment меняет Mach-O bytes, поэтому совпадение
binary hash с build artifact не используется вместо проверки установленного reader.

Zisofs CLI проверяет native mkzftree bytes, shifts 0/8/10/15/17,
таблицу через block boundary, sub-page sparse, expanding blocks, zero-size,
ignored embedded header, header size 0, padding, equal sparse pointers,
multi-section и 512-byte blocks. CRC/truncated zlib, short/long decoded output,
неверный порядок/границы pointers и короткая таблица дают DataError.
ABI/backend проверяют независимые/retained streams, seek, metadata/progress,
I/O HRESULT, отмену и Unicode selected/source-entry extraction.

Native Linux 6.8.0-146 read-only loop mounts подтвердили native bytes,
shift 0/17, pointer-table boundary, padding, header size 0/ignored header, empty, sparse marker,
multi-section и 512-byte geometry (с явным `block=512`). Expanding-block fixture
даёт kernel EIO с `isofs_get_blocks: block >= EOF`; sub-page sparse fixture
возвращает неверные bytes начиная с offset 1025. Эти результаты не считаются
native PASS и не воспроизводятся как требование совместимости. Все mounts
использовали nodev/nosuid/noexec; temporary images/mounts удалены.

Zisofs focused seven-suite batch: macOS arm64, Linux x64 и Windows x64/MSVC
7/7 PASS, Qt 6.12; remote receipt `20261019T161308Z-b9195c80`.
macOS app build и фактический CMake install проверили exact extracted bytes,
ошибку повреждённого потока и GPL/LGPL/syscall/zlib notices; staging удалён.
Source post-comparison и exact-source SHA-256 выполнены отдельно от runtime.
Full/slow/UI/physical-device acceptance не выполнялась.

Read-policy/descriptor iteration: CLI и ABI проверяют фильтрацию до обхода
повреждённого child, showassoc/unhide и combined flags, primary/Joliet/RR
selection, отключение ZF decode, legacy sizes с неиспользуемым invalid tail,
ISO/HS descriptor sector и отказ при неверных параметрах. Дополнительно
проверены supplementary перед primary, отсутствие primary, flags Joliet,
первая Joliet-family запись и поздний boot record. Необязательные повреждённые
даты не ломают backend listing/selected/source-entry extraction. API проверяет
GMT/fractions/normalization и отсутствие timestamps для пустых/повреждённых
fields, retained streams и стабильность текущего map после reset options.

Native Linux 6.8.0-146 read-only mounts подтвердили 31 positive fixture case,
два negative root-choice controls и отдельно уточнённое сочетание flags.
Проверялись выбранный namespace/отсутствие filtered entries и payload bytes.
Descriptor timestamp values и виртуальный El Torito stream не публикуются
Linux VFS и проверены отдельно через archive ABI/CLI. Все образы generated-only;
mounts и temporary files удалены.

Focused seven-suite batch: 7/7 PASS на macOS arm64, Linux x64 и Windows
x64/MSVC, Qt 6.12; descriptor-selection receipt
`20261019T193927Z-91afd260`. Финальный macOS seven-suite batch после сохранения
El Torito extension — 7/7 PASS; Linux/Windows `iso`/`iso_stream` — 2/2 PASS,
receipt `20261019T194211Z-c58111ec`. Earlier policy/flags batches также прошли
на трёх ОС; их green status не заменяет финальное source comparison.

После реализации повторно сверены тела/branches/callers, bounded reads,
physical-size consumers и LGPL/GPL/standalone boundaries; pinned source hashes
проверены отдельно. macOS app target собран, actual CMake install проверил все
31 read profile по extracted bytes, отказ неверных options и пять notices.
Staging удалён. Full/slow/UI/physical-device acceptance не выполнялась;
существующие incremental build trees и compiler caches сохранены.

После уточнения fixtures и wide-mask предупреждения повторены `iso`/`iso_stream`
на всех трёх ОС: 2/2 PASS, remote receipt `20261019T161535Z-d5535c87`.

High Sierra CLI/ABI/backend проверяют exact bytes, автоопределение без
расширения, XAR/multi-section, retained streams, 1024-byte logical blocks,
невалидный descriptor/root/name и очистку каталога после failed Open.
ISO с перекрывающимся HS magic остаётся ISO. Recording dates проверяются
на ±52/±53 timezone, нормализации 32 января и HS reserved flags byte.

Native Linux 6.8.0-146 read-only mounts подтвердили HS plain/XAR/multi-section,
primary после чужого descriptor, 1024-byte sector-padding variant, шесть
date variants с точным UTC epoch и ISO-first collision case. У fixture с
padding на границе 1024 старое ядро пропускает child record до 2048;
это уже установленное version difference с pinned `dir.c`, не native PASS.
Все образы содержали generated payloads; временные mounts/images удалены.

High Sierra focused seven-suite batch: 7/7 PASS на macOS arm64, Linux x64 и
Windows x64/MSVC, Qt 6.12; remote receipt `20261019T172921Z-4bcca3a0`.
После финальной правки descriptor scan повторены `iso`/`iso_stream`: 2/2 PASS
на всех трёх ОС, remote receipt `20261019T173239Z-d22e4cac`.
macOS app target собран; фактический CMake install распознал extensionless
ISO/Joliet/HS, извлёк plain/XAR/multi-section/leading-descriptor/1024-byte данные
и отверг неверный descriptor. GPL/LGPL/syscall/zlib notices сверены по содержимому;
staging удалён. Post-transfer source comparison выполнен отдельно.
Full/slow/UI/physical-device acceptance не выполнялась.

Naming CLI/ABI проверяют нормализацию каждого Joliet компонента, surrogate
pairs и их повреждения, NUL/empty identifier, полное длинное UTF-8 имя,
UTF-8/CP437 и normal/raw/Acorn mappings, malformed Acorn marker/length,
RR fallback без NM. Backend проверяет selected/source-entry extraction для
Unicode и нормализованного parent, source-entry для длинного имени и отказ
traversal без изменения файла вне destination. Неверные open properties
не меняют прежний режим, reset возвращает archive defaults.

Native Linux 6.8.0-146 read-only mounts: 12 naming/mapping/omission cases PASS;
дополнительный long-name case воспроизвёл kernel truncation 274 → 18 UTF-8
байт. Это отличие не воспроизводится в модуле. Native NLS сравнивался в raw
CP437 bytes, Unicode archive ABI — в UTF-8. Все fixtures generated-only,
temporary mounts/images удалены. Source comparison выполнен отдельно.

Naming seven-suite batch: 7/7 PASS на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12; remote receipt `20261019T174846Z-d42720f7`.
После empty-identifier boundary правки повторены `iso`/`iso_stream`: 2/2 PASS
на всех трёх ОС, receipt `20261019T175213Z-7605db25`.
macOS app target собран; фактический CMake install проверил component/emoji/long
names, empty omission, CP437, normal/raw/Acorn и RR fallback extraction bytes,
а также GPL/LGPL/syscall/zlib notices. Одноразовый staging удалён.
Full/slow/UI/physical-device acceptance не выполнялась; persistent builds и
compiler caches сохранены.

Deep-tree fixtures: native Linux 6.8.0-146 read-only loop mounts прочитали
leaf bytes на глубинах 768 и 4096 через openat, без создания host deep tree.
Оба CLI engines извлекают 4096-level leaf в stdout; ABI проверяет полный
каталог, отмену по progress, очистку после deep cycle/error и retained stream
после Close/Open/release. Backend listing публикует 4097 entries и выполняет
точный source-entry extract; Windows technical separators проверяются в
штатном native namespace. Запись полного дерева в host filesystem здесь не
проверяется: stream acceptance не отменяет host path limits.

Deep-tree/shared-selector batch: 9/9 PASS на macOS arm64, Linux x64 и Windows
x64/MSVC, Qt 6.12. Linux nine-suite receipt `20261019T181425Z-791eb546`,
Windows `20261019T181616Z-0f8cf443`; после финального PCH boundary и mask cases
Linux five-suite PASS `20261019T182136Z-010cf69a`, Windows wildcard PASS
`20261019T182558Z-de6a82b4`. Финальный backend regression: macOS PASS,
Linux `20261019T182903Z-61cbc20b`, Windows `20261019T183038Z-c6b9a7c4` — PASS.
Исходный ISO parser, общий selector, Qt selection, snapshot/inventory/cache,
runtime package и прямые security consumers проверены адресно; full/slow/UI
acceptance не выполнялась. Проверки reused incremental builds и compiler cache.

macOS app target собран. Фактический CMake install извлёк 4096-level leaf
с точным совпадением bytes за 3,69 s, отверг deep cycle и установил четыре
notices без изменений. Это результат данного smoke, не performance contract.
Temporary images/mounts/staging удалены; объёмный diagnostic sample заменён
кратким свидетельством. Post-transfer source comparison выполнен отдельно.

Directory-inode iteration: CLI и ABI проверяют независимость от parent/root
size и descriptor flags, noncontiguous root/child sections, padding и split
record, предел 101 sections и отказ от незавершённой/повреждённой цепочки.
Parent NM сохраняется при конфликтующем PX; mode/UID/GID/nlink берутся из
own dot record. Проверены retained streams, failed Open cleanup, ошибка
источника при чтении root inode и backend selected/source-entry extraction.

Native Linux 6.8.0-146 read-only loop mounts подтвердили десять valid cases,
включая точные bytes при 101 sections и own-directory metadata. Generated
images и mounts удалены. Pinned `fs/isofs`/UAPI hashes сверены, изменённые
тела и callers повторно сопоставлены с исходниками отдельно от runtime.

Seven-suite batch: macOS arm64 и Linux x64 7/7 PASS; Linux receipt
`20261019T185539Z-2acea143`. MSVC потребовал byte-pointer arithmetic вместо
GNU void-pointer extension; Windows x64 после правки 7/7 PASS,
receipt `20261019T185628Z-769b041e`. Финальные `iso`/`iso_stream` после этой
правки: macOS и Linux 2/2 PASS, Linux `20261019T185630Z-daa3de6b`.
macOS app target собран; actual CMake install проверил восемь directory
variants, отказ при превышении section limit и четыре notices. Staging удалён.
Full/slow/UI/physical-device acceptance не выполнялась; incremental builds
и compiler caches сохранены.

NLS iteration: CLI fixtures проверяют все подключённые charset families и
aliases по извлечённым bytes, имена каталогов, непредставимые surrogate units,
EUC-JP IBM/NEC/halfwidth sequences и длинное многобайтовое имя. ABI дополнительно
проверяет точные Unicode paths, uppercase alias, сохранение charset при ошибочной
property, reset/reopen и retained stream после освобождения handler.
Проверки не читают исходники и не фиксируют layout registry.

Восемь focused suites (`iso`, `iso_stream`, `fat`, baseline/inventory,
runtime cache/package и archive security consumer): 8/8 PASS на macOS arm64,
Linux x64 и Windows x64/MSVC, Qt 6.12. Remote receipt
`20261019T191201Z-6eaa3b2a`. Shared FAT/NLS caller проверен отдельно от ISO
codec cases; runtime reused persistent build trees и compiler caches.

Native Linux 6.8.0-146 read-only loop mounts подтвердили 58 cases: raw encoded
NLS paths и payload bytes; Unicode archive ABI проверялся отдельно. Mac codecs
в госте установлены как modules, но не имеют `nls_<charset>` autoload aliases.
После проверки config/modinfo они временно загружены под штатными module names;
все 11 загруженных этой проверкой modules затем выгружены. Mounts/images удалены.
Для длинных EUC-JP имён kernel выдаёт 25/23 bytes вместо 281/279: это отдельное
свидетельство прежнего uchar truncation, а не native parity PASS. Archive reader
сохраняет полные имена; источник и данные при этом не меняются.

Post-transfer comparison подтвердил исходные tables и полные conversion bodies
49 новых C units после перечисленных narrow adaptations; fallback table/body
сверены с `nls_base.c`. Pinned NLS reference hashes соответствуют manifest.
macOS app target собран, actual CMake install извлёк payloads для всех 59
NLS fixture cases и установил пять notices с точным содержимым. Staging удалён.
Full/slow/UI/physical-device acceptance не выполнялась.
