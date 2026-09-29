# Ревизия происхождения кода 7zFM

Здесь хранится реестр происхождения кода: upstream-ревизии и исходники,
перенесённые символы, адаптации, пропущенные части и долг переноса.
Правила работы заданы в [AGENTS.md](../AGENTS.md); наблюдаемое поведение
и платформенные отличия — в [сверке с 7zFM](7zfm-parity.md).
Владельцы компонентов и интеграционные границы описаны в
[архитектуре](architecture.md). Эти документы ссылаются на реестр,
не дублируя сведения о происхождении и историю переноса.

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

## P0-01.4 — reconciliation и закрытие inventory (2026-09-28)

Project baseline `415513c`; upstream — [ip7z/7zip exact commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Таблицы P0-01.1–.3 ниже — актуальный исходный реестр; более ранние тематические
записи после него сохраняют историю частичных портов, не заменяют omissions.
Все 112 unique paths имеют owner, source scope/dependencies и decision:
30 mirrored modules reviewed against bodies, 82 missing UI modules require
source reuse with local presentation/platform/engine adapters. Inactive
comment-only/definition/build files identified individually. No mandatory
unported function was closed as done or N/A. Headers/resources belong to their
modules; dependency closure is re-reviewed and expanded by each functional owner.

| Область | Upstream UI .cpp | Mirrored UI | Missing UI |
| --- | ---: | ---: | ---: |
| FileManager | 67 | 22 | 45 |
| Agent | 7 | 2 | 5 |
| GUI | 10 | 2 | 8 |
| Common | 25 | 1 | 24 |
| Всего | 109 | 27 | 82 |

Три supporting mirrored paths — CPP/Common/Wildcard.cpp,
CPP/Windows/FileDir.cpp, CPP/7zip/Common/MethodProps.cpp. Coverage проверен
равенством sets всех 109 UI paths плюс всех 30 local src/CPP paths, без duplicate
rows/лишних source paths. Owner IDs относятся к 44 пакетам плана; slash shorthand
внутри группы не создаёт новых IDs. Требующие более поздней capability ветки
остаются у соответствующих owners, включая SFX legacy, VerCtrl, device copying,
email integration, checksum handler и alternate command entrypoints.

Сейчас необходимы: upstream string/vector/property/censor reuse, portable folder
contracts и narrow process/filesystem/platform services; snapshots у ArchiveBackend
не заменяют application bodies. Qt/std replacements в borrowed modules остаются
migration debt, названной в строках. Upstream module boundaries authoritative;
не создавать рядом вторую реализацию. P0-02 следующий по приоритету/зависимостям.

Проверки 2026-09-28 macOS: `git diff --check`; data-processing assertions для
112 unique owner rows, точного coverage и counts; повторная побайтовая сверка
32 vendored source/header + License.txt с `git show <full revision>:<path>`;
upstream HEAD exact baseline, status clean. Все passed. Docs-only, runtime/UX
unchanged, build/CTest/layout не требуются и не запускались. Windows/Linux и
rendered Windows visual parity не проверены. P0-01 done как inventory; 1/44
packages complete, 43 remaining. Это не source/workflow/final acceptance.

## P0-01.1 — реестр первых 16 зеркальных модулей (2026-09-28)

Проверенный project baseline: `a5923d4913f487b46a36c8637165e2a74aa85b1d`,
чистая рабочая копия. Для **каждой** строки оригинал — соответствующий путь
относительно корня [ip7z/7zip на полном commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Локальная реализация находится в `src/<путь>`. Ни одна строка не означает
полный перенос файла. Прочитаны локальные bodies/headers, соответствующие
оригинальные функциональные bodies и непосредственные зависимости.
Formatting отличается отдельно от функциональных изменений.

В таблицах FM = `CPP/7zip/UI/FileManager`, Agent = `CPP/7zip/UI/Agent`.
«Остальное» означает все bodies исходного файла за вычетом явно перечисленного
перенесённого scope; названные группы уточняют владельцев, а не разрешают
пропустить неназванную функцию. Headers входят в scope своих `.cpp`.

| Путь / owner оставшегося scope | Скопированная основа и адаптация | Пропущенные символы/ветки и непосредственные зависимости |
| --- | --- | --- |
| FM/App.cpp — P0-02, P0-08, P1-01 | `AddUniqueStringToHead`, `CFolderHistory::{Normalize,AddString}`, `AddSizeValue`, `AddValuePair1/2`, `AddPropValueToSum`, `CPanel::GetItemsInfoString`: сохранены циклы, sentinel неизвестного размера, лимиты 100/history и summary. QString/русские captions и snapshot заменяют Lang/IFolder/PROPVARIANT. Локальный `CPanel` constructor — glue. | Весь `CApp` и `CPanelCallbackImp`: Create/CreateOnePanel, SetListSettings, toolbar, Save/ReleaseApp, переключение/фокус панелей, OnCopy, OnSetSameFolder/OnSetSubFolder, title/notify. `App.h`, `AppState.h`, `Panel.h`, CopyDialog, LangUtils, RegistryUtils, Win32 controls/synchronization. `CAppState` пока отсутствует; удаление lock допустимо только при GUI-thread ownership. |
| FM/PanelFolderChange.cpp — P0-04 | `SetBookmark`, `OpenBookmark`, `FoldersHistory`: исходные чтение/запись slots и обратное восстановление списка после OK; явные state/settingsPath, Qt dialog и возврат выбранного пути вместо BindToPathAndRefresh. | ReleaseFolder, SetNewFolder, SetToRootFolder, BindToPath/BindToPathAndRefresh, GetFolderPath, LoadFullPath/LoadFullPathAndShow, combo events, GetParentDirPrefix/OpenParentFolder, CloseOneLevel/CloseOpenFolders, OpenRootFolder/OpenDrivesFolder/OpenFolder/OpenAltStreams. Dependencies: RootFolder, FSFolder, FileFolderPluginOpen, OpenCallback, CFolderLink/parent stack, selected state, combo/icon services. Навигация MainWindow — ещё самостоятельная реализация. |
| FM/PanelItems.cpp — P0-06, P0-07, P1-08 | Только `GetItem_UInt64Prop` и хвост `GetItemSize`: parent sentinel и throw при ошибке сохранены; switch четырёх snapshot properties заменяет generic variant. `IsItem_Folder/GetItemRelPath` в локальном Panel.h — snapshot glue. | Fast-path IFolderGetItemName, все прочие свойства; InitColumns и helpers, Add/DeleteColumn, RefreshListCtrl variants, SaveSelectedState/GetSelectedNames, Get_ItemIndices_* (Selected/Operated/All/OperSmart), open/edit, item name/prefix/path access, SaveListViewInfo. Dependencies: IFolder, PropVariantConv, PropID, PropertyName, ExtractingFilePath, Sort, ViewSettings, RootFolder. |
| FM/PanelMenu.cpp — P1-10, P0-09, P2-09 | `AddListAscii/AddSeparator/AddPropertyPair`, ограниченный `AddPropertyString`, single-item enumeration и multi-item totals из `Properties`. Snapshot заменяет COM, числа через IntToString, размер через ConvertSizeToString. | `Properties`: raw/security/hex, folder properties, archive levels/errors и native filesystem properties; остальное — InvokeSystemCommand, EditCut/Copy/Paste, shell context menu, CreateSystem/File/SevenZipMenu, IsThereReadOnlyFolder/CheckBeforeUpdate, InvokePluginCommand/OnContextMenu. Dependencies: ListViewDialog, PropertyName/FormatUtils, PropVariantConv, IFolderArcProps, Windows Menu/COM и Explorer integration. |
| FM/PanelKey.cpp — P0-03, P0-07, P0-08 | `MainWindow::handlePanelKey` — Qt adapter выбранных условий `OnKeyDown` и combo edit: Backspace, Shift+F4, bookmarks Alt+digits, Enter, Alt+F1/F2. Это **не** полный source port OnKeyDown: dispatch через QAction/handleSelectionKey принадлежит MainWindow. | Оригинальные OnKeyDown/FindVKeyPropIDPair и таблица сортировки не скопированы целиком. Остались RightCtrl bookmarks, Tab/F9 panel callbacks, Ctrl+F3–F7, Copy/Move same-panel modifiers, clipboard, alternative selection, same/subfolder arrows, другие command branches. Dependencies: CPanel/CApp callbacks, HelpUtils, PropID, list selection/focus. |
| FM/PanelSelect.cpp — P0-07 | `CPanelSelection::{SelectSpec,SelectByType,SelectAll,InvertSelection}` сохраняют loops и folder/no-extension/extension ветви исходного CPanel. QString/std::vector и Items snapshot заменили upstream strings/status vector; dialog и UpdateSelection вынесены в Qt callers. | OnShiftSelectMessage/OnArrowWithShift, OnInsert, UpdateSelection, KillSelection, OnLeftClick и alternative-selection state. OnUpWithShift/OnDownWithShift в baseline закомментированы, не обязательные активные bodies. Dependencies: Wildcard, ComboDialog, LangUtils, CPanel real-index mapping и list redraw. |
| FM/PanelSort.cpp — P0-06 | `CompareFileNames_ForFolderList` pointer loop скопирован; char16_t сохраняет Windows UTF-16, QChar upper — platform adapter. `GetExtensionPtr` адаптирован. Qt `CompareItems` сохраняет folders-first/name/prefix/index tie-break intent, но заменяет оригинальный property dispatch: это частичная адаптация с долгом. | CompareFileNames_Le16, SetSortRawStatus, CompareItems2 raw/reparse/IFolderCompare/variant ветви, generic трёхпроходный CompareItems, SortItemsWithPropID/OnColumnClick. Закомментированный SortItems не активная ветвь. Dependencies: CpuArch, PropVariant, PropID, FileLink/reparse, CPanel columns/direction. |
| FM/ViewSettings.cpp — P0-06, P0-08, P0-09 | Save/ReadFolderHistory, Save/ReadFastFolders, Save/ReadCopyHistory, Save/ReadStringList и AddUniqueStringToHeadOfList: исходные keys и порядок. QSettings INI заменяет Registry; UStringVector сохранён для history/favorites, QStringList для copy history. | CListViewInfo::Save/Read, CWindowInfo::Save/Read, QueryBuf, Save/ReadToolbarsMask, CListMode::Save/Read, Save/ReadPanelPath и integer helpers. Dependencies: MyString/MyTypes, Registry, Synchronization, binary column/window records. Собственные настройки MainWindow не подтверждают перенос этих bodies. |
| FM/FSFolder.cpp — P0-05, P1-03, P1-10, P2-05/06 | GetAbsPath, CreateFolder/CreateFile/Rename/Delete сохранены из mutation methods; Qt filesystem, bool/ErrorMessage и callbacks заменяют services/HRESULT/IProgress. GetRelPath адаптирует исходный inline helper; constructor/AddPath — snapshot glue. Guards indices, selected-link rejection и recursive cancellation — местная политика. | Init, CFsFolderStat::Enumerate, LoadSubItems/LoadItems, comments, item/property/raw/name/size/prefix getters, ReadFileInfo/ReadChangeTime, CompareItems, BindToFolder*/BindToParentFolder, WasChanged/Clone, Calc/GetItemFullSize, SetProperty, GetSystemIconIndex, SetFlatMode/SetShowNtfsStreamsMode. Dependencies: FileFind/FileIO/FileName/FileDir, IFolder/IArchive, TextPairs, PropVariant, metadata services. Enumeration в Qt MainWindow ещё migration debt. |
| FM/FSFolderCopy.cpp — P1-02, P0-11, P2-06 | `CCopyStateIO::MyCopyFile` 64KiB read/write/error/progress loop; CallProgress, IsDestChild, CombinePath; CopyFile_Ask только read/write branch, CopyFolder только recursive copy skeleton. Qt NewOnly/staging, flush/time, unreadable/link guards вместо native services. | AskWrite/skip/rename/overwrite, MoveMode и source delete orchestration, native CopyFile_NT/CopyFile_Sys/MoveFile_Sys и CopyProgressRoutine, CFSFolder::CopyTo/CopyFrom, totals/CFsFolderStat, ADS/direct destination, error callbacks. Dependencies: FilePathAutoRename, IFolderOperationsExtractCallback, FileDir/Find/IO/DLL/ErrorMsg. Transaction/rollback owner сейчас filesystemtransfer.cpp; это integration, не перенос omitted bodies. |
| FM/PanelSplitFile.cpp — P1-06, P0-10 | CVolSeqName::{SetNumDigits,ParseName,GetNextName}, CThreadSplit/Combine::ProcessVirt loops; AddInfoFileName и summary/output-name blocks из CApp::Combine представлены GetInfo/GetCombinedName. QFile, prevalidated stage paths, bool/error и callback вместо NIO/HRESULT/CProgressSync; flush/source-change guards добавлены. | CApp::Split/Combine как owners selection/destination/dialog/progress/refresh не перенесены целиком. CPreAllocOutFile/preallocation и progress file-count/path/title не перенесены. Dependencies: App/Panel, Copy/SplitDialog, SplitUtils, Format/LangUtils, NIO, ProgressDialog2. Qt worker/transaction остаются adapters; full caller review в P1-06. |
| FM/SplitUtils.cpp — P1-06 | Все три активные функции ParseVolumeSizes/AddVolumeItems/GetNumberOfVolumes и k_Sizes перенесены: parse/overflow/units/comment и repeat-last-size control flow сохранены. std::wstring/std::vector вместо UString/CRecordVector; QComboBox вместо Win32 combo. | Активных функций не пропущено; container replacement требует миграции или обоснования. Dependencies: StringToInt и MyTypes (vendored), MyCharLower_Ascii заменён локальным ASCII expression. Это полный functional scope данного `.cpp`, не завершение volume workflow. |
| Agent/Agent.cpp — P1-10, P0-04, P1-04, P2-06 | Только CAgentFolder::{LoadFolder,LoadItems,GetNumberOfItems} ветви CProxyArc; GetItem и constructor — glue real-index mapping. std::vector вместо upstream vector. | _proxy2/alt-stream traversal и archive-open guard; весь CAgent lifecycle/codecs/open/reopen/close/read/extract/update capability; CAgentFolder name/prefix/size/property/raw/sort/bind/stream/real-indices/extract и property enumeration. Dependencies: MyCom, PropVariant, LoadCodecs/OpenArchive/UpdateAction, IFolder/IFolderArchive, AgentProxy. COM transport заменяется backend, bodies от этого не становятся N/A. |
| Agent/AgentProxy.cpp — P1-10, P2-06, P2-10 | CProxyArc::FindSubDir overloads, MergeSubDirs/AddDir, AddRealIndices/GetRealIndex, GetDirPath_as_Prefix* и CalculateSizes traversal; Load tree/long-path skeleton. QString/std::vector/std::deque и ArchiveEntry заменяют owned names/vectors/IInArchive. FindDir — local navigation helper. | CalculateSizes CRC/definedness; Load raw/default-path/errors/progress/memory limit, AllocStringAndCopy/FreeFiles, GetDirPathParts_isChanged, GetRealIndices_Unsorted; все CProxyArc2/CProxyDir2/SetDirPrefix_as_LONG_PATH. Dependencies: OpenArchive/CArc, IArchive/raw props, ItemName, PropVariantConv, Wildcard::CompareFileNames. Qt case comparison заменяет upstream comparator и требует review; container substitutions — debt. |
| CPP/Common/Wildcard.cpp — P0-07, P0-12, P2-07 | EnhancedMaskTest/DoesWildcardMatchName pointer body и SplitPathToParts_Smart; char16_t/Qt wrapper, QChar upper, OS separator adapter. g_CaseSensitive принудительно false для Windows-style selection, в upstream глобальная platform policy. | IsPath1PrefixedByPath2, CompareFileNames/path comparators, SplitPathToParts, wildcard detection, NCensor::CItem/CCensorNode/CCensor (add/check/include/exclude/prefix/drive handling). Dependencies: MyString/MyVector, StringConvert, platform path macros. Маска выбора не заменяет filesystem censor; не вводить второй authoritative upstream wildcard module. |
| CPP/Windows/FileDir.cpp — P1-03, P1-02, P1-12, P2-05/07 | DeleteFileAlways и RemoveDirWithSubItems из Windows ветви: read-only, reparse guard, recursion и error aggregation skeleton. QFile/QDir services, callbacks cancel, no-follow directory-link removal; POSIX policy local. | Остальное: directory/current/full/temp path, CreateComplexDir, move/progress/hardlink, time/attribute/ownership и CTempFile/CTempDir, Win32 long-path и upstream POSIX implementations. Dependencies: FileFind/FileIO/FileName/TimeUtils, StringConvert, ErrorMsg, native platform APIs. В upstream уже есть POSIX bodies — сначала проверить reuse, не переписывать их автоматически в Qt. |

### Типы, границы и следующие prerequisites

`MyString`, `MyVector`, `MyTypes`, `StringToInt`, `IntToString` уже доступны из
`third_party/7zip`; их полный byte/provenance review относится к P0-01.2.
Использование QString в control/service boundary необходимо для Qt. Замена
внутренних upstream vectors/strings в Agent, selection и SplitUtils не доказана
как необходимость: это migration debt соответствующего owner, не новый стандарт.
`std::deque` сейчас сохраняет ссылки на CProxyDir при AddDir; при возврате
upstream containers надо сохранить исходную reference stability и control flow.
Property snapshots нужны на границе `7zz`, но их ограниченные поля не заменяют
generic property pipeline. `IFolder`/variant/COM зависимости надо разложить на
portable application contracts и transport adapters; Win32 controls заменить Qt.
Не копировать engine в widgets и не превращать engine-facing ветки в N/A.

Текущая файловая инвентаризация подтверждает исходные 30 mirrored `.cpp`
(27 UI + 3 supporting), отсутствующих UI — 82. FM/BrowseDialog.cpp и
FM/PanelListNotify.cpp входят в оставшиеся 14 файлов P0-01.2; их имена
уточнены в чеклисте, без изменения знаменателя. Окончательная сверка
ownership — P0-01.4. P0-01.1 фиксирует ровно 16 уникальных путей таблицы выше.

Проверки: 2026-09-28, macOS, docs-only, `git diff --check` и сверка наличия
16 путей в обеих рабочих копиях; upstream HEAD совпадает с baseline, upstream
рабочее дерево чистое. Build/CTest/layout не запускались: code/UX не менялись.
Runtime Windows/Linux и pixel parity этим аудитом не подтверждены.

## P0-01.2 — остальные 14 модулей и portable dependencies (2026-09-28)

Project baseline: `cfd02a5` после P0-01.1, чистая рабочая копия.
Для всех путей ниже exact source — [ip7z/7zip,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
локально `src/<путь>`. FM/GUI/Common здесь означают соответствующие
`CPP/7zip/UI/` каталоги. Scope omissions: все bodies кроме явно перечисленных
перенесённых portions; имена ниже уточняют обязательные оставшиеся ветви.

| Путь / owner оставшегося scope | Copied/adapted symbols | Omitted scope, dependencies и причины |
| --- | --- | --- |
| FM/BrowseDialog.cpp — P1-11 | Только `Dlg_CreateFolder`: clear → CComboDialog → Cancel/OK → raw Value. Qt HWND/dialog result и русские Lang captions. | Весь CBrowseDialog (Init/Reload/parent/path/events/sort/create/enter/FinishOnOK), CBrowseInfo::BrowseForFile, MyBrowseForFolder и CorrectFsPath/path cleanup. Dependencies: FileFind/Dir/Name, RootFolder/drives, ComboDialog, native browse wrappers/Lang, filters. Native QFileDialog сейчас заменяет presentation, но не доказывает перенос validation/navigation bodies. |
| FM/PanelListNotify.cpp — P0-06, P0-07, P0-09 | Оба `ConvertSizeToString` и INT_TO_STR_SPEC: исходный integer/grouping алгоритм, внешний результат QString вместо UString. | IsSizeProp, SetItemText (все generic/raw/metadata formatting), OnItemChanged/OnNotifyActivateItems/OnNotifyList, OnCustomDraw, Refresh_StatusBar; GetCpuTicks diagnostic branch не включён. Dependencies: Panel, PropVariantConv, FormatUtils/PropertyName, FileLink/security, list notifications. Основной panel hover и status pipeline ещё не портированы. |
| FM/ComboDialog.cpp — P1-03, P0-07, P1-14 | OnInit/OnOK функциональная последовательность; raw Value и Strings, Qt layout/focus/selected edit заменяют controls/NormalizeSize/OnSize. Constructor/accept/value — glue. | LangSetDlgItems отсутствует (P1-14); Win32 OnSize заменён layout, не отдельный algorithm debt. Sorted code в оригинале закомментирован. Dependencies: ComboDialog.h/.rc, Control/Static, LangUtils, upstream strings заменены QStringList только на dialog boundary. |
| FM/CopyDialog.cpp — P1-01, P1-14 | OnInit, OnButtonSetPath, OnOK и browse-button dispatch: history/value/info, normalize separator и reset selection. Qt signals/layout/native browse заменяют Win32 OnButtonClicked/OnSize. | Lang, UNDER_CE initialization workaround не перенесены; CE branch отдельно не доказывает неприменимость остальных callers. Local sources-summary constructor — glue, основной summary уже в App.cpp. Dependencies: CopyDialog.h/.rc, BrowseDialog::MyBrowseForFolder, FileName::NormalizeDirPathPrefix, LangUtils. Full routing/overwrite не принадлежит dialog и остаётся P1-01/P0-11. |
| FM/SplitDialog.cpp — P1-06, P1-14 | OnInit/OnButtonSetPath/OnOK: filename title, first preset, path, trim/parse/empty check; Qt signals/layout/native browse и inline error вместо Win32 message. Добавлен signed-qint64 bound. | Lang; original resize/control wrappers заменены presentation. Dependencies: SplitDialog.h/.rc, SplitUtils, BrowseDialog, FileName, CopyDialogRes, resourceGui. UString/CRecordVector result сохранён частично (VolumeSizes), QString/std conversion parser — debt SplitUtils, не оправдание новой parser реализации. |
| FM/PasswordDialog.cpp — P0-12, P1-14 | Все functional handlers ReadControls/SetTextSpec/OnInit/OnButtonClicked/OnOK, включая read-before-echo-toggle. QString и Qt controls/result вместо Win32/Lang. | Lang отсутствует; generic CDialog fallback заменён signals. Dependencies: PasswordDialog.h/.rc, LangUtils, edit/control wrappers. Read password dialog не закрывает callback retry/Ask/encrypted mutation scopes. |
| FM/ListViewDialog.cpp — P1-10, P1-14 | ListView_GetSelected, OnInit, CopyToClipboard/ShowItemInfo/DeleteItems/OnEnter/OnOK, активные keyboard/activation branches OnNotify. UString/CUIntVector сохранены; qtlistview, Qt clipboard/signals/filter/layout заменяют services/OnSize. | Lang; version-specific g_LVN_ITEMACTIVATE_Support заменён Qt click policy, Cmd keys/modifier guards и 400ms hover — local presentation. Dependencies: ListViewDialog.h/.rc, EditDialog, RegistryUtils/CFmSettings, CListView adapter. Generic/raw property provider относится к P1-10, не к этому списку. |
| FM/EditDialog.cpp — P1-10, P1-14 | OnInit Title/Text; layout вместо OnSize. Resource styles read-only/multiline/no-wrap/scroll/Enter сохранены через Qt. eventFilter — ES_WANTRETURN adapter. | Lang отсутствует. Dependencies: EditDialog.h/.rc, GuiCommon.rc, ViewSettings ToQString. Это detail dialog; внешний editor/viewer из P1-08 — иной workflow. |
| FM/RegistryUtils.cpp — P0-07, P0-08, P1-13/14, P2-07 | CFmSettings::Load/Save, 7 keys/default false и порядок; SaveOption/ReadOption services адаптированы QSettings bool/UInt32 preserving invalid default. FmSettingsPath — local config-location glue. | Save/ReadRegLang, editor/viewer/diff/version-control, Save/ReadLockMemoryEnable, Save/ReadFlatView, Save/Read_ShowDeleted, HKLM helpers и прочие registry functions. Dependencies: Registry, StringConvert, MyString, synchronization/services; persistence adapter необходим между ОС, portable fields/logic сохранять. |
| FM/SettingsPage.cpp — P0-07, P1-12, P2-07 | Части OnInit/OnApply/OnButtonClicked для ShowRealFileIcons/ShowGrid/SingleClick, init/changed flags. Qt dialog вместо property page; snapshot omitted flags сохраняется при OK. | ShowDots/FullRow/AlternativeSelection/ShowSystemMenu, large-pages capability/privilege, extraction limit spin/range/validation/save, EnableSpin, OnCommand, OnNotifyHelp и Options Apply container. Dependencies: SettingsPage.h, SettingsPage.rc/SettingsPage2.rc, RegistryUtils, ZipRegistry, MemoryLock/System/ErrorMsg/StringToInt/HelpUtils. Controls не считать готовыми только потому, что есть CFmSettings поля. |
| GUI/CompressDialog.cpp — P1-05, P1-07, P2-01/02 | Static format/method tables; GetStaticFormatIndex adapted by format name. Numeric portions SetDictionary2/SetOrder2/GetOrderMode/SetSolidBlockSize2/SetNumThreads2; Get_Lzma2_ChunkSize, GetMemoryUsage_Threads_Dict_DecompMem, Add_Size/AddMemUsage/AddMemSize/AddMemComboItem/SetMemUseCombo/Get_MemUse_Bytes. Path portions GetExtDotPos/SetArchiveName*/ArcPath_WasChanged/SetArcPathFields/GetFinalPath_Smart; FindRegistryFormat/SetEncryptionMethod/GetEncryptionMethodSpec. IsAsciiString and password block OnOK extracted as CheckPassword; history/encrypt persistence and solid conversion blocks OnOK extracted separately. | Full CCompressDialog::OnInit/OnOK/OnCommand/FormatChanged/SetMethods/SetLevel2/SetMethod2, registry selection within numeric setters, SaveOptionsInMem, GetMethodSpec*, SFX/KeepName/hash filename, volume/shared/delete/update/path modes, memory-blocking prompt, OnHelp, parameters; **all COptionsDialog** time/links/security/precision handlers. Dependencies: CArcInfoEx/LoadCodecs, ZipRegistry, MethodProps, System affinity/RAM, SplitUtils, BrowseDialog/FileName, Lang/Help, CompressDialog.rc/CompressOptionsDialog.rc. Qt preferences widgets live in compressiondialog*.cpp; pure functions moved out of original class are existing structure debt, not a model for new ports. Commented ZSTD portions in baseline are not active missing branches; external/user codecs remain mandatory capability review. |
| GUI/ExtractDialog.cpp — P0-12, P2-07, P1-14/15 | AddComboItems/AddUniqueString; parts OnInit/OnOK for modes, force flags, password visibility, 16-entry history, SplitDest, trim/normalization; UpdatePasswordControl/OnButtonSetPath and signals replacing OnButtonClicked. QString/Qt layout/native browse/settings path services. | Absolute-path choice hidden, Ask transport absent (currently maps safe no-overwrite), GetBoolsVal/CheckButton_TwoBools/GetButton_Bools and ElimDup/NtSecurity, full initial Password/ArcPath title pipeline, OnHelp; SFX/no-registry conditional workflows not ported. Dependencies: ExtractDialog.h/.rc, ExtractMode, ZipRegistry, Wildcard::SplitPathToParts_Smart, FileName/Dir, Browse/Lang/Help. `options/destination/settingsPath` — integration helpers, not upstream bodies. |
| Common/ZipRegistry.cpp — P1-07/12, P2-07/09 | ParseMemUse/CMemUse::Parse and inline GetBytes/Clear; partial NCompression::CInfo::Save/Load and NExtract::CInfo::Save/Load. Original key names, numeric absent sentinel, force flags and format iteration retained; QSettings replaces Registry/lock, Qt strings at persistence boundary. | Compression Level/global bool pairs/Options/time precision/MAC/link/security fields; Extract ElimDup/Security and Read/Save_ShowPassword/Read/Save_LimitGB; NWorkDir::CInfo::Save/Load; CContextMenuInfo::Save/Load; omitted Key_*bool pair helpers. MemLimit_Save/Load are commented in baseline, not active missing symbols. Dependencies: Registry/Synchronization, ExtractMode, MethodProps, StringToInt. Legacy history/extract fallback is local migration; QSettings bool conversion differs from strict registry reads and needs review when extending fields. |
| CPP/7zip/Common/MethodProps.cpp — P1-07, P2-01 | Only Calc_From_Val_Percents: body copied including saturation/overflow; MyTypes interface. | StringToBool, parameter/dict/log-size parsing, CProps/CCoderProps/CMethodProps and coder property conversions/memory helpers, property-name lookup; dependencies: StringToInt/MyString/MyVector, PropVariant, ICoder/PropID. Process engine owns compression; advanced application property validation still requires source review. |

### Vendored closure и UI references

На 2026-09-28 проверены **33 файла** (32 source/header + License.txt): каждый
побайтово совпал с `git show <full-baseline>:<path>`, License.txt — с
`DOC/License.txt`. README.md — локальная документация, не copied source.
Из них sevenzip_common собирает StringToInt, MyString, IntToString, MyWindows,
StringConvert, UTFConvert и Windows/System; headers/defines входят через
`third_party/7zip/CPP`. PropID/ExtractMode, CpuArch/MyBuffer и portable type
closure не следует считать отдельными завершёнными UX-функциями.
README vendor inventory обновлён до полного фактического набора; исходные notices
и license не изменены. При будущем расширении closure проверять optional/platform
includes и callers, не заменять portable bodies из-за Win32 имени каталога.

Прочитаны reference `.rc` для Combo/Copy/Split/Password/ListView/Edit,
SettingsPage2, Extract/Compress и include CompressOptionsDialog; соответствующие
Qt handlers сверены с функциональным source. Это source/resource audit,
без visual change: geometry/JSON/screenshot и runtime checks не повторялись.
Русские captions вместо Lang, native Browse, inline Split errors и scrolling Copy
summary — существующие adapters, указанные в parity. Отсутствие controls
Options/Help/absolute/security и functional callers остаётся обязательной работой.
Windows rendered reference не использован; pixel parity не подтверждена.

Checks: 2026-09-28 macOS docs-only — byte comparison всех 33 vendored файлов,
unique-path coverage 16 + 14 = 30 и `git diff --check`; build/CTest не нужны.
P0-01 parent остаётся partial до absent-module review и полной coverage сверки.

## P0-01.3a — отсутствующие FM owners и callbacks (2026-09-28)

Project baseline: `d03de90`. Для всех 12 путей ниже FM =
`CPP/7zip/UI/FileManager`; exact source — [ip7z/7zip на
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Эти `.cpp` отсутствуют в `src/`: copied scope пуст, **все активные bodies
остаются открытыми**. Проверены исходные bodies, includes, headers и callers;
таблица определяет перенос/адаптацию, а не заявляет завершённый workflow.
Названные symbols — опорные точки review; неназванные bodies не исключены.

| Путь / owner | Обязательный функциональный scope | Dependencies, решение и необходимые adapters |
| --- | --- | --- |
| FM/FM.cpp — P0-02/03/08/09, P1-14, P3-04 | Startup/command-line/start path, InitInstance/window lifecycle, ExecuteCommand, SaveWindowInfo, splitter ratio/clamp и CApp::MoveSubWindows. | App/AppState/Panel, MyLoadMenu, ViewSettings, Lang/Registry/StringUtils, Windows security/system/alloc. Перенести application flow; HWND/message loop/accelerators/DPI/window metrics заменить Qt и platform services. DLL security/privilege branches проверять по ОС отдельно; Win32 entrypoint не основание исключить startup и persistence. |
| FM/Panel.cpp — P0-02/03/04/05/08/09, P1-05/06, P2-03/06 | CPanel Create/ReleasePanel/OnCreate/OnDestroy, list/combo messages, focus, folder-type/path queries, view/flat/streams modes; AddToArchive, ExtractArchives, CThreadTest::ProcessVirt/TestArchives и GetFilePaths. | App/Panel/RootFolder/FSFolder, IFolderArchive, ArchiveName/CompressCall, ExtractCallback, PropVariant, resources. Сохранить state и operation orchestration; notifications/controls заменяются Qt, IFolder transport и engine calls — ArchiveBackend adapters. Наличие самостоятельных MainWindow actions не переносит эти bodies. |
| FM/MyLoadMenu.cpp — P0-03/09, P1-13/14/15, P2-01/03/04/09 | MyLoadMenu/translation, CFileMenu::Load, OnMenuActivating availability/check state, ExecuteFileCommand и OnMenuCommand: view/sort/select/bookmarks/panels/operations/tools/help routing. | App/Panel, RegistryUtils/LangUtils/HelpUtils, BrowseDialog2/AboutDialog, CompressCall, PropID, resource/menu IDs. Перенести исходные branch/command tables в owner, QAction/QMenu заменить HMENU services; один dispatch и availability для keys/toolbar/menu. Shell-only entries требуют platform capability, не исключения всего меню. |
| FM/OpenCallback.cpp — P0-04/10/12, P1-09, P2-02 | COpenArchiveCallback CheckBreak/SetTotal/SetCompleted, password request/cache и открытие progress; отмена и PasswordWasAsked state. | ArchiveOpenCallback/IOpenCallbackUI, PasswordDialog, ProgressDialog2; OpenCallback.h выбирает ProgressDialog при Z7_SFX. Сохранить callback state/return semantics; BSTR/window/thread boundary адаптировать через backend и GUI-thread requests. Process API не оправдывает потерю password/cancel paths. |
| FM/ProgressDialog2.cpp — P0-10, P2-10 | CProgressSync totals/status/ratio/messages/stop/pause; CProgressDialog stats/range, elapsed/remaining/speed, pause/background priority, cancel-confirm с восстановлением pause и completion race, Create/OnExternalCloseMessage/ProcessWasFinished; CProgressThreadVirt lifecycle/exception/final errors, HResultToMessage. | ProgressDialog2.h/.rc/Res, MessagesDialog/ListView, Lang/IntToString, Thread/Synchronization/ErrorMsg, taskbar COM. Copy/update/test/extract callers используют этот модуль. Скопировать sync/lifecycle/statistical bodies; Qt UI/queued completion и process control — narrow adapters. Pause/priority нуждаются в реальном backend/platform support; fake enabled controls недопустимы. |
| FM/ProgressDialog.cpp — P2-02, P3-05 | Legacy CProgressSync/CProgressDialog totals, pause/stop, timer, Create/thread-close и cancellation flow. | ProgressDialog.h/.rc, Thread/Synchronization, Lang; build evidence: SFXSetup/makefile собирает ProgressDialog.obj, ExtractCallbackSfx.h включает его; FM/FM.mak, GUI/makefile и SFXWin/makefile выбирают ProgressDialog2.obj. Решение отложено до SFX entrypoint review: сохранить применимые bodies либо доказать использование официального standalone runtime. **Не N/A** по одному отсутствию у нас. |
| FM/MessagesDialog.cpp — P0-10, P1-14 | CMessagesDialog OnInit/OnSize: multiline messages → list, numbering/columns/widths; dialog completion. | MessagesDialog.h/.rc, LangUtils, Control/ListView, resourceGui. Qt presentation adapter, общий provider сообщений/ownership из ProgressDialog2. Существующая выдача одной строки ошибки не заменяет список сообщений. |
| FM/OverwriteDialog.cpp — P0-11/12, P1-01, P1-14 | COverwriteDialog OnInit/OnDestroy/OnButtonClicked, ReduceString/SetFileInfoControl/SetItemIcon, file name/size/time/icon presentation, Yes/No/YesToAll/NoToAll/AutoRename/Cancel. | OverwriteDialog.h/.rc/Res, FileFind/PropVariantConv/ResourceString, Lang/IntToString и native icon services. Перенести решения и comparison preparation; Qt controls/icons/metrics адаптировать, полный result contract передать FS и extract callbacks. |
| FM/PanelCopy.cpp — P1-01/04/09/16, P2-03/06 | CPanelCopyThread::ProcessVirt dispatch FS copy / archive extract/test / CopyTo, ShowFinalResults, CPanel::CopyTo/CopyFrom/CopyFromNoAsk, zone stream inheritance, password/progress/messages и selection/focus refresh. | FSFolder/IFolderOperations, ExtractCallback/UpdateCallback100, HashGUI, ZipRegistry, parent CFolderLink, Thread. Сохранить orchestration и branch order; portable folder operation interface и backend/process adapter вместо COM/threads, Win32 focus/notify guards — Qt owner. Hash folder Move E_NOTIMPL и error/cancel distinction сохранить. |
| FM/PanelOperations.cpp — P1-03/04, P0-10/11 | CThreadFolderOperations ProcessVirt/DoOperation, DeleteItems internal/recycle paths, begin/end inline rename, CorrectFsPath, CreateFolder/CreateFile/RenameFile/ChangeComment, selected state restoration. | Panel/FSFolder, UpdateCallback100, ComboDialog/Dlg_CreateFolder, PropVariant/FileName/Wildcard, shell recycle API. Перенести validation/update guards/refresh bodies; Qt editing/confirmation/recycle services заменить Win32, mutations остаются provider/backend. Create refresh even on error и comment capability/error ветви обязательны. |
| FM/UpdateCallback100.cpp — P0-10/12, P1-04/05/09 | CUpdateCallback100Imp Init, totals/completed/ratio, messages/operation result/error counts, CryptoGetTextPassword2 with AskPassword/defined state. | UpdateCallback100.h, IFolderArchive/IProgress/ICrypto, ProgressDialog2, PasswordDialog/Lang. Сохранить callback semantics; HRESULT/BSTR/process progress/password prompt — adapters. Это не engine implementation и не N/A из-за 7zz integration. |
| FM/ExtractCallback.cpp — P0-10/11/12, P1-08/09, P2-06/07/10 | CExtractCallbackImp open/extract progress/results/error flags, AskOverwrite/AskWrite conflict modes и auto-rename, password, stream/operation callbacks и RequestMemoryUse; CVirtFileSystem Write/FlushToDisk memory→disk, metadata/ADS/zone restore. | ExtractCallback.h, ProgressDialog2/Overwrite/Password/Mem dialogs, ArchiveOpenCallback/OpenArchive, ExtractingFilePath/FilePathAutoRename/StreamUtils, FileFind/Dir/IO/PropVariant, ZipRegistry/PropertyName/Lang. Сохранить policy/return branches; engine stream transport через backend, filesystem/memory/GUI services узкими adapters. Different directory/file conflict, remembered memory choice, unknown error bits и encrypted-header diagnostics остаются обязательными. |

Проверки: 2026-09-28 macOS, docs-only, `git diff --check`; 12 уникальных
путей существуют в exact upstream и отсутствуют в local mirrored tree;
суммарный реестр покрывает 42 уникальных модуля (30 mirrored + 12 missing).
Runtime/build/layout не менялись и не проверялись. Platform-specific решения
и полный dependency closure остаются открытыми у owners и в P0-01.4/P3-05.

## P0-01.3b1 — отсутствующие FM Options, Language и service dialogs (2026-09-28)

Project baseline `9eced86`; exact upstream для каждой строки —
[ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
FM = `CPP/7zip/UI/FileManager`. Все 12 `.cpp` отсутствуют локально:
copied/adapted scope пуст, все активные bodies обязательны до переноса или
доказанного platform/entrypoint решения. Проверены bodies, соответствующие
dialog headers/resources (включая Edit/Folders/MenuPage2.rc) и callers.

| Путь / owner | Исходные symbols и обязательный scope | Dependencies и решение адаптации |
| --- | --- | --- |
| FM/OptionsDialog.cpp — P1-12/13/14, P2-08/09 | OptionsDialog: шесть страниц System/Menu/Folders/Edit/Settings/Language в исходном порядке; property-sheet result, reload menus/toolbars/lang items/list settings и refresh всех panels после Apply. | App, MyLoadMenu, все page headers/resources, PropertyPage/DialogSize. Перенести container/control flow; Qt tabs/Apply/Cancel presentation и settings transaction вместо Win32 property sheet. Одиночный текущий Settings dialog не покрывает контейнер. |
| FM/EditPage.cpp — P1-08/13 | CEditPage OnInit/OnApply/OnCommand/OnButtonClicked, Edit_BrowseForFile: три viewer/editor/diff команды, changed flags, SplitCmdLineSmart, executable browse. | RegistryUtils, BrowseDialog, PanelItemOpen::SplitCmdLineSmart, Help/Lang, EditPage2.rc. Сохранить initialization и save-only-changed flow; executable filter/platform command launch адаптировать для ОС, без запуска из widget parser. Закомментированное восстановление старых parameters после Browse не активный scope. |
| FM/FoldersPage.cpp — P1-12 | CFoldersPage OnInit/GetWorkMode/MyEnableControls/GetWorkDir/ModifiedEvent/OnApply и browse: system/current/specified temp path, removable-only, dirty/init state. | NWorkDir::CInfo/ZipRegistry, BrowseDialog, Help/Lang, FoldersPage2.rc. Перенести decision/persistence flow; Qt radio/path controls и portable removable-volume service. WasChanged закомментирован, не активная обязательная функция. |
| FM/LangPage.cpp — P1-14 | CLangPage OnInit: enumerate/load/sort языки, default marks, compare sorted ID lists с английским baseline, missing/extra lines; OnApply/OnCommand/ShowLangInfo, AddVectorToString limits. | Common/Lang CLang, FileFind/ResourceString, LangUtils/RegistryUtils, LangPage.rc. Перенести loops/types и choice state; Qt combo/text, file enumeration и resources adapters. Информация о неполном переводе и bad-file errors не исключаются как presentation-only. |
| FM/LangUtils.cpp — P1-14 | LangOpen/GetLangDirPrefix/LoadLangOneTime/ReloadLang/OpenDefaultLang, kLangPairs/kLangs/FindShortNames, Lang_GetShortNames_for_DefaultLang, LangString overloads, AddLangString, dialog/window caption/colon helpers. | Common/Lang, DLL module dir, Synchronization, Window/ResourceString, RegistryUtils. Portable parser/retrieval/fallback bodies сохранить; Qt control/resource ID map и OS locale adapter вместо LANGID/Win32. Registry user override/path-separator guard и English '-' sentinel сохранить. Global mutable store требует явного ownership/thread contract. |
| FM/AboutDialog.cpp — P1-15, P2-01, P3-04 | CAboutDialog OnInit/OnHelp/OnButtonClicked: codecs load errors, version/CPU/date, homepage/help. | LoadCodecs/CCodecs, MyVersion/CpuArch, Lang/Help, AboutDialog.rc/logo/copyright. Перенести initialization; Qt dialog и URL open service, отдельно показывать version проекта/engine без неверного upstream attribution. External-codecs error branch остаётся с owner capabilities. |
| FM/HelpUtils.cpp — P1-15, P3-04 | ShowHelpWindow topic routing: module-relative 7-zip.chm, HtmlHelp либо hh.exe process; UNDER_CE/MinGW branch пуст. | DLL/StringConvert, HtmlHelp или ProcessUtils/FileDir/FileName; callers из всех Options pages, Compress/Extract/Benchmark/Browse/About/menu. Qt/OS help service нужен на macOS/Linux с теми же topic destinations и shipped help resources. Пустая upstream MinGW ветка не разрешает оставить F1 без результата в продукте. |
| FM/LinkDialog.cpp — P2-05, P0-08 | CLinkDialog init/current target/type, GetSymLink reproduce/validate reparse data, OnButton_Link hard/sym-file/sym-dir/junction/WSL create/delete checks; CApp::Link selection/destination/refresh. | FileDir/Find/IO/Name, CReparseAttr/FillLinkData, App/Panel, Browse/Lang, LinkDialog.rc. Сохранить flow; Qt presentation и portable hard/symlink services, Windows-specific junction/WSL/reparse capability. Не переписывать reparse encoding в widget; не скрывать unsupported platforms как полную готовность функции. |
| FM/MemDialog.cpp — P1-07, P2-07 | CMemDialog AddInfoMessage_To_String/OnInit/EnableSpin/OnButtonClicked/OnContinue: required/limit/RAM info, allow/skip default, save next limit/remember current action, numeric validation. | StringToInt, Windows/System portable RAM API, ErrorMsg/MyMessages, Lang/ExtractRes, MemDialog.rc; caller ExtractCallback::RequestMemoryUse. Перенести policy/ranges/parse; Qt spin/edit/radio/errors, callback result transport. Commented AddAction/SkipFile не активные branches; SkipArc/Cancel обязательны. |
| FM/MenuPage.cpp — P1-13, P2-07/09 | CMenuPage OnInit/OnApply/event changed flags: two-bitness shell handlers, cascaded/icons/ElimDup/zone, kMenuItems checkbox→flags, recheck registry state после apply errors. | Explorer ContextMenuFlags/RegistryContextMenu/resources, ZipRegistry/CContextMenuInfo, DLL/FileFind/ErrorMsg, Lang/Format/Help, MenuPage2.rc. Qt page/registry service adapters; Windows handler branch separately capability-gated. ElimDup и zone влияют на extraction callers и не N/A вместе с shell DLL. |
| FM/FormatUtils.cpp — P0-09/10, P1-10/14 | NumberToString и оба MyFormatNew: upstream UInt64 conversion и replacement всех '{0}' placeholders. | IntToString/MyString, LangUtils. Portable bodies можно vendor/reuse через единственный mirrored module; QString conversion только boundary, не самостоятельный replacement algorithm. |
| FM/PropertyName.cpp — P0-06/09, P1-10/14 | GetNameOfProperty: translated 1000+propID для ID<1000, supplied name fallback, decimal ID fallback. | IntToString, LangUtils, PropertyName.h/PropID; callers PanelMenu/ListNotify/ExtractCallback/Browse. Перенести body и property IDs, Qt string boundary; hardcoded current labels не подтверждают generic pipeline. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check` и 54 unique paths
(30 mirrored + 24 missing), все 12 новых путей существуют в upstream и
отсутствуют локально. Приложение/UX не менялись; build/CTest/layout не требуются.
Windows/Linux runtime и rendered visual parity этим review не подтверждены.

## P0-01.3b2 — отсутствующие FM providers и platform support (2026-09-28)

Project baseline `ca5030a`; FM = `CPP/7zip/UI/FileManager`, для каждой строки
exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Все 12 `.cpp` отсутствуют в local mirror, copied scope пуст. Прочитаны
bodies, headers и FM.mak/build callers; все активные bodies остаются открытыми.
Пустые/comment-only единицы ниже отличаются от неперенесённой функции:
этот вывод не закрывает их functional parent packages.

| Путь / owner | Исходный scope и omissions | Dependencies и решение |
| --- | --- | --- |
| FM/RootFolder.cpp — P0-04/05, P1-11, P2-06 | CRootFolder Init/LoadItems/GetProperty/BindToFolder overloads/parent/properties/icons, AreEqualNames и special folder helpers; Windows Computer/Documents/Network/Volumes и non-Windows single root branch. | FSFolder/FSDrives/NetFolder/AltStreamsFolder, MyCom/MyString/PropVariant/PropID, FileName/DLL/ShlObj/Lang/SysIcon. Скопировать routing/name/path bodies; Qt/platform special dirs/icons и folder transport adapters. Non-Windows root branch уже существует: inspect/reuse, не invent отдельную navigation модель. |
| FM/FSDrives.cpp — P1-11, P2-01/05, P3-05 | CFSDrives LoadItems/properties/bind, CDriveInfo device name, AddExt/GetFileSize, CopyFileSpec/CopyTo physical/logical volume image reading с AskWrite/progress. Остальные mutation methods возвращают E_NOTIMPL. | FileSystem/Find/IO/Name/Dir, Alloc/Defs/PropVariant, FSFolder/Lang/SysIcon, IFolderOperationsExtractCallback; FM.mak non-CE и Z7_DEVICE_FILE. Сохранить portable orchestration, platform volume enumeration/device IO adapter. Device imaging scope явно решить в acceptance, не потерять его внутри Drives audit. Исходные unsupported mutations не превращать в активные команды. |
| FM/NetFolder.cpp — P1-11, P2-10 | CNetFolder Init/resource lookup, LoadItems enumeration с access-denied interactive connection retry, provider/local/comment properties, share→FSFolder или nested network bind/parent/icons. Name-based bind E_NOTIMPL. | Windows/Net CResourceW/CEnum/GetResourceInformation/AddConnection2/GetResourceParent, PropVariant, FSFolder/SysIcon. Сохранить resource traversal и return paths; Windows network service и portable mounted-share capability adapters. Qt filesystem list не эквивалент network containers; credentials не сохранять в app state. |
| FM/AltStreamsFolder.cpp — P2-06, P1-03, P0-11 | CAltStreamsFolder Init/LoadItems/property/sort/parent/clone/change detection, CreateFile/Rename/Delete/CopyTo, CopyStream/UpdateFile/GetFsParentPrefixSize; native stream rename fallback copy-delete. CopyFrom/CopyFromFile/CreateFolder/SetProperty E_NOTIMPL; commented stream-extraction/CopyFrom не активные bodies. | FileFind CStreamEnumerator/change notifications, FileIO NtSetInformationFile, FileDir/FileName/Wildcard, FSFolderCopy CCopyStateIO, ExtractingFilePath/IFolderArchive, SysIcon/PropVariant. Windows filesystem ADS capability отдельна от archive streams. Сохранить guards/AskWrite/totals/error semantics, platform stream/watcher/IO adapters; macOS/Linux applicability решить по capability, не blanket N/A всего P2-06. |
| FM/FilePlugins.cpp — P2-01/02/08 | CExtDatabase::FindExt/Read: CArchiveFolderManager extensions/icon lookup, split list/merge duplicate extensions. External CPluginInfo loading loop закомментирован. | Agent/ArchiveFolder manager, PluginLoader, StringUtils/SplitString, MyCom/BSTR. Перенести active extension database bodies, metadata supplied by backend/capability service; DLL loader adapter отдельно. Комментарии не подтверждают работающие внешние handlers. |
| FM/RegistryAssociations.cpp — P2-08 | NRegistryAssoc CShellExtInfo::ReadFromRegistry/IsIt7Zip, key path helpers, AddShellExtensionInfo/DeleteShellExtensionInfo, icon index parse и open command registration. | Registry/StringConvert/StringToInt/IntToString; SystemPage callers. Windows service должен сохранить key/hive/error semantics; macOS/Linux association providers отдельные platform adapters с документированной difference. При переносе не выполнять регистрацию на машине агента: тестировать isolated registry/service fixture. |
| FM/EnumFormatEtc.cpp — P1-16, P2-09 | CEnumFormatEtc deep-copy/lifetime, Next/Skip/Reset/Clone и CreateEnumFormatEtc validation. Active caller PanelDrag::EnumFormatEtc. | IEnumFORMATETC/FORMATETC/DVTARGETDEVICE, CoTaskMemAlloc/Free, MyCom2/IDecl. Windows OLE service adapter; QMimeData formats projection для portable drag. Native iterator bodies на Windows не терять, clone position/ownership semantics сохранить. Применимость portable iterator body решить в P1-16, не объявлять drag/drop N/A. |
| FM/ClassDefs.cpp — P2-01/02/09, P3-04 | GUID definition translation unit: MyInitGuid + Agent.h/MyWindowsNew.h/Explorer MyExplorerCommand.h; runtime algorithms отсутствуют. | MyWindows/COM interface declarations, FM.mak ClassDefs.obj. Build adapter для Windows interfaces; duplicate GUID definitions исключить. Решение о включении зависит от ported interfaces, не functional replacement. |
| FM/ProgramLocation.cpp — P3-04/05 | Только StdAfx include; ProgramLocation.h содержит guard, активных functions/fields нет. | FM.mak ProgramLocation.obj сохраняет пустую unit. Не создавать новый location algorithm под этим именем; module-dir behavior реально находится в DLL/Help/Lang/Plugin services. Empty unit не закрывает resource location requirement. |
| FM/RegistryPlugins.cpp — P2-02, P3-05 | Активных bodies нет: ReadPluginInfo/ReadFileFolderPluginInfoList и header types находятся в comments. FM.dsp перечисляет файл, FM.mak его не включает; похожие calls в FilePlugins/FileFolderPluginOpen тоже commented. | RegistryPlugins.h/MyString и исторические DLL/plugin declarations. Не активировать старый закомментированный loader автоматически. Обязательные external handler/codecs capabilities остаются у Agent/LoadCodecs/PluginLoader и P2-02; comment-only inventory decision не их N/A. |
| FM/StdAfx.cpp — P3-04/05 | Только include StdAfx.h: compiler warning/Windows-version commentary и Common/Common.h, активных algorithms нет. | PCH/compiler integration adapter; mirrored sources сохраняют original includes, возможно portable common header reuse. Не считать platform macros функциональным port; compare header scope при каждом dependency update. |
| FM/StringUtils.cpp — P0-02, P1-08, P2-01 | SplitStringToTwoStrings quote-state loop: removes quotes, splits first unquoted space; JoinStrings commented. | MyString и StringUtils.h (SplitString inline), FM startup argv parsing callers, FilePlugins metadata split. Portable body копировать с original types/control flow; OS argv boundary отдельно, не заменять shell parser style expression. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 66 unique paths
(30 mirrored + 36 missing). Все новые upstream paths существуют, local copies
отсутствуют. Code/UX не менялись, build/CTest/layout не запускались;
Windows/Linux и device/network/ADS runtime остаются unverified.

## P0-01.3b3 — отсутствующие FM open, drag, hash и integration workflows (2026-09-28)

Project baseline `6ce4cac`; FM = `CPP/7zip/UI/FileManager`, exact source для
каждого пути — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Все девять `.cpp` отсутствуют локально, copied scope пуст, все активные bodies
остаются открытыми. Проверены functional bodies перечисленных групп, headers,
includes/resources/callers. Это inventory review; при реализации каждого owner
потребуется полный source comparison его scope и dependency closure.

| Путь / owner | Обязательный source scope | Dependencies и adapter decision |
| --- | --- | --- |
| FM/FileFolderPluginOpen.cpp — P0-04/10/12, P1-06/09, P2-01/02 | CFfpOpen::OpenFileFolderPlugin, CThreadArchiveOpen::Process, filename split, GetFolderLevels/GetFolderError: open path/stream with forced format, password inheritance, volume callback lifetime, per-level/non-open errors. | Agent/CArchiveFolderManager, ArchiveOpenCallback, OpenCallback/ProgressDialog2, PluginLoader, PropVariant/PropertyName/Format/Lang, FileName/Thread. Сохранить lifecycle/error/format decisions; IFolderManager transport и Thread/GUI через backend adapters. Историческая external plugin preference loop commented, не evidence работающего loader. |
| FM/PanelItemOpen.cpp — P0-04/12, P1-08/09/15, P2-06/10, P3-02 | OpenAsArc* и parent links, OpenParentArchiveFolder/OnOpenItemChanged, OpenItem/EditItem/OpenItemInArchive: direct stream → bounded memory → temp disk fallback; external viewer/editor/diff, process/child tracking, changed-temp update prompt, readonly/errors, exit/temp lifetime; IsVirus_Message и extension routing. | App/Panel CFolderLink/CTempFileInfo, CFfpOpen/CopyTo/CopyFromFile/UpdateCallback100/CVirtFileSystem, FileStreams/StreamObjects, ProcessUtils/ToolHelp, FileName/PropVariant/ExtractingFilePath/Registry/Lang. Скопировать flow и security checks; Qt signals/prompts/process services и backend nested extraction/update. Windows process snapshot/handle waits адаптировать по ОС, отсутствие handle не равно завершённой работе. Commented CBufSeqOutStream_WithFile/GetTime/ReadZoneFile/DeleteOldTempFiles не активные bodies; actual temp deletion browser — BrowseDialog2. |
| FM/PanelDrag.cpp — P1-16, P1-01/09, P2-09/10 | CDataObject SetData/GetData/QueryGetData/EnumFormatEtc ownership/transfer metadata и delayed extraction; CDropSource QueryContinueDrag/GiveFeedback; CPanel::OnDrag; CDropTarget enter/over/leave/drop, PositionCursor/GetEffect/GetTargetPath/source handshake; CompressDropFiles/right-button menu, CApp drag focus state. | IDataObject/IDropSource/IDropTarget/FORMATETC/CF_HDROP/MemoryGlobal/Shell/EnumFormatEtc, App/Panel, ArchiveName/CompressCall/ExtractingFilePath/Messages/Lang. Functional command/effect/temp/wait decisions сохраняются; Qt QDrag/QMimeData и platform OLE adapters заменяют transport. Internal FS copy, archive update, external→Add, same-drive modifiers, Cancel и target/source operation ownership не сводить к URL-drop handler. |
| FM/PanelCrc.cpp — P2-03, P0-10, P2-10 | CDirEnumerator traversal, CThreadCrc ProcessVirt two-pass scan/hash, progress/error accounting, result display once, CApp CalculateCrc2/CalculateCrc filesystem versus archive stream dispatch. | FileFind/IO/Name, MyBuffer/LoadCodecs/CHashBundle/HashGUI, ProgressDialog2, App/Panel/CopyTo. Перенести traversal/orchestration/statistics; hashing engine остаётся official engine/backend, callbacks/UI/services adapters. Flat mode recursion, archive hashMethods, first-file name, errors и Cancel учитывать. |
| FM/BrowseDialog2.cpp — P1-15, P2-10, P3-02 | CBrowseEnumerator bounded recursive totals/no reparse traversal; CBrowseDialog2 Reload/sort/selection/parent/keyboard/context/properties/delete, temp-name filters, prefix restriction/confirmation, MyBrowseForTempFolder; shared StartApplication/StartApplicationDontWait. | BrowseDialog2.h/.rc, RegistrySettings/Lang/Help/Format/PropertyName/PropIDUtils/SysIcon, FileFind/Dir/Name, ProcessUtils/Shell/PropVariant. Перенести active Delete Temporary Files workflow; Qt list/layout/menu/focus, platform shell launch/process/temp services. OnCommand фильтра commented; disabled fixed temp filter не feature debt. Prefix/path/link guards должны пройти security regression; screenshot не доказывает их. |
| FM/SystemPage.cpp — P2-08, P1-12 | CModifiedExtInfo/CAssoc state, CSystemPage Init/ChangeState/OnApply/RefreshListItem/AddIcon/events/keys; current/all-users association cycle, save changed only, error revert/reload and SHChangeNotify. | SystemPage.h/.rc, FilePlugins/RegistryAssociations, Registry hives, DLL/ErrorMsg/ShlObj, Lang/Help, image/list/property-page controls. Сохранить state/selection/error algorithms; Qt page/columns/icons и per-platform association services. Executable open command must name our app через service; tests не меняют реальные associations. |
| FM/SysIconUtils.cpp — P0-05/06/09, P1-11, P2-10 | Shell_GetFileInfo* wrappers/CSIDL/image lists; CExtToIconMap GetIconIndex, sorted attribute/extension binary searches, bounded caches/compressed maps/numeric split-extension shortcut. | SysIconUtils.h, FileDir, ShlObj/COM memory/native shell icons, MyString/MyVector. Portable cache/search bodies сохранить; OS icon lookup/image ownership adapters, Qt QIcon boundary. Реальные per-file icons/settings должны учитывать slow services и big-list limits; generic icons не подтверждают завершённый scope. |
| FM/TextPairs.cpp — P0-05, P1-03/04/10 | CPairsStorage parse/save/sort/binary lookup/add/replace/delete/GetValue, quoted ID/BOM/CR handling. | TextPairs.h/MyString/MyVector, FSFolder comment storage callers. Полностью portable bodies копировать/reuse, не заменять parser Qt text split; сохранить one authoritative module. Unicode/space IDs, duplicate update и malformed lines покрыть consumers tests. |
| FM/VerCtrl.cpp — P1-08, P3-05 | CApp::VerCtrl Edit/Commit/Revert/Diff active menu commands: bounded file read/data/time compare, numbered backups, readonly/timestamp adjustments, overwrite confirm and diff launch. | App/Panel, RegistryUtils VerCtrlPath, FileFind/IO/Dir/Name/StringToInt, OverwriteDialog; MyLoadMenu active IDM_VER_* routing. Это исходный local version-control workflow, не Git integration. Перенести filesystem/process/UI adapters; user-facing scope уточнить в P1-08/P3-05, без N/A из-за отсутствия отдельной строки плана. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 75 unique audited
paths (30 mirrored + 45 missing FM), полное покрытие всех upstream FM `.cpp`.
P0-01.3b1–b3 покрывают 33 оставшихся missing FM; P0-01.3a — первые 12.
Code/UX не менялись, build/CTest/layout не запускались. Runtime Windows/Linux,
OLE drag/shell/device и rendered Windows parity не проверены.

## P0-01.3c1 — отсутствующие Agent modules (2026-09-28)

Project baseline `3e914ce`. Agent = `CPP/7zip/UI/Agent`; exact source для
всех пяти строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied scope пуст; все активные bodies остаются обязательными. Проверены
bodies и interfaces из Agent.h/IFolderArchive/UpdateCallbackAgent.h, FM callers
и AGENT_OBJS. Portable functional policy должна сохраняться при замене COM
и in-process engine transport на ArchiveBackend/официальный 7zz.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Agent/ArchiveFolder.cpp — P1-01, P0-12, P2-06/07 | CAgentFolder SetZoneIdMode/SetZoneIdFile/CopyTo: reject archive move-out, interface callback, cur/no paths/no paths alt-stream selection, Extract с Ask. | Agent/Proxy2, ArchiveExtractCallback, ExtractMode/IFolderArchive; PanelCopy caller. Сохранить path/zone/overwrite decisions, transport адаптировать в backend; filesystem extraction adapter не заменяет policy. Commented SetReplaceAltStreamCharsMode не активная функция. |
| Agent/ArchiveFolderOpen.cpp — P0-04/12, P1-06/11, P2-01/02/08 | CCodecIcons LoadIcons/FindIconIndex, CArchiveFolderManager LoadFormats/OpenFolderFile/GetExtensions/GetIconPath: lazy formats, parsed icon pairs, non-open-error folder, progress refs и bind-to-root lifetime. | LoadGlobalCodecs/CCodecs libraries, Agent/OpenArchive, StringToInt/ResourceString/DLL, IFolderManager/BSTR. Resource/DLL icons и engine-open transport адаптировать; active external codecs metadata остаётся. Commented FindFormat/GetTypes/CreateFolderFile не активные bodies, не замена актуальной capability модели. |
| Agent/AgentOut.cpp — P1-04/05/09, P2-02/06, P3-02 | CAgent SetFolder/SetFiles, EnumerateArchiveItems/2, DoOperation/2 action-set→directory/archive pairs→UpdateProduce, properties/SFX prefix, processed empty-file tracking; CommonUpdate/DeleteItems/CreateFolder/RenameItem/CommentItem/UpdateOneFile/SetProperties. | AgentProxy/2, EnumDirItems/UpdatePair/UpdateProduce/UpdateCallback/UpdateAction, Wildcard/FileName/TimeUtils/ItemNameUtils, FileStreams/CopyCoder/IFolderArchive. Functional pair/path/rename/comment policy переносить; compression/UpdateItems выполняет official engine. Backend должен выразить исходные capabilities и mutations, не исполнять engine API в widgets. Не заменять сохранение неизменных entries перебором заново написанного algorithm. |
| Agent/ArchiveFolderOut.cpp — P1-04/09, P1-02, P2-06, P0-10 | CAgentFolder GetPathParts/CommonUpdateOperation и mutation wrappers: CanUpdate/hash guards, original prefix/tail stream, temp archive, KeepMode/Close, MoveToOriginal, MoveArc callbacks/Before_ArcReopen, processed source deletion only after success, ReOpen и restore Proxy/Proxy2/alt-folder path. | WorkDir/CWorkDirTempFile, FileStreams/LimitedStreams/CopyCoder, AgentOut/UpdateCallbackAgent/IFolderArchive_Update_MoveArc, FileDir/Find. Сохранить transaction/return/reopen order, cancellation during replace и error recovery; process/temp/filesystem/GUI services adapters. CreateFile E_NOTIMPL; SetProperty только BSTR comment ZIP; не объявлять unsupported generic mutations готовыми. |
| Agent/UpdateCallbackAgent.cpp — P1-04/05/09, P0-10/12 | CUpdateCallbackAgent SetCallback, item/totals/ratio, OpenFileError S_FALSE versus ReadingFileError failure, GetStream/update/extract result/delete operation reports, lazy crypto interfaces/password. | IUpdateCallbackUI/Common UpdateCallback, IFolderArchive callback/2/crypto/compress-progress, ErrorMsg/IntToString. Сохранить result/callback distinctions и password-defined semantics, process progress/event/error adapter. WriteSfx/CheckBreak здесь upstream no-op; отмена приходит через вызываемые callbacks и не становится optional. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 80 unique paths
(30 mirrored + 50 missing), полный coverage всех Agent `.cpp` в baseline.
Runtime/source implementation не менялись; build/CTest/layout не запускались,
Windows/Linux и engine callback transport пока не проверены.

## P0-01.3c2 — отсутствующие GUI modules (2026-09-28)

Project baseline `f6e1411`. GUI = `CPP/7zip/UI/GUI`; exact source всех
восьми строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Проверены source bodies, headers, BenchmarkDialog.rc и GUI/makefile.
Copied scope пуст; функциональные bodies ниже остаются обязательными.
GUI/makefile собирает отдельный 7zG entrypoint, а не FM: его process startup
адаптируется к Qt executable, общие command/error/workflow semantics не N/A.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| GUI/BenchmarkDialog.cpp — P2-04, P0-10, P2-10 | Benchmark/ParseNumberString, CBenchmarkDialog initialization/memory limits/dictionary/thread/pass selection, Start/Restart/Stop/Cancel, result formatting/aggregation, UpdateGui, CThreadBenchmark::Process, CBenchCallback/2 и CFreqCallback. | Common Bench/COneMethodInfo/CTotalBenchRes, system RAM/CPU/affinity, locks/thread/timer/messages, Lang/Help. Сохранить pass loop, first-pass frequency, error/cancel/join/restart order, formulas и capped history; engine Bench через official 7zz adapter. Qt layout по BenchmarkDialog.rc: compress/decompress current/resulting, usage/speed/rating, totals и log; TotalMode отдельный text view. Windows resource не доказывает rendered parity. |
| GUI/ExtractGUI.cpp — P0-12, P1-01/03, P2-02/06/07 | ExtractGUI и CThreadExtracting::ProcessVirt: output-path validation/normalization, dialog options transfer, callback initialization, Extract result/stats/hash report и messageWasDisplayed. | ExtractDialog/ExtractCallback/ArchiveExtractCallback/Extract/HashGUI, FileDir/Find/Name/Thread, ProgressDialog2. Сохранить forced/dialog option precedence, E_ABORT и distinction finished/messages; filesystem, Qt modal/progress и engine process adapters. Z7_SFX conditional scope требует отдельной P2-02/P3-05 сверки. |
| GUI/HashGUI.cpp — P1-14, P0-10/12 | CHashCallbackGUI scan/open/process/error callbacks, ProcessVirt→HashCalc, AfterLastFile/AddHashBundleRes, HashCalcGUI и ShowHashResults. | HashCalc/CHashBundle, ProgressDialog2/Thread, ListViewDialog/Lang/FormatUtils. Сохранить digest/data/name/stream statistics, S_FALSE file errors и отсутствие final report при E_ABORT; backend hash engine и Qt two-column result/progress adapters. |
| GUI/UpdateCallbackGUI.cpp — P0-10/12, P1-04/05/09 | CUpdateCallbackGUI totals/ratio/scan/archive/item/extract reports, OpenFileError/ReadingFileError, FailedFiles, crypto state/password prompt, deleting-after-compress, MoveArc и WriteSfx. | Common UpdateCallback/IUpdateCallbackUI, OpenResult_GUI/ErrorMsg/FormatUtils, ProgressDialog2/PasswordDialog. Сохранить S_FALSE open-error versus S_OK read-error reporting, counters, password-defined/asked state и cancellation; process callback/event adapter. Upstream no-op hooks не отменяют требования lifecycle. |
| GUI/UpdateCallbackGUI2.cpp — P0-10, P1-04/09, P2-02 | Init/SetOperation_Base, ShowAskPasswordDialog, MoveArc_Start/Progress/Finish и wrappers. | Lang operation tables/FormatUtils, progress Sync/WaitCreating/CheckStop и PasswordDialog. Сохранить ожидание создания перед password prompt, E_ABORT, `< 1<<57` overflow branch процента, repeat-percent CheckStop и clearing status; Qt signals/sync/process adapters. |
| GUI/UpdateGUI.cpp — P0-12, P1-04/05, P2-02/06 | ParseProp_Time_BoolPair/ParseProperties, SplitOptionsToStrings/IsThereMethodOverride/ParseAndAddPropertires, SetOutProperties, g_UpdateMode_Pairs, ShowDialog, UpdateGUI/CThreadUpdating::ProcessVirt. | CompressDialog/CCompressionInfo, UpdateArchive/UpdateOptions/UpdateAction/WorkDir, LoadCodecs, command properties/FileName/ModuleDir/Registry. Сохранить action-set mapping, one-command/one-file/capability guards, dialog round trip, method override/property replacement/time/security/link/password/volume/shared/delete/SFX decisions; Qt/platform adapters и official engine transport в backend. |
| GUI/GUI.cpp — P0-04/12, P1-01/03/04/05/14, P2-04, P3-05 | Main2 command-line phases/codecs/formats/password, benchmark/extract/update/hash dispatch, archive enumeration, failed-file/error exit codes; WinMain exception boundaries. | ArchiveCommandLine/LoadCodecs/ExtractGUI/UpdateGUI/HashGUI/Benchmark, FM callbacks/Lang, Windows DLL security/common-controls/OLE. Preserve applicable command routing/options/errors with Qt startup adapter; standalone 7zG invocation/exit behavior needs explicit acceptance scope, cannot erase shared functional bodies as presentation. |
| GUI/StdAfx.cpp — P0-02 | Only include StdAfx.h; no functional body. | Header delegates FM/StdAfx.h and MSVC warning pragma. Build/PCH adapter, no functional port claimed. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 88 unique paths
(30 mirrored + 58 missing), полный Agent/GUI coverage в exact baseline.
Runtime/UI не менялись; build/CTest/layout не запускались, rendered Windows
reference и Windows/Linux runtime не проверены. Benchmark и остальные workflow
остаются открытыми в своих functional packages.

## P0-01.3d1 — Common names, paths, update pairs и temp policy (2026-09-28)

Project baseline `0c70e3b`. Common = `CPP/7zip/UI/Common`; exact source всех
десяти строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные bodies, связанные interfaces и callers в FM/Agent/GUI/Common.
Copied scope пуст. Здесь есть portable policy, которую нельзя заменить
самостоятельным Qt/std algorithm или считать перенесённой вместе с `7zz`.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveName.cpp — P1-04/14, P1-08 | CreateArchiveName: file/parent-folder base name, hash keep-name, single-dot stripping, valid filesystem name, collision suffixes для 7z/zip/tar/wim/sha256 и baseName. | MyString/MyVector, Wildcard/StringToInt, C Sort HeapSort, FileDir/Find/Name, ExtractingFilePath; Panel/PanelDrag callers. Сохранить suffix scan/sort и `_2` gap selection; narrow file-info/parent/root adapters для платформ. Commented tar special-case не активная policy. |
| Common/DefaultName.cpp — P0-04, P1-01 | GetDefaultName3/2: recognized extension/addSubExtension, fallback last dot, `~` при отсутствии extension, TrimRight. | UString, OpenArchive::DefaultName callers. Portable unchanged body candidate; не заменять QFileInfo completeBaseName. |
| Common/ExtractingFilePath.cpp — P0-12, P1-01/06/11/13, P2-07 | ReplaceIncorrectChars/Correct_AltStream_Name/IsSupportedName/CorrectUnsupportedName/Correct_PathPart/Get_Correct_FsFile_Name/Correct_FsPath/MakePathFromParts; platform trail replacement defaults, Windows reserved names, ADS postfix/RLO, empty/dot parts и absolute prefixes. | UString/UStringVector, Wildcard/FileName; ArchiveExtractCallback, Extract, FM temp/drag/items и AltStreamsFolder callers. Retain platform branches and control flow; safety validation in backend supplements this correction policy. Correct_FsPath alone не доказывает containment или symlink safety. Header's obsolete `[]` comment не actual replacement: body uses `_`. |
| Common/SetProperties.cpp — P0-12, P1-04/05, P2-06 | ParseNumberString, CPropPropetiesVector, SetProperties: empty/no-interface success, trailing +/- boolean when no value, uint32/uint64/string conversion, stable names/values lifetime. | MyCom/MyString/StringToInt/PropVariant/ISetProperties; Update/OpenArchive callers. Preserve property typing before process serialization; replace only in-process COM dispatch with backend official-engine adapter. |
| Common/SortUtils.cpp — P0-05, P1-04/05/14 | CompareStrings/SortFileNames: upstream filename comparison, sorted index vector, empty-input return. | Wildcard CompareFileNames/CUIntVector; EnumDirItems/UpdatePair. Portable source candidate; no Qt locale comparator replacement. |
| Common/TempFiles.cpp — P0-10, P1-04/05/09, P2-02/06 | CTempFiles::Clear and header destructor: reverse-order cleanup, NeedDeleteFiles controls deletion but paths always consumed. | FStringVector/FileDir::DeleteFileAlways; Update owns lifetime. Filesystem adapter only; retain success-path ownership transfer and error cleanup policy, inspect caller changes in d3. |
| Common/UpdateAction.cpp — P1-04/05/09, P0-12 | Add/Update/Fresh/Sync/Delete seven-state action tables; header CActionSet::IsEqualTo/NeedScanning. | UpdateAction NPairState/NPairAction, AgentOut/UpdateGUI/Update/command parser. Portable source candidate; preserve exact tables and scan policy when building official-engine options. |
| Common/UpdatePair.cpp — P1-04/05, P2-06/07, P0-12 | MyCompareTime, CompareArcItemsBase/CompareArcItems, ThrowError/GetUpdatePairInfoList: sorted merge, duplicate/censor collision checks, directory distinction, time/size states, ADS HostIndex. | DirItem/CDirItems/CArcItem/CFiTime/CArcTime, TimeUtils, IArchive, SortUtils/Wildcard/upstream vectors; AgentOut/Update callers. Preserve DOS/Unix saturation, precision clipping and sub-100ns comparisons. Engine boundary не повод переписать pair algorithm; timestamp/filesystem inputs use narrow adapters. |
| Common/UpdateProduce.cpp — P1-04/05/09, P0-12 | UpdateProduce and CUpdatePair2 helpers: ignore/delete notification, copy/compress collision guards, orphan ADS rule, anti-item props, IsSameTime, ReserveDown. | UpdatePair/UpdateAction/IUpdateProduceCallback, AgentOut/Update callers. Retain NewData/NewProps/UseArcProps and action flow; engine executes resulting mutations. ShowDeleteFile return is not propagated by this body: do not invent an upstream failure contract. |
| Common/WorkDir.cpp — P1-09, P2-06, P0-10/11 | GetWorkDir current/specified/system and Windows removable-only policy; CWorkDirTempFile::CreateTempFile/MoveToOriginal, release output before replace. | ZipRegistry CInfo, FileName/System/Dir, CTempFile/FileStreams; GUI UpdateGUI/Agent ArchiveFolderOut. Settings/filesystem/drive-type adapters; preserve basename/prefix normalization, original target and failure HRESULT. Non-Windows removable-only behavior requires explicit platform parity decision, not silent rewrite. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 98 unique paths
(30 mirrored + 68 missing), все десять source files существуют в baseline.
Runtime/UX не менялись; build/CTest/layout не запускались. Mandatory bodies и
platform/engine adapters остаются открытыми до functional ports и regression checks.

## P0-01.3d2a — Common open callbacks и property conversion (2026-09-28)

Project baseline `9aad76e`. Common = `CPP/7zip/UI/Common`; exact source обеих
строк — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Проверены bodies, interfaces и consumers. Copied scope пуст; это открытые
functional modules, включая code внутри platform/crypto/SFX conditionals.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveOpenCallback.cpp — P0-04/10/12, P1-03, P2-01/02/08 | COpenCallbackImp Init2/subarchive/properties/progress/password/GetStream; CMultiStreams linked LRU Insert/Remove/Close/PrepareToOpenNew/EnsureOpen; CInFileStreamVol Read/Seek/GetSize/destructor и AtCloseFile. | FileFind/Name/System, FileStreams/StreamUtils, MyCom/PropVariant/IArchive/IPassword, ArchiveExtractCallback::IsSafePath; OpenArchive lifetime. Preserve reopen callback precedence, PasswordWasAsked, subarchive S_FALSE, safe-volume-path/wildcard guards, bounded open descriptors and saved-position reopen, volume names/sizes/used flags. Official engine owns in-process streams through 7zz boundary; retain application callbacks/policy with narrow adapter and explicitly review engine-owned scope. IProgress SetCompleted checks break even when byte totals ignored. No blanket N/A for COM presentation. |
| Common/PropIDUtils.cpp — P0-05/12, P1-11/13, P2-07 | ConvertPosixAttribToString/ConvertWinAttribToString, ConvertPropertyToShortString2/ConvertPropertyToString2; SID/owner/ACL helpers and ConvertNtSecureToString, Windows CheckNtSecure, reparse-tag table/ConvertNtReparseToString. | CpuArch endian helpers, IntToString/StringConvert/PropVariantConv/PropID, FileIO::CReparseAttr; PanelListNotify/PanelMenu/BrowseDialog2, GUI ExtractGUI, ArchiveExtractCallback consumers. Preserve precision/ns100/time-zero handling, CRC/inode/address and attributes formatting, descriptor size/offset validation and reparse fallback hex. Copy portable conversion bodies, adapt typed backend property input and platform services; Qt text output only. Security display parses archive bytes across platforms; native ACL application remains Windows-specific. Commented Data_To_Hex/device/ACE-detail bodies are omitted upstream code, not active requirements. |

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 100 unique paths
(30 mirrored + 70 missing), оба exact-source paths существуют.
Runtime/UX не менялись; build/CTest/layout не запускались. Volume transport,
property/security behavior и Windows/Linux проверки остаются functional work.

## P0-01.3d2b — Common command parser (2026-09-28)

Project baseline `6d0d8a9`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный `.cpp`, options/parser header и GUI/Console consumers.
Copied scope пуст. GUI/GUI.cpp и Console/Main.cpp вызывают Parse1/Parse2;
локального CArcCmdLineParser пока нет. Standalone CLI scope нужно согласовать
с acceptance P3-05; общая archive command policy обязательна независимо от него.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveCommandLine.cpp — P0-04/12, P1-01/03/04/05/09/14, P2-01/02/06/07, P3-05 | CArcCommand group/path helpers, switch tables/ParseArchiveCommand, CNameOption/AddNameToCensor/listfile/non-switch/rename/switch wildcard helpers, Windows ParseMapWithPaths/CEventSetEnd, ParseUpdateCommandString/2/SetAddCommandOptions/SetMethodOptions, FindCharset/SetBoolPair/ParseSizeString, CArcCmdLineParser Parse1/Parse2, non-Windows module prefix helpers. | CommandLineParser/Wildcard/ListFileUtils/StringToInt/StringConvert/IntToString/MyException, EnumDirItems/UpdateAction/Update/Extract/Hash options, FileDir/Name/System/ErrorMsg/PropVariantConv, Windows mapping/events/privileges, Alloc large pages. Preserve recursive/matching/mark rules, default mask and `--` boundary, rename-pair validation, action-state restrictions/multiple archives, volume last-zero rejection, method properties, charset and overflow checks. Preserve extraction forced overwrite/path precedence, separate archive censor, absolute-mask rejection, ADS/link/security/owner/zone/time/shared flags, stdout/data-channel conflicts and update/delete/SFX/email guards. Adapt OS process/terminal/mapping/module services narrowly; settings/widgets pass typed options through ArchiveBackend, engine serialization belongs there. Global case/trail/time/large-page state needs scoped integration review without changing parser algorithms. Commented hashgen/GetUpdatePairActionType/Linux huge-page branch are inactive, not proof of omitted active feature completion. |

Parse1 содержит реальные process side effects (affinity/privileges/large pages),
а не только parsing: их нельзя без review выполнять в GUI-потоке при каждом
построении команды. Backend process adapter должен сохранять необходимые engine
options и отделять process startup services. Windows mapping transport имеет
size/marker/termination checks и event RAII; portable transport требуется review.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 101 unique paths
(30 mirrored + 71 missing), exact source path существует. Runtime/UX не менялись;
build/CTest/layout не запускались. Parser integration и platform behavior открыты.

## P0-01.3d2c1 — Common codecs/capabilities (2026-09-28)

Project baseline `6ab0a55`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный source, CArcInfoEx/CCodecs interfaces и FM/Agent/GUI/Common
consumers. Copied scope пуст. Codec implementation остаётся official engine;
это не отменяет port metadata/capability/loading policy и явный adapter audit.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/LoadCodecs.cpp — P0-04/12, P1-04/05/11/14, P2-01/02/04/06/07, P3-05 | RegisterArc, CArcInfoEx FindExtension/AddExts, ParseSignatures, typed handler/method property helpers; CCodecs LoadCodecs/LoadFormats/LoadDll/LoadDllsFromFolder/Load/CloseLibs, IsSupportedDll, registry/module lookup, format name/extension/type-chain lookup, external codec/hasher interfaces and Get_CodecsInfoUser_Vector/error formatting. | RegisterArc/RegisterCodec/ICoder/IArchive, upstream MyCom/strings/buffers/vectors/StringToInt/StringConvert/PropVariant, DLL/FileFind/FileIO/ErrorMsg/Registry, external module ABI and optional large pages. Preserve sorted format order, paired/add extensions, flags/time flags/signature offsets, old-interface fallback, update support, dotted type/`*` lookup and exe exclusion. Retain typed/default/error semantics, incompatible-library reporting, unused-library cleanup and SetCodecs(NULL) before unload. Backend supplies official-engine capabilities/metadata; Qt widgets must not independently guess capability from extensions. In-process decoder/encoder/hasher factories and DLL exports belong engine transport adaptation; preserve original source scope and document exactly which remain delegated to 7zz. External handler support remains required review, not N/A. |

Header scope включает CreateArchiveHandler/CreateInArchive/CreateOutArchive,
CArcInfoEx flag/time helpers и compare: это часть dependency closure даже если
factory выполняет engine. Commented local SplitString body не используется:
active AddExts вызывает supporting SplitString; не копировать старую реализацию
ради устранения зависимости. Windows registry fallback и non-Windows `7z.so`
baseline не означают автоматически подходящую packaged Qt lookup policy.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 102 unique paths
(30 mirrored + 72 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Capability adapters и platform loading открыты.

## P0-01.3d2c2 — Common archive opening/nesting (2026-09-28)

Project baseline `4546ae9`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный source и COpenOptions/COpenType/CArc/CArchiveLink interfaces,
проверены consumers. Copied scope пуст. Source включает функциональный parser
handler внутри UI/Common: его расположение не делает algorithm Qt presentation.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/OpenArchive.cpp — P0-04/12, P1-01/03/06/11, P2-01/02/07/08, P3-05 | NArchive::NParser CParseItem/CHandler sorted regions/unknown gaps/properties/extract/streams; Archive_GetItemBoolProp/IsItem_* и typed archive helpers; CArc GetItem_PathToParent/Path/DefaultPath/Path2/GetItem/Size/MTime; signature/order helpers, offset/extract-to-open callbacks, CArcErrorInfo/ReadBasicProps/PrepareToOpen/CheckZerosTail/OpenArchiveSpec/OpenStream2/OpenStream/OpenStreamOrFile/ReOpen; CArchiveLink KeepMode/Close/Release/Open/2/3/ReOpen; ParseComplexSize/ParseTypeParams/ParseType/ParseOpenTypes. | LoadCodecs/DefaultName/SetProperties/ArchiveOpenCallback/DirItem/IArchive, MyCom/ComTry/strings/vectors/Wildcard/UTFConvert/StringToInt/CpuArch, FileDir/FileStreams/LimitedStreams/StreamUtils/ProgressUtils/CopyCoder. Preserve typed property failures/defaults, raw UTF16/path escape conversion, tree/ADS parent handling/deleted prefixes and time precision fallback. Preserve format exclusions/forced type/extension/signature/backward/UDF order, scan limits/callback offsets, SFX and tail/error distinctions, nested main-subfile order/depth bound, used-volume tracking and Open_Finished, reverse close and single-level reopen guard. Official engine handles archive streams/decoder/parser execution through narrow backend transport; copied application path/type/lifecycle policy remains required. Widgets must not recreate signature search or access in-process archive APIs. |

OpenStream2 имеет отдельные main-extension, start-archive и sliding-buffer
signature-scan phases; parser собирает sorted regions/unknown gaps и может
вернуть parser archive вместо обычного archive. Engine-owned delegation этих
тел требует точного scope/acceptance в P3-05; отсутствие Qt parser view не N/A.
OpenArchiveSpec может test-extract для определения physical size. ReOpen
preserves global offset и ограничивает вложенность; Open2 учитывает только
использованные volumes. Source port должен сохранить эти distinctions, включая
S_FALSE versus HRESULT failure, а не одинаковое сообщение «не архив».
Commented old Set_ErrorsText/IsHashType/path reconstruction не активные bodies.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 103 unique paths
(30 mirrored + 73 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Open/reopen/parser transport и platform parity
остаются открытыми до functional implementation и regression checks.

## P0-01.3d3a — Common scan/extract orchestration (2026-09-28)

Project baseline `682128d`. Exact source обоих Common paths — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные bodies, headers и consumers; copied scope пуст. Файловые
services адаптируются отдельно от upstream censor/scan/operation policy.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/EnumDirItems.cpp — P0-05/10/12, P1-04/05/11/14/16, P2-01/06/07/10 | FindFile_KeepDots; CDirItems AddDirFileInfo/AddError/ScanProgress/GetPrefixesPath/GetPhyPath/GetLogPath/ReserveDown/AddPrefix/DeleteLastPrefix/constructor/AddSecurityItem/EnumerateOneDir/EnumerateDir/EnumerateItems2/SetLinkInfo/FillFixedReparse/FillDeviceSizes; EnumerateDirItems/_Spec/EnumerateForItem/EnumerateAltStreams/CanUseFsDirect/IsVirtualFsFolder/EnumerateItems/EnumerateDirItemsAndSort; ConvertToLongName/s and CMessagePathException constructors. | DirItem/EnumDirItems headers, Wildcard/SortUtils/upstream strings/vectors, FileDir/IO/Name/Find/SecurityUtils/UTFConvert, POSIX owner APIs and Windows reparse/ADS; AgentOut/Update/HashCalc/GUI/CompressCall2. Preserve physical/logical prefixes, include/exclude recursion/direct-name optimization, dot names, folder/file/ADS stats, scan error callback propagation and periodic progress. Retain link traversal/stream suppression, security buffer/dedup policy, Windows link target reduction, POSIX device-size/owner mapping, long-name censor merge and duplicate sorted full-path errors. Narrow platform metadata/enumeration adapter, worker execution for expensive scans; no independent QDirIterator policy rewrite. |
| Common/Extract.cpp — P0-12, P1-01/03/14, P2-01/02/07/08/10 | SetErrorMessage/DecompressArchive/Find_FileName_InSortedVector/Extract: archive sizes and batch loop, Open_Strict/OpenResult, hash-handler guard, volume skip/total correction, output-dir substitution/PE basename/ElimDup, indices/censor/ADS filtering, hard-link prepare, extraction callback close/result precedence and statistics. | Extract options/IExtractCallbackUI, OpenArchive/ArchiveExtractCallback/ExtractingFilePath/HashCalc/LoadCodecs, Wildcard/FileFind/Dir/Name/PropVariantConv; GUI ExtractGUI/CompressCall2. Preserve E_ABORT early exit versus continue-on-open-error, no-files success callback, test+CRC mode, reset progress after Open, close failure only replaces S_OK, multivolume duplicate avoidance and Win zone inheritance. ArchiveBackend official-engine process replaces Extract/Open stream dispatch; copied orchestration/path/callback policy stays. Output containment validation supplements this code and belongs backend. |

Extract uses `CompareFileNames` both for sorting and binary volume-path search;
its documented past case-sensitivity fix is relevant regression evidence.
EnumDirItems distinguishes SymLinks mode from following links, and its recursion
decisions depend on explicit censor subnodes. Preserve these cases before any
platform service adaptation; identical root/folder display alone proves nothing.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 105 unique paths
(30 mirrored + 75 missing), оба exact-source files существуют. Runtime/UI не
менялись; build/CTest/layout не запускались. Scan/extract adapters и platform
behavior остаются открытыми до functional ports и regression checks.

## P0-01.3d3b — Common extraction callback (2026-09-28)

Project baseline `9289748`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитан полный source, проверены callback header/lifecycle helpers и связи с
Extract/ArchiveOpenCallback/Agent. Copied scope пуст. Это обязательная functional
policy, включая работу с filesystem; engine transport требует narrow adaptation.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/ArchiveExtractCallback.cpp — P0-10/12, P1-01/03/11/13/14, P2-01/02/07/10 | COutStreamWithHash, privileges/office-zone helpers, CHardLinkNode/PrepareHardLinks; CArchiveExtractCallback Init/progress/CreateComplexDirectory/time/error/property helpers; CLinkLevelsInfo/IsSafePath/CensorNode_CheckPath/2; ReadLink/owner/Read_fi_Props/CorrectPathParts/CreateFolders/CheckExistFile/GetExtractStream/GetItem/GetStream/PrepareOperation/CloseFile; link filesystem guards/CreateHardLink2/SetLink/SetLink2/DeleteLinkFileAlways_or_RemoveEmptyDir, CLinkInfo parsing/normalization; CloseReparseAndFile/attributes/security/SetOperationResult/ReportExtractResult/password/hash disk-stream/RequestMemoryUse; CDirPathSortPair/CFiTimesCAM/SetPostLinks/SetDirsTimes/CloseArc. | OpenArchive/ExtractingFilePath/PropIDUtils/HashCalc/FilePathAutoRename/ItemNameUtils, MyCom/ComTry/upstream strings/vectors/UTFConvert/Wildcard, Alloc/CpuArch, FileDir/Find/Name/IO/Time/PropVariantConv/SecurityUtils/streams. Preserve metadata types/defaults, inode+stream-id hard-link grouping, overwrite answers and mode changes, incoming/existing auto-rename and renamed-parent ADS mapping, path-prefix removal modes, split handling, link-data limit, preallocation/truncation and close HRESULT precedence. Preserve deferred links/empty placeholders, dangerous-level/path-through-link guards, archive-root-relative link normalization, Windows/WSL/POSIX translation, zone-before-timestamps, owner/ACL/attribute ordering, hash/stat reporting and memory limit callback flags. Backend engine/filesystem/event adapters own execution; Qt dialogs supply decisions, never duplicate algorithms. |

CloseArc выполняет CloseReparseAndFile → SetPostLinks → SetDirsTimes, сохраняя
первую ошибку и clearing arc; closer header отвечает за завершение даже при
early return. CheckExistFile отличает skip/failed deletion return S_OK от
cancel E_ABORT/rename E_FAIL; эти distinctions нельзя потерять в общем статусе.
Link safety — не только lexical sanitization: SetLink2 проверяет виртуальный
target и существующие filesystem links по dangerous-level policy. Обязательный
backend containment слой нужно сверить с ней, а не заново написать вместо тела.
Active GetStream отвергает split output как unsupported, хотя GetExtractStream
содержит split branches; scope обоих фиксируется без заявления реализации.
Commented MyCopyFile/copy-link и delayed-symlink legacy не активные requirements.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 106 unique paths
(30 mirrored + 76 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Callback/process integration и filesystem
success/failure/cancel/Unicode/malicious-path regression остаются обязательными.

## P0-01.3d3c — Common update transaction (2026-09-28)

Project baseline `1a5f850`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный source и Update.h options/callback interfaces; consumers —
GUI/UpdateGUI и Common/CompressCall2. Copied scope пуст; compression transport
остаётся official engine, transaction/command policy требует source port.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/Update.cpp — P0-10/12, P1-04/05/09, P2-01/02/06/07/10, P3-01/05 | CUpdateErrorInfo helpers, CMultiOutStream_Bunch cleanup/deletion control, CArchivePath parse/final/volume/temp paths, CUpdateOptions InitFormatIndex/SetArcPath, CUpdateProduceCallbackImp, CRenamePair Prepare/CompareTwoNames/GetNewPath, Compress, Censor_AreAllAllowed/Censor_CheckPath/EnumerateInArchiveItems, move-progress proxy и UpdateArchive including Windows MAPI branches. | UpdateAction/Pair/Produce/Callback/DirItem/EnumDirItems/OpenArchive/SetProperties/TempFiles/LoadCodecs, MyCom/strings/vectors/Wildcard, FileDir/Find/Name/Time/DLL/PropVariantConv, FileStreams/LimitedStreams/MultiOutStream/CopyCoder/MAPI. Preserve smart/exact/add naming, format capability/type/readonly/tail/multivolume guards, rename first-match/ADS/anti props, action pair production/stats, scan decisions and parent metadata. Preserve unique-new temp creation, SFX module lookup/copy, existing prefix/tail preservation, latest-time tracking, UpdateItems→all-inputs-closed→flush/close→processed status order. Backend process/filesystem/platform adapters replace transport/services; widgets pass typed options and callbacks. |

UpdateArchive после success закрывает input archive, отключает deletion output
volumes/temp и только затем replaces original with temp. При move failure/cancel
после удаления original сохраняет temp; при new-archive cancel может включить
cleanup. MoveArc_Finish может вернуть delayed E_ABORT; не превращать его в
безусловный success и не удалять sources после него. DeleteAfterCompressing
ограничен одним compress-action command, использует processed status и повторную
проверку size/ctime/mtime/link before delete; folders удаляются в depth order,
только empty. Это обязательная safety policy, а не удобство Qt filesystem API.
Windows email workflow вызывает MAPI_DIALOG с attachment lifetime/current-dir
restorer; portable integration/acceptance P3-01/05 остаётся open, не N/A.
Известное source FIXME self-inclusion сравнивает paths без canonicalization:
backend safety adaptation требует отдельного regression и documented deviation.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 107 unique paths
(30 mirrored + 77 missing), exact source path существует. Runtime/UI не менялись;
build/CTest/layout не запускались. Transaction success/failure/cancel, prefix,
volumes/SFX и delete-after regression остаются обязательными functional checks.

## P0-01.3d3d — Common update callback (2026-09-28)

Project baseline `5063a41`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные UpdateCallback.cpp/.h; consumer Update.cpp проверен в d3c.
Copied scope пуст; все активные callback paths остаются обязательными.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/UpdateCallback.cpp — P0-10/12, P1-04/05/09, P2-01/02/06/07/10, P3-05 | CArchiveUpdateCallback constructor, progress/ratio, GetUpdateItemInfo/GetRootProp/GetParent/raw props, GetProperty, UpdateProcessedItemStatus/GetStream2/GetStream, operation/extract reporting, volume streams, password callbacks, InFileStream_On_Error/On_Destroy; Windows GetRelativePath. | UpdatePair/Produce, DirItems/OpenArchive, MyCom/ComTry/strings/vectors/UTF/Wildcard, FileStreams/StreamObjects/ItemNameUtils, FileDir/Name/PropVariant/SecurityUtils/Synchronization. Preserve arc-vs-disk property precedence, renamed names/comments, anti item properties and empty file streams, root/security/reparse/owner/device metadata, hardlink identity map, per-mode stdin and preserve-atime handling. Backend owns typed operation events/streams and engine transport; Qt handles callback presentation only. |

GetStream2 checks cancellation before opening and reports open errors through UI;
retry without preserve-atime precedes final failure. StopAfterOpenError and resource
exhaustion (Windows ERROR_NO_SYSTEM_RESOURCES / POSIX EMFILE) must terminate even
when UI accepts continuing. Reparse data uses memory streams; processed status
is set for supplied file/reparse streams, not as proof of transaction success.
Update.cpp additionally gates deletion on successful completion and metadata.
Open-file index/path registration precedes callback attachment for exception
safety; destruction removes registration without throwing and accumulates latest
mtime under lock. Preserve lifetime and thread safety across engine integration.
Reporting uses archive server indices through CArc, not ArcItems indices (v26.03);
out-archive extract-result reporting returns E_FAIL. Volumes repeat the last size
and create new numbered files; password defined-state remains distinct from empty
password. Commented arc-property/report-finished interfaces and debug branches
are inactive baseline code, not missing mandatory features.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 108 unique paths
(30 mirrored + 78 missing); exact source path exists. Runtime/UI unchanged,
build/CTest/layout не запускались. Stream failure/cancel/lifetime, metadata,
password and update deletion regressions remain required during functional port.

## P0-01.3d4a — Common benchmark integration (2026-09-28)

Project baseline `153e222`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полный Bench.cpp (5036 lines), Bench.h и GUI/BenchmarkDialog call sites;
Console/BenchCon delegates to Bench. Copied scope пуст; GUI source scope записан
в P0-01.3c2. Benchmark algorithms are upstream source, not a Qt replacement.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/Bench.cpp — P2-04, P2-10, P3-05 | CBaseRandomGenerator/CBenchRandomGenerator, benchmark/CRC streams, platform clocks/user time, CBenchInfoCalc/ProgressStatus/ProgressInfo, CBenchInfo rating/usage/speed, CBenchProps, CAffinityMode, CEncoderInfo Generate/Encode/Decode, MethodBench, thread flushers, GetBenchMemoryUsage, CRC/hash/frequency benches, method/hash tables, print helpers, CTotalBenchRes, TotalBench/TotalBench_Hash and Bench property/mode/iteration orchestration. | Bench.h/CreateCoder/MethodProps/StreamObjects/StreamUtils/Property; C CRC/CpuArch/RotateDefs; Common buffers/strings/Wildcard; Windows portable Thread/Synchronization/SystemInfo/FileFind/FileIO and POSIX timing. Preserve deterministic generated data/salts, CRC verification and size/finish checks, coder properties/password cache warmup, filter versus coder paths, affinity/groups and LZMA encoder/decoder thread pairing, complexity and weighted totals. Engine codec/hash factories remain official; backend owns execution and typed results, Qt owns controls/callback display. |

CBenchInfo NumIterations means threads during progress, iterations after Code;
do not multiply partial and final values identically. Preserve 16-bit usage scale,
minimum dictionary complexity, divide-by-zero guards and saturation. Ready/start
barriers exclude generation from measurements; RAII flushers start in exit mode
and join created threads on failure. Hash buffer freeing occurs outside measured
interval. Keep cancellation/error propagation and final-result flags; E_NOTIMPL
is skipped only in total method/hash suites, not blanket benchmark success.
GUI callback selects old LZMA benchmark; text callback supports hash/filter
bandwidth, wildcard totals, file input, dictionary/thread sweeps and CPU frequency
phases. Memory-driven dictionary reduction and limits are functional policy.
Current 7zz transport needs a narrow engine execution/result adapter retaining
these bodies and GUI formulas; exact callback/transport capability remains open
for P2-04, not an authorization to reproduce them from console text. Platform
clock/CPU/group/large-page services require scoped adapters, no GUI global changes.
Commented debug/alternative hash-update/CRC routines are inactive baseline paths.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 109 unique paths
(30 mirrored + 79 missing), exact source exists. No runtime/UI changes or
build/CTest/layout runs. P2-04 still requires functional and geometry checks;
Windows/macOS/Linux native benchmark parity remains unverified.

## P0-01.3d4b — Common hash calculation and checksum handler (2026-09-28)

Project baseline `8a1f3e2`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны полные HashCalc.cpp/.h; GUI/HashGUI callbacks/worker и Extract IHashCalc
integration установлены. Copied scope пуст, codec/hash primitives — official engine.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/HashCalc.cpp — P2-03, P2-01, P0-04/12, P1-04/05/09, P3-05 | CHashBundle methods/init/update/final, CHasherState digest sums/formatting, HashCalc scan/read/callback orchestration, HashHexToString, filename escape/tag/line helpers, NHash::CHashPair parsing, CHandler properties/Open/Close/Extract/UpdateItems/SetProperties, method inference and Codecs_AddHashArcHandler. | CreateCoder/MethodProps/DirItem/EnumDirItems/IFileExtractCallback/LoadCodecs, FileStreams/ProgressUtils/StreamObjects/StreamUtils/ItemNameUtils/IArchive, UTF/strings/DynLimBuf, C Alloc/CpuArch. Preserve sorted unique method IDs/default CRC32/wildcard expansion, digest limit, four digest groups/extra carry bytes, directory/file/ADS totals, name hashing/path separator normalization, scan policy/reparse streams/preserve-atime/share-for-write. Backend owns streams/codec transport/censor/callbacks; Qt owns progress and result presentation. |

HashHexToString reverses uppercase byte order for <=8-byte digests, preserves
lowercase order for longer digests; summed digests append extra bytes when group
has multiple members. Final hashes 16-byte type prefix, digest and two-byte path
units; preserve source algorithm rather than invent a cross-platform encoding.
HashCalc opens skip only on callback S_FALSE, updates totals for file growth,
propagates read/cancel errors and records scan errors in bundle. Its direct
checksum-file generation branch is commented; active writer is CHandler::UpdateItems.
Handler accepts GNU/BSD/tag/cksum/PGP and zero/CRLF lines, backslash escape policy,
filename/size/method inference; PGP checksum metadata is not signature verification.
CanUpdate disables opened PGP/cksum mutation; keep checks and output flags.
Extract requires external input callback, validates digest and optional stored
size, reports unsupported U/^ modes/methods separately from CRCError, and may copy
external input to an output stream. Path resolution must reuse extraction safety
boundary; checksum entries cannot authorize arbitrary filesystem reads/writes.
Update preserves old FullLine or regenerates renamed properties, inherits flags
unless explicitly set, bounds output and writes after complete generation.
GetStream S_FALSE sets needWrite=false but the baseline still emits a digest line;
do not silently change that flow during copying. Any corrective adaptation needs
its own regression/deviation record. Handler registration is application checksum
integration, not a new hash or compression format implementation. Qt adapters
must retain this scope even when 7zz provides hash primitives. Commented report
interfaces/debug/default-method alternatives are inactive, active branches open.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`, 110 unique paths
(30 mirrored + 80 missing), exact source exists. Runtime/UI unchanged;
build/CTest/layout not run. Known digests, sums/names/ADS, escaped Unicode,
malicious checksum paths, errors/cancel and handler lifecycle remain mandatory
functional tests; native parity across three platforms remains unverified.

## P0-01.3d4c — Common command entrypoints and inventory coverage (2026-09-28)

Project baseline `b35a90a`. Exact source — [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Прочитаны оба полных .cpp и общий CompressCall.h (CompressCall2.h отсутствует);
FM Panel/MyLoadMenu/PanelDrag и Explorer ContextMenu callers, FM/Explorer makefile.
Copied scope пуст; это альтернативные реализации одного интерфейса, не два
владельца одного алгоритма в проекте. FM/Explorer makefiles выбирают CompressCall;
Compile scope второго варианта отдельно подтверждается при его functional port.

| Путь / owner | Обязательный source scope | Dependencies и адаптация |
| --- | --- | --- |
| Common/CompressCall.cpp — P0-03/10/12, P1-04/05/16, P2-03/04/09, P3-05 | GetQuotedString/error helpers, Call7zGui, AddLagePagesSwitch, CRandNameGenerator/CreateMap, CompressFiles, ExtractGroupCommand/ExtractArchives/TestArchives/CalcChecksum/Benchmark. | CompressCall.h, Random/strings/MyCom, Windows DLL/ErrorMsg/FileDir/FileMapping/MemoryLock/ProcessUtils/Synchronization, FM RegistryUtils. Preserve add/exact naming, format/email/show-dialog options, grouped archive/hash inputs, ElimDup/zone, hash-file versus digest command, total benchmark dispatch and process/event wait semantics. Adapt Windows mapping/event handshake and 7zG.exe module discovery to typed backend operation/lifetime transport; command syntax remains ArchiveBackend-owned. |
| Common/CompressCall2.cpp — P0-03/10/12, P1-04/05, P2-03/04, P3-05 | GetQuotedString/error helpers, ThrowException_if_Error, CREATE_CODECS/LOAD_EXTERNAL_CODECS, CompressFiles, ExtractGroupCommand/ExtractArchives/TestArchives/CalcChecksum/Benchmark. | Shared CompressCall.h; MyException/EnumDirItems/LangUtils, GUI BenchmarkDialog/ExtractGUI/UpdateGUI/HashGUI/ExtractRes, CCodecs/hash handler/censor/update/extract options. Preserve InitFormatIndex/SetArcPath validation, no-wildcard input censor, archive enumeration/sort/full paths, MultiArcMode, callback failed-file/IsOK checks, already-displayed errors and silent E_ABORT. Replace HWND/messagebox with Qt parent/error presenter and backend execution; preserve option/control flow bodies. |

CreateMap writes initial zero plus nul-terminated UTF-16 names and adds map size/
event to include switch. Non-waiting caller still waits for either process exit
or reader event before releasing mapping; portable listfile/IPC must retain
equivalent input lifetime and filename semantics. Call7zGui returns launch/wait
success without inspecting child exit status; backend must separately report
operation failures rather than claim completed archive from process creation.
Large-page registry/risk decision is platform policy; no unconditional enable.
CompressCall2 outer !Z7_EXTERNAL_CODECS guard makes its inner external-codecs
macro branch inactive in this baseline. waitFinish is unused in direct GUI
variant; do not fabricate asynchronous completion equivalence. Both variants
route checksum-file creation through hash Update workflow; digest-only path is
HashCalcGUI/command h. Benchmark total mode sets method wildcard. Read/source
audit does not authorize sending email or installing shell integration.
Choose one authoritative adapted entrypoint per workflow; retain relevant policy
from both variants locally behind process/platform adapters. No active mandatory
command is N/A merely because Windows 7zG/IPC is unavailable on macOS/Linux.

Проверки: 2026-09-28 macOS docs-only, `git diff --check`; 112 unique exact-source
paths = all 109 UI modules + 3 supporting modules, all 30 local paths covered,
82 missing UI modules assigned audit decisions. No runtime/UI changes or
build/CTest/layout runs. Parent P0-01 awaits .4 reconciliation; functional ports
and native three-platform checks remain open.

## Активация общего списка и выбор при наведении (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/UI/FileManager/ListViewDialog.cpp::{OnInit,OnNotify}`: перенесена
оставшаяся ветка CFmSettings.Load/SingleClick → ONECLICKACTIVATE/TRACKSELECT,
сохранён безусловный FULLROWSELECT. Активация делегирует существующему OnEnter:
один столбец подтверждает focused path, Alt или два столбца показывают detail.
Qt itemClicked/itemDoubleClicked заменяют LVN_ITEMACTIVATE/NM_DBLCLK и глобальный
флаг версии common controls; Ctrl/Shift/Meta исключены из активации, поскольку
Qt clicked приходит и для жестов выделения. Клавиатурная активация не изменена.

`qtlistview.cpp/.h` — новая узкая presentation-адаптация TRACKSELECT, для которого
Qt не имеет прямого аналога: dwell 400ms, выбор всей строки без открытия,
остановка при leave/press/modifiers/drag, QPersistentModelIndex защищает от
удаления hovered row, повторная проверка позиции — от scroll/reorder.
Win32 default hover time не запрашивается: фиксированный Qt interval является
явным платформенным отличием. Основная панель table/icon view пока не получила
hover/track-select. Функциональные тела списка/clipboard/delete/detail сохранены.

FmSettingsPath — общий Qt/INI path adapter, перенесён из MainWindow без изменения
политики override/AppConfigLocation/toolbar.ini; такого пути нет в registry upstream.
FoldersHistory передаёт explicit SettingsPath, остальные вызывающие используют
общий adapter. Не добавлена повторная реализация config location в dialog.
Проверки list_activation покрывают single/double, modifiers, Alt/properties detail,
hover on/off, leave и удаление row; прямые consumers — list_view_dialog,
folder_history, properties, settings, fm_settings. Layout не изменён.

## Настройки файловых списков (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/UI/FileManager/RegistryUtils.cpp/.h::CFmSettings::{Load,Save}`
перенесены в исходные пути `src/`: семь полей, defaults=false, имена ключей,
порядок чтения/записи и сохранение default при некорректном значении сохранены.
QSettings INI group FM заменяет HKEY_CURRENT_USER FM; путь передаётся явно.
ReadOption принимает bool/UInt32 и проверяет !=0 как UINT32ToBool вместо registry DWORD. Остальные функции
RegistryUtils (editor/language/flat view/large pages) не перенесены.
Старые самостоятельные ключи list/* не читаются и не мигрируются.

`SettingsPage.cpp/.h::{OnInit,OnApply,OnButtonClicked}` перенесены частично:
ShowRealFileIcons/ShowGrid/SingleClick, флаги init/changed, чтение при
инициализации и запись только изменённой страницы при подтверждении.
Qt QDialog/check boxes/signals заменяют CPropertyPage, CheckButton и notifications;
OnApply сохраняет snapshot пропущенных полей вместо чтения отсутствующих controls.
Cancel ничего не сохраняет. Отдельный Apply и OptionsDialog с шестью страницами
отсутствуют. Не перенесены controls/ветки ShowDots, FullRow, AlternativeSelection,
ShowSystemMenu, large pages, extraction RAM, help и Lang. Порядок трёх controls
взят из SettingsPage2.rc; самостоятельная settingsdialog.cpp удалена,
header оставлен как alias. MainWindow адаптирует поддерживаемые флаги к Qt.
CFmSettings принадлежит sevenzip_common, CSettingsPage — sevenzip_panel.
Notice/лицензия сохранены; форматирование адаптаций — clang-format.

Проверки: fm_settings (defaults, invalid INI, round trip, cancel/confirm,
сохранение пропущенных flags, отсутствие записи без изменений, геометрия),
settings (применение и один щелчок в четырёх режимах disk/archive).
Геометрия/JSON проверяют структуру Qt; Windows rendering не проверен.

## Запрос пароля и группировка Compress после сверки диалогов (2026-09-28)

Baseline: официальный [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

`CPP/7zip/UI/FileManager/PasswordDialog.cpp/.h/.rc`: перенесены
`CPasswordDialog::{ReadControls,SetTextSpec,OnInit,OnButtonClicked,OnOK}`
в исходные относительные пути `src/CPP/7zip/UI/FileManager/PasswordDialog.*`.
Сохранён порядок чтения Password/ShowPassword, обновления password char/text,
инициализации checkbox и чтения при OK. Win32 controls, resource attachment,
Create/parent HWND и routing buttonID заменены Qt controls/layouts, OnInit/exec
и отдельным соединением clicked. UString заменён QString на границе формы.
Lang/иконки/Win32 базовые обработчики не перенесены; подписи русские, кнопки
стандартные Qt. Модуль принадлежит sevenzip_panel, не backend.

QInputDialog в `mainwindow_backendevents.cpp` заменён этим диалогом. Прежние
правила cancel/empty request и lifetime пароля сохраняются; пароль/ShowPassword
не сохраняются в INI. Отмена после переключения ShowPassword отбрасывает
весь результат формы, хотя upstream ReadControls обновляет её публичные поля.
Generated reference содержит полный commit и хеши resource/shared include/source;
password contract сопоставляет все пять controls. Это структурная проверка,
не проверка Windows rendering. LGPL notice сохранён; лицензия —
`third_party/7zip/License.txt`.

`CPP/7zip/UI/GUI/CompressDialog.rc::IDD_COMPRESS`: исправлено Qt presentation
расположение поддерживаемых controls — compression слева, encryption справа.
Ресурс размещает password labels над edit; Qt encryption form теперь делает то же.
Дополнительный source picker следует за основной формой, чтобы не вытеснять
archive/options при открытии. Qt scroll area, wrapping длинных compression rows,
отдельное destination field, группировка и отсутствие неподдерживаемых controls
остаются явно ограниченным представлением исходного ресурса. Новые функциональные
алгоритмы Compress не добавлены, provenance уже перенесённых методов не меняется.

Проверки: password_dialog, ui_parity_password, ui_archive_states,
compression_encryption, ui_geometry; последний проверяет оба столбца на двух
размерах и в 7z/ZIP. Полная сверка и оставшийся долг —
[аудит диалогов](7zfm-parity.md#аудит-диалогов-2026-09-28).

## Шифрование и пароль Add to Archive (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/GUI/CompressDialog.cpp/.h/.rc`,
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h` и `GUI/UpdateGUI.cpp::SetOutProperties`.

В `src/CPP/7zip/UI/GUI/CompressDialog.cpp/.h` перенесены
`CCompressDialog::{SetEncryptionMethod,GetEncryptionMethodSpec,FindRegistryFormat}`
и `IsAsciiString`. Qt preferences делегируют поиск тому же исходному helper.
Исходный password-блок `OnOK` выделен в `CheckPassword`, чтобы UI и backend
использовали один алгоритм. Сохранены UString, порядок проверок ZIP ASCII
(0x20–0x7F), AES >99, сравнение повторного пароля только при скрытом пароле,
7z AES-only, ZIP ZipCrypto/AES, восстановление aes-prefix из CFormatOptions,
default-index и RemoveChar('-'). Qt QStringList/index заменяют ComboBox;
FindRegistryFormat использует snapshot CInfo вместо контролов.

`createarchivedialog_encryption.cpp` — Qt presentation/glue для
`OnInit`, `UpdatePasswordControl`, соответствующих частей `FormatChanged` и
`SaveOptionsInMem`: повторное поле и label скрываются при показе, значение
сохраняется; отдельный флажок EncryptHeaders доступен только для 7z.
Исходный порядок password/reenter/show/method/headers взят из CompressDialog.rc.
QFormLayout заменяет Win32 controls. Формирование таблиц/валидация остаются в
исходном GUI-модуле. CFormatOptions.EncryptionMethod и CInfo.ShowPassword/
EncryptHeaders сохраняются через QSettings вместо Registry; пароль не хранится.

Backend повторяет перенесённый password-блок до stage. `UpdateGUI::SetOutProperties`
передаёт em только для непустого spec; адаптер 7zz делает то же через -mem,
поэтому пустой ZIP spec даёт исходный ZipCrypto вместо прежнего принудительного
AES. Header encryption передаётся существующим -mhe. Валидация доступных em и
ZIP/header combinations — интеграционная проверка двух поддерживаемых форматов.
Исходные UI error messages заменены локализованными Qt-сообщениями; возврат error
enum заменяет ShowErrorMessageHwndRes, остальной OnOK остаётся частичным портом.
Нет password initialization из существующего архива, SFX/внешних кодеков и
других частей полного CompressDialog. Форматирование — clang-format.

История длинных путей больше не увеличивает minimum width всей формы:
AdjustToMinimumContentsLengthWithIcon сохраняет редактируемую строку и полный
текст в dropdown. Это необходимая Qt-адаптация resize, обнаруженная геометрией.

## Путь архива, история и архиватор (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/GUI/CompressDialog.cpp/.h` и
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h`; адаптированные владельцы сохраняют пути в `src/`.

`CCompressDialog::{SetArchiveName,SetArchiveName2,GetFinalPath_Smart,
SetArcPathFields,ArcPath_WasChanged}` и `GetExtDotPos` перенесены для 7z/ZIP:
сохранены поиск точки после разделителя, исключение ведущей точки,
удаление последнего расширения при смене формата, Trim, fallback StartDirPrefix,
разбор выбранной истории и ветка `always`. Qt snapshot текста заменяет Win32
ComboBox; QString — тексты этой границы, UString сохранён для исходных
операций расширения; QDir/QFileInfo — адаптеры
NName::GetFullPath/GetFullPathAndSplit. Проверка NUL необходима на границе native API.
Метаданные двух доступных кодеков заменяют CArcInfoEx; оба имеют KeepName=false.
SFX, KeepName=true, OriginalFileName, hash handlers, внешние кодеки и
OnButtonSetArchive пока не перенесены.

`CCompressDialog::OnOK` содержит только исходный блок истории/ArcType: сначала
новый полный путь, затем прежние уникальные пути до 20. Чтение контролов и Save
остаются в Qt-обвязке. Локальный AddUniqueString сохраняет алгоритм одноимённого
helper из ExtractDialog.cpp без зависимости rules/backend от виджета извлечения.
Полный OnOK ещё не перенесён. `NCompression::CInfo::{Save,Load}` получили исходные
ArcPaths/ArcType, defaults и ключи ArcHistory/Archiver; QSettings INI заменяет
Registry, остальные группы сохраняются.

`createarchivedialog.cpp` остаётся presentation/integration glue с неперенесёнными
частями формы. Combo показывает полное имя с расширением; activated заменяет
CBN_SELCHANGE/PostMessage. Backend определяет формат по суффиксу, поэтому Qt
подтверждает только совпадающие с выбранным форматом .7z/.zip, существующего
родителя и незанятую цель; upstream допускает более общий путь.
Форматирование — обязательный clang-format.

## Свойства и двухколоночный список (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Пути ниже относительно `CPP/7zip/UI/FileManager/`; адаптированные файлы — в `src/`.

| Исходник и символы | Заимствование и адаптация |
| --- | --- |
| `EditDialog.cpp/.h/.rc::CEditDialog` | Перенесён весь модуль: UString Title/Text и OnInit; QPlainTextEdit/layout заменяют Win32 attachment/OnSize, Qt-локализация — LangSetDlgItems, event filter — ES_WANTRETURN. ShowItemInfo сохраняет обе ветки и Windows-only LF→CRLF при отсутствии CR. |
| `PanelMenu.cpp::{AddListAscii,AddSeparator,AddPropertyPair,CPanel::Properties}` | Перенесены перечисление свойств и сводка single/multiple selection с UStringVector и UInt64 totals. CPanelItem/CPanelProperty заменяют COM metadata снимком доступных полей, Qt — LangString и создание modal. Числа форматируют ConvertSizeToString/ConvertUInt64ToString. |
| `PanelItems.cpp::{GetItem_UInt64Prop,GetItemSize}` | Сохранены parent-index, пустое свойство, ошибка 2723400 и делегирование GetItemSize; snapshot заменяет COM/variant. |
| `ListViewDialog.cpp/.h::{OnInit,CopyToClipboard,ShowItemInfo,DeleteItems,OnEnter}` | Перенесены NumColumns/Values, усечение display copy, raw copy/detail и параллельное удаление Strings/Values. `qtlistview.h` адаптирует CListView к QTreeWidget; полный текст показывает перенесённый CEditDialog. Лимит 1024 wchar_t зависит от ширины wchar_t платформы. |

Не перенесены raw/security/flags/errors, полный ConvertPropertyToString2,
IFolderProperties/IFolderArcProps levels, non-open level, IFolderGetItemName
fast path, остальные команды PanelMenu/PanelItems, single-click/hover и
Win32 notifications. Qt-список заменяет filesystem shell properties.
`third_party/7zip/CPP/7zip/PropID.h` скопирован без изменений из того же baseline;
адаптированные файлы форматируются clang-format.

## История папок и общий список (2026-09-28)

Следующая итерация переносит `AppState.h::CFastFolders::{SetString,GetString,
Save,Read}` и `PanelFolderChange.cpp::CPanel::{SetBookmark,OpenBookmark}`
из того же полного baseline ниже. Сохранены sparse vector, заполнение пустыми
строками до индекса и отсутствие изменения state при GetString вне списка.
Qt adapter передаёт полный отображаемый prefix; OpenBookmark возвращает его
для общего navigateTo вместо Win32 BindToPathAndRefresh. Save/Read используют
`ViewSettings.cpp/.h::{SaveFastFolders,ReadFastFolders}`, ключ FolderShortcuts.
Критическая секция исключена по той же причине GUI-thread ownership.
Двухполевая структура Bookmark и самостоятельное ветвление открытия удалены.
Исчезнувшая внутренняя папка открывает ближайший существующий parent через
общую навигацию, включая уже открытый архив без повторного чтения.
MVP-настройки history/paths и favorites/* больше не читаются по указанию
пользователя; миграционного fallback нет. Остальной CAppState пока не перенесён.

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Из `CPP/7zip/UI/FileManager/AppState.h` перенесён `CFolderHistory`;
из `App.cpp` — `AddUniqueStringToHead`, `Normalize`, `AddString` с исходным
пределом 100 и сравнением `UString::IsEqualTo_NoCase`. Имена, вектор,
порядок удаления дублей и вставки сохранены. State принадлежит GUI thread,
поэтому Win32 critical section отсутствует; Save/Read принимают путь INI.
`ViewSettings.cpp/.h::{SaveFolderHistory,ReadFolderHistory}` сохраняют исходный
ключ FolderHistory; настройки прежнего Qt MVP игнорируются.

Из `ListViewDialog.cpp/.h/.rc` перенесена одноколоночная ветка:
OnInit, ListView_GetSelected, DeleteItems, CopyToClipboard, ShowItemInfo,
OnEnter, OnOK и клавиши OnNotify. QListWidget/event filter/layout/clipboard
заменяют Win32 controls/OnSize/notifications. Сохранены множественный выбор,
удаление первого выбранного до исчерпания выбора, флаг изменений, индекс
фокуса и платформенный перевод строки clipboard. Meta добавлен для macOS.
ShowItemInfo использует перенесённый CEditDialog.
NumColumns/Values и усечение значений перенесены следующей
итерацией выше; Settings single-click/hover пока пропущен.
Qt выбирает surviving current row после удаления; Win32 focus восстановить
буквально нельзя. Отдельная кнопка удаления MVP убрана по исходному rc.

`PanelFolderChange.cpp::CPanel::FoldersHistory` сохраняет ветки OK/Cancel,
обратную вставку отредактированного списка и выбор focused item. State/parent
передаются аргументами вместо _appState/GetParent, путь возвращается Qt adapter
вместо BindToPathAndRefresh. Остальные функции модуля пока не перенесены.
MainWindow хранит history отдельно от combo; combo — только presentation.

Неизменённые зависимости взяты из того же baseline в `third_party/7zip/`:
`CPP/Common/{MyString.h,MyString.cpp,IntToString.h,IntToString.cpp,
StringConvert.h,StringConvert.cpp,UTFConvert.h,UTFConvert.cpp,MyBuffer.h,
MyWindows.cpp}` и `C/CpuArch.h`. Они сохраняют upstream notices; остальные
зависимости MyVector/MyTypes/Common/7zTypes уже присутствовали.
MyWindows обеспечивает BSTR на POSIX, Windows использует oleaut32.
Преобразование QString↔UString ограничено Qt/settings boundary и учитывает
разную ширину wchar_t. Форматирование адаптированных файлов — clang-format,
vendored sources не переформатированы.

Воспроизводимый baseline новых итераций FSFolder/FileDir, Split/Combine и
Combo/Browse: официальный репозиторий
[`ip7z/7zip` на commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные пути, перенесённые symbols, пропущенные части и адаптации указаны
в разделах ниже. Для обновления каждого частичного модуля сравнивать этот
snapshot с новой ревизией, включая зависимые/пропущенные ветки; форматирование
рассматривать отдельно от functional diff. Соседний checkout остаётся read-only.

## Инструмент сверки UI (2026-09-28)

Инструмент UI-сверки описан в [docs/ui-reference.md](ui-reference.md).
Сохранённый Copy baseline извлечён из той же полной ревизии официального
`ip7z/7zip`: `CopyDialog.rc`, `CopyDialogRes.h`, `CPP/7zip/GuiCommon.rc` и
`CopyDialog.cpp::{OnInit, OnSize, OnButtonClicked, OnButtonSetPath, OnOK}`.
JSON сохраняет ресурсные данные и короткие навигационные выдержки, не
функциональную реализацию. Python-парсер/сверка — новый локальный tooling;
новых заимствованных алгоритмов или изменений Qt presentation в этой задаче нет.
Контракт покрывает шесть контролов и выбранные Copy/Move состояния; полный
source-port audit существующих модулей остаётся в соответствующих разделах.

## Общий ввод имени/маски и создание папки (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ComboDialog.cpp/.h/.rc::{CComboDialog, OnInit, OnOK,
OnSize}` и `BrowseDialog.cpp/.h::Dlg_CreateFolder`.
Адаптированные реализации сохраняют исходные пути и symbols. Прежний
самостоятельный combodialog.cpp удалён; combodialog.h — compatibility alias.

Перенесены Title/Static/Value/Strings, порядок заполнения и получение сырого
Value в OnOK. Qt-конструктор задаёт поля до OnInit; QDialog, QComboBox,
QLabel/layouts заменяют CModalDialog/Win32 controls/OnSize. Qt AddItem выбирает
первую запись, тогда как исходный AddString не меняет введённый Value:
после наполнения восстанавливается исходный edit text. accept проходит через
OnOK; getters — Qt compatibility glue. QStringList используется только на
presentation boundary; никакого нового functional parser не добавлено.

Dlg_CreateFolder скопирован с очисткой destName перед dialog, исходными
Title/Static/default Value, возвратом false при Cancel и Value при OK.
QWidget заменяет HWND, русские строки — LangString. CreateFolder теперь
использует эту функцию, как PanelOperations.cpp; создание файла и SelectSpec
переходят на исходный CComboDialog через alias. Остальная BrowseDialog,
CorrectFsPath, inline Rename и complex-name validation остаются долгом.
Notices/LGPL сохранены; clang-format обязателен. sevenzip_panel компилирует
общий dialog один раз и включает AUTOMOC для Qt meta-object.

## Split dialog и подготовка Combine (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/SplitDialog.cpp/.h/.rc`,
`PanelSplitFile.cpp::{CApp::Split, CApp::Combine, AddInfoFileName}` и
`App.cpp::AddValuePair2`. `CSplitDialog` сохранён в исходном относительном пути;
самостоятельные volumedialog.cpp/.h удалены.

Перенесены OnInit (имя в title, Path, AddVolumeItems, выбор первого размера),
OnOK (Path, Trim, ParseVolumeSizes, отказ пустого/ошибочного ряда),
OnButtonSetPath (browse, NormalizeDirPathPrefix, сброс индекса). VolumeSizes
использует исходный CRecordVector<UInt64>; временный std::vector нужен для
текущего адаптированного SplitUtils parser, Qt-worker принимает QList<qint64>.
Предел qint64 сохраняет допустимые смещения QFile. QComboBox/QDialog/layouts
заменяют CModalDialog/Win32 controls/OnSize; native QFileDialog заменяет
MyBrowseForFolder. Inline label заменяет modal error, accept вызывает OnOK.
Qt-конструктор передаёт публичные FilePath/Path перед OnInit.

Блок сводки CApp::Combine скопирован в CThreadCombine::GetInfo в том же
PanelSplitFile.cpp: сохранены AddInfoFileName, AddValuePair2, первые два имени,
условие многоточия и последний файл. Извлечение метода необходимо для
разделения Qt orchestration и исходной логики при отсутствии полной CApp.
AddValuePair2 экспортирован из существующего App.cpp, как в upstream.
CCopyDialog получил конструктор для Title/Static/Value/Info: Combine теперь
использует исходный общий диалог вместо отдельного Qt dialog.

mainwindow_volumes.cpp переносит подготовительные ветки CApp::Split:
повторную проверку источника после dialog, сравнение размера первого тома,
GetNumberOfVolumes и подтверждение >=100. Оба сценария нормализуют назначение
и вызывают CreateComplexDir (QDir::mkpath) после подтверждения. Сохранены
адаптерные ограничения на выбранные symlink, занятые имена, 10000 частей,
транзакционную публикацию и откат. Проверка пропусков и пустых частей остаётся
существующей integration adaptation; discovery loop не объявляется новым
source port. Полный CApp, взаимодействие двух панелей и Win32 progress — долг.
Форматирование — clang-format; notices/LGPL сохранены.

## Мутации файловой системы и рабочий поток удаления (2026-09-28)

Источник: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/FSFolder.cpp::{GetRelPath, GetAbsPath, CreateFolder,
CreateFile, Rename, Delete}`, `FSFolder.h::{CDirItem, CFSFolder}`,
`CPP/Windows/FileDir.cpp::{DeleteFileAlways, RemoveDirWithSubItems}`.
Адаптированные файлы сохранены по исходным путям; notices и LGPL сохранены.

Перенесены ветки mkdir → CreateComplexDir, Create_NEW, Rename с родительским
префиксом, индексный цикл Delete с SetTotal/SetCompleted и рекурсивный обход
RemoveDirWithSubItems. Сохранён исходный порядок файлов, накопление isError
в рекурсивном обходе и частичный результат удаления. Files/Folders и indices
используют исходные CObjectVector/CRecordVector. Остальные методы CFSFolder
не объявляются перенесёнными.

Необходимые изменения: QFile/QDir/QFileInfo заменяют NIO/Win32 filesystem
services; QString — платформенные пути, bool/ErrorMessage — HRESULT и last
error, callbacks — IProgress. AddPath и конструктор формируют снимок панели
Qt, включая Parent для плоского вида; это новая интеграционная обвязка.
Win32 FILE_ATTRIBUTE_READONLY заменён Qt permissions только на Windows:
на Unix удаление зависит от прав родительской папки. QFile удаляет сам symlink;
ветка HasReparsePoint сохраняет запрет обхода его цели. Qt enumeration не
имеет Next(error): проверяется читаемость папки, а rmdir выявляет оставшиеся
записи. Сохраняется существующая политика отказа для непосредственно
выбранного symlink и незамещения dangling symlink при Rename.

FileDeleteOperation и mainwindow_filedelete.cpp — Qt worker/presentation glue:
исходный прогресс i преобразуется в число i+1 для QProgressDialog, отмена
проверяется до удаления каждого объекта и в рекурсии, завершение всегда
обновляет панель, деструктор ожидает поток. Дополнительный CheckBreak нужен,
потому что исходный RemoveDirWithSubItems не принимает IProgress и иначе
большое дерево не реагировало бы на отмену. UI сохраняет прежнее ограничение
создания одним именем; CFSFolder поддерживает исходную ветку составного пути.
Корзина/SHFileOperation, alternate streams и остальные FSFolder методы — долг.
Форматирование — обязательный clang-format.

## Qt layout diagnostics

Reference inspected: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/ExtractDialog.rc` (`IDD_EXTRACT`) and
`CPP/7zip/UI/GUI/CompressDialog.rc` (`IDD_COMPRESS`).
`docs/ui-layout-contracts.md` records control grouping/order and current Qt
adaptations. `tests/layout_inspection.cpp` is new test-only Qt integration glue:
runtime QObject-tree inspection, ancestor-clipped QWidget geometry, and JSON
reports are specific to Qt, so no upstream implementation is copied for this
helper. It does not replace upstream dialog functional logic or establish
pixel-level parity. `ui_geometry` checks reachability, footer geometry, and
Extract column ordering on disposable fixtures.

Источник сравнения: `../7zip-upstream`, commit `0766b73`, прежде всего
`CPP/7zip/UI/FileManager/`. Эта таблица охватывает все производственные файлы
`src/` на 28 сентября 2026 года. «Собственный» означает, что реализация написана
в этом репозитории по поведению 7zFM, а не перенесена из исходного файла.

Цель и обязательные правила переноса заданы в `AGENTS.md`: исходная реализация
копируется с сохранением имени файла, относительного пути, символов и структуры
модуля. UI-представление адаптируется к Qt; остальная логика меняется только там,
где этого требуют платформенные или интеграционные зависимости. Это относится
и к функциональному коду внутри Win32/COM UI-модулей. Собственная реализация
с совпадающим поведением остаётся долгом переноса.

Адаптированные исходники приложения размещаются в `src/<путь-upstream>`,
неизменённые общие зависимости — в `third_party/7zip/<путь-upstream>`.
Для каждого переноса фиксируются commit, исходные пути и символы,
скопированные и адаптированные части, а также причины каждого существенного
отклонения. Одних совпадающих имён недостаточно для признания прямого переноса.
Сохраняются исходные уведомления и применимая лицензия; текст лицензии для уже
перенесённых фрагментов лежит в `third_party/7zip/License.txt` из того же checkout.
Архивные операции сейчас остаются за `ArchiveBackend` и официальным `7zz`.

| Наши файлы | Соответствующий исходник upstream | Текущее состояние |
| --- | --- | --- |
| `src/CPP/7zip/UI/FileManager/PanelSort.cpp`, `.h` | `CPP/7zip/UI/FileManager/PanelSort.cpp::CompareFileNames_ForFolderList`, `CompareItems` | Сравнение имён и порядок «папки перед файлами → свойство → базовое имя → префикс → индекс загрузки» перенесены с сохранением имён; Указательный компаратор скопирован напрямую с заменой Windows `wchar_t` на `char16_t`; перегрузка `QStringView` преобразует вход на границе. Qt item data заменяет свойства `IFolder`. Направление, начальный порядок свойств и расширение по последней точке адаптированы по `PanelSort.cpp`, `FSFolder.cpp` и `Agent.cpp` того же commit. Остальная сортировка по сырым и форматным свойствам пока не перенесена. |
| `src/CPP/Common/Wildcard.cpp`, `.h` | `CPP/Common/Wildcard.cpp::DoesWildcardMatchName`, `EnhancedMaskTest` | Указательный `EnhancedMaskTest` скопирован с заменой Windows `wchar_t` на `char16_t`; перегрузка `QStringView` преобразует вход, Qt выполняет `MyCharUpper`. Для выбора в панели применяется нечувствительное к регистру сравнение 7zFM. |
| `src/CPP/7zip/UI/FileManager/PanelSelect.cpp`, `.h`, `src/mainwindow_selection.cpp` | `CPP/7zip/UI/FileManager/PanelSelect.cpp::SelectSpec`, `SelectByType`, `SelectAll`, `InvertSelection` | Функциональная часть `CPanel` выделена в `CPanelSelection`: методы, порядок ветвей и вектор выбора сохранены. Компонент не зависит от виджетов; `mainwindow_selection.cpp` адаптирует `ComboDialog`, фокус и Qt selection model, публикуя выбор одним обновлением. Альтернативный режим выбора upstream и обработчики мыши/диапазона остаются за Qt и не перенесены. |
| `third_party/7zip/CPP/Common/StringToInt.cpp`, `.h` и их заголовки `Common*.h`, `My*.h`, `NewHandler.h`, `StdAfx.h`, `C/7zTypes.h`, `7zWindows.h`, `Compiler.h`, `Precomp.h` | Те же относительные пути upstream | Побайтовые копии без изменения алгоритмов и форматирования. Собирается `sevenzip_common`; Qt-код использует оригинальный `ConvertStringToUInt64`. |
| `src/CPP/7zip/UI/FileManager/SplitUtils.cpp`, `.h` | `CPP/7zip/UI/FileManager/SplitUtils.cpp` | Перенесены `ParseVolumeSizes`, `GetNumberOfVolumes`, список `k_Sizes` и `AddVolumeItems`. Алгоритмы сохранены; `UString` / `CRecordVector` заменены стандартными контейнерами, Win32 combo — `QComboBox`. Worker применяет последовательность размеров и повторяет последний; диапазон ограничен `qint64`. |
| `src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp`, `.h` | `CPP/7zip/UI/FileManager/PanelSplitFile.cpp::CVolSeqName`, `CThreadCombine::ProcessVirt`, `CThreadSplit::ProcessVirt` | Алгоритм нумерации и циклы Combine/Split перенесены с сохранением имён; `QString` / `QFile` / callback заменяют строки, NIO и progress sync. Проверки изменений входа и flush — платформенная адаптация. |
| `src/CPP/7zip/UI/GUI/CompressDialog.cpp`, `.h` | `CPP/7zip/UI/GUI/CompressDialog.cpp::CFormatInfo`, `g_Formats`, `g_*Methods`, `GetStaticFormatIndex`, `SetMethod2`, `SetDictionary2`, `SetOrder2`, `SetSolidBlockSize2`, `SetNumThreads2`, `GetMemoryUsage_Threads_Dict_DecompMem`, `OnOK` | Таблицы форматов, методов, уровней и флагов взяты из upstream. Qt-преобразование возвращает имена методов с исходным фильтром 7z. Один источник используется UI и backend; SFX и внешние кодеки не представлены в текущем UI. |
| `src/CPP/7zip/UI/Agent/AgentProxy.cpp`, `.h`, `Agent.cpp`, `Agent.h` | `CPP/7zip/UI/Agent/AgentProxy.cpp::CProxyArc`, `CProxyDir`, `Agent.cpp::CAgentFolder::LoadFolder`, `LoadItems`, `GetNumberOfItems` | Перенесены бинарный поиск и отложенное слияние подпапок, дерево явных/неявных папок, итеративный подсчёт размеров, раскрытие выбора в реальные индексы и плоский обход. Qt-строки и стандартные контейнеры заменяют upstream-контейнеры; данные `ArchiveBackend` — COM-свойства. CRC, сырые свойства, `CProxyArc2` и alternate streams не представлены процессным листингом. |
| `src/main.cpp`, `mainwindow.cpp`, `mainwindow.h`, `mainwindow_menus.cpp`, `mainwindow_panel.cpp` | `FM.cpp`, `App.cpp`, `App.h`, `Panel.cpp`, `Panel.h`, `PanelItems.cpp`, `resource.rc` | Qt-оболочка одной панели. Конструктор, сборка меню и сборка панели разделены; перенос правил меню и компонентов панели ещё не завершён. |
| `src/mainwindow_backendevents.cpp` | `PanelCopy.cpp`, `PanelCrc.cpp`, `PanelItemOpen.cpp`, `OpenCallback.cpp`, `ExtractCallback.cpp` | Адаптер асинхронных сигналов `ArchiveBackend`: переходы между этапами хеширования/перемещения, просмотр извлечённого файла и восстановление состояния. Он необходим для процессного backend; функциональные сценарии ещё требуют сверки с исходными callback и panel-модулями. |
| `src/mainwindow_filelist.cpp` | `PanelFolderChange.cpp`, `PanelItems.cpp`, `PanelOperations.cpp` | Собственные переходы по файловой системе, наполнение строк и создание. Нужен перенос по исходным функциональным модулям. Выбор по маске и типу вынесен в `PanelSelect.cpp`. |
| `src/mainwindow_archivelist.cpp` | `PanelItems.cpp`, `PanelSort.cpp`, `PanelMenu.cpp` | Построение дерева, раскрытие выбранных папок и плоский обход делегированы перенесённым `CProxyArc` / `CAgentFolder`. Строки таблицы адаптируют их данные к Qt. Собственные правила доступности команд ещё требуют переноса. |
| `src/mainwindow_archiveopening.cpp` | `PanelFolderChange.cpp`, `PanelItemOpen.cpp` | Собственная цепочка вложенных архивов с временными файлами и backend. Нужно сверить стек папок и открытие по содержимому. |
| `src/mainwindow_archivecommands.cpp` | `PanelCrc.cpp`, `PanelOperations.cpp`, `PanelCopy.cpp`, `PanelItemOpen.cpp` | Собственная связка действий с `ArchiveBackend`; выбор и правила команд требуют сверки. |
| `src/mainwindow_fileoperations.cpp`, `mainwindow_filedelete.cpp` | `PanelOperations.cpp`, `PanelCopy.cpp`, `FSFolder.cpp` | Qt-обвязка подтверждения, рабочего потока и команд. Создание, переименование и удаление делегированы перенесённым методам CFSFolder; остальные правила панели остаются долгом. |
| `src/CPP/7zip/UI/FileManager/PanelKey.cpp` | `PanelKey.cpp::OnKeyDown`, `Panel.cpp::CMyComboBoxEdit::OnMessage`, `App.cpp::CPanelCallbackImp::SetFocusToPath` | Qt-диспетчер клавиш списка и адреса: Shift+F4, Alt+F1/F2, Tab, закладки и делегирование команд выделения. Однопанельный режим сохраняет upstream-правило общей цели Alt+F1/F2. |
| `src/mainwindow_preferences.cpp` | `PanelMenu.cpp`, `PanelSort.cpp`, `PanelFolderChange.cpp`, `App.cpp`, `PanelKey.cpp` | Собственный код меню, сортировки, истории, закладок и событий. Это несколько upstream-модулей в одном файле. |
| `src/CPP/7zip/UI/FileManager/PanelMenu.cpp`, `PanelItems.cpp`, `src/mainwindow_dialogs.cpp`, `propertiesdialog.cpp`, `propertiesdialog.h` | `PanelMenu.cpp::Properties`, `ListViewDialog.cpp`, `PanelItems.cpp::GetItemSize/GetItem_UInt64Prop`, `UI/Agent/AgentProxy.cpp::CalculateSizes` | Перенесены цикл перечисления доступных snapshot properties, сводка выбранных объектов и двухколоночный ListViewDialog. Qt-обвязка только собирает snapshot; размеры папок — CProxyArc. Форматные/сырые свойства и остальные PanelMenu/PanelItems branches отсутствуют. |
| `src/mainwindow_volumes.cpp`, `volumeoperations.cpp/.h`, `src/CPP/7zip/UI/FileManager/SplitDialog.cpp/.h` | `PanelSplitFile.cpp`, `SplitDialog.cpp/.h/.rc`, `CopyDialog.cpp` | CSplitDialog OnInit/OnOK/browse перенесены, Combine использует общий CCopyDialog и исходную сводку. Нумерация, parser, число томов и worker-циклы перенесены ранее; транзакции, проверка путей и discovery остаются Qt-адаптацией. |
| `src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp`, `FSFolder.h` | `CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CCopyStateIO::MyCopyFile`, `CCopyState`, `CopyFolder`, `CopyFile_Ask`, `IsDestChild` | Цикл чтения/записи, накопленный прогресс, рекурсивная copy-ветка и проверка вложенного назначения перенесены с сохранением имён. `QFile`, `QDir` и callbacks заменяют платформенные операции; stage создаётся с `NewOnly`, ошибки записи проверяются `flush`, сохраняется дата. Транзакционная обвязка, move и интерактивный overwrite остаются адаптацией/долгом. |
| `src/filetable.cpp`, `filetable.h` | `PanelItems.cpp`, `PanelSort.cpp`, `PanelDrag.cpp` | Qt-адаптер таблицы и drag-out. Функция сравнения вынесена в зеркальный `PanelSort.cpp`. |
| `src/filesystemscanner.cpp`, `filesystemscanner.h` | `FSFolder.cpp::LoadSubItems`, `Panel.cpp::ChangeFlatMode` | Собственный порционный обход через Qt. Нужна сверка фильтров, ссылок и отмены. |
| `src/filetransferoperation.cpp`, `filetransferoperation.h` | `FSFolderCopy.cpp`, `PanelCopy.cpp` | Qt-worker с временными файлами и откатом. Копирование дерева/байтов и проверка вложенного назначения делегированы перенесённым `CopyFolder`, `CopyFile_Ask`, `CCopyStateIO` / `IsDestChild`. Перемещение, предварительная проверка дерева для rename и транзакционное завершение остаются адаптацией. |
| `src/fileviewerdialog.cpp`, `fileviewerdialog.h` | `PanelItemOpen.cpp`, `EditDialog.cpp`, `EditPage.cpp` | Собственный Qt-редактор и интеграция с backend; в upstream есть отдельные режимы редактора и внешнего открытия. |
| `src/createarchivedialog.cpp`, `createarchivedialog_compression.cpp`, `createarchivedialog_resources.cpp`, `createarchivedialog.h` | `CPP/7zip/UI/GUI/CompressDialog.cpp`, `CompressDialog.rc`, `FileManager/PanelCopy.cpp` | Qt-форма выбора источников и сжатия. Правила уровней (`SetLevel2`) и методы берутся из перенесённого `GUI/CompressDialog.cpp`; Ветки размера слова LZMA/LZMA2, Deflate/Deflate64 и порядка PPMd/PPMdZip перенесены в `SetOrder2`; Словари доступных методов, Auto/Non/Full solid, потоки и оценки памяти перенесены; путь/история, шифрование, пользовательский лимит памяти и профили перенесены последующими итерациями выше; выбор источников и остальные поля полного OnOK остаются долгом. |
| `src/CPP/7zip/UI/GUI/ExtractDialog.cpp`, `.h`, `src/CPP/7zip/UI/Common/ZipRegistry.*` | `CPP/7zip/UI/GUI/ExtractDialog.cpp::CExtractDialog::OnInit`, `OnOK`, `AddComboItems`, `AddUniqueString`; `ZipRegistry.*::NExtract::CInfo` | Исходные таблицы режимов, история и сохранение режимов/показа пароля перенесены. Qt заменяет presentation; `src/extractdialog.h` — compatibility alias, прежняя самостоятельная реализация удалена. SplitDest и SplitPathToParts_Smart перенесены следующей итерацией; ElimDup и NtSecurity остаются долгом. |
| `src/CPP/7zip/UI/FileManager/CopyDialog.*`, `ViewSettings.*`, `App.cpp`, `Panel.h`, `PanelListNotify.cpp`, `src/copydialog.h` | `FileManager/CopyDialog.cpp`, `.h`, `.rc`; `ViewSettings.cpp::ReadCopyHistory`, `SaveCopyHistory`, `AddUniqueStringToHeadOfList`; `App.cpp::Copy`, `CPanel::GetItemsInfoString`; `PanelListNotify.cpp::ConvertSizeToString` | Перенесены CCopyDialog, история, сводка выделения и форматирование чисел. Qt layouts заменяют OnSize, QSettings — Registry, snapshot свойств — COM; старый header содержит только alias. Остальные App/Panel/ListNotify функции пока не перенесены. |
| `src/CPP/7zip/UI/FileManager/ComboDialog.cpp/.h`, `BrowseDialog.cpp/.h`, `src/combodialog.h` | `FileManager/ComboDialog.cpp/.h/.rc`, `BrowseDialog.cpp::Dlg_CreateFolder` | Перенесены OnInit/OnOK и создание папки через общий dialog; Qt заменяет presentation, header alias сохраняет существующих потребителей. Остальная BrowseDialog не перенесена. |
| `src/CPP/7zip/UI/FileManager/SettingsPage.*`, `RegistryUtils.*`, `settingsdialog.h` | `FileManager/SettingsPage.cpp/.h`, `RegistryUtils.cpp/.h`, `SettingsPage2.rc` | Частичный source port страницы и CFmSettings, подробно выше; OptionsDialog и остальные controls остаются долгом. |
| `src/archiveformats.h` | `FileManager/FilePlugins.cpp`, `PanelFolderChange.cpp`, `PanelItemOpen.cpp` | Собственный статический список расширений для эвристики двойного щелчка и фильтра диалога. В upstream расширения приходят от установленных обработчиков; движок сам определяет формат при явном открытии. |
| `src/archivebackend.cpp`, `archivebackend.h`, `archivebackend_listing.cpp`, `archivebackend_mutations.cpp`, `archivebackend_validation.cpp` | `CPP/7zip/UI/Console/Main.cpp`, `CPP/7zip/UI/Common/OpenArchive.cpp`, `Update.cpp` и движок `CPP/7zip/Archive/` | Адаптер процесса `7zz`, парсер его вывода, проверка путей и транзакционное обновление. Не является копией движка; переносить код форматов сюда нельзя. |

Вывод ревизии: строки со статусом «собственный» обозначают долг переноса, а не
принятую конечную архитектуру. Переименование файлов само по себе его не
исправит. Каждый такой блок требует сверки исходных функций, зависимостей и
тестов, переноса переносимой логики и Qt-адаптации платформенных вызовов.

Сравнение имён `PanelSort.cpp::CompareFileNames_ForFolderList` теперь использует
указательный алгоритм напрямую из upstream `0766b73`. Единственная замена
в теле алгоритма — `wchar_t` на `char16_t`, чтобы сохранить UTF-16 Windows
на macOS/Linux. `MyCharUpper` использует Qt; отдельная перегрузка преобразует
`QStringView` в строки с завершающим нулём, включая срезы без терминатора.

Цикл объединения `CThreadCombine::ProcessVirt()` перенесён из upstream
`PanelSplitFile.cpp` (`0766b73`) в тот же относительный путь. Сохранены порядок
частей, чтение до EOF блоками 1 МиБ, проверка полной записи и прогресс после
каждого блока. `QFile` и callback заменяют NIO/HRESULT/CProgressSync. Адаптация
добавляет проверку изменения размера, `flush` и отмену до первого чтения.
`VolumeOperation` владеет проверкой путей, временным именем, публикацией
и откатом; Split делегирует чтение/запись перенесённому `CThreadSplit::ProcessVirt`.

Правила первой части и имени результата Combine используют исходный
`CVolSeqName::ParseName` и перенесённый блок `CApp::Combine` в
`CVolSeqName::GetCombinedName`. Снято собственное ограничение `.001`;
поддерживаются `part01`, `part.01`, `.001` и числовые имена.

Цикл `CThreadSplit::ProcessVirt()` перенесён из `PanelSplitFile.cpp` (`0766b73`)
в тот же модуль: блоки 1 МиБ, ограничение чтения остатком тома, повтор последнего
размера, переход к следующему файлу и прогресс через 4 МиБ либо на границе тома.
Qt-адаптер передаёт список временных путей вместо `VolBasePath`, проверяет размер
источника и сообщает ошибки `flush`. Предварительное выделение длины из
`CPreAllocOutFile` не переносится: Qt пишет временный файл последовательно.
`VolumeOperation::copyBytes` удалён; транзакционная публикация и откат остаются
в `VolumeOperation`.

Ветка LZMA/LZMA2 `CCompressDialog::SetOrder2()` перенесена из upstream
`0766b73` в `src/CPP/7zip/UI/GUI/CompressDialog.cpp`. Сохранены генерация
8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 273 и автоматическое значение
32 для уровней ниже 7, иначе 64. UI и backend используют одну функцию.
Qt-часть формы сжатия выделена в `createarchivedialog_compression.cpp`;
явный выбор сохраняется при смене уровня, автоматический не передаёт `fb`
движку. Ветки порядка Deflate/PPMd подключены последующими итерациями, описанными ниже.

Числовой ряд `CCompressDialog::SetSolidBlockSize2` перенесён из `0766b73`
в исходный `GUI/CompressDialog.cpp`: степени 2^20–2^36. Qt хранит размеры
в МиБ вместо логарифмов; UI и backend используют один источник. Сохраняется
явное значение при смене уровня, для ZIP/Store блок отключён. Подписи
показывают МиБ/ГиБ. Расчёт автоматического размера и отдельный Full Solid
из upstream пока не перенесены; Auto оставляет выбор движку.

Скопированы проверки `fi.Solid_()`/нулевого уровня и числовой цикл `i = 20..36`.
Вместо вставки значений в Win32 `m_Solid` функция возвращает данные для Qt combo
и проверки `ArchiveBackend`; преобразование логарифмов в МиБ требуется текущим
полем `CompressionOptions::solidBlockMiB`. Registry, Add_Size, расчёт Auto,
Non Solid и Full Solid из этой функции не включены в данный перенос.
Форматирование изменено только по обязательному clang-format gate.

### Итерация: порядок Deflate / Deflate64

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetOrder2`.
Скопированы обе ветки Deflate: условия Auto 32/64/128, цикл и замена
последнего значения на 258/257. Сохранены имена метода, порядок вычислений
и границы; clang-format меняет только оформление. Win32 AddOrder и выбор
Registry заменены возвратом данных и текущим выбором Qt, как в ранее
перенесённой ветке LZMA. `GetMethodID` — узкий адаптер между строковым
`CompressionOptions` и исходными ID из таблицы форматов; upstream получает
ID из item data Win32 combo. Backend использует тот же ряд для проверки и
передаёт существующий параметр `fb` движку. Ветки порядка PPMd перенесены следующей итерацией.

### Итерация: порядок PPMd / PPMdZip

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetOrder2`,
`GetOrderMode`, `CPP/7zip/UI/GUI/UpdateGUI.cpp::SetOutProperties`.
Скопированы ветки PPMd (ряд 2,3,4,5,6,7,8,10,12,14,16,20,24,28,32 и Auto
4/6/16/32) и PPMdZip (2–16, Auto level+3), включая исходные циклы,
условия и формулы. Как в остальных ветках, AddOrder/Registry заменены
выдачей данных и текущим выбором Qt. `GetOrderMode` скопирован с передачей
ID вместо чтения Win32 combo; добавлен пустой default для clang -Wswitch.
Backend адаптирует исходный выбор `di.OrderMode ? "o" : "fb"` из
SetOutProperties к существующей строке аргумента QProcess. COM property
vector не используется при процессной границе с 7zz; функция целиком пока
не перенесена. Размер памяти PPMd и его dictionary branch остаются долгом:
UI отключает словарь, движок использует свой Auto. Подпись «Размер слова»
сохранена как в ресурсе upstream. Только clang-format меняет оформление.

### Итерация: словарь LZMA / LZMA2

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2`,
`Combo_AddDict2`. Скопированы формула Auto, цикл i=30..62, пропуски малых
значений, ограничение kLzmaMaxDictSize (15<<28) и архитектурный предел
kLzmaMaxDictSize_Up. Сохранены типы UInt32/size_t, условия и порядок
вычислений. Qt получает список вместо Win32 combo; Registry и выбор
ближайшего сохранённого значения заменены сохранением доступного текущего
выбора (как в остальных опциях). Пересчёт сохранённого Registry dictionary
>=3.75 GiB не нужен: отсутствуют Registry и произвольные размеры в Qt combo.
Перенесены проверки кратности и выбор единиц из Combo_AddDict2; форматирование
строки выполняет QString, подписи локализованы в КиБ/МиБ. UI и backend
используют один исходный ряд. CompressionOptions хранит байты (quint64)
вместо МиБ, чтобы не терять 64/256 КиБ; адаптер 7zz добавляет суффикс b.
Auto=0 не передаёт словарь движку. Другие dictionary branches, RAM limits,
расчёт памяти и Registry остаются долгом. clang-format меняет оформление.

### Итерация: память PPMd в 7z

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kPPMd)
и `CPP/7zip/UI/GUI/UpdateGUI.cpp::SetOutProperties`.
Скопированы Auto=1<<(level+19), константы kPpmd_Default_4g /
kPpmd_MaxDictSize_Up, цикл i=38..62, пропуск 1.5 МиБ, ограничение размера
и break по архитектуре. Формулы, типы и порядок вычислений сохранены;
Win32 AddDict2/Registry заменены выдачей значений и текущим выбором Qt.
Ряд не достигает границы 4g на поддерживаемых 32/64-bit, поэтому размер
для подписи совпадает с реальным размером. Backend переносит выбор
OrderMode ? "mem" : "d" в существующий QProcess аргумент; COM property
vector остаётся вне процессной границы. Qt включает словарь по наличию
исходных вариантов; backend проверяет тот же список без собственного
ограничения на LZMA. RAM limits, Registry и PPMdZip пока не перенесены.
Обязательный clang-format меняет только оформление исходных циклов.

### Итерация: память PPMdZip

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kPPMdZip).
Скопированы Auto=1<<(level+19) и цикл i=20..28 с UInt32 dict=1<<i.
AddDict/Registry заменены возвратом данных и Qt selection; исходные
формулы, границы и типы сохранены. Backend уже использует GetOrderMode
и mem=<bytes>b; одинаковая подпись PPMd разрешается в kPPMdZip по таблице
ZIP. RAM limits и выбор Registry остаются долгом. Форматирование — clang-format.

### Итерация: словарь BZip2

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kBZip2).
Скопированы ветки уровня >=5 / >=3 / ниже и значения 900/500/100<<10,
цикл i=1..9 и UInt32 dict=(i*100)<<10. Условия, типы и формулы сохранены;
Win32 AddDict и сохранённый Registry default заменены Qt combo данными
и сохранением доступного текущего значения. Backend использует исходный
список и существующий d=<bytes>b. Исходные значения в KiB сохранены,
хотя BZip2 движок нормализует размер до блоков по 100000 байт.
Это не собственный набор размеров. Форматирование — clang-format.

В этой же итерации завершены dictionary branches доступных методов:
скопирована ветка kDeflate/kDeflate64 с фиксированным Auto 1<<15 / 1<<16.
AddDict2(Auto) заменён одной строкой Qt; явный словарь не доступен,
backend отклоняет его, поскольку исходный список не содержит ручных вариантов.
Ветка kCopy не подключается: метод Copy не экспонируется нашей таблицей
7z-методов, Store сохраняет отдельное отключение контролов. Остальные
неподдерживаемые форматы и закомментированный ZSTD не включены.

### Итерация: управление solid-блоками

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::Get_Lzma2_ChunkSize`, `Add_Size`,
`CCompressDialog::SetSolidBlockSize2`, solid-блок `OnOK`,
`CompressDialog.h::NCompressDialog::CInfo` и `UpdateGUI.cpp::SetOutProperties`.
Скопированы Get_Lzma2_ChunkSize целиком, выбор единиц Add_Size, расчёт
Auto solid (включая нормализацию BZip2 и пределы 16 МиБ/4 ГиБ/16 ГиБ),
ряд логарифмов 20..36, Non Solid=0 и Full Solid=64, а также преобразование
OnOK в байты с UInt64(-1) для Full. Win32 insertion/Registry заменены
данными Qt combo и текущим выбором; названия, формулы, типы, условия
и порядок вычислений сохранены. Add_Size использует QString для строки
и локализованные единицы, без собственного алгоритма размеров.
Только solid-поля CInfo скопированы: CompressionOptions наследует этот
запись-адаптер, остальные поля CInfo ещё не перенесены. Qt задаёт
SolidIsSpecified по доступности поля перед вызовом перенесённого OnOK,
как FormatChanged upstream. Backend проверяет байты через тот же ряд
и преобразование, переносит s=<bytes>b из SetOutProperties в 7zz -ms.
Прежние bool solid / solidBlockMiB и отдельный checkbox удалены:
единственный список теперь следует исходному порядку контролов.
Полный OnOK, Registry и XZ UI остаются долгом; исходная XZ ветка Auto
сохранена, но формат создания остаётся ограничен 7z/ZIP.
Форматирование — обязательный clang-format.

### Итерация: потоки и расход памяти

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetNumThreads2`,
`GetMemoryUsage_Threads_Dict_DecompMem`, `AddMemUsage`, RAM-блок `OnInit`
и проверка памяти `OnOK`; `CompressDialog.h::GetDict2`;
`CPP/7zip/Common/MethodProps.cpp::Calc_From_Val_Percents`;
`CPP/Windows/System.cpp`, `.h`.

System.cpp/.h, Windows/StdAfx.h, Common/Defs.h, Common/MyVector.h и C/Threads.h
скопированы побайтово в third_party/7zip с исходными путями и notices.
Переиспользованы CProcessAffinity и GetRamSize, включая Windows processor
groups, POSIX affinity, sysctl/sysinfo: они уже переносимы, новые Qt/sysconf
аналоги не написаны. POSIX CProcessAffinity не имеет Windows метода
Get_and_return_NumProcessThreads_and_SysThreads; нормализация num1/num2
скопирована из его тела в узкий адаптер GetSystemInfo. RAM-блок OnInit
сохранён с 32-bit пределом 7<<28, минимумом 64 МиБ и исходными 80%.
Calc_From_Val_Percents скопирован целиком в зеркальный Common/MethodProps.cpp;
COM/строковые свойства остальных функций MethodProps пока не нужны
процессной границе, поэтому адаптированный заголовок содержит только
используемый интерфейс и исходные UInt64/UInt32, без новых вычислений.

SetNumThreads2 сохраняет исходные пределы LZMA=2/LZMA2=512/BZip2=64,
Auto-only для 7z PPMd/Copy, ZIP=32/128 по архитектуре, CPU*2 предел списка,
а также циклы уменьшения Auto для ZIP и LZMA2 по памяти. Registry и
Win32 вставка заменены текущим Qt выбором и возвратом данных. Скопирована
полная активная функция оценки памяти со всеми доступными ветками и
исходными арифметикой/условиями/порядком; вместо методов виджета её
параметры содержат format, level, method, dictionary и blockSizeLog.
AddMemUsage сохраняет единицы MB/GB/TB и округление, QString заменяет
UString insertion. Пустой default устраняет enum -Wswitch без смены поведения.

Qt-представление ресурсов вынесено в createarchivedialog_resources.cpp:
GetDict2 адаптирует исходное переключение Spec/Auto к combo=0; системные
данные читаются один раз на диалог. Spinbox 0..256 заменён combo Auto/ряд,
показаны процессные/аппаратные CPU и оценки сжатия/распаковки по порядку
CompressDialog.rc. Backend получает тот же набор через порт, проверяет
явные потоки, применяет условие memUsage>limit из OnOK перед запуском
и сохраняет -mmt только для явного выбора. При неизвестной RAM отказ
не применяется (адаптация недоступного системного значения); Auto по
памяти отключён, как в исходном SetNumThreads2. Пользовательский RAM
selector, Registry, ZSTD и UI XZ остаются долгом. Форматирование — clang-format.

### Итерация: исходный разбор лимита памяти

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::NCompression::CMemUse`,
`ZipRegistry.cpp::ParseMemUse`, `CMemUse::Parse` и
`CPP/Common/MyString.h::MyCharLower_Ascii(wchar_t)`.
Структура, Clear/GetBytes, весь указательный парсер и ASCII helper
скопированы с сохранением имён, типов, ветвей и проверки переполнения.
Адаптация: Parse принимает QString и передаёт нуль-терминированную
std::wstring исходному wchar_t парсеру. Это позволяет использовать уже
скопированный ConvertStringToUInt64 на всех ОС, без нового Qt-парсера.
CMemUse использует ранее перенесённый Calc_From_Val_Percents, включая
насыщение и округление. Остальной ZipRegistry (Windows Registry,
критические секции и настройки форматов) не нужен для этого ядра и
остаётся долгом; не добавлены платформенные Registry зависимости.
Форматирование — обязательный clang-format. Подключение пользовательского
предела к диалогу и backend выполняется следующей итерацией.

### Итерация: пользовательский лимит памяти от диалога до движка

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::AddMemSize`, `AddMemComboItem`,
`SetMemUseCombo`, `Get_MemUse_Bytes`, RAM-блок `OnInit`, память в `OnOK`,
`SetNumThreads2`; `CompressDialog.h::CInfo::MemUsage`;
`UpdateGUI.cpp::SetOutProperties` (ветка memuse).
Скопированы форматирование размеров, Auto/процентные/байтовые элементы,
оба цикла выбора и вставка нестандартного текущего значения, GetBytes
и fallback Auto. Алгоритмы, типы, пределы и порядок ветвей сохранены.
Вместо UString и Win32 combo функции возвращают Qt-строки/данные;
Registry CFormatOptions заменён параметром spec, операции SetCurSel
удалены из ядра — текущий выбор принадлежит Qt. Show/Enable по MemUse_
не нужен: оба экспонируемых формата имеют этот flag. Для байтов mu.Val
эквивалентен исходному GetBytes(_ramSize_Reduced). CSystemInfo получил
исходное _ramSize_Reduced после минимального размера 64 МиБ.

SetNumThreads2 теперь вызывает перенесённый Get_MemUse_Bytes вместо
фиксированного RamUsageAuto. При смене solid-блока Qt пересчитывает
Auto-потоки; backend восстанавливает log из проверяемого CInfo, чтобы
оценка соответствовала выбранному solid. Это адаптер байтовой process
границы, не новая формула оценки памяти. OnOK сохраняет проверку
memUsage!=UInt64(-1), затем memUsage>limit; Qt заменяет MessageBoxError.
Проверка применяется лишь к созданию: диалог добавления не экспонирует
сжатие и backend сохраняет существующие параметры.

CInfo передаёт исходную CMemUse, backend повторяет проверку и переносит
ветку SetOutProperties: проценты в memuse=<value>%, байты в memuse=<value>b,
Qt QProcess добавляет префикс -m. Неизвестная RAM сохраняет ранее описанную
адаптацию Auto без отказа; явный предел проверяется. Оценка/RAM в подсказке
вместо исходной длинной строки со slash — адаптация ширины Qt-формы.
Windows Registry, сохранение по формату и остальные поля CInfo остаются
долгом. Форматирование — обязательный clang-format.

### Итерация: сохранение лимита памяти по формату

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::CFormatOptions::{FormatID,MemUse}`,
`CInfo::Formats`, `ZipRegistry.cpp::CInfo::{Load,Save}` (пределы памяти);
`CompressDialog.cpp::FindRegistryFormat`, `FindRegistryFormat_Always`,
`Get_FormatOptions`, `SaveOptionsInMem`, `SetMemUseCombo`, FormatChanged
и Save в OnOK. Поля остальных compression options не объявляются
перенесёнными: эта итерация ограничена памятью.

Скопированы поиск формата без учёта регистра, поиск-or-add, доступ по индексу,
сохранение MemUse предыдущего формата до его смены и формирование исходного
выбранного индекса при вставке нестандартного spec. Имена, порядок ветвей,
возвращаемые индексы и CObjectVector сохранены; используется уже побайтово
скопированный Common/MyVector.h. Qt presentation-зависимые UString/
CSysString заменены QString, Get_ArcInfoEx/Info.FormatIndex — текущим
и предыдущим именем Qt combo. Тела принадлежат одному Qt адаптеру
createarchivedialog_preferences.cpp; нет второго алгоритма поиска форматов.

В ZipRegistry.cpp скопированы Clear при Load, обход formatIDs,
создание CFormatOptions, чтение MemUse и Formats.Add; Save сохраняет
исходный обход FOR_VECTOR и заменяет только группу options. Win32
Registry open/create/GetRegString/SetRegString заменены QSettings INI
в выделенной группе Compression/Options существующего toolbar.ini.
Имена MemUse32/MemUse64 сохраняют исходное разделение архитектур.
Win32 CS_LOCK не нужен: загрузка/сохранение вызываются только в GUI thread.
Методы принимают путь вместо глобального registry key, что обеспечивает
детерминированные фикстуры. Другие группы настроек не затрагиваются.
Load/Save переносят только память/FormatID; остальные поля исходного CInfo
и CFormatOptions остаются долгом. Save вызывается после всех проверок OK;
Cancel и неуспешная валидация не записывают изменения. Форматирование —
обязательный clang-format.

### Итерация: исходные режимы и история диалога извлечения

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/ExtractDialog.cpp::{kPathModeButtonsVals,kOverwriteButtonsVals,
AddComboItems,AddUniqueString,CExtractDialog::OnInit,OnOK}`, `.h`;
`CPP/7zip/UI/Common/ZipRegistry.*::NExtract::CInfo::{Load,Save}`;
`CPP/Windows/FileName.cpp::NormalizeDirPathPrefix`.
`UI/Common/ExtractMode.h` скопирован побайтово в third_party/7zip.

Класс CExtractDialog и адаптированные тела находятся в зеркальных GUI путях.
Исходные таблицы, исходный порядок overwrite Ask/Overwrite/Skip/Rename/
RenameExisting, циклы AddComboItems и AddUniqueString, предел 16,
обработка выбранного/введённого пути, исключение выбранного или последнего
элемента, Trim, NormalizeDirPathPrefix, порядок ветвей смены режимов и
показа пароля, очистка/пересборка истории и сохранение в OnOK скопированы.
Сохраняются CObjectVector, enum и CBoolPair upstream. Win32 controls,
строки локализации и сообщения заменены Qt; UI constructor — presentation
адаптация существующей формы по ExtractDialog.rc. Исходная нормализация
пути перенесена локальным QString helper: пустой путь, проверка последнего
разделителя, добавление разделителя. Qt принимает оба разделителя на Windows.

Qt editable combo может удерживать индекс при редактировании текста:
узкий адаптер переводит этот случай в CB_ERR до исходной ветки OnOK.
После addItem восстанавливается typed text, поскольку Qt автоматически
выбирает первый элемент. Backend API переводит исходные enum в свои
ExtractionOptions; параметры 7zz остаются за ArchiveBackend. Absolute
paths не добавляются в combo (граница backend их не поддерживает); kAsk
адаптирован к прежнему отказу при конфликте вместо интерактивного callback.

Load/Save переносят только пути, режимы/Force и ShowPassword из NExtract
CInfo; QSettings заменяет Registry, GUI-thread вызовы не требуют CS_LOCK.
Сохраняются оригинальные имена ключей в Extraction; старый history/extract
читается только как миграционный fallback. Пароль не записывается.
MainWindow лишился самостоятельного алгоритма истории, старый cpp удалён,
header содержит только alias ExtractDialog=CExtractDialog. Неиспользуемые
SFX, Lang, Help, NtSecurity, ElimDup, SplitDest пока остаются долгом.
Обязательный clang-format изменяет оформление скопированного тела.

### Итерация: SplitDest и исходное разделение пути

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/Common/Wildcard.cpp::SplitPathToParts_Smart`,
`CPP/7zip/UI/GUI/ExtractDialog.cpp::CExtractDialog::{OnInit,OnOK,
OnButtonClicked}` (ветки IDX_EXTRACT_NAME_ENABLE / SplitDest),
`UI/Common/ZipRegistry.*::NExtract::CInfo::SplitDest`.

Указательный обход SplitPathToParts_Smart скопирован в существующий
зеркальный Common/Wildcard.cpp; имя, start/p, пропуск одного конечного
разделителя и обратный поиск сохранены. QString/QChar заменяют UString/
wchar_t, left/mid заменяют SetFrom/присваивание p. IsPathSepar сохраняет
исходное разделение платформ: на Windows slash/backslash, POSIX slash.
Структура borrowed Wildcard не реорганизуется; это единственный helper.

Скопированы ветки OnInit: при SplitDest.Checked исходный путь разделяется,
пустой prefix заменяется name (корень и одиночное относительное имя), иначе
name попадает в отдельный Edit. OnOK сохраняет исходные Trim, +=pathName,
NormalizeDirPathPrefix, сравнение SplitDest со старым значением и Def=true
при изменении. Значение по умолчанию true и Key_Get_BoolPair_true перенесены
в QSettings Load/Save NExtract::CInfo; только изменённый bool сохраняется.
Name field показывается/скрывается Qt toggled вместо Win32 OnButtonClicked.
Отмена не записывает флажок/историю, поле имени само не сохраняется:
в истории хранится составной конечный путь, как в upstream.

Qt presentation повторяет checkbox/Edit перед Path mode слева, Password
справа по ExtractDialog.rc. У исходного пустого checkbox добавлены tooltip
и accessibleName; Edit получил accessibleName/placeholder. Qt getter
до OK собирает preview из тех же полей, после OK возвращает DirPath.
Параметры движка не меняются: backend проверяет составное назначение
существующей защитой путей/ссылок и создаёт его. NtSecurity, ElimDup,
интерактивный Ask остаются долгом. Форматирование — clang-format.

## Полный профиль доступных параметров сжатия (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::NCompression::CFormatOptions`,
`ZipRegistry.cpp::CInfo::Save/Load`, `Key_Set_UInt32/Key_Get_UInt32`;
`CPP/7zip/UI/GUI/CompressDialog.cpp::SaveOptionsInMem`, `FormatChanged`,
`SetLevel2`, registry branches `SetMethod2`, `SetDictionary2`, `SetOrder2`,
`SetSolidBlockSize2`, `SetNumThreads2`.

Скопированы поля Level/Dictionary/Order/BlockLogSize/NumThreads/Method,
инициализация и ResetForLevelChange/Reset_BlockLogSize. Save/Load сохраняют
исходный порядок полей и DWORD sentinel `UInt32(-1)` (отсутствующее значение).
Win32 Registry заменён узким QSettings INI адаптером; UString Method заменён
QString на границе Qt. У SaveOptionsInMem сохранены ветви преобразования
UInt64 dictionary в DWORD, включая `UInt32(-2)` для 4 GiB. Нулевые Qt Auto
словаря/слова/потоков преобразуются в исходный sentinel. Solid хранит исходный
логарифм, в том числе 0/64/Auto. Пока параметры времени, произвольные options
и encryption method CFormatOptions не подключены.

В существующем Qt presentation adapter createarchivedialog_preferences.cpp
FormatChanged восстанавливает level → method → dictionary/order → solid →
threads, с исходной проверкой совпадения метода перед восстановлением его
параметров. QSignalBlocker заменяет отсутствие Win32 CBN_SELCHANGE при
программном заполнении. Недоступные значения возвращаются к Auto; выбор
большого словаря ограничивается текущим исходным рядом. SetLevel2 теперь
использует исходные ветви persisted level (default 5, invalid >9 →9) и
nearest-selection цикл. Обработчики смены уровня/метода/словаря перенесены
следующей итерацией. Пароли в профиль не входят.

## Смена уровня сжатия (2026-09-28)

Источник того же commit: `CompressDialog.cpp::OnCommand`, ветка
`IDC_COMPRESS_LEVEL`, `CompressDialog.h::SetMethod/MethodChanged`,
`ZipRegistry.h::CFormatOptions::ResetForLevelChange`.
В Qt-обработчик скопирован вызов ResetForLevelChange перед заполнением
метода и его зависимых контролов. Исходный reset очищает Method и присваивает
sentinel Level/Dictionary/Order/BlockLogSize/NumThreads; MemUse сохраняется.
Программное заполнение выполняет уже адаптированный FormatChanged, с
записью текущего выбранного Level в in-memory профиль перед заполнением:
это необходимо, поскольку Qt adapter заполняет также level combo, тогда
как исходная ветка CBN_SELCHANGE использует уже выбранный Win32 level.
Расчёты solid/threads/memory выполняются в исходном порядке зависимостей.
SFX/checksum ветки не применимы к текущим поддерживаемым 7z/ZIP.
Уровень 0 выключает Qt-контролы; список методов оставлен видимым и
недоступным (upstream очищает его). Пароль/лимит памяти не сбрасываются.

В той же итерации перенесены ветви `IDC_COMPRESS_METHOD`,
`IDC_COMPRESS_DICTIONARY`, `IDC_COMPRESS_MEM_USE`. Qt MethodChanged
заполняет словарь и слово в исходном порядке, затем solid/threads/memory
по вызывающему обработчику upstream. Параметры восстанавливаются только
при совпадении Method с сохранённым профилем; другие методы получают Auto.
При смене словаря скопированы SaveOptionsInMem, проверка
`blockSizeLog != kSolidLog_NoSolid && != kSolidLog_FullSolid` и
Reset_BlockLogSize: обычный размер solid сбрасывается к Auto, специальные
0/64 сохраняются. SaveOptionsInMem при смене лимита сохраняет explicit threads
перед их пересчётом. Оба сохранения меняют только in-memory профиль до OK.

## Рекурсивное копирование файловой системы (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CCopyState::CallProgress`,
`CopyFile_Ask` (UseReadWriteMode), `CombinePath`, `CopyFolder`.
В том же зеркальном файле сохранены порядок CallProgress → IsDestChild →
создание папки → перечисление → рекурсивная ветка/CopyFile_Ask, имена
srcPath2/destPath2 и накопление StartPos после MyCopyFile.
Самостоятельный обход copyEntry и обвязка copyFile удалены из worker.

CCopyState вынесен в существующий FSFolder.h и namespace NFsFolder для
доступа Qt-worker; из CProgressInfo нужен только StartPos. COM callbacks
заменены функциями Progress/CurrentFile, HRESULT/RINOK — bool и проверкой
результата; ошибки передаются в ErrorMessage. QString/QFileInfo/QDir заменяют
FString/CFileInfo/CEnumerator и платформенные операции. Qt entryInfoList
не предоставляет ошибку Next; перед обходом проверяется доступность папки.
Скрытые и системные записи включены, ссылки отклоняются, stage создаётся
только в отсутствующей папке (mkdir вместо CreateComplexDir).

AskWrite адаптирован как отказ при конфликте до открытия файла, а не
интерактивный запрос. Неиспользуемые skip/TotalSize и Win32 CopyFileEx ветки
не подключены. MoveMode/MoveFile_Sys/RemoveDir остаются у транзакционного
worker: он использует rename и откат, копирование с удалением источника
нарушило бы текущую гарантию сохранности при ошибке. Межтомный перенос
остаётся неподдержанным. Mandatory clang-format меняет оформление.

## Диалог копирования/перемещения и история (2026-09-28)

Источник того же commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/CopyDialog.cpp`, `.h`, `.rc`,
`ViewSettings.cpp::SaveStringList`, `ReadStringList`, `SaveCopyHistory`,
`ReadCopyHistory`, `AddUniqueStringToHeadOfList`, `App.cpp::CApp::Copy`
(обновление истории назначения). Исходный класс CCopyDialog и поля
Title/Static/Value/Info/Strings сохранены в зеркальном пути. OnInit заполняет
историю перед Value и устанавливает Info; OnOK получает Value только при OK.
OnButtonSetPath сохраняет cancel → NormalizeDirPathPrefix → SetCurSel(-1)
→ SetText. Qt QFileDialog заменяет MyBrowseForFolder, slash-проверка после
fromNativeSeparators — платформенный NormalizeDirPathPrefix.

Win32 Attach/SetItemText/NormalizeSize/OnSize заменены Qt-контролами и layouts
с исходным порядком label → path/browse → info → OK/Cancel. Вместо Static
SS_LEFTNOWORDWRAP используется read-only QPlainTextEdit без переноса строк:
прокрутка сохраняет доступ к длинным именам и большому выделению. Входной
список имён и конструктор — существующая Qt интеграция, а не перенос полного
CPanel::GetItemsInfoString (property totals ещё не подключены). Исходный
kCopyDialog_NumInfoLines сохранён для будущей сводки.

Save/ReadStringList используют QSettings вместо Registry и explicit settings
path вместо HKEY; вызовы идут в GUI-thread, lock не нужен. Имя CopyHistory
сохранено; старый copy/history не читается и не изменяется (fallback удалён
в итерации истории/избранного по указанию пользователя). Исходный цикл
удаляет все совпадения без учёта регистра и вставляет новый путь в начало;
App copy workflow ограничивает список 20 записями и сохраняет после принятого
существующего назначения. Проверка/создание назначения и backend остались
на прежних границах; произвольное переименование цели/новая папка из CApp::Copy
пока не перенесены. Самостоятельный copydialog.cpp удалён; header — alias.
Форматирование изменено обязательным clang-format.

## Сводка выделения для Copy/Move (2026-09-28)

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/App.cpp::CPanel::GetItemsInfoString`,
`AddSizeValue`, `AddValuePair1`, `AddValuePair2`, `AddPropValueToSum`;
`PanelListNotify.cpp::ConvertSizeToString` (обе перегрузки, INT_TO_STR_SPEC);
`Panel.h::GetItemsInfoString`, `CopyDialog.h::kCopyDialog_NumInfoLines`.
Скопированы исходные циклы и ветви: суммы папок/файлов раздельно, неизвестное
значение UInt64(-1) распространяется без дальнейшего счёта, общий размер
выводится только при двух ненулевых определённых суммах, затем путь и первые
11−6 имён с разделителем для папок и многоточием для остатка. Сохраняются
имена CPanel, методов и _currentFolderPrefix. Форматирование UInt32/UInt64
скопировано целиком с промежуточным Byte[32], wchar_t[32] и группами по три
цифры; QString::fromWCharArray заменяет UString только на выходе.

В зеркальном Panel.h портирована только часть CPanel, владеющая сводкой.
IFolderFolder заменён snapshot CObjectVector<CPanelItem> с RelPath/IsFolder/
SizeDefined/Size; upstream CRecordVector<UInt32> используется без изменений.
AddPropValueToSum получает тот же индекс и свойство Size без COM/PROPVARIANT;
языковые resource строки заменены русскими Qt строками. QString заменяет
UString для UI текста, noexcept — throw(). Конструктор snapshot и optional
Info аргумент CCopyDialog — узкая интеграция: исходный App устанавливает Info
до Create, а Qt создаёт виджеты в конструкторе.

MainWindow::selectedCopyInfo только собирает свойства выбранных строк:
относительное имя из представления, размеры файлов из UserRole, папок архива
из CProxyArc. Для папок диска SizeDefined=false соответствует FSFolder.cpp
kpidSize до FolderStat_Defined. Missing-size свойства архивных файлов уже
нормализуются текущим backend в 0; различение отсутствующего Size остаётся
долгом parser/metadata, без изменения границы ArchiveBackend в этой итерации.
Сводка не устраняет пересечение выбранных папок/файлов: как upstream, суммы
относятся к выбранным строкам. `sevenzip_panel` собирает эти UI модули один
раз, не добавляя QWidget-зависимость к sevenzip_common. clang-format меняет
оформление. Предыдущий конструктор имён оставлен как compatibility fallback;
рабочие F5/F6 теперь всегда получают перенесённую сводку.
