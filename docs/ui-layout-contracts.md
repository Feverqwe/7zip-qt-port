# Text specifications and geometry checks

### Persisted source locations (P3-05.14, required scope)

Reference: pinned PanelFolderChange SetBookmark/OpenBookmark/FoldersHistory,
App.h focused-panel forwards, MyLoadMenu bookmark popup/dispatch and
ListViewDialog.rc. Keep ten Set/Open slots and original captions/shortcuts.
Saving uses the focused panel's stable source prefix, including virtual archive
links and namespace locations; an empty root prefix follows the original slot
semantics, and opening an empty slot binds the original root namespace.
Materialized temporary engine paths must not enter settings.
Open/history selection bind the stored source location and publish the resulting
panel through existing owners. Source deepest-existing-folder behavior remains
authoritative; previously stored Qt display entries need explicit coverage.
Verify menu availability/handler agreement, both panels, Unicode/spaces/literal
` / `, nested archive close/reopen/restart, history editing/Cancel, operation
guards, unchanged archive/input bytes and no unintended peer navigation. Inspect
EN/RU normal980×620/compact640×400 address/list and bookmark/history widget JSON.
Native device/UNC/ADS and rendered Windows acceptance remain separate checks.
Original BindToPath reuses an already open nested chain. After that chain closes,
CAgentFolder::BindToFolder finds directories rather than opening archive files:
a stored nested location stops at its containing outer archive. Verify this
original fallback on close/restart as well as exact reuse while still open.
History uses its existing540×360/360×240 dialog geometry contract.

### Source root command target (P3-05.13, implementation validation active)

Reference: pinned resource.rc IDM_OPEN_ROOT_FOLDER, MyLoadMenu, App.h inline
OpenRootFolder, PanelFolderChange::OpenDrivesFolder and Panel.cpp list WM_CHAR.
The command/backslash/slash opens the drive list: Windows Computer prefix and
original drives; POSIX retains the documented CFSDrives mounted-volume service.
It closes archive links and uses the focused panel, leaving the peer intact.
Do not substitute the separate root namespace's Computer/Documents/Network rows.
Verify EN/RU normal/compact address/list/status geometry, command and both key
routes, source provider choice, parent/back restoration, selection and input bytes.
Native device/UNC and rendered Windows acceptance remain separate requirements.
The integrated Qt command/key consumers use the retained source drive transition.
The separate root namespace is reached through Parent; Back from a filesystem
location restores its actual stored namespace prefix. After source selection
restoration, same-row Open remains available. Current checks belong to the audit.
Live EN→RU→EN updates both bound drive addresses and the focused window title.
Preserve a pending address draft, existing drive rows and selection; verify both
panels at normal/compact sizes. The header reload uses the source path/icon/title
stage without adding history or enumerating the drive list again.

### Source address dropdown (P3-05.12, required remaining scope)

Reference: pinned PanelFolderChange.cpp::LoadFullPathAndShow,
OnComboBoxCommand and AddComboBoxItem, with RootFolder names and source path
bindings. The dropdown presents the current path hierarchy with indentation,
then the original Windows Documents/Computer/drives/Network entries. Display
names and binding paths are distinct; UNC server and archive-link boundaries
follow the source algorithm. POSIX drive/mount substitutions need an explicit
platform disposition. The visited-path history remains in FoldersHistory.
Selecting a row binds its stored source path and returns focus to the list;
Escape restores the current address. Each panel uses its own current prefix.
Check EN/RU normal980×620/compact640×400 widget JSON, popup order/indentation,
visible text, address/list exposure and keyboard selection/refusal. The former
history-only dropdown did not satisfy this source contract. Current Mac public
selection/EN-RU geometry coverage is recorded in the port audit; native/platform
and rendered Windows acceptance remains open. Escape restores the bound source
prefix, including its native separator, and posts list focus. A newer focus
interaction outside that panel cancels its stale Qt delivery.
When popup closure temporarily clears application focus, the adapter uses the
window's retained focus widget to preserve that guard and the source list request.

The closed editor also shows the current path icon, independently of popup
selection. Reference: the same LoadFullPathAndShow body and SysIconUtils helpers.
Disk folders and archive subfolders show a directory icon; an archive-link root
shows its file icon, with extension fallback for a virtual nested archive. Parent
navigation restores the corresponding outer archive or filesystem icon. Qt uses
a leading QLineEdit action for the original ComboBoxEx edit image; the action and
address/list must remain exposed in all four sizes/languages above. Namespace,
UNC, Windows device and rendered icon/style acceptance still need native checks.

### Source caption helpers (P3-05.10, validation active)

Reference: pinned LangUtils colon/remove-colon/window bodies and actual
EditPage2/ProgressDialog2/BenchmarkDialog resources and OnInit callers.
Translated Benchmark Speed loses exactly one trailing colon; missing entries
retain the full resource caption. Edit viewer retains `&View:` fallback.
Progress packed/files labels use original append-colon policy. Initial and live
reload share these rules, preserving pending input. Benchmark Cancel/Help,
Link Cancel and Progress Cancel use source standard-control mappings; completed
Progress rebinds to IDS_CLOSE. Existing form sizes/order and footer contracts
apply. Current Mac48 EN/RU footer reports pass exposure, size hints, source
captions and separation. Final native seven consumers and48 footer reports also
pass, followed by final native full256PASS. Linux and rendered acceptance remain open.

### Namespace Hash result consumers (P3-05.8, validation active)

`namespace_hash` invokes original PanelCrc/HashGUI results for Windows private
device and ADS fixtures. The existing ListView contract applies: two-column
propertiesList, then listViewButtons, with source OK/Cancel captions and roles.
EN/RU success and recovery inspect540/420×360 reports: rows and footer fully
exposed, vertically separate, buttons ordered without overlap and at least their
size hints. Twenty current native reports pass; rendered pixel parity and final
platform acceptance remain pending. Cancellation produces no successful result.

### Standard source dialog buttons (P3-05.9, accepted button scope)

Reference: pinned LangUtils.cpp::kLangPairs/LangSetDlgItems, GuiCommon.rc and
each dialog's resource/OnInit caller. Selected 7-Zip language controls OK,
Cancel, Yes, No, Close, Help and Continue. Missing entries keep the resource
caption; Memory uses IDCONTINUE. Hot reload preserves roles and pending input.
Options property-sheet/native dialogs retain their platform language owner.
`language_standard_buttons` checks twelve source consumers at their normal/
compact sizes below and produces48 EN/RU footer reports per platform. Reports on all three platforms reviewed
2026-10-06 show visible, separate standard controls and exact source captions;
rendered Windows parity and parent/final acceptance remain pending.

### Source shell popup (P2-09, in progress)

Reference: pinned PanelMenu.cpp::CreateFileMenu/CreateSevenZipMenu and Explorer
ContextMenu.cpp::QueryContextMenu, including source MenuLogo.bmp. System is
inserted before the 7-Zip group when requested; native System is connected on
Windows and unavailable on Unix. Cascaded source Open archive, Open archive submenu, Extract files,
Test archive and Add to archive remain in source query order. Available Open/
Extract/Test/Hash, direct Add to7z/ZIP and SHA256 generation use concrete services;
modal Add uses the source fixed-selection dialog. Windows email commands follow
the same native Add/update capabilities and retained MAPI workflow; Unix email
remains unavailable. The native_shell_email consumer checks all three original
email rows in EN/RU at980×620/640×400, including geometry and Unicode filenames.
No additional separator is inserted by Qt between plugins and CFileMenu: the
corresponding source append-separator block is commented out.
shell_menu_window checks actual popup/independent Open success, forced-type
refusal, password cancel and selection result. It checks each submenu action
rectangle, order and text width at EN/RU980×620/640×400 and writes four
shell-menu-en/ru-980/640 JSON reports. Current macOS submenu bounds are172×115
EN and201×115 RU, fully exposed; their nested Open-as menu is intentionally
hidden until activated. One affected EN submenu PNG was inspected specifically
for the source bitmap mask/rendering. These checks do not establish rendered
Windows pixel parity or complete native-extension UX acceptance.

P2-09.2 System presentation acceptance compares actual pinned Windows7zFM
File/System with the same source panel/provider query and Qt projection:
22 rows at96DPI preserve order, grouping, state and icons. Reference and Qt
PNGs were reviewed, including Share orientation. Both fonts are Segoe UI9pt;
QWindows11Style supplies24px rows/padding/rounded frame versus native22px rows
and9px separators. The adapter copies source GDI icons and forwards native
measure/draw/mnemonic messages for owner-drawn rows, submenus and callback
icons. Failed measurement disables that row. Geometry reports continue to
check EN/RU normal/compact popup visibility. This affected-state comparison
does not establish exact pixel parity or other DPI/platform rendering.

shell_update_window additionally checks direct Add7z/ZIP/generation success,
broken existing archive refusal, selection and result appearance. Its eight
EN/RU980×620/640×400 root/hash JSON reports include QAction texts, source verbs,
order, enabled state and rectangles. Source order is modal Add,
Add to7z, Add toZIP, CRC SHA submenu; the hash submenu preserves the eleven
calculation rows, separator, Generate and Test. Current root bounds287×92 EN/
316×92 RU, hash355×301 in both languages, every visible action fully exposed.

### Shell Menu page (P2-09, accepted)

