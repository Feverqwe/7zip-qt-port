# FileManager / Agent / Qt: лишняя проекция записей

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
Флаги Flat View и сортировка имеют исходного владельца и Qt consumers для
настроек, событий и представления; они требуют собственной проверки переходов,
поэтому в этом изменении не удалены. Пароль также участвует в callback и
повторной операции. Исследование не объявляет эти области полностью очищенными
от migration debt.

Алгоритмы архивов, Agent proxies, real indices, пути операций, selection,
отмена, recovery consent и внешний интерфейс не изменены. Это локальный
рефакторинг Qt-интеграции, а не новый перенос исходного модуля или полный
аудит всех ветвей FileManager/Agent. Измеренный выигрыш памяти/времени не заявлен.

## Проверки

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
