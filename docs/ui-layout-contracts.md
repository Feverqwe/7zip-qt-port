# Text specifications and geometry checks

For resource-derived comparison and reviewed C++ states, see
[upstream UI references](ui-reference.md) and `$sz-ui-parity`.
`ui_parity_copy` checks fresh Copy/Move reports against a pinned resource baseline.

For layout work, read the affected view's specification below, run its focused
test, and inspect the generated JSON. Screenshots are a fallback for fonts,
icons, styling, or defects that geometry cannot explain. These checks establish
structure and usability on the tested Qt platform, not pixel-level Windows parity.

## Commands and reports

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

## Folder history list

Reference: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ListViewDialog.rc/.cpp` and
`PanelFolderChange.cpp::CPanel::FoldersHistory`.
The single-column list fills the resizable body; OK/Cancel sit below it.
There is no separate Delete button in the reference. Delete affects all
selected rows, Ctrl+A/C copy/select, Enter confirms the focused path.
Long paths remain reachable through scrolling, without eliding stored text.
`folder_history` verifies list/footer visibility and separation at 540×360
and 360×240 and writes JSON reports. Qt geometry establishes structure and
usability; rendered Windows parity remains unverified.

## Selected properties

Reference: ListViewDialog.rc; source scope and baseline are recorded in the
[properties audit](upstream-port-audit.md#свойства-и-двухколоночный-список-2026-09-28).
Two columns (property/value) fill the body with a visible empty header,
followed by OK/Cancel. No label form or custom selection title remains.
The value shown in the list is limited to 1024 upstream wchar_t units,
then ` ...`; CRLF/LF become spaces. Copy and Enter/double-click detail
use the full value. Delete is disabled for properties.
Geometry: list/header/footer at 540×360 and 360×240, long Unicode paths and
horizontal scrolling. Test coverage is described in [testing](testing.md).
Only structural Qt usability is verified; a rendered Windows reference
was unavailable.

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
the upstream summary and eleven-line limit from a Qt property snapshot. `copy_dialog`
writes four JSON reports and checks these relationships with a large selection.
These checks establish structure and usability on macOS, not pixel parity.

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

- Source navigation and selection precede archive name and format.
- Compression controls retain level, method, dictionary, word size, solid block,
  CPU threads, and memory controls. Encryption contains method, password,
  confirmation, and Show Password; destination follows the options.
- Options are scrollable. At 760×720 and 640×480, format, compression controls,
  password fields, and destination must each be fully reachable by scrolling.
- Target preview and the Create/Cancel footer remain outside the scrolling
  area. Footer buttons remain visible and do not overlap in both 7z and ZIP.

Adaptations: source picker and destination fields integrate the Qt file-manager
workflow. Upstream has a two-column compression/options form; the current Qt
view stacks groups in a scroll area. This specification records existing layout
and does not claim that it matches the original two-column appearance.
`ui_geometry` covers reachability and the footer; existing compression tests
cover method/format-dependent enabled states and persistence.

When extending a view, update its specification and add the relevant geometric
relationship or conditional state to its focused test. Add contracts for other
views as they are changed rather than rewriting all UI tests at once.
