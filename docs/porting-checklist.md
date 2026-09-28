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

Начальный счёт: **0/44 завершённых пакетов; 44 остаются**.
P0: 0/12; P1: 0/16; P2: 0/10; P3: 0/6.
Подшаги не увеличивают число parent packages; новый самостоятельный scope
добавляется отдельной строкой и меняет знаменатель в обоих документах.
Это счётчик выполнения плана, не процент исходных строк upstream.

## P0 — критическая основа и ежедневный UX

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [ ] | P0-01 | Реестр symbols, omissions, dependencies всех 109 UI `.cpp` и текущих зеркальных модулей | — | partial — P0-01.1–.2 done; P0-01.3–.4 открыты |
| [ ] | P0-02 | CApp/CPanel owners и независимое состояние панелей | P0-01 | todo |
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
| [ ] | P0-01.3 | Все 82 отсутствующих `.cpp`: inspect source scope/callers/build dependencies, назначить decision/owner без механического N/A | P0-01.2 | active |
| [ ] | P0-01.4 | Проверить полное покрытие 109 UI + 3 supporting `.cpp`, согласовать audit/plan/owners/counts, записать checks и handoff | P0-01.3 | todo |

Коммиты: P0-01.1 — `cfd02a5` (16 modules audit), P0-01.2 — `d03de90`
(14 modules и vendor baseline). Подшаг P0-01.3 разделён до начала правок:

| Подшаг | Owner/scope и критерий готовности | После | Статус |
| --- | --- | --- | --- |
| P0-01.3a | Текущий чат: отсутствующие FM App/Panel/navigation/commands/progress/overwrite/callback modules; source bodies/callers, decision/owner/dependencies | P0-01.2 | done — audit § P0-01.3a; 2026-09-28 macOS docs-only, diff check, 12 missing / 42 unique paths; SFX legacy decision открыт |
| P0-01.3b | Текущий чат: остальные отсутствующие FM modules; source bodies/callers, platform/build scope, decision/owner/dependencies | P0-01.3a | done — b1–b3, 33 modules; полный FM inventory |
| P0-01.3c | Текущий чат: отсутствующие Agent/GUI modules; source bodies/callers и process adapters, decision/owner/dependencies | P0-01.3b | done — c1/c2, 13 modules, полный Agent/GUI inventory |
| P0-01.3d | Текущий чат: отсутствующие Common modules; source bodies/callers и dependency closure, decision/owner/dependencies, coverage всех 82 | P0-01.3c | active |

Каждый подшаг проверяется docs-only (`git diff --check`, unique-path coverage)
и коммитится отдельно. Родитель P0-01.3 закрывается только после 3d.

P0-01.3b разделён перед фиксацией audit:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3b1 | Текущий чат: 12 FM options/language/help/dialog/format modules; bodies/resources/callers, owners и adapter decisions | P0-01.3a | done — audit § P0-01.3b1; 2026-09-28 macOS docs-only, diff check, 54 unique paths |
| P0-01.3b2 | Текущий чат: 12 FM provider/platform/support modules; bodies/build callers, owners и adapter decisions | P0-01.3b1 | done — audit § P0-01.3b2; 2026-09-28 macOS docs-only, diff check, 66 unique paths |
| P0-01.3b3 | Текущий чат: 9 FM open/drag/hash/Browse/system modules; bodies/callers, owners и adapter decisions; coverage 33 оставшихся FM | P0-01.3b2 | done — audit § P0-01.3b3; 2026-09-28 macOS docs-only, diff check, 75 unique paths, полный FM inventory |

Local commits: P0-01.3a `9eced86`, b1 `ca5030a`, b2 `6ce4cac`.
P0-01.3b завершён после b3; их docs-only diff/coverage проверки и коммиты
выполняются отдельно. Следующий доступный scope — P0-01.3c.

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
| P0-01.3d3 | Текущий чат: EnumDirItems/Extract/ArchiveExtractCallback/Update/UpdateCallback; operations и callback policy versus engine boundary | P0-01.3d2 | active |
| P0-01.3d4 | Текущий чат: Bench/HashCalc/CompressCall/CompressCall2; engine integration/dependency scope и полный inventory coverage | P0-01.3d3 | todo |

Все 24 missing Common modules обязательны к audit. Каждый подшаг требует
diff/unique-path checks и отдельного commit; d закрывается после d4.
P0-01.3c2 commit: `0c70e3b`.

P0-01.3d2 разделён до audit крупных modules:

| Подшаг | Scope и критерий | После | Статус |
| --- | --- | --- | --- |
| P0-01.3d2a | Текущий чат: ArchiveOpenCallback/PropIDUtils, callbacks/volume lifetime и property/security/reparse presentation bodies, owners/adapters | P0-01.3d1 | done — audit § P0-01.3d2a; 2026-09-28 macOS docs-only, diff check, 100 unique paths |
| P0-01.3d2b | Текущий чат: ArchiveCommandLine, parser phases/options/censor/update command scope | P0-01.3d2a | done — audit § P0-01.3d2b; 2026-09-28 macOS docs-only, diff check, 101 unique paths |
| P0-01.3d2c | Текущий чат: LoadCodecs/OpenArchive, capabilities/format selection/nesting/reopen source scope | P0-01.3d2b | done — c1/c2, оба modules |

Каждый подшаг — отдельный docs-only проверенный commit. Parent d2 остаётся
active до d2c. P0-01.3d1 commit: `9aad76e`.
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
| P0-01.3d3d | Текущий чат: UpdateCallback, streams/properties/update/error/cancel source scope | P0-01.3d3c | active |

Каждый substep — docs-only diff/unique-path checks и отдельный commit.
P0-01.3d2c2 commit: `682128d`.
P0-01.3d3a commit: `9289748`.
P0-01.3d3b commit: `1a5f850`.

Следующий текущий scope: **P0-01.3**. Parent остаётся открытым до всех четырёх
подшагов; наличие карты filenames в плане не заменяет review реализации.

При первом разделении добавить здесь, сохраняя остальных:

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
