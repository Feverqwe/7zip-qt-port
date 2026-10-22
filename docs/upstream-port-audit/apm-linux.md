# Apple Partition Map: geometry и завершённое открытие

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/Archive/ApmHandler.cpp` скопирован целиком в `src/` под исходным
путём и зарегистрирован в обоих runtime overlays. Классы, свойства, имена,
legacy map reader, extraction и stream framework сохранены. Полное
форматирование скопированного C++ unit отделяется от функциональных изменений.
Manifest: [upstream-baseline.json](../upstream-baseline.json).

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`,
`block/partitions/mac.c`, `mac.h`, `check.h`. До изменений прочитаны полный
`mac_partition`, структуры DDM/PM и `put_partition`; со стороны 7-Zip — полный
APM handler, общие `CHandlerCont`/extent streams и helpers из `DmgHandler.cpp`
для типов/главного вложенного раздела. После изменений повторно сравнены
значимые ветви. Linux C bodies не копируются и не переводятся в C++:
сохранён usable исходный parser 7-Zip. Notices и лицензия исходного engine
сохраняются; kernel driver и новый GPL bridge не добавляются.

## Результат сверки

DDM принимает все представимые power-of-two block sizes 512..32768, которые
принимает Linux, вместо прежнего потолка 4096. Probe согласован с parser.
Резервные поля DDM и слово после PM signature больше не являются условием
распознавания: Linux их не проверяет. Для обычной APM количество записей
берётся из первого PM, последующие map-count поля не меняют длину обхода.
Неверная более поздняя PM signature завершает карту с сохранением предыдущих
записей, как в Linux. Ошибка чтения, включая E_ABORT, прекращает открытие
и очищает частичные entries; следующий Open начинает новое чтение.

Границы partition вычисляются в 64 битах и проверяются по доступному image,
до публикации. Дополнительная source-сверка `core.c::blk_add_partition` в
[MBR-итерации](mbr-linux.md) уточнила полное native поведение после `mac_partition`:
start за концом устройства пропускается, пересекающий конец размер ограничивается
доступными полными блоками. APM применяет то же правило, сохраняя пригодных
соседей и не возвращая отсутствующие bytes как успешно читаемый раздел.
Недостоверный DDM block-count больше не раздувает physical size за пределы
источника. Пригодные partition offsets/sizes остаются главным read contract.

Сохранён исходный режим 512-byte installation-image map при DDM 2048 и других
геометриях; для него сохранено прежнее чтение меняющегося map count.
Linux не переключается на эту альтернативную карту: это сохранённая
совместимость 7-Zip, проверяемая отдельно от native parity. Прежний archive
quota допускает 256 entries; Linux отвергает initial count >= DISK_MAX_PARTS.
Поддержка 256 не выдаётся за Linux parity. PM status flags не фильтруют чтение,
как и у Linux; исходное применение flags к выбору главного вложенного файла
и отображение характеристик сохранены.

PowerPC boot/root selection, RAID registration, page cache и создание block
devices — kernel/VFS consumers, отсутствующие у read-only archive API.
Их отсутствие не означает пропущенную ветвь чтения APM. Mount, запись,
firmware callbacks и реальные устройства не входят в выполненную проверку.

## Validation

Native Linux `6.8.0-147-generic`, CONFIG_MAC_PARTITION=y: 19 disposable readonly
loop images. 12 valid вариантов (семь block sizes, reserved fields, first-count,
end signature, oversized DDM count) дали ожидаемые partition start/size и
точный payload. Семь invalid signature/geometry/initial-count образов не дали
разделов. Loops отключены, временные images удалены. Legacy 512-byte map
не выдаётся за native positive case.

CLI checks обоих shipped engines проверяют forced extraction, автоматический
выбор внешнего контейнера (`-t*`), неизменность источника, exact payload и normal
code 2 для malformed maps. Временный CLI runtime содержит неизменённые engine
и core module без посторонних plugins. Полный runtime остаётся в ABI/backend
проверках: partial-open cleanup, source E_FAIL/E_ABORT, повторное открытие и
`ArchiveBackend::extractSourceEntry` для 8192-byte geometry с Unicode path.
Произвольный тестовый payload не объявляется пригодной HFS filesystem;
автоматическое recursive открытие внутреннего HFS не является проверкой APM.

Focused suites: `apm`, `apm_stream`, `gpt`, `gpt_stream`, `archive_device`,
`device_folder`, `runtime_cache`, `runtime_package`, `upstream_baseline`,
`upstream_inventory`. Linux x64 и Windows x64/MSVC 10/10 PASS, receipt
`20261020T052328Z-9eb05fcd`; macOS arm64 10/10 PASS (5.50 s).
После удаления дублирующего malformed fixture обе APM suites повторно прошли
на Linux/Windows, receipt `20261020T052527Z-25aac1b0`; production unit неизменён.
Exact baseline verification: 422 exact, 222 adapted, 11 local paths, одна revision.
Дополнительные EOD fixtures и актуальная acceptance — в [MBR-аудите](mbr-linux.md).
Slow, full gate и UI appearance проверки в этой итерации не запускались:
изменён один format handler без изменения общего stream framework.

Остальной scope, включая MBR и дополнительные shared formats, остаётся
в [едином реестре](filesystems-linux.md).

### Повторная integration проверка — 2026-10-22

При расширении GPT geometry обычная CLI suite выявила прежний routing regression:
14 valid APM variants при automatic opening выбирают `lzma` в обоих engines.
Forced APM/ABI reader checks проходят. Сбой повторён на production sources
`20652199` после временного удаления всех GPT/HandlerCont изменений и пересборки
того же incremental runtime. Это не новый GPT regression; прежняя успешная
acceptance выше не доказывает актуальную automatic-routing acceptance.
APM routing остаётся незавершённой частью общей задачи.
