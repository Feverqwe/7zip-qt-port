# Чеклист портирования для последовательных чатов

План и scope: [porting-plan.md](porting-plan.md).
Готовый запрос агенту: [porting-chat-prompt.md](porting-chat-prompt.md).
Исходная дата: 2026-09-28. Это список **оставшейся** работы, не оценка MVP с нуля.

## Как выбрать и закрыть шаг

- [ ] Прочитать `AGENTS.md`, этот чеклист и нужный раздел плана.
- [ ] Проверить `git status`, current HEAD, indexed memory, audit/parity и код.
- [ ] Выбрать первый открытый доступный ID по P0 → P1 → P2 → P3 и зависимостям.
- [ ] Указать scope, upstream revision/symbols, owner и необходимые adapters.
- [ ] Если пакет велик, записать несколько крупных интегрированных результатов
  с простыми ID и одним уровнем подшагов; helpers/dependencies — внутренние задачи.
- [ ] Выполнить целый компонент/сценарий вместе с consumers, адаптерами и UX,
  сохранив чужие незакоммиченные изменения. За чат можно завершить несколько итераций.
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
  крупную итерацию отдельно после проверок; размер уточнён 2026-09-29 в плане.
  Не коммитить каждый helper или запись промежуточной приёмки как отдельную
  итерацию. Stage только свои файлы/hunks, сохранить
  чужие изменения; записать local commit в evidence. Push не выполнять.

Эти checkboxes — повторяемый шаблон одного чата, не общий счётчик. Агент может
копировать его в запись своего подшага; они не становятся один раз «готовыми
навсегда». Не закрывать строку пакета до всех его подшагов и интеграции.

При перезапуске применять раздел «Размер итерации и коммита» плана. Старые
глубокие ID ниже сохраняются как evidence; новые уровни к ним не добавлять.
Для оставшегося scope записать крупные результаты с простыми ID и ссылками
на старые открытые строки, сохраняя их незавершённый статус до проверки результата.
Работу соседних пакетов учитывать под их ID, с точными prerequisite-связями;
не поглощать P0-04/P0-05/P0-10 цепочкой подшагов P0-02. Счётчик parent packages
менять только при выполнении их критериев готовности.

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

Текущий счёт: **4/44 завершённых пакетов; 40 остаётся** (2026-09-29).
P0: 4/12; P1: 0/16; P2: 0/10; P3: 0/6. P0-01 закрыт как inventory, P0-02 как owners/lifecycle,
P0-03 как integrated command policy/dispatch, P0-04 как native navigation;
обязательные функциональные порты не закрыты этим аудитом.
Подшаги не увеличивают число parent packages; новый самостоятельный scope
добавляется отдельной строкой и меняет знаменатель в обоих документах.
Это счётчик выполнения плана, не процент исходных строк upstream.

## Рабочие итерации после укрупнения (2026-09-29)

