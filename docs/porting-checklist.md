# Чеклист портирования для последовательных чатов

План и scope: [porting-plan.md](porting-plan.md).
Готовый запрос агенту: [porting-chat-prompt.md](porting-chat-prompt.md).
Исходная дата: 2026-09-28. Это список **оставшейся** работы, не оценка MVP с нуля.

## Как выбрать и закрыть шаг

- [ ] Прочитать `AGENTS.md`, этот чеклист и нужный раздел плана.
- [ ] Проверить `git status`, current HEAD, indexed memory, audit/parity и код.
- [ ] Выбрать первый открытый доступный ID по P0 → P1 → P2 → P3 и зависимостям.
- [ ] Указать scope, upstream revision/symbols, owner и необходимые adapters.
- [ ] Если пакет велик, записать подшаги с результатами до начала реализации.
- [ ] Выполнить один шаг/подшаг, сохранив чужие незакоммиченные изменения.
- [ ] Перенести исходные bodies; не считать rename или behavioral rewrite source port.
- [ ] Закрыть UX того же scope: action/menu/shortcut, форму, focus, ошибки и Cancel.
- [ ] Добавить осмысленные regressions; операции проверять success/failure/cancel.
- [ ] Build affected targets, focused CTest и direct consumers.
- [ ] Для visual/text/resize changes проверить specification, normal/compact
  geometry и current-build JSON; resource source и platform limits записаны.
- [ ] Code/test/build changes: `./scripts/check.sh` согласно `AGENTS.md`.
  Docs-only: `git diff --check`. Старые checks не использовать как новые.
- [ ] Обновить audit (commit/paths/symbols/copied/adapted/omitted/dependencies)
  и parity; architecture/testing/README менять по фактическим изменениям claims.
- [ ] Записать результат в таблицу ниже, полезный handoff в `.codex/notes/`,
  сохранить индекс других задач и указать следующий доступный ID.
- [ ] По поручению пользователя от 2026-09-28 коммитить каждую завершённую
  итерацию отдельно после проверок. Stage только свои файлы/hunks, сохранить
  чужие изменения; записать local commit в evidence. Push не выполнять.

Эти checkboxes — повторяемый шаблон одного чата, не общий счётчик. Агент может
копировать его в запись своего подшага; они не становятся один раз «готовыми
навсегда». Не закрывать строку пакета до всех его подшагов и интеграции.

## Правила статусов и счётчика

`todo` — ещё не выполнен; `active` — текущая работа с owner/date;
`partial` — завершён подшаг, осталась обязательная часть;
`blocked` — указан конкретный prerequisite/ограничение;
`done` — выполнен scope и приложены доказательства;
`N/A` — только доказанная неприменимость платформенной/entrypoint ветки.
N/A отдельной ветки не закрывает весь пакет, содержащий применимые функции.

Ставить `[x]` только при `done`. В результате указать audit section, tests с
датой/ОС/result и применимую ревизию; unverified platforms перечислить явно.
Если код уже реализован другим чатом, сначала проверить bodies и current checks,
затем учесть результат, а не реализовывать повторно.

Текущий счёт: **1/44 завершённых пакетов; 43 остаются** (2026-09-28).
P0: 1/12; P1: 0/16; P2: 0/10; P3: 0/6. P0-01 закрыт как inventory,
обязательные функциональные порты не закрыты этим аудитом.
Подшаги не увеличивают число parent packages; новый самостоятельный scope
добавляется отдельной строкой и меняет знаменатель в обоих документах.
Это счётчик выполнения плана, не процент исходных строк upstream.

## P0 — критическая основа и ежедневный UX

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [x] | P0-01 | Реестр symbols, omissions, dependencies всех 109 UI `.cpp` и текущих зеркальных модулей | — | done — audit § P0-01.4; .1–.4 done; 2026-09-28 macOS docs-only diff/112 paths/33 vendor byte matches |
| [ ] | P0-02 | CApp/CPanel owners и независимое состояние панелей | P0-01 | active — текущий чат, 2026-09-28; подшаги ниже |
| [ ] | P0-03 | Единые command availability/dispatch для menus/toolbar/keys | P0-02 | todo |
| [ ] | P0-04 | Исходная навигация, адрес и parent-folder stack | P0-02, P0-03 | todo |
| [ ] | P0-05 | FSFolder enumeration/model/flat view | P0-02, P0-04 | todo |
| [ ] | P0-06 | Столбцы, сортировка, время и auto-refresh | P0-05 | todo |
| [ ] | P0-07 | Mouse/keyboard selection и полные list settings/hover | P0-03, P0-05 | todo |
| [ ] | P0-08 | Две панели, F9/Tab/Alt+F1/F2, persistence | P0-02, P0-03, P0-04, P0-07 | todo |
| [ ] | P0-09 | Исходные menus/toolbar/context/status и UI contracts | P0-03, P0-06, P0-08 | todo |
| [ ] | P0-10 | Общий progress/messages, cancel-confirm, pause/background | P0-02, P0-03 | todo |
| [ ] | P0-11 | OverwriteDialog и решения конфликта Copy/Move | P0-10 | todo |
| [ ] | P0-12 | Настоящий Ask при Extract через общий callback | P0-11 | todo |

