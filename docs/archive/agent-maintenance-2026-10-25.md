# Аудит агентской документации и tooling — 2026-10-25

Работа выполнена в основном checkout, ветка `master`, база `457051c8`.
Область: маршруты чтения, актуальность агентских инструкций и планов,
локальная память, entrypoints скриптов и жизненный цикл worktree.
Исторические source/runtime receipts и baselines сохранены; этот аудит
не является новой приёмкой архивных readers или интерфейса.

## Первый проход: изменения документации и cleanup

- `AGENTS.md` и глобальный skill `worktree` теперь оставляют обычную задачу
  в текущей ветке/checkout. Новая ветка и worktree требуют явного запроса;
  повторяющиеся требования к выбору regressions сокращены до ссылки на критерии.
- В архив перенесены внедрённые планы устройств, редизайна и ZIP. Их содержимое,
  решения и ограничения сохранены; относительные и входящие ссылки обновлены.
  Карта документации ведёт к действующим контрактам и source audit.
- Исправлены путь к upstream во вложенном checkout, прежний UI-якорь Options
  и статус интеграции возврата исходной криптографии в master.
- Длинный локальный индекс памяти сохранён отдельным историческим индексом;
  входной README содержит маршруты и ссылки на сохранённые исследования.
- `doctor.sh` привязывает диагностику к своему checkout при запуске из другой
  директории. Профиль `tooling` включает существующий `project_memory_setup`.

## Очистка checkout

Удалены 17 дополнительных checkout, в том числе невмерженные эксперименты,
и 55 регистраций отсутствующих checkout. Остался основной `master`.
Все feature-ветки сохранены; два detached HEAD без других удерживающих refs
получили backup-ветки. Эксперименты не объединялись с master.

Для `fix-app-openssl` и `squashfs-libsquashfs` сохранены патчи и архивы
изменённых/новых файлов. Содержимое архивов перечитано и сверено по SHA-256
перед удалением. После очистки в одноразовом каталоге восстановлены все
53 dirty/new пути из сохранённого HEAD, патча и архива; итоговые хеши совпали.
Сборочные деревья, зависимости, runtime caches, локальная
память и review/failure evidence перенесены без очистки compiler caches.
Два осиротевших синтетических SMB probe завершены после проверки отсутствия
живых монтирований. Работающих сборок/приложений в удаляемых checkout не найдено.

Локальные manifest, хеши, исходные пути/HEADs и инструкция восстановления:
`build/worktree-retention/2026-10-25/README.md`. Эти ignored-материалы существуют
на машине проведения аудита; сохранённые ветки доступны в её Git-репозитории.
Незавершённый libsquashfs-прототип и его license/LZO-RLE вопрос остаются
сохранённым исследованием, без разрешения на потерю поведения.

## Проверка

macOS: `python3 scripts/verify.py tooling --skip-runtime` — 6/6 PASS;
использован существующий `build/dev`, новые C++ targets не понадобились.
Проверены повторный setup памяти, выбор CTest/slow, incremental workflow,
runner sync/изоляция, runtime cache и buffered output. `doctor.sh`, запущенный
абсолютным путём из `/private/tmp`, PASS. Синтаксис всех проектных shell/Python
скриптов и JSON конфигурации разобран без исполнения внешних действий.
Валидатор skill-creator: все шесть проектных skills и глобальный `worktree` PASS;
PyYAML взят из уже установленного локального кэша, пакеты не устанавливались.

Проверены 301 Markdown-файл, 2983 локальные ссылки/якоря, 23 script entrypoints
в действующих руководствах и whitespace. Setup-шаблоны проверяются в целевом
расположении `.codex/`, а не относительно каталога assets.
Внешние HTTP-ссылки и исторические заявления о runtime заново не принимались.
Linux/Windows, full/slow suites и release acceptance не запускались: изменение
затрагивает документацию и platform-independent привязку пути в Bash tooling.

## Дополнительный аудит рабочих сценариев

По следующему запросу пользователя проверены не только ссылки, но и реальные
пути setup → поиск кода → выбор suites → configure/build/test → runner.
Сверены все шесть проектных skills вместе с `agents/openai.yaml`, 18 check wrappers,
их targets, CTest-регистрация, scenario matrix и UI capture catalog.

Исправлены обязательная полная сборка в навигационном skill, prompt regression
skill, старые утверждения о едином `current` и удаляемых runtime build trees,
ссылка на удалённую progress-suite и описание console parsing в `AGENTS.md`.
Карта документации теперь явно ведёт к offline tooling checks.

Release runner выполняет development gate и Release build без CTest для
отключённых Release tests. Windows reference builder использует owned snapshot,
проверяет reuse по входам и хешу executable, обновляет runtime sidecars,
сохраняет incremental outputs и отказывается перезаписывать чужой output.
Для эталона добавлен режим исходных makefiles `--original-only --no-build-rules`;
поведение обычных runtime/SFX snapshot не меняется.

