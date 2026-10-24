# Обновление 7-Zip до 26.05 — 2026-10-24

Официальный источник: <https://github.com/ip7z/7zip.git>, полный commit
`b88c6bc09339de32f7d18bbab900686b244f42ba` (release 26.05, 2026-10-21).
Предыдущая default baseline: `9128b80e3a471108678e2b3ec3c985861c8d7b0f`
(26.04). [Точечное обновление ZIP](zip-26.05.md) было выполнено ранее в этом
рабочем дереве; теперь default revision переведена целиком, `overrides` пуст.
Исходный HEAD Desktop: `d8a0aabd6c0d32c535d7d403763935cf9c84fb32`.
Несвязанные изменения QCOW/QEMU, упаковки и drag/drop сохранены.

## Сверка и перенос исходников

Сравнение выполнено до acceptance runs в отдельном Git checkout официального
репозитория. Рассмотрены все изменённые upstream units, полный diff записанных
owners, прямые callers, supporting types и build dependencies. Соседний
`../7zip-upstream` остаётся read-only и не обновлялся.

Exact dependencies `C/7zVersion.h` и `CPP/Common/MyString.h` скопированы
побайтно; оба `Compress/ShrinkDecoder` units уже совпадали с 26.05.
SHA256 полных оригинальных изменённых inventory units, полный commit и mapping
на местные owners хранятся в [baseline](../upstream-baseline.json).
Notices и лицензии сохранены: [официальная лицензия](../../third_party/7zip/License.txt).

Изменения адаптированных units перенесены узко:

- `Archive/HandlerCont.cpp::CHandlerCont::Extract`: инициализация extent info,
  исходный progress update и время жизни output stream; все result branches.
  Локальный `GetStream` с независимым восстановлением позиции retained streams,
  распознавание GPT/ext и cancellation остаются на прежних границах.
- `Archive/NtfsHandler.cpp::CMftRec::ParseRec`, SDK и GPL owners: запрет чтения
  nonresident FILE_NAME как resident bytes. В SDK перенесены resident gate
  volume metadata и полный 12-byte минимум VOLUME_INFORMATION.
  STANDARD_INFORMATION уже отвергает nonresident через `CAttr::ParseSi`.
  GPL active-file names проверяются через `AddNameMetadata`; volume state —
  через исходный Linux `resident_data_ex` с полным размером структуры.
  Оба варианта действительно читают nonresident reparse data через streams,
  поэтому ограничение upstream, где такое чтение отсутствует, не копируется
  поверх реализованного read path. WOF, deleted files и recovery сохранены.
- `UI/FileManager/ProgressDialog2.cpp::UpdateStatInfo`: исходная запись `%`
  после результата `ConvertUInt64ToString`; Qt controls и внешний текст те же.
- `CPP/Common/MyVector.h`: исходный `CRecordVector::AddNew_NoConstruct`.
  Локальные исправления containers не заменены оригинальным заголовком целиком.

Полные официальные native engine и sidecar `7z`, а также SFX, собираются из новой
default revision. Так получены полные изменения `ZHandler`/`ZDecoder`,
`MachoHandler`/`MubHandler`, `VdiHandler`/`VhdHandler`, включая новые связанные
types/API, makefiles и platform flags. Эти модули не подменяются частичным
локальным переносом. В частности, новая Mach-O обработка использует исходные
symbol-table sections, CPU names, metadata и bounds/alignment checks;
`.Z` получает новый decoder и исходную 9-bit compatibility.
Изменения `C/Util/7z/7zMain.c` принадлежат отдельному upstream utility, который
не является нашим sidecar: Desktop поставляет оригинальный `UI/Console/7z`.
Installer project `DOC/7zip.wxs` не подменяет Qt packaging.

Повторная source-сверка после переноса: изменённые тела и consumers подключены,
новых omissions в переносимых units нет. Runtime validation ниже имеет
собственные границы и не заменяет эту сверку.

## Исправления интеграции, выявленные проверками

Три уже записанных NTFS WOF extraction failures воспроизвелись: native listing
считал любой успешно декодированный reparse metadata текст ссылкой.
Upstream `ConvertNtReparseToString` выводит и WOF/другие tags; определение ссылки
в исходном `CReparseAttr::Parse` ограничено link records. Адаптер теперь использует
этот существующий parser, сохраняя raw bytes и listing text. Не добавлялся новый
reparse decoder; прежние WOF/link extraction regressions проверяют результат.

Linux link failure `NUpdateArchive::k_ActionSet_*` выявил неверного владельца
`UI/Common/UpdateAction.cpp`: `ArchiveCommandLine.cpp` уже находится в
`sevenzip_common`, а его constants оставались в `sevenzip_panel` и могли оказаться
за позицией статического linker scan. Source owner перенесён в `sevenzip_common`;
upstream implementation и значения не менялись, второго владельца нет.

При первом запуске в новом branch worktree выявлен ещё один скрытый прогретым
деревом gap: `archive_layer_chain_tests` использует Ext reader, но не требовал
`sevenzip_ext_runtime`. Зависимость объявлена явно. После сборки provider
Ext stream factory и цепочка QCOW → partition → Ext прошли успех, отмену и
ошибку source; добавления нового reader не потребовалось.

