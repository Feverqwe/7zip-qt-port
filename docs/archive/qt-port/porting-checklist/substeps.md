# Подшаги и handoff

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

[Правила выбора и счётчик](../porting-checklist.md).

## Подшаги, блокировки и handoff

P3-05.15,2026-10-07: integrated native ADS refusal, WIM F5 zone/streams and
bound filesystem Copy/Move publication are verified. Actual Root HRESULT,
ZoneId bytes, small/301-row provider and async focus REDs are retained. Worker
keeps original CFSFolder/GUI notification and COM ownership; per-pane reload
retains original OnCopy selection/focus continuation. Mac unsandboxed full
288/288PASS230.66s; fresh Windows11/11PASS164.12s and Linux10/10PASS43.52s,
269/270Qt with no focused skips;19 inputs match. Current geometry checks208
JSON/832enabled-exposed controls. Eleven slow suites and Windows VHD fixtures
remain excluded; whole .15/P3-05/native-full/rendered/package acceptance is open.
[Evidence](../upstream-port-audit.md#p3-0515--native-ads-root-source-zone-copy-and-bound-filesystem-reload-2026-10-07).

P3-05.15,2026-10-07: bounded native bound-archive Drop scenario is verified.
Actual terminal notify/focus REDs require transporting the source success-only
Refresh phase after synchronous guards, then completing list focus after rows.
Eight native cases and seven direct consumers pass on macOS; required default
full gate passes288/288 in240.89s. Fresh eight-consumer Linux/Windows runs pass
8/8 in40.82/159.98s,277Qt/0FAIL/0SKIP per OS; ten inputs match. Each OS has40
current native Drop reports checking160 enabled exposed controls at both sizes.
Writer error/Cancel/No/refusal policy
and ignored reload HRESULT stay source-owned; parent .15/P3-05 remain open.
[Evidence](../upstream-port-audit.md#p3-0515--native-bound-archive-drop-publication-2026-10-07).

P3-03,2026-10-07: bounded remaining Windows-consumer scenario is verified.
Native rollback RED requires source CAgent::Close before backup publication;
the backend invokes the close service supplied by its Agent-owning caller.
All128 Startup/192 Copy/82 Recovery cases retain their assertions and pass
current three-OS consumers. Native Windows controls/fonts and shared desktop
locking replace offscreen; measured aggregate budgets retain local state and
responsiveness deadlines. Current logs/manifests/normal-compact geometry are in
the audit. Stable/cold/full-platform/rendered/package and whole P3-03 acceptance
stay open.
[Evidence](../upstream-port-audit.md#p3-03--remaining-native-windows-consumers-2026-10-07-active).

P3-03,2026-10-07: native address hierarchy/UNC consumers pass after correcting
the popup mouse driver and filesystem icon fixture; product source unchanged.
Fresh Mac/Linux3/3 and Windows interactive/elevated4/4 pass, zero focused skips,
including native Parent. Current-build EN/RU normal/compact JSON and frozen
input hashes inspected. This closes address_combo/ui_archive_states/native
network failures from the earlier Windows default run; remaining default
failures and final platform acceptance stay active.
[Evidence](../upstream-port-audit.md#p3-03--native-address-popup-consumer-verification-2026-10-07).

P3-05.15,2026-10-07: changed native Parent reviewed against complete original
OpenParentArchiveFolder/CloseOneLevel/OpenParentFolder. Actual Windows spool
cleanup RED5 exposed FileTable's extra provider COM owner; Qt now retains only
a callback identity so source ReleaseFolder closes streams before cleanup.
Five representative2/301-row No/Cancel/Yes/writer error/actual progress Cancel
cases cover ignored publication errors, same-parent recovery, real F2/Parent
retry, byte preservation, child focus and filesystem OnTimer. Shared test
receipts normalize CRLF; payload bytes stay exact. Mac focused9/9PASS14.20s,
required unsandboxed full287/287PASS245.00s; Linux same-snapshot9/9PASS19.40s,
213Qt0fail0skip;24JSON/96 controls per OS and nine matching SHA inspected.
Windows same-snapshot Parent7QtPASS and all seven direct consumers187QtPASS;
24JSON/96 controls and nine input hashes inspected. Expanded9/10 suites retain
one address icon fixture failure, handled separately under P3-03. Eleven slow
suites excluded. Earlier Windows default failures remain P3-03 investigation;
this accepts only the bounded disk-child Parent consumer.
Native stream/memory update routes, Drop, complete body/resource/UX closure,
packages/cold stability/rendered
reference and P3-06 remain open.
[Scope/evidence](../upstream-port-audit.md#p3-0515--changed-native-nested-archive-parent-2026-10-07).

P3-05.15,2026-10-07: native F2 Rename сохраняет единственного владельца worker
ошибок — исходный progress dialog. Лишнее Qt-сообщение поймано actual RED2;
ранняя validation и unconditional ignored-result publication сохранены.
Actual native proxy/status SIGSEGV и deterministic timer/guard RED исправлены:
Qt periodic traversal/status reads учитывают исходные notification/status guards.
Нативная матрица2/301 покрывает запись/ошибку/реальную отмену, publication
HRESULT, Ctrl+R и повторный F2 с marks/provider/peer/bytes/guards.
Mac final unsandboxed full296/296PASS1395.53s,6512QtPASS/0fail/
15Windows-onlySKIP. Обе native матрицы195QtPASS/0fail/0skip каждая;
проверены2048 fresh normal/compact JSON и8192 enabled/fully exposed controls,
13 frozen inputs/хеши отчётов совпали после gate. Parent/P3-06 остаются active;
native Drop/changed-parent, полная сверка тел и platform/rendered приёмка открыты.
[Scope/evidence](../upstream-port-audit.md#p3-0515--native-rename-publication-and-error-ownership-2026-10-07).

P3-05.15, 2026-10-07: source OnCreate владеет initial/late panel publication;
reload service устанавливается вместе с контролами, constructor/toggle render
удалены.256 creation scenarios FS/transport/native ZIP/7z,2/301 rows,
both roles/EN-RU/table-icon/standard-alternative и original initial HRESULT policy.
128 startup/late-peer cases дополнительно проверяют requested main path,
original main choice отдельно от live projection, delayed initial focus и приоритет
peer input/public archive API с восстановлением address focus после modal progress.
Немедленное раскрытие saved peer не допускает focus completion внутри его Create.
Source panel0 left/panel1 right восстановлен при любом creation order и проверяется
в обеих normal/compact матрицах.
Final Mac arm64/Qt6.12: focused11/11PASS59.38s, unsandboxed full287/287PASS358.65s;
21SHA stable,768fresh JSON/3072 visible controls inspected. Current native/final
acceptance remains open.
[Evidence](../upstream-port-audit.md#source-panel-creation-publication-2026-10-07).
Статус active; native renderer-failure/full-body и остальные native/final
boundaries обязательны.

P3-05.15, 2026-10-07: integrated failed rollback/reopen и восстановление проверяются
отдельным interarchive_recovery:80 EN/RU both-pane/view/selection scenarios,
native ZIP/transport restore failure,301-row filesystem fallback, reopen
error/engine Cancel и реальный F5 после ремонта временного назначения.
[Evidence](../upstream-port-audit.md#interarchive-failed-rollback-and-recovery-2026-10-07).
Mac focused1PASS19.09s/full285PASS313.75s;82Qt0skip,320 current reports,
all10 frozen inputs stable. Validation-only scope; product implementation unchanged.
Статус active; operation renderer/OnCreate/full-body и native/final boundaries
остаются обязательными.

P3-05.15, 2026-10-07: interarchive Copy/Move публикует тот же bound destination
через source RefreshListCtrl с pre-update selection; physical rollback сохраняет
необходимый ReOpen и получает тот же saved state.
[Evidence](../upstream-port-audit.md#interarchive-copy-publication-and-rollback-2026-10-07).
Mac focused6PASS113.65s/full284PASS292.34s,256 transfer rows/512 reports;
all11 frozen inputs stable, full evidence in audit.
Статус active; failed rollback/reopen проверен отдельным компонентом выше,
остальные native/final boundaries обязательны.

P3-05.15, 2026-10-07: filesystem-to-archive Copy/Move публикует bound destination
через source RefreshListCtrl с pre-update selection вместо второго ReOpen.
[Evidence](../upstream-port-audit.md#filesystem-to-archive-copy-publication-2026-10-07).
Mac full283PASS205.12s;192 workflow rows/384 reports, all frozen inputs stable.
Статус active; interarchive/rollback и остальные native/final boundaries обязательны.

P3-05.15, 2026-10-07: source RefreshListCtrl подключён к load/publication перед
исходным selection tail; Qt restoration вызывает отдельный narrow adapter.
Удалён compensating all-visible ReOpen после drag/drop; CopyFromNoAsk сохраняет
original success-only reload. [Evidence](../upstream-port-audit.md#source-refresh-and-dragdrop-publication-2026-10-07).
Статус active; остальные source/renderer/native3OS/final boundaries обязательны.

P3-05.15, 2026-10-07: original CApp::RefreshView восстановлен для всех providers;
manual archive Refresh/Ctrl+R больше не вызывает independent ReOpen.
[Evidence](../upstream-port-audit.md#source-bound-archive-refresh-command-2026-10-07).
Статус active; остальные operation/renderer/source/native3OS/final boundaries
остаются обязательными.

P3-05.15, 2026-10-07: external OnOpenItemChanged completion публикует original
unconditional Refresh после попытки записи, включая failure/Cancel; ранние
refusal сохраняются. Optional saved selection отделён от update bool.
[Evidence](../upstream-port-audit.md#source-external-update-refresh-completion-2026-10-07).
Статус active; остальной Refresh/native3OS/final scope остаётся обязательным.

P3-05.15, 2026-10-07: UpdateSelection восстановил original item-change guard;
callback text остаётся доступным во время Qt selection/mark signals. Внешний
notify guard, исходные marks, обе панели EN/RU table/icon standard/alternative
и compact geometry покрыты регрессией.
[Evidence](../upstream-port-audit.md#source-selection-projection-notification-scope-2026-10-07).
Статус active; остальные full-body/native3OS/final acceptance обязательны.

P3-05.15, 2026-10-07: Refresh load/publication/selection сохраняют original
written-count policy; Parent и post-Copy/Move count HRESULT больше не трактуется
как load failure. 72 EN/RU both-pane detail/icon consumers, lifecycle/query-count
и существующие operation-error/Cancel consumers покрывают компонент.
[Evidence](../upstream-port-audit.md#source-refresh-loaded-count-consumers-2026-10-07).
Статус active; full-body/native3OS/final acceptance остаются открытыми.

P3-05.15, 2026-10-07: source OnNotifyActivateItems подключён к Enter и
двойному щелчку table/icon с исходной политикой всех Ctrl/Shift/Alt сочетаний.
256 real FS/ZIP EN/RU both-pane сценариев, notify guards, 16 folder-error/recovery
consumers и normal/compact geometry покрывают этот компонент.
[Датированное evidence](../upstream-port-audit.md#source-item-activation-notification-consumers-2026-10-07).
Статус остаётся active: полный модуль и current native3OS/final acceptance открыты.

P3-05.15, 2026-10-07: native archive publication сохраняет исходную политику
OnReload против ignored OnCreate/OpenItem Refresh; guards/redraw восстанавливаются
до сообщения и продолжения Copy. Внешний filesystem OpenItem передаёт свои
guards внутреннему open/Refresh; последующий OnTimer остаётся рабочим.
288 native failure/retry,96 real Copy/Move и4 cancellation/replacement workflows,
1540 fresh geometry reports и current Mac focused13/full289 PASS покрывают этот
компонент. [Evidence](../upstream-port-audit.md#p3-0515--native-list-publication-result-and-open-guards-2026-10-07).
Статус active: nested OpenItemInArchive, remaining native operation renderers,
full-body/current3OS/rendered/cold-stable-package/final acceptance остаются открытыми.

P3-05.15, 2026-10-07: следующий native nested компонент восстановил original
Agent::GetStream и stream/memory/disk guard handoff в OpenItemInArchive.
384 publication/recovery workflows,18 actual Cancel Yes/No/Cancel consumers
и native TAR normal/flat/child mapping/stream lease проверены в трёх
`native_nested_*` suites. Current Mac: focused3+12 CTest PASS, required full292/292
PASS629.29s,1572 fresh geometry reports; Windows-only15 Qt skips сохранены.
[Scope/evidence](../upstream-port-audit.md#p3-0515--native-nested-streams-and-publication-handoff-2026-10-07).
Статус active: оставшиеся full-body/operation-render/native3OS/rendered/
cold-stable-package/final acceptance обязательны.

P3-05.15, 2026-10-07: native external-editor update verification добавила384
реальных F4/CopyFromFile workflows: official writer success/Write E_FAIL/source
Cancel, native property publication failures, source marks/peer focus, Ctrl+R,
same-provider recovery и actual F4 writer retry. Матрица2/2PASS243.24s,
388QtPASS/0fail/0skip; unsandboxed full294/294PASS883.61s,
6122QtPASS/0fail/15Windows-onlySKIP. Проверены2048 fresh normal/compact JSON
и8192 enabled/fully exposed controls; входные файлы и хеши отчётов совпали
после завершения gate. P3-05.15 и итоговая приёмка остаются открыты.
[Scope/evidence](../upstream-port-audit.md#p3-0515--native-external-editor-update-publication-2026-10-07).
Production bodies unchanged; native rename/drop/changed-parent render consumers,
full-body/native3OS/rendered/cold-stable-package/P3-06 остаются обязательными.

P3-05,2026-10-07: toolbar Extract/Test подключены к исходным CApp/CPanel owners.
Archive Extract использует OnCopy; файловая панель — source batch ExtractGUI/Test.
Восстановлен ArcPath и исходная доступность ElimDup; удалён независимый hint.
Реальные Cancel/success/failure/empty archive, оба pane, focus, EN/RU и standalone
GUI consumers проверены. Mac required full289/289PASS231.40s; Windows native
interactive/elevated17/17PASS82.99s; Linux17/17PASS13.83s. Проверены60 fresh JSON
и696 controls, frozen inputs совпали с обеими VM. Slow suites/VHD fixtures
исключены; original Win32 rendered reference и whole P3 остаются открыты.
[Source/evidence](../upstream-port-audit.md#p3-05--toolbar-extracttest-source-consumers-2026-10-07).

P3-05,2026-10-07: восстановлен полный active CPanel::OnKeyDown и source
PanelMenu EditCopy/Cut/Paste; Qt input adapter сохраняет порядок modifiers,
bookmarks, сортировки, выбора, callbacks и posted Close. Исправлены реальные
Ctrl+C/Ctrl+Shift+A/Ctrl+W regressions; проверены оба pane, четыре view,
EN/RU, normal/compact и guard recovery. Mac full290/290PASS233.42s,
4473Qt/0fail/15Windows-onlySKIP; Linux focused10/10,244Qt/0fail/0skip,
и свежие File/View/Edit consumers8/8,132Qt/0fail/0skip.
Windows application+11 consumers собраны; native keyboard execution ожидает
входа в desktop гостя. Это проверенный ограниченный компонент; whole source,
UX/platform/packages/P3-06 и итоговая приёмка остаются открыты.
[Source/evidence](../upstream-port-audit.md#p3-05--source-keyboard-dispatcher-2026-10-07).

P3-05,2026-10-07: production opening теперь проходит исходные CAgent::Open и
CArchiveFolderManager::OpenFolderFile; восстановлены IInFolderArchive,
S_FALSE diagnostics, binding progress/Cancel и retained codec/stream lifetime.
Actual old-consumer RED сохранён. Mac full291/291PASS283.86s;
Linux focused12/12PASS29.77s; Windows core8/8PASS12.17s, без desktop/GUI claim.
Slow/VHD suites исключены; весь P3 и итоговая приёмка остаются открыты.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-agent-opening-and-archive-folder-factory-2026-10-07).

P3-05,2026-10-07: source Agent Close/failed-ReOpen/repair/rebind и binding
Cancel/retry проверены на ZIP/WIM; восстановлены LoadItems link-open guard и
ReadItems allocation/RAM/metadata/load order, исходная written-count policy.
Actual RED9 сохранён. Mac full292/292PASS289.45s, fresh focused17/17;
тот же source snapshot Linux17/17 и Windows core9/9. Все899 frozen inputs
совпадают с текущими файлами и runner tar. Whole-module/native GUI/rendered/
package/final scope остаётся открытым.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-agent-provider-lifecycle-and-metadata-binding-2026-10-07).

P3-05,2026-10-07: source directory CRC property projection восстановлена
для path proxy с исходными aggregate/explicit-record/error/local-variant
правилами. Real ZIP/7z list/Properties/Sort/Refresh/marks, EN/RU, обе панели,
normal/flat и unchanged bytes проверены; actual RED15 сохранён.
Mac full293/293PASS274.47s, focused6/6; same-source Linux10/10 и Windows core5/5
без required skips. Все902 snapshot inputs совпали в tar и обоих гостях;
по96 текущих geometry JSON проверено на Mac/Linux. Whole module/native GUI/
rendered/packages/P3-06 остаются открытыми.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-archive-directory-property-projection-2026-10-07).

P3-05,2026-10-07: source Agent CopyFrom into archived ADS restores the original
item loop/directory refusal and Windows filesystem-colon-prefix guard; a narrow
Unix callback metadata adapter enables the official WIM writer. Actual RED3;
Mac full294/294PASS253.75s, Linux7/7 focused, Windows5/5 unchanged retry after
one retained codec-catalog test timeout. Source/worker bytes/error/Cancel and
existing ADS UI consumers pass; whole-module/native desktop/rendered/package
and final acceptance remain open.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-agent-alternate-stream-updates-2026-10-07).

P3-05, 2026-10-07: actual F5/F6 FS/ZIP→archived ADS consumer restored source
OnCopy's notification guard during target activation after CopyFrom/rollback.
Old proxy-index crash reproduced; eight EN/RU both-pane workflows cover Cancel,
Move/Delete Cancel rollback, saved marks/focus/providers and independently
extracted bytes. Mac full295/295PASS277.65s; same-source Linux/Windows focused5/5,
all904 snapshot inputs match actual guest trees. Each OS's88 normal/compact
JSON pass contracts. Seven Agent bodies/headers, FAR/desktop wrappers and
dependencies now have explicit source disposition; native desktop/rendered/
packages and parent P3 acceptance remain open.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-copymove-publication-after-agent-updates-2026-10-07).

P3-05, 2026-10-07: five complete GUI worker/dispatcher/callback bodies and their
headers/callers/dependencies reconciled. Missing No Update Engines and bypassed
source question suppression in both Extract early path errors restored through
the original MyMessages owner's Qt presentation. Actual old-product RED retained;
12 EN/RU error/suppression/recovery rows and36 current message JSON across three
OS pass. Mac full296/296PASS339.25s; Linux/Windows seven direct consumers7/7,
all906 frozen inputs match actual guests. Other dialogs/common/native/package
and parent acceptance remain required;43/47 unchanged.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-gui-early-error-policy-2026-10-07).

P3-05, 2026-10-07: full ExtractDialog/HashGUI body/header/resource disposition
and CompressDialog path/history consumers reconciled. Original UString Trim,
FileName/FileDir resolution and shared history comparison restored; real EN/RU
Create/list/Extract with SplitDest, Escape, persisted NBSP paths and exact bytes
passes. Old-product six-failure RED retained. Mac full297/297; final nine direct
consumers9/9 on Mac/Linux/Windows,907 frozen inputs match actual guest trees.
All96 new normal/compact JSON pass current geometry checks. Whole CompressDialog,
BenchmarkDialog/common/native/rendered/package acceptance remains required;
43/47 and P3-03/04/05/06 remain open.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-gui-path-and-history-consumers-2026-10-07).

P3-05, 2026-10-07: source Create validation transaction/error policy reconciled:
disabled-password checks, unknown-RAM fallback limit, SaveOptionsInMem/path/volume
ordering and source suppressible versus unconditional messages. New50-result
regression covers refusal, real writer recovery, EN/RU and normal/compact geometry.
Mac full298/298; final12 direct consumers pass on Linux/Windows without skips,
908 frozen inputs match actual guest trees and168 new message JSON pass geometry.
Whole CompressDialog profile/default/clamp selection, BenchmarkDialog/common,
native/rendered/package acceptance remain;43/47 and P3-03/04/05/06 remain open.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-create-validation-error-policy-2026-10-07).

P3-05,2026-10-07: source compression profile selection restored inside original
dictionary/order/solid loops, including large markers, nearest/below-minimum
values, BZip2 decimal comparison, solid boundaries and unavailable explicit
threads. New regression proves old-product20 failures, Cancel/restart without INI
mutation and real accepted Create/list/test/extract bytes. Mac full299/299;
final14 direct consumers pass on Linux/Windows,909 frozen inputs match actual
guest trees and108 new EN/RU normal/compact JSON pass geometry. Whole Compress
Copy/unknown-method/header-service reconciliation, Benchmark/common/native/
rendered/package acceptance remains;43/47 and P3-03/04/05/06 remain open.
[Scope/evidence](../upstream-port-audit.md#p3-05--source-compression-profile-selection-2026-10-07).

P3-05,2026-10-07: source numeric method/control states reconciled for Store,
Copy, unknown/external methods, TAR and ordinary compression. Source empty/Auto
lists, dictionary sentinel/zero, memory visibility and Store thread policy reach
Qt controls and backend validation. Actual RED15 failures; real 7z/ZIP Store
Create/list/test/extract, refusal, Cancel and unchanged bytes/settings pass.
Mac full300/300; final Linux/Windows15 direct consumers each262 Qt passes without
failures/skips. All910 frozen inputs match local/archive/actual guests;84 new
EN/RU normal/compact JSON pass geometry. Existing Windows encryption geometry
fixture now processes pending layout before scrolling, retaining all assertions.
Whole-module final reconciliation, Benchmark/common/native/rendered/package
acceptance remains;43/47 and P3-03/04/05/06 remain open.
[Scope/evidence](../upstream-port-audit.md#p3-05--compression-method-and-control-states-2026-10-07).

P3-05,2026-10-07: complete BenchmarkDialog body/header/resource/caller disposition
records original metrics/worker/lifetime owners and Qt platform presentation.
POSIX affinity now copies source normalization; detached completion retains
source successful no-op. Actual visible fault RED2 and detached RED1 retained.
Mac full300/300; Linux/Windows3/3 consumers,22/20 Qt passes and0/2 explicit POSIX
fault skips;24 current normal/total EN/RU JSON pass geometry. All910 frozen inputs
match local/archive/actual guests. Compress final event/header, common modules,
native/rendered/package and P3-03/04/05/06 remain open;43/47 unchanged.
[Scope/evidence](../upstream-port-audit.md#p3-05--benchmark-source-and-platform-lifecycle-2026-10-07).

### P0-01 — инвентаризация исходного переноса

**done** — документационный source audit, 2026-09-28.
Baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
[Итоговый audit](../upstream-port-audit.md#p0-01--accepted-source-inventory),
[полный исторический реестр модулей](../history/p0-01-source-inventory.md).

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
[Итоговый audit](../upstream-port-audit.md#p0-02--accepted-owners-and-state),
[исторические source records](../history/p0-02-source-port.md),
[текущие отличия UI](../7zfm-parity.md#p0-02--owners-selection-and-lifetime).

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