Native Windows fixtures используют выбранный `SEVENZIP_WINDOWS_RUNNER_HOST`
вместо зашитого hostname. Build wrapper учитывает смену identity при configure;
CMake отслеживает появление reference manifest и изменение source baseline.
Исторический NLS assessment требует явного reference snapshot или reuse;
он не предлагает отсутствующие таблицы текущего checkout как исходный default.
Совместимый QEMU wrapper сохранён: его CLI доступен, использование остаётся
только по явному выбору этого режима, отдельно от Hyper-V.

Offline tooling после исправлений: macOS 7/7, Linux 7/7, Windows 8/8 PASS.
Последний общий remote receipt — `20261025T130342Z-db251043`;
Windows builder отдельно проверен в `20261025T125728Z-d88c51ef`.
Первые два Windows builder запуска обнаружили ошибки synthetic manifest
в новой fixture; они исправлены до успешной проверки. Проверено сохранение
output при повторе, обновление wrapper/runtime, сохранение object files,
неизменность исходного repository и отказ от перезаписи чужого output.
Новый workflow regression проверяет исчезновение native suite после смены
настроенного host, исключая использование устаревшей конфигурации.

На macOS дополнительно проверено, что baseline входит в RERUN_CMAKE dependency
graph; `tooling --plan` выбирает существующие script suites без C++ targets.
NLS assessment без explicit reference отказывает до создания output;
CLI старого QEMU wrapper доступен, виртуальные машины им не запускались.
Все семь применимых skills проходят валидатор, текущие Markdown-ссылки
и синтаксис shell/Python/JSON проверены. Full/slow/release gate не запускался.

Последующая адресная Windows-проверка выявила отсутствующую link closure
`ArchiveBackend` у двух native menu targets; они подключены к существующему
полному backend fixture-компоненту с общими production objects.
`.ps1` dispatch получил тот же process-scoped RemoteSigned, что и builders;
отдельная regression проверяет запуск при Restricted process default.
Ожидаемые lock errors теперь сообщают владельца без traceback/закодированного
скрипта, сохраняя отказ и саму блокировку; regression прошла локально и на Linux.

Настоящий Windows reference собран MSVC/NMake/ccache
(`20261025T131543Z-3cbaaea2`), повтор подтвердил reuse без compilation
(`20261025T131650Z-93ba3080`). Итоговый общий Windows batch
`20261025T133402Z-640461ac` — 4/4 PASS: `hyperv_runner`,
`native_menu_integration`, `native_shell_launch`, `native_rendered_system_menu`.
Это focused native workflow evidence, не full/release или pixel acceptance.
Три native suites дали 11 Qt checks, 0 failed/0 skipped. В offline `hyperv_runner`
на этом Limited desktop два prerequisite skip (symlink/rsync); изменённая lock
regression дополнительно прошла на Linux в `20261025T132413Z-dee08c35`.

Из тестовых presets удалён отключённый Release, `verify.py` отвергает его до
configure, а обычный пустой CTest запуск теперь возвращает ошибку вместо PASS.
Discovery через --show-only сохранён. Для этих платформонезависимых правок
локально прошли `ctest_selection`, `developer_workflow`, `hyperv_runner` — 3/3;
их отдельная кроссплатформенная приёмка не требовалась.

По уточнению пользователя правило batching относится к удалённым прогонам:
связанные известные follow-up правки проверяются вместе после готовности серии.
Локальные адресные тесты и incremental compilation разрешены по ходу работы.
Достаточность macOS для platform-independent изменений сохранена в AGENTS.md.
Политика отражена также в testing и regression skill.

Параллельная SMB-задача продвинула master до `e9c1f8d5`; её product/UI/translation
правки и собственные результаты не принадлежат этому аудиту. Активные runner locks
не снимались; два запуска аудита корректно отказались стартовать при чужом lock.

## Прежние основания long-suite лимитов

Перенесено из рабочего development guide без новой runtime-приёмки. Прежний
список описывался как suites с измеренным временем более 30 секунд на macOS;
текущие labels и timeouts определяются CMake, включая high-memory проверки.

Полная `interarchive_recovery` matrix превышала прежний cap 120 с при миграции
старых process hooks. Default subset `interarchive_recovery_native` покрывал
native reopen failure, cancellation и failed restore, отдельно от полной matrix.

Для `copy_archive_refresh` записаны 192 workflows, Cocoa fixture — 89 без row
failures до cap 120 с. Cap macOS был поднят до 540 с по двойной экстраполяции,
Windows сохранял измеренный cap 360 с. Это не меняло отдельные waits или
codec-discovery deadline. Новый полный replay трёх ОС был отклонён пользователем
2026-10-17. Эти числа и решение относятся к прежнему состоянию проверки.
