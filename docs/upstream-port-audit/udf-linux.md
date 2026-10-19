# UDF: sparse allocation и логический EOF

## Источники и выбранный сценарий

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Адаптированы `CPP/7zip/Archive/Udf/UdfIn.cpp`, `UdfIn.h`, `UdfHandler.cpp`
под исходными путями в `src/`. Оба engine получают одни runtime overlays;
[baseline](../upstream-baseline.json) фиксирует происхождение и зависимости.
Неизменённые `UdfHandler.h`, `StdAfx.h`, регистрация, CRC16, buffers и stream
helpers берутся из официального runtime snapshot. Уведомления и лицензия
7-Zip сохранены. Форматирование C++ учитывается отдельно от функционального diff.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
[Reference manifest](../udf-linux-baseline.json) фиксирует snapshot `fs/udf`;
он не означает перенос или проверку всех перечисленных файлов.
Для этой итерации сравнены `udf_map_block`, `__udf_get_block`, `inode_bmap`,
`udf_current_aext`, `udf_next_aext`, inline validation в `udf_read_inode`,
`udf_get_fileshortad`, `udf_get_filelongad` в `directory.c`,
соответствующие `ecma_167.h` allocation/FE/EFE структуры и partition mapper.
Linux определяет значения recorded/unrecorded descriptors, неявные holes и EOF.
GPL bodies в engine не копируются: используется существующий 7-Zip
`CExtentsStream`, включая его готовый zero-fill режим.

До реализации сравнены полные тела 7-Zip `CItem`, short/long/inline AD parsing,
`CheckExtent`, `CheckItemExtents`, `Read`, `ReadLad`, `ReadFromFile`,
`ReadItem`, metadata partition bootstrap и physical-size consumer в `Open`,
а также свойства, `GetStream` и `Extract`. Сохранены структура модуля,
классы, callbacks, progress, CopyCoder, directory traversal и registration.

## Изменения чтения

- Оба типа unrecorded extents дают нули; backing bytes allocated hole не
  выдаются пользователю, а unallocated position не используется как адрес.
  Короткий список AD также оставляет неявный нулевой хвост до размера файла,
  как unmapped blocks в Linux.
  Нулевая raw length завершает список short/long AD; последующие bytes не
  восстанавливают recorded data после terminator.
- Последний обычный extent ограничивается information length. Полный размер
  выделения и последующие descriptors не заставляют читать за EOF; проверка
  partition bounds применяется к реально читаемой части. Partition reference
  активных descriptors проверяется также для holes.
- Directory buffer читается только до information length. Ранее простое
  ослабление общей проверки равенства размеров позволило бы записать полный
  allocation extent в меньший buffer. Short reads и holes в каталогах не
  превращаются в успешно разобранное частичное дерево.
- Inline allocation сохраняет строгое равенство information length, как
  Linux. Данные вне lengthAllocDescs не становятся payload. Эту ветвь
  проверяют вместе с обычными allocation variants, включая inline directory.
- Physical-size scan использует те же активные длины, уменьшает остаток файла
  и пропускает holes. Sparse offsets и unused tail не расширяют physical size.
- Bootstrap metadata partition сохраняет полную проверку выделенного extent,
  поскольку этот caller создаёт partition mapping из всего descriptor.
  Zero-fill policy обычных файлов на него не распространяется.

После изменений повторно сравнены изменённые bodies и все callers проверок
размеров/extents. `Extract` использует `GetStream`; zero-fill, seek, source
ownership и межпоточная независимость остаются у существующего stream helper.
Новый parser или декодер для этих ветвей не добавлен.

## Проверки и оставшаяся работа

Fixtures основаны на disposable образе Ubuntu genisoimage 1.1.11. `udf`
проверяет оба engine: short/long AD, оба explicit hole вида, implicit holes,
EOF внутри allocation, чтение активной части на границе partition, unused tail,
inline file/directory и directory allocation padding. Отдельные повреждения
проверяют отказ для активного адреса вне partition, плохого partition reference,
невалидного continuation target, inline length mismatch и metadata bootstrap
с hole/выходящим за partition allocation.

`udf_stream` проверяет точные bytes и EOF для всех положительных вариантов,
seek, чередующиеся потоки, чтение после Close/reopen/release handler,
передачу source I/O failure и отмены, а также list/selected extraction и
`extractSourceEntry` через ArchiveBackend. Тесты используют публичный ABI,
не читают исходники и не закрепляют внутренний layout.

Все 17 положительных fixtures совпали по payload bytes с read-only loop mounts
нативного Linux `6.8.0-146-generic`; mounts и временные образы удалены.
Это runtime evidence для указанного ядра, а не выполнение pinned reference.

Пять focused suites (`udf`, `udf_stream`, `upstream_baseline`, `runtime_cache`,
`runtime_package`) прошли на macOS arm64, Linux x64 и Windows x64/MSVC,
Qt 6.12. Remote receipt: `20261019T202259Z-37da3f7a`. После узкого изменения
отступов повторены macOS checks; функциональный код после проверок не менялся.
macOS app собрана; actual CMake install прочитал все 17 вариантов, staging удалён.
Использованы постоянные incremental builds и compiler caches. Full/slow suites,
UI и чтение физических устройств в этой итерации не проверялись.

Полный UDF read audit остаётся открытым. В частности, ещё требуются перенос
allocation-continuation/indirect-ICB paths, VAT и sparable partitions,
полная metadata partition/mirror mapping, names/NLS, symlinks, специальные
inode и metadata/read policies. Проверка этой итерации не означает завершение
всех UDF возможностей или общей задачи форматов Linux. VFS, запись,
journal/mount ownership и page cache остаются вне archive read scope.
