# ISO9660: descriptors, logical blocks и extent streams

Работа входит в [общую актуализацию по Linux](filesystems-linux.md).
Эта итерация исправляет основной ISO9660 read path в обоих поставляемых engines.
Полная актуализация ISO9660 и отдельного UDF ещё не завершена.

## Источники и source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Сравнены полные `CPP/7zip/Archive/Iso/IsoIn.cpp`/`.h`, `IsoItem.h`,
`IsoHandler.cpp`, `IsoHandler.h`, `IsoHeader.cpp`/`.h`, `IsoRegister.cpp`,
используемые stream helpers, ABI и регистрация в console/module/standalone.
Четыре изменённых файла находятся под исходными путями в `src/`; `StdAfx.h`,
`IsoHandler.h`, `IsoHeader.cpp`/`.h` переиспользуются без изменений в
`third_party/7zip`. [7-Zip baseline](../upstream-baseline.json) фиксирует их
происхождение и runtime overlays. `IsoRegister.cpp` остаётся в pinned snapshot.
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
Linux используется как reference; GPL bodies в изменённый engine не копируются.
Новых независимых parsers/decoders не добавлено: адаптирован исходный 7-Zip.

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

## Оставшийся scope

Полное Linux Rock Ridge/SUSP чтение ещё требуется актуализировать: CE continuation,
NM/SL/TF/PX/PN, CL/PL/RE relocation, RR-vs-Joliet выбор, inode metadata.
Также остаются zisofs, High Sierra, naming/NLS/Acorn и date quirks, полная
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
