# Historical checklist checkpoints

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Архив промежуточных checkpoints и карты укрупнения из чеклиста. Счётчики, owners, «следующий ID» и pending scope относятся к своим датам/ревизиям. Текущие статусы — в [чеклисте](../porting-checklist.md).

P3-03,2026-10-10: текущий продуктc2cb7be после native launcher restoration проходит
полный Linux gate318/318;5059Qtpass/0fail/56 explicit Windows/NTFS skips,
11slow excluded. Все1517 archive/guest inputs и fresh official runtime hashes
проверены. [Текущий Linux receipt](../upstream-port-audit.md#p3-03--full-linux-gate-after-launcher-restoration-2026-10-10).
Родительские строки P3-04/05 согласованы с актуальными delivery/source receipts;
native/rendered/clean-OS/Windows-full и итоговые обязательства остаются открытыми.

P3-06,2026-10-10: исходный no-grip style общей строки состояния восстановлен;
EN/RU normal/compact и one→two→one панели проверены, строгий Add scroll probe
исправлен без изменения production focus/layout. Последующая проверка выявила
потерю сообщения при первом создании второй панели: лишняя запись «Готово»
удалена, сохранение Unicode-диагностики проверено на3ОС (Mac full321/321,
Linux/Windows focused4/4). [Source и текущие gates](../upstream-port-audit.md#p3-06--source-status-bar-style-and-current-geometry-consumers-2026-10-10).
Parent/native/rendered/final acceptance остаётся открытой; обновлённые пакеты включают оба UI delta.

P3-04,2026-10-10: свежие38b740a Release-пакеты прошли relocation15/14/14
на macOS/Linux/Windows, payload/notices/signature и актуальную dependency
проверку. Новый Linux artifact дополнительно проходит15/15 в изолированном
userspace; actual images234/233 files проверены. [Единый delivery receipt](../upstream-port-audit.md#p3-04--refreshed-delivery-after-status-corrections-2026-10-10).
Clean-OS/native/rendered/source-launcher и итоговые родители остаются открытыми;
43/47 не изменено, GitHub workflow не восстановлен.

P3-04,2026-10-10: current macOS/Windows artifacts дополнительно прошли
[binary-import review](../upstream-port-audit.md#p3-04--current-macos-and-windows-binary-dependency-closure-2026-10-10):
11 Mach-O/157 edges остаются внутри bundle/Apple system namespaces;
26 PE/344 imports обеспечены packaged DLLs и проверенными Windows system files.
Это bounded dependency evidence; clean-OS/native/rendered и итоговая приёмка открыты.

P3-04.1,2026-10-10: текущий Linux package проходит15/15 в новом FROM scratch
userspace без host Qt/SDK. Фактические234/233 image-файла сверены, negative
отличается только отсутствием7z.so. Проверены noexec refusal, process-scoped
TMPDIR recovery и cleanup без изменения системной политики.
[Linux isolated runtime и точные границы](../upstream-port-audit.md#p3-04--isolated-linux-runtime-and-temporary-directory-contract-2026-10-10).
Другие ОС, native/rendered, source launcher и итоговая приёмка остаются открытыми.

P3-04,2026-10-10: currentf169833 runtime packages собраны на3ОС;
relocated7zG/engine Mac15/15, Linux14/14, Windows14/14 PASS, notices/CRT/
подпись и source/runtime/artifact hashes проверены. Clean-OS/native/final
приёмка остаётся открытой;43/47 не изменено.
[Текущие артефакты и границы проверок](../upstream-port-audit.md#p3-04--current-three-platform-runtime-artifacts-2026-10-10).

P3-03,2026-10-10: текущий Linux full gate318/318 PASS на056bcd2;
5054Qt/0fail/56 Windows-NTFS skips,11slow excluded. Старые pending-Linux
обязательства P3-05.8/.10 закрыты в их bounded scope. Mac full321/321 относится
к тому же продукту; Windows current full/native/cold-stable остаётся открытым.
Computer Use не одобрил доступ к текущему macOS app; нативный UX не принят.
[Полная текущая матрица и evidence](../upstream-port-audit.md#p3-03--current-full-linux-gate-and-acceptance-boundaries-2026-10-10).
43/47 и родители P3-03/04/05/06 остаются открытыми.

P3-05,2026-10-10: all109 original cpp units now have explicit source-receipt
links/omission boundaries. Complete primitive-dialog review restores source
Split invalid-volume modal Critical/OK instead of the old inline adaptation;
six old-product regressions reproduce the difference. Mac full321/321 and
Linux16/16 pass; Windows15/16 retains list_settings_runtime exposure failure
before settings assertions, with no active runner logon. All1515 guest inputs,
runtime metadata and114 current widget reports are verified.
[Source ledger/dialog receipt and limits](../upstream-port-audit.md#p3-05--input-dialog-source-and-split-error-policy-2026-10-10).
BrowseDialog2 native launcher/PanelItemOpen continuation, whole native/rendered/
current-package/final acceptance remain mandatory;43/47 andP3-03/04/05/06 stay open.

P3-05,2026-10-10: complete ProgressDialog2 body/resource/caller/dependency map
retains source algorithms; Cancel/success message boxes regain source NoIcon,
errors retain Critical. Existing regressions reproduce17 old-product failures;
toolbar consumer and Windows native-font report delivery are reconciled together.
[Source disposition, checks and limits](../upstream-port-audit.md#p3-05--progressdialog2-source-and-message-policy-2026-10-10).
Whole-family/native/rendered/current-package/final scope stays mandatory;
P3-03/04/05/06 and43/47 remain open.

P3-05,2026-10-10: complete ExtractCallback/OpenCallback bodies, interfaces,
direct callers and virtual-file/memory/overwrite/error/password boundaries are
reconciled; no functional change is needed. Five current suites pass on all
three OS,79Qt/0 failures/0 skips each; all1513 guest inputs/runtime metadata
and87 widget reports are verified.
[Source disposition and limits](../upstream-port-audit.md#p3-05--extraction-and-opening-callback-disposition-2026-10-10).
Remaining ProgressDialog2/whole-family/native/rendered/current-package/final
scope remains mandatory; P3-03/04/05/06 and43/47 stay open.

P3-05,2026-10-10: native Copy retains pending source/peer publication until
operation guards are released and uses the source HRESULT diagnostic owner.
Old-product RED covers six stale Move cases and four wrong-icon error cases.
Mac full321/321 and first Linux/Windows12/12 precede the localized diagnostic
follow-up; final affected2/2 passes on all three OS,48Qt/0 failures each.
All1513 final guest inputs and96 current widget reports are verified.
[Receipt](../upstream-port-audit.md#p3-05--native-copy-terminal-phases-and-retained-guards-2026-10-10).
Physical/rendered/current-package/final scope remains mandatory;
P3-03/04/05/06 and43/47 remain open.

P3-05,2026-10-10: Copy publication now retains CApp's saved-state RefreshListCtrl
caller and ignored reload HRESULT, including async filesystem failure; explicit
OnReload and writer/rollback diagnostics remain separate. Actual old-product
RED28/32; Mac full320/320, Linux15/15, Windows15/16 followed by corrected
localized1/1 on both guests (test signal-wait race). All1512 final inputs and
120 selected reports are verified; slow matrix expectations compile without
executing long tests. [Receipt](../upstream-port-audit.md#p3-05--copy-publication-caller-and-result-policy-2026-10-10).
Remaining source/native/rendered/current-package/final acceptance stays mandatory;
P3-03/04/05/06 and43/47 remain open.

P3-05,2026-10-10: PanelItems lifecycle/body reconciliation restores original
SelectFocused and removes providerless model-row counts. Old-product RED and
native Win32 state/mask evidence are retained. Mac full320/320 and focused17/17,
Linux17/17 pass; Windows16/17 retains only the existing native Properties
exposure timeout. All1512 frozen guest inputs and eight runtime files/OS are
verified;168 selected normal/compact reports have no meaningful clipping.
[Receipt](../upstream-port-audit.md#p3-05--panelitems-selection-and-provider-lifecycle-2026-10-10).
Physical/native desktop/rendered, remaining source/package/final acceptance
stay mandatory; P3-03/04/05/06 and43/47 remain open.

P3-04,2026-10-10: fresh Release runtime packages at9731b16 pass relocated
7zG/engine ZIP+7z byte/error/missing-module consumers: Mac15/15, Linux14/14,
Windows localized14/14 after two private-helper failures. Linux also proves
packaged7zFM archive title/ownPID and unchanged ZIP. All1453 guest inputs and
complete runtime/license payloads are verified; retained1450 product inputs
are unchanged. [Package receipt](../upstream-port-audit.md#p3-04--three-platform-runtime-package-relocation-2026-10-10).
Native/desktop prerequisite and final acceptance remain open;43/47 unchanged.

P3-03,2026-10-10: пользователь подтвердил сохранение удаления GitHub workflow
из `96daee8de9f82374a4548e6dff2bd47a6fe5a891`. Hosted CI не восстанавливается;
обязательные локальные/runner и native проверки трёх ОС остаются в scope.
[Текущая граница CI и упаковки](../../../ci.md); P3-03 остаётся открытым до native приёмки.

P3-05,2026-10-10: original native PanelDrag protocol, source middle-button/root
policy and complete body/dependency disposition are restored with shared source
lifetime and Qt-native MIME ownership. Mac full320/320 and focused16/16 pass;
Linux16/16, Windows19/19 pass, including original COM/archive and actual windows
Qt platform consumers. All1453 frozen guest inputs and eight runtime files/OS
are verified;153 selected JSON reports preserve source menu order and exposed
normal/compact states. Localized Windows2/2 also passes an independent Qt/native
COM peer process; the final original GetTransfer-helper follow-up also passes
2/2,16Qt/0fail/0skip. All1450 retained inputs match that final Windows snapshot;
only its Windows native block and conditional Windows fixture differ from the
preceding three-OS source proof.
[Receipt](../upstream-port-audit.md#p3-05--original-native-paneldrag-protocol-and-qt-consumers-2026-10-10).
Physical pointer/Explorer, rendered/cold/package and remaining module/final
acceptance remain mandatory;43/47 and P3-03/04/05/06 stay open.

P3-05,2026-10-10: source Panel address-key and view-state component restores
CMyComboBoxEdit, CApp→CPanel view dispatch and original flat-toggle/refresh.
Actual old-product RED8address/4Drives rows are retained. Mac full318/318 and
focused22/22 pass; Linux22/22, Windows20/22 with eight existing popup-exposure
rows and one pre-dispatch SendInput refusal. All new cases pass;120reports
are inspected with existing compact status elision audited. Whole Panel drag/lifecycle/status disposition and
native/rendered/package/final acceptance remain mandatory;43/47 and
P3-03/04/05/06 stay open. [Receipt](../upstream-port-audit.md#p3-05--panel-address-keys-and-source-view-state-2026-10-10).

P3-05,2026-10-10: complete MyLoadMenu body/caller disposition restores parent
View activation, source state checks and date/precision/UTC choices. Public
both-pane/EN-RU RED4→GREEN; existing column/time/reopen consumers use the parent
event. Combined Mac full317/317PASS260.33s and focused9/9; Linux9/9PASS16.29s.
Windows8/9 in66.68s retains only two established service-desktop exposure
failures; View and column/time consumers pass. Normal/compact main/View/time
reports and retained fixture bytes/providers are
checked. [Source/platform receipt](../upstream-port-audit.md#p3-05--source-view-menu-activation-2026-10-10).
Eleven slow suites excluded; remaining source/native/rendered/package/final
scope remains open. P3-03/04/05/06 and43/47 are unchanged.

P3-05,2026-10-10: native nested ZIP→opened NTFS peer F5/Skip/overwrite Cancel/
dialog Cancel and original F6 refusal pass both panes/EN-RU with protected
bytes/providers and80normal/compact reports. Direct consumers exposed four
ZIP→WIM ADS routing regressions; the Qt colon predicate now preserves source
useTemp archive staging. Final Windows9/9PASS94.63s,168Qt/0fail/0skip;
Linux9/9PASS29.91s; Mac integrated full316/316PASS257.74s and localized9/9
PASS22.55s. Same1397product/test/build inputs match both runner archives.
See the [receipt](../upstream-port-audit.md#p3-05--native-archive-source-to-ntfs-peer-acceptance-2026-10-10).
Eleven slow suites excluded; whole source/native/rendered/package/final
acceptance remains open. Parents43/47 and P3-03/04/05/06 unchanged.

P3-03/P3-05.16, 2026-10-10: actual startup failures at df06775 now have a
native control diagnosis: SSH session 0 has an invisible service desktop;
even a separate Win32 window with WS_VISIBLE fails IsWindowVisible after two
ShowWindow calls. Qt reaches setVisible(true). No product/test/deadline change
or parent closure follows; interactive startup/rendered acceptance remains
required. [Evidence](../upstream-port-audit.md#p3-03--windows-service-desktop-visibility-diagnosis-2026-10-10).

P3-05.16, 2026-10-10: source launch show command now reaches InitInstance;
explicit state/no-activation overrides saved maximization, Hide remains hidden
and accepted hidden Close exits without the probe forcing application.quit.
Public/native old-caller RED and actual application-loop RED are retained.
Mac full315/315 PASS; final focused Mac7/7, Linux7/7 and Windows6/7, with only
the same two service-desktop native visibility failures. New launch/lifecycle
rows pass on their applicable platforms;1460 inputs match both guest trees,
51selected widget JSON fit. Concurrent Windows foreign build excludes cold-
stability claims. [Receipt](../upstream-port-audit.md#p3-05--launch-show-command-and-hidden-window-close-2026-10-10).
P3-03/04/05/06 remain open;43/47 and final acceptance are unchanged.

P3-05,2026-10-10: source default-drop folder/root admission and Windows
all-source drive/UNC prefix policy restored; POSIX retains mounted-volume
identity after the same guards. Panel core lifecycle/status/control/header
disposition is recorded without claiming whole PanelDrag. Mac default319/319
PASS; focused14/14 on Mac/Linux/Windows, including actual native ADS consumer.
1443 inputs match both guests;11 slow suites excluded. Native/rendered,
remaining family/package/final acceptance stays open;43/47 unchanged.
[Receipt](../upstream-port-audit.md#p3-05--source-default-drop-effect-and-panel-core-disposition-2026-10-10).

P3-05, 2026-10-10: original FM startup RAM cache now reaches PanelItemOpen through
one immutable process snapshot; source thresholds/fallback and existing shift
guard remain. Old reader fails all eight both-pane nested Open/Copy/Parent rows;
the restored consumer passes. Final Mac default gate 317/317 PASS; focused
Mac/Linux 9/9 PASS, Windows 8/9 with only the two established service-desktop
visibility failures. 1441 inputs match both actual guests; 12 merged primary
consumers pass with the foreign View-menu component preserved. Eleven slow
suites excluded; native/rendered/package/family closure stays open. 43/47 and
P3-03/04/05/06 are unchanged.
[Receipt](../upstream-port-audit.md#p3-05--fm-startup-ram-lifetime-and-nested-open-consumers-2026-10-10).

P3-05, 2026-10-10: full FM.cpp body/branch/caller map now distinguishes source
algorithms and explicit Qt/platform services. WM_NOTIFY's source SetButtonText
and both button tables supply short toolbar captions/tooltips after language
and action changes; the public old-product RED is retained. The map records
RAM point-of-use versus startup-cache as remaining source work at that receipt;
the subsequent RAM reconciliation restores startup lifetime without closing
FM/native/family/package/final acceptance.
Full Mac316/316 PASS; final focusedMac/Linux12/12 andWindows11/12 with only the
two established service-desktop visibility failures. Native STA/link-copy and
toolbar/language rows pass;1461inputs match both guests and81widget reports fit.
Eleven slow suites excluded;43/47 and P3-03/04/05/06 remain open.
[Receipt](../upstream-port-audit.md#p3-05--fm-platform-dispatch-and-source-toolbar-text-2026-10-10).

P3-05.16, 2026-10-10: original small-screen toolbar default now reaches
CApp::Create through existing Qt dialog-unit services, and startup initializes
source symbolic-link token privileges before Copy. Public toolbar and real
Windows entry RED regressions are retained. Mac full315/315 PASS; focused
Mac/Linux11/11, Windows10/11 with only the two established service-desktop
visibility failures. New native link-copy row passes;1460inputs match actual
guest trees and69selected3OS widget reports fit. Eleven slow suites excluded.
Native/rendered/cold-stable/package/final scope stays open;43/47 unchanged.
[Receipt](../upstream-port-audit.md#p3-05--startup-platform-consumers-and-toolbar-defaults-2026-10-10).

P3-05, 2026-10-10: complete filesystem-provider body/caller review restores
case-only Rename, source filesystem→opened peer NTFS-stream Copy/Move and
Windows main/super-path Copy retry. Public RED regressions reproduce each
omission; source admission/Skip/errors/cancellation and selected-state/peer
consumers remain. Mac full315/315 PASS; final focused Mac8/8, Linux8/8 plus
localized2/2 and Windows8/8 PASS. EN/RU/both-pane geometry produces16Mac,
16Linux and80Windows widget reports.
[Source/platform receipt](../upstream-port-audit.md#p3-05--filesystem-provider-rename-and-ntfs-destination-consumers-2026-10-10).
Archive-source→NTFS-stream, remaining source/entrypoint disposition and
native/rendered/installed/package acceptance retain their required owners.
This component does not close whole P3-05 or change43/47/P3-03/04/05/06.

P3-05, 2026-10-10: complete services/namespace body, header and caller review
records twelve FileManager units and23 pinned inputs. Original Windows CSIDL
root/address icons and both native image sizes are restored through the existing
Qt bridge. Settings/association/callback/comment/split/version-control/Link
source policies and their platform boundaries are explicitly disposed.
[Source receipt](../upstream-port-audit.md#p3-05--filemanager-services-and-native-namespace-icons-2026-10-10).
Whole FileManager startup/platform, native/rendered/package and final acceptance
remain open; this component does not change43/47 or P3-03/04/05/06.

P3-05.16, 2026-10-10: the integrated startup/Settings/Benchmark/engine workflow
restores source SetMemoryLock, allocator reset, token adjustment on Apply and
the shared CompressCall risk policy before `-slp`. Old Settings regression RED;
Mac full314/314 PASS255.87s (4946Qt/0fail/18existing skips), focused13/13 PASS.
Linux13/13 PASS6.62s; Windows12/13 in57.04s, with the two previously
baseline-reproduced external startup-window failures. New MemoryLock11Qt and
Hash8Qt pass on all3OS; Hash early-completion fixture race is deterministically
covered. Snapshot/archive/local/guest1437 inputs exact,60JSON with audited
Benchmark scrolling and no unexpected clipping. Existing Windows account rights
are unchanged before/after; no policy was provisioned. This does not close
native/rendered/startup acceptance or change the43/47 parent count.
[Source receipt](../upstream-port-audit.md#p3-05--startup-settings-and-large-page-engine-policy-2026-10-10).

P3-05, 2026-10-10: physical inventory now includes every cpp/header/resource/
build input in FileManager, Agent, GUI and Common (all 109 original cpp units),
with explicit disposition for every absent input and existing source owners.
GUI standalone/resources/language fallback and duplicate shared icon boundaries
reconciled. Complete borrowed baseline verifier and documentation diff pass;
[source receipt](../upstream-port-audit.md#p3-05--complete-ui-physical-inputs-and-gui-resource-disposition-2026-10-10).
Physical ownership is not full-body/native/rendered/package acceptance;
43/47 and P3-03/04/05/06 unchanged.

P3-05, 2026-10-10: repeated pinned FileManager cpp/header/resource/build
inventory and complete absent-file disposition recorded with actual owners;
FilePlugins/FileFolderPluginOpen bodies, headers, dependencies and callers
reconciled. Qt's existing Windows manifest supplies long-path/execution-level
transport; no speculative product change. Documentation diff and complete
borrowed baseline verification pass. [Source receipt](../upstream-port-audit.md#p3-05--filemanager-resource-and-plugin-dependency-reconciliation-2026-10-10).
This is not whole-module/platform/final acceptance; 43/47 unchanged.

P3-05, 2026-10-10: complete Copy/CRC/Sort and result/history/About/Help
body/type/resource/caller disposition reconciled. The name comparator now
uses original MyCharUpper through its UTF-16 adapter; actual old archive-panel
RED and public archive/filesystem regressions establish native Greek casing
before numeric ordering, selection and unchanged bytes. Required macOS gate
313/313 PASS (254.08s; 4928 Qt checks, 0 failures, 18 existing platform skips).
Linux/Windows x64 Qt 6.12 each passed 14 focused consumers (10.77s/38.96s;
218 Qt checks, 0 failures/skips), with 1432 local/archive/actual-guest inputs
exact and 1431 retained after wrapper removal. Two current normal/compact
flat-sort widget JSON reports per guest fit. Eleven slow suites excluded.
[Source and validation receipt](../upstream-port-audit.md#p3-05--core-copycrc-sort-and-results-reconciliation-2026-10-10).
Whole FileManager/native/rendered/package/final acceptance remains open;
43/47 and P3-03/04/05/06 unchanged.

P3-05,2026-10-10: complete BrowseDialog2 body/header/resource/caller disposition
and temporary-browser lifecycle reconciled. Missing source Ctrl+R/Ctrl+F3/F5/F6
and aggregate-count wait cursor restored with actual RED1 each. Required macOS
gate312/312 PASS252.70s, localized10/10 PASS5.87s83Qt/0fail/0skip. Linux/Windows
initial10/10 PASS7.63s/39.29s82Qt/0fail/0skip each; final localized browser13Qt
perOS passes,1429 local/archive/actual-guest inputs exact,1428 retained after
wrapper removal. Four current EN/RU normal/compact JSON perOS fit. Integration
atop22b7763 passes13/13 including startup/window placement. Eleven slow excluded;
[source receipt](../upstream-port-audit.md#p3-05--temporary-browser-source-lifecycle-reconciliation-2026-10-10).
Whole FileManager/native/rendered/package/final acceptance remains open;
43/47 and P3-03/04/05/06 unchanged.

P3-05,2026-10-10: source Copy/Combo portable DTOs, CommonDialog owner/flags and
active long/super-path CBrowseDialog restored with all direct consumers and UX.
Source routing RED2 and old Qt flags RED2 retained. Required macOS gate312/312
PASS250.51s,4908Qt/0fail/18 existing skips; final localized19/19 PASS14.00s,
295Qt/0fail/0skip. Linux/Windows x64 Qt6.12 each19/19 PASS24.75s/85.89s,
295/294Qt/0fail/0skip (one POSIX-only compiled slot).1429 frozen local/archive/
actual-guest inputs match in that snapshot;8 current
EN/RU normal/compact browser JSON perOS fit. Provenance/adaptations:
[source receipt](../upstream-port-audit.md#p3-05--source-path-dialogs-and-long-path-browser-2026-10-10).
Eleven slow suites excluded. Whole FileManager/native/rendered/package/final
acceptance remains open;43/47 and P3-03/04/05/06 unchanged.

The final localized source OnButtonClicked focus correction has an actual RED1
and green browser follow-ups on all three OS; Windows adds two super-prefix
rows. Integration atop293a826 passes28/28 direct consumers plus the localized
browser follow-up. Final input hashes are recorded separately from the earlier
full-consumer snapshot in the linked receipt; this does not close P3-05.

P3-05.16,2026-10-10: source CSplitterPos coordinate/bounds/fixed-point ratio
restored with startup/resize/handle/F9/close consumers. Actual saved300 RED at
normal/compact widths retained; expanded window19Qt PASS on3OS. Mac full313/313
PASS281.82s4930Qt/0fail/18existing skips; Linux16/16 PASS68.45s606Qt/0fail/0skip.
Windows14/16: completed suites565Qt/2fail/16existing native-Properties skips,
plus timed-out filesystem-selection partial output. Both external-startup rows
predate this port; previous22b repeats the same seven filesystem exposure failures
and45s timeout. Restored/recompiled current window19Qt passes.1434 before/after
archive/local/actualguest hashes match;1433 retained merged inputs match primary.
30 fresh JSON have no clipping. Native external-process/exposure/rendered and
remaining FM startup-platform acceptance stays open,43/47 unchanged;11slow excluded.
Combined browser/casing/splitter inputs1433 exact and macOS31/31 PASS29.31s,
840Qt/0fail/0skip; accepted foreign commits preserved.
[Receipt](../upstream-port-audit.md#p3-0516--source-splitter-resize-and-persistence-2026-10-10).

P3-05,2026-10-10: complete PanelListNotify body/type/caller disposition now maps
all active notification, cell text, selection, activation, paint and four-part
status policies to their source owners and Qt transports. No code/UI changes or
new test claims; docs diff check. Native input/rendered and remaining FM/resource
closure stay open;43/47 unchanged.
[Receipt](../upstream-port-audit.md#p3-05--panellistnotify-complete-body-and-caller-disposition-2026-10-10).

P3-05.16,2026-10-10: original InitInstance placement/panel/default/show/refusal/
fallback flow restored through narrow Qt services. Public old-product four-row
geometry RED retained;13 source/window cases and8 geometry JSON pass on each OS,
including normal/maximized Close/restart, panels/focus/splitter and protected bytes.
Required macOS full312/312 PASS259.88s4905Qt/0fail/18 existing skips;
Linux9/9 PASS13.31s134Qt/0fail/0skip. Windows8/9in80.08s127Qt/2fail: only actual
separate-process startup window discovery fails. Unchanged fixture against the
previous293a826 product repeats both failures; restored/rebuilt current source
retains13 passing window-placement cases. Native caller cause/acceptance remains
mandatory P3-03/P3-05; this does not close P3-05.16 or the parent.
All1427 archive/local/actual-guest hashes match before and after diagnostic input
restoration;1426 isolated retained after wrapper removal.24 current JSON have no clipping.
The later path-dialog commit97e7194 is preserved and its combined inputs checked
separately before the window-placement commit; combined macOS11/11 PASS7.12s,
162Qt/0fail/0skip.
[Receipt](../upstream-port-audit.md#p3-0516--source-initinstance-window-placement-2026-10-10).
Eleven slow suites excluded; remaining full FM/native/rendered/packages/final
acceptance stays open;43/47 unchanged.

P3-05,2026-10-10: ViewSettings complete body/caller disposition restores original
Copy history UStringVector/comparison and CApp20-entry ownership. Source-distinct
K/K Cancel/accept/reopen is covered; related Linux consumer exposed stale disabled
QActions after popup Cancel, now recomputed after popup/CallScope destruction.
Actual immediate public-action RED retained. Final macOS full311/311 PASS296.32s,
4892Qt/0fail/18 existing skips; Linux11/11 PASS12.82s104Qt/0fail/0skip.
Windows10/11 in38.11s102Qt/2fail: only native list_settings_runtime exposure waits
fail in SSHsession0 without a logged-on desktop. That P3-03 prerequisite stays
open; no test was dropped or relaxed. All1424 archive/local/actual-guest inputs
match,1423 retained after task-wrapper removal;156 current JSON have no clipping.
[Receipt](../upstream-port-audit.md#p3-05--viewsettings-and-source-copy-history-2026-10-10).
Eleven slow suites excluded. Mandatory P3-05.16 startup placement, remaining
FileManager/native/rendered/packages/final acceptance stay open;43/47 unchanged.

P3-05,2026-10-10: complete PanelOperations body/caller disposition and original
void Comment state/error/guarded Refresh lifecycle restored. Public source
publication RED0→1 retained; joined native property worker and quiet Cancel
adaptations remain. Final required macOS full311/311 PASS225.98s,
4891Qt/0fail/18 existing skips. Same-source Linux/Windows Qt6.12 each10/10
PASS30.24s/97.08s,295Qt/0fail/0skip;1424 archive/local/actual-guest hashes exact,
1423 retained after task wrapper removal. Current132 Comment JSON reviewed,
with documented status elision/empty fields separated from unexplained clipping.
The initial unpinned-runtime Mac307/311 failure is preserved. See
[source receipt](../upstream-port-audit.md#p3-05--paneloperations-disposition-and-source-comment-lifecycle-2026-10-10).
Eleven slow suites excluded; remaining FileManager/native/rendered/package/final
acceptance remains mandatory. Parents43/47 and P3-03/04/05/06 stay open.

P3-05,2026-10-10: complete PanelMenu body/caller disposition and source
OnContextMenu header/list/keyboard/empty/Cancel/plugin/file lifecycle restored.
Captured-pane Qt popups preserve source command and column ownership; actual
old-product Create File RED8 and Linux/Windows header-catalog RED8 each are
retained. Popup Close RED8/provider-release is fixed through the existing
application CallScope. Final required macOS gate310/310 PASS237.73s,4877Qt/0fail/18 existing
skips. Linux/Windows x64 Qt6.12 each10/10 PASS21.88s/73.61s,
181/185Qt/0fail/0skip;1423 archive/isolated/actual-guest hashes exact,
1422 retained after temporary wrapper removal;108 current popup JSON reviewed.
Scope and platform limits: [source receipt](../upstream-port-audit.md#p3-05--source-panel-context-menu-lifecycle-2026-10-10).
Eleven slow suites excluded. Whole FileManager/native/rendered/package/final
acceptance remains open;43/47 and P3-03/04/05/06 are unchanged.

P3-05,2026-10-10: complete PanelSelect body/caller disposition and source
Select/Deselect dialog lifecycle are reconciled with one wildcard algorithm.
Eight bounded both-pane EN/RU standard/alternative consumers cover Cancel,
mask/type/All/Invert, inactive owning-panel dispatch, provider/peer/focus and
protected bytes. Required isolated macOS gate309/309 PASS216.07s,
4856Qt/0fail/18 existing skips; two earlier full-gate failures and unchanged
focused retries are retained. Linux/Windows x64 Qt6.12 each8/8 PASS10.43s/49.96s,
114Qt/0fail/0skip;1419 frozen archive/local/actual-guest input hashes exact,
1418 retained after temporary wrapper removal;96 current Combo JSON reviewed.
Scope, provenance and limitations: [source receipt](../upstream-port-audit.md#p3-05--source-selection-command-lifecycle-2026-10-10).
Eleven slow suites excluded. Whole FileManager/native/rendered/package/final
acceptance remains open;43/47 and P3-03/04/05/06 are unchanged.

P3-05,2026-10-10: complete source Split/Combine scenario restored original
CApp workflow, peer destination, UInt64/Trim, contiguous-prefix discovery,
empty parts, NIO/preallocation and native workers, with narrow Qt progress/I/O/
transaction services and joined lifetime. Nine old-product RED rows retained;
EN/RU both-pane normal/flat success/failure/cancel/input-change consumers pass.
Required macOS full306/306 PASS251.91s; final localized6/6 PASS2.04s.
Linux/Windows x64 Qt6.12 each6/6 PASS7.64s/20.22s,106Qt/0fail/0skip,1416
local/archive/guest input hashes exact. Provenance, adaptations and platform
sharing behavior are in the [source receipt](../upstream-port-audit.md#p3-05--complete-source-splitcombine-scenario-2026-10-08).
Eleven slow suites excluded. Remaining full-body FileManager/native/rendered/
package acceptance remains mandatory;43/47 and P3-03/04/05/06 remain open.

P3-05,2026-10-10: complete source Create File/Folder restores original CPanel
workflow, pre-dialog snapshot, language IDs, validation/errors, native provider
calls, first-component focus and alternative marks. Source failure-refresh
policy and Qt deferred-scan guard handoff are verified; the initial full-gate
large-list failure is retained with its fix. Two normalized source bodies match.
Mac required full307/307 PASS248.66s,4804Qt/0fail/18 existing skips;
Linux/Windows x64 Qt6.12 each8/8 PASS25.71s/112.83s,159/162Qt/0fail/0skip,
1417 exact local/archive/guest hashes. Forty-eight current Combo JSON reviewed.
See the [source receipt](../upstream-port-audit.md#p3-05--source-create-filefolder-scenario-2026-10-10).
Eleven slow suites excluded. Full FileManager/native/rendered/package acceptance
remains mandatory;43/47 and P3-03/04/05/06 remain open.

P3-05,2026-10-10: source RenameFile/OnBeginLabelEdit now own focused-row
activation independently of operated marks, with narrow owning-pane table/icon
EditLabel transport. Eight old-product RED cases retained; Escape, reactivation,
Unicode/spaces, peer bytes and two symbolic-link targets are verified. Twenty
normal/compact editor JSON reports inspected; both normalized source bodies match.
Mac required full307/307 PASS248.24s,4812Qt/0fail/18 existing skips; later
localized final32Qt/0fail/0skip pass on macOS/Linux/Windows. Broad Linux11/11 and
Windows10/10 safe consumers pass26.76s/67.37s; two Windows native-window exposure
consumers remain unverified without an active desktop. See the
[source receipt](../upstream-port-audit.md#p3-05--source-rename-activation-scenario-2026-10-10).
Eleven slow suites excluded. Full FileManager/native/rendered/package acceptance
remains mandatory;43/47 and P3-03/04/05/06 remain open.

P3-05,2026-10-10: source Delete lifecycle restores original CPanel refresh/
notification phases, No/Cancel early return, source default Yes/Enter and one
captioned worker/trash error. Window's independent completion path is removed;
deferred scans consume restored guards, archive Move retains its transaction seam.
Old-product refusal RED8 and intermediate default/Enter RED10 are retained.
Mac required full308/308 PASS224.62s precedes the localized default correction;
final app/affected build and9/9 PASS21.80s,188Qt/0fail/0skip. Linux final9/9
PASS33.06s; Windows initial8/9 heartbeat failure and unchanged-snapshot retry9/9
PASS85.27s remain distinct. Exact1418 input hashes and96 current message JSON
are reviewed; wrapper removal preserves1417 retained inputs. See the
[source receipt](../upstream-port-audit.md#p3-05--source-delete-lifecycle-2026-10-10).
Eleven slow suites excluded. Whole FileManager/native/rendered/package acceptance
remains mandatory;43/47 and P3-03/04/05/06 remain open.

P3-05,2026-10-10: complete six Options page bodies/headers/resources/callers
are reconciled. Source association errors report once without vetoing later-page
Apply or OK; invalid RAM retains PSNRET_INVALID, raw input and source MyMessages
Critical/suppression policy. Actual old-product RED retained; combined errors,
correction, Cancel and restart verified. Mac required full308/308 PASS240.96s,
4853Qt/0fail/18 existing skips; Linux/Windows x64 Qt6.12 each10/10 PASS2.00s/
12.72s,80/81Qt/0fail/1Windows-registry-skip on Linux only. All1418 frozen inputs
match local/archive/both guests;1417 remain unchanged after wrapper removal.
Forty current page/error JSON per OS inspected. See the
[source receipt](../upstream-port-audit.md#p3-05--six-options-pages-and-apply-errors-2026-10-10).
Eleven slow suites excluded. Whole FileManager/native/rendered/package acceptance
remains mandatory;43/47 and P3-03/04/05/06 remain open.

## Рабочие итерации после укрупнения (2026-09-29)

P3-05,2026-10-10: operation-prompt review restores source memory validation
MyMessages suppression/title and modal MessagesDialog stack lifetime with native
Close/Enter/Escape results. Four actual old-product RED rows retained; normal/
compact, EN/RU, retry and direct native callback consumers verified. Mac required
final full308/308 PASS287.67s,4860Qt/0fail/18 existing skips; first305/308 failed
on the missing copied SFX fixture and is retained, recovery3/3 PASS8.42s.
Linux/Windows x64 Qt6.12 each11/11 PASS10.84s/28.55s,140Qt/0fail/0skip.
All1418 inputs match local/archive/actual guests;1417 retained after wrapper
removal. Thirty-eight widget reports per OS fit. See the
[source receipt](../upstream-port-audit.md#p3-05--operation-prompts-and-source-modal-lifetime-2026-10-10).
Password portable DTO/legacy process caller is subsequently reconciled in the
[password receipt](../upstream-port-audit.md#p3-05--password-state-and-process-callers-2026-10-10).
Whole FileManager/native/rendered/package acceptance remains mandatory;
43/47 and P3-03/04/05/06 remain open.
Eleven slow suites excluded.

P3-05,2026-10-10: complete PasswordDialog state/caller reconciliation restores
source UString and accepts raw empty asynchronous process passwords. Actual RED
and public Unicode/spaces/empty/cache/Cancel/reopen regressions are retained.
Mac full310/310 and Linux focused10/10 pass; Windows9/10 retains two native
window-exposure failures, while password rows pass. Interactive retry cannot
start without a logged-on runner desktop; its own pending task is removed.
Fourteen widget reports per OS fit;1420 frozen inputs match actual guest trees,
1419 retained after wrapper removal. See the
[source receipt](../upstream-port-audit.md#p3-05--password-state-and-process-callers-2026-10-10).
Whole native/rendered/package and parent acceptance remain open.

Применяются обновлённые правила [размера итерации](../porting-plan.md#размер-итерации-и-коммита-уточнение-2026-10-07).
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
