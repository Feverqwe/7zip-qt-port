# External tools and temporary resources

[Все UI-спецификации](../ui-layout-contracts.md). Разделы сохраняют исходные требования,
якоря и границы датированных свидетельств. Новые результаты проверок —
в [source audit](../upstream-port-audit.md).

### Native external update publication (P3-05.15)

After the original modified-file Yes prompt, native CopyFromFile retains the
source unconditional RefreshListCtrl policy for writer success/failure/Cancel.
The refresh HRESULT is ignored, including deferred S_FALSE/E_FAIL/E_ABORT;
only actual writer failure produces the source update error. Failed publication
is empty/enabled; Ctrl+R recovers the same folder. Successful publication restores
the pre-update marks/focused name. Final focus returns to the previously focused
peer control after rows, redraw and operation guards finish. Owned editor copies
are deleted on success and retained on failed/cancelled update. Private archive
bytes change only on success; retry and filesystem OnTimer remain usable.
Inspect both panes, EN/RU, table/icon and both selection modes at980×620/640×400,
with2/301 rows. Rendered Windows and current native3OS acceptance remain separate.

### Delete Temporary Files (P1-15.2)

Reference: pinned BrowseDialog2.cpp/rc (dialog93), six Name/MTime/Size/Files/
Folders/Name-2 columns. Controls retain Delete/Refresh, Parent/read-only path,
list, disabled filter, Help/Close ordering. Owned membership replaces upstream's
7z[E/O/S]+eight-hex naming convention; active or unconfirmed external entries remain visible with an
explanatory tooltip and disabled Delete. Enter navigates, Backspace returns,
Delete confirms eligible deletion, Ctrl+R refreshes, Ctrl+F3/F5/F6 sort by
name/time/size and toggle direction while retaining selection. Plain F5 remains
the Qt Refresh convenience. F1 opens fm/temp.htm, Escape
and Close exit without deletion. Close has initial focus. Focused buttons
activate on Return; no permanent Enter default remains on non-button focus.
Source OnOK only navigates with actual list focus; readonly-path Enter/public
accept is a no-op. Tab order follows the resource, with disabled controls skipped.

Initial client is466×344DLU, with8DLU root margins, source5/3DLU gaps between
command/path/list rows and8DLU gaps around filter/footer. Delete/Refresh/footer
buttons are64×16DLU with native caption protection; Parent is exactly24DLU wide
with16DLU minimum height. Path/filter heights have14DLU minimums. Source sample
strings initialize all six column widths through native content autosizing. The
existing500×320 compact minimum remains a Qt accessibility adaptation.
Source minimize/maximize/resize capabilities are requested without context-help;
native window managers may suppress modal chrome. Initial EN/RU reports and
focus-sensitive Enter policy are checked in `temp_cleanup`.

temp_cleanup writes EN/RU900×580/500×320 temp-cleanup-*.json; footer/path/filter
stay exposed, and the six-column list scrolls horizontally at compact width.
Geometry/source structure and a bounded original EN/RU comparison are checked
in the [current browser receipt](../archive/qt-port/upstream-port-audit/p3-06-source-browsers-2026-10-16.md).
Named controls remain exposed at normal/compact sizes; full workflow/pixel parity
is not inferred from those reports. Earlier P1-15 check.sh175/175 is historical.

### Offline Help and About (P1-15.1)

References: pinned HelpUtils/MyLoadMenu/CompressDialog topic calls and exact
official26.03 CHM HTML/CSS with release provenance in resources/7zip.
QTextBrowser replaces Win32 CHM presentation, retaining pages/links/fragments.
Help follows the application palette, including already open topics. Page,
table and syntax backgrounds, text and links remain readable in light/dark
palettes; theme changes preserve the topic, fragment, history and scroll position.
Original HTML, typography and layout remain unchanged. `help_source` checks
palette transitions and writes normal/compact `help-theme-*` widget reports.
help_source writes EN/RU760×560/420×300 help-*.json: browser, Back and standard
Close remain exposed. AboutDialog.rc/source is the structural reference for
About360×330/300×310 EN/RU: logo, version/date/copyright/free-software, footer
homepage/OK in source left-to-right order. The original ICO is110×63 and is
displayed at its natural size, rather than independently reduced to64×36.
OK owns initial keyboard focus/default Return. The source160×160DLU client
template maps through the font; application version/license rows may extend it
and protect the Qt minimum size. The free-software text is top-aligned inside
its source42DLU area. The natural logo cannot shrink; the added identity row
uses Qt's height-for-width measurement for both EN/RU and protects its minimum
content width/height. These added rows are explicit adaptations.
help_source writes about-*.json; all text fits and controls remain exposed.
help_ui checks actual Contents/F1/modal About, source Options page callbacks and
Add/Extract button/F1 without applying pending state; compression_advanced checks
original Options Help. Existing Add760/640, Extract600/470 and Options640/520
geometry reports cover the added Help footer. Current Qt usability is checked;
the bounded About EN/RU source/rendered comparison and native font/compact checks
are recorded in the [presentation receipt](../archive/qt-port/upstream-port-audit/p3-06-primitives-2026-10-16.md).
Complete pixel parity remains unverified; P1-15 acceptance includes cleanup above.

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
