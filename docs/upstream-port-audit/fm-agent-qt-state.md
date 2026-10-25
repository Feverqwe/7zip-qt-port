# FileManager / Agent / Qt: единый источник состояния

2026-10-25. Проверена цепочка открытия, публикации панели, переключения двух
панелей и извлечения: FileManager `CPanel`, Agent `CAgent` / `CAgentFolder`,
Qt archive opening, metadata transport и consumers команд.

## Source comparison

Источник — [ip7z/7zip](https://github.com/ip7z/7zip), закреплённый commit
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Полные `UI/Agent/Agent.cpp`,
`Agent.h`, `UI/FileManager/Panel.cpp`, `PanelMenu.cpp` сверены с текущим
sibling commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`: эти units одинаковы.
Исходные `GetArchive`, `ReadItems`, `BindToRootFolder`, `GetNumberOfItems`
и их локальные consumers проверены до и после изменения. Локальные transport
и guards закрытия/reopen остаются на своих границах.

Upstream хранит архивные metadata в `CArc` и отображает их через Agent proxies.
Дополнительный Qt-список в `MainWindow` не принадлежит исходному алгоритму:
он повторно обходил весь архив, преобразовывал пути и читал признаки каталогов,
но использовался только для проверки наличия записей перед извлечением.
Другой consumer присваивал такой признак в структуру доступности команд,
однако политика команд его не читала.

Удалены этот список, его копирование при переключении панелей и неиспользуемый
признак команд. Проверка перед извлечением читает количество непосредственно
через архивный metadata owner Agent. Она использует весь архив, а не количество
строк текущей папки; нулевое количество и ошибка чтения по-прежнему не запускают
извлечение. `CAgent::Init` принимает один источник metadata: прежний аргумент
`QList<ArchiveEntry>` уже игнорировался. Его consumers переведены на ту же
сигнатуру без изменения assertions и сценариев.

Новых внешних imports нет; исходные hashes и mappings в
[baseline](../upstream-baseline.json) не изменены. Существующие LGPL notices
сохранены. Проверка и повторяемое сравнение выполняются существующим
`scripts/upstream_baseline.py verify/review`; отчёт этого checkout находится
в `build/review/fm-agent-source-review.md`.

## Сохранённые границы

Транспортный `ArchiveEntry` → metadata snapshot нужен synthetic/custom providers;
при обычном native opening metadata поступают от retained handler.
Это не второй изменяемый список панели. Состояния открытия/reopen удерживают
данные до завершения worker и не заменяются ссылками на активную панель.
В первом проходе (`e53b61db`) Flat View и сортировка оставались кандидатами
для отдельной проверки переходов. Продолжение ниже устраняет их Qt-копии.
Пароль участвует в callback и повторной операции. Исследование не объявляет
всю область FileManager/Agent полностью очищенной от migration debt.

Алгоритмы архивов, Agent proxies, real indices, пути операций, selection,
отмена, recovery consent и внешний интерфейс не изменены. Это локальный
рефакторинг Qt-интеграции, а не новый перенос исходного модуля или полный
аудит всех ветвей FileManager/Agent. Измеренный выигрыш памяти/времени не заявлен.

## Проверки первого прохода

macOS / Qt 6.12.0, 2026-10-25:

- `sevenzip_desktop` и 22 selected test targets собраны штатным `build.sh`.
  Worktree использует собственное incremental build tree и общий compiler cache.
- 22/22 focused suites PASS: Agent folder/capability/property projection/real
  indices, archive opening/property catalog/proxy/update callback/selection
  runtime/metadata UI/reload command, source extract/update backend, drag archive,
  owned temp publication, location folder, Flat View, command policy runtime,
  panel focus/selected state, binding lifecycle и codec update.
  Существующие сценарии проверяют Unicode/пробелы, selection, свойства,
  обновление, ошибки и отмену; assertions сохранены. Новых тестов, повторяющих
  устройство кода, не добавлено.
- Первая сборка нашла пропущенный test caller новой сигнатуры; он исправлен.
  Затем 15 suites прошли, семь native suites не нашли runtime в новом worktree.
  Их повторный запуск с явно заданными `PATH` и `SEVENZIP_CODEC_MODULE_PATH`
  подготовленного harness runtime прошёл 7/7 за 48.42 s без sandbox.
  Промежуточные setup failures не считаются приёмкой.
- Baseline verify/review: PASS, 398 exact / 269 adapted / 21 local paths;
  `git diff --check`: PASS. Исходный baseline не продвигался.

Изменение не затрагивает platform-specific branches; Linux/Windows не запускались.
Полный gate, slow suites, release/package acceptance и сравнение внешнего вида
не выполнялись. Сохранённые focused receipts находятся в `build/review/` worktree.

## Продолжение: режимы панели и metadata

По следующему запросу пользователя расширена проверка владельцев и переходов.
Повторно сопоставлены полные исходные units `Panel.cpp`, `Panel.h`, `PanelSort.cpp`,
`PanelItemOpen.cpp`, `PanelFolderChange.cpp`, `App.cpp`, `MyLoadMenu.cpp` и
`MyLoadMenu.h`, включая `ChangeFlatMode`, `SetListViewMode`, `SortItemsWithPropID`,
`OnColumnClick`, `GetSortControlID`, `OnMenuActivating`, создание панелей и
переходы между файловой папкой и архивом. Эти upstream units одинаковы на
двух указанных выше commits. Проверены локальные callers, model/column adapters,
отложенные batches и восстановление активной панели.

Удалены дополнительные копии режима списка, двух Flat View preferences,
режима/колонки/направления сортировки из `MainWindow` и его `PanelUiState`.
Виджеты читают состояние `CPanel`; колонка Qt вычисляется из source property
и каталога. Исходный fallback при недоступной колонке и default direction
сохранены. Для checked state сортировки оба Qt-consumers используют уже
перенесённый `GetSortControlID`: изменена только видимость существующего символа,
нового импорта или второй реализации mapping нет. Новый `RefreshViewActions`
публикует режимы без повторного расчёта command availability на header event.

Удаление копии обнаружило баг: создание панели заменяло сохранённый disk Flat
View пустым `PanelUiState`. Теперь настройка поступает прямо в `CPanel` и
после повторного открытия окна вложенные файлы снова отображаются. Изменения
одной панели не записывают режимы другой; callbacks возвращаются к исходному
owner. Archive Flat View preference остаётся отдельной от disk, как в upstream.

Полная копия `ArchiveInfo` в окне также удалена: её единственный consumer
ограничивал Move для encrypted archive. Он читает metadata привязанного
ресурса, поэтому сторонний backend result не меняет policy открытой папки.
В самом ресурсе physical size и offset теперь имеют одно хранилище в `Info`;
убраны параллельные поля и повторная запись при open/reopen/device metadata.
Source property contract, диагностика tail/unexpected end и проверка возможности
обновления архива со смещением/SFX сохраняются. Из `Agent.h` удалена больше
не нужная зависимость от `ArchiveBackend`; исходные Agent algorithms не менялись.

Дополнительно просмотрены оставшиеся группы состояния:

| Область | Почему данные сохранены |
| --- | --- |
| `PanelUiState` controls, presentation paths, history и scan/viewport state | Привязаны к конкретным виджетам и незавершённой публикации, включая target path до завершения binding; source navigation/history имеют другие контракты. |
| Selection/row metadata | Source indices и marks, Qt selection, parent sentinel и presentation paths имеют разные роли; model identity сохраняется при reorder. |
| `fileListSettings_` | Снимок Qt-представления используется и при создании ещё отсутствующих controls; исходный `SetListSettings` передаёт его вместе с source flags. |
| Open/reopen/backend request snapshots | У каждого запроса своя lifetime/terminal latch; manager завершает создание metadata до публикации результата. Их данные не заменяются состоянием текущей панели. |
| Native binding flags и container context | Часть сохраняется после `CloseNativeOpen`, когда handler уже освобождён, но нужны безопасный reopen/update и глубина вложения; это не просто кэш живого handler. |
| Transport strings и property snapshots | Typed/native metadata и synthetic transport поддерживают разные consumers. Filesystem bytes, Unicode и display paths нельзя безусловно заменять одним строковым представлением. |

Это проверка выбранных цепочек владельцев, а не доказательство отсутствия любого
дублирования в репозитории. В просмотренных группах оставшееся состояние имеет
конкретных consumers или отличающуюся lifetime; оно не удалялось по сходству имён.
Новых imports, обновления baseline или изменений archive algorithms нет.
Текущий повторяемый source report: `build/review/state-owners-source-review.md`.

### Проверки продолжения

macOS / Qt 6.12.0, 2026-10-25:

- Первая focused серия Flat View / panel sort / sort commands / app lifecycle
  window: 4/4 PASS.
- Расширенная серия: 36/36 suites, 831 Qt checks, 0 failed / 0 skipped;
  216.82 s тестов. Проверены режимы, preferences и lifecycle двух панелей,
  каталоги/изменения/сортировка колонок, typed/raw properties, выделение,
  асинхронные большие списки, command policy, архивные свойства, device folder,
  reopen, обновление с SFX, ошибки и отмена. Использованы подготовленные native
  codec/CLI и SFX runtime, свой build tree и существующий compiler cache.
- Финальная source review после этого PASS уточнила границу fallback:
  скрытая колонка проверяется при перестройке каталога, а не при обычной
  пересортировке. Затронутые 15 suites повторены: 15/15 PASS за 80.34 s.
  `sevenzip_desktop` пересобран после этой локальной правки: PASS.
- Новая проверка Flat View обнаруживает потерю отображения descendants после
  повторного открытия окна. Расширена существующая проверка переключения
  панелей: независимые sort/view modes и сохранение provider. Command-policy
  regression подаёт сторонний backend result и проверяет ограничения открытой
  encrypted/read-only папки. Existing property tests продолжают проверять
  physical size, offset, tail и unexpected end через source archive contract.
- Baseline verify/review: 398 exact / 269 adapted / 21 local paths, PASS;
  `git diff --check`: PASS. Отчёты и точный набор suites/targets сохранены в
  `build/review/state-dedup-focused*`, `state-sort-final*` и
  `state-owners-source-review.md`. JSON normal/compact из текущих suites
  сохранены в `build/dev/ui-reports`; screenshots не запрашивались.

Layout, тексты и platform-specific алгоритмы не менялись. Linux/Windows,
полный gate, slow suites, package/release и rendered Windows comparison не
запускались. Этот PASS не заменяет их приёмку.