Reference: pinned MenuPage2.rc and source CShellDll discovery/OnApply.
Native then other-bitness integration checkboxes precede cascaded/icons/root
duplication controls, Zone row and scrollable command list. Missing DLL disables
its checkbox; a 32-bit process without another shell hides only the other-bitness
control. Explicit Apply runs other view first and resets each check to a fresh
registration query, including failure. Cancel initiates no registration.
menu_page checks these states through a fake module/registry service and writes
menu-shell EN/RU640×390/520×380 reports. Four current macOS reports reviewed:
compact498-wide controls accommodate the391-wide RU bitness hint; Zone label
and combo do not overlap, the498×185 RU list scrolls within fully exposed bounds.
The real page uses a native factory; Unix has no Win32 DLL integration, Windows
requires adjacent project modules. Production policy controls and twelve current
EN/RU normal/compact reports across all three systems are reviewed in P2-09.3.
Installed native consumers and actual guest registration are verified in .2/.3;
rendered Menu-page pixel parity remains unverified. P2-09 is accepted; the full
Options container and its initial-page behavior belong to P1-12.2.

### System associations (P2-08, accepted)

Reference: pinned SystemPage.rc and CSystemPage OnInit/ChangeState/OnApply.
Associate label precedes current-user and all-users plus buttons; each button
is centered above its corresponding association column. File type is the first
column, followed by current user and all users; the list grows while the Options
footer remains visible. No selection targets all supported rows. Unavailable
groups/unknown native types show a reason and cannot initiate registration.
All four source controls are mapped in system-page.contract.json; the generated
reference retains source hashes and the exact revision. system_page produces
EN/RU640×390/520×380 reports; ui_parity_system checks their reviewed spatial
relations and visibility with injected services. Four-report comparisons pass
on macOS, Windows and Linux; twelve current reports were reviewed. Compact
Windows list476×248, Linux476×240 and both80-wide buttons are fully exposed;
RU labels fit476-wide controls and footers remain visible. Windows RU Folders/
removable captions wrap to prevent hidden pages enlarging Options. Time-only
ZIP controls wrap at300 pixels; NTFS views retain single-line mtime/ctime and
380-pixel atime wrapping. Four current Windows WIM reports confirm480×430.
Rendered Windows parity and existing Add language clipping remain P3 work.

### Add metadata Options (P2-07, in progress)

Reference: pinned CompressDialog.rc Options NTFS group and COptionsDialog
OnInit/OnOK. Symbolic links precede hard links, alternate streams and security;
original handler flags control visibility. Time controls follow this group,
with OK/Cancel/Help in the footer. Unix hides NTFS streams/security.
source_link_storage writes EN/RU540×480/480×430 reports for actual TAR options.
Four current macOS reports reviewed: visible links and footer bounds equal
exposed bounds; compact checkboxes434 pixels wide exceed RU hints221/197.
This establishes structure/usability, not rendered Windows pixel parity.
source_metadata_storage additionally checks WIM security/ADS and all visible
time captions, Options/Add Cancel/OK/restart, and writes four Windows
`source-metadata-wim-{language}-{width}` reports. Current EN/RU540×480/480×430
reports reviewed: RU compact hint475×423 fits480×430, ADS397×18/security301×18
fit434-wide controls; last-access337×32 fits414×32; footer458×20 fully exposed.
Only the last-access caption wraps additionally for WIM; 3px vertical gaps keep
all source groups and controls reachable at compact sizes without reordering.

### Link (P2-05)

Reference: pinned FileManager/LinkDialog.rc and source OnInit/OnButton_Link.
Link from combo/browse precedes Link to combo/browse and current target text.
Link Type contains Hard→File Symbolic→Directory Symbolic→Junction→WSL in source
order. Link is the default footer button, followed by Cancel. Qt scrolls the
content at compact sizes; desktop-only Junction/WSL remain visibly disabled on
POSIX with a platform reason. Source radio/type choices and errors remain owned
by CLinkDialog; Qt controls bind original captions and LangSetDlgItems maps
IDCANCEL to its source language entry.
Radio captions use the existing220-pixel language wrapping binding so native
Windows EN/RU font metrics do not force horizontal clipping at360 pixels.
`link_dialog` writes EN/RU520×500/360×300 JSON and checks named controls after
scrolling and footer exposure. Twelve current reports reviewed across macOS,
Windows and Linux2026-10-03. Rendered Windows pixel parity remains unverified.

### Benchmark (P2-04)

Reference: pinned `CPP/7zip/UI/GUI/BenchmarkDialog.rc` and its source state bodies.
Dictionary/memory precede CPU threads, then Size→Speed→CPU Usage→Rating / Usage→
Rating columns; Compressing and Decompressing retain Current/Resulting rows.
Elapsed/Passes and Total Rating precede CPU/version/system information. Restart
and Stop sit beside the original log; Help/Cancel stay in the footer. Total mode
has elapsed time and a fixed-font console. Qt scrolls the normal control pane at
compact sizes and replaces Win32 rectangles/font setup with Qt layout services.

`benchmark_lifecycle` writes normal/total EN/RU JSON at960×650 and640×400,
checks all named labels/inputs after scrolling, fixed footer exposure and live
language changes. Current reports on all three OS were reviewed2026-10-03: normal compact
controls scroll vertically/horizontally, metrics captions fit; buttons remain
fully exposed. Total console is618×326 compact and938×576 normal. Cancel/Help use
the original standard language mapping402/409; control IDs2/9 are not language
IDs. No screenshot or rendered Windows reference has been used to claim pixel
parity. Windows spanning group/error/system labels retain natural width; wrapped
source text updates also set its required height, since a grid span's height
calculation otherwise assumes the wider span. The Windows console is618×332
compact/938×582normal; Linux618×330/938×580. Lower system rows are expected below
the initial compact viewport and are tested after scrolling. P2-04 acceptance
is recorded in the audit; whole-plan platform/rendered checks remain P3.

### SFX Options group (P2-02)

Reference: pinned `GUI/CompressDialog.rc` Options control order SFX→Shared→Delete,
followed by Encryption, and original CompressDialog naming/method decisions.
The Add form retains its two columns and common scrolling content. New archive
SFX is enabled only with a compatible native module, 7z format and source-supported
method. Changing to ZIP clears SFX; toggling restores the source archive extension.
Module absence and existing-target update disable SFX with a reason.

`sfx_creation` writes `sfx-options---760/640` and `sfx-options-ru-760/640` JSON,
checks exposed controls/order and checkbox size hints at760×720/640×480, and
checks source name/format/method transitions. Reports reviewed2026-10-03 on
macOS, Linux and Windows:
all Options captions fit; Encryption below it uses the existing compact scroll.
Windows show/layout events settle before ensuring scroll visibility. Its EN
checkbox widths385 and RU421 meet all size hints; exposedBounds equal bounds
at normal/compact sizes. This is resource structure/usability evidence;
rendered 7zFM pixel parity remains unverified. P2-02 acceptance is in the audit.

P2-02 About integration restores original codec-error message initialization.
The retained catalog supplies source GetCodecsErrorMessage; a Qt critical message
substitutes the original MessageBoxW/MB_ICONERROR. `help_source` checks exact source
text and exposed/wrapped label with600×300/400×250 resize requests and writes
`about-codec-error-600/400` JSON. About's local app caption now says official7-Zip
engine; EN/RU360×330/300×310 JSON was reviewed with visible/fitting text. No
rendered Windows reference is claimed for either standard Qt message or caption.
The standard QMessageBox auto-fit resolves both requests to400×121 on this host;
these reports prove fitting source text, not two distinct error-window sizes.

For resource-derived comparison and reviewed C++ states, see
[upstream UI references](ui-reference.md) and `$sz-ui-parity`.
`ui_parity_copy` and `ui_parity_password` check fresh reports against pinned
resource baselines.

### File-list drag target (P1-16 WIP)

Reference: pinned `PanelDrag.cpp::PositionCursor`, its `LVIS_DROPHILITED`
paint state, native right-drop popup pairs/masks and Copy-to-archive confirmation.
Qt preserves per-panel path→view→status order. Hover highlights the folder through
a separate paint role without changing selection; DragLeave/Drop removes it.
Source-panel and disabled-view drops are rejected. Hover/SingleClick stays
suspended during the source operation, including archive byte preparation.
`drag_drop_ui` covers table/icon targets in both directions, popup Copy/Cancel,
archive Yes/No/Cancel and real source cancellation. Reports `drag-*.json` at
980×620/640×400 keep both containers, paths, current views and status strips
exposed. The reviewed resource/source establishes structure and states; no
Windows rendered reference is available for pixel fidelity.

For layout work, read the affected view's specification below, run its focused
test, and inspect the generated JSON. Screenshots are a fallback for fonts,
icons, styling, or defects that geometry cannot explain. These checks establish
structure and usability on the tested Qt platform, not pixel-level Windows parity.

## Commands and reports

### Alternate streams namespace (P2-06, active)

Reference: pinned FileManager.rc IDM_ALT_STREAMS and
PanelFolderChange.cpp::OpenAltStreams, AltStreamsFolder properties and original
Agent CProxyArc2 folder/parent binding. Keep the existing main shell and file
list order: address, Name/Size/Packed/etc. columns and per-panel status. A colon
folder prefix names an ADS namespace, not a filesystem subdirectory. Stream
rows are files; no synthetic Unix xattr rows. Parent returns to the original
file owner. Filesystem CreateFolder/import remain disabled because their
original AltStreamsFolder methods return E_NOTIMPL.

