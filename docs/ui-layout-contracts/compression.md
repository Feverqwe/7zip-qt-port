# Archive creation and compression

[Все UI-спецификации](../ui-layout-contracts.md). Разделы сохраняют исходные требования,
якоря и границы датированных свидетельств. Новые результаты проверок —
в [source audit](../upstream-port-audit.md).

<a id="add-metadata-options-p2-07-in-progress"></a>

### Add metadata Options (P2-07)

Reference: pinned CompressDialog.rc Options NTFS group and COptionsDialog
OnInit/OnOK. Symbolic links precede hard links, alternate streams and security;
original handler flags control visibility. Time controls follow this group,
with OK/Cancel/Help in the footer. Unix hides NTFS streams/security.
source_link_storage requests EN/RU540×480/480×430 reports for actual TAR options;
native font/control minima may enlarge those requests, with links and footer
remaining fully exposed.
Normal and compact TAR states must expose both link controls and the footer,
with translated captions fitting their controls. This establishes
structure/usability, not rendered Windows pixel parity.
Explicit-value checkboxes keep their colon beside the indicator even in tall
rows. The precision caption wraps only when the available width requires it.
NTFS rows keep their source order and spacing when unsupported rows are hidden.
The fixed source group areas remain reserved; this is not a compact redesign.
source_metadata_storage additionally checks WIM security/ADS and all visible
time captions, Options/Add Cancel/OK/restart, and writes four Windows
`source-metadata-wim-{language}-{width}` reports. Current EN/RU540×480/480×430
reports reviewed: RU compact hint475×423 fits480×430, ADS397×18/security301×18
fit434-wide controls; last-access337×32 fits414×32; footer458×20 fully exposed.
Only the last-access caption wraps additionally for WIM; 3px vertical gaps keep
all source groups and controls reachable at compact sizes without reordering.

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
[upstream UI references](../ui-reference.md) and `$sz-ui-parity`.
`ui_parity_copy` and `ui_parity_password` check fresh reports against pinned
resource baselines.

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
[Native Add receipt and limits](../archive/qt-port/upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--native-add-geometry-tab-traversal-and-display-fixtures-2026-10-15).

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

### Advanced compression options (P1-07.1)

Reference: pinned `GUI/CompressOptionsDialog.rc`, `CompressDialog.rc` parameter
and Options rows, and `CompressDialog.cpp::COptionsDialog`. Add shows Parameters
after volumes, then an Options button with the native summary. At 760×720 and
640×480 the form scrolls to these controls while the confirmation footer remains
visible. `compression_advanced` writes `archive-advanced-760/640.json`.

Options uses the source256×248DLU initial client, protected by native control
minimums. It keeps resource grouping/order: optional NTFS controls, format time info,
Time group with explicit/default precision and modification/creation/access/time
of archive rows, preserve-access checkbox, then OK/Cancel/Help. Explicit/default
checkboxes retain the source `:` caption and descriptive accessible names.
Native capability policy hides unsupported NTFS
controls; ZIP UNIX/DOS precision hides creation/access rows. Archive mtime is
enabled. Preserve access is enabled on Windows and disabled with a reason on
POSIX; Help and F1 open the source Options topic through the application service.

The NTFS group retains its source72DLU minimum with native control protection.
The initial layout keeps unused NTFS space above Time when the group is hidden;
native control minima and compact resizing can reduce that empty space. Time rows retain source
order/spacing with native minimum protection for wrapped captions. Cocoa gives
the NTFS slot one extra logical pixel for its native group backing and uses
control-dependent horizontal spacing to separate the combo backing from its label.
Normal/compact requested sizes expand to actual native minima; text and controls
must remain exposed, rather than fit an impossible fixed-height request.

