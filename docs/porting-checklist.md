# Чеклист портирования для последовательных чатов

План и scope: [porting-plan.md](porting-plan.md).
Готовый запрос агенту: [porting-chat-prompt.md](porting-chat-prompt.md).
Исходная дата: 2026-09-28. Это список **оставшейся** работы, не оценка MVP с нуля.

## Языки исходников и рефакторинг (уточнение 2026-10-03)

**Переносимый upstream-код сохраняет исходный язык:** файлы из `C/` остаются
на C (`.c` / `.h`), из `CPP/` — на C++ (`.cpp` / `.h`). Нашу обвязку,
интеграцию и интерфейс пишем на **C++17 / Qt 6 Widgets**. Цель — ускорить
и упростить перенос за счёт прямого использования исходников и минимальной
обвязки; это не требует перевода upstream C на C++.

- Функциональную основу копируем из upstream `C/` и `CPP/`: сохраняем исходные
  типы, алгоритмы, control flow, имена и границы модулей. Переносимые зависимости
  используем напрямую; Qt нужен для виджетов и узких адаптеров к платформе.
- Адаптированные модули размещаем в `src/<upstream-relative-path>`, неизменённые
  зависимости — в `third_party/7zip/<upstream-relative-path>`. У каждого модуля
  остаётся одна авторитетная реализация.
- Новую C++-логику пишем для Qt presentation, платформенной и backend-интеграции
  либо функций без пригодной upstream-реализации. Перед этим проверяем исходники
  и фиксируем причину адаптации или отсутствия реализации в audit.
- Не заменяем исходную функциональную логику Qt/std-реализацией ради удобства.
  В затронутой области прежнюю самостоятельно написанную логику заменяем
  исходным C/C++-кодом. Рефакторинг уже написанной обвязки выполняем
  отдельными интегрированными итерациями P2-11.
- Архивные операции сохраняем за `ArchiveBackend`: widgets не формируют команды
  `7zz`; перенесённые native C/C++-модули подключаем через backend и его адаптеры.
- C-модули собираем как C; границу вызова из C++ оформляем через существующие
  upstream headers и предусмотренный ими C linkage. Не переименовываем `.c`
  в `.cpp` и не заставляем C++-компилятор собирать C-код ради унификации.
- Узкие платформенные bridges сохраняют необходимый платформе язык
  (например, Objective-C++ для macOS); причину исключения фиксируем в audit.

Прежние статусы и evidence сохраняются. Добавлен обязательный P2-11 для
рефакторинга уже написанного кода; итоговая приёмка включает его результат.
Документационное уточнение само по себе не закрывает пункты плана.

## Как выбрать и закрыть шаг