`alt_streams_command` writes Windows filesystem `alt-streams-{language}-{size}`
JSON in EN/RU at 980×620 and 640×400. `alt_streams_archive_ui` writes
`archive-alt-streams-{language}-{size}` for the native archive namespace at the
same sizes. Both require the address and table to remain fully visible.
Current macOS/Linux/Windows archive and Windows filesystem reports retain
toolbar→address→table→status order, with address, complete table and status
exposed at both sizes in both languages. Sixteen current JSON were reviewed.
These checks establish Qt structure/usability, not rendered 7zFM pixel parity.

### Add archive naming metadata (P2-01)

P2-09 WIP correction 2026-10-03: desktop `IDD_COMPRESS` places
Format/compression/memory/volumes/parameters/Options summary in the left column;
Update mode/Path mode/Options group/Encryption group are in the right column.
There is no Compression group box. `source_compress_dialog` tests these relations
and produces EN/RU760×720/640×480 top/bottom `source-add-*.json` reports.
Ordinary filesystem Add also forbids an embedded source picker: the actual panel
selection supplies its sources, checked by `filesystem_selection_runtime`,
`ui_operations` and `ui_geometry`. Archive folder is now a read-only label above
Archive+`...`; extra Save in/preview and Create/Add footer captions were removed.
All dialog branches now omit the source picker. Toolbar Add uses original
CPanel::AddToArchive; archive updates also use original two-panel Copy.
Remaining menu aliases are in [the mandatory UX audit](ux-port-audit.md).
macOS focused12/12PASS11.95s
includes the folder/footer correction. Eight current source-add top/bottom
reports were reviewed: normal columns/control groups fully exposed, compact
vertical scrolling without horizontal overflow, lower parameters and fixed
footer visible at the bottom. Qt output has no source picker/Save in/preview.

Current Archive header additionally follows IDT_COMPRESS_ARCHIVE_FOLDER and
IDC_COMPRESS_ARCHIVE's shared xArcFolderOffs: prefix and combo/browse have the
same left edge to the right of the Archive caption. The field is two lines;
prefix can wrap at compact sizes without imposing horizontal overflow. Display
separators are native; the read-only prefix does not alter source path state.
`source_compress_dialog` checks that shared edge in EN/RU normal/compact states;
the regression fails on the previous spanning-prefix layout. Regenerated
reports were reviewed at both sizes/languages: top controls are exposed, compact
prefix may wrap, and lower controls/footer are reachable after scrolling.
Initial filename text is fully selected, reflecting the original modal's first
editable combo; selectedText tests fail without the explicit Qt focus adapter.

The actual Windows reference additionally places the Archive caption between
the prefix and name rows; a vertically centered header layout now preserves
that relation even when the prefix wraps. IDT_COMPRESS_MEMORY_DE has its own
full-column row, rather than widening all preceding compression labels.
The six g1x/g1xs combo rows share their left edge and width in EN/RU at both
sizes. The new compact caption regression fails before the correction.
Six direct suites pass9.06s on macOS; eight regenerated top/bottom JSON reports
were reviewed: EN compact combo width148 versus level hint108, RU188 versus145,
caption between prefix/name, full-column memory and reachable lower fields/footer.
Fresh Windows/Linux runtime checks and rendered comparison remain required.
Latest affected6/6PASS9.39s; fresh full/VM/rendered checks remain pending.
The real Windows 7zFM52-control runtime at96DPI was captured and inspected;
its JSON/PNG provenance is recorded in parity. Native Windows Qt EN/RU normal/compact
rendering was compared with that reference; this identified omitted limit/RAM text
and an extra inline compression-memory label. The source PrintMemUsage text and
stacked label/value row are corrected. Eight regenerated macOS reports show the
complete usage/limit/RAM text and exposed fields at the bottom; focused5/5PASS3.14s
includes source Delete success/Cancel/refusal. The latest fresh snapshot passed
Windows13/13 in65.39s and Linux13/13 in25.17s; macOS full227/227PASS81.24s.
The nested-transfer focus restoration failure was corrected in the Qt adapter.
Eight current native Windows reports were reviewed: full usage/limit/RAM text
fits; normal groups fit, compact lower parameters become fully visible after
vertical scrolling, while top rows leave the viewport. Fixed footer stays visible.
Offscreen Windows box-font metrics are not the application's font metrics;
the thirteen focused Add/consumer tests use the native Windows plugin. This is not
whole P2-09 acceptance.

Actual application-runtime Add was also opened from the toolbar on Windows,
with the fixture selected in the file panel. The2026-10-04 capture is
`build/runners/ui-reference-20261003/qt-application-add-v8/application-add.png`
and adjacent JSON; app SHA256
`473f207e78581b6427ed91e9ec66db3e9e4da55a05c4a0748278ec48d94a5ef`,
snapshot `20261003T212905Z-62e3141a`. The PNG was inspected against the original
7zFM runtime: panel-selected sources, columns and complete memory text are
present, but the retained760×720 default extends the footer behind the taskbar
on this screen. This is a real application defect absent from explicitly resized
form tests. Content-based default sizing and a screen/footer regression passed
locally (default640×571, all controls/footer exposed). Fresh Windows13/13PASS70.86s;
the inspected actual application v9 capture shows642×578 and the full footer on
the available screen. Adjacent JSON records app SHA256
`c4d9055f35ed16398677f2ed3815573d18579818a94a846c259e2c13c0c6b712` and
snapshot `20261003T221959Z-6da11f81`; files are in
`build/runners/ui-reference-20261003/qt-application-add-v9/`. The v8 capture predates
the Copy focus fix and is not the latest cross-platform validation snapshot.
Both captures predate the subsequent CPanel Add workflow port; that change still
needs new consumer/platform validation. No remaining Add or global UX closure
is inferred from the sizing comparison.

Reference: pinned CompressDialog.cpp/rc and UpdateGUI.cpp::ShowDialog format/name
inputs. Metadata projection retains Archive→Format→Path mode grouping and source
format-switch naming rules. `compression_path` checks actual native default
extensions/KeepName/history aliases, detected ZIP with unknown target suffix and
missing-module disabled confirmation. Its archive-path760/640 JSON uses a real
catalog and exposes the path/format controls after scrolling to the top. Current
reports reviewed: native eligible transport choices, separate rows and
unclipped inputs/scroll viewport. `language_ui` EN/RU760/640 reports check the lower
scroll state and exposed preview/footer; upper controls are expected offscreen
there. Native one-file/name inputs follow the fixed panel selection; the reviewed
current metadata form uses the native Archive.7z suggestion.
`shell_update_window` writes14 actual toolbar Add reports,
`source-toolbar-add-<type>-760/640`, for creation/Cancel of seven native formats.
`codec_commands_ui` writes14 original two-panel Copy reports,
`codec-copy-<type>-640/480`, for unknown-extension archive update/Cancel.
The obsolete opened-archive Add picker reports do not describe the current UI.
2026-10-04 current14 Add/14 Copy JSON reviewed: Add760×720/640×480 keeps
left/right grouping, method label/combo centers aligned with a6px gap and
decompression values in their right field. Copy640×400/480×320 destination,
selection summary and footer are exposed. No visible label is clipped. Compact
top-state generic reports can place lower encryption offscreen; scroll-state
reports and geometry tests check reachability. macOS focused16/16PASS14.41s;
fresh Windows/Linux and full checks are pending.
Its new report coverage is English; existing shared EN/RU layout checks remain.
resources/geometry do not establish rendered Windows fidelity.

### Open archive picker (P2-01)

Historical P2-01 extension, removed2026-10-04 during P2-09 UX correction:
original IDM_MENU has no independent Open archive/Ctrl+O command. MainWindow's
picker action/consumer is removed; source Open/Open Inside and address navigation
remain. BrowseDialog/codec filter unit checks still validate their reusable
component, and do not imply that an extra File-menu entry is justified.

Reference: pinned BrowseDialog.cpp/CBrowseInfo and FilePlugins.cpp extension
database. Qt uses the native OS file picker; archive masks project matching native
handler metadata followed by All files, or only All files when metadata is absent.
The picker retains filter order/index, Unicode paths and .001 selection.
`browse_dialog` checks EN/RU filter presentation and selection with explicit test
fixtures; `codec_catalog` checks the actual metadata masks and failure fallback.
The OS-owned picker is a documented platform adaptation, not a Windows pixel
reference. Actual MainWindow startup/error/command integration and full P2-01
gate passed188/188(74.09s), macOS arm64 Qt6.11.2 2026-10-02.

### Delete Temporary Files (P1-15.2)

Reference: pinned BrowseDialog2.cpp/rc (dialog93), six Name/MTime/Size/Files/
Folders/Name-2 columns. Controls retain Delete/Refresh, Parent/read-only path,
list, disabled filter, Help/Close ordering. Owned membership replaces upstream's
7z* convention; active or unconfirmed external entries remain visible with an
explanatory tooltip and disabled Delete. Enter navigates, Backspace returns,
Delete confirms eligible deletion, F5 refreshes, F1 opens fm/temp.htm, Escape
and Close exit without deletion. No push button is an Enter default.

temp_cleanup writes EN/RU900×580/500×320 temp-cleanup-*.json; footer/path/filter
stay exposed, and the six-column list scrolls horizontally at compact width.
Geometry and source structure are checked; rendered Windows parity is unverified.
All four current reports were reviewed at acceptance: named controls are fully
visible at both sizes. Integrated P1-15 check.sh passed175/175 on macOS arm64;
Windows/Linux runtime remains unverified.

### Offline Help and About (P1-15.1)

