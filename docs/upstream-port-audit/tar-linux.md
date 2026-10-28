# TAR: Linux CPT reader и потоки SDK

## Итоговая read-side сверка — 2026-10-22

Read-side source audit TAR закрыт на закреплённом Linux commit для archive
projection ниже. Повторно сверены полные `load_tar_archive`, `is_mem_zero`,
дисковые структуры, `process_tar_file` и release/selection consumers с исходным
SDK header/item traversal, `linkdata` и partition-independent entry streams.
Все девять reference hashes manifest совпали с текущим snapshot.

| Read scope | Результат сверки |
| --- | --- |
| Headers и regular payload | Magic, octal fields, 512-byte records, payload spans и terminal zeros сопоставлены; SDK сохраняет более строгие checksum/source bounds и более широкие numeric forms. |
| Nonregular physical layout | Linux declared-size traversal доступен через `linkdata`; default GNU directory/hardlink compatibility и причина явного выбора описаны ниже. |
| Archive consumers | Текущие Open/GetStream/Extract используют исходные parser/copy/sparse bodies с уже описанными bounds, progress, source lifetime и independent cursors. |
| Дополнительный SDK scope | PAX/GNU/sparse и writer/update consumers сохранены; соответствующих читателей в Linux CPT нет, hardware acceptance из них не следует. |

Новых отсутствующих read branches в Linux TAR scope не обнаружено. Эта итерация
уточняет source completeness без изменения кода. Прежняя runtime evidence ниже
остаётся ограниченной указанными сценариями/платформами; после изменения общего
archive discovery TAR CLI suite обоих engines повторно прошла в macOS проверке
`fb09caf3`. Full writer, firmware upload и native device acceptance не заявляются.

## Исходники и полнота переноса

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полные `CPP/7zip/Archive/Tar/TarHandler.cpp`, `TarHandler.h`, `TarIn.cpp`,
`TarIn.h` скопированы по исходным путям в `src/`. Сохранены parser, properties,
PAX/GNU/sparse branches, extraction loop и writer properties. Прочитаны типы
`TarItem`/`TarHeader`, registration, `TarHandlerOut`, `TarUpdate`, `TarOut`,
ограниченные и буферные потоки, progress dependencies и прямые consumers.
Writer units остаются исходными SDK dependencies в runtime snapshot;
второго parser или writer не добавлено. [Manifest](../upstream-baseline.json)
включает четыре адаптированных unit в dynamic и standalone engines.
Функциональный diff отделён от clang-format одинаковым форматированием original.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
В `drivers/crypto/marvell/octeontx/otx_cptpf_ucode.c` проверены TAR types,
`load_tar_archive`, `is_mem_zero`, `process_tar_file`, release/selection и caller.
Сверены microcode types и numeric conversion dependencies;
[source baseline](../tar-linux-baseline.json) закрепляет inventory и hashes.
В production используется C++ parser SDK. Linux GPL C bodies не переносились
в archive engine; firmware acquisition, microcode selection/signatures и загрузка
в устройство не являются операциями чтения TAR и не выполнялись.

## Исправления чтения

Обычные и sparse streams владеют исходным stream и снимком нужных metadata;
symlink stream получает собственные bytes. Они сохраняют содержимое после
Close/Reopen/освобождения handler. Физическое чтение каждый раз восстанавливает
позицию общего источника, поэтому чередование streams и extraction не портит
данные. Seek проверяет отрицательные и переполняющие смещения; преждевременный
EOF не выдаётся за успешное полное чтение.

При создании symlink stream владелец принимает только буфер ссылки: исходный
архивный stream больше не присваивается перед перезаписывающим COM out-параметром.
Это устраняет лишнюю удержанную ссылку; формат и алгоритм чтения не меняются.

Open инициализирует состояние, очищает частичный список при HRESULT failure,
отмене или исключении, сбрасывает заимствованные callback/stream pointers.
После Close нет ложного неизвестного количества items. Сохранена диагностика
частично читаемого повреждённого архива при успешном salvage Open.
Переходы по данным и длинным metadata ограничены размером источника и Int64;
ошибки источника сохраняются, включая failure вместе с последними bytes read.
Есть initial/final progress и checkpoints внутри длинных PAX/нулевого хвоста.

Последовательный Extract больше не индексирует отсутствующий random-access
список, останавливает выбранный набор вовремя и учитывает физические данные
и padding. Sparse reconstruction переиспользует SDK map/zero-fill loop и для
последовательного источника. Отмена проверяется при пропуске данных и чтении
длинных metadata; callback не остаётся в archive после операции.

## Объявленные данные служебных записей

Linux пропускает объявленный aligned size всех записей, передавая microcode
consumer только regular types `0`/NUL. SDK по умолчанию обнуляет physical size
hardlink/directory. Для directory это осознанная совместимость с GNU tar:
существуют producers, записывающие ненулевой filesystem directory size без
данных. По одному header нельзя безошибочно различить эти два layout.

Добавлено явное read property `linkdata`: например,
`7zz x -ttar -mlinkdata=on archive.tar`.
Оно сохраняет declared physical size hardlink/directory, поэтому следующие
записи читаются с Linux alignment. Служебный payload не публикуется как обычный
файл. Стандартный режим сохраняет прежний GNU layout; переключатель Qt UI
не добавлялся. Исходный writer сохраняет существующие physical records при
обновлении; проверено добавление файла в такой архив.

## Границы reference и проверки

Linux CPT проверяет magic первого header и UInt32 octal size, но не TAR checksum,
PAX/GNU metadata или sparse maps. Слабая проверка kernel payload bound не
копируется. SDK сохраняет более строгую целостность и более широкий формат;
prefix/PAX names, base256, signed checksum и GNU sparse не урезаются до
возможностей microcode consumer. Правила empty-archive discovery и кодировки SDK
сохранены. Это завершённая сверка выбранного TAR read scenario, не утверждение
о hardware acceptance или полном POSIX writer audit.

Изолированный C-reference содержит неизменённые `load_tar_archive`,
`is_mem_zero`, numeric conversion bodies Linux с userspace allocation/firmware
adapters. Consumer заменён bounded collector длины и hash regular payloads,
а не эмуляцией устройства. Проверены 23 результата, включая ожидаемые различия:
checksum, base256, GNU directory без данных и extended sparse. Для успешных
результатов сверены payload lengths/hashes. Harness не обходит собственную
проверку источника ради воспроизведения небезопасного kernel чтения.

`tar` проверяет 16 вариантов обоими engines, девять повреждений, неизменность
источника, Unicode/spaces и update с GNU/PAX/sparse/linkdata. `tar_stream`
проверяет восемь retained/interleaved вариантов, последовательное выборочное
и полное извлечение с последующей записью, source/output HRESULT, отмену,
повторное использование и диагностику truncated payload. Existing consumers
покрывают archive open/extract/update, вложенные архивы и native gzip→TAR.
2026-10-20: macOS arm64 — 12/12 focused suites PASS, Linux x64 — 12/12 PASS.
Windows x64 — десять PASS в основном прогоне; `tar` и `source_update_backend`
получили startup/process timeouts и прошли адресный повтор без изменения кода
или лимитов. Причина таймаутов не установлена. MSVC warning о reference member
локального RAII guard исправлен до этого runtime-прогона.
После удаления неиспользуемых test includes локальные TAR suites повторно PASS;
source baseline, hashes и whitespace проверены. Full/slow suites, appearance
и физическое устройство не проверялись. Homebrew sevenzip не используется:
CLI и consumers берут подготовленный project runtime.
