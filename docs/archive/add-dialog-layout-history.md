# Add dialog layout investigation history

Historical observations extracted from the UI contract on 2026-10-18. Pending
statuses and test counts below describe their recorded snapshots, not current
acceptance. See the [current contract](../ui-layout-contracts/compression.md#add-archive-naming-metadata-p2-01)
and [completed UX acceptance](../ux-port-audit.md) for scope.

### Add archive naming metadata (P2-01)

At a fixed window size, changing format, compression level/method, dictionary,
word/solid size, threads or memory limit must preserve both column boundaries
and the positions of the remaining controls. Unsupported memory/header options
leave their space reserved, as in the source dialog. Long memory values remain
readable within their column. Manual resizing and language/font changes may
adapt the layout to the available space.

Native adaptation,2026-10-15: outer margins follow source m8 using current
dialog base units; both columns grow their nonfixed fields, including all six
shared-width compression combos. Complete combo/group widget rectangles must
fit the viewport; source buttons/checks remain reachable by Tab. Normal and
compact EN/RU source-add reports cover native styles and an explicit
FieldsStayAtSizeHint style, with distinct `-size-hints` report tags. The lower
memory usage/limit group is scrolled into view before asserting both Threads
and memory visibility. Padding-only address-icon clipping has its separate
glyph contract; it does not relax Add control bounds.
[Native Add receipt and limits](qt-port/upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--native-add-geometry-tab-traversal-and-display-fixtures-2026-10-15).

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
Remaining menu aliases are in [the mandatory UX audit](../ux-port-audit.md).
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