References: pinned HelpUtils/MyLoadMenu/CompressDialog topic calls and exact
official26.03 CHM HTML/CSS with release provenance in resources/7zip.
QTextBrowser replaces Win32 CHM presentation, retaining pages/links/fragments.
help_source writes EN/RU760×560/420×300 help-*.json: browser, Back and standard
Close remain exposed. AboutDialog.rc/source is the structural reference for
About360×330/300×310 EN/RU: logo, version/date/copyright/free-software, footer
OK/homepage. Added application version/license link are explicit adaptations.
help_source writes about-*.json; all text fits and controls remain exposed.
help_ui checks actual Contents/F1/modal About, source Options page callbacks and
Add/Extract button/F1 without applying pending state; compression_advanced checks
original Options Help. Existing Add760/640, Extract600/470 and Options640/520
geometry reports cover the added Help footer. Current Qt usability is checked;
rendered Windows parity remains unverified; P1-15 acceptance includes cleanup above.

### Live dialog language consumers (P1-14.2)

References are pinned Password/Copy/Split/Overwrite/Mem/Messages RC/source,
GUI ExtractDialog.rc and CompressDialog/CompressOptionsDialog resources.
`language_dialog` writes EN/RU normal/compact `language-*` JSON: password360/260,
copy/split/folder480/300, overwrite/memory/messages700/560, extract600/470.
It verifies text bounds and pending state across hot switch. `language_ui` adds
Add760×720/640×480 and advanced520×400/440×340. Add content uses its existing
scroll viewport; the footer remains exposed. Advanced archive-time checkbox
uses Qt text wrapping for the original long source words at compact width.
Reachability of all scrolling Add controls, current message forms and source
accelerator/buddy review form the accepted P1-14.2 scope. The final full gate
passed170/170 on macOS; these reports do not establish rendered Windows parity.
Progress reports cover EN/RU760×450/640×380 paused/background state and retain
source two-column grouping. Properties reports cover540×360/360×240 with the
existing table, rows and selection. Built-in viewer/editor reports cover760×560/
420×300 while preserving pending content; editable text is omitted from JSON.

P2-09 correction2026-10-04: toolbar Info uses the original IDM_PROPERTIES
consumer; the independent archive-summary screen is removed. Four current
`language-toolbar-properties-{en token -|ru}-{540|360}.json` reports retain
the source properties list/button grouping with full viewport and footer
exposure. The same source dialog shows archive/folder properties with zero
operated indices, tested through actual Info on an empty archive. These checks
validate source workflow/Qt geometry, not native Windows System Properties
or pixel parity.
`native_archive_properties` adds `native-error-type-{zip,7z}-{540,360}.json`:
the original Error Type row presents the rejected native handler while the
archive's Type remains the accepted handler. The same properties list remains
fully visible at540×360 and360×240. This checks values/grouping/geometry, not
rendered Windows pixel parity.
Split confirmation reference: pinned PanelSplitFile.cpp's IDs7304/7305 and
MB_YESNOCANCEL. `volume_workflow` checks original EN/RU100-part text and decisions
with normal1100×640/compact640×380 main windows. Twelve
`language-split-confirm-{en|ru}-{1100|640}-{1|2|3}.json` reports show the Qt managed
400×116 popup with full message and Yes/No/Cancel exposed on macOS. QMessageBox
ignores its title on macOS; Windows/Linux retain the title check but their
runtime and rendered parity remain unverified.
Archive preview-limit is a local bounded-viewer integration message, with no
equivalent native bounded-editor resource. `language_archive_command` checks
EN→RU→EN text/buttons at main-window1100×640/640×380; four
`language-archive-preview-*.json` reports show a complete400×116 Qt popup.
It also retains selected rows and verifies no extraction or listing restart.

Compression RAM rejection references the original CompressDialog.cpp
SetErrorMessage_MemUsage body rather than a separate resource dialog.
`language_archive_command` checks zero-limit rejection EN→RU→EN at Add760×720/
640×480, retaining pending values. Four `language-compress-memory-*.json`
reports show English400×226/Russian400×256 with full message/buttons. Dictionary,
memory and solid geometry tests expose their affected controls and write current
760×720/640×480 JSON with source B/KB/MB/GB and `*  ` values; other Add controls
retain the scrolling reachability requirement. Association error reports check
local EN/RU plaintext paths and full400×116 message/buttons. Progress cancellation
uses Qt platform standard button captions with original Yes/No/Cancel decisions.
No rendered Windows reference or cross-platform verification is claimed.

Add's language test now exposes every named atomic control by scrolling at both
sizes and checks full exposure, label/button widths and persistent footer.
`language-add-*.json` records its final bottom position; controls above the
viewport are verified at their own scroll positions. `language_focus` checks the
source RU Show-password/Parameters collision and composite CPU/RAM label buddies:
Tab reaches both controls, mnemonic cycling is checked when enabled by the OS,
and focus proxies reach the real inputs. Source RC fallback retains original
English accelerator positions when a translation is missing. Root language
reports at980×620/640×400 retain the same table/items/selection and source names;
the isolated `root_navigation` startup directory contains their current JSON.

### Menu page (P1-13.1)

Reference: pinned `FileManager/MenuPage.cpp`, `MenuPage2.rc` and
`Explorer/ContextMenuFlags.h`. Options page 2300 is titled 7-Zip and keeps source
order: two shell registration checkboxes, Cascaded, Icons, Eliminate duplication,
Zone label/combo, Context menu items label and the 14-row checkable list.
P2-09.3 now enables Cascaded/Icons/ElimDup and the list for the retained source
in-app consumers. Windows registration controls discover adjacent original-name
7-zip.dll/7-zip32.dll/7-zip64.dll and disable missing modules; Unix registration
stays unavailable with an explicit capability reason. Zone is
enabled on Windows; on POSIX it is disabled with a Windows-only explanation.
The source default is `* No`; existing custom integer values remain selectable.
Qt scrolls the list and uses a persistent OK/Cancel/Apply footer.

`menu_page` writes `options-menu-640/520.json` at 640×390 and 520×380; checkboxes
fit their text, grouping/order and Zone label/combo separation are checked, and
the list and footer remain exposed. No rendered Windows reference is available;
geometry checks establish Qt structure/usability on the locally tested OS only.

### Advanced compression options (P1-07.1)

Reference: pinned `GUI/CompressOptionsDialog.rc`, `CompressDialog.rc` parameter
and Options rows, and `CompressDialog.cpp::COptionsDialog`. Add shows Parameters
after volumes, then an Options button with the native summary. At 760×720 and
640×480 the form scrolls to these controls while the confirmation footer remains
visible. `compression_advanced` writes `archive-advanced-760/640.json`.

Options keeps resource grouping/order: optional NTFS controls, format time info,
Time group with explicit/default precision and modification/creation/access/time
of archive rows, preserve-access checkbox, then OK/Cancel/Help. Explicit/default
checkboxes have accessible names. Native capability policy hides unsupported NTFS
controls; ZIP UNIX/DOS precision hides creation/access rows. Archive mtime is
enabled. Preserve access is enabled on Windows and disabled with a reason on
POSIX; Help remains disabled pending P1-15.

`compression-options-{zip|7z}-{520|440}.json` covers actual 520×400 and 440×340
sizes, exposed controls and footer. Current Qt reports show small explicit-value
checkboxes beside their labels, no clipping and separate confirmation buttons.
Source visibility is applied before Qt computes minimum size so hidden NTFS
controls cannot enlarge the compact dialog. No rendered Windows reference was
available; these checks establish structure and usability on macOS only.

### Compression lifecycle and memory rejection (P1-07.2)

Reference: pinned CompressDialog.rc right Options group (Shared then Delete,
before Encryption), CompressDialog.cpp OnOK action/memory checks and
CompressOptionsDialog.rc archive-time/preserve controls. Qt keeps the right group
for existing Add even when encryption fields are omitted. Shared writing is
disabled with a Windows-only reason on POSIX. Delete starts unchecked, is enabled
in Add, and disabled with a reason in Update/Freshen/Synchronize. A checked value
survives switching modes in the open form but is excluded from disabled effective
requests and never persisted. SFX remains owned by P2-02.

`compression_advanced` writes archive-lifecycle-760/640.json at 760×720/640×480:
group and both checkbox texts fully fit, Shared precedes Delete, footer remains
exposed. Compact form uses scrolling; controls outside the viewport are reachable.
`compression_limit` writes memory-limit-760/640.json after exposing the native
memory combo. Both sizes fully expose it. A memory rejection leaves Add open for
retry, while Cancel leaves INI unchanged. Reports verify macOS Qt usability and
resource order; actual RAR decoder memory requests remain P1-18.

### Archive volume creation (P1-06.1)

Reference: pinned `GUI/CompressDialog.rc::IDC_COMPRESS_VOLUME` and source
`CompressDialog.cpp::OnOK`, `SplitUtils.cpp::AddVolumeItems`. The editable
«Разбить на тома, байт» combo follows compression controls, has nine source
presets and starts empty. Existing-target mode disables it. Invalid input keeps
the dialog open; final sizes below 100 KiB require Yes/No/Cancel before saving.
`archive_volume_creation` checks these states and scroll reachability at
760×720/640×480, writing `archive-volumes-760.json` and `archive-volumes-640.json`.
Current reports show the label/combo fully exposed with the footer below the
scroll area; compact layout uses scrolling. Windows rendering is unverified.

### Existing archive Add options (P1-05.1)