`compress_options_presentation` writes14 current reports:7z/ZIP/WIM × EN/RU ×
initial/compact, plus ZIP Unix precision. Initial focus and first Tab match the
actual original runtime:CTime→ATime explicit, Precision→CTime explicit, and
symbolic→hard links respectively. Backtab reverses that transition. Space/Down,
Cancel/reopen and Return→OK retain the source default/explicit and commit rules.
The existing advanced/timestamp/language suites retain persistence, Help and
live-language coverage. Source visibility is applied before Qt computes its
minimum; unsupported hidden NTFS controls do not force a compact minimum.

[The current receipt](../archive/qt-port/upstream-port-audit/p3-06-compression-options-2026-10-17.md)
maps all22 controls against eight actual original states and native reports on
three OS. Raw Windows runtime comparison passes8/8. POSIX comparisons retain
the expected Windows-only preserve-access/ADS/security state differences; they
are not exact Windows-state passes. No pixel or physical Mac-input claim.

Timestamp entries follow source SetPrec, including no selected item when the
handler's default precision is absent from its supported mask. Inserting entries
must not select the first item. With no selection the logical value is CB_ERR
(all-ones Auto), not zero. `compression_timestamp` covers eight capability/profile
states in EN/RU, explicit keyboard selection, OK/Cancel and restart. Its64
`timestamp-*.json` reports use520×400/440×340 and check precision reachability;
existing advanced-option probes cover the complete group order and footer.

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
- New and existing targets both expose Update mode above Path mode in the
  right column; Path mode offers Relative, Full and Absolute. Source
  CompressDialog.cpp::OnInit attaches/populates both combos without testing
  target existence, matching CompressDialog.rc and the current Qt constructor.
  The new-target native Linux state is recorded in the
  [native receipt](../archive/qt-port/upstream-port-audit.md#p3-03p3-04--native-linux-package-workflow-and-desktop-recovery-2026-10-14).
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
Its scroll probe uses each whole control rectangle with `ensureVisible`, since
Qt's `ensureWidgetVisible` can accept an exposed input-method cursor while a
field border remains outside the viewport. Initial source focus stays unchanged;
full-control visibility, ordering and footer assertions remain strict.
Saved method-dependent choices follow source SetDictionary2/SetOrder2/
SetSolidBlockSize2: nearest admissible entries, first-entry fallback for a value
below the minimum, large dictionary threshold/clamp and source BZip2 decimal
comparison. Solid below the first numeric entry stays Auto for 7z; full/non-solid
remain explicit. An unavailable explicit thread count leaves no selected entry,
as source SetNumThreads2 does. `compression_profile_selection` checks EN/RU
760×720/640×480 reachability and writes `profile-selection-*.json`; values and
Cancel/restart are asserted through public controls/options, outside JSON.
Solid and thread events update memory using source GetNumThreads2 and the cached
Auto count from the last SetNumThreads2 phase. Changing Solid preserves the
thread selector, including an unavailable saved selection; dictionary/memory-limit
events deliberately regenerate Auto. OK uses that same cached estimate for memory
refusal. `compression_event` covers EN/RU XZ state transitions, refusal/no-save,
restart and official-engine workflows, with reachable thread/memory controls at
760×720/640×480 in `compression-event-*.json`. Auto remains an all-ones operation
spec; the official engine owns the actual compression thread calculation.
The format combo follows the resource's CBS_SORT presentation order while its
selection follows source OnInit: explicit handler, then saved format, otherwise
the first native candidate before sorting. Browse filter indices refer to that
same displayed order. Hash method selection refreshes the source filename suffix
(SHA1 or Auto SHA256). `compression_format` covers current/reordered handler
indices and EN/RU 760×720/640×480 format reachability in24 `format-order-*.json`
reports, plus public handler/filter/name/profile and real ZIP workflow assertions.
`compression_encryption` checks the complete encryption group, control separation
and shown/hidden confirmation at both sizes, with a 400-character history path;
reports are `encryption-{760,640}-{0,1}.json`.
`compression` checks update and path-mode visibility and scroll reachability
at 760×720 and 640×480; current-build reports are
`update-options-normal.json` and `update-options-compact.json`.
