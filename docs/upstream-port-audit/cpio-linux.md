# CPIO: raw newc/crc members и hard links

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
`CPP/7zip/Archive/CpioHandler.cpp` перенесён целиком под исходным путём в `src/`
и включён в runtime overlays обоих engines. Сохранены parser, структуры,
properties, legacy binary/odc readers, extraction/copy/checksum и registration.
Полное форматирование скопированного unit отделено от функциональной сверки
с форматированным оригиналом. [Manifest](../upstream-baseline.json) закрепляет
единственный upstream baseline; notices и лицензия engine сохранены.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
До изменений прочитаны целиком CPIO state machine, hard-link hash и
`unpack_to_rootfs` из `init/initramfs.c`, описание buffer format и dispatch
`lib/decompress.c`. [Source hashes](../cpio-linux-baseline.json) закрепляют
проверенные оригиналы. Со стороны 7-Zip прочитаны полный handler, consumers
`kpidINode`/`kpidStreamId` в extract callback, ограниченные/extent streams,
copy coder и zero-tail helper. После изменений повторно сравнены затронутые
ветви и callers. Использован существующий C++ parser 7-Zip; Linux C bodies
в production не копируются и в C++ не переводятся.

## Исправленный read scenario

Raw newc/crc принимает дополнительные NUL в объявленной длине имени до
PATH_MAX, необязательный trailer и отсутствие конечного alignment padding
после полного последнего payload. Распознавание и reader согласованы для
старых regular-file записей с ненулевым rdev. Неиспользуемый checksum newc
не мешает чтению, как в Linux. Допускаются смешанные newc/crc records,
произвольные нулевые промежутки с выравниванием следующих заголовков и
несколько raw members после `TRAILER!!!`.

Hard links объединяются по device/inode/type только для участвующих в группе
записей с nlink >= 2; directories и symlinks не подменяют regular-file данные.
Последняя непустая запись задаёт содержимое всех aliases, независимо от
её положения. Число сохранённых aliases может быть меньше host nlink.
Trailer сбрасывает группу: независимо созданные members с одинаковыми inode
сохраняют собственные данные. Без trailer группа сохраняется. Проверка CRC
охватывает также сохранённые записи, чьи данные заменены последующим alias,
и пустые записи с неверной суммой.

Открытие при source HRESULT failure или отмене очищает частичные items;
повторное открытие читает новый источник. Нулевые промежутки проверяют отмену
порциями. Entry streams используют существующий extent reader с собственным
положением и удержанием источника; interleaved reads, Close/reopen и release
handler не перенаправляют уже выданный поток на другой payload.

## Границы и оставшийся scope

Это законченная итерация raw CPIO entry reading, не завершение всего initramfs.
Смешанные compressed/raw members, продолжение после compressed member и
совместный hard-link state через decompressor dispatch остаются открытыми
в [едином реестре](filesystems-linux.md). Обычный отдельный `.cpio.gz` уже
открывается вложенными archive handlers, но это не доказывает поддержку всей
грамматики initramfs. [Legacy LZ4/lzop readers](linux-frames.md) теперь перенесены
отдельным компонентом; их подключение к общему CPIO dispatcher ещё требуется.

Linux распаковывает rootfs через VFS; archive API представляет записи и их
данные. Создание special nodes, chown, boot/mount и replay конечного namespace
при повторении path не являются реализацией readonly archive reader.
Binary LE/BE и odc сохранены как дополнительная совместимость 7-Zip:
initramfs Linux их отвергает. Archive parser сохраняет диагностику оборванных
headers/data, ненулевых name/alignment bytes и trailer с данными; отдельные
lenient kernel branches для malformed input не объявляются format features.
Возможность поиска вложенного архива с ненулевым offset также сохранена;
проверки malformed CPIO фиксируют начало (`-tCpio:s0`) либо используют ABI.

## Validation

CLI обоих engines: 26 valid вариантов, exact payload каждого regular entry,
hard-link extraction на диск, Unicode/spaces, symlink metadata, automatic
recognition и test command; восемь malformed вариантов завершаются code 2.
Legacy binary LE/BE и odc проверяются отдельно от Linux parity. Источники
неизменны; disposable extraction trees удаляются. CLI использует копии shipped
engines/core без посторонних plugins; ABI/backend сохраняет полный runtime.

ABI checks: metadata sizes, independent/interleaved/retained streams,
E_FAIL/E_ABORT при частичном открытии и чтении, zero-tail callback cancellation,
очистка после неуспешного Open и повторное открытие. Прямой consumer
`ArchiveBackend::extractSourceEntry` извлекает Unicode entry с дополнительными
NUL в имени. Сборка использует существующие incremental outputs и ccache.

Дополнительно временный C-harness компилирует исходный Linux state machine
и hard-link routines без изменения их тел, с POSIX file/allocation adapters.
Он подтвердил 23 valid сценария (regular payloads, symlink targets и shared
inode identities) и отказ трёх checksum negatives в disposable каталогах.
Mtime/chown и decompress dispatch в harness отключены;
это проверка compiled reference parser, не native kernel boot acceptance.
Нативная загрузка initramfs и реальные устройства в этой итерации не проверены.

Focused suites `cpio`, `cpio_stream`, `runtime_cache`, `runtime_package`,
`upstream_baseline`, `upstream_inventory`: macOS arm64, Linux x64 и Windows
x64/MSVC — 6/6 PASS. Remote receipt `20261020T073127Z-412fc9f8`.
После расширения fixtures без изменения production handler обе CPIO suites
повторно PASS на всех трёх ОС; remote receipt `20261020T073350Z-c969f54f`.
Exact baseline verification: 422 exact, 224 adapted, 11 local paths, одна
upstream revision. Full/slow gate и UI appearance не требуются изменением
одного format handler и в этой итерации не выполнялись.