## P1 — основные сценарии и полноценный UX

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [ ] | P1-01 | Routing F5/F6 между panels/disk/archive | P0-08, P0-11, P0-12, P1-04 | todo |
| [ ] | P1-02 | Cross-volume move и полные FSFolderCopy branches | P1-01 | todo |
| [ ] | P1-03 | Inline Rename/Create/Delete/корзина | P0-03, P0-10, P0-11 | todo |
| [ ] | P1-04 | Archive mutations, encrypted writes и comments по capabilities | P0-03, P0-10, P0-11, P1-03 | todo |
| [ ] | P1-05 | Update/path modes и полное Add workflow | P1-04 | todo |
| [ ] | P1-06 | Архивные тома create/open/test/extract | P1-05 | todo |
| [ ] | P1-07 | Advanced options/time/parameters/memory prompts | P1-05, P0-10 | todo |
| [ ] | P1-08 | External viewer/editor/diff и changed-temp update | P0-04, P1-04 | todo |
| [ ] | P1-09 | Обновление цепочки вложенных архивов | P1-04, P1-08 | todo |
| [ ] | P1-10 | Полные properties/raw values/formatting | P0-06, P1-04 | todo |
| [ ] | P1-11 | Root/drives/network/Browse | P0-04, P0-05 | todo |
| [ ] | P1-12 | 6-page Options container, Apply/Cancel, Settings/Folders | P0-07, P0-08, P1-07 | todo |
| [ ] | P1-13 | Работающие страницы Edit/Menu | P1-08, P1-12 | todo |
| [ ] | P1-14 | Language и все app captions/fallbacks | P0-09, P1-12 | todo |
| [ ] | P1-15 | F1/Help/About/Delete Temporary Files | P0-09, P1-08, P1-14 | todo |
| [ ] | P1-16 | Drag/drop между панелями и наружу из архива | P1-01, P1-09, P0-07 | todo |

## P2 — расширенные функции и платформенная интеграция

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [ ] | P2-01 | Formats/codecs/capabilities, KeepName, Inside */# | P0-04, P1-05 | todo |
| [ ] | P2-02 | SFX и внешние handlers/plugins | P2-01, P1-06 | todo |
| [ ] | P2-03 | Исходный CRC/Hash/Test workflow | P0-10, P1-10, P2-01 | todo |
| [ ] | P2-04 | Benchmark controls/results/lifecycle | P0-10, P1-14 | todo |
| [ ] | P2-05 | Links/junctions и FS metadata | P1-02, P1-03 | todo |
| [ ] | P2-06 | ADS/streams/CProxyArc2 по platform capabilities | P1-10, P2-05 | todo |
| [ ] | P2-07 | Extract ElimDup/absolute/security/metadata | P0-12, P2-05, P2-06 | todo |
| [ ] | P2-08 | System page/file associations | P1-12, P2-01 | todo |
| [ ] | P2-09 | Shell integration и оставшаяся Menu page | P1-13, P2-08 | todo |
| [ ] | P2-10 | Большие списки/операции без GUI blocking | P0-05, P0-06, P0-10, P1-09, P1-16 | todo |

## P3 — приёмка, обновления и поставка

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [ ] | P3-01 | Reproducible baseline и проверенная update procedure | P0-01; maintained throughout | todo |
| [ ] | P3-02 | Сквозная regression/security traceability | P0/P1; relevant P2 rows | todo |
| [ ] | P3-03 | CI и реальные Windows/macOS/Linux checks | можно начать рано; final после P2 | todo |
| [ ] | P3-04 | Engine/Qt/resources/packages/licenses | P2-01, P2-02, P1-14, P1-15, P3-03 | todo |
| [ ] | P3-05 | Полная сверка omitted scope/dependency closure | P0–P2, P3-01 | todo |
| [ ] | P3-06 | Итоговая source/workflow/UX/platform приёмка | все обязательные пакеты | todo |

## Подшаги, блокировки и handoff

### P0-01 — инвентаризация исходного переноса

Owner: текущий чат, 2026-09-28. Baseline upstream:
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; project HEAD:
`21b775c035eb1464fae725608307f2ddc5ad8685` + документы плана.
Это документационный аудит: поведение приложения не меняется, нужны
проверка исходных bodies/dependencies и `git diff --check`, не app build.

| Готов | Подшаг | Scope и критерий готовности | После | Статус |
| --- | --- | --- | --- | --- |
| [x] | P0-01.1 | 16 mirrored `.cpp`: App/state, Panel navigation/items/menu/keys/selection/sort/summary, ViewSettings, FS operations, Split workers/utils, Agent/Proxy, Wildcard/FileDir. Реестр copied/adapted/omitted symbols и immediate dependencies, review bodies | — | done — audit § P0-01.1; 2026-09-28 macOS docs-only, diff/check paths; runtime других ОС не проверен |
| [x] | P0-01.2 | Остальные 14 mirrored `.cpp`: dialogs/preferences/Compress/Extract/ZipRegistry/MethodProps, включая BrowseDialog/PanelListNotify; их headers, resources, vendored dependencies и Qt/std migration debt | P0-01.1 | done — audit § P0-01.2; 2026-09-28 macOS docs-only; 33 vendored byte matches, 30 paths coverage, diff check; runtime других ОС не проверен |
| [x] | P0-01.3 | Все 82 отсутствующих `.cpp`: inspect source scope/callers/build dependencies, назначить decision/owner без механического N/A | P0-01.2 | done — .3a–d; 2026-09-28 macOS docs-only diff/coverage, 112 unique paths |
| [x] | P0-01.4 | Проверить полное покрытие 109 UI + 3 supporting `.cpp`, согласовать audit/plan/owners/counts, записать checks и handoff | P0-01.3 | done — audit § P0-01.4; 2026-09-28 macOS docs-only diff/112 paths/33 vendor byte matches |

