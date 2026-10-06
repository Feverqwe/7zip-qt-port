# Ревизия происхождения кода 7zFM

## P3-05.15 — source Copy/Move destination and continuation reconciliation (required scope, 2026-10-06)

Complete App.cpp/App.h body/data/caller review at project
c590f0f1f372f4a236662fac8f490a630d6ef964 and upstream
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17,
https://github.com/ip7z/7zip.git, is recorded in
[omitted scope](omitted-scope-audit.md#app-complete-bodycaller-review-2026-10-06).
All37 top-level active bodies, CButtonInfo::GetText, toolbar arrays and relevant
inline/state dependencies were reviewed; commented CheckFolderPath/IsFsPath/
GetFocusedPanelIndex are distinguished from the active reducer and inline getter.
No additional module/dependency was copied by that initial docs-only review.

OnCopy now retains the original destPanel.GetFsPath default and one-panel
Reduce_Path_To_RealFileSystemPath body in the authoritative App.cpp. The reducer
uses the existing portable FileFind/FileName dependencies; adaptations are
qualified namespaces, formatting and the existing Qt dialog string boundary.
No additional borrowed module or platform implementation is introduced.
IQtAppPresentation/QtAppPresentation's independent DefaultCopyDestination hook
is removed. MainWindow's separately used Extract-dialog default remains outside
this Copy change. Source names, paths, notices and the pinned baseline remain.

Current OnCopy is still a partial port: Windows root/ADS refusals,
possibleToUseDestPanel/destIsFsPath and Is_IO_FS_Folder
routing, full-path naming, option flags and post-operation selection/refresh
continuation require reconciliation. The Qt default fallback and repeated
state-based execution classification do not establish absence of usable source
logic. CopyArchiveToArchive's alternate-stream/zone overrides need explicit
native evidence/disposition. The accepted archive Move transaction composition
remains justified by original Agent CopyTo(moveMode)==E_NOTIMPL and the required
plan extension; source predicates must be restored without discarding it.

Earlier P1-01/02, P2-05/06/11 and Shift/direct-path acceptance remains bounded
valid evidence. It does not prove complete original OnCopy flow. The new
[text contract](ui-layout-contracts.md#source-copymove-destination-p3-0515-required-scope)
defines integrated consumer/UX acceptance before implementation. Diff check and
current baseline verification validate this review; implementation, actual new
regressions and current three-platform/cold-stable/rendered/final acceptance
remain required. No new runtime check is claimed for unchanged code.
Review checks pass git diff --check and reproducible baseline verification
(408 exact/196 adapted/3 local paths at the pinned revision). A supplementary
active-body census records the original/adapted SHA and all37 top-level bodies;
CButtonInfo/data/inline interfaces were reviewed manually. Filename/census/manifest
checks do not replace the required functional consumers or close P3-05.

Destination-default follow-up has an actual previous-product RED: F5 to a drives
peer displayed the source folder instead of an empty target (6 Qt pass/1 fail).
Current public ui_copy_same rows cover F5/F6, drives/root peer, EN/RU440×250 and
360×200, Cancel, unchanged Unicode control bytes, no output and no engine calls.
A real ZIP checks one-panel reduction to the existing parent before the retained
focused-versus-selected Shift Copy/Move success cases. The offscreen driver
explicitly activates the window between Cancel and the next command; this is not
evidence of rendered native focus parity. Sixteen current-build JSON reports were
inspected for grouping/order, sizes, visible/enabled controls, prompt fit and
destination/browse separation. The Copy controls remain within both contract sizes.

Validation on macOS27.0.1 arm64/Qt6.12.0: focused Copy suites2/2PASS1.18s,
CopyDialog9/UiCopySame22 Qt pass, zero fail/skip; mandatory unsandboxed check.sh
passes formatting, all-target build and252/252 CTest111.62s. Six tested code/test/
build inputs retain their pre-gate SHA. RED, focused/full logs, input manifest and
sixteen reviewed reports are frozen separately under build/review/source-copy-
destination-{red,final}. Remaining source classification/options/naming/completion,
native Windows/Linux, ADS/device, cold-stable/current-package/clean-OS/rendered
and P3-06 acceptance remain mandatory. This component does not close P3-05.15.

## P3-05.14 — persisted source-location consumers (required scope, 2026-10-06)

Complete original/adapted PanelFolderChange body/caller/dependency review at
project e314301c908b7b8e4ab8a27d016fe037f05650b8 and upstream
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17,
https://github.com/ip7z/7zip.git, is recorded in
[omitted scope](omitted-scope-audit.md#panelfolderchange-complete-bodycaller-review-2026-10-06).
It covers every active body, ANSI/CE notification transport, commented branches,
folder links/guards, FM.mak/CMake and original/Qt consumers. This is one reviewed
module, not109-module or runtime acceptance; no code/dependency copy is made.

The remaining active SetBookmark/OpenBookmark and history consumers require
source virtual prefixes instead of Qt display/engine paths. Current Namespace
and nested guards differ from the original focused-panel callers; a drive Save
action can be enabled but its handler returns without saving.
Empty-slot Open is also disabled/ignored instead of the original empty root binding.
Existing bounded mock/list persistence evidence remains valid but does not
establish namespace/nested/restart or original source-address binding. The required component must
reuse the existing CPanel binding and CFastFolders/CFolderHistory owners, cover
old Qt display entries without corrupting literal paths, and retain operation
guards, private fixtures and input bytes. The
[text contract](ui-layout-contracts.md#persisted-source-locations-p3-0514-required-scope)
defines its UI reference and integrated acceptance. Docs-only review requires
diff check; implementation and current three-platform/final checks remain open.
Review validation: git diff --check passes; current reproducible-baseline
verification passes408 exact/196 adapted/3 local paths at the pinned revision.
A declaration census confirms28 original bodies/helpers (including both
wide/ANSI end-edit overloads) have explicit dispositions; the census supplements
the manual body/caller review and is not a behavior or whole-port acceptance test.

The integrated follow-up retains the single existing CPanel static state-service
adaptations of SetBookmark/OpenBookmark and CFastFolders/CFolderHistory bodies.
Qt now supplies the bound focused panel's LoadFullPath prefix rather than its
display path; no folder binding or source string/vector algorithm is copied into
the widget. Source-address navigateTo uses the authoritative CPanel BindToPath
and existing renderer/progress publication. Central command policy removes the
extra root/nested/empty-slot prohibitions; operation/notification guards remain.
Archive/namespace history records source prefixes separately from displayed
header text, including nested chains. Filesystem history keeps its existing
canonical absolute Qt path serialization. Original source names and module
boundaries are unchanged; no additional borrowed dependency is introduced.

SavedSourcePath is a narrow compatibility adapter for the project's former Qt
display serialization, which has no upstream counterpart: only a ` / ` boundary
whose preceding prefix is an existing file is converted. The remainder is passed
unchanged to the source binder; literal spaces in components and stored old
entries are preserved. This is not a second filesystem/archive binding algorithm.
Source CAgentFolder::BindToFolder name/index bodies and proxy lookups were
inspected at the same revision: they bind directories, not archive files.
BindToPath reuses an already-open nested link but, after close/restart, stops at
the containing outer archive when the next component is a file. The first test
expecting automatic nested reopening was disproved by those original bodies;
that failed consumer log is retained separately. Required regressions now assert
exact open-chain reuse and the original cold fallback, without adding a new
nested traversal implementation or weakening runtime timing limits.

Actual previous-product RED has2 Qt pass/5 fail/0 skip: EN/RU drive Save is empty,
empty Open disabled, ordinary ZIP persists display text and nested Save disabled.
The initial two-panel fixture's ambiguous widget lookup was corrected before
freezing that RED. New public persisted_location consumers cover both panels,
source providers/prefixes, empty/root Save, real EN/RU ZIP/nested reuse/close/
restart, history persistence, exact outer/inner/input bytes and old display
entries through bookmark/history selection. POSIX literal ` / ` archive-folder
components are tested; Windows uses ordinary Unicode/space names supported by
its fixture filesystem. Existing bounded mock/history consumers now assert
source serialization, retaining actual displayed-address expectations and their
editing/Cancel/busy coverage. Six affected suites pass3.15s, including11
PersistedLocation Qt cases with0 fail/0 skip. Sixteen normal/compact EN/RU reports
cover drive/archive/history states. Native Windows plugin/desktop locking is
registered; current Windows/Linux and rendered/final acceptance remain open.
Mandatory unsandboxed format/all-target/full macOS gate passes252/252 in106.39s.
Eight frozen code/build/test SHA inputs remain unchanged; final11
PersistedLocation Qt cases pass0 fail/0 skip. Sixteen final widget JSON reports
independently pass exposure, menu/toolbar/address/list/status order, panel
placement, combo size hints and history footer review. The current baseline is
unchanged. This verifies the implementation on macOS; current native Windows/
Linux, device/UNC/ADS and rendered/cold-stable/parent acceptance remain required.

## P3-05.13 — source root-command target (implementation validation active, 2026-10-06)

Source https://github.com/ip7z/7zip.git at full revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. resource.rc's
IDM_OPEN_ROOT_FOLDER and MyLoadMenu dispatcher call CApp::OpenRootFolder; its
App.h inline body calls GetFocusedPanel().OpenDrivesFolder, not OpenRootFolder.
Panel.cpp::CMyListView::OnMessage routes slash/backslash characters to the same
drive view (the similar PanelKey switch branch is commented). The active
PanelFolderChange::OpenDrivesFolder closes open folders, constructs/sets CFSDrives,
initializes and refreshes it; CE instead selects the filesystem root service.
Root namespace navigation remains a distinct original OpenRootFolder body.

At discovery QtFileManagerCommands::OpenRootFolder delegates MainWindow::goRoot,
which calls CPanel::OpenRootFolder. Thus the actual command presents CRootFolder
entries instead of the source drive view; the authoritative OpenDrivesFolder body
is missing. PanelKey also binds backslash but omits the active slash route.
Existing alternativeRootUsesSourceMarks only checks nonempty rows and marks,
so its pass does not prove the command's destination. Current CFSDrives has the
source Windows drive/device provider and explicit POSIX mounted-volume service;
there is no need for a second enumerator. Source implementation, command/key
consumers, namespace/back behavior and native/runtime/geometry checks remain
mandatory under the distinct checklist ID. That initial review copied no functional
code and did not close P3-05 or retract historical bounded navigation evidence.

The integrated iteration copies the original CApp::OpenRootFolder inline body
into App.h and CPanel::OpenDrivesFolder into the authoritative PanelFolderChange
module, with their original symbols, order, dependencies and notices. CFSDrives
remains the single Windows provider/POSIX mounted-volume owner; its enumeration,
device, I/O and binding bodies are unchanged. The CE filesystem-root branch is
retained but not a supported desktop runtime claim. Qt command presentation now
selects the source drive destination explicitly; Back's empty root marker still
selects the distinct source namespace. The list key adapter maps ordinary slash,
backslash and keypad slash characters to the same command, permitting Shift
character translation and excluding Control/Alt/Meta combinations.

Existing Qt filesystem history now stores a namespace's original folder prefix
instead of replacing every namespace with an empty root marker. Back rebinds
stored source paths through CPanel and the existing renderer; it does not invent
a drive or archive path algorithm. Source provider choice, archive-link cleanup,
peer panel state and centralized command availability remain authoritative.
Root namespace tests now reach that namespace through drive view's original
Parent command; native device/Hash fixtures retain their selected source branch.
An existing same-row mount selection exposed stale command availability after
original RefreshListCtrl restored focus under its notification guard. The Qt
renderer now recomputes centralized availability after that guarded source call;
the regression checks actual Open availability before activation. No selection
algorithm or menu policy is duplicated.

Actual previous-product RED reports empty address instead of Volumes/ (2 Qt
pass/1 fail/0 skip). New public root_command consumers cover four routes, both
panels, EN/RU normal/compact reports, exact drive provider, command state, Parent/
Back, namespace-prefix history and real ZIP/nested ZIP link release with unchanged
input bytes. Test-owned settings explicitly select English before per-row reload.
Six affected consumers pass8.42s after the command-state fix; initial root consumer
failure is preserved separately. Mandatory unsandboxed format/all-target/full gate
passes251/251 in105.56s: RootCommand22, RootNavigation8, PanelKey55 and AddressCombo13
Qt cases pass with0 fail/0 skip. All fifteen frozen code/build/test SHA inputs
remain unchanged after the gate. Four final EN/RU two-panel widget JSON reports
pass independent side-by-side, menu/toolbar/address/list/status order, exposure
and combo size-hint review. Baseline verification passes408 exact/196 adapted/
3 local paths at the recorded full revision. Fresh read-only guest probes show
Windows114192384 bytes and Linux15966208 bytes free, with no check processes.
The previous two scoped cleanup questions remain unanswered; no files/disks are
removed or resized. Native Windows/Linux current runtime, device/Hash consumers,
rendered reference, stable/cold and parent/final acceptance remain open.

Follow-up live-language consumer review at the same pinned revision inspected
OptionsDialog's active language apply path (menu/toolbars/ReloadLangItems followed
by RefreshAllPanels), CApp::ReloadLangItems, CRootFolder::Init/LoadItems and the
existing CFSDrives folder path property. The commented FM.cpp language-message
branch is not an active caller. Qt's live LanguageChange already updates labels
and root namespace names in place, but leaves bound drive prefixes and window
title in the previous language. Actual previous-product RED has22 Qt pass/2 fail:
both normal and pending-draft rows retain Volumes/ instead of Тома/.

No additional source module or algorithm is introduced. The Qt address adapter
reloads the existing CPanel::LoadFullPath and LoadFullPathAndShow path/icon/title
stage for each bound CFSDrives panel, without adding history. Original prefix
and pending editor text are compared before reload; Qt restores the draft when
different. The live-language owner skips this stage while operations/source
progress or source notification guards prevent safe projection. Rows, selection
and the existing provider remain untouched; there is no drive re-enumeration or
archive request. This narrower event presentation adapts the source refresh to
Qt's existing pending-input contract. New public consumers check EN→RU→EN on
both panels, focused title, retained selected row/command, Unicode pending draft,
zero archive list calls and eight normal/compact widget reports.
Four affected suites pass1.74s; mandatory unsandboxed format/all-target/full Mac
gate passes251/251 in106.97s, RootCommand24/RootNavigation8/LanguageFocus5/
LanguageDialog7 Qt cases with0 fail/0 skip. Four pre-gate source/test SHA inputs
remain unchanged. Eight final widget JSON reports independently pass visibility,
menu/toolbar/address/list/status order, side-by-side and combo size-hint review.
Windows/Linux current runtime and all previously recorded final acceptance
requirements remain open; this follow-up does not close P3-05.13 or its parent.

## P3-05.12 — RootFolder/address consumer reconciliation (implementation validation active, 2026-10-06)

Compared complete RootFolder.cpp/.h and active PanelFolderChange address
construction/edit/selection bodies and dependencies at
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17,
https://github.com/ip7z/7zip.git. The [omitted-scope review](omitted-scope-audit.md#rootfolder-and-address-combo-consumer-review-2026-10-06)
records retained root bodies, POSIX entries, Documents platform service,
CSIDL/icon boundary, ADS refusal-code difference and the missing original
hierarchical address dropdown. At discovery, MainWindow history population and
itemText activation are independent glue, not the original ComboBoxPaths
construction/binding algorithm. The [text contract](ui-layout-contracts.md#source-address-dropdown-p3-0512-required-remaining-scope)
defines the source reference and required integrated consumers/checks before
UI implementation. The initial docs-only review copied no code and passed diff
check; subsequent implementation is recorded below. The existing P0-04/P1-11
bounded accepted workflows remain historical
evidence and do not close this active source omission.

The integrated implementation now copies OnComboBoxCommand's hierarchy/UNC
prefix/root-prefix/path-parts/archive-link/virtual-entry construction and stored
path selection, AddComboBoxItem's path association, and OnNotifyComboBoxEnter/
EndEdit return/escape branches into the authoritative PanelFolderChange module.
RootFolder_GetName_Computer/Network/Documents again supply the source names.
Typed IQtPanelAddressCombo replaces ComboBoxEx/notification/platform calls;
QtPanelAddressCombo paints indentation, supplies cached QIcons, queries original
Windows MyGetLogicalDriveStrings or existing POSIX mounts, and posts list focus
through Qt's event queue. A queued request applies only while focus still belongs
to its panel, preserving a newer interaction in another panel/control: Qt can
deliver synchronous focus changes before its zero-delay request. Native CSIDL
image indices remain a Qt icon adaptation;
the existing icon cache has a narrow indexed QIcon accessor. POSIX adds the
already-owned RootFolder Volumes entry and skips Windows Documents/device/CE
services. Original algorithms, folder owners and notices remain; formatting is
separate from functional adaptation. No sibling source is edited.

MainWindow's independent history-only dropdown population is removed. Its
navigation boundary returns the source binding HRESULT for virtual/archival
paths and accepts a stored-source-path flag, preserving exact source paths
without typed-relative/display-separator rewriting. Existing file transport
startup remains asynchronous. FolderHistory stays persistent and its dialog
remains usable: centralized availability now reads the original history owner
instead of the unrelated dropdown count. PanelKey's prior direct Enter bypass
now calls original EndEdit, retaining the one-navigation key consumption and
adding the source focus/escape behavior. No independent navigation/path
construction or archive syntax is added to the widget.

Actual old-product address regression fails to find its current parent component
(2 Qt pass/1 fail/0 skip); its log is frozen separately. Integration public
consumers verify mouse selection of parent/Computer, current EN/RU hierarchy,
real ZIP and nested ZIP folder selection and original bytes, two independent panels and posted
Enter/Escape focus. The history test now restores persistent locations through
the source FoldersHistory dialog rather than asserting the removed dropdown
substitute. An initial two-panel focus regression exposed the remaining direct
PanelKey Enter bypass; it is corrected through the source handler. Test fixtures
also reset their own settings and wait for posted focus before another panel
interaction. Focused address/ui_navigation/root suites pass3/3 in1.54s.
Initial full250 finishes244/250 passed194.64s; six failed suites retain their
log separately. Besides history-only fixture assumptions, shared consumers
expose a queued focus request stealing a newer panel interaction. The scoped
Qt focus adaptation above corrects it without changing the source path algorithm.
History restart/deletion consumers now check the actual source dialog. Seven
affected consumers pass8.60s after these changes; the final close guard prevents
the new widget callback from retargeting a closed application. Twenty current
source/build/test SHA inputs are frozen for the subsequent final full gate.
Four current macOS normal/compact EN/RU widget JSON reports pass an independent
menu/toolbar/address/list/status order, exposure and size-hint review. Full Mac
format/all-target/full gate passes250/250 in105.54s, including AddressCombo9 Qt
pass/0 fail/0 skip, navigation10, operations34, PanelKey55, main shell6 and codec
commands24 Qt pass/0 fail/0 skip. All twenty frozen inputs remain SHA-identical
through the final gate. Four final EN/RU geometry reports are frozen/reviewed
separately from the initial full failure. Baseline verification passes408 exact/
196 adapted/3 local across the pinned revision. Fresh read-only capacity probes
report Windows121425920 bytes and Linux12533760 bytes available, with no check
processes; both scoped cleanup questions remain unanswered and no deletion or
resize is performed. Windows/Linux current build/runtime, native
UNC/device selection, ADS refusal-code regression and rendered/final acceptance
remain open. No passing whole-module or platform gate is inferred from these
focused consumers. The separate current-edit-image omission identified by this
review is addressed in the subsequent iteration below.

### Current address edit image — implementation/validation active

At the same full revision/repository above, reviewed original
PanelFolderChange.cpp::LoadFullPathAndShow, GetRealIconIndex_for_DirPath and
Find_FileExtension_DotPos_in_path; PanelItems::RefreshListCtrl, Panel.cpp's CE
message and OnComboBoxCommand selection callers; RootFolder name helpers,
SysIconUtils shell/cache services, FileFind/FileName and CFolderLink dependencies.
The three original bodies now reside in the authoritative PanelFolderChange
module. Root/device/super-prefix, archive-link/virtual-file, filesystem-check and
extension/directory fallback order remains. Portable GetWinAttrib and separator
checks replace the corresponding platform services; Win32-only device and
super-prefix calls stay guarded, and the original CE edit-image exclusion stays.
Source notices and language remain; no upstream edit or second path algorithm.

Typed header services replace shell image lookup and ComboBoxEx's edit item.
QFileIconProvider supplies path icons, with the existing source attribute/
extension cache as fallback; Qt standard namespace icons share that Qt index
space. A bounded platform path cache prevents repeated requests from allocating
an icon per refresh, and Clear resets the added caches. Native system-image-list
indices remain separate. A leading QLineEdit action presents the selected icon
without tying it to popup selection. Disk, archive (including asynchronous list
projection), and namespace renderers call this stage for their own panel. Explicit
display-path/history arguments preserve existing Qt archive formatting and
nested-history exclusion; icon classification always reads the source native
prefix. These are narrow existing presentation/persistence adaptations, not a
change to source navigation or archive operations.

The actual previous product fails six public icon rows (3 Qt pass/6 fail/0 skip);
its RED log is frozen separately. Current regression checks folder, real ZIP root,
virtual nested ZIP root, archive subfolders, and return to outer archive/disk,
while preserving input bytes. EN/RU normal/compact rows inspect leading-action
visibility and geometry. Focused address consumer passes in0.72s (9 Qt cases).
Mandatory unsandboxed format/all-target/full gate passes250/250 in103.85s.
AddressCombo9, PanelKey55 and MainShell6 Qt cases pass with0 fail/0 skip. All ten
frozen code/test inputs remain SHA-identical after this gate. Four final EN/RU
widget JSON reports are frozen and independently reviewed: main-shell order,
combo size hints, editor and leading icon fully exposed. The Qt internal icon
button follows the closed editor geometry (22×18, fitting the16×16 icon), rather
than the standalone toolbutton size hint. Baseline verification passes408 exact/
196 adapted/3 local paths at the recorded full revision. Windows/Linux current
native paths, device/UNC icons and rendered style remain unverified; both previous
scoped capacity-cleanup questions remain unanswered. Source ADS refusal and stale
selection contracts, native/runtime/cold-stable and final acceptance remain open.

### Stale hierarchy selection and posted focus — validation active

Reviewed complete BindToPath filesystem-prefix trimming, OnComboBoxCommand's
stored-path selection and OnNotifyComboBoxEnter return/focus branches at the same
revision. An absent path binds the deepest existing folder with S_OK; this is
successful fallback, not a fabricated error result. New EN/RU mouse/Return rows
open the real popup, remove only owned empty parent/child directories, then select
the stale parent. They verify the fallback prefix/list, command availability,
control bytes, no archive engine calls and a rebuilt popup without the old row.

The initial focus check exposed an offscreen activation boundary. A narrowed
actual-product RED reactivates only the test window and still finds pathCombo
focused (2 Qt pass/1 fail/0 skip); it never supplies list focus. The queued Qt
adapter previously discarded the source focus request while application focus
was temporarily null. Unconditionally allowing null fixes this row but fails
three existing peer-panel consumers; those failed logs remain separate. The
final adapter consults window()->focusWidget() when application focus is null.
That retained widget also records newer panel/control intent while inactive,
preserving the cancellation guard without activating a production window.
Source binding/path/command algorithms are unchanged. Five affected suites pass
in7.95s. Mandatory unsandboxed format/all-target/full gate passes250/250 in104.82s:
AddressCombo13, PanelKey55, MainShell6 and CodecCommandsUi24 Qt cases pass with0
fail/0 skip. Both frozen code/test inputs remain SHA-identical; four final EN/RU
widget reports pass an independent order/exposure review. Windows/Linux current
runtime and native refusal/device/UNC/rendered acceptance remain open; this Mac
fix does not establish platform acceptance. The separate root-command caller gap
discovered during this review is recorded under P3-05.13 above.

## P3-05.11 — source UNC root binding and Qt consumer (validation active, 2026-10-06)

Source repository https://github.com/ip7z/7zip.git, full revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Reviewed complete original
`CPP/7zip/UI/FileManager/RootFolder.cpp::CRootFolder::BindToFolder`, NetFolder
Init/bind/parent, CPanel BindToPath/OpenParentFolder and CApp::OnCopy, with
folder interfaces and existing Windows Net/FileName/FileIO dependencies.
Restored the original Windows separator-led fallback after failed CFSFolder
Init: construct CNetFolder, assign subFolder and Init(name2). Normalization,
filesystem-first ordering and original native resource discovery remain.
Existing NetFolder/NWindows::NNet are authoritative; no enumeration, UNC path
algorithm, credentials or archive transport implementation is added. Unix
mounted-network policy and portable source dependencies remain as before.
Notices remain; formatting is separate from adaptation.

Actual old-product direct binding returns E_NOTIMPL (3 Qt pass/1 fail/0 skip).
The actual address consumer also crashes in CFSFolder::GetRelPath: Qt reports
the UNC server as a directory, then the independent listing shortcut casts
the source namespace to CFSFolder. Existing native binding now runs before
normal/flat listing, and non-FS results use the existing renderBoundFolder
dispatcher before any filesystem cast. This narrow Qt type/presentation service
uses original CPanel's provider choice; scan, sort, hash and worker algorithms
remain. The first integration passes direct binding and all four server-address
rows reach the namespace, but Copy remains enabled without IFolderOperations.
Centralized availability now requires original DoesItSupportOperations, matching
CApp::OnCopy's existing refusal and disabling an unsupported command. Logical
drives remain Copy-disabled; operations-capable device/file/archive Copy remains.

Tests extract the existing native current-SID-only SDK share fixture for reuse.
Both direct provider and root fallback open an owned local share, read exact
Unicode/space binary bytes, refuse a stale share and reload without changing
source bytes. Separate actual-window EN/RU normal/flat consumers enter the server
address, open only that share, return with source focus, refuse after its deletion,
refresh and assert no engine work. Eight980×620/640×400 reports pass independent
menu/toolbar/address/list/status order, exposure and size-hint review. No external
host, live user archive, remote credential or global-network enumeration fixture
is used; those are not proven by this local scenario. The initial crash's single
orphan share was identified by owned snapshot path/name, one28-byte file and SHA
before removing only its publication; diagnostic input bytes were retained.

Native RED and first-integration failure each retain six immutable guest/local
SHA artifacts. Final six suites pass38.03s; direct4 Qt and actual-window6 Qt
pass without skips, alongside device Copy, namespace Hash and operation consumers.
Fourteen final SHA artifacts preserve eight exact source/test inputs, logs and
eight current geometry reports. Mac initial five suites pass7.35s and full249
passes102.72s before the final capability adjustment; final six suites pass14.82s.
Final Mac format/all-target/full passes249/249102.97s. Implementation commit
c8176f75f5a2cf5e654bad736db1f6e4444fb9ca retains the eight verified inputs.
Final native formatting/all-target build passes; full gate finishes255/257
passed865.62s. VerCtrl's source-buffer-size fixture cannot resize its file to
256 MiB + 1 byte; the subsequent capacity probe reports202420224 bytes free.
ArchiveExternalEdit's large-file Viewer case fails temporary-file removal;
its cause is not established by the capacity probe. This differs from the
earlier update-failure prompt timeout. All761 guest/local SHA artifacts are
preserved in the separate evidence-network-root-full directory; earlier phases
remain intact. Final Linux capacity/check, stable Windows gate, rendered Windows
parity and parent/final acceptance remain open. No cache deletion or disk resize
is performed while the scoped cleanup questions remain unanswered. A subsequent
unchanged-binary Highest desktop repeat passes archive_external_edit in12.79s;
that isolated pass does not explain or supersede the preserved full-gate failure.

## P3-05.10 — source caption helpers (validation active, 2026-10-06)

Source repository https://github.com/ip7z/7zip.git, full revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Reviewed full original
`CPP/7zip/UI/FileManager/LangUtils.cpp/.h` helper bodies, dependencies and
EditPage/ProgressDialog2/BenchmarkDialog resources, OnInit handlers and callers.
Copied LangSetDlgItems_Colon, LangSetDlgItems_RemoveColon and LangSetWindowText
into their adapted original module. Original selected-file lookup, append-colon,
remove-one-trailing-colon and missing-entry behavior remain; existing recursive
Qt mutex replaces the platform critical-section service. No parser, catalog or
translation algorithm is added. Formatting is separate from functional changes.

IQtLangWindowText and existing IQtLangDialogControls supply narrow text services.
Private QtLangCaption adapts original helper output to existing Qt presentation:
caption, accessibility, wrapping and suffix application. Initial bindings and
LanguageChange use the same source bodies. Window/resource captions preserve RC
fallback when the selected file lacks an entry. Benchmark speed invokes original
RemoveColon; Edit's viewer fallback retains exact `&View:` and Progress packed/
files labels retain their resource captions until the original colon helper runs.
The inactive Z7_LANG reference block is not a second functional implementation.

Plain Benchmark Cancel/Help, Link Cancel and Progress Cancel now invoke original
LangSetDlgItems with native control IDs through QtBindStandardResourceText.
Local conditional 2→402/9→409 mapping is removed. Ordinary text rebinding clears
that adapter flag, allowing Progress's later IDS_CLOSE binding to retain `&Close`.
Roles, signals, operation state and cancellation handlers are unchanged. Qt/native
presentation services remain local; upstream checkout and language assets are
unchanged. The helper implementation alone did not reconcile the entire module;
the following body review records its remaining boundaries.

### LangUtils complete body/dependency review, 2026-10-06

Compared the complete pinned LangUtils.cpp/.h with their adapted counterparts,
including globals, compile branches and callers, rather than counting filenames.
All active desktop body groups have these dispositions:

| Original body/dependency | Current authoritative body or narrow replacement |
| --- | --- |
| LangOpen; CLang storage/parser | Original `CLang::Open(..., "7-Zip")`; portable Common/Lang implementation is reused. No Qt parser/catalog replacement. |
| GetLangDirPrefix; NDLL module directory | Windows still calls original NDLL; POSIX uses Qt application directory. Existing installed Lang takes precedence; bundled files are staged to owned native paths because CLang consumes files. Explicit directory injection supports deterministic fixtures. |
| LoadLangOneTime; g_Loaded; critical section | Original one-time/reload sequence; recursive Qt mutex supplies synchronization, also protecting existing public readers and caption helpers. |
| LangSetDlgItemText, LangSetDlgItems/kLangPairs, Colon, RemoveColon, WindowText | Original lookup/table/loops/colon rules; typed Qt item/window services replace HWND/GetDlgItem/SetText. Missing selected entries retain the supplied resource caption. Initial binding and LanguageChange use the same source helpers. |
| LangString overloads, AddLangString, OnlyFromLangFile | Original selected-file lookup, resource fallback or empty-first policy. ResourceString owns compiled upstream STRINGTABLE fallback and bundled upstream English; native DLL overload retains LoadStringW for its separate HINSTANCE. |
| kLangs, FindShortNames, Lang_GetShortNames_for_DefaultLang | Windows retains exact active table and primary/sub-language algorithm, including system/user mismatch refusal. POSIX supplies QLocale regional/primary names because Windows LANGID services do not exist there. |
| OpenDefaultLang | Original bounded two attempts, regional before primary, filename construction and successful language-ID assignment. |
| ReloadLang; ReadRegLang | Original clear/default/explicit `-`/separator/dot rules; existing QSettings/path boundary replaces Registry storage. |

Original Z7_LANG-off header inlines select English-only resources; this Qt build
always supports the selected language through the active bodies, so the disabled
language build configuration is not provided. The non-Unicode g_IsNT guard
targets obsolete Win9x; supported Qt6 Windows targets use Unicode/NT. Commented
locale diagnostics, unused UI-language API experiment and trailing commented
table entries are not active runtime bodies. These are explicit platform/build
dispositions, not unsupported active desktop branches silently removed.

Reviewed all ten active original window-helper bindings: About, Password,
Messages, Mem, Split, Overwrite, Link, Benchmark, Compress and COptionsDialog.
Their existing Qt resource/window bindings reach the source helper; Compress
Options uses the source IDS_OPTIONS resource whose English text is exactly the
RC caption `Options`. Nonempty option checkbox captions are rebound through
QtBindResourceText after control registration, preserving RC fallback. The
Extract window-helper line is commented upstream and is not an active caller.
Original ordinary, colon/remove-colon and standard-control callers remain owned
by their per-dialog functional/resource audits and P3-05.9/.10 tests. This review
closes LangUtils body disposition, not all109 UI modules, final Linux runtime,
rendered parity or the parent acceptance gate.

Actual old-product Benchmark regression fails on Russian `Скорость:` versus
source `Скорость` (8 Qt pass/1 fail/0 skip). Public language fixtures cover
partial-resource fallback, initial/reload/window text, append/remove exactly one
colon, standard-control lookup and dynamic Close. Final macOS focused seven
consumer suites pass11.39s; unsandboxed format/all-target/full249PASS105.99s.
Forty-eight final Mac EN/RU normal/compact footer reports pass caption, exposure,
size-hint and separation review. Initial nine-file native helper focus passes
six suites22.15s without Qt skips;183 immutable guest/local SHA artifacts retain
that phase. Its nine-file full gate passes256/256990.94s with753 separate
guest/local SHA artifacts. Final eleven-input native seven-consumer focus passes
24.28s without Qt skips;193 immutable guest/local SHA artifacts retain current
inputs/logs/reports and48 footer reports pass review. Its separate final
format/all-target/full gate passes256/256931.02s with753 immutable guest/local
SHA artifacts, preserving the initial-input full and original failed run.
Eight final Mac Link footer reports also pass; compact content
scrolling is assessed by the existing public test, not simultaneous exposure of
every scrolled control. Linux capacity and final three-platform acceptance remain
pending; neither earlier inputs nor a passing Mac gate establish final acceptance.

## P3-05.8 — source namespace Hash consumer (validation active, 2026-10-06)

Source repository https://github.com/ip7z/7zip.git, full revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Reviewed original
`CPP/7zip/UI/FileManager/PanelCrc.cpp` bodies and CApp/Panel callers,
Is_IO_FS_Folder/GetFsPath, FSDrives/AltStreams providers, Windows FileFind
device/stream discovery and FileIO device-size/read dependencies. Existing
CalculateCrc2/CDirEnumerator/CThreadCrc remain authoritative: source scanning,
32KiB reads, progress, cancellation, errors and hash-result presentation are
reused without a new digest or device-reading implementation.

Qt calculateHash now accepts an operations-capable filesystem namespace;
the centralized command policy no longer excludes Windows FSDrives from its
existing hash-provider lookup. Unix mounted-volume providers without operations
retain disabled Hash. Logical Windows drives still refuse Copy. The source
base-prefix test adds an empty-string guard before Back(): GetFsPath intentionally
returns empty for logical Computer drives. Physical and ADS prefixes retain the
original handling; no source enumeration or read algorithm is replaced.

The first native integration passes availability/ADS but all six VHD rows fail
on success or recovery with ERROR_SECTOR_NOT_FOUND: the original raw CInFile
loop reads past this device's end. Reviewed complete unchanged
`CPP/7zip/Common/FileStreams.cpp/.h` Read/Open/size/alignment bodies and callers.
The Windows input adapter now reuses that existing CInFileStream through its
public ISequentialInStream interface, including the original Z7_DEVICE_FILE
bounded/aligned reads. The upstream module remains exact in third_party and
compiled once in sevenzip_common; Unix retains its existing CInFile/ReadFull.
Read failures preserve the source HRESULT, error count and progress reporting.
This necessary native-device adaptation preserves PanelCrc's hash loop and
cancellation cadence rather than recreating device I/O or hiding an end error.

Test-only native SDK fixtures own read-only, no-letter32MiB VHDs and Unicode/space
ADS files. They compare SHA256 with an independent oracle, assert no engine job,
unchanged input/selection and EN/RU normal/compact results. VHD cancellation and
sharing refusal both attempt recovery while the owned attachment remains live.
Before the fix, native Windows regression has2Qt pass/9fail/0skip3421ms:
logical/device Hash disabled and ADS dispatch produces no result. Original full
gate and derived RED logs are preserved separately. Final native regression
passes11Qt/0skip7460ms; nine affected suites pass24.02s, all required native cases
without skips. Forty-six immutable guest/local SHA artifacts include earlier
failures, final logs and20 EN/RU normal/compact result reports. Report review
confirms rows/footer exposure and separation, source button captions/order and
size hints. Final unsandboxed Mac format/build/full249PASS103.64s. Fresh Linux
first-integration f979d559 format passes but linking fails with ENOSPC before
CTest; source/logs remain preserved. Final native same-snapshot format/build/full
passes256/256967.73s;753 guest/local SHA artifacts are retained separately from
the original255-suite failure. Six amended source inputs identify this derived
gate without overwriting source.tar or the original run.json. Linux capacity and
final Linux checks remain pending; a warm passing native full does not establish
the parent cold/stable requirement.
Rendered Windows pixel parity and parent/final acceptance remain open.

## P3-05.9 — source standard-button language mapping (accepted button scope, 2026-10-06)

Repository https://github.com/ip7z/7zip.git, full source revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Reviewed LangUtils.cpp/.h bodies,
dialog OnInit callers, GuiCommon.rc and selected language resources. Restored
CIDLangPair to its original LangUtils header owner (MyLoadMenu now reuses it),
kLangPairs, LangSetDlgItemText and LangSetDlgItems in their original module.
The original standard-table pass, selected-language lookup, missing-entry
policy and custom-ID pass remain; IQtLangDialogControls replaces native
GetDlgItem/CWindow text services. Existing recursive Qt mutex protects lookup.
This scope does not claim complete LangUtils body reconciliation.

QtLangButtonBox supplies native control IDs and original resource captions.
Its changeEvent applies the source mapping after Qt's own standard-caption
reset, preserving Memory's IDCONTINUE rather than resetting it to IDOK.
Copy, Split, Combo, Password, Memory, Edit, ListView, About, Extract, Compress,
compression options and BrowseDialog2 use this adapter; their existing roles,
signals and functional handlers remain. The source Options property-sheet
footer and Qt native dialogs keep their platform-owned language behavior.
No second language-ID table or translated literal catalog is introduced.

Old-product public regression: sixteen failing Qt rows across eight dialogs,
with About unchanged, plus Memory reload-to-OK. Current macOS focused test
passes27 Qt cases/0skip, covering twelve source dialog consumers, hot reload,
all seven mappings and absent-language resource fallback. Unsandboxed macOS
format/all-target/full248PASS102.99s; localized contract-size check passes four
suites2.57s. Forty-eight EN/RU normal/compact reports reviewed: footer order,
source captions, button size hints, clipping and exposure. Fresh Linux8d4f93c9
uses the same27 frozen inputs and passes the full244 suites196.02s;
696guest/local SHA artifacts and48 footer reports verified. Its27 Qt language
cases have no skips. Native Windows1bee2e9c uses those same27 inputs: existing
language_dialog passes7 Qt cases/0skip1230ms and language_standard_buttons
passes27/0skip2988ms, CTest1.81/3.61s. Forty-nine immutable guest/local SHA
artifacts preserve those completed sections and48 reports; footer source
captions, exposure, width/height hints and pairwise separation pass review.
The full Windows255 gate completed251/255PASS1194.85s, with733 guest/local SHA
artifacts. Failures are native_mapi wide-keep exit2, external_codecs callback5150ms,
codec_probe failure callback7350ms (both5000ms deadline), and large-list
disk-normal gap265ms/250ms at4096 rows. Their causes and stable/cold acceptance
remain P3-03; language suites pass independently. This accepts the shared
standard-button consumer scope; rendered parity, remaining language helpers
in P3-05.10 and parent/final acceptance remain open.

## P3-05.7 — Windows device-image provider and F5 consumer (2026-10-06)

Source repository https://github.com/ip7z/7zip.git, revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Original
`CPP/7zip/UI/FileManager/FSDrives.cpp/.h` bodies, callers in RootFolder/App/PanelCopy,
types, `FM.mak` device definition and Windows FileIO/FileFind dependencies reviewed.
The adapted module now copies `CDriveInfo::GetDeviceFileIoName`, CPhysTempBuffer,
CopyFileSpec, the physical-device LoadItems branch, GetFileSize, CopyTo, system
icon method and refused mutators. The original first-sixteen enumeration, naming,
4MiB copy loop, callback sequence and partial-file cancellation policy remain.
Includes/types are adjusted to existing portable owners; formatting is separate
from body adaptation. No new archive or image-format algorithm is implemented.

Windows alone exposes the original IFolderOperations/IFolderGetSystemIconIndex
interfaces. Existing Unix mounted-volume provider and its interface stay intact.
`Z7_DEVICE_FILE` is a public Windows sevenzip_common definition, so original
FileIO/FileFind and consumers share the same CFileBase layout and device-size
logic. Allocation uses the original C/Alloc.h and C implementation.
The unchanged native Windows FileSystem.cpp now belongs to sevenzip_common:
Z7_DEVICE_FILE makes original FileIO/FileFind call MyGetDiskFreeSpace, including
direct common-only DLL consumers. The module is compiled once, retaining its
vendor path/body; filesystem providers inherit that common dependency.
Native shell icons reuse the existing SysIconUtils adapter. Its entire authoritative module
moves from the panel's source list into a shared sevenzip_system_icons target:
filesystem providers and panel consumers can use it without a panel/filesystem
dependency cycle or a duplicate shell helper. It retains its original path,
symbols and bodies; the shared target owns Qt Widgets and native shell32 links.
Qt namespace rendering reads
source kpidIsDir; F5's device destination uses existing CPanel::CopyTo and source
progress/callback transport. Source CopyTo always calls CopyFileSpec with
writeToDisk=false; Move, CopyFrom and mutators remain E_NOTIMPL. Inactive commented
physical-folder binding is not enabled. Broader native open consumer review
and full omitted-scope acceptance remain pending. P3-05.8 above records the later
drive/device Hash consumer and its separate validation.
The new operations interface also applies to logical Computer drives; centralized
Copy availability excludes that non-device mode, which original CopyTo refuses
and the Qt dispatcher does not consume. A public logical-drive regression guards
against exposing Copy without a working consumer; the later Hash integration
deliberately replaces its disabled-Hash assertion with enabled availability.
P3-05.9 owns the shared LangSetDlgItems standard-button mapping: the RU Copy
reports expose an English Cancel footer, not corrected by this provider port.

The new focused direct/UI regressions own a disposable blank32MiB SDK VHD,
attached read-only with no drive letter. Upstream has no VHD fixture implementation;
this test-only SDK integration reuses original CAccessToken and restores only
its process privilege state. Six direct scenarios cover file/directory output,
Cancel, failed destination, skip and Move refusal. Four EN/RU native F5 scenarios
cover Copy/dialog Cancel and normal/compact geometry. Native old-product RED
on snapshot2f03bc0e: direct6 failures at LoadItems E_NOTIMPL and UI4 missing device
row, all0skip;17 guest/local SHA artifacts retained. Two earlier SDK build errors
and the amended test-input SHA manifests are preserved separately. The RED
snapshot uses unchanged production code and amended SDK test headers only.
Corrected snapshotf9d071cd, ten final code inputs plus a temporary wrapper frozen
at sourceSHA9fb141e759057c77c569ab4f824bf9db878dbf01d2b2d28f84cf22c14fd61f8b,
passes four native focused suites9.01s: device8Qt/UI6Qt, all0skip. Twenty guest/local
SHA artifacts include16 EN/RU normal/compact reports; grouping, order, control
sizes, disabled states and clipping reviewed against the text contracts. First
green candidate5fe92a62 fails compilation at missing QIcon include dependency;
that failure prompted the shared icon target above and remains recorded.
Mac affected4/4PASS0.84s; final unsandboxed format/all-target/full247PASS98.90s
after shared-icon/capability changes. Fresh Linuxa9b4d83b full243PASS192.26s,
648guest/localSHA, uses the same ten frozen code inputs before the Windows-only
FileSystem target correction. Initial native full32d2ab70 sourceSHA
d3e1bde6cb6ad207586b73a2ee342bba308da06fe561f19a959696a5df0750c7 fails127 while
linking native_module_path_fixture: original FileIO/FileFind reference
MyGetDiskFreeSpace without a common FileSystem owner. Its exact single-file
CMake correction is recorded as original/amended SHA, with original log/token/
exit preserved. Derived native_library/device/UI focused3PASS8.20s; full Windows
252/254PASS1105.61s fails unchanged process_pause timeout and large-list heartbeat
286/266ms at4096 rows;685 guest/local SHA retained. Subsequent logical-drive
availability regression fails on that candidate, then passes after the final
centralized capability guard: UI7Qt/0skip and unchanged process_pause4Qt/0skip,
focused2PASS4.68s with13 immutable SHA artifacts. The final same-source Windows
gate passes254/254 in808.73s;15 immutable logical-guard artifacts and a separate
685SHA full evidence collection preserve the previous failed full results.
Required native cases have no skips; only two explicit POSIX contracts skip.
This warm replay does not establish cold codec or stable performance acceptance.

Final eleven code inputs match Linuxaf428c09 archive and actual amended Windows
sources. Mac final unsandboxed full247PASS106.55s. Linuxaf428c09 initially stops
at ENOSPC; after source/terminal/no-active/648SHA guards, only the previous own
completed build/dev is removed. Same-source full retry242/243PASS127.56s has an
unchanged temp_cleanup timeout;645SHA retained, then temp_cleanup and three
command/root consumers pass4/4 in0.56s. The device-image component is accepted
within this source/Copy scope. P3-03 stable platform checks, P3-05.8/.9, parent omitted-scope
acceptance and rendered Windows device parity remain open.

## P3-03.1 — Finder drag-out lifetime correction (2026-10-05)

Source repository https://github.com/ip7z/7zip.git, revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17; original
`CPP/7zip/UI/FileManager/PanelDrag.cpp` `CPanel::OnDrag`,
`CDropSource::QueryContinueDrag`, `CDataObject::CopyFromPanelTo_Folder`,
effect/temporary-directory comments and current typed transfer callers reviewed.
No upstream file or adapted algorithm changed: only local Qt transport's
receiver-completion decision and public regression changed. Dependencies remain
QDrag/QMimeData and the existing owned-temp lease; no borrowed files added.

User manually dragged the disposable ZIP's binary Unicode/spaced item into
Finder twice on macOS 27.0.1, Qt 6.11.2, base
f4959976abc92f9d9fef33ff47019170149cc42e, at 19:13:57/19:14:04 Moscow time.
Both attempts returned -8058. Finder's CopyEngine recorded NSCocoaErrorDomain
Code 4/file missing and a Move attempt. Reading only the drag pasteboard showed
public.file-url naming the extracted item; the temporary root was already gone.
Destination stayed empty; source ZIP's expected digest is retained in the
disposable fixture baseline. This is failed native evidence, not acceptance.

[Qt 6.11.2 Cocoa dragging implementation](https://github.com/qt/qtbase/blob/v6.11.2/src/plugins/platforms/cocoa/qnsview_dragging.mm)
does not set the accepted external action while currentDrag exists. Its native
IgnoreAction cannot prove receiver cancellation. `QtPanelDragTransport` now
keeps archive bytes protected for that ambiguous default macOS executor outcome.
Typed completed/cancelled internal recipients and authoritative controlled
executor cancellations still release protection. Native Escape/refusal may
retain a protected root; no timer or guessed delay authorizes deletion, and
source selection stays marked when Qt reports IgnoreAction. Native retry and
cancellation evidence are recorded below; arbitrary receiver completion is not
inferred from that single transfer.

`drag_archive` adds a macOS row using real default QDrag::exec on the offscreen
platform. It fails before the fix (owned entries 0 rather than 1), and checks
retained payload bytes, immutable source ZIP, inactive protected ownership and
cleanup refusal after native IgnoreAction. This checks the conservative transport
contract; offscreen IgnoreAction is not native Finder proof.

Validation of the task diff on macOS arm64, 2026-10-05: affected app/test targets
built, final six focused drag/temp suites PASS 3.34s. First full gate retained
242/243 PASS with two old UI cancellation expectations failing inside drag_drop_ui;
those table/icon rows now check readable bytes, inactive external protection and
cleanup refusal after default native IgnoreAction. Final unsandboxed check.sh
format/all-target build/243 CTest PASS 98.93s. Windows/Linux checks were not run
for this correction; the new conservative native branch is macOS-only.

User retried in a fresh corrected app process and confirmed successful Finder
transfer without error. The received binary item is 1,048,576 bytes with SHA-256
fbbab289f7f94b25736c58be46a994c441fd02552cc6022352e3d86d2fab7c83, exactly matching
the input; original ZIP SHA-256 remains
5a261e25676a704cabeb4c79ead71a8e80f421c0731a9ec324164cdefdeaff83.
Finder moved the temporary item; its owned root remains externally protected
after app drag return. This confirms native recipient lifetime for that transfer,
not a native completion acknowledgement for arbitrary receivers.
User then confirmed native Escape cancellation. The cancelled destination has
no payload; received bytes still match and original ZIP still has its baseline
digest. The cancelled drag's temporary payload remains readable with the same
SHA-256 and external protection, consistent with the documented conservative
cleanup boundary. No successful cancellation is inferred from automation.
P3-03.1 accepted on macOS, 2026-10-05: native transfer, Unicode/spaces, exact bytes,
source preservation, actual failure/fix/retry and manual Escape verified.
Automatic native receiver completion/cleanup is not claimed; protected retained
roots are the explicit platform adaptation. Parent P3-03/P3-06 remain open for
their other required platform/workflow checks. Final docs-only follow-up passed
git diff --check; code is unchanged since the passing full gate.

## P3-05.6 — filesystem description comments (2026-10-06)

Source https://github.com/ip7z/7zip.git, full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed
`CPP/7zip/UI/FileManager/FSFolder.cpp/.h` LoadComments, SaveComments,
SetProperty/GetProperty, Files/CDirItem Parent and cache resets; TextPairs.cpp/.h
CPairsStorage lookup, serialization and trim handling; PanelOperations.cpp
ChangeComment and MyLoadMenu.cpp Comment/read-only/hash dispatch; ComboDialog
resources/OnOK and existing Qt source property-worker callers. Original
SaveComments and complete SetProperty branches are restored in their original
module with original names/types/control flow. Windows attribute-aware file
creation/write stays native. POSIX selects the existing portable source
Create_ALWAYS/write_full, preserving existing permissions; no Qt serializer,
format parser, new I/O algorithm or upstream edits. Existing UTFConvert,
FileIO/FileFind, CPairsStorage and notices are reused unchanged. The provider's
existing Qt capability interface now permits CPanel's shared command state to
reach the source dialog/worker. No second command-availability owner is added.

Invalid index/type/unsupported property and flat nested Parent branches retain
their source HRESULTs. Empty and trimmed-empty remove a pair; non-ASCII output
retains source UTF-8 BOM/CRLF; other loaded pairs and source file bytes remain.
SetProperty ignores SaveComments's false return and write success remains
unchecked in the original flow. A directory at descript.ion proves the same
S_OK failure policy without privileged filesystem assumptions; reload finds no
persisted comment. This source limitation is explicit rather than corrected by
an independent error policy. Direct callers must prime existing pairs through
the source read path, as ChangeComment already does.

Direct property RED2Qt before restoration; initial UI fixture used an archive
path role and a macOS-normalized filename, so those failed/timeout logs were
retained and the fixture corrected to the filesystem native-name role and
Cyrillic/CJK without an OS-composed letter. A separate real File Comment RED
with the old FS provider capability absent proves disabled state; after
restoration the same command passes. Public tests include success, invalid
input, empty/delete, flat nested refusal, write refusal policy, Ctrl+Z/Escape,
OK, other-pair/byte/permission preservation and source selection restoration.
The cohesive component test stays in its own suite; the UI case's dialog
responder and scoped fixture account for the function-size review trigger.

macOS27.0.1 arm64 Qt6.12 focused filesystem_comment/fs_folder/native_comment_ui
3/3 PASS5.06s. Eight EN/RU normal/compact JSON reviewed at360×150/260×150:
fitted prompt, editable combo and separate visible/enabled OK/Cancel. Final
affected4/4 PASS4.88s also covers main_shell and source ShowDots parent guard.
First full246/247 failed only the old always-disabled Comment shell assertion;
second full246/247 failed the new fixture's assumption that ShowDots defaults
on. Both failed logs are retained. The shell now checks source command ID/
shortcut; the component proves enabled focused-file and disabled parent states
with ShowDots explicitly enabled. Final unsandboxed formatting/all-targets/
247CTest PASS104.05s on unchanged production code.

Frozen `20261005T234212Z-b70429b5`, source SHA256
`98ea69b437adeb7b9322354965cd18ebab325f94aa67a5ca55a21faba304e905`,
matches the ten committed production/test/CMake inputs at29bb697. Linux Qt6.12
focused14/14 PASS22.92s and full formatting/build/243CTest PASS190.59s;
648 full artifacts and two focused logs have matching guest/local SHA256.
Filesystem comments9Qt/0skip and eight EN/RU normal/compact JSON pass.
The same Windows snapshot fails two Ctrl+Z cancel rows: the fixture stops its
responder immediately after the key event without awaiting active-window or
dialog delivery. codec_probe also times out at20s
without observing its Unicode entry marker. Whole focused12/14 remains failed,
and its full gate is not reached. These failures remain retained evidence.

The localized fixture correction waits for the active window and observed dialog
while the responder stays live. Slow-module marker paths use ASCII Base64 of
UTF-8 and explicit decoding, avoiding a local-code-page interpretation of raw
UTF-8 environment bytes on Windows. No production behavior, deadline, heartbeat
threshold or scenario coverage changes. macOS final affected2/2 PASS5.40s.
Fresh native Windows `20261005T235145Z-5da3edfc`, source SHA256
`1f6107c955dab391c2fac79bb22fdfa4b72d340412b3a8f74d27113ce3464366`,
contains exactly those three test changes plus the same temporary validation
wrapper; four frozen input byte checks pass. Filesystem comments9Qt/0skip,
all14 focused consumers and eight native EN/RU normal/compact reports pass;
30 focused artifacts and two phase logs have matching guest/local SHA256.
P3-05.6's writer/command component is accepted on three OS. Its fresh Windows
full run finishes241/252 PASS1183.71s under the Limited runner token, with
669 artifacts guest/local SHA256 verified. Filesystem comments9Qt/0skip pass;
11 privileged native fixture failures remain recorded in P3-03.10 below.
The subsequent macOS full247/247 PASS85.08s verifies the committed shortcut
and marker corrections. Rendered reference and parent P3-05's remaining
source bodies/resources/dependencies remain open.

## P3-03.9 — native editor prompt failure investigation (2026-10-06)

Frozen `20261005T223534Z-e1fa25e5` whole Windows gate finishes251/252,
922.64s,661guest/localSHA; only archive_external_edit fails. Its editor-no and
viewer-yes rows do not observe externalUpdatePrompt in the unchanged five-second
wait. Source revision `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git: reviewed PanelItemOpen.cpp MyThreadFunction,
CChildProcesses/Wait and Panel.h WasChanged_from_TempFileInfo, plus their current
IQtProcessServices/native-handle, QtExternalGuiCall readiness and MainWindow
CanHandleExternalUpdate callers. Baseline file metadata is captured before
launch, and original size/timestamp and child-process decisions remain in use.
These reads do not yet prove which completion/readiness phase causes the failure.

The existing public test assertion now reports child-finished marker, temporary
file size, backend busy and active-modal object name on failure. It retains the
same QTRY deadline and adds no extra wait, production hook or archive payload
logging. macOS affected archive_external_edit PASS6.74s after the localized
diagnostic addition; production remains the code verified by full246/246 in
P3-03.8.1. Windows focused execution in b70429b5 passes15Qt/0skip15.29s,
including both previously failing rows. The fresh5da3edfc focused consumers also
pass; full5da3edfc also passes15Qt/0skip11643ms. That whole gate fails on the
Limited token's privileged fixtures, recorded in P3-03.10. No functional fix or stable whole Windows
acceptance is claimed from these focused runs.

## P3-03.10 — explicit native runner elevation (2026-10-06)

Frozen Windows `20261005T235145Z-5da3edfc` finishes241/252 PASS1183.71s,
669 guest/local SHA256 artifacts. This invocation mistakenly used the default
Limited interactive principal rather than earlier private Highest validation.
Failures: owned_temp, temp_cleanup, source_extract_advanced,
source_link_extract, link_dialog, network_folder, archive_creation_lifecycle,
filesystem_link, project_memory_setup, source_link_storage, archive_metadata_ui.
Native symlink/security prerequisites are denied; project_memory_setup fails
on its native `.codex/README.md` symlink, and NetShareAdd returns Access Denied5.
No required Windows scenario is skipped or relaxed. Two POSIX-only skips remain
explicit. Privacy inspection667 artifacts/10 markers finds no fixture payload
leaks; five matching Qt row tags are permitted scenario identifiers.

`hyperv-runner.py` and `hyperv_interactive.py` now accept explicit
`--interactive --elevated` only for Windows-containing check/run requests.
Default remains Limited; all's Linux path stays ordinary. Highest belongs only
to the snapshot's owned scheduled task; the child verifies the actual native
administrator token before invoking the unchanged check.cmd. Requested elevation
in run.json is distinct from actual token evidence in interactive-session.txt.
No product algorithm, deadline, UAC/account/user-rights or machine policy changes.
The ordinary native COM consumer retains its desktop-token child.

Offline17 regressions pass, including main routing, invalid combinations,
Windows-only propagation, unchanged Limited payload and actual-token guard.
Unsandboxed macOS format/all-targets/full247CTest PASS85.08s; affected offline17
rerun passes after the localized CLI assertion. Fresh native validation
`20261006T002830Z-28cc3a3c`, sourceSHA256
`8b57e3a3bc7ad636866bfe9d930f703d9c53719e06318d8d4b7c2af8e4766cd2`,
proves Session1/Highest/Elevated=True through the project helper. Focused4/4
PASS7.80s: owned_temp11Qt, temp_cleanup11Qt, filesystem_link30Qt and ordinary
native_menu_integration3Qt, all zero skips. The pinned original rendered 7zFM
System-menu reference consumer also passes3Qt/0skip,1/1 PASS2.93s. Four immutable
logs/token/reference artifacts have matching guest/local SHA256; eight snapshot
inputs are verified against the working tree. Registered native full count253
includes the rendered consumer. P3-03.10 accepts the scoped runner prerequisite.
Subsequent focused14 finishes13/14 PASS108.76s, only external_codecs failing:
its five-second ready assertion requires6850ms in initTestCase. Full gate is
not reached. All39 terminal focused/build/UI artifacts are guest/local SHA256
verified; original and Qt affected System-menu captures show the same labels,
order, separators and states, with their native/Qt font and width adaptation.
Privacy scan37 logs/JSON against10 fixture markers reports zero payload leaks.
This narrow rendered comparison does not accept other dialogs or final P3-06.
No mandatory test or deadline is weakened; parent P3-03.8 and P3-03 stay open.

## P3-03.8.1 — owned codec discovery loader (2026-10-06)

Reviewed source https://github.com/ip7z/7zip.git, full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/Common/LoadCodecs.cpp/.h` IsSupportedDll, LoadDll,
LoadCodecs/LoadFormats, LoadDllsFromFolder, Load, SetLibCodecs and CCodecs/CCodecLib
ownership; `CPP/7zip/UI/Agent/Agent.cpp` LoadGlobalCodecs/FreeGlobalCodecs and
Open callers; `CPP/7zip/UI/GUI/GUI.cpp` Main2 loading caller. Original loading is
synchronous; it supplies no asynchronous Qt process-version/runtime-image delivery
owner. All original loading bodies, ABI checks, enumeration, dependencies and
notices remain unchanged. ArchiveCodecProbe's local scheduling now runs the
existing ArchiveCodecCatalog constructor in a joined QThread with immutable
inputs; the constructor still owns image copying and those source calls.
No new codec registration/discovery algorithm or borrowed module is added.
Qt owns queued completion, cancellation/deadline state and thread lifetime.

Four slow-module public regressions initially fail because GUI delivery cannot
observe native loading until it has already completed. After the change they
check GUI heartbeat/owning-thread callback, cancel and deadline, restart and
destruction with joined native exit and no stale callback. Native DLL entrypoints
have no interruption interface; cancellation discards their result after join,
and destruction waits for them. The five-second default is unchanged and also
covers the native phase. Only test fixtures deliberately hold a compatibility
query; production has no timing hooks or test-specific branches.

macOS Qt6.12 focused codec_probe/catalog/external_codecs3/3 PASS4.81s,
new probe13Qt/0skip. Direct native phase observation: process/start159–850ms,
total744–1482ms, maximum GUI heartbeat gap24–41ms with600ms native query.
Full unsandboxed format/all-target build/246CTest PASS102.20s. A localized
test-fixture adjustment leaves1500ms cold process-start headroom and holds the
native deadline query2100ms; production code/deadline are unchanged. Final
affected3/3 PASS6.43s after that adjustment. The new test is one cohesive public
probe-lifecycle scenario; its scoped environment/marker setup accounts for the
function-size review trigger, with no test access to probe internals.
Linux snapshot `20261005T230017Z-5b0d305c`, source SHA256
`a557425799c2997e7cbd66a840db90c48488ab126073fb0a8ae54f42df795891`,
contains the committed6896b96 loader; its five changed code/test/CMake inputs
match the committed bytes. Ten focused consumers pass with0skip, including
archive_external_edit15Qt, codec_probe13Qt and codec_update73Qt. Two immutable
focused logs have matching guest/local SHA; native probe process/start50ms,
total656ms (cancel/failure) or2169ms (deadline), maximum heartbeat gap11ms.
Linux full format/build/242CTest PASS187.89s. Later b70429b5 full243/243
PASS190.59s and focused14/14 preserve the loader; native process/start50–51ms,
total656–657ms or2121ms, maximum heartbeat gap11ms.
Windows b70429b5's slow-module test times out without its Unicode marker; the
localized ASCII environment transport correction described in P3-05.6 preserves
Unicode filesystem paths and all four lifecycle rows. Fresh native5da3edfc
codec_probe13Qt/0skip and all14 focused consumers pass. Two immutable phase logs
have matching guest/local SHA256. Direct native process/start78–79ms,
total689–690ms or2166ms, maximum heartbeat gap26–28ms; joined destruction passes.
The bounded owned-loader component P3-03.8.1 is accepted on three OS.
Real cold runtime-copy/DLL/version-process phase timing is not yet established;
these checks do not prove the cause of the earlier Windows5750ms ready signal
or accept parent P3-03.8's stable cold five-second completion requirement.

Diagnostic-only derived snapshot `20261006T003724Z-20a5b738`, sourceSHA256
`ff62d5b60a0a48c8721af5680850872bed9efc4c953c736dc41e2b15bcf002ad`,
adds fixed-label monotonic phase/byte-count logging to four private integration
copies, preserving native bodies and all deadlines. Their original/derived
hashes and original files are retained; finally restores all original sources.
Initial CTest uses -silent and suppresses qInfo phases: external_codecs13Qt
passes all three rounds; first codec_probe round fails heartbeat636ms against
unchanged250ms, later two rounds pass. This is derived diagnostic evidence,
not an unchanged-product failure or full gate acceptance.

Sequential native Session1/Highest verbose direct consumers reuse those saved
derived binaries, keeping source files restored and preserving the initial
batch/logs/token. Three external and three probe runs pass13Qt/0skip each.
First-discovery warm timings: version-process117–151ms, image prepare4ms,
main native load19–20ms, worker-to-GUI delivery4–9ms, backend ready165–198ms;
SetLibCodecs takes under1ms. Logging contains no paths or payload; all27 retained
artifacts have matching guest/local SHA256. These warm measurements do not prove
which cold phase caused5750/6850ms. A fresh first-visible diagnostic is required;
parent P3-03.8 remains active, and no speculative cache or deadline change is made.

## P3-03.8.2 — joined process/version discovery owner (accepted scheduling scope, 2026-10-06)

First-visible diagnostic `20261006T004424Z-c95da973`, sourceSHA256
`9fff66a512090f58c7b282fe499a0a7271f449341a20343cb98325ddc9b16fdb`,
retains18 guest/local SHA256 artifacts. All three external-codec direct runs
pass13Qt; initial ready964ms includes version-process901ms, image5ms,
main native load23ms and worker/GUI11ms. First codec_probe row fails heartbeat
548ms with process-started473ms after Start; later two probe suites pass.
Privacy9 logs/JSON/10 markers finds no fixture payload. This pinpoints a remaining
caller-thread phase, not the cause of the historical external6850ms completion.

Reviewed original `CPP/Windows/ProcessUtils.cpp` CProcess::Create/MyCreateProcess
and `CPP/7zip/UI/FileManager/PanelItemOpen.cpp` native launch, MyThreadFunction,
Thread_Create and joined exit callers at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
https://github.com/ip7z/7zip.git. Those process bodies launch external programs;
they do not capture/parse the integration's version output. The original
LoadCodecs/Agent native metadata owner remains authoritative and unchanged.
[Qt's QProcess contract](https://doc.qt.io/qt-6/qprocess.html#CreateProcessArgumentModifier-typedef)
and [Qt's Windows implementation](https://github.com/qt/qtbase/blob/dev/src/corelib/io/qprocess_win.cpp)
confirm the platform CreateProcess call in start; the latter is a supporting
implementation read, not a borrowed/pinned module or claimed6.12 source baseline.

Local ArchiveCodecProbe now schedules the whole existing transport in its owned
thread: module lookup, local QProcess creation/read/reap, unchanged bounded8192
output/version/library parsing, and existing ArchiveCodecCatalog construction.
Engine/arguments stay separate; no widget builds syntax. Atomic cancel/deadline
requests bridge the GUI timer to20ms process-wait checkpoints. Native create/DLL
entrypoints cannot be forcibly interrupted; closing joins them, then kills/reaps
an owned child and retains leases until completion. GUI completion and backend's
queued publication remain on their owners. Default5000ms and heartbeat250ms
are unchanged; no module cache, engine/plugin sharing or security-policy change.

The Windows-only regression uses SDK import-table declarations to replace only
the current test process's Qt CreateProcessW import for its scope, delay600ms,
then invoke the real API. It restores the import after the probe joins; no
product-only test hook, global OS hook or user process is involved. No equivalent
upstream fixture implementation exists. Public tests verify prompt return,
worker caller, GUI heartbeat/completion, failure/cancel/deadline/destruction and
no running child. Frozen unchanged production `20261006T005123Z-eaf10702`,
sourceSHA256`7cc7fbd350aac7fc87824dabed1335c82115aa5a79fb0e91281c80907817adbb`,
is RED13Qtpass/4fail/0skip9.33s: all four new rows block the caller. Five original
native log/build artifacts have matching guest/local SHA256.

macOS affected4/4 PASS6.57s; unsandboxed formatting/all-targets/full247CTest
PASS105.87s. Fresh shared Linux/native Highest Windows validation
`20261006T005414Z-7658306b`, sourceSHA256
`398821b37a2b26fb7c84a6939d026a81ad3dd172207b300a9c96126377397521`,
has seven frozen input byte checks. Linux focused15 PASS24.71s and full formatting/
all-targets/243CTest PASS187.78s;648 build/test/UI artifacts have matching guest/
local SHA256. Windows original rendered menu PASS3.30s, but focused13/15 fails:
external incompatible-module delivery exceeds5000ms (6700ms diagnostic), and
the new process failure row exceeds5000ms (8800ms), followed by the unchanged
20s CTest timeout. Thirty-five original build/test/UI artifacts have matching
guest/local SHA256. Three subsequent direct repeats of the exact saved binaries
pass external13Qt and probe17Qt, including all four new lifecycle rows, with
zero skips; seventeen repeat artifacts have matching guest/local SHA256.
Native loading heartbeat in the first repeat is18–20ms; all four delayed-start
rows enforce prompt return and250ms heartbeat. These repeats do not establish
cold stability. Warm focused15 PASS42.09s on the same snapshot; full formatting/
all-targets build passes, full251/253 fails989.14s with actual exit8. Probe17Qt/
0skip7583ms and external13Qt/0skip3055ms pass in the full gate. The two failures
are external-editor update-failure (error window not delivered before5000ms,
14Qtpass/1fail26.29s) and large-list disk-normal heartbeat285ms at2240 rows
(250ms limit,13Qtpass/1fail89.05s). Parent P3-03.9/.3 retain them; no required
case or limit is weakened. Privileged owned-temp11Qt, cleanup11Qt, filesystem
links30Qt and network3Qt pass with zero required skips. Only the two explicitly
POSIX-specific cleanup/lstat rows skip on Windows.

673 full build/test/UI and9 separately preserved warm gate artifacts have
matching guest/local SHA256; the initial35 failure artifacts are retained
separately. Source hash, actual terminal exit8 and absence of live own processes
are verified before final collection. The remote wrapper's exit0 is not the
gate result. Privacy scan of1363 current evidence text/JSON artifacts finds no
fixture-password exposure (ten allowed Qt row labels).

The bounded process/version scheduling and joined lifecycle component is
accepted on three OS and committed separately. Stable cold completion remains
parent P3-03.8, and the combined Windows/P3-03 gate remains unaccepted. Device
fixture drafts prepared afterward are outside this frozen/tested source scope.

## P3-05.5 — native archive property levels and codec owners (accepted property scope, 2026-10-06)

Source https://github.com/ip7z/7zip.git, full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed original Agent.cpp
GetArcNumLevels/GetArcProp/GetArcNumProps/GetArcPropInfo and their MainItemProperty
counterparts, Agent.h GetTypeOfArc, PanelMenu Properties, FileFolderPluginOpen
GetFolderLevels/GetFolderError, CArchiveLink Open/Open3/Release and CArcErrorInfo/
ReadBasicProps, ParseOpenTypes/ParseType, and Split/Gzip stream interfaces.
The original property bodies now read the retained native CArchiveLink and
resolve indices against its exact retained CCodecs owner. Both kpidErrorType
branches and Parser type are restored. Parent-item property calls preserve their
original level/SubfileIndex flow. Process-only readers retain transported metadata;
no native indices are copied into that unrelated table. The operation facade's
innermost type resolves by identity into the native link for existing comment
callers. New getters are narrow const Qt integration boundaries; no engine format,
stream parser, native archive algorithm, dependency or borrowed notice changes.

Public mislabeled ZIP/7z regression initially fails2Qt because error type is
VT_EMPTY. After restoration, source property collection exposes the rejected
handler. Original CFfpOpen returns only NonOpen diagnostics in ErrorMessage;
the initial opened-level warning expectation was corrected after source review,
with its failed log retained. A real module-unload crash after catalog replacement
was traced with LLDB to CFolderArchiveProperties: _resource released before its
native _rawProps interface. Member order now retains the module through every
native interface release. Tests require the catalog to survive replacement and
expire only after all bound owners release, and require original bytes unchanged.

Real Split/TAR and concatenated ZIP parser check levels, type/path, native property
info and parent-item size/info. Gzip cannot supply a seekable IInArchiveGetStream;
its invalid multi-level fixture result was retained and replaced with source Split.
Split with rejected inner TAR exposes final NonOpen metadata. It initially fails
the application opening because CLI autodetection retries the rejected inner level,
despite the original opener accepting the outer chain. The Qt adapter serializes
the already accepted handler names in original ParseOpenTypes order, terminating
before the rejected file. Original code has typed native inputs, not an existing
retained-link-to-CLI formatter; this narrow process boundary adds no detection rule.
Both native levels and catalog remain authoritative.

macOS Qt6.12 arm64 focused8/8 PASS1.35s; final new suite PASS0.89s, including
normal/compact reports for the error-type row. Four JSON reviewed: dialog540×360/
360×240, properties list518×307/338×187, enabled and fully visible. Full unsandboxed
format/all-target build/246CTest PASS105.49s. Fresh Linux/Windows validation
uses snapshot `20261005T223534Z-e1fa25e5`, SHA256
`766f4fa05080ec46d4333880436b7f9892df5b23636237da6b38c4c09431414d`,
with official runtime/SFX, original Windows reference and Highest session1.
Its14 focused consumers pass on Linux17.04s and native Windows51.61s, all0skip;
the new suite passes7Qt on each OS.11 immutable focused artifacts per guest have
matching guest/local SHA. Twelve normal/compact JSON across3OS were reviewed:
list518×307/338×187 on macOS,518×310/338×190 on Linux,518×306/338×186 on Windows,
all fully visible/enabled.10 production/test/CMake inputs equal frozen source bytes.
Fresh full Linux242/242 PASS193.85s,640guest/localSHA; Windows full251/252
FAIL922.64s,661guest/localSHA. Its only failed suite is archive_external_edit:
editor-no/viewer-yes do not observe the native update prompt in the existing
five-second wait,13Qtpass/2fail/0skip42.038s. The new native-property consumers
pass; this does not accept whole Windows validation. The only two whole-gate
Windows skips are POSIX directory-link/lstat fixtures. Codec cold failure does
not recur in this one run; stable timing is still unproven. Original failed
status/logs/source snapshot remain intact; privacy review scans1985Mac/Linux/
Windows logs/JSON with0 fixture payload violations. Temporary wrapper was
removed after both source transfers; original snapshot retains it.2095 local/
prior-snapshot logs/JSON scanned with0 fixture-password payload violations.
This accepts P3-05.5's native property/owner/partial-open component. Parent P3-05
remains open for other source-body/dependency scope;
rendered Windows parity is unverified.

## P3-05.4 — same/subfolder source callbacks (accepted callback scope, 2026-10-06)

Source repository https://github.com/ip7z/7zip.git, full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`: reviewed original
`CPP/7zip/UI/FileManager/App.cpp` CApp::OnSetSameFolder/OnSetSubFolder and
CPanelCallbackImp callers; `PanelKey.cpp` OnKeyDown VK_UP/LEFT/RIGHT branches;
Panel.h callback/folder/selection types; PanelFolderChange BindToPathAndRefresh,
SetNewFolder, LoadFullPath; FSFolder Init/BindToFolder/BindToParentFolder; Agent
folder binding and retained engine resource. The two App bodies replace the
callback stubs, preserving one-panel, missing-focus, non-folder, parent fallback,
failed/null bind, CloseOpenFolders and RefreshListCtrl control flow. Qt replaces
the source listView focused-item query with the existing selection adapter.
Original kParentIndex is retained locally, as in other adapted translation
units. The native Alt branches retain default list-arrow handling and coexist
with the existing Alt/Shift/keypad consumers. Licenses/notices remain unchanged.

Qt-only adaptations: released app owners refuse late callbacks; presentation
cancels a destination's old asynchronous generation before native rebinding
and renders that destination afterward. Each existing scanner/projection is
now per-panel; no new filesystem walk, archive logic, Qt notification filter
or dependency is introduced. Scoped projection retains the callback's panel
without moving source focus or canceling publication on a focus switch. Scan
guards follow the actual publishing panel mask, while modal Open/Operation
guards retain both panels. Disabling a Qt list can synchronously focus another
panel: the address in the busy panel receives focus and presentation policy
updates cannot retarget the remaining scan setup. Shutdown joins both workers.
Directly shared archive folders retain the native relative prefix; the Qt
display omits an empty archive separator, and their engine resource remains
owned after the source panel leaves the archive. No parent-stack copying or
independent archive navigation rule was introduced.

Public macOS regression initially fails three Alt scenarios, with the previous
55 Qt scenarios passing. Final focused folder-sync passes17Qt/0fail/0skip in
1.33s; direct shared consumers pass5/5 in8.78s. Ten current-build normal/compact
JSON reports are reviewed: each compact panel318px wide, address combo240×24,
table310×232, all relevant controls fully visible/enabled in ready states;
normal panels488px wide, combos410×24, tables480×452. Tests include Unicode/
spaces, both subfolder arrows, one-panel/no-focused-item/file no-ops, parent/
virtual root, failed native bind, destination replacement, close during scan,
and 3/512-item archive-provider lifetime after source departure. New independent
suite `panel_folder_sync` is registered for all consumers and native Windows
UI transport. macOS unsandboxed full245/245 PASS102.43s, including final
closed-owner and relative archive-address checks. First Linux/Windows snapshot
`20261005T213912Z-fbced9e7`, SHA256
`5ae3d5fc9bbb70e9760324e5b81202d0283bc94064cd07c75b5aa89b322e638d`,
retains failed wrapper and corrected diagnostic results: Linux focused9/9 passes,
then full build exhausts disk; Windows focused8/9, with two archive assertions
using unnormalized native separators. The fixture now normalizes those paths;
macOS focused1/1 passes in1.42s after rebuilding it. Completed own build outputs
were removed only after evidence/hash and inactive-process checks. Fresh snapshot
`20261005T215349Z-81d1be19`, SHA256
`928a99a80fd7695271b655b2a31bb73f663306c3bfa8ce1f90445cbe0ec66df8`,
uses official runtime/SFX, a built original 7zFM reference and the logged-on
Windows desktop (Highest, session1). Focused consumers pass9/9 on Linux18.50s
and Windows191.04s; folder-sync17Qt, keys55, lifecycle16, scanner14, selection18,
navigation10, flat8, large-list14 and network3 all have0fail/0skip. Each guest's
69 immutable focused files have matching guest/local SHA. Ten Linux and ten
Windows Alt-state JSON reports were reviewed, with both panels' addresses and
lists visible/enabled at normal/compact sizes. Windows4096-row benchmarks retain
exact publication count, ready9.869–11.178s and GUI gap157–200ms, within unchanged
15s/250ms limits. The18 production/test/CMake inputs match frozen snapshot bytes.
This accepts the callback/keyboard/publication component P3-05.4. Fresh full
Linux passes241/241 in189.39s, with636 guest/local SHA-verified artifacts;
its platform skips are Windows-only metadata/reparse/registry contracts.
The fresh full Windows gate fails250/251 under P3-03; parent P3-05 remains open for remaining
body/dependency scope. Rendered Windows parity for these newly exposed states
remains unverified.

## P3-05.3 — ClassDefs/native GUID dependency boundary (reviewed, 2026-10-05)

Source repository https://github.com/ip7z/7zip.git, revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Original FileManager/ClassDefs.cpp,
Common/MyInitGuid.h, FileManager/MyWindowsNew.h, Explorer/MyExplorerCommand.h,
their current ProgressDialog2/ContextMenu callers, DllExportsExplorer definition
order and CMake libraries were inspected. The full disposition is owned by
[omitted scope](omitted-scope-audit.md#classdefs-and-native-guid-dependencies-reviewed-on-2026-10-05).
Native headers are exact live dependencies, not blanket omissions. Portable GUIDs
have one ClassDefs owner; modern MSVC native GUIDs use SDK/uuid, shell DLL defines
only its own CLSID. Old SDK/MinGW/CE branch execution is not claimed. Docs-only
diff check; no manifest/source changes. Remaining UI bodies/resources and native
workflow evidence still keep parent P3-05 open.

## P3-05.2 — missing UI translation-unit dispositions (reviewed, 2026-10-05)

The [omitted-scope review](omitted-scope-audit.md) records the six missing paths
from the original109 FileManager/Agent/GUI/Common translation units, after full
body/header and makefile/caller inspection at the same pinned0766b733 revision.
It distinguishes empty/comment-only/PCH transport from active EnumFormatEtc
presentation and legacy ProgressDialog installer bodies. These active modules
are not called inactive; Qt MIME transport and original SFXWin/SFXCon bundle
selection provide explicit ownership boundaries. No code/body is copied or
modified in this documentation component. Filename coverage103/109 is not used
as functional acceptance. Remaining bodies/branches/resources/ClassDefs closure,
native transport proof and whole P3-05 remain open. git diff --check passes;
documentation-only scope requires no runtime or appearance check.

## P3-05.1 — Unix FM/GUI production consumers (accepted scope, 2026-10-05)

Source repository https://github.com/ip7z/7zip.git, full revision
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Original FM.cpp::WinMain2,
GUI.cpp::Main2/WinMain, complete ArchiveCommandLine.cpp::Parse1/Parse2,
their headers, callers and progress question policy were inspected. Unix main
previously bypassed source startup and never compiled the GUI dispatcher.
All platforms now compile the existing Qt FM/GUI entry owners. Windows keeps
GetCommandLineW and its tokenizer. Unix supplies already decoded argv values:
FM's new typed overload shares the original format/path/DoesFileExistNeedArc body
without trimming literal pathname whitespace or stripping quotes; GUI passes
the native vector to the original parser and unchanged command branches.

ArchiveCommandLine.cpp moves from exact third_party to its upstream-relative
src path. Parse1/Parse2, switches, list-file/censor/update/mapping control flow
are retained. Includes resolve through the existing portable/adapted headers.
Only its POSIX program-argv directory fallback is omitted: all consumers already
use the authoritative adapted Windows/DLL.cpp::GetModuleDirPrefix and Qt native
executable path. This avoids duplicate symbols and competing module paths.
Common/CommandLineParser.cpp and ListFileUtils.cpp remain unchanged dependencies,
now compiled on Unix too. Whole-file clang-format is recorded separately from
this functional omission/include adaptation. The reproducible manifest verifies
408 exact/196 adapted/3 local paths at the same revision. MyMessages.cpp retains
its single process question flag on all platforms; its native HWND bodies remain
Windows-only. ProgressDialog2 uses that existing GUI policy on Unix as well;
per-operation Qt overrides remain available.

Qt main preserves the association event receiver and independent openPath window
lifetime. Copied 7zG/7zFM Unix entries select the same production bodies; macOS
uses argv[0] for this dispatch because bundle executable metadata can resolve
applicationFilePath to the principal binary. No Finder automation/acceptance is
provided by these changes. The GUI source exception/exit policy and lazy engine
factory remain unchanged. Win32 security initialization stays Windows-only;
Unix error/information presentation uses QMessageBox with source messages.

Seven typed FM cases exercise real catalog/binding plus a separate Qt process
calling the shared production RunSevenzipFileManager body: quoted Unicode paths,
forced/wrong format, broken/encrypted Cancel, relative folders with literal outer
spaces and source-ignored tail. The helper observes public widgets and closes
them; it does not expose internals or prove native desktop integration. Unix
7zG subprocess cases perform Add/Test/Extract/excluded-handler/broken refusal,
official-engine verification and exact bytes, preserving source/archive/sentinel.
Initial GUI regression found omitted Unix progress -y policy; the RED log is
retained at build/review/p305-unix-gui-red-lasttest.log. macOS arm64 Qt6.11.2
focused2/2 PASS2.58s and full formatting/all-target gate243/243 PASS87.56s;
the startup wrapper's four consumers pass2.46s. The ordered full log is retained
at build/review/p305-main-full-lasttest.log. Root integration compiles each of
its142 translation units once, including the parser and FM/GUI entry owners;
ProgressDialog2 retains separate sync-only and presentation variants.

Frozen snapshot20261005T102806Z-31754f94, SHA
2923d12c3f932592bd00798a08985fdb780ab18c5240517ac79001696588736d,
contains this component and the separately accepted redraw policy. Linux's
original build stopped with ENOSPC before tests; that failed check.log remains.
After removing only verified closed task builds, the same snapshot/build retry
passes focused10/10 in14.78s and full239/239 in190.13s. Its626 artifacts are
guest/local SHA-verified. Unix GUI and FM cases have zero skips. Windows native
interactive focused10/10 passes49.63s; full246/248 fails1002.00s with
native_open_commands timeout and both disk large-list timing failures. Its647
original artifacts are guest/local SHA-verified. Those failures remain P3-03.3
work; no timeout, performance limit or required fixture is relaxed. The separate
modal-fixture correction is excluded from this source component's commit.

P3-05.1 accepts these production callers, parser dependency and observable
startup/dispatch consumers. It does not accept stable Windows full performance,
Finder/native event integration, packaged launch workflows or whole P3-05
body/resource/dependency closure. No sibling upstream files are changed.

## P3-04 — own-code notice preparation (active, 2026-10-05)

The user selected MIT for the project's own Qt/UI/integration code, superseding
their earlier GPL choice. Root LICENSE records that choice; inherited 7-Zip,
Qt and dependency notices retain their existing owners and licenses.
Both install routes include the root notice in a separate sevenzip-desktop
directory alongside the existing 7zip notices. No borrowed source or license
text was relicensed. Sequential macOS Qt6.12 format/all-target/full244/244
PASS83.17s. A fresh Release Qt6.12 runtime installation produced a signed
12,434,024-byte ZIP with exact root MIT text; package SHA256
`542ac18ed2d492f12c72ad4ad3559a74bbf365840f9415cbb3e3272fc9b95bb5`.
Its inside-out ad-hoc and deep/strict signature verification passed.
This is notice preparation, not whole P3-04 acceptance: three-platform packages,
relocated clean-environment archive workflows and missing-dependency scenarios
remain mandatory. No publish/signing identity or upstream change was made.

## P3-04.1 — Linux runtime archive and relocated consumers (2026-10-05)

Frozen source snapshot `20261005T181408Z-e5c5b06f`, based on local commit
`e31d722a697432e8c037ff59cc8176b4e4a1e457`, SHA256
`fad1c551704c1b9b2c0a28b2bd8fc1d08be2beb6feb5d4e7d8b3d06b23216394`,
builds Release with BUILD_TESTING=OFF using Ubuntu24.04.5 x64/GCC13.3/Qt6.12.0.
The existing pinned engine/module/SFX builders retain upstream
`https://github.com/ip7z/7zip.git` at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; no borrowed source changes.
CMake installation/Qt dependency deployment and scripts/package.py produce
the explicit runtime payload, enforce the source/build-file exclusion guard,
and include checksum-verified Qt/ICU and original 7-Zip/MIT notices.

`sevenzip-desktop-0.1.0-linux-x86_64.tar.gz` is34,051,702 bytes, SHA256
`d0b6609c59432bb64e87a577b3ce1aa257844846bdbf479e916b7599bce00248`;
the guest/local artifact copies match. The archive was extracted into a new
Unicode/spaced directory with a new HOME/settings file, PATH=/usr/bin:/bin,
and no SDK/Qt library/codec-module environment overrides. Installed 7z reports
its adjacent packaged7z.so. Installed7zG creates a ZIP, tests and extracts it;
installed7z lists it. Unicode/spaced filenames and binary payload match exactly.
Broken ZIP and a removed packaged7z.so both return the source fatal exit2.
The installed own-code notice matches root LICENSE byte-for-byte.

The first xcb attempt failed because this guest has no display:0; its console
log remains. The subsequent native xcb/Xvfb run passes all eight recorded checks
without an offscreen plugin. This proves relocation and ordinary system-library
compatibility on this Ubuntu runner, not deployment to another clean OS image,
interactive desktop UX, Windows packaging or whole P3-04/P3-06 acceptance.
The temporary build helper is removed after the frozen run; reproducible commands
are cmake Release configure/build followed by scripts/package.py with the three
matching private Qt-source/checksum/ICU-license inputs documented in CI.

The first Qt6.12 Windows full gate exposed platform separators in a Python
package fixture's expected path list. The fixture now compares as_posix(),
preserving exact expected payload paths. Local runtime_package passes1/1;
an independent guest copy passes all6 tests in0.046s without modifying the
original source snapshot. Original Windows full failure is retained; this
localized fixture check does not accept its whole gate.

## P3-04.2 — Windows Release runtime ZIP and relocated consumers (2026-10-05)

Frozen source snapshot `20261005T191436Z-27c52b66`, SHA256
`619deba1803cc77b74fe2ec6b87589f46b86bfde21b598f978a2d7d893c8a50b488d7`,
builds a separate `build/package-release` tree with Release/BUILD_TESTING=OFF,
Windows11 x64/MSVC19.44/Qt6.12.0. It reuses the original official engine/module
and SFX inputs at upstream `https://github.com/ip7z/7zip.git` commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. No borrowed body or package
implementation changed. CMake install and scripts/package.py follow the Release
workflow: runtime application/entrypoints, shell DLL, official7z.exe/7z.dll,
SFX, Qt/plugin closure, imported VC143 CRT DLLs and original notices. The CRT
installer and source/build/debug payload are excluded.

`sevenzip-desktop-0.1.0-windows-x86_64.zip` is28,224,845 bytes, SHA256
`22e50efb61cf3681de86684f9ad3f4f5150ad645f5534339f14077a0280d88f7`.
Thirteen original package/build/smoke artifacts have guest/local SHA256
verification. The local ZIP payload guard and byte-exact own-code MIT notice
comparison pass. Private Qt6.12 source/checksum/ICU notice inputs were separately
checksum-verified; VC143 notices accompany the imported app-local CRT closure.

An interactive session1 smoke extracts a disposable copy to a fresh Unicode/
spaced location, sets a new USERPROFILE/APPDATA/LOCALAPPDATA/settings file and
limits child PATH to Windows/System32, removing Qt/SDK/codec overrides. Actual
packaged7zG Add/Test/Extract succeeds with Unicode/spaced names and exact binary
bytes; packaged7z lists formats and the resulting ZIP, using its adjacent7z.dll.
Damaged ZIP and removal of that disposable7z.dll each return source fatal exit2;
all eight smoke scenarios pass. The original runtime ZIP is retained unchanged.
No shell registration, association, installer, publish or upstream write occurs.
This accepts bounded Release packaging/relocation on the existing runner, not
a new clean OS image, global native UX or final P3-04/P3-06 acceptance. macOS
relocated-package and final three-platform acceptance remain mandatory.

## P3-04.3 — relocated macOS runtime bundle consumers (2026-10-05)

The already audited Qt6.12 Release/BUILD_TESTING=OFF arm64 ZIP from own-code
notice preparation is tested by artifact identity:12,434,024 bytes, SHA256
`542ac18ed2d492f12c72ad4ad3559a74bbf365840f9415cbb3e3272fc9b95bb5`.
This is relocation of that retained artifact, not a rebuild of the current
notification-owner WIP. macOS27.0.1 build26A434; Qt6.12.0. Original source
engine/module/SFX provenance remains upstream
`https://github.com/ip7z/7zip.git` at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. No source or implementation changes.

ditto preserves the app bundle/symlinks in a fresh Unicode/spaced location.
Deep/strict ad-hoc signature verification succeeds before testing. Child
PATH=/usr/bin:/bin, new application settings and removed Qt/DYLD/codec/SDK
overrides exercise native Cocoa without an offscreen plugin. Actual packaged
7zG Add/Test/Extract and adjacent Resources/7zip/7z formats/list succeed;
Unicode/spaced filenames and binary bytes match exactly. Damaged ZIP and removal
of the disposable packaged7z.so each return source fatal exit2. The installed
MIT notice matches root LICENSE byte-for-byte; all eight smoke scenarios pass.
Original ZIP remains unchanged. Commands and original results are retained in
build/review/p304-macos-smoke.py and p304-macos-package-smoke.json/.log.
This accepts existing-host artifact relocation, not a new clean macOS image,
new-source final package builds, global native UX or final P3-04/P3-06 acceptance.

## P3-03.3.4 — callback property display (accepted component, 2026-10-05)

Source: https://github.com/ip7z/7zip.git, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Reviewed original FileManager/PanelItems.cpp::RefreshListCtrl callback insertion,
PanelListNotify.cpp::SetItemText property/raw/time formatting,
Panel.cpp::OnNotify notification suppression, FSFolder.cpp::GetProperty packed
size caching, and current CPanel/columns/row/refresh/statistics consumers.
The existing adapted CPanel::GetItemText remains the sole source formatter;
no borrowed bodies, manifest revisions or C/C++ boundaries changed.

Qt's previous row publication eagerly formatted every visible metadata cell.
ArchiveTableItem now requests source metadata on display/edit demand, using the
table's owning panel rather than the focused panel. Other roles keep their direct
path. Qt caches formatted display text until source statistics/time invalidation;
the native provider retains authoritative property values and sort comparison.
The table retains its displayed folder identity, checks item count before demand,
and obeys the original notification guard while a nested archive reopen can
release the old proxy. Window teardown disconnects callbacks and releases the
retained identity before CApp's member lifetime ends. Timestamp and statistics
consumers invalidate display text, including hidden cells, without eagerly
fetching properties. Name/path/selection roles and source sorting are preserved.
This is a C++17 Qt presentation adapter to Win32 callback text, not a replacement
property algorithm. Root-only adaptations remain outside the borrowed manifest.

Public panel_columns provider-demand/cache/notify regression is RED with the
previous static projection; folder_size_view hidden subfile-count regression is
RED in normal/compact before hidden-cell invalidation. Both are GREEN after their
fixes. Final focused3/3 PASS5.67s; sequential unsandboxed macOS arm64 Qt6.12.0
format/all-target/full244/244 PASS89.92s. Current disk/archive normal/compact and
folder-statistics JSONs show the table enabled, visible and fully visible.
4096-row Mac projections take235/144ms disk and122/85ms archive, maximum GUI
gap55/53/23/22ms, first heartbeat32 rows. These are this OS's results only.

Original snapshot `20261005T172432Z-e157bc55`, SHA
`24e6d1133998d931cafa60209e0cc4eab05d72f76878810cc6a8960549f644fe`,
preserves Linux helper exit126 and its same-SHA Bash retry's Qt6.4 deployment
configuration failure. Windows Qt6.8.3 Highest interactive focused6/7 PASS133.41s
fails only large-list disk-compact's unchanged15s limit (13 Qt passes/1failure);
45 original guest/local artifacts are SHA256-verified. Neither failure is
replaced with a passing gate. Final snapshot `20261005T173918Z-d21a05dd`, SHA
`f12e11d4d4b1a670327727d3da2b6e079eef3324c3c25e0d04f56446b382654a`,
selects the already installed Linux Qt6.12.0 kit: focused8/8 PASS12.53s,
format/all-target/full240/240 PASS183.40s; 626 original guest/local artifacts
are SHA256-verified. Windows Qt6.12 and whole-platform performance/network acceptance remain
open; no deadline, required skip or source property policy has been relaxed.

Windows Qt6.12.0 installation and live probe now pass. Snapshot
`20261005T175900Z-2a0f6f8f`, SHA
`9577adf3c7538619b7907e5145549cc8ffa18521108e747c79c1bb1515456e78`,
uses that kit in CMake and passes7/8 focused suites131.60s. The callback/hidden
statistics consumers pass; large_list_window has13 Qt passes/1failure/0skips:
disk-normal18,752ms exceeds the unchanged15s deadline, maximum GUI gap424ms.
Disk-compact9,344ms, archive-normal9,850ms and archive-compact9,910ms are retained.
47 original guest/local artifacts are SHA256-verified. The focused failure
prevented this snapshot's full gate; this is not Windows whole-gate acceptance.

Final original Windows snapshot `20261005T180727Z-3ebcd2be`, SHA256
`b07e7a78d1f22fecf4981e1c3303854798b8b4eee7895a6bd5dbf14c7a1e9476`,
completes Qt6.12.0 Highest interactive formatting/all-target/full247/249 in846.42s.
Only runtime_package's Python path separator fixture and external_editors_ui's
native minimum-height fixture fail; all source callback/hidden-statistics
consumers and full large_list_window pass.646 original artifacts are guest/local
SHA256-verified. The earlier focused timing failure remains recorded. This
accepts callback text, source notification/lifetime guard and invalidation on
three OS; the failed whole gate, required native network skip and stable Windows
timing still belong to parent P3-03. A fresh correction snapshot is required.

## P3-03.4 — one address Enter transition (accepted component, 2026-10-05)

Source: `https://github.com/ip7z/7zip.git`, full revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/PanelFolderChange.cpp::{OnNotifyComboBoxEnter,
OnNotifyComboBoxEndEdit,OnCommand}`: CBENF_RETURN navigates once; CBN_SELENDOK
owns selection from the address list. Reviewed the original bodies and current
PanelKey, address signals, history publication and filesystem scan consumers.
The existing C++17 Qt edit-key adapter consumes Return/Enter before QComboBox
can additionally activate the newly published history row. Direct returnPressed
and explicit history activation retain their existing consumers. No archive,
folder or path algorithm is replaced; the borrowed baseline revision is unchanged.

Public ui_navigation row-publication regression is RED for both keys: one key
publishes two rows for a one-file folder. It is GREEN for a new address and an
existing history address after key consumption. Local focused navigation/key/
large-list consumers pass3/3 in8.42s. An initial full gate after removing the
direct signal connection failed six existing signal consumers; its original log
is retained. Restoring that connection gives sequential unsandboxed macOS arm64
Qt6.12.0 formatting/all-target/full244/244 PASS91.04s. Windows performance
acceptance remains pending. Fresh Linux
snapshot `20261005T182031Z-beeaa1cf`, SHA256
`2cba1666a42e6c90b3b203ee8243ceb217dd60f7fb704561be3ba294bc7a95c1`,
passes10/10 focused consumers in21.39s and formatting/all-target/full240/240
in182.39s;626 original guest/local artifacts are SHA256-verified.
Fresh interactive Windows snapshot `20261005T183845Z-a2c659ac`, SHA256
`ea78c97f31fec0872c1bd01f5011c7fb0a9156234409ae2d073537e75dd1836c`,
passes navigation/key consumers and all-target/full249/249 in935.02s;
647 original guest/local artifacts are SHA256-verified. The earlier focused
set11/12 fails198.06s on the unchanged large-list deadline, so the combined
runner result remains failed. This accepts the bounded address-key component
on three platforms; it does not accept parent P3-03 or stable Windows timing.

The original Windows Qt6.12 snapshot3ebcd2be focused7/8 fails131.84s, with only
disk-compact exceeding15s. Diagnostic ready/report times are18698/41ms for that
row,9810/43ms for disk-normal and9617/44ms,9760/42ms for archive rows. Thus JSON
writing does not explain the timing failure. These measurements do not establish
that the Enter fix resolves Windows timing. All thresholds remain unchanged.

## P3-03.6 — Editor fixture native minimum (accepted, 2026-10-05)

Reference: `CPP/7zip/UI/FileManager/EditPage2.rc`, `EditPage.cpp`,
`OptionsDialog.cpp` and `CPP/Windows/Control/PropertyPage.cpp` at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
https://github.com/ip7z/7zip.git. Six original property pages determine the
sheet minimum. No production layout/body or copied dependency changes.
The original Windows whole gate rejected a compact test expecting exactly
520×380 while Qt's native sheet required 520×390. The test now compares the
requested size expanded by `minimumSize`, retaining exact width and every
field/Browse/footer visibility and separation assertion.

macOS focused Editor/package2/2 PASS and the final all-target unsandboxed
`scripts/check.sh`244/244 PASS82.35s. Fresh interactive Windows snapshot
`20261005T183845Z-a2c659ac`, source6dab845 plus the fixture, archive SHA256
`ea78c97f31fec0872c1bd01f5011c7fb0a9156234409ae2d073537e75dd1836c`,
passes `external_editors_ui` (10 Qt cases,0fail,0skip). Its current-build JSON
reports640×390/520×390 with unclipped controls; macOS reports640×390/520×380.
Saved focused log and reports have guest/local SHA256 verification. The
12-test Windows focused set has11passes and a large-list timing failure198.06s;
the snapshot's whole gate subsequently passes249/249 in935.02s with647
guest/local SHA256-verified artifacts. Combined runner status remains failed
because of its original focused failure. The original failure remains
historical evidence. This accepts the Editor fixture only: parent P3-03 and
rendered Windows pixel parity remain open.

## P3-03.5 — native Windows network fixture (accepted component, 2026-10-05)

Reviewed upstream `CPP/7zip/UI/FileManager/NetFolder.cpp/.h`:
`CNetFolder::Init`, `LoadItems`, `GetProperty`, `BindToFolder`,
`BindToParentFolder`; dependencies are `CPP/Windows/Net.cpp/.h` resource
information/enumeration and `FileManager/FSFolder.cpp::CFSFolder::Init`.
Baseline is `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git. Existing Windows bodies remain authoritative;
no production body, dependency or omitted branch is changed by this fixture.

CMake now selects `native_network_folder_tests.cpp` on Windows and the existing
mount-provider fixture on Unix. The native fixture creates a uniquely named
local SMB resource containing a Unicode/spaces filename and binary bytes;
its share descriptor grants access only to the current user SID. It exercises
original WNet server enumeration, share properties and binding to CFSFolder,
then reads the actual UNC bytes. Deleting only the owned share tests stale
binding refusal and refreshed enumeration, while the local bytes remain intact.
Scope guards clean the share after failed assertions. It uses no user archive,
external server, credential prompt or network/security-policy change.

Upstream Net.cpp offers client resource/connection wrappers, without local
share creation/deletion or a current-user SID extractor. SecurityUtils.h's
CAccessToken exposes privilege adjustment but keeps its handle private. These
test-specific setup/cleanup and SID/SDDL calls therefore use the Windows SDK;
they add no second application network implementation.

This replaces a required Windows QSKIP, not an optional applicability branch.
It requires running LanmanServer/LanmanWorkstation and a token allowed to create
the isolated local share. Read-only runner inspection confirmed both services
running. The fixture's single scenario includes creation, observation and
cleanup together so every failure keeps the ownership guard in scope.
macOS focused `network_folder` 1/1 PASS verifies the retained Unix fixture;
final unsandboxed formatting/all-target/full244/244 PASS102.07s.
Fresh interactive Windows snapshot `20261005T191436Z-27c52b66`, SHA256
`619deba1803cc77b74fe2ec6b87589f46b86bfde21b598f978a2d7d893c8a214`,
source8394704 plus this fixture, passes network1/1 in1.10s (3 Qt cases,
0fail,0skip). The original focused log is retained with guest/local SHA256
verification, alongside8 final reference artifacts and5 extra artifacts.
The first snapshot2f492305 failed compilation before share creation because
NetShareDel's SDK signature needs mutable LPWSTR; wstring::data fixes both
calls. Original logs remain. The successful network snapshot's combined
runner status remains failed because the later reference target exhausted disk.
Its independent same-SHA capacity retry passes native menu1/1 in10.60s
(3 Qt cases,0fail,0skip) after restoring32.7GB free space. Only generated
build/dev outputs of two completed task snapshots were removed, after
reverifying1293 retained evidence files; source/runtime/log/evidence remain.
This accepts the bounded
native network scenario, not a new whole Windows gate or remote credentials/UI.
Parent P3-03 stays open for stable performance and remaining platform scopes.

## P3-03.3 — Windows full-gate follow-up (active, 2026-10-05)

Latest Qt6.12 Windows snapshot a2c659ac passes its whole249/249 gate935.02s,
with647 guest/local SHA256-verified artifacts. Its earlier focused11/12 set
fails198.06s on disk-normal: ready18651ms/report43ms/max GUI gap415ms at0rows.
The whole gate's four ready times are9457/9247ms (disk normal/compact) and
9612/9643ms (archive), with maximum gaps194/197/141/148ms; LargeList has14Qt
cases,0fail,0skip. Thus a whole passing run does not establish cold-run
stability. Its remaining three Qt skips are two POSIX-only fixtures and the
required Windows network branch. Native network replacement is P3-03.5.
The benchmark now also records first/last public row publication times and the
number of inserted rows, through the existing model signal; no internal test
access or deadline changes. Same frozen Windows27c52b66 diagnostic run fails
1/1 in99.69s: disk-normal publishes8192 rows for4096 items, first41ms,
last18526ms, ready18718ms, maximum gap396ms at0rows. The other three cases
publish4096 rows each, ready9479..10114ms, maximum gaps150..199ms. All17
final artifacts are guest/local SHA256-verified. Separate diagnostic binaries
reuse its objects with two private Qt presentation objects carrying logs;
they preserve the frozen source and original binaries. A fresh isolated
TEMP/work-directory reproduction identifies native OnTimer→reload after
the first publication; no portable-signature mismatch is logged. A process
with TEMP outside the startup folder passes its single case in9391ms.
This is diagnostic evidence, not a performance fix or acceptance.
Further private source probes identify `CFSFolder::WasChanged` returning1
while `_portableFolderChanged` is false. The source folder path agrees with
the displayed folder. A probe waiting1100ms after publication repeats the
native reload and ends with redraw still disabled during the next projection;
this deliberate diagnostic wait does not change the committed test. Six
private probes retain58 guest/local SHA256-verified artifacts, including
copied source, compile/link commands and original logs. Logging perturbs timer
phase: an intervening probe passes before the timer fires, so it is not proof
of stability.
Exploratory address/history/removal counters were removed; the permanent
change consists only of first/last/publication-count metrics. Its exact body
already ran in frozen27c52b66 and macOS full244/244 PASS102.07s. A later
exploratory version passes macOS full244/244 PASS103.95s; after removing the
extra counters, unsandboxed focused1/1 PASS3.64s. The intermediate observer
lifetime SIGBUS was fixed before removal; sandboxed focused timing failures
are retained and do not replace the unsandboxed results. No production
performance fix or whole Windows acceptance is claimed.

P3-03.7 now isolates the notification-owner boundary with a public scanner
regression. Fresh Windows snapshot `20261005T202004Z-9f044ceb`, SHA256
`c2088a3a96388eb418d9b0d43879904d49b5bb3a1f4483aee788bf592a8a8f40`,
fails1/1 in4.74s (13Qtpass/1fail/0skip): immediately after worker exit, the
immutable published folder reports WasChanged1 instead of0. Five original
artifacts are guest/local SHA256-verified. The test also requires subsequent
real Windows writes to remain observable. Original source at
`https://github.com/ip7z/7zip.git`, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, was reviewed:
FileManager/FSFolder.cpp Init/WasChanged/LoadItems/Clone and native CPanel
BindToPath callers. Clone creates an empty owner and cannot retain the loaded
item cache. Instead the existing Qt loader's CreateFilesystemFolder→Init call
moves to its caller-thread construction; original LoadItems/flat walk and
callback cancellation remain in run(). Early cancellation releases the prepared
folder. No borrowed body, native notification filter or filesystem algorithm
changes. This restores the native binding lifetime while retaining Qt worker
enumeration. macOS affected5/5 PASS5.53s and unsandboxed full244/244
PASS99.65s; final scanner1/1 PASS1.42s. Same-source snapshot
`20261005T202546Z-21568cde`, SHA256
`c234c4d71e53cc30dd24314cbea584eb6aecfb2bf7d148182100337358438a23`,
passes Linux focused6/6 in9.36s and full240/240 in189.83s (626 artifacts
guest/local SHA256-verified). Windows focused/full large-list tests both
pass14Qt/0fail/0skip with the original15s readiness and250ms gap limits.
The four full-run cases publish4096 rows each, readiness9107..9490ms and
maximum gaps147..228ms. Windows full248/250 fails in1010.18s (649 artifacts
guest/local SHA256-verified): the new real-write test consumed its notification
again when QTRY_VERIFY reevaluated the successful condition, and unchanged
external-codec discovery exceeded its default5s wait. These failures remain
in the original evidence; they are not rewritten as a passing full gate.

The final regression retains the observed native signal across QTRY_VERIFY
reevaluations. A separate Windows executable uses the frozen production
objects and unchanged generated moc, substituting only that corrected test
object:14Qt/0fail/0skip PASS3971ms, including immutable-folder and subsequent
real-write checks. Seven sources/commands/log/session artifacts are guest/local
SHA256-verified under `build/review/windows-scanner-latched`, with origin SHA
recorded separately. The unchanged external-codec test passes a separate retry
1/1 in4.84s; this does not establish cold-start stability. P3-03.7 is accepted
as a bounded notification-lifetime fix; parent P3-03 stable performance and
whole-platform acceptance remain open. No timeout or coverage was weakened.

The P3-05.4 snapshot81d1be19 subsequently passes the same affected Windows
scanner/key/large-list/network consumers9/9 in191.04s, all0skip. Its production
inputs equal committed42aacc9; macOS full245/245 and Linux full241/241189.39s
pass (636 Linux artifacts guest/local SHA-verified). Native Windows focused
large-list publishes exactly4096 rows in each case, ready9869..11178ms and
maximum gaps157..200ms under unchanged limits. Full Windows250/251 fails
in1031.92s: unchanged external_codecs incompatibleModuleIsSkippedWithSourceError
misses its5s signal wait (Qt reports5750ms would suffice).659 full artifacts have
matching guest/local SHA. No deadline/coverage change or whole-platform acceptance
is inferred. Historical failures above remain intact; native/source performance
consumers pass again.

Current-kit native System-menu reference also passes on snapshot27c52b66 after
a separate capacity retry on the same source SHA:1/1 in10.60s,3Qt/0fail/0skip.
The original combined job's disk-full link failure and the first retry's cmd
path error remain recorded; their statuses are not overwritten. The pinned
original7zFM executable SHA256 is
`e0a75075331dcec383f1e0af567fbba8053b0b1e26edf2815b255c8a50b488d7`.
Its source makefile/FM.cpp and private registry-entry adapter provenance are
saved in reference.json. Runtime native/raw/Qt21-row menu state comparisons,
96DPI Segoe UI font data and both affected PNGs are reviewed; Qt style spacing
remains the documented adaptation. This does not accept other rendered dialogs
or the new whole250-test gate. [Reference procedure](windows-ui-reference.md#pinned-runtime-system-menu-reference).

P3-03.3.2 accepts only the native_open_commands modal fixture correction. Frozen
31754f94 full gate timed out because its one-shot observer could precede the
visible active QMessageBox. The fixture now polls the public active modal widget
with a stack-owned timer and stops after the original OpenSelectedItems returns;
the original twenty-item refusal and zero external side-effect assertions remain.
No production source, deadline, performance threshold or required skip changes.
macOS focused1/1 PASS0.51s and full243/243 PASS99.87s; full log retained at
build/review/p303-modal-main-full-lasttest.log. Fresh snapshot7a063225, SHA
fb5cd9dfe4de881b18491ad370f5f9e94453c9fc8b2b4635cbe1046ad88190a3,
builds and runs native_open_commands plus filesystem_selection_runtime:
Linux2/2 PASS1.99s and Windows native interactive2/2 PASS21.18s. Each platform
has23 Qt passes, zero failures/skips; four original artifacts per guest are
guest/local SHA-verified. A passing selection diagnostic does not erase its
older full crash. The current Windows31754f94 full246/248 failure1002.00s and
both large disk timing failures remain recorded in P3-05.1. Stable full gate and
native network coverage still keep this parent open.

The original b04b3b53 full/retry large-list failures remain as recorded in P3-02;
they are not passing platform gates. Original source PanelListNotify.cpp property
display/GetProperty/ConvertPropertyToShortString2 and FSFolder.cpp::GetProperty
at0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17 were inspected with their current
CPanel::GetItemText/sourceColumnText and column visibility callers.
mainwindow_filelist.cpp previously read QFileInfo::lastModified for every disk
row, then replaced the visible timestamp with the source cached CDirItem MTime.
It now leaves that initial hidden cell empty, like the packed-size cell; the
existing source property projection fills visible dates and the existing header
handler fills newly shown columns. No borrowed body, timestamp/sort/format policy,
baseline, UI text/layout or timeout is changed. This removes redundant native
metadata work. Alone it does not fix the timing:625d0cdf Windows focused3/4
passes130.16s, large-list13 Qt passes/1failure118945ms; disk-normal26933ms/
GUIgap465ms, disk-compact11970/204ms. Its23 original artifacts are SHA-verified.
macOS arm64 Qt6.11.2 focused projection/columns/scanner4/4 PASS5.54s and full
unsandboxed formatting/all-target242/242 PASS92.51s. Fresh integrated snapshot
20261005T093314Z-625d0cdf, revision12830a8 plus this narrow change, SHA
9f418a71d61f02db241b74e0d68be5f2a6ab9217f6e7e052fa8989e4b01acf24 runs
focused consumers followed by the full gate in each same build: Linux full
238/238 PASS185.27s,626 SHA-verified artifacts; Windows stops after focused failure.

Source PanelItems.cpp::RefreshListCtrl disables redraw before filling and restores
it after sort/selection. The current Qt policy already did this across archive
batches, but excluded scanningDisk. A deterministic public updatesEnabled
regression fails both disk normal/compact rows on macOS before the fix (12 Qt
passes/2failures3.13s), independently of Windows speed. refreshActions now applies
the same flag to disk/archive projections; the event loop and status/progress
remain active, and completion/navigation restores updates. No timeout changes.
Current Mac projection/columns/scanner/two-panel/navigation6/6 PASS6.06s.
Snapshot20261005T094614Z-34ba55ef now runs6 focused consumers then whole gates.
Current full Mac242/242 PASS94.55s and Linux238/238 PASS193.69s; Linux626 artifacts
guest/local SHA-verified. Windows focused6/6 PASS118.82s, large-list14 Qt passes
zero failures/skips102055ms, reaches the following full formatting/all-target
gate. Its focused log and8 normal/compact JSONs are separately SHA-verified
before full results can replace them. Windows full subsequently fails246/248
in852.18s: filesystem_selection_runtime SEGFAULT in sourcePathsAcrossViewsAndConsumers
and disk-normal19505ms/GUIgap423ms at0rows. Its647 original artifacts are guest/local
SHA-verified; focused evidence remains separate. An unchanged same-build native
selection diagnostic passes18 Qt cases/0failures/0skips11.94s, without replacing
the original crash. Stable Windows full, required native network coverage and
parent acceptance remain pending; old failures are not replaced.

P3-03.3.1 accepts only the integrated source redraw/cached-date policy and its
completion/navigation regression: macOS full242/24294.55s and Linux238/238193.69s
pass; all6 Windows focused consumers pass. The Windows full results above remain
failed and do not establish performance or parent acceptance. No timing threshold
or required fixture is relaxed. This narrow component is committed separately
from the independently tested Unix entrypoint work.

## P3-03.2 — local CI configuration (accepted, 2026-10-05)

Historical configuration evidence below is superseded for job/tool/artifact scope
by [Qt 6.12 build/runtime packaging](#qt-612-buildruntime-packaging-adaptation-2026-10-05)
and [current CI](ci.md); the recorded validation remains historical.

The complete `.github/workflows/check.yml` uses existing project runtime/SFX,
baseline, formatting/build/CTest scripts without duplicating source algorithms.
Official upstream https://github.com/ip7z/7zip.git is SHA-pinned at
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17; builders use private archives, leaving
the source checkout unchanged. Qt6.8.3 and Unix primary tools are pinned, action
refs use verified complete commits. `docs/ci.md` owns jobs/tool versions and
limits. No borrowed module/body, dependency provenance or UI changes.

Ubuntu24.04/macOS15 hosted jobs run the full gate; the dedicated Windows job is
manual-only and requires the authorized interactive/elevated native guest. It
does not execute PR code automatically on that guest or provision an agent.
Testing logs, widget JSON, compile database/cache, runtime/SFX provenance and
task-owned failure buffers are retained; pipe failure through tee is preserved.
Actionlint, embedded shellcheck and git diff --check pass locally. Current actual
three-platform project gate evidence remains in P3-02, including the failed
Windows large-list full/retry, rather than being described as a GitHub result.
No workflow publication, hosting, runner registration or GitHub execution was
performed; this accepts local configuration only, not P3-03 parent or final UX.

## P3-02 — regression/security traceability (accepted, 2026-10-05)

The new docs/regression-matrix.md maps all41 accepted functional P0/P1/P2
packages to source owners, integration boundaries and registered CTest consumers,
plus concrete security/Cancel/lifetime slots. Its147 consumer names were checked
against current registration; four Windows-only consumers are platform-specific.
The new archive_workflow public fixture reuses ArchiveBackend and the existing
FileTransferOperation/source FSFolderCopy, with ZIP/7z disposable Unicode/spaced
files. One backend lifetime covers create/list/select/extract/edit/rejected unsafe
replace/update/relist/Test/full extract and canceled/successful Move. It verifies
original disk inputs, retained archive members, exact edited bytes, unchanged
archive across filesystem transfer and absence of private stage residue.
It uses actual source listing names after normalized fixture lookup: macOS native
decomposed Unicode is not an archive-index alias. compression_encryption now
requires both shown and masked JSON report dumps to omit both password inputs.
No production algorithm, borrowed body, resource, UI workflow or baseline changes.
The new target consumes existing authoritative shared backend/transfer objects.
macOS arm64/Qt6.11.2 focused archive_workflow/compression_encryption2/2 PASS1.35s.
After preparing the pinned26.03 dynamic runtime and standalone7zz, the full
unsandboxed formatting/all-target gate passes242/242100.96s; archive_workflow
has4 Qt passes, compression_encryption11 and external_codecs13, all zero failures
or skips. An earlier242/242102.42s gate skipped external_codecs and is retained
as incomplete fixture evidence, not the accepted runtime result.
Frozen snapshot20261005T085136Z-b04b3b53, revisiondd3fded plus this test scope,
SHA225ff011fe0e5ede7f0084d789d5c5be494a99f030a045da65730536f3ed6438:
Linux x64 full formatting/all-target238/238 PASS193.91s,626 guest/local artifacts
SHA-verified. macOS/Linux each have15 Windows-only Qt skips: WIM reparse2,
NTFS extraction2, registry revision1 and metadata controls/storage10; required
Unix native module/engine fixtures run. Ten fixture markers were scanned in
1919 current three-platform logs/JSON artifacts without payload hits;
wrong-password matches are only QtTest row labels. Windows x64/Qt6.8.3 Highest
interactive session1 full formatting/all-target gate completes247/2481066.12s;
only large_list_window disk-normal exceeds the unchanged15s limit. Its original
647 artifacts are guest/local SHA-verified before retry. Required Windows
archive_workflow4, compression_encryption11, external_codecs13, source_link_extract11,
source_extract_advanced15, source_metadata_storage12, association_services8,
file_manager_startup11 and native_gui_workflow12 Qt passes have zero failures/skips.
The three Windows skips are POSIX link cleanup/stat2 and the still-unverified
native network-folder fixture1; the latter remains mandatory P3-03/P3-05.
Root production units remain Mac140/Win140/Linux138, each compiled once as
C++17/Objective-C++17; C entries/distinct remain46/44,47/45,46/44 with no forced
C++ language. Baseline409 exact/195 adapted/3 local verified. The unchanged
large-list retry also fails:13 Qt passes/1 failure,137221ms, disk-normal refresh
needs19150ms versus15000ms. Its647 separate guest/local SHA-verified artifacts
retain the original full result; no threshold or production code changed.
This accepts the P3-02 traceability, security/privacy and new transition coverage
with these recorded limits. It does not accept stable full Windows performance,
native network/Finder, source omission closure, installed packages or final UX.
The repeated existing large-list failure has a concrete P3-03 owner; neither
the full247/248 result nor its failed retry is reported as a passing gate.

## Windows Console opening error labels (accepted, 2026-10-05)

Inspected original CPP/7zip/UI/Console/List.cpp error printing and retained
OpenArchive.cpp opening-result consumers at
https://github.com/ip7z/7zip.git, commit0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
The file-only ArchiveBackend transport compares the original Console error label
to distinguish S_FALSE (unrecognized/encrypted/type-specific opening refusal)
from E_FAIL. Windows prints native separators while Qt callers can supply `/`;
the mismatch incorrectly returned E_FAIL and broke native opening/probe consumers.
archivebackend_listing.cpp now converts only the expected Windows physical path
to native separators before exact comparison. Existing macOS canonical comparison,
source/archive entry names, I/O failures and other error branches are preserved.
No borrowed body, dependency or baseline changes; no new opening algorithm.

Four public process-fixture rows cover ordinary-file, wrong-password, explicit-
type refusal and I/O failure with Unicode/spaces and unchanged input bytes.
The previous Windows full96d637fc reproduces the real opening S_FALSE failures;
current backend76 cases and ten direct consumers pass in f5d4ebde42.52s without
skips. macOS focused opening/backend/external/lifetime4/4 and full238/238103.48s,
Linux current full234/234194.36s pass. Full current Windows gate remains separately
owned; this accepts the narrow transport correction, not whole-platform acceptance.

## P2-11 — own glue registry (accepted, 2026-10-05)

The initial review started from bcb86e7 and the248 root src C++/header/Objective-C++ files,
not borrowed-module line counts. That initial macOS compile database contained133 unique
local translation units, all C++17;45 upstream C entries represent43 distinct
paths compiled as C, without C++ language flags. The two native Apple bridges
use Objective-C++17. CMake retains LANGUAGES C CXX and Apple-only OBJCXX. Windows/
Linux closure requires their actual builds, not inference from the Mac graph.
Original source reference throughout this registry is https://github.com/ip7z/7zip.git
at0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Existing per-module provenance and
accepted feature evidence remain in their named sections; this registry tracks
new review/remediation, not another feature implementation.

### Functional/component review register

| Row and files | Source owner, consumers and finding | Required change / verification | Review state |
| --- | --- | --- | --- |
| P: `mainwindow.cpp`, `mainwindow_panel.cpp`, `mainwindow_filelist.cpp`, `mainwindow_columns.cpp`, `mainwindow_refresh.cpp`, `mainwindow_archivecommands.cpp` | Qt panel construction mixed column persistence/context and reload/refresh; constructor held a full reload projection callback; addSources wrapper had no caller after source command migration | Preserve original CPanel owners and all Qt projection bodies; separate construction, columns and refresh; remove unused wrapper; actual selection/navigation/refresh/close/large-list consumers and sole object ownership | Accepted P2-11.12 |
| H: `qtlistview.{h,cpp}`, `qtfilelisthover.{h,cpp}` | ListViewDialog.cpp OnInit's TRACKSELECT style; CListView dialogs, MainWindow table/icons and TempBrowse. Two own400ms timer/state implementations | One existing hover adapter; public enable/disable/leave/modifiers/removal/close/wheel/hide, all dialog/main/temp consumers and unchanged normal/compact geometry | Resolved P2-11.1 |
| C: `archivebackend.h`, `archivebackend_mutations.cpp`, `createarchivedialog_source.cpp`, compression controls and consumers | CompressDialog.h NCompressDialog::CInfo already inherited, but five lower-case fields separately owned Level/Method/NumThreads/Dict64/Order; two conversions | Source DTO is authoritative; native autos, validation, format/property/settings/error semantics and actual compression/Add/direct backend/UI verified on three OS | Resolved P2-11.2 |
| U: `qtarchivesourceupdate.cpp`, `qtarchiveproperties.cpp`, `archivebackend_codecs.cpp` | AgentOut.cpp CommonUpdate/DoOperation and original IOutArchive::UpdateItems callbacks replace redundant native-writer fallback plans | Native writer/index/path/ADS proof, source properties lifecycle, prefix publication, errors/Cancel and actual commands verified on three OS; concrete absent-writer transport fallback retained | Resolved P2-11.5 |
| V: `qtarchivebatchopen.cpp`, `archivenativeopen.*`, `qtarchivefileresource.*` | OpenArchive.cpp CArchiveLink::Open2 already records actual callback-used VolumePaths/VolumesSize; own .001 enumeration recreated names | Retained source-link metadata now owns accounting and volume/readiness decisions; Split outer streams and absent-native fallback preserved. Batch/volume/SFX/extract checked on three OS | Resolved P2-11.3 |
| W: `qtarchivehashoperation.*`, `qtarchivecreationlifecycle.*`, `qtarchivevolumeoutput.*`, `qtarchivecopypreparation.*`, `qtworkerprogress.*` | Original ProgressDialog2.cpp CProgressSync owns worker flags/checkpoints through a Core-only target; optional source event preserves single-worker immediate wake | Eight public checkpoint rows and direct consumers pass on all three OS; native dialog polling, atomic cancel/publication and joined lifetime retained | Resolved P2-11.6 |
| F: `filetransferoperation.*`, `qtfiletransfertransaction.*`, `qtfiletransfercopycallback.*`, window file operations | FSFolderCopy.cpp owns copy/move/checkpoints; Qt transaction merge/rollback/remove-source combines orchestration and filesystem actions | Original recursion/deletion bodies reviewed; integrated transaction extraction preserves existing policies and sole worker state. Direct/window consumers and object ownership verified on three OS | Resolved P2-11.7 |
| B: `CMakeLists.txt`, `cmake/SharedPortObjects.cmake`, app/backend/dialog targets | Same authoritative production sources were recompiled per test; late source/link cloning and test TU macros require separate ownership | Twelve shared production groups plus Windows GUI entry; definitions, resources, ABI/platform links, late consumers and sole moc/source ownership verified on all three OS | Resolved P2-11.4 |
| R: `mainwindow_archive_transfer.cpp`, `archivebackend_transfer.cpp`, `qtarchivetransferbackup.h` | Original App.cpp OnCopy owns staged panel transfer; Compress/CopyCoder.cpp CopyStream owns stream copying. Widget previously performed archive backup and a manual byte-restoration loop | Backup/publication moved behind ArchiveBackend; original stream copying, real two-panel Move success/Cancel/rollback and publication refusal verified on three OS | Resolved P2-11.8 |
| S: `createarchivedialog_preferences.cpp`, `createarchivedialog_resources.cpp`, source `CompressDialog.cpp/.h` | Original FindRegistryFormat_Always/SaveOptionsInMem and OnOK memory refusal had functional bodies in widget glue | Source module owns profile creation, spec persistence/big-dictionary marker and memory refusal; Qt reads typed controls and presents source results. Previous-format/Auto/constructor/Cancel and direct compression/SFX consumers verified on three OS | Resolved P2-11.9 |
| J: `archivebackend.cpp`, `archivebackend_completion.cpp`, `archivebackend_listing.cpp`, `archivebackend_mutations.cpp` | QProcess completion mixed stage dispatch, archive publication/rollback and CLI listing projection. Original Update.cpp publication and FileDir.cpp CTempFile::MoveTo lack the existing restoration policy; Console/List.cpp has no inverse parser | One backend state owner; separated completion, mutation publication and listing projection preserve success/failure/Cancel and stage order; direct consumers and sole object ownership verified | Resolved P2-11.10 |
| K: `archivebackend_compression.cpp`, `archivebackend_creation.cpp`, `archivebackend_mutations.cpp` | Request-to-source-properties validation, CLI creation planning and existing-archive mutation shared one file; memory refusal duplicated original CompressDialog::OnOK | One source-owned memory refusal; separate immutable compression request projection, creation lifecycle and mutation publication; direct success/refusal/Cancel consumers and object ownership verified on three OS | Resolved P2-11.11 |

### P2-11.10 — backend process completion boundary (accepted, 2026-10-05)

P2-11.10 moves the existing CLI completion bodies,
without introducing another process/job owner or changing archive algorithms.
At the pinned registry revision, `CPP/7zip/UI/Common/Update.cpp::UpdateArchive`
deletes the original before `MyMoveFile_with_Progress`, retaining temporary output
on failure; `CPP/Windows/FileDir.cpp::CTempFile::MoveTo` also deletes first.
`CPP/7zip/UI/Common/WorkDir.cpp::CWorkDirTempFile::MoveToOriginal` delegates to
that operation. None supplies the existing target-to-backup, publish, restore
sequence. That process-boundary policy is retained in the backend mutation owner.
`CPP/7zip/UI/Console/List.cpp` prints the listing/property labels: the existing
inverse transport parser and result projection remain together. Source update,
extract and native reader/writer bodies are not copied again or reorganized.
The extracted C++17 units share the backend's original state, QObject signals,
QProcess and joined worker lifetime; only private helper declarations are added.
The late-consumer source list and shared backend object group include the same
completion unit. Existing public mutation/backend/source-update/GUI/create/
volume/SFX tests exercise the moved code. No UI text/layout changes are made.
After task-owned clang-format, reconstructing the original complete() from the
four private methods matches all1940 functional tokens, including string/character
literals. The three completion methods have63/50/61 body lines. No new archive
implementation is borrowed: existing per-module baselines/notices remain intact.
Main focused8/8 PASS4.50s; unsandboxed full gate passes formatting, all-target
build and241/241 CTest101.09s on macOS arm64. The full log is retained at
`build/review/p211-j-main-full-lasttest.log`. Its compile database has137 root
production units, each once, including the new completion unit.
Fresh integrated snapshot `20261005T075633Z-003d3531` passes Linux8/8 in15.92s
and Windows interactive Highest8/8 in63.28s. Actual Qt totals on each OS are
backend45, mutation24, sourceUpdate76, updateWindow11, compression21, volume26,
SfxCreation24 and SfxLifecycle10, all zero failed/skipped. Thirteen guest/local
artifacts per guest are SHA-256 verified and retained under that snapshot's
evidence directory. Private helpers retain QObject/process/worker lifetime;
no test-only internals are exposed. Whole-root inventory and final integrated
platform/UX acceptance remain P2-11/P3; focused guests do not replace that gate.

### P2-11.11 — compression request and creation boundary (accepted, 2026-10-05)

The new C++17 backend compression unit
retains source `NCompressDialog::CInfo`, original format/method/dictionary/order/
thread/solid/memory calculations, `GetCompressionProperties` and
`CheckDeleteAfterCompressingActionSet`. Strict public-request field checks and
projection into Console switches remain integration code: original OnOK reads
Win32 controls and does not validate arbitrary externally supplied DTO values.
The former backend memory refusal now calls the source `CCompressDialog::OnOK`
already accepted in P2-11.9; language lookup, required/RAM/limit message and the
unknown-RAM/unspecified-limit guard have one implementation. Original OnOK's
body/callers/types were reviewed again at the registry's pinned full revision.
Creation moves to the existing creation/lifecycle unit; all1186 functional tokens
of its body, including literals, match after formatting. Compression properties
and switches match except that source-owned memory call. The mutation unit is
335 lines, compression176 and creation274. The two longer methods were reviewed:
immutable request validation remains together; creation collects complete root/
path/collision/destination checks before installing one asynchronous job state.
These coherent phases remain intact rather than adding another mutable plan owner.
Upstream bodies/module organization are unchanged; no new borrowed file/revision
or source language change is introduced. The shared object and late-consumer
lists include the same new unit. Existing public compression_memory/limit tests
exercise success/refusal/cancel, Unicode bytes, native error text and preserved
input/output; no internal-only tests are added. Main focused10/10 PASS6.71s;
the unsandboxed full macOS gate passes formatting/all-target build and241/241
CTest100.13s, saved at `build/review/p211-k-main-full-lasttest.log`.
The first full attempt stopped at a task-owned documentation trailing space
before build/tests; corrected diff passes the complete gate above.
Fresh integrated snapshot `20261005T080746Z-f066976b` passes Linux10/10 in16.08s
and Windows interactive Highest10/10 in61.11s. Actual Qt totals are
sourceUpdate76, updateWindow11, backend45, mutation24, compression21, volume26,
SfxCreation24, SfxLifecycle10, compressionMemory14 and compressionLimit10,
all zero failures/skips on each OS. Twenty-three guest/local artifacts per guest
are SHA-256 verified. Mac/Windows138 and Linux136 root production units each
compile once; the compression unit belongs to the shared backend object owner.
No UI layout/text changes require new appearance inspection. Whole-root inventory
and final P3 platform/UX acceptance remain open.

### P2-11.12 — panel presentation boundaries (accepted, 2026-10-05)

Original FileManager `Panel.cpp::OnCreate`,
`PanelItems.cpp::OnTimer/OnReload/RefreshListCtrl` and their current adapters
were inspected at the pinned registry revision. Native `CPanel` retains column
catalog, property text, selection snapshots, timer/update guards and reload policy.
Qt QWidget/model/header updates have no original portable implementation.
Column projection/persistence/context/time redraw now reside together in
`mainwindow_columns.cpp`; manual refresh/reopen, filesystem change observation,
viewport restoration and the reload projection callback reside in
`mainwindow_refresh.cpp`. The latter remains a method of the existing MainWindow,
without a second panel/job/selection owner. The constructor only registers its
callback; original capture and invocation lifetime are preserved. Every moved
column/refresh/reload functional token, including literals, matches after
task-owned formatting. The unused addSources body/declaration is removed after
full source/test caller search; source CPanel::AddToArchive and backend public Add
remain the real consumers. Construction/filelist/columns/refresh units are
336/306/171/174 lines, constructor unit197. Existing longer construction/transfer
methods were reviewed as cohesive control wiring and one captured source command;
no unrelated source module organization or workflow is changed. Shared MainWindow
objects and late consumers include the same two new C++17 units. No new borrowed
module/revision or layout/text change is introduced. Main focused11/11 PASS18.70s,
full unsandboxed formatting/all-target build241/241 PASS101.78s. Integrated
snapshot20261005T082042Z-ee146952 Linux11/11 PASS25.04s; Windows10/11 PASS181.05s,
with large_list_window disk-normal exceeding its unchanged15s threshold
(13passed/1failed/0skipped). An unchanged Highest interactive session1 retry
passed1/1 CTest109.03s,14Qt rows without failures/skips. Retry projections were
disk13089/11624ms and archive9361/9377ms, maximum GUI gaps209/194/134/126ms.
Original failure39 artifacts and separate retry41 artifacts, plus Linux40,
were retrieved and guest/local SHA256-verified. No code, timeout or threshold
was changed for the retry. Other ten direct suites passed without Qt skips;
normal/compact widget reports were retrieved. The extraction is accepted;
stable final Windows/P3 gate and whole-root acceptance remain open.

### Explicit root-file scope (258 paths, reviewed component register)

This table spells out every root C++/header/Objective-C++ path at the P2-11.12
checkpoint; all paths are relative to `src/`. It supplements the functional
change rows and retained-body reasons above, rather than treating file counts
as proof of source parity. Borrowed `src/CPP` and `third_party/7zip` modules
remain in the per-module baseline/audit and P3-05 dependency/omission closure.
Listed CTest consumers select the component; their dated acceptance evidence
remains with its owner. All twelve remediation rows are accepted; final parent
evidence and retained responsibility decisions follow the table.

| Component and exact files | Source basis / disposition | Direct consumers |
| --- | --- | --- |
| Backend transport: `archivebackend.cpp`, `archivebackend.h`, `archivebackend_codecs.cpp`, `archivebackend_completion.cpp`, `archivebackend_compression.cpp`, `archivebackend_creation.cpp`, `archivebackend_hash.cpp`, `archivebackend_listing.cpp`, `archivebackend_mutations.cpp`, `archivebackend_preparation.cpp`, `archivebackend_sourceextract.cpp`, `archivebackend_sourceplan.cpp`, `archivebackend_sourceupdate.cpp`, `archivebackend_transfer.cpp`, `archivebackend_validation.cpp`, `archivebackend_volumes.cpp`, `archivebackend_workerprogress.cpp`, `archivenativeopen.cpp`, `archivenativeopen.h`, `archiveprogressoperation.cpp`, `archiveprogressoperation.h`, `archiveprogresstransport.cpp`, `archiveprogresstransport.h`, `qtarchivebackendrequest.cpp`, `qtarchivebackendrequest.h`, `qtarchivebatchextract.cpp`, `qtarchivebatchextract.h`, `qtarchivebatchopen.cpp`, `qtarchivebatchopen.h`, `qtarchivecopypreparation.cpp`, `qtarchivecopypreparation.h`, `qtarchivecreationlifecycle.cpp`, `qtarchivecreationlifecycle.h`, `qtarchivedeleteinputs.cpp`, `qtarchivedeleteinputs.h`, `qtarchivefileresource.cpp`, `qtarchivefileresource.h`, `qtarchivefoldermanager.cpp`, `qtarchivefoldermanager.h`, `qtarchivehashoperation.cpp`, `qtarchivehashoperation.h`, `qtarchivenativeopenprogress.cpp`, `qtarchivenativeopenprogress.h`, `qtarchivenativeproperties.cpp`, `qtarchiveproperties.cpp`, `qtarchiveproperties.h`, `qtarchivereopen.cpp`, `qtarchivereopen.h`, `qtarchivesourceextract.cpp`, `qtarchivesourceupdate.cpp`, `qtarchivesourceupdate.h`, `qtarchivestreamopen.h`, `qtarchivetransferbackup.h`, `qtarchivevolumeoutput.cpp`, `qtarchivevolumeoutput.h` | OpenArchive/AgentOut/ArchiveFolder/ExtractGUI/HashCalc/Update/CopyStream/CMultiOutStream; QProcess callback/metadata transport, native item-index proof, bounded publication and leased streams. Resolved U/V/W/J/K; codec algorithms remain source-owned. | `archive_backend`, `archive_mutation`, `source_update_backend`, `source_extract_backend`, `archive_volume_creation`, `sfx_lifecycle` |
| Codec/runtime discovery: `archivecodeccatalog.cpp`, `archivecodeccatalog.h`, `archivecodecimage.cpp`, `archivecodecimage.h`, `archivecodecprobe.cpp`, `archivecodecprobe.h`, `archiveenginelocation.cpp`, `archiveenginelocation.h`, `archiveexternalcodecs.cpp`, `archivesfxmodule.cpp`, `archivesfxmodule.h` | LoadCodecs/OpenArchive and original module ABI; immutable provider/module leases, subprocess probe, official engine/SFX location and byte-identity validation. No independent handler/compression implementation. | `codec_catalog`, `codec_probe`, `native_library`, `archive_backend`, `external_codecs`, `sfx_creation` |
| Interface/alias contracts: `combodialog.h`, `copydialog.h`, `extractdialog.h`, `qtcompresscallservices.h`, `qtcontextmenuservices.h`, `qtextractpathpolicy.h`, `qtfilemenuservices.h`, `qtfmoptionsdialog.h`, `qtmodalresult.h`, `qtpanelpluginservices.h`, `qttrashservice.h`, `qtupdateguiservices.h`, `settingsdialog.h` | Native dialog aliases, resource result constants and typed source service interfaces; small inline forwarding methods. CQtExtractPathPolicy owns only immutable destination and delegates already normalized path/link checks to ArchiveBackend. | `source_compress_dialog`, `source_extract_contract`, `archive_update_callback`, `settings`, `file_menu_policy` |
| Compress presentation: `createarchivedialog.cpp`, `createarchivedialog.h`, `createarchivedialog_advanced.cpp`, `createarchivedialog_compression.cpp`, `createarchivedialog_encryption.cpp`, `createarchivedialog_formats.cpp`, `createarchivedialog_language.cpp`, `createarchivedialog_preferences.cpp`, `createarchivedialog_resources.cpp`, `createarchivedialog_sfx.cpp`, `createarchivedialog_source.cpp`, `createarchivedialog_volumes.cpp`, `qtcompressoptionsdialog.h` | Source CompressDialog/COptionsDialog/CInfo owns parameters/profile/memory policy; Qt reads controls, binds resources and projects capabilities. Resolved C/S/K; no second DTO/profile owner. | `compression`, `compression_advanced`, `compression_preferences`, `compression_limit`, `compression_resources`, `shell_update_window` |
| Association: `desktopidentity.h`, `qtassociationfileopen.cpp`, `qtassociationfileopen.h`, `qtassociationplatform.h`, `qtassociationplatform_linux.cpp`, `qtassociationplatform_mac.cpp`, `qtassociationplatform_mac.mm`, `qtassociationplatform_win.cpp`, `qtassociationservices.cpp`, `qtassociationservices.h`, `qtmacassociationnative.h` | Source RegistryAssociations/SystemPage owns page/policy; Qt/native per-user platform APIs, app-open events and external-change revision checks. NSWorkspace/UTType/NSBundle require Objective-C++. | `association_services`, `association_file_open`, `system_page`, `file_manager_startup` |
| File/panel projection: `filesystemfolderloader.cpp`, `filesystemfolderloader.h`, `filesystemscanner.cpp`, `filesystemscanner.h`, `filetable.cpp`, `filetable.h`, `qtfolderprojection.cpp`, `qtfolderprojection.h`, `qtfolderpropertycapabilities.h`, `qtlocationfolder.cpp`, `qtlocationfolder.h`, `qtpanelfolder.cpp`, `qtpanelfolder.h`, `qtpanelfolderraw.cpp`, `qtpanellist.cpp`, `qtpanellist.h` | Source FSFolder/Agent/PanelItems/PanelSort/PanelSelect owns enumeration, properties and selection; Qt model/view, incremental projection, worker lifetime and inverse Console/List metadata have no upstream Qt equivalent. | `filesystem_scanner`, `large_list_window`, `panel_columns`, `panel_selected_state`, `archive_proxy` |
| Transfer transaction: `filetransferoperation.cpp`, `filetransferoperation.h`, `qtfiletransfercopycallback.cpp`, `qtfiletransfercopycallback.h`, `qtfiletransfertransaction.cpp`, `qtfiletransfertransaction.h` | Source FSFolderCopy/CCopyStateIO/CopyStream owns copying. Existing exclusive stage/whole-batch rollback and written-source postorder cleanup are not provided by source recursion/deletion; one worker state, resolved F. | `file_transfer`, `file_transfer_direct`, `language_file_transfer`, `worker_window_progress`, `fs_folder_copy` |
| App presentation and state: `main.cpp`, `mainwindow.cpp`, `mainwindow.h`, `mainwindow_archive_transfer.cpp`, `mainwindow_archivecommands.cpp`, `mainwindow_archivelist.cpp`, `mainwindow_archiveopening.cpp`, `mainwindow_backendevents.cpp`, `mainwindow_close.cpp`, `mainwindow_columns.cpp`, `mainwindow_dialogs.cpp`, `mainwindow_filelist.cpp`, `mainwindow_fileoperations.cpp`, `mainwindow_language.cpp`, `mainwindow_menus.cpp`, `mainwindow_panel.cpp`, `mainwindow_preferences.cpp`, `mainwindow_propertyinvalidation.cpp`, `mainwindow_refresh.cpp`, `mainwindow_root.cpp`, `mainwindow_selection.cpp`, `mainwindow_selectionpaths.cpp`, `mainwindow_twopanel.cpp`, `mainwindow_volumes.cpp`, `qtappoperationstate.cpp`, `qtappoperationstate.h`, `qtapppresentation.cpp`, `qtapppresentation.h`, `qtfilemanagercommands.cpp`, `qtfilemanagercommands.h`, `qtfilemanagercommandservices.h`, `qtfilemanagercommandworkflows.cpp`, `qtfilemanagermain.cpp`, `qtfilemanagerstartup.h`, `qtfilemanagerwindowservices.h`, `qtguicommand.h`, `qtguimain.cpp`, `volumeoperations.cpp`, `volumeoperations.h` | CApp/CPanel/WndProc/FM/GUI/PanelCopy/PanelSplit owns command/lifecycle policy; Qt window/panel projection, operation guards and asynchronous delivery. Resolved R/P; source command availability has one owner. Planned archive Move extension is distinguished below. | `ui_operations`, `ui_copy_same`, `ui_state`, `ui_navigation`, `app_lifecycle_window`, `binding_lifecycle`, `panel_focus`, `panel_runtime`, `panel_auto_refresh` |
| OS process services: `processpause.cpp`, `processpause.h`, `processpriority.cpp`, `processpriority.h`, `qtassociatedfileservices.cpp`, `qtassociatedfileservices.h`, `qtexternalprocess.cpp`, `qtexternalprocess.h` | Source EditItem/CChildProcesses/MyThreadFunction owns edit/wait workflow; OS detached launch, PID identity/group/reaping, suspend increments and priority restoration need native bridges. | `external_editors_ui`, `archive_external_edit`, `process_pause`, `temp_detached_launch` |
| Benchmark controls: `qtbenchmarkcontrols.cpp`, `qtbenchmarkcontrols.h`, `qtbenchmarkservices.cpp`, `qtbenchmarkservices.h`, `qtmemorylimitspin.cpp`, `qtmemorylimitspin.h` | Original Bench/BenchmarkDialog/GetRamSize and memory parser calculate values; Qt timer/combo/spin/resource adapters replace Win32 presentation. | `native_benchmark`, `benchmark_command`, `fm_settings`, `memory_limit` |
| Callback/pause transport: `qtcallbackdialogs.cpp`, `qtcallbackdialogs.h`, `qtcallbackpassword.cpp`, `qtcallbackpassword.h`, `qtcallbackprogresstransport.cpp`, `qtcallbackprogresstransport.h`, `qtexternalguicall.cpp`, `qtexternalguicall.h`, `qtprogresstransport.h`, `qtprogresswait.cpp`, `qtprogresswait.h` | Source callback/thread/event precedence; Qt GUI dispatch/start-wait barrier and capability-specific process versus worker pause/background. No duplicate progress state after W. | `archive_update_callback`, `progress_wait`, `archive_progress`, `archive_external_lifetime`, `worker_checkpoint` |
| Screen/control services: `qtcompressoptionscontrols.cpp`, `qtcompressoptionscontrols.h`, `qtdialogscreenservices.cpp`, `qtdialogscreenservices.h`, `qtlinkcontrols.cpp`, `qtlinkcontrols.h`, `qtprogresscontrols.cpp`, `qtprogresscontrols.h` | Original CompressDialog/LinkDialog/ProgressDialog2 handlers/resources; Qt widget construction, ID lookup, font/work-area geometry and control forwarding. Long form builders are one resource presentation responsibility. | `compression_advanced`, `link_command`, `progress_dialog`, `ui_geometry` |
| List hover: `qtfilelisthover.cpp`, `qtfilelisthover.h`, `qtlistview.cpp`, `qtlistview.h` | One Qt dwell adapter replaces Win32 TRACKSELECT presentation for all source dialog/main/temp views; resolved H. | `list_activation`, `list_view_dialog`, `folder_history`, `panel_key`, `temp_cleanup` |
| Drag and temp browser: `qtfilemanagerdrag.cpp`, `qtfilemanagerdrag.h`, `qtfilemanagerdraginput.cpp`, `qtfilemanagerdragpresentation.cpp`, `qtpaneldragmodel.h`, `qtpaneldragtransport.cpp`, `qtpaneldragtransport.h`, `qttempbrowsepresentation.cpp` | Original PanelDrag/App transfer policy; QDrag/MIME/right-drag hit testing and leased recipient lifetime replace OLE presentation. Real Finder proof remains mandatory P3-03.1. | `drag_drop`, `drag_archive`, `temp_cleanup` |
| Native menus/shell: `qtfilemanagershellmenu.cpp`, `qtfilemanagershellmenu.h`, `qtfilemenuprojection.cpp`, `qtfilemenuprojection.h`, `qtmenupageservices.h`, `qtnativemenupageservices.cpp`, `qtnativemenupageservices.h`, `qtnativeshellmenu.cpp`, `qtnativeshellmenu.h`, `qtnativeshellmenupresentation.cpp`, `qtnativeshellmenupresentation.h`, `qtprogramfilemenu.cpp`, `qtprogramfilemenu.h`, `qtshellarchiveservices.cpp`, `qtshellarchiveservices.h`, `qtshellmenupresentation.cpp`, `qtshellmenupresentation.h`, `qtshellregistryservices.h`, `wincontextmenuservices.cpp`, `wincontextmenuservices.h`, `winshellmenupresentation.cpp`, `winshellmenupresentation.h` | Source CFileMenu/CreateSystemMenu/CZipContextMenu/RegistryContextMenu owns query/invoke/registration. Qt COM/HMENU/GDI/DPI/message/resource and per-user native key services project that owner. | `file_menu_policy`, `native_system_menu`, `native_menu_integration`, `native_shell_data`, `shell_update_window` |
| Language/help: `qthelpservices.cpp`, `qthelpservices.h`, `qtlangcontrols.cpp`, `qtlangcontrols.h`, `qtlanguageservices.cpp`, `qtlanguageservices.h`, `qtstatuslabel.cpp`, `qtstatuslabel.h` | Original CLang owns parsing/lookup; Qt LanguageChange/resource spooling/bindings and offline CHM content viewer replace native dialog/CHM services. Status-part clipping retains tooltip/source text. | `language_ui`, `language_dialog`, `language_source_update`, `help_ui`, `panel_shell` |
| Taskbar: `qtlauncherprogress.cpp`, `qtlauncherprogress.h`, `qttaskbarprogress.h`, `qttaskbarprogress_linux.cpp`, `qttaskbarprogress_mac.mm` | Source taskbar interface; Linux launcher DBus and Apple NSDockTile protocols have no portable original service. Objective-C++ bridge preserves native HRESULT/BOOL ABI and owns only its badge value. | `progress_taskbar`, `launcher_progress` |
| Links/mounts/version control: `qtlinkservices.cpp`, `qtlinkservices.h`, `qtverctrlfileservices.cpp`, `qtverctrlfileservices.h`, `qtvolumemounts.cpp`, `qtvolumemounts.h` | Original FileFind/FileIO/FileDir/Reparse/time and VerCtrl owners; exclusive link stage/time rollback, Unix mount namespace and POSIX file-info/attribute bridges replace platform services. | `filesystem_link`, `link_command`, `volume_mount`, `root_navigation`, `verctrl` |
| Temp leases: `qtownedtempdir.cpp`, `qtownedtempdir.h`, `qtownedtempidentity.cpp`, `qtownedtempidentity.h`, `qtownedtempregistry.cpp`, `qtownedtempregistry.h` | Original TempFiles lacks persistent Qt leases, restart claim locks and OS object identity. Separate directory lease, registry and identity services protect external consumers and publication. | `owned_temp`, `temp_cleanup`, `owned_temp_publication`, `temp_detached_launch` |
| Source worker checkpoints: `qtworkerprogress.cpp`, `qtworkerprogress.h` | Original CProgressSync owns flags/checkpoints; Qt pimpl and publication lock only bridge native worker wake/lifetime, resolved W. | `worker_checkpoint`, `worker_window_progress`, `archive_volume_creation` |

Archive-to-disk/archive Move is an explicit accepted P1-01/port-plan extension:
original `Agent/ArchiveFolder.cpp::CAgentFolder::CopyTo(moveMode)` returns
`E_NOTIMPL`, and original `App.cpp::OnCopy` passes that flag directly. The Qt
workflow composes source Copy/Delete or official backend deletion after successful
extraction; source refusal/cancel and destination rollback remain checked.
This composition is retained because no usable original archive Move body exists
and the plan requires it; it is not claimed as identical Windows 7zFM behavior.
Real Finder, rendered UX, installed native integration and final complete platform
gates remain P3 evidence, independently of this integration-code review.
### Retained adapter review

Reviewed bodies provide concrete retention reasons below. The exact register
above accounts for every root file/header; all remediation rows are settled.
The following component reasons also preserve the original investigation scope.

| Files/component | Source basis / retained integration responsibility | Direct consumers/checks |
| --- | --- | --- |
| `qtfolderprojection.*`, `filesystemfolderloader.*`, `filesystemscanner.*`, `filetable.*` | FSFolder LoadItems and PanelItems/PanelSort own enumeration/order; Qt incremental widget projection and worker ownership have no Win32-independent presentation equivalent | large_list_window, filesystem_scanner, flat_view, panel_selected_state |
| `qtpanelfolder*`, `qtpanellist.*`, `qtlocationfolder.*` | Agent/native folder interfaces remain authoritative; inverse Console.List metadata transport and Qt item-model/control operations have no upstream Qt implementation | archive_proxy, binding_lifecycle, panel_property/columns, source_properties |
| `archivecodeccatalog.*`, `archivecodecimage.*`, `archivecodecprobe.*`, `archiveenginelocation.*`, `archiveexternalcodecs.cpp` | Original LoadCodecs/OpenArchive/handler ABI; Qt immutable module lifetime/discovery/process location. The original external-codecs ABI storage is not a second mutable workflow owner | codec_catalog, external_codecs, native_library, engine_location |
| `qtarchivebackendrequest.*`, `qtarchivefoldermanager.*`, `qtarchivenativeopenprogress.*`, `qtarchivereopen.*`, `qtarchivesourceextract.cpp` | Original Open/Extract/ReOpen own algorithms; Qt file/process/synchronous callback-to-GUI bridge, cancellation and lifetime | archive_open, nested_archive/edit_chain, source_extract, app_lifecycle_window |
| `qtarchiveproperties.*`, `qtarchivenativeproperties.cpp` | IInArchive/raw/native item-index proof gates retained reader use; Archive_IsItem_Dir supplies native defaults | source_properties, raw_properties, tree_proxy, comment, rar_memory |
| `qtownedtempdir.*`, `qtownedtempregistry.*`, `qtownedtempidentity.*` | Source TempFiles lacks persistent Qt leases/OS object identity/restart claims. Lease ownership protects external readers and publication | owned_temp, temp_cleanup, owned_temp_publication, detached_launch |
| `qtbenchmarkservices.*`, `qtbenchmarkcontrols.*`, `qtmemorylimitspin.*` | Original Bench/GetRamSize/Settings handlers retain calculation; Qt controls/timer/edit/updown adapt Win32 presentation | benchmark, benchmark_ui, fm_settings |
| `qtlinkservices.*`, `qtlinkcontrols.*`, `qtvolumemounts.*` | Native FileFind/FileIO/FileDir/Reparse retained; exclusive link staging/identity/time rollback and Unix QStorageInfo namespace are required OS integration | link_operations/ui, root_navigation, volume_mounts |
| `qtlanguageservices.*`, `qtlangcontrols.*`, `qtstatuslabel.*`, `qthelpservices.*` | CLang owns lookup/parser; resource fallback/LanguageChange/status-part projection and portable offline CHM viewer replace Win32 UI | language_ui/dialog/source_update, help_ui, panel_status |
| `qtexternalguicall.*`, `qtcallbackdialogs.*`, `qtcallbackpassword.*`, `qtprogresswait.*`, `archiveprogresstransport.*`, `qtcallbackprogresstransport.*` | Native callback/thread/event precedence retained; Qt GUI dispatch and process pause/priority rollback differ from native-worker capabilities | callback_lifecycle, progress_wait/transport, external_gui |
| `qtassociatedfileservices.*`, `qtexternalprocess.*`, `processpause.*`, `processpriority.*` | Source CChildProcesses/EditItem owns workflow; detached OS launch and waitable process ownership/pause/group/reap need separate platform bridges | external_editors, process_controls, temp_detached_launch |
| `qtassociation*`, `qtmacassociationnative.h`, `desktopidentity.h` | Original System/Register owner; per-user Linux/Windows registration, NSWorkspace/UTType/NSBundle need actual native APIs. Objective-C++ retained only for those APIs | system_page, association_services/file_open, fm_startup |
| `qtnative*menu*`, `win*menu*`, `qtshell*`, `qtfilemenu*`, `qtprogramfilemenu.*` | Original CFileMenu/CreateSystemMenu/CZipContextMenu owns tree/query/invoke; Qt projects native COM/HMENU/GDI/DPI/message ABI and dispatch | file_menu, native_system_menu, native_shell_data/com, menu_integration |
| `qtpaneldrag*`, `qtfilemanagerdrag*`, `qttempbrowsepresentation.cpp` | Original OnDrag/Drop/transfer owns command policy; Qt typed MIME/materialization/recipient lifetime and leased temporary browser are presentation/OS transport | drag_drop/archive, panel_key, temp_cleanup; actual Finder remains P3-03.1 |
| `qttaskbarprogress*`, `qtlauncherprogress.*` | Source taskbar progress projected through Windows interface, Linux launcher DBus or NSDockTile; Apple bridge avoids native BOOL ABI conflict | taskbar_progress, launcher_progress |

The remaining presentation groups retain these reviewed responsibilities:
`mainwindow*`/`qtapppresentation.*` project existing source commands, focus and
synchronous-call lifetime; `archivebackend*`/`archiveprogressoperation.*` own
process arguments, validation, phase/error delivery and native operation lifetime.
`createarchivedialog*`/`archivesfxmodule.*` bind the original CompressDialog DTO
and source capabilities to Qt and locate the official SFX binary. There is no
second compression profile owner after C/S/K. `qtappoperationstate.*` retains
per-window call/close guards; command/workflow/menu/startup units connect CApp,
WndProc, native FM/GUI entries and Qt services. Command availability remains
authoritative in QtFileManagerCommands; widgets do not construct engine syntax.
`volumeoperations.*` schedules original Split/Combine, and control/screen/version
services replace Win32 presentation and OS calls. No usable portable upstream
Qt implementation supplies these services. Their exact consumers are in the
register and accepted feature/component evidence, not inferred from filenames.
The small interface/alias headers (`combodialog.h`, `copydialog.h`,
`extractdialog.h`, `settingsdialog.h`, `qtfmoptionsdialog.h`,
`qtcompressoptionsdialog.h`, `qtmodalresult.h`, `qtfolderpropertycapabilities.h`,
`qtarchivestreamopen.h`, `qtextractpathpolicy.h`, and the `*services.h`/startup/
drag-model contracts) keep native declarations/typed boundaries rather than
independent archive algorithms. COptionsDialog's moved declaration retains its
source name/body and keeps Widgets out of the engine DTO header. Bounded
extraction path/link checking belongs to ArchiveBackend; source normalization,
link and write algorithms remain authoritative. Native typed interfaces retain
only declarations or small forwarding bodies; actual all-target compilation
and direct consumers verify their include/ABI closure.

### P2-11 parent acceptance (2026-10-05)

The final register has258 distinct existing root paths in20 components, exactly
matching current src root .cpp/.h/.mm files. Twelve integrated remediation rows
are implemented in verified integrated commits, ending with97c9e14. No new borrowed revision or
notice change is introduced by the final extraction. Baseline verification
reports409 exact/195 adapted/3 local paths at the pinned upstream revision.
Actual compile databases from the last integrated panel snapshot show:
macOS138 C++17 plus2 Objective-C++17 root units; Windows140 C++17 root units;
Linux138 C++17 root units. Each appears once. C entries/distinct paths are
46/44,47/45 and46/44 respectively, compiled as C without forced C++ flags.
Repeated C paths belong to separate targets; they are the same authoritative file.

Large custom files remain explicit review decisions: archivebackend.cpp550
retains process construction/lifetime and public dispatch after J/K; native
codecs390 and source-update589 retain one provider/callback bridge; filesystem
transaction402 retains its exclusive-stage/merge/rollback scope after F.
Fileoperations405 and menus373 retain source-command presentation; commands399
owns capability projection; native-shell presentation375 owns native menu ABI.
Further generic extraction would separate the same cohesive service or alter
borrowed callback control flow. Long native-command/control builders remain
reviewed presentation wiring. No size-only rewrite of upstream modules is made.

All remediations preserve source algorithms, Cancel/error/worker lifetime and
consumer behavior, with the dated public regressions and full macOS gates above.
Latest full macOS formatting/build241/241 passes; the last extraction's Linux
eleven consumers pass, and Windows ten pass plus unchanged large-list retry.
The original Windows timing failure remains preserved and final stable full
Windows/Linux gates, real Finder proof, packages, omitted-branch closure and
rendered EN/RU UX acceptance remain mandatory P3 work. Parent P2-11 accepts
the integration-code register and remediations, not final product acceptance.

### P2-11.1 — shared hover presentation (accepted, 2026-10-05)

Inspected original and adapted full ListViewDialog.cpp, its CFmSettings style
caller, current CListView/QtFileListHover, main table/icon and temporary-browser
consumers. The source dialog still owns activation, clipboard/delete/confirmation;
Qt supplies the400ms dwell because Win32 TRACKSELECT is unavailable. CListView
now delegates to the existing shared adapter and drops its second timer/index/
mouse overrides. Hide cancels the shared pending dwell just like wheel/leave/
press/disable/drag suspension. QObject view lifetime owns the adapter.

Public regression first fails wheel and hide/show (12 other Qt cases pass),
then verifies cancellation and fresh dwell restart along with close/disable.
Mouse delivery uses QTest's QWindow QPA overload, preserving the product timer.
Focused list_activation/list_view_dialog/folder_history/panel_key/properties_runtime/
temp_cleanup pass6/6 in7.49s on macOS arm64 Qt6.11.2. Unsandboxed full gate
passes formatting/all targets/238 tests in101.51s. Snapshotee52cd4b's eight
Linux consumers pass8/8 in23.90s; actual LastTest verifies all14 activation,
55 panel-key,22 progress-dialog,11 temp-cleanup and other cases without skips.
Windows's same snapshot passes8/8 in65.06s; actual LastTest verifies the same
14 activation/55 panel/22 progress and other affected cases with zero failures.
Its temp-cleanup's existing directory-link setup case skips because QFile::link
does not create the requested native directory symlink on Windows. That fixture
gap remains mandatory P3-02 review, separate from shared-hover acceptance; the
other10 temp-cleanup cases pass. No OS security policy or product change.
Baseline verifies409 exact/195 adapted/3 local, with paths/bodies unchanged.
No geometry/text/resource changes. All72 current Mac/Windows/Linux history/hash/
temp/progress JSON reports were retrieved/reviewed: every visible affected
control fully inside its normal/compact owner, grouping/order/separation retained.
The native Windows interactive desktop was used for focus/window consumers;
rendered pixel parity remains P3-06. This resolves registry row H, not P2-11.

### P2-11.2 — canonical source compression DTO (accepted, 2026-10-05)

Read original CompressDialog.h CInfo and control spec accessors, full OnOK,
GetMethodSpec/GetMethodID/SetMethod2, UpdateGUI.cpp SetOutProperties and current
request producer/sourceInfo/backend validation/creation/SFX bodies and callers.
The five lower-case request fields shadowed inherited native CInfo values:
setting native Level/Method/Dict64/Order/NumThreads did not reach the backend.
They are removed; all request consumers now use the original fields. The wrapper
initializes native Level to the previous public default5 (source SetLevel2's
normal default). Backend validation still requires a supported explicit0–9 level;
the other native auto values are UInt32/64(-1), not a second zero convention.
QString conversion occurs only at Qt method helpers/presentation; Method retains
UString and source GetMethodSpec's empty auto/store choice. UI method names,
selection, settings, memory/format validation and source property parser remain
their existing owners. sourceInfo drops its duplicate five-field conversion.

Public source-DTO property comparisons cover PPMd order/memory and LZMA2 fast-
bytes/threads, matching the actual retained GetCompressionProperties output.
Separate invalid native level/method/thread/dictionary/order rows reproduce the
previously ignored fields. Fixtures follow source capability tables: PPMd uses
auto threads, LZMA2 explicit2;128KiB is invalid for the tested LZMA2 dictionary,
while3MiB is valid and must not be called invalid. Existing zero-auto fixture
inputs/expectations were migrated to native sentinels, with success/extracted
bytes/failure/cancel checks retained. The unrelated shell-launch method fields
were excluded from the validated177-occurrence request-field migration.
Focused compression/source_compress_dialog/resources/preferences/dictionary/
order/solid/memory/limit pass9/9 in2.94s on macOS arm64 Qt6.11.2; the unsandboxed
full gate passes238/238 in105.58s. The added compression script's backend scope
passes10/10 locally in4.62s and in snapshot20261004T224014Z-cbd2b6e7 on Windows
in44.71s/Linux in17.70s. Actual Windows native metadata/link fixtures have no
skips; ten Windows-only metadata cases are inapplicable on Linux. Linux UI
snapshot20261004T223624Z-23e6eaf4 passes49/49 in75.16s with no QTest skips.
Windows Highest interactive full snapshot20261005T011510Z-a5cc8449 passes all49
script-defined UI consumers with zero QTest failures/skips, including all six
native metadata UI cases. Its unrelated resource parser/temp/list-settings
fixture failures remain separately owned; whole Windows gate is not accepted.
Current Windows backend scopef5d4ebde passes10/10 in42.52s without skips.
macOS current full238/238 passes103.48s; Linux current full234/234 passes194.36s.
Windows Properties consumers use an exposed-window observer for the real
native property sheet; unique extensionless disposable filenames avoid the
OS extension-display setting changing its caption. Main command/panel/language
fixtures activate their requested window before asserting native focus; native
rendering and command outcomes remain checked, with no Qt Properties substitute.
Current Mac/Windows/Linux compression JSON preserves exposed controls, source
values and compact scrolling; no
borrowed body/path/revision or UI layout changes. Baseline verification passes
409 exact/195 adapted/3 local paths. Other registry rows remain independently owned.

### P2-11.3 — retained source volume accounting (accepted, 2026-10-05)

Inspected original CPP/7zip/UI/Common/OpenArchive.cpp CArchiveLink::Open2,
its COpenCallbackImp volume stream/path/size accumulation, source Extract.cpp
batch skip/accounting consumers, current ArchiveNativeOpen/QtArchiveBatchOpen
and resource update/readiness callers. Source repository and full revision are
https://github.com/ip7z/7zip.git at0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
No borrowed bodies, notices, dependencies or baseline revision changed.

ArchiveNativeOpen now exposes actual retained-link volume presence and copies
its VolumePaths/VolumesSize. The batch adapter uses those source values, including
an empty list for a normal archive. Its previous numbered-file reconstruction is
used only without a native reader. Resource RequiresOuterOpen/TransportFormat
use that same native state while preserving nested streams, source read-only
flags and offset constraints. A false process volume count no longer disables
a proven single-file writer, and an omitted count cannot lose source batch
volume accounting. File/process transport and final publication stay unchanged.

Four public batch rows use actual128-byte ZIP volumes with Unicode/spaces and
compare to the original native link. They cover missing/untrusted process count,
missing first volume and native-open E_ABORT, with exact surviving/source bytes,
joined worker, zero process listing on refusal and no opened link after failure.
The fifth regression checks normal ZIP writer readiness with a false10001 count.
macOS focused4/4 and current full238/238103.48s pass. Linux final direct5/5 and
current full234/234194.36s pass; Windows a5cc8449 passes all five direct suites
(archive_open_options/source_extract_backend/source_extract_advanced/
shell_archive_services/codec_creation), with no QTest failures/skips. The49
compression/volume/SFX UI consumers also pass on Windows; its full244 gate has
three unrelated parser/modal fixture failures, so global platform acceptance
remains open. No layout/text changes; registry U/W/F/B are independently owned.

### P2-11.4 — production object ownership (accepted, 2026-10-05)

Final integrated macOS formatting/all-target build/full gate passes 240/240 in
102.40s; compile database970 has134 root production TUs, each compiled once.
Linux final full d20a669f passes 235/235 in190.58s; its959 commands have132
root production TUs, each compiled once. Windows 4b4d50c2 builds all targets;
1012 commands have134 root production TUs once, with47 C entries. Its four
test failures and resolution are retained below: unchanged external/large
retry passes2/2 in124.91s, and fresh b872c1c6 passes all nine native-update/UI/
checkpoint/Options consumers in58.54s after the separately owned ADS correction.
All actual Qt rows in those Windows consumers have zero failures/skips; the
full and focused artifacts are respectively647 and28 guest/local SHA-256
verified files. These checks accept the shared source/moc/build owners and
robust Options fixture. They do not declare a fresh global Windows gate passing;
final whole-platform and rendered UX acceptance retain their P3 owners.

Reviewed every app/test source consumer and its late source additions/clones,
compile definitions, include paths, generated moc/resources, platform link
closure and startup/environment registration before sharing the production
objects. The source bodies and language boundaries remain authoritative at their
existing paths. No new upstream algorithm, borrowed file, dependency revision,
notice or baseline change; original per-module records and pinned source
https://github.com/ip7z/7zip.git at
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17 remain unchanged.

The late CMake helper snapshots existing consumers and shares only complete
cohesive groups: ArchiveBackend, compression UI, MainWindow/command/drag/menu
integration, native menu presentation, original CompressDialog, Agent/AgentProxy,
PanelSelect, SplitDialog, PanelSplitFile, FileTransferOperation, FilesystemScanner
and VolumeOperations. A Windows-only thirteenth owner shares qtguimain.cpp for
the app and native MAPI host; their previous definitions were identical. A
partial source group fails configuration. Tests keep their own TU definitions
and resource/platform libraries. QObject headers move with their sole moc owner;
production source and generated metaobjects are linked into every direct consumer.

macOS full format/all-target gate passes 238/238 in 103.48s; its compile database
has 133 unique root production TUs, each compiled once. Linux current full passes
234/234 in 194.36s with 131 root TUs compiled once. The prior Windows full
a5cc8449 builds all targets and passes 241/244; its three fixture failures are
separately repaired. It reveals the final repeated GUI entry, now shared in
01389c3d's compile database: 1007 commands, 133 unique root production TUs,
no duplicates and 47 C entries. Both 01389c3d and isolated 6812d916 pass format
and build all 239 executables; CTest passes 243/244 in 1041.47s and 1066.95s.
Only list_settings_runtime times out. Its focused nine-suite run 7c962557
passes in 61.70s with no skips, but does not close the full-suite failure.
Diagnostic full 9db15dd5 passes 243/244 in 1095.84s. Its stderr proves that the
context menu opens, is inspected and returns normally; the remaining timeout
occurs at the second Options invocation. A fixture helper now waits for the
owned dialog to be visible in its modal loop before accepting it, rather than
using an unconditional singleShot. The inspected
[Qt 6.8.3 QDialog::exec body](https://github.com/qt/qtbase/blob/v6.8.3/src/widgets/dialogs/qdialog.cpp#L506)
calls show before installing its event loop, so acceptance during native show
can precede that loop; this is the fixture-race hypothesis, not a proven product
failure. macOS focused list_settings_runtime
passes 1/1 in 6.42s, and the subsequent full gate passes 238/238 in 99.72s.
Fresh isolated Windows B full 676ebd54 passes 243/244 in 1081.11s and repeats
the second Options timeout: the visible/modal-loop helper does not resolve it.
All 647 artifacts are SHA-256 verified locally and on the guest. Diagnostic
382fc123 localizes the wait inside the second Options constructor; fd0f406d
reaches all other pages and stalls constructing SystemPage. An unrelated native
association-revision defect is reproduced and repaired below. Final combined
Windows B/U/W gate b7579d33 passes formatting but fails before CTest: a resource
update lambda mixed the original RINOK int result and Windows HRESULT returns.
Its declared return type now preserves the existing HRESULT transport contract.
Repeat 4b4d50c2 passes formatting/all-target build and 241/245 in 897.62s;
external_codecs exceeds its wait, source_metadata_storage refuses ADS,
alt_streams_archive_ui times out and large_list_window exceeds its projection
limit. The final compile database has 1012 commands and 134 root production TUs,
each compiled once. Actual native-update73/UI24/checkpoint8/list5/association8
Qt rows all pass with zero skips. All 647 artifacts are SHA-256 verified on the
guest and locally. An unchanged desktop retry of external_codecs/large_list_window
passes 2/2 in 124.91s at their original limits, with separately preserved evidence.
Fresh native-update validation b872c1c6 resolves the ADS consumers below.
The full gate history remains distinct from the accepted build-owner scope.
Engine C sources and required module-specific variants retain their compiler,
definitions and paths; root Qt glue remains C++17, Apple native bridges
Objective-C++17. No source renaming, body reordering or UI geometry change.

### Windows association revision prerequisite (verified, 2026-10-05)

Reviewed original RegistryAssociations.cpp CShellExtInfo::ReadFromRegistry and
AddShellExtensionInfo/DeleteShellExtensionInfo, SystemPage.cpp OnInit/AddIcon
and retained FilePlugins.cpp CExtDatabase::Read/IFolderManager callers at
https://github.com/ip7z/7zip.git, commit
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. The original reader clears ProgramKey
for an absent association and reads only the extension and its DefaultIcon.
Existing copied/adapted scope, dependencies, notices and baseline remain unchanged.

The Qt Windows platform's added external-change fingerprint previously passed
an empty ProgramKey to QSettings::beginGroup, so allKeys recursively included
the entire Software/Classes root. An unrelated key could then invalidate an
absent association, and System initialization repeatedly scanned that root.
A two-line guard omits that empty path; actual extension, nonempty ProgID,
7-Zip registration and UserChoice paths keep their existing fingerprint rules.
No upstream equivalent provides the added external-change fingerprint itself;
the original association reader/writer algorithms remain authoritative.

A public native Read regression uses a unique absent extension and an unrelated
private current-user Classes key, removing only its fixture key on completion.
Windows d0448530 reproduces differing revisions: seven Qt rows pass, the new row
fails, zero skips. The earlier 5e272186 run had omitted the new slot through a
moc conditional and is explicitly not regression proof. The slot is now always
registered; its body documents a Windows-specific skip on other platforms.
Corrected native desktop b9119814 passes association_services and
list_settings_runtime in 10.10s, respectively eight and five Qt rows with zero
skips. Its trace proves the second System page completes and Options returns.
Six artifacts are SHA-256 verified locally and on the guest. Production traces
existed only in the isolated diagnostic checkout; the final main snapshot has
none. The later 4b4d50c2 full gate confirms list5/association8 with zero skips;
four other suites fail as recorded above, so global Windows acceptance is open.

### P2-11.5 — native update transport (accepted, 2026-10-05)

Final macOS formatting/all-target/full gate passes240/240 in102.40s. Fresh
Windows b872c1c6 and Linux b762e6b7 build and pass all nine direct suites in
58.54s and19.43s. Actual nativePath10/codec73/UI24/checkpoint8/list5/link12/
sourceUpdate76 Qt rows pass with zero skips on both guests. Windows additionally
passes metadata12/ADSUI5 without skips; Linux ADSUI5 passes and metadata has only
its ten explicit Windows-only skips. Their28/24 artifacts are guest/local
SHA-256 verified. The earlier full Linux235 gate and Windows full/all-target
history remain recorded separately; fresh global Windows/Linux acceptance is P3.
This resolves registry U, including its surfaced ADS guard defect, and accepts
the source/native-writer component rather than the still-open P2-11 inventory.

Reviewed original AgentOut.cpp::CommonUpdate/DoOperation/DeleteItems/RenameItem,
ArchiveFolderOut.cpp::CommonUpdateOperation, UpdateCallback.cpp property/stream/
crypto/raw callbacks, OpenArchive.cpp::CArc::GetItem_Path and
PanelOperations.cpp::CThreadFolderOperations::DoOperation, plus their retained
callers and interfaces. Source repository https://github.com/ip7z/7zip.git,
commit 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Existing copies and dependency
baselines remain at that revision. The original Agent::SetProperties body and
Agent.h ISetProperties interface are restored from that same pinned source;
no new source module or update algorithm is introduced.

The resource uses the retained handler's IOutArchive and original update callback
when its metadata indexes are proven. The backend owns writer calls and path
validation; the original work-dir publisher, reader reopening and callback
interfaces retain their lifetimes. Only E_NOINTERFACE permits the writer-query
fallback; a native writer error/cancellation returns directly. CArc::GetItem_Path
supplies original missing-path/default-name rules. Exact names replace NFC
identity assumptions; repeated names cannot prove a mapping and disable update
readiness. Changed properties keep the existing backend path policy, including
explicit absolute-path mode, before the handler receives them.

PanelOperations keeps its original worker dispatch/Create/return sequence and
now attaches the existing folder progress service for all operation types;
comment retains its native-only service. The resource checks original Sync before
and after update, before publication, including zero-item writers without progress
callbacks. No new pause/stop algorithm. The property facade now forwards native
ISetProperties through ArchiveBackend, advertising it only when the actual
handler supports it and the native index mapping is proven. Original Agent
vectors remain the sole owner of explicit properties; a resource flag makes
them take precedence over optional Qt CInfo projection, resets on native reopen
and on the source empty-property call. CommonUpdate keeps its no-reset behavior
and DoOperation keeps its explicit/reset calls. Restored COM interfaces require
existing owners to retain IFolderArcProps instead of an ambiguous IUnknown cast.
The property-array allocation uses RAII so native setter failure also releases
it before the original early return. Native single-archive SFX uses original
CArchiveLink::ReOpen/CArc::ReOpen to establish ArcStreamOffset and retain the
physical InStream after a handler initially scans its own prefix. ConfigureArc
passes that source stream/offset to the original ArchiveFolderOut publisher's
CopyStream_ExactSize/CTailOutStream branch; no prefix algorithm is recreated.
Process-only offsets, nested chains and volume sets remain guarded. Six public
SFX file/plan success/failure/cancel rows first reproduce refusal and then prove
byte-identical prefix retention and unchanged archives on failure/cancellation.
The current full macOS gate passes 238/238 in 99.72s and Linux 84047a43 passes
234/234 in 194.28s, each with native update 73 and UI24 Qt rows, zero skips.
Linux's 626 artifacts are SHA-256 verified locally and on the guest. Windows
runtime proof remains pending; this record does
not accept registry U or parent P2-11.

Public regression evidence: native writer selection RED38; disabling only the
path guard reproduces 18 unsafe Rename/CreateFolder publications; restoring NFC
comparison reproduces the Unicode mapping failure; native ZIP duplicate-name
fixture reproduces the ambiguous mapping failure. Current codec_update has 66
passing rows before the property bridge, including seven writer formats, failure/cancel, Unicode/spaces,
source-plan/file entrypoints and paused zero-item deletion. codec_commands_ui
passes 24 rows using source Sync for active Copy cancellation and source Delete
confirmation for instant zero-item handlers. macOS format/all-target/full gate
238/238 passes in 104.18s with both suites at zero skips. Linux full 4fad7f5c
passes format/all-target build and 234/234 tests in 190.01s; both suites also
have zero skips, and 626 artifacts are SHA-256 verified locally and on the guest.
The added public source-properties regression checks native stored/compressed
ZIP bytes, repeated updates/reset, invalid-property refusal and no external
mutation. Disabling only explicit-property precedence reproduces its failure;
the corrected focused suite passes 1/1 in 2.27s with 67 Qt rows, zero skips.
The properties bridge full gates pass on macOS 238/238 in 100.67s and Linux
c043b35c 234/234 in 184.59s, both with codec67/UI24 zero skips; the latter's
626 artifacts are verified locally and on the guest. The subsequent SFX scope
has the macOS/Linux gates above; Windows runtime proof remains pending.
The existing focused native-update suite grew past the file-size review trigger
to cover this same writer/callback/index contract; a separate target is deferred
until a further responsibility is introduced. Tests use private fixtures and the official
writer; no real user archive, native policy change or upstream write.

Windows 4b4d50c2 exposes four native WIM Add failures before the expected
success/failure/cancel callback: the new ordinary-file path guard rejected the
NTFS stream colon. Reviewed original UpdateCallback.cpp::GetProperty's explicit
kpidIsAltStream, OpenArchive.cpp::FindAltStreamColon_in_Path/CArc::GetItem and
ExtractingFilePath.cpp::Correct_AltStream_Name, their types and callers at the
same pinned revision. Validation now uses the original colon finder and stream
correction separately from the ordinary base-path policy, only for an explicitly
marked stream. The original optional :$DATA suffix is accepted. Unmarked paths
retain ordinary validation; stream flags with invalid types, malformed names,
empty names or unsafe base paths are refused before UpdateItems. No decoder or
new stream-name algorithm is added.
The separate public native_update_path suite keeps this validation contract out
of the existing writer lifecycle suite. With the previous validator, five of
eight malicious marked-stream paths reach the real ZIP writer on macOS; the
test verifies refusal before any output bytes and unchanged archive input.
Restoring the source-based guard gives ten Qt passes including init/cleanup.
Final affected native_update_path/codec_update/source_metadata_storage/
alt_streams_archive_ui passes 4/4 in 3.28s on macOS: respectively10/73/2/5
passes, with ten explicit Windows-only metadata skips. Full macOS and fresh
Windows/Linux nine-consumer validation now passes as recorded above. The earlier
RED and focused checkpoints remain historical evidence of the final correction.

### P2-11.6 — shared source worker checkpoint (accepted, 2026-10-05)

Final macOS formatting/all-target/full gate passes240/240 in102.40s, Linux
d20a669f full235/235 in190.58s, and Windows 4b4d50c2 builds all targets and
passes all eight checkpoint Qt rows. Fresh b872c1c6 Windows and b762e6b7 Linux
each pass all nine direct suites in58.54s/19.43s, including the final concurrent
publication row, with checkpoint8 and sourceUpdate76 at zero failures/skips.
The full Linux626/Windows647 and focused Windows28/Linux24 artifacts are
guest/local SHA-256 verified. This accepts registry W; the four unrelated full
Windows failures/resolution retain their records above, and whole-platform P3
acceptance remains separate. No UI layout change or upstream revision change.

Reviewed original ProgressDialog2.cpp/.h CProgressSync constructor, CheckStop,
Set_Paused/Set_Stopped, source dialog callbacks and native worker callers,
Windows/Synchronization.h CAutoResetEvent and C/Threads.c event/critical-section
bodies. Source repository https://github.com/ip7z/7zip.git, full commit
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Existing dependency revisions,
source notices and C/C++ compilation languages remain unchanged.

The four hash, creation-lifecycle, volume-output and copy-preparation adapters
now delegate their checkpoint state to original CProgressSync through the narrow
QtWorkerProgress owner. Their duplicated Qt pause/stop flags and wait loops are
removed. Original CProgressSync keeps its flag precedence, lock and loop. An
optional original CAutoResetEvent wakes a single worker immediately; configuration
precedes worker start. Native dialog callbacks retain the original polling mode
because they may have multiple concurrent callers. Event initialization errors
return through HRESULT. The unchanged C event implementation remains compiled as C.

ProgressDialog2 retains its source path and bodies. CMake compiles its Sync and
presentation portions separately, with one authoritative object per function:
sevenzip_progress_sync depends only on Qt Core/common, and the panel presentation
depends on that library. This resolves the filesystem-to-panel cycle without
moving or cloning source functions. The adapter's publication mutex preserves
copy preparation's atomic cancellation/publication ordering; it owns no second
pause/stop state. Worker phase algorithms, result snapshots and joined lifetime
remain with their existing owners.

Public Qt Core tests cover stop precedence, clean next owner, pause/resume,
paused cancellation, signals preceding waits and refusal to publish after cancel.
The focused five-suite gate passes in 3.28s. Unsandboxed macOS formatting,
all-target build and full CTest pass 239/239 in 99.65s. Linux 4a585a31 passes
formatting, all-target build and 235/235 in 187.87s; its 626 artifacts are
SHA-256 verified locally and on the guest. Each platform has seven checkpoint
Qt rows with zero skips and all direct consumers passing. Windows runtime
checks remain pending; registry W and parent P2-11 remain open. No UI layout change.
The added public concurrent-publication row verifies that Cancel waits for an
already entered publication action and stops subsequent checkpoints. The final
macOS gate passes 239/239 in 99.26s, with eight checkpoint Qt rows and the
explicit Windows-only association skip. This extends the earlier seven-row
platform evidence. Final Linux d20a669f passes formatting, all-target build and
235/235 in 190.58s, with eight checkpoint Qt rows, codec73/UI24/list5 and only
the explicit Windows-only association skip. All 626 artifacts are SHA-256
verified locally and on the guest. The subsequent Windows runtime and final
Linux direct-consumer checks pass as recorded at this section's acceptance.

### P2-11.7 — filesystem transaction boundary (accepted, 2026-10-05)

Reviewed original CPP/7zip/UI/FileManager/FSFolderCopy.cpp CopyFolder,
CopyFile_Ask and CCopyStateIO, FSFolder.cpp CFsFolderStat::Enumerate, and
CPP/Windows/FileDir.cpp RemoveDirWithSubItems/RemoveDirAlways_if_Empty, including
their callers and dependencies, at https://github.com/ip7z/7zip.git, full commit
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. The existing adapted FSFolderCopy
remains authoritative for copy recursion, overwrite callbacks and progress.
No borrowed body, notice, dependency revision or baseline is changed here.

The original immediate per-entry Move workflow has no whole-batch staging,
publication and rollback equivalent. Its statistics traversal ignores enumeration
errors and lacks the required strict source-type preflight. Recursive directory
deletion would delete skipped merge sources; the single-directory empty remover
does not supply the postorder written-source ledger cleanup. These concrete
differences justify retaining the existing narrow Qt policies rather than
replacing them with algorithms having different safety semantics.

QtFileTransferTransaction owns the moved filesystem
transaction bodies, with named target-preflight and completion/rollback phases.
FileTransferOperation retains QThread lifetime, immutable input, cancellation,
overwrite waiting and public signals. One stack transaction references that
worker through a private friendship; it does not duplicate mutable operation
state. Synchronous source callbacks remain within its lifetime. The per-item
stage/publish loop remains together to preserve its early exits and rollback
ordering; its size is a reviewed exception, not a copied new workflow.
All 16 original source consumers register the new implementation/header pair;
the shared production FileTransfer owner now includes that pair, so the moved
transaction has one authoritative object across app/window/direct consumers.
After phase extraction, file_transfer/file_transfer_direct/language_file_transfer
build and pass 3/3 in 0.22s on macOS. Full isolated validation and integration
with the shared production object owner, Windows/Linux and window consumers
remain pending. The isolated full Mac gate passes 237/238 in 101.95s; only
shell_update_window fails because this checkout has no official SFX module.
Preparing it from the pinned read-only source, then repeating that consumer,
sfx_creation/sfx_lifecycle and the three direct transfer suites passes 6/6 in
8.40s without source changes. FileTransfer19/Direct6/Language12/FSFolderCopy11
Qt rows passed in the full run. The extraction is now integrated on the main
branch after the separately accepted B/U/W commits. Main file_transfer,
file_transfer_direct, language_file_transfer, ui_copy_same and
worker_window_progress pass 5/5 in 2.33s. The integrated unsandboxed macOS
formatting/all-target build/full gate passes 240/240 in 100.34s; its LastTest
evidence is retained separately from subsequent focused runs. Fresh full
snapshot f9eb439e passes Linux formatting/all-target build and 236/236 in
189.27s; 626 guest/local artifacts are SHA256 verified. Mac135 and Linux133
root translation units each compile once, including the moved transaction.
Actual FileTransfer19/Direct6/Language12/FSFolderCopy11 and MainWindowOperations34
Qt rows pass without skips on both platforms. Windows f9eb439e passes formatting
and all-target build with 135 root TUs compiled once. All affected suites pass:
FileTransfer18/Direct6/Language12/FSFolderCopy11 and MainWindowOperations34 have
no failures/skips; the Windows-only source metadata12/ADSUI5 also pass.
The full run finishes 245/246 in 997.00s: archive_external_edit alone fails its
editor-yes prompt wait. Repeating that suite on the unchanged snapshot and desktop
session passes 1/1 in 10.80s, with all 15 Qt rows and no skips. Original 646 and
separate retry 647 guest/local artifacts are SHA256 verified; the initial failure
is not overwritten or described as a clean full Windows gate. No code or timeout
change was made for the retry. This accepts F's verified boundary and consumers;
whole-inventory acceptance and a stable final platform gate remain P2-11/P3.

### P2-11.8 — archive transfer rollback boundary (accepted, 2026-10-05)

The final window-file review found a destination-archive snapshot and manual
1MiB restore loop in MainWindow::transferBetweenArchivePanels. Inspected original
CPP/7zip/UI/FileManager/App.cpp CApp::OnCopy staged CopyTo/CopyFrom callers,
adapted CApp::CopyArchiveToArchive and source DeleteItems, and original
CPP/7zip/Compress/CopyCoder.cpp CCopyCoder::Code/CopyStream plus Common/FileStreams
and Windows/FileIO dependencies, at https://github.com/ip7z/7zip.git, full commit
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. The original staged panel workflow has
no destination snapshot restored after a later source Delete refusal; this
existing safety adaptation remains necessary. Its byte loop has a usable source
implementation and does not belong in a widget.

In an isolated checkout based on f6e409e, ArchiveBackend provides a leased
QtArchiveTransferBackup. The window requests the snapshot and restoration result;
it retains source command invocation, confirmations, errors and panel refresh.
QFile::copy remains the existing filename/permission service for the snapshot,
and QSaveFile retains atomic publication. The restore byte loop is replaced by
original CInFileStream and NCompress::CopyStream with a narrow sequential Qt
output adapter. No archive format, update algorithm or borrowed body is changed.
The shared backend object owner registers the implementation for all consumers.

Focused archive_transfer_backup/ui_operations pass 2/2 in 7.84s on macOS,
including empty/large Unicode destination bytes, absent-input refusal,
publication refusal preserving a replacement directory, and the existing real
two-panel archive Copy/Move with Delete Cancel rollback. The isolated unsandboxed
formatting/all-target build/full Mac gate passes 241/241 in 99.89s. Its LastTest
log is saved before the subsequent test-only registry isolation edit; the affected
backup test passes again in 0.38s, with six Qt rows and no skips. Fresh Linux
snapshot 56e65b26 passes the two direct/window suites in 8.71s; four guest/local
artifacts are SHA256 verified. Windows snapshot 364aed17 passes the same two
suites in 31.51s after the previous desktop gate and unchanged retry completed.
Both guest runs execute MainWindowOperations34 and ArchiveTransferBackup6 Qt
rows without failures/skips, with four SHA256-verified guest/local artifacts each.
These isolated snapshots are based on f6e409e; they include R and exclude F's
behavior-preserving extraction. R is integrated after the separate F acceptance
commit cd90da6, with byte-identical source/test files and both shared object
memberships retained. Main focused checks pass 2/2 in 8.23s, then the unsandboxed
formatting/all-target build/full Mac gate passes 241/241 in 103.66s, with 136 root
TUs each compiled once. Its full LastTest log is retained separately. This accepts
R's boundary and consumers; final integrated platform and inventory acceptance
remain P2-11/P3. No UI layout or text is changed.

### P2-11.9 — source compression persistence and memory guard (accepted, 2026-10-05)

Reviewed original CPP/7zip/UI/GUI/CompressDialog.cpp/.h
FindRegistryFormat_Always, SaveOptionsInMem, OnOK, GetDictSpec/GetOrderSpec/
GetNumThreadsSpec/GetBlockSizeSpec and their FormatChanged/SFX/OnOK callers;
original ZipRegistry CInfo/CFormatOptions, source memory calculation and
SetErrorMessage_MemUsage dependencies, at https://github.com/ip7z/7zip.git,
full commit 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Current Qt constructor,
format/method/control-change callers and registry/control ownership were inspected.

The source CompressDialog module now owns FindRegistryFormat_Always and the
SaveOptionsInMem body, including the original 32-bit dictionary sentinel/large-value
marker. Qt reads existing controls into the original NCompressDialog::CInfo spec
snapshot and passes the previous format, block log and memory string; native
GetComboValue(control,1) all-ones specs replace Qt's Auto=0 at the control boundary.
It deliberately does not reuse compressionOptions during format notifications,
because the visible format has already changed while the stored profile is old.
The existing missing-Params constructor guard and unconstructed encryption value
are preserved; original unused DictionaryChain branches remain omitted.

The original OnOK memory refusal is an overload returning its source message
instead of calling a Win32 message box. Existing native calculations/CMemUse/system
fields stay authoritative. The previous Qt adaptation for unknown RAM with no
explicit memory limit is retained; Win32 GetItemText fallback remains replaced by
the existing resource-language lookup. Qt presents the unchanged error text.
No DTO field, archive writer, format algorithm or layout is added. Borrowed paths,
notices and source baseline remain intact; these newly adapted source bodies are
recorded here without adding another borrowed file.

Main focused preferences/dictionary/limit/source-dialog pass 4/4 in 2.24s;
resources pass separately 1/1 in 1.14s. The unsandboxed formatting/all-target
build/full Mac gate passes 241/241 in 101.86s, with its LastTest log preserved.
Frozen snapshot 611e4f7c is based on integrated 5fbe9ba plus the four exact S
source files, including the accepted F/R/B/U/W boundaries. Linux passes all 13
compression/source/SFX/window suites in 19.61s; Windows passes them in 72.18s.
Actual Qt rows include preferences11, dictionary7, limit10, resources16,
source-dialog13 and shell-update43, all without failures/skips on all three OS.
Both guest runs preserve 86 SHA256-verified local/guest artifacts, including 82
current widget JSON reports. These checks accept S's source ownership and direct
consumers; entire root inventory and final platform/UX acceptance remain open.

## P2-10.4 — source worker progress snapshot (accepted, 2026-10-05)

Reviewed original CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CCopyStateIO::MyCopyFile,
PanelCopy.cpp worker/CopyTo callers, ExtractCallback.cpp::CExtractCallbackImp::
SetCompleted, PanelSplitFile.cpp::CThreadSplit/Combine::ProcessVirt and
ProgressDialog2.cpp::CProgressSync/UpdateStatInfo/OnTimer. Repository
https://github.com/ip7z/7zip.git at0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17;
current MainWindow transfer/split wiring, worker callbacks, result handlers and
quiesce/close ownership were inspected. No borrowed body or dependency changes.

The former Qt connections queued each native chunk/path to MainWindow before
updating source CProgressSync. Public64MiB Copy/Split/Combine fixtures fail all
three rows: after joining the worker without GUI pumping, the actual registered
source timer shows0 processed bytes instead of65536KB. The adapter now connects
directly to the existing source mutex snapshot, capturing only that stable
native state and a worker-local byte accumulator. It never accesses widgets or
mutable window state on the worker. Terminal handlers explicitly join before
closing the progress dialog; the existing close path disconnects/cancels/joins
before release. Public worker signals/checkpoints remain unchanged, including
direct cancellation subscribers. No new progress implementation or native copy/
split/combine algorithm. The fixture also counts drained MainWindow MetaCalls,
checks streamingSHA256 bytes and cleanup. Three additional public close-during-
callback rows verify cancellation, joined worker/dialog destruction and draining
late events without published output. Focused macOS regression passes all8 Qt
cases; the unsandboxed full gate passes formatting, all targets and238/238 tests
in106.24s. Snapshot20261004T215459Z-d5e791ae passes the eight direct consumers
on Windows11 x64 in54.79s and Linux x64 in15.63s. Retrieved actual LastTest logs
verify zero failed/skipped cases, including all8 worker-window cases,34 operation,
21 volume-workflow and26 volume-operation cases on both platforms. File-transfer
has18 Windows/19 Linux cases because its POSIX-link case is not compiled on
Windows. No UI layout/text/resource change or new borrowed module.

This accepts P2-10.4 and the integrated P2-10 parent: preparation/native-worker
delivery (.1), full large-list projection/navigation (.2), volume cancellation/
restart (.3) and filesystem/split consumers (.4) are all verified on three OS.
Their source baselines remain409 exact/195 adapted/3 local. Global rendered UX,
release checks and final acceptance retain their P3 owners; P2-11 is next.

## P2-10.3 — bounded volume progress (accepted, 2026-10-05)

Reviewed original full CPP/7zip/Compress/CopyCoder.cpp::CCopyCoder::Code/CopyStream,
CPP/7zip/UI/Common/Update.cpp::Compress volume/SFX output branch and
CPP/7zip/UI/FileManager/ProgressDialog2.cpp::CProgressSync/UpdateStatInfo/OnTimer,
plus current QtArchiveVolumeOutput, backend create/finish/cancelAndWait and
creation-deletion consumers. Source repository https://github.com/ip7z/7zip.git,
commit0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Existing CopyCoder128KiB reads,
progress every4MiB, CMultiOutStream output and Update/SFX bodies are unchanged;
no new borrowed dependency or revision. Their full provenance remains in P1-06.

The external CLI volume worker lacked source-dialog timer sampling and queued
every callback. Its public128MiB regression first fails with32 queued callbacks
after200ms occupied GUI, while25 other Qt cases pass. The initial elapsed-only
ceiling did not expose this backlog, so the test drains the backend's MetaCall
events separately. The narrow adapter now uses the existing mutex snapshot/33ms
GUI timer from P2-10.1; native checkpoints, exclusive stage publication, rollback,
source bytes and algorithm remain unchanged. A separate volume generation guards
queued finished signals and the worker start after its initial public progress
signal. Joining stops snapshot delivery before releasing the worker or emitting
the terminal result. Fixtures also cover immediate restart after paused joined
cancel, exact extracted bytes by streamingSHA256, no post-terminal progress,
occupied outputs, EN/RU, SFX and the creation-deletion consumer. Focused macOS
consumers pass4/4 in5.31s; unsandboxed full gate passes formatting, all targets
and237/237 in115.75s. Baseline verifies409 exact/195 adapted/3 local unchanged.
Snapshota887b250 worker scope passes Windows10/10 in41.53s and Linux10/10
in12.32s; retrieved actual LastTest has no skipped/failed Qt cases, including
all26 volume cases. Snapshot8377ef3c direct consumers pass Linux5/5 in7.95s.
Windows passes four consumers, but the fifth creation-deletion suite fails only
three native symlink setup fixtures under the Limited desktop token. That suite
already passes all24 cases using the privileged worker token in a887b250.
The script's desktop `volumes` scope therefore keeps the four creation/SFX/UI
consumers; native-link deletion stays in `workers`, without changing product,
fixture behavior, privileges or OS policy. Final desktop1779c543 passes Windows
4/4 in44.58s and Linux4/4 in6.97s. Retrieved actual LastTest verifies all26 volume,
24 SFX creation,10 SFX lifecycle and11 UI volume cases on both OS, with no skips.
This accepts the integrated P2-10.3 component on all three tested platforms.
Parent acceptance additionally requires P2-10.4's source progress snapshot for
filesystem Copy/Move and Split/Combine callers; this child does not accept it.

## P2-10.2 — full Qt list projection (accepted, 2026-10-05)

Reviewed original FileManager/FSFolder.cpp::LoadItems/LoadSubItems/SetFlatMode,
PanelItems.cpp::RefreshListCtrl/SaveSelectedState, PanelFolderChange.cpp::
SetNewFolder/ReleaseFolder and PanelSort.cpp::CompareItemIndices, with the current
normal/flat filesystem, archive, refresh, selection and two-panel callers.
Repository https://github.com/ip7z/7zip.git, full commit
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17; existing authoritative adapted bodies
and dependencies are reused without new borrowed modules or revisions.

QtFolderProjection is new scheduling glue, because the original Windows
ListView refresh does not provide Qt event-loop row publication. It retains
IFolderFolder and real indices, bounds actual archive row/property/icon creation
to 32 rows/8ms per turn and invalidates generations on navigation/close. Normal
filesystem loading above 256 items reuses FilesystemFolderLoader with the source
flat flag disabled; a bounded source callback probe preserves synchronous small
folder consumers. No enumeration, proxy, property, sorting or selected-state
algorithm is recreated. Qt's sort enabling already performs a sort, so final
publication configures the native order before enabling it once. The original
CPanel comparator remains authoritative. Selection restore and post-operation
path selection are delayed until completion, including inactive-panel restart.
The original RefreshListCtrl disables redraw from deletion through sorting and
selection. Archive publication retains that policy through Qt setUpdatesEnabled;
filesystem redraw stays incremental because suppressing it causes a603ms final
layout pause in the local fixture. An idle archive flat-toggle regression exposes
retained guards capturing synchronous disabled flags; the narrow adapter now
releases its synchronous guards before acquiring asynchronous scan guards.

Public MainWindow fixtures first fail for all four 4096-row disk/archive
normal/compact states: the first heartbeat occurs only after all rows exist.
Current fixtures require an intermediate heartbeat, full completion under 15s
and a maximum GUI gap below 250ms, with current-build widget JSON. Consumers
cover refresh selection, panel switches, create-file selection, navigation,
close and obsolete turns. The macOS full gate passes formatting, all targets
and 237/237 tests in104.17s after fixing the CMake `bookmarks`/`bookmark_tests`
startup-isolation mapping. Later localized fixture checks pass navigation/nested,
auto-refresh and native-window waits. Snapshot60d25b11 worker scope passes
Windows9/9 in25.94s and Linux9/9 in9.31s with no skips. Snapshot3a07b009 GUI
scope passes Linux11/11 in38.25s, with no skips and all four normal/compact
geometry reports fully visible. Windows passes7/11 suites in261.47s: remaining
path assertions, panel focus/hover and archive projection timing fail. Additional
comparison-only fixes and focus diagnostics precede fresh snapshot1be288cd.
After archive redraw adaptation the local four GUI suites pass17.19s and a full
macOS gate passes237/237 in101.12s. That snapshot precedes the newly failing
flat-toggle regression. After the guard-order correction, an aggregate run exposes
timing interference from parallel CTest processes at final4096-row publication;
the timing suite now runs serially, preserving both original ceilings. Final
macOS formatting/all-target/full gate passes237/237 in84.31s, including14
large-list cases with no skips. Filesystem normal/compact505/421ms, gaps84/74ms;
archive125/123ms, gaps31/33ms. Current four geometry reports are fully visible.
Baseline verification passes409 exact/195 adapted/3 local paths. Snapshot1be288cd
Linux passes11/11 in37.00s without skips, filesystem1060/977ms and archive609/506ms,
all gaps below100ms and geometry reviewed. Its Windows build is pending; this
snapshot precedes the final guard-order correction and serial timing setting.
Windows1be288cd passes9/11 suites in228.85s: archive normal/compact9373/9820ms
with174/132ms gaps now pass, filesystem normal26712ms/641ms still fails while
compact11779ms/205ms passes. All four geometry reports are fully visible, no
skips. Panel keys fail only mixed-panel focus and hover; popup waits pass.
The mixed-panel failure is reproduced locally by synchronous focus movement
during QStackedWidget switching. Reviewed original Panel.cpp::SetListViewMode
and its menu/keyboard callers: it changes the same control's style. The Qt
adapter now captures the native panel/controls and restores list focus across
the replacement. A separate public cancelled-scan panel-switch regression first
leaves Refresh disabled; returning operation state to the ready panel fixes it.
Original FSFolder.cpp::Init/WasChanged and PanelItems.cpp::OnTimer retain native
Windows change notifications. Normal-folder projection no longer duplicates
these with per-file Qt signature reads; root recovery and flat-descendant fallback
remain. Auto-refresh tests additionally change an existing file's bytes/size.
Three affected local GUI suites pass25.22s; fresh full/platform checks pending.
The subsequent unsandboxed macOS gate passes formatting, all targets and237/237
tests in103.03s. Its final filesystem468/389ms, gaps62/65ms, archive122/123ms,
gaps29/25ms benchmarks and all four normal/compact geometry reports pass.
Snapshot9a80cdce Linux passes11/11 in38.39s with no skips: filesystem1015/982ms,
gaps89/90ms, archive503/501ms, gaps55/53ms. Actual LastTest and current geometry
confirm the new consumers and fully visible controls. Windows9a80cdce passes10/11
suites in254.27s with no skips. Its complete large-list suite passes: filesystem
normal/compact12879/11351ms, gaps208/191ms; archive9767/9870ms, gaps134/136ms.
Actual LastTest and all eight geometry/benchmark JSON files were retrieved;
all four list/address/status geometries are fully visible. Only panel-key focus
and hover cases still fail in that snapshot.

Separate diagnostic executable copies reproduce both failures without altering
the snapshot or its evidence. Focus enters the inactive address combo during
Open, when native guards intentionally suppress panel activation, and remains
there after completion. Reviewed original ProgressDialog2.cpp::Create and
Windows/Control/Dialog.cpp::CModalDialog::Create: native progress uses a modal
DialogBoxParam. The Qt adapter now disables address combos alongside the lists
during Open/Operation. A public regression first fails before this correction,
then verifies both combos disabled during backend work, enabled afterwards and
the disk panel's command policy after focus restoration; compact state JSON
records disabled/ready controls. The hover adapter itself receives no MouseMove
in the failing fixture. The installed Qt6.8.3 qtestmouse.h confirms QWidget's
no-button helper only moves QCursor; the fixture now uses QWindow's QPA-event
overload with mapped coordinates. The diagnostic copy then passes both cases,
including the real400ms hover timer. Fresh snapshotb19c3aa2 and a new full
macOS gate are pending; diagnostic success does not accept the parent.
The subsequent macOS gate passes formatting, all targets and237/237 tests
in101.96s. Snapshotb19c3aa2 Linux passes11/11 GUI suites in41.55s, with zero
failed/skipped Qt cases in the actual LastTest. Its filesystem1039/984ms,
gaps91/91ms, archive515/496ms, gaps72/56ms and four current-build list geometries
pass. Both compact address-state reports show fully visible disabled controls
during Open and enabled controls after completion. The same final snapshot's
Windows Limited native-desktop run passes11/11 in212.64s. Retrieved actual
LastTest confirms all55 panel-key and14 large-list Qt cases pass, with no skips
in any suite. Filesystem normal/compact12896/11533ms, gaps207/194ms;
archive9332/9379ms, gaps132/125ms meet the original15s/250ms ceilings.
All four list/address/status geometries and both compact disabled/ready address
states are fully visible. The final macOS benchmarks are filesystem593/542ms,
gaps65/108ms, archive124/123ms, gaps28/28ms. This accepts P2-10.2 across the
three tested OS, with baseline409 exact/195 adapted/3 local unchanged.
Rendered Windows pixel parity and global UX acceptance remain in P3.

Read-only follow-up review discovers CreateVolumes still queues each original
CopyCoder progress callback (every4MiB) and lacks a generation guard around its
joined completion. Bounded delivery and restart proof for that consumer are
recorded separately as P2-10.3 before accepting the parent P2-10.

The earlier22e6cbca Windows snapshot exposes Unix-only path assertions, the
Limited interactive token's native symlink restriction and native focus/hover
fixture failures. Path assertions now normalize only at the comparison boundary;
worker and GUI verification use their corresponding Windows logon scopes.
Native desktop suites serialize focus/popups. Windows large-list timing fails
in that first aggregate run; an isolated normal disk case passes13441ms/225ms.
The original15s/250ms per-projection ceilings remain unchanged. Pending native
GUI/focus/performance failures above are historical and superseded by the final
acceptance. The parent remains open for P2-10.3.

## P2-10.1 — archive preparation and bounded worker transport (accepted, 2026-10-04)

Inspected original full FSFolderCopy.cpp::CCopyStateIO::MyCopyFile, FSFolder.h::
CCopyStateIO, source PanelCopy worker/callers, ProgressDialog2.h/.cpp::CProgressSync
and CProgressDialog::UpdateStatInfo, plus current backend start/cancel/join and
source mutation/plan/compatibility consumers. Source repository is
https://github.com/ip7z/7zip.git at0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
The existing authoritative adapted FSFolderCopy body is reused unchanged:64KiB
read/write loop, native metadata, callback cancellation and source errors.
No new borrowed module or dependency revision is introduced.

QtArchiveCopyPreparation is the narrow asynchronous transport missing from
upstream's Windows worker integration. It owns private peer staging and calls
that original body; ArchiveBackend::CopyPrepare retains continuation/state until
join and exclusive publication. Four former GUI QFile copy sites are removed
from source mutation/plan and compatibility mutations. Engine command policy,
source stage order and module boundaries remain unchanged. Existing source copy,
QtOwnedTempDir/registry and filesystem services are its dependencies. Windows
thread/window transport is replaced by QThread/mutex/timer; no native copy
algorithm or archive format is reimplemented. Exotic filesystem rename fallbacks
are not verified; collision refusal is a public fixture.

External hash and creation scan/deletion transport now uses one mutex snapshot
and33ms GUI timer instead of a queued call for each native callback. Source
CProgressSync uses the same snapshot/timer pattern, but is coupled to the dialog
owner and does not implement this external-backend signal transport. Native
HashCalc/EnumDirItems/DeleteInputs algorithms, cancellation and retained output
are reused unchanged. Operation/phase generations protect posts and queued
completion. Filesystem loader count is atomic and scanner-polled; per-file
source cancellation remains intact. No new functional scan implementation exists.

Original AgentProxy.cpp::Load/CalculateSizes/AddRealIndices and Agent.cpp::
LoadFolder/LoadItems/GetRealIndices were reviewed for large/deep fixtures.
CProxyArc2 uses the retained explicit traversal stack; path-mode CProxyArc
reduces excessive depth through its original1024-level policy. No proxy or
selection algorithm was changed. Public32768-node parent-chain and4096-level
path fixtures exercise native owners/indices/flat selection and destructor scope.
First platform snapshot f31a2c45 passes Linux8/8 but Windows7/8: compatibility
mutation fixtures assumed Unix separators, and the implicit-folder Rename
comparison missed native Windows descendants. Inspected original
Archive/Common/ItemNameUtils.cpp::ReplaceSlashes_OsToUnix and AgentOut.cpp::
CAgent::RenameItem/GetItem_Path2 callers. Mutation comparison paths now call that
existing source normalization through a narrow QString bridge; raw listing
metadata is unchanged. Fixtures compare OS-normalized paths and still verify
extracted bytes. The first full macOS gate passes235/236, failing only the
language fixture's obsolete synchronous empty-plan expectation; it now awaits
the terminal signal. Updated nine direct macOS suites pass3.92s. Final full
unsandboxed macOS gate passes formatting, all targets and236/236 tests in95.26s;
snapshot90ef1e7f passes Windows9/9 in42.83s and Linux9/9 in9.38s. Downloaded actual
Qt rows have no failures or skips in any of these nine suites. Baseline remains
409 exact/195 adapted/3 local verified paths. Reproducible native fixture timings
and thresholds are recorded in [testing](testing.md). This iteration
does not accept P2-10.2 Qt row/icon/navigation benchmarks or parent P2-10.

## P1-12.2 — six-page Options integration accepted (2026-10-04)

Prerequisites P1-13/P2-09 are accepted in commit6864399. Inspected full original
OptionsDialog.cpp::OptionsDialog and its Windows/Control/PropertyPage.cpp::
MyPropertySheet caller dependency at https://github.com/ip7z/7zip.git,
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17, plus all current Qt container Apply/
accept/reject/changed-state consumers. Both native character variants start on
page0; the Qt container incorrectly selected Settings4. A public regression
fails before the fix (actual4/expected0); the narrow presentation adaptation
now initially selects System, preserving page IDs and source order. No new
functional page algorithm or copied dependency is introduced.

New focused options_integration drives all six pages through the public controls
and Apply/OK/Cancel/restart. It checks discarded versus committed menu/editor/
work-folder/list/language values, explicit System association calls using an
isolated injected service, notification/callback counts and unrelated settings.
Ctrl+Tab/Shift+Ctrl+Tab preserves pending input, Qt focuses its tab bar, and the
Tab cycle reaches the visible page. This uses normal Qt keyboard services without a custom
focus-restoration algorithm. Existing options_window verifies actual refresh of
both panels while preserving their selection; source invalid memory input,
association failures, Help and page geometry remain in direct-consumer suites.

macOS focused8/8 passes in2.63s. Twenty-four current EN/RU normal/compact page
reports reviewed: all visible named controls fully exposed; page order, groups,
input rows and persistent footer preserved. Required unsandboxed macOS full gate
passes formatting/all-build and234/234 CTest in96.26s. Snapshot4b9f3d65 Linux
passes8/8 in2.91s; Windows passes6/8 in34.43s, failing the new single-Tab focus
expectation and existing external_editors_ui F3 helper-output expectation.
Qt's Windows focus cycle can visit the footer before the page; the fixture now
checks a bounded visible Tab cycle reaching the active page, preserving pending
input throughout. Snapshot71c7a6c5 then passes7/8 in34.65s, with only the existing
F3 helper-output expectation failing. The external-view fixture now waits for
the active MainWindow and View action readiness, asserts the QAction's F3 signal,
then observes the real helper arguments. Final Windows59cf7420 passes8/8 in21.05s
with this fixture; actual F3/F4 consumers pass without a production process-launch
change. These results supersede both failing intermediate snapshots. Final
macOS localized fixture checks pass2/2 in1.01s after the passing full gate;
Linux8/8 above verifies the same production change and predates only those
localized test corrections.

This accepts P1-12.2 and P1-12 together with previously accepted source page and
working-directory consumers. Current Windows, Linux and macOS geometry reports
cover72 EN/RU normal/compact page states, all visible named controls fully
exposed. Temp-directory failure/cleanup, invalid memory and association
configuration, source Apply order and both-panel refresh remain covered by direct
consumers; native Menu registration is separately verified by P2-09.3. Baseline
verifies409 exact/195 adapted/3 local paths. Windows rendered pixel correspondence
and the global UX audit remain open. Next mandatory available component is P2-10.

## P2-09.3 — Menu integration accepted (2026-10-04)

P2-09.2 is committed as329e54f. Current work restores native MenuPage module
names7-zip.dll/7-zip32.dll/7-zip64.dll and enables already-ported command
policies. Original MenuPage.cpp::OnInit/OnApply/CShellDll, its callers in
OptionsDialog.cpp and complete RegistryContextMenu.cpp::CheckContextMenuHandler/
SetContextMenuHandler were inspected at repository https://github.com/ip7z/7zip.git,
revision0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Their source state, list,
apply/requery order, flags and registration algorithm remain authoritative.
QtNativeMenuPageServices supplies only module/file/registry/error transport.
Unix now supplies the applicable in-app policy capability while native DLL/
registration remains unavailable; no independent Unix shell workflow is added.

The production-control regression first failed on disabled policy controls and
the absent Unix platform service. Corrected macOS focused menu_page/menu_policy/
shell_context/shell_menu_window passes4/4 in0.85s. menu_policy uses the actual
default page, Apply/Cancel, real Unicode/spaced file selection and original
QueryContextMenu/InvokeCommand: four cascade/icon combinations preserve source
flags, selected paths and ElimDup arguments. This is not yet whole-step acceptance.

New Windows native_menu_integration reserves only absent project-owned class/
handler keys in RUNNER-WINDOWS, records owner PID and drives the production
Menu page. It checks source registration/requery, actual CoCreateInstance,
real Shell IDataObject and OS context-handler discovery, then unregistration
and marker-scoped cleanup. Its CTest registration is limited to that disposable
guest; ordinary Windows builds do not write the user's registry through this
fixture. Windows607eb66a passes6/6 in9.26s, including that actual OS discovery
and cleanup (native3 Qt rows, zero failures/skips). Linux passes5/5 in0.99s.
Windows first5be5025a fails on a fixture FString/char overload;3765f473 reaches
the native fixture but fails on an SDK RegDeleteTreeW declaration. The fixture
now uses original-style FString += and a runtime API lookup. Both failures
precede fixture execution and are not registry/runtime evidence.

The corrected required unsandboxed macOS gate passes formatting/all-build and
233/233 CTest in95.44s; its first run failed only the obsolete language tooltip
expectation after controls became available. Twelve current-build EN/RU Menu
normal/compact language JSON across macOS/Windows/Linux are reviewed: order,
exposure and policy enabled states hold; Windows discovers the native DLL and
disables the missing other-bitness DLL, Unix disables native integration/Zone.
Expanded21a2a80a fails compilation because the standalone upstream flags header
requires MyTypes before its inclusion. The fixture now includes that dependency
explicitly. Snapshot72c8cb47 fails the expanded runtime expectation (34 commands
instead of one): the fixture passed a Unicode settings path through the ANSI CRT
environment setter. Snapshot15358740 supplies the child process environment
through QProcessEnvironment and passes6/6: actual registered DLL produces only
Extract Here, a flat native menu and its source bitmap; OS discovery and cleanup
also pass (native3 Qt rows, zero failures/skips).

A separate disk fixture exposes a real persistence defect: QSettings writes INI
booleans as true/false, while the adapter accepted only DWORD text. ZipRegistry.cpp
now shares one narrow bool-value transport adapter accepting both representations.
Original CContextMenuInfo::Load/Save and Windows/Registry.cpp::SetValue(bool)/
GetValue_bool_IfOk at the pinned revision were inspected; defined/default policy
and callers in compression/extraction remain unchanged. No upstream Qt INI
transport exists. The new context_menu_settings disk regression fails before
the fix and passes afterwards; six direct-consumer suites pass in0.75s on macOS.
The final required unsandboxed macOS gate passes formatting/all-build and233/233
CTest in96.35s; baseline verifies409 exact/195 adapted/3 local paths. Final
snapshot1f968069 passes Windows7/7 in9.68s and Linux6/6 in1.10s, including the
fresh-disk bool regression and actual native registration/policy consumer.
All Windows suites report zero Qt failures/skips; native registration has3 rows.

This accepts P2-09.3, P1-13.2 and the P1-13/P2-09 parents together with their
previously accepted components. Installed native launch, extraction/refusal/
Cancel, MAPI and Properties evidence remains in P2-09.2; Edit launch/validation/
Cancel remains in P1-08/P1-13.1. No extension was registered on the developer's
machine. Windows-only COM/registry branches are unavailable on Unix by source
platform guards; in-app command policies are verified on all three systems.
The full six-page Options acceptance is the next P1-12.2 result. Broad three-OS
release gates, global UX audit and final acceptance remain P3 scope; rendered
Menu-page pixel correspondence to original Windows is unverified.

## P2-09.2 — native Windows component accepted (2026-10-04)

This integrated step accepts the retained native COM/DLL, GUI/map sender,
MAPI, Properties, startup and installed-package consumers described below.
It supersedes the intermediate WIP outcomes in this section. Explorer handler
registration and Menu policy acceptance belong to P2-09.3, not this step.

Final System subset1c7b58c0 passes all three suites: native_menu_presentation
14 Qt rows, native_system_menu5, shell_menu_window21; zero failures/skips.
The adapter forwards IContextMenu2/3 initialization, measure/draw and printable
WM_MENUCHAR messages, including callback bitmaps and owner-drawn submenus.
Provider/HMENU ownership outlives Qt rows and filters. Real GDI fixtures cover
opaque, premultiplied and legacy zero-alpha pixels, transparent edges and both
DIB orientations. GetDIBits requests a top-down owned buffer: direct bmBits
interpretation had inverted the rendered Share icon and is rejected by the
multicolor orientation regression. Upstream contains no usable Qt conversion
or widget renderer; native query, IDs and invocation bodies remain unchanged.

Rendered comparison0a46f9dd passes1/1 CTest (10.69s), three Qt rows, zero skips.
It runs original7zFM and compares22 runtime System rows against both the raw
source query and Qt projection, including order, state, grouping and icon
presence. Both affected PNGs were inspected. Reference96DPI menu font is Segoe
UI height-12/weight400; Qt uses Segoe UI9pt under QWindows11Style, ratio1.
Qt style supplies24px rows, padding and rounded frame; native rows are22px
with9px separators. This records a Qt presentation adaptation, not an exact
pixel match or verification of other DPI/settings/platforms.

The reference is built from repository https://github.com/ip7z/7zip.git at
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17 using the original FileManager makefile
and resources in a separate archive snapshot. The wrapper includes the full
FM.cpp body with an entry-name alias. Only REG_PATH_7Z in RegistryUtils.cpp
and REG_PATH_FM in ViewSettings.cpp substitute a unique private product path;
original function bodies and shell providers' HKCU access are retained.
The manifest records original/adapted source, wrapper and binary hashes.
Creation refuses an existing key, normal source cleanup removes it, and
emergency cleanup requires the matching owner PID. No global HKCU override,
official registration change, Qt addition to upstream or sibling edit occurs.
See [reference reproduction](windows-ui-reference.md#pinned-runtime-system-menu-reference).

Final required local unsandboxed gate passes formatting/all-target build and
232/232 CTest in95.15s. Subsequent localized Windows presentation changes pass
their three direct consumers and the actual rendered reference; the affected
macOS shell_menu_window consumer passes1/1 in0.95s. Linux full evidence below
predates those Windows-only changes. Whole-platform final acceptance remains P3.

The following dated checkpoints preserve the failures and narrower checks that
led to this accepted component; pending labels there are historical.

Latest installed acceptance subset: Windows interactive023fc5e5 passes all three
package suites in32.83s. The actual installed DLL's reserved COM class is activated
through CoCreateInstance, then the source map sender launches installed7zG:
exact extraction, visible Cancel255 and damaged-archive refusal2 all pass
(5 Qt rows,0 failures,0 skips). Installed7zFM startup has11 passing Qt rows,
including its two actual process cases; the verifier's five cases pass.
The fixture now identifies7zG by executable identity within its own job: source
codec discovery can also launch7z.exe, so an assumed job count of exactly two
was incorrect. Damaged archives use the source progress error list, not an
assumed separate error popup. The private key and this snapshot's processes
are absent after the check. This does not register Explorer handlers or accept
P2-09.3 Menu policy. Linux full6e4afbef passes formatting/all-build and228/228
CTest in162.39s; this snapshot predates the new Windows menu presentation.

System presentation adapter WIP: original PanelMenu.cpp::CreateSystemMenu,
its callers and Windows/Menu.h retain PIDL/query/invocation policy. Their
native HMENU is normally rendered by Windows; no upstream Qt bitmap or
owner-draw widget implementation exists. Local QtNativeShellMenuPresentation
therefore owns only that presentation boundary and the provider lifetime.
It copies ordinary native item/checkmark HBITMAP pixels with Qt's Win32 image
adapter and forwards lazy submenu initialization. Interactive3ab31803 passes
native_menu_presentation/native_system_menu/shell_menu_window (3/3; respectively
3/5/21 Qt rows,0 failures,0 skips). The bitmap fixture verifies actual GDI pixel
values, checked/disabled/default state, source data/order, lazy dispatch and
provider/menu/pixel lifetime. Owner-draw message forwarding is now being added
and is not covered by that snapshot. Rendered Windows reference comparison and
whole P2-09.2 acceptance remain required.

Owner-draw subset d65a3df1 passes3/3 suites (6/5/21 Qt rows, zero failures/skips):
both IContextMenu2 and IContextMenu3 receive real WM_MEASUREITEM/WM_DRAWITEM
structures with source ID/itemData/state, draw into native GDI, and remain alive
until Qt widgets/menu message filters are destroyed. Mouse and Return dispatch
are exercised; failed measurement leaves that unrenderable command disabled.
Later df4350ba passes the WM_MENUCHAR bridge after b2f2a4c9 exposed Return being
forwarded as a character: the adapter now forwards only printable characters,
honors native MNC_SELECT/EXECUTE/CLOSE, and leaves control keys to Qt.
Snapshot75ab1f20 passes3/3 with callback bitmaps and owner-drawn submenus,
including preserved Qt text, four icon modes, lazy source child dispatch and
disabled-parent availability. Subsequent bitmap-alpha and reference checks
are pending; these earlier snapshots do not verify those changes.

The API boundary follows Microsoft's
[MENUITEMINFO contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-menuiteminfow):
HBMMENU_CALLBACK requires native measure/draw messages, distinct from a real
HBITMAP and from an owner-drawn whole row. Qt's
[Win32 image adapter](https://doc.qt.io/qt-6.8/qimage.html#fromHBITMAP) leaves alpha
interpretation to the caller. Native premultiplied DIB icons and legacy GDI
zero-alpha pixels now have separate regressions under validation.

Rendered-reference work uses the exact pinned original FM makefile in a separate
git-archive snapshot. tests/fixtures/native_reference_entry.cpp includes the
unchanged FM.original.cpp body with only its entry name aliased, then calls it
inside a private HKCU mapping on RUNNER-WINDOWS. Original resources, handlers,
types and native widgets are compiled by upstream nmake; no Qt is added to that
snapshot or the sibling checkout. scripts/build-windows-reference.ps1 records
repository, full revision, original entry/adapter/binary hashes and this entry
adaptation. The fixture refuses an existing private key, records its owner PID,
restores HKCU and uses original CKey::RecurseDeleteKey for cleanup. The test has
marker-scoped emergency cleanup; it does not alter official product registration.

Native reference compilation passes in89097891; its consumer initially fails to
compile on LONG-to-QJsonValue ambiguity, missing revision definition and a
Windows SDK minimum-version declaration. Those fixture issues are corrected.
The runtime1c6a4bed actually opens original7zFM File/System, captures its menu
at96DPI and exits normally. Its comparison fails on22 original versus21 Qt rows;
it is evidence for an actual rendered reference, not parity acceptance. The
Qt fixture now provides a valid source panel owner HWND and records both raw
queried rows and the Qt projection before comparing, to locate the difference.
Whole-step acceptance stays open while this result is investigated.

Current native-launch validation (2026-10-04): interactive snapshot84fbf652
executes the visible SHA256 dialog (independent digest through UI Automation)
and both normal/total Benchmark Close/join cases successfully. Its overall GUI
suite fails because the original CompressFiles waitFinish/map sender times out;
11 Qt rows pass,1 fails,0 skip. Owned-helper diagnostics are being added before
accepting that suite. This result does not supersede the requirement for a
complete passing GUI consumer check.

The new native_shell_launch fixture uses a host without Qt to load the actual
DLL, initialize IShellExtInit from a real Unicode/spaced Shell IDataObject,
query the original menu and invoke its Unicode Extract verbs. The original
ContextMenu::InvokeCommandCommon, NativeContextMenuServices and CompressCall
remain authoritative (same pinned revision below); no production command body
changes. The host releases COM/unloads the DLL after the original map/event
acknowledgement and retains only its own helper in a kill-on-close job.
Snapshot7e98e3c9 passes the three prior shell/COM/parser suites and the visible
Extract-dialog Cancel row with exit255. The success row extracts exact bytes
but incorrectly waits for a window after the short helper has already exited;
the test now accepts its signaled process handle and still requires exit0 and
unchanged archive/source/sentinel. Corrected acceptance is pending. No shell
registration or installed-package acceptance is claimed.

The subsequent local unsandboxed gate passes formatting/all-target build but
fails1/231 CTest (94.93s): codec_commands_ui WIM Cancel stops via a1ms timer
after the update has committed. Unchanged isolated rerun passes in3.81s,
demonstrating timing sensitivity rather than establishing full-gate acceptance.
The fixture now stops on the progress Show event before source OnInit releases
callback waiters; focused codec_commands_ui passes in4.22s. The source algorithm
and publication rules are unchanged; a new full gate is pending.

The corrected unsandboxed macOS gate5686 passes format/all-target build and
231/231 CTest in96.66s. Interactive c86e58b3 passes the whole native_gui_workflow
suite in19.38s, including original sender ZIP bytes, Test/Extract/refusal/Cancel,
SHA256 accessibility/digest and normal/total Benchmark Close/join. The Add
timeout did not recur; scoped owned-helper diagnostics remain in the fixture.
Interactive50281778 passes four shell/COM/parser/launch suites in7.17s; the
native launch subset at that snapshot contains success and visible Cancel only.
The expanded damaged-archive snapshot00b73cec stalls during transport after
source.tar transfer, before extraction or scheduled task creation. Its confirmed
owned local SSH child is stopped, producing exit255 and a missing-check.log
record; it is not application-test evidence.

Windows installation now has local CMake integration in
cmake/WindowsInstall.cmake and VerifyWindowsRuntime.cmake.in. The official
installer C/Util/7zipInstall/7zipInstall.c at the pinned revision was inspected:
WinMain/Install consume an embedded self-extracting archive via SzArEx_Extract,
and WriteCLSID/WriteShellEx write the official product registration. They do
not provide a Qt/CMake dependency deployment contract. These bodies are not
reimplemented or modified: CMake installs our already-built entry aliases/DLL
and the unchanged official engine artifacts, then calls Qt's own generated
deployment script. This new build integration checks runtime producer,
repository/revision/platform/architecture and every engine binary's recorded
SHA256 before copying app files. Runtime provenance and upstream license ship
under share; app/helper/shell/engine binaries share bin. Shell registration and
associations are not install side effects. Full distribution/licenses/SFX and
three-platform package acceptance remain P3-04.

Five windows_package_verification cases independently exercise that actual
pre-install script on Unicode/spaced temporary paths, including provenance,
architecture, missing/modified binaries and absent checksum refusal. The local
unsandboxed gate25030 passes format/all-build and232/232 CTest in78.49s.
Later affected startup/package-verification checks pass2/2 in0.64s; subsequent
localized changes are Windows-only fixture/lifecycle checks.

Installed-consumer b36b07b5 installs the binaries and Qt/plugins successfully
but native_shell_launch times out120s after a Cancel row receives no helper
report. Its later a9209337 snapshot passes the verifier and file_manager_startup
(including actual installed7zFM success/refusal); the installed DLL suite still
times out120s after its success row observes no extracted bytes. Neither accepts
the installed shell scenario. Desktop-scoped UI Automation diagnostics are
being added; SSH session0 MainWindowTitle does not establish window visibility.
The private registration fixture uses only the reserved project CLSID under
HKCU, refuses an existing key, activates via real CoCreateInstance, checks the
fixture owner marker for emergency cleanup and removes its own mapping before
helper/dialog waits. Registration is guarded to the authorized runner-windows
guest; official CLSID/handler registration is untouched. After a9209337 the
project class key and that snapshot's processes are verified absent.

GUI/GUI.cpp now contains the complete original Main2 dispatch and WinMain
exception/exit-code bodies from the pinned revision below. Qt owns entry and
platform initialization; the dispatcher still reads native GetCommandLineW,
uses the exact parser, prepares source options/callbacks/censors, enumerates
archives and routes Benchmark/Extract/Update/Hash with original error policy.
A delayed service factory occupies the original codec-construction point:
Parse2 acknowledges the sender before backend codec discovery. The retained
catalog already performed source Load/AddHash, so those calls are not duplicated;
external providers come from the same backend catalog and retain their lease.
GUI calls existing QtShellArchiveServices; its cohesive Extract overload forwards
excluded formats/hash bundle to the same source ExtractGUI body. No archive
algorithm or command syntax is supplied by the entry glue. Full original
My_SetDefaultDllDirectories startup is retained via unchanged C/DllSecur.c/.h,
compiled as C, and unchanged Common/ExitCode.h supplies source exit values.

Windows app post-build creates7zG.exe and7zFM.exe aliases of the same application
image. main.cpp selects the GUI entry for7zG.exe and the retained File Manager
startup for Windows7zFM/regular launches. This is build integration, not
package/installed proof.
Native helper process testing sends malformed mapping through actual7zG.exe,
requires event acknowledgement and original exit7 before backend discovery.
Windows63acf889 passes three shell/COM/parser suites in3.87s, including that
process; it predates the restored C DLL-security call. Windows6f7d535c then builds
the C owner and passes all three suites in14.11s with that call enabled. Local
shared services2/2 pass0.67s; subsequent full
unsandboxed macOS format/build/229CTest passes96.16s. Baseline verification passes
406 exact/195 adapted/3 local paths. Complete native helper archive workflows,
failure/Cancel, MAPI, package/installed and System rendering remain mandatory.
Native batch opening now carries the original COpenOptions through
QtArchiveFolderManager, ArchiveBackend and NativeArchiveReader to the retained
ArchiveCodecCatalog's source CArchiveLink::Open3. The catalog binds its own
codec owner and clears the already-consumed Qt service pointer; excluded
formats, open properties and layered COpenType options remain source-owned.
Existing file/stream publication, callbacks and cancellation retain their
owners. The GUI passes its unchanged ArcType to the process listing transport,
including source type chains; no second type parser/serializer is introduced.
Public archive_open_options regressions cover allowed/excluded ZIP, Unicode
multi-volume zip.split, invalid RAR opening properties and cancellation with
unchanged input bytes. Restoring the old batch entry makes excluded ZIP extract
incorrectly; the fixed bridge passes. scripts/check-archive-open.sh passes all
five direct suites locally in1.44s and in snapshot16577146 on Windows in9.04s
and Linux in2.37s. These are component checks, not native helper success/Cancel,
installed-extension or final platform acceptance.
The subsequent unsandboxed macOS gate passes formatting, all-target build and
230/230 CTest in114.32s, including the new public open-options suite.

Windows interactive snapshota6f1abab passes native_gui_workflow in11.47s:
the unchanged native CompressFiles sender launches actual7zG.exe with its own
map/event selection and waitFinish. A disposable Unicode/spaced file produces
a ZIP that the official engine tests and extracts with exact input bytes.
The fixture host's private job bounds only its spawned helper on timeout;
normal source completion disarms that test-only cleanup. Launcher's original
S_OK does not stand in for archive success. Expanded GUI Test/Extract/refusal
and visible Add cancellation coverage is being checked separately; MAPI,
installed/package, native System rendering and full .2 acceptance remain open.

Expanded interactive snapshot3ef6c02c exposes a retained-entry policy gap:
7zG t -y waits for the final source message because the adapted progress
operation does not inherit g_DisableUserQuestions. The suite reports the
30-second process failure, then reaches its old60-second suite timeout.
Upstream ProgressDialog2 and ExtractCallback read that original global policy.
The Windows progress constructor now initializes its existing per-operation
flag from that source owner; GUI also forwards YesToAll to the callback's
memory-question flag. Existing explicit operation overrides remain available.
The narrowed shared progress/service/open consumers pass locally3/3 in1.24s;
interactive snapshotb1091585 passes the native workflow suite in6.87s after
the progress fix: source sender ZIP creation, GUI Test/Extract, excluded ZIP
handler, broken archive and actual visible Add-dialog Close returning255.
Inputs and existing output sentinel remain unchanged on refusal/Cancel.
The later callback memory-policy forwarding and its low-limit RAR process
regression pass in interactive snapshot773ce06f in5.93s together with all preceding
native GUI cases: its forced1MiB RAR limit refuses without a memory question,
payload publication or changed archive bytes. No orphan7zG process remained
after the failed suite. This completes the tested Add/Test/Extract/refusal/Add
Cancel process subset; source email, remaining shell consumers, package and
installed/rendered acceptance remain open.

Native email process coverage now runs the same RunSevenzipGUI/GUI.cpp owner
with private Mapi32.dll fixtures, using the installed Windows SDK26100 MAPI ABI.
The test host preloads an exact private module and verifies its basename,
resolved module path and marker before entering the dispatcher; failure stops
the fixture before any real mail provider can be invoked. No production
environment hook or replacement mail orchestration is introduced. The original
Update.cpp body remains the sole owner of Unicode-first/ANSI-fallback messages,
MAPI_DIALOG/MAPI_TO, full attachment paths, synchronous send, cwd restoration
and EMailRemoveAfter CTempDir lifetime. Provider cancellation retains the
original ignored-return policy, rather than turning it into archive failure.
Snapshot31eb934d passes five Windows email cases in6.21s: Unicode keep/remove,
provider Cancel/remove, ASCII ANSI fallback and missing-export refusal. Copied
attachments independently test/extract through the official engine; original
input bytes remain unchanged, temporary attachment directories disappear after
the call and the provider's cwd change is restored. The first f10a6ef0 build
failed on a fixture string concatenation and was corrected before this result.
The later unsandboxed macOS gate17800 passes format/all-build/230CTest in96.30s.
File Manager email availability is connected to the existing Windows Add/update
capabilities. Snapshotf9834a5a passes native_mapi and native_shell_email2/2 in11.47s,
with all five process and fourteen actual File-menu rows passing and zero skips.
The menu consumer verifies three verbs across EN/RU normal/compact sizes, real
attachment bytes, source selection clearing, visible Add Cancel and missing
MAPI-export refusal. The intentionally reverted availability snapshot8cc595f4
fails all fourteen menu rows; restoring the Windows cases makes them available.
Two later snapshots0f2ea417/b282d59f exposed a fixture defect: original CFmSettings
was saved without Load, leaving selection flags uninitialized. Calling its
original Load before Save fixes the fixture without changing product selection.
All twelve current UTF-8 widget JSON reports were reviewed: original three-row
order, Unicode filenames, enabled/fullyVisible rows,334×78 EN and373×78 RU.
Unix preserves its explicit MAPI refusal. Package/installed
and native System rendering remain required for .2 acceptance.

The same pinned FM.cpp now supplies the active WinMain2 command-line fragment,
WM_CREATE absolute-path/file detection and its archive-refusal message flow,
plus complete WinMain exception/exit policy. Per-launch QtFileManagerStartup
replaces g_MainPath/g_ArcFormat process globals; unchanged StringUtils.cpp/.h
and its unchanged StdAfx.h provide the exact quoted-string splitter. Commented
alternate parsers remain omitted. MainWindow feeds these options directly into
the retained CApp::Create/CreateOnePanel/CPanel::Create/BindToPath at construction,
rather than opening a second path after default panel creation. Qt awaits the
existing ArchiveBackend catalog discovery before an initial archive bind;
Windows defaults use original empty-path/saved-panel behavior. WM_CREATE refusal
travels as a typed construction failure to the Qt InitInstance boundary, where
the original error is presented and FALSE/0 is returned; E_ABORT stays silent.
Qt replaces the native event loop/message box and widget creation, and per-window
CApp destruction joins external workers instead of g_ExitEventLauncher. DLL
security initialization remains unchanged C. Existing link/list/language/window
owners retain their responsibilities; this does not claim a complete native
window-placement or installed-extension port. Mac startup/direct consumers pass
3/3 in0.94s; Linux37359fb6 passes3/3 in1.70s. Windows37359fb6 first fails compilation:
native WinMain cannot be overloaded, so its retained exception body is scoped
under NQtFileManager while retaining the symbol. Windows2ed23e18 then passes3/3
in12.00s, but its nine startup Qt rows prove only the seven in-process scenarios:
MOC omitted the two process slots guarded with _WIN32. Using Q_OS_WIN corrects
registration. Windows desktopb6182130 passes3/3 in13.38s with eleven startup rows,
zero failures/skips, including actual7zFM.exe Unicode ZIP success and forced-type
refusal, visible windows and normal0 exit after closing their own HWNDs. No process
from that snapshot remains. Restoring only the old empty-format argument in
MainWindow makes wrong-forced-type incorrectly open ZIP; the restored bridge
passes again locally1/1 in0.64s. Full unsandboxed macOS31835 passes format, all
build targets and231/231CTest in98.30s; the subsequent guard change affects only
Windows test registration. Baseline409 exact,195 adapted/3 local paths verifies
against the pinned source repository. Native DLL launch consumers, package,
installed extension and System rendering still prevent whole .2 acceptance.

The same full pinned revision supplies unchanged Common/CommandLineParser.cpp/.h,
Common/ListFileUtils.cpp/.h and UI/Common/ArchiveCommandLine.cpp/.h. Inspected
complete parser bodies, Parse1/Parse2 callers in GUI.cpp and CompressCall's
UTF-16 mapping/event sender before reuse. Windows compilation resolves existing
adapted FileIO/FileDir, EnumDirItems/Extract/Update/HashCalc headers through narrow
private include roots; these modules retain their single authoritative owners.
The complete borrowed POSIX branch is retained physically but is not compiled:
this native GUI consumer is Windows-only and the project already owns the Unix
GetModuleDirPrefix implementation. The source parser now owns g_LargePagesMode;
the temporary Qt benchmark definition is removed. Its original privilege,
wildcard, list-file, update-switch and map/event control flow remains unchanged.
Baseline verification passes403 exact/194 adapted/3 local paths. The subsequent
macOS full unsandboxed gate passes formatting, all-target build and229/229CTest
in93.16s; Windows-only parser runtime remains outside that result. Public parser
fixtures cover native quoted command lines and Unicode/spaced mapping selections
for Add/Extract/Test/Hash, malformed marker/size/terminator and unavailable map
refusal with the original sender acknowledgement. Windows565e901f passes all
three native shell/COM/parser suites; the final censor tree admits the mapped
paths and refuses another filename. Extract/Test/Hash consume pre-items during
Parse2, while Add retains them for its dialog; fixtures follow that public
contract. GUI dispatch/process roundtrip and email/package consumers remain
required. Windows13fe8ebd stopped before build due to a full guest disk; only
compiled outputs/CMake directories of this goal's terminal failed3ee10fb1 were
removed after identity/process checks, preserving sources, logs, CTest and
widget evidence. It freed17,988,820,992bytes without VM/VHDX/toolchain changes.

Expanded Windows desktop snapshot4b1e4d10 passes16/16 suites in53.64s, including
all native Properties rows, shell COM/data, language geometry and spool cleanup.
Its Linux build failed with ENOSPC before tests. Only generated build outputs
from the goal's earlier terminal failed e5549311 snapshot were removed after
checking that no process referenced it; sources, logs, CTest and widget evidence
were preserved. Retry4ee3d663 passes11 suites but cannot run alt_streams_command
because the script built it only on Windows. After including that target on all
platforms, Linuxfc5aa56a passes12/12 in4.04s. This is focused desktop/component
evidence, not full Windows/Linux or installed-extension acceptance.

The same pinned revision now supplies CZipContextMenu's original four COM bases
and Z7_COM_UNKNOWN_IMP_4_MT, complete LoadPaths/LoadItems/IExplorerCommand and
IEnumExplorerCommand bodies in ContextMenu.cpp. The only enumeration-body
adaptation is an explicit null HMENU cast for the shared presentation overload.
Exact MyCom2.h/MyExplorerCommand.h dependencies preserve SDK/fallback declarations.
The native default constructor owns a process service per object, including child
commands; in-app consumers retain their injected service. Original DLL count
increments/decrements remain, with the image's count defined once in the shared
ContextMenu owner so both app and DLL compile one class implementation. Bitmap
destruction uses the previously audited native shared owner rather than deleting
the typed tree tag.

DllExportsExplorer.cpp keeps complete class factory, CreateInstance/LockServer,
DllMain, DllCanUnloadNow/DllGetClassObject and register/unregister bodies. Include
lookup and the existing project CLSID/name are adapted; the official 7-Zip class
is not replaced. ClassDefs remains the one portable interface-GUID owner: the
shell INITGUID include follows shared declarations and defines only its CLSID.
The source NtCheck.h and Explorer.def are unchanged dependencies. The Windows
sevenzip_shell target links these source owners into7-zip.dll with original
bitmap and FM icon resources; full version/manifest/package integration remains
pending. Native MyMessages.cpp bodies are retained with include adaptations and
their question flag defined once there; its header is exact. Native transport
forwards original CompressCall entry points, and Open retains the original
quoted GetModuleDirPrefix/7zFM.exe/MyCreateProcess branch. No new command syntax
or archive algorithm is introduced.

Windows FmSettingsPath uses the fixed project directory under GenericConfigLocation
for both app and in-process DLL, preserving SEVENZIP_SETTINGS_PATH. The previous
AppConfigLocation depended on the host's application identity. Language lookup
likewise uses the restored containing-module directory for installed Lang files,
then the existing bundled fallback; Unix paths stay unchanged. COM consumers
cover public factory/aggregation/refcount/interface identity, original enum
Reset/Next/Skip/Clone policy, nested children surviving root release, actual icon
extraction and a native host executable with no Qt application or Qt linkage.
These are not registered/installed Explorer or GUI/map/email acceptance. Baseline
verification passes397 exact/194 adapted/3 local at the pinned revision. Local
full unsandboxed format/build/229CTest passes96.34s. Windowsfcf2fe1c builds the
real DLL and native host and passes both native_shell_data/native_shell_com
suites in1.79s. Factory identity/refcounts, legacy IDataObject/HMENU, nested
children after root release, module icon extraction, host-independent settings
and no-QCoreApplication host are therefore runtime-tested. Earlier snapshots
115ae28e/84da222d/7151778e/3ca1eb98 stop at include/GUID or fixture macro issues;
these are corrected. ccbdbcd7 passes the other consumers but exposes the fixture's
incorrect ExtractIconEx count: requesting large and small icons returns two
successful extracted icons, consistent with the
[Microsoft API contract](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-extracticonexw).
The corrected fcf2fe1c then reaches all nested-child assertions. This does not
prove registered Explorer, native GUI/map/email or full component acceptance.

Native HMENU presentation now consumes the same ContextMenu.cpp command tree
as the Qt menu. The Windows overload generates at relative position0 and inserts
at the shell's requested position; it does not duplicate command selection,
IDs, naming or invocation. Inspected original QueryContextMenu callers,
MyInsertMenu/MyAddSubMenu, bitmap loading/destruction and resource2.rc at the
pinned revision below, plus complete Windows/Menu.h/.cpp conversion and insertion
bodies. winshellmenupresentation maps the typed tree to the existing unchanged
NWindows::CMenu/CMenuItem/InsertItem and CMenuDestroyer, retaining source menu
handling rather than supplying a second Win32 implementation. Its tree traversal,
submenu ownership transfer and module-local LoadBitmap glue is needed because the
shared generator has typed presentation handles. The build-only resource2.rc
is derived from the single pinned original, retaining its string table and
MenuLogo.bmp ID and adapting only configured resource paths. A shared native
bitmap lifetime replaces the original raw HBITMAP owner;
the tree's unsigned bitmap tag is never passed to DeleteObject. Native consumer
fixtures compare the entire recursive HMENU with the source tree for cascade,
icon and1/20-item selections, preserve foreign entries and invoke the original
numeric Test verb. Windows103aa012 focused native_shell_data passes1/1 suite
in1.22s after building the authoritative source and generated RC. The preceding
fc83aa1a broad build stopped at a redundant test include before tests; that
unused ContextMenuFlags.h include is removed. Local shell_context passes1/1
in0.27s. The full unsandboxed local gate passes formatting/build and228/229
in94.15s; only upstream_inventory sees the transient duplicate RC owner. After
removing that owner, focused inventory passes1/1 in0.06s and baseline verification
passes392 exact/192 adapted/3 local. No current local failure remains, but this
is not a new complete229/229 run. COM/DLL consumers and rendered reference
acceptance remain required before accepting this component.

Same pinned0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17 from
https://github.com/ip7z/7zip.git. Inspected original PanelMenu.cpp complete
InvokeSystemCommand, Properties, CreateShellContextMenu and their IContextMenu,
IGetFolderArcProps/operated-selection callers; current FSFolder does not provide
IGetFolderArcProps. Copied the complete InvokeSystemCommand body into its
preserved module, changing only QWidget parent to native HWND. Properties()
restores the original early Windows shell branch, then reuses the existing
source collector and CListViewDialog Qt modal presentation. Archive metadata
enumeration remains one implementation. MainWindow synchronizes selection and
calls this source entry; the redundant PropertiesDialog production wrapper and
its CMake registrations are removed. Direct projection fixtures use public
collector calls, not that retired wrapper. Unix cannot use the Win32 COM shell
property provider and retains the existing source-based portable collector.

Local macOS arm64 direct consumers6/6 PASS3.33s after builds; previous affected
3/3 PASS3.01s and projection2/2 PASS1.06s. Windows native_properties_window uses
disposable uniquely named Unicode/spaced files/folders in eight flat/view modes,
real File/Info invocation, native property-sheet discovery/Close, unchanged bytes
and selection; missing file requires no portable fallback. Source filesystem
runtime assertions move to that native consumer on Windows, while archive
metadata/language coverage remains portable. Windows87f3aa71 stopped during
test compilation: language_ui lacked the test-support base needed by its new
archive fixture. That base is restored. The snapshot also predates the row
lookup correction: filesystem paths are in column1, not archive Qt::UserRole.
Windowsb171aa21 builds the affected targets and finishes4/7 PASS145.52s;
properties retained an obsolete filesystem Qt-modal timer and crashed after its
failed assertion, source_properties assumed Unix separators, and the new native
fixture missed sheets and flat asynchronous rows. The first two fixtures now use
the public collector and native-separator normalization. Native discovery now
uses the unique disposable filename across shell process/proxy-owner boundaries,
records diagnostic HWND metadata and waits for flat rows. This discovery change
is not a confirmed cause of the missing-sheet failures. Fresh Windows9a898937
contains those corrections and the native selection/verb additions below;
validation is running. No native/runtime acceptance or .2 commit.

Windows9a898937 subsequently builds and finishes6/8 PASS136.01s. Its native
selection/verb, source projection, runtime/language and ADS suites pass. Properties
has a second obsolete filesystem modal timer in its loaded-metadata fixture;
that fixture now calls the public collector twice against the same loaded source
folder. Native property sheets are positively identified in the shell host:
correct unique titles and #32770 class, but visible=0. Windows0296e0a7 still
observes hidden sheets with explicit SW_SHOWNORMAL; the proposed zero-nShow
cause is disproved. The fixture closes hidden sheets on failure as well.
Native visible-sheet acceptance remains open. No rendered pixel-parity claim is made.
COM/DLL/process/email/installation and System icons/owner-drawn remain required.
The source AltStreamsFolder interface table has no IGetFolderArcProps, and
InvokeSystemCommand excludes that provider. Windows therefore retains the
original no-op Properties branch for ADS instead of the former extra Qt dialog;
its existing command regression now checks this source refusal. That later
fixture change also postdates87f3aa71. Three additional local archive/ADS/language
consumers pass3/3 in1.12s before the assertion adjustment (ADS runtime unavailable
on macOS). Native Windows ADS validation remains pending.

The native selection dependency is now restored from the same pinned revision:
complete Windows/Shell.cpp and Shell.h bodies, including HDROP/IDLIST names,
FILE_ATTRIBUTES_ARRAY, CItemIDList lifetime and BrowseForFolder. Adaptations are
include lookup and Windows-only declarations around the shared CFileAttribs;
formatting is separate from functionality. Windows/MemoryGlobal.h/.cpp are exact
unchanged dependencies under third_party, used through their original headers.
Windows-only CMake registration uses Unicode and shell32. The added public
native_shell_data fixture covers Unicode/ANSI ordering, malformed DROPFILES/CIDA
fallback, attribute refusal/reset and an actual disposable shell IDataObject.
Its Windows compilation/runtime remains pending; source baseline verification
passes389 exact/192 adapted/3 local. Native COM Initialize consumers still need
to be connected; these dependencies do not establish extension acceptance.
The complete original ContextMenu::Initialize, native InvokeCommand and
GetCommandString/MyCopyString_isUnicode bodies are retained in ContextMenu.cpp,
under Windows guards, with declarations in its existing header. They reuse the
same command map and InvokeCommandCommon typed transport. Native selection/verb
coverage now drives a real shell IDataObject through Initialize and the shared
generator, validates ANSI/Unicode/truncated/unknown command strings and numeric
Explorer lpVerbW-null fallback, dispatches Test to a recording transport, then
checks null-data refusal/reset and unchanged bytes. The COM interface tables,
factory/refcounts and DLL consumer remain pending. Local affected4/4 PASS0.95s,
then Properties/projection/shell3/3 PASS1.06s on macOS arm64 after fixture fixes.
Full unsandboxed macOS arm64/Qt6.11.2 gate passes formatting, all-target build
and229/229 CTest in107.72s for this Properties/selection/verb snapshot. Windows
native compilation/runtime remains pending; this does not accept .2.

The same source revision now supplies complete Windows CompressCall.cpp launch
bodies in their original module: Call7zGui, CreateMap/CRandNameGenerator,
AddLagePagesSwitch, CompressFiles, ExtractGroupCommand, ExtractArchives,
TestArchives, CalcChecksum and Benchmark. The shared GetQuotedString remains
one implementation. Include lookup is adapted; ReadLockMemoryEnable uses the
existing FmSettingsPath adapter. ErrorMessage retains the shell DLL's original
null owner through an immutable local handle instead of adding the unused
mutable g_HWND global to in-app callers. Windows function signatures are restored
beside the existing typed in-process overloads. Original 7zG.exe name, native
CProcess command line, UTF-16 mapping, random names, event/process wait and
exception/error flow are retained. Unchanged Common/Random.h/.cpp and
Windows/FileMapping.h are vendored with Win-only Random compilation and Unicode
launcher registration. Baseline verification passes392 exact/192 adapted/3 local.
This launcher addition postdates the passing full macOS gate and Windows9a898937;
affected local shell_context passes1/1 in0.22s, but native compilation, process/map
round trip, GUI parser caller and actual installed DLL consumer remain pending.

The full Windows3ee10fb1 gate finishes190/225 PASS in1340.05s, with35 failed
suites; Windows full acceptance remains open. Direct consumer corrections include native-separator drag
assertions and QtArchiveFileResource::Spool's Windows sharing violation. Spool
now uses original FileDir::MyGetTempPath/CreateTempFile2 with the final native
COutFileStream handle, retaining CopyStream, registered ownership and publication.
This removes QTemporaryFile's retained handle and the second writable open.
POSIX uses the original COutFile::mode_for_Create=0600 to preserve private spool
permissions. An added public regression verifies failed/cancelled copy leaves no
temporary file/lease and keeps original bytes; its8MiB fixture reaches the source
copy coder's4MiB progress boundary. Existing publication success/failure/cancel
and private POSIX permissions remain checked. Local affected4/4 PASS0.89s before
the final permissions assertion. The latest full unsandboxed macOS arm64 gate
passes formatting, all-target build and229/229 tests in97.56s. Windows0296e0a7
builds the native launcher and finishes8/10 suites PASS in134.42s: its original
spool publication cases pass, while the two new progress cases predate the8MiB
fixture correction and native Properties still times out with hidden sheets.
Progress fitting failures require current-build Windows geometry; language_dialog
now writes reports before fitting assertions so failures retain that evidence.

Subsequent native diagnostic of one disposable Properties fixture observes both
the property sheet and its File Manager HWND hidden in the same process; the
sheet's owner is a hidden StubWindow32. This does not prove an asynchronous
foreign host. The fixture now requires qWaitForWindowExposed and native owner
visibility before invocation, preserving the visible-sheet assertion; snapshot
4e3eccce is checking this precondition. Its240s suite timeout accommodates all16
rows plus failure cleanup. No production force-show workaround is introduced.

Two timestamp fixtures previously left PROPVARIANT::wReserved3 uninitialized.
Inspected the pinned PropVariant.h setters, Get_Ns100 and the formatter caller;
fixtures now use SetAsTimeFrom_FT_Prec_Ns100 with all reserved fields set and
retain the nine-digit assertions. The file-column fixture flushes its bytes and
sets mtime through its writable handle, required by Windows attributes access.
Local formatting/property/column consumers3/3 PASS0.67s; Windows proof pending.

The pinned Windows/DLL.cpp::MyGetModuleFileName complete ANSI/Unicode path body
is restored, including MAX_PATH refusal and shared directory/fallback behavior.
Only upstream's external g_hInstance service changes: GetModuleHandleExW with
FROM_ADDRESS|UNCHANGED_REFCOUNT resolves this implementation's containing module,
so a shell DLL uses its own adjacent GUI/resources instead of the host EXE's.
This avoids adding a mutable per-consumer global; the returned handle is not
freed. The adapter follows [Microsoft's documented contract](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexw).
POSIX keeps the existing Qt executable-path adapter. A Windows-only real DLL
fixture links the authoritative module and is copied to a disposable Unicode/
spaced path; native_library checks its own DLL path, the host EXE path and unload.
Local native_library1/1 PASS0.67s; new Windows DLL runtime proof remains pending.

Windows4e3eccce finishes8/11 suites PASS112.47s. Every native Properties row
fails its exposed-owner precondition before invocation; explicit Normal process
startup also fails that precondition. This isolates a runner/native-window issue
for further diagnosis, without accepting product visibility. Its new8MiB spool
failure/cancel cases pass, but publication assertions expose native versus Qt
separator mismatch when matching the owned record. Spool now normalizes only the
native-created QString path at its Qt boundary, preserving source file handles.
Progress JSON confirms Windows offscreen fallback metrics (Elapsed time width
hint156px versus74px control). progress_dialog/language_dialog join the existing
native Windows UI/font tests; no source OnSize geometry rewrite is made.
Fresh Windows0bfa77d0 checks the latest DLL, timestamp, spool and font changes.

The subsequent local full unsandboxed gate passes formatting/all-target build,
but finishes228/229 PASS94.85s with archive_external_lifetime timeout. Focused
rerun confirms two lifetime assertions fail; owned_temp_publication passes.
The local whole-component gate remains open until that consumer is resolved.

That lifetime failure was reproduced with an orphaned owned fixture left after
CTest timeout: a live --archive-editor process from this test retained its
control/temporary path. The preserved source same-name discovery correctly
waited for it. Signalling only that fixture's .exit control ends it; the suite
then passes1/1 in3.09s and three repeats in9.35s without source changes. The
latest full unsandboxed macOS arm64 gate passes formatting/all-target build and
229/229 in94.21s, including the CLI inspection's explicit -sccUTF-8.

Windows0bfa77d0 finishes13/15 suites PASS110.92s. Native DLL path, timestamp/
mtime, native-font Progress and selection/verb consumers pass. Spool publication
and owned records now pass; the remaining success assertion reads CLI output
without its UTF-8 switch and now uses the same -sccUTF-8 as ArchiveBackend.
Native Properties fails before invocation because SSH has no input desktop.
Read-only session inspection finds the active runner console in session1 and
SSH processes in session0. A disposable scheduled task in that existing logon
passes the actual exposed-owner/visible-sheet file consumer (3 Qt cases including
setup/cleanup,1.404s), and the task is removed. Full16-row proof remains pending.
The project runner now has --interactive for Windows/all, preserving the exact
snapshot check.cmd/toolchain while using an InteractiveToken task for the current
runner user; completed tasks are removed and session/exit evidence is recorded.
It has no credential/login or hidden-desktop fallback. Offline16 runner checks
pass, including option boundaries and Linux/Windows routing/failure records.
Fresh d12dce13 is checking all15 consumers through this reproducible desktop path.

Windowsd12dce13 subsequently finishes14/15 suites PASS44.43s through the
interactive path. All16 native Properties flat/view/file/folder rows and missing
refusal pass with exposed owner, visible native tabs, closure, unchanged bytes/
selection and no archive extraction. Only the pre-UTF8-switch publication
assertion fails; its production publication/ownership and other8 cases pass.
Current-build EN/RU Progress JSON at640×380 and760×450 is reviewed: native label
widths155/195px, Elapsed hints70/51px and Ratio100/90px, height16px, fullyVisible
and labelFits true. This confirms the font-engine diagnosis without changing
source OnSize. The completed unique scheduled task is confirmed absent. Native
System icons/owner-drawn/rendered comparison and COM/DLL/GUI/email/package/
installed consumers still prevent whole .2 acceptance.

## P2-09.1 — File Manager consumers acceptance (2026-10-04)

Accepted scope is the integrated in-app component: retained Explorer/PanelMenu
command generation/invocation, popup/program-menu lifetime, native Add/update/
SFX/hash/extraction/testing, original external Open/F3/F4 and basic Windows
System query/invoke. Source module/dependency provenance and exact adaptations
are recorded in the P2-09 entries below; the baseline remains
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17 from https://github.com/ip7z/7zip.git.
Independent picker/File/text-viewer workflows have been retired with their
consumers. Native extension/process/email/Properties/rendering and final Menu
integration remain mandatory P2-09.2/.3; this does not accept parent P2-09 or
the global UX audit. Full Windows validation remains open, with its corrected
test harness checked locally and its latest immutable runner still building.

Continuation uses the existing source port below, without repeating its source
bodies. Remaining P2-09 scope is grouped into three integrated results in the
plan/checklist; acceptance follows the source/consumer checks, not that
reorganization. Full local unsandboxed
`scripts/check.sh` passes formatting, all-target build and229/229 CTest in95.26s,
macOS arm64 Qt6.11.2. Seven focused Open/F3/F4/menu consumers pass13.74s.
Current EN/RU File menus980/640 have28 source rows, Exit last, no clipped rows;
their shell children have5 rows with no clipping. Source Add760×720 and640×480
top/bottom reports retain compact scrolling and exposed footer.

The subsequent `archive_item_open` failure fixture writes partial output before
the transport fails. It verifies no external launch, removal of temporary
output and unchanged disposable archive. The source progress error window stays
open for acknowledgement; the fixture must close its final Close state after
the final-message transition, as existing external-editor regressions do.
Its initial timeout was a missing test responder, not a production correction.
After adding that responder and the original IDS_CLOSE resource dependency,
the focused regression passes1/1 in0.60s, unsandboxed macOS. This test-only
addition postdates the passing full gate; application sources are unchanged.
The focused runner script now builds/runs archive_item_open on both guests.
Fresh snapshot2a6fa514 passes Linux8/8 in15.55s; Windows7/9 passes in53.09s,
with a native-separator expectation in archive_item_open and a missing encrypted
nested edit prompt. The former expectation is normalized at the test boundary;
the latter now checks original extracted bytes and fixture completion before
waiting for the source prompt, with source error text on failure. Local affected
Open/nested suites pass2/2 in4.92s. Fresh Windowsf73a28cc passes9/9 in39.57s,
including the new Open failure fixture. Three further nested_edit_chain repeats
in that same immutable Windows snapshot pass in26.99s through scripts/ctest.sh;
the preceding prompt failure does not reproduce with the completion handshake.
No production correction or root-cause claim is made for that earlier failure.
Full Linuxe5549311 passes formatting/all-target build, then222/225 CTest in159.59s.
The remaining ui_state/panel_selection expectations assume case-insensitive
matching, while the retained upstream Wildcard.cpp defaults to case-sensitive
matching on Linux. Their assertions now use that source policy, including
SelectByType/inversion. agent_output inherits SEVENZIP_CTEST_JOBS=1 from the
runner but expects the wrapper's default4; the default fixture now clears that
override, preserving separate override/precedence checks. No application behavior
is changed. Local affected suites pass3/3 in0.73s. Full Linuxc5a4a1b3 passes
formatting/all-target build and225/225 CTest in166.75s (Ubuntu24.04.5 x64,
GCC13.3, Qt6.4.2), snapshotSHA256
944ea3fb55543d9af6697460af019be291d487c75e34bc7b212f761d1143863b.
Windowsfdbf3ff2 passes formatting but fails all-target compilation: moc sees
POSIX slots guarded by compiler-only _WIN32, while MSVC excludes their bodies.
Platform-only slots in fs_folder, panel_selected_state, property_formatting and
native_library now use Qt's Q_OS_WIN guard, so moc and compiler agree and the
Windows LoadEx test is included there. Four local affected suites pass4/4 in2.53s.
Full Windows3ee10fb1 is pending; Linuxc5a4a1b3 predates these final guard changes,
whose POSIX behavior is unchanged. Snapshot2a6fa514 predates the new failure fixture.
Native extension, email, System Properties,
installed/rendered Windows UX and final acceptance remain open.

## P2-09 WIP: program File-menu source dispatch (2026-10-04)

Native System prerequisite: copied the complete pinned
PanelMenu.cpp::CFolderPidls, ShellFolder_ParseDisplayName and
CPanel::CreateShellContextMenu bodies into the existing preserved module.
PIDL RAII, desktop binding, super-prefix retry, empty-selection refusal, item
relative paths and GetUIObjectOf control flow remain original. Only GetParent
uses Qt QWidget::winId to obtain the Windows HWND; debug macros retain the
original disabled configuration. The declaration/build are Windows-only.
The same module now also retains original CreateSystemMenu and the native
CMINVOKECOMMANDINFOEX body from InvokePluginCommand (upstream lines999–1084).
The latter is factored into CPanel::InvokeShellContextMenu; the existing shared
InvokePluginCommand keeps numeric range routing, success selection clearing and
error presentation. HWND comes from QWidget::winId; no verb strings or archive
command syntax are reconstructed. lpVerbW remains NULL as upstream requires for
Explorer subhandler compatibility; ANSI and Unicode directories remain present.

New qtnativeshellmenu is Qt/Windows presentation glue only. It retains the COM
object and native HMENU per menu generation, balances successful STA
initialization, projects source-generated text/order/state/default/checked rows,
and forwards WM_INITMENUPOPUP through IContextMenu2 for lazy submenus. It does
not replace shell PIDL enumeration or command generation. QtFileManagerShellMenu
now connects the source System group on Windows and rejects stale QAction
generations before numeric invocation. Unix retains the explicitly unavailable
Windows shell group. Owner-drawn native rows/icons, arbitrary third-party shell
handler messages and rendered Windows comparison are still unverified; the
whole native UX and P2-09 are not accepted. Native argument/error contracts and
actual Windows generation/lifetime tests are added. Snapshot13edb0ee compiles
the bridge with MSVC14.44/Qt6.8.3 and passes7/7 focused suites in36.79s,
zero skips. Eight EN/RU normal/compact JSON reviewed:29 root rows with System
first,19 child rows retaining OS labels/state, zero clipping. Native icons,
owner-drawn rendering, COM DLL installation and whole P2-09 remain pending.
Current snapshotc1b68eed passes8/8 Windows focused suites in35.54s, zero skips.
GetCommandString supplies canonical native verbs for the real Copy regression:
the selected Unicode/spaced file reaches the OS clipboard through numeric
InvokeCommand, source success clears selection, and stale QAction invocation
is rejected after generation replacement. The test opens the File menu once;
its earlier duplicate popup unintentionally exercised only the stale guard.
Source baseline is https://github.com/ip7z/7zip.git commit
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17,
CPP/7zip/UI/FileManager/PanelMenu.cpp and Panel.h. Only borrowed
declarations/body additions and required formatting changed in that owner.
The49-suite e0a5aaaa snapshot predates this copy and cannot validate it.

Current retirement gate32797: unsandboxed formatting/all-target build and
229/229 CTest PASS97.44s on macOS arm64 Qt6.11.2. File/menu/editor migration
has no helper commit and does not accept the whole step. Subsequent localized
Add presentation correction centers the Archive caption between prefix/name
and removes memory-decompression caption participation in the g0xs/g1xs label
column. Pinned CompressDialog.rc lines61–63/98 and actual original/v12 Windows
PNG establish these relations. Functional compression bodies are unchanged.
The compact EN/RU caption regression fails before correction; six direct suites
PASS9.06s after it. Eight regenerated source-add JSON were reviewed. This later
edit is covered by focused checks; it postdates the229-test full gate.
New49-suite Windows/Linux snapshot20261004T032721Z-e0a5aaaa is running.
Previous45-suite69d3153c failed Windows link due to disk exhaustion; Linux
retry compiled but failed nested opening readiness, unsupported QtTest Cyrillic
key input, missing screenshot directory and case-insensitive keypad expectations.
Tests now wait for Open Inside availability, send Unicode QKeyEvents directly,
check history JSON/geometry with opt-in PNG, and retain source Linux case policy.
Only completed owned build/dev caches were removed; snapshots/logs/reports retained.

Subsequent retirement of Journal and built-in Text View/Edit: reviewed the
same pinned FileManager/resource.rc menu, PanelItemOpen.cpp::EditItem /
StartEditApplication and ProgressDialog2/MessagesDialog. These extra commands
have no original menu/resource owner. Removed their QAction/extension dispatch,
recent-operation collector, independent FileViewerDialog and unused MainWindow
preview/extract/update wrappers. Original F3/F4, external process lifetime,
archive update prompts and source error accumulation are retained unchanged.
The old file_viewer/language_viewer suites are retired; native Open/temp/Cancel
cases move to archive_item_open, existing source external editor suites retain
success/failure/Cancel, and language_archive_command covers original launch
failure text/geometry. A 17 MiB native viewer fixture covers absence of the
former preview restriction. This is removal of migration debt, not a new
source algorithm or completion of P2-09. Fresh checks are still in progress.
Baseline: https://github.com/ip7z/7zip.git at
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Earlier descriptions of the local
editor and journal below are historical evidence, superseded by this retirement.

Subsequent alias removal: inspected pinned `FileManager/resource.rc::IDM_MENU`,
`App.cpp` toolbar command IDs, existing `Panel.cpp::AddToArchive` and original
Copy/Open workflows. Removed independent OpenDialog/CreateArchive/AddSources/
ExtractSelected extension IDs, QAction construction, Ctrl+O picker command,
widget fields, dispatch/availability and MainWindow::openDialog. Those menu
entries are absent in the original. Add uses source toolbar/Explorer entry;
archive updates use original two-panel Copy; selected extraction uses source
Copy To/F5, Open uses the source Open/Open Inside/current address workflow.
Archive context no longer appends the extra Qt Add/Extract/Test/Info group after
the source projection. Tests migrate selection availability to Copy To and
filesystem creation to actual toolbar Add, while direct opening/volume/password/
cancel behavior remains tested. Actual construction absence is now asserted,
not merely removal from an activated menu. Journal/built-in text view/edit
aliases and their independent consumers remain OPEN, not accepted adaptations.
Fifteen direct suites initially14/15PASS25.58s: panel_key had retained the
former CreateArchive-only disabled expectation for an archive panel. Original
toolbar Add is a different command and source AddToArchive owns IO_FS refusal;
the expectation was corrected, and ui_operations covers that actual refusal.
Further focused/full checks of these later edits are running.
Subsequent full9342 passed formatting/all-target build/230 CTest in96.54s on
macOS arm64 Qt6.11.2. The focused script now includes45 suites, adding actual
Info/menu/Properties/panel/alias consumers that the older29-suite snapshot did
not cover. Windows-only CMake test platform overrides use native fonts/style
for those affected views; bash syntax/diff checks pass and fresh snapshot
`20261004T025853Z-69d3153c` is running on both guests. Those testing-only changes
postdate the macOS full gate and leave its compiled source behavior unchanged.

Subsequent Info correction: original `FileManager/App.cpp::g_StandardButtons`
binds IDB_INFO/IDS_BUTTON_INFO to IDM_PROPERTIES. Qt now binds the toolbar to
that same native ID, using the existing original CApp/PanelMenu Properties
consumer. Independent ArchiveInformation extension, showInfo/QLabel summary
and stale infoDialog lifetime are removed. Original PanelMenu::Properties
continues into folder/archive properties with zero operated indices; the Qt
early-empty return and archive-only toolbar extension have been removed.
Filesystem Properties still uses the recorded portable source-property dialog
adaptation until mandatory native System transport; this change does not claim
that Windows InvokeSystemCommand branch is complete. New/migrated tests cover
actual Info item/folder modal, empty archive, command identity, retained values/
EN→RU→EN/backend calls and four540/360 JSON. Runtime filesystem Info exercises
all four views in flat/non-flat mode. Five direct consumers pass3.16s on macOS.
These changes FOLLOW the passing program-menu full gate230/230PASS95.22s and
snapshot0b1bdcab, so those earlier checks do not validate Info. More direct
policy/source/language/menu checks subsequently pass9/9 in9.76s.

Baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
retrievable from `https://github.com/ip7z/7zip.git`.
`CPP/7zip/UI/FileManager/MyLoadMenu.cpp::OnMenuActivating` File branch
(upstream385–408) copied into the existing preserved owner. The actual Qt
menu bar replaces global HWND/GetMenu; guarded submenu identity and
remove-all/CreateFileMenu control flow are retained. CPanel source
CreateFileMenu, InvokePluginCommand and CApp ExecuteFileCommand own paths,
selection, query IDs, invoke and release. No second functional generator.
The rest of OnMenuActivating Edit/View/Favorites branches is not ported by
this change; the older bookmark partial port remains separately recorded.

New QtProgramFileMenu is presentation/lifetime glue: retained static resource
actions, disposable rendered plugin prefix, focused-panel lease, deferred Qt
action disposal, source menu projection. QtFileManagerShellMenu supplies an
optional dispatcher into original CApp and a generation check because Qt can
keep removed QAction objects alive until DeferredDelete; Win32's removed HMENU
does not supply an equivalent lifetime mechanism. During native proxy replacement
or scanning no folder properties are queried. Original CallScope dispatch checks
remain valid inside the source command; a blanket callActive invoke guard would
incorrectly reject it and was removed before successful checks.
Popup consumers use the retained static list to avoid projecting dynamic shell
commands twice. Program menu omits independent aliases, but their construction
and legacy workflows still exist and MUST be removed during the same P2-09 work.
System/COM/DLL, source GUI process/email and installed extension UX remain open.

macOS arm64 Qt6.11.2: initial three consumers pass9.04s; expanded
shell_menu_window passes0.90s, including real program Open success/forced-type
failure/password Cancel, same-ID stale QAction refusal and panel-switch refusal.
Four EN/RU980/640 File JSON and four shell-child JSON reviewed: original order,
Exit last, no aliases, no clipped rectangles. Native program Add success/modal/
Cancel/broken-existing is covered by shell_update_window. VM snapshot0b1bdcab
remains running; no whole P2-09 acceptance or helper commit.
Subsequent full64502 passed formatting/all-target build/230CTest in95.22s.
Linux0b1bdcab stopped at linker due to a full disk, before CTest. Only generated
build folders of OWN completed b6571bdb/98243ecb snapshots were removed after
process/path checks; source snapshots retained,8.2GiB freed. Same immutable
Linux0b1bdcab is being retried separately; Windows original run continues.
The explicit retry with the original runtime PATH/catalog/jobs environment
subsequently passed Linux29/29 in42.99s; its separate retry log preserves the
initial helper NoSpace failure. This snapshot still predates Info correction.
Windows0b1bdcab passed29/29 in129.34s; combined helper exit1 retains the initial
Linux NoSpace failure, while the separate Linux retry passes as above. Eight
current Windows EN/RU980/640 program File/shell-child JSON reviewed:28 root rows,
5 child verbs, source order/Exit last/no clipping. These reports precede Info
and alias removal and do not validate those later changes.

Separate earlier Add-only evidence: current snapshot07e2aeb5 passes Windows29/29
in123.49s and Linux29/29 in38.34s; native actual v12 PNG/JSON personally compared
with the pinned original, SHA256 and remaining visual differences in parity.
It validates method selection/header/focus corrections, not later File edits.

## P2-09 WIP: actual modal Add and UX correction (2026-10-03)

2026-10-04 subsequent method-selector migration: copied original
`GUI/CompressDialog.cpp::SetMethod2`, `GetMethodID_RAW`, `GetMethodID`, both
`GetMethodSpec` overloads and `IsMethodEqualTo` into the preserved owner from
the same pinned baseline. Qt combo insertion/data/selection replace Win32
controls; level, registry and MethodChanged presentation callback are explicit
inputs instead of implicit dialog members. Source loop, automatic -1 sentinel,
external-method indexing, SFX filtering, Store/TAR/Zstd/hash branches and
registry/keep-method selection are retained. First method has the original
`*  ` caption; source options save an empty automatic method spec while Qt's
CLI DTO uses the estimated actual method name. Standalone legacy callers without
a bound method control retain explicit MethodID/HashMethodSpec adapters; these
are not claims that the entire original dialog has been ported. Existing
GetMethodNames helper remains for non-modal tests/callers, not the actual form.
Thirteen affected local suites pass (10.49s), including actual toolbar native
SFX and source automatic/explicit/Store/TAR state transitions. Current EN/RU
normal/compact source reports retain the automatic caption without clipping;
five SFX bottom reports confirm encryption/footer reachability after scrolling.
Full and fresh VM checks of this subsequent migration remain in progress.

The actual pinned Windows reference was additionally driven through initial Add,
SFX on, then ZIP: original methods are `*  LZMA2/LZMA/PPMd/BZip2`, then
`*  LZMA2/LZMA/PPMd`, then **only LZMA**, with SFX unchecked/disabled.
Original SetMethod2 applies the previous SFX filter before FormatChanged disables
SFX. Retain this source sequencing rather than imposing the old Qt test's
independent expectation of Deflate. Native result JSON:
`build/runners/ui-reference-20261003/original-method-transition-20261004/states.json`.
Capture task result0, dialog cancelled and the owned completed task removed.
The subsequent native v2 probe also reads CB_GETCURSEL: initial and SFX index0,
ZIP-after-SFX index**-1**, despite its one LZMA item. QComboBox automatically
selects the first inserted item; source CComboBox::AddString does not. The Qt
insertion adapter now preserves an absent selection until the original explicit
selection branches run; GetMethodID_RAW preserves CB_ERR for that state.
The corresponding current-index/spec regression fails before this correction
(four EN/RU normal/compact cases). This fix postdates the passing230-test method
gate and29-suite VM snapshot; fresh affected checks are in progress. Native v2
JSON is beside the first artifact in `original-method-transition-20261004-v2`;
task result0, Cancel, own completed task removed.

Actual default Windows Qt v11 vs pinned original additionally exposed the
Archive header grouping: source `CompressDialog.rc` places folder prefix and
Archive combo in the same xArcFolderOffs column, with caption to their left.
Qt's spanning prefix above a separate Archive row was an independent layout
assumption. The form now has one two-line field (prefix + combo/browse) and the
original caption column. A geometric regression fails before the correction
at both sizes/languages (prefix x11 vs combo x65/58). Windows uses native
separators only in the read-only display prefix; internal source paths and
validation remain unchanged. This focused presentation correction keeps the
existing constructor/module boundary; extraction is deferred because no new
workflow/responsibility is introduced. Six affected suites pass9.48s on macOS;
eight current EN/RU normal/compact top/bottom reports were reviewed: shared
edge, caption/prefix/name exposure at top, wrapped prefix without horizontal
overflow, unchanged columns and reachable footer at bottom. Subsequent initial
focus correction is still being validated: the actual original default PNG
shows the archive name fully selected, while Qt's scroll hierarchy left it
unselected. The four-state selectedText regression fails before the explicit
editable-combo focus/selectAll adapter. It adds no extra user interaction.

2026-10-04 source-modal SFX integration is in progress. `UpdateGUI` retains its
source worker and adjacent `7z.sfx` fallback; the injected runtime can supply
the retained catalog's already validated native module path. Qt no longer
disables SFX merely because the input selection came from source ShowDialog.
Original format/method eligibility and validated module availability remain.
The existing Unix executable-bit adapter is shared by CLI and native output;
source archive/compression/SFX stream bodies are unchanged by that adapter.
Stdout performs no filesystem permission update. A permission failure reports
the output path without deleting the published file.

Native split SFX exposed a required Unix naming adaptation: a suffixless
executable produced `name.001`, while original `OpenArchive.cpp::Open3` searches
`name.7z.001`. `CArchivePath::GetFinalVolPath` now accepts explicit SFX mode;
only this mode retains VolExtension with an empty BaseExtension. Normal/Exact
archive names keep the source default behavior; Windows standard `.exe` names
already retain VolExtension. Both original Compress/UpdateArchive call sites
transport their existing SfxMode. A real native split-SFX launch fails before
this correction, even though 7zz validates its payload volumes. Current native
toolbar SFX success, password/header encryption, split volumes, cancellation
and broken-existing-output cases pass on macOS arm64, together with five direct
consumer suites (6/6, 8.91s). The native module was built from the pinned source;
SHA256 `ad64e6ffb725efdab3a2d3072ebffb6b20f74af10a34ed4ebb8c782cdc32b99b`.
These changes postdate the previous full gate and VM snapshots; fresh full and
platform validation is required. This does not accept P2-09.

Baseline remains `https://github.com/ip7z/7zip.git` at full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. The previously copied
`GUI/UpdateGUI.cpp::ShowDialog` now has a real `PresentCompressDialog` consumer.
QtShellArchiveServices retains its factory catalog and supplies original user
codec metadata. `createarchivedialog_source.cpp` is Qt control/DTO integration;
it does not replace source ShowDialog/UpdateArchive. Actual source modal success,
broken existing output and Cancel are checked with official native engine output.

Reviewed desktop `GUI/CompressDialog.rc`, `CCompressDialog::OnInit`,
`UpdatePasswordControl`, `OnButtonSetArchive` and caller
`FileManager/Panel.cpp::AddToArchive`. The caller uses the operated filesystem
selection; CompressDialog contains no source picker. Ordinary filesystem Add
now omits the independently written picker. Plain left compression controls,
left volumes/parameters and right Update/Path/Options/Encryption restore source
structure. Separate Save in and target preview were removed, folder prefix is
a read-only label above Archive+`...`, standard footer is OK/Cancel/Help.
The save picker uses CBrowseInfo's platform adapter; remaining independent
File-menu aliases are still mandatory migration debt.

2026-10-04: copied `FileManager/Panel.cpp::AddToArchive` into its preserved owner,
retaining IO-folder refusal, selected-only indices/empty-selection error,
GetFsPath/drives root, GetFilePaths/CreateArchiveName and original CompressFiles
flags/error branch. The only adaptations are an injected IQtCompressCallServices
instead of the process-global launch entry and `/` for the POSIX filesystem root
(Windows keeps `C:\\`). MAX_PATH comes from the existing platform header.
MainWindow's old create/add picker workflows and custom CopyFrom options builder
were removed. Its glue retains catalog/selection/call lifetime, presents the
existing source modal and refreshes only a successful presented operation;
Cancel/refusal must not restart flat enumeration or relist a controlled archive.
The CPanel body still does not KillSelection, matching the commented source line.
Every CreateArchiveDialog branch now omits the picker; no navigation/list/add/remove
source controls remain. Source Add can update existing outputs through UpdateGUI;
it no longer invokes the independently written ArchiveBackend::create consumer.
Legacy menu aliases remain to be removed under the program File-menu owner.

2026-10-04: removed the unused production `existingTarget`/`existingFormat`
constructor branch. It independently hid encryption, disabled path/format/browse
and volumes, and forced a fixed output. The source form now owns those controls
for existing and new outputs alike. Existing-output intent is accepted without
touching its bytes; source UpdateArchive decides whether the output can be
updated. Constructor tests were migrated to ordinary/source forms and native
unknown-extension update remains covered through original two-panel Copy.
All-target compilation and12 affected suites pass (9.66s, macOS arm64).
The widget also no longer demands existing destination parents or independently
preflights input existence/selection: original OnOK validates GetFinalPath_Smart,
and Common/Update.cpp::Compress creates parents through CreateComplexDir.
`compression_path`'s new-parent confirmation test fails before the correction;
native toolbar Add success/Cancel now also cover a Unicode/spaced new parent.
Validation of this subsequent preflight correction is pending.

Tests formerly choosing sources inside Add are being migrated to filesystem
selection and source toolbar Add, or original two-panel Copy for a detected
archive with unknown extension. Password, path modes, Freshen/Update, processed
deletion and TAR links retain real archive/input checks. Seven native formats
now have actual toolbar Add success/Cancel and normal/compact reports.
`QtFileManagerDrag::CompressFiles` now transports the original PanelDrag flags
through source CompressCall2/UpdateGUI instead of ArchiveBackend::create.
`source_drop_add` verifies native 7z/ZIP, modal Cancel and broken-existing refusal
with Unicode/spaced inputs and zero CLI create starts. The old controlled archive
Copy test remains separate: its synthetic archive cannot be opened by native
factories. Error progress is dismissed via the original Close state.
Encryption method is beside its combo and decompression memory right-aligned,
as in the reviewed Windows reference. Password labels remain above their edits.
Current macOS focused16/16PASS14.41s (2026-10-04); full and platform checks are
pending and the older227-test gate does not validate these new changes.

2026-10-04: copied original `CompressDialog.cpp::AddFilter`, `k_DontSave_Exts`
and complete `OnButtonSetArchive` filter/extension/format branches into the
preserved owner. Original `BrowseDialog.h::CBrowseFilterInfo` is reused. Win32
combo count/data/selection is adapted to the existing ArcIndices/Info model;
QString path storage is converted at the source UString boundary. The Qt caller
applies the existing SaveOptionsInMem/FormatChanged binding when Format changes.
The bool return lets the caller leave widgets untouched on Cancel; the optional
typed BrowseFileHandler supplies deterministic dialog results in tests. The
default uses actual CBrowseInfo/QFileDialog, not a test-only production path.
CBrowseInfo translates original descriptions/masks into Qt's name-filter syntax,
hiding the additional Qt mask details so descriptions are not displayed twice.
Original filter order/index, omitted save aliases, aggregate/all-files filters,
SFX `kExeExt` (Windows `.exe`, existing Unix empty-extension adaptation), recognized aliases, trailing dot, confirmed-format extension and
all-files extension inference remain source decisions. `source_archive_browse`
checks seven confirmation cases, Cancel and SFX; focused1/1PASS0.41s on macOS.
The current macOS focused19 suites pass in14.93s. Snapshot
20261003T232818Z-24176433 passed Linux18/18 in28.87s, but Windows failed a
separator-only browse expectation and crashed in the Qt refresh timer during
native archive-to-archive Move. The expectation now normalizes separators.
The timer and command projection honor native modal/notification guards before
querying the worker-owned proxy; borrowed Agent bodies remain unchanged.
`modal_refresh_window` holds real F5 Copy through two timer ticks and checks
source panel/selection and unchanged input on Cancel. Removing the timer guard
makes this regression fail. Full230 exposed an additional flat-scan cancellation
regression: the notification guard must restrict property queries, while the
flat-view toggle remains available during enumeration. That focused fix and
new platform/full checks are pending; P2-09 remains unaccepted.

The reference was extracted by `check_ui_parity.py` with resource/shared input
hashes into local `build/runners/ui-reference-20261003/` (two desktop dialogs).
Windows reference 7zFM was built from an unchanged exact Git archive in a
separate VM directory, not the sibling checkout. Version26.03, x64,
SHA256`310fc41187fe3482de653a12cb3fc27d2826f9d18e95bd9a7835bf44cfc54151`.
Actual Add runtime JSON/PNG with52controls,96DPI/client624×546 was captured and
inspected; artifact paths are in parity. The capture tool permits one explicit
native HWND and avoids cross-process MapDialogRect, which caused an observed
USER32 access violation. Resource previews retain in-process units; external
runtime geometry records physical pixels and null dialog units.

macOS arm64 Qt6.11.2 gate passed formatting, all-target build and227/227 CTest
in92.40s; subsequent focused12/12PASS11.95s covers the UTF-8 listing and full-control
scroll check. These checks precede the latest PrintMemUsage correction below.
Windows offscreen runs5/12 and6/12 exposed fallback box-font metrics, confirmed
against the native plugin: four of five UI suites passed there, with the remaining
editable-combo check corrected to scroll its whole bounds rather than its input
cursor. The thirteen Add/consumer tests now use the Windows native platform.
Fresh Windows snapshot20261003T211327Z-1c50e01d passed11/12 suites64.86s,
including all Add geometry/language cases. `ui_operations` failed its nested
archive-to-opposite-archive refusal expectation; the later diagnosis/fix is below.
That snapshot predates the latest memory/Delete corrections. No cross-platform
acceptance is claimed.
Original7zFM and Qt native-platform EN/RU normal/compact PNG/JSON were obtained.
Their structural comparison also exposed omitted limit/RAM text. The original
`CCompressDialog::PrintMemUsage` body is now copied with Qt text return and explicit
parsed spec/system fields replacing Win32 control/state access. Unknown usage,
explicit/automatic limits, known/unknown RAM and decompression branches are retained;
the compression label now sits above the value/limit row as in the resource.
The independent Delete checkbox disable for non-Add modes was removed after
reviewing OnInit/OnOK and Update.cpp::CheckDeleteAfterCompressingActionSet:
the original UI keeps it available and the worker rejects an incompatible set.
Actual source modal Delete success/Cancel/incompatible-mode refusal are covered,
including input preservation on refusal/cancel. Current macOS focused5/5PASS3.14s
includes these changes plus PrintMemUsage branches. Eight regenerated EN/RU
normal/compact top/bottom reports retain full memory text, visible lower fields
after vertical scrolling and the fixed footer. Full macOS gate passed formatting,
all-target build and227/227 CTest in88.37s on2026-10-04. Subsequent localized
`ui_operations` test adds observable Copy enable/dialog/destination prerequisites
for Windows diagnosis; the later fresh passing snapshot is below. User-directed
application-wide correction is mandatory in [ux-port-audit.md](ux-port-audit.md),
including accepted historical scope. P2-09 remains unaccepted; source-modal SFX,
email, native System/COM/DLL/GUI launch/packaging/installed Windows UX and program
File-menu consumer still require implementation and validation. No helper commit,
push, publication or upstream write was performed.

Windows nested-transfer diagnosis confirmed that after Copy's modal closed,
the source request still identified panel0 but Qt's active projection had become
panel1. The defensive consumer discarded the request. Original
`FileManager/App.cpp::OnCopy` keeps `srcPanel`/`destPanel` references through that
modal. `QtAppPresentation::ExecuteCopy` now restores the captured source's Qt
projection before consuming the original request, retaining the close guard.
The previously failing case passes in an isolated Windows diagnostic build
(1/1,1.37s); its temporary scalar traces were removed and original-file hashes
verified. This private diagnostic is not a fresh snapshot/full acceptance.
New macOS full gate passed formatting/all-target build/227of227 CTest in81.24s.
Fresh Linux13/13PASS25.17s includes the Copy projection fix. Windows validation
for this exact fresh snapshot passed13/13 in65.39s: run
`20261003T215547Z-2dad9bab`, snapshotSHA256
`4014610770264b58c548038f9ea94777a686e5e276656725ff20cab327d44520`.
This focused three-platform result does not close the remaining whole P2-09 or UX scope.
Linux's prior fresh13-suite check passed13/13 in24.24s after the fixture used an
explicit1MiB LZMA2 dictionary: level9's implicit dictionary required2727MB against
the guest's source memory limit2290MB. Product validation remains unchanged.

Здесь хранится реестр происхождения кода: upstream-ревизии и исходники,
перенесённые символы, адаптации, пропущенные части и долг переноса.
Правила работы заданы в [AGENTS.md](../AGENTS.md); наблюдаемое поведение
и платформенные отличия — в [сверке с 7zFM](7zfm-parity.md).
Владельцы компонентов и интеграционные границы описаны в
[архитектуре](architecture.md). Эти документы ссылаются на реестр,
не дублируя сведения о происхождении и историю переноса.

## P2-09 — shell/context integration (in progress 2026-10-03)

The source ShowDialog functional body, FindActionSet, ParseProp_Time_BoolPair,
ParseProp/ParseProperties and CompressDialog::SetMethods are now copied from
the same full pinned revision. The existing PrepareFormatSelection and work-dir
helpers keep their single authoritative source blocks. Options, action sets,
metadata, volumes, password, method properties, selected format and archive path
roundtrip remain source control flow. PresentCompressDialog replaces only modal
Create/IDOK; its default still returns E_NOTIMPL until the fixed-selection Qt
presentation is connected. GetUserCodecs is a narrow provider boundary; the
retained factory owner must supply external method metadata to the real modal.
CInfo adds original ArcPath and a QString password transport; conversions are
local. Unix SFX keeps the existing native executable path without an exe suffix.
Headless regressions cover roundtrip, cancellation without publishing edits,
unsupported action set, multiple-command refusal and external encoder/filter
selection. No modal Add UX or whole-step acceptance is claimed by these tests.

Fresh Hyper-V snapshots of the preceding worker/popup change: Linux x64 Ubuntu
24.04.5 passes17/17 in13.33s (20261003T190248Z-afecbd89, snapshot SHA256
72db721373f297baa54758caa3a8358bdc2ad936b8cbdb00cf770305c2bbb01a).
Windows11 x64 builds and passes13/17 in44.50s (20261003T190251Z-88395521,
snapshot SHA256 addb2c9e53bd1eb5cccdb6bb3472e05a2ff1222d78b4efad6b78ea06e0a8baaf).
The source_update_backend and new GUI/popup suites pass. Four failures exposed
host-newline-dependent baseline fixtures, a slash-sensitive context expectation,
and SFX CPU normalization in two suites. Fixtures now preserve exact bytes and
compare native paths at the Qt boundary. Original PeHandler.cpp g_MachinePairs
reports 0x8664 as x64; ArchiveCodecCatalog now maps this to Qt x86_64 alongside
amd64. Runtime and SFX revision/version/hash were verified on the guest.
The repeat focused runs now pass on both guests: Linux17/17 in13.10s
(20261003T191851Z-1dd72c8f, snapshot SHA256
47b22c2063eefc23d88dd9f3f083ab3de7c0449b8177a134dd6963e04eeed576), Windows17/17
in56.10s (20261003T191849Z-c6a8afca, snapshot SHA256
f39489644b84585d4e442687b857dcd550c564d7f744554758c9f0b9dc98ae1b).
All four preceding failures are resolved in these runs; they also exercise the
new source modal roundtrip and method filtering. Local focused10/10 in6.73s
passes. Final unsandboxed macOS arm64/Qt6.11.2 check.sh passes formatting,
all-target build and226/226 tests in74.99s (2026-10-03), baseline385/191/3.
No code changed after its start. A preceding full run during concurrent builds/
CTest in the same dev directory failed five UI/editor/lifetime suites; all five
passed with isolated generated settings/temp/registry paths (5/5 in16.04s),
then the standard full gate passed after the parallel checks had ended. No
product code was changed between those runs. Other runner tooling changes in
the shared worktree are preserved and remain outside this owned port scope.
These narrow runs are not full Windows/Linux gates or installed shell-extension
UX evidence.
The eight popup/root/hash EN/RU normal/compact JSON from each of those guests
were read and reviewed: source order/verbs/enabled states match, all visible
action rectangles are exposed. Windows row widths are455 EN/503 RU and563 hash;
Linux273 EN/305 RU and342 hash. This is widget geometry and structural evidence,
not a rendered Windows reference or installed Explorer acceptance.

GUI/UpdateGUI.cpp now copies CThreadUpdating, its ProcessVirt and main UpdateGUI
from pinned0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Fields, callback Init,
SFX default, compress/hash title, Create/result and messageWasDisplayed flow
remain source bodies. HWND becomes QWidget; the update and modal ShowDialog
call sites go through IQtUpdateGUIServices. This worker checkpoint preceded the
source ShowDialog addition above; modal presentation/Add/email remain disabled.
ArchiveBackend transports the exact
COpenType vector and raw UString path to its retained native factory owner;
no type re-parsing, catalog copying or archive algorithm is added. GUI preflight
owns busy/stale checks; the native worker only reads the atomic catalog lease,
not QProcess/widget state. Unix email is explicitly refused because the source
MAPI block is Windows-only; its platform integration remains mandatory scope.
QtShellArchiveServices connects source CompressFiles through this worker when
no external launcher sink is provided and preserves its actual result despite
the original wrapper's final S_OK after handled exceptions. Direct popup Add
to7z/ZIP and SHA256 generation are now available through source command enums;
modal Add/email, native System/program File-menu/COM/package remain pending.
Focused10/10PASS6.24s macOS2026-10-03, including native success/error/cancel,
stale lease and actual popup result/selection/auto-refresh. Eight current EN/RU
normal/compact root/hash JSON reviewed; the report helper now includes QAction
text/order/bounds/enabled/source verbs. No rendered Windows reference or new
screenshots are claimed. The whole P2-09 step is not accepted.
Full unsandboxed macOS arm64/Qt6.11.2 gate now passes formatting, all-target
build and226/226 CTest in90.12s (2026-10-03), baseline385/191/3, before the
ShowDialog and Windows fixes above. The physical Hyper-V host is reachable;
the focused guest results above do not prove installed native UX.

The full Common/Update.cpp `Compress` and main `UpdateArchive` bodies are now
copied from the same pinned revision, including source rename matching,
update-pair production, callback setup, writer properties, stream/tail/volume
output, scanning, temporary publication, Windows MAPI and source failures.
Common/TempFiles.cpp/.h retain their paths and source cleanup bodies; changes
are include portability, notices and clang-format. Existing extracted SFX,
mtime, delete-action and processed-deletion phases remain authoritative and
are called from the newly copied orchestration. A local deletion callback
adapter forwards IUpdateCallbackUI2 without changing the source deletion loop.
Both SFX consumers share the original CopyStream/Close phase through
WriteSfxStream. Compress retains its input/output open order and WriteSfx
notification without reopening the module.
ArchiveBackend::updateSourceArchive and ArchiveCodecCatalog::updateArchive
confine the source mutable-pointer signature to the retained codec owner;
the catalog and factory tables are only read. They do not copy library owners.
UpdateCallback, UpdatePair and UpdateProduce move between CMake targets from
sevenzip_location to sevenzip_filesystem alongside their source caller; their
paths and bodies are unchanged. This avoids a static-library dependency on the
Agent/UI provider for panel-only backend consumers.
This Common prerequisite predates the source UpdateGUI/direct popup consumer
recorded above; modal ShowDialog and email/native launch remain pending.
Validation after correcting static-library ownership: focused6/6PASS3.98s;
full unsandboxed check.sh passes formatting, all-target build and224/224 CTest
in83.41s on macOS arm64/Qt6.11.2, 2026-10-03. Baseline385 exact/191 adapted/3
local paths. Windows/Linux runtime and native MAPI remain unverified for this
addition. No popup capability is enabled by the backend prerequisite.

Not accepted. Pinned baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
`https://github.com/ip7z/7zip.git`. Inspected complete Explorer ContextMenu query/
invoke and IExplorerCommand bodies, RegistryContextMenu, MyMessages,
DllExportsExplorer, FM MenuPage and Common CompressCall launch/map dependencies.
ContextMenu now copies command/hash tables, FillCommand/AddCommand, extraction
suffix/reduced-name helpers, original FindExt/exclusion predicates,
QueryContextMenu, FindVerb and InvokeCommandCommon. Source CCommandMapItem,
command enums/fields retain their names. Win32 drawing handles are replaced by
a presentation tree through CMenu/CMenuItem/CMenuDestroyer adapters; insertion
order and source map-before-menu/command-ID accounting remain unchanged.
Typed IQtContextMenuServices substitutes launch/error call sites; raw paths and
open type are passed separately instead of an OS command string. CContextMenuInfo
receives the existing explicit settings path. Bitmap loading becomes a resource
presentation token; drawing/allocation errors cannot supply a Win32 last-error.
Source constructor DLL refcounts and native Initialize/InvokeCommand/GetCommandString/
IExplorerCommand methods are omitted here until their separate native facade.
Partial Windows/Shell.h copies NShell::CFileAttribs; other shell helpers are not
ported by this change. Common/CompressCall.cpp copies shared GetQuotedString;
Windows process/mapping-launch functions remain pending integration.

Common/CompressCall2.cpp additionally copies ExtractGroupCommand, ExtractArchives,
TestArchives, CalcChecksum and CompressFiles. Source CCensor/EnumerateDirItemsAndSort, sorted/full
path vectors, MultiArcMode, type parsing, output wildcard mode, ElimDup/ZoneMode
and exception/error flow remain. CREATE_CODECS/LOAD_EXTERNAL_CODECS/global HWND
are replaced by IQtCompressCallServices catalog/provider and GUI boundaries.
The outer !Z7_EXTERNAL_CODECS build guard is omitted: this application uses an
explicit retained catalog and native opener rather than those global loaders.
Direct wrappers return HRESULT to make integration refusal/cancellation observable;
generation still delegates the source hash compression request. CompressFiles
retains CUpdateOptions/action, ParseOpenTypes/InitFormatIndex/SetArcPath, censor,
callback failed-file and exception flow. Its original final S_OK after a caught
error is preserved; callers must observe service errors, not infer success from
that return alone. The default service remains E_NOTIMPL; the concrete service
now uses the source UpdateGUI worker recorded above.
Common Update.cpp/.h additionally copies CArchivePath path/name/final/temp bodies,
CUpdateOptions::InitFormatIndex/SetArcPath and option/interface structures.
CRenamePair::Prepare/GetNewPath and the main UpdateArchive body are now copied
as recorded above. Complete
GUI UpdateCallbackGUI.cpp/.h functional bodies are copied; only precompiled and
portable resource/include paths change, with required project formatting.
The concrete Add/UpdateGUI worker/dialog service remains required work.
New source grouping/checksum fake GUI cases and direct consumers pass3/3 in0.57s
on macOS2026-10-03, after successful test rebuild. This does not prove a concrete
engine consumer or native registration; no new controls were enabled.

QtShellArchiveServices now supplies retained catalog metadata, source ExtractGUI
through QtArchiveBatchOpen and source HashCalcGUI through native providers.
The catalog lease retains factories; only Formats metadata is copied for source
type parsing. A bounded final-path/link policy reads the current source output
root (including dialog edits and per-archive wildcard substitution); the source
Extract body remains authoritative for destination naming. QtArchiveBatchOpen
transports an explicit one-format/hash name separately. Multiple/compound types
remain unsupported here; the eventual Open/native facade must preserve them.
QtArchiveFileResource::ConfigureArc maps a native handler name into the caller's
metadata table using source FindFormatForArchiveType. It never copies a native
index between codec owners. This restores source IsHashHandler's manifest-folder
branch, which a new valid .sha256 Test fixture exposed as failing beforehand.
The existing process-only/empty-table path keeps FormatIndex=-1.

macOS2026-10-03 shell_archive_services/shell_context/source_extract_advanced/
source_extract_backend/native_hash_ui final pass5/5 in0.72s after current target rebuilds.
Actual Unicode/spaced ZIP/7z fixtures verify wildcard extraction bytes, Test,
wrong forced Hash refusal, exact SHA-256 in source results, valid/CRC-error hash
manifests, refusal of a parent-path manifest and overwrite cancellation preserving
existing bytes and archives. Rebuilt application and direct Update/SFX consumers;
source_update_backend/source_metadata_storage/update_callback/archive_update_callback/
sfx_creation/compression_path pass6/6 in4.20s. Baseline verifies384 exact,
187 adapted and3 local paths at the pinned revision; diffcheck passes.
The responder initially timed out by clicking a parent progress control in a
nested result dialog event; it now returns after dismissing each result dialog.
Fake source CompressFiles tests cover raw paths, Add/Exact extension modes,
email/show flags, unsupported/read-only handlers and error/cancel reporting.
This is localized evidence; P2-09 full gate and Windows/Linux/native installed
checks are pending, with Menu and FM placeholders still disabled.

Application and focused targets compile on macOS; shell_context/source_extract_advanced/
menu_page3/3PASS1.37s2026-10-03. Deterministic fixtures check flags/directory guards,
extended verbs, Unicode/spaced raw paths, multi-part extraction names, CRC tree/
generation/Test and16-item display versus full17-item name collision handling.
This is source-owner validation, not whole P2-09 acceptance. FM System/7-Zip
placeholders and Menu shell controls remain disabled. The source query's16-item display reduction,
full-selection invoke recomputation, flags/verbs/command-ID budgets and directory
guards must remain in the copied owner. Required adaptations are menu rendering,
typed launch/registration services and a project-owned native extension identity;
the original CLSID must not replace an installed official 7-Zip extension.
Native extension packaging/installed-runner checks remain part of this iteration.

RegistryContextMenu.cpp/.h now copies CheckHandlerCommon/CheckContextMenuHandler,
MyCreateKey and SetContextMenuHandler with the original two-by-four handler
matrix, result propagation, best-effort Approved write, reverse class-key
deletion and retained shared handlers. IServices/IKey/CKey transports abstract
the native root/handle/read/write/create/delete calls for deterministic fixtures;
it supplies no registration policy. UString replaces platform CSysString so the
same body is testable on Unix. Native access/WOW flags keep Windows values.
The XP-era dynamic RegDeleteKeyEx resolver is omitted: the Qt6 Windows baseline
provides the API, and deletion belongs to the native service. That service and
actual MenuPage Apply consumer are still pending. The project class is
`{52D06B47-DA74-4A5F-8B5E-4494C729CC9C}` with SevenzipDesktop handler key names,
so it cannot replace official 7-Zip's class/key identity. Tests cover each view,
Unicode/spaced DLL paths, class/path checks, Apartment model, handler roles,
Approved refusal and failed DLL-value writes, unregister order/shared-key
retention, and preservation of pre-existing official registration. No OS
registration has been performed, and this source seed does not establish native
extension packaging or installed UX acceptance.

MenuPage.cpp/.h now copies CShellDll, the discovery/WOW-view loop, shell-change
branch and reverse-order OnApply/registration/error/Check requery. Qt checkboxes
replace source GetItem/CheckButton/EnableItem/HideItem; IQtMenuPageServices
supplies module paths, process bitness, registry and errors. Module filenames
are project-owned sevenzip_shell.dll/32/64 variants. Source wasChanged/prevValue
flow and unchanged-settings behavior remain; available command controls clear
their localized unavailable tooltip binding, including later language changes.
The native service factory is used by the real page. Unix returns unavailable
for Win32 shell modules; Windows discovers adjacent project DLLs and wraps
unchanged upstream Windows CKey for registry handles. Classes maps explicitly
to HKCU Software/Classes (per-user installation instead of the source merged
HKCR write root); the Approved HKLM write remains source best-effort. The Qt
application directory replaces original module-global HWND/DLL location. Module
packaging and Windows compilation/runtime validation are still pending, so this
does not establish native UX acceptance or enable absent modules.

Current macOS application/menu/language targets build. Final menu_page/
shell_registry/shell_context/shell_archive_services/language_ui pass5/5 in0.79s.
Fake Menu tests verify reverse Apply, failed registration requery/reset,
Cancel without writes, missing DLL, 32-bit process without other shell, source
flags persistence and EN/RU640×390/520×380 grouping/order/checkbox widths.
Four current menu-shell reports reviewed: compact controls498px wide, largest
RU bitness hint391px, zone row separate, list498×185 fully exposed and scrollable.
Reference is pinned MenuPage2.rc/OnInit/OnApply; resource structure and Qt geometry
do not establish rendered Windows pixel parity. Existing Options/language
consumers pass; full P2-09 gate/native extension checks remain open.

The same pinned revision now supplies Panel.cpp::GetFilePaths and
PanelItems.cpp::Add_ItemRelPath2_To_String, preserving reserved path-vector
construction, index order, prefix/name lookup (including the native fast-name
interface), original exceptions and Windows drive suffix. Only the unused
Windows-only start offset is marked unused on Unix. MainWindow's filesystem
selection transport delegates to GetFilePaths rather than rebuilding paths.
PanelMenu.cpp::CreateSevenZipMenu copies the source query preamble, plugin ID
range, CMF_EXPLORE/extended-verbs flags, FirstDirIndex and retention of every
successful context query, including zero items. A retained unique_ptr replaces
the Win32 IContextMenu COM pointer; the existing explicit settings and typed
workflow service are injected. Debug/legacy commented COM branches are omitted.
CreateFileMenu's shell preamble, System menu, actual context presentation and
InvokePluginCommand remain pending; this preparation does not enable them.
Current macOS app and affected targets build; shell_context/file_menu_policy/
panel_selection/filesystem_selection_runtime/drag_drop_add_ui pass5/5 in1.25s.
The new real filesystem fixture verifies Unicode/spaced paths, reversed indices,
directory guards, raw compression arguments, extended verbs and empty-query
retention. Windows readiness fails at physical-host SSH with timeout, then
No route to host; no Windows build/registration evidence is claimed.

Current complete local checkpoint: unsandboxed scripts/check.sh passes formatting,
all-target build and222/222 CTest in87.94s on macOS arm64 Qt6.11.2 2026-10-03.
The final fixture also compares paths to literal local fixture names, normalized
only at the native-separator presentation boundary. Baseline verification passes
384 exact/189 adapted/3 local paths at the pinned revision; both diffchecks pass.
This gate validates the current WIP, not the remaining native/FM consumers or
whole P2-09 acceptance; no intermediate helper commit is made.

The actual disk popup now calls the copied CreateFileMenu shell preamble:
release contexts, read Shift, obtain operated indices/FirstDirIndex, exclude
archive folders, create 7-Zip then optionally System. IQtPanelPluginMenuServices
transports handles/keyboard/app preference; CFileMenu's Qt regular-item
projection starts after the separately rendered plugin prefix, remapping the
original menu.GetItemCount() to projection offset0. The former independent
MainWindow preamble and its extra separator are removed; upstream's corresponding
append-separator block is commented out. Root namespace shell transport remains
unavailable; native System keeps its existing disabled group pending its facade.
Source PanelMenu::InvokePluginCommand retains System/SevenZip offsets, absent
context refusal, current folder and success-only KillSelection. Native commandInfo
structure packing/error presentation is behind IQtPanelPluginServices. ContextMenu
additionally copies InvokeCommand's final numeric-offset validation and dispatch
body with COM_TRY; native ANSI/Unicode pointer decoding is still pending.

QtFileManagerShellMenu renders copied IDs/labels/tree; MenuLogo.bmp is copied
unchanged from the same pinned repository revision, and Qt applies its background
color as the icon mask. Source Open/Extract/Test/Hash commands are connected;
Add/email/generation remain disabled. Actual Open adapts source MyCreateProcess
of another7zFM to another MainWindow with independent ArchiveBackend process/
operation state and the same immutable codec-module lease. The new backend
factory is integration glue; upstream obtains that separation by process launch,
not a reusable factory. Raw path/type reach the existing source BindToPath.
An archive-only S_FALSE probe is retained when BindToPath successfully clamps
to the containing filesystem folder; it is no longer reported as archive success.
Concrete service result tracking prevents failure/cancel selection clearing and
duplicate error boxes while preserving the copied Query/Invoke workflows.

New shell_menu_window tests invoke the real popup against local Unicode/spaced
7z archives: separate-window opening, forced ZIP refusal and encrypted-password
cancel preserve bytes and caller folder/backend. Successful invocation clears
selection; failure/cancel retain it. EN/RU980×620/640×400 action geometry/order/
text-width checks and four current JSON pass; reviewed bounds172×115 EN and
201×115 RU, with the Open-as menu intentionally hidden. One EN submenu PNG
inspected only for the new bitmap rendering/mask. Windows rendered parity is
unverified. Current focused shell_menu_window/panel_shell/file_menu_policy/
shell_context pass4/4 in2.24s macOS2026-10-03 after rebuilt targets; an earlier
seven direct consumer run passed7/7 in2.01s before the last preamble change.
Baseline now385 exact/189 adapted/3 local paths. The full unsandboxed
scripts/check.sh for this integrated checkpoint passes formatting, all-target
build and223/223 CTest in83.85s on macOS arm64 Qt6.11.2 2026-10-03.
Native COM/DLL exports/package/installed UX, program
File-menu plugin lifetime and remaining Add/email/generation are still mandatory;
the whole P2-09 iteration is not accepted and has no helper commit.

## P2-08 — System associations (accepted 2026-10-03)

Whole iteration accepted after source/body review and checks below. Baseline
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
`https://github.com/ip7z/7zip.git`. Source `FileManager/SystemPage.cpp/.h`
supplies EExtState, CModifiedExtInfo::SetState/GetString, CAssoc::GetIconIndex,
ChangeState's mixed-selection algorithm and OnApply's per-row rollback/first
error flow. These bodies are copied with project formatting. QWidget/QTreeWidget
replace Win32 property-page/list controls; typed IQtAssociationServices replaces
registry and shell notification entry points. Qt keyboard/mouse translation
preserves current-user/all-users commands, no-selection means all, and the
source no-modifier cell click. Source labels, column order and two buttons are
referenced to SystemPage.rc. The generated System resource reference and reviewed
contract cover all four controls at EN/RU640×390/520×380. macOS resource
comparison passes four reports with three declared adaptations on each of macOS,
Windows and Linux. All twelve current reports were reviewed for order, grouping,
bounds and clipping; rendered Windows pixel parity remains unverified.

Source `FileManager/FilePlugins.cpp/.h::CExtDatabase` extension lookup, SplitString
and merge loop are copied; Read receives the retained IFolderManager instead of
creating another Windows plugin manager. Commented legacy PluginLoader branches
and CE-only CAB omission are not built. Original SystemPage AddIcon environment
expansion/ExtractIconEx/DestroyIcon is retained on Windows; QIcon replaces its
image list and QFileIconProvider provides native non-Windows presentation.

Partial `UI/Agent/ArchiveFolderOpen.cpp` and `Agent.h` copy CCodecIcons::LoadIcons,
FindIconIndex, AddIconExt and CArchiveFolderManager::LoadFormats/GetExtensions/
GetIconPath. The source STRINGTABLE100 association catalog replaces the former
independent projection of every detection alias. Global loading is replaced by
the validated retained CCodecs catalog; COM opening remains with the existing
QtArchiveFolderManager/OpenArchive owner, not a second opener. The latter caches
the icon owner with its catalog lease, preserving source WasLoaded semantics.
Windows reads original loaded DLL resources through the copied Unicode module
MyLoadString/MyLoadString2 bodies in `CPP/Windows/ResourceString.cpp`; ordinary
Qt captions retain the compiled resource boundary. Non-Windows modules have no
PE resources: exact `Bundles/Format7zF/resource.rc` STRINGTABLE100 is compiled
through the existing resource generator, accepting numeric resource IDs.
External ELF/Mach-O icon tables are omitted because they have no Win32 resource
API; the internal official association table remains authoritative. No format
icon rendering fidelity is claimed on those systems. Open filters still use
the full source handler catalog; association extensions intentionally exclude
detection-only aliases such as docx. ProgramLocation.cpp has no functional body.

`RegistryAssociations.h/.cpp` copies CShellExtInfo data, original IsIt7Zip prefix
test and Windows ReadFromRegistry/AddShellExtensionInfo/DeleteShellExtensionInfo
bodies under _WIN32. Includes resolve through the existing upstream dependency
roots; Windows location/service/page consumers use consistent _UNICODE/UNICODE
so source CSysString records are UTF-16 across the interface. Native Windows
services now call these registration bodies and SHChangeNotify; effective modern
Windows UserChoice is preserved, with an explanatory Apply tooltip. Registration
does not force a default application change. The source per-row operation may
partially write before an OS error; the adapter does not promise atomic rollback.

New native adapters have no corresponding upstream macOS/Linux implementation.
Linux uses GIO installed desktop-entry/default MIME services for the current
user, verifies that the desktop executable resolves to the running application,
and refuses unknown MIME types. All-users writes require an administrator and
remain disabled. macOS uses NSWorkspace/UTType, requires macOS12+ and an .app
bundle, waits for its completion/consent result, and restores a previous handler
from application-owned history on Clear. An empty macOS default has no supported
API; missing restore history is refused. History-write failure can follow a
successful OS change and is reported rather than presented as rollback.

QtAssociationServices records native revision snapshots, refuses externally
changed records before Write, and updates equal-snapshot aliases sharing one
native MIME/UTType scope after its own success. This is a pre-write stale-state
check, not an atomic operating-system compare-and-swap. SystemPage refreshes a
refused row before another decision. Options index0/Apply/Help and MainWindow's
retained manager are connected; Apply disables reentrant Options actions and
shows errors without closing the dialog. Source Settings validation regains focus
after the native call. Menu/Folders/time captions wrap for native Windows font
metrics without changing source text/order or enabling unfinished shell controls.

Development changes no real system associations: injected fake services cover
detect/apply/revert, denied/absent/failed reads, aliases, external changes and
source/actual Options workflows. Final macOS unsandboxed check.sh passed formatting,
all-target build and219/219 tests67.46s. Later localized username/time-caption
changes passed affected9/9 tests3.21s; current Windows12/12PASS25.26s and
Linux12/12PASS3.17s include native adapter compilation and direct consumers.
Baseline384 exact/182 adapted/3 local verifies against the pinned checkout.
Expanded Windows Add/Editor path/geometry/lifetime and Linux Add geometry failures
remain P3 work; these focused results do not claim full Windows/Linux gates.

Native MIME/UTType rows now refresh together after the entire Apply loop; source
Windows independent-extension semantics remain unchanged. A fake public-page
test changes one alias, then clears it, and checks every shared-type cell.
Availability/tooltips are refreshed on language changes and after native reads.
The source OnInit GetUserNameW/256-character Unicode branch is copied into the
Windows platform adapter. POSIX getpwuid/NSUsername supply the equivalent header
on Linux/macOS; a localized fallback is used when unavailable. Fake Unicode
username tests preserve the header and pending state across language changes.
Time-only ZIP captions wrap at300 pixels; NTFS metadata keeps original single-line
mtime/ctime and380-pixel atime wrapping. This presentation adaptation preserves
the actual Windows WIM480×430 contract; four fresh EN/RU WIM reports were reviewed.

New `qtassociationfileopen.*` is native integration with no upstream macOS
counterpart. It retains early QFileOpenEvent requests, rejects nonlocal/empty
URLs and dispatches asynchronously after the receiver is connected. main.cpp
routes native requests through the existing MainWindow::openPath; an additional
window gives an already-running application an independent backend/lifetime,
matching the source Windows association launch's separate instance. Command-line
startup retains its existing source integration. Built .app metadata now declares
all source STRINGTABLE100 extensions as Viewer/Alternate, with a stable project
bundle identity instead of the previous placeholder. CMake's metadata projection
does not replace CCodecIcons' runtime parser and reconfigures when the source
resource changes. Qt/Apple document this native event/declaration boundary:
[QFileOpenEvent](https://doc.qt.io/qt-6/qfileopenevent.html),
[CFBundleDocumentTypes](https://developer.apple.com/documentation/bundleresources/information-property-list/cfbundledocumenttypes).
No development check registers OS defaults. Fake early/repeated events, remote
URL refusal, the actual opening consumer and built-bundle declaration are included
in the final checks above. An earlier full run timed out archive_external_lifetime:
an owned orphan fixture triggered unchanged source same-name process protection.
After its normal control-file exit, focused replay passed1/1 in3.11s and the full
219-test gate passed. No process-wait algorithm was changed to hide that failure.
Installed native default-app registration, rendered Windows reference and Finder
remain explicit P3 acceptance obligations; no push or upstream writes occurred.

## P2-07 — extraction integration (accepted 2026-10-03)

Whole iteration accepted: source extraction controls/persistence, bounded final
paths/links, native security/ADS/reparse restoration, metadata Add and processed
deletion, actual MainWindow consumers and directory detached startup. Full macOS
formatting/all-target/214 CTest passed81.10s; final affected4/4PASS1.45s and
lifecycle1/1PASS1.28s. Windows13/13PASS33.56s plus final lifecycle1/1PASS2.48s;
Linux final13/13PASS10.15s. Current EN/RU normal/compact Extract/Metadata JSON,
including four native WIM Options reports, reviewed. Baseline382 exact/175
adapted/3 local verified. Full Windows/Linux gates, rendered Windows reference
and Finder remain P3 obligations. Historical WIP checkpoints below are superseded
by this acceptance. Baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
`https://github.com/ip7z/7zip.git`. GUI `ExtractDialog.cpp/.h` restores original
GetBoolsVal/CheckButton_TwoBools/GetButton_Bools, ElimDup/NtSecurity and the third
path mode; Qt substitutes control reads/writes. Common ZipRegistry restores
defined-value persistence under original ElimDup/Security keys through QSettings.
ExtractGUI restores dialog→options metadata flow. Common Extract::DecompressArchive
remains the authoritative root-elimination algorithm, not a Qt replica.

Absolute extraction is bounded by the chosen destination. Raw traversal,
case/Unicode collisions and occupied link ancestors remain refused. IQtExtractPathPolicy in
CExtractNtOptions receives the source-corrected path from
ArchiveExtractCallback::GetExtractStream before folder creation; CreateFolders
moves below the check, with its body preserved. Qt validates containment and
filesystem ancestors, reporting a localized reason through source MessageError.
Batch-open validates each actual listing before its source extraction loop.
These checks are not an atomic defense against concurrent ancestor replacement.

Native link extraction retains original CLinkInfo normalization/rebasing,
SetLink/SetLink2 deferred creation, CLinkLevelsInfo safety and SetPostLinks
metadata flow. The same borrowed policy now checks the final origin and target
in CreateHardLink2 and the non-hard SetLink2 branch immediately before creation.
Qt only resolves a relative final target and validates containment/ancestors;
it does not implement TAR link parsing or upstream link-safety algorithms.
MainWindow and batch preflight permit archive links only after a matching native
service has been proved. A link remains a leaf for archive path conflicts,
including directory links; file-only transport continues to refuse links.
For Windows native ADS, preflight permits a separately validated base and stream
name only with a proven native service and the listing's Alternate Stream flag.
The final callback policy receives original CReadArcItem::IsAltStream alongside
the source-corrected path. ArchiveBackend validates the bounded base and its
occupied ancestors independently, including base links; ordinary colon paths
remain refused. Original CorrectPathParts/Correct_AltStream_Name, parent rename
mapping and stream creation remain unchanged. Whole native WIM extraction of
Unicode/spaced base and stream bytes passed Windows; the regression failed
before this integration adaptation. Original NtSecurity restoration passes a
Windows protected explicit DACL fixture with the option on/off; archive and
source rights remain intact. Native Add security-only passes as well. Adding
the first ADS exposed a stale CArc::Ask_AltStream after publication:
the source CProxyArc2 rejects its file parent when this old flag is false.
CAgent::ReOpen now refreshes ConfigureArc from the fresh native handler before
unchanged ReadItems. Windows ten focused suites pass (26.27s), including
independent security/ADS on/off, failure/Cancel preservation and processed-source
deletion after native publication.

Qt extraction destinations now use QDir::toNativeSeparators when entering batch
OutputDir, GUI dialog OutputDir and MainWindow CopyTo.folder. Source GetFullPath
preserves already absolute strings, and FillLinkData_WinLink embeds its input in
NT reparse data; forward slashes from Qt produced a visually resolved but unreadable
absolute target on Windows. Native WIM relative and absolute file/directory link
byte reads now pass without changing source link construction. Source Metadata
Options WIM EN/RU transaction/persistence/compact geometry passed Windows.
Last-access-time captions wrap and Qt vertical gaps are explicit, with source
order retained. Four current WIM JSON reports reviewed; RU compact dialog is
480×430 with475×423 size hint, metadata labels and footer fully exposed.
POSIX source_link_extract covers file/directory/dangling/hard links, common root
elimination, original absolute-link rebasing, unsafe targets, occupied ancestors
and overwrite Cancel with partial writes preserved. Windows native fixtures
pass, including WIM reparse links. Full macOS format/build/214 CTest passed
(81.10s); subsequent affected checks pass after localized Windows/Options fixes.
Current focused Windows11/11PASS40.19s and Linux13/13PASS10.51s do not establish
full platform or rendered Windows acceptance. Final expanded Windows checks
passed13/13 (33.56s), with final lifecycle1/1 (2.48s); Linux13/13 (10.15s).

QtArchiveFileResource proves native count/path order for any retained native
reader, then routes original IInArchive::Extract through ArchiveBackend. Levels
that cannot prove order retain the file service; RAR/hash mismatch still refuses
fallback. Hash keeps its safe-read guard. Source Agent Extract receives borrowed
CExtractNtOptions for selected-copy metadata/path checks while the modal worker
is joined. Whole-root extraction uses original ExtractGUI/Common Extract for
every overwrite mode. Selected/current-folder extraction preserves Agent source
semantics; ElimDup is disabled in that dialog context because Agent does not
perform Common root elimination.

Related BrowseDialog2 directory Open Outside : 7-Zip uses existing navigation.
Main.cpp supplies the launch path to openPath, resolving it against launch cwd
before native navigation. FM.cpp's active g_MainPath startup and BrowseDialog2's
original command are the reference; QApplication arguments replace Win32 parsing.
Detached launch retains unconfirmed external owned-temp protection.

The folder/process-plan bridge projects defined IsDir through original
Archive_IsItem_Dir, preserving its default/type rules for native handlers such
as gzip that omit this optional property. Other native fields remain authoritative;
missing values are not synthesized. This fixed the expanded gzip Add regression.

Standalone Windows ZIP/7z creation now refuses SymLinks before engine start.
The pinned `CPP/7zip/Archive/Zip/ZipRegister.cpp` and
`CPP/7zip/Archive/7z/7zRegister.cpp` do not advertise `kSymLinks`; the native
catalog follows those original capabilities. A real Windows standalone fixture
proved that file/dangling links survive CLI storage but a directory reparse link
becomes an ordinary directory, followed by deletion of the input link. This
integration guard prevents that loss; it does not assert all CLI links are
unsupported. POSIX standalone compatibility remains tested, and Windows native
TAR/WIM link storage uses source-advertised capabilities. The regression verifies
refusal, preserved raw links/target bytes and no output publication or engine start.

Creation now carries the original CompressDialog::CInfo SymLinks pair through
ArchiveCommandLine.cpp's kSymLinks/kMinus (`-snl`/`-snl-`) into the external official
engine. Catalog-backed creation requires the original Flags_SymLinks capability.
The pre-publication deletion snapshot uses Common/Update.cpp's CCensor plus
EnumerateItems sequence, preserving original leaf reparse-data handling (the
Agent EnumerateItems2 sequence omits it). QtArchiveDeleteInputs retains the
same SymLinks pair for the unchanged source UpdateArchive deletion body.
Links remain refused for deletion when storage was not requested. File-only
source-update staging explicitly refuses SymLinks because it cannot carry
reparse properties. Metadata Add now uses a native writer; Windows runtime
checks pass as recorded above.

CompressDialog restores original Combine_Two_BoolPairs/Set_Final_BoolPairs
bodies and four Info pairs (SymLinks/HardLinks/AltStreams/NtSecurity). Narrow
LoadMetadataOptions/SaveMetadataOptions entry points replace FormatChanged/OnOK
orchestration. Source CArcInfoEx flags determine support; original Compression
Security/AltStreams/HardLinks/SymLinks persistence uses QSettings. The Options
modal is a transaction; Add OK saves it. Unsupported requests clear their
command pairs while registry preferences survive format changes. Unix hides
NTFS streams/security.

AgentOut keeps its source update plan and callback bodies. Integration assigns
Common/Update.cpp's DirItems SymLinks/ReadSecure/ScanAltStreams and callback
StoreSymLinks/StoreHardLinks/StoreNtSecurity fields. Link-aware enumeration uses
source CCensor/EnumerateItems, including selected leaves; non-GUI relative names
retain their physical folder prefix. After successful writing, the existing
AgentOut processed-item loop additionally includes AreReparseData, matching
Common/Update.cpp::Compress's post-pass. TAR can write link properties without
requesting a stream, so this adaptation is necessary for original deletion.
Snapshot/pairs survive until publication and source no-follow deletion.

MainWindow scoped extraction now derives preflight names from the same source
Agent real indices used by CopyTo. Previously extracting a current folder with
an empty selection validated the entire archive listing and rejected unrelated
root entries. The existing GetRealIndices/property projection is reused; source
selection and extraction bodies are unchanged. The focused metadata UI suite
exercises actual Add/Options/native TAR update/processed deletion, selected
folder extraction and current-folder extraction in EN/RU, plus Add Cancel.
Windows/Linux native link fixtures use original FillLinkData/SetReparseData and
FileIO file identity, replacing Windows-wide test skips. Unix uses source
SetSymLink_UString and inode identity. Windows focused nine suites passed
(43.11s), including actual MainWindow metadata Add/Extract, native link storage,
cycle/dangling/directory links and source update consumers. This is not a full
Windows gate or whole-step acceptance; the later security and whole-archive ADS
checks recorded above supersede that checkpoint's open consumers.

At the Windows source-to-process mutation boundary, ReadPath/ReadNewPath now
call original NArchive::NItemName::ReplaceSlashes_OsToUnix. The source Sync
callback correctly retained the child while deleting parent directory records;
unnormalized native separators defeated FindMutationPlan's descendant guard,
causing recursive CLI deletion. Regression uses a real opened native archive,
observes original DeleteOperation, and verifies preserved child bytes for ZIP/7z.
QtPanelFolder projects transport paths with original GetOsPath/GetOsPath_Remove_TailSlash;
custom presentation names retain their own representation.

Options wraps PreserveATime and time-precision captions to preserve source
control order at compact Windows sizes. Add geometry settles the scroll layout
before inspecting the complete Parameters field; QLineEdit cursor visibility
alone does not prove whole-field exposure. Functional source option bodies are
unchanged. No rendered Windows reference has been compared.

Runner compilation prerequisites found by the current full snapshots: Qt6.4
has no QTimeZone::UTC enum, so fixture constructors use equivalent QTimeZone(0).
A Windows MOC/compiler mismatch in filesystem_cleanup is fixed by retaining
the slot on both platforms and guarding its POSIX-only body. Process FileName
loads QueryFullProcessImageNameW dynamically via original Z7_GET_PROC_ADDRESS,
following PanelItemOpen's loader pattern without raising inherited SDK targets;
this Qt service retains DOS-path output for file comparison. Upstream's PSAPI
helper returns device paths and cannot directly provide that contract.

ArchiveBackend shares its validated source CProperty projection between CLI
and native transports. Metadata Add calls original SetProperties then
IOutArchive::UpdateItems with the retained catalog's detected format/capabilities,
not an open-type hint or a later catalog. Native GetFileTimeType supplies source
update comparisons. File-only staging refuses unrepresentable metadata.
ZIP/7z Register.cpp omit kSymLinks despite CLI link support; GUI/native choices
preserve that distinction. TAR is the verified POSIX metadata consumer.

source_link_storage covers TAR Add/link roundtrip, target preservation,
publication then processed-input deletion, native failure/cancel preserving
archive/input bytes, hard-link inode identity, ZIP/7z capability refusal,
Options Cancel/OK, restart/format filtering and EN/RU540×480/480×430 geometry.
Tests caught the missing reparse post-pass and incorrect assumptions about
reopen listing counts and QLabel-only geometry helpers. Final focused six
suites PASS6.02s. Full unsandboxed format/all-target build/211 CTest PASS81.52s.
After the localized native-phase progress fix, four direct suites PASS2.55s:
source_link_storage, codec_update, native_comment, source_extract_advanced.
The metadata transport detects its native phase from retained settings because
PanelCopy uses the default progress-service call; source Sync owns Pause/Resume.
The fixture verifies idle backend/unchanged archive during pause before writing.

Latest macOS checks: extraction17/17 PASS7.67s; creation/direct-consumer6/6
PASS4.46s; final layout/split/TAR3/3 PASS1.00s. Full unsandboxed check.sh passes
formatting/all-target build/210 CTest81.51s after correcting legacy mode-count,
basic-CLI option and modal-completion test premises. Baseline382/175/3 verified;
four EN/RU700×390/600×340 Extract JSON reports inspected. Windows/Linux
native security/metadata, links/streams extraction, archive link storage and
processed-input deletion, Windows source-update failures, final UX/full gate
and acceptance remain mandatory. No whole-step commit yet. P2-06 below describes historical acceptance.

## P2-06 — source ADS/streams acceptance (2026-10-03)

Baseline for these modules is
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
`https://github.com/ip7z/7zip.git`. Complete original
`CPP/7zip/UI/FileManager/AltStreamsFolder.cpp/.h` is copied under `src/`:
enumeration, stream properties, binding, cloning, comparison, notification,
CreateFile, Rename, Delete, CopyTo and Windows FileIO/FileFind calls retain
their original bodies. Includes, formatting, sort overload and copy diagnostic
types adapt to the existing authoritative portability owners. Original
CreateFolder, SetProperty and active CopyFrom/CopyFromFile return E_NOTIMPL;
commented upstream import implementations remain commented, not invented.
The module is built only on Windows; Unix xattrs are not NTFS ADS.

`FSFolderCopy.cpp::CCopyStateIO::MyCopyFile(CFSTR,CFSTR,DWORD)` restores the
source 64 KiB CByteArr/native FileIO loop, original HRESULT/IProgress and source
deletion. POSIX uses the original FileIO ReadFull/write_full interfaces.
Create_NEW replaces upstream Create_ALWAYS for exclusive owned stages.
INVALID_FILE_ATTRIBUTES suppresses ADS base attribute/time changes. Ordinary
file metadata and link checks retain the previously accepted transaction
adaptation. The QString overload only supplies native paths/attributes and the
existing Qt cancellation predicate. One authoritative PanelSort remains in
sevenzip_panel; duplicate per-consumer source registration is removed.

`RootFolder.cpp` restores the source colon-tail namespace and
`PanelFolderChange.cpp::OpenAltStreams` retains its interface/bind/fallback flow.
The Win filesystem fallback is disabled on Unix. Qt only dispatches the source
command, renders folder names/properties/indices and refreshes the namespace.
Filesystem ADS Copy/Move/Delete/CreateFile/Rename use existing CPanel workers;
Properties uses the existing source dialog. CDisableNotify prevents stale row
callbacks during Move reload. Native item paths select newly created/renamed
streams; the ADS label editor uses the source stream name.

`Agent.cpp` restores the original CProxyArc2 branches in LoadFolder/LoadItems,
Init/count/name/prefix/full-prefix/properties, folder/parent/ADS binding and real
index expansion. Existing AgentProxy.cpp/.h remain the sole proxy algorithms.
The Qt GetItem wrapper preserves original flat/non-flat index mapping; invalid
public indices and unbound ownership are guarded. ReadItems retains original
GetRawProps+IsTree selection and source RAM checks; callback-null load progress
remains P2-10. ReOpen copies the active Restore FolderItem body from
ArchiveFolderOut.cpp::CommonUpdateOperation, applying it to the newly opened
owner and preserving the source root-alt/file-alt sentinel semantics.

QtArchiveFileResource/Properties retain native tree flags, raw parents and item
properties only after exact count/path-order proof. Arbitrary native binding
was rejected because TGZ levels differ; tree, ZIP-comment, RAR and hash readers
keep their explicit supported boundaries. Qt rows consume the folder interface,
not a CProxyArc-only pointer. Backend listing adds original Console `-sns` so
archive ADS are present for order proof. The CurrentPaths validation adapter
admits only declared non-link ADS in the matching colon folder and validates
the plain output stream name, retaining collision/link/unsafe-base guards.
Only separators are normalized for selected/current/native paths; raw metadata
and diagnostic names stay intact. Console password recognition accepts both
original UserInputUtils.cpp prompt variants, including Windows echo suppression.

The file-only Console extraction adapter carries native `kpidIsAltStream` into
an explicit `-sns` switch only for stream items. Unix otherwise skips ADS even
with stdout extraction. The original callback owns output naming; ordinary
payloads retain stream exclusion. A synthetic Windows WIM fixture (provenance
in tests/fixtures/streams/README.md) now checks actual ADS on Unix as well.
On macOS, archive names whose UTF-8 differs from QFile::encodeName are passed
in a private UTF-8 selection list instead of argv: QProcess filesystem encoding
decomposes names, which must not change archive matching. The list has an owned
temporary lease through process completion. Names containing line breaks cannot
use this list transport and receive an explicit error; native extraction remains
required for that combination. This is a process boundary adaptation, not a
change to source comparison or normalization rules.
The Qt boundary is documented by
[QFile::encodeName](https://doc.qt.io/qt-6/qfile.html#encodeName) and the pinned
[Qt 6.11.2 QProcess argument conversion](https://github.com/qt/qtbase/blob/v6.11.2/src/corelib/io/qprocess_unix.cpp).
Use `-i@list` before `--`: positional `@list` after `--` is treated literally.

Native tree output is connected through ArchiveBackend to the original
handler IOutArchive::UpdateItems and original Agent/ArchiveFolderOut callback
and transaction. This avoids flattening source parent/ADS indexes into a CLI
mutation plan. Portable actual WIM Rename/exact19byte extraction passes on
macOS, Linux and Windows. Windows actual ADS Rename/Delete Cancel/Yes retains
the source final-stream fallback and passes official archive Test/exact main
bytes. Final Windows archive suite1/1PASS6.60s Qt5/5zero skips after forcing
the transferred fixture's VM mtime: source SHA256 matched local, but older moc
had omitted the new slot. Final Linux nine suites PASS23.82s, all Qt cases zero
skipped. Sixteen EN/RU980×620/640×400 JSON reviewed across mac/Linux archive,
Windows archive and Windows filesystem ADS: address/table/status fully visible,
source grouping intact. Baseline382exact/175adapted/3local verified.
Final macOS arm64 Qt6.11.2 unsandboxed gate passed formatting, all-target build
and207/207CTest120.58s. Earlier failed checkpoints are superseded. Final VM
checks exposed a Qt integration bug in updateSourceAdd: canonical QFileInfo paths
use '/' on Windows too, so a native QDir::separator containment suffix rejected
prepared files. The narrow fix keeps the containment guard and source callbacks;
the actual archive-to-archive drag regression now checks added payload bytes.
Disposable drag rows isolate their ownership registries, retaining the original
unconfirmed-receiver protection assertions without leaving shared test records.
Final localized consumer checks and whole-step acceptance are recorded below.

P2-06 is accepted as a whole iteration. Final localized macOS drag/source-update
consumers2/2PASS4.43s; Linux drag/contract/source-update3/3PASS3.21s, zero Qt skips.
Windows final drag/contract2/2 passed (Qt9+4 zero skips); the seven relevant Add,
directory/multiple input, Move, Cancel, protected archive and unsafe-path cases
passed through the project CTest wrapper1/1PASS1.39s (Qt9 zero skips). Earlier
Windows14-suite regression passed12 suites61.23s and Linux17-suite passed16
45.60s; their drag/contract failures are superseded by these focused results.
The expanded Windows source_update_backend suite remains failing: slash-sensitive
listing assertions, synthetic Sync metadata yielding an empty archive, absolute
path expectations and a30s timeout. This is retained for P2-07/P3-02/P3-03;
no complete Windows gate or rendered Windows parity is claimed. Counts35/46;
nextP2-07. Finder remains the separate mandatory P3-03.1.

## P2-05 — source links/metadata acceptance (2026-10-03)

P2-05 is accepted as a whole iteration. Final unsandboxed macOS arm64Qt6.11.2
check.sh passed formatting, all-target build and205/205CTest82.95s. Final focused
Windows11 ARM64Qt6.8.3/MSVC14.44 seven suites PASS30.24s, all Qt cases zero
skipped; Linux ARM64Qt6.4.2/GCC13.3 seven suites PASS0.15s, zero skipped.
Twelve current EN/RU520×500/360×300 JSON across the three platforms reviewed;
all controls become exposed through compact scrolling and footer remains visible.
Baseline382exact/173adapted/3local verified. Counts34/46; nextP2-06. This is not
full P3 acceptance: native FinderP3-03.1, rendered/full platform checks remain.

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
`https://github.com/ip7z/7zip.git`. Complete original
`CPP/7zip/UI/FileManager/LinkDialog.cpp/.h` copied into mirrored source paths,
including CApp::Link, default link selection, GetSymLink reproduction validation,
path/type/nonempty-file checks, browse and original hard/reparse operations.
Qt replaces modal/combo/radio/control/message/resize services. Exact original
LinkDialog.rc/Res.h are copied unchanged. Win32 OnSize rectangles are replaced
by the original grouping/order in Qt layouts with a compact scroll pane.
English resource captions are retained by dynamic resource-language bindings;
Cancel uses the original standard402 mapping.

Existing adapted Windows/FileLink.cpp and FileDir.cpp already retain original
hardlink, reparse encoding/parsing/device-control and POSIX readlink/symlink/link
bodies. They are reused, not reimplemented. POSIX does not have Windows reparse
data: OnInit reads the original raw readlink target and classifies the target
only for the presentation's file/directory choice. A symlink inode itself is
exempt from the Windows stored-directory-attribute type check. Junction/WSL
radio controls are disabled outside Windows. Empty plain POSIX files are refused
instead of hiding their bytes behind a reparse attribute. Editing an existing
POSIX link creates a private same-parent link and uses the source rename adapter
atomically, preserving the old link on failure. Clearing it unlinks the POSIX
inode; Windows DeleteReparseData retains its original behavior.

Actual File menu dispatch for one operated filesystem item, selection restoration,
create/Cancel and multiple-selection/archive refusal are verified. Fixtures cover
raw relative/absolute/dangling/cyclic/directory links, Windows junction, hardlink
shared bytes and inode identity, retarget/clear, denied process privileges on
Windows and POSIX parent permissions, copy/move/delete/cancel and metadata.

FM.cpp's original `g_SymLink_Supported` and `Set_SymLink_Supported` are added
to its existing partial port, preserving WM_CLOSE/WM_COMMAND/IsLargePageSupported
bodies. Exact original `Windows/MemoryLock.cpp/.h` supply EnablePrivilege and
EnablePrivilege_SymLink, including the original process-token privilege checks;
Windows builds those files with consistent Unicode definitions. Qt calls this
source initialization once when creating the Link dialog. POSIX has symbolic
links without Windows privilege initialization. This does not port all FM
startup or enable the large-pages setting.

Integration retains source CFsFolderStat/CopyTo/
CopyFileSystemItems/CopyFolder control flow, replacing the previous blanket
link refusals with native IsOsSymLink leaf handling. This explicitly deviates
from upstream Windows recursive CopyFolder/CopyFileEx behavior to meet the
port's no-implicit-follow requirement: directory links/junctions do not recurse;
raw GetReparseData is cloned by source SetReparseData (Windows) or SetSymLink
(POSIX), then source SetLinkFileTime preserves link timestamps. POSIX raw target
bytes remain unchanged, including relative/non-UTF-8 targets. New owned-stage
platform glue is necessary because there is no upstream portable raw-link clone
operation. Same-filesystem Move uses original MyMoveFile/rename; transaction
fallback clones the object, publishes it, and deletes only the source link.
Destination links remain refused by the transaction boundary. Original Windows
FSFolder IArchiveGetRawProps bodies, Reparse member and LoadSubItems reparse/file
identity capture are restored; failed capture does not read uninitialized handle
information. POSIX reports no NT raw property. CalcItemFullSize skips link leaves.

Compact Windows EN/RU captions exceeded viewport minimum
width; existing languageWrapWidth binding now wraps radio captions at220 pixels.
Resource text/order retained; geometry is structural/usable proof, not rendered
7zFM pixel parity. Failed target-open assertions account for different platform
QFileInfo dangling-exists semantics. Windows EN/RU520/360 JSON reviewed: footer and radio content
exposed, long captions wrap within280-pixel compact controls; upper path controls
are naturally outside the final bottom scroll position but were individually
exposed by the geometry test. Link-copy now reserves Windows empty objects with
original CreateDir/Create_NEW before SetReparseData; a failed POSIX exclusive
symlink never removes a racing destination. POSIX target type validation uses
Find_FollowLink solely to classify the chosen target; mutation still stores raw
target and does not traverse it.

Source POSIX MyMoveFile delegates EXDEV to byte-copying MyMoveFile_with_Progress;
that path would follow a link. QtRenameFileSystemEntry therefore supplies only
rename on POSIX and reuses original MyMoveFile on Windows. Native link Move and
the staged consumer explicitly clone raw reparse/readlink data on rename refusal,
check cancellation before deleting the old link, and remove their new object on
failure. Linux tests use private temporary roots on different /tmp and /dev/shm
devices, verifying real EXDEV for the source primitive and staged consumer.
Ordinary buffered file copies now use the existing original SetDirTime and
SetFileAttrib_PosixHighDetect after closing/flushing output, preserving supported
timestamps and attributes/mode; metadata failures reach the copy error boundary.
New copied folders restore metadata after child creation. Existing merge folders
retain their own attribute policy. This is an explicit P2-05 adaptation: upstream
CopyFolder does not restore portable directory metadata. Unix inode identity,
ownership and ctime are OS-created values; symlink permissions are OS-specific,
and link timestamps use no-follow SetLinkFileTime. No ownership impersonation or
extract metadata policy is added here. Archive Add link storage/deletion and
extract security/metadata remain separate backend capabilities; the filesystem
Link command does not enable unavailable archive workflows.

## P2-04 — source Benchmark acceptance (2026-10-03)

P2-04 is accepted within the scope below; this final record supersedes all WIP
checkpoints in this section. Complete source Common/Bench and GUI/BenchmarkDialog
own the native calculations, callbacks, state, properties, memory policy and
worker lifecycle. Backend providers/services and Qt controls are connected to
the real Tools menu; success, failure, cancellation, Restart/Stop/Close, deferred
MainWindow close and source Help are checked. No CLI output formula substitutes
missing source metrics. Total mode remains callable through the original entry
without adding its UNDER_CE-only command to the desktop Tools resource.

Final evidence: unsandboxed macOS check.sh formatting/all-target build/202CTest
PASS89.68s, followed by localized text geometry/Help fixes' three affected suites
PASS5.82s. Linux fivefocused suites PASS3.34s, all Qt cases zero skipped, then
final three affected suites PASS2.28s. Windows fivefocused targets build and
CTest PASS4.09s, all Qt cases zero skipped. Baseline378exact/171adapted/3local
verified; all checks2026-10-03, Qt6.11.2/macOS arm64, Qt6.4.2/Linux arm64,
Qt6.8.3/MSVC14.44/Windows arm64. Eight EN/RU normal/total JSON per platform
reviewed:960×650/640×400, exposed footer/console, named controls reachable by
scrolling and fitting text. Compact top-state system rows are intentionally
below the viewport; geometry tests scroll to expose each one. Rendered Windows
pixel parity and whole-plan platform acceptance remain P3.

Windows found three integration issues and the final tests verify their fixes:
the original FM startup large-page flag dependency; spanning labels stretching
beyond a compact viewport; Total worker completion preceding its queued UI text.
The Qt label adapter retains natural-width group/system/error captions and the
wrapped label's required height after source SetItemText. The test waits for the
visible Total output before closing. The original numerical/state bodies remain
unchanged. This is a whole iteration; helper commits were not made.

### Source scope and earlier checkpoints (superseded by final evidence above)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
`https://github.com/ip7z/7zip.git`. Complete `CPP/7zip/UI/Common/Bench.cpp/.h`
copied into the mirrored source path. Only canonical includes, provenance and
required clang-format differ; original benchmark generation, engine invocation,
callbacks, timing, memory policy and ratings remain authoritative. Exact
`CPP/Windows/SystemInfo.cpp/.h` copied unchanged into third_party; existing
System/Registry/CreateCoder/MethodProps dependencies are reused. No new digest,
compression or rating implementation is written.

The bounded official CLI probe (`b -mmt=1 -md18 -mtimems=50 1`) succeeded, but
its text output lacks all Current UnpackSize values used by the original dialog.
The native Bench callback exposes these source metrics directly through the
retained official codec catalog. The new focused native_benchmark fixture probes
actual encode/decode callbacks and cancellation with256KiB/1thread/50ms settings;
it imposes no golden performance numbers. sevenzip_common and its focused test
target build successfully on macOS. Focused native_benchmark1/1PASS0.46s,
four Qt cases passed/zero skipped, macOS arm64Qt6.11.2. Its first discovery
attempt using the default engine location failed before Bench; the fixture now
selects the explicit official7zz as existing native catalog tests do. Baseline
376exact/167adapted/3local verified. GUI BenchmarkDialog port, menu consumer,
memory refusal, Stop/Restart/Close, geometry, platform checks and full gate remain
mandatory; this prerequisite does not accept P2-04 or advance package counts.

Subsequent WIP now copies complete GUI/BenchmarkDialog.cpp/.h: CSyncData,
CBenchProgressSync, CThreadBenchmark, CBenchCallback/CBenchCallback2/CFreqCallback,
property parsing, memory refusal, pass/ratings aggregation, UpdateGui and
Restart/Stop/Cancel/finish-join bodies are retained. Qt replaces CModalDialog,
combo/edit/font/control resizing and message/timer services through
CQtBenchmarkWindow; layouts and fixed-width fonts replace Win32 rectangles and
CMyFont. POSIX uses the original CProcessAffinity Get/processor getters because
the source Windows-only combined getter is absent there. GetTickCount uses a
monotonic QElapsedTimer. CBenchCallback2::CheckBreak now reads Exit under the
same original critical section used by SendExit, avoiding an unsynchronized
cross-thread Qt cancellation read. Exact BenchmarkDialog.rc/Res.h are vendored.
The RC has no STRINGTABLE; adding it to rc_strings.cmake was disproved by the
generator's required table contract. Its original captions are presentation
fallbacks in the Qt adapter; that generator is unchanged.

CompressCall2.cpp/CompressCall.h partially copy only the original Benchmark
entry/declaration. Its totalMode `m=*` properties and default iteration count
are retained; CREATE_CODECS/LOAD_EXTERNAL_CODECS/global HWND are replaced by
scoped backend catalog providers and QWidget. Other compression/extract/checksum
call wrappers from these files remain omitted; their existing workflow owners
are not accepted by this partial file. MyLoadMenu's original two-panel timer
guards are retained in Qt dispatch. Benchmark action is connected and enabled
only when source FindMethod_Index confirms native LZMA encoder/decoder. The
immutable catalog outlives all provider/callback references through worker join.

sevenzip_panel/native_benchmark target builds on macOS. Actual bounded native
GUI Benchmark completes, presents original resulting GIPS and joins before close;
focused1/1PASS5.57s at the first GUI checkpoint. Further deterministic lifecycle,
memory refusal, actual menu, language/geometry/JSON, cross-platform and final
gate checks remain open; no P2-04 acceptance or helper commit.

The backend now owns `IQtBenchmarkServices`: its production adapter delegates
unchanged arguments to original `Bench` and `NWindows::NSystem::GetRamSize`.
Only those two calls in BenchmarkDialog cross this narrow engine/platform seam;
the source property, memory, callback and result bodies remain authoritative.
Services/catalog live through worker join. Original CompressDialog.cpp is also
registered in sevenzip_panel to reuse its sole `SetErrorMessage_MemUsage`
implementation; the CMake consumer link rule excludes the owner itself.

Deterministic public workflow tests now cover original metrics, total console,
Restart→Stop→Close joins, memory refusal without an engine call, engine errors,
normal/compact geometry and EN→RU→EN while open. Qt construction is split into
normal pane, metrics and summary builders; resource text bindings preserve
source captions and original LangUtils standard button mapping402/409. A first
geometry test incorrectly expected a shortened Russian title; corrected to the
source translation. JSON review also caught control2 translated as language2;
the original control→language mapping fixed it with a Cancel assertion.

Actual MainWindow Tools→Benchmark tests use confirmed native providers and the
backend service seam, checking provider absence, cancel/error, modal lifecycle,
join before services release and window usability afterward. The fake RAM32MiB
initially caused the source's default-memory refusal before its worker; the
menu fixture now reports8GiB without allocating it. Latest focused3/3PASS5.57s
on macOS arm64Qt6.11.2 before the added active-modal MainWindow close assertion.
Current eight normal/total EN/RU widget JSON reports reviewed, resource structure
and Qt usability only. Cross-platform and full gate are still pending.

The macOS unsandboxed full gate subsequently passed formatting/build/202CTest,
89.68s. Linux final fivefocused suites passed3.34s, all Qt cases zero skipped;
normal/total EN/RU JSON reviewed with exposed footer/console. The later source
Help callback assertion passes macOS benchmark_lifecycle1/1,0.52s.
Windows first linking exposed `Bench.cpp::Add_LargePages_String`'s startup
dependency `g_LargePagesMode`. The original `FM.cpp` default definition(false)
is retained in the platform services adapter. Qt startup does not enable the
LockMemory privilege; the large-pages option is disabled. The rest of FM startup
and SetMemoryLock are not ported by this definition. Bench's diagnostic body is
unchanged. Windows rebuilding and final focused checks remain mandatory.

## P2-03 — source CRC/Hash/Test acceptance (2026-10-03)

Baseline for this package: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
retrievable from `https://github.com/ip7z/7zip.git`. This section supersedes the
earlier partial HashCalc/HashGUI debt statements for the bodies now copied.
P2-03 is accepted within this scope; whole-plan acceptance remains open.

- `CPP/7zip/UI/FileManager/PanelCrc.cpp`: copied CDirEnumerator, CThreadCrc,
  CApp::CalculateCrc2/CalculateCrc, scan/read passes, error counts, aggregation,
  progress and final result routing. Qt replaces HWND; POSIX uses the original
  CEnumerator CDirEntry/Fill_FileInfo and CInFile::ReadFull interfaces. Original
  Panel.h CMyBuffer is retained. Local external-provider parameter replaces
  LoadGlobalCodecs/global storage; its caller retains the immutable catalog.
  Filesystem symlinks/reparse objects remain refused at the existing port boundary.
- `CPP/7zip/UI/Common/HashCalc.cpp/.h`: complete source HashCalc, Hash bundle,
  line generation/parsing, NHash::CHashPair/CHandler reader/writer/verifier,
  PGP cleartext reader branch and Codecs_AddHashArcHandler are now copied.
  Includes/formatting are adapted. NHash::CHandler owns a CExternalCodecs context
  through ISetCompressCodecsInfo instead of reading a mutable global pointer;
  AddDefaultMethod receives that context explicitly. The original source has
  no cryptographic signing implementation: its PGP branch is read-only, not
  a new signature generator or a promise of signature authentication.
- `CPP/7zip/UI/GUI/HashGUI.cpp/.h`: complete CHashCallbackGUI/HashCalcGUI worker
  and original callbacks, result formatting and ListViewDialog flags/order.
  QWidget/OnInit/exec replace HWND/Create; functional hash bodies are retained.
- Exact dependencies `CPP/Common/DynLimBuf.cpp/.h` are vendored unchanged.
  CMake moves existing EnumDirItems, SortUtils, FileLink, UniqBlocks and
  ProgressUtils dependencies into sevenzip_common so the complete HashCalc
  module has no common-to-filesystem cycle. Paths and authoritative files stay
  unchanged; digest implementations are the previously borrowed original ones.

MainWindow now invokes source CalculateCrc2 with scoped catalog providers.
Archive selections use source CopyTo streamMode/hashMethods and original hash
extract callbacks. The engine-only transport can still spool one unnamed stream
privately before CopyStream feeds that callback; there is no application-level
extract-selection-then-CLI-hash workflow. Native Hash/RAR handlers use the typed
ArchiveBackend extraction boundary after item-order verification. CAgent Init
retains source hash-handler flag and physical hash-file directory prefix.
The local hash-read guard rejects unsafe manifest paths and inner symlinks/
reparse objects before source callbacks read external files. Qt only adapts
process/thread ownership, cancellation/pause and presentation; it implements no
digest algorithm. Backend hash/hashMethods now run original HashCalc in an owned
worker; provider/catalog lifetime outlives all retained IHasher instances.
Native workers accept the source progress dialog's background state in the host
process, just as the existing volume worker does. Windows build consumers exposed
two header dependencies: SplitDialog explicitly includes MyTypes for UInt64;
QtFileManagerCommands explicitly includes MyWindows before its service interface,
matching its workflow implementation's Win32 CreateFile macro spelling.
No original source symbol is renamed. A comparison with the exact baseline after
identical clang-format confirms that the three complete borrowed bodies differ
only at the recorded include, Qt/platform/provider and link-boundary adaptations.
The newly built Windows selection consumer also exposed a prior Rename hint
adapter that split source paths only on `/`. It now obtains the original panel's
GetItemName(kpidName), preserving the source folder's name policy on Windows and
Unix. The existing forged-display-path regression validates that result; the
transport lifecycle assertion checks cancellation HRESULT and a nonempty localized
message instead of assuming the host language is English.
Source method lookup determines enabled original menu entries; the upstream
menu's fixed command list/order is preserved. PanelCopy now propagates SetMethods
failure rather than silently running without the requested hashers.

Initial macOS arm64 checks: source workflow/bundle/results, checksum, selection
and binding lifecycle 6/6 PASS2.77s; subsequent checksum (encrypted/wrong password/
Cancel/corrupt ZIP) and native hash-file UI (good/CRC/missing/unsafe) 2/2 PASS0.77s,
zero skips. Source hash-file write/read/verify/error/cancel and multi-method/`*`
worker checks pass. These focused results are dated evidence, not a full gate.
Final unsandboxed macOS arm64 Qt6.11.2 check.sh passed formatting, all-target
build and199/199 CTest79.48s. Later localized MyWindows/Rename/lifecycle fixes
passed their five direct consumers2.79s. Linux ARM64 Qt6.4.2/GCC13.3 focused
12/12PASS3.82s and final five direct consumers5/5PASS3.28s, zero skipped cases.
Windows11 ARM64 Qt6.8.3/MSVC14.44 built13 targets and passed13/13 CTest13.04s,
zero skipped cases. Baseline374 exact/165 adapted/3 local verified. Current-build
hash-results540/420 JSON reviewed: list/footer/buttons fully visible, source
control order and flags retained. No rendered Windows/pixel parity claim.
Whole-iteration local commit follows; no push or upstream writes.

The first full gate passed formatting/all-target build and198/199 CTest; the
filesystem_selection_runtime fixture still expected old fake-backend hashing and
waited on the new modal results. It now checks actual source file totals/size and
absence of errors after forged display paths, across four ordinary/flat views;
it asserts no backend hash call. Its45s timeout bounds a future modal regression.
Intermediate targeted macOS3/3PASS1.64s and Linux3/3PASS1.35s then passed;
the successful repeated full gate and final platform results above supersede
this failed checkpoint. FinderP3-03.1 and remaining mandatory packages stay open.

## P1-17 — ZIP item comment acceptance (2026-10-02)

Accepted whole iteration after unsandboxed check.sh1910 on isolated own-scope
snapshot `/private/tmp/sevenzip-p117-review-h0ufqfu9`: formatting, all-target build
and191/191 CTest PASS86.83s macOS arm64 Qt6.11.2. All162 owned source/build-module/
test files match current checkout byte-for-byte; only unrelated runner hunks were
excluded. Shared checkout check.sh43923 passed formatting/build and191/192 suites
88.64s, failing solely foreign runner_vm because tests/runner_vm_tests.py was
removed concurrently. Foreign code/docs/index hunks preserved outside this commit.

Original source ZIP writer and dependencies, CommentItem/SetProperty/ChangeComment,
ArchiveBackend/native factory/property-order/transaction/reopen adapters and actual
GUI consumer are complete for per-item ZIP comments. Ordinary/AES success,
writer failure, cancellation, clear/raw Unicode/spaces, metadata/input bytes,
independent extraction, nested parent confirmation and native pause/cancel checked.
Four current EN/RU Combo JSON reports360×150/260×150 reviewed. Baseline351 exact,
163 adapted,3 local paths verified against recorded0766b733 revision. Detailed
source scope, deviations, omitted registration/export layer and historical
checkpoints below remain authoritative; earlier pending/disabled states superseded.
Windows/Linux runtime and rendered Windows appearance remain unverified P3 scope.
Counts29/46; next P1-18 source RAR memory consumer. No push/upstream mutation.

## P2-01 — native codecs/formats acceptance (2026-10-02)

P2-01 accepted after full unsandboxed check.sh12893: formatting, all-target build
and188/188 CTest PASS74.09s macOS arm64 Qt6.11.2, 2026-10-02. Baseline verified
224 exact/161 adapted/3 local paths at the recorded0766b733 revision. This whole
iteration is committed locally as `Port native codecs and archive format workflows (P2-01)`;
no helper commit, push, publication or sibling-upstream mutation. The evidence
below preserves intermediate checkpoints and corrected approaches; those earlier
pending-gate statements are superseded by this acceptance. Source code scope,
narrow Qt/platform/process adaptations and actual capability limits remain as
recorded. Windows/Linux runtime and rendered Windows parity are unverified and
mandatory in P3; native ZIP comment/RAR decoder GUI consumers P1-17/18 remain open.

Final MainWindow continuation uses actual native metadata in Add and source
Inside */# actions. `codec_commands_ui` covers all seven writers through the
real Add dialog (success/cancel, exact Unicode/spaced binary bytes), real content
opening/parser and read-only commands, missing/mismatched module read fallback,
and single-stream Delete results/cancellation. Add restoration retains the
detected existing type while a selection is empty/ineligible, then emits the
actual previous→current format transition so original parameter controls refresh.
This is Qt source-picker integration, not a replacement eligibility algorithm.
Fourteen current Add reports at760×720/640×480 were reviewed; fixed detected
format, source methods and exposed footer are checked. Windows rendered parity
and Windows/Linux runtime remain unverified.

Copied mutation input must retain its original basename in a private subdirectory:
changing pathless bzip2/xz DefaultName would make the CLI selector miss the source
item and incorrectly report success. The native handler algorithms are unchanged:
Bz2Handler.cpp::UpdateItems rejects zero items, while XzHandler.cpp::UpdateItems
calls Xz_EncodeEmpty and its GetNumberOfItems still reports one. Tests verify those
actual distinct outcomes and cancellation, with preserved source bytes on refusal.
Exact private paths use -sae, corresponding to original
ArchiveCommandLine.cpp::ParseArcNameMode and Update.cpp::CArchivePath::ParseFromPath
(k_ArcNameMode_Exact). This prevents CLI extension appending for extensionless
owned spools; a disposable actual-engine reproduction and owned_temp_publication
verified the adaptation. The first full gate found that spool regression; the
subsequent focused4/4PASS4.00s and final control/publication4/4PASS5.66s passed on
macOS arm64 Qt6.11.2 2026-10-02. The repeated full gate is pending at this checkpoint.
The small output-path adaptation remains in existing UpdateArchiveSourceStream;
splitting that existing broad orchestration is deferred to its native callback
transport work rather than combining a structural rewrite with this fix.

Latest native opening continuation copies the complete
`CPP/7zip/UI/Common/OpenArchive.cpp` from
[0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
superseding the earlier property/parser-only fragment. Original NParser handler,
signature ordering/scanning, CArc::PrepareToOpen/OpenStream2/OpenStreamOrFile,
zero-tail checking, native nested CArchiveLink::Open/Open2 and ReOpen bodies are
retained in their original owner and control flow. Exact SetProperties.cpp/.h
are added under third_party at the same source paths/revision; source notices and
the existing applicable license are preserved. Compile-time SFX/Windows branches
are retained but not claimed exercised by the local non-SFX macOS build.
Required deviations are canonical include owners, provenance notices, the
existing optional QtOpenService early branch, and sharing the already copied
default-name block via CArc::SetDefaultName. Native OpenStream calls that same
body; it is not duplicated. A fresh exact upstream snapshot formatted once with
the repository style was compared against the adapted file: only these recorded
changes remain. The sibling checkout was not changed.
`native_archive_opening` directly executes native Open3 with actual module
handlers, without QtOpenService: ZIP unknown extension, explicit 7z, gzip/TGZ
AddExt, concatenated ZIP parser, forced-type refusal and source callback
cancellation/finished. These direct tests alone do not prove GUI integration.
The application continuation now calls original native Open3 in the source
QtArchiveFolderManager worker through ArchiveBackend::openNativeArchive.
New local ArchiveNativeOpen is only a lifetime/callback adapter: it keeps the
source CArchiveLink and its exact immutable catalog together. Its stable proxy
forwards the opening UI during Open3 and detaches it afterward, because source
volume streams retain COpenCallbackImp after the opening dialog has gone.
QtArchiveFileResource exposes actual native archive properties and closes native
streams before publication/owned temporary cleanup. Item metadata/extraction,
updates and reopen remain process integration; this is not a claim that the
whole source Agent/native extraction consumer is complete. Missing/failed module
discovery preserves the documented process-only read fallback. A valid catalog
uses native detection and returns its refusal/cancellation before process start.
`native_folder_opening` covers source CFfpOpen native properties, catalog reload
lease, wrong forced-type refusal before process start, worker cancellation,
encrypted-header password propagation and unchanged original bytes. Together
with native_archive_opening and all42 codec_update scenarios it first passed3/3(2.74s).
The final stable proxy/lifetime continuation adds actual TGZ and Split→7z
source-folder extraction round trips. Source Open only descends when its original
IInArchiveGetStream→IInStream contract succeeds: the installed gzip handler keeps
its TAR item at the gzip level. The initial automatic-TAR test expectation was
incorrect and corrected after inspecting original Open and actual module behavior.
Native nested chains preserve parsed opening intent across process listing,
extract and reopen; empty native types use explicit *:r at that integration
boundary. Process writer readiness refuses volume sets/nested native chains,
even after native Close. The expanded consumer run caught a real split
Delete/Rename regression introduced by the earlier source read-only migration;
the resource now exposes that process publication constraint through IOutArchive
availability. No compression/format algorithm was rewritten for this refusal.
Native readers are closed before removing an unsuccessful owned stream spool,
which is necessary when platform file handles prohibit removal while open.
After terminal all-target build82764,37 direct suites passed37/37(15.88s), macOS
arm64 Qt6.11.2 2026-10-02; baseline224/161/3 verified. Final command/mutation UX
and full gate remain open; no P2-01 acceptance or helper commit is claimed.
Source COpenOptions callers must initialize excludedFormats/props, and use fresh
options after releasing native streams; the source API mutates stream pointers.
The latest direct native suite passed1/1(0.31s), macOS arm64 Qt6.11.2 2026-10-02.
Baseline verified224 exact/161 adapted/3 local. Full P2-01 acceptance remains open.
After terminal all-target build,25 direct suites passed25/25(9.06s), macOS arm64
Qt6.11.2 2026-10-02. This includes native opening/cancel and the42 source writer
transactions; it is not the final full repository gate.

Local source-plan transport was also extracted coherently into
archivebackend_sourceplan.cpp: validation, KeepName selection/output and stage
orchestration have separate functions; archivebackend_sourceupdate.cpp retains
single-file/add/mutation transports. Original borrowed modules were not split.
No backend behavior was intentionally changed by this extraction.

Latest source update continuation removes four ZIP/7z transport guards and the
QtArchiveSourceUpdate guard in favor of actual native UpdateEnabled metadata.
Source CAgentFolder/CAgent update and publication bodies remain unchanged; Qt
projects their callback plan into official ArchiveCommandLine actions, preserving
the original input and output transaction boundaries. Native explicit type is
passed to Delete/Rename too, so temporary/unknown filenames do not select a
different parser. Catalog-free compatibility remains ZIP/7z.
The local callback-plan projection now deletes only omitted items, because the
source Add action set overwrites referenced replacements. Delete-before-Add
incorrectly required an empty intermediate one-stream archive. Native KeepName
single-file plans use the existing separate-output update transport and source
properties; copying an unnamed bzip2/xz input to result.format would change its
source DefaultName before matching the replacement. This is process transport
adaptation, not a replacement of source update-state planning or archive codecs.
Actual source folder CopyFromFile and CopyFrom/Add-plan publication covers seven
types under unknown suffixes, binary Unicode/spaced paths and42 success/failure/
cancel scenarios. Controlled failures write partial output before exiting;
controlled cancellation observes actual50% output after writing. Original
archive and replacement source bytes remain unchanged after non-success.
Terminal all-target build and23 direct suites passed23/23(8.61s), macOS arm64
Qt6.11.2 2026-10-02; baseline222/161/3 verified. These checks do not prove every
mutation or final command/error UX. Full P2-01 gate/acceptance/commit remain open.

Latest GUI continuation removes the ZIP/7z projection for new creation, retaining
the native PrepareFormatSelection indices. Existing-target update choices still
require their implemented transactional service. Qt forwards the selected native
name in CompressionOptions and accepts native main extensions/aliases rather
than requiring the handler name as filename suffix. The source
CompressDialog.cpp::FormatChanged encryption flags now drive group enabled and
header visibility states. Source SetMethod2's TAR-at-level0 exception is retained
in the Qt method control and process property DTO; SetSolidBlockSize2 uses the
original Solid_ flag, including xz. A source-selection refresh emits one final
format transition after rebuilding the combo so method/password state follows
the actual choice. These changes adapt existing source state to Qt/CLI fields;
no compression algorithms are rewritten.
Focused codec_creation_ui covers seven types at760×720/640×480, real creation,
explicit selected type, source-changing gzip→7z, disabled retained passwords,
footer/control exposure and actual POSIX TAR magic (TarHeader.cpp constants).
Current top/encryption JSON reports reviewed for all14 states. After terminal
all-target build,21 direct suites passed21/21(7.66s), macOS arm64 Qt6.11.2,
2026-10-02. Generic update transport and final full P2-01 gate remain open;
no Windows/Linux runtime or rendered Windows parity is claimed.

Typed creation continuation: local `CompressionOptions::format` carries the
stable native handler name across the Qt/process boundary. Backend creation
uses `CCodecs::FindFormatForArchiveName` or the explicit native name, then the
ported `UpdateGUI::PrepareFormatSelection` for actual source eligibility. It
does not infer write capability from a filename. The existing staging,
publication, source lifecycle and official-engine compression remain owners;
method defaults now use the source format table rather than Deflate for every
non-7z format. Standalone callers without a catalog retain ZIP/7z compatibility.

`Console/List.cpp` transport records for bzip2/xz can omit Path; bzip2 can also
leave sizes undefined. The local listing parser retains these real records,
without inventing size/property values. Their display/extraction name comes
from the already copied `CArc::SetDefaultName` and exact `DefaultName.cpp`, at
the recorded upstream revision above. The original `GetItem_DefaultPath`
extension-property branches are not newly claimed by this transport projection.
Seven actual handlers (7z/ZIP/gzip/bzip2/xz/tar/WIM) round-trip binary Unicode/
spaced files under an explicitly selected type and unknown target suffix.
Failure after partial output and cancellation remove staging and preserve the
source; RAR and gzip-directory eligibility refuse before process launch.
After terminal all-target build, 17 selected suites passed17/17(7.54s), macOS
arm64 Qt6.11.2, installed official engine/module26.03, 2026-10-02. Baseline
verified222 exact/161 adapted/3 local. At this backend checkpoint UI choices projected ZIP/7z;
generic update transports, final UI consumers and the full gate remain pending.
P2-01 is not accepted and no helper commit has been made.

Selection/name continuation copies the active ShowDialog one-file/CFileInfo/censor
and format eligibility blocks into UpdateGUI.cpp::PrepareFormatSelection. The
fragment stays in its original owner; explicit CCompressDialog/CCodecs/censor
inputs replace the unavailable full CUpdateOptions/modal signature. Native Find,
Include, single-path/non-directory, UpdateEnabled, KeepName, hash/SWFC override,
OriginalFileName and KeepName decisions retain source bodies/control flow.
ArcIndices.Clear is the necessary repeated-picker adaptation; Qt handles error
presentation and method-control setup. The original user-codec SetMethods block
and remaining ShowDialog options/modal orchestration are not claimed by this
fragment; external codec methods remain P2-02.

Real Add source addition/removal rebuilds native selection inputs and keeps the
current format while projecting the implemented ZIP/7z transports. Naming uses
actual one-file/OriginalFileName/KeepName inputs before switching format. Initial
sources are prepared before constructing the form, and the already ported full
CreateArchiveName body supplies the suggestion instead of an independent generic
name for metadata-configured callers. Standalone no-catalog compatibility retains
its prior generic suggestion. Qt list/path projection owns no archive naming or
format eligibility algorithm. All-format creation/update transport is still
unfinished and the remaining writer choices are not enabled prematurely.

After terminal affected builds, six direct naming/Add/metadata/language suites
passed6/6(1.43s), macOS arm64 Qt6.11.2 2026-10-02: actual native one-file versus
multiple/directory eligibility, OriginalFileName, no-update-engine refusal, real
source-picker two-files→one KeepName transition and native initial archive name.
Current native archive-path760/640 JSON reviewed: Archive.7z suggestion,
path/format/path-mode order, exposed controls/preview/footer at both sizes.
After rebuilding direct source-update/source-extract/operations/advanced/encryption
consumers, the final selection/name checkpoint passed11/11(7.53s) on the same
platform. Baseline verified222 exact/161 adapted/3 local. This does not accept P2-01;
remaining writer transports, final live command/error UX and full gate are open.

Add/naming continuation copies CompressDialog.cpp::SetArchiveName2,
SetArchiveName and the metadata lookup portion of ArcPath_WasChanged from the
recorded baseline. ArcFormats/ArcIndices/Info.FormatIndex/m_PrevFormat,
OriginalFileName/KeepName and main-extension decisions retain source roles.
Qt supplies field/selection state instead of Win32 control reads; the combo
binding still owns SaveOptionsInMem/FormatChanged delivery. IsSFX/GetMethodSpec
are narrow input adapters; their SFX/hash consumers remain P2-02/P2-03, not an
acceptance of those workflows. Legacy standalone callers without metadata retain
their earlier two-format compatibility path; it is not codec discovery.

CreateArchiveDialog retains its catalog lease, populates implemented ZIP/7z
transports from actual UpdateEnabled metadata and passes native indices into
naming/history. Existing-target format comes from the opened archive's detected
type, so an unknown-suffix ZIP does not silently select7z. Failed metadata gives
an empty format list with an error tooltip and disabled confirmation. Qt format
projection/configuration is extracted into createarchivedialog_formats.cpp to
keep the existing construction owner below400 lines; no borrowed module is split.

Six focused naming/Add/metadata/language suites passed6/6(1.39s), macOS arm64
Qt6.11.2 2026-10-02 before the final IsSFX/GetMethodSpec source-symbol adaptation.
After subsequent affected and all-target terminal builds,16 direct naming/Add/
encryption/advanced/update/catalog/opening/extract/operations/drag/browse/language
suites passed16/16(7.72s) on the same platform. Baseline verified222 exact/161
adapted/3 local paths. Current-build archive-path760/640 JSON reviewed:
path→format→path-mode order, two native transport choices, exposed controls and
scroll viewport at both sizes. EN/RU Add760/640 JSON reviewed: footer/preview
remain visible while upper controls are outside the viewport after the test
scrolls to lower controls, as expected; the focused path reports inspect the top.
No rendered Windows reference or full P2-01 acceptance is claimed. Native gzip
KeepName and gzip→ZIP/7z transitions are helper behavior coverage, not an actual
gzip Add consumer. Source UpdateGUI::ShowDialog one-file/original-name/format
selection and full writable-format transports remain mandatory next work.

Latest integrated checkpoint: ArchiveCodecProbe is local process/platform glue;
upstream Console/Main.cpp prints its version but has no parser of its own output
or Qt asynchronous probe. It reads only the bounded official banner (not the
ambiguous Formats extension/signature table), finds the official module relative
to the canonical engine/app/install prefix, enforces ABI and exact version through
CCodecs, and owns start failure/nonzero exit/malformed output/deadline/cancel.
Backend delivery is queued so receivers can destroy the backend safely. Snapshots
are published with shared-pointer atomic operations because source workers read
them. The default MainWindow starts discovery; injected backends are caller-owned.

Open filters and IFolderManager::GetExtensions now project actual source metadata.
Native FilePlugins extension lookup remains its owner; Qt adapts the Windows icon-
resource extension service to typed metadata. Source CArc::OpenStream's default-
name block is copied into CArc::SetDefaultName: only codecs/index inputs are passed
explicitly. Process-owned arcs retain FormatIndex=-1 because their COpenOptions
do not share the in-process catalog; the resource retains the metadata lease and
applies the body using the detected handler, not a filename guess. Exact
DefaultName retains alias/AddExt/tilde/trim rules. The old completeBaseName and
archiveformats.h guesses are removed. Menus and panel transfers use source folder
read-only state. ZIP/7z are explicit implemented transactional transports; native
encoder presence does not enable other mutation paths. Without a catalog the
existing detected-type transport works; an explicitly failed catalog refuses it.

After terminal affected builds, nine codec/catalog/probe/opening/source-extract/
source-update/UI-operations/drag-archive/browse suites passed9/9(7.42s), macOS arm64
Qt6.11.2 2026-10-02. Real fixtures cover unknown-suffix ZIP, gzip with misleading
ZIP suffix, TGZ→.tar source naming and concatenated ZIP parser '#' read-only;
source CFfpOpen and folder kpidReadOnly are exercised, original bytes unchanged.
Earlier async/filter checkpoint passed5/5(1.10s). No full gate, iteration acceptance
or helper commit is claimed. Add metadata/KeepName and final live command/error UX
remain mandatory. Historical checkpoints below precede these consumer changes.

Latest checkpoint additionally copies OpenArchive.cpp::ParseComplexSize,
ParseTypeParams, ParseType and ParseOpenTypes from the same baseline, retaining
original symbols, branches, type/colon/dot/overflow/recursion rules. No native
opening/parser algorithm is claimed complete by that parsing fragment.
DefaultName.cpp/.h are exact reusable dependencies, with GetDefaultName3/2
extension removal, AddExt, no-extension tilde and trailing-space rules unchanged.

ArchiveBackend now owns an immutable shared ArchiveCodecCatalog. This is local
integration glue around CCodecs: explicitly supplied module and verified engine
version, native ABI/property load/error, strict module-version comparison, no
capabilities on failure, sorted native metadata and presentation extension list.
Lookup/update and open-type parsing delegate to source owners. Old retained
catalogs keep their module alive during replacement and native handlers must be
released before their retained catalog. No global singleton is introduced.
At that earlier checkpoint runtime discovery/probe and UI consumers were pending;
the latest integrated checkpoint above supersedes that status.

After successful terminal builds of app and codec suites, codec_capabilities/
codec_catalog passed2/2(0.40s), macOS arm64 Qt6.11.2 2026-10-02: native */#/chain/
offset/overflow and default names, missing module, busy/unverified-version guard,
real module metadata, mismatched version, reload and retained live RAR5 reader.
Earlier 13-consumer evidence below precedes this latest backend/parser checkpoint.
The latest seven codec/catalog/opening/source-extract/source-update/hash/native-
callback suites passed7/7(3.89s) after terminal builds. Baseline verified222 exact/
161 adapted/3 local paths. No full P2-01 gate or helper commit is claimed.

Baseline [0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17):
`CPP/7zip/UI/Common/LoadCodecs.cpp/.h` copied in full under original src paths.
Registration, codec/hasher/format property decoding, ABI compatibility, DLL
discovery/error/cleanup, extensions/signatures, lookup and handler creation
retain upstream bodies. Only includes and separately recorded formatting adapt.
The formerly exact header moves to src to resolve the single adapted DLL owner;
there is no duplicate implementation. RegisterArc.h and Windows/Registry.cpp/.h
are exact dependencies; Registry builds only on Windows with Unicode definitions.

DLL.cpp::GetModuleDirPrefix retains the source directory/fallback body.
MyGetModuleFileName uses Qt's executable path: upstream has only a Win32 resource
module lookup, while this application has no separate resource DLL. Existing
CLibrary bodies remain unchanged. No registry writes or registration are performed.
Z7_EXTERNAL_CODECS is enabled consistently across consumers. Existing hash tests
select builtin providers with nullptr. PanelCopy activates the source-commented
local CExternalCodecs lifetime because this port has no FM global singleton;
attaching runtime providers remains pending, without adding a mutable global.

codec_capabilities checks source alias/AddExt/case/path/type-chain behavior,
missing library, ordinary-library refusal and the installed official 7z.so26.03
arm64 metadata. DOCX resolves to ZIP; RAR5 is read-only; gzip retains KeepName/
tgz→.tar. Real RAR5 reader and ZIP writer objects are created and released before
unloading. This does not prove ZIP comment writing or decoder GUI callbacks;
P1-17/18 remain open. The installed engine module is not claimed to be built
here from the source baseline. Missing real-module coverage explicitly skips
and remains unverified on that platform.

After terminal builds, codec_capabilities/native_library/native_hash_bundle/
hash_results passed4/4(0.56s), macOS arm64 Qt6.11.2 2026-10-02. Baseline
verified220 exact/161 adapted/3 local paths. All-target consumer build passed;
13 codec/library/hash/drag/source-extract/source-update/UI-operations/opening/
lifecycle/focus consumers then passed13/13(7.54s). No full gate, accepted P2-01
iteration or helper commit is claimed.
Remaining mandatory scope: Add metadata/KeepName, final live Open Inside */# and
missing/incompatible-module UX checks, direct consumers and the full gate. The
installed native handler objects still do not accept P1-17/18.

## P1-16 — drag/drop source owner (accepted implementation, 2026-10-02)

Integrated implementation accepted after source-body/dependency review, real
archive bytes and live two-panel workflows, normal/compact widget reports and
the full unsandboxed formatting/build/179-test gate (67.66s), macOS arm64
Qt6.11.2. Baseline verification:218 exact/159 adapted/3 local paths. Historical
WIP evidence below records intermediate checks, not the final status.
Native Finder acceptance remains mandatory under separate P3-03.1; Windows/Linux
runtime and rendered Windows parity remain unverified under P3-03/P3-06.
Eager materialization and unsupported native promises remain explicit adaptations;
large-list/GUI-thread preparation review belongs to mandatory P2-10.

Baseline remains [0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
PanelDrag.cpp now copies CDataObject::CopyFromPanelTo_Folder, CDropSource's
QueryContinueDrag/GiveFeedback, CDropTarget PositionCursor/IsFsFolderPath/
GetEffect_ForKeys/GetEffect/GetTargetPath, and CApp CreateDragTarget/SetFocusedPanel/
DragBegin/DragEnd. CApp owns the drop target; the prior presentation forwarding
did not establish source drag state. Object creation occurs in the Qt CApp
constructor before any presentation can focus a panel. Native vectors/strings,
copy options and archive CopyTo consumers remain authoritative.

Declarations move into qtpaneldragmodel.h because Qt MIME/presentation needs the
native carrier/state interfaces; implementation remains in original PanelDrag.cpp.
OLE interface/refcount/storage/capture/cursor plumbing is replaced by
qtpaneldragtransport. Win32 point/window/list hit testing and enabled state are narrow
presentation services; POSIX same-drive detection must use actual storage volumes,
not treat all absolute paths as the same '/' drive. GetEffect and continuation
branch order remain source bodies. The source commented-out MOVE mask stays
disabled. Highlight fields move from the panel to its single drop-target owner;
ClearState removes presentation highlight before clearing references.

Source CPanel::OnDrag now retains support/right-button/operated-index gates,
native 7zE CTempDir and corrected filesystem names, DragBegin/End, source
CopyTo options/moveIsAllowed, transfer decision, error/messages and final selection
branches. CF_HDROP HGLOBAL/COM lifetime and DoDragDrop are replaced by a narrow
Qt transport interface and names carrier. On POSIX the Windows-only ADS-prefix
guard is omitted and normalization remains; missing E_UNEXPECTED uses its original
HRESULT value. Comments/debug/OLE clipboard-only calls are outside this scope.
The source SetTransfer carrier exposes native flags/mode/effect/command/folder and
typed coordinates; source SetData2 needProcessBySource and GetData/GetTransfer
flag construction remain in PanelDrag. OLE medium validation/version/ISA/COM
serialization is omitted for the trusted typed in-process carrier, never decoded
from an external MIME marker or serialized pointer. External sources supply URLs.
Native owned temp lease is provided to the receiver transport: protection must be
persisted before external delivery; positive receiver completion clears it,
otherwise OnDrag disables automatic root deletion. This replaces the explicitly
documented upstream delayed-recipient deletion hazard, not source archive logic.

Source Drop/DragEnter/Over/Leave and SendToSource_auto retain native command choice,
right-menu masks, external FS Add/internal FS Copy/open-archive Copy and confirmation,
target-path handshake, source-versus-target execution and flags. Win32 OLE delivery,
message box and popup presentation become narrow Qt services. Typed failure/cancel
results return from CompressDropFiles/CopyFromNoAsk so the source preserves
selection on target failure. Qt's release transition occurs after target execution,
so the transport restores its Copy_HRESULT after QueryContinueDrag's original
pre-drop reset. Failed typed GetData returns before exposing nonexistent paths.

Source CompressDropFiles retains parent/root-temp path choice, archive naming,
wait flags and CopyFsItems/CopyFromNoAsk branches. CreateArchiveName's entire body
is copied into original UI/Common/ArchiveName.cpp with include/formatting changes;
ArchiveName.h is exact vendored source. PanelCopy's previously unconnected
CopyFrom_Paths branch now uses its native NFsFolder::CopyFileSystemItems body,
whose owner was already ported; CopyFsItems and CopyFromNoAsk are copied from
Panel.h/PanelCopy.cpp. Archive/Add execution remains behind native folder/backend
owners. QtFileManagerDrag's compression callback opens the existing Add dialog,
uses native CCompressDialog::SetArchiveName for the selected format's suffix,
and awaits the existing backend progress consumer.

Qt transport prepares archive bytes with native CopyFromPanelTo_Folder before
QDrag::exec because Qt has no portable OLE release-before-drop hook. This can
extract for a subsequently cancelled drag and materialize before an internal
destination is known; it prevents native receivers seeing paths without bytes.
Temporary ownership is protected before exposing URLs, unknown successful
receiver completion stays durable, and known cancelled/finished recipients clear
protection. No native promise or delayed byte generation is claimed.

The live QtFileManagerDrag controller now binds table/icon sources and all
panel viewports, including the second panel created later by F9. Native popup
pairs/masks/order and Yes/No/Cancel confirmation become Qt callbacks; command
selection remains in PanelDrag. GetItemName is exposed to the production hit-test
adapter without changing its body. Source Begin/End and late CopyTo are guarded
for the entire drag; close is refused, hover suspended and single-click suppressed.
Qt's Static icon movement resets dragEnabled, so the common policy is reapplied
after a mode change. Rejected/cancelled drops do not reopen archives. Disabled
viewports, nonlocal URLs and unaccepted synthetic Drop events are rejected before
execution. Native right drag is filesystem-only; Qt defers a macOS press context
menu until release to allow that gesture. These parts form the integrated P1-16
implementation accepted above. Upstream OnDrag documents
unknown external recipients can still use bytes after DoDragDrop returns;
ownership protection must retain that uncertain lifetime rather than delete
by elapsed time. Native platform promises and file managers require separately
recorded platform evidence. EnumFormatEtc's OLE-only format enumerator has been
reviewed; Qt QMimeData supplies its local-URL format enumeration. No native
promise or portable delayed materialization branch is claimed. The Qt Add
consumer always awaits in-process completion even when the source permits a
separate nonmodal 7zG process; native wait flags remain recorded.

After terminal successful builds, focused drag_drop/app_lifecycle/panel_focus
passed3/3(0.58s), macOS arm64 Qt6.11.2 2026-10-02. Native OnDrag is exercised
with an actual Unicode/spaced filesystem file: recipient reads selected bytes,
Cancel preserves selection, successful delivery clears selection without deleting
the original, drag indices reset and notifications resume. Other cases check
opposite/source panel rejection, hovered-folder path/highlight removal, same-volume
effect fallback and archive continuation versus cancellation. These are partial
owner checks at the earlier checkpoint; actual archive bytes were subsequently
verified below. Baseline at that checkpoint217 exact/158 adapted/3 local; Windows/Linux runtime
unverified. No P1-16 acceptance or local commit yet.

Subsequent focused drag_drop/drag_archive/source_extract_backend passed3/3(0.96s)
after successful terminal builds, macOS arm64 Qt6.11.2 2026-10-02. A disposable
official-engine ZIP provides actual binary Unicode/spaced-file bytes to native
CPanel::OnDrag → CopyTo → source Agent/resource → Qt MIME/fake recipient. It checks
finished recipient cleanup, unconfirmed receiver protection after local leases end,
drag cancellation and actual engine extraction cancellation; returned Move never
deletes archive members or changes the original ZIP. Disk→disk ControlCopy and
ShiftMove run source target handshake and actual source CopyTo. Latest baseline
verified218 exact/159 adapted/3 local. Live widgets/UX/full gate/platform recipients
remain mandatory, not accepted by these transport fixtures.

Final focused drag_drop/drag_archive/source_extract_backend/app_lifecycle/
panel_focus/fs_folder_copy passed6/6(0.99s), after terminal successful build;
the final archive fixture additionally covers a recipient refusing delivery.

Live drag_drop_ui exercises both target views and both panel directions,
actual ControlCopy/ShiftMove bytes, folder highlight without selection,
source-panel refusal, native right popup Copy/Cancel, external archive Yes/No/
Cancel with actual ZIP update/re-extraction, and real source callbacks through
offscreen QDrag cancellation. Active roots cannot be claimed, close is refused,
SingleClick suppressed and temp roots removed after cancellation. Archive right
gestures retain source refusal. Hover resumes after DragLeave without opening.
Eight current-build JSON reports at980×620/640×400 were reviewed: visible panels,
paths, view stacks and status strips are exposed; Windows rendered fidelity
remains unverified. Focused11/11 passed7.44s macOS arm64 Qt6.11.2 2026-10-02,
before subsequent native archive→disk/archive fixtures (drag_archive passed0.65s).
Final filesystem→Add success/suffix regression passed1/1(0.55s). Full unsandboxed
check.sh passed formatting/build/all179 CTest cases(67.66s), macOS arm64 Qt6.11.2
2026-10-02. No code/test/build changes followed this passing gate.

The actual Finder attempt could not run: Computer Use returned
“Computer Use was not approved to use sevenzip_desktop”. The isolated app process
exited normally; the empty recipient directory is not success evidence. Access
has been requested; by user instruction the Finder check is now a separate
mandatory P3-03.1 under platform acceptance P3-03/P3-06. It does not block
implementation acceptance of P1-16 or independent P2 work. Native transfer
remains unverified; Windows/Linux runtime remains unverified.

## P1-15.2 — owned-temp cleanup acceptance (2026-10-01)

Accepted integrated cleanup and P1-15 parent: actual command/modal browser,
ownership and native/Qt lifetime, source confirmation/cancellation/first failure,
active child-editor protection, properties/preferences/navigation and Help/F1.
After terminal builds, 18 affected CTest suites passed (12.43 s); unsandboxed
check.sh passed formatting, app build and all 175 tests (66.82 s), macOS arm64
Qt 6.11.2. Four current EN/RU 900×580/500×320 widget JSON reports were reviewed:
all named controls remain exposed, including path/filter/footer at compact size.
Baseline verified: 217 exact / 158 adapted / 3 local. Count 26/46; next P1-16.
Windows/Linux runtime and rendered Windows parity remain unverified.

Pinned source remains [0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original BrowseDialog2::Reload filters OS temp names `7z[E/O/S]` plus eight hex
digits; this establishes a naming convention, not ownership. Its OnDelete
prefix guard/selected indices/one-or-multi confirmation/native formatters/
delete loop/first failure/reload and MyBrowseForTempFolder are the required
functional basis. They are now copied into mirrored BrowseDialog2.cpp/.h and
connected through native MyLoadMenu → CApp → QtFileManagerCommands.
BrowseDialog2.rc establishes Delete/Refresh, Parent/read-only path, list,
disabled filter and Help/Close ordering. The native cleanup command is enabled
when navigation is available and no modal consumer is active.

Copied functional scope: CBrowseItem/CBrowseEnumerator::Enumerate and its
200-dir/2000-file limits; native vectors and one-child aggregate; PrintProps*
and PrintFileProps; CompareItems, sorting transitions, Reload selection/focus,
GetParentPath/OpenParentFolder/OnItemEnter, single/multiple confirmation,
OnDelete's native per-item removal/first-failure return/reload; context command
IDs, ordering, selection constraints and properties; MyBrowseForTempFolder title
and original fm/temp.htm Help topic. The shared Browse_ConvertSizeToString body
is copied into its original BrowseDialog.cpp owner. Exact BrowseDialog2.rc/Res.h
are vendored at the pinned revision. Qt control construction/rows live in
qttempbrowsepresentation.cpp; public class declarations move from the original
CPP to its header for this adapter, without a second functional implementation.

Required adaptations: POSIX CEnumerator::CDirEntry + Fill_FileInfo(false),
IsOsSymLink/GetWinAttrib/FiTime conversion services; Qt controls/messages/cursors,
column sorting invokes the original comparator; registry lookup replaces only
top-level naming membership. Identity/root/no-link ancestry is revalidated under
the claimed root lease immediately before source deletion. Active/unconfirmed
external leases deny claims. A failed PrintFileProps reload invalidates source
selection references, so confirmation returns rather than targeting a new row.
POSIX MyGetTempPath now uses the actual Qt/OS temp service (TMPDIR/macOS per-user
temp), replacing upstream's explicit FIXME /tmp fallback; Win32 service remains
unchanged. Canonical OS temp aliases are resolved for this browser boundary.
Adjacent work/publication paths and prior roots outside current OS temp are
excluded, even if tracked for lifecycle purposes.

Qt OS association and detached self-open replace Win32 shell/process services;
protection is persisted before launch and cleared only on launch failure.
Completion without a waitable handle is deliberately unconfirmed. Directory
Open Outside : 7-Zip remains disabled with a reason until P2-07 supplies a
directory command-line consumer; current main.cpp opens archive files only.
Omitted source scope: drives/non-temp navigation (blocked by the temp boundary),
UNDER_CE layout, commented-out filters/path editor, Win32 shell image lists,
Win32 drawing/message plumbing. FM ShowDots/SingleClick/FullRow/Grid use Qt
parent rows, shared track-select hover and a paint adapter; Shell icons use
QFileIconProvider. The dot parent stops at OS temp just like the Parent command,
excluding the non-temp ancestor entry. Parent navigation and all six functional
columns remain. These presentation adaptations do not
claim pixel parity; directory new-window integration remains documented debt.
On macOS the membership/path key accounts for composed versus decomposed
enumeration names, while revalidating the actual root OS identity before deletion.
This fixed an observed missing registered Unicode-file row; other platforms
retain exact names. No ownership is inferred from Unicode-normalized names alone.

The original CTempDir and CTempFile creation/filename/Remove/MoveTo bodies stay
in mirrored Windows/FileDir.*. After source creation a narrow Core ownership
adapter registers the OS object and holds a lease through automatic cleanup.
Registration failure closes the output/removes the created object and returns
an error rather than leaving an untracked success. Native naming algorithms,
upstream types and deletion behavior remain unchanged. CTempFileInfo has a
shared lease copied with native nested CFolderLink/CTmpProcessInfo records;
DisableDeleting is not treated as proof of inactivity. PanelItemOpen's known
process wait/snapshot loop clears external protection only after its handles
finish. Unknown association/shutdown/wait-error paths retain protection.
Successful launch disables automatic deletion before native worker creation:
an allocation/thread failure must not delete a file already handed to a process.
The source wait/update/decision loops otherwise remain intact.

No upstream persistent ownership/lease registry exists: original temporary
cleanup relies on filename filtering and can browse arbitrary OS temp. New
qtownedtempregistry/identity/dir are necessary platform/lifetime glue, not a
replacement deletion algorithm. Records use atomic QSaveFile in a private
application cache directory (explicit private root for tests), per-object
QLockFile with age-based expiry disabled, native device/file ID and directory
kind, and durable unconfirmed-external protection. One generated UUID per OS
identity prevents a second registration from bypassing an existing lock.
ClaimForDeletion re-reads identity/protection under the lock. Links/junctions,
malformed records and replaced objects cannot establish ownership. Windows
identity uses source CFileBase handle information with read/write/delete sharing
and OPEN_REPARSE_POINT; POSIX lstat supplies device/inode/type. This is a narrow
identity service, following the existing FSFolder::ReadFileInfo platform boundary.

P2-09 Linux correction (2026-10-04): snapshot847c6ded reproduced native
CTempDir::Create refusing a directory after a retained edit was deleted outside
the registry (`owned_temp::deletedRetainedDirectoryAllowsLaterNativeCreation`).
Linux reused the same device/inode/type at the source-generated path, so an
inactive record falsely matched the new object. QtOwnedTempIdentity now appends
statx birth-time seconds/nanoseconds when the filesystem provides STATX_BTIME;
content edits do not change that identity. Other platforms and filesystems
without birth-time support retain their prior identity service. This narrow
registry adaptation leaves original FileDir naming/creation/control flow intact.
The regression exercises 50 native create/retain/delete cycles, rather than
mocking object IDs. Corrected Linux snapshot2291d930 passed7/7 focused suites
in14.52s, including source external/nested editor consumers. macOS full
formatting/build/229CTest passed99.37s. A subsequent localized50-cycle regression
checks that recreated user directories cannot be cleanup candidates; macOS
focused rebuild/CTest passed0.54s, Linux95406fe7 passed7/7 in15.28s, zero skips.
Unsupported-filesystem inode reuse remains a limitation of the fallback identity.

Qt-owned hash/view/nested extract/update/rollback/volume/language directories
use the same lease alongside QTemporaryDir automatic cleanup. Stream-to-file
spooling retains its source CopyStream body and holds a lease with its private
engine file. Adjacent archive staging paths are tracked for lifecycle purposes;
the source cleanup presentation retains the OS-temp boundary and
does not expose arbitrary filesystem deletion. Standard output/staging paths
owned by existing backend publication remain behind ArchiveBackend.

owned_temp passed1/1(0.33s), then direct external/nested/extract/update/viewer/
language/volume consumers passed14/14(10.60s), macOS arm64 Qt6.11.2 2026-10-01.
Additional actual child-editor/wait-error ownership and failed-registry fixtures
passed2/2(3.33s) at the earlier registry checkpoint. The final 18-suite and full
gate above include actual cleanup versus active child-editor fixtures. Windows/
Linux identity/lock/runtime behavior is unverified locally.

Publication adaptation: mirrored ArchiveFolderOut::CommonUpdateOperation retains
the source MoveToOriginal / MoveArc_Finish / Before_ArcReopen / ReOpen ordering.
Immediately after a successful physical move, QtArchiveFileResource transfers
the native CTempFile lease to its existing private spool destination. CTempFile
and CWorkDirTempFile expose only that lease; their move algorithms are unchanged.
For the same OS object the held identity lock and record are relocated; a
successful cross-volume copy registers the new object only at the already owned
private spool. Ordinary user archives return without registration. A stale
replaced-object record is pruned when its last lease dies. Ownership-adapter errors
retain source reopening and callback-error priority; a cancelled Finish after a
successful move still follows the ownership transfer. This is narrow integration
glue, not a replacement publication algorithm.

owned_temp_publication checks native CWorkDirTempFile publication for private
spools versus user archives, and actual CAgentFolder::CommonUpdateOperation
rename/reopen with a private spooled ZIP on success, engine failure and cancellation.
It observes changed identity, retained active lock, denied cleanup claim, original
user bytes and final spool removal. Passed1/1(0.67s), macOS arm64 Qt6.11.2.
Actual cross-volume IO and late MoveArc_Finish cancellation are not exercised by
these fixtures; failed physical moves do not register an unproven destination.
temp_cleanup additionally checks Yes/No/Cancel through actual message-box buttons,
Unicode/spaced file and directory deletion, unowned names, link navigation,
active/unconfirmed entries, replaced second-object identity stopping multi-delete,
refresh selection, context IDs/properties and source FM view preferences. OS launch
is intercepted for the SingleClick fixture. main_shell checks actual modal command/
Escape, and archive_external_lifetime opens cleanup during a real child editor.

## P1-15.1 — Help/About/current context acceptance (2026-10-01)

Accepted integrated scope: native Contents/F1, source About/version/license,
all current Options/Add/Extract/advanced Help callbacks, offline links/fragments,
unavailable error, pending input/dirty Apply preservation and EN/RU normal/compact
geometry. Final unsandboxed check.sh passed formatting, app build and172/172
CTest(63.63s), macOS arm64 Qt6.11.2 2026-10-01. Parent P1-15 remains open;
next P1-15.2 owned-temp cleanup, count25/46 unchanged. Windows/Linux runtime
and rendered Windows parity remain unverified.

Baseline [ip7z/7zip0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Mirrored HelpUtils.cpp/.h retains ShowHelpWindow, kHelpFileName and native FString
module-prefix + `7-zip.chm::/` + topic construction. Qt module-directory/offline
display replaces GetModuleDirPrefix/HtmlHelp/hh.exe; Qt parent supplies lifetime.
Win32/CE/MingW display branches use the common Qt adapter. Native MyLoadMenu
Contents `FM/index.htm` and COptionsDialog::OnHelp kHelpTopic_Options are wired
through their original command switch/functions. Main Contents/F1 and advanced
compression Help are actual consumers. Source Edit/Folders/Settings/Menu/Lang
OnNotifyHelp callbacks retain their original topic constants, including source
case and fragment. Qt Options dispatches the selected page's native callback;
F1 and Help never apply pending settings. CCompressDialog::OnHelp and native
ExtractDialog::OnHelp retain original topics; Qt parent is a presentation input.
Win32 CModalDialog::OnHelp's base notification has no Qt control-message counterpart.
Disabled System/Benchmark callbacks remain with P2-08/P2-04; cleanup Help is .2.

AboutDialog.cpp/.h preserves CAboutDialog::OnInit/OnHelp/OnButtonClicked,
kLangIDs, native UString version/date calls, start.htm and homepage switch.
Qt bindings replace LangSetWindowText/LangSetDlgItems; Qt modal positioning,
initialization/standard OK replace CModalDialog services. QDesktopServices
replaces Win32/CE ShellExecute variants. Original external-codec diagnostic
branch is omitted because g_CodecsObj is unavailable in the current external-7zz
adapter; its capability owner is P2-01/P2-02. Exact AboutDialog.rc/Res.h,
7zipLogo.ico, MyVersion.h and C/7zVersion.h are vendored at the full baseline.
The original logo/version/date/copyright/free-software/OK/homepage grouping
remains. Added local application version and explicit original license link
distinguish borrowed source version from external engine; no engine version is
inferred. Desktop version comes from CMake project metadata. EN/RU360×330/300×310
About JSON is reviewed: all labels/buttons exposed, source and local text fit.
Homepage tests intercept Qt URL handling; no live service is contacted.

Exact HTML/CSS comes from the official26.03 installer already used by Language,
SHA2560859c524b8a63551848f0c246abddcb1d0b7b656b0fbfe879f8d85e61a9e6edd.
resources/7zip/help-provenance.json records retrievable URL, CHM hash and all78
exact extracted members (70 HTML,8 CSS). CHM binary index streams are omitted;
QTextBrowser uses original relative links. Encoding/CRLF/content is unchanged.
The release help is English; local window/navigation/unavailable context is
EN/RU. Qt normalizes source topic filenames for native case-insensitive CHM
lookup, retaining fragment case. Offline Qt HTML replaces native CHM presentation
on all platforms; Windows/Linux runtime/rendering remains unverified. Explicit
root fixtures exercise unavailable help; HTTP(S) links launch only after a click.

help_source passed1/1 (0.44 s); actual Contents/F1 and advanced Help plus main
shell/language consumers passed5/5 (2.76 s), macOS arm64 Qt6.11.2. Current
EN/RU760×560/420×300 JSON inspected: browser, Back and standard Close exposed.
Additional integrated Help/About/current-context focused set passed11/11(4.70s);
final real Add/Extract F1 and geometry/resource consumers passed4/4(1.05s).
Baseline verified215 exact/156 adapted/3 local; all78 HTML/CSS bytes and original
CHM member hash independently compared with the extracted pinned installer.
Final native-UString About/actual UI consumers passed2/2(0.54s); full gate above
accepts .1 as one integrated result, without helper commits. Actual cleanup
owner is BrowseDialog2::MyBrowseForTempFolder/OnDelete; DeleteOldTempFiles is
commented out. Owned-temp lifetime/cleanup and parent acceptance remain mandatory.

## P1-14.2 — remaining dialog consumers and acceptance (2026-10-01)

**Accepted current scope:** all current app forms, source captions and dynamic
messages, Qt/integration context and Core-only worker messages support source
English/Russian. Pending input, selections, action identity, modal decisions,
raw paths/engine text and archive bytes remain intact. Static source RC fallback
and dynamic LangString fallback are distinct; partial/malformed files, long
labels, original mnemonic collisions, Tab/buddy reachability and normal/compact
EN/RU geometry are checked. Earlier partial checkpoints below are historical.
Final unsandboxed `./scripts/check.sh` passed formatting, app build and170/170
CTest (63.11 s), macOS arm64 Qt6.11.2, 2026-10-01. Failed-gate fixture regression
set passed13/13 (3.00 s); final isolated language command/main-dialog consumers
passed2/2 (1.04 s). Baseline/inventory verify210 exact/152 adapted/3 local at the
full pinned revision below. P1-14 parent is accepted, count25/46; next P1-15.
Windows/Linux runtime and rendered Windows parity remain unverified; macOS
native mnemonic/standard-button policy is retained. Future disabled Help/System/
Benchmark/shell/decoder capabilities retain their separate mandatory owners.

The final caption review distinguishes source `LangSetDlgItemText`/
`LangSetWindowText` from dynamic `LangString`: missing dialog translations retain
the original RC caption, including case and accelerator position. Local
`QtBindResourceText` implements that presentation service using the unchanged
`LangString_OnlyFromLangFile`; it does not change native lookup or parser logic.
Password/Split/Extract/Overwrite/Messages/Mem, Add/advanced, Progress labels and
Settings/Folders/Edit/Menu/Language static controls use their original desktop
RC literals. Dynamic titles/prompts selected by callers still use `LangString`.
Progress changes its binding from resource Cancel to dynamic IDS_CLOSE at
completion; its regression caught and fixed stale resource fallback on hot switch.

`language_focus` checks actual RU Add's original duplicate `П` mnemonic on Show
password and Parameters, retaining both controls and pending secrets. Qt cycles
focus where the platform enables mnemonics; Tab reachability is unconditional.
macOS can suppress mnemonics by native policy, so this is not Windows/Linux
runtime evidence. Composite CPU/RAM form rows now expose the real combo through
Qt focus proxies and accessible names; EN→RU→EN tests retain values. Partial,
blank-ID and malformed files are exercised in the actual Add form: original
`CLang::OpenFromString` skips blank lines and advances IDs, so a blank line does
not define an empty caption. An attempted native lookup API extension was
reverted after this source inspection. Original EN/RU assets remain unchanged.
Disabled Menu shell/Zone tooltips now have explicit local EN/RU context.

Latest direct RC/focus/Options/advanced consumers passed8/8 (5.69 s), macOS arm64
Qt6.11.2; the final Progress binding correction passed2/2 (9.68 s). Current compact
Add/advanced/Mem/Progress JSON was inspected with full footer/buttons and expected
scrolling exposure. Baseline verifies210 exact/152 adapted/3 local paths. The
whole-app final gate and residual picker-filter review remain open.

The first whole-app gate passed formatting/build and161/170 tests (208.02 s),
with nine failed/not-run suites caused by remaining fixed-Russian/old-Auto
fixtures: Copy/Extract titles, cancellation text, resource selectors and native
New Folder default. Progress Cancel geometry also required fixed Russian Yes/No;
it now checks complete platform standard buttons and native default/Escape and
decision behavior. Combo's failed timer assertion returned before rejecting the
modal dialog; that test process was terminated, the observation/assertion was
moved outside the timer and a30 s CTest timeout added. Failures remain subject
to rebuilt focused and full checks; this run is not acceptance.

Residual Open archive filter was a local Qt extension: upstream
BrowseDialog.cpp::CBrowseInfo::BrowseForFile accepts caller filter descriptions,
and EditPage supplies `*.exe`; no upstream owner builds this app's supported-
archive suffix filter. ArchiveFormats::openDialogFilter now supplies explicit
local EN/RU Archives/All files captions, retaining every existing mask, `.001`,
selection and FilterIndex. `browse_dialog` exercises the real picker and Unicode
first-volume selection in both languages. Codec capability/suffix detection is
unchanged migration debt outside this caption task.

The second gate passed formatting/build and168/170 tests (226.14 s), leaving
File Viewer modal waiting and Language Main Dialog timeout. A macOS process
sample proved File Viewer waited in `openFileViewer::exec` after a30 ms one-shot
test responder had already fired. Viewer/edit responders now wait for actual
visible dialogs and stop before acting, with assertions after modal completion;
the suite has a60 s bound. The Language sample showed MainWindow construction
sorting the populated build directory, not a language-lock deadlock. The existing
per-suite startup/TMP isolation loop ran before new Language targets were defined;
it is now applied after all targets, keeping startup filesystem input deterministic.
Its cancellation warning responder also has local bounded lifetime rather than
callbacks capturing expired loop variables. No production timer or sort body
was changed for these fixture failures.

Source baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
The `FileManager/{PasswordDialog,OverwriteDialog,MemDialog,MessagesDialog,
SplitDialog,CopyDialog,BrowseDialog,ComboDialog}` and `GUI/ExtractDialog` modules
retain their previously ported functional bodies. Source `LangSetWindowText`,
`LangSetDlgItems` IDs, `LangUtils.cpp` standard-control mappings, RC captions,
`App.cpp::CApp::OnCopy` and `PanelSplitFile.cpp` caller title/prompt IDs now feed
Qt presentation bindings. `PasswordDialogRes.h`, `CopyDialogRes.h` and
`SplitDialogRes.h` are copied unchanged into the exact-source manifest.

Bindings install a Qt LanguageChange filter on the window and update caption,
accessible name, optional tooltip/placeholder and source filename suffix only.
Pending passwords, path/name edits, history, selection and modal decision flags
are not reinitialized. Overwrite size/time text and Mem info are refreshed from
their original source formatting bodies; Messages updates the header/button
geometry without replacing existing rows. The independently written directory
overwrite caption is removed: upstream uses the same file-caption IDs for this
branch. Split invalid volume uses source ID7307 in the existing inline Qt error
label; Mem invalid arguments reuse upstream ErrorMsg (POSIX system text remains
English). Extract mode arrays and AddComboItems now use the original source IDs
and control flow; the unsupported absolute extraction mode remains omitted as
before. Hot switch changes item text in place, retaining data and selection.
The old local hint claiming that NoPaths refuses duplicate basenames is removed;
the source overwrite decision handles these collisions.

Add to Archive and the advanced Options presentation are being bound to
CompressDialog.cpp/RC IDs. `createarchivedialog_language.cpp` owns Qt form-label
bindings and text-only item refresh; source numeric settings and functional
owners remain unchanged. `QtLocalText`/`QtBindLocalText` are explicitly limited
to Qt/integration text without upstream IDs, with English fallback and Russian
selected from the source language-file metadata. No source translation IDs or
original distribution assets are invented or modified.

Focused `language_dialog/extract_prompt/password_dialog/volume_dialog` passed
4/4 (1.41 s); after adding Extract, `language_dialog/extract_dialog` passed 2/2
(0.65 s), macOS arm64 Qt6.11.2. This is partial evidence, not .2 acceptance.
Remaining operation/worker/local backend context, complete EN/RU
reachability/fallback/accelerators and the final full gate
are still mandatory. Windows/Linux runtime/rendered parity remain unverified.

Additional in-progress consumers: Add source field labels, password errors,
level/path/update and solid labels, unchanged numeric item data; its additional
Qt source picker/save-folder/footer use explicit local EN/RU bindings. Memory
default display now restores source `k_Auto_Prefix` (`*  `) rather than the
independent Russian prefix. Small-volume confirmation uses existing native
`FormatUtils::MyFormatNew`/`NumberToString` with source ID7308. Advanced Options
binds the original kLangIDs_Options. For hot switch, COptionsDialog::AddPrec
retains its original formatting body and accepts an optional text-only output:
the Qt event updates existing item text using the original payload without
calling SetPrec/OnInit or committing pending checkbox/time policy. Its Type
caption uses the original property owner and retains the existing format/method
suffix. Qt standard buttons replace previous independent RU overrides.
QTextLayout wraps the long source archive-time checkbox at 300 logical px:
QCheckBox lacks native wordWrap; this retains the original words/accelerator
and prevents RU compact minimum width expanding from440 to466px.
Latest focused eight dialog/language/compression consumers passed8/8 (3.06 s);
source baseline verified210 exact/152 adapted/3 local, macOS arm64 Qt6.11.2.
Current JSON reports inspected: core dialogs and advanced EN/RU no clipped
text, advanced compact440×340 retained. Add content outside the viewport is
scrollable and still requires explicit reachability checks before .2 acceptance.

Progress now copies upstream ProgressDialog2.cpp kLangIDs/kLangIDs_Colon and
binds them before its original OnInit flow. Initial Qt resource fallbacks are
English; the two colon captions follow the original service. A LanguageChange
handler refreshes the original cached Pause/Continue/Background/Foreground/
Paused strings and calls the existing SetPauseText/SetPriorityText/OnSize without
changing Sync, transport state or accumulated messages. Finished IDCANCEL keeps
source Close ID408 metadata after OnExternalCloseMessage. New hot-switch tests
cover paused/background state, unchanged percentage/messages and finished Close;
the transport fixture asserts that language events send no operation requests.
Native priority, worker, cancellation and resource contracts passed8/8 (12.81 s).
The existing fast-finish worker fixture now waits at most50×10ms for modal
activation before observing Close focus: it previously saw enabled/default and
focusWidget=IDCANCEL while hasFocus was still false; the same focus requirement
remains and failure closes the dialog. Static English caption expectations were
updated to exact source Pause/Background; no operation semantics changed.

PanelMenu.cpp::AddPropertyString retains its source enumeration/formatting body
and records property IDs plus copied BSTR fallback at the Qt row boundary.
ListViewDialog stores those IDs on existing items; hot switch calls original
GetNameOfProperty, updates Strings used by clipboard/item info, and keeps values,
row identity and selection. It performs no property/backend requery. Properties
title and full-value EditDialog title use source IDs. FoldersHistory and SelectSpec
caller title/prompt now use original source IDs6601/6402/6403/6404. These changes
do not replace their source history/mask algorithms. Actual Properties EN/RU
normal540×360/compact360×240 reports inspected; focused language_ui/folder_history/
panel_selection/list_view_dialog/properties_runtime passed5/5 (2.52 s).

The built-in FileViewerDialog is a local bounded UTF-8 editor. Upstream
PanelItemOpen.cpp launches external viewer/editor applications; EditDialog.cpp
only presents read-only list values and has no reusable file-read/write editor
body. Therefore its captions/accessible names/validation notices use explicit
local EN/RU text; file content, pending cursor/selection/document-modified state,
save/cancel flow and QFile/QSaveFile operations are unchanged. Read-only error
presentation retains raw QFile error detail and translates the local context on
hot switch. Focused language_viewer/file_viewer passed2/2 (0.92 s), macOS arm64
Qt6.11.2. These remain partial .2 checks: local main-window extras/messages,
backend context, whole-app reachability/accelerators/long labels and full gate
are still required; no intermediate component acceptance/commit is claimed.

Additional .2 work binds the Qt-only File menu group (Open/Create/Add to open
archive, selected Extract, last-operation log, built-in text viewer/editor),
Back and the toolbar window title to explicit EN/RU text. These entries have no
corresponding native command/resource caption; native commands still use
MyLoadMenu. Language changes retain QAction identity, enabled state and shortcut.
The pure QtLocalText lookup now lives in qtlanguageservices rather than the
widget adapter, so backend presentation context does not depend on controls.
The nine local validateExtraction rejection messages now use this service;
its existing checks/control flow, raw paths, engine arguments and listing parser
are unchanged. Source ExtractCallback.cpp/ExtractingFilePath.cpp do not provide
these transport preflight captions. This text-only change does not claim a
source port of the legacy safePath/validateExtraction implementation; its
functional migration debt remains subject to its source owner.
Focused archive_backend/language_ui passed2/2 (1.08 s), macOS arm64 Qt6.11.2.
The new actual backend fixture checks EN→RU→EN empty destination and unsafe
Unicode/spaced path rejection with unmodified raw path and no extraction effect.
An initial test fixture omitted the on-disk archive and was rejected by the
snapshot identity guard; the corrected fixture now reaches the intended checks.
MainWindow EN/RU reports at1100×640/640×380 check address/table/toolbar exposure.
Other local main-window/operation/backend context, accelerators, long-label and
scroll reachability checks and final full gate remain open.

The local Archive information summary now binds its existing metadata snapshot
to EN/RU window/label text, with no re-list. It is an existing Qt/7zz integration
summary, not the source Properties dialog; no additional native metadata algorithm
is claimed. Open Archive picker/system-open error and the legacy built-in-editor
operation context now select local EN/RU text. Its changed-file confirmation
uses the existing source FormatUtils::MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE,
RelPath), matching PanelItemOpen.cpp's message owner; the existing workflow is
unchanged. language_main_dialog/ui_navigation/language_ui passed3/3 (7.22 s),
macOS arm64 Qt6.11.2. Initial new fixture used an unrelated positional ArchiveInfo
aggregate; explicit files/unpackedSize fields corrected it before the passing run.
Archive summary EN/RU420×240/300×180 reports expose the complete metadata label.
This remains partial evidence; other operation/backend captions and .2 acceptance
are still pending.

ArchiveBackend.cpp's local process-start/busy/preflight/transaction/exit context,
password-redaction marker and completion captions now select EN/RU through
qtlanguageservices. Native diagnostic owners retain source IDs; these captions
describe the separate CLI integration and do not invent resource IDs. Listing
tokens, process arguments, exit parsing and raw engine detail are unchanged.
MainWindow completion no longer recognizes cancellation by a Russian string:
an explicit HRESULT comes from the actual completion owner. The backend signal
passes lastResult; native batch/PanelCopy passes its own result; the Add progress
caller uses CProgressThreadVirt::Result after successful Create. Source Create
returns S_OK for successful modal creation, independently of the worker result.
This adaptation preserves upstream result semantics at the Qt completion bridge.
An initial backend-only result lookup incorrectly warned for source overwrite
Cancel; a sampled ui_operations stack identified that path. The owned hanging
test was terminated, then explicit source results fixed the regression.
Direct archive_backend/language_main_dialog/ui_archive_states/ui_navigation/
ui_operations passed5/5 (12.79 s), macOS arm64 Qt6.11.2. The context-menu test's
stale Russian local Add expectation now uses default English. Controlled fixtures
report E_ABORT explicitly for cancellation. Further actual backend EN→RU→EN
success/preflight/engine-exit context checks preserve the raw engine tail, and
actual canceled Create runs in both languages with E_ABORT/no output effect;
archive_backend passed1/1 (1.33 s). No full .2 gate or acceptance yet.

Further .2 consumers: ArchiveBackend creation/volume lifecycle and their Qt
workers now use local EN/RU context for creation/deletion/progress, collisions,
publication failures and cancellation. Native EnumDirItems/MultiOutStream/
CopyStream bodies, transactional paths, flags and result codes remain unchanged.
These captions describe CLI staging, not native compression-format logic.
Actual variable ZIP/7z volumes, protected/unprotected success, occupied preflight/
post-compression destination, compression/volume cancellation and pause-cancel-
join now run in both languages and verify exact completion/error context with
unchanged Unicode/spaced paths and output cleanup. Six direct suites passed6/6
(6.77 s): archive_volume_creation/archive_creation_lifecycle/ui_archive_volumes/
volume_operations/ui_operations/language_dialog, macOS arm64 Qt6.11.2.
The native PanelSplitFile.cpp source title IDs7303/7402 and source size/confirmation
IDs7304/7305/7306 now feed the existing MainWindow Split/Combine caller. Original
MyFormatNew/NumberToString formats the part count; modal buttons/decisions remain.

MainWindow opening uses source FileFolderPluginOpen.cpp's IDS_OPENNING3303;
hash continuation uses source PanelCrc.cpp's IDS_CHECKSUM_CALCULATING7500.
Journal result/error and local archive-move continuation context now select EN/RU
while retaining the raw backend message. An actual source Open cancellation
regression runs EN→RU→EN and observes selected-language status and journal.
Direct ui_archive_states/ui_navigation/ui_operations/language_main_dialog/
archive_progress passed5/5 (48.24 s). Initial fixture set the language before
MainWindow reloaded its settings; language is now selected after construction.
The legacy move-out assertion expected Russian and returned before dismissing
the modal error, causing the diagnostic timeout; it now checks English and
dismisses before asserting. No production workaround or timeout relaxation.
Other operation/worker/backend captions, scrolling reachability, fallback and
accelerator/long-label review plus the integrated full gate remain mandatory.

Split/Combine worker chain now selects EN/RU for the existing QFile/transaction
adapter context in mirrored PanelSplitFile.cpp and VolumeOperation. The source
worker/sequence control flow, payload bytes, stage/publication paths and QFile
error detail are unchanged; only existing local captions change. Original source
Combine IDs7404/7405 and MyFormatNew(IDS_FILE_EXIST3008,path) replace independently
written preflight wording. Other safety/transaction captions have no native
resource and use explicit local pairs. Formatting is applied separately from
these text bindings; no functional body reorganization is included.
MainWindow suppresses cancellation error presentation using the operation's
atomic isCanceled state; it does not recognize a translated message string.
The public getter is consumed by production completion handling, not added for
test access. Existing callback/cleanup lifetime is retained.
Worker success, collision/gap, cancel rollback, removed/grown input parts and
changed split source now run in EN/RU, assert raw Unicode paths/bytes and no
partial output. Actual MainWindow Split→Combine also verifies selected-language
completion. Seven direct suites passed7/7 (6.46 s), macOS arm64 Qt6.11.2:
volume_operations/volume_workflow/combine_names/split_destination/volume_dialog/
language_dialog/ui_operations. Stale Russian source-change/name expectations
were corrected before this passing run.
The added EN/RU100-part confirmation matrix covers dialog Cancel and Yes/No/
Cancel in normal1100×640 and compact640×380 main windows. Source message text
and applicable title are asserted; twelve current JSON reports show the Qt
managed400×116 confirmation with all labels/buttons exposed. The latest focused
volume_workflow passed1/1 (1.18 s); prior worker/workflow2/2 passed0.91 s.
QMessageBox intentionally ignores window titles on macOS, documented by
[Qt](https://doc.qt.io/qt-6/qmessagebox.html#setWindowTitle); tests assert this
platform behavior and retain the source title assertion on other platforms.
No rendered Windows reference or Windows/Linux runtime verification is claimed.
MainWindow archive-command captions now use the original source IDs: PanelCrc
IDS_CHECKSUM_CALCULATING/INFORMATION, FileFolderPluginOpen IDS_OPENNING,
GUI compression3301 and extraction/testing3300/3302; PanelCopy's CopyTo branch
retains IDS_COPYING6004. Local bounded viewer/temporary-directory/update-result
context uses explicit EN/RU pairs; source messages and raw paths are unchanged.
References remain the full pinned revision above, including PanelCopy.cpp's
title selection and ExtractGUI.cpp/UpdateGUI.cpp. No source functional body,
argument, selection, metadata or archive algorithm changes are included here.
The existing mirrored GUI/resource2.h remains authoritative; baseline verification
caught an attempted duplicate exact copy, which was removed before acceptance.
Baseline remains210 exact/152 adapted/3 local.
The archive preview-limit regression uses actual MainWindow selection and
17-MiB metadata: EN→RU→EN at1100/640 widths retains selection/backend counts,
shows the complete local diagnostic and writes four400×116 popup reports.
language_archive_command passed1/1 (0.53 s); six direct archive/window suites
passed6/6 (46.23 s); after the authoritative-header correction the three affected
language_archive_command/archive_progress/ui_operations suites passed3/3 (7.41 s).

File-transfer presentation now uses source PanelCopy.cpp's IDS_COPYING6004 and
IDS_MOVING6005 plus explicit EN/RU pairs for existing Qt staging/safety/error
context in FileTransferOperation and mirrored FSFolderCopy.cpp. The native
CopyTo/AskWrite/recursive/progress bodies, rollback/publication/delete order,
bytes and raw paths remain unchanged. Source hardcoded English filesystem
diagnostics retain their English fallback; Russian is a local presentation
adaptation because those original diagnostics have no language resource ID.
Formatting is accounted separately; existing large owner files are retained for
this focused caption change, without introducing a second responsibility.
FileTransferOperation marks the actual overwrite Cancel answer and exposes a
typed completion kind. MainWindow no longer compares cancellation text.
Fully rolled-back cancellation is Canceled; the existing partial-move warning
is Failure even after cancellation, preserving its visibility. Atomic completion
state is written before emitting the existing result signal and used in production.
Core-only LangUtils/ResourceString/qtlanguageservices and the original bundled
resource move to sevenzip_filesystem alongside CLang/file I/O; Qt widget bindings
remain in sevenzip_panel. This avoids adding a Widgets dependency or circular
panel dependency to filesystem workers and keeps each module authoritative.
language_file_transfer covers EN/RU success Copy/Move, raw Unicode/spaced paths,
overwrite Cancel/rollback, true failure and partial-move cancellation retaining
published bytes. The direct seven suites passed7/7 (6.77 s), macOS arm64Qt6.11.2:
language_file_transfer/file_transfer/file_transfer_direct/fs_folder_copy/
ui_operations/language_ui/language_dialog. Initial build used the wrong resource
header for6004/6005; the existing resource.h/resourceGui.h fixed the declarations.
Other archive-transfer/backend/integration messages and the whole-app .2 gate
remain open; none of these partial checks closes .2 or the parent.

Archive-to-archive local warning/result context now uses EN/RU with source
IDS_COPYING/IDS_MOVING. CApp's copied staged CopyTo→CopyFrom flow derives from
pinned upstream App.cpp::Copy (lines780–824); the existing delayed source-delete
and destination rollback adaptation is retained, with no transaction changes.
The actual ZIP Copy/Move/declined-delete test now runs EN/RU and checks selected
language completion, extracted bytes, source membership and exact rollback.

archivebackend_sourceupdate.cpp's existing CLI/staging validation, no-op and
error context now selects EN/RU via Core language services. Source Update.cpp/
Agent Update callback/ArchiveCommandLine.cpp decisions, stage ordering, process
arguments, safety checks and raw paths are unchanged. This presentation-only
change does not claim a source port of the remaining independently written
integration validation. The dedicated language_source_update scenario checks
actual ZIP update/no-op/cancel, unsafe add/protected-policy rejection and raw
Unicode/spaced missing source path, preserving input/output bytes and HRESULT.
The final five direct suites passed5/5 (7.44 s), macOS arm64Qt6.11.2:
language_source_update/source_update_backend/archive_backend/ui_operations/
language_ui. The first archive-transfer direct run caught a stale Russian nested
archive rejection assertion; it now checks exact English after dismissing the
message, with no production workaround. Core language loading and dialog direct
consumers separately passed language/language_main_dialog/language_archive_command
3/3 (52.38 s) after the library ownership move. The integrated .2 gate and
remaining mutation/worker context remain pending at that checkpoint.

Further source presentation port, at the same full baseline above:
`CPP/7zip/UI/GUI/CompressDialog.cpp::{AddSize_MB,AddSize_MB_id,
SetErrorMessage_MemUsage}` (source lines1030–1063) now retain their original
UString bodies, overflow-aware ceil-MB conversion, resource IDs, line order and
blank lines. The formatter declaration is exposed in the mirrored header so
the existing compression UI and ArchiveBackend preflight share one body. The
original usage-caption fallback in OnOK (IDS_MEM_REQUIRED_MEM_SIZE, then
IDT_COMPRESS_MEMORY with colon removal) is retained. Qt supplies the final
QString and QMessageBox; existing RAM calculations and rejection checks are
unchanged. This is compression preflight, not the mandatory actual decoder
RAM callback of P1-18.

The same module restores original `k_Auto_Prefix`/`Modify_Auto(AString&)`,
`Combo_AddDict2`'s AString text body and `Add_Size(AString&,UInt64)` (source
lines1616–1625,1825–1836,2390–2402). Narrow QString adapters convert only the
native ASCII size/count display. Win32 combo insertion/item data remain adapted
by the existing Qt callers; original B/KB/MB/GB and `*  ` now replace the local
Russian unit/Automatic formatter. All existing algorithm/choice omissions in
the prior CompressDialog audit remain; this does not claim the entire module.
Hot switch retains these language-independent numbers and item payloads.

`archivebackend_mutations.cpp` local validation/staging context now uses EN/RU;
password and invalid-volume errors reuse source3805/3806/7307. No validator,
argument, stage, safety guard or archive algorithm changed. Source-extract busy/
invalid-stream context and Qt filesystem mutation/association notices also use
local EN/RU, preserving raw engine/system text and Unicode paths. FSFolder's
native mutation bodies remain authoritative. ExtractGUI error popup title is
the original ShowErrorMessage's `7-Zip`; its original hardcoded invalid-output
diagnostic is retained. Progress AskCancel keeps Qt platform standard buttons,
original Yes/No/Cancel control flow, default Yes and Escape Cancel.

Actual creation preflight tests cover native password rejection, local invalid
method, zero RAM limit and pending input; source-extract rejection and occupied
Unicode filesystem targets retain bytes/HRESULT. The native full RAM message
is checked EN→RU→EN at both Add sizes; four current popup JSON reports show
English400×226/Russian400×256 with complete message/buttons. Association failure
checks EN/RU, plaintext raw path, E_FAIL and current400×116 JSON. Eight direct
suites passed8/8 (7.80 s): language_archive_command/language_source_update/
compression_limit/language_file_transfer/fs_folder/native_fs_operations/
source_extract_backend/external_launch, macOS arm64Qt6.11.2.

Dictionary/memory/solid tests now use geometry/JSON instead of unconditional
PNG captures. Current760×720/640×480 reports expose the affected combo fields,
source units and automatic prefix; scrolling off-viewport controls are covered
separately by reachability requirements. A fifteen-suite direct run passed11
and failed4 on stale Russian cancellation/Automatic expectations (28.85 s).
After fixture corrections, order/dictionary/solid passed in the four-suite run
(3 passed,1 failed;3.31 s); its last Deflate/Deflate64 Automatic expectation
was then corrected and compression_memory passed1/1 (1.76 s). Cancellation
checks now assert exact English and E_ABORT, with no production workaround.
The eleven passing suites include language UI/commands/update, compression limit/
general compression, flat view, list settings, auto refresh, navigation and
both progress consumers. These remain focused evidence, not a full .2 gate.

Main-window local read/scan/count captions and the editable address placeholder
now select EN/RU. `QtArchiveReOpen` uses source FileFolderPluginOpen's
IDS_OPENNING for the existing metadata reopening lifecycle; request/HRESULT/
transport state are unchanged. Language refresh now identifies only the source
parent row by its existing UInt32(-1) item index, preserving real directory
path tooltips. A regression checks directory row identity, path, selection and
address input/placeholder across EN→RU→EN. Whole-app reachability, accelerator/
long-label/fallback acceptance and the final full gate remain open; baseline
scope remains210 exact/152 adapted/3 local and no helper commit is made.

The copied `MyLoadMenu.cpp::OnMenuActivating` bookmark branch (source508–546)
now supplies its two UString loops through a narrow MyLoadBookmarks Qt entry
point. It retains IDS_BOOKMARK,100/50 long-path truncation, empty `-` and original
shortcut suffixes. Qt updates existing QAction objects instead of removing and
appending HMENU items; actual QKeySequence/dispatch stay intact, tooltip keeps
the full path as a local presentation extension. MainWindow's independent
Russian `— пусто` formatter is removed; hot switch invokes the same source body.
Unused disabled temp-bookmark block remains omitted as in upstream.

RootFolder's POSIX Volumes extension has no source resource ID and now uses
explicit local EN/RU instead of unconnected QCoreApplication::translate.
Windows FSDrives's Computer path restores its original LangString(IDS_COMPUTER)
expression. MainWindow hot switch invokes native CRootFolder::Init and projects
its names into existing items/native-name metadata by index, with sorting
temporarily suspended; it performs no filesystem enumeration/backend request/
folder binding. Existing root/drive workflow omissions remain unchanged.
The regression checks source root name, item identity/selection and980×620/
640×400 EN/RU JSON. Bookmark regression checks exact long-path truncation,
source captions and action/shortcut identity without backend calls.

Add reachability now exposes every named atomic control through the existing
scroll area in EN/RU at760×720/640×480, checks labels/button widths and the
persistent footer at every position. Bottom-position JSON inspected shows
selected-source list, Remove and preview/footer exposed; controls above that
viewport were checked at their own scroll positions. First added address test
expected the macOS `/var` alias rather than canonical `/private/var`; corrected
fixture passed language_ui1/1 (0.80 s). Add reachability passed1/1 (0.82 s),
then bookmarks/language_ui/main_shell passed3/3 (1.24 s). Final root_navigation/
volume_mount/bookmarks/language_ui/main_shell passed5/5 (1.20 s), macOS arm64
Qt6.11.2. An initial root projection build called the private concrete method;
it now uses the existing public IFolderFolder interface. Full .2 gate and
whole-app accelerator/fallback/long-label review remain pending.

The residual inventory also replaced initial auxiliary table captions with
existing GetNameOfProperty and explicit local Order, and bound the disabled POSIX
removable-drive explanation to EN/RU. Native column enumeration/settings and
FoldersPage state/apply bodies are unchanged. `language_ui/options_dialog/
panel_columns` passed3/3 (1.66 s), checking initial Path/Order, hot tooltip and
existing column persistence. The first build named an unregistered
column_layout_tests target and ran no tests; the registered panel_columns target
was built successfully before this check. Diff/format inspected on owned files.
Mnemonic inventory of current Add JSON finds the original RU `П` shared by Show
password and Parameters; EN has no such duplicate. Its Qt focus/cycling behavior
and form-label buddy review remain open rather than altering original assets or
claiming accelerator acceptance from a caption inventory.

## P1-14.1 — accepted Language/Options/main window integration (2026-10-01)

The integrated .1 scenario is accepted; whole-app .2 and parent remain open.
Source module baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, https://github.com/ip7z/7zip.git.
Common/Lang.cpp is copied unchanged and built with the existing FileIO in
sevenzip_filesystem; Lang.h was already vendored. Signature/UTF-8/escape/id/offset,
1 MiB limit and binary lookup bodies are unchanged. LangPage.cpp/.h preserve
native CLangInfo/CObjectVector, enumeration/filtering, sort, locale marks,
missing/extra ID comparison, 50-line/1500-character info limits and apply flow.
Qt replaces the property page/combobox/info/error services. POSIX enumeration
uses upstream CEnumerator::Next(CDirEntry,found)/Fill_FileInfo in place of the
Windows Next(CFileInfo) overload. Help remains P1-15; LangPage.rc/Res.h are exact.

LangUtils.cpp/.h now preserve load-once, source two-attempt default-language
selection, reload/path checks and lookup/fallback bodies. Windows retains the
original kLangs/FindShortNames and system/user LANGID logic; QLocale supplies
primary/regional codes only on POSIX where Windows LANGID services do not exist;
QRecursiveMutex protects source language storage when read by current worker
consumers. RegistryUtils Read/SaveRegLang retain source CuString calls through
the existing explicit FM INI service. No source-parser duplicate is introduced.

Language data is not in the upstream Git source checkout. Exact original
Lang/en.ttt and Lang/ru.txt come from the official 26.03 Windows archive, listed
by https://www.7-zip.org/download.html and retrieved without executing the
installer. Release URL, archive SHA-256/size and individual resource SHA-256
are pinned in resources/7zip/provenance.json; original resource headers and
distribution License.txt are retained. These assets have a separate archive
baseline, not a fictitious path/revision in the source Git manifest. Qt bundles
the files and stages them in an owned temporary directory only when no installed
module/Lang directory exists; the unchanged CLang file reader consumes that
directory. This does not write into the installation or the upstream checkout.

MyLoadMenu.cpp now copies k_LangID_TopMenuItems, CIDLangPair/FindLangItem and
MyChangeMenu's recursive control flow, popup inference, timestamp/bookmark skips,
OpenInside */#/Benchmark2 and tab suffix logic. QList/QAction replace HMENU/item
services. The source reset-to-English LoadMenu operation is folded into each
lookup's compiled STRINGTABLE/original en.ttt fallback; captions change in place
to preserve QAction identity, shortcuts, enabled/check state and connections.
The old fixed Russian ResourceString override is removed. Source English tables
have precedence over en.ttt, matching resource fallback. en.ttt is read by the
unchanged CLang parser and retained immutable for concurrent fallback readers.

OptionsDialog uses the original six page IDs/order and active CLangPage, source
modified/apply state, with Qt property-sheet services. Invalid Settings memory
input retains pending language until a successful Apply. Cancel preserves a
previous successful Apply. Qt LanguageChange replaces the original after-sheet
menu/toolbar/ReloadLangItems notification: current top-level widgets receive the
event after the source language is applied. MainWindow updates source menus,
toolbar button resource IDs, existing property headers and both panels' selected
status captions without changing row/selection/column state. CApp::ReloadLangItems
and AddSizeValue/AddValuePair1/2/GetItemsInfoString retain upstream native UString,
IDs and formatting bodies; QString conversion remains only at Qt presentation
boundaries. PanelSplitFile reuses the same source summary helper.

Qt text bindings store source IDs/fallbacks on presentation objects; editable
values are never rewritten. Options Settings/Folders/Edit labels and accessible
names use source IDs; Edit viewer colon and Menu other-bitness caption retain
source formatting. Menu item hot refresh reuses its source formatting body while
preserving flags/current zone. Folders Browse title now uses its source ID;
Edit native file filter masks replace independently written Russian filter names.
Settings invalid-argument errors use existing upstream ErrorMsg, including its
documented POSIX English system-code text. SettingsPageRes.h is copied exact.

Current focused language/language_ui/options_dialog/external_editors_ui/
source_properties/panel_shell6/6 passed3.91 s on macOS arm64 Qt6.11.2; additional
language_ui1/1 passed0.62 s after partial-language menu regressions. Tests cover
source parser failures and escapes, missing/traversal/malformed fallback, actual
bundled RU, Apply/Cancel/restart, invalid Settings with pending language, action/
shortcut/selection/header/toolbar state and pending editor commands. Partial
language tests preserve timestamp/CRC/bookmark labels, */# and tab suffixes;
missing language resets formerly Russian menus to English. EN/RU Options JSON
at640×390/520×380 for all five available pages inspected: grouping/order, bounds,
checkbox text widths and wrapping memory label fit; Qt standard buttons retain
platform text. These checks preceded the final gate recorded below. Parent P1-14
stays open for .2: other dialogs/messages, local main-window extras/tooltips without
source IDs, backend message context and whole-app accelerator/long-label review.
Windows/Linux runtime and rendered Windows reference parity remain unverified.

The first full unsandboxed gate built/formatted successfully but failed26/163
(464.16 s) after removal of the fixed RU table: source labels invalidated old
caption assertions and modal Close responders; properties_tests was explicitly
terminated after its stale-date responder left the modal open. No acceptance.
Tests now select actual Russian explicitly where that is their fixture, using
ru.txt's `Сжатый`/`Изменен`/`Тип Ошибки`; other default/fallback tests assert exact
source English. Progress modal responders now recognize source English Close.
Focused consumers22/25 passed25.20 s before remaining captions were corrected.
The full gate also exposed compact two-panel size: Panel.cpp's icon-only
VIEW_PARENTFOLDER control must not show its longer translated menu text. Qt
standard parent/back arrow buttons adapt that shared platform image service;
QAction menu captions remain intact. QtStatusLabel elides constrained status
text, retains full native text/accessibility/copy value and exposes it in tooltip;
it replaces Win32's bounded status part rendering only. Corrected focused checks
panel_key/properties_runtime/language_ui/panel_shell4/4 passed7.63 s after these
changes; current640×400 widget JSON inspected, all status fields/arrow controls
visible. Source Russian Prefix1030 is `Путь` (same translation as Path1003),
not the former independently written `Префикс пути`; fixture follows original IDs.
Navigation button images survive QAction::changed during reload. These focused
checks preceded the final gate below; .2/parent remain open.

The corrected full gate had162/163 passing tests in57.28 s; its remaining failure was the
Progress text contract's old fixed-RU Close/Foreground/Continue states. Those
expectations now use exact source English while keeping the existing grouping,
visibility, pause/error/close and resizing contracts. All code consumers passed
in that run. No global Russian override was restored or behavior checks removed.
The final full gate below followed that contract correction.

Final evidence: unsandboxed `./scripts/check.sh` PASS163/16358.87 s on macOS arm64
Qt6.11.2, format/build and source inventory/baseline checks passed (207 exact,
152 adapted,3 local). After the final contract correction its five source fixtures
passed10.33 s; latest panel_key/properties_runtime/language_ui/panel_shell4/4
passed7.63 s. Worktree/staged SHA-256 of all three original release assets match
provenance. No changes followed the successful full gate except acceptance docs.
P1-14.1 closes as one integrated user scenario; parent24/46 unchanged. P1-14.2
remains mandatory for other forms/messages/local Qt captions, accelerator and
long-label matrix before whole-app acceptance. No Windows/Linux/runtime or
rendered Windows parity claim is made from these macOS checks.

## P1-13.1 — source MenuPage/Options/Zone scenario (2026-10-01)

Immutable baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git. Copied/adapted
`CPP/7zip/UI/FileManager/MenuPage.cpp/.h`: CContextMenuItem/kMenuItems,
Clear_MenuChanged, LoadLang_Spec, OnInit list/Zone/default/custom-value bodies,
OnApply BoolPair/Flags/Zone flow, OnButtonClicked, OnCommand and OnItemChanged.
Native UInt32 flags and UString/MyFormatNew are retained. Qt property-page,
list/combobox/check controls and item check-state comparison replace Win32
messages; `_initMode` prevents initialization from dirtying the page. Options
keeps pageID2300 and source caption 7-Zip, adds its modified/apply handler.
Edit is the accepted P1-08.1/.2/.3 implementation, not a new rewrite.

Exact dependencies: Explorer ContextMenuFlags.h, resource.h/resource2.rc,
FileManager/MenuPageRes.h and GUI/ExtractDialogRes.h. ResourceString/CMake compile
the original Explorer STRINGTABLE into the existing Qt fallback service;
BITMAP entry is not a shell rendering port. Formatting is separate from the
functional adaptations. Provenance paths are recorded in upstream-baseline.json.

Reviewed consumers: original Explorer QueryContextMenu reads Cascaded/MenuIcons/
ElimDup/Flags; this application's ContextMenu.cpp currently ports only FindExt.
Its DLL/registry/service consumer is mandatory P2-09/P1-13.2. CShellDll native
DLL discovery, Check/SetContextMenuHandler/requery/error branches are omitted
until that adapter exists. Registration, Cascaded/Icons/ElimDup and item flags
therefore remain disabled with capability reasons on every platform. They are
not connected to the FM File menu or ordinary Extract defaults. Source Menu
ElimDup configures shell extraction, not FM extraction. Help is P1-15 and language
file/retranslation services P1-14; current captions come from source English.

WriteZone already has the original PanelCopy::CopyTo → IFolderSetZoneIdMode/File
→ CAgentFolder/Agent::Extract → CArchiveExtractCallback::CloseFile consumer.
The file-only 7zz transport feeds bytes into that source callback, which writes
Windows Zone.Identifier before timestamps. Only Windows enables this control;
POSIX preserves stored values without exposing a nonfunctional operation.
The constructor accepts that presentation-service capability for deterministic
tests; it is selected by IsZonePropagationSupported in the actual Options owner.

Relevant migration debt fixed: the old CContextMenuInfo Qt persistence removed
unchanged BoolPairs, unlike source Key_Set_BoolPair. It now writes only Def=true
pairs, preserves unmodified flags and deletes the WriteZone override for UINT32(-1)
as source Key_Set_UInt32 does. This avoids losing shell preferences on Zone-only
Apply. Source bodies remain in their existing authoritative modules.

Focused menu_page/context_menu_settings/options_dialog/external_editors_ui/
source_extract_backend passed5/5 in1.87 s. Full unsandboxed check.sh formatting,
build and161/161 CTest passed57.27 s on macOS arm64 Qt6.11.2. Inspected fresh
options-menu-640/520 JSON: source checkbox order, separate Zone label/combo,
120/110px list and exposed footer, no checkbox text clipping at640×390/520×380.
Exact baseline verification passed203 exact/149 adapted/3 local paths from the
pinned read-only checkout. Windows ADS assertion is a conditional test branch,
not an executed macOS check. Windows/Linux runtime and rendered appearance
remain unverified. P1-13 parent remains open for P1-13.2/P2-09, full goal24/46
unchanged; next available P1-14.

## P1-12.1 — Settings/Folders, work-dir и обе панели (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git. Скопированы и адаптированы
`CPP/7zip/UI/FileManager/FoldersPage.cpp/.h`: CFoldersPage, m_WorkDirInfo,
GetWorkMode/MyEnableControls/GetWorkDir/ModifiedEvent/OnButtonClicked/
OnFoldersWorkButtonPath/OnApply. Сохранены native CInfo/mode order, init/needSave
и apply flow; Win32 property-page controls заменены Qt, EN_CHANGE — textChanged,
Browse — существующий MyBrowseForFolder. FoldersPageRes.h скопирован без изменений.
Source help/lang services ещё не подключены; owners P1-15/P1-14. Только Windows
WorkDir поддерживает source removable-drive branch; POSIX checkbox disabled,
stored value сохранён, native device integration остаётся P2-05.

Partial OptionsDialog подключает Folders на source page index2/ID2400,
modified-state и Apply. Source WorkDir::CreateTempFile получает FmSettingsPath
по умолчанию вместо пустого settings path, чтобы существующий source update
consumer реально использовал сохранённую рабочую папку. Explicit settings path
по-прежнему доступен deterministic fixtures; GetWorkDir algorithm не изменён.

`SettingsPage.cpp/.h` возвращают исходные _largePages_wasChanged,
_memx_wasChanged, EnableSpin, RAM GB rounding/range и OnApply branches
(original111–267,271–320,417–478), включая native UString/ConvertStringToUInt32,
порядок general→large pages→memory save, UINT32(-1) для unset и source max1<<30.
Zero принимается исходным validation, но следующее OnInit трактует его как
unset. Commented legacy MemLimit combo/UnderCE/help/lang paths не объявлены
перенесёнными. QtMemoryLimitSpin заменяет Win32 edit/up-down: raw text до Apply
не исправляется, стрелки ограничены вычисленным source RAM range. QMessageBox
и возврат PSNRET_INVALID заменяют ShowErrorMessage/property-sheet notification;
Options остаётся открытым и выбирает Settings/input focus при неверном вводе.

`FM.cpp::IsLargePageSupported` (original486–503) добавлен к существующим
WM_COMMAND/WM_CLOSE bodies без их удаления. Source Windows capability сохранена,
POSIX возвращает false; большие страницы в Qt Settings disabled там, где нет
этой Windows privilege capability. RegistryUtils::ReadLockMemoryEnable/
SaveLockMemoryEnable используют прежние default/defined branches и Options INI
вместо Windows base registry. Gui EnablePrivilege_LockMemory заменён backend
передачей `-slp` в официальный engine: `Common/ArchiveCommandLine.cpp1124–1245`
проверяет risk и включает privilege внутри child process. Allocator/privilege
engine здесь не переписаны; Windows runtime не проверен локально.

`GUI/UpdateGUI.cpp::ShowDialog` working-dir block529–539 скопирован в прежний
partial module и минимально извлечён в GetCompressionWorkingDir: возвращается
WorkingDir field вместо ещё отсутствующего CUpdateOptions. Mode/current branch,
MyGetFullPathName, GetWorkDir/CreateComplexDir сохранены. Backend Create stages
передают actual source path через `-w`; native Console/Common Update owns temporary
archive prefix, final publication остаётся в существующей backend transaction.
FM update использует единственный CWorkDirTempFile. Его build ownership перенесён
из sevenzip_location в sevenzip_filesystem, чтобы GUI create и Agent update
использовали один модуль без library cycle; filename/path/functional body те же.

`App.h::RefreshAllPanels` (original172–181) скопирован с NumPanels/
LastFocusedPanel/OnReload loop. Applied consumer перечитывает source list flags
и обе видимые панели. Qt reload presentation для archive теперь LoadItems/
projection/selected-state restore; filesystem сохраняет прежний scanner owner.

Focused options_dialog/work_dir/external_editors_ui3/3 PASS1.26 s macOS arm64
Qt6.11.2. Folders geometry/JSON640×390/520×350 просмотрены: source radio order,
path/Browse row, removable checkbox, отдельный footer без clipping.
Проверены Cancel/Apply-then-Cancel, invalid config defaults и actual Unicode
temporary file/cleanup. Final normal640×390/compact520×380 Folders/Settings JSON
проверены; compact height выросла из350 из-за actual Settings minimum361.
Focused8/8 PASS5.72 s (options_window/options_dialog/fm_settings/settings/
panel_auto_refresh/archive_opening/external_editors_ui/work_dir), поздние
options_dialog/archive_backend2/2 PASS1.03 s. Actual Create с Unicode work dir
и отказ occupied work path, archive_mutation cancellation consumers проверены.
Первый gate160/160 PASS56.85 s предшествовал GUI Create consumer. Следующий
gate159/160 выявил test focus assertion: offscreen hasFocus зависит от foreground,
но pending input widget в dialog.focusWidget правильный. Проверка input focus
исправлена; focused options_dialog1/1 PASS0.45 s. Final unsandboxed check.sh
formatting/build/160 из160 CTest PASS55.99 s, macOS arm64 Qt6.11.2,2026-10-01.
P1-12.1 принят; следующий доступный ID P1-13.
Parent P1-12 остаётся открыт до .2 и остальных page owners; три pages disabled.
Windows/Linux runtime/rendered Windows parity не проверены.

## P1-10.2 — полный Properties flow и доступная metadata (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git. В mirrored `FileManager/PanelMenu.cpp`
восстановлены `kSpecProps`, AddSeparatorSmall и Properties item/raw/folder/archive
level/non-open bodies (original100–420). Сохранены UString/AString/CRecordVector,
GetPropertyInfo/GetRawPropInfo loops, пропуск undefined/empty, CRC/checksum upper
hex до8 bytes, lower hex для остальных, security formatting, data:N после256
bytes, source totals и reverse level ordering с MainItemProperty sections.
FormatUtils/PropertyName/PropIDUtils/ListViewDialog/EditDialog остаются прежними
единственными source implementations. Source localized selected-count format
восстановлен вместо QString::arg; временные Russian resources для двух captions
используют существующий ResourceString adapter до language owner P1-14.

Agent.cpp/.h восстанавливают исходные IFolderProperties, IGetFolderArcProps и
IFolderArcProps, kFolderProps/GetFolderPropertyInfo/CRC и GetArcNumLevels/
GetArcProp/GetArcNumProps/GetArcPropInfo/MainItemProperty bodies
(original1232–1379,1892–2002). Body/paths/names сохранены. Две codec-index lookup
ветки ErrorType не имеют file-only service и возвращают VT_EMPTY; их P2-01 scope
сохранён, формат ошибки не выдумывается. GetTypeOfArc текущего one-handler owner
использует actual engine ArchiveType; in-process codec catalog не объявлен
перенесённым. Windows-only InvokeSystemCommand("properties") заменён общим Qt
Properties и native IFolder enumeration для файловой системы; shell integration
остаётся P2-09. Modal construction вынесена в уже существующий PropertiesDialog.

File-only adaptation: backend сохраняет исходные listing bytes отдельно от
trimmed/redacted journal. Reverse parser для `Console/List.cpp` technical output
в upstream отсутствует; Qt glue сохраняет фактически полученные item/header
fields и brace multiline headers. CRC возвращается typed UI4; размеры сохраняют
полный UInt64, absent/invalid остаются undefined, zero остаётся defined.
Остальные доступные CLI display fields возвращаются named BSTR без выдуманных
native типов; это сохраняет engine representation, включая его normalization.
Generic properties и header names могут оставаться английскими до language
integration. Пароли не печатаются новым adapter в журнал.

`Console/List.cpp::CFieldPrinter::PrintItemInfo` raw branch (590–647) допускает
полный hex только до64 bytes. QtPanelFolder raw service восстанавливает фактически
полученные Checksum/SHA-1/SHA-256 bytes и хранит их в immutable metadata owner;
missing item data возвращает empty raw. Реальный SHA-256 manifest проверяет
`Common/HashCalc.cpp::NHash::CHandler::{GetNumRawProps,GetRawPropInfo,GetRawProp}`
(1059–1109): checksum — kpidChecksum/NPropDataType::kRaw. Handler/decoder bodies
здесь не копируются и не реализуются, Hash workflow остаётся P2-03. `data:N`,
decoded security/reparse и большие raw payloads сохраняются только как actual
display text; bytes, которых CLI не отдаёт, не синтезируются. Native Properties
raw branch отдельно сохраняет исходные limits/formatting для полноценного provider.

CProxyArc2 decision: полные copied AgentProxy.cpp/.h bodies и BSTR lifetime fix
сохраняются с прежним provenance. Process transport не отдаёт GetParent/raw names/
ADS graph; checksum-only raw service возвращает E_NOTIMPL для parent и не
активирует proxy2. Source CProxyArc остаётся authoritative path tree и считает
explicit/implicit directory totals/CRC. Alternate-stream dispatch/binary graph
сохраняется обязательным P2-06; reparse/security capabilities — P2-05. Нет
claims полного Agent.cpp/ArchiveFolderOpen.cpp или in-process codec port.

`source_properties` проверяет actual ZIP CRC и Hash checksum bytes, UInt64 max,
defined zero/undefined, sparse SHA raw catalog, source folder/arc levels и
MainItemProperty/non-open errors, длинный Unicode header, полный clipboard/detail.
Properties consumers обновлены с item-only expectations на исходную folder
summary и native path naming; unavailable path не заменяется snapshot prefix.
UI presentation properties и actual engine properties хранятся раздельно:
formatted snapshot не расширяет native archive catalog. Unsupported CRC у UI
snapshot сохраняет E_NOTIMPL. Runtime fixtures различают item и folder sections
с одинаковыми source names. Первый full gate выявил эти три consumer проблемы
и TAR extraction failure: после сохранения blank values пустая Symbolic Link
ошибочно считалась ссылкой. Link flag теперь требует непустого actual value;
existing nested_archive TAR и unsafe archive_backend regressions проверяют
извлечение обычного файла и отказ для настоящих unsafe paths/links.
Focused5/5 PASS1.03 s; final SHA sparse1/1 PASS0.50 s, macOS arm64 Qt6.11.2,
2026-10-01. Geometry/JSON540×360/360×240 просмотрены, source ListViewDialog.rc
задаёт list/buttons; rendered Windows reference и Windows/Linux runtime
не проверены. После исправлений focused6/6 PASS2.04 s (panel_folder,
archive_property_catalog, properties_runtime, source_properties, nested_archive,
archive_backend). Повторный unsandboxed check.sh: formatting/build и158/158
CTest PASS55.35 s. P1-10.2 и parent P1-10 приняты в описанном file-only scope;
P2-01/P2-05/P2-06/P2-09/P2-10 остаются обязательными. Следующий ID P1-12.

## P1-10.1 — исходный F3 folder-size workflow (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git, `CPP/7zip/UI/FileManager/PanelItems.cpp::EditItem(bool)`
lines1043–1085. Восстановлен полный исходный body: !useEditor QI
IFolderCalcItemFullSize, Get_ItemIndices_Operated, обход выбранных папок,
CalcItemFullSize(index,NULL), needRefresh/InvalidateList/return; иначе focused
native index и исходная file branch. Контейнеры, порядок и игнорирование HRESULT
сохранены; F4 папки не запускает. Уже принятый `FSFolder.cpp::{CalcItemFullSize,
CFsFolderStat::Enumerate}` и FolderStat_Defined остаются единственным владельцем
размеров/числа объектов. Archive Agent не предоставляет CalcItemFullSize:
его размеры explicit/implicit папок уже вычисляет source CProxyArc, новый
параллельный calculator не добавлен.

Win32 InvalidateList заменён narrow Qt invalidation presentation: существующие
строки перечитывают source properties и numeric sorting data без LoadItems или
повторного binding, которые уничтожили бы FolderStat_Defined. Callback ограничен
активной панелью, сохраняет selection/focus, обновляет status/actions.
Проверка Qt availability теперь разрешает F3 для operated folders только при
реальном IFolderCalcItemFullSize; F4 по-прежнему требует focused file.
Новый coherent `mainwindow_propertyinvalidation.cpp` содержит только projection.
Расчёт сохраняет исходный синхронный NULL-progress flow; responsive large-tree
adaptation остаётся обязательным P2-10. Прежние symlink/reparse ограничения
FSFolderStat не объявляются решёнными этим шагом, owner P2-05.

`folder_size_view` использует реальный CFSFolder, keyboard F3, Unicode/space tree,
пустую папку (defined zero), несколько выбранных папок с фокусом на файле;
проверяет source totals, displayed sizes, неизменность provider, selection/focus
и отсутствие archive operations. Normal980×620/compact640×400 current-build
JSON и geometry просмотрены. Focused5/5 PASS1.41 s macOS arm64 Qt6.11.2,
2026-10-01. Первый gate обнаружил missing source registration у root_navigation;
Qt projection подключён автоматически ко всем targets с mainwindow.cpp.
Focused root_navigation/folder_size_view2/2 PASS0.59 s. Финальный unsandboxed
check.sh formatting/build/157 из157 CTest PASS54.89 s. P1-10.1 принят;
parent P1-10 не закрыт:
Properties/raw/folder/archive levels/CRC/full-value acceptance остаются .2.
Windows/Linux runtime и rendered Windows parity не проверены.

## P1-09 — обратная запись вложенной цепочки (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Переиспользованы уже скопированные mirrored
`FileManager/PanelItemOpen.cpp::{OpenAsArc,OpenParentArchiveFolder,OnOpenItemChanged}`,
`PanelFolderChange.cpp::{CloseOneLevel,OpenParentFolder}`, `Agent/Agent.cpp::CanUpdate`,
`ArchiveFolderOut.cpp::CommonUpdateOperation`, `AgentOut.cpp::UpdateOneFile` и
`UpdateCallbackAgent.cpp`. Parent link, CTempFileInfo, FileInfo change check,
индекс записи, cached password, Yes/No/Cancel, source ReOpen и удержание файла
при ошибке остаются исходными. Новый алгоритм parent chain не введён.

Необходимая file-only адаптация: исходный маленький CVirtFileSystem stream
материализуется через исходные COutFileStream/Create_NEW/CopyStream в тот же
FilePath, которым владеет CTempFileInfo. Раньше Qt manager записывал этот stream
в отдельный private spool, изменения которого source parent link не видел.
`IQtArchiveStreamOpenServices` передаёт признак владения через существующий
FileFolderPluginOpen worker; после успешного открытия OpenAsArc снимает IsVirtual
и фиксирует реальную FileInfo. QtArchiveFileResource не удаляет owned файл;
исходный parent link остаётся единственным владельцем cleanup. Неуспешный probe,
неверный пароль, cancellation и copy failure удаляют только собственную копию;
это сохраняет исходный Create_NEW fallback FlushToDisk. Generic unowned streams
продолжают использовать private spool. Цена адаптации — disk I/O для небольшого
вложенного архива, который upstream мог держать только в памяти.

Capabilities проверяет source IsThereReadOnlyFolder по каждому parent level;
engine UpdateEnabled service разрешает только текущие ZIP/7z. Qt nested-only
guards сняты у Add/Rename/Delete/CreateFolder/Move; encrypted Move и дополнительный
built-in text editor сохраняют прежние ограничения. Native F4 работает с cached
password. TAR/GZip, составной tar.gz и остальные handlers без update service
не перепаковываются независимым packer. Source No и Cancel отклоняют изменения
и удаляют temp; ошибка update сохраняет изменённый temp и внешний archive.

Agent/ArchiveFolderOpen.cpp::OpenFolderFile изучен: его in-process archive->Open
и codec/icon DLL services остаются ранее указанным P2-01/P3-05 scope. Здесь
используется уже принятый QtArchiveFolderManager typed transport и исходный Agent
owner, а не заявляется полный порт ArchiveFolderOpen.cpp или OpenArchive::Open2.

`nested_edit_chain` проверяет реальные Unicode/space inner7z/outerZIP bytes,
F4 и Rename/Delete/CreateFolder, protected оба уровня, ошибки/отмену каждой записи,
No/Escape parent prompt, wrong/cancel inner password, read-only parent/child TAR,
cleanup/retention и parent listing/focus. `external_launch` также проверяет
rejected memory probe → обычный external fallback. Focused11/11 PASS7.70 s,
macOS arm64 Qt6.11.2, 2026-10-01. Prompt geometry/default/escape и current-build
JSON проверены при owner980×620 и640×400. Windows/Linux runtime и rendered
Windows parity не проверены; source MessageBox establishing workflow не является
pixel reference. Первое полное выполнение выявило устаревшее memory-only
ожидание source_extract_backend; тест теперь проверяет bytes materialized file
и source cleanup. Повторный focused2/2 PASS4.35 s; финальный unsandboxed
check.sh format/build/156 из156 CTest PASS54.56 s. Baseline verify PASS197 exact,
145 adapted, 3 local. P1-09 принят; следующая обязательная итерация P1-10.

## P1-08.3 — VerCtrl source workflow (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Copied the complete FileManager/VerCtrl.cpp
into its mirrored path: ConvertPath_to_Ctrl, CFileDataInfo::Read, 256 MiB buffer
limit, ParseNumberString, CreateComplexDir_for_File, WriteFile, FILETIME helpers
and CApp::VerCtrl. Original CByteBuffer, comparisons, branch order, readonly
guards, `_7vc/<filename>/001...` history enumeration/rename, Commit precision
loop, Revert default-No overwrite and Diff order remain. Source WriteFile's
void/error and partial-write semantics remain; this is not an atomic version store.
Formatting is the repository clang-format adaptation, separate from behavior.

Windows uses original BY_HANDLE_FILE_INFORMATION/CInFile/COutFile/CEnumerator
and SetFileAttrib. New QtVerCtrl file services project only unavailable POSIX
Win32 methods: upstream FileIO fstat/read_full/write_full/create/time APIs,
upstream FILETIME conversions and POSIX enumeration's explicit found output.
POSIX preserves high permission bits; readonly removes write permissions, and
clearing readonly restores owner-write. macOS birth time supplies creation time;
other POSIX uses stat ctime, and upstream POSIX SetTime cannot promise Windows
creation-time restoration. mtime/atime use upstream FILETIME precision. Windows
CE/legacy presentation is outside the Qt-supported targets. CPanel error/parent
methods regain upstream public visibility for the source WriteFile helper.

Copied RegistryUtils ReadReg_VerCtrlPath uses source `7vc` key through shared
FM INI. Upstream has no UI writer for this advanced setting; no new setup dialog
is invented. CFileMenu remains the only owner of Diff/7vc/one-file/readonly
visibility, names and ordering. Qt builds appended actions, binds native IDs,
reads 7vc for program/context projections and refreshes source selection and
availability after execution. Released/unbound panels produce no projection;
direct-consumer tests exposed and fixed an initially unguarded refresh call.

`verctrl` fixtures cover two backup versions, readonly/mtime preservation,
exact hour rounding, controlled Diff argv, identical Edit and timestamp-only
Revert, Yes/No/Escape, missing backup, timestamp disagreement, unavailable backup
root, source buffer limit, absent configuration and multiple selection. Revert
uses original OverwriteDialog with default No and hidden extra buttons; normal
700×440 / compact560×440 JSON and main-window980×620 / 640×400 geometry reviewed.
Focused7/7 PASS2.32 s; final compact/default-No assertions pass1/1, 0.63 s.
Unsandboxed check.sh PASS155/155, 49.53 s, format/build/inventory PASS;
baseline verify PASS197 exact/145 adapted/3 local. P1-08.3 and parent P1-08
accepted with integrated .1 disk/settings/Diff and .2 archive process/update
consumers in that gate. No omitted VerCtrl functional branch;
Windows/Linux runtime and rendered Windows visual parity remain unverified.

## P1-08.2 — archive external-process integration (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Copied PanelItemOpen.cpp CChildProcesses
(284–431) and known-process MyThreadFunction (1133–1328) into the authoritative
mirrored module. Original containers, sorted PID set, parent/same-name discovery,
firstPass, 60-handle limit, two-second fallback, changed/unchanged checks,
read-only error, Yes update, No/Cancel cleanup and failed-update retention remain.
Handles and Win32 wait/snapshot calls use narrow IQtProcessServices adapters;
CSnapshotProcess is exported to the service header because both boundaries need
its record. QElapsedTimer supplies the monotonic GetTickCount interval.

QtNativeProcessServices supplies OS operations only. Windows reuses unchanged
Windows/ProcessUtils.cpp CProcess::Create (same pinned source), with Unicode
build selection, Toolhelp snapshots and waitable process handles. Supported Qt
Windows versions use QueryFullProcessImageNameW rather than legacy dynamic PSAPI.
POSIX uses posix_spawnp with literal argv, waitpid for owned children, and process
identity stamps for opened PIDs (macOS libproc start time, Linux proc start time).
Closing a live owned POSIX handle leaves the app running and starts a PID-only
reaper; it does not retain UI/archive objects. No shell is involved. A fresh
process group retains child lineage after POSIX orphan reparenting; snapshot
ParentId projects that group to the source-known PID set. Descendants that
deliberately leave the group still require observed native parent/same-name
capability; a controlled child surviving its parent verifies inherited lineage
on the local macOS runtime. Windows/Linux runtime remains unverified.
Platform references: [POSIX spawn](https://www.man7.org/linux/man-pages/man3/posix_spawn.3p.html),
[wait](https://www.man7.org/linux/man-pages/man3/wait.3p.html), macOS SDK libproc.h /
sys/proc_info.h and [Apple libproc implementation](https://github.com/apple-oss-distributions/xnu/blob/main/libsyscall/wrappers/libproc/libproc.c).

QtExternalGuiCall projects MessageBox/SendMessage through a queued, cancellable
request. Pending or checking calls keep worker records alive; GUI close sets the
panel-owned exit flag before joining. A delivered modal call retains its record
through completion and uses existing CanCloseNow. MainWindow defers requests
until backend/operation/modal owners are free; runExternalArchiveUpdate marks
source progress/operation ownership, suppresses ordinary backend completions and
refreshes the edited panel with source selection before restoring the active one.
StartEditApplication now obtains a known handle for archive launches; disk
detached launch and no-handle association remain distinct. Archive F3/F4 are
connected, including source read-only edit/error semantics. Wait handles expose
Running/Exited/Error separately, preserving source wait-error retention instead
of interpreting failed OS waits as normal process exit.

Real fixtures cover ZIP Yes/No/Escape-Cancel, F3/F4, metadata/selection refresh,
active-process and pending-request window close, failed/cancelled update,
read-only TAR preservation, busy deferral, stale location and another focused
panel. Native lifetime fixtures cover orphan child discovery, short/long unchanged
cleanup, launch failure and injected wait failure. Extraction failure/cancel and
password cancellation leave originals intact. Protected ZIP AES and 7z headers
are checked with correct and wrong passwords after updates.

The ZIP regression exposed an existing CLI adapter omission: input password
delivery alone did not define output encryption for newly compressed data.
Inspected pinned ZipHandlerOut.cpp 392–430 (CryptoGetTextPassword2,
Password_Defined, inherited AES mode), ZipUpdate.cpp SetEncrypted and CLI password
callback. ArchiveBackend::updateSourceFile now adds bare `-p` for protected ZIP;
the password stays on stdin. Existing 7z inherited encryption is retained.
This is a transport adaptation, not a new format implementation. The fixture's
ZIP password is ASCII because upstream rejects non-ASCII ZIP passwords;
7z uses a Unicode/space password. Existing no-handle and direct consumers pass.
Prompt text/geometry JSON reviewed at normal/compact owner sizes 980×620/640×400;
all labels/buttons fit. Focused 8/8 PASS, 4.85 s macOS arm64 Qt6.11.2.
Unsandboxed check.sh PASS154/154, 48.27 s; format/build and inventory PASS.
Exact baseline verify PASS197 exact/144 adapted/3 local paths. P1-08.2 accepted;
P1-08.3 VerCtrl and parent acceptance remain mandatory. Old Windows CE branches,
debug traces and legacy runtime API lookup are omitted for Qt-supported systems;
the original functional discovery/wait/update bodies remain authoritative.
Windows/Linux runtime and rendered Windows visual parity unverified.

## P1-08.1 — integrated disk View/Edit/Diff (2026-10-01)

Same pinned baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Inspected FM PanelItemOpen.cpp helpers
StartAppWithParams/StartEditApplication/EditItem (697–833), CApp::DiffFiles,
known/child-process worker (1133–1328), EditPage.cpp/.h/.rc/EditPage2.rc,
OptionsDialog.cpp six-page order, RegistryUtils Diff keys, VerCtrl.cpp and
Common/TempFiles.cpp. Existing no-handle association and native update bodies
remain authoritative, rather than being replaced by the built-in text editor.

Copied disk CPanel::EditItem and StartEditApplication bodies into the mirrored
PanelItemOpen.cpp. Native UString, SplitCmdLineSmart, program/parameter ordering,
per-mode settings and Windows directory/notepad default remain. CProcess Create
is a narrow injected Qt service; StartAppWithParams sends original parsed base
parameters and separate path vector instead of serializing path argv on POSIX.
Windows service retains native parameter string and original quoted path loop;
POSIX QProcess splits base arguments and appends literal paths, without shell.
Disk launches detach as source disk CProcess destructor closes its handle without
waiting. This service is explicitly unsuitable for archive known-process editing.
POSIX has no upstream WindowsDir/notepad default: an unconfigured viewer uses the
OS association, while Edit requires configuration and reports source launch error.
RegistryUtils adds copied SaveRegDiff/ReadRegDiff through existing FM INI service.

Copied CApp::DiffFiles overloads preserve selected-pair order, one-selected-item
opposite-panel selection and relative fallback, including flat-to-normal basename.
Copied PanelItems EditItem(bool) focused-file branch uses the native selection
owner; GetFocusedFileIndex shares its guards with Qt availability. F3/F4 now
route to native disk EditItem; configured Diff uses native menu projection and
is hidden when its command is empty. The existing limited text viewer/editor
remains an explicit extra File menu feature without F3/F4 shortcuts.

New mirrored EditPage.cpp/.h retains CEditPageCtrl, three controls, OnInit,
WasChanged/OnCommand/OnApply and Browse flow. QLineEdit signals project EN_CHANGE;
CBrowseInfo replaces Win32 file picking. Browse replaces the whole command,
including discarding old parameters as upstream does. POSIX uses an all-file
application filter; Windows retains exe/all-files. EditPageRes.h is copied
unchanged. All use the same pinned baseline above and are in the manifest.

OptionsDialog.cpp is a partial P1-12 prerequisite: original six-page order and
IDs are retained, with Editor and existing Settings functional, four unavailable
pages disabled. QtFmOptionsDialog names the presentation container to avoid the
existing GUI COptionsDialog class. Qt property-sheet services supply modified
state/Apply/OK/Cancel and call each source OnApply. CSettingsPage can be embedded
without its standalone footer; MainWindow applies committed list flags immediately.
Help, System/Menu/Folders/Language bodies and Settings memory/large-pages branches
remain with P1-12/13/14/15/P2-08. This is not a full OptionsDialog source-port claim.

Still omitted: archive known/child-process wait and complete changed-temp flow
(P1-08.2); VerCtrl.cpp workflow (P1-08.3); source F3 selected-folder
CalcItemFullSize branch (P1-10). Archive F3/F4 and folder F3 remain disabled.
Source process and property dependencies must be ported before those consumers
are enabled; no-handle association does not validate them. Parent remains open.

Focused current-tree build and external_launch PASS 1/1, 0.73 s macOS arm64,
Qt 6.11.2. Tests cover native disk EditItem with separate Viewer/Editor settings
and exact source path vector, real owned test executable at Unicode/space path,
literal argv including `$()`/semicolon, launch failure and Diff key independence.
macOS Qt process argument encoding yields decomposed Unicode; semantic fixture
comparison uses NFC, while the native injected service test checks exact UString.
Integrated external_editors_ui checks Apply/Cancel/Browse, F3/F4 focused-file
launch, pair/opposite/fallback Diff and normal/flat paths, plus launch failure.
Editor options geometry and reviewed current-build JSON cover 640×390 and
520×350 against EditPage2.rc grouping/order. Final focused 9/9 PASS, 2.18 s;
unsandboxed check.sh 151/151 PASS, 45.16 s on macOS arm64 Qt 6.11.2.
Initial gate exposed binding_lifecycle's old archive F3 assertion; it now
checks extra text-view availability and disabled archive native F3 separately.
Inventory PASS; exact baseline verification: 196 exact, 144 adapted, 3 local
paths across the pinned revision. No Windows/Linux runtime or rendered Windows
parity verification. P1-08.1 accepted; parent P1-08 remains open.
Windows/Linux runtime remain unverified.

## P1-07.2 — source lifecycle integration (2026-10-01)

Baseline remains `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git. Inspected source `GUI/UpdateGUI.cpp`
flag projection (429–468), `GUI/CompressDialog.cpp` PreserveATime capability,
Options and OnOK bodies, `Common/ZipRegistry.cpp/.h` global PreserveATime
BoolPair persistence, `Common/ArchiveCommandLine.cpp` `-ssp`/`-ssw`/`-stl`,
and `Common/Update.cpp` latest-time/volume assignment (847–908) and
post-publication deletion (1853–1930).

The existing copied Options bodies now expose archive mtime. The source global
PreserveATime BoolPair and registry save/load calls are added through the existing
INI service. Qt request DTO fields preserve source names; the backend translates
lifecycle flags to the original CLI switches outside method properties. The
official engine calculates latest mtime for multi-root create; CMultiOutStream
receives that completed-output MTime/MTime_Defined before writing volumes, as in
Update.cpp. For existing Add, `Common/Update.cpp::Compress`'s latest-time loop
(805–845) is copied into `GetUpdateArchiveMTime`, retaining native pairs,
full-precision disk times and retained archive properties. AgentOut merges the
original update callback's LatestMTime after success (source 883–897), then
transports CFiTime to CWorkDirTempFile before native MoveToOriginal publication.
This also covers Freshen without new data. CopyStream transfers bytes only;
SetMTime forwards to the existing upstream COutFileStream service. Original
transaction, failure and reopen branches remain intact; formatting is separate.
AgentOut.cpp also projects source PreserveATime/ShareForWrite into the original
CArchiveUpdateCallback used for staging original inputs, before the CLI reads
its temporary copy. ApplyOutputMTime lives in qtarchivefileresource.cpp so native
callback tests do not acquire dependencies on the complete CLI update service.

PreserveATime is a Windows capability: pinned `Windows/FileIO.cpp::CInFile::Open`
only performs SetFileTime suppression under `_WIN32`. Real macOS `-ssp` accepted
the flag but changed atime. POSIX Options therefore disables it with a reason;
direct backend requests fail before engine launch. A persisted Windows setting
is retained in INI but excluded from POSIX effective requests. No independent
post-read timestamp restoration is introduced. Source Shared/Delete controls
are projected in CompressDialog.rc order, ahead of Encryption. Shared writing
is enabled only on Windows, where the native file opening service implements
sharing modes. Delete defaults off and is not persisted; source action-set
checks reject Update/Freshen/Synchronize with deletion before any engine launch.

`Common/Update.cpp/.h` is a new **partial port**, inventoried at the same pinned
revision. Copied bodies: UpdateArchive's action-set check (1147–1153), deletion
phase (1853–1930), CDirPathSortPair/processedItems/live Size/MTime/CTime guards,
deepest-first empty-directory cleanup, and the latest-time loop above. The
full Compress/UpdateArchive writer, rename/censor/codec/multivolume pipeline and
their earlier helpers remain omitted. Check/helper extraction and a narrow
IUpdateDeletionCallback replace access to the monolithic writer's local state;
callback method names and functional control flow remain original. Added
snapshot-size/null validation protects the adapter contract. Native deletion
ignores physical DeleteFileAlways/RemoveDirAlways_if_Empty failures, as upstream;
it does not delete skipped or changed files. No separate deletion algorithm.

`QtArchiveDeleteInputs` holds native snapshots/processed flags. Native Add
captures original GetStream completion flags and runs deletion only after
MoveToOriginal; existing F6 moveMode flow remains separate. Create enumerates
original inputs with authoritative EnumDirItems in a pause/cancel QThread before
CLI stages. Exit-zero completion of every stage and final single/volume
publication precede marking those scanned inputs processed and deleting them.
Nonzero/warnings, launch/publication failure or early cancellation preserve
sources. Cancellation during deletion can leave the published archive and a
partially deleted source set, matching source phase ordering. Create with links
rejects delete-after-compress until the P2 link owner is ported. EnumDirItems and
SortUtils move from sevenzip_location to sevenzip_filesystem ownership; their
bodies are unchanged and compile once. Qt worker/mutex/queued progress and the
backend prepare/delete jobs are integration glue, not borrowed algorithms.

Compression preflight retry/cancel follows copied CompressDialog checks and is
covered through the Add form. The remaining real **decoder** memory-request
transport is mandatory P1-18 (historical P1-07.2 scope); parent P1-07 acceptance
excludes only this explicitly separated function, not the rest of the plan.

### P1-18 prerequisite — actual RAR decoder memory callbacks

Historical prerequisite investigation (2026-10-01); the capability gap below
is superseded by P1-18 implementation. Its older checks do not validate new code.

Pinned `Archive/Rar/Rar5Handler.cpp::CHandler::Extract` is the actual caller of
IArchiveRequestMemoryUseCallback (3009 and 3145). Its dictionary/header/solid
sequence logic supplies native required/allowed sizes and flags. Our mirrored
Rar5Handler currently ports only BLAKE2sp registration; the real reader/decoder
is external. `Console/ExtractCallbackConsole.cpp::RequestMemoryUse` (556–619)
prints an English diagnostic and returns Allow/Skip locally; it has no GUI
request/answer protocol. Parsing that text or guessing from Method strings is
not a source port. Existing FM RequestMemoryUse/CMemDialog tests exercise a
synthetic callback only. Prerequisite: copied native RAR handler/reader, decoder,
crypto/stream dependencies and one authoritative registration, routed through
ArchiveBackend to the existing source GUI callback (or an explicit source-derived
structured engine bridge). Revisit with that capability and deterministic real
RAR5/RAR7 fixtures testing Allow/Skip/Cancel/remember on worker callbacks, including
root/selected/Test/batch consumers. P1-12 Settings persistence and P1-08–16 do
not need a running decoder callback; final P3 traceability/omitted closure do.

Current-tree macOS arm64/Qt 6.11.2 focused checks (2026-10-01): 11/11 PASS,
3.97 s (creation lifecycle, update callback, volume creation/UI, advanced,
compression limit/memory/preferences, source update, UI update, extract prompt).
The earlier 149-suite gate belongs to the preliminary timestamp integration;
final unsandboxed check.sh passes formatting/build and 150/150 CTest (42.21 s).
The first final gate had 149/150 PASS (42.92 s), with only inventory rejecting
new files absent from Git index; staging them corrected inventory/baseline (2/2,
0.54 s), and the complete rerun passed. Current-build
Options/Add/lifecycle and memory-limit JSON at normal/compact sizes is reviewed;
Windows/Linux runtime and rendered Windows reference remain unverified.

## P1-07.1 — source Advanced Options и параметры (2026-10-01)

Baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied the complete OPTIONS section of `CPP/7zip/UI/GUI/CompressDialog.cpp`:
`COptionsDialog` Bool1/BoolBox helpers, precision construction, time capability
states, `OnInit/OnCommand/OnButtonClicked/OnOK`, and the original
`CCompressDialog::ShowOptionsString` helpers/body. Original names, native
CBoolPair/UString/CFormatOptions and control flow remain in the mirrored CPP.
`CompressDialog.h` receives original CBool1 and CInfo property fields/defaults.
The adapted COptionsDialog declaration is in `qtcompressoptionsdialog.h` because
the mirrored DTO header also serves Core-only engine consumers. This avoids
adding a Widgets dependency to those consumers; it is not a second implementation.

Copied `GUI/UpdateGUI.cpp`'s entire property-helper block: `AddProp_*`,
`SplitOptionsToStrings`, `IsThereMethodOverride`, `ParseAndAddPropertires`,
`SetOutProperties`. `GetCompressionProperties` is the narrow adapter for the
original ShowDialog call order. Native CObjectVector<CProperty> is projected
to individual `-m` argv entries by ArchiveBackend, replacing its independent
property construction. The existing QString encryption field needs a conversion
overload and `isEmpty`; other added fields retain native types. Widgets do not
parse parameters or construct engine switches. `UpdateGUI` and its original
UpdateAction dependency now compile once in sevenzip_panel so backend consumers
and sevenzip_location share the same authoritative body.

`Common/ZipRegistry.h/.cpp` gains original Options/TimePrec/CBoolPair profile
fields and their Save/Load order. Native registry DWORD operations use QSettings;
undefined pairs remove the key. The GUI resource header is an exact unchanged
copy at `third_party/7zip/CPP/7zip/UI/GUI/CompressDialogRes.h`, inventoried in the
pinned manifest. Source notices and license remain; formatting is clang-format.

`qtcompressoptionscontrols.*` supplies Qt controls and original control service
names; `createarchivedialog_advanced.cpp` projects pinned `7zRegister.cpp` and
`ZipRegister.cpp` time flags into the original CArcInfoEx owner. Qt applies
OnInit visibility before calculating minimum size. Resources
`CompressOptionsDialog.rc` and `CompressDialog.rc` establish grouping/order.
Options OK updates a temporary per-format profile; Cancel discards it, and Add
Cancel never writes INI. The source summary accompanies the Options button.

Partial scope remains explicit: no complete CompressDialog/ShowDialog/UpdateGUI
port is claimed. Shared writing, preserve access time, archive mtime,
delete-after-compress and memory retry/cancel remain P1-07.2; unsupported lifecycle
controls are disabled and SetArcMTime is rejected by the adapter. NTFS/link flags
are unsupported in this owner and hidden by native capability policy; their
functional owners remain P2-05. Retained tar/gzip Options branches are not exposed
by the current ZIP/7z Add form. Help remains disabled pending P1-15.

`compression_advanced` checks normal/compact geometry, source capability states,
Cancel and per-format persistence, method override, real ZIP/7z mtime precision
round-trips, omitted mtime, invalid/unsupported properties and cancellation with
no partial output. DOS rounding follows source TimeUtils on POSIX; Windows native
rounding is not locally verified. Focused seven-suite gate passed on macOS arm64,
Qt 6.11.2 (7/7, 4.06 s). Full gate evidence is recorded in the checklist.

## P1-05.4 — directory callback plans и приёмка Add (2026-10-01)

Baseline remains [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reviewed source bodies: `CPP/7zip/UI/Agent/AgentOut.cpp::EnumerateArchiveItems`,
`CAgent::DoOperation`, `CPP/7zip/UI/Common/UpdatePair.cpp::GetUpdatePairInfoList`,
`UpdateProduce.cpp::UpdateProduce`, `EnumDirItems.cpp::EnumerateItems`, and
`CPP/Common/Wildcard.cpp::CItem::CheckPath`. Their previously copied bodies and
module boundaries are retained; this iteration changes only the local
`qtarchivesourceupdate.cpp` and `ArchiveBackend` process adapters.

The callback may omit an explicit directory record while retaining a child.
The ordinary Delete recognizer now declines that shape, so it cannot silently
route Synchronize to recursive CLI deletion. The source-plan recognizer carries
removed directory records and retained original paths separately. The backend
deletes omitted files first, then omitted directories deepest first with exact
retained-descendant exclusions. The official CLI has no item-index deletion
interface; these stages adapt its path matching to the authoritative callback
plan without extracting/recompressing retained data. All stages use the existing
transaction copy, password, progress, cancellation and publish boundary.

Regressions exercise the source Agent's Full-path Synchronize on ZIP/7z with
Unicode/space directories, an older retained disk child and omitted directory
records. Direct backend cases retain an explicit subtree while removing its
unwanted nested directory, check byte contents, reject unsafe exclusions and
cancel during the second deletion stage with exact-original rollback.
No visible controls or layout change in this iteration. P1-05.1–.3 retain their
historical evidence; advanced options/formats, nested update, links/streams and
absolute extraction remain owned by P1-07/P2-01/P1-09/P2-05/P2-06/P2-07.
Duplicate/case-folded archive names and noncanonical unsafe paths remain rejected
by the integration guard; their broader source closure is reviewed in P3-05.
Windows/Linux runtime and rendered Windows appearance remain unverified.

Validation on macOS arm64/Qt 6.11.2, 2026-10-01: affected targets built;
unsandboxed focused `source_update_backend`, `ui_operations`, `ui_update_modes`,
`command_policy_runtime`, `filesystem_selection_runtime`, `binding_lifecycle`,
`panel_runtime` passed 7/7 (16.00 s); full unsandboxed `./scripts/check.sh`
passed formatting/build and 146/146 CTest (61.96 s). Fresh Add JSON reports
760×720/640×480 retain fully visible four-mode and three-path combos and footer.
Initial validation exposed slow startup sorting of build outputs and of the
shared temporary parent after a fixture's saved panel path disappeared; those
runs were stopped after a timeout. CMake now isolates current and temporary
directories per MainWindow suite. This is a test integration adaptation;
application navigation and sorting remain unchanged. Parent P1-05 is accepted
for the documented top-level ZIP/7z scope; the listed platform checks remain.

## P1-05.3 — защищённый Add в существующий архив (2026-10-01)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original paths and symbols: `CPP/7zip/UI/FileManager/PanelCopy.cpp`
`CPanel::CopyFrom`, `CPP/7zip/UI/Agent/AgentOut.cpp`
`CAgent::DoOperation`, `CPP/7zip/UI/Agent/UpdateCallbackAgent.cpp`
`CUpdateCallbackAgent::CryptoGetTextPassword2`, and
`CPP/7zip/UI/GUI/UpdateGUI.cpp` `FindUpdateMode`.
The source panel and Agent still produce the Add action plan and forward the
opened archive password through the original update callback. No functional
source body is replaced in this iteration.

The local `QtArchiveFileResource` callback adapter marks a plan protected
only when its callback supplies a password. `ArchiveBackend` keeps its
transactional copy and rejects an unmarked password-bearing plan. For 7z it
lets the existing archive's encryption and header mode carry through the
`7zz` update; explicitly passing `-p` would trigger a second password prompt
after opening the protected header, while `-mhe=off` would change the header
mode. For ZIP AES, `-p` is retained so added items are encrypted; the engine
retains the existing AES method. These switches and process details have no
portable upstream counterpart because the project uses the official CLI
engine as its update transport.

Qt command availability permits Add on a writable protected top-level
archive, and the dialog reuses its opened password without requesting it
again. Direct source tests verify encrypted old and Unicode new file bytes,
ZIP AES, 7z encrypted headers, wrong-password failure and cancellation
rollback. The UI test verifies one password prompt and successful protected
Add. The existing-target dialog still does not offer a separate password or
encryption-method control; it preserves the opened archive's protection.
Directory filtering with a retained descendant and remaining Add branches
are P1-05.4. Windows/Linux runtime and rendered Windows appearance remain
unverified. On macOS arm64/Qt 6.11.2, affected targets built, focused
`source_update_backend`, `ui_update_modes`, `ui_operations`, `app_state` and
`command_policy_runtime` passed after updating the policy expectation, and
the full unsandboxed `./scripts/check.sh` passed on 2026-10-01.

## P1-05.2 — режимы путей нового и существующего архива (2026-10-01)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original paths and symbols: `CPP/Common/Wildcard.cpp::CCensor::{AddItem,
AddPathsToCensor}`, `CPP/7zip/UI/Common/Update.cpp::UpdateArchive`,
`EnumDirItems.cpp::EnumerateItems`, `CPP/7zip/UI/Agent/AgentOut.cpp::
CAgent::DoOperation`, `ArchiveFolderOut.cpp::CAgentFolder::CommonUpdateOperation`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::OnInit/OnOK` and
`CompressDialog.rc::IDC_COMPRESS_PATH_MODE`.

The unchanged upstream `CCensor::AddPathsToCensor` and already adapted source
`EnumerateItems` decide logical names: Relative drops the selected item's
parent, Full retains the path without its volume/root, and Absolute retains
the absolute name. A narrow GUI-only branch in `CAgent::DoOperation` now calls
that original enumeration body; the existing `EnumerateItems2` route remains
for other callers. `CAgentFolder::UpdateFromPaths` supplies the absolute
selection and scoped path mode through local `IArchiveFolderInternal`, while
the original pair/action/callback operation remains the authority for update
decisions. Qt's `CreateArchiveDialog` adds the source path-mode choices and
passes the selection through `CPanel::CopyFrom`.

`ArchiveBackend` is platform integration: for a new archive it stages selected
parents separately for Relative, the filesystem root for Full, and absolute
names via `7zz -spf` for Absolute. The temporary archive is published only
after every stage succeeds. The existing-archive callback adapter stages
absolute item streams under safe temporary aliases, then asks the backend to
rename those names to their original absolute names after delete/add. The
backend validates both aliases and requested names, uses `-spf` for absolute
delete/rename, and keeps all mutations in the temporary archive. These
process and validation steps have no reusable upstream implementation because
the project bridges the original in-process callback to `7zz`.

Port scope: ZIP and 7z, new and existing archives, files and simple directory
trees including empty directories. `FindSourceUpdatePlan` still declines an
omitted explicit directory that contains an otherwise kept child; the CLI
`7zz d` command removes that child too. Protected Add is covered by
P1-05.3. Absolute archive entries are listed but the current safe extraction
path rejects them; extraction policy belongs to
P2-07. Full path root/UNC behavior needs Windows runtime validation. These
directory branches belong to P1-05.4 before parent acceptance.
`clang-format` affects only the edited portions of mirrored C++ files; source
notices remain. The baseline inventory now lists the two `UpdateGUI` files
introduced in P1-05.1. On macOS arm64/Qt 6.11.2 the full unsandboxed gate
passed formatting, build and 145/146 CTest cases; only `upstream_inventory`
failed because the prior commit omitted those two entries. After this JSON
correction, focused `upstream_inventory` and `upstream_baseline` passed.
Focused `source_update_backend`, `ui_update_modes` and `compression` also
passed. Windows/Linux runtime and rendered Windows appearance are unverified.

## P1-05.1 — существующий архив: update modes и compression (2026-10-01)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original paths and symbols: `CPP/7zip/UI/GUI/UpdateGUI.cpp::g_UpdateMode_Pairs`/
`FindUpdateMode` and `UpdateGUI`, `CompressDialog.h::NUpdateMode`/
`CInfo`, `CompressDialog.cpp::k_UpdateMode_Vals`/`OnInit`/`OnOK`,
`CompressDialog.rc::IDC_COMPRESS_UPDATE_MODE`,
`CPP/7zip/UI/Agent/AgentOut.cpp::CAgent::DoOperation` and
`ArchiveFolderOut.cpp::CAgentFolder::CommonUpdateOperation`,
`CPP/7zip/UI/Common/UpdateAction.cpp`, `UpdatePair.cpp`, `UpdateProduce.cpp`.
The exact Common action sets and produce algorithm were already present; the
mirrored Agent operation retains the source pair calculation and callback.

`src/CPP/7zip/UI/GUI/UpdateGUI.cpp` copies the four original mode/action-set
pairs and lookup loop, omitting the unrelated Win32 update dialog and command
line setup. `CompressDialog.h` now carries the original enum and info defaults.
Qt `CreateArchiveDialog` shows the original four modes and existing compression
controls for an existing target; `CPanel::CopyFrom` passes the selected source
action set through local `IArchiveFolderInternal::UpdateFrom` to a narrow
`CAgentFolder` adapter, which calls original `SetFiles` and
`CommonUpdateOperation`. The interface keeps the portable panel library free
of a link dependency on the concrete Agent. `QtArchiveFileResource` carries the
selected options through this source callback route. The resource's local
callback-plan adapter reads source keep/new/remove decisions, stages the native
streams and timestamps, and asks `ArchiveBackend` to update a separate copy
using `7zz` delete/add stages. The backend owns validation, process switches,
transaction failure/cancel cleanup and compression option checks; Qt widgets
never build engine commands. The create/archive compression switch builder was
extracted without changing its validation. The selected archive resource is
kept alive through the synchronous source worker and refresh.

This is a partial `UpdateGUI.cpp`/`CompressDialog` port: the verified scope is
existing, top-level ZIP/7z files with relative names. Path modes, directory
edge cases and encrypted Add need P1-05.2; the larger Win32 dialog body and
format options unrelated to this workflow are omitted. Source callback plans
unsupported by the file-only `7zz` adapter still fail explicitly. The dialog
and plan backend reject protected Add until encryption preservation is
implemented; the pre-existing source F5 protected Add route is unchanged. Required
`clang-format` affects presentation of borrowed snippets only. Source notices
and LGPL reference remain in the mirrored files. macOS arm64 byte-level tests
cover four modes on ZIP and 7z, Unicode/staged replacement, rollback,
compression method and UI route; Windows/Linux and rendered Windows appearance
are unverified.
The older controlled-backend `ui_operations` fixture now checks the Add
dialog's Cancel path and mode availability; its separate drag/drop test still
checks `addSources`. The dialog's real source route is covered in
`ui_update_modes`.
Focused `source_update_backend`, `compression`, `ui_update_modes`, `app_state`
and `panel_folder` passed. The full unsandboxed `./scripts/check.sh` passed
146/146 CTest on macOS arm64/Qt 6.11.2, 2026-10-01.

## P1-03.1 — filesystem CreateFolder/CreateFile/inline Rename (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CThreadFolderOperations`,
`IsCorrectFsName`, `CPanel::CorrectFsPath`, `CreateFolder`, `CreateFile`,
`OnEndLabelEdit`, `RenameFile`; `BrowseDialog.cpp::CorrectFsPath`; already
accepted `FSFolder.cpp::CFSFolder::CreateFolder/CreateFile/Rename` and
`ComboDialog.cpp`.

The mirrored `PanelOperations.cpp` copies source operation enum, worker dispatch,
`CUpdateCallback100Imp` lifecycle, name validation and source path correction.
The `CREATE_FILE` worker branch is a local adaptation: upstream calls
`IFolderOperations::CreateFile` directly, while this Qt port keeps filesystem
mutations on the source progress thread. Source `OnEndLabelEdit` label control,
real-index/selection/reload sequence is adapted to `QTableWidget`: F2 begins
inline editing in all four view modes, Qt commits on Enter and cancels on Escape,
and `MainWindow`
reloads and selects the resulting path. `CreateFolder`/`CreateFile` use the
ported `ComboDialog` presentation. The path name is passed to source
`IFolderOperations`; creation of nested directories and existing-parent file
paths comes from the original FSFolder body. The Windows `CorrectFsPath` loop
is copied from BrowseDialog, and its POSIX passthrough branch is retained.
Qt replaces Win32 list control, HWND, messages and presentation state;
`CheckBeforeUpdate`, source symbols and module boundaries remain. Required
`clang-format` changed formatting, not functional bodies.

This is a partial `PanelOperations.cpp` port: `DeleteItems`, recycle-bin shell
path and confirmation, archive changes and `ChangeComment` remain P1-03.2/
P1-04. No source label-edit control or Windows shell recycle implementation
is claimed on macOS/Linux. A directly selected symlink remains rejected by
the existing Qt command policy; Windows and Linux runtime and rendered
Windows inline appearance remain unverified.

## P1-03.2 — filesystem Delete and native Trash (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CPanel::DeleteItems`,
`CThreadFolderOperations::ProcessVirt`/`DoOperation`, the existing
`FSFolder.cpp::CFSFolder::Delete`, and
`MyLoadMenu.cpp::WndProc` (`IDM_DELETE`, `!IsKeyDown(VK_SHIFT)`).
The mirrored `DeleteItems` retains the source timer guard, operated-index
collection, selected-state snapshot, single file/folder/multiple prompt choice,
`CDisableNotify`, `FOLDER_TYPE_DELETE` dispatch, title refresh and partial
result semantics. `MyLoadMenu` still supplies the bool for recycle versus
permanent deletion; Qt's list key adapter now sends Shift+Delete through that
same source command switch.

The original `SHFileOperationW` Windows shell call is replaced by the narrow
`IQtTrashService` boundary. Its production implementation delegates each
selected path to `QFile::moveToTrash`, which uses the platform desktop's
Trash/Recycle Bin. Qt does not present the source shell confirmation, so the
source type/count prompt is used before both routes. The source permanent
worker and `CUpdateCallback100Imp` run unchanged at the operation boundary;
`CFSFolder::CheckBreak` is connected to its progress `CheckStop` for recursive
cancellation. Qt reloads `FSFolder` and restores source selection after every
accepted result, including partial failure/cancel; this replaces Win32 list
refresh. The independent application delete worker was removed. Its test-only
driver moved under `tests/` to retain deterministic FSFolder callback,
recursive-cancel and partial-failure regressions. Directly selected symlinks
remain rejected; nested links are unlinked without following their targets.
Required `clang-format` formatting is separate from functional adaptation.

Only filesystem deletion is included. Archive deletion remains P1-04.
macOS test UI uses an injected Trash adapter to avoid modifying the user's
actual Trash; native service behavior and Windows/Linux runtime remain
unverified locally. No Win32 long-path shell limitation is claimed for Qt.
Unix permission-denied CreateFile/Rename/Delete and occupied inline Rename
are covered by focused regressions; the full macOS arm64 gate passed 137/137
CTest cases on 2026-09-30.

## P0-03 — integrated command policy/dispatch (accepted, 2026-09-29)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
https://github.com/ip7z/7zip. `MyLoadMenu.cpp/.h::CFileMenu::Load` and its fields,
constructor, g_Zvc_IDs/Strings are copied under original paths. The complete
functional loop retains context/program distinction, Diff/Benchmark filtering,
Split/Combine file eligibility, read-only and hash-folder switches, link count,
alt-stream support, compact disabled/separator omission, insert counters,
version-control file attributes/size checks and trailing-separator removal.
`IQtFileMenuServices` replaces source/destination CMenu, popup copying, Registry
settings and screen-size check. Item IDs/UString remain upstream; native portable
CFileInfo remains the file inspector. Source language/resource menu construction
remains mandatory P0-09/P1-14; the command switches are connected below.

`PanelMenu.cpp::CPanel::CreateFileMenu` copies operated-index collection,
directory classification, CFileMenu population, alt-stream capability query and
Load. Win32 shell/7-Zip COM menus remain P0-09/P2-09, disabled until implemented;
current Qt command items use the source-owned policy. `Panel.h` IO-FS/pure-drives
predicates and IsFolder_with_FsItems retain original FS/network branches (the
latter moves to Panel.cpp to contain its FileName/StringConvert dependency).
FindDir_InOperatedList retains the index loop but queries existing source
IsItem_Folder: full RefreshListCtrl directory cache loading remains P0-05.

`Agent.cpp::CAgentFolder::GetFolderProperty` ReadOnly/IsHash branches and
`Agent.h::Is_Attrib_ReadOnly`, `IsThere_ReadOnlyArc` are ported for this consumer.
Original long-path/attribute/read-only arc ordering and typed VARIANT_BOOL remain.
The source codec catalog FormatIndex/UpdateEnabled predicate is supplied by
ArchiveUpdateEnabled querying the metadata owner's actual IOutArchive capability;
the external engine transport has no in-process codec index. Complete catalog/
parser capabilities remain P2-01; this adaptation does not infer update support
from filename or rewrite compression. Portable CFileInfo.GetWinAttrib initializes
original _attrib at the current Init/Open bridge. Win32 INVALID_FILE_ATTRIBUTES
is DWORD(-1) on platforms without that header constant. Actual read-only file,
long-path change, read-only arc, absent/present writer and hash-property regression
fixtures are isolated; no engine command is invoked in capability tests.

Policy focused test passed 1/1 (0.28 s); agent_capability/agent_folder/
archive_opening/file_menu_policy passed 4/4 (1.31 s), macOS arm64 2026-09-29.
ExecuteFileCommand/OnMenuCommand bodies are now copied with original switch IDs,
method names/arguments, bookmark/time ranges and status refresh ordering.
IQtFileManagerCommandServices and its menu extension replace the global CApp's
platform/presentation entry points. `FM.cpp` WM_COMMAND and toolbar ExecuteCommand
retain original toolbar IDs and two-panel timer guards. `CApp::ExecuteCommand`
owns the entry, rejects a closed lifecycle, and protects synchronous native
callers with the P0-02 operation CallScope. Exit is deliberately outside that
scope so the source WM_CLOSE lifecycle can run. `QtFileManagerCommands` attaches
all existing File/Edit/View/Favorites actions, toolbar, context-menu shared
actions, panel activation and bookmark/selection shortcuts. Qt extensions use
IDs 1000–1006, below original plugin start 1100. Action enabled/check state is a
projection; execution recomputes the native policy even for a stale enabled
QAction. The former refreshActions matrix and independent callbacks are removed.
QtFileMenuProjection owns no widget or HMENU and uses CPanel::CreateFileMenu;
transport restrictions (busy/scan, nested/encrypted updates, currently supported
engine updates and current selected-only Copy/Move consumers) are local to the
application adapter. Full Copy/Move smart/all-items semantics remain P1-03.

`PanelMenu.cpp::CheckBeforeUpdate` and `Panel.h::DoesItSupportOperations` retain
the original interface test, reverse current/parent read-only chain and localized
error construction. Current Rename/Move/Delete/Create consumers call that source
preflight. Complete mutation workflow bodies remain P1-01/P1-03/P1-04; current
copy/edit/extract/hash transports are not declared fully source-ported here.
Unsupported Diff/version/link/streams/plugins/help/benchmark/second-panel/time
consumers have no available registry entry and cannot dispatch placeholders.

The original read-only policy exposed a missing provider prerequisite:
`CFSFolder` had no IFolderOperations. The native interface now copies source
GetAbsPath/CreateFolder/CreateFile/Rename/Delete functional bodies in FSFolder.cpp
and uses existing portable upstream CreateDir/CreateComplexDir/COutFile/MyMoveFile.
QString entry points delegate to those bodies; Qt progress callbacks adapt to
IProgress, preserving original SetTotal/SetCompleted order and cancellation.
Const GetAbsPath serves the existing const presentation bridge. POSIX EEXIST
replaces Win32 ERROR_ALREADY_EXISTS in the directory fallback branch. Rename
retains the existing no-replacement check, and direct selected symlinks remain
unavailable; recursive deletion uses the already adapted no-follow FileDir
service. Null progress/out-of-range indices are rejected before dereference.
CopyFrom/CopyFromFile E_NOTIMPL bodies are also upstream; full CopyTo/SetProperty
remain explicitly unavailable native branches pending P1-03/P1-04. This partial
interface is not completion of those mandatory workflows or all of FSFolder.

Toolbar prerequisites for native OnMenuCommand retain original CApp fields,
ReadToolbar/SaveToolbar/Switch* methods, bit mask/default high-bit rule and
SaveToolbarChanges Save→Reload→Move order. ViewSettings saves the original
Toolbars key via explicit INI path; invalid/out-of-range values use original
default. Qt legacy toolbar/* keys migrate only when Toolbars is absent. Qt owns
QToolBar construction, grouping/metrics/icons and layout activation; checkboxes
mirror source flags with blocked signals and route through CApp. Full source
toolbar/resource/icon presentation remains P0-09. Flat-view dispatch also uses
the command entry; full source model/timer refresh remains P0-05.

Additional copied bodies: App.cpp::RefreshTitle/RefreshTitlePanel retain the
focused-panel test, empty-prefix `7-Zip`, PrevTitle cache and update order; QWidget
setWindowTitle replaces SetText. Create clears the cache and command service on
reinitialization. Partial CPP/Windows/Control/Dialog.cpp/.h::IsDialogSizeOK retains
original integer division/multiplication and both work-area comparisons. Qt
system font metrics replace packed GetDialogBaseUnits and QScreen availableGeometry
replaces GetWorkAreaRect; other Dialog.cpp bodies are not ported by this change.
Qt policy refresh uses one snapshot; execution always recomputes it. Recursive
filesystem cancellation explicitly preserves E_ABORT across the portable service.
Delete's Qt error projection retains the failed native item path after partial
deletion, instead of replacing it with a generic message. SelectByType availability
uses the source focused item, including empty selection, not operated indices.
The fake backend now supplies typed fixture format/openFolder rather than
accidentally falling through to a real `/missing/7zz` reopen; explicit metadata
keeps priority. This test adaptation does not weaken native writer capability.
The same fake overrides source extractSourceEntry with queued completion/cancel;
native OpenItemInArchive no longer falls through to a real engine in UI fixtures.
Navigation tests explicitly select one item after Select All before single-item
Enter, because copied OpenSelectedItems operates the full selection. Both Up
presentations share original parent dispatch, including leaving archive root.
Current-build main-window JSON at 980×620, 720×480 and 640×400 shows toolbar,
address, list and status bar fully visible. Geometry establishes Qt usability,
not rendered Windows parity. Latest focused ui_navigation passed 1/1 (1.06 s);
screen/menu/native FS cancellation and nested/native navigation checks passed
their focused cases. The current complete gate remains pending.
First complete gate compiled successfully but failed 4/129 cases (125 passed,
247.68 s): missing deletion-path projection; unchanged priority child reply timeout;
viewer fixture typed format/source-stream/lifetime omissions; parent fixture and
SelectByType focused policy. After fixes the five direct filesystem/viewer/key/
policy consumers passed 5/5 (3.35 s), and unchanged priority retest passed 1/1
(3.78 s). Viewer success/failure/cancel remains covered; final file lifetime is
checked on native CopyTo's output, independently of disposed transport scratch.
These focused checks preceded the final full gate recorded below.

Final acceptance: ./scripts/check.sh formatting/build PASS and 129/129 CTest
PASS (87.96 s), macOS arm64/macOS 27/Qt 6.11.2, 2026-09-29. Complete current
command component and its existing consumers are accepted; all omitted feature
workflow/model/resource/shell scopes above remain mandatory in their owners.
Windows/Linux runtime and rendered Windows parity remain unverified.

Earlier intermediate checks: native_fs_operations/fs_folder passed; ui_state
passed after synchronizing a double-clicked unselected Qt row before the native
OpenSelectedItems loop. ui_state/ui_layout/app_lifecycle passed 3/3 (2.35 s).
Runtime policy fixture compares canonical native filesystem paths (macOS /var
aliases /private/var); its initial lexical comparison was corrected without
changing production path logic. These are implementation checks, not P0-03
acceptance; current full gate and remaining state matrix are still pending.
The same application policy is checked again before side effects so a stale
shortcut cannot bypass a disabled command. Plugin Invoke/release remain ordered
services, with actual plugin commands unavailable until P2-09. Qt service calls
replace HWND activation/close, Options/About/help/temp dialogs; future unsupported
features remain mandatory in their owners. All timestamp/menu/language construction
omissions are still explicit; copied switches do not imply implemented consumers.

P0-04 navigation prerequisite inside the command integration copies PanelItems.cpp
OpenSelectedItems/OpenFocusedItemAsInternal, PanelItemOpen.cpp OpenItem/
OpenFolderExternal and PanelFolderChange.cpp OpenFolder. These preserve native
operated/focused indices, 20-item limit, parent insertion, first internal-folder
break, multi-item external flow, archive/known-extension/virus/error/cancel branches,
BindToFolder → SetNewFolder → LoadFullPath → RefreshListCtrl. QtPanelList adapts
list focus/selection/repaint/EnsureVisible; source MessageBox_Error_LangID is copied
to Panel.cpp. Existing no-handle IQtExternalFileServices supplies OS association;
full known process/editor owners remain P1-08. No separate helper iteration or
P0-04 acceptance: production consumers and its full navigation gate remain open.

Production command registry/dispatch, state matrix/keyboard/
context integration, UX and full gate remain pending. No helper commit or package
acceptance. Source notices retained; formatting applied separately. Windows/Linux
and rendered Windows parity remain unverified.

## P0-02 — accepted owners and state

Parent P0-02 is accepted. Its former recursive helper chain is historical evidence,
not remaining work. The [archived source records](history/p0-02-source-port.md)
preserve all original paths/symbols, copied/adapted/omitted portions, project baselines
and checks verbatim. Every borrowed module uses
[ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Adapted application files retain upstream paths under src; unchanged dependencies
remain under third_party/7zip with original notices/license. Formatting is separate
from functional changes. This consolidation adds no source or runtime acceptance.

## P0-02.1 — application history/favorites owner

Copied FileManager/AppState.h CAppState fields and Read/Save order; the explicit
settings-path argument delegates to QSettings adapters. MainWindow reads the owner
before menus/panels; confirmed history/favorites edits retain immediate persistence.
Historical acceptance: project baseline `36fad59`, commit `eedf926`, macOS
2026-09-28, focused 5/5 and full formatting/build/54 CTest.

## P0-02.2 — application/panel lifecycle owner

Copied App.cpp/.h Create/CreateOnePanel/Save/ReleaseApp and Panel.cpp/.h
Create/OnCreate/ReleasePanel, native focus/index/tab callbacks, per-panel settings
and disable-all-before-any-release ordering. ViewSettings/RegistryUtils retain
source bit packing and keys; QSettings replaces Registry and source keys take
precedence over migrated legacy Qt keys. QtAppPresentation supplies widgets and
guarded targets; the former global app/launcher references become per-instance
owners. Save stores an open archive's outer parent. Historical acceptance:
macOS arm64 2026-09-29, full formatting/build/121 CTest (85.30 s), startup
success/failure/cancel, independent state and 16 normal/compact archive JSON reports.

## P0-02.3 — operation-state and window-close integration

Copied FM.cpp WM_CLOSE soft-exit/drop-revoke/Save/ReleaseApp/window-save/hard-exit
order through IQtFileManagerWindowServices. Qt additionally cancels/joins backend,
scanner, file/volume and per-panel external workers while inputs still live.
QtAppOperationState holds source CPanel processing guards across asynchronous
completion: Scan permits navigation, Open/Operation block consumers, completion
restores guards after rows/selection agree, and closing prevents late callbacks.
QSettings stores source binary Position/Panels using logical Qt coordinates in
place of WINDOWPLACEMENT. Historical acceptance: macOS arm64 2026-09-29,
focused 8/8 and full formatting/build/122 CTest (84.83 s).

### Integrated prerequisites and remaining boundaries

| Former P0-02 scope | Accepted result / authoritative audit section |
| --- | --- |
| Native IFolder/COM/property types, CAgent/CFSFolder ownership, source item indices, path/property/selection helpers and selected consumers | Archived module records; P0-04.1/.2 navigation, P0-05 model, P0-06 sort/columns/time, P0-07 selection |
| CTempFileInfo/CFolderLink, FileDir cleanup, CLibrary transfer, root/nested parent stack and changed-file update | Archived dependencies; P0-04.1/.2 integrated source owner and temporary lifetime |
| FormatUtils/resources, CProgressSync, native threading/errors, progress/statistics/icon/priority/taskbar/startup wait and update callbacks | Archived dependencies; P0-10.1/.2 integrated modal worker and all current consumers |
| Timer/notify/status guards, all-panel release and process/operation quiescence | P0-02.2/.3 and P0-06.4 |

The archive records narrow native/Qt adaptations, including POSIX library transfer
copied from the Windows class, Compare_FiTime, the 100 ms POSIX sleep and 500 ms
startup wait, strict resource extraction, native progress clamping, real priority
failure rollback, Cocoa badge ownership and Linux DBus transport/desktop identity.
They do not imply complete upstream modules. Full editor/known-process workflows
remain P1-08; language loading P1-14; raw/ADS/reparse capabilities P1-10/P2-05;
shell integration P2-09; packaging and remaining engine/platform branches P3-05.
Memory/zone and other omitted branches keep the owners specified by their module
and later integrated records. None becomes N/A through this cleanup.

Historical macOS checks establish local behavior, source contracts and geometry;
Windows/Linux runtime, rendered Windows/Dock fidelity and actual Linux launcher
rendering remain unverified. The accepted neighboring packages retain their own
evidence; parent counters do not change.

## P0-05.1 — filesystem read model and normal/flat consumers (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([FSFolder.cpp/h](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager),
same revision for `TextPairs.cpp/h`). `TextPairs.cpp/h` were copied to their
mirrored paths; only `StdAfx.h`/relative Common includes became project include
paths. `CPairsStorage` parse, sort, lookup, replacement and save bodies remain
the original implementation. Required clang-format is mechanical and separate
from the include adaptation. `CFSFolder::LoadComments`, lazy `kpidComment`
lookup/cutoff, `ReadFileInfo`, `ReadChangeTime`, `CalcItemFullSize`,
`CFsFolderStat::Enumerate` and `Clone` start from the original bodies. Previously
accepted `LoadItems`/`LoadSubItems` and typed-property bodies remain the owner.

The Windows link and NT change-time calls are retained under `_WIN32`; POSIX
uses `lstat` for the same inode/link/status-change metadata and the portable
7-Zip `CFiTime` conversion. POSIX `CEnumerator::Next`/`Fill_FileInfo` adapt the
source Windows iterator shape. In flat recursion the source continues past an
unreadable child; the local cancellation callback alone propagates `E_ABORT`.
The outer root still reports its own binding failure to Qt. Directory-size
enumeration retains source count/size/progress flow and does not follow POSIX
symlinks. `mainwindow_filelist.cpp` now takes visible names/prefixes and sort
roles from source properties/indices; NFC normalization is limited to Qt text.
QFileInfo remains for Qt icons and pre-existing timestamp presentation; native
time formatting and complete column/sort ownership are P0-06.

The read-model consumer checks cover quoted UTF-8 `descript.ion` comments and
reload, hard links/inode/change time, Unicode nested directory statistics with
cancellation, and hidden/empty/unreadable sibling enumeration. Normal/flat UI
and source properties consumers are exercised by `flat_view`,
`panel_selected_state` and `properties_runtime`. The incomplete source
`IFolderCompare`/fast item-name sort path and periodic model refresh belong to
P0-06, system icons to P0-09, comment writes/`SetProperty` to P1-03, raw
reparse/ADS interfaces to P2-05/P2-06. P0-05.2 still owns large asynchronous
scan, stale batches and full parent acceptance. Windows/Linux runtime and
rendered Windows comparison remain unverified.
Focused `fs_folder`, `flat_view`, `panel_selected_state`, `properties_runtime`
pass 4/4, 6.13 s; the full `./scripts/check.sh` formatting/build/all CTest gate
passes 130/130, 88.79 s on macOS arm64, 2026-09-29. Current-build 980×620 and
640×480 flat scan/presentation JSON retains visible toolbar, address, list and
status; these reports do not measure Windows rendering. P0-05 parent acceptance
is not claimed by this read-model component.

## P0-05.2 — large flat filesystem presentation and P0-05 acceptance (2026-09-29)

Baseline remains ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([FSFolder.cpp source](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/FSFolder.cpp)).
No borrowed functional body changes in this iteration: the accepted
`CFSFolder::LoadSubItems`/`LoadItems`, `GetProperty`/`GetRelPath` and native
item indices remain the sole flat enumeration model. `FilesystemFolderLoader`
still adapts worker scheduling and atomic cancellation; `FilesystemScanner`
transports its completed source folder to Qt and rejects stale generations.
The GUI batch cap changed from 128 to 32 because presentation can resolve
native icons per row after batch preparation. This bounds one delivery even
when the preparation timer has not expired. It does not change source order,
filters, metadata or the worker's count-progress cadence.

The deterministic nested Unicode fixture contains 2048 files and two parent
folders; it verifies 2050 ordered source indices, at most 32 rows per GUI
batch, an event-loop turn before completion and no failure. Existing scanner
tests cover restart during native enumeration, restart/cancel during row
publication, destruction and invalid roots. Existing source-model tests cover
hidden/empty entries, unreadable siblings, Unicode names, symlink-cycle
suppression, metadata and cancellation. The 1500-file `flat_view` scenario
checks normal/compact status, partial listing, cancel, mode switch and
navigation without stale rows. Current-build compact report at 640×480 keeps
the toolbar, address, table and status fully visible; its upstream reference
is the File Manager list/flat command structure, not a rendered Windows image.

The P0-05 source filesystem model, narrow async adapter and current normal/flat
consumers are accepted within the above scope. Source sort/column/time and
auto-refresh remain P0-06, native icons P0-09, write-side `SetProperty`/comment
editing P1-03, raw reparse/ADS P2-05/P2-06 and larger-operation responsiveness
P2-10. Windows/Linux runtime and rendered Windows appearance remain unverified.
Focused `fs_folder`, `filesystem_scanner` and `flat_view` pass 3/3 (2.51 s);
`./scripts/check.sh` passes formatting, app build and 130/130 CTest (85.77 s)
on macOS arm64, 2026-09-29. The updated current-build 640×480 JSON has a
fully visible toolbar, address, table and status. No pixel parity is claimed.

## P0-06.1 — typed source sort and current list consumers (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([PanelSort.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelSort.cpp),
same revision for `PanelItems.cpp` and `Panel.h`). Existing mirrored
`PanelSort.cpp` already contained the upstream natural filename pointer loop.
This iteration ports `CompareItems2`/`CompareItems` property iteration into
`CPanel::CompareItemIndices`: parent and folder priority, source `IFolderCompare`
dispatch, natural base-name/extension comparison, `CPropVariant` type/BSTR/value
comparison, name→prefix→source-index fallback. The Qt row comparator now passes
original source indices and a property ID; the previous independent Qt value/
text comparator was removed. `FileTable` holds only a non-owning panel pointer,
whose lifetime is managed by `CApp`; Qt still reverses property order for
descending and compensates folder grouping. Header/action changes synchronize
the native panel's `_sortID`/`_ascending` with the visible state. Root rows use
the same item type and source owner.

Necessary adaptations: upstream Win32 `LPARAM` list-item indices come from Qt
row roles; `wchar_t` names are converted at the existing Qt text boundary;
`SortPropIDForColumn` maps the current seven-column view. The original raw
property branch and `SetSortRawStatus` require the source column catalog and
remain P0-06.2. View column/time persistence and timer refresh remain
P0-06.2/.3. Windows `ListView_SortItems` and focus-scroll call are represented
by Qt sorting and the current shared selection model. There is no second
functional sort implementation for bound folders.

The old flat-prefix test expected natural digit order for `dir2`/`dir10`.
Upstream compares `kpidPrefix` as a BSTR, so the test now expects lexical
`dir10` before `dir2` after equal names. Focused source-sort/flat/selection/root
consumers and current-build 980×620/640×400 JSON validate the resulting Qt
behavior; Windows rendering/runtime and Linux runtime remain unverified.
Focused `panel_sort`, `flat_view`, `panel_selected_state` and `root_navigation`
pass 4/4 (5.64 s); `./scripts/check.sh` passes formatting, app build and
130/130 CTest (88.78 s) on macOS arm64, 2026-09-29. The current-build compact
sort report has fully visible toolbar, address, table and status. P0-06.2/.3
and parent acceptance are still mandatory.

## P0-06.2 — source column catalog, raw comparison and Qt header (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed original paths: `CPP/7zip/UI/FileManager/PanelItems.cpp` (`InitColumns`,
`SaveListViewInfo`, `GetColumnVisible/Width/Align`, `ShowColumnsContextMenu`),
`ViewSettings.h/.cpp` (`CColumnInfo`, `CListViewInfo::Save/Read`),
`PanelSort.cpp` (`CompareItems2` raw branch and `CompareFileNames_Le16`),
`PanelListNotify.cpp` (`GetItemText` property/raw branches), `Panel.h`, and
`CPP/7zip/UI/Agent/Agent.h/.cpp` (`CAgentFolder::GetNumRawProps`,
`GetRawPropInfo`, `GetRawProp`, `GetParent`). `PropertyName.cpp` and portable
`CPP/7zip/UI/Common/PropIDUtils.cpp` are existing reviewed dependencies.

The mirrored `PanelItems.cpp` now builds `CPropColumns` from each bound folder's
typed and raw property capabilities, merges the saved visibility/width/order,
forces Name visible and restores its property sort. The original
`SaveListViewInfo` column-order/hidden-column construction and binary
`CListViewInfo` v1 layout remain in their upstream owners. The former fixed
Qt `columns/2..4` flags are replaced by a per-FolderTypeID `QSettings` byte
record; hex-encoded UTF-8 type IDs prevent INI slash/Unicode key collisions.
Registry calls and Win32 `LVCOLUMN` reads are replaced only by a Qt header
snapshot. Qt `FileTable` holds the current source property/raw mapping, retains
hidden path/index transport cells, displays source-formatted property text and
sorts original item indices without parsing display strings. The dynamic header
menu lists all reported properties, keeps Name visible, restores shown columns
and resets a hidden sort column to Name. Four view modes share the same rows and
column state. The previous seven-position `SortPropIDForColumn` adapter was
removed. `GetColumnAlign` keeps the original property/type switch with Qt
alignment constants; copied modules use project clang-format separately from
functional comparison.

`PanelListNotify.cpp::GetItemText` retains the source size grouping, BSTR newline
replacement, `ConvertPropertyToShortString2` fallback, raw security/reparse
conversion and short-hex/long-data branches. The Name and flat-prefix Qt
presentation remains the accepted P0-05 adaptation. Time uses level 0 here;
five precision levels, UTC control and view-specific formatting belong to
P0-06.3. The source raw comparison's empty/data-type/reparse branch is copied;
the Qt row supplies its source index and raw flag. Malformed reparse bytes are
rejected before pointer comparison, a portability guard absent from the Win32
call site. `CAgentFolder` now implements the upstream raw interface and maps
folder rows to original archive indices; `qtarchiveproperties.cpp` forwards an
available raw provider across the `7zz` metadata adapter. The current external
`7zz` listing does not supply binary raw fields, so real archive raw values are
not claimed: the code path is checked with a deterministic provider fixture,
and complete archive raw-value transport remains mandatory P1-10. `_proxy2`
alternate streams remain P1-10/P2-05. There is one authoritative copy of each
borrowed functional body.

`panel_columns` checks Unicode/space filesystem values, raw catalog/text/sort,
all-column header control, independent filesystem/archive records,
width/order/sort restoration and eight current-build JSON reports (four modes
at 980×620 and 640×400); toolbar, address, list and status stay visible.
`view_settings_columns` checks exact binary bytes and malformed records;
`agent_folder`, `panel_sort` and `ui_layout` cover direct consumers. Focused
5/5 pass after the local alignment/test-cleanup edits; the app rebuild passes. The full
`./scripts/check.sh` before that edit passed formatting, app build and 132/132
CTest (93.36 s) on macOS arm64; the subsequent alignment-only change received
focused rebuild/tests. Windows/Linux runtime and rendered Windows visual parity
remain unverified. P0-06.3 time and P0-06.4 auto-refresh/parent acceptance are
still mandatory.

## P0-06.3 — source time precision and UTC View workflow (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed original paths/symbols: `CPP/7zip/UI/FileManager/Panel.h`
(`_timestampLevel`, MIN default), `App.h` (`_timestampLevels`,
`GetTimestampLevel`, `SetTimestampLevel`), `PanelListNotify.cpp::GetItemText`,
`MyLoadMenu.cpp` View/Time popup and `WndProc` time dispatch;
`CPP/Windows/PropVariantConv.h/.cpp` and
`CPP/7zip/UI/Common/PropIDUtils.cpp::ConvertPropertyToShortString2` are the
previously vendored, unchanged formatting dependencies. Portable
`CPP/Windows/TimeUtils.cpp` supplies the current UTC sample. The shared
`FormatUtils.cpp` has no time-formatting body and was reviewed but not changed.

`CPanel` now retains the source MIN default and supplies its selected level to
the copied `GetItemText` property branch. `CApp` owns the same all-panel level
update and menu-level vector as upstream. The source View/Time popup loop
formats five examples (DAY/MIN/SEC/NTFS/NS) with `ConvertUtcFileTimeToString`,
records the available levels, and uses the original `IDM_VIEW_TIME` range and
`IDM_VIEW_TIME_UTC` dispatch. Qt `QMenu`/`QAction` replace Win32 menu handles;
the existing `QtFileManagerCommands` boundary redraws visible typed time cells
without replacing the source formatter or converting displayed text back into
timestamps. The column comparator continues to read raw `FILETIME` properties.
Qt `QSettings` additionally persists precision and UTC between launches;
upstream retains these across panels but only in memory. The external `7zz`
local listing text is converted to `FILETIME` at the existing metadata adapter;
its actual precision cannot exceed the listing's supplied digits. No time
logic was copied into `FormatUtils`, and no duplicate formatter was introduced.

`panel_columns` now checks five menu examples, day/second/UTC transitions,
reopening, raw time sort despite equal day strings, Unicode/space filesystem
and archive values, and current-build 980×620/640×400 JSON. The reports show
fully visible toolbar, address, view stack and table. Focused
`view_settings_columns|ui_layout|command_policy_runtime|panel_sort|panel_columns`
passes 5/5; `./scripts/check.sh` passes formatting, app build and the full
CTest gate (133 tests registered) on macOS arm64. Windows/Linux runtime and
rendered Windows menu parity remain unverified. Auto-refresh and full P0-06 acceptance
remain mandatory P0-06.4.

## P0-06.4 — source timer policy and portable filesystem refresh (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed `CPP/7zip/UI/FileManager/App.h` (`AutoRefresh_Mode`,
`Change_AutoRefresh_Mode`, `SetPanels_AutoRefresh_Mode`), `Panel.h`/`Panel.cpp`
(per-panel mode, one-second timer), `PanelItems.cpp`
(`RefreshListCtrl_SaveFocused`, `OnReload`, `OnTimer`) and `FSFolder.cpp`
(`CFSFolder::WasChanged`). The app/panel mode, source timer guards,
`IFolderWasChanged` query and saved `CSelectedState` flow are copied into their
existing modules. Qt owns timer delivery, a View-menu check action, persisted
mode, and the callback that reloads the current filesystem view through the
source panel and restores the visible top path/scroll position. Notification
processing remains disabled during active operations and window teardown.

The original Windows `CFSFolder::WasChanged` consumes a native change
notification; its POSIX branch always returns false. The narrow Qt adapter
polls metadata of the displayed folder and source-enumerated entries every
second and marks the source panel changed. It skips active/flat scans, so it
does not read a partially published list. A missing current directory clears
stale visible rows and records its absent signature; recreation triggers a
fresh bind. Archive folders do not implement `IFolderWasChanged` upstream, so
archive refresh remains explicit. This does not port Win32 notification handles
or claim archive auto-refresh. Large-tree polling cost remains part of P2-10.

`panel_auto_refresh` checks Unicode file creation/removal, current item,
selection/scroll, disabling and persisting the View option, manual refresh,
and disappearing/reappearing folders on macOS. Earlier P0-05 flat scan
cancel/stale-batch and P0-06.1–.3 sort/columns/time and four-view geometry
evidence remains applicable. Windows/Linux runtime and rendered Windows visual
parity remain unverified. Focused `panel_auto_refresh` and the full
`./scripts/check.sh` gate pass (134/134 CTest) on macOS arm64. The first full
run was denied DBus socket creation and process-priority calls by the sandbox;
the rerun with those system permissions passed. A later one-line action-state
fix and selection assertion passed the affected build/focused CTest again.

## P0-07.1 — source list settings and main-panel activation (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed `CPP/7zip/UI/FileManager/SettingsPage2.rc` control order and
`SettingsPage.cpp::{OnInit,OnButtonClicked,OnApply}` for all seven list flags;
`RegistryUtils.cpp::CFmSettings::{Load,Save}` and `App.cpp::SetListSettings`
were already source-derived. Reviewed `PanelItems.cpp::InitList` (`showDots`
and `kParentIndex`), `PanelListNotify.cpp::OnNotifyList` (one-click activation,
track-select and item changes), `PanelSort.cpp` parent-index ordering, and
`PanelMenu.cpp::CreateFileMenu` (`ShowSystemMenu` only for filesystem).

The Qt `CSettingsPage` now follows the original seven-control order and
retains its source init/changed/apply policy. ShowDots, FullRow and
ShowSystemMenu are read and written through the existing `CFmSettings` owner;
cancel and an unchanged page do not write. AlternativeSelection is visible
but disabled, retains any pre-existing stored value, and remains mandatory
P0-07.3 because upstream uses an independent marked-item vector and
Ctrl/Shift range policy. Large pages, extraction RAM, six-page Options,
Help and language controls remain P1-12.

The source `showDots`/`kParentIndex` rule is presented in the bound Qt table,
including flat/normal filesystem and archive views; the virtual root omits it.
The parent item keeps its source sentinel through sort and navigation. Qt
preserves the source `FullRow` visual distinction with a delegate that paints
only the Name cell's selection when the option is off, without changing the
row-level selection model or source item indices. The `ShowSystemMenu` flag
adds an OS file-manager action only in the filesystem context; native Windows
shell verbs have no current cross-platform Qt equivalent and full context
menu parity remains P0-09. System icons and grid keep their prior consumers.

`qtfilelisthover.cpp` is new presentation glue for the source
`LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT` policy on both table and icon
views. It uses the already established 400 ms Qt dwell adapter, validates
the persistent model index and cursor position after scroll/removal, and
cancels on modifiers, drag, press and leave. It never opens on hover. Qt
single-click activation remains guarded against Ctrl/Shift/Meta gestures.
The fixed interval and platform file-manager action are explicit deviations.

`fm_settings` and `list_settings_runtime` exercise seven-control order,
disabled alternative mode, persistence/cancel, ShowDots on disk/archive/root,
normal/reverse sort, system-menu visibility, full-row paint difference,
single-click hover guards and selected-row retention. Current-build settings
340×300/460×300 and list 640×400/980×620 JSON show visible controls and
unclipped table/address. A targeted Qt screenshot confirmed the selection
paint difference; rendered Windows parity and Windows/Linux runtime remain
unverified. Source selection/range/alternative-mode acceptance remains P0-07.2/.3.
Focused direct-consumer CTest passes 8/8; `./scripts/check.sh` passes the full
135/135 CTest gate on macOS arm64 in the unsandboxed environment required by
AGENTS.md. A later localized tooltip and drag-guard test correction passed the
affected build and focused CTest. The first optional screenshot fixture wrote
inside its temporary directory and was corrected to use the build report
directory; it is not part of the product path.

## P0-07.2 — source selection commands (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([exact source](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelSelect.cpp)).
`src/CPP/7zip/UI/FileManager/PanelSelect.cpp` now holds the source bodies and
branch order of `CPanel::UpdateSelection`, `SelectSpec`, `SelectByType`,
`SelectAll` and `InvertSelection`, using the existing upstream
`CRecordVector<bool> _selectedStatusVector`, `GetRealItemIndex`,
`GetItemName`, `IsItem_Folder` and source `Wildcard.cpp` matcher.
The independent `CPanelSelection`/`QString` snapshot and second rule owner
are removed. `Panel.h` retains the upstream method names and `QtPanelList`
supplies one narrow selected-state setter and redraw adapter. Mask dialog
creation remains in `mainwindow_selection.cpp`: Qt has no Win32
`CComboDialog::Create`, so the accepted `UString` mask enters the source loop
as an argument. `SaveSelectedState` synchronizes Qt's current selection before
each command until the source item-notification/mark body is ported.
The native `UString` wildcard overload exposed an existing macOS integration
bug: Qt/AppKit headers define `TARGET_OS_IPHONE` as `0`, so the old `#ifdef`
selected iPhone case sensitivity on desktop macOS. `Wildcard.cpp` now tests
the macro value, retaining upstream's desktop case-insensitive rule and its
source matcher instead of reverting to the Qt UTF-16 selection wrapper.

The parent sentinel is skipped in selection projection and type commands;
source folder indices remain stable across sorted table/icon views. Source
`_enableItemChangeNotify` suppression maps to the existing `CDisableNotify`
guard while Qt selection signals fire; this prevents command availability
refresh from overwriting the source vector midway through a multirow command.
The window refreshes command availability after the completed projection.
`CPanel::KillSelection`, `OnLeftClick`,
`OnArrowWithShift`, `OnShiftSelectMessage`, `OnInsert` and alternative-mark
state remain P0-07.3. The old `OnUpWithShift`/`OnDownWithShift` bodies are
commented out in the baseline and are not active omissions. Current tests
cover the bound source vector, parent sentinel, mask cancellation, disk/archive
mask/type and keypad actions in four views. Windows/Linux runtime and
rendered Windows parity remain unverified. Focused direct consumers
`panel_selection`, `panel_key`, `ui_state`, `panel_selected_state`,
`archive_selection_runtime` and `filesystem_selection_runtime` pass 6/6;
the full unsandboxed `./scripts/check.sh` gate passes on macOS arm64
2026-09-29 (135/135 CTest). No layout or text changed in this iteration;
the P0-07.1 normal/compact geometry evidence remains applicable.

## P0-07.3 — source alternative marks, mouse and keyboard selection (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([PanelSelect.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelSelect.cpp),
[PanelKey.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelKey.cpp),
[PanelListNotify.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelListNotify.cpp)).
Copied the active control flow of `CPanel::OnLeftClick`,
`OnArrowWithShift`, `OnShiftSelectMessage`, `OnInsert` and `KillSelection`
into the mirrored
`PanelSelect.cpp`, plus their `_selectionIsDefined`, `_selectMark`,
`_startGroupSelect` and `_prevFocusedItem` fields in `Panel.h`. The Qt click
signal supplies row/modifier state where Win32 used `NM_CLICK`/`LVKF_*`;
the key adapter calls the source shift-range completion after Qt moves focus,
replacing `PostMsg(kShiftSelectMessage)`. `QtPanelList` projects the original
marked vector as RGB(255,192,192) backgrounds in the shared table/icon model,
and constrains Qt selection to the single focused row where upstream sets
`LVS_SINGLESEL`. Parent sentinel rows are skipped. `EnsureSelectionVectorSize`
is narrow Qt initial-load glue: this presentation can publish rows without a
`RefreshListCtrl` call, while upstream initializes its vector during list
construction. It also preserves marks during incremental row loading.

`CApp::SetListSettings` now calls the panel transition, while the original
`CSettingsPage` reads/writes AlternativeSelection through `CFmSettings` with
Apply/Cancel persistence. `SaveSelectedState` retains source marks in that
mode; `RefreshListCtrl` restores marked names independently from Qt focus.
Current command availability, hash/extract guards and Copy summary use source
operated indices, so a single focused Qt row does not replace multiple marks.
`CDisableNotify` prevents Qt selection notifications from resetting the source
vector mid-command. Unadapted Win32 custom-draw, `LVS_*` style bits and
`HWND` messages are presentation only; source mark/range bodies remain.
`PanelKey.cpp` still has other P0-08/P0-09/P1 command branches outside this
selection scope. The deleted-item custom-draw branch remains P0-09; two-panel
drag/drop remains P1-16. The inactive commented
`OnUpWithShift`/`OnDownWithShift` are not ported.

`panel_selection` tests the source mark/range/Insert/KillSelection bodies,
focus restoration and parent sentinel. `panel_key` runs disk/archive ×
normal/flat × four views, plus root, settings toggles, Copy summary,
filesystem row removal, scroll and hover. `fm_settings` verifies enabled
Apply/Cancel persistence. Current-build 32 normal/compact 980×620/640×400
JSON reports and one removal report show visible unclipped lists; targeted
table/icon viewport screenshots show red marks separate from the blue focused
row. Focused direct-consumer CTest passes 9/9 and the full unsandboxed
`./scripts/check.sh` gate passes 135/135 on macOS arm64 2026-09-29.
Windows/Linux runtime and rendered Windows visual parity remain unverified.

## P0-08.1 — source panel switch and disk/disk Qt projection (2026-09-30)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([App.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/App.cpp),
[App.h](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/App.h),
[FM.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/FM.cpp),
[PanelKey.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelKey.cpp),
[resource.rc](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/resource.rc)).
Copied `CApp::SwitchOnOffOnePanel` creation/show/hide/layout control flow
into mirrored `App.cpp` and retained the original two `CPanel` owners, paths,
list modes and focus callbacks. Qt's `IQtAppPresentation::ShowPanel` replaces
Win32 `Enable`/`Show`; QSplitter replaces `FM.cpp::MoveSubWindows` HWND geometry.
The Qt action presents the resource's View → 2 Panels F9 command. The list's
Tab adapter calls original `CPanelCallbackImp::OnTab`; panel focus events still
call `CApp::SetFocusedPanel`. On failed second creation, Qt rolls back NumPanels
to one so a rejected widget cannot leave an invisible source owner active.

Win32 has separate child controls for each panel. `mainwindow_twopanel.cpp`
provides their Qt equivalent: each panel retains its own widgets, visible
location/view/sort and refresh snapshot; the existing command presenter projects
the active panel. The original `CPanel` still owns folders and source selection
vectors. The Qt filesystem scanner is single-owner: switching during a flat
scan cancels its generation and rebinds that panel when reactivated, preventing
queued rows from reaching the other panel. The existing portable metadata poll
now visits both visible disk panels. A missing provider during rebinding leaves
source file commands disabled instead of entering `CreateFileMenu` with null.

Scope is disk/disk including separate paths, view modes, selection, focus,
normal/flat loading and inactive refresh. Archive/disk and archive/archive
projection, Alt+F1/F2 target, startup splitter/layout restore and Copy/Move
destination default remain P0-08.2; actual cross-panel transfer bodies remain
P1-01. This is a partial port of the broader App/FM/Panel layout workflow, not
acceptance of the P0-08 parent. Win32 splitter drawing, drag target and direct
`HWND` messages are Qt presentation or later P1-16 adapters. Required project
clang-format changes only formatting. `app_lifecycle` covers success/failure of
the switch, while `panel_key` covers F9/Tab, active commands, path/view/selection,
inactive refresh, flat scan and JSON normal/compact/hidden geometry. The full
unsandboxed `./scripts/check.sh` gate passes 135/135 on macOS arm64 2026-09-30.
Windows/Linux execution and rendered Windows parity remain unverified.

## P0-08.2 — mixed panels, source path focus and restored layout (2026-09-30)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([App.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/App.cpp),
[PanelKey.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelKey.cpp),
[ViewSettings.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/ViewSettings.cpp),
[FM.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/FM.cpp),
[Panel.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/Panel.cpp),
[resource.rc](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/resource.rc)).
The existing mirrored `CPanelCallbackImp::SetFocusToPath` retains the source
one/two-panel target rule; the `PanelKey.cpp` Qt key adapter now calls it for
Alt+F1/F2. The existing `CWindowInfo::Read/Save` byte layout remains upstream;
`MainWindow` uses its panel count/current index/splitter coordinate at startup
and close, while Qt's QSplitter provides geometry. The startup presenter
temporarily projects each CPanel when it creates controls and restores the
requested active panel after widgets enter the Qt event loop. Both bound
filesystem and archive folders render into their own Qt panel state; archive
metadata, folder path, list entries and view state remain with that projection.

The Qt Copy/Move/Extract dialog adapter takes the default from the source
`CApp::OnCopy` opposite-panel rule and `CPanel::GetFsPath`. It keeps the current
one-panel destination when only one panel is visible. Real cross-panel transfer
to a target archive and full F5/F6 operation routing remain P1-01; accepting a
non-directory target in the current filesystem transfer rejects it safely.
`CApp::Save` skips an uncreated hidden panel so its previously saved path/view
and flat mode survive one-panel sessions. Source `CApp::Save` writes both slots;
the difference is needed because this Qt application defaults to one visible
panel rather than 7zFM's two.

Focused `panel_key`, `app_lifecycle`, `app_lifecycle_window`, `panel_focus`,
`panel_runtime`, `native_navigation`, and `binding_lifecycle` CTest consumers
passed on macOS arm64. The new mixed disk/archive, archive/archive,
Alt+F1/F2, dialog defaults, restored/hidden-panel tests and normal/compact
widget JSON establish the Qt scope. The full unsandboxed `./scripts/check.sh`
gate passes 135/135 registered CTest cases on macOS arm64, 2026-09-30. P0-08
parent is accepted; Windows/Linux runtime and rendered Windows visual parity
remain unverified.

## P0-09.1 — главное меню и toolbar 7zFM (2026-09-30)

Точный baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/FileManager/resource.rc` (`IDM_MENU`, порядок,
разделители, сочетания, `IDB_*`), `MyLoadMenu.cpp::CFileMenu::Load`,
`ExecuteFileCommand`, `App.cpp::g_ArchiveButtons/g_StandardButtons`,
`FM.cpp::OnCommand` и `ViewSettings.cpp::Read/SaveToolbarsMask`.
`MyLoadMenu.cpp` и application command/toolbar-state bodies уже перенесены в
предыдущих P0-03/P0-08 итерациях; текущая итерация подключает их к общему
Qt QAction меню. `resource.rc` ранее скопирован в `third_party/7zip`.

Без изменений скопированы 14 bitmap-файлов из того же каталога и commit:
`Add[2].bmp`, `Extract[2].bmp`, `Test[2].bmp`, `Copy[2].bmp`, `Move[2].bmp`,
`Delete[2].bmp`, `Info[2].bmp` (каждая пара означает исходный большой и малый
ресурс). Проверено байтовое совпадение с baseline checkout; применимая
лицензия — `third_party/7zip/License.txt`. В Qt `mainwindow_menus.cpp`
собирает группы/порядок из `IDM_MENU`, а `qtfilemanagercommands.cpp` связывает
Open Inside `*`/`#` и Exit с уже перенесённым исходным dispatch и общим
active-panel policy. `QMenu`/`QAction` заменяют Win32 HMENU/accelerators;
`mainwindow_preferences.cpp` выбирает оба размера BMP и применяет исходный
magenta mask вместо прежних нарисованных Qt иконок. Это presentation adapter,
а не независимый functional command switch. Русские подписи остаются текущим
Qt переводом до полного source language port P1-14.

Comment, Diff, Link, Alternate Streams, Benchmark, Delete Temporary Files,
Help Contents и About показаны как disabled source пункты: их functional/UI
owners ещё открыты (P1-08/P1-15/P2-09/P3-05). Действующие Qt Open/Create/Add/
Extract Selected/Journal оставлены отдельной группой File для доступа к
архивным сценариям до переноса их исходного входа. Исходный File menu в
компактном Qt окне может прокручиваться по правилам QMenu/монитора; исходная
Win32 small-screen фильтрация уже перенесена в `CFileMenu::Load`, но menu
presentation пока показывает disabled пункты явно по требованию плана.
Контекстное меню, status totals и system icons остаются P0-09.2;
ни полный P0-09, ни rendered Windows parity эта итерация не закрывает.

`main_shell` проверяет source order/ID/недоступные команды/shortcuts,
байтово скопированные BMP в Qt и active-panel меню после переключения/Tab.
Отчёты `main-shell-980x620`, `main-shell-640x400` и отдельные file/view menu
JSON описывают текущую Qt геометрию. Focused 8/8 и полный unsandboxed
`./scripts/check.sh` 136/136 прошли 2026-09-30. Проверена только macOS arm64;
Windows/Linux runtime ещё не проверен.

## P0-09.2 — per-panel context/status и системные иконки (2026-09-30)

Точный baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные пути и символы: `CPP/7zip/UI/FileManager/PanelMenu.cpp::CPanel::CreateFileMenu`,
`OnContextMenu`; `MyLoadMenu.cpp::CFileMenu::Load` и `ExecuteFileCommand`;
`PanelListNotify.cpp::CPanel::Refresh_StatusBar`, `ConvertSizeToString`;
`Panel.cpp::Create`, `OnSize` (четыре части status, под списком);
`SysIconUtils.cpp/.h::CExtToIconMap::GetIconIndex`, `FindInSorted_Attrib`,
`FindInSorted_Ext`; `resource.rc::IDM_MENU`.

Сохранённый ранее `CPanel::CreateFileMenu` содержит source policy для
operated indices, read-only, FS/hash/alt-stream folders и вызывает
`CFileMenu::Load`. В этой итерации контекстный Qt popup передаёт ему полный
source File action sequence с `programMenu=false` и строит результат по
исходному порядку и enabled состоянию. Win32 `OnContextMenu` служит
референсом позиционирования с клавиатуры: `(-1,-1)` заменяется центром
текущего элемента. Qt mouse/keyboard events и `QMenu::exec` заменяют HWND,
`TrackPopupMenu`. Отдельная Qt группа archive Add/Extract/Test/Info остаётся
после source file commands до переноса полного archive/context workflow.
`CreateSevenZipMenu` и `CreateSystemMenu` требуют Win32 COM/Explorer/extension
services; их группы на диске показаны disabled, системная — только при
ShowSystemMenu. Они остаются обязательными владельцам P1-02/P2-09/P3-05,
без объявления ready/N/A. Встроенная прежняя Qt команда открытия в системном
файловом менеджере удалена из context, так как подменяла source shell group.

Тело `CPanel::Refresh_StatusBar` скопировано в зеркальный `PanelListNotify.cpp`
в исходном порядке: operated/total, сумма выбранных размеров, размер и mtime
текущего выбранного элемента. `GetItemSize`, `GetRealItemIndex` и исходный
`ConvertPropertyToShortString2` остаются авторитетными; Qt адаптация меняет
только `_statusBar.SetText` на возвращаемые четыре `QString` поля и добавляет
переведённый префикс счётчика до P1-14. Каждый Qt panel размещает четыре
QLabel под списком; общий MainWindow QStatusBar несёт сообщения сканирования
и операций. Ранее разрозненная строка с числом объектов больше не является
единственным per-panel status.

В зеркальный `src/CPP/7zip/UI/FileManager/SysIconUtils.cpp/.h` перенесены
имена структур/класса, sorted binary searches, циклы attribute/extension
cache, лимит 65536 и shortcut числовых split-суффиксов. Win32
`SHGetFileInfo`/CSIDL/HIMAGELIST вместо копирования на macOS/Linux заменены
узким `QFileIconProvider` → `QIcon` adapter; исходные `Shell_GetFileInfo*`
функции остаются Windows-only и не вызываются Qt consumers. Диск и архив
запрашивают одну кэшированную карту при включённом ShowRealFileIcons; при
выключенном флаге остаются стандартные Qt значки. Источник сам комментирует
часть image-list integration в `PanelListNotify`; текущий Qt adapter
использует активную cache policy, но пиксельный вид системных иконок зависит
от ОС. Для особых provider icons (network/drive/shell namespace) нужны
P1-11/P2-10/P3-05.

`panel_shell` и прямые consumers проверяют disk/archive context, keyboard
anchor, status size/date/count и независимость двух панелей, кэш extension/
split и Qt JSON при 980×620/640×400. Первый полный gate выявил устаревшее
ожидание `ui_archive_states` о Refresh в контексте и скрытии disabled Rename;
тест обновлён по source меню, focused проверка прошла. После этого focused
direct consumers 11/11 и полный unsandboxed `./scripts/check.sh` 137/137
прошли на macOS arm64 2026-09-30. P0-09 parent принят; рендеринг Windows и
Windows/Linux runtime не подтверждены.

## P0-04.2 — root/address/parent window integration (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
The previously copied `RootFolder.cpp` (`CRootFolder::LoadItems`, `GetProperty`,
`BindToFolder`) and `PanelFolderChange.cpp` (`CPanel::OpenRootFolder`,
`OpenParentFolder`, `BindToPath`, `LoadFullPath`) remain the functional owner.
`PanelKey.cpp` now routes the resource `IDM_OPEN_ROOT_FOLDER` backslash command
through CApp; its other source key flow is retained. Qt's `mainwindow_root.cpp`
presents `IFolderFolder` item names and original indices, without a second root
enumeration or path algorithm. `mainwindow_archiveopening.cpp`,
`mainwindow_filelist.cpp` and `qtfilemanagercommandworkflows.cpp` bind root,
typed relative paths, parent and Back through the panel; Qt history uses an
empty marker for the source root prefix. `qtfilemanagercommands.cpp` derives
Parent availability from the native folder, including POSIX `/`, and prohibits
filesystem mutations on the read-only root. View-menu text, icon, list cells,
address clearing and normal/compact presentation are Qt adaptations.

The original Windows RootFolder has Computer/Documents/Network/Volumes; POSIX
has one Computer entry binding `/`. Windows drives/network namespace and Browse
providers remain P1-11, source icons/menu completeness P0-09, filesystem model
and polling P0-05/06, external editor P1-08. No full-module RootFolder claim is
made. Current root, relative/Unicode address, Parent/Back, busy stale-command
and source-index checks are in `root_navigation`; earlier P0-04.1 archive/nested,
password/cancel/update and TAR/GZip evidence is retained. Windows/Linux runtime
and rendered Windows comparison remain unverified. The current-build root JSON
reports at 980×620 and 640×400 establish Qt geometry on macOS only.
The first full gate found seven flat-filesystem crashes while CPanel had no
bound folder during scanning; the command projection now checks that transient
state before querying IsRootFolder. The affected eight CTest suites pass 8/8,
13.45 s; final formatting/build/all CTest gate passes 130/130, 87.96 s on
macOS arm64, 2026-09-29. This completes current P0-04 parent navigation scope;
the named P0-05/P0-09/P1-08/P1-11 owners retain their omissions.

## P0-04.1 — source navigation/parent-stack component (accepted prerequisite, 2026-09-29)

Latest internal scope (2026-09-29; supersedes earlier partial callback snapshots):

MainWindow root/nested/parent consumers now call the source CPanel BindToPath,
OpenItemInArchive and OpenParentFolder bodies. The Qt ArchiveParent stack and
its duplicate temporary-folder owner have been removed. Archive rows query the
current CAgentFolder properties; address presentation adapts the existing
separator to a native binding path. Ordinary disk binding also uses BindToPath.
The file-only engine resource retains ArchiveInfo for presentation beside its
existing authoritative metadata transport; it does not add a parent stack.
Source CAgent::ReOpen, GetPathParts and ordinary-folder restore control flow
back a narrow Qt worker/modal refresh bridge. Refresh failure must release
invalidated native proxies. CSelectedState crosses the delayed Qt row-loading
boundary as a presentation snapshot so source parent-focus requests survive.
These consumers complete the integrated P0-04.1 prerequisite. App/affected builds pass;
focused native_navigation_window, nested_archive, source_extract_backend,
panel_navigation, external_launch and source_update_backend pass 6/6, 3.37 s
on macOS arm64 2026-09-29. Real archive cases include unknown extension,
TAR/GZip, nested Unicode/space ZIP and externally updated ZIP refresh with
selection/parent focus. Native selection restoration suppresses notifications;
the Qt presenter must refresh command availability afterward (outside-open
regression covers this). A source transport cancellation now returns E_ABORT
without SetOperationResult(DataError), matching native engine callback semantics;
the regression verifies no erroneous result callback, idle backend and preserved
archive bytes. Native CPanel destructor CloseOpenFolders is retained in Panel.cpp
after stopping Qt notifications and joining per-panel external workers; a nested
memory temp-directory fixture verifies cleanup and preserved original archive.
Window listing fixtures now finish requests inside the source modal owner;
busy-command, keyboard, password, cancel, error, selection and mutation-refresh
assertions remain active during that lifecycle. Fixtures create owned local files
because source binding checks file existence. Bookmark disappearing-folder cases
check restore-to-surviving-ancestor control flow. A flat-mode renderer guard stops
old row selection notifications while the native provider changes its item count.
Deferred selection is consumed only after native row loading, then cleared on
folder release; fresh archive opens cannot inherit another owner's selection.
Root opening retains backend result text in the Qt status/journal and presents
failed opening after the native joined modal lifecycle. Native progress splits
directory and basename across lines; folder properties retain trailing separators.
Focused bookmark passes 1/1, 0.61 s; properties/panel_sort 2/2, 1.29 s;
panel_selected_state 1/1, 3.45 s; settings/panel_key 2/2, 3.82 s;
panel_runtime/archive_selection_runtime/binding_lifecycle pass individually
2.14/2.21/2.93 s; ui_operations/ui_archive_states/properties_runtime pass
3/3, 4.49 s. These macOS arm64 results precede the final formatting/full gate.
Final acceptance: check.sh formatting/build/all CTest PASS 119/119, 58.65 s,
macOS arm64 2026-09-29, /tmp/sevenzip-p0-04-1-check-final.log. Initial sandbox
run could not create private DBus sockets; the permitted local gate used required
socket/process permissions. The first unsandboxed run caught obsolete checksum
post-open waiting and timestamp catalog expectations; both were corrected and
the final complete gate passed. Additional nested_update drives native
OpenParentFolder/changed-file prompt through a real ZIP update, failure and cancel:
successful replacement cleans owned temp; unsuccessful update preserves both
outer archive bytes and changed temp. Focused nested_update/checksum pass 2/2,
0.90 s; timestamp catalog 1/1, 0.25 s. Native extraction callback-contract tests
have their own focused target (operation suite279 lines, contract137).
Current-build 16 archive-runtime JSON reports show address/active view fully
visible at 980×620 and 640×480 in all four normal/flat modes. No Windows rendered
reference or Windows/Linux runtime validation is claimed. All 88 newly vendored
files match the exact recorded upstream baseline byte-for-byte. Required formatting
of adapted src/tests is separate from functional adaptations. The prerequisite
was committed as `d0bc1d2`; full P0-04/P0-03 command acceptance,
known/editor/child-process workflows, model/transport and other parent omissions
remain mandatory under their recorded owners.

Commit scope review: all 248 staged paths belong to this integrated prerequisite,
including its retained unfinished work recorded in the sequential-goal handoff.
The scope contains 62 adapted upstream files, 46 Qt/engine integration files,
39 regression/consumer files, 88 exact upstream dependencies and 13 build/docs/UX
evidence files. Native extraction, update, hash callback and prompt dependencies
are needed by PanelCopy/OpenItemInArchive/changed-file return; they are not separate
completed packages. Every non-vendor source/test basename is accounted for in
this audit, the handoff or CMake registration. The index matches the checked
working tree; the 88 vendor files match the recorded baseline byte-for-byte.
No local memory, temporary files, plan/prompt edits or upstream changes are included.

External launch dependency now copies SplitCmdLineSmart and native editor/viewer
ReadRegEditor/SaveRegEditor key selection with explicit QSettings FM storage.
The native no-process-handle branch of CTmpProcessInfo/MyThreadFunction and
CTmpProcessInfoRelease is connected to an injected association service.
CExitEventLauncher fields/event/thread accounting remain source-derived; the
owner is panel-scoped instead of a mutable global. Native Thread_Wait_Close
replaces the Win32 wait/close calls and always joins before panel members die,
including hard exit (the original 100 ms timeout could outlive a Qt/POSIX owner).
Tracking capacity is allocated before launch/thread creation, preventing an
allocation exception from freeing a record already used by its worker.
IsReadOnlyFolder/IsThereReadOnlyFolder are copied at their original PanelMenu path.
Exact unchanged Common/AutoPtr.h supports the source unique-pointer ownership.

The association service intentionally returns no process handle; successful
launch therefore retains the temp until owner exit. The original tail checks
the baseline file size/time before deleting: changed data remains on disk.
Known/short-lived main processes, CChildProcesses snapshot/handle tracking,
StartEditApplication and their active changed-file update sender remain
mandatory P0-04/P1-08 scope. MainWindow now attaches the manager and association
service; the component is not accepted as the complete external-open workflow.
Focused external_launch uses real ZIP extraction and a fake association, never
starts an external application: successful unchanged cleanup, changed-file
preservation and failed-launch cleanup; independent editor/viewer settings and
source quoted/unquoted command split cases also pass. Latest external_launch
plus source_extract_backend passes 2/2, 1.37 s on macOS arm64 2026-09-29 before
the later localized explicit destructor join edit. Rebuilt affected extraction/
update/external consumers after that edit pass 3/3, 2.07 s. Added memory-probe
fallback verifies a plain file is first extracted into memory, returns native
S_FALSE from its archive probe, then flushes to disk for association launch;
external_launch passes after that addition (0.54 s).

QtAssociatedFileServices is narrow Qt/OS glue around the existing injectable QUrl
opener; it reports failure/throw as E_FAIL with a plain-text launch error.
MainWindow now attaches retained manager/association services to its source panel,
with service lifetime preceding CApp member lifetime. Its old navigation consumers
are still being migrated. The service exposes no process handle; original Win32
PIDL/shell handle and extensionless-dot branches of BrowseDialog2::StartApplication
and known/editor process services remain mandatory, not claimed by this Qt API.
Fake opener regression preserves Unicode/space/literal markup in the QUrl path
and verifies plain-text failure. Latest external_launch/application_starts pass
2/2, 0.76 s; app builds on macOS arm64 2026-09-29. No user applications launched.

PanelCopy.cpp now copies CPanelCopyThread, native archive CopyTo/test dispatch,
callback/password/messages/result lifecycle, hash/statistics setup and parent-zone
inheritance. CCopyToOptions and CPanelCallback declarations retain native fields
and method names. HWND/result presentation uses QWidget and the existing source
progress dialog; a scoped resource transport connects the joined worker to 7zz.
The callback receives the explicit settings path. CContextMenuInfo preserves
native defaults and defined-value Save/Load branches with QSettings replacing
the Options registry key. Native filesystem CopyFileSystemItems remains an
explicit mandatory P0-05/P1-02 dependency returning E_NOTIMPL in that branch;
later update/copy-from/delete PanelCopy bodies are not ported by this subset.
CApp callback implementation/title ownership remains mandatory P0-02 integration.

PanelItemOpen.cpp additionally copies OpenAsArc_Index, IsVirus_Message and the
internal OpenItemInArchive flow: direct stream, bounded CVirtFileSystem, disk
fallback, password inheritance, native corrected name, zone options, archive
probe and temp/link ownership. IsItem_AltStream is copied from PanelItems.cpp;
Explorer/ContextMenu.cpp contains only its original shared FindExt dependency,
not shell integration. RAM information is obtained through the native portable
System service per invocation instead of mutable globals; deep nesting guards
the source shift against an out-of-range shift. Error presentation uses native
MyFormatMessage and Qt plain-text prompts. The external no-handle association
subset is described above; known-process/editor tracking and changed-temp update
sender remain mandatory, and unavailable services report unsupported. MainWindow
still uses its old stack; this is internal WIP, not completed navigation parity.

Source extraction now implements the native all-items contract (ignores a
nonnull indices pointer for count == UInt32(-1)), validates selected indices
before side effects, reports total unpacked bytes and accumulates CopyStream
progress between entries. A skipped callback stream retains the native skip
contract. Public CPanel::CopyTo disk/memory and native nested open/parent return
are tested with real binary ZIP fixtures and Unicode/space names; the latest
rebuilt source_extract_backend passes on macOS arm64 2026-09-29 (0.75 s).
All-items/multiple-entry progress regression now passes: a deliberately invalid
nonnull index is ignored for the all-items sentinel, both binary files match,
and completed bytes remain monotonic through the combined total. Rebuilt direct
consumers pass 10/10, 3.41 s: extract_prompt, ui_parity_overwrite/ui_parity_mem,
source_extract_backend/source_update_backend, native_hash_bundle,
native_extract_callback, panel_navigation, archive_opening and
compression_preferences. Separate context_menu_settings passes (0.27 s),
including undefined flags/unrelated setting preservation and invalid values.
No full gate, accepted iteration or commit is claimed for this continuation.

The native OnOpenItemChanged message body now takes a typed CTempFileInfo/prefix/
password request instead of LPARAM/window handle. Source prefix guard, stale
index relocation by relative name, saved selection, disable-notify, update and
result flow are retained; a released-folder guard is the necessary Qt lifetime
adaptation. Real ZIP regression rejects mismatched prefix and removed item with
no engine start, relocates index 99 to the existing Unicode name, updates only
that file, then rejects a released owner. Latest affected extract/update passes
2/2, 1.71 s (macOS arm64 2026-09-29); external process sender remains mandatory.

Exact unchanged resourceGui.rc is now included in compiled native STRINGTABLE
inputs, resolving missing HashGUI/test/open/moving source messages. Fixed Russian
captions remain the presentation adaptation pending the full language owner.
hash_results checks real CRC32 rows, source title/flags/no initial selection and
list/footer/buttons at 540x360 and 420x360; current JSON has visible 518x307/398x307
lists and separate footer. The test uses QPushButton sizeHint for caption sizing
(the inspection labelFits helper is specifically for QLabel). Latest hash_results,
source_extract_backend/source_update_backend and resource_strings_generator
pass 4/4, 2.05 s, macOS arm64 2026-09-29. No PNG/rendered Windows parity claim.

Hash dependency scope for PanelCopy: MethodProps.cpp now copies the complete
original module (only canonical includes/formatting adapted). HashCalc.cpp ports
CHashBundle::SetMethods/InitForNewFile/Update/SetSize/Final, native AddDigests,
CHasherState::AddDigest/WriteToString and HashHexToString. The rest of the hash
file reader/writer/verifier/signer remains mandatory P2-03 scope. Exact unchanged
CreateCoder.cpp/FilterCoder.cpp/.h/RegisterCodec.h provide source factories.
HashGUI.cpp/.h port only native value/bundle result formatting and ShowHashResults;
the existing source-derived Qt ListViewDialog replaces HWND/Create. HashCalcGUI
and its separate worker remain mandatory with the hash owner.

Unchanged native C hash implementations/registration dependencies now supply
CRC32/CRC64/CKSUM/MD5/SHA1/SHA256/SHA384/SHA512/SHA512-256/SHA3/XXH64 and BLAKE2sp.
The global BLAKE2sp hasher/registration block is copied from Rar5Handler.cpp at
its original adapted path; the archive-handler body is not copied by this
dependency port. Applicable source/license notices are preserved. All dependencies
use the same recorded full upstream revision. CMake OBJECT/interface linkage
keeps the original registration constructors in final consumers instead of
silently discarding them from static archives. HashCalc is in sevenzip_common,
HashGUI in sevenzip_panel, preserving dependency direction; archive operations
continue through ArchiveBackend and the official external engine.

Native hash regression checks CRC32's fixed vector/statistics, incremental
MD5/SHA1/SHA256/SHA384/SHA512/SHA3-256 against Qt's independent digest provider,
wildcard registration including BLAKE2sp/CRC64/XXH64, and unsupported method
failure. App/affected targets build; native_hash_bundle plus source_extract_backend
passes 2/2, 1.03 s, macOS arm64 2026-09-29. Native IArchiveFolder and zone setter
interfaces are now advertised by CAgentFolder with their source bodies; the
complete IInFolderArchive owner and full source PanelCopy integration remain
mandatory. This dependency work does not close P0-04.1 or P2-03.

Agent.cpp now also contains the full native CAgentFolder::Extract body;
ArchiveFolder.cpp copies CopyTo and zone setters. Native path parts, extraction
callback setup/options, selected real indices, hard-link preparation and closer
remain source bodies. Native IArchiveFolder and zone setter COM interfaces expose
these bodies; the complete IInFolderArchive owner remains mandatory.
The Win32 alternate-stream prefix
check is guarded on POSIX, where a colon is a filename character. New source
fields retain upstream defaults. QtArchiveFileResource now attaches an extraction
service and advertises folder operations when that capability exists, even for
read-only archives; update remains independently guarded.

The file-only boundary invokes ArchiveBackend::extractSourceEntry per selected
native entry. Official 7zz binary stdout goes to an owned private staging file;
stderr carries process messages/progress/password prompts. Native CopyStream
then feeds the source callback stream with its progress/break contract. Subsequent
jobs restore normal channels. Original paths are CLI selectors, never staging
filesystem names; source callbacks own corrections and final filesystem names.
CLI selection cannot distinguish duplicate original names, so that unsupported
transport branch returns E_NOTIMPL before concatenating data and remains mandatory
codec/index transport debt. Error distinctions/metadata links and complete
multi-item progress remain under review, not accepted parity.

New source_extract_backend checks native public CopyTo into disk and bounded
virtual files, Unicode/spaces/binary bytes, binary-vs-message channel separation
and channel reset, existing output preservation, failed process start, cancelled
child cleanup/original preservation and malformed archive cleanup. App/affected
targets build; focused extraction/update/opening passes 3/3 (latest command
completion recorded in local memory). No P0-04.1 full gate/commit/acceptance.

FileManager `ExtractCallback.cpp/.h` now copies the full CExtractCallbackImp,
CGrowBuf/CVirtFile/CVirtFileSystem functional bodies from
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at
https://github.com/ip7z/7zip. Includes and required formatting are adaptations.
OverwriteDialog.cpp/.h retains CFileInfo, ReduceString, AddSizeValue,
SetFileInfoControl text body, default/extra actions and native returned IDs;
MemDialog.cpp/.h retains AddSize_GB/AddInfoMessage_To_String, RAM/range/default
branches and OnContinue parsing/result fields. Win32 controls/layout/modal calls
become Qt Widgets; shell icons use QFileIconProvider with the optional second
EXE/ICO icon omitted. A separate edit with arrow buttons preserves typed values
outside the native spinner range. Native result constants have a narrow portable
header. New modal bodies execute completely on the GUI thread through a joined
source progress parent; Password delegates to the existing scoped presenter.
The global DisableUserQuestions switch becomes callback-owned state. Settings
use explicit Ini paths for native Read/Save_LimitGB and show-password branches.
POSIX filesystem times use native FiTime/FILETIME conversions; Win32 read-only
alternate-stream attribute restoration stays Windows-only. Common zone helper
bodies are compiled on POSIX for CVirtFileSystem too: a colon-suffixed POSIX
file is a named file, not a Windows alternate stream. Native extraction zone
activation remains Windows-guarded; OS-specific zone propagation still requires
integrated review, and is not declared N/A.

Exact dependencies added: Common/Lang.h, OverwriteDialogRes.h/.rc,
MemDialogRes.h and resourceGui.h. Overwrite STRINGTABLE is included in the same
compiled source resource owner with fixed Russian captions. The reference tool
now parses ICON, with a deterministic regression to avoid swallowing adjacent
resource statements. Generated pinned reference/contract pairs cover both new
views. New extract_prompt cases exercise native memory/spill bytes and filename
correction, callback stop and persisted memory limit, overwrite answers/default/
cancel/native formatting, and memory field enable/invalid/commit/cancel.

Evidence: app and affected targets build; focused
`^(extract_prompt|ui_parity_overwrite|ui_parity_mem|ui_parity_tool|native_extract_callback|source_update_backend|archive_opening)$`
passes 7/7, 2.04 s, macOS arm64 / Qt 6.11.2, 2026-09-29. Each view has four
current-build JSON reports at actual widths 700/560, no tested clipping; Windows
rendering and Windows/Linux runtime unverified. This is internal WIP, not a
completed iteration/commit. Full AgentFolder Extract/CopyTo, file-only engine
extract transport, PanelCopy/OpenItemInArchive and MainWindow's one native
navigation owner remain mandatory before P0-04.1 acceptance/full gate.

Baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
https://github.com/ip7z/7zip. P0-10.1 prerequisite is accepted at `d966b79`.
Inspected `CPP/7zip/UI/FileManager/PanelItemOpen.cpp` OpenAsArc/Name/Index,
OpenParentArchiveFolder, CThreadCopyFrom/OnOpenItemChanged and
OpenItemInArchive memory/temporary/password/zone branches; PanelFolderChange.cpp
BindToPath/LoadFullPath/OpenParentFolder/CloseOneLevel/CloseOpenFolders;
FileFolderPluginOpen.cpp CThreadArchiveOpen/CFfpOpen; OpenCallback.cpp;
Common/ArchiveOpenCallback.cpp; Agent/ArchiveFolderOpen.cpp and
ArchiveFolderOut.cpp CopyFromFile/CommonUpdateOperation. These bodies, required
dependencies and existing root/nested/address/parent consumers form one result.
The old Qt ArchiveParent stack remains migration debt until replaced; no helper
or newly copied dependency closes this iteration or its parent.

Current internal dependencies: unchanged FileStreams.cpp/.h, StreamUtils.cpp/.h,
UniqBlocks.h and ArchiveOpenCallback.cpp/.h copied under third_party from the
exact baseline. Native open/volume stream lifetime, counters, password and
reopen contracts retained. Full Common/Wildcard.cpp/.h replaces the previous
partial copy in its existing mirrored path. One source EnhancedMaskTest body
is parameterized for wchar_t/native case policy and UTF-16/Windows-style Qt
selection; Qt overloads only convert/present at that boundary. All targets now
link the one common implementation instead of compiling the same source again.
Canonical includes and mandatory formatting are separate adaptations.

ArchiveExtractCallback.cpp currently ports only complete CLinkLevelsInfo::Parse
and IsSafePath dependencies needed by native volume opening; all other extraction
bodies remain mandatory. FileDir.cpp gains original GetFullPathAndSplit, POSIX
FiTime_To_timespec/SetFileTime_Base/SetDirTime and C_umask bodies required by
unchanged FileIO/FileStreams. MyGetFullPathName's existing GetFullPath forwarding
is collapsed to that same native call; no filesystem path algorithm is replaced.
FileIO.cpp is now built on POSIX as well as Windows.

OpenCallback.cpp/.h copies all functional counter/break/finished/password bodies;
HWND/INT_PTR become guarded QWidget/qintptr. Qt password creation/retrieval runs
on the GUI thread through the existing callback service, including accepted
empty password and ShowPassword. Original Read_ShowPassword/Save_ShowPassword
use explicit settings path/QSettings instead of Registry and preserve their
default false/read/write scope. No password is persisted.

Status: active WIP. Affected dependency/selection/update targets build; integrated
source/consumer acceptance and focused checks pending. No full gate or commit
is appropriate before the integrated navigation result.

Further current WIP: FileFolderPluginOpen.cpp/.h retains complete active source
worker, error-level composition, callback creation/result/password/lifetime and
library transfer flow; injected native IFolderManager replaces only construction
of the in-process CArchiveFolderManager at the engine boundary. CPluginLibrary
becomes its native CLibrary base because disabled plugin-loading branches are
not executed. Settings path and QWidget parent are explicit Qt services.
PanelItemOpen.cpp now contains OpenAsArc/Msg/Name, OpenParentArchiveFolder and
CThreadCopyFrom/OnOpenItemChanged bodies; PanelFolderChange gains native folder
interface Release/QueryInterface, LoadFullPath and CloseOneLevel/CloseOpenFolders.
MainWindow still uses its former stack: runtime integration remains mandatory.
Source changed-temp YES updates, NO/CANCEL cleanup and update-error preservation
are retained; original CANCEL on this prompt does not cancel leaving the folder.
Native StreamObjects.cpp/.h and C/Alloc.c/.h are exact unchanged dependencies for
memory streams. POSIX ProcessWasFinished also signals the existing finished
latch for FileFolderPluginOpen's own worker entry point; Create still joins it.

Focused evidence so far: panel_selection/update_callback 2/2, 0.75 s;
open_callback 1/1, 0.34 s; panel_navigation 1/1, 6.04 s before the last POSIX
generic-worker latch extension. These verify component contracts only, not
completion of P0-04.1 or real engine/navigation consumers. macOS arm64 2026-09-29.

After the generic-worker latch extension, focused progress_worker/navigation/
open_callback/archive_progress/progress_cancel pass 5/5, 5.45 s; native navigation
fast-finish test now 0.34 s. Further WIP copies BindToPath and SetToRootFolder
functional bodies, with only CDirEntry replacing CFileInfo in the POSIX enumerator
boundary (Windows aliases them). RootFolder ordinary filesystem binding and
CFSFolder::InitToRoot are exact early prerequisites; full root/drives/network/
Browse namespace remains P1-11, ADS P0-07 and system icons P0-09. RootFolder's
network construction/fallback and ADS branches explicitly return E_NOTIMPL
until those required owners are ported, not done/N/A. Native drive provider
still has its documented unfinished volume capability. Names/indices, root and
ordinary path binding retain source flow; Qt DocumentsLocation substitutes the
Windows shell path service, icon indices remain -1 pending icon presentation.
This is not acceptance of the full RootFolder module or its namespace.

Further WIP in the same iteration: source BindToPathAndRefresh,
GetParentDirPrefix, OpenParentFolder and OpenRootFolder bodies are copied;
RefreshListCtrl's no-argument overload retains its source CSelectedState wrapper.
The existing selection-only overload still lacks the full list load/presentation
stage; MainWindow consumer replacement is mandatory before acceptance.

QtArchiveFolderManager now implements the injected native IFolderManager boundary
for the source CFfpOpen worker. GUI process requests, password requests and stop
are queued to ArchiveBackend; the worker observes completion even while paused
and waits for engine quiescence. The shared ArchiveProgressTransport supplies the
same native owned-process controls used by Add rather than another implementation.
Forced archive format is passed through ArchiveBackend::openFolder, not widgets.
Source COpenArchiveCallback and COpenCallbackImp retain password/volume contracts.
The engine's listing transport maps the exact Console/List.cpp
Print_OpenArchive_Error/Open_Strict S_FALSE labels at this baseline; unknown
output, start/I/O errors and crashes remain E_FAIL, cancellation E_ABORT.
Only error-label comparison uses canonical Unicode on macOS, whose engine emits
filesystem-native NFD; native/archive entry names are preserved.

Exact unchanged Compress/CopyCoder.cpp/.h and StdAfx.h are additionally vendored
from the same baseline. Native IInStream opening uses its CopyStream algorithm
and native COutFileStream at the file-only engine boundary. QtArchiveFileResource
supplies only a private temporary filename and cleanup; every bound folder retains
the source CAgent, which now retains the external engine resource in place of an
in-process archive stream owner. Failed opening removes the spool, successful
opening retains it until the last source folder is released. This does not yet
accept OpenItemInArchive extraction-to-memory, virtual-file update/writeback,
zone branches or real nested/MainWindow consumers; those remain mandatory.

Focused evidence: opening/progress/navigation/open_callback 4/4, 2.08 s after
GUI password/cancel and macOS label fixes; after memory-spool integration,
archive_opening/panel_navigation 2/2, 0.96 s. Real 7zz 26.03 (matching baseline
7zVersion.h) ZIP/Unicode-space file and native memory-stream fixtures pass,
including folder-retained spool lifetime, S_FALSE cleanup, empty password,
password cancellation, stop-confirm/quiescence and failed engine start.
After source parent/root-method integration and additional parent/pop/root
regressions, the same four focused tests pass 4/4, 2.34 s. All evidence is
macOS arm64 on 2026-09-29; Windows/Linux runtime and integrated final acceptance
remain unverified. No additional helper commit/full gate has been made.

Changed-file update integration has an additional exact dependency: existing
AgentProxy uses QString/std containers and listing-based Load in place of native
owned names, vectors, CArc/IInArchive properties and full memory/CRC/raw handling.
The old accepted evidence remains limited to its documented copied control-flow
subset; it does not validate the omitted native model. Source
ArchiveFolderOut::CommonUpdateOperation saves/restores path parts and reopens the
source CAgent after its work-directory transaction; AgentOut::UpdateOneFile
builds native CUpdatePair2 values for one real index and keeps original names.
These bodies cannot be replaced by calling the old delete-then-add Qt
replaceEntry workflow and described as a source port. Native model, update pair,
work-directory/temporary/move/reopen prerequisites must accompany this consumer;
their full parent-model/update acceptance remains under P0-03/P1 owners.

Current WIP replaces the earlier partial AgentProxy implementation with the full
baseline AgentProxy.cpp/.h: CProxyArc/CProxyArc2, native owned names/vectors,
property/default-name/CRC/memory-limit/raw-parent handling and path/real-index
algorithms. Qt path lookup used by remaining presentation consumers is now a
narrow adapter outside this borrowed module. QtPanelFolder exposes a read-only
IInArchive metadata view over the same engine catalog; a forwarding metadata
adapter supports existing IFolderFolder providers. Native CAgent::ReadItems calls
CProxyArc::Load(CArc, progress), rather than rebuilding the tree from a Qt list.
Full OpenArchive.h declarations and its item-boolean/default-path helpers are
copied; the remaining opening/parser/CArchiveLink bodies are still mandatory,
not accepted merely because their declarations are present. Exact unchanged
LoadCodecs.h, DirItem.h, Property.h and Archive/Common/ItemNameUtils.cpp/.h plus
StdAfx.h are additional dependencies from the same recorded baseline.

One necessary functional deviation fixes CProxyArc2::Load's BSTR fallback:
the baseline declares CPropVariant inside the fallback branch but consumes its
bstrVal pointer after that scope destroys the value. The value now remains alive
through AllocStringAndCopy. A deterministic native-tree fixture with Unicode,
spaces, invalid parents, memory-limit failure and reload covers this path.
Temporarily restoring the baseline scope makes the name assertion fail (empty
root name versus `root й space`); restoring the fix passes. Native model/opening/
navigation/property/agent focused tests pass 5/5, 1.61 s on macOS arm64,
2026-09-29; the application builds. Earlier historical Qt-container claims remain
evidence only for their recorded scope, superseded by this current unaccepted WIP.

Work-directory prerequisites now copy full Windows/FileDir.cpp/.h source bodies
and Common/WorkDir.cpp/.h, including CTempFile creation/exclusive collision retry,
work-mode selection, output lifetime and MoveToOriginal. CTempDir's unchanged
source Create/Remove flow is enabled on POSIX; its Win32-only recursive removal
dependency uses the existing Qt filesystem boundary there. QString/cancellation
cleanup overloads are retained for current consumers. Exact C_FileIO.h and
MyBuffer2.h are vendored dependencies. NWorkDir::CInfo preserves defaults and
load validation; QSettings replaces Registry using explicit settings paths and
original option names. CreateTempFile takes that explicit path instead of reading
implicit global settings. This is prerequisite scope inside P0-04.1, not full
filesystem or update acceptance. Focused work-directory/filesystem cleanup/
operations/opening/navigation tests pass 5/5, 1.59 s (macOS arm64, 2026-09-29).
They cover Unicode/space paths, specified/current/default work modes, missing
specified-path fallback, publication, abandoned-output removal, invalid work
directory preserving the original, and POSIX temp-directory symlink cleanup.

The same result now includes complete source EnumDirItems.cpp/.h and SortUtils.cpp
plus exact UpdateAction.cpp/.h, UpdatePair.h, UpdateProduce.h, SortUtils.h and
Common/StdAfx.h dependencies. Full UpdateCallback.cpp/.h and
Agent/UpdateCallbackAgent.cpp/.h are being connected next; native pair/property/
stream/password/error bodies are retained with canonical dependency includes.
Exact Windows/SecurityUtils.cpp/.h and original InitLocalPrivileges from
ArchiveExtractCallback.cpp supply the retained Windows security branches.
These newly copied callbacks are WIP dependencies, not accepted update consumers.
UpdateCallbackAgent uses native UInt64 instead of Windows-only UINT64 in its two
counter definitions. OpenArchive.cpp now additionally retains the complete native
CArc item path-to-parent/path/path2/item/size/time bodies and alt-stream colon
helper, needed by callbacks; codec-opening/parser/link bodies remain unported.
Focused archive_update_callback/work_dir/archive_proxy tests pass 3/3, 0.80 s.
The callback fixture checks original archive names alongside new disk data,
native update-pair index/flags, stream close notifications, accepted password,
failed-file messages, cancellation and Before_ArcReopen clearing stop status.
Current application and direct UI consumer targets build; archive selection,
panel runtime, binding lifecycle and compression/extract settings checks pass
5/5, 4.99 s. All results are macOS arm64, 2026-09-29. Full P0-04.1 consumer
replacement and required update/reopen transaction remain open; no helper commit
or full gate is appropriate at this point.

Further WIP removes the unused CAgent ArchiveEntry list and duplicate retained
folder-property pointer. CArchiveLink owns the authoritative native CArc metadata;
CAgentFolder and its remaining Qt selected-path consumer query that owner.
Original CArcErrorInfo::ClearErrors and CArchiveLink KeepModeForNextOpen/Close/
Release bodies are copied, along with the source RAM-limit policy for CProxyArc.
AgentOut.cpp currently copies only SetFolder, SetInArchiveInterfaces, CommonUpdate
and full UpdateOneFile; IArchiveFolderInternal/GetAgentFolder and CanUpdate use
their original bodies. SetFolder is temporarily a regular method on the partial
owner, rather than advertising an incomplete IOutFolderArchive interface.
All other AgentOut operations, full Agent interfaces/raw-folder branches and the
ArchiveFolderOut transaction/reopen consumers remain mandatory.

The source UpdateOneFile fixture passes (archive_update_callback, 0.37 s): only
one real index inside a bound subfolder changes, the archive name survives a
different disk filename, unrelated pairs remain unchanged, and invalid selection/
missing disk file/tail guard fail before the engine call. Later affected native
owner/opening/callback/backend checks pass 4/4, 1.67 s, macOS arm64 2026-09-29.
All newly vendored files compare byte-for-byte with the recorded baseline;
the sibling checkout has no modifications.

ArchiveBackend::updateSourceFile is a new narrow process transport for native
IOutArchive callbacks. The exact baseline ArchiveCommandLine.cpp
ParseUpdateCommandString/SetAddCommandOptions protocol supplies `-u-` and the
separate `!output` command; selected-file pair states force new data/properties,
while unselected entries are copied. It writes an uncommitted result for native
WorkDir publication and never invokes the old Qt delete-then-add workflow.
Current standalone engine fixtures pass for ZIP/7z, original bytes/other entries,
cancel after native process pause, failed start, existing-output preservation and
unsafe names. Source ExtractingFilePath.cpp/.h is copied in full for this new
boundary: IsSafePath and Correct_FsPath validate staging names. Names requiring
correction are rejected at the engine boundary because changing their name would
violate native KeepOriginalItemNames; other extraction's former safePath remains
migration debt in its own required scope. NUL cannot cross a native path boundary.
QtArchiveSourceUpdate now supplies the narrow file-only IOutArchive service.
The source AgentOut UpdateOneFile produces the native callback plan; the service
accepts only that plan, stages GetStream through unchanged CopyStream/FileStreams,
and copies the separate engine result into the caller's native output stream.
Qt only provides private names/directories and queued process services. The
archive property wrapper retains the resource and advertises IOutArchive only
when that service is attached. Read-only consumers do not acquire a backend link
dependency: the resource stores an injected service, not engine decisions.
The native worker fixture now calls CAgent::SetFolder/UpdateOneFile for a bound
subfolder with a differently named disk file; original archive bytes, original
entry name, sibling entry and closed source input streams are checked against
the real 7zz result. QtArchiveBackendRequest's modal worker fixture checks that
source Create waits for process completion. Both remain internal P0-04.1 WIP:
production transaction/reopen/MainWindow consumers are mandatory and incomplete.

CArc::ReadBasicProps and Archive_GetArcProp_UInt/Int are copied with their full
source bodies from the same baseline; the existing GetOpenArcErrorFlags owner is
reused. ArchiveInfo now transports the engine's numeric Offset header alongside
Physical Size. QtArchiveFileResource supplies only those typed properties and
the actual source file size; native ReadBasicProps computes tail/unexpected-end
and CanUpdate retains its native tail guard. Tests cover exact fit at offset 16,
tail size 32 and unexpected end. Prefix publication remains mandatory; the current
update service rejects nonzero offsets until native CommonUpdateOperation is
connected. Metadata catalog enumeration and remaining archive header properties
are still required. Focused source_update_backend/archive_update_callback/
archive_opening/archive_proxy pass 4/4, 1.70 s; app and affected targets build
on macOS arm64 2026-09-29. No P0-04.1 full gate or acceptance is claimed.

Further internal P0-04.1 work copies native CArc::ReOpen, CArchiveLink::ReOpen,
CAgent::ReOpen/Close and Agent.h KeepModeForNextOpen from that same baseline.
The unchanged LimitedStreams.cpp/.h and its StdAfx.h are vendored dependencies.
The existing native archive wrapper is retained across reopen; a queued file-only
service relists metadata into that wrapper, using the same catalog construction
as initial opening. It does not create another CAgent or duplicate folder tree.
Codecs are NULL at this integration boundary because the external engine owns
codec selection. The empty-link ReOpen→Open2 branch is explicitly unsupported
until full codec/open-owner capability is connected, and remains mandatory.
Nonzero-offset stream/property adjustment also remains required before accepting
the native SFX branch; the current update service rejects such archives.

ArchiveFolderOut.cpp now copies GetPathParts, the original empty-directory helper,
C_CopyFileProgress_to_FolderCallback_MoveArc, CommonUpdateOperation and
CopyFromFile. The CopyFromFile branch retains native WorkDir creation, output
stream ownership, callbacks, close/publish/reopen, proxy reset and path restoration.
CAgent's regular methods replace calls through its not-yet-complete native COM
owner interface. Other AgentOut command branches explicitly return E_NOTIMPL
and remain mandatory under their owning operation packages; this is a partial
module, not a complete ArchiveFolderOut port. POSIX directory inspection uses
upstream CEnumerator::DirEntry_IsDir(..., false); clearing the Windows read-only
attribute is Windows-only. CopyFromFile is now the native COM method on a
conditional IFolderOperations contract. That contract is available only with
an attached update service and native CanUpdate approval; the limited engine
service additionally disables it for nonzero offsets. Unavailable CreateFolder/
CreateFile/Rename/Delete/CopyTo/CopyFrom/SetProperty remain explicit E_NOTIMPL
adapters, not completed ports. The resource owns an injected progress factory
that retains the resource and sets/clears the nonowned CProgressSync pointer
through the source worker join. QtArchiveFolderManager attaches metadata/reopen
and ZIP/7z update services when the initial listing succeeds.

The real-engine source transaction fixture passed (source_update_backend,
0.82 s, macOS arm64 2026-09-29): it invokes CopyFromFile in a bound subfolder,
then observes the updated size through that same folder after native path
restoration, extracts updated bytes and checks the unchanged sibling. Earlier
standalone WorkDir/reopen fixture passed together with archive_opening (2/2,
1.42 s). These checks do not complete the integrated navigation iteration.

The current real panel fixture invokes CPanel::OnOpenItemChanged, its original
CThreadCopyFrom, conditional native IFolderOperations and native transaction
through the resource progress factory. Updated size is observed through the same
panel folder, and real extracted bytes plus the unchanged sibling are checked.
Native cancellation at engineStarted returns E_ABORT after quiescence and leaves
the original archive unchanged. Focused source_update_backend/
archive_update_callback/archive_opening/archive_proxy pass 4/4, 1.96 s on macOS
arm64 2026-09-29; app and affected targets build. MainWindow still uses its former
Qt ArchiveParent stack; remaining opening/extraction/memory/zone and production
navigation consumers are required before acceptance. No extra commit/full gate.

Common/ArchiveExtractCallback.cpp/.h is now copied in full from the baseline,
replacing the former link/path helper-only implementation. Its native algorithms,
stream/file/link/attribute/time/cancel/overwrite and callback control flow are
unchanged; adaptations are canonical includes and project formatting. Exact
ProgressUtils, FilePathAutoRename, CreateCoder.h and MethodId.h are vendored.
HashCalc.h and the previously partial MethodProps.h now carry full native
declarations/inline bodies with adapted includes; their remaining implementation
and active hash consumers remain mandatory in their owners. Common extraction,
native opening stream services and CArc bodies are compiled in the filesystem
library alongside their file/link dependencies, rather than making the common
string/contracts library depend on filesystem operations. Engine-owner targets
explicitly link that dependency. There is still one authoritative implementation.

The full callback exposed metadata debt in QtPanelFolder: IInArchive unavailable
properties must return S_OK/VT_EMPTY, rather than E_NOTIMPL, and MTime must be a
native FILETIME. The adapter now converts the local timestamp presentation emitted
by List.cpp to FILETIME with native precision metadata, preserving seven fractional
digits. There is no reverse CLI timestamp decoder in the borrowed application
module: this conversion is an external-engine data/Qt timezone boundary, not a
replacement of native CArc timestamp or extraction logic. Custom presentation
snapshots retain their former text contract. The native CArc test observes a
31-tick difference between two source timestamps and rejects malformed text to
VT_EMPTY. Metadata absent from the listing, including links/attributes, remains
explicitly unavailable and mandatory for the complete model/engine transport.

Native extraction callback fixtures exercise Unicode paths, source traversal
correction confined to destination, existing-file skip and cancelled progress.
These tests failed before the typed metadata fix and pass afterward. Affected
native_extract_callback/source_update_backend/archive_opening/archive_backend/
compression_preferences/open_callback pass 6/6, 3.33 s; app and affected targets
build on macOS arm64 2026-09-29. Full source FileManager ExtractCallback,
virtual filesystem/memory and actual engine Extract/CopyTo consumers are next
internal tasks. This is not P0-04.1 or P0-06/P0-10.2 acceptance.

## P0-10.1 — integrated progress/callback component (2026-09-29)

This integrated component is accepted in its stated scope. Historical evidence
below is retained; remaining consumers are grouped in the checklist's iteration map.
The reproducible source baseline is
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
[ip7z/7zip](https://github.com/ip7z/7zip).

Copied source functional bodies: FileManager/ProgressDialog2 modal creation,
native worker/Process/result/exception composition, finish and cancel-confirm;
GUI/UpdateCallbackGUI2 Init/operation/move/password state;
FileManager/UpdateCallback100 all eight COM callback interfaces and methods;
FileManager/ExtractCallback::SetExtractErrorMessage. Adapted application files
retain their upstream paths under src/CPP/7zip/UI. Exact unchanged interfaces
Archive/IArchive.h, IPassword.h and ICoder.h are under third_party/7zip/CPP/7zip.
ClassDefs remains the single GUID definition owner. Agent/IFolderArchive.h and
Common/IFileExtractCallback.h retain all interface declarations, with opaque
CCodecs/CArchiveLink declarations replacing heavyweight engine includes:
the current integration executes official 7zz through ArchiveBackend.

Qt adaptations: modal/event/message/default-button services; atomic worker→GUI
lifecycle flags; explicit per-dialog question policy; POSIX startup wait and
join; password widgets constructed/executed/destroyed on the GUI owner thread
through qtcallbackpassword. Current resource language provider uses original
non-Z7_LANG fallback; complete language-file ownership remains P1-14.
Source Before_ArcReopen clears stop only, retaining pause.

ArchiveProgressOperation is the first connected integration owner: MainWindow
Add/drop uses the original native modal worker and callbacks; its old QProgressDialog
owner is removed for this scenario. Remaining progress/journal consumers are
P0-10.2, not omitted completion. The bridge publishes external paths with source
Set_FilePath and polls completion even while paused: blocking a local callback
in CheckStop would otherwise miss independent engine completion. Unknown byte/file
totals remain unknown; a percent-only atomic fallback updates only bar/title.
Accepted empty password is sent as an empty line through backend stdin, preserving
source callback acceptance. Start exceptions return to original Process composition;
after-start exceptions first quiesce the child through cancelAndWait. Scoped
MainWindow ownership policy restores normal routing after modal completion.

processpause supplies the missing external-process service: POSIX SIGSTOP/CONT,
Windows owned thread handles/native CThread suspend/resume with bounded snapshot
rechecks and rollback. It does not contain archive logic. Relevant native contracts:
[SuspendThread](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread),
[Thread32Next](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-thread32next),
[POSIX signals](https://man7.org/linux/man-pages/man7/signal.7.html).
ArchiveBackend owns the process handle/PID lifetime, native controls and reset
after exit; widgets receive no process IDs or command syntax.

Source button pause failure restores sync state; child priority failure restores
parent policy and reports errors. Startup propagates an active parent background
policy; foreground remains normal spawn policy until an explicit live transition.
Already cancelled startup emits no policy request. POSIX stop/continue delivery
is asynchronous; native owned-child observations confirm stop/resume, not a fake
progress-only pause. Windows preserves another owner's suspend counts and retains
partial resume failures for retry; Windows/Linux runtime remains unverified.

Source/interface body comparisons passed for complete UpdateCallback100 and
IFolderArchive/IFileExtractCallback declarations ignoring only formatting/comments
and escaped macro line breaks. GUI2 matches except the reviewed password service.
The three added vendor headers byte-match the baseline; sibling checkout stays
clean. Current inventory: root C++45, mirrored C++44, third_party paths89
(86 prior tracked paths plus these three headers). Required formatting is separate
from functional adaptations.

Focused evidence on macOS arm64, Qt 6.11.2, 2026-09-29: progress_worker/cancel/dialog
3/3 in 11.45 s; update_callback 1/1 in 0.29 s; process_pause 1/1 in 0.97 s.
Final focused 9/9 passed in 15.81 s; later localized archive/consumer checks passed,
including after-start exception cleanup and cancel of Add preserving the original
archive. Full `./scripts/check.sh` passed formatting, app build and **101/101 CTest
in 72.05 s** on macOS arm64/Qt 6.11.2, 2026-09-29; log
`/tmp/sevenzip-p0-10-1-check.log`. Current-build parity passed all 24 reports/26
controls. Reviewed four new normal/compact Close reports: no clipping or unfit text,
correct hidden Pause/Background, error list and default Close; unknown totals empty.
Native confirmation and password behavior/geometry are covered separately.
Windows/Linux runtime and rendered Windows parity remain unverified and mandatory;
all remaining progress/journal consumers and parent P0-10 acceptance stay P0-10.2.

## P0-10.2 — all current progress/message consumers (2026-09-30)

Source baseline for each borrowed module:
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
[ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
The upstream `CPP/7zip/UI/FileManager/MessagesDialog.h/.cpp/.rc` and
`MessagesDialogRes.h` supply `CMessagesDialog::AddMessageDirect`, `AddMessage`,
`OnInit`, `OnSize`, two-column numbered messages and the Close footer. The
functional message-splitting/numbering loops are copied under the original
paths. Win32 `CModalDialog`/`CListView`/DLU services become a narrow Qt
`QDialog`/`CQtProgressListView` presentation; resource IDs and original 456×176
dialog units are retained. Font metrics may enlarge the Close control. The
File menu's local last-operation action reopens the dialog modelessly, so a
completed hash result stays inspectable; this action is a project extension.
The original source's `Create` modal dispatch is not reused for this local
menu workflow. `ResourceString` supplies the Russian `IDS_MESSAGE` caption.

All other current archive operations now use the existing source
`CProgressDialog` via `ArchiveProgressTransport`; filesystem copy/delete and
Split/Combine use it through `QtCallbackProgressTransport`. Their existing
`FileTransferOperation`, `FileDeleteOperation` and `VolumeOperation` workers
remain operation owners; this iteration replaces only the former
`QProgressDialog`/custom journal presentation. The generic transport forwards
confirmed source cancellation to the worker and applies the real native
process-priority service. Those workers have no cooperative pause mechanism,
so the source Pause button is disabled for them. Volume reports actual total
and completed bytes; file deletion reports total/completed items; file
transfer reports accumulated completed bytes, while its total and ETA remain
unknown rather than fabricated. The 7zz process transport retains true child
pause/background controls and the percent-only fallback from P0-10.1.

`CProgressDialog::OnCancel` invokes the narrow transport after the original
YES/NO/CANCEL decision. Unsupported pause is skipped during the source's
temporary cancel-question pause; its normal body and restoration remain.
`ArchiveProgressOperation::ProcessVirt` treats a confirmed stop as E_ABORT even
when synchronous backend completion wins the latch, avoiding a false final
error after cancellation. GUI-thread async consumers initialize source
controls before showing even when their parent window is hidden. Stage handoff
uses `QDialog::done` so a finished stage cannot be mistaken for a new cancel
request. The source close/final message is delivered before command
re-enablement; worker quiescence and partial deletion refresh remain under
the existing operation-state owner. Backend diagnostics and final errors feed
source `CProgressSync`; a single UStringVector retains last-operation messages
for the source MessagesDialog, with no independent QPlainTextEdit owner.

Omitted from this package: source FileManager drag-error caller branches outside
the current workflows, new pause capabilities in filesystem/volume workers,
full language-file loading (P1-14), future P1–P3 operations, rendered Windows
pixel comparison and Windows/Linux runtime checks. Those are not counted as
implemented by the P0-10 parent. Qt 6.11.2/macOS arm64 current-build
normal/compact MessagesDialog JSON reports show both controls fully visible,
ordered and separated, with a fitting Close caption. Focused direct consumers
`progress_cancel`, `checksum`, `ui_operations`, `ui_archive_states`,
`app_lifecycle_window`, `volume_workflow` passed 6/6 in 11.65 s. The final
unsandboxed `./scripts/check.sh` passed formatting, app build and all 137/137
CTest cases on macOS arm64, Qt 6.11.2, 2026-09-30. No Windows/Linux runtime
or rendered Windows reference was available locally. After a documentation-only
scope correction to the progress resource contract, its targeted
`ui_parity_progress` fixture passed unsandboxed (5/5 including dependencies);
the sandboxed native-priority fixture cannot exercise this macOS service.

## P0-12.1 — interactive Ask through the 7zz transport (2026-09-30)

Exact source baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
from [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reviewed `CPP/7zip/UI/GUI/ExtractDialog.cpp/.rc`,
`ExtractGUI.cpp::ExtractGUI`/`CThreadExtracting::ProcessVirt`,
`UI/FileManager/ExtractCallback.cpp::AskOverwrite`/`AskWrite`,
`UI/Common/ArchiveExtractCallback.cpp::CheckExistFile`, and
`UI/Common/Extract.cpp::DecompressArchive`/`Extract`. The existing source
mirrors retain `CExtractDialog` mode ordering/persistence,
`CExtractCallbackImp::AskOverwrite` answer mapping, and the complete original
`CArchiveExtractCallback::CheckExistFile` decision body. P0-12.1 connects the
same `COverwriteDialog` to the user-facing extraction path, sharing the
original native-button-ID→`NOverwriteAnswer` conversion with filesystem
Copy/Move. Qt file metadata is translated to source `CFileInfo` and the
source dialog owns presentation. No upstream file was modified.

The local official `7zz` 26.03 process exposes overwrite interaction only as
English text on stdout plus stdin answers in a disposable conflict probe;
its documented CLI has `-ao{a|s|t|u}` and `-y`, but no structured per-file
callback. Parsing prompt wording or translated paths is unsafe. The necessary
`ArchiveBackend` adapter therefore validates the listed entries, asks for
decisions before extraction, then runs `-aos` for unoccupied entries and
targeted `-aoa`/`-aou` passes for accepted conflicts. `No` leaves an occupied
file untouched; Yes/No to All and AutoRename update per-operation state in the
same order as source `CheckExistFile`. The callback lives in the GUI event
loop; cancellation closes the source dialog and aborts before any pass. Each
subsequent pass keeps password/cancellation state, and an error stops the queue.
This CLI selection/queue is integration code, not claimed as copied source
algorithm or an in-process invocation of `CheckExistFile`.

Differences and remaining mandatory scope: source `CheckExistFile` asks at the
moment of write; this adapter has a preflight-to-write race even after
revalidation. 7zz writes directly, so failure/cancel may leave partial output;
there is no transactional rollback. `ExtractGUI.cpp` and `Common/Extract.cpp`
are not yet source-ported into runtime orchestration; their option precedence,
batch/callback close semantics and relevant omitted branches remain P0-12.3.
The existing non-Ask RenameExisting/Extracted modes, selection validation and
password prompt stay enabled. Filesystem symlink, type and unsafe-path guards
remain in backend validation. P0-12 parent is open until the source integration
scope and final acceptance are checked.

Focused `extraction_options`, `ui_operations`, `extract_dialog`,
`ui_archive_states`, `file_transfer`, `archive_backend` passed 6/6 on macOS
arm64, Qt 6.11.2; real 7z/ZIP Ask, Unicode, NoPaths selected, wrong-password
retry, occupied auto-rename target, unavailable dialog and cancel are covered.
Unsandboxed `./scripts/check.sh` passed formatting, app build and 137/137
CTest cases; after a localized cleanup/fixture edit,
`extraction_options` was rebuilt and passed again. Windows/Linux runtime and
rendered Windows parity are unverified.

## P0-12.2 — root FM extraction through native callback (2026-09-30)

Exact original source is
[`ip7z/7zip` commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17):
`CPP/7zip/UI/FileManager/PanelCopy.cpp::CPanel::CopyTo` and
`CPanelCopyThread::ProcessVirt`, `CPP/7zip/UI/Agent/Agent.cpp::CAgentFolder::Extract`
and `GetRealIndices`, `CPP/7zip/UI/Common/ArchiveExtractCallback.cpp::CheckExistFile`,
`CPP/7zip/UI/FileManager/ExtractCallback.cpp::AskOverwrite` and
`CPP/7zip/UI/GUI/ExtractGUI.cpp::ExtractGUI`. The first five source bodies were
already copied under matching `src/` paths. This iteration connects the root
file-manager Extract command to those bodies; `PanelCopy.cpp` retains its source
thread/callback/error flow, with a narrow `CCopyToOptions::extractMode` branch
that supplies the source `IArchiveFolder::Extract` path/overwrite parameters and
uses the original Extract progress title. The external `7zz` still supplies
archive bytes through `QtArchiveFileResource::AttachExtractService` and
`ArchiveBackend::extractSourceEntry` (file-only CLI stdout staging), not an
in-process codec. No upstream checkout file was changed; notices and license
remain in the mirrored modules and `third_party/7zip/License.txt`.

`MainWindow::extractWithSource` selects this route only for a listed, root,
unencrypted file-backed archive and FullPaths/Ask. It passes source panel indices, or all
root indices, through the native folder proxy; output validation and symlink/
unsafe-name guards remain in `ArchiveBackend`. The source callback now asks at
`GetStream` before each file, so the P0-12.1 preflight-to-write interval is
removed for this route. Source progress owns the modal lifecycle and the
existing backend completion owner ignores each staged stream completion until
the native worker joins. The result then restores command state and journal.
Real UI tests assert Yes, selected No, AutoRename and Cancel, source dialog
metadata, preserved existing bytes and no `ArchiveBackend::overwriteRequired`
preflight signal. Controlled backend consumers retain their async transport.

This is a partial P0-12 port. Nested or encrypted archives, NoPaths and non-Ask
modes still use the P0-12.1 backend path. `ExtractGUI.cpp::ExtractGUI`/
`CThreadExtracting::ProcessVirt` and `Common/Extract.cpp::DecompressArchive`/
`Extract` batch orchestration have been reviewed but not copied or wired;
option precedence, multi-archive statistics, callback close/error composition,
volume handling and related dependencies remain P0-12.4. Source callback writes
directly; failure/cancel can leave partial output. Destination changes after
validation remain a race, and rendered Windows plus Windows/Linux runtime parity
are unverified.

Focused `ui_operations`, `source_extract_backend` and `extraction_options`
passed 3/3; after extending the UI decisions fixture, `ui_operations` passed
again. The full unsandboxed `./scripts/check.sh` passed C++ format, app build
and all 137 CTest cases on macOS arm64 with Qt 6.11.2 on 2026-09-30.

## P0-12.3 — root NoPaths through source FM callback (2026-09-30)

Exact baseline:
[`ip7z/7zip` commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Common/ArchiveExtractCallback.cpp::GetStream` path-mode switch,
`CPP/7zip/UI/Agent/Agent.cpp::CAgentFolder::Extract`, and
`CPP/7zip/UI/FileManager/PanelCopy.cpp::CPanelCopyThread::ProcessVirt`.
These source bodies remain in their existing mirrored paths. The Qt command
adapter now passes the dialog's NoPaths mode into the already connected native
`IArchiveFolder::Extract` route for a listed, unencrypted root archive. No
algorithm was rewritten and no upstream file modified. `ArchiveBackend` still
validates all selected entries, destination ancestry, link/type hazards and
flattened name collisions before the native worker starts. A real ZIP/Unicode
selected-folder test confirms the flattened file, source Ask at write time,
replacement bytes and absence of the P0-12.1 preflight signal. The source
callback retains an explicit archive directory entry as an empty folder in
NoPaths mode; this comes from its own path-mode body and is recorded as a
visible difference from the earlier CLI `e` transport.

Two related paths were investigated and remain P0-12.4. For an encrypted
archive, the file-only `7zz -so` adapter redirects stdout as binary and only
parses stderr for prompts; a real UI source-extract test timed out waiting for
password handling. The current transport has no verified structured prompt.
The source callback can also open or replace the output before password
verification. Encrypted user extraction therefore stays on the existing
preflight/test path until an authenticated stream contract prevents a hang and
wrong-password data loss. For selection inside a bound archive subfolder,
`CAgentFolder::Extract` passes current-folder path parts to the source callback,
which strips that prefix even with FullPaths; a real selected-item test found
the expected full destination conflict was not asked. The established backend
route is retained until the source root/batch owner supplies the correct path
base. `GUI/ExtractGUI.cpp` and `Common/Extract.cpp` remain mandatory source
orchestration, along with nested/encrypted acceptance. No unverified behavior
is marked complete.

Focused `ui_operations`, `archive_selection_runtime`, `ui_archive_states`
and `extraction_options` passed 4/4. The full unsandboxed
`./scripts/check.sh` passed formatting, app build and all 137 CTest cases on
macOS arm64 with Qt 6.11.2 on 2026-09-30. Windows/Linux runtime and rendered
Windows parity remain unverified.

## P0-12.4 — encrypted root and current archive folder (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Agent/Agent.cpp::CAgentFolder::Extract`,
`CPP/7zip/UI/Common/ArchiveExtractCallback.cpp::GetStream`/`CheckExistFile`,
`CPP/7zip/UI/FileManager/PanelCopy.cpp::CPanel::CopyTo` and
`ExtractCallback.cpp::CryptoGetTextPassword`/`AskOverwrite`. Their previously
copied bodies and source control flow stay authoritative. The Qt command adapter
now uses them for an encrypted root archive and for the current bound archive
folder. The source folder deliberately strips its current prefix for FullPaths:
the target is relative to the viewed folder, matching its original path policy.

`QtArchiveFileResource::ExtractItems` is only the external engine service. For
an encrypted archive it requests the source callback password, runs the already
listed archive through `ArchiveBackend::test`, then calls source `GetStream`.
This ordering prevents an invalid password from opening or replacing an
existing target. The same password is supplied to every file-only `7zz -so`
request before the process starts, avoiding the unhandled stdout prompt found
in P0-12.3. This is a narrow process-boundary adaptation, not a replacement
extraction algorithm. The backend still owns process arguments, validation,
path/symlink checks and cancellation. Wrong password returns through source
progress/error UX; the user can retry extraction with a new password. As with
the upstream direct writer, a later failure/cancel can leave already extracted
files; no atomic rollback is promised.

Real UI fixtures cover encrypted 7z and ZIP with Unicode/space names and
occupied output: wrong password preserves old bytes and never reaches overwrite;
the next correct attempt invokes `COverwriteDialog` and writes new bytes. A
bound ZIP subfolder fixture confirms source Ask at its relative destination and
no backend preflight. Native folder navigation, existing root NoPaths and
backend consumers remain separately covered. Local macOS arm64/Qt 6.11.2 checks
are recorded in the P0-12.4 checklist row: focused 5/5, five repeated
`ui_state`/`ui_operations` checks, and an unsandboxed full `./scripts/check.sh`
with 137/137 CTest cases. Windows/Linux runtime and rendered
Windows appearance are unverified.

At the P0-12.4 commit this was still a partial P0-12 port. Nested archive engine resources and
`CPP/7zip/UI/GUI/ExtractGUI.cpp::ExtractGUI`/
`CPP/7zip/UI/Common/Extract.cpp::Extract`/`DecompressArchive` multi-archive
orchestration remained P0-12.5 and were excluded from that commit. Their
integrated port is recorded below. The existing P0-12.1 backend fallback
continues for unsupported routes and non-Ask modes.

## P0-12.5 — source batch extraction and nested archives (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Common/Extract.cpp/.h::Extract`/`DecompressArchive`,
`CPP/7zip/UI/GUI/ExtractGUI.cpp/.h::ExtractGUI`/`CThreadExtracting`,
`CPP/7zip/UI/Common/OpenArchive.cpp/.h::CArchiveLink::Open2`/`Open3`,
`CPP/7zip/UI/GUI/resource2.h` and existing `ExtractDialog.cpp/.h`.
The complete `Extract.cpp` archive loop and file selection/decompression bodies
are copied into their mirrored paths; `ExtractGUI.cpp` retains its thread,
options, result and progress sequence. Include paths, Qt dialog/message calls,
language resource lookup and the new file-only opener/progress arguments are
adapted. Required `clang-format` changes are mechanical and are not upstream
functional changes. `ExtractDialog::SetInitialOptions` maps upstream forced
options and persisted defaults to the existing Qt combo controls and password.
`resource2.h` supplies source IDs for archive-count text.

The full upstream `CArchiveLink::Open2` codec detection and recursive stream
opener cannot run with the external `7zz` engine. Its mirrored body is a narrow
delegation to `IQtArchiveOpenService`; `Open3` keeps the source `Open2` then
`Open_Finished` ordering. `QtArchiveBatchOpen` lists one file at a time through
`QtArchiveFolderManager`, creates source `CArc` metadata and preserves the
source `Extract` loop, wildcard censor, archive result callback, overwrite
state, cancellation, password and close. `QtArchiveBatchExtract` only converts
confirmed Qt options and archive paths to source arguments. The whole-root
`MainWindow` Ask consumer uses this route after the existing destination guard;
selected/current-folder Ask keeps `CPanel::CopyTo`. Nested resources now use
the same split. Backend signals for each `7zz` child remain hidden from the
outer window until the source modal worker joins. Source callback errors are
not reported as success when the upstream return HRESULT is `S_OK`.

For split `.001` archives the external `7zz -slt` listing reports `Volumes` and
`Total Physical Size`. The narrow adapter records those part paths and sizes in
source `CArchiveLink::VolumePaths`/`VolumesSize` and supplies the combined
logical stream length to `CArc::ReadBasicProps`. `-t7z` is omitted on the
file-only stream request so `7zz` can open the outer Split handler. The source
loop then skips later volumes and maintains total progress. The current adapter
supports numeric `.001` naming and rejects unsupported naming or missing parts;
the full in-process codec/volume opener and broader format handling remain
P3-05 migration debt, not a claimed port. Non-Ask extraction and additional
Extract modes/metadata remain P2-07.

Real macOS fixtures cover two ZIP archives with a shared Unicode/space path
and source overwrite Yes/Cancel, a split multi-volume 7z with one list/open and
exact extracted bytes, nested whole and selected archive Ask with source
overwrite, encrypted 7z/ZIP wrong-password preservation then correct retry,
and forced-versus-saved dialog modes. These are structural and behavioral
checks on macOS arm64/Qt 6.11.2; they do not establish rendered Windows parity
or Windows/Linux runtime behavior. Focused `extract_dialog`,
`source_extract_backend`, `nested_archive` and `ui_operations` passed 4/4;
the required full `./scripts/check.sh` passed formatting, app build and
137/137 CTest cases on 2026-09-30.

## P0-11 — filesystem overwrite decisions (2026-09-30)

Exact source baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
from [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reviewed `CPP/7zip/UI/FileManager/OverwriteDialog.cpp/.h/.rc`,
`FSFolderCopy.cpp::CopyFile_Ask`/`CopyFolder`,
`ExtractCallback.cpp::AskOverwrite`/`AskWrite`,
`UpdateCallback100.cpp`, and `CPP/7zip/Common/FilePathAutoRename.cpp/.h`.
The existing mirrored `COverwriteDialog` source `CFileInfo`, `ReduceString`,
`SetFileInfoControl`, `OnInit`, `OnButtonClicked` and original button IDs remain
the sole presentation implementation. Its native icon/control services are Qt
adapters; P0-11 adds directory-specific wording, since a whole-folder collision
now uses this prompt. `FilePathAutoRename.cpp` remains an unchanged, single
vendored implementation. The existing mirrored `CExtractCallbackImp::AskOverwrite`
switch supplies the exact Win32 button-ID to `NOverwriteAnswer` mapping reused
by the filesystem GUI adapter. The per-operation Ask/Skip/Overwrite/Rename
state and rename path follow its `AskWrite` decision branch; the worker's
transaction stages output before publication and keeps displaced entries as
backup until the complete source set succeeds. The backup/rollback, Qt
condition-variable request and worker ownership are local integration code,
not claimed as copied 7-Zip bodies.

`FileTransferOperation` now sends immutable source/destination paths to the
GUI; its worker waits without touching widgets. Cancel wakes the waiter,
closes an active source dialog and rolls back staged/published work. The same
source decisions cover files and whole directories. Whole-directory replacement
is an explicit transaction adaptation; upstream `CopyFolder` can merge a
directory recursively through `AskWrite` for each contained file. This
different merge workflow remains P1-02 along with native MoveFile/CopyTo,
cross-volume moves, and any nested source callback not used by current
stage-copy targets. Archive extraction Ask and `7zz` protocol remain P0-12;
this work does not mark them implemented. Symlinks and mixed file/directory
types remain rejected. A race that occupies an intended new/renamed target
before publication fails instead of replacing that new entry. Failed rollback
must be reported with its surviving backup; that hardening remains under the
filesystem transaction owner.

`file_transfer` covers Yes/No/YesAll/NoAll/AutoRename/Cancel, repeated
conflicts, occupied rename target, Unicode, file/directory conflict, byte
progress and replacement rollback. `ui_operations` covers F5/F6 dialog
integration, AutoRename, No and cancellation. `extract_prompt` and
`ui_parity_overwrite` cover source controls and file/directory text at 700×440
and 560×440; current-build folder JSON was inspected. Focused 4/4 passed on
macOS arm64, Qt 6.11.2. Unsandboxed `./scripts/check.sh` passed formatting,
app build and all 137/137 CTest cases on 2026-09-30.
Windows/Linux runtime and rendered Windows pixel parity are unverified.

## P0-01 — accepted source inventory

P0-01 is accepted as a documentation inventory, not functional port completion.
The [archived module inventory](history/p0-01-source-inventory.md) preserves all
21 original audit records and module tables verbatim: paths, symbols, copied/
adapted/omitted portions, dependencies, owners, decisions, project baselines and checks.
The exact retrievable source baseline is
[ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).

The 2026-09-28 snapshot covers **112 unique .cpp paths: 109 UI plus three
supporting modules**. Thirty mirrored modules (16 in .1 and 14 in .2) were reviewed
against upstream bodies and dependencies; all 82 then-missing UI modules were
inspected and assigned source-port decisions/feature owners in .3. FileManager,
Agent, GUI and Common coverage includes callers, build dependencies and platform
constraints. No missing mandatory branch was mechanically declared N/A. Inactive
comment-only/definition/build files were identified individually. Headers/resources
remain attached to their module records; familiar paths alone do not prove a port.

Historical verification: macOS 2026-09-28, docs-only path/coverage/diff checks;
33/33 vendored source/header/license files byte-matched the pinned baseline.
Inventory commits: .1 `cfd02a5`, .2 `d03de90`, final .3 coverage `415513c`,
.4 reconciliation `36fad59`. No app build, CTest or rendered/runtime verification
was performed for this inventory. Intermediate coverage counts are retained only
in the archive and do not describe today's checkout.

Later P0-02–P3 source records update their own modules and omissions; consult those
accepted results before acting on an archived finding. The inventory establishes
coverage and provenance, not completion of compression/engine, UI workflows or
Windows/Linux runtime. This documentation consolidation changes no parent count
or functional acceptance.

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
Это evidence первоначального dialog adapter; основной table/icon hover
подключён позднее в § P0-07.1 выше.

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
Это исторический partial scope; расширение страницы и текущих consumers
описано в § P0-07.1 выше, AlternativeSelection остаётся P0-07.2.

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

## P1-04.1 — незашифрованные архивные CreateFolder/Rename/Delete (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Agent/AgentOut.cpp::CAgent::DeleteItems/CreateFolder/RenameItem`,
`CPP/7zip/UI/Agent/ArchiveFolderOut.cpp::CAgentFolder::Delete/CreateFolder/Rename`
и `CommonUpdateOperation`, `CPP/7zip/UI/FileManager/PanelOperations.cpp::
CPanel::CreateFolder/OnEndLabelEdit/DeleteItems` и `CThreadFolderOperations`.
Сохранены исходные пары `CUpdatePair2`, real-index expansion (включая
содержимое папки и альтернативные потоки), `DeleteOperation`, проверка
совпадения имени папки, перенос имени потомков, `CArchiveUpdateCallback`
и порядок temp → close → MoveToOriginal → ReOpen → восстановление папки.
Последний FM owner был перенесён в P1-03; в этой итерации подключены его
архивные consumers и source progress/cancel/confirmation.

`qtarchivesourceupdate.cpp` адаптирует только внешний `IOutArchive` transport:
проверяет исходный callback-план и имена через source `Correct_FsPath`, выводит
Delete/Rename/CreateFolder и передаёт их `ArchiveBackend::updateSourceMutation`.
Backend копирует исходный архив в отдельный output и применяет `7zz` с
`-spd`, не публикуя копию; публикация и откат остаются в оригинальном
`CAgentFolder::CommonUpdateOperation`. Empty directory создаётся во временном
source root. Original ZIP/7z writer исполнялся бы in-process; такой внешний
file-only adapter обязателен для текущего engine boundary. `SetAsDir` и
`GetCurUtc_FiTime` заменяют Win32-only поля `Attrib`/`FILETIME` на переносимые
исходные типы. Обязательный clang-format меняет только оформление.

Qt F7, F2 inline и Delete/Shift+Delete теперь вызывают source CPanel;
после native ReOpen список перерисовывается без второго ReOpen, сохраняются
выбор/фокус и путь папки. Flat editor показывает source basename, а не
отображаемый prefix. Существующий `ArchiveBackend::remove` пока нужен
архивному Move/transfer P1-01; `add`/CopyFrom, comment/SetProperty,
зашифрованные и вложенные архивы, общий SetProperties и handler writer
capabilities остаются открытыми P1-04.2/.3, P1-05/P1-09. Это частичный
порт файлов AgentOut/ArchiveFolderOut; зеркальные имена не означают
готовность остальных функций. Original notices сохранены, лицензия:
`third_party/7zip/License.txt`.

## P1-04.2 — пароль при source archive mutation (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CThreadFolderOperations::DoOperation`:
возвращены исходные строки наследования `UsePassword`/`Password` из
`_parentFolders.Back()` после `CUpdateCallback100Imp::Init`. Остальные
исходные тела `CAgentFolder::CommonUpdateOperation` и
`CUpdateCallback100Imp::CryptoGetTextPassword2` сохраняются от P1-04.1.
Отклонение — `GetParentFolders()` вместо прямого доступа к частному полю Qt
панели; clang-format меняет оформление.

Новый file-only adapter передаёт пароль из `ICryptoGetTextPassword2` в
`ArchiveBackend::updateSourceMutation` и stdin 7zz. При повторном открытии
оригинальный `COpenCallbackImp::ReOpenCallback` несёт update callback;
`QtArchiveFileResource` извлекает из него уже определённый пароль до
`openFolder`, сохраняя исходный порядок publish → ReOpen. Если пароль не
определён, прежний callback-путь запрашивает его. Win32/in-process 7-Zip
writer здесь заменён внешним file-only сервисом; 7z header encryption и ZIP
AES поддержаны 7zz, новый пустой ZIP folder entry сам по себе не содержит
шифруемых данных. Add/Edit/Move, nested updates и comment/SetProperty этим
частичным портом не покрыты. Source notices и лицензия сохранены в
`third_party/7zip/License.txt`.

## P1-17 — source ZIP writer and GUI consumer (details, 2026-10-02)

Current GUI checkpoint supersedes the historical disabled/next statements below:
original `PanelOperations.cpp::CPanel::ChangeComment` now owns focused real-index
selection, parent/read-only/hash guards, native comment property, item title,
shared `ComboDialog` and refresh. The original `ArchiveFolderOut::SetProperty`
and `CAgent::CommentItem` functional bodies remain the property/update owners.
Qt replaces Win32 list/control calls and runs SetProperty on the existing source
`CThreadFolderOperations` worker with `CUpdateCallback100Imp`, rather than the
original NULL-progress synchronous GUI call. This is the responsiveness adapter;
the source ZIP update plan, callbacks, work-dir publication and reopen are reused.

The narrow `IQtFolderPropertyCapabilities` interface exposes actual comment
support to Panel without coupling shared presentation targets to concrete Agent.
The CApp command route binds resource `IDM_COMMENT` and original Ctrl+Z shortcut.
Comment is available only for a focused real item with the validated native ZIP
writer and writable ancestor chain. `ComboDialog::BindLanguage` preserves a
caller-supplied dynamic title when titleID is zero: source language ID0 means
7-Zip, not an absent string. Prompt and editable value still use the shared view;
raw whitespace and empty comments remain valid.

The caller retains original Set Comment Error/HRESULT presentation. Qt macOS
message boxes omit window captions, so the original error caption is shown as
the message heading with the HRESULT detail below. Cancellation is quiet.
Native progress uses original Sync pause/stop callbacks; the local transport
accepts native pause requests without attempting to suspend a nonexistent child
process. A subsequent process-backed metadata reopen uses the existing controls.

Current GUI/callback changes built37772 exit0; wrapper22135 exit0,5/5 suites
PASS15.95s on macOS arm64 Qt6.11.2 (native_comment_ui/native_comment/
codec_commands_ui/combo_dialog/archive_progress). Actual EN/RU plain/AES command
success, editable cancellation, UTF-8 oversized-comment writer refusal/error,
focus, readback, bytes and extraction checked. Four current Combo JSON reports
cover360×150/260×150 EN/RU. Nested publication and native paused cancellation
tests are being added; no full gate or acceptance yet. Windows/Linux runtime and
rendered Windows appearance remain unverified.

Latest backend checkpoint (supersedes the prerequisite-only next statements
below): copied original `CAgent::CommentItem` body from
`CPP/7zip/UI/Agent/AgentOut.cpp`, original ZIP/VT_BSTR `CAgentFolder::SetProperty`
guard/body from `ArchiveFolderOut.cpp` and its `AGENT_OP_Comment` dispatch. No
comment plan algorithm rewritten; existing source CArchiveUpdateCallback owns
CommentIndex/Comment and metadata forwarding. The process-backed Agent's
GetTypeOfArc adapter resolves its retained detected ArchiveType because its
synthetic CArc FormatIndex cannot index the native/global catalog.

ArchiveCodecCatalog retains verified module metadata, flags, signatures and
extensions, replacing only the ZIP input/output factory hooks with the copied
source CHandler (`LibIndex=-1`). Source OpenArchive performs detection/opening
and per-instance SetCompressCodecsInfo as before; the immutable catalog/handler
lease owns external module lifetime. Other factories remain module-backed.
ArchiveBackend::updateNativeComment invokes that original handler IOutArchive;
the Qt resource dispatches only the source callback's explicit comment plan
to this route. Other update/extract/item-list transports remain explicit process
integration. No handwritten ZIP record editing or CLI comment workaround.

The source folder proxy currently uses CLI item order. Before native per-item
properties/comment updates, QtArchiveFileResource verifies count and each Path
in the same order (NFC comparison only at the Qt Unicode boundary). A mismatch
retains process properties and disables comment capability; native indexes
are never guessed. Once validated, original native properties preserve comment
and writer metadata. The source work-dir transaction closes all native readers
before publication; reopen now obtains a fresh native reader and validates order
again. The existing progress/open/password projection was extracted unchanged
to qtarchivenativeopenprogress.cpp/.h for initial open and reopen.

Actual native_comment tests exercise source IFolderOperations::SetProperty through
CAgent/CommonUpdate/ArchiveBackend/source ZIP writer, transaction publication and
reopen, Unicode comment read-back and empty-comment clearing. Ordinary/AES256
archives each cover success, original writer refusal of65536-byte comments, and
callback E_ABORT; original input/archive unchanged on non-success and no owned
staging remnants. Successful archives extract exact binary bytes through 7zz.
Final affected build90336 exit0; focused wrapper76216 exit0:5/5 suites PASS2.82s
(native_comment/native_zip_writer/native_folder_opening/codec_update/
owned_temp_publication), macOS arm64 Qt6.11.2, 2026-10-02. Full all-target build
36443 subsequently failed a backend-only link: its target does not contain Agent,
so the source g_ExternalCodecs fallback storage could not remain in Agent.cpp.
The single empty ABI storage definition now lives in the narrow standalone-library
integration unit `src/archiveexternalcodecs.cpp`, referenced by sevenzip_zip.
No global catalog is populated; per-handler leases remain authoritative. This
relocation is a build boundary adaptation of the original Agent global, not a
new codec algorithm. Agent.cpp no longer contains the duplicate definition.
Repeated all-target build35574 exited0 (only existing Qt date deprecations).
No full gate/GUI acceptance/commit.
Next: actual CPanel::ChangeComment command/capability/dialog/error/refresh and
UI geometry, broader shared-property/open/update regressions and full gate.

Subsequent shared regression checkpoint: wrapper43406 found20/22 passing, with
SIGSEGV in archive_opening and codec_commands_ui parser cases. LLDB20670 showed
the new local hasCommentWriter guard indexing Formats with source parser
FormatIndex=-1 (unsigned4294967295). The native parser algorithm was unchanged;
the integration capability guard now checks the signed index and catalog bounds
before lookup. Rebuilt all targets63060 exit0; repeated the same expanded wrapper
67928 exit0:22/22 suites PASS14.39s, macOS arm64 Qt6.11.2, 2026-10-02.
Current baseline verifier e684d2 PASS351 exact/163 adapted/3 local; diff check
passed. No live build/test/debug handles. This supersedes earlier live handles
and failed checkpoint results; GUI/full gate/iteration acceptance remain pending.

Baseline: https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Copied complete original
`CPP/7zip/Archive/Zip/ZipHandler.cpp`, `ZipHandlerOut.cpp`, `ZipIn.cpp`,
`ZipItem.cpp`, `ZipOut.cpp`, `ZipUpdate.cpp`, `ZipAddCommon.cpp` and the
codec/crypto/stream dependency closure into `third_party/7zip/` with their
original paths. Dependency units include original LZMA/XZ/PPMd encoders and
decoders, ZIP/AES crypto and C support. No compression or ZIP algorithm rewritten.
Registration (`ZipRegister.cpp`) is not copied/linked: the integration
explicitly constructs the source handler behind ArchiveBackend, while the existing
leased module catalog continues to own format discovery. This is partial bundle
integration, not a replacement of the entire upstream DLL/export layer.

129 newly borrowed files are recorded individually in upstream-baseline.json:
127 exact plus two adapted headers under `src/CPP/7zip/Archive/`:
`Common/HandlerOut.h` and `Zip/ZipCompressionMode.h`. Their only functional
adaptation is canonical include ownership to reuse the already adapted
`Common/MethodProps.h`; notices and project formatting are accounted separately.
The single original `g_ExternalCodecs` fallback definition is provided in local
`src/archiveexternalcodecs.cpp` for the source handlers' external-codec ABI. It remains empty;
future consumers must pass/retain their immutable catalog per handler, rather
than mutate shared global codec state. Shared dependencies have one authoritative
source path; the isolated static target may compile existing stream/time modules
also needed by other static libraries.

`cmake/native_zip.cmake` builds these original sources as `sevenzip_zip`.
`native_zip_writer_tests` constructs the source `NArchive::NZip::CHandler`, uses
the already copied `CArchiveUpdateCallback` comment/no-data update plan, and
verifies a Unicode/spaces comment, two entries, unchanged Path/Size/CRC/encryption,
and exact binary extraction through the independent installed official `7zz`.
Ordinary and AES256 ZIP each cover success, output write failure, and callback
cancellation; original archive/input bytes remain unchanged in all cases.
This is a prerequisite check, not the source CAgent::CommentItem or GUI consumer.

Checks: isolated source-library build52641 exit0; final native writer test build
85154 exit0; wrapper CTest307021 exit0, 1/1 suite PASS0.51s (six data scenarios),
macOS arm64/Qt6.11.2, 2026-10-02. Intermediate compile failures were exported
adapted-header include paths and a test using the wrong FileStreams API; link
failure identified the omitted original global fallback. No live test/build
handles, full gate, acceptance or iteration commit yet. Command remains disabled.
Baseline verifier e39244 passed351 exact/163 adapted/3 local paths across the
single pinned revision; git diff --check passed after documentation updates.
Next: backend-owned native handler/lease integration, actual source Agent comment
and SetProperty/ChangeComment GUI, publication/reopen/refresh/failure/cancel and
full checks. Windows/Linux runtime and rendered Windows parity remain unverified.

## P1-04.3 — prerequisite для ZIP item comment (2026-09-30; перенесён в P1-17)

Точная база исследования: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CPanel::ChangeComment`,
`CPP/7zip/UI/Agent/ArchiveFolderOut.cpp::CAgentFolder::SetProperty` и
`CommonUpdateOperation(AGENT_OP_Comment)`, `AgentOut.cpp::CAgent::CommentItem`,
`CPP/7zip/Archive/Zip/ZipHandlerOut.cpp::CHandler::UpdateItems`.
Исходный FM получает `kpidComment`, открывает ComboDialog, затем передаёт
`VT_BSTR` в SetProperty; Agent помечает `NewProps` одного real index и отдаёт
`CommentIndex`/`Comment` исходному ZIP writer. Другие форматы SetProperty
отклоняет.

Установленный `7zz` 26.03 на macOS arm64 имеет CLI команды a/d/e/h/i/l/rn/
t/u/x и перечисленные в `7zz -h` ключи без операции изменения ZIP item
comment. Текущий проект не содержит исходный `ZipHandlerOut.cpp` и его
in-process writer dependencies; file-only `QtArchiveFileResource::Update`
не может выполнить `CommentIndex`/`Comment`. Не заменяем ZIP-структуры своей
реализацией и не называем операцию готовой. Prerequisite: портировать ZIP
`IOutArchive` handler и необходимые исходные зависимости в ArchiveBackend,
сохранив транзакционную публикацию, затем подключить исходные Agent/FM bodies
и Qt диалог, success/failure/cancel, Unicode и сохранность данных/пароля.
Никакие исходные bodies в этом исследовании не скопированы. Это историческое
исследование P1-04.3 теперь принадлежит самостоятельному blocked пакету P1-17;
parent P1-04 принят по ранее проверенным .1/.2 и не включает ZIP comment.

## P1-11.1 — корень и смонтированные тома (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/RootFolder.cpp/.h::CRootFolder::Init/LoadItems/
BindToFolder/GetProperty`, `FSDrives.cpp/.h::CFSDrives::LoadItems/GetProperty/
BindToFolder`, `FSFolder.cpp::CFSFolder::Init`. Исходные body структуры
`CRootFolder`/`CFSDrives` и перечисление/свойства Windows-дисков сохранены из
P0-04/P0-05. В этой итерации non-Windows root получает второй виртуальный
элемент «Тома», который привязывает `CFSDrives`; исходный «Компьютер» → `/`
сохранён. Это намеренная platform adaptation: 7zFM Win32 показывает drives,
а прежний non-Windows branch исходника только `/`.

`qtvolumemounts.cpp` — узкий platform service на `QStorageInfo`, передающий
путь, отображаемое имя, файловую систему и доступные размеры в source
`CDriveInfo`; корень `/` остаётся доступен, даже если ОС не вернула его в
перечне. `CFSDrives::BindToFolderSpec` заново проверяет присутствие mount и
доступность каталога до source `CFSFolder::Init`, чтобы исчезнувший или
нечитаемый том не открыл устаревший путь. Инъекция provider на экземпляр
нужна для локальных deterministic fixtures, без глобального состояния.
`MainWindow::renderRootFolder` представляет два исходных folder type в одном
Qt списке, показывает source свойства томов и блокирует mutation commands.

Не перенесены Windows `CNetFolder`, browse navigation и device-image
`CopyFileSpec/CopyTo` из `FSDrives.cpp`: network owner P1-11.3, Browse entry
points owner P1-11.2, image
операции — P2-05/P3-05. Системный `QStorageInfo` на macOS не доказывает
Windows/Linux mount behavior; фактические сети/credentials не тестируются.
Оригинальные notices сохранены, лицензия — `third_party/7zip/License.txt`.

## P1-11.2 — source Browse entry points и picker consumers (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/BrowseDialog.cpp/.h::MyBrowseForFolder`,
`CBrowseInfo::BrowseForFile`, уже перенесённые `Dlg_CreateFolder` и
`CorrectFsPath`; consumers `CopyDialog.cpp::OnButtonSetPath`,
`SplitDialog.cpp::OnButtonSetPath`, GUI `ExtractDialog.cpp::OnButtonSetPath`.
Сохранены имена entry points, очистка результата до вызова folder picker,
разделение исходного FilePath на каталог/имя, возврат прежнего значения при
Cancel и индекс выбранного фильтра. Qt `QFileDialog` заменяет Win32
`NShell::BrowseForFolder`/`CommonDlg_BrowseForFile`; его native OS picker
управляет контролами, а `CBrowseInfo` остаётся владельцем пути и результата.
Текущие CreateArchive и MainWindow Open Archive используют те же входы.

Существующие статические Qt filters передаются presentation-адаптеру;
исходное построение filters из handler registry остаётся P2-01. Win32
`CommDlgExtendedError`/super-path fallback не переносится на Qt/Unix picker;
для Windows длинных путей это непроверенная ветка. Внутренний
`CBrowseDialog` tree/list/Reload/OnItemEnter/FinishOnOK (USE_MY_BROWSE_DIALOG)
не строится: в текущем upstream desktop используется системный picker,
а визуальный custom fallback для super/device paths остаётся обязательным
при подтверждении такого Windows workflow. `BrowseDialog2` Delete Temporary
Files остаётся P1-15. Original notices сохранены, лицензия —
`third_party/7zip/License.txt`.

## P1-11.3 — сетевой namespace (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/NetFolder.cpp/.h::CNetFolder::Init/LoadItems/
GetProperty/BindToFolder/BindToParentFolder/GetFolderProperty`,
`CPP/Windows/Net.cpp/.h::NWindows::NNet` и
`CPP/7zip/UI/FileManager/RootFolder.cpp::CRootFolder::BindToFolder`.
`CPP/Windows/Net.cpp/.h` скопированы без функциональных изменений в
`third_party/7zip/CPP/Windows/`, собираются только на Windows с `mpr`.
Windows тела `CNetFolder` сохранены под `_WIN32`, включая исходное перечисление
ресурсов, свойства, binding и родительскую иерархию; Qt presentation
использует тот же `IFolderFolder`. Исходные имена, границы модуля и порядок
операций сохранены; clang-format изменил только оформление.

На macOS/Linux Win32 `WNet*` не доступен. Узкий `QStorageInfo` service помечает
известные сетевые файловые системы; POSIX ветка `CNetFolder` сохраняет
source-интерфейс и свойства `Name`/`Provider`, выдаёт только уже
смонтированные ресурсы и передаёт открытие в source `CFSFolder`. Перед
binding повторно проверяются mount, тип пути и читаемость: stale/исчезнувший
или недоступный ресурс не открывается. `CRootFolder` добавляет «Сеть» после
«Тома», а `CPanel::OpenParentFolder` остаётся owner возврата; Qt command
availability и Back разрешены для дочернего namespace. Это намеренное
отклонение от Windows browse: обнаружение новых серверов, подключение,
credentials и remote discovery не заявлены на POSIX; тесты не обращаются к
сети. Иконки/детали provider зависят от Qt/ОС, Windows rendered reference и
runtime Windows/Linux не проверены. Device-image части `FSDrives` остаются
P2-05/P3-05. Source notices сохранены, лицензия —
`third_party/7zip/License.txt`.

## P3-01 — воспроизводимый baseline и review обновления (2026-09-30)

Полный текущий реестр `docs/upstream-baseline.json` закрепляет repository
`https://github.com/ip7z/7zip.git` и commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` для каждого указанного пути;
`overrides` позволяет записывать отдельные полные ревизии. 192 неизменённых
файла `third_party/7zip/` совпали побайтово, включая `License.txt` из
`DOC/License.txt`; 135 адаптированных `src/CPP/` имеют соответствующий
исходный путь; три локальных заголовка PanelSelect/PanelSort/PanelSplitFile
такого пути в исходном commit не имеют. Повторных authoritative source paths
между группами нет. Старые количественные snapshots в audit сохраняются как
историческое evidence; актуальный счёт берётся из manifest, не из числа строк
упоминаний в тексте.

`scripts/upstream_baseline.py verify` повторно извлёк все исходные bytes через
`git show <full-commit>:<path>` из независимого временного Git snapshot,
скопированного без hardlinks, без старого sibling path; missing/stale entries,
двойной owner и несовпадение exact files теперь дают ошибку. Проверка
публичного сетевого clone не выполнялась. `review` сравнивает сохранённую
ревизию с предложенной до переноса, отдельно маркирует exact/adapted. В
одноразовом потомке source commit два синтетических изменения —
`CPP/7zip/UI/FileManager/NetFolder.cpp` (adapted) и `CPP/Windows/Net.cpp`
(exact) — были обнаружены и описаны; проектный код и sibling checkout не
изменялись. Реальные upstream functional fixes в P3-01 не переносились.
Дальнейшая процедура, включая проверку omitted branches, dependencies,
локальных Qt отклонений и обновление provenance только после tests, описана в
`docs/upstream-update.md`. Эта текущая приёмка требует поддержания manifest
при каждом следующем source port. Исходные notices и LGPL/public-domain
условия остаются в соответствующих файлах и `third_party/7zip/License.txt`.
`upstream_baseline`/`upstream_inventory` и полный `check.sh` 142/142 прошли
на macOS arm64 2026-09-30; Windows/Linux runtime не проверялся.

## P1-01.1 — F5/F6 между filesystem и диском назначения (2026-09-30)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/App.cpp::CApp::OnCopy`,
`PanelItems.cpp::CPanel::Get_ItemIndices_All/Get_ItemIndices_OperSmart`,
`PanelCopy.cpp::CPanel::CopyTo`, `CopyDialog.cpp/.h/.rc` и
`CPP/7zip/UI/Agent/ArchiveFolder.cpp::CAgentFolder::CopyTo`.
`Get_ItemIndices_All/Get_ItemIndices_OperSmart` перенесены из оригинала с
исходными именами и порядком. Уже перенесённый `CPanel::CopyTo` теперь
используется F5/F6 из верхнего архива, а оригинальный `CAgentFolder::CopyTo`
выбирает `kCurPaths` для текущей папки. Для F6 это стадия копирования:
upstream `CAgentFolder::CopyTo(moveMode)` возвращает `E_NOTIMPL`, поэтому
существующий Qt/`ArchiveBackend` двухстадийный Extract → Delete остаётся
узкой функциональной адаптацией; исходные записи удаляются только после
успешной стадии копирования. Ошибка/отмена не запускают Delete.

Qt `MainWindow` передаёт F5 из архива общему `CCopyDialog`, выставляет
`CurrentPaths` для исходного `CopyTo`, сохраняет source summary/history и
обновляет активную и вторую filesystem панели после копирования/перемещения.
Проверка `ArchiveBackend::validateExtraction` отнимает текущий префикс перед
проверкой конфликтов в папке назначения и отвергает записи вне текущей папки.
В корне архива прежний `7zz` fallback эквивалентен по путям, если source
extract service отсутствует; в подпапке при отсутствии service операция
отклоняется. Filesystem worker и его overwrite/cancel остаются интеграционным
долгом P1-02; исходное `App.cpp::CApp::OnCopy` управление destination,
temp staging и `CopyFrom` ещё не перенесено и входит в P1-01.2/.3.
Ссылки/ADS, внешние handlers и вложенные архивы остаются у P2-05/P2-06,
P2-02 и P1-09. Никакой исходный модуль не объявлен полностью закрытым только
по этому сценарию. Исходные notices сохранены; лицензия —
`third_party/7zip/License.txt`.

Focused `ui_operations`, `archive_selection_runtime`, `extraction_options`,
`source_extract_backend`, `copy_dialog`, `ui_layout`, `panel_key` прошли на
macOS arm64; Copy/Move dialog JSON 440×250 и 340×220 просмотрены. Полный
`./scripts/check.sh` (format, app build, все CTest) прошёл на macOS arm64,
Qt 6.11.2, 2026-09-30. Windows/Linux runtime и rendered Windows parity
не проверены для этого изменения.

## P1-01.2 — disk→archive через исходный CopyFrom/update (2026-09-30)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные владельцы: `CPP/7zip/UI/Agent/Agent.h`,
`AgentOut.cpp::CAgent::SetFiles/DoOperation`, его
`EnumerateArchiveItems/EnumerateArchiveItems2/CAgUpCallbackImp/CDirItemsCallback_AgentOut`,
`ArchiveFolderOut.cpp::CAgentFolder::CopyFrom` и ветка `AGENT_OP_Uni`,
`CPP/7zip/UI/FileManager/Panel.h`, `PanelCopy.cpp::CPanel::CopyFrom/CThreadUpdate`,
`CPP/7zip/UI/Common/UpdatePair.cpp::GetUpdatePairInfoList` и
`UpdateProduce.cpp::UpdateProduce`. Тела исходных SetFiles, CopyFrom,
DoOperation и helper обхода/выбора пар перенесены в зеркальные пути;
`UpdateProduce.cpp` скопирован без изменений в `third_party/7zip`.
`UpdatePair.cpp` скопирован в `src/CPP/7zip/UI/Common` с изменёнными
include путями и проектным форматированием; её алгоритм сравнения времени,
сортировки и формирования пар сохранён.

Необходимые адаптации: `CAgent::DoOperation` принимает только уже открытый
архив, поскольку создание нового архива принадлежит отдельной операции
Qt/`ArchiveBackend`; его branch создания `IOutArchive` через `CCodecs`
возвращает `E_NOTIMPL`. Запись alternate streams в переносимом `CDirItem`
недоступна и явно возвращает `E_NOTIMPL`. Source property plan остаётся в
теле, но текущий UI не задаёт `m_PropNames/m_PropValues`. Исходный
`CThreadUpdate` адаптирован к имеющемуся `CProgressThreadVirt` и
`IQtProgressServices`, сохранив вызов `IFolderOperations::CopyFrom`, пароль
верхнего архива, сообщения и progress/cancel. `qtarchivesourceupdate.cpp`
проверяет source callback plan, готовит файлы/папки во временном каталоге,
переносит время через `SetDirTime` и передаёт набор в
`ArchiveBackend::updateSourceAdd`. Backend отвергает опасные пути и ссылки,
использует официальный `7zz` с исходным Add action set
`p1q1r2x2y2z2w2`, отдельным output и паролем; native
`CommonUpdateOperation` публикует результат через WorkDir, повторно
открывает архив и для F6 только после успешной публикации удаляет
обработанные исходники. Это узкая граница вместо недоступного встроенного
writer, не новая реализация архива.

Qt F5/F6 из filesystem панели маршрутизирует выбор в текущую папку
противоположной открытой ZIP/7z панели, используя исходные относительные
имена, `CCopyDialog` и путь второй панели по умолчанию. После успеха
обновляются обе панели; отказ, вложенный архив и произвольный другой
archive destination не объявлены поддержанными. Дерево папок, Unicode,
пробелы, байты, время файла, защищённый верхний 7z, F6 после публикации,
отмена и небезопасный путь проверяются в `source_update_backend`; сквозной
F5/F6 с двумя панелями и refresh проверяется в `ui_operations`. Полный
`CApp::OnCopy` dispatch, archive→archive staging и остальные destination
варианты остаются P1-01.3; filesystem `FSFolderCopy` — P1-02,
ADS/links/metadata completeness — P2-05/P2-06. Исходные notices и лицензия
сохранены в `third_party/7zip/License.txt`.

Проверка: `./scripts/build.sh dev sevenzip_desktop source_update_backend_tests
ui_operations_tests`, адресные `source_update_backend` и `ui_operations`
прошли; полный unsandboxed `./scripts/check.sh` (форматирование, сборка,
все CTest) прошёл на macOS arm64, Qt 6.11.2, 2026-09-30.
Windows/Linux runtime и rendered Windows parity для этого пакета не проверены.

## P1-01.3 — archive→archive и исходная маршрутизация Copy/Move (принято, 2026-10-01)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные тела и зависимости: `CPP/7zip/UI/FileManager/App.cpp::CApp::OnCopy`
и `CPanelCallbackImp::OnCopy`,
`PanelItems.cpp::CPanel::GetItemName_for_Copy`, уже перенесённые
`PanelCopy.cpp::CPanel::CopyTo/CopyFrom`, `Agent/ArchiveFolder.cpp::CopyTo` и
`Agent/ArchiveFolderOut.cpp::CopyFrom`. Функциональное ядро `OnCopy` сохранено
в зеркальном `App.cpp`: проверка источника, OperSmart/focused selection,
исходный `CCopyDialog` с summary/history, `CorrectFsPath`, выбор второй
панели и условий временного переноса. `CopyArchiveToArchive` сохраняет исходный
`CTempDir` → `CPanel::CopyTo` → имена `GetItemName_for_Copy` →
`CPanel::CopyFrom`. `PanelItems.cpp` использует исходное свойство `kpidOutName`
и коррекцию имени. Notices и лицензия остаются в зеркальных файлах и
`third_party/7zip/License.txt`; проектное `clang-format` учитывается отдельно.
`CopyDialog.cpp/.h` собираются один раз в `sevenzip_panel`, поскольку исходный
`CApp::OnCopy` стал непосредственным потребителем диалога; отдельные Qt-цели
связываются с этой библиотекой.

Qt-адаптация: `IQtAppPresentation::DefaultCopyDestination/ExecuteCopy`
связывает исходную маршрутизацию с панелями и текущими асинхронными
filesystem/ArchiveBackend операциями. Временная папка и публикация целевого
архива принадлежат исходным `CopyTo/CopyFrom` и транзакционному backend.
F6 удаляет источник только после успешной публикации; перед удалением Qt
сохраняет байты целевого архива и восстанавливает их при отказе/отмене.
Обновление обеих панелей, сохранение фокуса и сообщения остаются узкой Qt
границей. Для двух одновременно открытых архивов backend хранит отдельный
проверенный снимок списка (путь, размер, времена, entries, encryption),
восстанавливает нужный контекст перед Test/Extract/мутацией и сбрасывает
снимок после записи. Шифрованная исходная extraction callback вызывает
`testSourceArchive`, поскольку уже владеет исходными метаданными независимо
от последнего отображённого списка.

Это частичный перенос `OnCopy`: исходная ветка Shift+F5/F6 `copyToSame`,
переименование одной цели, создание произвольного filesystem destination и
полные `FSFolderCopy.cpp` copy/move branches пока не подключены; исходный
`PanelKey.cpp` вызывает `OnCopy(..., shift)` и этот consumer нельзя считать
неприменимым. Ветка зафиксирована как обязательный P1-01.4 после P1-02;
parent P1-01 до неё не принимается. Windows-specific alternate streams, links и
metadata остаются P2-05/P2-06. Windows/Linux runtime и rendered Windows
reference не проверены. Реальные ZIP/7z UI фикстуры проверяют F5/F6 между
двумя панелями, байты, отмену с точным откатом, зашифрованный источник,
невозможность записи из вложенного архива и обновление панелей. Backend
тестирует Test/Extract/Add по первому архиву после listing второго и отказ
для устаревшего снимка; фиктивные helper-тесты используют существующий
временный файл. Проверены `archive_backend`, `source_extract_backend`,
`source_update_backend`, `archive_progress`, `extraction_options`,
`panel_folder`, `copy_dialog`, прямой набор UI 10/10 и baseline verifier
193 exact/136 adapted/3 local. Полный unsandboxed `./scripts/check.sh`
прошёл 142/142 CTest на macOS arm64/Qt 6.11.2, 2026-10-01.

## P1-02.1 — cross-volume F6 staged fallback (принято, 2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CopyFolder/CopyFile_Ask`,
`CCopyStateIO::MyCopyFile`, `CCopyState::CallProgress` и исходная ветка
`CopyFolder` «MoveFile_Sys, затем recursive copy/delete при отказе».
Эти тела были частично перенесены ранее в зеркальный `FSFolderCopy.cpp`:
recursive copy и чтение/запись уже используются `FileTransferOperation`.
В этом результате добавлена необходимая cross-volume ветка в Qt transaction
adapter, а не заявлен перенос ещё отсутствующих `CFSFolder::CopyTo` и
`CCopyState::MoveFile_Sys`; они входят в P1-02.2.

Причина адаптации: исходный MoveMode удаляет файлы при копировании, а текущий
Qt worker публикует весь набор транзакционно и должен откатить его при ошибке
до удаления. Поэтому F6 сначала пробует rename в stage; если файловая система
отказывает (включая границу томов), тот же source `CopyFolder`/
`CopyFile_Ask` копирует в stage без удаления. После публикации всех целей
worker удаляет скопированные источники. Узкий `FileSystemAdapter` позволяет
детерминированно вызвать отказ rename и отказ удаления без второго диска.
До фазы удаления отмена и ошибка откатывают цели и сохраняют исходники.
Если удаление уже началось и завершилось частично, worker оставляет
опубликованные байты, сообщает неуспех и частичный результат; обещание
полной атомарности было бы ложным. Исходные Windows MoveFileWithProgress,
AskWrite per-entry callback, прямое имя цели и recursive merge не включены
и остаются P1-02.2. Ссылки и полные метаданные — P2-05/P2-06.

Тесты forced fallback с Unicode/пробелами, деревом, занятой целью,
отменой и частичным удалением проходят в `file_transfer`; прямые disk-panel
F5/F6 UI consumers проходят. Полный unsandboxed `./scripts/check.sh`
прошёл 142/142 CTest на macOS arm64/Qt 6.11.2, 2026-10-01.
Windows/Linux runtime и реальный второй том не проверены.

## P1-02.2 — full FSFolderCopy callback and directory merge (принято, 2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp::{CCopyState,CopyFile_Ask,
CopyFolder,CFSFolder::CopyTo,CopyFileSystemItems}` и `FSFolder.h`;
consumers `PanelCopy.cpp::CPanelCopyThread::ProcessVirt`,
`ExtractCallback.cpp::CExtractCallbackImp::AskWrite`. Эти исходные symbol,
обход/подсчёт, AskWrite→copy/move→progress, рекурсия и direct-path ветка
остаются в зеркальном модуле. `CFSFolder::CopyTo` перенесён из `FSFolder.cpp`
stub в `FSFolderCopy.cpp`; этот файл теперь единожды компилируется в
`sevenzip_filesystem`. `CopyFrom/CopyFromFile` остаются оригинальными
`E_NOTIMPL`.

Адаптации: `CFSFolder` собирает полный путь как `_path + GetRelPath(fi)`
вместо ещё не восстановленного исходного `GetFullPath`. Portable
`CFsFolderStat` принимает путь в конструкторе. Windows `CopyFile_Sys`
использует исходный `CopyFileExW` callback; POSIX не имеет его и использует
перенесённый `CCopyStateIO::MyCopyFile`. `MoveFile_Sys` делегирует в
адаптированный source `MyMoveFile_with_Progress`; для каталога Qt rename
возвращает управление исходной recursive ветке при отказе, поскольку POSIX
FileDir EXDEV fallback копирует только обычные файлы. POSIX `CEnumerator`
заполняет `CFileInfo` через `Fill_FileInfo`; решение AskWrite для stage
передаёт узкий `CFileTransferCopyCallback`, без widget в source module.
Исходный `CExtractCallbackImp::AskWrite` остаётся COM consumer для
`CPanel::CopyTo`. MainWindow F5/F6 сохраняет транзакционный Qt worker:
для занятого каталога он копирует прежнее дерево в stage, применяет тот же
`CopyFolder` с per-file AskWrite и публикует результат. F6 удаляет только
записанные исходные файлы; пропущенные остаются. Ошибка/отмена до удаления
откатывает stage, а отказ удаления после публикации сохраняет целевые байты
и сообщает частичный результат.

Alternate streams и ссылки явно отклоняются до переноса (P2-05/P2-06);
metadata и Windows super-path ветки требуют отдельных platform проверок.
Прямое имя цели проверено через native `CFSFolder::CopyTo`, но Shift+F5/F6
Qt consumer относится к P1-01.4. Проверены `fs_folder_copy`, `fs_folder`,
`file_transfer`, прямой two-panel filesystem UI consumer и normal/compact
`main_shell`/`panel_shell` JSON; полный unsandboxed `check.sh` прошёл 143/143
CTest на macOS arm64/Qt 6.11.2, 2026-10-01. Реальный второй том,
Windows/Linux runtime и rendered Windows reference локально не проверены.

### P1-18 implementation — native RAR memory consumers

Baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`https://github.com/ip7z/7zip.git`. Existing adapted
`CPP/7zip/Archive/Rar/Rar5Handler.cpp` now retains the complete original handler,
including header parsing, Open/Extract, dictionary/memory request control flow,
properties and BLAKE2sp/arc registration. Changes are canonical include paths,
source notice and project formatting. It remains one authoritative module in
the forced hash/arc registration object; a second vendored handler is not built.
Direct baseline comparison after excluding includes/comments and normalizing
C++ backslash-newline splices found all 16,536 functional tokens identical.
Formatting changed macro line continuations, not their logical bodies.

Exact vendored dependencies are recorded in `docs/upstream-baseline.json`:
Archive/Common/FindSignature, Archive/HandlerCont, Rar5Handler.h/RarHeader/RarVol
and StdAfx headers, Compress/HuffmanDecoder and Rar5Decoder, Crypto/HmacSha256
and Rar5Aes. Existing source stream, AES and ZIP/common dependencies are reused.
`CPP/7zip/Compress/RarCodecsRegister.cpp` copies the original RAR5 factory and
registration macros; legacy RAR1/2/3 registrations and decoder dependencies are
omitted and remain supplied by the official codec module. The native RAR5
registration object is forced into consumers so original CreateCoder finds the
copied decoder before external codecs. `cmake/native_rar.cmake` groups this
dependency closure without reorganizing original source modules; common/RAR/ZIP
static archive dependencies are linked through CMake.
The first full gate exposed LoadCodecs filesystem symbols missing from
foundation-only consumers. Existing FileDir/FileName/FileFind/FileIO/TimeUtils
and FileDir's Qt owned-temp identity/registry dependencies now have their sole
build owner in sevenzip_common (QtCore only), alongside original FileStreams.
No module is copied again or rewritten; filesystem/application targets consume
that foundation. This closes the complete handler/hash registration dependency
graph for every consumer, not just archive UI targets.

Verified module metadata retains ownership of format capabilities; only Rar5
reader construction is replaced with the source factory. Native reader format
indices are bounds checked. QtArchiveFileResource validates every source item
index/path against the file-only listing before exposing native extraction.
Mismatch refuses extraction rather than silently routing memory decisions to
the console. ArchiveBackend calls original IInArchive::Extract; original Agent,
FM CExtractCallbackImp::RequestMemoryUse and CMemDialog own decisions. Native
progress uses cooperative source synchronization without requiring a child
process. Batch opening applies original SetProperties after Open and before
Extract because its file-only adapter has already opened the native handler;
RAR memx/crc/mt extraction properties retain original interpretation.

Deterministic stored RAR5/RAR7 fixtures declare a 2 MiB dictionary and force the
public handler memx limit to 1 MiB. The actual handler requests the original
required/allowed values and flags through the GUI callback: no synthetic memory
invocation or console text parsing. A pinned libarchive compressed fixture
checks the copied decoder against known payload size/CRC; its provenance and
distribution notice are under `tests/fixtures/rar/`. Installed official 7zz
26.03 refuses that compressed fixture with Unsupported Method; independent
libarchive extraction is used where available. These checks do not establish
RAR7 compressed interoperability, Windows/Linux runtime or rendered parity.
Encrypted/solid and RAR multivolume GUI workflows are not established by these
small fixtures. Original handler bodies are retained, but the file-only batch
adapter still refuses non-.001 volume sets; the source module's full scope is
not evidence of full workflow/platform coverage.
P1-18 acceptance is recorded below; the additional unverified workflows above
remain explicit limits, not evidence supplied by these fixtures.
MainWindow root/selected Extract and Test also use the native reader for all
overwrite modes. Original source overwrite enums remain authoritative; local
RejectExisting validation maps to source Skip against a newly appearing file.
Unavailable index contracts refuse commands explicitly. The existing broad
mainwindow_archivecommands glue is kept focused on this routing change; its
general extraction/state component split is deferred rather than reorganizing
unrelated commands in the source-port iteration.

#### P1-18 acceptance (2026-10-02)

`native_rar_memory` passes35 Qt cases (28 handler Allow/Skip/Cancel/CRC rows,
four batch remember/save/reuse/cancel cases, compressed decoder and setup).
`native_rar_ui` passes15 Qt cases (12 actual root/selected × six overwrite
mode/Test rows, traversal refusal and setup). Exact source requests carry
2 MiB required/1 MiB allowed and original flags through source GUI callbacks.
Skip preserves output, cancellation returns E_ABORT, original archives are
unchanged; source IsOK separately reports CRC/Skip errors even when HRESULT
is S_OK. Missing index contracts refuse actual source extraction with no output.
Independent libarchive bytes/known CRC establish compressed RAR5 decoding;
installed7zz still cannot supply that comparator. Actual memory dialog Extract/
Test reports at700×460/560×460 were inspected: all visible controls fit,
labels fit, grouping/order and single-archive hidden remember follow source.

Focused direct consumer gate8/8 PASS6.96s; after foundation dependency correction,
foundation/RAR4/4 PASS1.30s and final RAR2/2 PASS1.25s. Baseline verifier367exact/
164adapted/3local PASS. Final unsandboxed `./scripts/check.sh` passes formatting,
all-target build and193/193 CTest79.91s, macOS arm64 Qt6.11.2. First full gate's
foundation linker failure was fixed before this acceptance. Windows/Linux runtime
and rendered Windows reference remain unverified. Whole iteration local commit
subject: `Port source RAR decoder memory requests and GUI consumers (P1-18)`.
No helper commit, push, publication or upstream change.

### P2-02 acceptance — source SFX and external modules (2026-10-03)

Whole P2-02 is accepted. The investigation below records copied CompressDialog
method/name/control bodies, Update PrepareSfxModule/WriteSfxModule and LoadCodecs
directory/ABI/SetCodecs flow, narrow Qt/7zz/platform adapters and omitted obsolete
FilePlugins/RegistryPlugins/ProgramLocation branches. All borrowed modules use
the reproducible official repository baseline
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Five exact dependencies are added:
Compress CodecExports.cpp/DllExportsCompress.cpp/Codec.def, Windows COM.h and
7zip/Common/UniqBlocks.cpp. Verifier372 exact/164 adapted/3 local passes.

Official native SFXCon/SFXWin, Console/Format7zF and Windows/Linux Alone2 runtimes
are actually compiled in isolated baseline snapshots. Both Windows producer
scripts reject unowned artifacts and junction outputs without changing sentinels.
Actual Add/SFX single/split/AES/delete-after-publication, native extraction,
engine/volume pause, second-root/prepublication cancellation and module refusals
pass. External codec and global-only source-handler facade support real engine
Create/List/Test/Extract/native leases, another catalog's release, nested assets,
missing-main Formats, ABI warnings/mismatch/cancel and actual junction cycles.

Final checks: unsandboxed macOS arm64 Qt6.11.2 check.sh formatting/all-target
build/197 CTest PASS107.31s; Linux ARM64 Qt6.4.2/GCC13.3 nine focused suites
PASS2.79s with zero Qt skips; Windows11 ARM64 Qt6.8.3/MSVC14.44 clean build and
seven focused suites PASS13.76s. Windows external/SFX have zero skips; the
existing owned_temp linkedRootAndLinkedObjectAreRejected file-symlink fixture
remains skipped and belongs to platform P3 work. UTF-8 console compiler prefix
and actual Ninja header records are verified after the clean Windows build.
Current EN/RU SFX JSON normal/compact geometry is reviewed on all three OS.
This establishes structure/usability, not rendered 7zFM pixel parity.

One whole local iteration commit; no helper commits/push/publication/upstream
writes. Counts31/46,15 remain. P3 full platform acceptance, rendered Windows
reference and separate mandatory Finder P3-03.1 remain open. Next P2-03.
Historical pending/failure statements below are superseded by this acceptance.

### P2-02 investigation — SFX and external modules (historical, 2026-10-02)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
`https://github.com/ip7z/7zip.git`. FM FilePlugins.cpp uses its internal Agent
folder manager; external CPluginLibrary branches are commented out.
RegistryPlugins.cpp is entirely commented-out legacy discovery. ProgramLocation
contains no active functional implementation. Do not revive these as an invented
current plugin workflow. Actual external archive/codec discovery and compatibility
are original Common/LoadCodecs.cpp::Load/LoadDllsFromFolder/LoadDll/IsSupportedDll.
The immutable catalog now loads original Codecs/Formats directories when the
dynamic engine's source Libs output confirms the deployment. Standalone7zz lacks
Z7_EXTERNAL_CODECS and cannot supply this capability. The official Console7z and
Format7zF module are built unchanged from the same pinned snapshot by the new
runtime builder; Debug/deployed-engine location and explicit path selection are
local runtime glue. No compression or archive handler is recreated.

LoadCodecs.cpp::Load retains its main/registry/global-SetCodecs source branches;
its two folder calls and NeedSetLibCodecs decision are factored unchanged into
LoadExternalModules(baseFolder). The Qt catalog supplies the engine deployment
folder instead of its own executable folder. SetLibCodecs factors the original
Load SetCodecs loop without changing its body. A confirmed dynamic deployment
owns private copies of its engine, main DLL and Codecs/Formats trees in
ArchiveCodecImage; the catalog retains those files until its COM owner unloads.
Native readers and child engine operations use that same private deployment.
This isolates the DLL-global CExternalCodecs context between retained catalogs;
OpenArchive::PrepareToOpen retains the original per-instance fallback. Original
paths are restored in source diagnostics and deployment comparisons. This is
local runtime/lifetime glue, not a replacement loader or archive algorithm.
Discovery and archive jobs exclude one another so publication cannot change the
engine or remove its image during an operation. Local copied ZIP/RAR factory
hooks explicitly retain NeedSetLibCodecs after the source g_NumArcs decision,
because those replacement factories are not registered in g_Arcs. A main-only
dynamic deployment's copied ZIP handler reads an actual BZip2 ZIP via this context.
The current macOS fixtures verify real external codec decoding and retained
catalog isolation, plus a deployment with no main DLL and an unchanged official
module in Formats: native exact-byte decoding and engine Create/List/Test/Extract
pass while the missing-main warning remains visible. The test-only external
format facade delegates all IInArchive work to the unchanged official module,
reports a distinct read-only type and withholds ISetCompressCodecsInfo. Actual
decoding of the external F001 Copy method therefore uses the original DLL-global
SetCodecs; it retains exact bytes after a second confirmed catalog is created
and released. Forced-type backend Open/Test/Extract also passes. The fixture
borrows an already-loaded core module handle and owns no extra global loader
lease. This is synthetic test metadata/protocol glue, not a new archive algorithm.
The facade additionally requires a nested resource at object creation. Its
retained image opens another handler after that resource is removed from the
original deployment; native/engine tests pass. Folder copying retains assets,
while original LoadCodecs direct-module enumeration remains unchanged. Cyclic
directory links refuse preparation; external/system dependencies are still
resolved by the platform loader, not relocated by this adapter. Windows/Linux
image isolation still needs verification within P2-02.

ArchiveCodecProbe only parses the source Console Libs/version transport (including
CRLF); original LoadDll/IsSupportedDll own module metadata and ABI compatibility.
Implicit main-module selection transports the engine's Libs resolution, including
the Codecs/Formats base folder when the main module is missing; explicit module
requests retain mismatch refusal. The catalog refuses engine/native deployment
mismatch. Original nonfatal load
warnings retain working modules and reach startup status plus copied About's
GetCodecsErrorMessage/error-box branch. Qt substitutes the parent, modal service
and retained catalog for global g_CodecsObj. GetCodecsErrorMessage is const-qualified
for that read-only lease, with its functional body unchanged. About's local app
caption now describes the official engine rather than a fixed standalone client.

The focused sfx_lifecycle consumer accepts the actual Add form, passes its
public target/sources/options to ArchiveBackend, and verifies native extraction
after single/split creation and source deletion. Cancellation at the second
root and before volume publication preserves both sources and leaves no output;
engine and volume-worker pause/resume retain correct output. Two separate source
folders are required: same-folder files intentionally form one engine stage.
Small deterministic random inputs keep the real compression process alive for
pause testing; Store can finish before Qt delivers started. These tests exercise
the existing copied lifecycle and platform pause adapters. Latest affected four
suites pass2.63s on macOS arm64.

Advanced method override follows the original UpdateGUI.cpp::ShowDialog
SplitOptionsToStrings/IsThereMethodOverride/SetOutProperties/ParseAndAddPropertires
order at the recorded baseline. The copied GetCompressionProperties retains
this order. SFX UI filters the selected method, while explicit advanced properties
may override it as upstream permits; no new blanket restriction is introduced.
Supported defaults and their native SFX decoding are verified separately.

Linux ARM64 verification builds unchanged Console/Format7zF/SFXCon/Alone2 at the
recorded revision in an isolated guest snapshot. SFX SHA-256 is
de371b3bb5ff832d6a3196f5171638f978fd62f98a6ff38136c0e371cbf996e4.
Qt6.4.2/GCC13.3.0 on Linux6.8.0-142 compiles the affected six targets; all six
CTest suites pass1.52s with zero Qt skips. Private images, global-only handler,
nested assets, missing-main and source SFX workflows therefore have direct Linux
evidence. Four EN/RU SFX JSON reports expose ordered controls at normal/compact
sizes, with checkbox width exceeding size hints. No rendered Windows parity claim.

The build exposed a narrow FSFolderCopy.cpp adapter error: Qt quint64 and upstream
UInt64 have different underlying integer types on Linux. CCopyState::CallProgress
and its Qt IO progress lambda now pass a local UInt64 value to the original
IProgress::SetCompleted, preserving the callback contract and copy flow. Existing
filesystem/copy regressions pass on macOS2/2 and Linux; no source algorithm changes.

Windows fixtures now select the native GUI module and .exe naming. SfxWin.cpp
WinMain2 always extracts, accepts -y/-o/-p and sets eo.TestMode=false. Accordingly
the fixtures use a separate official engine for Test and omit the console x verb
when executing GUI SFX; split Test addresses the payload, not the bare module.
Windows native consumer execution remains unverified, not N/A.

The Windows Qt build exposed explicit dependency gaps: processpause.h,
processpriority.h and qtarchivevolumeoutput.h now include original MyTypes.h
for UInt32/UInt64 and MY_UNCOPYABLE rather than relying on Unix MyWindows.h.
QtOwnedTempIdentity uses a local CFileBase-derived identity handle to expose
the protected original Create service with its existing zero-access,
read/write/delete sharing and reparse-point flags. CInFile::OpenReparse cannot
replace it because its read-only sharing would reject concurrently held writable
temporary files. No original FileIO body changes. FSFolderCopy's local move
progress adapter is named QtMoveFileProgress to avoid the existing Windows
CopyProgressRoutine callback name; callback and copy/move flow are unchanged.
External fixture paths select .exe/.dll on Windows and original Unix names
elsewhere. Host owned-temp/pause/priority and filesystem regressions pass;
these are portability fixes, not Windows runtime acceptance.

Further Windows compile fixes retain the original constants when Windows already
defines CBN_SELCHANGE/IDCLOSE, keeping local declarations only on other hosts.
QtPanelFolder explicitly casts an already range-checked property index to
unsigned for CObjectVector's original overloads: Windows UInt32 is unsigned long.
PanelOperations keeps original CComInitializer; its missing CPP/Windows/COM.h
dependency is copied byte-for-byte from the recorded revision into third_party,
using the existing Windows/COM.h include-root convention.
Both Windows producer scripts actually refuse unowned artifact and junction
output-directory fixtures before compilation, preserving sentinel SHA-256 and
the junction target's single-file inventory. Positive compilation and these
failure paths do not establish interactive Windows SFX or pixel parity.

Windows BrowseDialog::CorrectFsPath qualifies original NFind::CFileInfo
(Unix excludes this branch). NetFolder's retained Windows icon calls now reach
SysIconUtils.cpp's copied Shell_GetFileInfo_SysIconIndex_for_Path Unicode branch
and original HRESULT wrapper. SHGetFileInfoW, default attributes, flags,
icon-index output and GetLastError_noZero_HRESULT control flow are retained;
obsolete ANSI/NT dynamic dispatch and other CSIDL/image-list helpers are omitted.
The Qt icon cache remains its separate presentation service. Only Windows links
shell32. The original symbols/source paths share the recorded full baseline
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17; no second icon algorithm is introduced.

Windows fixture linking exposed invalid symbols `=` and `!` in CMake's generated
export-all definition. The codec fixture now uses unchanged original
CPP/7zip/Compress/Codec.def from the same baseline. The two local facade/ABI
fixtures have explicit test-only .def lists of their public module functions;
internal COM/Qt symbols are not part of the plugin ABI. Unix exports and original
CodecExports/DllExportsCompress bodies are unchanged. Baseline verification now
passes371 exact/164 adapted/3 local paths; four exact dependency files are staged
only for inventory. Whole P2-02 gate/acceptance and commit remain outstanding.
QtLocationFolder's archive metadata adapter inserts raw-property entries with
explicit PROPID keys instead of assigning QMap<quint32> to QMap<PROPID>:
Windows typedefs use distinct C++ types of the same32-bit width. Values and
property ordering are retained; existing raw-property consumers remain covered.

Windows linking then exposed dependencies retained by original extraction/
enumeration: Convert_WinPath_to_WslLinuxPath, FillLinkData_WslLink/WinLink,
NIO GetReparseData/SetReparseData and CUniqBlocks::AddUniq. The FileLink.cpp
Windows bodies were already copied but gated by a temporary local capability
macro; that extra guard is removed now that their FileIO/FileDir services exist.
Their original _WIN32/UNDER_CE guards and functional bodies are unchanged.
UniqBlocks.cpp is copied exactly from the recorded baseline beside its existing
header and built by sevenzip_filesystem. Broad ADS/link/security workflow
acceptance remains P2/P3; supplying these dependencies is not that acceptance.
Baseline verification after UniqBlocks.cpp passes372 exact/164 adapted/3 local
paths. Five exact dependencies are staged for inventory, without a helper commit.

MSVC dumpbin identifies a concrete ReOpen ABI mismatch: its implementation used
class IFolderFolder while the caller used the original struct IFolderFolder.
QtArchiveFileResource and QtArchiveReopen forward declarations now match the
original struct COM-interface tags, including the open callback. This changes
neither interface IDs/vtables nor archive behavior; it makes Windows decorated
function names agree across translation units. Unix mangling ignored this tag
difference, so host success alone did not expose the bug.

Repeated linking after the header fix exposed a separate build prerequisite:
CMake had recorded a mojibake Russian msvc_deps_prefix, so Ninja ignored MSVC
include dependencies and reused an older object. Its timestamp predates the
changed header; dumpbin confirmed the old class-tag symbol. The isolated Windows
guest cache/object directory is recreated with process-local VSLANG=1033.
build.sh now defaults that diagnostic language on Windows when unset, preserving
explicit caller settings. Compiler diagnostics only; product localization and
global VM environment are unchanged. Existing malformed caches require a clean
configuration; this is documented rather than silently deleting user outputs.
The installed ARM64 compiler contains only1049 resources, so VSLANG=1033 alone
does not change its Russian output. A separate minimal CMake/Ninja fixture with
UTF-8 console (`chcp65001`) actually recompiles after a header changes VALUE7→8;
both exit values confirm dependency delivery. build.sh now uses that console
encoding as well, restoring the caller's previous code page on exit. Explicit
language settings remain respected; no language pack/toolchain installation.

The first native Windows focused run compiles all seven targets and passes
codec_catalog, sfx_lifecycle, owned_temp and process_pause; SFX creation's20
non-layout cases pass. Failures are not accepted: priority test transport expected
LF instead of native CRLF, external facade used a non-native full DLL path, and
geometry scroll ran before Windows show/layout events settled. Test adapters now
normalize CRLF, use native path separators and settle events before scrolling.
Original LoadDll intentionally ignores ERROR_BAD_EXE_FORMAT from Windows
LOAD_LIBRARY_AS_DATAFILE; invalid-file expectations now follow that branch,
while Unix retains its warning. The VM's facade discovery gets a15s test wait
(measured7.15s); real backend timeouts are unchanged. The Windows resource-cycle
fixture uses original FillLinkData_WinLink/SetReparseData/GetReparseData to create
and verify a disposable junction rather than skipping directory-link checks.
The fixture removes only the junction and retains its target. Windows JSON
reports are written before geometry assertions for failure diagnosis; no screenshots.

Follow-up Windows focused runs reach6/7 suites: creation24/24 (including four
EN/RU geometry rows), lifecycle, catalog, owned-temp, pause and priority pass.
Two external-module failures remain at that checkpoint. Diagnostic module lists
show native loading the facade while the official engine omits it. The facade
is a local fixture, not a copied archive implementation: Qt-only path/marker
helpers are replaced with dependency-free C++17 filesystem/stream glue so
original Console's C/DllSecur.c search policy can load it without Qt DLLs.
All format/codec operations still delegate the exact original module, with no
extra loader lease. Deployment mismatches now report both module lists and the
original codec errors, making this refusal actionable.

The actual Windows junction fixture exposes a snapshot cycle missed by pathname
comparison. ArchiveCodecImage now identifies Windows directory ancestors using
the original CFileBase::Create/GetFileInformation service, following junctions
with FILE_FLAG_BACKUP_SEMANTICS and read/write/delete sharing. Volume/file IDs
identify the target across aliases; Unix retains canonical-path ancestry.
This is local immutable-runtime/platform glue, not a new loader algorithm.
The retained source loader still enumerates its original directories and ABI.

Two unchanged test dependencies are vendored at the exact baseline:
CPP/7zip/Compress/CodecExports.cpp and DllExportsCompress.cpp. The test fixture
registers the already borrowed CopyCoder under a synthetic ID/name, preserving
its algorithm and original module exports. Actual Console Create/List/Test/Extract
and native7z handler extraction prove use of this external codec; a main-only
catalog reports UnsupportedMethod for the same archive. A retained native handler
survives replacement of another catalog. Unicode/space plugin path, ABI-incompatible
and non-library warnings, mismatched deployment, discovery cancel and default
engine-path selection/refusal are covered. Tests use disposable files only.
Runtime-builder unowned-artifact refusal preserves exact foreign bytes.
Baseline verifier:369 exact/164 adapted/3 local; the two exact sources are staged
only for its tracked-file inventory, not a helper commit.

Official `CPP/7zip/Bundles/SFXCon/makefile.gcc` builds a native macOS arm64 console
SFX in an isolated snapshot, without changing upstream. New build-sfx.sh archives
the exact recorded revision, uses its unchanged makefiles/engine, handles the
host SDK include-path workaround and produces module/license/provenance JSON
with SHA-256. Windows GUI SFX remains a separate official runtime dependency.
Both manual snapshot and script produced SHA-256
ad64e6ffb725efdab3a2d3072ebffb6b20f74af10a34ed4ebb8c782cdc32b99b, version26.03.
An official7zz26.03 disposable Unicode/space fixture made from this module passes
native self-Test and Extract. 7zz leaves the result non-executable: a Unix
publication permission adapter is required before offering native SFX execution.
This check used only temporary generated data; it does not accept an app consumer.

CompressDialog.cpp retains original IsMethodSupportedBySfx and SFX method filtering
from SetMethod, CheckSFXControlsEnable and OnButtonSFX. Qt substitutes editable
path text and a returned enable flag for Win32 control access. Windows keeps
`.exe`; Unix adapts GUI kExeExt to the empty extension already used by original
Common/Update.cpp, for the native console module rather than a Windows target.
Source names, method IDs and control flow are retained.

Current WIP copies CUpdateErrorInfo and its three error setters from Update.h/.cpp,
the UpdateArchive module-presence/path-fallback branch into PrepareSfxModule,
and Compress's separate-volume SFX stream/copy/close body into WriteSfxModule.
Runtime-directory injection replaces GetModuleDirPrefix; Qt staging replaces the
final output path, while CInFileStream/COutFileStream/CopyStream remain original.
The single-output prefix and rsfx property are executed by official7zz, not a new
archive writer. The volume consumer compresses the payload without an SFX prefix,
uses the original CMultiOutStream, and stages/publishes the module separately.
WriteSfx UI callback is represented by worker progress; module copy has no separate
progress callback, as original CopyStream uses NULL. Other Compress/UpdateArchive
branches are omitted or delegated to the existing engine adapter.

ArchiveSfxModule is local runtime/integration glue: reads the build's recorded
source revision/host/version and content digest with original C/Sha256; the catalog
uses original CArchiveLink/Open3 and MachO/ELF/PE handlers' kpidCpu to check native
format and architecture. The sidecar provides reproducible provenance/integrity,
not a signed authenticity guarantee. New build-sfx.ps1 snapshots the same pinned
revision and invokes unchanged SFXWin/makefile with MSVC nmake PLATFORM; version
comes from original 7zVersion.h, manifest/license from that exact snapshot. It
guards unowned/reparse artifacts and avoids PowerShell 5.1 binary pipelines.
Windows Read requires this builder's producer. Parser validation succeeds on
Windows11 ARM64 10.0.26300. The earlier missing-tools probe inspected PATH only:
installed VS2022/MSVC14.44.35207, SDK10.0.26100.0 and Git are available through
a process-local developer environment. Actual unchanged SFXWin compilation passes;
module SHA-256 c87deca002afe1616dd7ce24d60824f135f75c0c6e3a0b0ea74df75167103004.
The new build-codec-runtime.ps1 uses the same pinned snapshot and ownership guards
to compile unchanged Console/Format7zF/Alone2 Windows makefiles. Both clients
report26.03 arm64, with the dynamic client loading its prepared7z.dll. Native
Windows Qt consumers remain unverified. Unix publication adds owner execution
permission before atomic rename. Missing/incompatible modules are refused.

An actual official7zz26.03 Unix `-sfx -v` fixture creates `self.001`, but unchanged
SFX OpenArchive.cpp::OpenStreamOrFile looks for `self.7z.001`; self-Test fails until
the volume is renamed. Our Unix integration explicitly publishes `<self>.7z.NNN`
to match that original reader, while preserving the native module unchanged.
Windows uses the original `.exe` to `.7z.NNN` naming intent. This necessary Unix
deviation is not an upstream change.

The Qt form now places source IDX_COMPRESS_SFX before Shared/Delete in the original
Options group; native catalog/module capabilities gate it, and source method/name
branches drive toggles and format transitions. Existing-target updates keep SFX
disabled. Focused macOS arm64 checks cover single/multi-root, ordinary/split output,
AES/header encryption, native generated self-Test/Extract, missing module, occupied
output, cancellation, unchanged source/module, and normal/compact EN/RU geometry.
Remaining external-module consumers/protocols, Windows/Linux runtime and full gate remain open.
Focused sfx_options/sfx_creation/compression/archive_volume_creation4/4PASS2.91s;
after the localized SFX→ZIP method-list restoration, affected sfx_creation/
compression2/2PASS1.40s and git diff --check PASS, macOS arm64 Qt6.11.2.
No P2-02 acceptance, full gate or helper commit has been claimed.

External/runtime/About WIP checks2026-10-02 macOS arm64 Qt6.11.2:
sevenzip_desktop and affected targets build PASS; final external_codecs/
codec_catalog/help_source/codec_commands_ui/help_ui5/5PASS4.42s; baseline verifier
369/164/3 PASS; git diff --check PASS. Earlier SFX focused evidence above still
applies to its last checked consumers; a whole-component full gate remains due.
About appcaption fourEN/RU360/300 JSON reviewed. Standard codec-error QMessageBox
fits at actual400×121 for both600/400 resize requests; no two-size rendering
claim is made for that auto-fit dialog.

## P1-06.2 — source consumers томов (принято, 2026-10-01)

Та же точная база `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Из `CPP/7zip/UI/FileManager/Panel.cpp::TestArchives` скопирована archive-folder
ветка: `Get_ItemIndices_OperSmart`, QueryInterface `IArchiveFolder`, исходные
`CCopyToOptions::{streamMode,showErrorMessages,testMode}`, `CopyTo` и error/abort
ветви. В `Panel.h` скопирован исходный трёхаргументный `CopyTo` overload,
который берёт пароль последнего `_parentFolders` и вызывает общий consumer.
Win32 presentation заменена существующим Qt service; архивная ветка не
переписана. Filesystem `::TestArchives(paths)` batch entry point и upstream
закомментированный старый `CThreadTest` сюда не перенесены: текущая команда
доступна в открытом архиве; filesystem entrypoint остаётся mandatory omitted
scope P3-05 для `Panel.cpp`, а не считается неприменимым.

MainWindow связывает production test command с этим source consumer; источник
без extract service сохраняет прежний transport fallback. Source `PanelCopy`,
Agent `ArchiveFolder/Agent`, `Common/Extract`, `GUI/ExtractGUI` и callbacks
переиспользуются. `QtArchiveFolderManager`/`QtArchiveBatchOpen` уже передают
`7zz` listing, embedded format и numbered VolumePaths в исходные владельцы:
внешний engine открывает Split handler, поэтому для extract `.001` нельзя
принудительно задавать inner `-t7z`. Native Common Extract loop исключает
следующие VolumePaths из повторного batch извлечения. Полные in-process
codec opening bodies `OpenArchive::Open/Open2` не подменяются claims этого
шага; их прежний omitted scope сохраняется в общем audit/P2-01/P3-05.

Новый integration тест создаёт тома через принятый source stream, затем открывает
их через `BindToPath`/`FileFolderPluginOpen`, проверяет исходный smart selection,
test callback и root Ask extraction. Пароль проверяется до overwrite GetStream;
missing/corrupt volumes показывают source errors. Cancel открытия, теста и byte
извлечения освобождает backend и возвращает commands. Существующие тома не
изменяются. Qt native picker получает `*.001` в локальном archive filter;
upstream Windows filesystem batch presentation остаётся отдельным scope.

Проверки: focused 9/9 6.14 с; unsandboxed `./scripts/check.sh` — formatting,
build и 148/148 CTest, 40.58 с, macOS arm64/Qt 6.11.2 2026-10-01.
Parent P1-06 принят; Windows/Linux runtime и rendered Windows reference
не проверены. Source creation Add normal/compact JSON принят в P1-06.1;
nonvisual test routing переиспользует прежний source progress, picker filter
проверен через native-service Qt fixture без заявления геометрии OS dialog.

## P1-06.1 — создание архивных томов (2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/Common/MultiOutStream.cpp` скопирован целиком в зеркальный `src/`
путь; изменены includes, provenance notice и clang-format. Header `.h`
сохранён побайтно в `third_party/7zip/`; baseline manifest фиксирует оба файла.
Сохранены `CMultiOutStream` numbering, переменные размеры, seek, ограничения
открытых файлов, `FinalFlush_and_CloseFiles` и cleanup. Переиспользуются исходные
`FileStreams`, `CopyCoder`, `FileIO`, `FileDir`, `FileFind` и `System` adapters.
`SplitUtils.cpp::{ParseVolumeSizes,GetNumberOfVolumes}` восстановлены с исходными
`UString/CRecordVector`, control flow и overflow checks; Qt остаётся только
consumer/preset presentation. Реализация одна, в `sevenzip_panel`.

`GUI/CompressDialog.cpp::OnOK` volume block перенесён с Trim/Clear/parser,
ошибкой размера и Yes/No/Cancel для последнего размера менее 100 KiB;
`CompressDialog.rc` задаёт combo/label, `AddVolumeItems` — исходные девять presets.
Qt `createarchivedialog_volumes.cpp` связывает этот результат с controls.
Исходник не сохраняет произвольную историю размеров между запусками; список
presets и пустое начальное значение сохранены. Сохранение профиля происходит
только после успешной проверки и подтверждения.

`Common/Update.cpp::Compress` задаёт исходную интеграцию `CMultiOutStream`.
Её encoder orchestration ещё не перенесена целиком: официальный внешний `7zz`
не изменяет многотомный архив, а Qt создание использует несколько source-root
стадий. Поэтому `ArchiveBackend` завершает один официальный временный архив,
затем `QtArchiveVolumeOutput` передаёт готовые bytes через исходный stream и
публикует numbered set. Это требует места для полного временного архива плюс
томов; encryption/format не реализуются заново. Worker поддерживает pause/cancel,
проверяет occupied targets и откатывает только свои опубликованные файлы.
Защищённый 7z на последующих стадиях использует пароль открытого архива вместо
повторного CLI new-password prompt. Изменение существующего набора томов
отклоняется; source open/list/test/extract acceptance остаётся P1-06.2, SFX — P2-02.

Проверки: focused 8/8 3.86 с; unsandboxed `./scripts/check.sh` — formatting,
build и 147/147 CTest, 41.04 с, macOS arm64/Qt 6.11.2 2026-10-01.
Baseline verify: 194 exact, 139 adapted, 3 local paths. Normal/compact JSON
проверены; Windows/Linux runtime и rendered Windows reference не проверены.

## P1-01.4 — Shift+F5/F6 и прямое имя назначения (принято, 2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelKey.cpp::OnKeyDown` (Shift+F5/F6 →
`OnCopy(..., shift)`), `App.cpp::CApp::Copy` (focused index, default name,
`CorrectFsPath`, direct destination/parent creation), `PanelCopy.cpp::CopyTo`,
`FSFolderCopy.cpp::CFSFolder::CopyTo` и
`CPP/7zip/UI/Agent/ArchiveFolder.cpp::CAgentFolder::CopyTo`/
`Agent.cpp::CAgentFolder::Extract`. Исходные `App.cpp` и `PanelKey.cpp`
сохраняют зеркальные пути и symbols; изменены только Qt presentation/dispatch
части. Исходный `FSFolderCopy.cpp` и Agent bodies остаются единственными
реализациями в своих модулях.

`PanelKey` теперь передаёт Shift в source `OnCopy`, независимо от обычной
selection-gated доступности QAction: исходная ветка выбирает именно элемент
под фокусом. `App.cpp` сохраняет признак завершающего разделителя и создаёт
родительский каталог для одиночного прямого пути. В Qt transaction adapter
`FileTransferOperation` передаётся точный target вместо папки для одного
файла/каталога; staging, исходный `FSFolderCopy` AskWrite, backup/rollback и
F6 publication/delete остаются общими. Для обычного F5/F6 один новый путь
без завершающего разделителя тоже является прямым именем, а существующая
папка остаётся папкой назначения. Самокопирование и вложенная цель каталога
отклоняются до изменения данных.

Для archive source Shift направляет explicit focused index в исходный Agent
`CopyTo`; F6 удаляет этот index после успешного извлечения. Это не переименование
записи архива: исходный `CAgentFolder::Extract` нормализует destination как
каталог, поэтому абсолютный путь извлекает элемент в указанную папку. Если
source extract service недоступен, Shift показывает отказ без fallback на
текущий selection. Запись под новым именем внутри архива относится к иному
source update workflow, а не к этой ветке `CopyTo`.

Проверены `file_transfer_direct` (Unicode direct file, directory move,
occupied Yes/No, self/child refusal), `ui_copy_same` (Shift F5/F6 filesystem,
обычный F5 direct filename и parent creation, Shift F5/F6 с настоящим ZIP
и отличающимся выделением). Полный unsandboxed `./scripts/check.sh`
прошёл 145/145 CTest на macOS arm64/Qt 6.11.2, 2026-10-01.
Windows/Linux runtime и rendered Windows reference локально не проверены.

## Qt 6.12 build/runtime packaging adaptation (2026-10-05)

The engine and SFX still build unchanged from
`https://github.com/ip7z/7zip.git` at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/Console`, `CPP/7zip/Bundles/Format7zF`,
`CPP/7zip/Bundles/Alone2` (Windows), `CPP/7zip/Bundles/SFXCon` (Unix),
and `CPP/7zip/Bundles/SFXWin` (Windows). Existing pinned builders, portable
types and runtime boundaries remain authoritative. Upstream's native build
files do not deploy a Qt application: cross-platform CMake runtime installation,
Qt deployment, artifact filtering and macOS signing are project integration.
No archive algorithm or upstream module is replaced by this packaging glue.

`scripts/release.{py,sh}` and the Windows Qt repository-layout adapter are
transferred from the user-selected htopQt checkout, with application naming and
build-only CI messages adapted. Its Qt 6.12.0/ICU license pins and inside-out
ad-hoc signing sequence are retained. Source archives serve only as private
license extraction inputs. [CI](ci.md) owns artifact scope, source exclusion,
signing limits and release helper behavior.

Qt 6.12 deprecates `Q_OS_MAC`; the macOS-only adaptations in
`src/CPP/7zip/UI/FileManager/PanelOperations.cpp::CPanel::ChangeComment`,
`src/archivebackend_sourceextract.cpp::ArchiveBackend::extractSourceEntry`
and the corresponding comment UI regression now use `Q_OS_MACOS`.
The inspected upstream `CPP/7zip/UI/FileManager/PanelOperations.cpp` retains
the original comment workflow; this change only updates Qt platform detection.
The existing caption and Unicode selection adaptations preserve their behavior.

Test-only deprecated UTC construction now uses `QTimeZone::UTC`; drag fixtures
select `QPointF` on Qt 6.12 and retain `QPoint` for older kits. On macOS the two
DBus fixtures discover the installed libdbus and set their own loader environment:
official Qt kits load that library dynamically, unlike the previous Homebrew
kit. The production application does not acquire a Homebrew path dependency.

Validation on 2026-10-05, macOS arm64 with the official Qt 6.12.0 kit:
unsandboxed `scripts/check.sh` formatting/all-target build and 244/244 CTest PASS
(91.03s). Changed consumers passed 10/10; final runtime-package fixtures passed
again after plugin/ZIP filtering. `actionlint`, release tag validation and
`git diff --check` pass. Release app/test rebuilds have no deprecated diagnostics.
The final 11.9 MiB ZIP contains only the app, Qt Core/Gui/Widgets/DBus frameworks,
Cocoa/style/ICO plugins, pinned engine/module/SFX and notices. A relocated
clean-path launch, engine format discovery, deep/strict signature verification,
source-payload guard and post-signing SFX checksum all pass. Linux/Windows builds
and GitHub execution remain unverified for this change; no release/push ran.
