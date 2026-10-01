# Text specifications and geometry checks

For resource-derived comparison and reviewed C++ states, see
[upstream UI references](ui-reference.md) and `$sz-ui-parity`.
`ui_parity_copy` and `ui_parity_password` check fresh reports against pinned
resource baselines.

For layout work, read the affected view's specification below, run its focused
test, and inspect the generated JSON. Screenshots are a fallback for fonts,
icons, styling, or defects that geometry cannot explain. These checks establish
structure and usability on the tested Qt platform, not pixel-level Windows parity.

## Commands and reports

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
Registration, Cascaded/Icons/ElimDup and list controls are disabled with an explicit
capability reason until the real Explorer consumer is ported by P2-09. Zone is
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
The Qt existing-archive form shows target, format, update mode and compression
controls above the source picker. `compression` checks the mode list, normal
760×720 and compact 640×480 footer/scroll reachability, then writes
`update-options-normal.json` and `update-options-compact.json`.
Path-mode and protected-archive controls remain P1-05.2. These reports verify
Qt structure and clipping on the checked host; Windows rendering is unverified.

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
edit validation preserves raw input until Apply. The partial
six-page property sheet has Menu/Folders/Editor/Settings/Language enabled and System disabled;
this does not establish full OptionsDialog parity.
Folders follows pinned FoldersPage2.rc: working-folder label, system/current/
specified radios, path/Browse row, removable-only checkbox. Path and Browse are
enabled only for specified mode. POSIX removable-only is disabled until the
platform owner supplies drive-type support. `options_dialog` checks640×390 and
520×380 JSON/geometry, native mode order and separated footer. The height grows
from350 because Settings now has minimum361 with its additional source controls.
`options_dialog` also checks Settings memory row grouping and input focus after
invalid Apply, writes options-settings JSON at both sizes. Full six-page
integration remains P1-12.2.
`fm_settings` checks unclipped labels, order and separated footer at 460×390
and 340×390, writing current-build settings JSON reports. Rendered Windows
parity remains unverified.

Language reference: pinned `LangPage.rc`, `LangPage.cpp` and original26.03
`Lang/en.ttt`/`Lang/ru.txt` (asset baseline in resources/7zip/provenance.json).
The page keeps language label, combo and read-only scrollable native metadata
report in source order. `language` checks600×320/480×280; `language_ui` writes
`options-language-*-*.json` for all five available Options pages at640×390 and
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
footer at 640×390/520×380 and writes external-editor-options widget JSON.
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
hash command/file workflow remains mandatory P2-03.

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
  row, path mode, and overwrite mode in that order. The password group is right
  of the modes and contains the password field and Show Password.
- Toggling `extractNameEnable` hides/shows the folder-name field without
  obscuring modes or confirmation buttons.
- The explanatory hint wraps above the footer. OK and Cancel stay visible,
  distinct, and inside the window at requested sizes 640×360 and 470×330;
  Qt may expand to its style-dependent minimum size. Static labels fit.

Adaptations: the pinned desktop upstream resource uses combo boxes for modes,
as does Qt. Help and unsupported root-duplication/security controls
are omitted. The backend-specific hint is added. See `docs/7zfm-parity.md` for
behavioral limitations. The tests check these relationships rather than hardcoded
pixel coordinates or native button ordering.

## Create archive dialog

Reference: the same upstream commit, `CPP/7zip/UI/GUI/CompressDialog.rc`,
`IDD_COMPRESS`. Implemented view: `src/createarchivedialog*.cpp`.

- The editable archive-path combo and format precede the options and source picker.
  The combo retains the extension and offers the last twenty destinations;
  its text and the format row do not overlap at either tested size.
- For an existing target, the update-mode row precedes the path-mode row;
  the latter offers Relative, Full and Absolute. A new target has the same
  path-mode row without an update-mode row.
- Compression controls retain level, method, dictionary, word size, solid block,
  CPU threads, and memory controls. Encryption retains the upstream order:
  password, confirmation, Show Password, method, Encrypt file names.
  Show Password hides confirmation and its label; headers are disabled for ZIP.
  Compression stays in the left column and encryption in the right at both sizes.
  Password and confirmation labels sit above their fields, as in the resource.
  Destination and the source picker follow the options. Long archive history
  does not widen the form.
- Options are scrollable. At 760×720 and 640×480, format, compression controls,
  password fields, and destination must each be fully reachable by scrolling.
- Target preview and the Create/Cancel footer remain outside the scrolling
  area. Footer buttons remain visible and do not overlap in both 7z and ZIP.

Adaptations: source picker and destination fields integrate the Qt file-manager
workflow. The source picker follows the archive form so it does not displace the
primary upstream controls. The two columns remain in a scroll area; compression
rows wrap at compact widths rather than removing the second column. Unsupported
upstream options are still absent; this is a partial resource adaptation.
`ui_geometry` covers column ordering, archive-before-source ordering, reachability
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
