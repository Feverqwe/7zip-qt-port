# Сверка интерфейса с оригинальным 7zFM

<a id="current-final-acceptance-matrix-2026-10-10"></a>

## Current final acceptance matrix (2026-10-17)

### Resource/workflow reconciliation after Compression Options

At `7a8592f`, the desktop includes in the pinned FileManager `resource.rc`,
GUI `resource.rc`/`resource2.rc`/`resource3.rc`, menu groups and toolbar callers
are reconciled with the component receipts below. Source bodies and dependency
dispositions remain owned by the [existing ledger](omitted-scope-audit/ledger.md);
this table maps acceptance evidence, rather than creating another source inventory.
String-only property resources and CE/legacy/standalone entrypoints retain their
ledger dispositions. A receipt checks its stated states and revision; its mere
existence does not make every current workflow or platform checked.

| Applicable scene | Existing source/native/reference evidence | Remaining boundary |
| --- | --- | --- |
| File/Edit/View/Favorites/Tools/Help; archive/standard toolbars; one/two panels | [Main chrome](upstream-port-audit/p3-06-main-chrome-2026-10-16.md), [native shell](upstream-port-audit/p3-06-native-shell-2026-10-16.md), [address/Root](upstream-port-audit/p3-05-address-root-2026-10-17.md) | Qt action/state and native widget checks are distinct from Cocoa global-menu OS input. The later original main-window reference supersedes the shell receipt's missing-reference statement. |
| Copy/Move destination, both panes and publication | [24 runtime pairs](upstream-port-audit/p3-06-copy-runtime-2026-10-17.md), [native publication](upstream-port-audit/p3-05-copy-native-2026-10-17.md), [source closure](upstream-port-audit/p3-05-source-closure-2026-10-17.md) | Declined expanded `copy_archive_refresh` matrix remains unverified; accepted smaller/eleven-long contracts are retained. |
| Add, Extract and child metadata Options | [Add/Extract reference](ui-layout-contracts/shared.md#rendered-addextract-reference-p3-06-2026-10-15), [Add gutter/manual check](upstream-port-audit/p3-06-add-scroll-2026-10-15.md), [Compression Options](upstream-port-audit/p3-06-compression-options-2026-10-17.md) | Latest Options component has current native three-OS coverage and refreshed packages. Existing original initial views do not cover every conditional archive-format state. |
| Overwrite, Progress and operation Messages | [Rendered reference](upstream-port-audit/p3-06-2026-10-15.md), [shared modal consumers](upstream-port-audit/p3-06-modal-geometry-2026-10-17.md), [prompts](upstream-port-audit/p3-06-source-prompts-2026-10-16.md) | Retain the documented fonts/minima/button-order adaptations and separate actual operation/error/Cancel coverage. No pixel-equality claim. |
| Password, Memory and Edit prompts | [Original focus/defaults and native consumers](upstream-port-audit/p3-06-source-prompts-2026-10-16.md), [widget workflows](upstream-port-audit/p3-03-widget-dialog-fixtures-2026-10-16.md) | Qt widget responders do not establish native NSAlert acceptance; the user's accepted message scenes retain their exact scope. |
| Browse and temporary files | [Original populated views, keyboard probes and native three-OS consumers](upstream-port-audit/p3-06-source-browsers-2026-10-16.md) | Native folder/save OS panels use the separately accepted manual scenes; Xvfb evidence is not a physical desktop observation. |
| Shared input, Split/Combine destination and About | [Original EN/RU views, source callbacks and native consumers](upstream-port-audit/p3-06-primitives-2026-10-16.md) | Composite owner checks retain their revisions; they are not a new whole-product gate. |
| Properties/History lists, Link and normal/total Benchmark | [Ten original states and three-OS component checks](upstream-port-audit/p3-06-secondary-dialogs-2026-10-16.md) | POSIX Junction/WSL capability differences and native modal chrome remain explicit. |
| Six Options pages, Apply/Cancel, language and keyboard | [Frame](upstream-port-audit/p3-06-options-2026-10-16.md), [keyboard](upstream-port-audit/p3-06-options-keyboard-2026-10-17.md), [footer/manual Control](upstream-port-audit/p3-06-options-control-2026-10-17.md), [live System](upstream-port-audit/p3-06-system-live-2026-10-17.md), [GIO workflow](upstream-port-audit/p3-06-system-native-2026-10-17.md), [native mutation/opening](upstream-port-audit/p3-06-native-associations-2026-10-17.md) | Windows unique-extension HKCU mutation/recipient and manual Mac ZIP assignment/restoration are checked separately. HKLM/protected UserChoice and corrected packaged Finder opening remain outside that acceptance. |
| External application/desktop integration | [Finder acceptance](porting-checklist/p3.md), [Explorer transfer/Escape](upstream-port-audit/p3-03-native-drag-fixtures-2026-10-16.md#physical-windows-explorer-recipient-2026-10-17), [editor completion](upstream-port-audit/p3-03-editor-stability-2026-10-16.md), [source closure](upstream-port-audit/p3-05-source-closure-2026-10-17.md) | Keep OS-recipient, command-dispatch and synthetic-widget evidence separate. Unrelated dialog changes do not invalidate unchanged accepted Finder transport. |

No additional unpaired desktop dialog family is identified by this reconciliation.
It does not close P3-06: installed registration/consent and the remaining complete
native workflow acceptance still need their stated evidence, while Windows cold
discovery remains P3-03.8. Existing passing component checks need not be repeated
solely to replace an older generic “full audit open” sentence. A new implementation
iteration requires a concrete source gap or observable failure; a stronger auxiliary
oracle alone does not expand mandatory scope. The three parent packages and44/47
count remain open until their actual prerequisites are accepted.

### Current receipts

[Native association workflow](upstream-port-audit/p3-06-native-associations-2026-10-17.md)
исправляет Windows registry command (`/` давал ShellExecute error5) и macOS
file-open во время discovery (`E_FAIL`). Реальные HKCU Apply/Clear/recipient и
public native window workflow проходят; user ZIP assignment/restoration принят
отдельно от failed opening на пакете7a8592f. Исправление требует обновления
пакетов и повторной проверки этого конкретного Finder workflow.

[Current Compression Options delivery](upstream-port-audit/p3-04-compression-options-delivery-2026-10-17.md)
packages product7a8592f on three OS:1056 exact inputs, Release/OFF,
relocation15/14/14, payload/licenses/SFX and fresh dependency/private-ABI checks
PASS. This supersedes the package-refresh prerequisite in the older component
receipts below. Their remaining cold/native boundaries are unchanged.

[Add Options presentation/keyboard](upstream-port-audit/p3-06-compression-options-2026-10-17.md)
закрывает найденный resource-frame/colon/focus gap дочернего COptionsDialog.
Actual original8states и source first-Tab сверены с native3OS; focused5/5,
95Qt и native14Qt PASS на каждой ОС. Windows raw comparison8/8; POSIX сохраняет
24 ожидаемых Windows-only state differences, без иных расхождений.
Current packages7a8592f include this correction; общий native/cold scope открыт.

[Current default gates](upstream-port-audit/p3-03-final-default-2026-10-17.md)
на product2a7303c: Mac329/330, Linux331/332 и Windows Limited331/348 executed
PASS; исходные failures сохранены. Единственный Mac/Linux сбой — хрупкая
проверка preferred button width; исправленный caption-fit fixture проходит
default1/1 и native8Qt на каждой ОС. Все16 Windows privilege/setup failures
проверены отдельно: scoped administrator16/16,229Qt/0fail/0skip.
Codec/pause consumers проходят в текущем whole run, но причина
исторических cold failures и итоговая приёмка остаются открытыми.

[Options footer/Cocoa Control routing](upstream-port-audit/p3-06-options-control-2026-10-17.md)
исправляет два actual keyboard gaps по original footer reference: окно теперь
переключает страницы из нижних кнопок и принимает physical-Control Qt events
на Mac. RED→GREEN на3OS; native27/23/23Qt PASS. Пользователь затем подтвердил
оба physical Control направления из поля и footer на текущей сборке2a7303c.
Полный native/cold audit остаётся открытым; коррекция включена в текущие
[packages2a7303c](upstream-port-audit/p3-04-options-delivery-2026-10-17.md).

[Предыдущая delivery40b3439](upstream-port-audit/p3-04-native-icons-delivery-2026-10-17.md)
обновляет три частных Release packages после System/app icon corrections,
relocation15/14/14 и fresh dependency/payload/license/native PE checks PASS.
Mac ICNS bytes exact; strict raw/native pixel probe FAIL с native tile/shadow
сохранён отдельно, не переименован в pixel PASS. Cold/full native scope открыт.

[Source application icon](upstream-port-audit/p3-06-application-icon-2026-10-17.md)
восстанавливает исходный FM.ico у Qt app/window и в Windows/macOS native
артефактах. Null-icon RED→GREEN, shown-window native3OS и decoded PE/AppKit
checks PASS; privileged Windows startup проверен отдельно от Limited setup
failures. Полный native/cold обход и обновление packages остаются открытыми.

[Live System values and macOS bundle image](upstream-port-audit/p3-06-system-live-2026-10-17.md)
добавляют40 actual catalog rows в четырёх native состояниях Windows/macOS,
pending indexed/bundle icon и Cancel с неизменными native revisions. Исправлен
значок Own на macOS; regression RED4→GREEN, Cocoa6Qt/12JSON, Options10/10 и
associations4/4 PASS. Independent original Windows40-row EN/RU inventory совпадает
с четырьмя Qt состояниями по всем колонкам/порядку без отличий. Installed mutation/consent и
общий native/cold scope остаются открытыми; поставка требует обновления.
Follow-up native image oracle также проверяет все40 initial images на Windows/
macOS × четыре состояния:18 registered images и22 empty per state,0diff;
System registration, app branding и screenshot pixel parity этим не принимаются.

[Native Linux System icons](upstream-port-audit/p3-06-system-native-2026-10-17.md)
дополняют проверенный scope реальными GIO file/theme images и source
pending/Apply/Cancel/external-conflict workflow в private XDG: native12Qt/28JSON,
включая actual Options Critical/OK и сохранённые внешние preferences. Пять
неизменённых native Options suites проходят на3OS по69Qt/86JSON; последующий
Windows/macOS read-only scope принят отдельно выше, общий native/cold walkthrough открыт.
Пакеты09a6f6d ниже ещё не включают эту source-коррекцию.

[Copy/Move runtime comparison](upstream-port-audit/p3-06-copy-runtime-2026-10-17.md)
добавляет actual original EN/RU initial/compact и24/24 native Qt pairs на3OS,
исправление узкой Browse20DLU и по6 focused suites/101Qt плюс native14Qt/24JSON.
Проверены declared structure/state/fit и отдельный keyboard workflow; XCB —
Xvfb/Openbox. Pixels/physical input/OS panels и остальные parent claims ниже
этой ограниченной приёмкой не закрываются.

[P3-05 source/integration closure](upstream-port-audit/p3-05-source-closure-2026-10-17.md)
завершена по актуальным body/caller receipts, inventory и focused3OS consumers:
счёт44/47. Это не меняет partial/unverified native/rendered/cold claims ниже.

[Source address/Root namespace contracts](upstream-port-audit/p3-05-address-root-2026-10-17.md)
приняты по текущим3OS consumers и Windows device/ADS/UNC, обеим панелям и EN/RU.
Исправлены shell attributes и потеря namespace в Back;48normal/compact address
reports/crops проверены с оригинальными closed-editor видами. Это bounded
приёмка P3-05.12/.13; полный native/rendered/cold scope ниже остаётся открытым.

Матрица различает текущие native launcher и delivery components и прежние full/native receipts.
Checked означает только названную
проверку; partial/unverified не дают итоговой приёмки P3-06.
[Windows whole17fab0f](upstream-port-audit/p3-03-windows-whole-2026-10-17.md)
завершён327/343 executed PASS,16FAIL2298.39s:15 privilege/setup suites и
actual external-codec timeout5215ms/5000ms. Probe18QtPASS, source/CMake restore,
stock rebuild и1047product/1472compiled inputs verified; cold/final приёмка открыта.
[Current Windows native Copy/publication](upstream-port-audit/p3-05-copy-native-2026-10-17.md)
дополняет матрицу:9/9 suites,310Qt/0fail/0skip,800JSON; ADS/zone и actual
archive/peer failure/Cancel/recovery consumers проверены на17fab0f.
Clean-OS проверки Windows/macOS/Linux исключены из текущей приёмки по явному
решению пользователя2026-10-15. Это waiver, а не свидетельство прохождения;
package/relocation/native/rendered требования остаются в scope.
Текущие source-коррекции [Agent lifetime](upstream-port-audit/p3-05-proxy-lifetime-2026-10-17.md)
и [worker deadline](upstream-port-audit/p3-03-codec-deadline-2026-10-17.md)
проверены с actual RED/public regressions. Product7dadf6d имеет current Mac/Linux
default gates и свежие bounded packages; tests83163a4 исправляют только native
fixtures и aggregate budgets. Все11long на83163a4 проходят на macOS/Linux:
2100Qt/0fail/0skip на каждой ОС. Windows Limited fullv90 завершён325/342
executed PASS,17FAIL2462.20s:15 privilege/setup suites и2 cold codec failures
(6800/7200ms при исходном5000ms). Windows all11 replay ещё не завершён;
scope и evidence — в [длительном receipt](upstream-port-audit/p3-03-long-acceptance-2026-10-16.md).
Последующий [общий CDialog/Copy компонент](upstream-port-audit/p3-06-modal-geometry-2026-10-17.md)
проверен полными Mac325/325 и Linux324/324 gates, текущими Cocoa/XCB consumers
и22 выполненными Windows focused suites. Последующий Windows focused8/8PASS106Qt
проверяет отсутствовавший Progress fixture; все11long PASS на3OS по2100Qt.
ScopedHighest15split и исправленный Link16Qt также проверены отдельно;
current Windows full/cold/native остаются открытыми. [Пакеты17fab0f](upstream-port-audit/p3-04-2026-10-16.md#delivery-after-shared-modal-geometry-17fab0f)
проверены на3OS с1047 exact inputs и relocation15/14/14.
Эти изменения не заменяют оставшийся полный native/rendered обход и не
устанавливают причину исторических Windows cold delays.
[Physical Windows Explorer Copy/Escape](upstream-port-audit/p3-03-native-drag-fixtures-2026-10-16.md#physical-windows-explorer-recipient-2026-10-17)
теперь проходят на17fab0f с точным Unicode/space именем, bytes и неизменным
архивом; это отдельный проверенный native сценарий, не полная приёмка.
Overwrite/font-DLU коррекция проверена как отдельный компонент:
[rendered reference, регрессии и gates](upstream-port-audit/p3-06-2026-10-15.md).
Текущий продукт1f5e16a также содержит [Add gutter correction](upstream-port-audit/p3-06-add-scroll-2026-10-15.md):
native Cocoa/Windows и Linux offscreen consumers, full macOS и ручная macOS
проверка пользователя приняты в указанном scope. [macOS пакет1f5e16a](upstream-port-audit/p3-04-2026-10-15.md#p3-04--add-edge-gutter-macos-delivery-refresh-2026-10-15)
и Windows/Linux пакеты1f5e16a обновлены и проверены. Прежние packages/full
receipts сохраняют свои исходные ревизии и не подтверждают текущий вариант.
Последующая [Options source-frame коррекция](upstream-port-audit/p3-06-options-2026-10-16.md)
проверена focused/native на3OS, с полным macOS gate и rendered original six-page
reference. [Три свежих пакетаd20d50d](upstream-port-audit/p3-04-2026-10-16.md)
также проверены в bounded delivery scope. [Windows phase diagnosis](upstream-port-audit/p3-03-codec-timing-2026-10-16.md)
не устанавливает причину cold failures и не закрывает стабильность.
Последующий [Split/shared input/About компонент](upstream-port-audit/p3-06-primitives-2026-10-16.md)
проверен source/rendered и focused/native на3OS: macOS full323/323 PASS264.06s
до отдельно проверенных локальных правок; Windows/Linux current-owner composite
по32Qt/0fail/0skip. [Свежие2fcc0e1 packages](upstream-port-audit/p3-04-2026-10-16.md#current-delivery-after-primitive-presentation-2fcc0e1)
проверены на3OS с1047 exact product inputs и relocation15/14/14; они не заменяют
whole gates гостей или полный native обход. [Current Windows whole gate](upstream-port-audit/p3-03-editor-stability-2026-10-16.md)
завершился338/339 с одним external-editor error-wait failure. Три codec/pause
suites проходят; причина прежних cold delays не доказана. Последующая sparse
диагностика выявила ошибочное принятие процессов за потомков редактора после
повторного использования parent PID. Windows snapshot adapter проверяет порядок
создания через исходный CProcess; RED падает только на recycled-parent-id,
GREEN7Qt и три native editor repetitions180Qt проходят без изменения5s/90s
лимитов. Новый macOS whole323/323260.39s и Linux whole322/322509.94s проходят.
Текущий Windows whole завершился339/3402550.18s: editor60Qt PASS, отдельный
external_codecs startup wait FAIL6750ms/5000ms.1623 uploaded inputs и stock
source/config verified. Эти результаты не заменяют
ещё не завершённую общую приёмку.
[List/Link/Benchmark presentation](upstream-port-audit/p3-06-secondary-dialogs-2026-10-16.md)
добавляет source frames/grouping, десять оригинальных EN/RU видов и текущие
native Qt consumers на3OS: по10focused/3native CTest и36JSON. Mac full323/323
PASS262.65s. [Runner context correction](upstream-port-audit/p3-03-runner-priority-2026-10-16.md)
восстанавливает обычный priority только проектной интерактивной задачи; причина
codec delay и стабильность whole Windows этим не установлены.
[Edit/Password/Memory/Messages](upstream-port-audit/p3-06-source-prompts-2026-10-16.md)
добавляет восемь оригинальных EN/RU initial views, source frame/focus/default
button и paired spinner keyboard. Current Mac full324/324266.43s проходит;
Windows/Linux по13focused/4native PASS без skip, включая реальный Windows RAR
memory. Полный Linux323/323529.30s предшествует последним focus-изменениям.
Предыдущий Windows secondary whole имеет339/3412750.99s: process_pause timeout
и codec_probe entered-marker delay; editor60Qt и external_codecs проходят.
Этот результат сохраняется как failed, без нормализации числа341 до340.
Current packages и полный native обход остаются открытыми.
Последующий cadb215 whole проходит: Windows342/3422375.07s,
5428Qt/0fail/20skip и Linux323/323530.12s,5165Qt/0fail/66skip.
Это не устанавливает причину прежних failures или cold stability.
[Browse/Temp correction](upstream-port-audit/p3-06-source-browsers-2026-10-16.md)
проверена native на3OS, с original populated EN/RU reference и keyboard probes;
новый Mac full324/324265.65s,5187Qt/0fail/66skip проходит. Текущий Windows whole
после Browse и финальное обновление packages остаются открытыми.
[Текущий launcher, логи, входы и ограничения](upstream-port-audit.md#p3-05--native-associated-launcher-and-process-lifetime-2026-10-10);
[текущий полный Linux gate и версии](upstream-port-audit.md#p3-03--full-linux-gate-after-launcher-restoration-2026-10-10).

Нативное дополнение2026-10-14: [Linux package workflow и console recovery](upstream-port-audit.md#p3-03p3-04--native-linux-package-workflow-and-desktop-recovery-2026-10-14).
Windows desktop был восстановлен обычным входом; после Windows Update и
перезагрузок пользователь повторно открыл active console Session1.
[Продолжение2026-10-15](upstream-port-audit/p3-03-2026-10-15.md)
фиксирует full Windows332/337, границы служебной диагностики, GIO default-open,
обнаруженный Clear defect и исправление provider headers при смене языка.
[Linux native correction](upstream-port-audit/p3-04-2026-10-15.md) проверяет
quoted identity, Clear/reopen/Cancel и corrected provider headers на реальном
Xorg. [Текущая поставка2f8e194](upstream-port-audit.md#p3-04--delivery-after-native-association-correction-2026-10-15)
проверена на3OS и отдельно в isolated Linux userspace; actual Linux Release
native workflow проверен с live Qt mappings. Эти package receipts предшествуют
текущим UI adaptations; full native/rendered acceptance остаётся открытой.

[Native Linux placement/navigation](upstream-port-audit.md#p3-03p3-05--native-linux-placement-and-navigation-2026-10-15)
на текущем продукте дополнительно проверяет normal/maximized Close/restart,
splitter/F9 и address/Root/history/bookmarks/keyboard на Xorg/Openbox:
8/8 CTest,122Qt/0fail/0skip. JSON normal/compact/EN-RU просмотрены;
это ограниченный Linux-сценарий, оставшийся полный обход обязателен.
[Native panel workflows](upstream-port-audit.md#p3-03p3-05--native-linux-panel-workflows-2026-10-15)
дополняют его columns/language/Rename и Copy/Move/Refresh/Comment:
22/22 CTest,687Qt/0fail/0skip; geometry/paint consumers проверены на Linux,
исходные Windows GUI failures и slow/native-writer scope остаются открытыми.
[Native Add consumers](upstream-port-audit.md#p3-03p3-06--native-linux-add-consumers-and-activation-2026-10-15)
дополняют Linux-сценарий:42 unchanged passes и5/5 после восстановления
activation prerequisite в fixtures. Продуктовый layout не изменён;
native Windows и rendered reference этим не подтверждены.
[Native Extract/prompts](upstream-port-audit.md#p3-03p3-06--native-linux-extract-and-prompt-lifecycle-2026-10-15)
проверены вместе с resource-contract subset; legacy focus/layout fixtures
исправлены, четыре Windows-only skips сохранены. Windows console после ручного
входа активна; old-snapshot native replay3/4 не закрывает fresh full/stability.

| Область приёмки | macOS arm64 | Linux x64 | Windows x64 |
| --- | --- | --- | --- |
| Default format/build/CTest | checked composite:2a7303c full329/330 PASS403.97s,5073Qt/1fail/66skip; sole language fixture corrected, default1/1/native8Qt PASS. Original full remains failed | checked composite:2a7303c serial full331/332 PASS585.10s,5067Qt/1fail/66skip; sole language fixture corrected, default1/1/native8Qt PASS. Initial compiler kill retained | checked composite:2a7303c Limited331/348 executed PASS2231.68s,17failed;5259Qt/71fail/20skip. Separate language default1/1/native8Qt and scoped administrator16/16,229Qt PASS. Source/stock restoration verified; original Limited failures and no-new-VHD exclusions retained |
| Исходные модули, headers/resources/callers | checked: [P3-05 source/integration closure](upstream-port-audit/p3-05-source-closure-2026-10-17.md), body/dependency ledger и current consumers; full native workflow отдельно | checked: общий source scope принят P3-05, POSIX/current consumers проверены; full native workflow отдельно | checked: P3-05 source scope и original native launcher/handle/disk/browser/archive consumers приняты; full native workflow отдельно |
| EN/RU, normal/compact, команды и workflow | partial: автоматические contracts/regressions проходят; полный нативный обход не принят | partial: current package на реальной консоли, EN/RU/two-panel normal/compact, Options/Copy и create/open/test/extract проверены; полный обход открыт | partial: выполненные focused consumers/JSON; current interactive gate и полный обход не приняты |
| Нативные dialogs/shortcuts/icons/drag/registration/FS | partial: историческая ручная Finder-проверка принята отдельно; Cocoa keyboard notification protocol0–9 проверен, physical input/full native обход не проверены | partial: delivery8d56d8d Copy/Overwrite/Language/PCManFM drag/Escape; Release2f8e194 GIO workflow проверен; corrected keyboard XTest1/native consumers7/7; full native scope открыт | partial: active console Session1; corrected keyboard consumers7/7,143Qt/0fail/0skip; original Limited reference also suppresses physical RightCtrlShift0, positive1/AltShift0 PASS; current full/native acceptance открыт |
| Наблюдение текущего приложения через Computer Use | manual alternative: пользователь выбрал ручную проверку; Add gutter и Save/Folder EN/RU select/cancel подтверждены для1f5e16a. Прямой Computer Use не разрешён; остальной native scope открыт | partial: current package/affected views наблюдались через Hyper-V console API; полный rendered audit открыт | unverified: console доступна, полный обход текущего приложения не принят |
| Сверка с отрисованным Windows 7zFM того же pin | partial: current Cocoa Add/Extract2/2 PASS9Qt/12JSON; Windows initial EN/RU reference available, grouping/fit checked; no Mac PNG or full rendered/input acceptance | partial: current XCB Add/Extract2/2 PASS9Qt/12JSON/12PNG; representative normal/compact views inspected against four original initial states; full rendered audit open | partial: four pinned original Add/Extract EN/RU initial views inspected, current Qt2/2 PASS9Qt/12JSON/12PNG; grouping/order/labels/fit match with documented Qt metrics/scroll adaptation, not pixel equality. Earlier System menu20entries/states/icons remains bounded evidence; full audit open |
| Runtime packages; clean-OS scope исключён пользователем2026-10-15 | checked: [delivery7a8592f](upstream-port-audit/p3-04-compression-options-delivery-2026-10-17.md), Release/relocation15,1056 exact inputs, notices/signature/Mach-O/privateABI; source ICNS unchanged, previous strict raw/native pixel failure retained, no pixel-parity claim | checked: та же delivery7a8592f, Release/relocation14,1056 exact inputs, notices/SFX/fresh ELF/privateABI; Xvfb GUI smoke, clean OS waived | checked: та же delivery7a8592f, Release/relocation14,1056 exact inputs, notices/SFX/CRT/PE/trusted-system resolution и fresh packaged icon/mask equality; clean OS waived |
| Cold/stable Windows, итоговая трёхплатформенная приёмка | partial: полный native scope остаётся открытым | partial: current broad XCB235/235560.14s,4505Qt/0fail/37skip closes prior broad fixture failures for its snapshot; subsequent localized consumers retain their receipts | partial: current2a7303c whole external_codecs13Qt/codec_probe18Qt/process_pause4Qt PASS. Historical5215ms and later deadline failures retained under unchanged5000ms application deadline; cause/cold stability unresolved, full native scope open |
| GitHub CI | N/A по явному решению пользователя сохранить удаление workflow | N/A по тому же решению | N/A по тому же решению |

[Widget-dialog workflow fixture](upstream-port-audit/p3-03-widget-dialog-fixtures-2026-10-16.md)
после локальной security-wait коррекции: девять affected default suites проходят
на3OS, по151Qt/0fail/0skip. Linux/Windows также проходят девять whole native
suites с теми же Qt totals. Cocoa проходит восемь whole suites и отдельно18Qt
representative editor checks; полная96-row editor matrix достигает90s cap.
Это Qt widget scope, не ручная приёмка NSAlert. Полный macOS gate324/324234.12s
предшествует последней локальной security-wait коррекции; affected follow-up
проверяет изменённый consumer на3OS.

Slow suites исключены из default gates по AGENTS.md. Пользователь явно разрешил
все11 на3OS; failing historical runs, corrected affected passes и текущие
all-eleven replays различаются в [long receipt](upstream-port-audit/p3-03-long-acceptance-2026-10-16.md).

[Copy publication widget fixtures](upstream-port-audit/p3-03-copy-widget-fixtures-2026-10-17.md)
дополняют текущую матрицу: шесть native suites на каждой ОС, по100Qt/0fail/0skip
и320JSON/0PNG; Mac/Linux default также6/6PASS. Producte3b3c65 сохранён.
Новый12-й slow `copy_archive_refresh` пользователь явно отказался запускать;
его полный current native matrix остаётся unverified, предложенный responder
сохранён вне проверенного коммита. Эти widget-проверки не заменяют physical
input, native OS panels или оставшийся rendered обход.

[Bounded native editor coverage](upstream-port-audit/p3-03-native-editor-coverage-2026-10-16.md)
закрывает coverage всех48 Cocoa refused-update rows тремя16-row запусками;
исходный aggregate90s timeout сохраняется отдельно. Wholeeditor default/native
60QtPASS наLinux/Windows. Scoped Highest12symlink suites191QtPASS не является
новым wholeWindows gate. Current Limitedcodec discovery снова fails; key/probe
проходят отдельно, без утверждения causal fix/stability.

[Current native navigation/selection receipt](upstream-port-audit/p3-03-native-navigation-fixtures-2026-10-16.md)
records source/config guards, scoped widget error delivery and resting selection
paint. Windows Limited full failures remain actual failures; the empty disabled
device entry in LastTest is excluded from accepted passes. Current native Windows
two-suite60QtPASS and separate Cocoa34/26QtPASS do not replace a whole gate.

[Main-panel chrome](upstream-port-audit/p3-06-main-chrome-2026-10-16.md)
сверен с восемью original Windows views: один Parent, source status каждой
панели и скрытый пустой global feedback. Mac full324/3245191QtPASS; bounded
native consumers проверены3OS по отдельным receipts. Пользовательский дефект
inline Rename focus frame и ручная приёмка macOS сообщений остаются открыты.

Последующий [Rename focus-frame receipt](upstream-port-audit/p3-06-rename-focus-2026-10-16.md)
проверяет исправление default/native subset на3OS. Пользователь2026-10-16
принял fresha2254e5 outline и настоящие EN/RU Delete→Cancel/existing-name messages,
fit кнопок/текста и возврат фокуса в список. Этот конкретный manual scope закрыт.
Новый Mac full323/324 сохраняет unchanged taskbar child handshake failure;
remaining native/cold/platform и guest delivery приёмка не подменяется этой проверкой.

Последующий [fresha2254e5 delivery](upstream-port-audit/p3-04-2026-10-16.md#current-macos-delivery-after-optionsmainrename-a2254e5)
обновляет пакеты на3OS с1047 exact inputs и15/14/14 relocated scenarios,
licenses/SFX/signature/dependency/privateABI проверками. Новый
[native drag fixture receipt](upstream-port-audit/p3-03-native-drag-fixtures-2026-10-16.md)
снимает scoped XCB hang и уточняет OLE early cancel; current Mac full324/324
PASS, но whole native/cold/source приёмка остаётся отдельным обязательством.

Windows Options/direct consumers также проверены: семь suite с Limited токеном
и отдельный source_link_storage с Highest для disposable symlinks дают88Qt/0fail/
0skip в каждом режиме. Сбой symlink setup в Limited сохранён; это два запуска,
не новый полный Windows gate.

[Native Options spacing](upstream-port-audit/p3-06-options-spacing-2026-10-16.md)
изменяет два presentation owners после product0e7c0ee; перечисленные пакеты
сохраняют прежнюю ревизию и требуют обновления после этой коррекции. Ручные
Save/Folder и отдельные message-panel attestations не переносятся на другие owners.
Текущий source style строк состояния и geometry consumers — в
[P3-06 receipt](upstream-port-audit.md#p3-06--source-status-bar-style-and-current-geometry-consumers-2026-10-10).
Текущие артефакты, dependency/rootfs proofs и точные границы relocation-проверки — в
[P3-04 receipt](upstream-port-audit.md#p3-04--delivery-after-native-launcher-restoration-2026-10-10).
Linux userspace/rootfs, отсутствие host Qt, missing-module и TMPDIR проверены
в [isolated runtime receipt](upstream-port-audit.md#p3-04--isolated-linux-runtime-and-temporary-directory-contract-2026-10-10).
Подробная методика macOS/Windows load commands и системных импортов — в
[binary dependency receipt](upstream-port-audit.md#p3-04--current-macos-and-windows-binary-dependency-closure-2026-10-10).
Открытые обязательства остаются в P3-03/04/05/06;43/47 не изменено. Историческая
успешная проверка не превращается в проверку нового бинарника или другой ОС.

Уточнение пользователя 2026-10-03: исправить во всём приложении подобные
самостоятельные допущения, как выбор источников внутри Add to Archive.
Это обязательная часть текущего портирования и итоговой P3-06 приёмки.
Прежняя запись «Qt adaptation» сама по себе не оправдывает иной workflow.

Эталон исходников: `https://github.com/ip7z/7zip.git`, полный commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Для поведения проверяются
обработчики и callers; для отрисовки — настоящий Windows 7zFM того же commit.
Требования доступности, масштабирования и платформенные сервисы фиксируются
с конкретной причиной. Они не разрешают добавлять новые шаги, поля, команды
и подтверждения ради удобства реализации.

| Область | Исходный владелец | Найденное отличие / необходимая проверка | Статус |
| --- | --- | --- | --- |
| Native Copy terminal phases | App.cpp::OnCopy; PanelCopy.cpp; CPanel::MessageBox_Error_HRESULT | Premature refresh consumed pending Move publication while guards blocked loading. Keep pending publications until guard release and present writer result first. Replace independent Qt Warning with original Critical/OK owner; retain both source diagnostic phases and macOS caption omission. | Six stale-Move and four wrong-icon old-product RED cases; final focused2/2 on three OS and32reports/OS. Full/native snapshot precedes localized diagnostic follow-up. Physical/rendered/current-package/final scope stays open. [Receipt](upstream-port-audit.md#p3-05--native-copy-terminal-phases-and-retained-guards-2026-10-10). |
| Ошибка обновления после Copy/Move | App.cpp::OnCopy; PanelItems.cpp::RefreshListCtrl/OnReload | Порт подменял исходный final RefreshListCtrl вызовом OnReload и показывал лишнюю ошибку загрузки. Восстановлены сохранённое состояние и игнорирование reload HRESULT, включая worker; явный Refresh и ошибки записи/отката сохраняют диагностику. | Mac full320/320; Linux15/15, Windows15/16 с последующим исправленным localized1/1 на обоих гостях;120reports. Native/rendered/current-package/final приёмка открыта. [Receipt](upstream-port-audit.md#p3-05--copy-publication-caller-and-result-policy-2026-10-10). |
| Нативный протокол drag и средняя кнопка | PanelDrag.cpp::CDataObject/CDropSource/CDropTarget, CompressDropFiles; EnumFormatEtc.cpp | Межпроцессные Windows-форматы были заменены только in-process carrier; отсутствовали performed-effect и средняя кнопка, временный корень был drive-relative. Исходный protocol восстановлен; Qt получает входящий IDataObject через публичный application-owned converter. WasCanceled сохраняет принятую защиту выделения при Cancel. | Mac full320/320; focused16/16 Mac/Linux,19/19 Windows;153reports, исходный порядок popup и видимые действия. Physical pointer/Explorer/rendered приёмка остаётся обязательной. [Receipt](upstream-port-audit.md#p3-05--original-native-paneldrag-protocol-and-qt-consumers-2026-10-10). |
| Эффект перетаскивания по умолчанию | Panel.cpp::GetDriveOrNetworkPrefix; PanelDrag.cpp::IsItSameDrive/GetEffect | AltStreams, пустой/относительный FS-путь ошибочно предпочитали Move при положительном ответе Qt volume service. Восстановлены исходные проверки и Windows prefix loop; POSIX заменяет только определение тома. Остальные lifecycle/status/control ветви Panel имеют конкретную карту Qt owners. | Mac full319/319; focused14/14 на Mac/Linux/Windows. Native ADS consumer проходит Windows. Физический ввод/OLE receiver/rendered приёмка и whole PanelDrag остаются открыты; [receipt](upstream-port-audit.md#p3-05--source-default-drop-effect-and-panel-core-disposition-2026-10-10). |
| Адресные клавиши и состояние плоского вида | Panel.cpp::CMyComboBoxEdit::OnMessage, SetListViewMode, ChangeFlatMode; App.h | Control-W из адреса не закрывал окно; плоский вид в Drives менял только QAction и терялся при View activation/переходе в папку. Исходные обработчики и состояние восстановлены; Qt заменяет сообщения, стили и отмену загрузчика. Tab/Backtab сохраняет draft и фокусирует собственный список. | Mac full318/318 и focused22/22; Linux22/22, Windows20/22 с прежними popup-exposure и pre-dispatch SendInput failures. Новые cases проходят;120normal/compact JSON проверены,24compact Windows status elisions учтены. Native/rendered приёмка открыта. [Receipt](upstream-port-audit.md#p3-05--panel-address-keys-and-source-view-state-2026-10-10) |
| Открытие меню «Вид» и примеры времени | MyLoadMenu.cpp::OnMenuActivating; resource.rc View; App/Panel getters | Дата и пять примеров времени заполнялись только при открытии дочернего popup. Родительское View теперь выполняет исходный обработчик, отражая view/sort/flat/toolbar/UTC без выполнения команд. QAction сохраняют identity и bindings. | Actual RED4 → GREEN; обе панели/EN-RU, precision/UTC persistence, CTime fallback и normal/compact main/View/time JSON. Native desktop/rendered Windows приёмка остаётся открытой. [Receipt](upstream-port-audit.md#p3-05--source-view-menu-activation-2026-10-10) |
| Большие страницы: запуск и Apply | FM.cpp::SetMemoryLock; SettingsPage::OnApply; CompressCall::AddLagePagesSwitch | Восстановлены исходные native privilege calls, save-on-failure и риск до `-slp`; POSIX option остаётся disabled. Layout/text не изменены. | Actual RED; MemoryLock11Qt PASS3OS. Mac full314/314, Linux13/13; Windows12/13 с двумя прежними external startup-window сбоями.60JSON без неожиданного clipping, Benchmark сохраняет проверенную прокрутку. Права runner не изменены; native allocation/rendered/final acceptance открыты. [Receipt](upstream-port-audit.md#p3-05--startup-settings-and-large-page-engine-policy-2026-10-10) |
| Окно: сохранённое положение и максимизация | FileManager/FM.cpp::InitInstance/SaveWindowInfo; ViewSettings.cpp::CWindowInfo | Исходный InitInstance применяет rectangle/maximized и Panels через узкие Qt services. Закрытие и повторный запуск проверяют normal/maximized, панели, фокус и разделитель; Qt сохраняет logical client coordinates. | Window placement13Qt/8JSON PASS на каждой из3ОС; Mac full312/312, Linux9/9. Windows8/9: два external-process startup сбоя повторяются на прежнем коде, причина/native rendered/final приёмка открыты. [Receipt](upstream-port-audit.md#p3-0516--source-initinstance-window-placement-2026-10-10) |
| Разделитель: исходная координата и пропорция resize/F9 | FM.cpp::CSplitterPos, WM_CREATE/WM_SIZE/MoveSubWindows/SaveWindowInfo | Исходные границы, fixed-point ratio и округление, ручка4; Qt переносит события мыши/resize и сохраняет ограничения содержимого. | Actual RED saved300→299/298; window19Qt PASS на3ОС,30JSON без clipping. Mac full313/313, Linux16/16; Windows14/16 с прежними external-startup/exposure сбоями, подтверждёнными на предыдущем коде. Native/rendered acceptance открыта. [Receipt](upstream-port-audit.md#p3-0516--source-splitter-resize-and-persistence-2026-10-10) |
| Copy/Move: история назначения и доступность после popup | FileManager/ViewSettings.cpp::AddUniqueStringToHeadOfList; App.cpp::OnCopy; PanelMenu.cpp::OnContextMenu | Qt case folding удалял исходно отдельную строку K при добавлении K. Восстановлены source UStringVector/comparison и Qt boundary; Cancel/accept/reopen проверяются через реальные source dialogs. Завершение popup восстанавливает доступность QAction после временных source guards. | macOS full311/311 и Linux11/11 PASS; Windows10/11, native list-settings требует активного рабочего стола. [Receipt](upstream-port-audit.md#p3-05--viewsettings-and-source-copy-history-2026-10-10); rendered/global приёмка отдельно |
| Add: выбор источников | FileManager/Panel.cpp::AddToArchive; GUI/CompressDialog.rc | Picker удалён из всех ветвей диалога. Исходный CPanel::AddToArchive перенесён целиком с typed CompressFiles services: источники из панели, пустое выделение и архивная панель дают исходные ошибки, native ShowDialog/UpdateArchive создаёт/обновляет output. Семь форматов, параметры, metadata/password/Cancel проверены; unknown-extension update использует исходный two-panel Copy. Drag Add также идёт через source CompressCall2, с native success/error/Cancel. | macOS focused16/16PASS14.41s; свежие full/Windows/Linux проверки идут |
| Add: компоновка и дополнительные поля | GUI/CompressDialog.rc/.cpp | Format и compression слева; Update/Path и Options/Encryption справа; volumes/parameters слева; лишняя Compression group, отдельные Save in/preview и Create/Add вместо OK удалены. Folder label над Archive+`...`. Encryption method теперь в одной строке с combo, decompression memory выровнено вправо по настоящему Windows runtime. Normal/compact geometry и текущие 14 Add/14 Copy JSON просмотрены; labels не обрезаны, compact нижние поля доступны прокруткой. AddFilter/OnButtonSetArchive перенесены; порядок фильтров, aliases, extension/Cancel проверены. Устаревшая constructor-only existingTarget ветвь с блокировкой полей ещё подлежит удалению. | macOS focused19/19PASS14.93s; текущие platform/full проверки и legacy branch открыты |
| Add: размер окна | GUI/CompressDialog.rc; настоящий Windows 7zFM | В Qt оставался размер760×720 от прежнего picker. Размер теперь следует содержимому и доступной области экрана. Повторный настоящий Windows runtime v9:642×578, OK/Cancel/Help полностью доступны; local640×571 JSON и default/frame regression проверены. | Windows13/13PASS70.86s и runtime проверены; snapshot предшествует следующему Add source port |
| Add: значения памяти | GUI/CompressDialog.cpp::PrintMemUsage; CompressDialog.rc | Сравнение настоящего 7zFM и Qt на Windows выявило отсутствие limit/RAM в видимой строке и label сбоку вместо сверху. Исходный PrintMemUsage перенесён с Qt text return; ветви и новые EN/RU reports проверены. Новый снимок запущенного приложения показывает полную строку. | macOS full227/227; Windows/Linux focused13/13, global acceptance открыта |
| Add: параметры времени/NTFS | GUI/CompressOptionsDialog.rc; CompressDialog.cpp::COptionsDialog; Win32 dialog manager | Source256×248DLU, группы/колонки, explicit `:` captions, initial focus и Tab/Backtab восстановлены в Qt adapter. Native minima защищают wrapped RU; Cocoa group/combo backing имеет узкие geometry adaptations. Source timestamp/profile/OK/Cancel/Help bodies сохранены. | [Current receipt](upstream-port-audit/p3-06-compression-options-2026-10-17.md): five direct suites95Qt и native14Qt PASS3OS,14JSON/OS, original8states/22controls. Raw Windows8/8, explicit POSIX state differences retained; full native/cold/package scope отдельно. |
| Add: Delete и update mode | GUI/CompressDialog.cpp::OnInit/OnOK; Common/Update.cpp::CheckDeleteAfterCompressingActionSet | Самостоятельное отключение checkbox для Freshen/Update/Sync удалено. Исходная форма принимает intent, source worker проверяет совместимость после OK. Успех/Cancel/отказ с сохранением input проверены. | macOS full227/227; Windows/Linux focused13/13, global acceptance открыта |
| File/menu/toolbar/context | FileManager/MyLoadMenu.cpp, App.cpp, PanelMenu.cpp, resource.rc; Explorer/ContextMenu.cpp | Independent Open/Create/Add-to-open/Extract-selected QAction, extension IDs/dispatch и Ctrl+O picker удалены. Source toolbar/Explorer Add, two-panel Copy update, selected Copy To/F5 и Open/Open Inside сохранены. Archive popup больше не добавляет отдельную группу Qt Add/Extract/Test/Info. Program-menu source lifetime и Info→Properties подключены. Journal и built-in Text View/Edit, их collector/dispatch/FileViewerDialog также удалены; source F3/F4 и external process/update prompts сохранены. | Указанные aliases удалены; main_shell проверяет отсутствие действий. Полная native/rendered/P2-09/P3-06 приёмка остаётся открытой |
| Extract и prompts | GUI/ExtractDialog.rc/.cpp, FileManager/OverwriteDialog.rc/.cpp | Проверить исходный набор полей, defaults, скрываемые состояния, overwrite questions и кнопки. Проверки безопасного назначения остаются за backend. | Открыто |
| Copy/Move, две панели | FileManager/CopyDialog.rc/.cpp, App.cpp, PanelCopy.cpp | Проверить последовательность выбора источника/цели, defaults, prompts и команды в архивном namespace; сохранить исходный способ добавления в архив. | Открыто |
| Navigation/list/views/selection | FileManager/Panel.cpp, PanelKey.cpp, PanelSelect.cpp, PanelItems.cpp, PanelListNotify.cpp | Проверить focused versus selected, пустую папку, адрес, root/parent, shortcuts, columns и transitions во всех режимах. | Открыто |
| Options и дочерние страницы | FileManager/OptionsDialog.cpp, Windows/Control/PropertyPage.cpp и *Page.rc/.cpp | System first и шесть source pages сохранены. Начальный frame использует source316×296DLU и native font/chrome; normal/compact layout и Apply/OK/Cancel/restart сохраняют consumers. Original EN/RU six-page reference просмотрен; native minimum Windows390px учтён. Initial focus, Tab/Backtab и keyboard page activation восстановлены по физическому original Windows reference, включая checked radio и select-all. | [Frame](upstream-port-audit/p3-06-options-2026-10-16.md) и [current keyboard component](upstream-port-audit/p3-06-options-keyboard-2026-10-17.md): focused native5/5 на3OS,69Qt/0fail/0skip и86JSON/0PNG наОС; macOS/Linux также default5/5. Live association icons/values и полный pixel/global audit остаются открытыми. |
| Standard dialog captions | FileManager/LangUtils.cpp::LangSetDlgItems, GuiCommon.rc и dialog OnInit callers | Исправлены English Cancel при выбранном RU и сброс Memory Continue в OK после смены языка. Исходная таблица standard IDs подключена к двенадцати dialog consumers; roles/signals сохраняются. Qt native dialogs и Options property-sheet имеют отдельного платформенного владельца. | P3-05.9 accepted button scope:27Qt/0skip и по48 EN/RU normal/compact footer reports на3OS; rendered/global acceptance и source colon helpers .10 открыты |
| Progress/messages/password | FileManager/ProgressDialog2.rc/.cpp, MessagesDialog.rc/.cpp, PasswordDialog.rc/.cpp | Проверить дополнительные тексты/кнопки, pause/cancel/close, повторные ошибки и modal lifetime. Полный ProgressDialog2 проверен2026-10-10; отмена/успех восстановлены без добавленных Qt-значков, ошибка сохраняет Critical. [Карта исходных тел](omitted-scope-audit.md#progressdialog2-complete-body-disposition-2026-10-10); native/rendered приёмка остаётся. | Открыто |
| Properties/Comment/Link/Split/Combine/Benchmark | Соответствующие FileManager и GUI .rc/.cpp | Проверить параметры вызова, набор контролов, условные состояния, action semantics и Help. Split invalid-volume восстановлен как исходный modal Critical/OK2026-10-10; прежняя inline-адаптация удалена. [Карта диалогов](omitted-scope-audit.md#primitive-input-and-read-only-dialog-bodies-2026-10-10); native/rendered приёмка остаётся. | Открыто |
| Help/About и OS integration | FileManager/AboutDialog.rc/.cpp, HelpUtils.cpp, SystemPage.cpp, Explorer | Проверить самостоятельные экраны/команды и необходимые различия native платформ; не подменять исходный workflow новой формой. | Открыто |

Source keyboard reconciliation restores the complete active PanelKey::OnKeyDown
dispatcher and PanelMenu clipboard bodies. Ctrl+C/Ctrl+Insert copy actual
selected raw names in source-index order with CRLF, Ctrl+Shift+A follows Select
All and Ctrl+W posts Close. Modified shortcuts, bookmark precedence, guards,
both panes/four views, EN/RU normal/compact and editor launches are covered;
the inactive source Ctrl+PgDn branch remains a consumed no-op despite its menu
caption. This closes the identified independent keyboard dispatcher gap,
not the whole Navigation row or rendered Windows parity. Native Windows
keyboard execution remains pending desktop login; see
[source/evidence](upstream-port-audit.md#p3-05--source-keyboard-dispatcher-2026-10-07).

Native changed-archive Parent for disk-extracted children now has bounded consumer verification against
PanelItemOpen::OpenParentArchiveFolder and PanelFolderChange::CloseOneLevel/
OpenParentFolder: No and Cancel discard/exit, failed or cancelled Yes preserves
the modified spool while still returning to the parent, and final Refresh
errors remain ignored. Five native representative2/301-row cases check recovery, real writer
retry, child focus, peer bytes/guards and normal/compact controls. This closes
that consumer gap, not the full Navigation/Copy/Move or rendered UX rows above;
see the [dated audit](upstream-port-audit.md#p3-0515--changed-native-nested-archive-parent-2026-10-07).

Конкретный пропуск Root/drive-view command зафиксирован в
[P3-05.13 source audit](upstream-port-audit.md#p3-0513--source-root-command-target-implementation-validation-active-2026-10-06)
и text contract: проверка непустого списка и selection marks не доказывает
исходную цель команды. Исходный переход и consumers теперь интегрированы;
проверки provider/prefix/Parent/Back/peer state добавлены. Native/rendered
приёмка остаётся обязательной, текущие свидетельства — в source audit.

Interarchive Copy/Move теперь сохраняет provider при успешной публикации и
pre-update selection при физическом rollback. Native ZIP/7z и transport
ошибка/Cancel/Delete No/Cancel/error/engine cancellation проверяются отдельным
[сценарием P3-05.15](upstream-port-audit.md#interarchive-copy-publication-and-rollback-2026-10-07).
Отдельный interarchive_recovery проверяет failed rollback/reopen, fallback folder,
предупреждения и повторный F5 после восстановления временного назначения; обе
панели EN/RU table/icon/standard-alternative в normal/compact состояниях.
Native metadata/platform и полная UX-приёмка остаются открытыми.

Source OnCreate теперь начинает публикацию до открытия следующей панели;
позднее создание peer использует тот же owner без дополнительного Qt render.
Reference — полное программное построение list/header/status в upstream
Panel.cpp::OnCreate и App::Create/SwitchOnOffOnePanel. Initial/late creation,
2/301-row FS/transport/native ZIP/7z, оба view/selection режима и EN/RU
normal/compact проверяются в panel_creation_publication. Это не pixel/rendered
Windows и не полная native renderer-failure приёмка.
Saved FS0/main archive1 дополнительно завершает первое чтение внутри второго
Bind; source-provider counts должны соответствовать видимым строкам обеих панелей.
Отдельный panel_creation_path проверяет requested versus saved main path,
включая ordinary startup справа, и terminal focus после двух301-row scans.
Явный ввод в peer или новый public archive request имеет приоритет над ещё
не завершённым initial focus. Оба режима/языка/selection и normal/compact
проверяются без утверждения rendered Windows parity.
После modal opening выбранный адресный контрол сохраняет фокус; API fixture
доказывает выбор peer настоящим mouse input, title и physical focus до запроса.
Немедленное раскрытие сохранённого peer до первого focus callback сохраняет
его адрес/строки; отложенный initial focus нужен только ещё читающему стартовому
списку. Синхронный source Create уже устанавливает фокус.
Source FM.cpp::MoveSubWindows задаёт panel0 слева/panel1 справа. Вставка Qt по
индексу сохраняет этот порядок при archive-first и late peer creation; обе
normal/compact матрицы теперь проверяют положение панелей явно.

Полная сверка App.cpp выделяет
[P3-05.15](upstream-port-audit.md#p3-0515--source-copymove-destination-and-continuation-reconciliation-required-scope-2026-10-06):
восстановить исходный выбор/классификацию destination и Copy/Move continuation,
пустая цель peer и однопанельный reducer уже восстановлены с F5/F6/Cancel/EN-RU
normal/compact consumers. Classification/refusals,
path+HRESULT diagnostics и captured execution теперь восстановлены; staged
options возвращены к исходным defaults, native metadata evidence ещё требуется.
Menu/F5/F6 без marks теперь доходят до source OperSmart; EN/RU normal/compact
summary/Cancel проверены. Timer guard handoff исправлен для одной/двух панелей;
Source post-operation saved states/title/notify/focus body теперь выполняется
после terminal execution и публикации, с сохранением peer selection и снятием
ordinary source marks; deferred progress больше не сбрасывает title. Оба source
roles/EN-RU и normal/compact panel JSON проверены. Full RefreshListCtrl load
semantics/native completion и остальные source branches ещё обязательны.
Unconditional destination publication теперь восстановлена для цели вне peer,
с retained selection и ожиданием bounded/worker load; namespace OnReload также
публикует provider rows. Success/Move/dialog Cancel/overwrite Cancel и resulting
normal/compact geometry входят в focused consumers; полная source-load и
native/platform приёмка остаются открытыми.
Source load/property stage теперь общий для namespace/archive/FS renderers,
включая удаление старых строк до смены provider indices и отказ при неверном
directory type. Existing flat/selection/Copy consumers и namespace error-state
geometry проверяют эту границу; это не закрывает remaining RefreshListCtrl
presentation и native/HRESULT/timer scope.
Синхронные namespace/archive HRESULT больше не теряются renderer callback.
Namespace Refresh использует исходный OnReload с исходным error message;
EN/RU explicit/timer/command/failure/Cancel/recovery и60 OS-managed message
reports проверяют эту границу. Post-Copy selection stage теперь сохраняет
исходный early return после load failure; обе source roles/Copy/Move проверяют
ошибки, Cancel и повторную загрузку без исключения из count provider.
Синхронный FS reload больше не теряет provider через повторную привязку;
filesystem Refresh идёт через source OnReload. EN/RU обе роли и реальный FS
peer Copy/Move проверяют Cancel/ошибку и восстановление, с48 message probes.
Реальная worker error теперь сохраняет HRESULT/provider и показывает source
message до завершения async publication. Временная peer projection больше не
получает source state при завершении Copy/Move; title/focus и peer recovery
проверены, включая normal/compact resulting-panel reports.
Missing-folder reload больше не оставляет скрытые native items/marks после
очистки Qt rows: source enumeration и восстановление проверены с обеими
selection modes, EN/RU и обеими панелями, с32 empty-view reports.
Missing-address descendants теперь проверены через source closest ancestor с
actual address/title/focus и unchanged peer/bytes;32 panel reports покрывают
оба размера. Qt transport получает исходное Windows outer-quote handling через
общий helper, с native-only conditional routes для последующей проверки.
Исходный CalledFromTimer теперь сохраняет адресный draft, cursor/selection и
focus при обновлении списка; manual Refresh публикует текущий путь. Проверки
обоих roles/EN-RU охватывают малый/worker/flat FS и bound root/ZIP consumers.
Родительская строка теперь получает исходный fallback selection и явный
name-based focus. Open/Enter/Open Outside сохраняют доступность для выбранного
`..` через source handlers; native marks/operated indices исключают sentinel.
Hidden-row/standard-alternative/small-worker-flat checks охватывают обе панели
и EN/RU, с192 normal/compact reports и private control bytes.
Source row-icon policy также подключена к FS/archive/namespace consumers:
slow-folder query, positive-index validity, исходные атрибуты/mask/directory bit.
Lookup остаётся в bounded GUI publication; failure/Cancel сохраняет HRESULT и
очищает failed rows. Real FS/ZIP small/301 on/off и normal/compact geometry имеют
focused coverage; native Windows shell appearance и global acceptance открыты.
Source name callback также подключён: исходные RLO/space/empty/Windows-marker
branches, generic cell errors и suppression. Raw operation/editor names остаются
отдельными от display text; Rename completion читает editor value. Public callback
и real ZIP Rename/Cancel/refusal/bytes покрыты с14 normal/compact name reports;
native marker rendering и full acceptance остаются открытыми.
Уточнение2026-10-10: native ListView probe подтвердил, что LVS_SINGLESEL не
выделяет строку при восстановлении одного фокуса. Qt теперь сохраняет исходный
SelectFocused и отдельные marks; без selected row focused metadata остаётся
пустой. Providerless model rows не создают source marks. Public restore/status
и manual/timer Reload consumers проверяют этот контракт; native/rendered
приёмка остаётся отдельной. [Evidence](upstream-port-audit.md#p3-05--panelitems-selection-and-provider-lifecycle-2026-10-10).

Полная [карта PanelItems](omitted-scope-audit.md#panelitems-complete-body-map-2026-10-06)
выявила ещё незавершённые исходные потребители: красный foreground удалённых
архивных элементов (IsItem_Deleted / OnCustomDraw, default marking enabled)
теперь восстановлен через исходный getter/policy и Qt model/delegate; прежний
archive-level flag получает потребителя. Table/icon/selection/guard/reload/rebind
и current red-pixel checks пройдены на macOS. Мутации header column menu
теперь принадлежат исходному ShowColumnsContextMenu: показ добавляет колонку
в конец, скрытие сортируемой колонки меняет metadata без немедленной сортировки.
EN/RU обе панели, active/inactive owner, Cancel, move/resize/Refresh и focus
проверены; Qt сохраняет скрытые logical sections и пишет компактный visible order.
Текущие native checks и rendered Windows reference обязательны.
InitColumns также исправила принудительное возвращение Name в начало: source
логический индекс сохраняется отдельно от visual order, который теперь переживает
Refresh/reopen в обеих панелях EN/RU. Прямой вызов очищает source marks до
enumeration, сохраняя Qt rows/selection до следующего restoration consumer.
Focused и normal/compact JSON проверки пройдены на macOS. Повторная сверка
SaveListViewInfo выявила независимую запись Qt width/order в `_columns`: она
удалена. Hide/show снова берёт исходную catalog width; resize без hide/show
сохраняется для следующего InitColumns/reopening. Прежняя регрессия173px после
hide/show закрепляла Qt-допущение и исправлена; catalog/visible/saved metadata
проверены отдельно.
Raw/generic publication также исправлена: Qt больше не смешивает первый PropID
с точным descriptor исходной колонки и не вытесняет generic Size raw-колонкой.
Namespace alignment теперь следует исходному GetColumnAlign. EN/RU обе панели,
неактивный provider/peer, RawProp errors/Cancel/recovery и16 JSON проверены на Mac.
Duplicate-ID menu теперь следует исходным first-visible-ID delete и append-new-copy:
повторные пункты доступны, новые секции сохраняют callback/source indices, отдельные
ширины и selection/peer focus. EN/RU обе панели и32 normal/compact JSON проверены
на Mac. Duplicate-ID sort теперь использует первый catalog raw-флаг и исходный
toggle по PropID через generic/raw/append секции. Qt сигнал применяется после
автоматического mouse sort; индикатор остаётся на нажатой секции, Size action
привязана к PropID.16 EN/RU обе панели с empty raw, selection/peer/Refresh и64
normal/compact JSON проверены на Mac. Текущие native/rendered проверки обязательны.
Schema/error consumers также восстановили отсутствие лишнего Name при raw-only/
no-Name/empty catalog и повторное перечисление одного provider/type. Source
потребляет записанные count при S_FALSE/E_FAIL и пропускает individual info errors/
IsDir; existing initialized-zero адаптация обрабатывает незаписанный выход.
12 EN/RU обе панели с menu Cancel, selection/focus/peer/bytes/recovery и72 JSON
проверены на Mac. Qt zero-height empty header записан как platform presentation;
native/rendered приёмка остаётся обязательной.
Label-only highlight теперь следует первому source logical subitem при raw Name
и no-Name; фактическая delegate paint-регрессия ловит прежнее невидимое выделение.
Icon/list projection теперь следует тому же source subitem для текста и значка,
включая menu hide/re-add raw Name и append-порядок. Lifecycle охватывает четыре
режима, EN/RU обе панели, peer focus/selection и normal/compact geometry.
Callback publication также удаляет лишний count gate перед source GetItemText:
валидные опубликованные cells сохраняются при позднем count HRESULT. Property
error/empty text, notify/provider guards и rebind/selection/peer остаются проверяемыми
consumers;32 detail/icon recovery/rebind reports покрывают EN/RU обе панели на Mac.
Сквозной FS/real ZIP/7z/schema/reopen consumer теперь проверяет source defaults,
per-type widths/order/visibility и отдельные Type/Name changes с сохранением
selection/peer focus. Normal/compact EN/RU обе панели проверены на macOS;
[evidence](upstream-port-audit.md#source-real-provider-column-workflow-2026-10-07).
Refresh written-count policy теперь совпадает с оригиналом: count HRESULT не
заменяет валидные rows ошибкой, включая Parent/post-Copy/Move. Qt хранит один
numItems между load/publication/selection и задаёт ноль для unwritten output.
EN/RU обе панели, detail/icon, Refresh/timer/recovery и bytes покрыты текущими
consumers; [evidence](upstream-port-audit.md#source-refresh-loaded-count-consumers-2026-10-07).
Enter/double-click теперь используют original OnNotifyActivateItems и все
Ctrl/Shift/Alt сочетания, включая Properties/Outside, в table/icon обеих панелей.
EN/RU real FS/ZIP, guards, Cancel Properties, folder-error/recovery и normal/compact
reports покрывают macOS; native Windows shell sheets/current3OS/rendered остаются
обязательными. [Evidence](upstream-port-audit.md#source-item-activation-notification-consumers-2026-10-07).
Source selected-state notifications также теперь обновляют owning-pane marks
немедленно, включая inactive peer, через original OnItemChanged. Snapshot больше
не перезаписывает marks при guard; deletion/alternative guards, source indices,
Reload/peer/bytes проверены в EN/RU обеих ролях с normal/compact reports.
[Evidence](upstream-port-audit.md#source-selection-notification-and-snapshot-consumers-2026-10-07).
Сверка тел не подтверждает текущую отрисовку или завершение всего модуля.
Остальная full/post-Copy error и
native/platform приёмка остаётся открытой.
Принятый архивный Move/rollback сохраняется. Existing
Copy/Shift checks не закрывают остальные branches; новый text contract обязателен.

Сверка полного PanelFolderChange также выявила обязательный
[P3-05.14](upstream-port-audit.md#p3-0514--persisted-source-location-consumers-required-scope-2026-10-06):
namespace/nested Save ограничения, отказ доступной drive-команды и обработка
сохранённых путей при обнаружении расходились с исходными consumers. Source
virtual prefix, доступность/handler и saved-location binding восстановлены;
старые Qt display entries, история, обе панели и original nested reuse/cold
fallback имеют public coverage. Mac implementation gate и16EN/RU reports
проверены; current native/platform/rendered приёмка остаётся открытой.

Устаревшая `existingTarget` constructor-only ветвь Add удалена: поля path/format/
browse/volumes/encryption больше не блокируются ради отдельного update workflow.
All-target build и12 affected suites PASS9.66s на macOS. Также удалены
самостоятельные требования существующей папки назначения и preflight источников:
исходные scan/UpdateArchive отвечают за ошибки и создание папки после OK.
Регрессия новой папки падает до исправления; последующие проверки ещё идут.
Actual Windows v10 Add просмотрен рядом с pinned original:642×546, без picker,
footer полностью доступен, encryption method same-row и memory right-alignment
подтверждены отрисовкой. Source SFX уже подключён к retained validated module;
обычный/password/split executable и Cancel проходят локально. Новая сборка
ещё проверяется на Windows/Linux. Открыты automatic-method `*` и исходное
очищение списка методов при Store; их нельзя считать допустимыми Qt adaptations.
Artifact/provenance — 7zfm-parity.

Исходный `App.cpp::g_StandardButtons` задаёт Info как `IDM_PROPERTIES`.
`QtFileManagerCommands::AttachFileActions` теперь привязывает её к исходному ID.
ArchiveInformation extension и самостоятельный summary screen удалены, tests
мигрированы к исходной modal Properties. Item/empty-archive/EN→RU→EN/backend
counts и actual filesystem Info во всех view/flat режимах проходят локально;
five direct consumers PASS3.16s. Native Windows System Properties и свежая
платформенная проверка этого последующего изменения ещё обязательны.

2026-10-04: Add selection/header/focus corrections validated separately by
macOS full230/230PASS93.41s and snapshot07e2aeb5 Windows29/29PASS123.49s /
Linux29/29PASS38.34s. Actual default Qt v12 PNG compared with the pinned original:
no source picker, native path display, selected filename, SFX/automatic method
and visible footer. Remaining spacing/control-width/Archive-caption differences
are still open; full visual correspondence is not claimed.

Program File now uses original OnMenuActivating/CreateFileMenu and CApp plugin
invoke/release. Reopening, same-ID stale QAction and panel-switch checks pass,
as do program Open success/refusal/Cancel and program Add success/error/Cancel.
Four EN/RU normal/compact root and child JSON inspected, no clipped rows or
visible aliases. Legacy alias QAction construction/workflows still need removal;
this partial result does not close the File/menu/toolbar/context row.

2026-10-04: Journal и built-in text view/edit QAction, extension IDs и dispatch
удалены. Удалены их независимые накопитель сообщений, FileViewerDialog и
MainWindow preview/extract/update wrappers. F3/F4 используют исходный EditItem;
Progress/Messages продолжают показывать ошибки, Cancel не создаёт второго окна.
Native Open lifetime/Cancel перенесены в archive_item_open; source external edit
success/failure/Cancel coverage сохранена. Текущие проверки ещё идут; это не
закрывает глобальную строку File/menu/toolbar/context.

Каждая строка закрывается только после исправления соответствующих consumers,
регрессий, текущих EN/RU normal/compact reports и сравнения с доступным Windows
runtime. Различия, которые ещё не исследованы, не считаются допустимыми или
исправленными. Счёт принятых пакетов не увеличивается от этой инвентаризации.