Reference: `CompressDialog.rc` update/path rows and `CompressDialog.cpp`
initialization at upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
The obsolete separate existing-target form and its source picker have been
removed. Add uses the same source form with editable archive/format/browse,
volumes and encryption; source UpdateArchive owns existing-output eligibility.
`compression` checks the mode list, normal
760×720 and compact 640×480 footer/scroll reachability, then writes
`update-options-normal.json` and `update-options-compact.json`.
Current path/password checks are covered by the source modal and native
two-panel Copy consumers. These reports verify structure and clipping on the
checked host; a passing geometry check alone does not verify Windows rendering.

The main-window caption is native CApp::RefreshTitle's focused folder prefix,
with `7-Zip` for an empty prefix. Toolbar option checks mirror native CApp flags;
existing grouping and action order stay in the main-window normal/compact reports.
Command availability comes from native menu policy plus transport restrictions.
Screen-size menu filtering uses available monitor geometry and Qt system-font
units, independently of the main window's compact resize fixture.

### Main menu and toolbar (P0-09.1)

Reference: `CPP/7zip/UI/FileManager/resource.rc::IDM_MENU`,
`App.cpp::g_ArchiveButtons/g_StandardButtons` and `MyLoadMenu.cpp` at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Six top-level menus follow
File/Edit/View/Favorites/Tools/Help. File follows the RC command groups, then
one Qt archive-workflow group before Exit. Unsupported source commands remain
visible disabled. Archive and Standard toolbar groups use the seven exact
large/small BMP pairs and source mask; text/size/visibility derive from the
native toolbar flags. `main_shell` asserts menu action order, IDs, shortcuts,
enabled state, active-panel routing and resource icons, then writes
`main-shell-980x620.json`, `main-shell-640x400.json` and File/View popup reports
for both sizes. Qt QMenu may scroll when its source command list exceeds the
available screen; JSON records its geometry and action tests cover non-widget
QActions. The Windows rendered menu is not yet available for pixel comparison.

### Per-panel context and status (P0-09.2)

Reference: `Panel.cpp::Create/OnSize` gives every panel a status strip below
its list with four parts (`{220, 320, 420, -1}` Win32 boundaries), while
`PanelListNotify.cpp::Refresh_StatusBar` supplies selected/total, selected
size, focused size and focused date. `PanelMenu.cpp::OnContextMenu` calls
`CreateFileMenu(programMenu=false)` and anchors keyboard invocation to the
focused item. Qt uses four labels inside each panel status bar, with adaptive
widths for compact two-panel layout; the main-window bar displays operation
messages. `panel_shell` checks all four labels visible and independent at
980×620 and 640×400 and writes `panel-shell-*.json`. It also writes disk and
archive context popup JSON at normal/compact sizes; QActions are non-widget
objects, so their order/enabled state is asserted separately. Disabled shell
groups are explicit pending integrations. The upstream Win32 resource/source
establishes structure, not rendered pixel dimensions on macOS.

### Two disk panels (P0-08.1)

Reference: FM `App.cpp::SwitchOnOffOnePanel`, `FM.cpp::MoveSubWindows`,
`PanelKey.cpp::OnKeyDown` and `resource.rc` View → 2 Panels (F9) at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. The panels sit left/right
under the shared toolbar, with an address row above each file list. F9 hides
the inactive panel and expands the active one; Tab moves list focus between
visible panels. `panel_key` writes `two-panel-disk-table-640.json`,
`two-panel-disk-980.json`, `two-panel-disk-640.json` and
`one-panel-after-f9-640.json`. It checks both addresses and active lists are
fully visible at 980×620 and 640×400. JSON confirms Qt structure and clipping
only; rendered Windows parity remains unverified.

`panel_folder_sync` extends this contract with Alt+Up (same folder) and
Alt+Left/Right (focused child or parent). Source focus remains in its panel;
the destination's pending list is disabled until source indices, sorting and
selection are coherent. Its address remains available during Scan. Commands
on the ready source panel remain available. Each panel publishes independently;
only destination replacement or close cancels that destination's pending list.
Current reports `alt-folder-{same-small,sub-left-small,sub-right-large}-{980,640}`
and `alt-archive-{small,large}-{980,640}` check both address rows and lists.
For a directly shared native archive folder with no destination parent stack,
the address presents its native relative folder prefix without a leading
archive separator. The archive resource retains the engine path separately.
Rendered Windows parity for these states remains unverified.

### Mixed and restored panels (P0-08.2)

The same 7zFM panel structure applies when either side displays an archive.
`panel_key` writes `two-panel-archive-980.json`,
`two-panel-archive-640.json`, `archive-hidden-panel.json`,
`two-panel-restored.json` and `hidden-left-restored.json`. Geometry checks
require both visible address rows and lists to fit at 980×620 and 640×400,
and check the saved splitter coordinate after reopening. A compact QSplitter
clamps its coordinate to the panels' minimum sizes. JSON covers Qt grouping
and clipping; rendered Windows parity remains unverified.

### Virtual root main window (P0-04.2)