Коммиты: P0-01.1 — `cfd02a5` (16 modules audit), P0-01.2 — `d03de90`
(14 modules и vendor baseline). Подшаг P0-01.3 разделён до начала правок:

| Подшаг | Owner/scope и критерий готовности | После | Статус |
| --- | --- | --- | --- |
| P0-01.3a | Текущий чат: отсутствующие FM App/Panel/navigation/commands/progress/overwrite/callback modules; source bodies/callers, decision/owner/dependencies | P0-01.2 | done — audit § P0-01.3a; 2026-09-28 macOS docs-only, diff check, 12 missing / 42 unique paths; SFX legacy decision открыт |
| P0-01.3b | Текущий чат: остальные отсутствующие FM modules; source bodies/callers, platform/build scope, decision/owner/dependencies | P0-01.3a | done — b1–b3, 33 modules; полный FM inventory |
| P0-01.3c | Текущий чат: отсутствующие Agent/GUI modules; source bodies/callers и process adapters, decision/owner/dependencies | P0-01.3b | done — c1/c2, 13 modules, полный Agent/GUI inventory |
| P0-01.3d | Текущий чат: отсутствующие Common modules; source bodies/callers и dependency closure, decision/owner/dependencies, coverage всех 82 | P0-01.3c | done — d1–d4, 24 modules; полный Common inventory |

Каждый подшаг проверяется docs-only (`git diff --check`, unique-path coverage)
и коммитится отдельно. Родитель P0-01.3 закрывается только после 3d.

P0-01.3b разделён перед фиксацией audit:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3b1 | Текущий чат: 12 FM options/language/help/dialog/format modules; bodies/resources/callers, owners и adapter decisions | P0-01.3a | done — audit § P0-01.3b1; 2026-09-28 macOS docs-only, diff check, 54 unique paths |
| P0-01.3b2 | Текущий чат: 12 FM provider/platform/support modules; bodies/build callers, owners и adapter decisions | P0-01.3b1 | done — audit § P0-01.3b2; 2026-09-28 macOS docs-only, diff check, 66 unique paths |
| P0-01.3b3 | Текущий чат: 9 FM open/drag/hash/Browse/system modules; bodies/callers, owners и adapter decisions; coverage 33 оставшихся FM | P0-01.3b2 | done — audit § P0-01.3b3; 2026-09-28 macOS docs-only, diff check, 75 unique paths, полный FM inventory |

Local commits: P0-01.3a `9eced86`, b1 `ca5030a`, b2 `6ce4cac`.
P0-01.3b завершён после b3; docs-only diff/coverage проверки и коммиты
выполнены отдельно. На тот момент следующий scope был P0-01.3c (теперь done).

P0-01.3c разделён перед audit:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3c1 | Текущий чат: пять отсутствующих Agent modules, bodies/callers/interface closure, owners и backend adapters | P0-01.3b3 | done — audit § P0-01.3c1; 2026-09-28 macOS docs-only, diff check, 80 unique paths / полный Agent inventory |
| P0-01.3c2 | Текущий чат: восемь отсутствующих GUI modules, bodies/resources/build entrypoints, owners и process adapters | P0-01.3c1 | done — audit § P0-01.3c2; 2026-09-28 macOS docs-only, diff check, 88 unique paths / полный GUI inventory |

Каждый — docs-only diff/unique-path checks и отдельный commit; parent c
закрыт после c2. P0-01.3b3 commit: `3e914ce`, c1: `f6e1411`.

P0-01.3d разделён перед audit на самостоятельные source scopes:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3d1 | Текущий чат: ArchiveName/DefaultName/ExtractingFilePath/SetProperties/SortUtils/TempFiles/UpdateAction/UpdatePair/UpdateProduce/WorkDir; bodies/callers, owners, dependencies | P0-01.3c2 | done — audit § P0-01.3d1; 2026-09-28 macOS docs-only, diff check, 98 unique paths |
| P0-01.3d2 | Текущий чат: ArchiveCommandLine/LoadCodecs/OpenArchive/ArchiveOpenCallback/PropIDUtils; parser/open/capability source scope и adapters | P0-01.3d1 | done — d2a/b/c, пять modules; 103 unique paths |
| P0-01.3d3 | Текущий чат: EnumDirItems/Extract/ArchiveExtractCallback/Update/UpdateCallback; operations и callback policy versus engine boundary | P0-01.3d2 | done — d3a–d, 108 unique paths |
| P0-01.3d4 | Текущий чат: Bench/HashCalc/CompressCall/CompressCall2; engine integration/dependency scope и полный inventory coverage | P0-01.3d3 | done — d4a–c, 112 unique paths |

Все 24 missing Common modules обязательны к audit. Каждый подшаг требует
diff/unique-path checks и отдельного commit; d закрывается после d4.
P0-01.3c2 commit: `0c70e3b`.