Применяются обновлённые правила [размера итерации](porting-plan.md#размер-итерации-и-коммита-уточнение-2026-09-29).
Исторические ID, статусы, audit, commits и evidence ниже сохранены. Старые
открытые строки, перечисленные в карте, **входят в новые результаты**, а не
закрываются сменой структуры. Их дочерние строки — внутренний список работ;
отдельных новых завершений или коммитов для helpers/методов/ресурсов не будет.
У всех текущих итераций owner — этот чат. После P0-06.4 счёт parent packages — 6/44.

| Рабочий ID | Целый результат и критерий приёмки | Точные prerequisites / исторический открытый scope | Статус |
| --- | --- | --- | --- |
| P0-10.1 | Подключённый общий source progress/callback component: CProgressDialog/CProgressThreadVirt, modal/finish/error/cancel-confirm, pause/background и реальные worker/process capabilities; необходимые UpdateCallbackGUI2/UpdateCallback100Imp bodies, первый реальный consumer через ArchiveBackend, замена его прежнего progress owner; success/failure/cancel/exception/prompt races, source text/normal/compact JSON и focused/full gate | Досрочный prerequisite для P0-04.1 changed-file update; принятые native folder/provider contracts, processing guards и sync/platform services из historical P0-02 достаточны, полные P0-02/P0-03 не требуются для этого компонента. Входит открытый P0-02.2b3c3b2b (включая b3b/b3c) и P0-02.2b3c3b2c; уже done helpers/services не переносятся повторно | done — audit § P0-10.1; integrated Add/drop, native callbacks/control/worker lifetime; focused 9/9 15.81 с, later affected archive checks 1.24/1.26 с; check.sh 101/101 72.05 с; 24 JSON states/26 controls; macOS arm64 2026-09-29, Windows/Linux runtime unverified; P0-10.2 и parent приёмка обязательны |
| P0-04.1 | Source navigation/parent-stack component: OpenAsArc, CFolderLink, CloseOneLevel/CloseOpenFolders, изменённые временные файлы и update prompt, CThreadCopyFrom/OnOpenItemChanged, BindToPath/LoadFullPath; current root/nested/address/parent consumers используют один native owner без Qt ArchiveParent/location дубликатов; password/failure/cancel/temp lifetime и UX проверены | Досрочный prerequisite для завершения P0-02 owners. После P0-10.1 и принятых typed providers/selection contracts. Входят открытые P0-02.2b3c/.2b3d, .2b3c3b2d/.2b3c3b3 и remaining navigation portions .2b3b3c3; progress/callback часть принадлежит P0-10.1 | done — audit § P0-04.1; root/nested/address/parent native consumers, changed nested success/failure/cancel, exact result/journal, selection/temp lifetime; check.sh 119/119 58.65 с; 16 normal/compact archive JSON; macOS arm64 2026-09-29. Known/editor/child-process P1-08, routing P0-03 и full P0-04 parent обязательны; Windows/Linux unverified |
| P0-04.2 | Цельный remaining root/address/parent scenario и parent P0-04 acceptance: native RootFolder rendering/entry, source command+shortcut, filesystem/archive/nested path routing and history/bookmarks, failure/password/cancel rollback, relative/root/Unicode/tar.gz checks, current normal/compact JSON, focused/full gate | После P0-04.1 и P0-03. Использует их принятый evidence; historical remaining P0-02.2b3c/.2b3d navigation scope закрывать только соответствующим результатом. Полные editor/known-process owners остаются P1-08, namespace network/ADS — P1-11 | done — audit § P0-04.2; root/Computer/Parent/Back/relative Unicode/stale busy + prior nested/password/cancel/tar.gz evidence; focused 8/8 13.45 с, check.sh 130/130 87.96 с; root JSON 980×620/640×400; macOS arm64 2026-09-29. Parent P0-04 принят; Windows/Linux runtime и rendered Windows unverified |
| P0-02.2 | Один полностью подключённый CApp/CPanel owner: Create/CreateOnePanel/ReleaseApp, independent panel state, focus/selection/location contracts, all-panel disable-before-release; MainWindow остаётся Qt presentation, teardown/refresh/navigation consumers проверены | После P0-04.1; existing P0-02.1/selected/provider evidence сохранены. Входит remaining own lifecycle P0-02.2c и owner-only portions .2b/.2b3/.2b3b; navigation/FS/progress код учитывается у соседних owners | done — audit § P0-02.2; native Create/CreateOnePanel/OnCreate/Save/ReleaseApp/ReleasePanel, per-panel focus/settings, startup success/failure/cancel и direct consumers; check.sh 121/121 85.30 с, 16 normal/compact JSON; macOS arm64 2026-09-29. P0-02.3/full parent, command/title/drop/model owners обязательны; Windows/Linux unverified |
| P0-02.3 | Application/panel operation-state component и полная P0-02 приёмка: start/error/cancel/window-close порядок, quiescence до освобождения owners, независимые экземпляры и сквозные consumer checks | После P0-02.2 и P0-10.1. Входит прежний P0-02.3 и remaining owner teardown/operation consumers; routing rules остаются P0-03 | done — audit § P0-02.3; native WM_CLOSE/operation guards/CWindowInfo, scanner/backend/file/volume quiescence; focused 8/8 7.54 с, check.sh 122/122 84.83 с macOS arm64 2026-09-29. Parent P0-02 принят; P0-03/04/05/08/10 и P1-08 omitted scopes обязательны, Windows/Linux unverified |
| P0-05 | Полный filesystem model/enumeration/flat component с текущими consumers, source property/filter/load bodies, scheduling/cancel/stale batches и большим deterministic fixture; принятые source enumeration/provider части используются как evidence | После P0-02/P0-04 по parent плану; ранее досрочно выполненные FS portions P0-02.2b3b2/.2b3b3b/.2b3b3c зафиксированы только в их фактическом scope. Их remaining FS части входят сюда, remaining navigation — P0-04.1 | done — P0-05.1/.2 и historical source evidence; 2048 Unicode flat fixture, stale cancel/navigation, 1500-row normal/compact UX, check.sh 130/130 85.77 с macOS 2026-09-29; P0-06/09/P2-10 scope отдельно, Windows/Linux unverified |
| P0-05.1 | Source filesystem read-model component с normal/flat consumers: исходные LoadItems/LoadSubItems, property/comment/link/folder-stat bodies, source item indices/filters, UX cells/actions и tests для hidden/empty/Unicode/links/inaccessible | После P0-04.2; принятые CFSFolder enumeration/typed-property portions использовать как evidence, не копировать заново. Сортировка/CompareItems и полная колонная модель остаются P0-06 | done — audit § P0-05.1; TextPairs/CFSFolder source bodies, source normal/flat names/prefixes, hidden/empty/unreadable/Unicode/links and stat cancel; focused 4/4 6.13 с, check.sh 130/130 88.79 с macOS arm64 2026-09-29; current 980/640 flat JSON reviewed. P0-05.2/parent mandatory; Windows/Linux runtime unverified |
| P0-05.2 | Большой flat-view сценарий с исходной моделью, narrow async scheduling/cancellation adapter, stale batch при переходе, ограничением symlink cycles, детерминированным large fixture и полной P0-05 приёмкой | После P0-05.1; текущие FilesystemScanner/FolderLoader сохраняются только как scheduling/Qt transport, functional enumeration — source CFSFolder | done — audit § P0-05.2; 32-row GUI delivery, 2048 Unicode files/2050 source rows, event-loop heartbeat, existing cancel/stale/symlink/1500-row UX evidence; focused 3/3 2.51 с, check.sh 130/130 85.77 с macOS arm64 2026-09-29; parent принят |
| P0-06.1 | Единая исходная сортировка filesystem/archive/root для всех четырёх видов: `PanelSort.cpp` typed property/variant policy, имя/префикс/index tie-break, folder-first, Qt row adapter, visible sort action/header и regression | После P0-05; использует принятые source index/property/selection portions; raw property sort с column catalog, time/auto-refresh остаётся .2/.3/.4 | done — audit § P0-06.1; Qt comparator заменён source property/index loop, prefix BSTR order, header/action sync; focused 4/4 5.64 с, check.sh 130/130 88.78 с, JSON 980×620/640×400 macOS arm64 2026-09-29; .2/.3/.4/parent остаются обязательными |
| P0-06.2 | Целый сценарий колонок: source property catalog/capabilities/raw sort, width/order/visibility/sort/view persistence, динамический context header menu, сохранение текущих consumers и нормальная/compact геометрия четырёх режимов | После P0-06.1; owner FM `PanelItems`/`ViewSettings`/`PropertyName`; не считать текущие три Qt toggle полной моделью | done — audit § P0-06.2; source catalog/raw compare and CListViewInfo, dynamic Qt header, per-type layout/sort, four-mode 980/640 JSON; full check.sh 132/132 93.36 с macOS arm64, final focused 5/5 + app rebuild; real 7zz binary raw transport P1-10 остаётся обязательным, Windows/Linux runtime unverified |
| P0-06.3 | Формат и точность времени как целый пользовательский сценарий: source property-to-text/TimeUtils, пять уровней меню и UTC, значения filesystem/archive, сохранение выбора и сортировка по raw FILETIME, normal/compact UX | После P0-06.2; owner `PanelListNotify`/`MyLoadMenu`/`FormatUtils`/Windows TimeUtils, не дублировать форматирование в Qt | done — audit § P0-06.3; source time level/menu dispatch and unchanged formatter, FS/archive typed values, UTC/persistence/raw time sort, 980/640 JSON; focused 5/5, full check.sh PASS (133 registered CTest) macOS arm64 2026-09-29; auto-refresh .4/parent and Windows/Linux runtime remain mandatory |
| P0-06.4 | Auto-refresh модель/UX и полная P0-06 приёмка: source timer/notification policy, changes/failure/cancel, focus/selection/scroll across refresh и соответствующие checks | После P0-06.3; owner `PanelItems`/`PanelListNotify`/`Panel`, текущие source selected-state portions использовать как evidence | done — audit § P0-06.4; source CApp/CPanel timer/selection flow, POSIX metadata adapter, View toggle/persistence, missing-folder recovery; focused `panel_auto_refresh` and full check.sh 134/134 pass after sandbox DBus/priority rerun on macOS arm64 2026-09-29; P0-05 flat scan cancel/stale and P0-06.1–.3 geometry evidence retained; Windows/Linux runtime unverified |
| P0-07.1 | Цельный Settings/list-presentation scenario: исходный порядок общих FM list flags в одной Qt форме, Apply/Cancel/persistence; подключить ShowDots/FullRow/ShowSystemMenu, icons/grid/single-click и hover/track-select к table/icon представлениям disk/archive; проверить modifiers/drag/leave/removal/scroll, focus/tooltips и normal/compact geometry | После P0-03/P0-05; owner `SettingsPage`/`RegistryUtils`/`PanelListNotify`, текущий `qtlistview` только адаптер для property dialog. AlternativeSelection control остаётся disabled до исходной selection-модели P0-07.2; память/large-pages и 6-page Options — P1-12 | done — audit § P0-07.1; source page/ShowDots and Qt table/icon/hover adapters, normal/compact JSON, targeted paint inspection, focused direct consumers 8/8 and full unsandboxed check.sh 135/135 macOS arm64 2026-09-29; final localized tooltip/drag test fix passed focused build/CTest; parent P0-07 и Windows/Linux runtime обязательны |
| P0-07.2 | Полный source selection-command component: заменить самостоятельный `CPanelSelection` на тела `CPanel::SelectSpec/SelectByType/SelectAll/InvertSelection/UpdateSelection`, сохранить исходный индексный вектор и подключить mask/type/all/invert в table/icon disk/archive/root/flat, включая диалог, focus/selection, cancel, keyboard shortcuts и normal/compact checks | После P0-07.1; source `PanelSelect.cpp`, `Wildcard.cpp`, `QtPanelList`; прежние CPanelSelection tests служат evidence, но дубликат после замены удалить. Разблокирует единый selection owner для .3 | done — audit § P0-07.2; source CPanel vector/commands, Qt dialog/list adapter and notification guard, macOS wildcard policy fix; focused 6/6, full unsandboxed check.sh 135/135 macOS arm64 2026-09-29; .3/parent and Windows/Linux runtime remain mandatory |
| P0-07.3 | Альтернативная source mark-модель, mouse/range/keyboard и полная P0-07 приёмка: `PanelSelect`/`PanelKey`/notify OnLeftClick/OnArrowWithShift/OnInsert, Ctrl/Shift/Alt/keypad, включённый AlternativeSelection control, table/icon disk/archive/root/flat, row removal/scroll/focus/accessibility и tests normal/compact | После P0-07.2; source marked vector и Qt notification adapter принадлежат одному panel owner; P0-08 two-panel отдельный consumer | todo; parent P0-07 открыт |
| P0-10.2 | Общий progress/messages для всех archive/filesystem/volume consumers: удалить разрозненные progress/journal owners, сохранить сообщения/partial results и destruction/cancel races; полная P0-10 приёмка и актуальные geometry/contracts | После P0-10.1, P0-02/P0-03 и требуемых model consumers; необходимые transport/callback dependencies — внутренние работы результата, engine не переписывается | todo |

Порядок ближайшей работы: P0-10.1 → P0-04.1 → P0-02.2 → P0-02.3 →
P0-03, затем остальные parent IDs по приоритету и выполненным зависимостям.
P0-04 целиком закрывается только после своей command/navigation приёмки с P0-03;
P0-05 и P0-10 целиком — только после перечисленных parent критериев.
Все остальные открытые P0/P1/P2/P3 строки остаются крупными рабочими пакетами
с прежними ID и зависимостями. Если понадобится несколько результатов, допустим
только один уровень .1/.2, каждый с подключённым компонентом/сценарием и UX.
Historical требования отдельного commit на старый helper ниже являются записью
прежнего процесса, а не правилом для новых итераций. Git history не переписывается.

Карта начата на HEAD `f21eee3`; параллельно пользователь сохранил новые правила
в коммите `73359b3` (`upd plan`). Этот коммит и его checklist/prompt/plan правки
сохранены; текущий HEAD — `73359b3`.
При укрупнении сохранены собственные WIP: ProgressDialog2 cpp/h,
qtprogresscontrols cpp/h, progress_worker_tests, его CMake registration и прежняя
active запись checklist. Они завершены внутри единого P0-10.1 вместе с callback
dependencies, реальным Add/drop consumer и UX. Historical checks 96/96 сохранены;
новая приёмка P0-10.1: focused 9/9, 24 JSON states и check.sh 101/101, 72.05 с
на macOS arm64 2026-09-29. Windows/Linux runtime остаётся обязательным.

## P0 — критическая основа и ежедневный UX

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [x] | P0-01 | Реестр symbols, omissions, dependencies всех 109 UI `.cpp` и текущих зеркальных модулей | — | done — audit § P0-01.4; .1–.4 done; 2026-09-28 macOS docs-only diff/112 paths/33 vendor byte matches |
| [x] | P0-02 | CApp/CPanel owners и независимое состояние панелей | P0-01 | done — audit § P0-02.3 parent acceptance; .1/.2/.3 и native location prerequisite, full gate 122/122; macOS 2026-09-29, Windows/Linux unverified |
| [x] | P0-03 | Единые command availability/dispatch для menus/toolbar/keys | P0-02 | done — audit § P0-03; native policy/CApp dispatch/current Qt consumers; focused fixes 5/5 3.35 с, check.sh 129/129 87.96 с; macOS arm64 2026-09-29; Windows/Linux unverified |
| [x] | P0-04 | Исходная навигация, адрес и parent-folder stack | P0-02, P0-03 | done — audit § P0-04.1/.2; source binding/open/parent/root and current window consumers, nested error/password/cancel rollback, focused 8/8 and check.sh 130/130 on macOS arm64 2026-09-29; namespace/network/icons P1-11/P0-09, editors P1-08 remain mandatory; Windows/Linux unverified |
| [x] | P0-05 | FSFolder enumeration/model/flat view | P0-02, P0-04 | done — audit § P0-05.1/.2; source FS model and current normal/flat consumers, 2048-file ordered batches, stale navigation/cancel, 1500-row UX; focused 3/3, check.sh 130/130 macOS 2026-09-29; Windows/Linux unverified |
| [x] | P0-06 | Столбцы, сортировка, время и auto-refresh | P0-05 | done — audit § P0-06.1–.4; source sort/catalog/formatter/timer, Qt four-view columns/time/auto-refresh, selection/scroll and failure recovery; focused `panel_auto_refresh`, full check.sh 134/134 macOS arm64 2026-09-29; archive binary raw transport P1-10 и large-list polling P2-10 обязательны отдельно, Windows/Linux runtime unverified |
| [ ] | P0-07 | Mouse/keyboard selection и полные list settings/hover | P0-03, P0-05 | partial — .1 settings/list presentation/hover and .2 source selection commands accepted (full check.sh 135/135 macOS arm64 2026-09-29); .3 source mark/mouse/range/AlternativeSelection and parent приёмка обязательны |
| [ ] | P0-08 | Две панели, F9/Tab/Alt+F1/F2, persistence | P0-02, P0-03, P0-04, P0-07 | todo |
| [ ] | P0-09 | Исходные menus/toolbar/context/status и UI contracts | P0-03, P0-06, P0-08 | todo |
| [ ] | P0-10 | Общий progress/messages, cancel-confirm, pause/background | P0-02, P0-03 | active — P0-10.1 accepted integrated prerequisite для P0-04.1; P0-10.2 и полная parent приёмка обязательны |
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

После owners следующим доступным пакетом был **P0-03**, принят ниже.
Теперь следующий пакет — **P0-04**. Source inventory .1–.4 и P0-02 owners завершены;
P0-01 done. Inventory не закрывает обязательные функциональные порты P0–P3.

### P0-03 — integrated command component (2026-09-29)

Одна рабочая итерация P0-03, без helper-коммитов: исходный CFileMenu policy и
ExecuteFileCommand/OnMenuCommand dispatch; необходимые active-panel/operated
indices/read-only/capability bodies и узкие Qt services; current menus, toolbar,
context и keyboard consumers подключаются к одному owner. Удалить дублирующие
refreshActions правила и прямые QAction workflow branches. Qt-only busy/closed/
transport ограничения применяются в том же owner, не распределяются по widgets.
Операции, не перенесённые в своих пакетах, остаются disabled с причиной.
Приёмка: матрица filesystem/archive/nested/read-only/encrypted/loading/operation,
focused item versus selection, stale/disabled dispatch и одинаковый consumer route;
тексты/geometry/JSON при затронутых видимых состояниях, focused tests и полный gate.
Baseline тот же `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; bodies
MyLoadMenu.cpp/.h, PanelMenu.cpp, PanelKey.cpp, App.cpp/.h изучены до изменений.
Полные mutation/Copy/Edit workflows сохраняются у P1-03/P1-04/P1-01/P1-08; P0-03
переносит их command routing/capability границу и не закрывает эти пакеты.

Приёмка P0-03: done, 2026-09-29 macOS arm64/Qt 6.11.2. Native CFileMenu,
ExecuteFileCommand/OnMenuCommand/WM_COMMAND, operated/focused/read-only rules,
toolbar persistence/title и current menus/context/toolbar/keys подключены.
Filesystem/archive/nested/loading/busy/closed и missing-writer/encrypted matrix,
native open paths, source FS prerequisite success/failure/cancel и все consumers
проверены. Focused fixes 5/5 (3.35 с); ./scripts/check.sh: formatting/build PASS,
129/129 CTest (87.96 с). Current normal/small/compact JSON inspected; Windows
rendered parity, Windows/Linux runtime unverified. Root/address оставшийся P0-04,
model/timer P0-05/06, two-panel P0-08, resource/shell P0-09/P2-09 и полные
mutation/editor workflows остаются обязательными, не N/A. Следующий ID: P0-04.

### P0-02 — owners и panel state

Owner: текущий чат, 2026-09-28. Upstream baseline как P0-01.
Parent принят после source bodies, интеграции и consumer checks .1/.2/.3;
старые открытые соседние scopes сохраняются у owners карты укрупнения.

| Подшаг | Результат и критерии | После | Статус |
| --- | --- | --- | --- |
| P0-02.1 | CAppState из AppState.h: общие history/favorites owner, Read/Save, window integration; независимость instances, persistence, history/bookmark consumer tests | P0-01 | done — audit § P0-02.1; 2026-09-28 macOS build dev, focused 5/5 и check.sh 54/54 |
| P0-02.2 | CApp/CPanel lifecycle, location и panel focus/selected state; один panel через owners, separate instances для P0-08; source bodies + navigation/selection consumer checks | P0-02.1 | done — integrated scope/evidence в карте укрупнения и audit § P0-02.2; P0-02.3/full parent остаются обязательными |
| P0-02.3 | Operation start/error/cancel/close state ownership через panel/application adapters; сквозные consumer checks, architecture/audit/parity, parent acceptance | P0-02.2 | done — integrated component, focused 8/8 и check.sh 122/122; карта укрупнения/audit § P0-02.3, 2026-09-29 macOS |

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
| P0-02.2c | Create/CreateOnePanel и lifecycle/teardown ordering source scope, all-panel disable-before-release; current single-panel integration и checks | P0-02.2b | done within integrated P0-02.2 — audit § P0-02.2; check.sh 121/121 85.30 с, macOS arm64 2026-09-29; no separate helper commit |

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
| P0-02.2b3c | CTempFileInfo/CFolderLink и root/nested parent stack, source close/pop/cleanup ordering; cancellation/failure/temp-lifetime checks | P0-02.2b3b2, P0-02.2b3b3c3c | active — a–d below; provider/selected-consumer prerequisites accepted, full runtime parent stays open through b3d |
| P0-02.2b3d | BindToPath/LoadFullPath/parent navigation source scope и flat-mode owner, window integration без duplicate state, all location consumers; b3 acceptance | P0-02.2b3c | todo |

Native parent-stack work is split before code. Preserve original structs, paths,
names and close/update/pop order; inspect additional dependencies before each
functional port and subdivide if needed. No omitted mandatory branch is N/A.

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c1 | Original platform cleanup and library-transfer prerequisites for CTempFileInfo/CFolderLink; exact native types and narrow Qt boundary; a/b below | P0-02.2b3b2, P0-02.2b3b3c3c | done — a/b verified; broader FileDir/DLL omissions remain explicit mandatory owners |
| P0-02.2b3c1a | Windows/FileDir.cpp original typed DeleteFileAlways/RemoveDir and required Windows SetFileAttrib; Qt deletion wrapper delegates instead of independent readonly logic; Unicode/spaces/readonly/missing/nonempty/symlink regressions and direct consumers | P0-02.2b3b3c3c | done — audit § P0-02.2b3c1a; focused 6/6, check.sh 78/78; macOS 2026-09-29; Windows/Linux runtime unverified |
| P0-02.2b3c1b | Windows/DLL CLibrary original load/free/LoadEx plus source transfer/copy guard for parent links across platforms; deterministic local library fixture, failure/idempotence/lifetime checks; omitted module-path helpers explicit | P0-02.2b3c1a | done — audit § P0-02.2b3c1b; focused 5/5, check.sh 79/79; macOS 2026-09-29; full module-path/resource services mandatory |
| P0-02.2b3c2 | Original CTempFileInfo/CFolderLink types and cleanup/change detection/parent ownership, required native time comparison; copied versus platform adaptations documented and public behavior checks | P0-02.2b3c1 | done — audit § P0-02.2b3c2; focused 5/5, check.sh 80/80; macOS 2026-09-29; full runtime stack/update remains mandatory c3/c4 |
| P0-02.2b3c3 | Original source parent stack and CloseOneLevel/CloseOpenFolders/OpenParentArchiveFolder functional bodies, required update dependencies and root/nested runtime integration | P0-02.2b3c2 | active — a–e split below before code |
| P0-02.2b3c4 | Root/nested cancel/failure/pop/temp/update lifetime acceptance, direct consumers, source omissions review; parent closure only after complete mandatory scope | P0-02.2b3c3 | todo |

C3 dependencies inspected after c2: original close/pop calls changed-nested update
before deleting the link; update uses processing guards and a modal background
CopyFromFile service. Port those prerequisites before integrating close/pop.
Current chat owns all substeps; UI and transport adapters stay narrow.

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3a | Original processing flags and CDisableTimerProcessing/CDisableTimerProcessing2/CDisableNotify/Disable_Processing_Timer_Notify_StatusBar; native initialization, source RefreshListCtrl guards and Qt notification boundary; nested/restore/exception and selection-consumer checks | P0-02.2b3c2 | done — audit § P0-02.2b3c3a; pre-fix regressions failed; focused 4/4, check.sh 81/81, after localized final-command refresh affected 7/7; macOS 2026-09-29 |
| P0-02.2b3c3b | Original OpenParentArchiveFolder/OnOpenItemChanged update dependencies and Qt dialog/progress/7zz service adaptations; retain changed-file YES/NO/CANCEL/error/cleanup flow, success/failure/cancel regression and required resource/layout checks | P0-02.2b3c3a | active — b1–b3 split below before code |
| P0-02.2b3c3c | Original OpenAsArc, _parentFolders/_library/flat-mode ownership and CloseOneLevel/CloseOpenFolders bodies with transport opening adapter; source push/pop/lifetime checks; no duplicate functional implementation | P0-02.2b3c3b | todo |
| P0-02.2b3c3d | Root/nested runtime opening, restore/update/cleanup paths consume source stack, remove Qt ArchiveParent functional state; preserve provider snapshots only in transport boundary and test all direct consumers | P0-02.2b3c3c | todo |
| P0-02.2b3c3e | C3 combined source/body/dependency and runtime acceptance; explicit mandatory memory/password/zone/platform obligations, no parent closure with missing branches | P0-02.2b3c3d | todo |

B's resource, background-update and consumer dependencies are separated before
code; current chat owns all parts. Preserve original native helper bodies and
keep modal presentation/process services behind their narrow adapters.

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b1 | Complete original FormatUtils NumberToString/MyFormatNew bodies and unchanged resource.h/resourceGui.h/resource.rc captions; strict generated STRINGTABLE boundary with explicit mixed-resource mode; numeric/Unicode/placeholder/resource and generator failure/idempotence checks | P0-02.2b3c3a | done — audit § P0-02.2b3c3b1; focused 3/3, check.sh 82/82, vendor bytes 70/70; macOS 2026-09-29 |
| P0-02.2b3c3b2 | Original background CopyFromFile/OnOpenItemChanged service dependencies with Qt progress/cancel and ArchiveBackend update boundary; source indices/native paths, error/password/operation contracts and deterministic success/failure/cancel | P0-02.2b3c3b1 | active — a–d split below before code |
| P0-02.2b3c3b3 | Original OpenParentArchiveFolder changed-file prompt/update/cleanup body, Qt dialog services and source native stack prerequisite; YES/NO/CANCEL/missing/unchanged/update-error lifetime checks and required dialog text/geometry | P0-02.2b3c3b2 | done inside integrated P0-04.1; panel_navigation/nested_update and full 119/119 gate; parent omissions retained |

B2 source dependencies require native synchronization/error services, original
progress state and thread lifecycle before the update callback consumers. Current
chat owns all scopes; avoid independent Qt implementations of upstream state.

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2a | Original portable Windows/Synchronization/Thread/C Threads and ErrorMsg prerequisites; CProgressSync fields and full methods in mirrored ProgressDialog2, narrow platform sleep service; pause/resume/stop/null counters/unknown state/native errors/thread lifetime regressions | P0-02.2b3c3b1 | done — a1/a2 verified; full modal progress/callback/update remains mandatory b–d |
| P0-02.2b3c3b2b | Original CProgressThreadVirt Process/Create and required CProgressDialog lifecycle/presentation adapted to Qt; modal background work, final error handling, cancel/pause and thread joins; original resource/text normal/compact geometry and deterministic failures | P0-02.2b3c3b2a | done via P0-10.1 — b1–b3 evidence retained; integrated native worker/modal/control/Add acceptance, check.sh 101/101; final Windows/Linux runtime remains mandatory |
| P0-02.2b3c3b2c | Original UpdateCallbackGUI2/UpdateCallback100Imp interfaces and full required callback bodies; progress/error/password/move/stop/reopen contracts through source synchronization, preserve remaining mandatory callback/engine owners | P0-02.2b3c3b2b | done via P0-10.1 — full native interfaces/bodies and first real Add consumer; focused callback/password/error/reopen/transport checks and check.sh 101/101; remaining consumers P0-10.2 |
| P0-02.2b3c3b2d | Original CThreadCopyFrom/OnOpenItemChanged with typed folder operation/ArchiveBackend boundary and update acceptance; source path/index/password handling, success/failure/cancel preserving temporary ownership before caller cleanup | P0-02.2b3c3b2c | done inside integrated P0-04.1; source_update_backend/nested_update and full 119/119 gate; no separate helper commit |

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2a1 | Unchanged C/Threads.c, Windows Thread/Synchronization/Handle headers and complete ErrorMsg cpp/h; native threading link and deterministic event/lock/join/reuse/error formatting regressions, exact byte comparison | P0-02.2b3c3b1 | done — audit § P0-02.2b3c3b2a1; focused 4/4, check.sh 83/83, vendor bytes 76/76; macOS 2026-09-29 |
| P0-02.2b3c3b2a2 | Complete original CProgressSync in mirrored ProgressDialog2, original native types/synchronization/error owners with narrow sleep adaptation; pause/resume/stop/counters/null/unknown/error/final-message checks and direct consumers; no full dialog acceptance claim | P0-02.2b3c3b2a1 | done — audit § P0-02.2b3c3b2a2; focused 3/3, check.sh 84/84, all functional bodies compare equal after formatting/sleep adaptation; macOS 2026-09-29 |

Progress resources/handlers inspected before new presentation code: original view
has two statistic columns, status/file path, progress bar, error list and ordered
Background/Pause/Cancel controls. Rendering resources do not prove Windows pixels.
Current chat owns the following prerequisites and complete modal implementation.

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b1 | Original GetTimeString and unchanged progress/control/operation resources, strict generator supports source brace-delimited tables; generated upstream view reference, duration/overflow/metadata and generator regressions; no dialog acceptance claim | P0-02.2b3c3b2a | done — audit § P0-02.2b3c3b2b1; pre-change generator regression failed, focused 4/4, check.sh 85/85, vendor bytes 82/82 and source function match; macOS 2026-09-29 |
| P0-02.2b3c3b2b2 | Original CProgressDialog presentation/statistics/range/title/message/pause/background functional bodies and Qt widget/platform adapters; source grouping/order and text spec, normal/compact geometry/JSON with current source counter states; full omitted modal/worker lifecycle remains b3 | P0-02.2b3c3b2b1 | done — a/b bounded source presentation accepted, all 26 controls/20 current JSON reports; b3 full modal/worker lifecycle and final Windows/Linux platform acceptance remain mandatory |
| P0-02.2b3c3b2b3 | Complete CProgressDialog Create/WaitCreating/finish/cancel/error lifecycle and original CProgressThreadVirt Process/Create, native workers/joins and required Qt platform services; modal success/failure/exception/cancel/pause/finish-during-prompt and combined view acceptance; split before code if needed | P0-02.2b3c3b2b2 | done via P0-10.1 — a evidence retained; b/c integrated native lifecycle, joins, races and Add consumer; 24 JSON states, check.sh 101/101; Windows/Linux runtime remains mandatory |

The current chat owns b3. Source Windows waits retain WaitForMultipleObjects;
portable native CThread has no timed completion query. Supply a narrow monotonic
start/finish wait boundary, preserve source events and join before retirement.
No wait-service acceptance closes the full modal or update workflow.

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b3a | Portable start-request/worker-finished wait service for the original 500 ms two-handle boundary where POSIX upstream lacks an implementation; finish precedence, latched events, monotonic timeout, native worker wake/join/ownership checks; no dialog/worker workflow acceptance yet | P0-02.2b3c3b2b2 | done — audit § P0-02.2b3c3b2b3a; focused 3/3 0.56 s, check.sh 96/96 63.43 s, native thread/event/join fixtures; macOS 2026-09-29; source modal integration b/c remains mandatory, Windows/Linux runtime unverified |
| P0-02.2b3c3b2b3b | Copy complete source Create/WaitCreating/ProcessWasFinished/CheckNeedClose/OnMessage/OnExternalCloseMessage, CProgressCloser/MyThreadFunction/CProgressThreadVirt Process/Create and HResultToMessage; narrow Qt queued/modal/message services and native worker joins; delayed/fast success/error/exception, final/error-close states, normal/compact JSON and deterministic lifetime checks | P0-02.2b3c3b2b3a | done via P0-10.1 — original native flow, atomic worker flags, Qt modal/messages, joined success/error/exception and first real Add consumer; current normal/compact JSON, check.sh 101/101 |
| P0-02.2b3c3b2b3c | Copy source OnCancel/OnButtonClicked cancel-pause-confirmation and finish-during-prompt branches; original YES/NO/CANCEL resource/text/focus, restore pre-prompt pause, joined cancellation and full modal combined acceptance through real deterministic workers/fixtures | P0-02.2b3c3b2b3b | done via P0-10.1 — source YES/NO/CANCEL/Escape, previous pause restoration, finish-during-prompt races and real paused Add cancellation preserve archive/cleanup; check.sh 101/101 |

B2 statistics/message presentation can be accepted independently of process
priority/taskbar services; keep those mandatory and disable unfinished controls.
No view-only acceptance closes the full progress or update workflow.

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b2a | Source CProgressDialog fields/init, resize, range/statistics/time/title/message/clipboard and pause rendering; narrow Qt controls/font/tick/clipboard services; current native counter states, source text specification and all controls mapped to normal/compact JSON/geometry; cancel/background unavailable until their required services | P0-02.2b3c3b2b1 | done — a1/a2 presentation subset verified; mandatory taskbar/icon/priority b and modal-worker/cancel/finish b3 remain open |
| P0-02.2b3c3b2b2b | Original Windows taskbar/process-priority/icon bodies and required portable platform services/capabilities, source background transition and complete presentation acceptance; no mandatory service marked N/A; native/Qt behavior and all affected consumers verified | P0-02.2b3c3b2b2a | done — b1/b2 bounded native/source integration accepted; no mandatory N/A; macOS native checks and portable Linux transport, final Windows/Linux platform checks remain mandatory |

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b2a1 | Complete original file-local ReduceString, ConvertSizeToString, GetChangedString, GetPower32/64, MyMultAndDiv and GetNumDigits bodies in ProgressDialog2.cpp; native arithmetic/string boundaries verified without exporting helpers or changing native visibility | P0-02.2b3c3b2b1 | done — focused 3/3 0.57 s, check.sh 86/86 44.78 s; seven bodies equal after formatting; macOS 2026-09-29; no displayed view acceptance |
| P0-02.2b3c3b2b2a2 | Original CProgressDialog converter, fields/init/resize, statistics/title/messages/clipboard/pause with narrow Qt services; source resource text and every control reviewed in normal/compact current-build JSON/geometry; cancel/background disabled pending mandatory services | P0-02.2b3c3b2b2a1 | done — audit § P0-02.2b3c3b2b2a2; focused 10/10 10.10 s, check.sh 88/88 53.75 s; all 26 controls/16 JSON reports; native clamp baseline failed before fix; macOS 2026-09-29; explicit omitted platform/modal scope stays mandatory |

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b2b1 | Exact MyWindowsNew taskbar SDK/interface prerequisite and native FM.ico/IDI_ICON resource; copy original OnInit conditional icon-loading branch with narrow Qt resource/IconID service, verify real source asset loading/default/invalid-ID behavior and presentation consumers; no taskbar/priority acceptance yet | P0-02.2b3c3b2b2a | done — audit § P0-02.2b3c3b2b2b1; focused 5/5 10.03 s, check.sh 88/88 53.64 s, exact vendor bytes 84/84; macOS 2026-09-29; native taskbar/background still mandatory b2 |
| P0-02.2b3c3b2b2b2 | Original taskbar constructor/state/value/target/destructor bodies and OnPriorityButton transition; implement required macOS/Linux native services with actual reversible priority/capability evidence, integrate source background/default-button states and full presentation checks; split before code if dependencies require | P0-02.2b3c3b2b2b1 | done — a/b prerequisites and source integration accepted, native success/failure/restoration; focused 6/6 13.81 s, check.sh 95/95 64.46 s; actual Windows/Linux runtime remains final mandatory acceptance |

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b2b2a | Native reversible whole-process background service prerequisite: retain original Windows SetPriorityClass/GetCurrentProcess boundary and reuse native CProcess for owned child PID, implement required Darwin process-policy and Linux all-thread scheduler services where upstream has no portable owner; actual success/failure/restoration and owned process/thread lifetime checks; source button remains disabled until integration | P0-02.2b3c3b2b2b1 | done — audit § P0-02.2b3c3b2b2b2a; focused 5/5 0.76 s, check.sh 89/89 58.07 s, vendor bytes 85/85; macOS 2026-09-29 native self/child/worker checks unsandboxed; Windows/Linux unverified; mandatory button/transport integration remains open |
| P0-02.2b3c3b2b2b2b | Original taskbar initialization/state/value/target/destructor and OnPriorityButton bodies with narrow native/Qt services; required Cocoa/desktop taskbar capability, background/default-button integration and full source presentation acceptance; modal-worker/cancel still mandatory b3 | P0-02.2b3c3b2b2b2a | done — b1/b2/b3 bounded native/source acceptance; modal-worker/cancel b3 and actual Windows/Linux final platform acceptance remain mandatory |

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b2b2b1 | Narrow Cocoa Dock progress service where upstream has no POSIX taskbar owner; native AppKit capability, state/value/clear and ownership/lifetime checks; explicit process-wide badge presentation adaptation, no source dialog/button acceptance yet | P0-02.2b3c3b2b2b2a | done — audit § P0-02.2b3c3b2b2b2b1; macOS 2026-09-29 actual AppKit state/value/cleanup/capability failures; focused 4/4 9.48 s, check.sh 90/90 57.75 s; rendered Dock and source integration remain unverified/open |
| P0-02.2b3c3b2b2b2b2 | Native Linux desktop progress service with real transport/capability evidence, application identity and lifecycle; preserve unavailable-desktop behavior as an actual runtime capability, not mandatory N/A; no source button acceptance yet | P0-02.2b3c3b2b2b2b1 | done — bounded native transport/adapter/identity implementation accepted through a/b below; macOS portable checks only; actual Linux desktop and P3-05 packaging/final platform acceptance remain mandatory, not N/A |
| P0-02.2b3c3b2b2b2b3 | Copy original Windows constructor/state/value/target/destructor and OnPriorityButton functional bodies with narrow native services; background/default-button success/failure restoration and full source normal/compact UI specification/geometry/JSON acceptance | P0-02.2b3c3b2b2b2b2 | done — a/b copied source integration verified; 26 controls/20 normal/compact JSON reports; focused 6/6 and full 95/95 on macOS 2026-09-29; Windows/Linux runtime remains unverified |

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b2b2b3a | Copy original taskbar constructor/HrInit, source target selection, state/value/call sites and destructor clear; narrow native HWND/guarded Qt target adaptation, safe pre-OnInit lifetime and actual Cocoa source-dialog counter/pause/error/cleanup integration; Background/Cancel stay unavailable until their required scopes | P0-02.2b3c3b2b2b2b2 | done — audit § P0-02.2b3c3b2b2b2b3a; focused 5/5 11.85 s, check.sh 93/93 58.93 s; macOS 2026-09-29 native Cocoa source integration; Windows/Linux runtime and b3b priority/default-button remain mandatory |
| P0-02.2b3c3b2b2b2b3b | Original OnPriorityButton toggle/caption/title flow through native priority service, explicit real failure rollback/report, source Background default/focus/Enter semantics and complete normal/compact presentation specification/geometry/current JSON acceptance | P0-02.2b3c3b2b2b2b3a | done — audit § P0-02.2b3c3b2b2b2b3b; native success/refusal/error/retry/restoration, default/focus/Enter; focused 6/6 13.81 s, check.sh 95/95 64.46 s, 20 JSON reports/26 controls; macOS 2026-09-29; modal/worker/7zz propagation and Windows/Linux acceptance remain mandatory |

| Substep | Scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3c3b2b2b2b2a | Portable QtDBus launcher protocol owner: native Update/Query URI/map transport, value/state/clear and registration lifetime, real disconnected/invalid failure; deterministic owned private DBus fixture and native round trips, no live user session or desktop-rendering claim | P0-02.2b3c3b2b2b2b1 | done — audit § P0-02.2b3c3b2b2b2b2a; native private-bus round trips and cleanup on macOS 2026-09-29, focused 4/4 1.52 s, check.sh 91/91 58.59 s; Linux adapter/identity/rendering and source integration remain mandatory |
| P0-02.2b3c3b2b2b2b2b | Linux CQtTaskbarProgress adapter and application identity using the verified transport; actual session/capability failures and relevant consumers, compatible-desktop integration evidence where available; P3-05 packaging and rendered Linux checks remain mandatory if unavailable locally | P0-02.2b3c3b2b2b2b2a | done — audit § P0-02.2b3c3b2b2b2b2b; macOS portable adapter/native private-bus checks 4/4 2.09 s; check.sh 92/92 60.72 s; localized header-list registration rebuilt + affected 4/4 2.09 s; actual Linux rendering/packaging and source integration still mandatory |

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
| P0-02.2b3b3c2 | Properties consumes source folder catalogs/typed values and operated indices; copy original formatting dependencies and preserve Qt dialog/geometry; remove window metadata reconstruction | P0-02.2b3b3c1 | done — scoped selected-item consumer c2a–c2c verified; raw/archive-level/shell/native time remain mandatory owners |
| P0-02.2b3b3c3 | Remaining location/navigation/mutation consumers and teardown acceptance; verify source index ownership and temporary context fields before source parent/path stack migration | P0-02.2b3b3c2 | active — owned provider/selected consumer a–c verified; full focused navigation/context migration remains mandatory b3c/d |

Remaining consumer/lifetime migration is split before code. Full path/outer-stack
port stays P0-02.2b3c/d; no old borrowed compatibility branch is declared N/A.

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c3a | CAgentFolder owned lifecycle only: remove test-only external-proxy constructor/Init and child/parent/catalog compatibility branches; migrate fixtures to source-owned CAgent factory; surviving child/parent/catalog and unbound failure checks | P0-02.2b3b3c2 | done — audit § P0-02.2b3b3c3a; focused 10/10, check.sh 72/72; macOS 2026-09-29 |
| P0-02.2b3b3c3b | Remaining selection/navigation/mutation consumers use source indices and owned folder interfaces where available; inspect original paths/real-index mapping before implementation, split additional dependencies if needed; retain explicit temporary full-path boundary until source stack | P0-02.2b3b3c3a | done — b1–b3 verified; selected-consumer scope only; remaining focused navigation/context consumers explicitly mandatory b3c/d |
| P0-02.2b3b3c3c | Consumer replacement/failed load/cancellation/teardown acceptance and direct regressions; record remaining path/outer-stack prerequisites without claiming them completed | P0-02.2b3b3c3b | done — c1–c3 verified; scoped provider/selected consumer acceptance; full source release/stack/navigation remain mandatory |

Selection translation prerequisites split before implementation. Original Extract
flat-folder expansion policy and unsupported proxy2/alternate streams remain
explicit mandatory operation/handler scopes, not a complete Extract claim.

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c3b1 | Original CAgentFolder GetRealIndex/GetRealIndices, CProxyArc GetRealIndices_Unsorted and CUIntVector AddRealIndices; unchanged C/Sort dependency; normal/flat explicit/implicit directories, flags, overlap/duplicate/sorted/empty selection and existing consumers | P0-02.2b3b3c3a | done — audit § P0-02.2b3b3c3b1; focused 5/5, check.sh 73/73, vendor bytes 67/67; macOS 2026-09-29 |
| P0-02.2b3b3c3b2 | Window archive-selection paths consume source operated/real indices and owner's typed path metadata, remove row/proxy reconstruction; selected-folder/flat policy recorded, extraction/copy/move/hash direct regressions and remaining consumer inventory | P0-02.2b3b3c3b1 | done — audit § P0-02.2b3b3c3b2; focused 2/2 and expanded 7/7, check.sh 74/74; macOS 2026-09-29 |
| P0-02.2b3b3c3b3 | Remaining single-item/filesystem path and hash-result selection consumers use source folder helpers; inspect original bodies/dependencies before code and split if needed; full outer/native path stack remains b3c/d | P0-02.2b3b3c3b2 | done — a–e verified; remaining consumer inventory in audit § P0-02.2b3b3c3b3e |

Remaining path consumer prerequisites are split before code. Original names/types/
control flow stay authoritative; temporary full-location adapter and complete
native/outer path stack are not considered accepted by these narrower consumers.

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c3b3a | Original Agent.cpp GetName/GetFullPrefix _proxy bodies for bound single items; normal/flat/subfolder/full-root prefixes, Unicode and retained child-owner tests; proxy2 stays mandatory P1-10 | P0-02.2b3b3c3b2 | done — audit § P0-02.2b3b3c3b3a; focused 3/3, check.sh 74/74; macOS 2026-09-29 |
| P0-02.2b3b3c3b3b | Original CPanel relative/full filesystem path helpers and required type/prefix classification; source UString signatures and owned fixtures, precise drive/outer-stack omissions and native path boundary checks | P0-02.2b3b3c3b3a | done — audit § P0-02.2b3b3c3b3b; focused 5/5, check.sh 75/75; macOS 2026-09-29 |
| P0-02.2b3b3c3b3c | Archive single-item rename/view/edit and hash-result selected targets consume source names/full prefixes/indices; success/failure/cancel direct regressions, remaining open/navigation owner inventory | P0-02.2b3b3c3b3b | done — audit § P0-02.2b3b3c3b3c; focused 9/9, check.sh 75/75; macOS 2026-09-29 |
| P0-02.2b3b3c3b3d | Filesystem selected paths and relevant single-item consumers use owned source paths; native Unicode/flat/sorted/source-index/operation regressions without inventing path algorithms | P0-02.2b3b3c3b3c | done — audit § P0-02.2b3b3c3b3d; focused 3/3 and expanded 8/8, check.sh 76/76; macOS 2026-09-29 |
| P0-02.2b3b3c3b3e | Remaining consumer acceptance and documented dependency boundary to full source BindToPath/LoadFullPath/parent stack; close only verified scope | P0-02.2b3b3c3b3d | done — audit § P0-02.2b3b3c3b3e; focused 7/7, check.sh 76/76; macOS 2026-09-29; full native location consumers stay mandatory |

Consumer lifecycle acceptance is split before code. This is the bound-provider/
selected-consumer prerequisite for native parent/path integration, not completion
of remaining focused navigation or duplicate context fields.

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c3c1 | Public consumer regressions across archive replacement, failed/cancelled listing, retained owner after cancelled extraction, and filesystem return; source paths/indices, disabled loading commands, no stale requests/temporary paths | P0-02.2b3b3c3b | done — audit § P0-02.2b3b3c3c1; focused 7/7, check.sh 77/77; macOS 2026-09-29 |
| P0-02.2b3b3c3c2 | Teardown operation quiescence and temporary/folder lifetime ordering; inspect source ReleaseApp/ReleasePanel/CloseOpenFolders and current backend/thread destruction, adapt only Qt/7zz services and add deterministic destruction regressions | P0-02.2b3b3c3c1 | done — audit § P0-02.2b3b3c3c2; pre-fix regression failed, focused 5/5, check.sh 77/77; macOS 2026-09-29; full native release/stack mandatory |
| P0-02.2b3b3c3c3 | Combined lifecycle acceptance, explicit remaining source stack/interfaces/command obligations and dependency ordering; close only validated provider/selected-consumer scope | P0-02.2b3b3c3c2 | done — audit § P0-02.2b3b3c3c3; unchanged runtime 1189a82 gate 77/77, vendor 67/67; docs-only diff check; macOS 2026-09-29 |

Dependency clarification (c3c3): b3b/b3b3/b3b3c/b3b3c3 remain active because
their full location/navigation/context acceptance requires b3d. Native stack b3c
therefore depends on the completed typed providers b3b2 and bounded consumer
lifecycle c3c, rather than waiting for the broader runtime parent's b3d-dependent
closure. This removes a dependency cycle without waiving any original functions;
close the broader parents only after b3c/d integration and their remaining checks.

P0-02.2b3b3c1 commit: `6bd328b`. Properties migration is split before code:

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c2a | Compile unchanged portable PropVariantConv/PropIDUtils and required FileLink dependencies; typed strings/numeric/bool/time/attributes/reparse/invalid-input formatting regressions; no duplicate formatter | P0-02.2b3b3c1 | done — audit § P0-02.2b3b3c2a; focused 10/10, ASan 1/1, check.sh 65/65, vendor bytes 61/61; macOS 2026-09-29; Windows native link operation dependencies remain mandatory P2-05/P3-05 |
| P0-02.2b3b3c2b | Source property-name/PanelMenu formatting branches and required folder-property gaps; narrow Qt resource and platform services; catalog/unknown/error behavior tests | P0-02.2b3b3c2a | done — scoped formatter/cache b1–b3 verified; live archive catalog and complete Properties integration remain c2c |
| P0-02.2b3b3c2c | Live Properties binding/operated indices, remove window metadata snapshots, normal/flat/four-view size/metadata/geometry acceptance and existing direct consumers | P0-02.2b3b3c2b | done — a–c verified below; full module/raw/handler metadata remains mandatory |

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
| P0-02.2b3b3c2ca | Typed archive transport property catalog and original CAgentFolder catalog expansion/remapping; normal/flat/property presence/error checks, full engine metadata remains P0-03/P0-04 | P0-02.2b3b3c2b | done — a1/a2 verified below; full handler metadata and borrowed lifetime remain mandatory |
| P0-02.2b3b3c2cb | PropertiesDialog/MainWindow consume operated indices/live CPanel owner, remove snapshot metadata reconstruction, restore source packed aggregate visibility and retain existing raw Modified transport text until mandatory native time parsing P0-03/P0-04; focused source metadata/layout checks | P0-02.2b3b3c2ca | done — audit § P0-02.2b3b3c2cb; focused 8/8 and expanded direct consumers 6/6, check.sh 71/71; two normal/compact JSON reports reviewed; macOS 2026-09-29 |
| P0-02.2b3b3c2cc | Filesystem/archive normal/flat/four-view runtime acceptance for source index/name/counts, immutable/absent/zero metadata, transitions and normal/compact Properties geometry | P0-02.2b3b3c2cb | done — audit § P0-02.2b3b3c2cc; focused 9/9, check.sh 72/72, 48 normal/compact JSON reports reviewed; macOS 2026-09-29 |

| Substep | Owner, scope and acceptance | After | Status |
| --- | --- | --- | --- |
| P0-02.2b3b3c2ca1 | QtPanelFolder 7zz typed Path/Size/PackSize transport catalog, preserve legacy custom-property callers; source types/names/Unicode/zero/absence/invalid-index checks | P0-02.2b3b3c2b | done — audit § P0-02.2b3b3c2ca1; focused 7/7, check.sh 71/71; macOS 2026-09-29 |
| P0-02.2b3b3c2ca2 | Original CAgent ReadItems ThereIsPathProp scan and CAgentFolder GetNumberOfProperties/GetPropertyInfo plus kProps; transport Path→Name, optional Name/counts/flat Prefix and failure scope | P0-02.2b3b3c2ca1 | done — audit § P0-02.2b3b3c2ca2; focused 7/7, check.sh 71/71; macOS 2026-09-29; borrowed compatibility delegation remains mandatory c3 |

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