Reference: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/RootFolder.cpp`, `PanelFolderChange.cpp`,
`PanelKey.cpp`, `resource.rc` View menu. The native root has an empty address
and `7-Zip` caption; the list contains source-provided item names and indices.
Parent is disabled at the root, enabled at POSIX `/`, and mutations are disabled
on the root provider. The usual menu, toolbar, address, list, status order stays.
`root_navigation` checks visibility without clipping at 980×620 and 640×400 and
writes `macos-root-980x620.json`/`macos-root-640x400.json` on the checked host.
The JSON verifies Qt structure/usability; source resources establish grouping,
not Windows pixel appearance. Windows runtime capture is still unverified.

### Mounted volumes (P1-11.1)

On macOS/Linux the virtual root adds «Тома» after «Компьютер». The mounted
volume list has the source `CFSDrives` name/properties, the normal menu → toolbar
→ address → list → status order, and disables filesystem mutations until a
volume opens. `root_navigation` writes current-build
`macos-mounts-980x620.json` and `macos-mounts-640x400.json`. At 640×400 the
toolbar, address, table and panel status are fully visible; hidden optional
status segments are not counted as clipping. Windows drive/resource layout
still needs runtime verification.

### Windows device namespace and F5 Copy (P3-05.7)

Reference: pinned RootFolder/FSDrives provider, PanelKey/App::OnCopy and the
existing resource.rc main shell/CopyDialog.rc contracts. Device rows are files
by source kpidIsDir; Copy is available, Move and filesystem mutations disabled.
The same menu → toolbar → address → list → per-panel status order remains.
`fs_drives_device_ui` writes sixteen EN/RU success/Cancel current-build reports:
namespace980×620/640×400 and Copy640×400/480×320. Native Windows Qt6.12.0 reports
reviewed2026-10-06: compact address537×22/list632×227, panel status632×21 are
fully visible. Compact Copy places label above path340×22/Browse81×26, selection
458×212 below it and separate OK/Cancel81×26 at y283; all remain inside480×320.
The source device properties use TotalSize, so the existing status Size field
is not an independent device-length report. Hidden menus are not clipping.
These reports verify native Qt grouping/geometry, not rendered 7zFM pixel parity.

### Network namespace (P1-11.3)

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`RootFolder.cpp`/`NetFolder.cpp` source hierarchy. On macOS/Linux «Сеть» is
the third root entry and lists already mounted network shares. The menu →
toolbar → address → list → status order remains. Back and Parent are enabled
inside this namespace; mutation commands are disabled. `root_navigation`
writes `macos-network-980x620.json` and `macos-network-640x400.json` and
checks address and list visibility at both sizes. These reports establish Qt
geometry, not Windows pixel appearance; live shares and Windows rendering
remain unverified.

P3-05.11 restores the original Windows server/container address fallback.
`native_network_navigation` enters an owned local server in EN/RU normal/flat
states, opens only its private share and returns with source focus. Eight
`network-server-{language}-{flat}-{width}.json` reports at980×620/640×400
check menu/toolbar/address/list/status order, full exposure and size hints.
Source-backed binding chooses the namespace even when Qt classifies its address
as a directory. Copy and mutations remain disabled without source operations;
Parent/Back stay available. Current native reports pass. Owned local SMB bytes/
refusal are proven by public tests; external shares/credentials and rendered
Windows pixel parity remain unverified.

### Source extraction prompts (P0-04.1 prerequisite)

`extract_prompt` produces OverwriteDialog reports at 700×440 / 560×440 with
extra buttons shown/hidden, and MemDialog reports at 700×460 / 560×460 with
limit editing disabled/enabled. `ui_parity_overwrite` / `ui_parity_mem` run
that current-build producer fixture before comparing pinned RC contracts.

Overwrite order is destination notice, question start, old file icon/details,
question end, new file icon/details, then two rows of Yes/Yes to all/Auto rename
and No/No to all/Cancel. Source ReduceString, byte size and timestamp formatting
remain native. No is the default when requested; extra actions hide together.
Qt supplies one file icon; the optional second Windows EXE/ICO icon is omitted
explicitly in the contract. Widths must fit labels and footer buttons without
clipping. Default and native result IDs are checked behaviorally.

Memory order is native requirement/limit/RAM message, change-limit checkbox,
independent edit/spinner and GB label, Action group containing Allow/Skip and
Remember, Continue/Cancel. Source RAM-dependent default and spinner range,
text validation, NeedSave/Remember/SkipArc and cancellation are preserved.
The text field remains independent of the spinner range, including source
acceptance of zero; invalid text does not close the dialog. Qt spinner buttons
adapt the original edit/up-down pair. These checks establish structure and local
Qt usability. Native PanelOpen integration is checked in P0-04.1; Windows
rendering remains unverified.

```sh
./scripts/build.sh dev ui_geometry_tests
ctest --preset dev -R '^ui_geometry$' --output-on-failure
```

CTest writes reports to `build/dev/ui-reports/`. `tests/layout_inspection.h`
also supports other Qt tests through `UiInspection::writeReport(widget, tag)`.
`SEVENZIP_UI_REPORT_DIR` overrides the output directory; standalone test runs
default to `ui-reports/` in the current directory. `ui_layout`, `extract_dialog`,
and `compression_preferences` now write JSON instead of mandatory PNGs.
Older screenshot-producing tests have not all been migrated.

Each JSON object contains class, object name, visible/enabled state, bounds in
root coordinates (`[x,y,width,height]`), exposed bounds after ancestor clipping,
size hint, and nested children in Qt object order. Static labels, button text,
group titles, and noneditable combo selections are included. Editable values
are omitted, including passwords in show-password mode. Static labels can still
contain generated paths; reports use disposable fixtures and are local artifacts.
Object-tree order is not tab order: use geometry for spatial relationships.

`fullyVisible` checks ancestor clipping, not occlusion by another sibling or
window. `separate` checks specified pairs; container/child overlap is intentional.
`labelFits` uses Qt size hints/height-for-width, not pixel analysis. Scrolled
content can be clipped until brought into view. Avoid blanket overlap or
visibility assertions on the entire widget tree.

For an unresolved rendering question:

```sh
SEVENZIP_CAPTURE_SCREENSHOTS=1 ctest --preset dev -R '^ui_geometry$' --output-on-failure
```

This writes optional PNGs next to the JSON. Open only the affected screenshot.

## Archive metadata cells

Reference: pinned `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/PanelListNotify.cpp` property-to-text branches and
`CPP/7zip/UI/Agent/Agent.cpp` typed GetProperty. Unknown size/packed metadata
must leave the corresponding file cell empty; defined zero must display `0`.
Directory size cells retain the current empty presentation. The hidden transport
columns, view modes and navigation grouping are unchanged. P0-06.2 adds the
source property catalog to the visible table columns.

`panel_selected_state` checks normal/flat archive cells in all four views at
980×620 and 640×480, verifies central/active view visibility, and writes
`archive-runtime-mode*-flat*-*.json` under `build/dev/ui-reports`. Review geometry,
control order and clipping in those current-build reports. These establish Qt
usability/typed value behavior, not rendered Windows pixel parity. The later
`panel_columns` reports cover source property columns and their header controls.

For recursive filesystem loading, `flat_view` also writes normal/compact
`filesystem-native-scan-*` and `filesystem-presentation-*` reports. Navigation
remains above the disabled list while loading/presenting, and both stay visible
without clipping. Status distinguishes scan progress from shown row count; list
commands return after completion or failure. This Qt responsiveness adaptation
keeps source enumeration in CFSFolder, not a presentation-layer walker.
P0-05.1 reads displayed normal/flat names and prefixes from bound CFSFolder
properties, with NFC only for Qt text. The same 980×620 and 640×480 current-build
`filesystem-native-scan-*` / `filesystem-presentation-*` reports check visible
toolbar, address, list and status during scan/presentation; they do not prove
rendered Windows parity or final large-scan responsiveness (P0-05.2).

`panel_selected_state` writes normal/flat `filesystem-runtime-*` reports at
980×620 and 640×480. Filesystem size cells use native typed metadata: uncalculated
directory size is absent, file zero is defined, and POSIX links display their own
length. Source item indices are independent of sorted rows. Path controls, shared
view stack and status bar keep the existing order and must remain fully visible.

### Source sorting in the file list (P0-06.1)

Reference: pinned `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/PanelSort.cpp::CompareItems/CompareItems2` and
`PanelItems.cpp` list state. Directories group before files in either direction;
the chosen typed property sorts first, then base name, flat prefix and source
index. Source `kpidPrefix` BSTR comparison is lexical, so equal `common.txt`
names under `dir10` and `dir2` place `dir10` first. Header and sort action
remain in sync. The list, address, toolbar and status keep their existing order
and stay unclipped at 980×620 and 640×400. `panel_sort` writes
`source-sort-flat-980.json` and `source-sort-flat-640.json`; these are current Qt
structure reports, not a rendered Windows comparison. P0-06.2 covers raw-column
sorting and the column header menu; time precision controls remain P0-06.3.

### Source property columns (P0-06.2)

Reference: pinned `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/PanelItems.cpp::InitColumns/SaveListViewInfo` and
`PanelListNotify.cpp::GetItemText`. The header exposes folder-type property
columns, preserving visibility, width, order and sort between openings. Name
cannot be hidden. The shared list remains fully visible in all four modes at
980×620 and 640×400. `panel_columns` writes eight `source-columns-*` JSON
reports for those states. These establish the Qt structure and usability; they
do not establish rendered Windows parity or availability of binary raw metadata
from the external `7zz` backend (P1-10).

### Time precision and UTC (P0-06.3)

Reference: pinned `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`MyLoadMenu.cpp` View/Time popup construction and command dispatch,
`PanelListNotify.cpp` property-to-text branch, and unchanged portable
`PropVariantConv.cpp`/`PropIDUtils.cpp`. The popup presents five current-time
examples (day, minute, second, 100 ns, nanosecond) and a separate UTC toggle.
Changing either setting redraws visible time columns in place; day-level ties
still sort by source `FILETIME`. `panel_columns` checks the menu text and state,
filesystem/archive cells, persistence and fully visible list at 980×620 and
640×400 in `source-time-*.json`. These Qt reports do not establish rendered
Windows parity.

## File list settings

Reference: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/SettingsPage2.rc` and `SettingsPage.cpp`.
The Qt page follows the resource order: Show `..`, real file icons, full-row
selection, grid lines, single click, alternative selection, system menu, large
pages, extraction-memory label and checkbox/edit/arrows/RAM row, then OK/Cancel.
Alternative selection is enabled and persists the source mark mode.
The page has no heading absent from the resource.
Large pages are capability-gated Windows controls; source extraction-memory
edit validation preserves raw input until Apply. The six-page property sheet
keeps System/Menu/Folders/Edit/Settings/Language order. Original OptionsDialog
and Windows/Control/PropertyPage.cpp set the initial page to System (nStartPage0);
the Qt container now follows that state. Ctrl+Tab/Shift+Ctrl+Tab move between
pages using Qt's tab-bar focus; the Tab cycle reaches the visible page and may
visit the footer first on Windows. Pending input
survives switching; unavailable native capabilities remain disabled within pages.
This establishes structure and interaction, not pixel correspondence.
Folders follows pinned FoldersPage2.rc: working-folder label, system/current/
specified radios, path/Browse row, removable-only checkbox. Path and Browse are
enabled only for specified mode. POSIX removable-only is disabled until the
platform owner supplies drive-type support. `options_dialog` checks640×390 and
520×380 JSON/geometry, native mode order and separated footer. The height grows
from350 because Settings now has minimum361 with its additional source controls.
`options_dialog` also checks Settings memory row grouping and input focus after
invalid Apply, writes options-settings JSON at both sizes. `options_integration`
checks all-six-page Apply/OK/Cancel/restart and visible keyboard focus;
P1-12.2 evidence is recorded in the port audit.
`fm_settings` checks unclipped labels, order and separated footer at 460×390
and 340×390, writing current-build settings JSON reports. Rendered Windows
parity remains unverified.

Language reference: pinned `LangPage.rc`, `LangPage.cpp` and original26.03
`Lang/en.ttt`/`Lang/ru.txt` (asset baseline in resources/7zip/provenance.json).
The page keeps language label, combo and read-only scrollable native metadata
report in source order. `language` checks600×320/480×280; `language_ui` writes
`options-language-*-*.json` for the other five Options pages at640×390 and
520×380 in English/Russian, verifies bounds, checkbox widths and wrapped label
height. Reports reviewed2026-10-01 on macOS: largest shell-bitness RU caption
391 px fits476 px, memory label wraps within31 px, editor fields retain input.
Qt standard buttons retain platform conventions. This validates these Qt forms'
structure/usability, not rendered Windows pixel parity or remaining app dialogs.
Panel parent navigation follows source Panel.cpp VIEW_PARENTFOLDER's icon-only
control using Qt's standard up image; the menu caption stays independent and
button images survive language/state changes. Current640×400 two-panel JSON shows
29×28 parent buttons and all four status fields visible. QtStatusLabel permits
bounded status widths, elides long text and exposes full text in a tooltip;
the source value/accessibility remains intact. `panel_key` verifies640×400 and
restored splitter300, and `language_ui` verifies icons after EN/RU reload.
`panel_key` checks red marks and single focused selection across four view
modes, disk/archive and flat/normal folders, writing 980×620 and 640×400
window JSON reports. The parent sentinel stays unmarked. These reports establish
Qt geometry, not Windows pixel parity.

## External editor settings

Reference: the same pinned revision, `EditPage2.rc`, `EditPage.cpp` and
`OptionsDialog.cpp`. Editor presents Viewer, Editor, Diff in source order, each
label above a command field and Browse button. The property-sheet footer is
OK/Cancel/Apply; Apply is initially disabled, tracks source modified flags and
resets after applying. Cancel after Apply retains committed values. Browse Cancel
preserves the command; selecting a program replaces it and drops old parameters.
`external_editors_ui` checks unclipped fields/buttons, separated controls and
footer at requested 640×390/520×380 and writes external-editor-options widget JSON.
Width remains exact; height may expand to the six-page sheet's native minimum.
On 2026-10-05 Qt6.12 Windows reports 640×390/520×390; macOS reports
640×390/520×380. All command fields, Browse buttons and footer remain visible
and separated in both states. The test compares the actual size to the requested
size expanded by `minimumSize`, while retaining the clipping assertions.
These reports validate Qt geometry and resource structure; rendered Windows
parity remains unverified.

Archive changed-file prompt reference: the pinned `PanelItemOpen.cpp`
`MyThreadFunction`, `IDS_WANT_UPDATE_MODIFIED_FILE` and Yes/No/Cancel MessageBox.
Qt uses plain text, the relative filename and three source answers. Source No
and Cancel both discard the temporary copy without updating; read-only or failed
update retains it. `archive_external_edit` checks fully visible labels/buttons,
text fit and actual Yes/No/Escape at normal/compact owner sizes 980×620/640×400,
writing `external-update-prompt-*` JSON. Qt chooses message-box sizing; these
reports establish usable geometry, not rendered Windows pixel parity.

## VerCtrl commands and Revert

VerCtrl reference: pinned `VerCtrl.cpp`, `MyLoadMenu.cpp` dynamic Zvc IDs and
`OverwriteDialog.rc/.cpp`. Commands append after the File menu's resource items
and use original names/mnemonics; Diff and `FM/7vc` must be configured. Readonly
files show Ver Edit; writable files show Commit/Revert/Diff. Revert changes
OverwriteDialog's default to No and hides Yes All/No All/Auto Rename. `verctrl`
checks actual Yes/No/Escape, default/visibility and unclipped geometry at
700×440/560×440 and writes current-build dialog and owner-window JSON.
Rendered Windows parity remains unverified.

## Folder history list

Reference: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ListViewDialog.rc/.cpp` and
`PanelFolderChange.cpp::CPanel::FoldersHistory`.
The single-column list fills the resizable body; OK/Cancel sit below it.
There is no separate Delete button in the reference. Delete affects all
selected rows, Ctrl+A/C copy/select, Enter confirms the focused path.
Long paths remain reachable through scrolling, without eliding stored text.
SingleClick changes activation to one click and enables hover selection without
confirmation. Modified selection clicks do not activate. Interaction coverage
is in `list_activation`; this adaptation does not change the geometry contract.
`folder_history` verifies list/footer visibility and separation at 540×360
and 360×240 and writes JSON reports. Qt geometry establishes structure and
usability; rendered Windows parity remains unverified.