P0-01.3d2 разделён до audit крупных modules:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3d2a | Текущий чат: ArchiveOpenCallback/PropIDUtils, callbacks/volume lifetime и property/security/reparse presentation bodies, owners/adapters | P0-01.3d1 | done — audit § P0-01.3d2a; 2026-09-28 macOS docs-only, diff check, 100 unique paths |
| P0-01.3d2b | Текущий чат: ArchiveCommandLine, parser phases/options/censor/update command scope | P0-01.3d2a | done — audit § P0-01.3d2b; 2026-09-28 macOS docs-only, diff check, 101 unique paths |
| P0-01.3d2c | Текущий чат: LoadCodecs/OpenArchive, capabilities/format selection/nesting/reopen source scope | P0-01.3d2b | done — c1/c2, оба modules |

Каждый подшаг — отдельный docs-only проверенный commit. Parent d2 закрыт
после d2c. P0-01.3d1 commit: `9aad76e`.
P0-01.3d2a commit: `6d0d8a9`.

P0-01.3d2c разделён перед audit тела OpenArchive (3690 lines):

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3d2c1 | Текущий чат: LoadCodecs bodies/capability interfaces/consumers и engine-library boundary | P0-01.3d2b | done — audit § P0-01.3d2c1; 2026-09-28 macOS docs-only, diff check, 102 unique paths |
| P0-01.3d2c2 | Текущий чат: OpenArchive bodies/item paths/format selection/nesting/reopen и source-to-backend scope | P0-01.3d2c1 | done — audit § P0-01.3d2c2; 2026-09-28 macOS docs-only, diff check, 103 unique paths |

Parent c/d2 закрываются после c2. Каждый substep проверяется и коммитится
отдельно. P0-01.3d2b commit: `6ab0a55`.
P0-01.3d2c1 commit: `4546ae9`. D2 закрыт после c2; следующие обязательные
modules — d3 operations/callbacks, затем d4 bench/hash/calls.

P0-01.3d3 разделён до review operations bodies:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3d3a | Текущий чат: EnumDirItems/Extract, scan/censor/output-dir/operation orchestration scope | P0-01.3d2c2 | done — audit § P0-01.3d3a; 2026-09-28 macOS docs-only, diff check, 105 unique paths |
| P0-01.3d3b | Текущий чат: ArchiveExtractCallback, filesystem/overwrite/link/security/progress source scope | P0-01.3d3a | done — audit § P0-01.3d3b; 2026-09-28 macOS docs-only, diff check, 106 unique paths |
| P0-01.3d3c | Текущий чат: Update, transaction/rename/scan/volume/SFX/delete source scope | P0-01.3d3b | done — audit § P0-01.3d3c; 2026-09-28 macOS docs-only, diff check, 107 unique paths |
| P0-01.3d3d | Текущий чат: UpdateCallback, streams/properties/update/error/cancel source scope | P0-01.3d3c | done — audit § P0-01.3d3d; 2026-09-28 macOS docs-only, diff check, 108 unique paths |

Каждый substep — docs-only diff/unique-path checks и отдельный commit.
P0-01.3d2c2 commit: `682128d`.
P0-01.3d3a commit: `9289748`.
P0-01.3d3b commit: `1a5f850`.
P0-01.3d3c commit: `5063a41`.

P0-01.3d4 разделён перед review engine integration:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3d4a | Текущий чат: Bench, benchmark orchestration versus official engine primitives, dependencies/adapters | P0-01.3d3d | done — audit § P0-01.3d4a; 2026-09-28 macOS docs-only, diff check, 109 unique paths |
| P0-01.3d4b | Текущий чат: HashCalc, hash orchestration/format/parser versus engine, dependencies/adapters | P0-01.3d4a | done — audit § P0-01.3d4b; 2026-09-28 macOS docs-only, diff check, 110 unique paths |
| P0-01.3d4c | Текущий чат: CompressCall/CompressCall2, engine calls и final unique-path coverage | P0-01.3d4b | done — audit § P0-01.3d4c; 2026-09-28 macOS docs-only diff/coverage, 112 unique paths |

Каждый substep требует source-body review, docs-only checks и отдельный commit.
P0-01.3d3d commit: `153e222`.
P0-01.3d4a commit: `8a1f3e2`.
P0-01.3d4b commit: `b35a90a`.
P0-01.3d4c commit: `415513c`.

Следующий доступный пакет: **P0-02**. Source inventory .1–.4 завершён;
P0-01 done. Inventory не закрывает обязательные функциональные порты P0–P3.

### P0-02 — owners и panel state

Owner: текущий чат, 2026-09-28. Upstream baseline как P0-01.
Parent остаётся open до source bodies, интеграции и consumer checks всех частей.

| Подшаг | Результат и критерии | После | Статус |
| --- | --- | --- | --- |
| P0-02.1 | CAppState из AppState.h: общие history/favorites owner, Read/Save, window integration; независимость instances, persistence, history/bookmark consumer tests | P0-01 | done — audit § P0-02.1; 2026-09-28 macOS build dev, focused 5/5 и check.sh 54/54 |
| P0-02.2 | CApp/CPanel lifecycle, location и panel focus/selected state; один panel через owners, separate instances для P0-08; source bodies + navigation/selection consumer checks | P0-02.1 | active |
| P0-02.3 | Operation start/error/cancel/close state ownership через panel/application adapters; сквозные consumer checks, architecture/audit/parity, parent acceptance | P0-02.2 | todo |