SFX builders используют прежние официальные makefiles, но теперь обновляют один
owned snapshot и сохраняют outputs/compiler cache. Общий snapshot builder
предоставляет original-only режим без native runtime overlays; header dependency
generation также охватывает SFXCon/SFXWin, чтобы версия обновлялась при
incremental rebuild. Regressions исполняют synthetic consumer до/после version
change и проверяют original-only source isolation с сохранением outputs.

## Ресурсы

Официальный [Windows x64 installer 26.05](https://github.com/ip7z/7zip/releases/download/26.05/7z2605-x64.exe)
сверен с resources. EN/RU и License совпали побайтно; в HTML/CSS справки
изменился только `Help/start.htm`, приветствие с версией. Новые release/member
SHA256 записаны в `resources/7zip/{provenance,help-provenance}.json`;
CHM index streams по-прежнему не импортируются. UI layout не менялся.

## Проверки

Baseline verify: PASS, одна исходная ревизия; task-owned diff whitespace PASS.
Native `7z`/module и SFX подтверждают 26.05 в metadata; macOS reference client
также выводит 26.05. Исходные exact units совпадают с официальными bytes.

macOS arm64 / Qt 6.12: 25/25 focused suites PASS. Scope: ZIP Shrink/writer,
native archive test/open/list/properties/source extraction, NTFS streams/records/
security, MBR/APM/GPT streams, layer chain, progress format/dialog, creation/update,
codec catalog, SFX creation/lifecycle, help, application startup и baseline/cache
tooling. После локальных исправлений повторены baseline (1/1) и layer-chain/
native-archive-test (2/2). Первые creation/update/SFX failures внутри sandbox
исчезли при повторе вне sandbox; WOF failures были воспроизведены вне sandbox
и исправлены классификацией reparse, а не изменением ожиданий.

Linux x64 / Qt 6.12: engine/SFX и выбранные targets собраны. Batch
`20261024T072525Z-18d421c8`: 19/25 PASS, остальные — readiness/timeout failures.
В повторе `20261024T074439Z-04925e26` прошли полные `mbr_stream`, `ntfs_security`
и `native_archive_test`, включая новые .Z/Mach-O regressions. `ntfs_stream` и
`codec_update` снова достигли общих лимитов suite. Layer chain в новом дереве
выявила отсутствовавший Ext provider, исправленный выше; после его сборки
адресно прошли Ext retained-stream factory и stored QCOW chain для success,
cancel и source failure (2,76 с). Весь расширенный layer-chain matrix после
этого не повторялся. Отдельно прошли три WOF backend-extraction rows,
Unicode-link и junction (8,83 с, включая init/cleanup).

Windows 11 x64 / Qt 6.12 / MSVC: engine, Windows GUI SFX и targets собраны.
Interactive batch `20261024T073522Z-e62ebe81`: 20/25 PASS. Четыре suites
(`ntfs_stream`, `archive_layer_chain`, `native_archive_test`, `codec_update`)
достигли своих timeout; ещё один failure был в новом provenance test,
который предполагал LF в оригинальном Git blob. Исправлено сравнение с самим
blob, сохраняя bytes CRLF. Адресный Windows повтор этого случая PASS (2,33 с);
остальные baseline tests, включая SFX header rebuild, прошли в исходном batch.
Отдельные nonvisual/offscreen проверки новых .Z padding и Mach-O symbol-table
cases прошли (7,68 с), как и три WOF extraction rows, Unicode-link и junction
(4,63 с). Это не native desktop/UI acceptance и не зелёный полный batch.

Контроль задержек на Linux использовал одни и те же Qt binaries/plugins:
в одном существующем runtime build временно собраны 26.04 и затем 26.05,
с обязательным восстановлением 26.05. Один MBR scenario прошёл на обеих:
3,64 с / 2,52 с. Это не доказывает причину всех timeout и не заменяет их
разбор; увеличение тестовых лимитов или обход compiler cache не применялись.

Во время проверки параллельно завершились изменения harness и QCOW другой
задачи. Ранние batches использовали прежний `current`; последний Linux batch
уже использовал постоянный каталог ветки по обновлённому workflow. Старые
деревья сохранены, кэши не очищались. Первая Windows попытка Python-only
перепроверки через новый interactive harness была отвергнута до remote launch
проверкой пути; её точный lock освобождён после подтверждения отсутствия запуска.
Исправленный Python case проверен напрямую в уже синхронизированном snapshot.

Итог: source update выполнен; перечисленные новые/исправленные behavior cases
имеют evidence на трёх ОС. Общая runtime-приёмка runners остаётся частичной:
полные NTFS/update/layer-chain matrices и Windows native-archive suite не имеют
зелёного общего результата в этих прогонах. Не считать это full acceptance.
Полный gate, slow suites, package/release acceptance и rendered UI comparison
не запускались. Incremental build trees и compiler caches сохранены.