## Hash results

The native HashGUI result presenter uses the same ListViewDialog resource and
two-column property/value body. Its title is «Контрольные суммы»; source bundle
rows include the filename, byte size and requested digest. It starts without
selected rows and retains the native DeleteIsAllowed flag. OK/Cancel remain
below the list. `hash_results` checks exact source-generated rows against a
CRC32 fixture, title/state and unclipped, separate list/footer/buttons at
540×360 and 420×360; reports are `hash-results-540`/`hash-results-420`.
Horizontal scrolling remains available for long Unicode names/values. This
checks Qt structure/usability, not rendered Windows pixel parity. The complete
hash command/file workflow is connected and accepted in P2-03. PanelCrc and
HashCalcGUI invoke this same source presenter; current540/420 JSON reviewed
2026-10-03, macOS full gate199/199 and Windows13/Linux focused consumers passed.

## Full text detail

Reference: EditDialog.rc; source scope and baseline are recorded in the
[properties audit](upstream-port-audit.md#свойства-и-двухколоночный-список-2026-09-28).
CEditDialog has one read-only multiline
edit above Close, with both scroll directions and no automatic wrapping.
Text/Title are copied without truncation. Enter remains in the edit
(ES_WANTRETURN); Tab moves focus to the footer. Geometry: 480×320 and 320×240,
visible, separate text area and footer. Coverage is described in
[testing](testing.md). Rendered Windows parity remains unverified.

## Selected properties

Window Properties now reads the bound source folder/operated indices. File
packed size is visible for filesystem and archive owners; empty source values
are omitted. Rows follow the source property catalog, with the existing full
current-location footer. Archive Modified text remains sparse raw compatibility
data until native time parsing; it is not a precision/timezone acceptance claim.

Reference: ListViewDialog.rc; source scope and baseline are recorded in the
[properties audit](upstream-port-audit.md#свойства-и-двухколоночный-список-2026-09-28).
Two columns (property/value) fill the body with a visible empty header,
followed by OK/Cancel. No label form or custom selection title remains.
The value shown in the list is limited to 1024 upstream wchar_t units,
then ` ...`; CRLF/LF become spaces. Copy and Enter/double-click detail
use the full value. Delete is disabled for properties.
Geometry: list/header/footer at 540×360 and 360×240, long Unicode paths and
horizontal scrolling. Test coverage is described in [testing](testing.md).
`panel_property` writes `typed-properties-540` and `typed-properties-360` reports
with source typed maximum-size and multiline error values. List/header/footer
remain visible and separated; names/values remain two columns, long values use
the existing source truncation/detail contract and error LF displays as spaces.
Only structural Qt usability is verified; a rendered Windows reference
was unavailable.

`properties_runtime` produces `properties-runtime-flat*-mode*-*.json`,
`properties-directory-flat*-mode*-*.json` and
`properties-summary-flat*-mode*-*.json` for file, directory and aggregate states
in all normal/flat/four-view combinations at 540×360 and 360×240. The same list,
empty two-column header and separate visible OK/Cancel footer apply to each.

## Shared name/mask dialog

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ComboDialog.cpp/.rc` and
`BrowseDialog.cpp::Dlg_CreateFolder`. Implemented view: `ComboDialog.cpp`.

- The prompt precedes one editable combo, with OK/Cancel at the bottom.
  Initial Value remains visible after populating ordered Strings.
- Folder creation uses the shared dialog and default "Новая папка";
  file creation and selection masks use the same view with their own prompts.
- OnOK preserves raw text, including spaces and an empty value. Validation
  belongs to the calling operation. Cancel leaves the public Value unchanged.
- `combo_dialog` checks folder/file/mask views at 360×150 and 260×150:
  fitted prompt, reachable combo/footer, control order and separate buttons.
  JSON reports accompany the checks; screenshots remain optional.
- ZIP comment uses source `PanelOperations.cpp::ChangeComment` with the focused
  item name and localized Comment in its title, localized IDS_COMMENT2 prompt
  and the existing raw property value. `native_comment_ui` checks EN/RU at the
  same two sizes: current comment-en/ru-360/260 JSON reports preserve prompt,
  editable combo and separately reachable OK/Cancel. titleID=0 preserves the
  caller's dynamic title; source language ID0 is the application name.
- Original Set Comment Error/HRESULT remains the error presentation. On macOS,
  which omits QMessageBox window captions, the caption is the visible message
  heading and HRESULT detail is informative text. Tests exercise real writer
  refusal on both languages and ordinary/AES ZIP.
- Filesystem comments use the same source dialog, focused name/title and
  IDS_COMMENT2 prompt. `filesystem_comment` checks existing value, EN/RU,
  normal360×150/compact260×150, File Comment and Ctrl+Z, OK and Escape, selected
  rows and unchanged file bytes. Eight filesystem-comment-en/ru-0/1-360/260 JSON
  show a fitted prompt, editable combo and separately visible/enabled OK/Cancel.
  The source FSFolder writer trims the accepted comment; ZIP's raw whitespace
  behavior remains separate. No new dialog or rendered Windows parity is claimed.

Qt layouts replace OnSize. Plain-text wrapping handles Qt label rendering;
the native AddString behavior is adapted by restoring initial edit text after
Qt populates the combo. Windows rendering was unavailable; geometry establishes
structure/usability on the tested macOS Qt platform.

## Split and Combine dialogs

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/SplitDialog.rc`, `SplitDialog.cpp::{OnInit, OnOK,
OnButtonSetPath}`, and `PanelSplitFile.cpp::CApp::Combine` with `CopyDialog.rc`.
Implemented views: `SplitDialog.cpp` and the existing `CopyDialog.cpp`.

- Split puts the destination label above its editable combo, with Browse on
  the right. The volume-size label and editable preset combo follow below.
  The source filename is appended to the window title, as in OnInit.
- OK/Cancel remain at the bottom. Invalid volume input shows a wrapping
  inline error above the footer and keeps the dialog open, including accept().
- Combine uses the original shared Copy dialog: destination, Browse,
  read-only scrolling summary and footer. The summary lists the count and
  size, source prefix, first two names, ellipsis when needed and final name.
- `volume_dialog` checks normal/compact Split at 480×240 and 340×240 in
  normal/error states, and Combine at 480×250 and 340×250 with 2/3/5 parts.
  It writes JSON and verifies order, reachable controls, fitted labels,
  separated path/Browse and separate OK/Cancel.

Qt layouts replace OnSize arithmetic; Qt native folder browsing replaces
MyBrowseForFolder. The inline error replaces the upstream modal MessageBox.
Reports establish structure/usability on macOS; a rendered Windows reference
was unavailable and pixel-level parity remains unverified.

## Copy/Move dialog

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/CopyDialog.rc` and `CopyDialog.cpp::OnSize`.
Implemented view: `src/CPP/7zip/UI/FileManager/CopyDialog.cpp`.

- Destination label precedes the editable path; Browse stays to its right.
- Selection counts and known sizes precede the source path and first five
  relative names; a larger selection ends with an ellipsis. Folder names end
  in a separator. The summary stays below the destination and above OK/Cancel.
- Names preserve line boundaries. Read-only scrolling makes the displayed
  summary and long filenames reachable without enlarging the footer.
- At 440×250 and 340×220 in Copy and Move states, the label fits, path and
  Browse do not overlap, the selection area and footer stay fully visible,
  and OK/Cancel remain separate. Qt may enforce its style minimum size.

Adaptations: Qt layouts replace Win32 resize arithmetic; QPlainTextEdit with
no wrapping replaces the static text. `CPanel::GetItemsInfoString` supplies
the upstream summary and eleven-line limit from the active source folder. `copy_dialog`
writes four JSON reports and checks these relationships with a large selection.
These checks establish structure and usability on macOS, not pixel parity.
`panel_runtime` additionally writes `copy-source-flat*-mode*-*.json` for normal/
flat source summary values in all four views. Flat archive directory size is
absent by the source folder contract and must not be replaced by a proxy total.

## Extract dialog

Reference: upstream commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/ExtractDialog.rc`, desktop `IDD_EXTRACT`.
Implemented view: `src/CPP/7zip/UI/GUI/ExtractDialog.cpp`.

- Destination label and editable history occupy the top; Browse is on the right.
- Below the destination, the left column contains the optional folder-name
  row, path mode, root-duplication checkbox, and overwrite mode in that order.
  The password group is right of the modes, with Password and Show Password.
  Restore file security is below it; Unix disables it with a platform reason.
- Toggling `extractNameEnable` hides/shows the folder-name field without
  obscuring modes or confirmation buttons.
- The explanatory hint wraps above the footer. OK and Cancel stay visible,
  distinct, and inside the window at requested sizes 640×360 and 470×330;
  Qt may expand to its style-dependent minimum size. Static labels fit.

P2-07 source_extract_advanced additionally checks EN/RU700×390/600×340 exposed
controls, checkbox width and source grouping/order. Four JSON reports reviewed
2026-10-03: compact modes are286 pixels wide, Russian ElimDup requires285;
inspected bounds equal exposedBounds. Rendered Windows parity is unverified.

Adaptations: the pinned desktop resource and Qt use combo boxes for modes.
Help uses the source topic. Selected/current-folder workflows disable root
duplication with a scope reason. The backend-specific hint is added.
See `docs/7zfm-parity.md` for
behavioral limitations. The tests check these relationships rather than hardcoded
pixel coordinates or native button ordering.

## Create archive dialog

P2-01 creation states: native eligible formats are 7z/ZIP/gzip/bzip2/xz/tar/WIM;
the one-file KeepName constraint comes from source UpdateGUI. References for
enabled states are CompressDialog.cpp::FormatChanged (Encrypt_/EncryptFileNames_),
SetMethod2 (TAR GNU/POSIX remains selectable at level0), and SetSolidBlockSize2
(source Solid_ flag includes xz). Disabled encryption remains in the right
column; Encrypt file names is hidden outside 7z. `codec_creation_ui` produces
`{type}-creation-top-{760,640}.json` and `{type}-creation-{760,640}.json` at the
top and encryption scroll positions. All seven types expose their relevant
controls by scrolling and retain the footer. These are Qt structure/state
checks; a rendered Windows reference and Windows/Linux runtime remain unverified.

Reference: the same upstream commit, `CPP/7zip/UI/GUI/CompressDialog.rc`,
`IDD_COMPRESS`. Implemented view: `src/createarchivedialog*.cpp`.

- The editable archive-path combo and format precede the options.
  The combo retains the extension and offers the last twenty destinations;
  its text and the format row do not overlap at either tested size.
- For an existing target, the update-mode row precedes the path-mode row;
  the latter offers Relative, Full and Absolute. A new target has the same
  path-mode row without an update-mode row.
- Compression controls retain level, method, dictionary, word size, solid block,
  CPU threads, and memory controls. Encryption retains the upstream order:
  password, confirmation, Show Password, method, Encrypt file names.
  Show Password hides confirmation and its label; headers are hidden for ZIP.
  Compression stays in the left column and encryption in the right at both sizes.
  Password and confirmation labels sit above their fields, as in the resource.
  The encryption-method label/combo share one row; decompression memory is
  right-aligned. No source picker, Save in or preview is present. Long archive
  history does not widen the form.
- Options are scrollable. At 760×720 and 640×480, format, compression controls,
  password fields must each be fully reachable by scrolling.
- The OK/Cancel/Help footer remains outside the scrolling
  area. Footer buttons remain visible and do not overlap in both 7z and ZIP.

Adaptations: the two columns remain in a scroll area; compression
rows wrap at compact widths rather than removing the second column. Unsupported
upstream options are still absent; this is a partial resource adaptation.
`ui_geometry` covers column ordering, absence of a source picker, reachability
and the footer; existing compression tests
cover method/format-dependent enabled states and persistence.
`compression_encryption` checks the complete encryption group, control separation
and shown/hidden confirmation at both sizes, with a 400-character history path;
reports are `encryption-{760,640}-{0,1}.json`.
`compression` checks update and path-mode visibility and scroll reachability
at 760×720 and 640×480; current-build reports are
`update-options-normal.json` and `update-options-compact.json`.

## Archive password prompt

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/PasswordDialog.cpp/.h/.rc` (`IDD_PASSWORD`).
`CPasswordDialog` shows a prompt, password edit, Show Password, then OK/Cancel.
Initial Password/ShowPassword are applied before display. Toggling copies the
current edit into public state, then changes echo mode without changing its text,
as in ReadControls/SetTextSpec. OK/Enter commit raw text; the caller discards a
rejected dialog and keeps the existing backend cancellation policy.

`password_dialog` checks masked/shown states at 360×170 and 260×170, control
order, label fitting, footer visibility and separation. `ui_parity_password`
maps all five resource controls to these current-build reports. Behavioral tests
cover initial values, Unicode/spaces, show/hide, OK/Enter and Cancel. Editable
password values stay out of reports. Russian captions and Qt button ordering
are documented adaptations. Windows rendering remains unverified.

## Progress dialog and modal lifecycle

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
ProgressDialog2.rc/ProgressDialog2a.rc (`IDD_PROGRESS`) and the complete handlers
and statistic/message dependencies in ProgressDialog2.cpp. Keep the original
two statistic columns: elapsed/remaining/files/file-total/errors on the left;
total/speed/processed/packed/ratio on the right. Below them, status, two-line
directory/name, progress bar, two-column error list, then Background/Pause/Cancel
in that order. Percentage belongs in the title; hide text inside the bar.

Initially errors label/value/list are hidden. New messages show all three and
use source numbered first lines and unnumbered continuations; copy selected
rows or all rows if none selected. ShowCompressionInfo=false hides packed and
ratio labels/values without rearranging the source groups. Pause changes to
Continue, prefixes the title and freezes timer rendering/elapsed accumulation;
resume preserves elapsed time. Files-progress scales by count, while speed
remains byte based. Unknown totals clear size/remaining totals according to
the original cached state. Native endpoint/unknown/overflow behavior is retained.

Source OnSize controls geometry through Qt font-DLU/rect/move services; round
line height upward to fit QLabel text. Source NormalizeSize only handles
oversized windows/work-area overlap; it does not supply a minimum size.
Normal 760×450 and compact 640×380 geometry tests check every visible control,
text/button fitting and footer/list separation. `ui_parity_progress` maps all
26 IDs to 24 JSON reports across compression/errors/paused, native priority and finish-Close
states. Captions are fixed Russian; clipboard shortcuts also accept macOS Meta.
Background is the source default button and receives initial focus when native
priority capability is available; Enter on focused Pause toggles pause instead.
Real priority failure restores the previous caption/title and reports a source
Sync error. Cancel uses source YES/NO/CANCEL and Escape-as-Cancel, temporarily
pausing and restoring prior pause. Native completion while the question is open
is deferred until the answer; completed errors hide Pause/Background and focus
the default Close button. Native message boxes use plain text. Their Qt platform
layout is checked separately from the 26-control source resource; macOS ignores
message-box window titles. Add in an open archive was the first operation consumer
under P0-10.1. P0-10.2 connects the other current archive, filesystem and volume
operations. Filesystem/volume workers expose cancellation and real background
priority, but Pause is disabled because they have no worker pause capability.

MessagesDialog.rc (`IDD_MESSAGES`) uses a two-column numbered/message list and
Close footer. The Qt source port preserves AddMessageDirect/AddMessage and the
OnInit/OnSize control flow; `IDL_MESSAGE` is above `IDOK` at 760×450 and
640×380. Current-build JSON confirms both controls are visible, separated and
within the dialog. The File menu reopens this source view modelessly for the
last operation; this is an explicit Qt workflow adaptation. Font-DLU scaling
may enlarge the Close button to its Qt size hint. Windows rendered appearance
remains unverified.
No Windows rendered reference; geometry does not establish pixel parity.
The original IconID branch loads the exact FM.ico through a Qt ID/resource
service, including native 16/32/48 frames; default negative IDs preserve the
existing icon. Image decoding tests do not verify Windows title-bar rendering.

When extending a view, update its specification and add the relevant geometric
relationship or conditional state to its focused test. Add contracts for other
views as they are changed rather than rewriting all UI tests at once.