P0-01.4 commit: `36fad59`. P0-02.1 project baseline `36fad59`, exact source
revision и copied/adapted scope в audit. Checks: `./scripts/build.sh dev`,
`ctest --preset dev -R '^(app_state|folder_history|bookmarks|ui_navigation|ui_state)$' --output-on-failure`
(5/5 после завершения build), `./scripts/check.sh` (format/build/54 CTest passed),
`git diff --check`; 2026-09-28 macOS. Первый преждевременный focused запуск
не нашёл ещё не слинкованный app_state executable; повторный запуск после build
прошёл. Visual contracts unchanged, geometry inspection не требуется.
Windows/Linux не проверены; parent P0-02 остаётся open.

P0-02.1 commit: `eedf926`. P0-02.2 разделён перед изменением owners:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2a | CApp owns CAppState и два independent CPanel instances; focused-panel/list-versus-address state, source focus restoration и Qt event adapters; одна видимая панель, все четыре modes и независимость focus проверены | P0-02.1 | done — audit § P0-02.2a; macOS focused 3/3, check.sh 55/55 |
| P0-02.2b | CPanel location/parent stack и selected-state bodies через narrow folder/list/backend adapters; navigation/refresh selection consumers, без duplicate state | P0-02.2a | active — b1–b3 ниже |
| P0-02.2c | Create/CreateOnePanel и lifecycle/teardown ordering source scope, all-panel disable-before-release; current single-panel integration и checks | P0-02.2b | todo |

P0-02.2 parent закрывается только после a–c. Source baseline остаётся exact
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17; audit фиксируется с каждой частью.

P0-02.2a checks 2026-09-28 macOS: affected targets build dev;
`ctest --preset dev -R '^(panel_focus|panel_key|bookmarks)$' --output-on-failure`
3/3; `./scripts/check.sh` format/build/55 CTest; `git diff --check`.
Construction-order и editable-combo focus failures исправлены до passing runs.
Layout/text unchanged; Windows/Linux runtime не проверены.

P0-02.2a commit: `5bc96ad`. P0-02.2b разделён до изменения selection:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b1 | CSelectedState, SaveSelectedState/GetSelectedNames/Get_ItemIndices_Selected и selection/focus portion RefreshListCtrl; Qt list binding, filesystem refresh normal/flat consumers, sorted names и deleted focus fallback | P0-02.2a | done — audit § P0-02.2b1; focused 5/5, check.sh 56/56 |
| P0-02.2b2 | Archive refresh/flat-mode/edit result consumers используют тот же selected-state owner; failure/cancel checks | P0-02.2b1 | done — audit § P0-02.2b2; focused 5/5, check.sh 56/56 |
| P0-02.2b3 | CPanel location/parent stack source bodies, nested archive/navigation adapters и checks; удалить дублирующее location state | P0-02.2b2 | active — b3a–b3d ниже |

P0-02.2b1 checks 2026-09-28 macOS: affected targets/direct consumers build dev;
`ctest --preset dev -R '^(panel_selected_state|panel_focus|panel_copy_info|ui_navigation|panel_selection)$' --output-on-failure`
5/5; `./scripts/check.sh` format/build/56 CTest; `git diff --check`.
Первый новый fixture сравнивал /var с canonical /private/var; исправлен до
passing runs. Layout/text unchanged; Windows/Linux runtime не проверены.

P0-02.2b1 commit: `c412350`. P0-02.2b2 использует уже перенесённые source bodies;
reference consumers: Panel.cpp ChangeFlatMode, PanelItemOpen.cpp
OnOpenItemChanged SaveSelectedState/RefreshListCtrl bracket. Полные эти modules
и flat/location ownership остаются обязательными за пределами этого consumer scope.

P0-02.2b2 checks 2026-09-28 macOS: affected targets build dev;
`ctest --preset dev -R '^(panel_selected_state|ui_state|ui_archive_states|flat_view|file_viewer)$' --output-on-failure`
5/5; `./scripts/check.sh` format/build/56 CTest; `git diff --check`.
Layout/text unchanged; Windows/Linux runtime не проверены.

P0-02.2b2 commit: `e7870e5`. P0-02.2b3 разделён перед переносом folder contract.
Owner всех частей: текущий чат; exact upstream baseline не меняется.

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3a | Unchanged IFolder/IStream/IProgress declarations, MyCom/MyUnknown/MyInitGuid и PropVariant; partial ClassDefs GUID owner; compile/runtime checks interface identity, refcount lifetime и Unicode property ownership | P0-02.2b2 | done — audit § P0-02.2b3a; focused 2/2, check.sh 57/57, vendor bytes 42/42 |
| P0-02.2b3b | Narrow folder adapters для Qt/backend snapshot, CPanel folder binding и path/property source bodies; filesystem/archive location ownership и regression consumers | P0-02.2b3a | active — b1–b3 ниже |
| P0-02.2b3c | CTempFileInfo/CFolderLink и root/nested parent stack, source close/pop/cleanup ordering; cancellation/failure/temp-lifetime checks | P0-02.2b3b | todo |
| P0-02.2b3d | BindToPath/LoadFullPath/parent navigation source scope и flat-mode owner, window integration без duplicate state, all location consumers; b3 acceptance | P0-02.2b3c | todo |

P0-02.2b3a checks 2026-09-28 macOS: affected app/dependency/consumer targets build
dev; `ctest --preset dev -R '^(folder_contract|app_state)$' --output-on-failure`
2/2; `./scripts/check.sh` format/build/57 CTest; exact-source byte comparison
42/42 (41 headers/sources + license). Первый link обнаружил
sorting include до GUID initialization; порядок исправлен до passing runs.
Staged diff-check затем обнаружил исходные upstream trailing spaces; premature
local commit отменён soft-reset без потери изменений. .gitattributes сохраняет
точные vendor bytes, исключая vendored h/cpp/c из whitespace errors; local source/
tests/docs проверяются. Финальный full gate со staged files прошёл formatting,
обе diff-check, build и 57/57 CTest на macOS 2026-09-29 (local date).
Visual contracts unchanged; Windows/Linux runtime не проверены.

