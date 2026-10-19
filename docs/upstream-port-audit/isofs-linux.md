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
`isofs_read_inode` defaults/dot-record projection используются для relocated inode,
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
normal/Acorn/raw modes; `iocharset=utf8|cp437` выбирает Joliet charset.
Например, dynamic engine принимает `7z l -mmap=a image.iso`.
Acorn использует исходные ARCHIMEDES marker, mode и file-type rules.
Неверный тип/значение возвращает E_INVALIDARG без изменения прежних options;
пустой SetProperties сбрасывает defaults. Обычный ISO по умолчанию сохраняет
исходное 7-Zip отображение, Joliet/RR используют исправленный path.
Standalone `7zz` не получает GPL readers и этот interface extension.
Прежняя [FAT/NLS dependency boundary](fat-linux.md) переиспользуется без
изменения shared C sources; NLS tables не копируются второй реализацией.
Остальные kernel NLS tables пока не подключены: неизвестный charset явно
отклоняется; это остаётся незавершённым read scope, а не полной NLS parity.

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

## Оставшийся scope

Остаются дополнительные NLS charsets и descriptor-date quirks,
проверка остальных RR compatibility branches на производителях, полная
dot-record normalization/root multi-section semantics. Эти read features остаются задачей,
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