- [ ] Прочитать `AGENTS.md`, этот чеклист и нужный раздел плана.
- [ ] Проверить `git status`, current HEAD, indexed memory, audit/parity и код.
- [ ] Выбрать первый открытый доступный ID по P0 → P1 → P2 → P3 и зависимостям.
- [ ] Если отдельная функция заблокирована, проверить зависимости других сценариев,
  вынести её в собственный ID и продолжить первый доступный результат; правило —
  в [плане](porting-plan.md#блокировка-отдельной-функции-и-продолжение-плана).
- [ ] Указать scope, upstream revision/symbols, owner, исходный язык C/C++,
  напрямую используемые модули и необходимые C++ Qt/platform/backend adapters.
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

При перезапуске применять раздел «Размер итерации и коммита» плана. Исторические
глубокие ID сохраняются в audit/archive как evidence; новые уровни к ним не добавлять.
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
`blocked` относится к своему ID, а не ко всему плану. Не выбирать его повторно
автоматически; возвращаться при новой capability или отдельной итерации по
снятию prerequisite. Следующий шаг определять по фактическим
capabilities, а не только по старой ссылке на parent; изменение зависимости
фиксировать в плане и строке пакета.

Ставить `[x]` только при `done`. В результате указать audit section, tests с
датой/ОС/result и применимую ревизию; unverified platforms перечислить явно.
Если код уже реализован другим чатом, сначала проверить bodies и current checks,
затем учесть результат, а не реализовывать повторно.

Текущий счёт: **36/47 завершённых пакетов; 11 остаётся** (2026-10-03).
Добавлен P2-11; исторические счётчики в evidence не пересчитываются.
P0: 12/12; P1: 16/18; P2: 7/11; P3: 1/6. P0-01–P0-12, P1-01–P1-11, P1-14–P1-18, P2-01–P2-07 и P3-01
закрыты по строкам ниже; незавершённые обязательные функции сохраняют owners.
Подшаги не увеличивают число parent packages; новый самостоятельный scope
добавляется отдельной строкой и меняет знаменатель в обоих документах.
Это счётчик выполнения плана, не процент исходных строк upstream.

## Рабочие итерации после укрупнения (2026-09-29)

Применяются обновлённые правила [размера итерации](porting-plan.md#размер-итерации-и-коммита-уточнение-2026-09-29).
Исторические audit, commits и evidence сохранены; P0-01/P0-02 свёрнуты
в итоговые сводки и архив. Старые
открытые строки, перечисленные в карте, **входят в новые результаты**, а не
закрываются сменой структуры. Их дочерние строки — внутренний список работ;
отдельных новых завершений или коммитов для helpers/методов/ресурсов не будет.
У всех текущих итераций owner — этот чат. После P0-11 счёт parent packages — 11/44.

| Рабочий ID | Целый результат и критерий приёмки | Точные prerequisites / исторический открытый scope | Статус |
| --- | --- | --- | --- |
| P0-10.1 | Подключённый общий source progress/callback component: CProgressDialog/CProgressThreadVirt, modal/finish/error/cancel-confirm, pause/background и реальные worker/process capabilities; необходимые UpdateCallbackGUI2/UpdateCallback100Imp bodies, первый реальный consumer через ArchiveBackend, замена его прежнего progress owner; success/failure/cancel/exception/prompt races, source text/normal/compact JSON и focused/full gate | Досрочный prerequisite для P0-04.1 changed-file update; принятые native folder/provider contracts, processing guards и sync/platform services из historical P0-02 достаточны, полные P0-02/P0-03 не требуются для этого компонента. Входит прежний progress/worker/callback scope P0-02; уже done helpers/services не переносятся повторно | done — audit § P0-10.1; integrated Add/drop, native callbacks/control/worker lifetime; focused 9/9 15.81 с, later affected archive checks 1.24/1.26 с; check.sh 101/101 72.05 с; 24 JSON states/26 controls; macOS arm64 2026-09-29, Windows/Linux runtime unverified; P0-10.2 и parent приёмка обязательны |
| P0-04.1 | Source navigation/parent-stack component: OpenAsArc, CFolderLink, CloseOneLevel/CloseOpenFolders, изменённые временные файлы и update prompt, CThreadCopyFrom/OnOpenItemChanged, BindToPath/LoadFullPath; current root/nested/address/parent consumers используют один native owner без Qt ArchiveParent/location дубликатов; password/failure/cancel/temp lifetime и UX проверены | Досрочный prerequisite для завершения P0-02 owners. После P0-10.1 и принятых typed providers/selection contracts. Входит прежний parent-stack/path/changed-file-update scope P0-02; progress/callback часть принадлежит P0-10.1 | done — audit § P0-04.1; root/nested/address/parent native consumers, changed nested success/failure/cancel, exact result/journal, selection/temp lifetime; check.sh 119/119 58.65 с; 16 normal/compact archive JSON; macOS arm64 2026-09-29. Known/editor/child-process P1-08, routing P0-03 и full P0-04 parent обязательны; Windows/Linux unverified |
| P0-04.2 | Цельный remaining root/address/parent scenario и parent P0-04 acceptance: native RootFolder rendering/entry, source command+shortcut, filesystem/archive/nested path routing and history/bookmarks, failure/password/cancel rollback, relative/root/Unicode/tar.gz checks, current normal/compact JSON, focused/full gate | После P0-04.1 и P0-03. Использует их принятый evidence; прежний navigation scope P0-02 принят только в рамках этих результатов. Полные editor/known-process owners остаются P1-08, namespace network/ADS — P1-11 | done — audit § P0-04.2; root/Computer/Parent/Back/relative Unicode/stale busy + prior nested/password/cancel/tar.gz evidence; focused 8/8 13.45 с, check.sh 130/130 87.96 с; root JSON 980×620/640×400; macOS arm64 2026-09-29. Parent P0-04 принят; Windows/Linux runtime и rendered Windows unverified |
| P0-02.2 | Один полностью подключённый CApp/CPanel owner: Create/CreateOnePanel/ReleaseApp, independent panel state, focus/selection/location contracts, all-panel disable-before-release; MainWindow остаётся Qt presentation, teardown/refresh/navigation consumers проверены | После P0-04.1; existing P0-02.1/selected/provider evidence сохранены. Входит прежний application/panel lifecycle scope; navigation/FS/progress код учитывается у соседних owners | done — audit § P0-02.2; native Create/CreateOnePanel/OnCreate/Save/ReleaseApp/ReleasePanel, per-panel focus/settings, startup success/failure/cancel и direct consumers; check.sh 121/121 85.30 с, 16 normal/compact JSON; macOS arm64 2026-09-29. P0-02.3/full parent, command/title/drop/model owners обязательны; Windows/Linux unverified |
| P0-02.3 | Application/panel operation-state component и полная P0-02 приёмка: start/error/cancel/window-close порядок, quiescence до освобождения owners, независимые экземпляры и сквозные consumer checks | После P0-02.2 и P0-10.1. Входит прежний P0-02.3 и remaining owner teardown/operation consumers; routing rules остаются P0-03 | done — audit § P0-02.3; native WM_CLOSE/operation guards/CWindowInfo, scanner/backend/file/volume quiescence; focused 8/8 7.54 с, check.sh 122/122 84.83 с macOS arm64 2026-09-29. Parent P0-02 принят; P0-03/04/05/08/10 и P1-08 omitted scopes обязательны, Windows/Linux unverified |
| P0-05 | Полный filesystem model/enumeration/flat component с текущими consumers, source property/filter/load bodies, scheduling/cancel/stale batches и большим deterministic fixture; принятые source enumeration/provider части используются как evidence | После P0-02/P0-04 по parent плану; ранее досрочно выполненные FS portions P0-02 зафиксированы только в их фактическом scope. Их remaining FS части входят сюда, remaining navigation — P0-04.1 | done — P0-05.1/.2 и historical source evidence; 2048 Unicode flat fixture, stale cancel/navigation, 1500-row normal/compact UX, check.sh 130/130 85.77 с macOS 2026-09-29; P0-06/09/P2-10 scope отдельно, Windows/Linux unverified |
| P0-05.1 | Source filesystem read-model component с normal/flat consumers: исходные LoadItems/LoadSubItems, property/comment/link/folder-stat bodies, source item indices/filters, UX cells/actions и tests для hidden/empty/Unicode/links/inaccessible | После P0-04.2; принятые CFSFolder enumeration/typed-property portions использовать как evidence, не копировать заново. Сортировка/CompareItems и полная колонная модель остаются P0-06 | done — audit § P0-05.1; TextPairs/CFSFolder source bodies, source normal/flat names/prefixes, hidden/empty/unreadable/Unicode/links and stat cancel; focused 4/4 6.13 с, check.sh 130/130 88.79 с macOS arm64 2026-09-29; current 980/640 flat JSON reviewed. P0-05.2/parent mandatory; Windows/Linux runtime unverified |
| P0-05.2 | Большой flat-view сценарий с исходной моделью, narrow async scheduling/cancellation adapter, stale batch при переходе, ограничением symlink cycles, детерминированным large fixture и полной P0-05 приёмкой | После P0-05.1; текущие FilesystemScanner/FolderLoader сохраняются только как scheduling/Qt transport, functional enumeration — source CFSFolder | done — audit § P0-05.2; 32-row GUI delivery, 2048 Unicode files/2050 source rows, event-loop heartbeat, existing cancel/stale/symlink/1500-row UX evidence; focused 3/3 2.51 с, check.sh 130/130 85.77 с macOS arm64 2026-09-29; parent принят |
| P0-06.1 | Единая исходная сортировка filesystem/archive/root для всех четырёх видов: `PanelSort.cpp` typed property/variant policy, имя/префикс/index tie-break, folder-first, Qt row adapter, visible sort action/header и regression | После P0-05; использует принятые source index/property/selection portions; raw property sort с column catalog, time/auto-refresh остаётся .2/.3/.4 | done — audit § P0-06.1; Qt comparator заменён source property/index loop, prefix BSTR order, header/action sync; focused 4/4 5.64 с, check.sh 130/130 88.78 с, JSON 980×620/640×400 macOS arm64 2026-09-29; .2/.3/.4/parent остаются обязательными |
| P0-06.2 | Целый сценарий колонок: source property catalog/capabilities/raw sort, width/order/visibility/sort/view persistence, динамический context header menu, сохранение текущих consumers и нормальная/compact геометрия четырёх режимов | После P0-06.1; owner FM `PanelItems`/`ViewSettings`/`PropertyName`; не считать текущие три Qt toggle полной моделью | done — audit § P0-06.2; source catalog/raw compare and CListViewInfo, dynamic Qt header, per-type layout/sort, four-mode 980/640 JSON; full check.sh 132/132 93.36 с macOS arm64, final focused 5/5 + app rebuild; real 7zz binary raw transport P1-10 остаётся обязательным, Windows/Linux runtime unverified |
| P0-06.3 | Формат и точность времени как целый пользовательский сценарий: source property-to-text/TimeUtils, пять уровней меню и UTC, значения filesystem/archive, сохранение выбора и сортировка по raw FILETIME, normal/compact UX | После P0-06.2; owner `PanelListNotify`/`MyLoadMenu`/`FormatUtils`/Windows TimeUtils, не дублировать форматирование в Qt | done — audit § P0-06.3; source time level/menu dispatch and unchanged formatter, FS/archive typed values, UTC/persistence/raw time sort, 980/640 JSON; focused 5/5, full check.sh PASS (133 registered CTest) macOS arm64 2026-09-29; auto-refresh .4/parent and Windows/Linux runtime remain mandatory |
| P0-06.4 | Auto-refresh модель/UX и полная P0-06 приёмка: source timer/notification policy, changes/failure/cancel, focus/selection/scroll across refresh и соответствующие checks | После P0-06.3; owner `PanelItems`/`PanelListNotify`/`Panel`, текущие source selected-state portions использовать как evidence | done — audit § P0-06.4; source CApp/CPanel timer/selection flow, POSIX metadata adapter, View toggle/persistence, missing-folder recovery; focused `panel_auto_refresh` and full check.sh 134/134 pass after sandbox DBus/priority rerun on macOS arm64 2026-09-29; P0-05 flat scan cancel/stale and P0-06.1–.3 geometry evidence retained; Windows/Linux runtime unverified |
| P0-07.1 | Цельный Settings/list-presentation scenario: исходный порядок общих FM list flags в одной Qt форме, Apply/Cancel/persistence; подключить ShowDots/FullRow/ShowSystemMenu, icons/grid/single-click и hover/track-select к table/icon представлениям disk/archive; проверить modifiers/drag/leave/removal/scroll, focus/tooltips и normal/compact geometry | После P0-03/P0-05; owner `SettingsPage`/`RegistryUtils`/`PanelListNotify`, текущий `qtlistview` только адаптер для property dialog. AlternativeSelection control остаётся disabled до исходной selection-модели P0-07.3; память/large-pages и 6-page Options — P1-12 | done — audit § P0-07.1; source page/ShowDots and Qt table/icon/hover adapters, normal/compact JSON, targeted paint inspection, focused direct consumers 8/8 and full unsandboxed check.sh 135/135 macOS arm64 2026-09-29; final localized tooltip/drag test fix passed focused build/CTest; parent P0-07 и Windows/Linux runtime обязательны |
| P0-07.2 | Полный source selection-command component: заменить самостоятельный `CPanelSelection` на тела `CPanel::SelectSpec/SelectByType/SelectAll/InvertSelection/UpdateSelection`, сохранить исходный индексный вектор и подключить mask/type/all/invert в table/icon disk/archive/root/flat, включая диалог, focus/selection, cancel, keyboard shortcuts и normal/compact checks | После P0-07.1; source `PanelSelect.cpp`, `Wildcard.cpp`, `QtPanelList`; прежние CPanelSelection tests служат evidence, но дубликат после замены удалить. Разблокирует единый selection owner для .3 | done — audit § P0-07.2; source CPanel vector/commands, Qt dialog/list adapter and notification guard, macOS wildcard policy fix; focused 6/6, full unsandboxed check.sh 135/135 macOS arm64 2026-09-29; .3/parent and Windows/Linux runtime remain mandatory |
| P0-07.3 | Альтернативная source mark-модель, mouse/range/keyboard и полная P0-07 приёмка: `PanelSelect`/`PanelKey`/notify OnLeftClick/OnArrowWithShift/OnInsert, Ctrl/Shift/Alt/keypad, включённый AlternativeSelection control, table/icon disk/archive/root/flat, row removal/scroll/focus/accessibility и tests normal/compact | После P0-07.2; source marked vector и Qt notification adapter принадлежат одному panel owner; P0-08 two-panel отдельный consumer | done — audit § P0-07.3; source marks/mouse/range/Insert/KillSelection, settings/persistence, disk/archive/root/flat/four views, row removal/hover/scroll, 33 JSON and two targeted viewport inspections; focused 9/9 and full unsandboxed check.sh 135/135 macOS arm64 2026-09-29; Windows/Linux runtime and rendered Windows parity unverified |
| P0-08.1 | Целый disk/disk two-panel scenario: исходный `CApp::SwitchOnOffOnePanel`, независимые Qt panel presentations/state, видимый splitter, F9/Tab/focus, active command/navigation routing, selection/view/location persistence, hidden-panel return и normal/compact UX | После P0-07; сохраняет два source `CPanel` владельца. Доработка archive/mixed modes и Alt+F1/F2 — .2; отдельные helper/method коммиты не делать | done — audit § P0-08.1; two-panel disk paths/views/selection/active Copy, both F9 hide sides, Tab, inactive refresh and flat-scan restart; normal/compact JSON; focused panel_key/app_lifecycle and full unsandboxed check.sh 135/135 macOS arm64 2026-09-30; .2/parent and Windows/Linux runtime mandatory |
| P0-08.2 | Целый mixed-panel scenario и parent приёмка: disk/archive, archive/archive, Alt+F1/F2 history/path focus, Copy/Move destination readiness, commands/toolbar active target, restore splitter/layout/location/view/selection и compact/hidden JSON | После P0-08.1; transfer bodies остаются P1-01, но destination default должен быть проверяем | done — audit § P0-08.2; disk/archive + archive/archive state, Alt+F1/F2, Copy/Move/Extract opposite destination, persisted count/focus/path/view/splitter and hidden-left path, 5 Qt JSON reports; focused direct consumers and full unsandboxed check.sh 135/135 macOS arm64 2026-09-30; P1-01 transfer and Windows/Linux runtime remain |
| P0-09.1 | Целая оболочка main menu/toolbar: исходный `resource.rc` command/order/grouping/shortcuts, `MyLoadMenu`/`FM` dispatch/checks, toolbar flags и исходные bitmaps через Qt; все текущие команды привязаны к active panel, отсутствующие явно disabled, normal/compact меню/toolbar JSON и keyboard tests | done — audit § P0-09.1; source menu order/IDs/shortcuts, 14 exact BMPs, Qt active panel routing and disabled missing commands; main_shell normal/compact window + File/View JSON, focused 8/8 and full unsandboxed check.sh 136/136 macOS arm64 2026-09-30. Context/status/system icons and parent — .2; Windows/Linux runtime and rendered Windows parity unverified |
| P0-09.2 | Целый panel context/status scenario и parent приёмка: `PanelMenu` policy/группы по выбору и типу папки, `PanelListNotify::Refresh_StatusBar` totals/selection/details, `SysIconUtils` platform adapter, two-panel active state, full source/resource UI contract и normal/compact JSON | done — audit § P0-09.2; source context projection, per-panel 4-part status and source icon cache with Qt lookup; disk/archive + keyboard/two-panel normal/compact JSON, focused direct consumers 11/11 and full unsandboxed check.sh 137/137 macOS arm64 2026-09-30. Shell extension/system verbs P1-02/P2-09/P3-05 remain disabled/mandatory; Windows/Linux runtime and rendered Windows parity unverified |
| P0-10.2 | Общий progress/messages для всех archive/filesystem/volume consumers: удалить разрозненные progress/journal owners, сохранить сообщения/partial results и destruction/cancel races; полная P0-10 приёмка и актуальные geometry/contracts | После P0-10.1, P0-02/P0-03 и требуемых model consumers; необходимые transport/callback dependencies — внутренние работы результата, engine не переписывается | done — audit § P0-10.2; source progress для всех текущих archive/filesystem/volume consumers, source MessagesDialog вместо journal, настоящий background и disabled Pause у workers без capability, backend/worker error + partial-result lifecycle, normal/compact JSON; focused 6/6 11.65 с, unsandboxed check.sh 137/137, parity fixture 5/5 macOS arm64/Qt 6.11.2 2026-09-30; Windows/Linux runtime и rendered parity не проверены |
| P0-12.1 | Интегрированный пользовательский Ask для текущего 7zz Extract: общий source OverwriteDialog, Yes/No/All/AutoRename/Cancel, реальные 7z/ZIP/selected/no-path/encrypted consumers, устойчивый backend transport без разбора текста и проверки UX | После P0-11; source ExtractCallback/ArchiveExtractCallback/ExtractGUI/Extract reviewed, CLI adapters изолированы за ArchiveBackend; source FM callback и batch scope закрываются .2/.3 | done — audit § P0-12.1; focused 6/6, полный unsandboxed check.sh 137/137, позже targeted extraction_options PASS после локальной правки; macOS arm64/Qt 6.11.2 2026-09-30. P0-12 parent partial, Windows/Linux runtime и rendered Windows parity непроверены |
| P0-12.2 | Целый root FM Extract Ask через source CPanel::CopyTo → CAgentFolder::Extract → CArchiveExtractCallback с 7zz stream adapter, общими backend guards, source progress/overwrite UI, реальными Yes/No/AutoRename/Cancel и fallback для неготового owner | После .1; root FullPaths незашифрованный consumer переносится на исходный per-file callback; nested/encrypted/no-path/batch ExtractGUI/Common/Extract остаются .3 | done — audit § P0-12.2; focused и полный gate, macOS arm64/Qt 6.11.2 2026-09-30; Windows/Linux runtime и rendered Windows parity непроверены |
| P0-12.3 | Целый root NoPaths/Ask user flow через source FM callback: выбранная папка, конфликт файла, flatten, проверка безопасности/совпадений, source prompt/progress и тест с настоящим ZIP | После .2; расширяет source path mode без переписывания алгоритма; явный directory entry source сохраняет пустую папку | done — audit § P0-12.3; focused и полный gate macOS arm64/Qt 6.11.2 2026-09-30; Windows/Linux runtime/rendered parity непроверены |
| P0-12.4 | Целый encrypted-root/current-folder Ask сценарий через source FM callback: проверка пароля до GetStream/перезаписи, реальные 7z/ZIP wrong/correct, повторный запуск, текущая папка и относительный путь назначения, Unicode/пробелы, source progress/overwrite, backend guards и focused/full gate | После .3; исходные `CAgentFolder::Extract`, `CArchiveExtractCallback`, `CPanel::CopyTo`, file-only 7zz adapter; вложенный архив и batch GUI/Common остаются .5 | done — audit § P0-12.4; реальные encrypted 7z/ZIP wrong→correct без потери старых байтов, nested-folder ZIP Ask, source overwrite и нулевой backend preflight; focused 5/5, modal-race repeat 5/5, полный unsandboxed check.sh 137/137 на macOS arm64/Qt 6.11.2 2026-09-30; P0-12 parent, Windows/Linux runtime и rendered Windows parity остаются обязательными |
| P0-12.5 | Завершить P0-12: nested archive и batch paths через исходную `ExtractGUI.cpp`/`Common/Extract.cpp` orchestration, option precedence, callback close/results, multi-archive/volume handling, failure/cancel/password, backend security, UX и итоговую parent приёмку | После .4; in-process `CArchiveLink::Open3` отсутствует в текущем порте, нужен узкий 7zz opener, сохраняющий source Extract loop; уже скопированные незавершённые модули не считаются готовыми | done — audit § P0-12.5; root/nested whole Ask через source ExtractGUI/Common, selected nested через FM, 2 ZIP с общим Unicode-путём и Yes/Cancel, split `.001` с пропуском следующих томов, encrypted wrong→correct, forced/saved modes; focused 4/4, полный `check.sh` 137/137 macOS arm64/Qt 6.11.2 2026-09-30; Windows/Linux runtime и rendered Windows parity непроверены |

После P1-03 был выполнен P1-04.1/.2; ZIP comment выделен в P1-17.
P1-01.4 принят после P1-02; parent P1-01 закрыт. Следующий доступный
результат по обязательному порядку — P1-05. Исторический порядок prerequisites
P0-10.1 → P0-04.1 → P0-02.2 → P0-02.3 → P0-03 уже подтверждён выше.
P0-04/P0-05/P0-10 приняты после перечисленных parent критериев; их
отдельные дальнейшие функциональные ветви сохраняются под собственными ID.
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
| [x] | P0-07 | Mouse/keyboard selection и полные list settings/hover | P0-03, P0-05 | done — .1/.2/.3 integrated settings/hover, source commands and alternative marks with disk/archive/root/flat/four-view keyboard/mouse UX; 33 JSON, targeted table/icon paint, focused 9/9 and full check.sh 135/135 macOS arm64 2026-09-29; Windows/Linux runtime and Windows rendered parity unverified |
| [x] | P0-08 | Две панели, F9/Tab/Alt+F1/F2, persistence | P0-02, P0-03, P0-04, P0-07 | done — .1 disk/disk и .2 disk/archive + archive/archive, active command target, destination default, Alt+F1/F2, count/focus/path/view/splitter restore and hidden path; audit § P0-08.1/.2, Qt normal/compact/hidden JSON, full check.sh 135/135 macOS arm64 2026-09-30; actual cross-panel transfer P1-01, Windows/Linux runtime and rendered Windows parity unverified |
| [x] | P0-09 | Исходные menus/toolbar/context/status и UI contracts | P0-03, P0-06, P0-08 | done — audit § P0-09.1/.2; исходный порядок/dispatch/toolbar BMP, context policy, per-panel status и icon cache, normal/compact JSON; focused 11/11 and full check.sh 137/137 macOS arm64 2026-09-30. Shell verbs P1-02/P2-09/P3-05, язык P1-14 и Windows/Linux/rendered parity остаются обязательными |
| [x] | P0-10 | Общий progress/messages, cancel-confirm, pause/background | P0-02, P0-03 | done — audit § P0-10.1/.2; source dialog + callbacks, archive/file/volume consumers, source diagnostic list, реальные cancel/background/7zz Pause и disabled unsupported worker Pause; full check.sh 137/137 macOS arm64 2026-09-30; Windows/Linux runtime и rendered Windows parity остаются непроверенными |
| [x] | P0-11 | OverwriteDialog и решения конфликта Copy/Move | P0-10 | done — audit § P0-11; исходные dialog IDs/answers и AutoRenamePath, per-operation Ask/All, GUI↔worker wait/cancel, replacement backup/rollback, file/folder conflicts, F5/F6 UX и 700×440/560×440 JSON; focused 4/4, полный check.sh 137/137 macOS arm64/Qt 6.11.2 2026-09-30. Source recursive directory merge P1-02, archive Ask P0-12, Windows/Linux runtime и rendered Windows parity остаются обязательными |
| [x] | P0-12 | Настоящий Ask при Extract через общий callback | P0-11 | done — P0-12.1–.5; source FM selected/current folder, source GUI/Common whole root/nested and batch, 7zz stream/open adapters, password/cancel/overwrite and split volumes; audit § P0-12.1–.5, focused 4/4, full `check.sh` 137/137 macOS arm64/Qt 6.11.2 2026-09-30. Extended modes/metadata P2-07, Windows/Linux runtime and rendered Windows parity remain mandatory |

## P1 — основные сценарии и полноценный UX

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [x] | P1-01 | Routing F5/F6 между panels/disk/archive | P0-08, P0-11, P0-12, P1-04 | done — P1-01.1–.4; audit § P1-01.1–.4, все disk/archive маршруты, Shift focused branch и direct destination; полный `check.sh` 145/145 macOS arm64/Qt 6.11.2 2026-10-01; Windows/Linux runtime и rendered Windows reference не проверены |
| [x] | P1-01.1 | F5/F6 disk↔disk и archive→disk: исходные OperSmart/Agent CopyTo, общий CopyDialog, обе панели, success/failure/cancel и byte-level fixtures | P0-08, P0-11, P0-12 | done — audit § P1-01.1; focused 7/7, byte-level root/current-folder copy+move and cancel, both-panel refresh, CopyDialog 440×250/340×220 JSON, полный `check.sh` PASS macOS arm64/Qt 6.11.2 2026-09-30. Обычный `CApp::OnCopy` dispatch остаётся P1-01.3, Shift branch — P1-01.4, FSFolderCopy — P1-02; Windows/Linux runtime unverified |
| [x] | P1-01.2 | Disk→archive: исходный CopyFrom/SetFiles/update, transaction, защищённый верхний архив, UX и tests | P1-01.1, P1-04 | done — audit § P1-01.2; source update plan, transactional 7zz adapter, F5/F6 two-panel routing, Unicode/empty directory/protected archive/cancel/unsafe path tests; focused tests and full unsandboxed `check.sh` PASS macOS arm64 2026-09-30. Archive→archive и обычный dispatch остаются P1-01.3, Shift branch — P1-01.4; Windows/Linux runtime unverified |
| [x] | P1-01.3 | Archive→archive через source temp staging, обычные F5/F6 ветки `CApp::OnCopy`: rollback, refresh обеих панелей, nested/encrypted capability gates и сквозная приёмка этих маршрутов | P1-01.1, P1-01.2 | done — audit § P1-01.3; real ZIP/7z two-panel copy/move/cancel/nested bytes, backend two-list Test/Extract/Add/stale, direct 10/10 affected UI, focused backend/source/progress tests, baseline 193 exact/136 adapted/3 local; полный unsandboxed `check.sh` PASS 142/142 macOS arm64/Qt 6.11.2 2026-10-01. Parent P1-01 остаётся открыт до P1-01.4; Windows/Linux runtime unverified |
| [x] | P1-01.4 | Shift+F5/F6 `copyToSame`, одиночный destination filename/rename, исходный `PanelKey` consumer и окончательная parent P1-01 приёмка | P1-01.3, P1-02 | done — audit § P1-01.4; source focused `OnCopy`, direct filesystem target, archive focused extraction/move, occupied/self/child tests; `file_transfer_direct`, `ui_copy_same` и полный `check.sh` 145/145 macOS arm64/Qt 6.11.2 2026-10-01; Windows/Linux runtime unverified |
| [x] | P1-02 | Cross-volume move и полные FSFolderCopy branches | P1-01.3 | done — P1-02.1/.2; исходные CopyTo/CopyFileSystemItems и AskWrite, staged cross-volume/merge, focused и полный `check.sh` 143/143 macOS arm64/Qt 6.11.2 2026-10-01; Shift consumer P1-01.4, ссылки/metadata P2-05/P2-06, Windows/Linux runtime остаются обязательными |
| [x] | P1-02.1 | Целый F6 cross-volume staging fallback: исходные FSFolderCopy copy bodies, публикация до удаления, occupied/Unicode/tree, cancel rollback, delete failure/partial set, инъекция файлового adapter | P1-01.3 | done — audit § P1-02.1; forced fallback/overwrite/cancel/delete failure и disk-panel consumers, полный unsandboxed `check.sh` PASS 142/142 macOS arm64/Qt 6.11.2 2026-10-01; настоящий второй том и Windows/Linux runtime не проверены |
| [x] | P1-02.2 | Полные `FSFolderCopy.cpp::CFSFolder::CopyTo`/`CCopyState` move/callback branches и parent приёмка: direct filename/new folder, recursive merge/AskWrite, source errors/progress/cancel и platform adapters | P1-02.1 | done — audit § P1-02.2; direct filename/new folder/file Move/cancel/invalid, Unicode CopyFileSystemItems, staged nested skip/replace/rename, F6 retained source/cancel/delete failure; `fs_folder_copy`/`file_transfer`/`fs_folder`, direct two-panel UI и normal/compact JSON; полный unsandboxed `check.sh` 143/143 macOS arm64/Qt 6.11.2 2026-10-01. Windows/Linux runtime/rendered Windows parity не проверены |
| [x] | P1-03 | Inline Rename/Create/Delete/корзина | P0-03, P0-10, P0-11 | done — P1-03.1/.2; macOS arm64 2026-09-30, source create/rename/delete, native Trash adapter, focused success/failure/cancel/permissions/partial tests и `check.sh` 137/137; Windows/Linux/rendered Windows/native Trash runtime не проверены |
| [x] | P1-03.1 | Целый filesystem CreateFolder/CreateFile/inline Rename: исходные PanelOperations/BrowseDialog path/name/selection/refresh bodies, текущие Qt consumers, составные пути, success/failure/Cancel и normal/compact UX | P0-03, P0-10, P0-11; FSFolder и ComboDialog source prerequisites уже приняты | done — audit § P1-03.1; macOS arm64 2026-09-30; focused `ui_operations`/`combo_dialog`/`native_fs_operations`/`panel_selected_state` и `flat_view`/`ui_state` прошли, `check.sh` 137/137; Windows/Linux/rendered Windows reference не проверены |
| [x] | P1-03.2 | Целый Delete/Shift+Delete: исходное подтверждение и worker lifecycle, platform recycle adapter, permanent-delete/partial/cancel/symlink guards, выбор и parent P1-03 acceptance | P1-03.1; archive mutations P1-04 отдельно | done — audit § P1-03.2; `ui_operations`, `filesystem_operations`, `native_fs_operations`, `panel_key`, `command_policy_runtime`, `filesystem_selection_runtime` прошли; `check.sh` 137/137 macOS arm64 2026-09-30; native Trash runtime и Windows/Linux остаются unverified |
| [x] | P1-04 | Archive CreateFolder/Rename/Delete для обычных и защищённых верхних ZIP/7z; source update и transactional publish через ArchiveBackend | P0-03, P0-10, P0-11, P1-03 | done — P1-04.1/.2 и audit § P1-04.1/.2; focused/full gate macOS arm64 2026-09-30 по строкам ниже. Исторический P1-04.3 вынесен в P1-17: comment не входит в эту приёмку; Windows/Linux runtime не проверены |
| [x] | P1-04.1 | Целый unencrypted archive CreateFolder/Rename/Delete workflow: source AgentOut/ArchiveFolderOut selection/update, FM consumers, transactional adapter, progress/error/refresh и UX | P1-03; existing Agent/ArchiveFolderOut partial prerequisites | done — audit § P1-04.1; ZIP/7z source callback + separate-output/native publish/reopen, implicit folder, bytes/mtime, unsafe/failure/cancel и F7/F2 inline/Delete UI; focused `source_update_backend`, `ui_operations`, `archive_selection_runtime`, `command_policy_runtime` прошли; `check.sh` 137/137 macOS arm64 Qt 6.11.2 2026-09-30, после локального metadata-test правки повторно прошёл `source_update_backend`; Windows/Linux/runtime и rendered Windows parity не проверены |
| [x] | P1-04.2 | Защищённые верхние архивы: source update/password flow и 7zz capability, bytes/metadata/ciphertext, success/failure/cancel | P1-04.1 | done — audit § P1-04.2; 7z header encryption/ZIP AES, retained password through source mutation and ReOpen, protected bytes/mtime, wrong password/cancel exact-original, F7/F2/Delete with one password prompt; focused/full gate macOS arm64; Windows/Linux runtime and rendered reference unverified |
| [x] | P1-05 | Update/path modes и полное Add workflow | P1-04 | done — P1-05.1–.4; исходные action sets/pair/callback и path enumeration, защищённый Add, directory filtering и транзакции; полный unsandboxed `check.sh` PASS 146/146, 61.96 с macOS arm64/Qt 6.11.2 2026-10-01. Current-build Add JSON 760×720/640×480 inspected; Windows/Linux runtime и rendered Windows reference unverified; advanced/nested/formats/links scope остаётся у своих P1/P2 owners |
| [x] | P1-05.1 | Existing top-level ZIP/7z Add/Update/Freshen/Synchronize для относительных файлов: исходные action sets/callback plan, compression/profile, UI, rollback и проверки | P1-04 | done — audit § P1-05.1; ZIP/7z newer/older/new/deleted и Unicode bytes, Store UI route, invalid/cancel rollback/profile, 760×720/640×480 JSON; focused `source_update_backend`, `compression`, `ui_update_modes`, `app_state`, `panel_folder` PASS, полный unsandboxed `check.sh` PASS 146/146 macOS arm64/Qt 6.11.2 2026-10-01. P1-05.2, Windows/Linux runtime и rendered Windows reference остаются |
| [x] | P1-05.2 | Relative/Full/Absolute path modes нового и существующего ZIP/7z, простые каталоги и транзакционный Add | P1-05.1 | done — audit § P1-05.2; source CCensor/EnumerateItems, Qt path control, staged rename для Absolute; реальные ZIP/7z, Unicode/empty directory, отмена второго create stage, traversal guard, normal/compact JSON; полный gate 145/146 из-за прежнего пропуска в inventory, после поправки focused inventory/baseline PASS macOS arm64/Qt 6.11.2 2026-10-01. Protected Add — P1-05.3, directory filtering и platform runtime — P1-05.4 |
| [x] | P1-05.3 | Защищённый Add для верхнего ZIP AES/7z, reuse password, rollback и UI | P1-05.2 | done — audit § P1-05.3; ZIP AES/7z header encryption, исходный callback password, source/UI success, wrong-password/cancel rollback; focused consumers и полный unsandboxed `check.sh` PASS macOS arm64/Qt 6.11.2 2026-10-01; Windows/Linux runtime и rendered Windows reference unverified |
| [x] | P1-05.4 | Сложные ветки каталогов и полная приёмка P1-05 | P1-05.3 | done — audit § P1-05.4; source Full-path Synchronize ZIP/7z сохраняет дочерние bytes без omitted directory records; kept subtree/removed nested directory, Unicode/space, unsafe rejection и second-stage cancel rollback. Focused 7/7 16.00 с, полный unsandboxed `check.sh` PASS 146/146 61.96 с macOS arm64/Qt 6.11.2 2026-10-01; UI startup/temp fixtures isolated after stale-path performance timeout. Windows/Linux runtime/rendered Windows reference unverified |
| [x] | P1-06 | Архивные тома create/open/test/extract | P1-05 | done — P1-06.1/.2; исходный CMultiOutStream creation, source open/list/TestArchives/Ask extract, occupied/missing/corrupt/password/cancel и UX; focused 9/9 6.14 с, полный unsandboxed check.sh 148/148 40.58 с macOS arm64/Qt 6.11.2 2026-10-01; Windows/Linux runtime и rendered Windows parity непроверены, обновление volumes отключено |
| [x] | P1-06.1 | Целое создание архивных томов ZIP/7z: исходные volume control/parser/confirmation/CMultiOutStream, transactional output, password/failure/cancel и UX | P1-05 | done — audit § P1-06.1; variable-size protected ZIP/7z, occupied/failure/cancel/pause, normal/compact JSON; focused 8/8 3.86 с, unsandboxed check.sh 147/147 41.04 с macOS arm64/Qt 6.11.2 2026-10-01; Windows/Linux runtime и rendered Windows parity непроверены |
| [x] | P1-06.2 | Open/list/test/extract архивных томов, missing/corrupt/password/cancel и parent приёмка | P1-06.1 | done — audit § P1-06.2; source TestArchives archive branch/CopyTo saved-password overload, `.001` picker и source panel/Agent/GUI consumers; focused 9/9 6.14 с, check.sh 148/148 40.58 с macOS arm64/Qt 6.11.2 2026-10-01; filesystem batch entrypoint остаётся mandatory P3-05, Windows/Linux runtime непроверены |
| [x] | P1-07 | Advanced options/time/parameters, lifecycle и compression memory retry/cancel | P1-05, P0-10 | done — P1-07.1/.2, audit § P1-07.2; реальный decoder callback из исторического .2 вынесен в обязательный blocked P1-18 с точным prerequisite; runtime Windows/Linux и rendered Windows unverified, 2026-10-01 |
| [x] | P1-07.1 | Целая Advanced Options форма и parameter field: source COptionsDialog/SetOutProperties/parser, timestamp capability/roundtrip, validation, format profiles и normal/compact UX | P1-05, P0-10 | done — source Options/parser/property bodies, ZIP/7z time flags/profiles/method override; focused 7/7 4.06 s, полный check.sh 149/149 57.09 s macOS arm64 Qt 6.11.2; Add 760×720/640×480, Options 520×400/440×340 JSON просмотрены; Windows/Linux runtime/rendering не проверены |
| [x] | P1-07.2 | Source lifecycle shared/preserve access/archive mtime/delete-after-compress, compression memory retry/cancel, multi-stage/volume/native Add consumers, persistence и parent приёмка | P1-07.1 | done — audit § P1-07.2, pinned Update action/deletion/latest-time bodies; focused 11/11 PASS 3.97 s, full unsandboxed check.sh formatting/build/150 CTest PASS 42.21 s macOS arm64 Qt 6.11.2 2026-10-01; Add/lifecycle/memory normal760×720/compact640×480 и Options520×400/440×340 JSON inspected; decoder scope сохранён P1-18; Windows/Linux runtime unverified |
| [x] | P1-08 | External viewer/editor/diff, changed-temp update и VerCtrl | P0-04, P1-04 | done — audit § P1-08.1/.2/.3; disk/settings/Diff, archive process/update и VerCtrl consumers интегрированы; final check.sh155/155 49.53 s macOS arm64 Qt6.11.2 2026-10-01, focused7/7 2.32 s; source OS adaptations документированы, Windows/Linux runtime unverified; nested chain P1-09 и folder F3 P1-10 остаются |
| [x] | P1-08.1 | Целый disk View/Edit/Diff: source command/settings/launch, EditPage UX Apply/Cancel/Browse, keyboard consumers, controlled process и normal/compact checks | P0-04, P1-04 | done — audit § P1-08.1; native launch/Diff/EditPage, F3/F4 focused-file и extra text commands; focused9/9 2.18 s, check.sh151/151 45.16 s, Editor JSON640×390/520×350; macOS arm64 Qt6.11.2 2026-10-01; Windows/Linux runtime unverified |
| [x] | P1-08.2 | Archive external View/Edit: source known/child-process wait, changed-temp prompt/update, read-only/error/cancel/cleanup | P1-08.1 | done — audit § P1-08.2; source child/worker + OS/queued GUI adapters, real F3/F4 Yes/No/Escape, orphan child/short-long unchanged/wait error, close/pending/busy/stale/other-panel, extraction/update failure/cancel/read-only, protected ZIP AES/7z; focused8/8 4.85 s, check.sh154/154 48.27 s; normal/compact prompt JSON980×620/640×400; macOS arm64 Qt6.11.2 2026-10-01; Windows/Linux runtime unverified, parent open |
| [x] | P1-08.3 | Source VerCtrl Edit/Commit/Revert/Diff, backup/metadata, platform services и parent integration | P1-08.1/.2 | done — audit § P1-08.3; whole source VerCtrl, native dynamic File/context commands, 7vc persistence, backup001/002/mtime/readonly, Revert Yes/No/Escape/errors/limit/config/selection; focused7/7 2.32 s, final geometry1/1 0.63 s, check.sh155/155 49.53 s; Revert JSON700×440/560×440, window980×620/640×400; macOS arm64 Qt6.11.2 2026-10-01; parent accepted, Windows/Linux runtime unverified |
| [x] | P1-09 | Обновление цепочки вложенных архивов | P1-04, P1-08 | done — audit § P1-09; real nested source F4/mutations, protected levels, failure/cancel/No/Escape, wrong password, readonly chain, bytes/cleanup/parent focus и normal/compact JSON; final check.sh156/156 54.56 s macOS arm64 Qt6.11.2 2026-10-01, focused11/11 7.70 s; Windows/Linux runtime unverified |
| [x] | P1-10 | Полные properties/raw values/formatting и source F3 folder-size | P0-06, P1-04 | done — audit § P1-10.1/.2; source F3 и Properties item/raw/folder/arc/MainItem/non-open, actual file-only UInt64/CRC/checksum/SHA transport, full Unicode clipboard/detail, normal/compact JSON; final focused6/6 2.04 s и check.sh158/158 55.35 s macOS arm64 Qt6.11.2 2026-10-01; codec/ADS/reparse/shell/large-tree P2 scope сохранён, Windows/Linux runtime unverified |
| [x] | P1-10.1 | Целый source F3 folder-size workflow | P0-06 | done — restored EditItem body, source FS totals, Qt invalidation без reload, normal/compact JSON, focused5/5 1.41 s; final check.sh157/157 54.89 s macOS arm64 Qt6.11.2 2026-10-01; Windows/Linux runtime unverified |
| [x] | P1-10.2 | Полные source Properties и доступный raw transport | P1-10.1 | done — source enumeration/Agent interfaces, actual native CRC/UInt64 и checksum/SHA bytes, folder totals/levels/full detail/clipboard, CProxyArc2 decision P2-06; final focused6/6 2.04 s, check.sh158/158 55.35 s macOS arm64 Qt6.11.2 2026-10-01; Windows/Linux runtime unverified |
| [x] | P1-11 | Root/drives/network/Browse | P0-04, P0-05 | done — P1-11.1/.2/.3; source root/mount/network and Browse flows, disappearance/unreadable/cancel/history fixtures, current-build JSON, focused/full gate macOS arm64 2026-09-30; Windows/Linux runtime and rendered Windows reference unverified; device imaging remains P2-05/P3-05 |
| [x] | P1-11.1 | Целый root→mounted volume→filesystem workflow: source RootFolder/FSDrives owner, Qt mount adapter, disappearance/unreadable, navigation/history and normal/compact UX | P0-04, P0-05 | done — audit § P1-11.1; fake mount disappearance/unreadable, current-build root→mount UI, focused tests and macOS arm64 full gate; Windows/Linux native mounts unverified |
| [x] | P1-11.2 | Целые folder/file picker Browse flows: source entry points, Qt native service, all current Copy/Split/Extract/Create/Open consumers, Cancel/path/filter/history UX and tests | P1-11.1 | done — audit § P1-11.2; focused `browse_dialog`, `copy_dialog`, `extract_dialog`, `volume_operations`, `compression`, `ui_navigation`, `ui_operations`; macOS full gate; Windows/Linux picker runtime unverified |
| [x] | P1-11.3 | Network namespace: source CNetFolder traversal/properties/bind, Windows service and portable mounted-share capability, fake fixtures and UX | P1-11.1 | done — audit § P1-11.3; fake mounted-share Unicode/space, unreadable/disappeared binding, network root/Back/Parent, normal/compact JSON; focused/full gate macOS arm64 2026-09-30; live remote service and Windows/Linux runtime unverified |
| [ ] | P1-12 | 6-page Options container, Apply/Cancel, Settings/Folders | P0-07, P0-08, P1-07 | in progress — .1 integrated Settings/Folders принят; .2 full six-page acceptance после остальных page owners; source prerequisite P1-08.1 сохранён |
| [x] | P1-12.1 | Цельный Settings/Folders scenario, working/temp/RAM/large-pages, Apply/OK/Cancel, обе панели и UX | P0-07, P0-08, P1-07 | done — audit § P1-12.1; source Settings/Folders/App refresh/UpdateGUI work-dir bodies, Qt controls и native WorkDir/CLI consumers; actual Create success/work-path failure/cancel, two panels selection/focus, normal640×390/compact520×380 JSON; final check.sh160/160 55.99 s macOS arm64 Qt6.11.2 2026-10-01; focused8/8 5.72 s и final input1/1 0.45 s; Windows/Linux runtime unverified, parent open |
| [ ] | P1-12.2 | Итоговая интеграция шести страниц и приёмка Options parent | P1-12.1, P1-13, P1-14, P2-08, P2-09 | todo — unavailable page controls disabled до соответствующих capabilities |
| [ ] | P1-13 | Работающие страницы Edit/Menu | P1-08, P1-12.1 | in progress — Edit accepted P1-08; Menu .1 accepted; parent остаётся открыт до .2 source shell consumer |
| [x] | P1-13.1 | Source MenuPage/Options/Zone scenario, persistence, capability state и реальный FM extraction consumer | P1-08, P1-12.1 | done — audit § P1-13.1; source Menu bodies/flags/resource fallback, unchanged BoolPair preservation, real PanelCopy/Agent extraction consumer, explicit unavailable shell/POSIX Zone controls, JSON640×390/520×380 inspected; focused5/5 1.87 s, full unsandboxed check.sh161/161 57.27 s macOS arm64 Qt6.11.2 2026-10-01; baseline203 exact/149 adapted/3 local verified; Windows ADS/Windows/Linux runtime/rendering unverified; parent open, next P1-14 |
| [ ] | P1-13.2 | Source shell policies/flags/registration и итоговая Edit/Menu приёмка | P1-13.1; P2-09 adapter | deferred — QueryContextMenu и registry/OS consumer пока не перенесены; disabled controls не закрывают mandatory scope |
| [x] | P1-14 | Language и все app captions/fallbacks | P0-09, P1-12.1 | done — .1/.2, audit § P1-14.2 acceptance; все текущие forms/messages/local context EN/RU, source RC/dynamic fallbacks, pending state/typed cancel, long-label/keyboard checks; full unsandboxed check.sh170/17063.11s macOS arm64 Qt6.11.2 2026-10-01; счёт25/46, Windows/Linux runtime/rendered parity unverified |
| [x] | P1-14.1 | Source Language, Options Apply/Cancel/restart и main window hot switch | P0-09, P1-12.1 | done — audit § P1-14.1; native parser/runtime/LangPage, original release EN/RU, source menu/toolbar/headers/status/Options consumers; dirty input/actions/selection/restart/fallback retained; EN/RU Options640×390/520×380 and compact panels inspected; final focused4/4 7.63 s, progress contract fixtures5/5 10.33 s, full unsandboxed check.sh163/163 58.87 s macOS arm64 Qt6.11.2 2026-10-01; baseline207 exact/152 adapted/3 local; parent .2 open, next P1-14.2; Windows/Linux runtime/rendering unverified |
| [x] | P1-14.2 | Все оставшиеся app captions, messages, forms и полная Language приёмка | P1-14.1 | done — audit § P1-14.2 acceptance; native FM/Extract/Add/advanced/Progress/Properties, viewer/menu/summary/commands, Split/Combine and backend contexts EN/RU; RC fallback/source formatters/bookmarks, preserved input/selection/bytes/HRESULT, real collision/Tab/buddy/fallback checks and normal/compact JSON. Final failed-gate fixtures13/13PASS3.00s; isolated modal consumers2/2PASS1.04s; full unsandboxed check.sh170/170PASS63.11s macOS arm64 Qt6.11.2 2026-10-01; baseline210 exact/152 adapted/3 local verified. Parent accepted; next P1-15; Windows/Linux runtime/rendered parity unverified |
| [x] | P1-15 | F1/Help/About/Delete Temporary Files | P0-09, P1-08, P1-14 | done — .1/.2, source offline Help/About и owned-temp BrowseDialog2; audit § P1-15.2 acceptance, full check.sh175/175PASS66.82s macOS arm64 Qt6.11.2 2026-10-01; счёт26/46, Windows/Linux runtime/rendered parity unverified |
| [x] | P1-15.1 | Source Help/About, offline topics/links, version/license, F1 и текущие context consumers | P0-09, P1-08, P1-14 | done — audit § P1-15.1; native HelpUtils/About/page callbacks, exact release HTML/CSS; pending state/dirty Apply retained, offline/unavailable/license/intercepted homepage, EN/RU normal/compact JSON. Focused11/11PASS4.70s, final F1/geometry4/4PASS1.05s, final native About2/2PASS0.54s; full unsandboxed check.sh172/172PASS63.63s macOS arm64 Qt6.11.2 2026-10-01; baseline215 exact/156 adapted/3 local verified. Parent .2 mandatory/open, next P1-15.2; Windows/Linux runtime/rendered parity unverified |
| [x] | P1-15.2 | Source Delete Temporary Files, owned temp registry/lifetime, confirm/cancel/partial failure и приёмка parent | P1-15.1, P1-08 | done — audit § P1-15.2 acceptance; actual source BrowseDialog2/modal command, registry/native+Qt/nested+process lifetime, spool publication, Yes/No/Cancel/first failure/replaced identity/active child-editor, context/props/FM preferences/Help/F1; focused18/18PASS12.43s, full unsandboxed check.sh175/175PASS66.82s macOS arm64 Qt6.11.2 2026-10-01; EN/RU normal/compact JSON reviewed, baseline217 exact/158 adapted/3 local verified; Windows/Linux runtime unverified |
| [x] | P1-16 | Drag/drop между панелями и наружу из архива | P1-01, P1-09, P0-07 | done — audit § P1-16; source OnDrag/Drop/transfer/CompressDropFiles/CApp + live Qt both views/lazy panels/popup/confirmation/Add; actual ZIP bytes/lifetime/refusal/cancel, disk Copy/Move, live two-panel и archive Yes/No/Cancel, close/SingleClick/hover guards; полный unsandboxed check.sh formatting/build/179 tests PASS67.66s macOS arm64 Qt6.11.2 2026-10-02, baseline218 exact/159 adapted/3 local verified. Finder proof отдельно в mandatory P3-03.1; Windows/Linux runtime/rendered parity unverified в P3-03/P3-06; eager Qt materialization documented, preparation responsiveness P2-10 |
| [x] | P1-17 | ZIP per-item comment: исходный in-process writer, ChangeComment/SetProperty, ArchiveBackend adapter, UX/error/refresh и tests | P1-04; native module/lease из P2-01 | done — audit § P1-17 acceptance; complete source ZIP handler/writer/dependencies, original CommentItem/SetProperty/ChangeComment и actual GUI/native backend/work-dir publication/reopen; EN/RU plain/AES success/failure/dialog Cancel/clear/raw spaces/focus, nested parent Yes/No, native callback pause/resume/cancel-paused, unchanged bytes/metadata и independent extraction. Current four Combo JSON360/260 reviewed; baseline351 exact/163 adapted/3 local verified. Unsandboxed full check.sh из isolated own-scope snapshot: formatting/all-target build/191 CTest PASS86.83s macOS arm64 Qt6.11.2 2026-10-02. Shared checkout191/192PASS88.64s: only foreign runner_vm failed absent file; foreign hunks preserved/excluded from iteration. Windows/Linux runtime/rendered Windows parity unverified; counts29/46 |
| [x] | P1-18 | Реальные RAR5/RAR7 decoder memory requests через ArchiveBackend → source GUI callback: Allow/Skip/Cancel/remember, persistence и root/selected/Test/batch consumers | P0-12; native RAR reader/decoder из P2-01 | done — audit § P1-18 implementation/acceptance; полный source handler/decoder/crypto, actual 2MiB/1MiB requests/flags, Allow/Skip/Cancel, whole/selected Extract/Test, batch remember/save/new-callback reuse, CRC failure, compressed RAR5 bytes/CRC/libarchive, MainWindow шесть overwrite modes/Test без console fallback, traversal/index-contract refusal. Четыре actual Mem JSON700/560 reviewed; baseline367 exact/164 adapted/3 local verified, 16,536 handler functional tokens identical. Полный unsandboxed check.sh formatting/all-target build/193 CTest PASS79.91s macOS arm64 Qt6.11.2 2026-10-02. Windows/Linux runtime/rendered Windows и дополнительные compressed RAR7/encrypted/solid/multivolume workflows unverified; whole iteration commit, no push |

## P2 — расширенные функции и платформенная интеграция

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [x] | P2-01 | Formats/codecs/capabilities, KeepName, Inside */# | P0-04, P1-05 | done — audit § P2-01 acceptance; complete source LoadCodecs/OpenArchive/default naming/eligibility, immutable leased catalog, async module/version discovery, metadata filters, seven typed writer transports. Native CFfpOpen worker/properties/password/cancel/leases, actual TGZ/Split extraction; source Add42success/failure/cancel plus MainWindow Add7formats success/cancel, Inside */#, missing/version mismatch read-only commands, bzip2 refusal/XZ empty-stream Delete/cancel and exact extensionless spool publication. Add normal760/compact640 JSON14 reviewed, native format/method restoration fixed. Full unsandboxed check.sh formatting/build/188 CTest PASS74.09s macOS arm64 Qt6.11.2 2026-10-02; baseline224/161/3 verified. Process item/extract/update/reopen integration explicit; P1-17/18 actual consumers remain mandatory, Windows/Linux runtime/rendered parity unverified. Whole iteration local commit, no helper commit/push |
| [x] | P2-02 | SFX и внешние handlers/plugins | P2-01, P1-06 | done — audit § P2-02 acceptance; source CompressDialog/Update/LoadCodecs, native SFX single/split/AES/Add/delete/pause/cancel, external codec/global-only facade/no-main Formats, immutable images/leases/assets and actual Windows junction cycle refusal. Official pinned runtimes compiled on all three OS; EN/RU SFX JSON reviewed. Final Windows11 ARM64 Qt6.8.3/MSVC14.44 clean build/focused7/7PASS13.76s; SFX/external zero skips, existing owned_temp symlink case deferred P3. Linux ARM64 Qt6.4.2/GCC13.3 focused9/9PASS2.79s zero skips. Full unsandboxed macOS arm64 Qt6.11.2 formatting/all-target build/197 CTest PASS107.31s, baseline372/164/3, 2026-10-03. Whole local iteration commit; rendered Windows parity/full P3/Finder remain open |
| [x] | P2-03 | Исходный CRC/Hash/Test workflow | P0-10, P1-10, P2-01 | done — audit § P2-03 acceptance; complete source PanelCrc/HashCalc/HashGUI, native Hash reader/writer/Test, original providers/method menus, filesystem scan/read and archive stream callbacks. Multiple methods/*, ordinary/flat selection, known digests, encrypted/wrong password/corrupt ZIP, unsafe manifest/error/cancel, modal lifetime/background, forged presentation paths verified. Hash results540/420 JSON reviewed. Full unsandboxed macOS arm64 Qt6.11.2 formatting/all-target build/199 CTest PASS79.48s; later localized fixes5direct consumers PASS2.79s. Linux ARM64 Qt6.4.2/GCC13.3 focused12/12PASS3.82s plus final5/5PASS3.28s, Windows11 ARM64 Qt6.8.3/MSVC14.44 build13targets/13CTestPASS13.04s, zero focused skips. Baseline374/165/3 verified,2026-10-03. Whole local iteration commit; rendered Windows/full P3/Finder remain open |
| [x] | P2-04 | Benchmark controls/results/lifecycle | P0-10, P1-14 | done — audit § P2-04 acceptance; complete source Bench/BenchmarkDialog, partial original CompressCall Benchmark entry, backend native providers/services, real Tools menu, source metrics/total console, memory refusal/error/Restart/Stop/Close/join, deferred MainWindow close and Help. Bounded native callbacks/GUI, deterministic fixtures and EN/RU normal/compact JSON checked. Full unsandboxed macOS formatting/build/202CTest PASS89.68s + final affected3/3PASS5.82s; Linux5/5PASS3.34s + final3/3PASS2.28s; Windows5/5PASS4.09s zero focused Qt skips, 2026-10-03. Baseline378 exact/171 adapted/3 local verified. Rendered parity/full platform acceptance remain P3 |
| [x] | P2-05 | Links/junctions и FS metadata | P1-02, P1-03 | done — audit § P2-05; source LinkDialog/CApp::Link, Windows privileges/raw FSFolder, native no-follow leaf Copy/Move/Delete и file/new-folder metadata, actual File menu/Cancel/selection, hardlink inode/bytes, privilege refusal, Linux real EXDEV; macOS full unsandboxed format/build/205CTestPASS82.95s, Windows7/7PASS30.24s и Linux7/7PASS0.15s zero focused skips,12EN/RU JSON reviewed, baseline382/173/3,2026-10-03. Whole local iteration commit; FinderP3-03.1 и rendered/full P3 остаются обязательными |
| [x] | P2-06 | ADS/streams/CProxyArc2 по platform capabilities | P1-10, P2-05 | done — complete AltStreamsFolder, source CCopyStateIO/Agent proxy2, native WIM writer и actual FS/archive Copy/Move/Create/Rename/Delete/Properties/Cancel consumers; audit § P2-06. macOS format/build/207CTestPASS120.58s; final mac2/2PASS4.43s, Linux3/3PASS3.21s, Windows drag/contract2/2 и семь Add cases1/1PASS1.39s zero Qt skips; prior VM core regressions12/14 Windows,16/17 Linux с исправленными focused failures. Sixteen EN/RU normal/compact JSON; baseline382/175/3 verified. Expanded Windows source_update_backend absolute/Sync failures/timeout сохраняются P2-07/P3; full Windows gate/rendered parity/Finder не заявлены. 2026-10-03 |
| [x] | P2-07 | Extract ElimDup/absolute/security/metadata | P0-12, P2-05, P2-06 | done — source bool pairs/persistence, bounded final-path/link policy, native extraction, directory Open Outside : 7-Zip, metadata Options/native TAR/WIM Add, security/ADS/reparse round-trip и processed deletion; audit § P2-07. macOS format/build/214CTestPASS81.10s + final4/4PASS1.45s/lifecycle1/1PASS1.28s; Windows13/13PASS33.56s + final lifecycle1/1PASS2.48s; Linux final13/13PASS10.15s. EN/RU normal/compact JSON reviewed, baseline382/175/3. Source Sync separators/first-ADS reopen fixed; Windows standalone ZIP/7z link request refused before engine start with preserved inputs. Linux historical full199/210 failures remain integration/P3 (creation fixture now passes); full Windows/Linux gates, rendered parity and Finder remain open. 2026-10-03 |
| [ ] | P2-08 | System page/file associations | P1-12.1, P2-01 | todo |
| [ ] | P2-09 | Shell integration и оставшаяся Menu page | P1-13.1, P2-08 | todo — проверка реальных policies/flags также закрывает P1-13.2; parent dependency не создаёт цикл |
| [ ] | P2-10 | Большие списки/операции без GUI blocking | P0-05, P0-06, P0-10, P1-09, P1-16 | todo |
| [ ] | P2-11 | Рефакторинг уже написанной обвязки и устранение дублирующей функциональной логики | P0-01, P0-02; проверка изменяемого scope после соответствующего feature owner | todo — инвентаризация собственного кода и C/C++ boundary, затем интегрированные итерации по компонентам; upstream C сохраняется, наша обвязка C++17. Критерии и границы — в плане; обязательный результат для P3-05/P3-06 |

## P3 — приёмка, обновления и поставка

| Готов | ID | Результат | После | Статус / evidence |
| --- | --- | --- | --- | --- |
| [x] | P3-01 | Reproducible baseline и проверенная update procedure | P0-01; maintained throughout | done — audit § P3-01; 192 exact/135 adapted/3 local manifest, independent disposable Git snapshot, synthetic old→new review one adapted+one exact file, `upstream_baseline` and `upstream_inventory`, `check.sh` 142/142 macOS arm64 2026-09-30; maintain manifest/audit for each later port; public remote fetch unverified offline |
| [ ] | P3-02 | Сквозная regression/security traceability | Принятые P0/P1 scenarios; relevant P2 rows; P1-17/18 для полной приёмки | todo — матрицу принятого scope можно собирать до P1-17/18 |
| [ ] | P3-03 | CI и реальные Windows/macOS/Linux checks | можно начать рано; final после P2 | todo — Windows focused source_update_backend теперь проходит после source path normalization, полный набор ещё не прошёл; Linuxfull199/210 с11 integration failures. Cross-platform final gate не принят |
| [ ] | P3-03.1 | Нативная проверка archive drag-out в Finder: реальные bytes, Unicode/пробелы, отмена, исходный архив и temp lifetime | P1-16; обязательна для приёмки P3-03/P3-06 | blocked — Computer Use отказал в доступе к sevenzip_desktop, native transfer не выполнялся; отдельный пункт по поручению пользователя 2026-10-02, synthetic/fake receiver tests не заменяют Finder proof |
| [ ] | P3-04 | Engine/Qt/resources/packages/licenses | P2-01, P2-02, P1-14, P1-15, P3-03 | todo |
| [ ] | P3-05 | Полная сверка omitted scope/dependency closure | Принятый scope P0–P2, включая P2-11, P3-01; P1-17/18 для полной приёмки | todo — аудит принятого scope можно начать до P1-17/18 |
| [ ] | P3-06 | Итоговая source/workflow/UX/platform приёмка | все обязательные пакеты | todo |

## Подшаги, блокировки и handoff

### P0-01 — инвентаризация исходного переноса

**done** — документационный source audit, 2026-09-28.
Baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
[Итоговый audit](upstream-port-audit.md#p0-01--accepted-source-inventory),
[полный исторический реестр модулей](history/p0-01-source-inventory.md).

| Принятый scope | Историческое evidence |
| --- | --- |
| 30 mirrored modules: original bodies, adaptations, omitted functions/dependencies | .1 `cfd02a5`, .2 `d03de90`; 16 + 14 modules |
| Все 82 отсутствовавших UI .cpp: source scope/callers, decision и feature owner | .3 final `415513c`; FM/Agent/GUI/Common |
| Полное покрытие 112 unique paths (109 UI + 3 supporting), provenance и baseline | .4 `36fad59`; 33/33 vendor bytes, path/coverage/diff checks на macOS |

Это inventory snapshot, не текущий счёт исходников и не закрытие функциональных
портов P0–P3. Приложение не менялось; app build/CTest/runtime/visual checks в
этом пакете не выполнялись. Исторические omissions сверять с более поздними
module records; глубокие ID не возвращать в очередь и не дробить дальше.

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

**done** — parent принят на macOS arm64 2026-09-29. Старое дерево helper ID
свёрнуто; его промежуточные active/todo не являются текущей очередью.
[Итоговый audit](upstream-port-audit.md#p0-02--accepted-owners-and-state),
[исторические source records](history/p0-02-source-port.md),
[текущие отличия UI](7zfm-parity.md#p0-02--owners-selection-and-lifetime).

| Результат | Статус и историческая приёмка |
| --- | --- |
| P0-02.1 — CAppState, history/favorites и persistence | done; commit `eedf926`, focused 5/5, full 54/54; macOS 2026-09-28 |
| P0-02.2 — CApp/CPanel Create/Save/Release, отдельные owners и focus/settings | done; startup success/failure/cancel, full 121/121 (85.30 с), 16 normal/compact JSON; macOS 2026-09-29 |
| P0-02.3 — operation guards, cancel/close/quiescence и полная parent приёмка | done; focused 8/8, full 122/122 (84.83 с); macOS 2026-09-29 |

Соседние scopes учитываются у своих результатов: navigation/parent stack/update
P0-04.1/.2; FS model P0-05; sort/columns/time/timer P0-06; selection P0-07;
two-panel P0-08; shell P0-09; progress/callback consumers P0-10.1/.2.
Повторно реализовывать их prerequisites или добавлять уровни к старым ID не нужно.

Незавершённые обязательные ветки сохраняют feature owners: editor/known-process
P1-08, language P1-14, raw/ADS/reparse P1-10/P2-05, shell P2-09 и remaining
engine/platform/packaging P3-05; точные omissions — в module audit.
Windows/Linux runtime и rendered Windows parity не проверены локально.
Счёт parent packages не меняется; это сводка прежней приёмки, не новые checks.

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