P0-02.2b3a commit: `dfee6fd`. P0-02.2b3b разделён перед сменой folder owner:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3b1 | Qt metadata snapshot implements unchanged IFolderFolder; CPanel uses CMyComPtr folder, source SetNewFolder/ReleaseFolder/property/name/path helpers, existing summary/property consumers preserved | P0-02.2b3a | done — audit § P0-02.2b3b1; focused 5/5, check.sh 58/58, vendor bytes 44/44 |
| P0-02.2b3b2 | Filesystem/Agent location adapters и source bind/path scope на portable folder contract; local deterministic binding/error checks | P0-02.2b3b1 | done — a–c ниже; remaining engine/optional contracts retain mandatory owners |
| P0-02.2b3b3 | Window runtime connects folder owners and preserves source real indices/loading/failure/cancel; duplicate location fields removed by mandatory BindToPath/LoadFullPath P0-02.2b3d after parent stack | P0-02.2b3b2 | active — b3a–b3c ниже |

P0-02.2b3b1 checks 2026-09-29 macOS: affected targets/direct consumers build dev;
`ctest --preset dev -R '^(folder_contract|panel_folder|panel_copy_info|properties|panel_selected_state)$' --output-on-failure`
5/5; `./scripts/check.sh` со staged files — format, обе diff-check, build,
58/58 CTest; exact-source bytes 44/44. Visual contracts unchanged;
Windows/Linux runtime не проверены.

P0-02.2b3b1 commit: `07b7326`. P0-02.2b3b2 разделён до изменения Agent:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3b2a | CAgentFolder IFolderFolder identity, _proxy bind/name/prefix/folder-property source bodies и typed property delegation; normal/flat/root/parent/name/error tests, legacy enumeration preserved | P0-02.2b3b1 | done — audit § P0-02.2b3b2a; focused 4/4, check.sh 59/59, vendor bytes 45/45; macOS 2026-09-29 |
| P0-02.2b3b2b | CFSFolder folder identity/path/bind/property source scope через portable filesystem services; root/Unicode/failure tests, existing mutations preserved | P0-02.2b3b2a | done — b1–b3 ниже; optional property/operations scopes remain mandatory in their owners |
| P0-02.2b3b2c | Owned archive metadata/provider context вместо borrowed proxy при долгом binding; location factory, source remaining required scope и b2 acceptance | P0-02.2b3b2b | done — c1–c2 ниже; remaining engine/interface scope explicitly assigned P0-03/04 and feature owners |

_proxy2/alt streams, raw/archive handler properties, COM lifecycle/operations
не становятся done от одного _proxy binding subset. Оставшиеся обязательные
ветки фиксируются audit и в owning feature steps.

P0-02.2b3b2a commit: `49f7975`. Filesystem scope разделён до переноса:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3b2b1 | Portable FileFind/TimeUtils dependencies, FileName source path bodies и FileDir GetCurrentDir; deterministic Unicode, dots, missing-path, symlink enumeration checks | P0-02.2b3b2a | done — audit § P0-02.2b3b2b1; focused 3/3, check.sh 60/60, vendor bytes 53/53; macOS 2026-09-29 |
| P0-02.2b3b2b2 | CFSFolder source enumeration, identity и typed properties; preserve mutation consumers; normal/flat/Unicode/error checks | P0-02.2b3b2b1 | done — audit § P0-02.2b3b2b2; focused 4/4, check.sh 61/61, vendor bytes 55/55; macOS 2026-09-29 |
| P0-02.2b3b2b3 | Source folder binding/parent/root scope, portable root adaptation и b acceptance; Windows drive/network required scope explicit | P0-02.2b3b2b2 | done — audit § P0-02.2b3b2b3; focused 4/4, check.sh 61/61, vendor bytes 57/57; macOS 2026-09-29; Windows/Linux runtime unverified |

P0-02.2b3b2b3 commit: `0297571`. Owned archive scope разделён до изменения:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3b2c1 | Source CAgent proxy ctor/delete/ReadItems/root-bind subset; CAgentFolder retains source agent reference in children/parents, typed metadata adapter; input/root/agent release lifetime regressions | P0-02.2b3b2b | done — audit § P0-02.2b3b2c1; focused 4/4, check.sh 61/61; macOS 2026-09-29 |
| P0-02.2b3b2c2 | Filesystem/archive location factories use owned contract, typed provider snapshots и b2 acceptance; source GetSize/CalculateSizes unknown-metadata handling; required remaining engine/interface branches retain owning IDs | P0-02.2b3b2c1 | done — audit § P0-02.2b3b2c2; focused 6/6, check.sh 62/62; macOS 2026-09-29 |

При следующем разделении добавить здесь, сохраняя остальных:

