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

The main-window caption is native CApp::RefreshTitle's focused folder prefix,
with `7-Zip` for an empty prefix. Toolbar option checks mirror native CApp flags;
existing grouping and action order stay in the main-window normal/compact reports.
Command availability comes from native menu policy plus transport restrictions.
Screen-size menu filtering uses available monitor geometry and Qt system-font
units, independently of the main window's compact resize fixture.

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
The partial page keeps the resource order of supported controls: real file
icons, grid lines, single click. OK/Cancel sit below them. The Qt dialog has
no heading absent from the resource. Other controls and the property sheet
are omitted; this does not establish full OptionsDialog parity.
`fm_settings` checks unclipped labels and separated controls/footer
at 460×180 and 340×180, writing settings JSON reports. Rendered Windows
parity remains unverified.

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
message-box window titles. Add in an open archive is the first operation consumer
under P0-10.1 validation. Other consumers remain mandatory P0-10.2 work.
No Windows rendered reference; geometry does not establish pixel parity.
The original IconID branch loads the exact FM.ico through a Qt ID/resource
service, including native 16/32/48 frames; default negative IDs preserve the
existing icon. Image decoding tests do not verify Windows title-bar rendering.

When extending a view, update its specification and add the relevant geometric
relationship or conditional state to its focused test. Add contracts for other
views as they are changed rather than rewriting all UI tests at once.
