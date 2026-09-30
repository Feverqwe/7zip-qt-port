# P0-02 — historical source-port evidence

Archived on 2026-09-30 from docs/upstream-port-audit.md at project commit
`31cb3bb44dcf1ad1b05f9a362db59e12429b3c17`. The sections below are preserved
verbatim, including intermediate statuses, inventories and deferred dependencies.
They are dated evidence, not a work queue or the current completion status.

Current status and remaining owners: [checklist](../porting-checklist.md#p0-02--owners-и-panel-state).
Current source record: [audit](../upstream-port-audit.md#p0-02--accepted-owners-and-state).
Current workflow differences: [parity](../7zfm-parity.md#p0-02--owners-selection-and-lifetime).
All source repository references, exact upstream revisions, original paths,
copied/adapted/omitted bodies and historical checks remain in their original entries.
Later integrated results supersede earlier claims that their prerequisites remain open.

## P0-02.3 — operation-state and window-close integration (accepted, 2026-09-29)

Baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
https://github.com/ip7z/7zip. Original `CPP/7zip/UI/FileManager/FM.cpp`
`WndProc::WM_CLOSE` supplies soft exit → revoke drop target → conditional
CApp::Save → ReleaseApp → conditional SaveWindowInfo → hard exit. This body is
copied at its original path through `IQtFileManagerWindowServices`;
Qt closeEvent supplies destruction. Qt asynchronous workers are additionally
joined before Save/ReleaseApp while folders and temporary inputs live.
The global exit launcher becomes existing per-panel launchers. Win32 activation/
drop registration becomes Qt deactivation/acceptDrops removal; full drag/drop
remains P0-08. Only WM_CLOSE is ported here: startup, command dispatch and other
message branches remain mandatory.

Original FM.cpp::ExecuteCommand, App.cpp::CApp::OnCopy and PanelOperations.cpp
retain timer/notify guards around synchronous modal operations. No upstream
asynchronous state owner exists. QtAppOperationState holds the original CPanel
RAII guards across worker completion for both independent panels, owned by CApp.
Scan permits navigation while suppressing inconsistent selection notifications;
Open/Operation block consumers. Transition retains guards, completion restores
them after rows/selection agree, close restores them before disabling all panels.
A scoped live-call counter rejects reentrant close that would release a caller's
folder; modal/open close is rejected like a disabled Win32 parent. Qt enums
remain presentation adapters; complete command policy, OnTimer and RefreshListCtrl
remain P0-03/P0-05.

ViewSettings.cpp/.h::CWindowInfo::Save/Read copy original binary record loops,
macros, keys, flags, signed conversion and exact-size validation: Position is five
little-endian DWORDs, Panels three. QSettings QByteArray replaces Registry binary
values; four Int32 fields replace RECT. Wrong QVariant types are rejected.
The close consumer supplies Qt logical normal geometry with exclusive right/
bottom, maximized state and CApp panel/focus fields. Until P0-08 has a visible
splitter, its saved coordinate is preserved (half width only when absent).
Placement restoration and complete splitter behavior remain mandatory P0-08.

FilesystemScanner::cancelAndWait is Qt thread quiescence, not an enumeration
algorithm port: cancel/join/delete loaders before folder release. QPointer
protects already queued finished callbacks: disconnect cannot remove a MetaCall.
Backend/scanner/file/volume callbacks stop before join; terminal-state guards
reject queued completion and deferred view/hash actions. Destructor invokes the
native close body too, with forced quiescence if a modal child vetoed close.
Source notices remain; repository formatting is applied separately.

Focused close/owner/window-info/scanner tests passed 4/4 (1.70 s); expanded
binding_lifecycle/ui_operations/ui_navigation/ui_archive_states and those four
passed 8/8 (7.54 s, macOS arm64, Qt 6.11.2, 2026-09-29). `./scripts/check.sh`
passed formatting, app/all-consumer build and 122/122 CTest (84.83 s).
`git diff --check` passed; upstream checkout remains clean. P0-02 acceptance
combines P0-02.1 shared state, P0-02.2 creation/focus/persistence/independence,
P0-04.1 prerequisite native location/selection owners and P0-02.3 operation/
close lifetime, with root/nested/address/refresh/selection/start/error/cancel/
close consumers in the full gate. This accepts the required owner component,
not complete App.cpp/Panel.cpp/FM.cpp modules. Historical omitted branches remain
mandatory under P0-03 command/title policy, P0-04 remaining navigation, P0-05
model/timer, P0-08 second visible panel/drop/placement and P1-08 editor/child
process owners; P0-10 still requires all-consumer progress integration.
Windows/Linux/rendered Windows parity unverified.
No visible layout/text changes.

## P0-02.2 — application/panel lifecycle owner (accepted iteration, 2026-09-29)

Baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
https://github.com/ip7z/7zip. Current internal work copies App.cpp
CApp::ReleaseApp and Panel.cpp CPanel::ReleasePanel at their original paths.
All panels disable timer/notify/status-bar before the first folder release,
including the invisible panel. MainWindow calls ReleaseApp after its existing
backend/scanner/file-operation quiescence. The local per-panel external-file
launcher is joined before its folders are released; upstream uses a global
launcher. Win32 context-menu COM owners are absent from the Qt port and their
mandatory integration remains P0-03/P0-09/P2-09, not a completed lifecycle dependency.

ViewSettings.cpp/h CListMode::Read/Save bit packing and GetPanelPathName,
ReadPanelPath/SavePanelPath are copied with explicit QSettings storage replacing
Registry. The ListMode/PanelPath keys remain original; invalid or overflowing
DWORD values retain source defaults. RegistryUtils.cpp/h ReadFlatView/SaveFlatView
use the original per-panel FM/FlatViewArc keys and existing bool registry adapter.
These persistence dependencies are internal to the owner iteration.

App.cpp/h now copies Create/CreateOnePanel/Save creation and persistence control
flow: visible-panel filtering, invalid focus fallback, combined one-panel width,
main archive owner attempted first, needOpenArc early return and per-panel
settings. CPanelCallbackImp retains its original class/path and index/focus/tab
routing. The former g_App reference in OnTab is per-instance `_app`. Panel.cpp/h
copies Create path normalization/binding, needOpenArc and creation result rules,
and OnCreate name-sort/ascending/list-focus defaults and view-mode clamping.
QtAppPresentation replaces Win32 menu/toolbar/child-control construction. It
provides the existing single visible panel; the second presentation/splitter is
mandatory P0-08. PanelDrag.cpp retains the SetFocusedPanel owner assignment at
its source path; CDropTarget synchronization remains mandatory P0-08.

MainWindow now invokes CApp::Create rather than constructing the panel directly;
rendering an already bound filesystem folder does not close/rebind its source
owner. Native panel callbacks own focus changes. Qt teardown suppresses callback
dispatch after notification shutdown; guarded Qt widget references replace HWND.
SetListSettings retains CFmSettings values for all panels and passes view styles
to the Qt adapter. CApp::Save runs after workers quiesce and before ReleaseApp,
and stores the outer archive's parent rather than an internal virtual folder.
The legacy view/mode and view/flatArchive keys migrate once only when the source
per-panel keys are absent. Source keys take precedence; ongoing view/flat changes
use CListMode/SaveFlatView. Disk flat compatibility state remains a Qt scanner
integration constraint pending P0-05. Source complete RefreshListCtrl/OnTimer,
command/title/drop bodies and full Win32 menu-equivalent integration remain
mandatory P0-03/P0-05/P0-08/P0-11. Their current Qt consumers are not source
completion evidence; the owner callback boundary does not claim these bodies.

Focused owner/window builds pass. app_lifecycle/binding_lifecycle/settings/
panel_focus pass 4/4, 4.90 s on macOS arm64 2026-09-29 before additional startup
fixtures. Native owner fixtures cover disable-all-before-release, hidden owner,
idempotence, source settings, creation failure, early non-archive return and
separate panel state. New window fixtures check preference migration/precedence
and outer parent persistence. Initial flat fixture expected one row, but native
flat traversal includes its implicit directory too; the corrected fixture checks
both rows and the visible child. Final owner/window focused checks pass 2/2,
0.87 s, including archive startup success/failure/cancel. Final check.sh
formatting/build/all CTest PASS 121/121, 85.30 s, macOS arm64 2026-09-29,
/tmp/sevenzip-p0-02-2-check.log. The 16 current full-gate archive-runtime reports
show address and active views fully visible at normal/compact sizes in all four
normal/flat modes. Required formatting is separate from functional adaptations.
Windows/Linux runtime and rendered Windows parity remain unverified. The
integrated owner iteration is accepted; P0-02.3/full parent acceptance is mandatory.

## P0-02.2b3c3b2b3a — portable startup wait boundary (2026-09-29)

Project baseline `e9908a5`. Exact upstream baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` in
[ip7z/7zip](https://github.com/ip7z/7zip). Inspected
`CPP/7zip/UI/FileManager/ProgressDialog2.cpp::CProgressDialog::Create`,
`ProgressDialog2.h::WaitCreating`, `CPP/Windows/Thread.h`,
`CPP/Windows/Synchronization.h`, `C/Threads.c` and `.h`.
Original Windows startup waits for worker HANDLE index 0 or creation event index
1 with kCreateDelay (500 ms). Portable CThread supplies create/detach/join, and
CEvent supplies blocking wait; neither exposes timed worker completion or a
multi-object wait. There is no usable POSIX implementation of that service.

qtprogresswait.cpp/.h is new narrow portability glue, not a copied functional
workflow. It provides latched requested/finished signals, finish precedence when
both are set, mutex-protected predicate checks and a monotonic QDeadlineTimer
condition wait that retains one deadline across wakeups. No GUI event processing,
statistics, cancellation, thread creation or operation logic is added. Original
Windows WaitForMultipleObjects and source native worker/events remain the planned
owners; full integration is mandatory b3b. Portable fast completion must join,
because Thread_Close detaches rather than proving worker exit.

progress_wait checks zero/finite timeout before releasing an owned native
CThread, requested/finished wakeup, latched idempotence and finish precedence in
both signal orders. Native CManualResetEvent gates the worker, and RAII releases
and joins it on all assertion paths. No production internals are exposed.
No presentation changes or full dialog/workflow acceptance follow from this
service prerequisite. Windows/Linux runtime remains unverified locally.
Focused checks passed 3/3 in 0.56 s; full check.sh formatting/build/CTest passed
96/96 in 63.43 s on macOS 2026-09-29. Source inventory stays 42 mirrored CPP
(35 UI/seven support); root Qt/integration CPP becomes 42. Vendor bytes unchanged.

## P0-02.2b3c3b2b2b2b3b — source priority/default button (2026-09-29)

Project baseline `ab289b9`; exact upstream baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` in
[ip7z/7zip](https://github.com/ip7z/7zip). Sources:
`CPP/7zip/UI/FileManager/ProgressDialog2.cpp` OnPriorityButton,
SetPriorityText, SetTitleText; `.h` member declaration;
`ProgressDialog2a.rc` DEFPUSHBUTTON Background and PUSHBUTTON Pause;
`ProgressDialog2.rc` Foreground string. Retain original _background toggle,
SetPriorityText call and existing copied caption/title bodies. Native priority
request delegates to processpriority, retaining original Windows API policy
and previously verified POSIX services. No new scheduler algorithm in widgets.

Necessary deviation: original SetPriorityClass ignores failures. On native
failure, reverse the toggle and append the real HRESULT via source
Sync.AddError_Code_Name; the old caption/title stays truthful and original
elapsed-second error display shows the error. Enable Background only when actual
native GetBackground capability succeeds. Preserve original initial _background
false assumption and process-wide policy lifetime; destructor does not reset it.
Qt services restore source default Background, initial focus and Enter on focused
Pause. Cancel remains disabled until full modal/worker lifecycle is ported.

progress_priority checks actual native process policy, both captions/titles,
Background Enter/default/focus, focused Pause Enter without priority change,
restoration and normal/compact button fitting/current JSON. RAII restores the
test process's initial native policy. progress_priority_failure owns a copied
Unicode/space executable and uses Darwin external background enforcement to
produce a real foreground refusal; verify toggle rollback, one source error,
visible error controls, then remove external enforcement and verify successful
retry. Child and policy are joined/restored on all assertion paths. Fixture
uses app.exit rather than quit: this incomplete source view intentionally rejects
closing until its mandatory modal/cancel lifecycle is ported. No production
close behavior is changed. No fake native service or exposed production internals.

Twenty current JSON reports map all 26 source controls, including native
background/foreground captions at 760×450 and 640×380. Windows/Linux runtime,
rendered Windows/Dock reference, full modal/worker/cancel/finish/error workflow
and 7zz child policy propagation remain mandatory subsequent/platform checks.
Focused checks passed 6/6 in 13.81 s; full check.sh formatting/build/CTest passed
95/95 in 64.46 s on macOS 2026-09-29. All 20 current JSON reports passed source
parity contracts; inspected button bounds/text/size hints at both sizes, no clipping.

## P0-02.2b3c3b2b2b2b3a — original taskbar lifecycle (2026-09-29)

Project baseline `0a84d24`. Original baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` in
[ip7z/7zip](https://github.com/ip7z/7zip), files
`CPP/7zip/UI/FileManager/ProgressDialog2.cpp` and `.h`.
Copied constructor CoCreateInstance/HrInit, destructor NOPROGRESS clear,
SetTaskbarProgressState (both overloads), OnInit target selection and state call,
SetProgressPos value call, UpdateStatInfo first-error state call and OnPauseButton
state call. Preserve native state precedence, range/position cache and call order.
Mandatory clang-format changes formatting separately from these functional bodies.

Windows retains CMyComPtr/ITaskbarList3 and exact SDK declarations. Qt platform
owns GUI COM initialization; ole32 is linked explicitly. GetNativeTaskbarWindow
converts the guarded QWidget target to HWND only at the native service boundary.
GetParent and original *this target become parentWidget and this. QPointer starts
null and clears on widget destruction, avoiding use of an uninitialized or retired
target before OnInit/after owner deletion. POSIX constructor initializes the
previously verified CQtTaskbarProgress and discards it on real capability failure;
the source state/value/destructor control flow stays the same.

New progress_taskbar tests observe actual Cocoa Dock API state from the source
dialog: counters, pause/resume, error precedence, end value and prior badge restore.
Main/parent/dialog target branches, never-shown lifetime and deleted main target
are exercised; the process-wide Cocoa badge cannot distinguish per-window targets.
No production internals are exposed. Original blocking Set_NumBytesCur requires
resuming before a synchronous GUI-thread update; the test follows that contract.
Background/default-button integration remains mandatory b3b; complete modal,
worker, cancel, finish and operation integration remain later mandatory scopes.
Windows/Linux native compilation/runtime and rendered reference remain unverified.
Focused checks passed 5/5 in 11.85 s; full check.sh formatting/build/CTest passed
93/93 in 58.93 s on macOS 2026-09-29. No layout/text/availability changes require
new appearance checks; existing progress geometry/reference consumers passed.

## P0-02.2b3c3b2b2b2b2b — Linux session/identity adapter (2026-09-29)

Project baseline `75de494`; original 7zip baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
No upstream Linux taskbar owner exists. qttaskbarprogress_linux.cpp provides
the narrow native CQtTaskbarProgress interface using the verified launcher
transport. Require an actual QGuiApplication on its GUI thread, use its desktop
entry basename plus .desktop for the native URI, and select the real session bus
only inside this platform adapter. Reject missing identity, failed initialization,
wrong thread and null window target. No widget or archive backend publishes DBus.
The Linux adapter is linked into sevenzip_panel only on Linux; Apple tests compile
it in a separate executable so Cocoa's implementation remains authoritative there.

Set the application's Qt desktop basename to sevenzip_desktop through one
desktopidentity constant in main.cpp. This matches the executable and reserves
the entry identity required by later P3-05 packaging. It does not install a desktop
entry, icon, association or claim packaging complete. Qt's
[desktopFileName property](https://doc.qt.io/qt-6/qguiapplication.html#desktopFileName-prop)
expects the basename without a path or trailing extension. Main remains local Qt
startup glue; the full original FM startup is not accepted by this identity change.

Extract the existing bounded private-bus test fixture unchanged into its own
files/library for reuse, rather than duplicate daemon/connection teardown. New
taskbar_adapter exercises the actual adapter in an owned copied executable with
a Unicode/space name. Its environment redirects only that child's session bus to
the private fixture; an explicit fixture-address guard prevents accidental live
session use from child mode. Observe native signals and Query replies while its
GUI event loop runs; verify source-shaped normal/error/pause/value calls, null/
invalid/unsupported failures, joined worker thread failure without state change,
explicit owner destruction/entry removal and child shutdown. Separate child modes
check absent GUI, missing/invalid identity and nonexistent bus. Every owned daemon,
child and thread is stopped/joined before fixture destruction. No production
internals are exposed for tests.

macOS arm64 affected app/adapter/launcher targets build; focused adapter/launcher/
Cocoa/app smoke 4/4 passed (2.09 s). Mandatory ./scripts/check.sh format/app
build/CTest passed 92/92 (60.72 s), exit 0. After a local CMake-only header-list
registration, affected targets rebuilt and the same four cases passed again
(2.09 s); implementation unchanged, diff-check clean.
These checks verify portable C++/Qt/DBus boundaries on macOS, not a Linux kernel,
windowing system or rendered launcher. Actual Linux and Windows runtime checks,
P3-05 descriptor/icon/packaging, source Windows/taskbar/priority/default-button
integration b3 and full modal/worker/cancel/update remain mandatory. No new view
layout/text geometry changed; actual desktop rendering remains unverified.

## P0-02.2b3c3b2b2b2b2a — native launcher transport prerequisite (2026-09-29)

Project baseline `62cee4d`; 7zip source remains pinned to
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Original ProgressDialog2 taskbar bodies remain mandatory b3; upstream has no
Linux launcher owner. qtlauncherprogress is new narrow OS protocol glue, not a
replacement source statistics/state implementation. It owns an explicit DBus
connection and one native launcher entry; widgets and archive operations do not
select buses or publish protocol maps. Reuse native HRESULT/UInt64 and TBPFLAG.

Implement the documented
[Ubuntu launcher protocol](https://wiki.ubuntu.com/Unity/LauncherAPI): application
desktop-ID URI, Update changed-property maps, Query complete state at the same
object path, and full replay when com.canonical.Unity becomes owned. QObject
exports only the explicitly scriptable protocol method/signal. Each instance has
a unique object path, without mutable global identity/state. Publish native
double progress, visibility and urgency; clamp display fraction, preserve error
state on value updates, and clear owned visibility/urgency before unregistering.
Missing bus, invalid desktop basename, missing initialization, wrong owner thread
and disconnected transport return failures. Do not change cached native state
after a failed publish. Idempotent changes send no redundant delta.

QtDBus is used instead of adding libunity/GLib dependencies. The protocol's
stability limitation is explicit; Qt itself uses the same launcher interface for
native Unix badges in
[Qt v6.11.2 qdesktopunixservices.cpp](https://github.com/qt/qtbase/blob/v6.11.2/src/gui/platform/unix/qdesktopunixservices.cpp).
No Qt functional body is copied. Native protocol facts are used at the OS
boundary; source dialog pause/error/target decisions remain outside this owner.
There is no colored paused/error or indeterminate progress property: pause keeps
the fraction, error uses native urgency, and indeterminate returns ENOTSUP rather
than a fake animation. The original dialog only requests normal/paused/error/
no-progress states. Desktop compatibility and rendering are separate from successful
transport; no mandatory source branch is declared N/A.

launcher_progress starts only an owned private dbus-daemon and two explicit
connections. Native signal/reply round trips check Unicode/space desktop IDs,
double/map signatures, deltas versus complete Query, service-name registration/
restart replay, zero/UInt64/over-total values, error/pause/normal/clear,
unregister-on-destruction, invalid arguments, absent/disconnected bus and joined
worker owner-thread failures with state preservation. No live session bus or
external service is used. Sandbox bind fails EPERM; authorized unsandboxed checks
exercise the private Unix socket. QtDBus is required for Linux, and on Apple only
for these tests; dbus-daemon is a Unix test dependency. CTest is initialized before
dependency selection so default BUILD_TESTING configurations do not omit coverage.
macOS arm64 focused launcher/Cocoa/priority/sync 4/4 passed (1.52 s); mandatory
./scripts/check.sh format/app build/CTest passed 91/91 (58.59 s), exit 0;
diff-check clean. Linux application identity,
CQtTaskbarProgress integration and actual desktop rendering remain mandatory b2b;
Windows/source UI and modal/worker/cancel/update remain open.

## P0-02.2b3c3b2b2b2b1 — Cocoa Dock service prerequisite (2026-09-29)

Project baseline `ab47135`; original taskbar call sites remain pinned to
ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at
https://github.com/ip7z/7zip, ProgressDialog2.cpp/.h constructor, OnInit,
SetTaskbarProgressState, SetProgressPos and destructor. Upstream has no POSIX
taskbar implementation. New qttaskbarprogress_mac.mm is narrow platform
presentation glue, not a replacement for those mandatory original bodies.
qttaskbarprogress.h carries their native HrInit/SetProgressState/SetProgressValue
interface and TBPFLAG ABI constants (same values as MyWindowsNew.h).

Cocoa uses the application's process-wide Dock badge rather than a Windows
per-window colored bar. It shows a percentage, an ellipsis when indeterminate,
and pause/error markers; selected-window policy still belongs to the source
dialog. Source counts remain UInt64; native badge text is a clamped approximate
percentage, and does not replace source statistics or progress arithmetic.
[Apple badgeLabel](https://developer.apple.com/documentation/appkit/nsdocktile/badgelabel?language=objc)
defines native badge text and system-owned appearance. Preserve the initial badge
when clearing or destroying this owner; avoid erasing a later badge installed by
another owner. Initialize only on the GUI thread with an existing NSApp/dockTile;
do not create an application or advertise capability for an offscreen platform.
Missing application and non-GUI calls report real errors. SDK/native object
ownership stays entirely in the Objective-C++ file behind an opaque CImpl.

Use unchanged MyTypes.h/7zTypes HRESULT/UInt64 types at this boundary.
MyWindows.h cannot enter Objective-C++: its Win32 BOOL=int conflicts with Apple's
BOOL=bool. This is resolved by a narrow include boundary, without modifying
vendor types or leaking Cocoa into source CProgressDialog. AppKit is linked only
on Apple; CMake enables C++17 Objective-C++ only there. The existing formatting
gate now checks .mm files as well as C++ headers/sources.

taskbar_progress_mac checks actual AppKit badgeLabel after value/state/clear,
blocking pause/error states, UInt64/max/over-total values, idempotent initialization,
invalid arguments, destructor cleanup and another badge owner's preservation.
A separate owned child with QCoreApplication proves absent-Cocoa capability;
a joined Qt worker verifies non-GUI failures leave the badge unchanged. Neither
uses another application or a live service. macOS arm64 focused service/progress/
current JSON consumers 4/4 passed (9.48 s). Mandatory ./scripts/check.sh
format/build/CTest passed 90/90 (57.75 s), exit 0, with Objective-C++ formatting
and current progress JSON consumers included; diff-check clean.
No source taskbar branch or Background button is accepted by this prerequisite;
Linux desktop service, Windows bodies/target/lifetime, source background/default
button and full modal/worker/cancel/update remain mandatory. No rendered Dock or
Windows pixel fidelity is claimed from native API getters or Qt geometry.

## P0-02.2b3c3b2b2b2a — native process background prerequisite (2026-09-29)

Project baseline `d2307a2`; source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Copy complete unchanged `CPP/Windows/ProcessUtils.h`, including CProcess::Open
and inherited CHandle ownership. Only those inline process services are consumed;
Create/MyCreateProcess and PSAPI services are not linked or claimed ported.
Original ProgressDialog2.cpp::OnPriorityButton uses GetCurrentProcess and
SetPriorityClass(IDLE_PRIORITY_CLASS/NORMAL_PRIORITY_CLASS). processpriority
preserves that Windows API boundary, adding an owned-child CProcess handle and
real HRESULT failures for the external 7zz transport. The source button body
and child transport propagation remain mandatory subsequent integration.

Upstream has no equivalent Darwin/Linux service for this Windows-only branch.
New narrow platform glue is therefore necessary. Darwin setpriority with
PRIO_DARWIN_PROCESS uses PRIO_DARWIN_BG/0 for reversible process-wide policy.
The [Apple XNU implementation](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/kern_resource.c)
sets the target task but get_background_proc queries current_task, so
getpriority(child) cannot verify the child. Query proc_pidinfo(PROC_PIDTBSDINFO)
instead, using the effective internal/external Darwin-background flag bits
0x8000/0x10000 documented in Apple's
[proc_info.h](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/proc_info.h).
These flags are private SDK definitions: local constants are an explicit OS
compatibility dependency, not copied functional code. Verify effective state
after setting; another policy owner preventing foreground restoration returns
an error rather than false success. No unrelated process is targeted.

Linux scheduling is per thread. Enumerate /proc/PID/task and change every live
thread between SCHED_OTHER/SCHED_BATCH, retaining SCHED_RESET_ON_FORK. Native
CRecordVector owns snapshots; bounded rechecks include newly created threads,
which inherit creator policy. Preserve unrelated real-time/idle or mixed policy
by returning ENOTSUP. On failure attempt to restore captured threads; continually
changing thread sets return EAGAIN. Concurrent external scheduler changes and
rollback failures cannot be made atomic by these OS APIs. No irreversible nice
or SCHED_IDLE change is used. Linux code/runtime remains unverified locally.

process_priority uses only its own process and an owned copied test executable
with Unicode/spaces. A native worker exists before switching the child; separate
native observations verify both main and worker, idempotence, repeated restoration,
invalid PID/output preservation and join-before-destruction. macOS arm64 focused
native/progress consumers 5/5 passed (0.76 s). Sandbox setpriority returns EPERM;
authorized unsandboxed checks exercise the actual successful API. Mandatory
./scripts/check.sh format/build/CTest passed 89/89 (58.07 s), exit 0; exact
vendor byte comparison passed 85/85 (84 dependencies/resources plus license),
diff-check clean. Windows compilation/runtime remains
unverified, as does Linux. No displayed control changed; background/taskbar,
default-button and modal/cancel/update acceptance remain mandatory.

## P0-02.2b3c3b2b2b1 — original progress icon and taskbar header (2026-09-29)

Project baseline `a464b8d`; exact source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Copy unchanged `CPP/7zip/UI/FileManager/MyWindowsNew.h` (native SDK/old-SDK
ITaskbarList interfaces/flags/GUID prerequisite) and FileManager/FM.ico into the
same vendor paths. The header is included on Windows only; native taskbar bodies
remain mandatory b2. All original SDK conditionals/definitions/bytes are retained.

Copy the original ProgressDialog2.cpp::OnInit `if (IconID >= 0)` icon-loading
branch, replacing only Win32 HINSTANCE/MAKEINTRESOURCE/HICON/ICON_BIG services
with Qt LoadIcon(int)/SetIcon(QIcon). Native IDI_ICON remains 1 from the already
vendored resourceGui.h. Qt embeds the exact source ICO through upstream_progress_icons
and preserves all three 16/32/48 frames. Explicit resource initialization keeps
static-library consumers reliable; unknown IDs return an empty icon, while a
negative IconID preserves the existing default icon because the source branch
does not run. No widget statistics, resize or command behavior is changed.

GUI/UpdateGUI.cpp and ExtractGUI.cpp set IconID=IDI_ICON before Create. Their
GUI/FM.ico is byte-identical to FileManager/FM.ico (SHA-256
`649f8e81cce9d8e3ab9a0d06db31788f5ced3c6ed99a78304a87dba35ecf06cc`);
one authoritative original asset serves these callers through the Qt ID mapping.
Other icons/bitmaps, native GUI resources/callers and full application icon/menu/
toolbar migration remain mandatory P0-09/P1-13/P3-05 and related workflow steps.
No redraw or new artwork replaces the original asset. Notice/license baseline is
preserved; required formatting applies only to local C++.

progress_dialog now checks unchanged default ID, actual embedded image frames,
invalid ID and repeated loading through the public windowIcon boundary. Source
icon pixels are not claimed visually equivalent to a rendered Windows window;
there is no Windows rendered reference. The existing 26-control/16-report geometry
contract remains valid. macOS arm64 focused five progress/presentation consumers
5/5, 10.03 s; full `./scripts/check.sh` format/build/CTest 88/88, 53.64 s,
exit 0, diff-check clean and exact vendor bytes 84/84 (83 files + license).
Windows SDK header
compilation/runtime and Linux remain unverified here. Taskbar/background services
and full modal-worker/cancel/finish/update remain mandatory, not N/A.

## P0-02.2b3c3b2b2a2 — source progress presentation subset (2026-09-29)

Project baseline `21ccf28`; exact source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Original `CPP/7zip/UI/FileManager/ProgressDialog2.h/.cpp` supplies CProgressDialog
fields/defaults, complete nested CU64ToI32Converter, error-control visibility,
range/position cache, ShowSize, UpdateStatInfo, title/main-title composition,
pause caption/transition, message splitting/numbering/append/autosize policy,
selected-or-all clipboard loop and no-op OnOK. Copy those functional bodies;
native UInt64/UString/UStringVector/CUIntVector and lock/counter snapshots remain.
UpdateStatInfo preserves original elapsed wrap arithmetic, file-versus-byte
progress choice, unknown totals, remaining/speed/ratio calculations, per-second
refresh, directory/name split and middle truncation. SetProgressRange retains
the source converter's 15-bit scale and endpoint +1. No Qt arithmetic replaces it.

Copy original OnSize control flow: margins/row step from native controls,
character reduction limit, three footer buttons, message minimum-height branch,
two label/value columns and control moves. Adapt only unused Win32 event args,
RECT representation/height, InvalidateRect→update and widget services. OnInit
retains cache reset, controls attachment, created-event signaling, captions,
two columns, error/compression visibility and 200 ms timer. Qt dispatch invokes
OnInit/OnSize/OnTimer/OnNotify/OnOK under the original names. No-op reject keeps
the unfinished cancel route unavailable; it is not a port of OnCancel.

New src/qtprogresscontrols.cpp/.h owns only Qt presentation/platform services:
all 26 resource IDs and initial DLU positions from ProgressDialog2a.rc, plain
labels, progress bar without embedded percentage, existing CListView two-column
row selection, native-style control text/rect/move APIs, font units, QElapsedTimer
modulo-32-bit ticks and clipboard. Original Windows/Control/ProgressBar.h sends
PBM_SETPOS; its documented nearest-boundary clamp is retained in the Qt service
because QProgressBar otherwise ignores out-of-range positions. A failing
regression kept 25 instead of source maximum 101 before this adaptation;
see [Microsoft PBM_SETPOS](https://learn.microsoft.com/en-us/windows/win32/controls/pbm-setpos).
Round font line height upward for QLabel;
initial downward rounding failed labelFits at every size. NormalizeSize adapts
the original Windows/Control/Dialog.cpp work-area/overlap/oversize/maximize
policy; it does not introduce a minimum window size. Fixed Russian captions use
the existing language boundary; full CLang remains P1-14. QPointer protects the
main-window title target lifetime; Ctrl shortcuts also accept macOS Meta.

Partial scope is explicit: taskbar COM initialization/state/value/destructor
calls, hwndForTaskbar selection, IconID resource loading and whole-process
priority/background transition remain mandatory b2b2b. Source CheckNeedClose
calls, OnCancel/OnButtonClicked cancel confirmation, finish-during-question,
OnMessage/OnExternalCloseMessage, WaitCreating/Create/ProcessWasFinished and
CProgressCloser/CProgressThreadVirt remain mandatory b3. Constructor native
events are copied prerequisites, not modal/thread acceptance. Background and
Cancel controls stay disabled. MainWindow still owns its existing operation
progress; callback/update integration remains b2c/b2d. No parent step is closed.

One-off comparison of twelve complete function bodies finds only formatting,
recorded GetTickCount service substitution and deferred taskbar calls; source
SetProgressPos/OnSize/OnInit/OnNotify/clipboard adaptations are reviewed directly.
No code/dependency baseline revision changed. UI extractor now indexes supported
bool/void/INT_PTR member definitions including event helpers, excludes declarations,
and has a deterministic regression; re-extracted progress reference changes only
its navigation symbol list, with identical source/resource hashes and controls.

progress_dialog tests unknown/counter/file-mode/compression/Unicode-path/title
states, source 0/32767/32768/64-bit progress scaling, completion/backward progress,
pause freeze/resume elapsed time, multiline/empty/trailing-LF errors, append
numbering and selected-or-all copy. Geometry covers 760×450 and 640×380,
compression on/off, errors hidden/shown and paused rendering. All 26 controls map
to 16 current-build JSON reports; reviewed ui_parity_progress checks source
grouping/order/conditional states. No screenshots or Windows rendered reference;
pixel fidelity and Windows/Linux runtime remain unverified. Focused source,
parity-tool and existing Copy/Password consumers: final 10/10, 10.10 s on macOS
arm64 2026-09-29, including button fitting and native clamp checks.
Full `./scripts/check.sh` format/build/CTest 88/88, 53.75 s, exit 0;
`git diff --check` clean. No source counts change; root Qt/integration cpp count
38 includes the new narrow control service. Exact vendor baseline remains 82/82.

## P0-02.2b3c3b2b2a1 — source statistic helper bodies (2026-09-29)

Project baseline `aed9488`; exact source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip,
`CPP/7zip/UI/FileManager/ProgressDialog2.cpp`: ReduceString,
ConvertSizeToString, GetChangedString, GetPower32, GetPower64, MyMultAndDiv and
GetNumDigits. Copy all seven complete file-local bodies to the existing mirrored
module, retaining names, static visibility, native UString/UInt64/UInt32/Byte,
thresholds, loops, branch order and overflow approximation. Only required
clang-format differs; all seven bodies compare equal after removing whitespace.

MyMultAndDiv repeatedly shifts the larger factor and the divisor when the sum
of factor bit lengths exceeds 64. At extreme inputs the divisor reaches zero;
the original then returns the reduced product without division. Tests retain
that source behavior, including `(2^63 * 2) / 2` yielding `2^62`, rather than
substituting a newly written exact arithmetic algorithm. No correction to this
upstream calculation is claimed in this port.

The focused test compiles the authoritative ProgressDialog2.cpp in its own
translation unit and links sevenzip_common, avoiding exported test-only helpers,
duplicate source bodies or a visibility change to the original functions.
Regressions cover native size thresholds and buffer guards, ordinary/overflow/
zero arithmetic, every 32/64-bit power boundary, changed/unchanged/empty Unicode
strings with spaces and source message-number digit transitions. Helpers remain
unconsumed by a production CProgressDialog until mandatory a2; the converter,
statistics/control rendering, source resize, titles/messages/clipboard/pause,
platform services and complete modal/thread/update workflows remain unported.
No displayed view or parent acceptance is claimed by this prerequisite.

Checks on macOS arm64 2026-09-29: affected three targets build; focused
progress_statistics_helpers/progress_format/progress_sync 3/3, 0.57 s.
Full `./scripts/check.sh` format/build/CTest 86/86, 44.78 s, exit 0;
`git diff --check` clean. Windows/Linux unverified.

## P0-02.2b3c3b2b1 — source progress resources and duration formatter (2026-09-29)

Project baseline `20f5b1e`; exact source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Copy original ProgressDialog2.cpp GetTimeString and UINT_TO_STR_2 macro into the
existing mirrored module, retaining UInt64 hours, two-digit minute/second logic,
unbounded hour conversion above 99 and caller-supplied native buffer. Declaration
is available at the production header boundary for progress presentation. Native
IntToString is reused; only include paths, notices and formatting differ.

Vendor unchanged ProgressDialog2Res.h/ProgressDialog2.rc/ProgressDialog2a.rc,
GUI/resource3.h/.rc and CPP/7zip/GuiCommon.rc at their original paths. The resource
generator now accepts source BEGIN/END or brace-delimited STRINGTABLE pairs and
requires matching end delimiters. Mixed-resource opt-in and strict table parsing
remain unchanged; malformed/missing/nested tables fail before replacing output.
Progress and update-operation tables join the generated resource bundle; fixed
Russian pause/continue/foreground/cancel/close and operation captions adapt the
existing UI language boundary. Full CLang loading remains P1-14.

Generate docs/ui-reference/progress-dialog.reference.json with the existing
extractor from the exact upstream resource/handlers and shared includes. It
records IDD_PROGRESS, all controls, hashes and the desktop profile; no resource
data is hand-authored. It establishes source grouping/order, not Qt or Windows
appearance acceptance. Full statistic/rendering/title/messages/pause/background/
modal/cancel/thread/finish/error integration remains mandatory b2/b3 and P0-10/
P1-11; no progress view is accepted by this prerequisite alone.

New progress_format tests zero/second/minute/hour boundaries, 99→100 hours,
32/64-bit maximum durations, termination/buffer guard and native caption lookup.
Generator regressions cover original brace tables, mismatched delimiters/nesting
and output preservation. Testing the new fixture against the pre-change generator
reproduces rejection of a valid brace table. No displayed view changes occur.
Validation macOS arm64, 2026-09-29: affected app/progress-format/sync/formatter
targets built; focused 4/4 passed (1.00 s), mandatory check.sh format/build/85 CTest
passed (45.11 s), git diff --check passed. GetTimeString tokens match original
after formatting; exact vendor source/resource/license comparison 82/82 with no
differences (81 dependencies/resources plus License). Reference extraction produced
one original desktop dialog with exact pinned hashes. Source/root CPP counts stay
42/37. Windows/Linux runtime and full Qt progress view remain unverified.

Project baseline `674ec1d`; original CPP/7zip/UI/FileManager/ProgressDialog2.h/.cpp
at ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
https://github.com/ip7z/7zip. Copy complete CProgressSync and both message structs
into the mirrored module, retaining all fields, native types, inline methods,
constructor defaults, macros and every CProgressSync function body. Preserve
native critical-section scopes, stopped-before-paused checks, lock release before
pause sleep, unknown totals, null update preservation, mutation-before-abort,
clear-stop-only behavior and source error accumulation/trailing-LF handling.

Only platform service adaptation: Windows retains Sleep(kPauseSleepTime); POSIX
uses QThread::msleep with the same original 100 ms interval because Win32 Sleep is
unavailable there. Native synchronization and error formatting reuse unchanged
upstream owners from a1. Includes, notices and formatting are other local changes;
no Qt counters, containers, stop/pause policy or error formatter replaces source.

Partial module scope is explicit: full CProgressSync/message records are ported;
CProgressDialog, CProgressThreadVirt, progress rendering/resources/timer/remaining
time/ratio/background/message/pause/cancel UI, worker exception/result handling
and actual archive transport/callback integration remain mandatory b/c/d and
P0-10/P1-11. The existing QProgressDialog operation UI remains migration debt;
this state prerequisite does not claim a complete dialog or end-user workflow.

New progress_sync checks unknown totals, null completed/ratio pointers, scan
metadata preserving completion, Unicode/space paths, null path clearing, real
native worker paused-then-resumed/cancelled, stop precedence while paused,
callback E_ABORT and reopen retaining counters/errors, final-message presence and
source error/name composition. Workers join before source state is destroyed;
failure cleanup stops a paused worker. Bounded CTest timeout and local state only.
No visible UI change. Validation macOS arm64, 2026-09-29: affected app/sync/native
services/formatter targets built; focused 3/3 passed (0.69 s), mandatory check.sh
format/build/84 CTest passed (44.35 s), git diff --check passed. One-off comparison
of all CProgressSync functional bodies matches after removing formatting and the
documented sleep service adaptation. Source becomes 42 mirrored CPP (35 UI/seven
support), FM 29/67; vendor/root counts remain 75 plus License/37. Windows/Linux
compilation/runtime and full progress UI remain unverified locally.

## P0-02.2b3c3b2a1 — unchanged native thread/synchronization/error services (2026-09-29)

Project baseline `4644aae`; exact source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Vendor unchanged C/Threads.c and CPP/Windows/Synchronization.h, Thread.h,
Handle.h, ErrorMsg.cpp/.h at original relative paths. Existing C/Threads.h,
Precomp/Compiler/7zTypes/7zWindows/Common/WinDefs/StringConvert dependencies
already match that baseline. Compile native Threads and ErrorMsg once in
sevenzip_common, linking CMake Threads::Threads for the platform threading
service. Preserve all native Windows/POSIX branches and source notices/bytes.
No Qt mutex/thread/error formatting replaces their algorithms or ownership.

These modules provide CProgressSync's original critical-section/error dependencies
and CProgressThreadVirt's native worker/event join services. Full CProgressSync,
modal progress/thread state, callbacks, archive update and parent-stack integration
remain mandatory a2/b/c/d and their feature owners. Existing independent Qt error
formatters in other modules are migration debt until those consumers are ported;
this prerequisite does not claim their replacement or full platform acceptance.

New native_services uses source manual/auto-reset events, semaphore, native
critical-section lock and two workers incrementing shared state before explicit
Wait_Close. It checks exact joined counts, thread reuse, empty-close idempotence,
event reset/recreation, native error formatting and unknown-HRESULT hex fallback.
Workers use local state only; a failed second create still releases/joins the
first worker. CTest has a bounded timeout. Windows system text is not assumed
English; POSIX HRESULT/errno behavior follows the exact source branch. No visible
UI or archive operation changes. Validation macOS arm64, 2026-09-29: affected
app/native-services/native-library/formatter/folder-link targets built; focused
4/4 passed (0.80 s), mandatory check.sh format/build/83 CTest passed (49.01 s),
git diff --check passed. Exact vendor source/resource/license bytes 76/76 with
no differences, comprising 75 original dependencies/resources plus License.
Mirrored CPP/UI/root counts are unchanged. Windows/Linux compilation/runtime
remains unverified locally.

## P0-02.2b3c3b1 — complete FormatUtils and source caption bundle (2026-09-29)

Project baseline `138f6db`; exact original ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Copy all FormatUtils.cpp/.h functions into the mirrored FileManager owner:
NumberToString's upstream UInt64 conversion and both MyFormatNew overloads,
retaining UString replacement of every literal {0}, native arguments, return
types and functional bodies. Only include paths, removed Win32 StdAfx dependency,
notices and repository formatting change; no Qt replacement algorithm is added.
Original IntToString/MyString and adapted non-Z7_LANG resource lookup are reused.

Copy unchanged FileManager resource.h/resourceGui.h/resource.rc into third_party
at the original relative paths. The exact source STRINGTABLE is added to the
existing generated ResourceString bundle; its two modified-file update/error
captions receive fixed Russian presentation strings with the original placeholders
and line breaks. These captions are prerequisites, not visible dialog integration.
Full source CLang/default-language selection remains mandatory P1-14.

Resource adaptation: resource.rc also contains Win32 menus/accelerators/bitmaps
and included dialogs. The existing generator has an explicit MIXED_RESOURCE_FILES
allowlist whose files must also be declared inputs. It extracts only their table
strings; parsing remains strict inside each STRINGTABLE and rejects missing or
incomplete tables before replacing output. Strict resources retain strict
outer-syntax validation. No referenced include/bitmap/menu is loaded or claimed
ported by caption extraction; full commands/resources/platform presentation remain
mandatory P0-09/P1-13/P3-05 and their feature owners. CMake script minimum matches
the project's 3.21 requirement for IN_LIST policy and deterministic generation.

New format_utils tests UInt64 zero/32-bit boundary/maximum, Unicode/spaces,
multiple/empty/no-placeholder replacements, untouched {1}/%1 and nonrecursive
literal-placeholder arguments, native source-resource overload and missing ID.
Generator regressions cover explicit mixed-mode opt-in, table-only output,
repeat identity, unsupported/missing/incomplete/undeclared inputs and preserving
valid output on failures. No archive operation or visible layout/text is changed.
Validation macOS arm64, 2026-09-29: affected app/formatter/property targets built;
focused formatter/resource-generator/property tests 3/3 passed (0.56 s), mandatory
check.sh formatting/build/82 CTest passed (42.74 s), git diff --check passed.
Vendored source/resource/license byte comparison 70/70 with no differences.
Source CPP becomes 41 (34 UI + seven support), FM mirrored paths 28/67; vendor
becomes 69 unchanged dependencies/resources plus License. Windows/Linux runtime
remains unverified. Parent update/close integration stays mandatory c3b2/b3/c/d/e.

## P0-02.2b3c3a — source processing guards and Qt notifications (2026-09-29)

Project baseline `5d9f9c7`; original ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Copy Panel.h _processTimer/_processNotify/_processStatusBar and complete inline
Disable_Processing_Timer_Notify_StatusBar, CDisableTimerProcessing,
CDisableTimerProcessing2 and CDisableNotify bodies, retaining field names,
save/disable/restore order, nullable pointer and one-time pointer restore,
SetMemMode_Enable and Z7_CLASS_NO_COPY contracts. Formatting is tracked separately
from functional adaptation. PanelItems.cpp RefreshListCtrl now creates the original
timer/notify guards before selection restoration, as in the source function.

Qt adaptation: the original Panel.cpp Create true initialization moves temporarily
to field initializers because Qt CPanel owners exist before full Create migration.
ShouldProcessNotifications is a narrow production presentation query replacing
Panel.cpp OnNotify's original early return when _processNotify is false. Existing
table selection/current-cell adapters use it before refreshActions. No independent
notification policy is added. Timer/status flags and guard bodies are preserved;
full OnTimer/OnNotify/status-bar dispatch and all-panel Disable-before-ReleaseApp
integration remain mandatory P0-02.2c/P0-02.3 and later list/command owners.

The Qt filesystem refresh and flat-mode callers explicitly refresh commands after
the guard restores final selection, corresponding to upstream RefreshListCtrl's
post-Restore status/notification update. Without that final update, Copy/Properties
would remain disabled after selection restoration. Existing panel_selected_state
adds command availability assertions across normal/flat and four presentation
modes; no new layout or visibility policy is introduced.

New panel_processing exercises exception-safe nested notify restoration, a
permanently disabled panel, source SetMemMode_Enable, nullable/idempotent pointer
restore and observable Qt selection notifications during source RefreshListCtrl.
The raw Qt signal still fires but the guarded consumer skips intermediate state;
final selected source indices and resumed notifications are checked. No production
internals are exposed solely for tests. Root/nested update/stack integration is
still mandatory c3b–e; this is their processing prerequisite, not full acceptance.
No visible layout/text change. Validation macOS arm64, 2026-09-29: removing the
RefreshListCtrl guards reproduced intermediate consumerCalls=2 (expected 0);
restored source guards passed focused 4/4 (3.70 s), mandatory check.sh format/
build/81 CTest (42.30 s). Subsequent localized final-command refresh assertions
reproduced eight failures before the fix; affected app and seven consumer targets
rebuilt, final focused 7/7 passed (8.72 s). Required formatting/diff checks pass
after that localized edit; no unchanged full gate is repeated. Windows/Linux
runtime remains unverified, source/vendor counts unchanged.

## P0-02.2b3c2 — original temporary-file and parent-link records (2026-09-29)

Project baseline `1b3ab4a`; original CPP/7zip/UI/FileManager/Panel.h at
ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
https://github.com/ip7z/7zip. Copy complete CTempFileInfo and CFolderLink structs
into the existing mirrored Panel.h, retaining all fields, constructors, comments,
upstream FString/UString/CFileInfo/CMyComPtr/CLibrary/CByteBuffer types and inline
DeleteDirAndFile/WasChanged_from_TempFileInfo/WasChanged_from_FolderLink bodies.
Explicit NeedDelete controls file-then-directory cleanup; directory removal is
nonrecursive. Change detection retains size-or-modification-time comparison and
the original virtual-archive override. No independent Qt record replaces them.

Required platform adaptation: replace Win32 CompareFileTime with the existing
upstream global Compare_FiTime. Windows TimeUtils.h maps it to CompareFileTime;
POSIX's unchanged TimeUtils.cpp compares native timespec seconds/nanoseconds.
There is no new time conversion or comparison algorithm. Includes and formatting
are other local changes; original native cleanup/library owners from c1 are reused.

Partial integration remains explicit: this ports the complete two records and
their helpers, not CPanel's _parentFolders/_library, CloseOneLevel/CloseOpenFolders,
OpenParentArchiveFolder, OpenAsArc or BindToPath/LoadFullPath. Those and nested
changed-archive update workflows remain mandatory c3/c4/b3d and operation owners.
Existing Qt archiveParents_ is migration debt until runtime stack integration.
IsVirtual/Password/ZoneBuf fields are preserved; memory-archive, password and zone
workflows are not accepted merely because their record fields compile.

New folder_link exercises public native records: borrowed/owned cleanup, repeat
cleanup, nonempty parent preservation, Unicode/space paths, native size/time
changes and virtual override, source CObjectVector copy retaining a live folder
and independent CByteBuffer, original Add(empty-link)-then-Library.Attach order,
loaded-copy guard and transfer back before pop. Fixture uses a local compiled
shared library and local filesystem provider; no live archives or services.
No visible UI/layout changes. Validation macOS arm64, 2026-09-29: affected app,
folder-link, native-library, cleanup, path and lifecycle targets built; focused
5/5 passed (2.49 s), mandatory check.sh formatting/build/80 CTest passed (42.34 s),
git diff --check passed. Windows/Linux compilation/runtime remain unverified.
Source/module/vendor counts are unchanged; only an existing header is extended.

## P0-02.2b3c1b — native library lifetime/transfer prerequisite (2026-09-29)

Project baseline `3bfe67e`; exact original source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Copy CPP/Windows/DLL.cpp CLibrary Free/Load Windows and POSIX functional bodies,
Windows LoadEx and POSIX GetProcAddress. Preserve HMODULE/CFSTR/DWORD signatures,
Free-before-load, failed unload retaining its handle, Win98/Unicode selection,
native dlopen flags/dlsym/dlclose and boolean return contracts. Header retains
original platform class branches, names and module ownership.

Required adaptation: source FileManager CFolderLink transfers Library via
Attach/Detach after copying an empty link; upstream POSIX CLibrary lacks those
methods and the Windows loaded-copy guard. Copy original Windows constructor
guard (integer exception 20230102), IsLoaded and Attach/Detach bodies into the
POSIX class. No new transfer algorithm or Qt loader is introduced. Include paths,
notices and project formatting are local adaptations. sevenzip_common builds
one authoritative DLL implementation and links CMAKE_DL_LIBS where required.

Partial scope: MyGetModuleFileName/GetModuleDirPrefix are declared but not ported
here, not marked N/A. DLL.cpp's module-path implementation is Windows-only and
uses g_hInstance; upstream POSIX directory-prefix implementation belongs to
UI/Common/ArchiveCommandLine.cpp and its Console-supplied g_ModuleDirPrefix.
These are not dependencies of parent-link library lifetime. Their resource/tool/
language/options/help callers remain mandatory P1-08/P1-13/P1-14/P1-15/P3-05.
No production in-process archive engine/plugin loading is introduced; 7zz remains
behind ArchiveBackend. Original CFolderLink/stack integration follows c2/c3.

New native_library uses a compiled local C shared-library fixture with one exported
constant-return function. Tests copy it to a Unicode/space path and check failed/
empty load, missing procedure, source loaded-copy rejection, detached transfer,
replacement ownership, repeated Free, failed reload releasing the prior module
and destructor unload. Windows-only LoadEx/transfer coverage is present but its
runtime remains unverified locally. This tests source lifetime/loader contracts,
not full archive workflows or module-path services. No displayed UI changes.

Validation macOS arm64, 2026-09-29: app/native-library/cleanup/lifecycle/typed-folder
targets built; focused 5/5 passed (2.68 s), mandatory check.sh 79/79 (46.93 s),
formatting and git diff --check passed. Source support count increases to seven
mirrored CPP modules (40 total, UI remains 33); root Qt glue stays 37 and vendor
dependencies/resources stay 66 plus original License. Windows compilation/LoadEx
runtime and Linux runtime remain unverified locally.

## P0-02.2b3c1a — native single-item temporary cleanup prerequisite (2026-09-29)

Project baseline `0aa7401`; exact source ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Copy original CPP/Windows/FileDir.cpp DeleteFileAlways and RemoveDir bodies for
Windows/POSIX, plus required Windows SetFileAttrib. Preserve original CFSTR/
DWORD signatures, NFind::GetFileAttrib, readonly-file attribute clearing, directory
guard, Win98/Unicode main-path branches and long-path fallback through existing
source FileName GetSuperPath. POSIX retains remove/rmdir directly. Source comments
and notices remain; project formatting is the only functional-body difference.

Adaptations are header/include paths and the existing namespace layout. QString
DeleteFileAlways now only converts native separators/Unicode through us2fs and
delegates; its independent QFileInfo/QFile readonly policy is removed. Each native
borrowed function has one implementation. This prerequisite introduces no archive
operation or path parser and keeps widgets behind their operation owners.

The broader FileDir port remains partial: recursive Qt RemoveDirWithSubItems and
other FileDir functions/time/attributes/copy/temp services remain mandatory in
their filesystem/platform operation owners and P3-05 closure. This does not port
CTempFileInfo/CFolderLink yet; those follow library support and native time review.
Source POSIX DeleteFileAlways uses remove(), so it can remove an empty directory;
Windows uses DeleteFile and rejects that target. Retain this source platform
contract rather than invent a shared policy. Current UI directory routing remains
in its existing directory operation owner.

New filesystem_cleanup validates typed/native and Qt-boundary readonly Unicode/
space files, repeated/missing deletion, nonempty directory/file rejection and
empty directory removal/platform contract. POSIX symlink cleanup removes file/
directory links while retaining target bytes and a readonly target. Actual Windows
reparse/alternate-stream/link acceptance remains mandatory P2-05/P1-10/P3-03.
No layout/text change; native single-item cleanup is a dependency, not a completed
parent-stack or complete filesystem operation workflow claim.

Validation macOS arm64, 2026-09-29: native filesystem/app/direct consumer targets
built; focused 6/6 passed (2.53 s) after direct-consumer relinking; mandatory
check.sh 78/78 (42.58 s), formatting and git diff --check passed. Windows bodies
are copied with their original branches but Windows compilation/runtime and Linux
runtime remain unverified locally. Source/vendor counts are unchanged.

## P0-02.2b3b3c3c3 — combined bounded acceptance and dependency ordering (2026-09-29)

Project baseline `1189a8299fc65327bc1fbaef73063ac66df1b042`; upstream remains
ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at
https://github.com/ip7z/7zip. Documentation-only acceptance adds no source body,
runtime or platform changes. The unchanged runtime has current gate evidence:
c3c1 focused 7/7, c3c2 focused 5/5 and latest check.sh 77/77 (41.91 s), macOS
arm64 2026-09-29. Vendor comparison confirms 67/67 exact files including License;
39 source CPP/33 UI, 37 root Qt glue and 66 vendored dependencies/resources remain.

Accepted prerequisite: owned typed CAgent/CFSFolder binding, native selected
indices/properties/paths and selected consumer transitions, plus 7zz transport
quiescence before Qt temporary cleanup. Original native lifecycle/navigation is
not accepted by those fixtures. Remaining obligations are:

| Mandatory scope | Unported/remainder |
| --- | --- |
| P0-02.2b3c | Original CTempFileInfo/CFolderLink, parent ownership, source close/pop/update/cleanup order, root/nested temporary lifetime and required platform dependencies |
| P0-02.2b3d | Original BindToPath/LoadFullPath/parent/flat ownership, focused open/navigation/path controls, all context consumers and removal of duplicate Qt location/parent fields |
| P0-02.2c | Original Create/CreateOnePanel/ReleaseApp/ReleasePanel, disable all panel notifications before releasing any; remaining typed interface/release fields required by the source body |
| P0-02.3/P0-03 | Operation/application state, complete source capabilities/dispatch and start/error/cancel/close integration |
| Later operation/handler owners | Native mutation/Extract/Copy workflows, raw/alternate-stream metadata, native archive time, CRC/hash and platform/shell integration listed in earlier consumer inventory |

Broader b3b runtime parents stay active until full navigation/context integration.
The old b3c→b3b→full-navigation-b3d→b3c ordering would create a cycle. Make b3c
depend explicitly on accepted typed providers b3b2 and bounded lifecycle c3c;
keep b3d after b3c and close broader runtime parents only after their original
requirements pass. This is prerequisite refinement, not N/A or a completed claim
for omitted functions. Next source prerequisite is native temporary cleanup and
library transfer support before copying original temporary/parent structures.

Validation for this documentation-only substep is git diff --check; runtime code
is unchanged from the verified commit and its full gate is not repeated.

## P0-02.2b3b3c3c2 — transport quiescence before temporary cleanup (2026-09-29)

Project baseline `dc9f7e8`; upstream baseline remains ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Inspect original App.cpp ReleaseApp, Panel.cpp ReleasePanel/destructor and
PanelFolderChange.cpp CloseOpenFolders/CloseOneLevel: stop notification activity
before releasing panel/folder/library resources. Their complete native lifecycle
body ports remain mandatory P0-02.2c and the parent-stack scopes; this iteration
does not replace them with a claimed completed Qt implementation.

The application uses 7zz in QProcess, so upstream in-process library/COM teardown
has no directly usable process-stop implementation. Add narrow ArchiveBackend
cancelAndWait transport glue: preserve virtual cancellation, kill any remaining
process, wait for its termination before returning. Existing operation completion
and cancelled-mutation cleanup remain authoritative in ArchiveBackend; no archive
or source path algorithm is added. Its destructor reuses this quiescence stage
after disabling its internal process callbacks. MainWindow disconnects backend/
scanner callbacks and cancels scanning before stopping transport while temporary
paths and source-folder members are still alive. Existing filesystem operation
cancel/wait and scanner worker join remain in their owners.

A failing pre-fix regression observed zero backend cancellations when destroying
an active archive-hash window. The corrected test requires cancellation while the
temporary directory still exists, then idle backend and removed directory after
window destruction; late backend events cannot issue more requests. Real-process
tests use this test executable's controlled list fixture, without shell/services/
real archives: immediate-start cancellation, ready-process cancellation, completed
failure before return, idempotent idle stop and subsequent reuse. The fixture has
a bounded 30-second lifetime; killed cancellation must finish within five seconds.

No layout/text changes. Full native ReleaseApp/ReleasePanel/CloseOpenFolders,
source outer stack/interface release and cross-platform acceptance remain open.

Validation macOS arm64, 2026-09-29: affected application/backend/viewer/nested/
lifecycle targets built; focused 5/5 passed (6.00 s), mandatory check.sh 77/77
(41.91 s), formatting and git diff --check passed. The baseline destruction case
failed before the fix. Vendor bytes match pinned source 67/67 including License;
source/module inventory is unchanged. Windows/Linux runtime remains unverified.

## P0-02.2b3b3c3c1 — bound consumer replacement/failure/cancel acceptance (2026-09-29)

Project baseline `3a82e8d`; source ip7z/7zip remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
No functional source body is added or changed: acceptance targets the existing
source CPanel selection/property/path binding and owned CAgent/CFSFolder adapters.
Original PanelFolderChange ReleaseFolder/SetNewFolder and PanelItems operated
selection/path bodies establish the owner boundary; Qt loading/backend events
remain integration code until full source BindToPath/LoadFullPath/outer stack.

New binding_lifecycle public-window tests cover normal/flat/four views and both
failed/cancelled listing. Loading clears rows and disables selected commands.
Cancelled archive hash extraction removes its temporary directory and retains
the correct owner/selection for another extraction. A replacement archive supplies
its own metadata and paths despite forged presentation roles. Failed/cancelled
subsequent listing returns to the filesystem owner; hash requests its original
Unicode/space path without issuing stale archive extraction. Fixtures use local
temporary files and controlled backend signals, without services or user archives.

This validates provider/selected-consumer transitions, not complete native
navigation, command dispatch, full interface catalog or shutdown. Teardown and
operation quiescence follow c3c2; outer-stack/source path integration remains
mandatory P0-02.2b3c/d and final parent acceptance stays open.

Validation macOS arm64, 2026-09-29: affected lifecycle target built; focused
7/7 passed (8.11 s), mandatory check.sh 77/77 (32.44 s), formatting and
git diff --check passed. No visible layout/text change or source/vendor inventory
change; Windows/Linux runtime remains unverified.

## P0-02.2b3b3c3b3e — selected-consumer acceptance and remaining boundaries (2026-09-29)

Project baseline `6ac6746`; exact original source remains ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Compare PanelItems.cpp Get_ItemIndices_Operated/SaveSelectedState/RefreshListCtrl
and Panel.cpp AddToArchive. No new functional algorithm is introduced. Remove
QtPanelList's itemRows_ hash and PrepareItemIndices: after source folder paths
replaced the row fallback, this map had no readers. Remove its two synchronization
calls only; original selection, name/focus and path functional bodies remain.

Public-window regressions verify that an unselected focused item is not a Create
source, while selecting it supplies its original path despite forged row path/type
in normal/flat/four views. A subsequent empty-folder
owner supplies no stale paths. Create Cancel never requests backend work. The
existing Qt empty-folder dialog remains; complete source AddToArchive validation,
errors and CompressFiles are mandatory P1-05 rather than completed here.

Remaining consumer inventory is explicit:

| Consumer | Current authority/debt | Mandatory destination |
| --- | --- | --- |
| Copy/Move summary, selected Properties | Source operated indices and typed owner metadata | Complete catalog/raw/native-time/command scopes P0-03/P0-04/P1-10/P2-09 |
| Archive selected extraction/mutation/hash extraction | Source real indices and typed full Path; 7zz flat expansion/dedup adaptation retained | Native Extract policy and operation integration P0-12/P1-04 |
| Archive Rename/View/Edit and hash-result targets | Source names/full prefixes; Qt workflow glue | Native open/edit/hash P0-04/P2-03, mutation P1-04 |
| Filesystem selected operation/View/Edit/Create/volume paths | Source full paths, native prefix restoration | Complete native operation workflows P1-01/P1-03/P1-05/P2-01/P2-02 |
| openItem/openInside/openOutside and their availability | Current Qt row path/type, focused-row routing | Source BindToPath/LoadFullPath/parent/open integration P0-02.2b3c/d and P0-03/P0-04 |
| selectedPaths/selectPaths, post-transfer selection | Remaining Qt row path restoration | Source refresh/selection integration P0-02.2b3d/P0-07/P1-01 |
| Drag/drop paths, native icon lookup | Remaining presentation path/type roles | Source drag and shell/icon boundaries P1-02/P0-09/P3-05 |
| directory_/archiveFolder_/archiveParents_ and temporary display prefix | Independent Qt context/stack pending replacement | P0-02.2b3c/d, not accepted by selected-consumer tests |

This closes only the planned selected-consumer helper/binding scope. Full native
location consumers, command dispatch, outer stack and platform workflows remain
mandatory; familiar helper paths or successful Qt tests do not complete modules.

Validation macOS arm64, 2026-09-29: affected app/selection/runtime targets built;
focused 7/7 passed (7.83 s); mandatory check.sh passed 76/76. Formatting and
git diff --check passed. Initial focused expectation incorrectly included an
unselected focus; comparison with the unchanged original guards corrected the
test and descriptions before the gate. No layout/text changes. Windows/Linux
runtime remains unverified; source/vendor inventory is unchanged.

## P0-02.2b3b3c3b3d — filesystem selected-path consumers (2026-09-29)

Project baseline `b4bfbec`; ip7z/7zip source baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at https://github.com/ip7z/7zip.
Inspect original PanelOperations.cpp DeleteItems operated-index/full-path calls,
PanelItems.cpp GetItemRelPath/GetItemRelPath2/GetItemFullPath and
PanelFolderChange.cpp GetFolderPath and Panel.cpp AddToArchive's operated-index
selection. The already copied functional bodies remain
authoritative and unchanged; this iteration binds their filesystem consumers.

selectedFilesystemPaths moves from row text enumeration into the shared Qt
selection/path boundary. It gets source operated indices, restores the native
normalized folder prefix through source GetFolderPath (Properties may have set a
display prefix without a separator), and converts GetItemFullPath via us2fs and
QtFilesystemPath. Qt decoding/native separator conversion remains at the OS
boundary. No new path-composition, enumeration or normalization algorithm is
introduced. Existing Copy/Move/Delete/Rename/hash/split/combine consumers share
this method; View/Edit and Create dialog selection now use it as well.

Complete LoadFullPath/BindToPath/outer-parent prefix ownership remains mandatory;
the temporary prefix restoration is a narrow integration adaptation. Native
operation control flows, focused open/navigation, selection restoration and
drag/drop row consumers are not closed by this selected-path binding. Full
filesystem operation owners and command availability remain their later steps.
Dialogs/layout/text are unchanged. New filesystem_selection_runtime checks
normal/flat/four views, sorting, Unicode/spaces, duplicate basenames, a preceding
Properties call and forged path/type roles through hash and Create input/Cancel.
Existing actual filesystem Copy/Move/Delete/Rename and UTF-8 Edit regressions
also forge row paths, checking original files and operation outcomes.
Create input follows source operated selection. An unselected focused item is
excluded by the original GetSelectedCount/IsItemSelected guards; complete
AddToArchive/CompressFiles remains P1-05. The earlier focused-fallback description
was incorrect and is superseded by the original-body comparison in substep e.

Validation macOS arm64, 2026-09-29: affected application/consumer targets built;
focused 3/3 passed (2.67 s), expanded direct consumers 8/8 (7.66 s), mandatory
check.sh 76/76 (33.88 s), formatting and git diff --check passed. No visual
layout/text change requires geometry inspection. Windows/Linux runtime remains
unverified. Source counts remain 39 mirrored/33 UI; root Qt glue is 37 files;
66 exact vendored dependencies/resources plus original License remain unchanged.

## P0-02.2b3b3c3b3c — archive single-item and hash-result consumers (2026-09-29)

Project baseline `fc97d31`; original source remains ip7z/7zip commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
https://github.com/ip7z/7zip. This integration consumes the already copied
Agent.cpp GetName/GetFullPrefix/GetRealIndices and PanelItems.cpp
Get_ItemIndices_Operated/IsItem_Folder bodies. IsItem_Folder becomes public for
production command consumers; its original implementation is unchanged.

New mainwindow_selectionpaths.cpp owns the narrow Qt synchronization and path
conversion boundary, shared by all window targets. It synchronizes the native
selection until full source OnNotify integration, then uses source operated
indices. Logical archive targets compose source GetFullPrefix + GetName; expanded
extraction paths retain the existing source real-index/property transport mapping.
Rename, View/Edit and hash-result targets no longer read row path/directory roles.
View/Edit size limits read typed kpidSize from the owner. Single-file command
availability uses source IsItem_Folder, including return from failure/cancel.
Existing Qt dialogs, viewer limit, backend operation flow and error handling remain
integration code; this is not a port of complete native open/edit/hash workflows.

Regressions corrupt presentation roles while preserving source indices: Unicode
rename retains its original hint/path and Cancel makes no request; successful
hash uses original logical selected targets after extraction in normal/flat and
four views; archive View/Edit preserve paths, selection/focus, saved bytes and
temporary cleanup across success/failure/cancel. Filesystem selected paths follow
substep d; open/navigation still use presentation paths until native binding and
parent stack integration. Full command routing is P0-03, open/edit P0-04, source
Extract P0-12 and complete CRC/hash P2-03. No mandatory branches are waived.

Validation macOS arm64, 2026-09-29: affected app/consumer targets built; focused
9/9 passed (7.06 s), mandatory check.sh passed 75/75 (37.08 s), formatting and
git diff --check passed. Initial View/Edit failure/cancel regression exposed the
remaining presentation-based eligibility predicate and was fixed before the gate.
No layout/text change requires appearance inspection. Windows/Linux runtime is
unverified. Source/vendor counts are unchanged; one Qt integration file is added.

## P0-02.2b3b3c3b3b — native source panel path helpers (2026-09-29)

Project baseline `5bc9b8d`; pinned ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Restore original PanelItems.cpp
GetItemRelPath UString signature/body; copy GetItemRelPath2/GetItemFullPath.
Copy Panel.cpp GetFolderTypeID/IsFolderTypeEqTo/IsRootFolder/IsFSFolder/
IsFSDrivesFolder/IsAltStreamsFolder/IsArcFolder/IsHashFolder/GetFsPath bodies and
Panel.h IsDeviceDrivesPrefix/IsSuperDrivesPrefix. Original type comparison,
variant handling, two-character Windows drive-name branch, source pure-drive
prefix omission and device/super prefix retention remain intact. Existing
adapted Windows/FileName.cpp IsDrivePath2 supplies the dependency; no new path
parsing/normalization algorithm is introduced.

Restore _currentFolderPrefix to original UString storage. Qt setter/constructor
inputs convert once through ToUString; summary and Properties presentation use
existing conversions only at the boundary. Native folder constructor retains
GetFolderPath directly. Remove QtPanelList::GetItemRelPathForItem and the
independent ownerless row fallback; the legacy selection/focus fixture now binds
its existing source-interface snapshot provider. Empty unbound panels still
support zero-item state saving, without calling path helpers on absent owners.

Adaptations are namespace-qualified native calls, Qt boundary conversions and
project formatting. Caller-supplied temporary prefix still replaces complete
LoadFullPath/outer-stack initialization; callers must supply the source's
normalized full folder prefix before GetItemFullPath, rather than rely on a
display prefix previously set by Properties. Parent/native stack and full
FSDrives/alternate-stream folder providers remain mandatory b3c/d/P0-04/P1-08/
P1-10. Classifier/helper bodies do not claim those folder modules or centralized
command availability complete. Remaining path consumers follow c/d/e.

New panel_path uses native owned CFSFolder in normal/flat modes with Unicode/
spaces/duplicate basenames and validates full paths after external owner release.
A public typed folder fixture checks pure/device/super drive prefixes, source
folder/arc/hash classification and failed/type-invalid metadata contracts. Native
Windows separator expectations are conditional and remain unverified locally.
No displayed UI changes; source-to-Qt strings retain the existing contract.

Validation macOS arm64, 2026-09-29: affected application/path/state/folder/copy/
Properties targets built; focused checks passed 5/5 (3.97 s). Mandatory check.sh
passed 75/75 and git diff --check passed. Initial test fixture build used a
private CPropVariant char-pointer constructor; it now uses the original supported
assignment API without changing source types. Windows drive separator branches
and Windows/Linux runtime remain unverified locally; native/source/vendor counts
unchanged. Remaining path consumers and full stack/command scopes stay open.

## P0-02.2b3b3c3b3a — source archive item names/full prefixes (2026-09-29)

Project baseline `96c5cab`; pinned ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Copy original Agent.cpp
SET_realIndex_AND_dir macro and GetName/GetFullPrefix _proxy functional bodies
at their original module. Names/signatures/UString return types, flat item versus
current directory selection, directory/file branch and full-prefix index
selection remain original. Source GetDirPath_as_Prefix owns path construction.

Adaptations: current proxy directory arrays use size() and current Name/prefix
storage is QString, converted through existing ToUString at that boundary.
These are existing documented proxy migration debt, not a reason to invent a
second name/path algorithm. The macro invocation adds a semicolon solely for
project clang-format statement layout; it does not change control flow.
Original _proxy2/alternate-stream branches remain
mandatory P1-10. No complete Agent module or engine operation claim. Single-item
window/path consumers are subsequent steps; their current row adapters remain
explicit migration work.

agent_folder now verifies both normal and flat source names/full root-relative
prefixes, a bound nonroot base, Unicode/spaces, duplicate file basenames in
different subfolders and explicit empty directory. Root/parent references are
released before enumeration; the retained source owner remains authoritative.
No displayed UI changes, so appearance inspection is not required.

Validation macOS arm64, 2026-09-29: affected application/folder/selection targets
built; focused consumers passed 3/3 (1.09 s). Mandatory check.sh passed 74/74
(33.94 s); git diff --check passed. Windows/Linux runtime remains unverified
locally; no vendor/module-count change or upstream edits. Relative/full native
panel path helpers and consumer bindings remain b–e.

## P0-02.2b3b3c3b2 — source-owned archive selection paths (2026-09-29)

Project baseline `9829490`; same pinned ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Existing original PanelItems
Get_ItemIndices_Operated and Agent.cpp GetRealIndices own selection and archive
mapping. MainWindow's integration adapter synchronizes Qt selection through
SaveSelectedState, gets source operated indices and delegates mapping to the
bound owned CAgentFolder. It queries original full Path values from the immutable
typed archive provider, matching original engine GetArchive()->GetProperty
(arcIndex, kpidPath) at the process boundary. Remove Qt path/directory/proxy-index
roles, independent proxy expansion and entries_ path reconstruction from this
consumer. Narrow read-only GetArchiveProperties accessor exposes the existing
retained transport interface; it contains no functional algorithm.

Local integrations: synchronization remains temporary until full OnNotify;
QStringList converts BSTR paths to the ArchiveBackend request boundary and
deduplicates overlapping source results. Failed/type-invalid Path query returns
an empty request atomically, with no partial operation/fallback to row metadata.
The 7zz consumer retains its existing selected-folder expansion policy, passing
includeFolderSubItemsInFlatMode=true. The original engine Extract passes false;
this explicit transport policy preserves current Copy/Move/hash directory
workflows pending mandatory source Extract integration P0-12, and is not claimed
full Extract parity. ArchiveBackend still owns arguments, validation and process
operations. No codec/format implementation or command syntax enters widgets.

Dedicated archive_selection_runtime covers normal/flat/four views, corrupted
Qt path/directory/proxy roles with intact source item parameters, directory+file
overlap, source archive ordering and duplicate basenames. Public selected
extraction, Copy, Move and hash extraction use source paths; Cancel/failed Copy
returns to idle, cancelled Move never removes entries. This is selected-path
integration acceptance, not full operation success/callback/format acceptance.

Remaining consumer inventory: filesystem selectedFilesystemPaths, single-item
rename/view/edit/open paths, and hashSelection_ result targets still use rows;
they remain explicit b3 work, inspected/split before changes. Current location
fields, full BindToPath/LoadFullPath/outer stack stay mandatory b3c/d and P0-04.
No new UI presentation; appearance inspection is unnecessary for this iteration.

Validation macOS arm64, 2026-09-29: affected application/selection/mapping and
direct consumer targets built. Initial focused selection/mapping checks passed
2/2 (0.92 s); expanded operation/loading/Properties/hash consumers passed 7/7
(6.26 s). Mandatory check.sh passed 74/74 (32.76 s) and git diff --check passed.
Windows/Linux runtime remains unverified locally; source/vendor counts unchanged
and upstream remains read-only. Remaining b3 consumers and c3 parent stay open.

## P0-02.2b3b3c3b1 — source operated-to-archive index mapping (2026-09-29)

Project baseline `7c52e61`; exact [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copy Agent.cpp CAgentFolder::GetRealIndex/GetRealIndices original _proxy
branches and AgentProxy.cpp CProxyArc::GetRealIndices_Unsorted body. Existing
AddRealIndices restores original CUIntVector result and Add calls; one recursive
implementation remains. Copy C/Sort.c and C/Sort.h byte-for-byte into unchanged
vendor paths with their public-domain notices. Existing Precomp/CpuArch/7zTypes
dependencies remain exact vendor files. CMake enables C for the original Sort.c
and compiles it in sevenzip_common; no std/Qt sorting replacement is introduced.

Source normal selection delegates unsorted directory/file expansion; flat mode
retains flag-dependent recursion, implicit-directory omission and explicit
directory archive-index handling. Original final HeapSort preserves overlapping
duplicates. Source names, signatures/types, loops, branches and module owners
remain. Necessary adaptations: size() on the existing adapted proxy's std
directory arrays, include path to unchanged vendor C dependency, and project
formatting. Existing tree containers/string adapters remain documented migration
debt; this focused step restores the result vector without a whole proxy rewrite.
Current window caller changes only vector/result iteration type, preserving
its existing row-derived path policy until b2; it is not claimed source-migrated.

Mandatory omitted branches: CProxyArc2/_proxy2 and alternate-stream expansion
P1-10/full handler metadata. The retained includeAltStreams signature has no
effect in the original _proxy branch. Original engine Extract passes false for
includeFolderSubItemsInFlatMode; current 7zz UI expands selected directories.
Caller policy must be recorded during b2/operation migration, not silently
claimed complete. Full Extract operation and source path/outer stack remain open.

New agent_real_indices checks normal/flat explicit/implicit folders, source
real-index values, both expansion policies, replaced output, sorted archive order,
overlap/repeated selections with preserved duplicates, empty selection and 2048
reverse indices. Existing direct proxy tests retain unsorted recursive traversal
contracts using the original vector type. No UI presentation changes.

Validation macOS arm64, 2026-09-29: affected application/mapping/proxy/folder
targets built; focused tests passed 5/5 (0.63 s). Mandatory check.sh built all
consumers and passed 73/73 (39.80 s); git diff --check passed. Exact upstream
vendor comparison passed 67/67 including License.txt (66 source/header/resource
files); local vendor README is project documentation. Initial new test build
needed the source vector's explicit ConstData() rather than an implicit pointer
conversion; production/source types were preserved. Windows/Linux runtime remains
unverified locally. Window consumer migration follows in b2.

## P0-02.2b3b3c3a — owned Agent folder lifecycle only (2026-09-29)

Project baseline `2511ee8`; pinned ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Inspected Agent.h original Init and
Agent.cpp BindToFolder_Internal/BindToParentFolder/GetNumberOfProperties/
GetPropertyInfo. Source folder Init retains the agent; child/parent construction
always passes that owner. Remove the independently added external-proxy
constructor and metadata-only Init overload, which were used only by legacy
archive_proxy/agent_folder fixtures. Child/parent binding now unconditionally
uses the existing original owner branch. Catalog queries have one original
owned expansion/remapping path; default unbound public calls fail safely with
E_FAIL and count zero rather than dereferencing an absent agent. No duplicate
source algorithm or new proxy copy is introduced.

The retained partial source Init still omits proxy2/alternate-stream state and
uses IUnknown instead of engine IInFolderArchive at the 7zz boundary; archive
type/provider references remain narrow transport adaptations. Full engine/raw/
alternate-stream operation interfaces remain mandatory P0-03/P0-04/P1-10.
This removes borrowed compatibility debt only, not the full engine/outer-stack
scope. Other source bodies and authoritative module paths remain unchanged.

Fixtures now use the same owned source factory as the runtime. They retain
independent CProxyArc instances only when testing that module directly, never
as a folder backing store. Public checks retain child→ancestor traversal after
root/input/provider/explicit agent release; the former borrowed fixture now
also exposes the source five-property catalog after root release. Flat subtree
loads and 4500-directory counts use owned folder interfaces. Unbound catalogue
queries return failure without publishing undefined counts. No UI appearance
change, so appearance inspection is unnecessary for this iteration.

Validation macOS arm64, 2026-09-29: affected application/folder/catalog/runtime
targets built; focused consumers passed 10/10 (6.96 s). Mandatory check.sh
passed 72/72 and git diff --check passed. Windows/Linux runtime remains
unverified locally; upstream checkout remains unchanged. Further consumer
selection mapping is c3b; full path/outer-stack migration remains open.

## P0-02.2b3b3c2cc — live Properties runtime acceptance (2026-09-29)

Project baseline `bed7edf`; same pinned upstream
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from ip7z/7zip. This acceptance
iteration changes no borrowed implementation. Dedicated properties_runtime
exercises the existing source PanelItems operated-index selection, PanelMenu
Properties enumeration/aggregate, native CFSFolder metadata/cache and owned
CAgentFolder normal/flat catalogs through the public window action.

Eight normal/flat × four-view scenarios cover filesystem→archive→filesystem,
cleared selection on owner replacement, Unicode/space duplicate basenames and
source relative prefixes, size/primed packed cache before mutation versus after
Refresh, uncalculated filesystem directories, archive directory counts/normal
totals versus absent flat size, known/unknown/zero fields, file counts absent,
raw Modified compatibility digits, deliberately corrupted presentation cells,
and folder+file source aggregates. Cancel closes each Properties dialog without
archive extraction. Test fixtures own no real user archives or secrets.

The shared ListViewDialog.cpp/.rc reference is unchanged. Normal/compact
540×360/360×240 JSON reports for file, directory and aggregate in all eight
scenarios verify two columns, header/list/footer ordering and fully visible
OK/Cancel. This is structural Qt acceptance, not Windows pixel parity.
Full raw/archive-level/shell Properties and handler time/metadata remain
mandatory P0-03/P0-04/P1-10/P2-09. OnNotify/full path/outer-stack ownership and
remaining consumers stay open under P0-02; no whole module completion claimed.

Validation macOS arm64, 2026-09-29: affected target built; focused Properties,
catalog, native loading and navigation consumers passed 9/9 (5.44 s). All 48
current-build widget reports reviewed; list/header/footer/buttons visible with
unclipped exposed bounds and stable ordering. Mandatory check.sh passed 72/72;
git diff --check passed. Windows/Linux runtime and rendered Windows reference
remain unverified locally. Scoped selected-item consumer c2 is complete;
remaining navigation/mutation/teardown consumers are c3.

## P0-02.2b3b3c2cb — live source Properties binding (2026-09-29)

Project baseline `93e832b`; pinned [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
CPP/7zip/UI/FileManager/PanelMenu.cpp source Properties aggregate now always
calls AddPropertyString(kpidPackSize, packSize, message), restoring the original
branch after native filesystem packed metadata became available. The local
archive bool/visibility filter is removed from CPanel::Properties and callers.
Existing copied typed enumeration/aggregate bodies remain authoritative.
PanelItems.cpp Get_ItemIndices_Operated remains the original source algorithm;
MainWindow synchronizes selection through existing SaveSelectedState until the
full OnNotify port, then supplies source indices and its bound focused CPanel.

Local Qt presentation glue PropertiesDialog now only invokes that CPanel and
initializes the existing ListViewDialog. PropertyItem and its snapshot constructor
are removed. MainWindow no longer reconstructs metadata from table text,
QFileInfo, display roles or a second proxy calculation. It still supplies the
visible full location to the existing temporary folder-prefix adapter until
mandatory LoadFullPath/outer-stack migration; no native path algorithm is added.

Preserving current archive timestamp UX requires carrying ArchiveEntry.modified
into the existing immutable metadata provider. Inspected source Console/List.cpp
CFieldPrinter::PrintItem emits kpidMTime from FILETIME via the listing formatter;
it does not provide a reverse 7zz-text reader usable here. No independent date
parser is added. MTimeText compatibility field advertises sparse kpidMTime as
VT_BSTR only when supplied, returns VT_EMPTY per missing item and retains raw
text/fractional digits. The original PanelMenu variant formatter handles it.
This truthfully describes unparsed transport text, not native FILETIME or a
completed time/precision/timezone source port. Canonical native archive time
parsing and complete handler metadata remain mandatory P0-03/P0-04/P1-10.
Filesystem timestamps already come from the copied native folder properties.

Regression uses real source folder ownership/indices rather than invented
PropertyItem totals: valid selected folder descendants, file packed size and
empty counts, known/unknown sizes, sparse raw timestamp. Window archive cells
are deliberately changed to prove they cannot alter Properties values; loaded
filesystem Size remains 7 after external file replacement with 23, with original
lazy packed cache primed before mutation. Unicode normalization is only in the
test comparison of native macOS decomposed names, not a rewritten source value.
Source filesystem packed size is now visible; unknown fields remain omitted.

Mandatory omissions: full raw/archive-level and native shell Properties paths
P0-03/P0-04/P1-10/P2-09; unparsed archive timestamps as above; full OnNotify,
LoadFullPath and outer/native ownership migration under P0-02. Synthesized
Type/item-Path rows from the old snapshot are removed in favor of source catalogs
and existing current-location footer. No whole PanelMenu/Properties port or
cross-platform visual parity is claimed. Four-view/flat runtime acceptance is cc.

Validation macOS arm64, 2026-09-29: affected application and tests built; initial
focused checks passed 8/8, expanded Properties/context-menu direct consumers
passed 6/6 (2.83 s). First full gate found one obsolete synthesized item-path
expectation in ui_archive_states; it now checks the source name and current
directory footer. Repeated mandatory check.sh passed 71/71; git diff --check
passed. Fresh 540×360 and 360×240 Properties widget JSON reviewed: two-column
header/list order, separate footer, visible OK/Cancel, no clipped controls.
No Windows rendering reference or Windows/Linux runtime verification available.

## P0-02.2b3b3c2ca2 — source owned archive property catalog (2026-09-29)

Project baseline `3132579`; pinned [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
CPP/7zip/UI/Agent/Agent.cpp kProps, CAgentFolder GetNumberOfProperties/
GetPropertyInfo and ReadItems ThereIsPathProp block are copied as the functional
base. Owned runtime folders retain original property count expansion, hiding
Prefix outside flat mode, optional Name insertion, Path-to-Name remapping,
delegated descriptor query and canonical extra-property types. The source
ReadItems loop resets ThereIsPathProp and scans original property IDs/descriptors
before proxy loading; the named source field is retained in CAgent.h.

Adaptations: existing GetArchive() returns the immutable typed 7zz provider
instead of an in-process archive engine; ReadItems queries that provider before
allocating/publishing the proxy. Source ignores GetNumberOfProperties failure
in ReadItems/GetPropertyInfo; RINOK is required at the transport boundary so an
error cannot leave an undefined count or publish a partial owner. GetPropertyInfo
validates the visible extra-property index before indexing kProps, including
hidden Prefix in normal mode. These checks add safe public adapter failure
behavior without replacing the source catalog algorithm. Source formatting and
portable includes are the other changes.

Borrowed external-proxy compatibility initialization still has no CAgent owner
and retains its previous direct catalog delegation. It does not run a duplicate
property scan or claim to implement owned source semantics. This legacy branch
and full borrowed/outer lifecycle migration remain mandatory P0-02 c3; all
runtime factories and their children/parents use the owned source branch.
Commented optional alt-stream/proxy2 catalog paths and complete handler metadata
remain mandatory P0-03/P0-04/P1-10; no whole Agent module completion is claimed.

ArchiveTransport has no engine file descendant counts, so its existing
GetProperty count slots now return VT_EMPTY rather than a fabricated file zero;
source CAgentFolder supplies its computed directory counts unchanged. Legacy
custom presentation count values remain as supplied. Regressions check normal/
flat descriptors/order/types, Path remap/optional Name, canonical directory
totals and flat size absence, unknown file counts, invalid indices, owned
catalog after external owner release, count/descriptor failure before root
publication, successful retry, and later query failures. No current window
dialog changes until live Properties binding; appearance inspection is not
required for this nonvisual catalog step.

Validation macOS arm64, 2026-09-29: affected application/Agent/folder/Properties
targets build; focused direct consumers 7/7 (2.96 s); scripts/check.sh formatting,
full build and CTest 71/71 (35.80 s). Source/vendor counts remain unchanged;
the sibling source checkout remains unmodified. Windows/Linux runtime is
unverified locally, and this catalog check does not establish rendered parity.

## P0-02.2b3b3c2ca1 — typed 7zz archive transport catalog (2026-09-29)

Project baseline `a7338e0`; inspected pinned [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
CPP/7zip/UI/Agent/Agent.cpp GetNumberOfProperties/GetPropertyInfo and ReadItems
ThereIsPathProp scan; CPP/7zip/UI/FileManager/IFolder.h property contract and
k7z_PROPID_To_VARTYPE from CPP/7zip/Common/PropId.cpp. Source CAgentFolder derives
its visible catalog from the archive engine, remaps Path to Name and appends
counts/flat Prefix. Its current partial adapter only delegates to the metadata
provider; the factory previously supplied an empty custom catalog.

This prerequisite adds an explicit ArchiveTransport catalog to the existing
QtPanelFolder and selects it in CreateArchiveFolder. Its fixed Path/Size/PackSize
descriptors describe the immutable ArchiveEntry fields already transported by
7zz, use the unchanged source property IDs/type table and no localized BSTR
caption. GetProperty Path returns the full transported relative path; size/pack
retain existing presence flags and typed UInt64 values. Invalid indices return
the existing public E_INVALIDARG convention. Legacy custom presentation catalog
callers retain their existing default mode and union of custom property names.

No source implementation provides a 7zz transport-to-IFolder catalog: the
inspected source queries an in-process IInArchive. This is new narrow integration
glue at that boundary, not a replacement for the original CAgentFolder catalog
algorithm. The latter and original ThereIsPathProp scan are mandatory ca2 next;
live Properties cb/cc still follows. Full handler metadata (time/attributes/CRC/
raw/archive levels/stream properties and absent/U64 parser semantics) remains
P0-03/P0-04/P1-10; no absent fields are advertised as implemented or N/A.

Public adapter/factory regressions check full Unicode/space paths, canonical
catalog types/no captions, immutable input ownership, maximum UInt64/known zero,
absent values despite known descriptors, empty archives, bad indices and legacy
custom catalog preservation. No displayed text/layout changes in this transport
prerequisite; runtime catalog expansion and Properties integration remain open.

Validation macOS arm64, 2026-09-29: affected application/folder/runtime/property
targets build; focused direct consumers 7/7 (3.09 s); scripts/check.sh formatting,
full build and CTest 71/71 (34.91 s). No new production source files/vendor bytes;
counts remain unchanged. Windows/Linux runtime remains unverified locally.

## P0-02.2b3b3c2b3 — original PanelMenu typed properties (2026-09-29)

Project baseline `e749ee6`; pinned [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
CPP/7zip/UI/FileManager/PanelMenu.cpp AddPropertyString both overloads are copied
completely: VT_EMPTY skip, original error/warning flags conversion/zero skip,
UI8/UI4/UI2 plus IsSizeProp grouped-size branch, native ConvertPropertyToString2
with source precision 9, empty formatted suppression, original ErrorType warning
pair and GetNameOfProperty lookup. UInt64 overload constructs the original
CPropVariant and delegates to the same helper, removing independently written
caption/number switches. Original UString, PROPID, variant types and control
flow are retained; fully qualifying NCOM and ToUString at the existing QString
size formatter return are narrow adaptations, plus includes/project formatting.

Single-item enumeration now calls that original helper after the original
GetPropertyInfo/GetProperty success checks, replacing the independent BSTR-only
restriction. Existing source aggregate loop now shares typed caption/formatting;
counts group digits as source IsSizeProp requires. sevenzip_panel links the
existing sevenzip_filesystem formatter owner; no module implementation is copied
again. A public test-only typed folder contract fixture drives complete catalogs,
unknown/custom names, typed sizes/counts/date/CRC/bool, empty/zero and failed
descriptor/read paths, combined flags/invalid types and expected-type warning.

Mandatory partial scope: window still creates legacy Qt Properties snapshots.
Its archive-only packed aggregate visibility adaptation remains until c2c live
binding supplies native filesystem packed metadata; no fabricated filesystem
packed value is added. Source always-visible packed aggregate branch must be
restored there. Folder/archive-level properties, _folderRawProps and archive
levels remain P0-03/P0-04/P1-10; source native filesystem shell Properties path
remains P2-09. Full CPanel::Properties/PanelMenu is not claimed complete by these
helper bodies. Original warning English literals remain language work P1-14.

Visible count-grouping and typed formatting checks use the existing two-column
Properties text specification and source ListViewDialog.rc order, at normal
540×360 and compact 360×240. Current-build typed reports check unclipped
list/header/footer, separated buttons, full value retention, displayed source
truncation and LF normalization. No screenshots or rendered Windows reference
are available/required for this structural check.

Validation macOS arm64, 2026-09-29: affected application/Properties/panel targets
build; final focused 7/7 (0.91 s); scripts/check.sh formatting/full build/CTest
70/70 (35.34 s). Initial geometry fixture needed the concrete QAbstractButton
include, corrected before passing checks. Reviewed current-build JSON reports
typed-properties-540/-360: list [11,11,518,307]/[11,11,338,187], visible header,
footer [11,324,518,25]/[11,204,338,25], separated visible OK/Cancel controls.
Counts remain 39 source cpp, 33 mirrored UI paths, 64 vendored files plus license.
Windows/Linux runtime and rendered Windows parity remain unverified locally.

## P0-02.2b3b3c2b2c — source archive error property helpers (2026-09-29)

Project baseline `607eec5`; pinned [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
CPP/7zip/UI/Common/OpenArchive.cpp GetOpenArcErrorFlags is copied completely:
optional defined flag reset, original VT_UI8/UI4/EMPTY branches, UInt32 cast,
invalid-type throw 151199. Source declaration/default argument is retained in
the scoped OpenArchive.h; unrelated engine interfaces are not replaced.
CPP/7zip/UI/FileManager/ExtractCallback.cpp copies the complete original
k_ErrorFlagsIds array and GetOpenArcErrorMessage: ordered eleven bits, skipping
missing resource strings, encrypted-header password hint, newlines, clearing
only resolved flags, original hex fallback for remaining bits. UString, source
constants, helper names and control flow are unchanged apart from formatting
and portable include paths. ExtractCallback.h exposes the original helper
prototype from the cpp as a narrow consumer interface, not callback classes.

Byte-exact CPP/7zip/UI/GUI/ExtractRes.h and Extract.rc provide all source resource
IDs and English texts. The existing resource generator now takes both resource
tables; its multi-input/late-error regression preserves valid output. Russian
message overrides are presentation adaptation, with original English fallback
retained in the generated bundle. Existing PropID.h/7zTypes supply flags/array
size; no archive engine dependency is invented for these standalone helpers.
sevenzip_panel compiles this subset for property presentation only.

Mandatory omissions: OpenArchive engine opening, CArc/CArcErrorInfo and open
callbacks P0-03/P0-04; full ExtractCallback classes, ErrorInfo_Print/HRESULT
messages, extraction/test/update workflows and progress/cancel P0-05/P0-12 and
their operation owners; full language loading/selection P1-14; PanelMenu and live
Properties binding following substeps. These partial filenames do not establish
completed modules. No visible text/layout changes until consumer integration.

Validation macOS arm64, 2026-09-29: affected application/property targets build;
focused 5/5 (0.48 s), extended multi-resource generator 1/1 (0.04 s);
scripts/check.sh formatting/full build/CTest 69/69 (33.49 s). Initial signed
test-fixture constructor is private upstream; fixture uses public Set_Int64
before the passing checks. All 65 vendored files including license match source
bytes. Counts: 33 mirrored UI paths/76 missing, 39 src/CPP cpp, 36 root Qt cpp,
64 vendored dependencies/resources plus license. Windows/Linux runtime remains
unverified locally; no rendered UI acceptance is claimed for these helpers.

## P0-02.2b3b3c2b2b — original property name/resource fallback (2026-09-29)

Project baseline `d2360d3`; pinned [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
CPP/7zip/UI/FileManager/PropertyName.cpp GetNameOfProperty is copied completely:
IDs below 1000 first try LangString(1000 + propID), then custom name, then the
original UInt32 decimal conversion. UString, PROPID, branch order and temporary
buffer are retained; include paths, source notice and project formatting adapt.
PropertyName.h retains its declaration with explicit portable MyWindows types.
LangUtils.h copies the original non-Z7_LANG three inline resource fallbacks;
it does not claim to port the Z7_LANG language-file branch or CLang parser.

PropertyNameRes.h and PropertyName.rc are copied byte-exact into third_party
under original paths. There is one authoritative English caption table. Local
scripts/rc_strings.cmake converts the supported single-line STRINGTABLE resource
syntax to a generated build-directory header; no reusable cross-platform RC
loader exists in the inspected source. Unsupported/incomplete syntax fails
before replacing existing valid output, and repeated generation is stable.
Generated declarations refer directly to unchanged source resource macros.

CPP/Windows/ResourceString.cpp/.h retain MyLoadString interfaces. Their Win32
LoadStringW/ANSI buffer/HINSTANCE presentation service is replaced by compiled
resource entries and fixed Russian caption overrides for the existing UI.
Windows instance overload routes to the application's single portable resource
bundle; no opaque HINSTANCE type is invented on POSIX. The initial unchanged
header failed there because HINSTANCE is Windows-only, so this narrow header
adaptation is authoritative under src; no duplicate vendored header remains.
These resource-service bodies are new platform/presentation glue, not a claim
that the original native LoadString buffer logic is compiled across platforms.

Mandatory omissions: full LangUtils.cpp/CLang loading, ReloadLang/LoadLangOneTime,
native default-language resolution, language discovery and UI selection P1-14;
PanelMenu typed/error properties and live Properties binding following substeps;
raw/archive-level property interfaces P0-03/P0-04/P1-10. None is closed or N/A.
No currently displayed text/layout changes: the dependency is not yet wired
into PanelMenu. New focused public tests cover localized/source English priority,
custom Unicode/empty names, unknown and maximum IDs, replacement/append semantics;
generator tests use Unicode/space paths, repeat generation and invalid syntax.

Validation macOS arm64, 2026-09-29: affected application/Properties/property-name
targets build; focused 5/5 (0.65 s), Unicode-path generator rerun 1/1 (0.07 s);
scripts/check.sh formatting, full build and CTest 68/68 (33.39 s); all 63 vendored
files including source license match pinned source bytes. Windows/Linux runtime
and rendered Windows parity remain unverified locally. Counts now 31 mirrored UI
paths/78 missing, 37 src/CPP cpp, 36 root Qt cpp, 62 vendored dependencies/resources
plus License.txt; path counts do not establish complete modules.

## P0-02.2b3b3c2b2a — source numeric property classification (2026-09-29)

Project baseline `d41d64c`; pinned [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
CPP/7zip/UI/FileManager/PanelListNotify.cpp IsSizeProp is copied in full, with
the original switch, all 22 cases and false fallback. Only project formatting
and throw() to noexcept change; original UINT/PROPIDs are retained. This adds
the missing dependency for PanelMenu's numeric size/count formatting, without
another classifier. Existing ConvertSizeToString remains its authoritative
formatter. Size/count/offset/stream/error-count properties are distinguished
from timestamps, checksum, inode/address identifiers and error flags.

Public classification regression is a focused test target. No visible text or
layout changes in this dependency substep. PanelMenu use, property resources,
error text and live Properties binding remain mandatory following substeps;
full PanelListNotify notification/selection/column logic is not completed.

Validation macOS arm64, 2026-09-29: affected application/test targets build;
focused classification/formatting/copy/properties/folder tests 5/5 (0.88 s);
scripts/check.sh formatting, full build and CTest 66/66 (34.43 s). Initial gate
rejected include ordering; clang-format corrected it before the passing gate.
No Windows/Linux runtime or rendered UI claim is made.

## P0-02.2b3b3c2b1 — source filesystem packed-size property (2026-09-29)

Project baseline `47f562b`; unchanged pinned source [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
CPP/7zip/UI/FileManager/FSFolder.cpp MyGetCompressedFileSizeW and GetProperty
kpidPackSize. Copies the complete cached property branch: UNDER_CE size path,
one-time PackSize_Defined, directory/API-failure fallback to fi.Size and typed
property assignment. The previous E_NOTIMPL placeholder is removed. Existing
source LoadItems/LoadSubItems reset cached flags; property catalog already includes
the source packed-size descriptor, so there is no independent catalog algorithm.

MyGetCompressedFileSizeW retains the original Windows native call, error check,
64-bit high/low composition, main/long-path selection and fallback flow. The
existing portable FileName owner supplies source path helpers. Source old-SDK
declaration is retained behind its Windows conditions. POSIX has no equivalent
Windows compressed-allocation API in the inspected portable services; the narrow
adapter returns unavailable and the original property branch falls back to
fi.Size. It does not introduce a new st_blocks/compression algorithm. Native
allocation/link/stream metadata and full Properties capabilities remain mandatory
P1-10/P2-05/P2-06/P3-05 scopes; this fallback is explicit, not N/A or a claim of
equivalent compressed allocation on all platforms.

Regression checks public normal/flat properties for Unicode/space files: typed
packed values, uncalculated directory Size empty but source packed zero defined,
cached values after the file disappears, missing-file native lookup falling back
to loaded size, and reload removing stale records. The previous placeholder
fails these assertions. No new UI text/layout: live Properties binding and full
formatting remain the following substeps. Windows runtime cannot be inferred
from the local POSIX fallback checks.

Validation on macOS arm64, 2026-09-29: affected application/test targets build;
focused filesystem/property consumers 5/5 pass (3.10 s); scripts/check.sh passes
formatting, application build and all 65 CTest cases (31.42 s). Windows and Linux
runtime checks remain unverified locally.

## P0-02.2b3b3c2a — original typed property formatters (2026-09-29)

Project baseline `6bd328b`; exact source [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
New byte-exact third_party modules: CPP/Windows/PropVariantConv.cpp and
CPP/7zip/UI/Common/PropIDUtils.cpp/.h/StdAfx.h. Entire files are retained and
compiled, not rewritten into Qt/std formatting. Their existing byte-exact
MyWindows/PropVariant/IntToString/StringConvert/UTFConvert/CpuArch/FileIO/FileName
headers supply types and native dependencies. PropVariantConv joins sevenzip_common;
PropIDUtils joins sevenzip_filesystem, which owns its native FileLink dependency.

src/CPP/Windows/FileLink.cpp copies the entire original file and preserves its
namespaces, functions, parser bodies and control flow: GetString, CReparseAttr
Parse/IsOkNamePair/GetPath, CReparseShortInfo Parse, and POSIX NIO GetReparseData/
SetSymLink/SetSymLink_UString are compiled. No new link or raw-data algorithm is
written. Mandatory formatting and source notice are local differences. A private
include search path resolves unchanged support headers beside the adapted FileDir
header. This does not create a second FileDir/FileLink implementation.

Partial compile scope: original Windows WriteString/FillLinkData_WslLink/
FillLinkData_WinLink/Convert_WinPath_to_WslLinuxPath and NIO device/link
read/write branches are retained in the same source file but also require
SEVENZIP_FILE_LINK_NATIVE_OPERATIONS. It is not enabled: their native
CreateComplexDir/platform path and creation dependencies are not yet ported in
FileDir. FileLink's own CreatePrefixDirOfFile helper is retained inside that
guard. Those Windows link creation/deletion/native I/O workflows remain mandatory
P2-05 (links), P1-11 (native browse/read), P2-07 (security metadata) and P3-05
(omitted dependency closure), not done or N/A; portable parser compilation
does not validate them. This guard keeps the formatter dependency narrow and
prevents unfinished native operations from being advertised. Win32 parser and
CheckNtSecure source branches retain their original platform conditions.

The new focused formatter suite covers typed numeric/bool/Unicode values,
special CRC/POSIX/Windows attributes/inode/address conversions, deterministic UTC
precision/zero time, valid Unicode reparse targets, all truncated prefixes, minor
reserved-field warnings, invalid offsets falling back to opaque data and unknown
tags, plus invalid/unsupported security data. It uses synthetic buffers, not user
archives or live Windows link/ACL objects.
The POSIX native link fixture also checks Unicode target bytes, existing-link
failure without altering the source file, and failed-read clearing of stale data.
Source SetSymLink_UString preserves the supplied Unicode target as UTF-8;
QFile::encodeName applies macOS normalization and is not its byte-level oracle.
No current UI text/layout changes. Source PropertyName/PanelMenu formatting and
live Properties binding remain the
next mandatory c2b/c2c scopes; full raw/catalog/engine coverage remains P0-03/04.

Comparison with upstream using the repository's clang-format style shows only
the added source notice and two Windows operation compile guards; parser/function
bodies are unchanged. All 61 vendor files (60 source/header files plus license)
match the pinned baseline byte-for-byte. Counts now: 35 src/CPP cpp (30 UI plus
5 support), root Qt cpp unchanged at 36; original UI src-path inventory remains
30 mirrored/79 absent, with portable PropIDUtils compiled from the vendor owner.
Focused native/variant/panel/Properties/runtime consumers passed 10/10 (2.58s),
macOS 2026-09-29. ASan property_formatting passed 1/1 (0.27s; leak detection
disabled).
./scripts/check.sh passed formatting, app build and all 65 CTest (36.60s).
Windows/Linux runtime and Windows native reparse/ACL acceptance remain unverified;
portable binary fixtures do not establish platform workflow or rendered parity.

## P0-02.2b3b3c1 — bound source Copy/Move summary (2026-09-29)

Project baseline `fa109fd`; unchanged pinned source [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
CPP/7zip/UI/FileManager/PanelItems.cpp Get_ItemIndices_Operated, App.cpp
GetItemsInfoString/AddPropValueToSum and Agent.cpp GetProperty size branches.
Copies the complete operated-index body and declaration, preserving selected
status lookup, nonempty early return, selected-count/focus checks and parent
sentinel exclusion. Only _listView is adapted to the existing QtPanelList;
GetSelectedCount maps native row selection to QItemSelectionModel.

The window no longer builds CPanelItem metadata/indices or a second QtPanelFolder
for Copy/Move info. It synchronizes current Qt selection through the existing
SaveSelectedState boundary, gets source operated indices and invokes the unchanged
summary body on the active bound CPanel. Full source OnNotify remains mandatory
P0-02 lifecycle/selection scope; the synchronization call does not claim to port it.
SetCurrentFolderPrefix is narrow temporary Qt display-path glue, preserving the
current archive/physical address and separator until source LoadFullPath/CFolderLink
are connected in P0-02.2b3d. No new path traversal algorithm is introduced.

Observable correction: source CAgentFolder exposes directory size only in normal
mode; flat directory size is VT_EMPTY. The removed row adapter supplied proxy
totals in both modes. The existing copied AddPropValueToSum now propagates this
absence into the summary instead of presenting an unsupported flat total.
Unknown file size and defined zero retain source semantics; source names/order
come from actual indices rather than visible row order. Copy/Move dialogs and
archive/filesystem operation routing are unchanged. Properties still has its
explicit snapshot migration scope P0-02.2b3b3c2; remaining consumers/teardown are
P0-02.2b3b3c3. No parent module/plan step is completed by this substep.

Validation on macOS 2026-09-29: affected app/selection/runtime/Copy/window targets
built. Focused panel_folder/panel_runtime/panel_copy_info/copy_dialog/ui_operations/
panel_selected_state/flat_view/ui_navigation passed 8/8 (6.63s). The runtime
Move regression covers source flat-directory absence versus normal total, known
file size, unknown file size, Unicode source names and Cancel in all four views;
the previous proxy-total adapter would fail the flat expectation. Sixteen fresh
440×250/340×220 reports reviewed: label, destination/Browse, summary and footer
fully visible in the source order. Source modules/file counts are unchanged.
./scripts/check.sh passed formatting, app build and all 64 CTest (32.12s).
Windows/Linux runtime and Windows rendered parity remain unverified.

## P0-02.2b3b3b3 — readonly runtime integration acceptance (2026-09-29)

Project baseline `72ce34b`; unchanged source baseline [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reviewed PanelFolderChange.cpp SetNewFolder/ReleaseFolder, PanelItems.cpp
RefreshListCtrl LoadItems/name restoration and FSFolder/Agent read-only item
owners against the integrated scopes recorded below. No new functional body or
source revision is introduced by this test-only acceptance iteration.

The focused panel_runtime suite exercises filesystem → archive → filesystem in
normal/flat and all four views, Unicode names/duplicate basenames, selection
clearing across owners, source-index-to-property mapping after presentation sort,
retained immutable archive input and distinct native/archive/unknown/zero sizes.
It mutates a filesystem file after folderReady but before GUI presentation: the
display must retain the loaded source size until the next source reload, proving
that QFileInfo is not a second metadata owner. Another case opens an archive
before native filesystem handoff and waits for retired workers' deletion; stale
folderReady/completed signals must never overwrite the archive.

Completion covers the readonly source runtime scope P0-02.2b3b3b only. Full source
RefreshListCtrl columns/formatting/icons/notification/timer branches, source full
path/parent stack, optional interfaces and operations remain mandatory in their
explicit P0-02/P0-03/P0-04/feature owners. Copy/Properties still rebuild Qt metadata
snapshots; removing those duplicate consumers is the next P0-02.2b3b3c prerequisite,
not claimed as completed here. No visible layout/text change or screenshot is
required for this regression-only scope; Windows/Linux runtime remains unverified.

Validation on macOS 2026-09-29: panel_runtime built and passed 1/1 (1.45s);
focused runtime/selection/focus/flat/navigation/archive states/scanner/nested
archive/Properties/Copy summary passed 10/10 (7.37s). ./scripts/check.sh passed
formatting, app build and all 64 CTest (26.13s). No production sources, source
revision or module/file counts changed. The parent plan remains 1/44 complete;
this closes only the runtime-loading child and continues to consumer migration.

## P0-02.2b3b3b2c — bound filesystem runtime (2026-09-29)

Project baseline `cf83093`; source baseline [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
CPP/7zip/UI/FileManager/FSFolder.cpp LoadItems/LoadSubItems/GetRelPath/GetProperty,
PanelItems.cpp RefreshListCtrl/GetItemName and PanelFolderChange.cpp SetNewFolder.
No upstream revision is changed. Existing copied filesystem functional bodies,
types, module paths and root-first traversal remain authoritative.

The normal window replaces QDir::entryInfoList with factory/SetNewFolder/LoadItems
and presentation of source Files/GetRelPath. Flat loading binds the completed
worker-owned folder before publishing GUI batches. Both publish actual source
indices and use the bound folder's typed IsDir/Size properties rather than
QFileInfo target metadata. CPanel's borrowed GetFolder getter serves this production
renderer; COM ownership remains with CPanel and the scanner. QString/QFileInfo,
icons, display paths and remaining timestamp formatting are Qt presentation;
they do not enumerate again. Native-to-Qt conversion is shared in qtlocationfolder,
including QFile::decodeName macOS normalization and native Windows wide paths.

RefreshListCtrl's bound-folder name branch now uses the original GetItemName
property helper, matching the source name/prefix restoration loop. Its legacy
unbound fixture branch retains QtPanelList lookup. Native macOS names and visible
Qt names can have different normalization; comparisons must use one source owner,
while path/display conversion stays at the Qt boundary. No independent Unicode
comparison algorithm or extra raw-name role is introduced.

Necessary traversal deviation: the original Windows LoadSubItems recurses every
directory in flat mode, including reparse directories. A narrow native attribute
guard preserves the existing Qt no-follow policy and avoids junction cycles.
POSIX already uses unchanged Fill_FileInfo(false); links retain lstat size/type,
so the runtime now displays link metadata rather than QFileInfo's followed target.
Windows directory links retain native directory attributes. Qt QFile::link creates
shortcuts on Windows, so local POSIX link fixtures do not validate that branch.

Scope remains partial: full source RefreshListCtrl (columns, icon/timer/notify
branches), BindToPath/LoadFullPath and outer/nested CFolderLink are mandatory later
P0-02 owners. Complete property catalogs, raw/link/stream properties, formatting,
operation interfaces and engine branches remain mandatory P0-03/P0-04/feature
owners. This integration does not complete those modules or parent P0-02.

Validation on macOS 2026-09-29: affected app/native/window targets built;
focused fs_folder/location_folder/filesystem_scanner/panel_selected_state/flat_view/
ui_navigation/ui_operations/filesystem_operations/file_transfer/properties/
panel_copy_info passed 11/11 (5.82s). New normal/flat runtime regression checks
link length/type, absent directory size, no cycle traversal and explicit directory
link navigation; selected/current decomposable Unicode names survive refresh and
mode changes in all four views. Four fresh 980×620/640×480 filesystem runtime JSON
reports reviewed: pathCombo/address, active table/view stack and status bar fully
visible, with the existing grouping/control order. ./scripts/check.sh passed
formatting, app build and all 63 CTest (30.62s). Windows/Linux runtime and rendered
Windows parity remain unverified; no screenshot/pixel-parity claim. Counts unchanged:
34 src/CPP cpp (30 UI), 36 root Qt cpp, 57 vendor files.

## P0-02.2b3b3b2b — native worker and GUI ownership handoff (2026-09-29)

Project baseline `52ed631`; pinned upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Calls existing source CFSFolder Init/SetFlatMode/LoadItems/LoadSubItems/GetRelPath
and the previous exact Clear/cancel adaptation. Native enumeration remains in
that single authoritative module; FilesystemScanner's independent QDirIterator
walker is removed. Root normal QDir listing remains explicit migration scope
P0-02.2b3b3b2c, not completed here.

PanelItems.cpp RefreshListCtrl calls LoadItems synchronously. Inspected
PanelOperations.cpp CThreadFolderOperations ProcessVirt covers create/delete/
rename, not an asynchronous listing handoff. New filesystemfolderloader.cpp/.h
is a narrow Qt thread service for that missing cross-platform integration:
creates the folder via the existing factory in run(), installs atomic cancel/
count callbacks, calls the original loading body, removes callbacks and releases
failed results. Progress signals are throttled to first/every 128 source records.
The caller waits for QThread completion before reading/detaching its result;
default upstream non-atomic COM references are never concurrently used by GUI
and the loading worker. An unfinished source folder is never published.

FilesystemScanner keeps cancelled workers until completion, suppresses obsolete
generations, and cancels/joins them before destruction. GUI receives a completed
source folder and source-index batches (max 128/8ms scheduling), with QPointer
guards for reentrant destruction from callbacks. QFileInfo is presentation input
created from source GetRelPath, not a second enumeration. POSIX filename conversion
uses QFile::decodeName; local Qt qfile.h explicitly maps macOS NFD to NFC. Windows
converts native wide paths without an ANSI round trip. Native source bytes and
metadata remain unchanged; bypassing Qt decoding initially failed existing
Unicode navigation/focus regressions and was corrected at this boundary.

Observable adaptation: native scanning now runs off the GUI thread with count
progress; rows appear in bounded GUI batches after immutable-owner handoff,
instead of while an independent walker was still enumerating. Both stages remain
responsive/cancellable. Status distinguishes scanning from shown rows; failure
reenables the list and reports unreadable folder. Widgets/7zz operations remain
outside native enumeration. CPanel filesystem binding and normal listing remain
mandatory P0-02.2b3b3b2c; no full source RefreshListCtrl/BindToPath claim is made.

New filesystem_scanner tests check GUI batches/source-index order/retained COM
metadata, native progress restart rejecting old owners, presentation restart,
cancel before start, active teardown, callback-driven deletion and invalid path.
flat_view verifies cancellation/navigation while rows grow and normal/compact
progress JSON; panel_selected_state covers focus/selection in four views.
Affected scanner/flat_view/selected_state/ui_navigation built; final focused 5/5
passed (4.05s), macOS 2026-09-29. ASan filesystem_scanner passed 1/1 (0.72s,
leak detection disabled). Four current-build progress reports reviewed at
980×620/640×480: pathCombo, active table/view stack and status bar fully visible;
normal/compact navigation grouping retained. ./scripts/check.sh passed formatting,
app build and all 63 CTest (28.39s). No screenshot/pixel parity claim.
New source thread service adds one root Qt cpp (36 total); src/CPP cpp remains
34 (30 UI), vendor remains 57 unchanged files. Windows/Linux runtime unverified.

## P0-02.2b3b3b2a — cancellable source filesystem loading (2026-09-29)

Project baseline `9014788`; exact source [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
CPP/7zip/UI/FileManager/FSFolder.cpp LoadSubItems/LoadItems and FSFolder.h Clear.
Copies original Clear body/name (Files/Folders, commented Streams branch) and
uses it instead of duplicate local clears. Existing source enumeration body and
root-first/recursive Folders start/end grouping are unchanged. Prior POSIX native
enumerator/Fill_FileInfo adaptation and explicit EOF/failure propagation remain.

The Windows source loads synchronously without this Qt cancellation boundary.
To run the same body in a worker, the existing Qt integration callbacks CheckBreak
and SetCompleted are checked before each enumerator Next and after Files.Add,
respectively. False returns E_ABORT and reports no successful complete listing;
count progress is monotonic and no widgets run in these callbacks. No replacement
walker, Qt container or changed enumeration order is introduced. Source callback
interfaces for operations remain mandatory later; these existing callbacks are
only narrow integration services. LoadItems clears partial records on a failed
HRESULT or exception before its existing COM_TRY converts the exception. This
prevents a cancelled/failed worker from handing off incomplete folder data.

New fs_folder regression covers cancellation before enumeration, after reaching
flat recursion, and an injected progress allocation exception, with pre-existing
rows cleared, zero item count/no stale Folders, monotonic progress and successful
reload after removing callbacks. Unicode/space/non-BMP fixture paths remain local.
Checks macOS 2026-09-29: affected fs_folder/filesystem_operations/location_folder/
flat_view targets built, focused 4/4 passed (1.21s), ./scripts/check.sh passed
formatting/app build/all 62 CTest (26.55s). No visible or vendor changes. Worker handoff and actual
window filesystem binding remain mandatory P0-02.2b3b3b2b/c; this substep does not
complete those consumers or P0-02. Windows/Linux runtime remains unverified.

## P0-02.2b3b3b1 — owned archive runtime (2026-09-29)

Project baseline `190abef`; upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reuses the already copied Agent.cpp CAgent proxy lifetime/ReadItems/root binding,
CAgentFolder BindToFolder_Internal/SetFlatMode/LoadItems/GetItem/GetProperty and
PanelFolderChange.cpp SetNewFolder/ReleaseFolder bodies. No second functional
implementation or proxy instance is introduced: MainWindow's value CProxyArc is
removed. Its archive root is IFolderFolder/CMyComPtr created through the existing
Qt input factory; the active source CAgentFolder is retained by CPanel::_folder.
A read-only CAgentFolder GetProxy accessor is Qt presentation glue for existing
path expansion/aggregate/column consumers; the source tree stays with CAgent.

The current FindDir path adapter resolves a directory, then the original
BindToFolder_Internal constructs its owned folder. Source SetFlatMode/LoadItems
populate matching indices before SetNewFolder clears/replaces rows; every Qt
row's existing param is this folder's index. This deliberately does not claim
the full source BindToPath body or its omitted alternate-stream/subarchive
branches: those remain mandatory in P0-02.2b3d after CFolderLink. Current archive
path fields remain temporary integration transport until that owner exists.

Listing constructs the immutable owner before publishing entries/root. Factory
HRESULT failure is retained until the backend's matching completion, then uses
existing error/rollback UI. Opening another archive or a filesystem location
releases the active folder before its root. Existing nested Qt parent frames
retain the root by COM ownership, so returning restores the same source agent
rather than rebuilding an independent proxy. Full outer+nested CFolderLink and
temporary archive teardown source order remain mandatory P0-02.2b3c.

File size/packed columns now read typed source GetProperty variants. Source
PanelListNotify.cpp SetItemText numeric/text branches and its
ConvertPropertyToShortString2/Windows PropVariantConv.cpp VT_EMPTY handling establish
the empty-versus-zero distinction; the full formatting module remains P0-03/04.
VT_EMPTY
renders empty without a numeric sort role; defined zero renders 0. Other current
columns retain Qt/7zz entry transport; complete property catalogs, presence flags,
UInt64 parsing, timestamps, raw/CRC/_proxy2 and operations remain P0-03/04 and
feature owners. Filesystem runtime is still mandatory P0-02.2b3b3b2; Copy/Properties
snapshot adapters and remaining consumers are P0-02.2b3b3c, not completed here.

New panel_selected_state typed-size regression covers normal/flat and four view
modes, unknown versus known zero, caller-input clearing, normal/compact geometry
and current-build widget JSON. Existing refresh, failure/cancel, navigation,
properties and real nested-archive tests cover direct consumers. Checks macOS
2026-09-29: affected app and nine window targets built; final focused 9/9 passed
(7.29s). Current-build 16 reports inspected: pathCombo and active view fully
visible, normal/compact geometry preserves navigation above the view and column
grouping; inactive stack view is hidden. ./scripts/check.sh passed formatting,
app build and all 62 CTest (25.45s). Qt archive row rendering is extracted into
appendArchiveEntry in the same cohesive file, separating binding from rendering;
Windows/Linux runtime and rendered Windows parity remain unverified. No upstream
file or vendored file changed; 34 adapted CPP cpp (30 UI), 35 root Qt cpp,
57 exact vendor files remain unchanged. CMake links window consumers to the
existing sevenzip_location library; authoritative Agent sources retain one path.

## P0-02.2b3b3a — source list item parameters (2026-09-29)

Project baseline `57804df`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Panel.h GetRealItemIndex body is copied to PanelItems.cpp, retaining GetItemParam
failure throw 1 and unsigned cast. QtPanelList replaces LPARAM/CListView retrieval
with quintptr/QModelIndex and the existing Qt UserRole+6 source-order parameter.
Rows without that role retain the legacy/fixture physical-row fallback. This
replaces the independently written GetRealItemIndex(row)==row shortcut; source
indices survive sorting. Parent UInt32(-1) remains excluded.

GetItemRelPath uses source GetItemPrefix+GetItemName when an IFolder owner exists.
Presentation-only legacy panels use a derived param-to-row map, rebuilt once at
Save/Refresh and cleared on attach/delete, to avoid quadratic row lookup. This
belongs to Qt control adaptation, not folder algorithms. GetFolderItemCount
adapts source GetNumberOfItems at that boundary, with row count only for unbound
legacy panels. Save's Qt status synchronization fills source-index bits, then
assigns selected rows by their parameter. Source SaveSelectedState/GetSelectedNames
and property bodies remain. Invalid parameter/range rejects before vector access;
the parent parameter is skipped.

The previously copied partial RefreshListCtrl still runs after Qt loading/sorting.
Matching branches and independent cursor/selection semantics remain; the Qt row
loop maps each row to its source index and writes source-index bits. Folder-item
count controls reserve/size even with omitted rows. This does not port omitted
full RefreshListCtrl loading/columns/icons/timer/redraw/sort or OnNotify tracking;
those remain mandatory in P0-02 runtime/lifecycle and P0-03/04 feature owners.
Existing four-view presentation remains; no dots row is added to the application.

New panel_folder regression permutes source records versus displayed rows,
selects one item with a different unselected current item, verifies saved names,
real selected indices and correct source size/path summary, then restores after
another permutation and with a hidden-row index gap. A fixture-only parent
sentinel must not become a folder index or selected operation item. Existing
panel_selected_state covers filesystem/archive/flat refresh in four views.
Affected app/panel_folder/panel_selected_state/flat_view/file_viewer/ui_navigation
targets built; focused 5/5 passed macOS 2026-09-29. Initial full gate failed
ui_operations SIGTRAP after active listing teardown; all other 61 tests passed.
Single rerun and 15 repetitions passed, as did 15 ASan ui_operations repetitions,
so those repetitions alone did not establish safety. A new deterministic
archive_mutation destroyedActiveBackend test fails before the fix (unexpected
finished callback, followed by SIGTRAP), with a helper process still active.

ArchiveBackend destructor now disconnects QProcess callbacks, kills/waits for its
owned process while fields remain alive, and removes an unfinished mutation's
temporary archive after process exit. This is necessary Qt/7zz integration glue:
upstream Windows/ProcessUtils.h CProcess/CHandle has no Qt asynchronous signals
or this backend buffer lifetime, and no corresponding upstream body can be
copied. The previous implicit destructor destroyed QString/list state before
QProcess emitted readyRead/finished. The regression covers active listing and
partial create, no shutdown completion callback, original input preserved and
temporary file cleanup. Full source operation/lifecycle scope P0-02.3 remains
mandatory. Fixed focused 7/7 passed (6.43s), ASan ui_operations/archive_mutation
2/2 passed (10.26s, leak detection disabled). Final ./scripts/check.sh passed
formatting/build/all 62 CTest (29.49s), macOS 2026-09-29; layout/text unchanged;
Windows/Linux runtime unverified.

## P0-02.2b3b2c2 — Qt location factory and unknown size boundary (2026-09-29)

Project baseline `70fbcf3`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
New qtlocationfolder.h/.cpp is Qt/7zz input glue: no upstream implementation takes
QString/ArchiveEntry snapshots. Filesystem factory converts native QString to
FString, calls source FileName GetFullPath/NormalizeDirPathPrefix and CFSFolder
Init, and follows source scoped COM/Detach/COM_TRY conventions. Archive factory
maps the existing ArchiveEntry fields into owned QtPanelFolder typed records,
then uses source CAgent initialization/root binding ownership. It creates no
engine operations or commands. Source folder/module bodies stay in their owners.
sevenzip_location registers the factory and the existing authoritative Agent
sources; it links filesystem/panel dependencies. It is not connected to window
runtime until P0-02.2b3b3. Archive virtual outer path requires later CFolderLink/
LoadFullPath port; root relative path remains empty as source semantics require.

Unknown metadata support exposed migration debt: CalculateSizes directly added
signed negative sentinels to unsigned totals. AgentProxy.cpp GetSize is now copied
from source zero-init/CPropVariant/ConvertPropVariantToUInt64 control flow, with
ArchiveEntry typed lookup replacing IInArchive GetProperty. Negative sentinel
becomes VT_EMPTY and zero aggregate contribution, distinct from defined zero.
CalculateSizes retains original DFS/control flow, restores source CUIntVector
and Add/Size/DeleteFrom operations, and source per-item GetSize/size/packSize
addition. The current proxy's std-container directory/file storage and Qt names
remain an earlier adaptation; CRC definedness/addition, tree/alt/raw and memory
limits remain mandatory P0-03/04, not completed by this size subset.

CPanelItem PackSizeDefined is a local metadata-input flag; default true preserves
existing summaries/Properties snapshot behavior, while the factory sets it from
the negative sentinel. QtPanelFolder returns VT_EMPTY for unknown packed size.
Source ArchiveEntry parser still lacks full presence/type flags and uses qint64;
full engine catalog, UInt64 range, timestamps/link/raw properties and presence
parsing remain mandatory in P0-03/04. Factories expose the current read-only
subset, not the complete IFolder/engine port. Backend retains unsafe path and
link validation for extraction/mutations; these factories perform no writes.

location_folder fixtures verify source normalized path/Unicode binding, missing/
file/empty rejection, child lifetime after factory/root release, archive input
release with typed metadata/type retained, unknown versus zero size/packed size,
root aggregate totals and empty archive/null parent. archive_proxy adds unknown
and unknown-only directory aggregate checks that fail with the old signed add.
Affected location_folder/panel_folder/panel_copy_info/properties/agent_folder/
archive_proxy targets built; focused 6/6 passed macOS 2026-09-29.
Full `./scripts/check.sh` with staged files passed formatting, both diff checks,
app build and 62/62 CTest.
No visible UI/layout/text changes;
Windows/Linux runtime unverified.

## P0-02.2b3b2c1 — source Agent proxy ownership subset (2026-09-29)

Project baseline `0297571`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Agent.h/.cpp adds CAgent _proxy/ArchiveType fields, source constructor null default,
destructor conditional delete, ReadItems existing-proxy early return/allocation/
Load skeleton and BindToRootFolder allocate/Init/Detach body. CAgentFolder Init
copies source _agent strong reference/_agentSpec raw identity; child/parent Init
again propagates the same agent rather than only a borrowed proxy/provider.
Default child flat=false remains. Existing borrowed overload remains only for
the current lexical stack traversal/tests, pending window migration.

The engine IInFolderArchive contract requires IArchive, LoadCodecs and extract
callback implementations; the current engine boundary is ArchiveBackend/7zz.
This partial CAgent exposes only IUnknown ownership, replacing the source
CMyComPtr<IInFolderArchive> with CMyComPtr<IUnknown>, not claiming the omitted
engine interface. GetArchive returns the existing typed metadata IFolderFolder
provider instead of IInArchive. Source proxy construction loads the owned 7zz
entry snapshot via the earlier CProxyArc adapter; _archiveProps substitutes
archive-link readiness. Qt Init copies that immutable snapshot/provider/type.
It rejects null provider and repeat initialization, so a retained child cannot
refer to a proxy replaced by a later listing; refresh creates a new owner.
Load exceptions delete/reset the partial proxy before propagating to COM_TRY,
allowing a retry without reusing incomplete data. These are explicit integration
adaptations, not copies of Open/ReOpen/Close or complete ReadItems engine logic.

Partial scope: _proxy2/tree/alt streams, RAM-limit setup, archive property catalog
discovery, CArchiveLink/load-codecs/open/close/reopen and engine/extract/update/
raw/zone/operation contracts remain mandatory in P0-03/04 and their inventory
owners. This source ownership subset does not mark them done or N/A. Metadata
provider/catalog adaptations from earlier b2a remain explicit. Factory assembly
and runtime panel binding remain following substeps; no window behavior changes.

agent_folder regressions release input lists/provider, root and explicit agent
references before traversing surviving descendants; then release child/grandchild
and walk back through newly bound ancestors to root. Typed metadata and archive
type stay valid. Missing-provider/uninitialized root fails; initialized empty
archive succeeds; repeat Init is rejected without changing the existing folder.
Affected app/agent_folder/archive_proxy/flat_view/ui_navigation targets built;
focused 4/4 passed macOS 2026-09-29. Full `./scripts/check.sh` with staged files
passed formatting, both diff checks, app build and 61/61 CTest. No visible
UI/text/layout change. Windows/Linux runtime unverified.

## P0-02.2b3b2b3 — filesystem binding, parent and logical drives (2026-09-29)

Project baseline `e931b09`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
FSFolder.cpp copies BindToFolderSpec, both BindToFolder bodies and active desktop
BindToParentFolder branches, replacing the temporary unavailable binding methods.
Init now copies Windows CFindChangeNotification and FindFirst fallback validation;
WasChanged copies notification draining and bool result. IFolderWasChanged and
its member are added from FSFolder.h. LoadItems again consumes WasChanged before
clearing/loading. Index guards and COM exception guards adapt existing public
entry points; source allocation/Init/Detach/failed-parent control flow remains.

The source File Manager parent body is Windows-specific. POSIX substitutes an
absolute slash-root check for IsDrivePath and returns S_OK/null above `/`; native
source stat-follow-link validates Init because the Windows notification fallback
does not exist there. This rejects missing/file paths during bind rather than
deferring error until load. Other parent path splitting stays source FString.
The source NetFolder block is commented out in this baseline; its inactive text
is omitted, not implemented here. Required active network/root workflows remain
owned by P1-11/P2-05. Windows UNC/super paths retain source path helpers and
native services, but cannot be declared platform-verified from macOS tests.

FSDrives.h/.cpp adds the source read-only logical-drive subset: CDriveInfo fields,
Init, normal LoadItems, GetNumberOfItems/GetProperty/AddExt, folder binding,
null parent, catalog and folder type/path. Native Windows FileSystem.cpp/.h are
vendored unchanged and built on Windows. Source drive-type strings and removable
read guard, known-size flags and native metadata calls remain. The copied AddExt
uses DRIVE_CDROM, with its Win32 numeric value supplied only for POSIX compilation.
LangString(IDS_COMPUTER) becomes the Qt Russian Computer label at the presentation
boundary; volume/super prefixes remain source strings. COM guards added around
load/bind. The read-only module is the dependency of Windows CFSFolder drive-root
binding; POSIX normal root does not expose a Windows drives folder.

Partial FSDrives scope: device/physical-volume enumeration, CDriveInfo device-name
helper, CopyFileSpec/GetFileSize/CopyTo, full IFolderOperations and system-icon
bodies remain mandatory under inventory P1-11/P2-01/05/P3-05. Volume-mode load
returns E_NOTIMPL until that source scope is ported; it is not N/A or complete.
Source unsupported mutations must stay disabled. POSIX CFSDrives LoadItems is
unavailable because its native drive enumeration is Windows-specific; actual
POSIX root is handled above. Filesystem comments/packed/link/raw/stat/compare/
clone/icon/COM operations omissions from b2 remain required in owning steps.
No runtime window integration or visible UI change is claimed by this subset.

fs_folder regressions add Unicode name/index binding, child/ancestor ownership,
file/missing/invalid binding failures, root parent (Windows FSDrives conditional
branch), default child flat=false after flat-index bind, WasChanged identity and
empty-root error. Focused fs_folder/file_find/filesystem_operations/file_transfer
4/4 passed macOS 2026-09-29; exact vendor bytes 57/57 (56 source/header files
plus license). Full `./scripts/check.sh` with staged files passed formatting,
both diff checks, app build and 61/61 CTest. Windows/Linux runtime unverified.

The dated P0-01 FSDrives inventory row is superseded only for these copied
read-only symbols; its device imaging/system icon/operations scope remains open.

## P0-02.2b3b2b2 — CFSFolder enumeration/property subset (2026-09-29)

Project baseline `e77ee7f`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
FSFolder.h copies the IFolderFolder/IFolderSetFlatMode/CMyUnknownImp subset,
_flatMode default false, FString _path/Folders and CDirItem CFileInfo base,
parent/stat/pack fields. Source file/path types replace QString metadata debt.
FSFolder.cpp copies Init assignment, GetNumberOfItems, SetFlatMode, LoadSubItems
normal/flat recursion/grouping, GetRelPath, basic GetProperty switch,
GetFolderProperty and property catalog/flat-prefix count bodies. PropId.cpp and
its StdAfx.h are vendored unchanged for the authoritative property-type table;
GetPropertyInfo uses the original IFolder macro, without a new type map.

The Windows File Manager source expects enumerator CFileInfo output. POSIX uses
source Next(CDirEntry) then Fill_FileInfo(false), retaining no-follow metadata.
Found/EOF/error is explicit; recursive failures propagate instead of the source
bool-only loop collapsing errors or ignoring recursive results. LoadItems clears
partial/stale rows on error and catches COM exceptions. Source comment reset and
WasChanged are omitted until those dependencies are ported. Time properties use
upstream PropVariant_SetFrom_FiTime for timespec/FILETIME; attributes use source
GetWinAttrib. Qt conversion remains at mutation/constructor boundaries. AddPath
imports native source file metadata for the existing mutation snapshot, retaining
the requested basename when metadata lookup fails so operation errors still
identify a missing file;
callbacks and no-overwrite/cancel policy remain. Formatting is applied separately.

Partial scope: Init Windows notification/validation and binding/parent/root
remain required in the next iteration; binding returns E_NOTIMPL here. Window
integration remains P0-02.2b3b3. Compressed allocation, comments/TextPairs,
ReadFileInfo/ReadChangeTime, link/raw/reparse, compare, folder-stat calculation,
WasChanged/Clone/system icon and IFolderOperations COM bodies remain mandatory
in P0-03/04 and their feature owners. Known unsupported properties return
E_NOTIMPL; no dummy values claim completion. The source catalog without
FS_SHOW_LINKS_INFO remains, including deferred properties; its additional link
branch is not yet copied. Existing typed mutations do not complete COM operations.

New fs_folder tests cover normal/flat reload, Unicode/non-BMP, base-relative
prefixes, typed size/time/attributes versus uncalculated directory size, source
type/path/catalog, invalid indices and failed reload clearing. The existing
filesystem_operations assertion now converts source FString at its boundary.
Focused fs_folder/file_find/filesystem_operations/file_transfer 4/4 passed
macOS 2026-09-29. The first focused run caught the missing-file basename loss;
fixed before passing checks. The Unicode fixture compares canonical-equivalent
names because macOS can return decomposed filesystem names; production source
names are preserved. Initial builds also exposed the PropId StdAfx dependency
and the global FiTime adapter namespace, corrected before passing checks.
Full `./scripts/check.sh` with staged files passed formatting, both diff checks,
app build and 61/61 CTest. Exact vendor bytes: 55/55 (54 source/header files plus
license). Windows/Linux runtime unverified.
No visible UI/text/layout change.

## P0-02.2b3b2b1 — portable filesystem services (2026-09-29)

Project baseline `49f7975`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
FileFind.cpp/.h, FileIO.cpp/.h, FileName.h, TimeUtils.cpp/.h and MyLinux.h
are copied unchanged. FileFind/TimeUtils compile on all targets; FileIO.cpp
compiles on Windows for FileFind's native handle information dependency.
POSIX FileIO.cpp is not built: its file writes require additional FileDir
umask/time services, outside this read-only enumeration prerequisite.
No device-file feature is enabled; its allocator/engine integration remains
required when that owning scope is ported. Windows native bodies remain in
the vendored modules; their runtime acceptance is unverified locally.

FileName.cpp is copied in full to its original adapted path. Only include
locations and required formatting change; full path resolution, dot handling,
Windows long/super/UNC path bodies and symbol boundaries remain upstream.
It includes our authoritative FileDir.h rather than duplicating FileDir.
FileDir.cpp GetCurrentDir copies both source platform bodies, retaining the
Windows dynamic buffer/race checks and POSIX PATH_MAX then dynamic getcwd
fallback. The local MY_PATH_MAX alias is expanded to PATH_MAX and source
comments/formatting differ; existing Qt deletion services remain untouched.
Other FileDir functions and platform branches remain mandatory in their
owning scopes; this prerequisite does not complete CFSFolder or FileDir.

New file_find fixtures verify Unicode/non-BMP names and metadata, omission of
dots, distinction between EOF and a missing directory, source full-path dot
resolution/current directory and POSIX no-follow versus follow-link metadata.
Windows reparse fixtures remain required; the POSIX-specific test is skipped
there explicitly. No visible UI/workflow change: runtime panel integration
remains P0-02.2b3b3. Focused file_find/filesystem_operations/file_transfer 3/3
passed macOS 2026-09-29. Full `./scripts/check.sh` with staged files passed
formatting, both diff checks, app build and 60/60 CTest. Vendor bytes: 53/53
(52 source/header files plus license).
An initial direct CMake build lacked macOS SDK C++ headers; the documented
scripts/build.sh toolchain workaround built affected targets successfully.

Inventory supplement (the P0-01 112-path table remains its dated snapshot):
`CPP/Windows/FileName.cpp` now has P0-02/P0-03/P1-03/P2-05 ownership. All source
functions and active platform branches are copied; include locations/formatting
are adapted as above. Dependency boundary: adapted FileDir GetCurrentDir and
unchanged MyString/StringConvert/FileName.h; Windows long-path bodies remain
present and require platform acceptance. The adapted FileDir inventory row's
current-directory omission is superseded by this source port; its other listed
omissions remain open.

## P0-02.2b3b2a — Agent _proxy folder binding subset (2026-09-29)

Project baseline `07b7326`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied Agent.h CAgentFolder IFolderFolder/IFolderSetFlatMode/CMyUnknownImp
declarations and _items CRecordVector (replacing the old std::vector adaptation).
Copied Agent.cpp _proxy branches of GetNumberOfItems, GetPrefix, GetProperty,
BindToFolder_Internal, both BindToFolder overloads, BindToParentFolder,
GetFolderProperty and SetFlatMode. Source constructor defaults remain; creating
a child/parent starts with flat=false, as upstream Init/constructor does. Source
ComTry.h is vendored unchanged; COM_TRY catches remain in binding/property bodies.
Existing LoadFolder recursion/control flow remains and uses original Add/Clear/
Size operations. The legacy count convenience now delegates to one authoritative
source interface count body rather than retaining a duplicate algorithm.

SET_realIndex_AND_dir becomes the existing Qt GetItem pair adapter, retaining
normal/flat mapping. Current CProxyArc model still has Qt string/std-container
adaptations from its earlier partial port; property/path access converts at the
boundary. GetPrefix retains base-relative folder-path helper control flow.
Source GetProperty's in-process _agentSpec->GetArchive property calls become an
explicit IFolderFolder metadata provider, owned with CMyComPtr. Init passes that
provider and archive type into children. Source metadata/path/count/type cases
are retained; property catalog methods currently delegate to the provider and
are not a port of the engine catalog logic. Missing provider returns E_NOTIMPL.
IsBound/row guards adapt the source open-archive invariant to current proxy data.
Initial builds exposed missing MyString/Defs includes; fixed before passing runs.

This is a partial read-only _proxy folder contract, not whole Agent.cpp. _proxy2/
alt streams, agent archive-link lifecycle, read-only/hash/raw/archive-level/
CRC properties (including directory CRC aggregation), name/prefix fast interfaces,
compare, streams, zone and operations bodies remain mandatory in their P0-03/04
and later owning feature scopes from the P0-01 inventory. They are not marked
done or N/A. No engine/format implementation is added. Proxy lifetime is still
explicitly external: existing window stack enumeration is preserved; long-lived
folder binding cannot replace it until P0-02.2b3b2c provides owned context.
Only metadata provider lifetime is shared by COM ownership in this substep.

New agent_folder fixtures verify source path binding by name/index, ancestor/root
handling, flat prefixes relative to base, duplicate basenames, Unicode/non-BMP,
metadata delegation after releasing the root folder, source folder totals/type,
file/missing-folder/invalid-row rejection and unbound load failure. Current-build
archive_proxy/flat_view/ui_navigation/agent_folder focused 4/4 passed macOS
2026-09-29. Full `./scripts/check.sh` with staged files passed formatting, both
diff checks, app build and 59/59 CTest. Exact-source vendor bytes: 45/45
(44 source/header files plus license). No layout/text changes;
Windows/Linux runtime unverified.

## P0-02.2b3b1 — panel IFolder property owner (2026-09-29)

Project baseline `dfee6fd`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied PanelItems.cpp GetItemName(returning UString), GetItemPrefix,
GetItem_BoolProp, IsItem_Folder (including optional _isDirVector),
GetItem_UInt64Prop and GetItemSize including IFolderGetItemName fast path.
Original CPropVariant type checks/throw IDs, ConvertPropVariantToUInt64 and
parent-item branches are retained. GetItemRelPath's name/prefix concatenation
is retained with QString conversion; bound Qt selection rows still supply their
presentation snapshot until the complete folder-load stage exists.

Copied PanelFolderChange.cpp GetFolderPath body; ReleaseFolder/SetNewFolder
release/assign/query sequence for IFolderFolder and IFolderGetItemName. COM smart
pointers remain upstream CMyComPtr. Other optional compare/raw/alt-stream/
operation interfaces and deleted-item state are omitted in this partial scope,
remaining mandatory with their feature ports. DeleteListItems adapts to Qt model
row removal; directory/selected caches are cleared here because the omitted
RefreshListCtrl folder-load stage currently cannot clear them after replacement.
Full lifecycle/notify guards and parent stack remain open.

App.cpp AddPropValueToSum now copies original IFolder GetProperty/variant/numeric
conversion body rather than reading a custom struct directly. Qt strings remain
the presentation boundary. PanelMenu.cpp copies the GetNumberOfProperties/
GetPropertyInfo/GetProperty enumeration with source failure/continue branches;
the existing Qt formatted-BSTR helper still displays values. Source non-BSTR
formatting, raw properties, archive levels and shell invocation remain open.
This preserves current dialog names/values/order and Copy summary output.

QtPanelFolder is a narrow adapter for existing CPanelItem metadata, not a source
replacement for filesystem or Agent folder algorithms. It owns one copied
snapshot, exposes typed source size/dir/name/prefix fields and VT_EMPTY for absent
size, and describes existing localized/preformatted property pairs with private
property IDs. The descriptor union avoids using another item's value when a
property is missing. BindToFolder/BindToParentFolder explicitly return E_NOTIMPL:
this metadata adapter is only for read-only summary/property presentation; real
location adapters remain P0-02.2b3b2–b3. No mandatory binding is marked complete.
CPanel's _folder is now the original interface owner; the old authoritative
CObjectVector property implementation has been removed from CPanel.

Unchanged additional dependencies: CPP/Windows/PropVariantConv.h (only its inline
ConvertPropVariantToUInt64 is used) and WinDefs.h. PropVariantConv.cpp timestamp/
format bodies are not added or claimed. Tests cover interface-backed panel after
external owner release, owned Unicode/non-BMP metadata, numeric versus absent
sizes, unsupported-type source error, missing per-item metadata and list/cache
release; existing summary/property consumers remain required. Focused five CTest
passed on macOS 2026-09-29; ./scripts/check.sh with staged files passed formatting,
both diff-checks, build and 58/58 CTest; exact-source byte comparison 44/44 passed.
No visible-text/layout changes; Windows/Linux runtime unverified.

## P0-02.2b3a — portable folder interfaces/ownership dependencies (2026-09-28)

Project baseline `e7870e5`; exact source: [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied unchanged to third_party/7zip: CPP/Common/MyCom.h, MyUnknown.h,
MyInitGuid.h; CPP/7zip/IDecl.h, IProgress.h, IStream.h,
UI/FileManager/IFolder.h; CPP/Windows/PropVariant.h and PropVariant.cpp.
All declarations and variant implementation bodies remain byte-for-byte source.
Existing MyWindows.cpp supplies portable BSTR/variant services; Windows links
oleaut32 and uuid for source runtime and IID_IUnknown. MyCom uses original
non-atomic reference counting; folder owners must stay on the GUI thread unless
the upstream atomic configuration and its dependencies are separately enabled.

ClassDefs.cpp retains upstream include initialization order MyWindows →
MyInitGuid → interface declarations, with blank groups preserving it under
project clang-format. Agent.h is replaced by the narrower unchanged IFolder.h
dependency because current Agent.h does not declare these COM interfaces.
This partially initializes folder/stream/progress GUIDs only. Win32
MyWindowsNew.h/Explorer MyExplorerCommand and remaining Agent interfaces are
omitted here and remain required with their module ports. One ClassDefs owns
these GUID definitions; consumers include declarations without INITGUID.
First link caught include sorting ahead of initialization; order was fixed.

No archive engine or compression implementation is added. Runtime window folder
binding, CTempFileInfo/CFolderLink and source navigation/close bodies are still
open. New folder_contract test uses only a deterministic local mock of the exact
IFolderFolder interface; it verifies QueryInterface identity/rejection, refcount
destruction, caller-owned BSTR property info, Unicode variant copy/clear/
detach/attach, folder path and missing parent. This establishes portable local
dependency behavior, not completion of filesystem/archive folder adapters.
Exact-source comparison passed all 42 vendored files (41 headers/sources plus
DOC/License.txt); current inventory is 29 mirrored UI/80 missing, 32 src/CPP cpp.
Checks 2026-09-28 macOS: affected app/dependency/consumer targets built; focused
folder_contract/app_state 2/2 passed; initial ./scripts/check.sh formatting/build/
57 CTest passed; exact-source bytes 42/42. Staged diff-check then detected original
upstream trailing whitespace in newly tracked vendored files. The premature local
commit was soft-reset, preserving all changes. .gitattributes exempts unchanged
vendored h/cpp/c bytes from whitespace errors; local source/tests/docs remain
checked. Final ./scripts/check.sh with staged files and the explicit policy
passed formatting, both diff-checks, build and 57/57 CTest on macOS
2026-09-29 (local date). No required check remains failed for this substep.
No visual changes; Windows/Linux runtime unverified.

## P0-02.2b2 — archive/flat/edit selected-state consumers (2026-09-28)

Project baseline `c412350`; exact upstream [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
This is integration of the already copied Panel.h/PanelItems.cpp selection bodies
from P0-02.2b1, not a claim to port whole additional modules. Reference consumer
sequence: Panel.cpp ChangeFlatMode → RefreshListCtrl_SaveFocused and
PanelItemOpen.cpp OnOpenItemChanged path resolution → SaveSelectedState →
OnOpenItemChanged update → RefreshListCtrl. The capture/update/restore bracket
is retained across asynchronous ArchiveBackend signals rather than a synchronous
COM call. Full ChangeFlatMode flags/parent-stack ownership and OnOpenItemChanged
path/update/thread bodies remain mandatory migration work.

Archive refresh captures/restores the same CSelectedState used for filesystem
refresh. Flat-mode changes capture before replacing rows and restore after normal
loading or scanner completion. Removed legacy scanSelection_/refreshSelection_
QStringList owners; one pending scan snapshot and one archive snapshot remain as
Qt asynchronous transport. If an archive folder disappears, the old relative-name
snapshot is discarded when falling back to its parent; a same-named file in that
different folder must not become selected. Failure/cancel clears pending refresh
through existing navigation rollback. That rollback workflow remains unchanged.

Archive edit captures before replacement and restores on successful relisting;
it no longer forces selection of the edited entry independently of source state.
Failed/cancelled replacement retains displayed selection/current item and drops
the pending snapshot. Backend prepareMutation works on a temporary archive and
commits on success; unlike upstream's direct IFolder update, failure does not
require reloading a potentially partially changed original. Existing real
archive_mutation tests verify mutation rollback/cancellation. Local edit uses
the same filesystem refresh instead of discarding current/selected state.

Checks 2026-09-28 macOS: affected targets built; focused current-build
panel_selected_state/flat_view/ui_state/ui_archive_states/file_viewer passed 5/5;
./scripts/check.sh formatting/build/56 CTest passed. No layout,
visible-text or control changes; Windows/Linux runtime remains unverified.

## P0-02.2b1 — filesystem refresh selection/focus snapshot (2026-09-28)

Project baseline `5bc96ad`. Exact source: [ip7z/7zip
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied Panel.h CSelectedState fields/defaults; PanelItems.cpp
Get_ItemIndices_Selected, GetSelectedNames and SaveSelectedState active bodies.
Commented alternative implementations are omitted. Original UStringVector sorting
and selected-status-vector traversal remain authoritative. GetRealItemIndex is
a Qt snapshot row adapter: current presentation already loaded/sorted the rows,
so selection capture/restoration uses their displayed indices, rather than
Win32 LVITEM lParam and pre-sort folder indices.

RefreshListCtrl is a partial port of its selection/focus portion: focusedPos
initialization, focusedNamePure/selectedName separator optimization, per-item
full relative-path equality and FindInSorted selection matching, cursor selection,
missing-cursor clamp and EnsureVisible. SetFocusedSelectedItem preserves focused
versus selected distinction. QtPanelList replaces control calls, item-name and
prefix/name property access; GetItemRelPath uses the displayed relative name.
The table and icon/list views share this model/selection model. Qt's selection
notifications are synchronized into _selectedStatusVector at SaveSelectedState;
full OnNotify/_mySelectMode paths remain open. Current presentation has a parent
toolbar button and no dots row, so showDots=false; the parent-item restoration
branch remains required when dots rows are ported. _mySelectMode=false is the
existing selection contract, not completion of that upstream mode.

Rows have already been inserted/sorted by current filesystem presentation.
RefreshListCtrl folder LoadItems, COM/interfaces, columns, icon loading,
timer/notify guards, sorting and redraw bodies are not ported in this substep.
Their existing independent implementation remains migration debt. The source
GetItemName/IFolderGetItemName optimization is replaced by Qt UserRole+3 and
displayed relative-path property access; separators use portable UString/Qt row
data instead of Windows NName. Existing property snapshot consumers stay intact.

MainWindow filesystem refresh captures CSelectedState before reloading and calls
this panel portion after normal loading or scanner completion. The pending flat
snapshot is cleared on navigation; other QStringList selection consumers remain
for P0-02.2b2. Archive location/parent stack and full lifecycle are still open.
Checks 2026-09-28 macOS: affected targets and direct consumers built;
panel_selected_state/panel_focus/panel_copy_info/ui_navigation/panel_selection
5/5 passed; ./scripts/check.sh formatting/build/56 CTest passed. Initial test
fixture compared /var with canonical /private/var; corrected before passing
runs. No layout/text changes;
Windows/Linux runtime and rendered parity remain unverified.

## P0-02.2a — application/panel owners and remembered focus (2026-09-28)

Project baseline `eedf926`. Exact upstream — [ip7z/7zip
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied App.h kNumPanelsMax, CApp NumPanels/LastFocusedPanel/AppState/Panels,
GetFocusedPanelIndex/IsPanelVisible/GetFocusedPanel/SetFocusToLastItem bodies;
initial defaults moved from constructor to field initialization. Qt integration
sets NumPanels=1 for current single visible panel; two independent CPanel owners
exist without global g_App. AutoRefresh/settings/toolbar/drag and all other CApp
bodies remain open. This does not close two-panel UX or Create/Release lifecycle.

Copied Panel.cpp SetFocusToList/SetFocusToLastRememberedItem branch bodies;
Win32 control SetFocus replaced with QWidget::setFocus and null guards for
unbound/destroyed presentation. Panel.h _lastFocusedIsList retains source name;
OnCreate default true initialized on construction pending full Create port.
AttachPresentation and QObject eventFilter are Qt glue for two control handles,
CMyListView WM_SETFOCUS and PanelFolderChange OnNotifyComboBox CBEN_BEGINEDIT.
QPointer handles widget destruction; binding current list when mode switches
keeps focus memory. Existing snapshot constructor/property functions unchanged.

MainWindow owns CApp; history/favorites consume AppState. FM.cpp WM_SETFOCUS
delegation becomes top-level FocusIn event; this is not WindowActivate handling
(WM_ACTIVATE reference branch is commented). Filter installs only after panel
construction and removes before member teardown. Address Tab delegates to
focused CPanel::SetFocusToList; Alt+F1/F2 popup behavior unchanged. Qt events set
panel list/address memory; full CPanelCallbackImp title/drag/focus-index dispatch
and SetFocusedPanel remain for subsequent lifecycle/two-panel scopes.

App.h/Panel.h/Panel.cpp use original paths/names/license. New module is partial:
Panel.cpp remaining creation/notifications/operations services and omissions in
P0-01 table stay open. Header CApp owner adds no second functional implementation.
Inventory now has 28 mirrored UI/81 missing UI (31 local cpp incl3 supporting);
P0-01 tables record their earlier snapshot, not current completion counts.
Checks 2026-09-28 macOS: affected targets built; focused panel_focus/panel_key/
bookmarks 3/3 passed; ./scripts/check.sh formatting/build/55 CTest passed.
Initial construction-order and editable-combo focus failures were fixed before
the passing runs. No layout,
caption/control visibility/resizing changes; Windows/Linux runtime unverified.

## P0-02.1 — application history/favorites owner (2026-09-28)

Project baseline `36fad59`. Original — [CPP/7zip/UI/FileManager/AppState.h at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/AppState.h).
Copied CAppState fields FastFolders/FolderHistory and Save/Read bodies in original
order into src/CPP/7zip/UI/FileManager/AppState.h. Only added explicit QString
settings-path argument forwarded to existing QSettings adapters. Existing
CFastFolders/CFolderHistory retain upstream UStringVector and GUI-thread ownership;
no second algorithms or global singleton. Source notice/license retained.

MainWindow replaces separate members with CAppState and calls Read before menus
and panel construction; history/favorites adapters consume its members. Confirmed
edits still save their individual collection immediately, preserving existing
cancel/selection/history UX. No geometry/text/control changes. Application
state Save is available for upstream lifecycle port; full CApp ownership,
CPanel location/selection/focus and operation lifecycle remain P0-02.2/.3.
The P0-01 snapshot statement that CAppState was absent is superseded here;
all other omissions remain open. Header-only original struct needs no new .cpp.

Checks 2026-09-28 macOS: build dev passed; focused app_state/folder_history/
bookmarks/ui_navigation/ui_state 5/5 passed after completed build; check.sh
format/build/54 CTest passed; diff check passed. Commit evidence in checklist.
Initial premature focused run missed unlinked executable; superseded by rerun.
No visual changes, geometry inspection not required. macOS only;
Windows/Linux runtime and rendered Windows parity unverified.