P0-02.2b3b2c2 commit: `57804df`. Runtime scope разделён до изменения:
outer archive/full path ownership требует следующего CFolderLink stack, поэтому
удаление duplicate location fields остаётся обязательным P0-02.2b3d, не N/A.

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3b3a | Source CPanel GetRealItemIndex/GetItemParam mapping; selection vector uses source indices; sorted/flat rows retain independent focus/selection and correct property consumers | P0-02.2b3b2 | done — audit § P0-02.2b3b3a; focused 7/7, ASan 2/2, check.sh 62/62; macOS 2026-09-29; teardown gate failure fixed with before-fix regression |
| P0-02.2b3b3b | Runtime folder loading/binding for filesystem/archive, source owner indices across timer/batch/flat loading; retain responsiveness and cancellation/error behavior | P0-02.2b3b3a | done — b1–b3 below; complete source columns/notify/path/interface branches remain mandatory in their explicit owners |
| P0-02.2b3b3c | All location/navigation/mutation consumers and binding teardown acceptance; document temporary fields with mandatory removal in P0-02.2b3d | P0-02.2b3b3b | active — c1–c3 below |

P0-02.2b3b3a gate выявил SIGTRAP при teardown активного 7zz listing.
До исправления scope дополнен узкой Qt integration prerequisite:
ArchiveBackend останавливает callbacks/process до разрушения полей; regression
удаляет backend при активном listing и незавершённом создании, проверяет отсутствие
completion callbacks и очистку временного архива. Это не закрывает обязательный
полный source operation/lifecycle scope P0-02.3.

P0-02.2b3b3a commit: `190abef`. Runtime loading разделён до изменения:

| Подшаг | Owner, scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3b3b1 | MainWindow archive listing/navigation owns CAgent root; CPanel binds owned CAgentFolder and renders matching source indices; one proxy implementation/instance, retained nested parent context; refresh/error/cancel/flat regressions | P0-02.2b3b3a | done — audit § P0-02.2b3b3b1; focused 9/9, check.sh 62/62; 16 normal/compact JSON reports inspected; macOS 2026-09-29 |
| P0-02.2b3b3b2 | FilesystemScanner/CFSFolder source enumeration and source-index publication; responsive normal/flat loading, cancellation/stale-generation/error handling and owned GUI handoff | P0-02.2b3b3b1 | done — b2a–b2c ниже; full source loading/parent stack scope remains explicit |
| P0-02.2b3b3b3 | CPanel runtime loading acceptance and direct consumers; source indices/property owners across filesystem/archive transitions and four views | P0-02.2b3b3b2 | done — audit § P0-02.2b3b3b3; focused 10/10, check.sh 64/64; four-view/normal-flat transitions and immutable source metadata; macOS 2026-09-29; full consumer migration remains P0-02.2b3b3c |

Полный BindToPath/LoadFullPath, outer+nested CFolderLink, optional interfaces,
engine property/operation scope остаются обязательными исходными owners; readonly
archive runtime здесь не считается полным переносом этих функций.

P0-02.2b3b3b1 commit: `9014788`. Filesystem loading разделён до изменения:

| Подшаг | Owner, scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-02.2b3b3b2a | CFSFolder LoadItems/LoadSubItems preserves source enumeration order/body, narrow progress/cancel callbacks; deterministic cancellation before enumeration/during flat recursion, no partial stale records on failure/cancel, successful reload | P0-02.2b3b3b1 | done — audit § P0-02.2b3b3b2a; focused 4/4, check.sh 62/62; macOS 2026-09-29 |
| P0-02.2b3b3b2b | FilesystemScanner loads source folder off GUI thread and transfers ownership after completion; native enumeration once, progress/cancel/generation/teardown regressions; no concurrent non-atomic COM ownership | P0-02.2b3b3b2a | done — audit § P0-02.2b3b3b2b; focused 5/5, ASan 1/1, check.sh 63/63; four normal/compact progress JSON reports inspected; macOS 2026-09-29 |
| P0-02.2b3b3b2c | MainWindow normal/flat source folder binding and source-index batched presentation; eliminate QDir/QDirIterator enumeration, retain cancellation/navigation responsiveness and direct consumers | P0-02.2b3b3b2b | done — audit § P0-02.2b3b3b2c; focused 11/11, check.sh 63/63; four normal/compact runtime JSON reports inspected; macOS 2026-09-29; Windows reparse acceptance remains required |

P0-02.2b3b3b runtime acceptance commit: `fa109fd`. Consumer migration split before
implementation; engine/raw/archive-level property branches remain their mandatory
P0-03/04/feature scopes, not N/A.

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c1 | Copy/Move summary uses source CPanel operated indices and bound folder properties, no rebuilt row metadata snapshot; source paths/names/totals and missing/zero sizes in normal/flat/four views; Qt display prefix remains explicit until full source path stack | P0-02.2b3b3b | done — audit § P0-02.2b3b3c1; focused 8/8, check.sh 64/64, 16 normal/compact source summary JSON reports inspected; macOS 2026-09-29 |
| P0-02.2b3b3c2 | Properties consumes source folder catalogs/typed values and operated indices; copy original formatting dependencies and preserve Qt dialog/geometry; remove window metadata reconstruction | P0-02.2b3b3c1 | active — c2a–c2c below |
| P0-02.2b3b3c3 | Remaining location/navigation/mutation consumers and teardown acceptance; verify source index ownership and temporary context fields before source parent/path stack migration | P0-02.2b3b3c2 | todo |

