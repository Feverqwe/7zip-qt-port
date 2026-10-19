# ISO9660: descriptors, logical blocks и extent streams

Работа входит в [общую актуализацию по Linux](filesystems-linux.md).
Основной ISO9660 read path исправлен в обоих поставляемых engines.
Отдельный GPL-модуль добавляет оригинальные Linux C Rock Ridge/SUSP readers.
Полная актуализация ISO9660 и отдельного UDF ещё не завершена.

## Источники и source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сравнены полные `CPP/7zip/Archive/Iso/IsoIn.cpp`/`.h`, `IsoItem.h`,
`IsoHandler.cpp`, `IsoHandler.h`, `IsoHeader.cpp`/`.h`, `IsoRegister.cpp`,
используемые stream helpers, ABI и регистрация в console/module/standalone.
Изменённые файлы находятся под исходными путями в `src/`; `StdAfx.h`,
`IsoHandler.h`, `IsoHeader.cpp`/`.h` переиспользуются без изменений в
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
структур и directory validation/lookup в `namei.c`. `compress.c` прочитан для
выделения отдельного zisofs scope; его декодер в этой итерации не переносится.
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
- Malformed directories, циклы и превышение исходного depth cap 256 явно
  отклоняются, вместо успешной публикации неполного дерева. Failed/canceled Open
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
Zisofs ZF metadata распознаётся оригинальной ветвью, но compressed stream пока
явно возвращает UnsupportedMethod. То же относится к попытке читать special
device/FIFO/socket как обычный файл; metadata доступна без обращения к устройствам.

Linux C компилируется как C11 только в `sevenzip_iso`, без unRAR. LGPL closure:
ISO handler/header, ArchiveExports, Common/Windows/stream helpers и CopyCoder.
Модуль устанавливается вместе с GPL/LGPL notices и Linux-syscall-note для
выбранного UAPI type. Standalone `7zz` сохраняет
core fixes и исходный ограниченный RR parser; новые Linux read features
предоставляет dynamic engine приложения, как у остальных GPL format-модулей.

## Оставшийся scope

Остаются zisofs decoding, High Sierra, naming/NLS/Acorn и оставшиеся date quirks,
проверка остальных RR compatibility branches на производителях, полная
dot-record normalization/root multi-section semantics. Сохранённый archive
depth cap отличается от VFS; его снятие требует переноса рекурсивного обхода
в безопасный для глубоких деревьев сценарий. Эти read features остаются задачей,
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