P0-02.2b3b3c1 commit: `6bd328b`. Properties migration is split before code:

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c2a | Compile unchanged portable PropVariantConv/PropIDUtils and required FileLink dependencies; typed strings/numeric/bool/time/attributes/reparse/invalid-input formatting regressions; no duplicate formatter | P0-02.2b3b3c1 | done — audit § P0-02.2b3b3c2a; focused 10/10, ASan 1/1, check.sh 65/65, vendor bytes 61/61; macOS 2026-09-29; Windows native link operation dependencies remain mandatory P2-05/P3-05 |
| P0-02.2b3b3c2b | Source property-name/PanelMenu formatting branches and required folder-property gaps; narrow Qt resource and platform services; catalog/unknown/error behavior tests | P0-02.2b3b3c2a | done — scoped formatter/cache b1–b3 verified; live archive catalog and complete Properties integration remain c2c |
| P0-02.2b3b3c2c | Live Properties binding/operated indices, remove window metadata snapshots, normal/flat/four-view size/metadata/geometry acceptance and existing direct consumers | P0-02.2b3b3c2b | active — a–c below |

P0-02.2b3b3c2a commit: `47f562b`. Formatter integration prerequisites split before
code; no missing required source property is treated as N/A.

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c2b1 | CFSFolder source cached PackSize branch and native Windows compressed-size helper; source fallback on unsupported POSIX API, typed/cached/normal-flat/catalog and existing filesystem consumers | P0-02.2b3b3c2a | done — audit § P0-02.2b3b3c2b1; focused 5/5, check.sh 65/65; macOS 2026-09-29; native allocation metadata remains mandatory |
| P0-02.2b3b3c2b2 | Original property-name/resource lookup and error/size formatting dependencies; narrow Qt resource adapter, source resource IDs, unknown/custom fallback and localized error checks | P0-02.2b3b3c2b1 | done — a–c verified below; formatter consumer remains b3 |
| P0-02.2b3b3c2b3 | Original PanelMenu typed formatting/item property branches, catalog/unknown/error/aggregate tests, preserve scoped raw/archive-level mandatory omissions | P0-02.2b3b3c2b2 | done — audit § P0-02.2b3b3c2b3; focused 7/7, check.sh 70/70, two normal/compact typed JSON reports reviewed; macOS 2026-09-29; temporary legacy snapshot packed-size visibility remains mandatory c2c |

P0-02.2b3b3c2b1 commit: `d41d64c`. Dependency owners split before code:

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c2b2a | PanelListNotify original IsSizeProp classification; numeric size/count/offset versus time/checksum/id formatting contracts | P0-02.2b3b3c2b1 | done — audit § P0-02.2b3b3c2b2a; focused 5/5, check.sh 66/66; macOS 2026-09-29 |
| P0-02.2b3b3c2b2b | PropertyName original name lookup and Qt resource boundary, source caption IDs; known/custom/unknown fallback tests; full language loading stays mandatory P1-14 | P0-02.2b3b3c2b2a | done — audit § P0-02.2b3b3c2b2b; focused 5/5, generator 1/1, check.sh 68/68, vendor bytes 63/63; macOS 2026-09-29 |
| P0-02.2b3b3c2b2c | OpenArchive GetOpenArcErrorFlags and ExtractCallback GetOpenArcErrorMessage source branches; typed/undefined/invalid/combined/encrypted/unknown flags tests | P0-02.2b3b3c2b2b | done — audit § P0-02.2b3b3c2b2c; focused 5/5, generator 1/1, check.sh 69/69, vendor bytes 65/65; macOS 2026-09-29 |

P0-02.2b3b3c2b3 commit: `a7338e0`. Live Properties prerequisites split before code:

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c2ca | Typed archive transport property catalog and original CAgentFolder catalog expansion/remapping; normal/flat/property presence/error checks, full engine metadata remains P0-03/P0-04 | P0-02.2b3b3c2b | active — a1/a2 below |
| P0-02.2b3b3c2cb | PropertiesDialog/MainWindow consume operated indices/live CPanel owner, remove snapshot metadata reconstruction, restore source packed aggregate visibility; focused source metadata and layout checks | P0-02.2b3b3c2ca | todo |
| P0-02.2b3b3c2cc | Filesystem/archive normal/flat/four-view runtime acceptance for source index/name/counts, immutable/absent/zero metadata, transitions and normal/compact Properties geometry | P0-02.2b3b3c2cb | todo |

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c2ca1 | QtPanelFolder 7zz typed Path/Size/PackSize transport catalog, preserve legacy custom-property callers; source types/names/Unicode/zero/absence/invalid-index checks | P0-02.2b3b3c2b | done — audit § P0-02.2b3b3c2ca1; focused 7/7, check.sh 71/71; macOS 2026-09-29 |
| P0-02.2b3b3c2ca2 | Original CAgent ReadItems ThereIsPathProp scan and CAgentFolder GetNumberOfProperties/GetPropertyInfo plus kProps; transport Path→Name, optional Name/counts/flat Prefix and failure scope | P0-02.2b3b3c2ca1 | todo |

```text
Parent: P?-?? — partial
Подшаг: P?-??.1 — todo/active/done
Scope и пользовательский результат:
После:
Owner/chat/date (если известен):
Upstream revision/paths/symbols:
Copied/adapted/omitted, причины:
Изменённые файлы:
Checks: команды, дата, OS, revision, результат:
Audit/parity links:
Remaining / platform unverified:
Следующий доступный ID:
```

Blocked запись обязана назвать недостающую capability, зависимость или решение.
Сначала выполнить независимую доступную работу; ограничения не превращать в
done/N/A автоматически. Новая ревизия upstream требует отдельного old→new review.
