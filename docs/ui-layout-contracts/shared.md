# Text specifications and geometry checks — shared

[Индекс](../ui-layout-contracts.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

### Options initial frame (P3-06, 2026-10-16)

Original `OptionsDialog.cpp`/`MyPropertySheet` use the largest desktop page
template. `GuiCommon.rc` supplies300×280 content DLU plus eight-DLU margins:
316×296 DLU for System/Menu/Settings/Language. Editor/Folders are shorter.
The initial Qt sheet must map that source page through its native font, adding
native tab/footer and outer-layout chrome. The rendered pinned Windows System
page exposes more than20 rows; an initial fake one-format page must retain that
viewport capacity at8pt/12pt and in EN/RU. A fixed640×390 frame is not that source
initial-size contract. Existing explicit640×390/520×380 normal/compact probes
remain valid Qt resizing adaptations of the fixed original property sheet.


### Options keyboard activation (P3-06, 2026-10-17)

Reference: pinned `OptionsDialog.cpp`, `SystemPage.rc`, the other five page
resources, `CPropertyPage::OnNotify`/`OnSetActive` and `MyPropertySheet`.
Initial System focus is the first enabled source tab stop: current-user `+`,
all-users `+`, then the association list. Tab and Backtab retain that order;
enabled footer buttons and the tab bar remain reachable. Map existing button
tab stops to Qt StrongFocus under Cocoa's restricted native button policy;
preserve intentional NoFocus controls and the OS keyboard preference.

Ctrl+Tab and reverse activate the new page's first enabled source control.
An exclusive radio group uses its selected enabled member; an initial text field
selects its contents. Pending values survive page switching and hide/show does
not reset an already focused field. A capability-disabled page can fall back to
the tab bar. `options_keyboard` checks EN/RU at640×390 and520×380 (expanded to
native minima), all four System availability masks and both six-page directions.
Current widget JSON checks visible control bounds; public assertions check focus,
selection and Cancel without association writes. [Source and native receipt](../upstream-port-audit/p3-06-options-keyboard-2026-10-17.md).

### Native System handler images (P3-06, 2026-10-17)

Source `SystemPage.cpp::AddIcon/OnInit/RefreshListItem` and
`SystemPage.h::CAssoc::GetIconIndex` choose the displayed handler image from the
source association state. A pending own selection uses the application image;
Clear removes it, and returning to Other restores that handler's image. Linux
adapts desktop-entry file/theme icons through GIO rather than treating an Exec
command as an icon path. Distinct handlers sharing an executable remain distinct.
The existing label/buttons/list/footer geometry is unchanged. Private-XDG
`linux_system_page_*` cases inspect EN/RU640×390/520×380 parent states and actual
Critical/OK after an external-choice conflict; all required groups remain visible
with a disabled explained all-users action. [Source/native acceptance](../upstream-port-audit/p3-06-system-native-2026-10-17.md).

### Native Copy terminal errors (P3-05)

Reference App.cpp::OnCopy, PanelCopy.cpp::CopyTo/ProcessVirt and
Panel.cpp::MessageBox_Error_HRESULT/MessageBox_Error_Caption at the audit's pin.
The standard message has HRESULT plain text, Critical icon and OK, caption7-Zip
(Qt omits it on macOS). Preserve worker and final OnCopy diagnostic phases;
E_ABORT has neither. Writer error precedes source/peer publication; retained
Move source loads after guards release. Both panes/Copy/Move/result states use
copy_publication_order terminal reports at980×620/640×400. Error resize requests
440×220/340×220 auto-fit to400×116 Mac,399×98 Linux and214×88 Windows for both
requests; inspect actual visible text/OK bounds, not requested sizes. These
32 reports/OS have no meaningful clipping and do not prove rendered Windows
pixels or physical input. [Receipt](../upstream-port-audit.md#p3-05--native-copy-terminal-phases-and-retained-guards-2026-10-10).

### Rendered Add/Extract reference (P3-06, 2026-10-15)

Add's compact form reserves a separate gutter for its non-transient scroll bar;
it must not cover fields or fade while scrolling is required. Native/translated
minimum widths include that gutter. The bar reaches the right window edge;
the source right margin separates it from the form, and footer buttons retain
their right inset. `source_compress_dialog::scrollBarKeepsItsOwnSpace`
checks EN/RU native and simulated overlay styles, compact top/bottom states and
expansion without scrolling. Cocoa640-wide requests may grow to651 for Russian
labels; the footer stays exposed and Parameters remains reachable.
[Correction and checks](../upstream-port-audit/p3-06-add-scroll-2026-10-15.md).

Rendered Add/Extract reference is separately available for original EN/RU initial
states. Its fixed desktop geometry is not a compact resize contract; Qt keeps
standard platform metrics and exposes compact Add content through scrolling with
a visible footer. Current native Add/Extract consumers supply12 JSON per OS;
Mac Extract may expand470-wide requests to554×343. [Reference/scope](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--rendered-addextract-and-native-layout-consumers-2026-10-15).

### Rename validation error (P3-05, 2026-10-15)

Source OnEndLabelEdit rejects an invalid filesystem name with the shared HRESULT
error: Critical icon, source7-Zip caption (macOS omits native captions), plain
formatted E_INVALIDARG and OK. It keeps the original name/selection and peer
state without loading rows. `panel_rename_state` covers both panes and EN/RU;
eight `rename-invalid-*` Qt reports check message and OK bounds at requested
440×220/340×220. macOS backing-widget reports fit400×116. Native Cocoa text/icon
checks do not use its hidden backing-widget geometry as rendered native proof.

Archive relative Rename retains the current provider and prefix while the source
reload publishes the new parent row. ZIP/7z file and implicit-folder scenarios
inspect the table at980×620/640×400 in eight `archive-rename-relative-*` reports.
Their geometry checks do not establish rendered Windows pixel parity.

### Parent keyboard focus (P3-05, 2026-10-15)

Parent keeps the keyboard owner while replacing rows: a focused list returns
to the same pane's visible table/icon view; an address caller retains its actual
Qt focus proxy. Preserve the source focused child row, peer address draft and
peer rows. Deferred publication must yield to newer address input or peer focus.
`native_navigation_window` covers both panes, four views, EN/RU and list/address
callers, with64 `parent-focus-*` reports at980×620/640×400 per QPA. Inspect both
addresses and visible views for enabled state and complete bounds. Focus and
draft contents are public behavioral assertions, deliberately absent from JSON.
This does not establish physical keyboard delivery or rendered Windows pixels.

### Refresh focus and alternative marks (P3-05)

Named focus restoration uses saved SelectFocused in both modes. A focused
unselected row remains unselected; source marks restore independently. With no
selected row, source focused-size/date fields stay empty even when alternative
marks exist. Inspect both panes/EN-RU/active and inactive ownership at980×620
and640×400 through `source-refresh-status` widget reports. Providerless attached
rows do not create source marks; rebind must retain the live notification route.
These are selection/status states of the existing controls, with no geometry or
text changes. Native ListView state probing does not establish rendered Windows
or physical input parity.

### Panel address keys and flat view (P3-05)

Preserve the source header/address above its own list and pane visibility after
F9. Tab/Backtab focuses that pane's visible table/icon list without submitting
the address draft. Check both panes and EN/RU at980×620/640×400, with address
and current list fully inside the window. `panel_address_keys` writes16 reports.
`flat_view` adds24 reports for Drives with source flat enabled, filesystem flat,
and filesystem normal in both panes/languages at the same sizes. Address/list
grouping and source View checked state must survive navigation and toggles;
protected fixture bytes remain unchanged. At compact Windows widths the existing
QtStatusLabel may elide selectedItemsStatus, with full source text in its tooltip;
this is separate from clipping address/list controls. These are Qt geometry/key delivery
checks, not a rendered original Windows or physical-input claim.

Source Delete uses PanelOperations.cpp's file/folder/count language IDs and
standard Yes/No/Cancel question. Refusal keeps the source selection/list.
Yes is the source default button; Enter confirms, while explicit No/Cancel
returns before refresh. Deferred consumers exercise real Return-key dispatch.
Accepted worker error/Cancel refreshes it. Native message boxes retain wrapping,
auto-fit and standard button grouping rather than a fixed resource rectangle.
Trash failures reuse Panel.cpp's captioned HRESULT presentation; macOS exposes
the caption as a heading because native window captions are omitted. The
bounded source_delete consumers inspect both languages/panes at400×200 and
300×180 probes:32 current JSON reports check reachable buttons and fitted text.
Source scope/evidence are in the [receipt](../upstream-port-audit.md#p3-05--source-delete-lifecycle-2026-10-10);
these probes do not establish rendered Windows pixel parity.

Find the affected dialog, command or plan ID with `rg -n` and read its contract
before changing layout/text/visibility. [Commands and reports](#commands-and-reports)
describes report generation; [UI reference tooling](../ui-reference.md) owns the
resource-to-widget comparison procedure. These contracts specify controls and
geometry, while [the checklist](../porting-checklist.md) owns current acceptance.

### Archive directory CRC projection (P3-05)

Source Agent::GetProperty and AgentProxy::CalculateSizes supply directory CRC
to the existing list column and Properties collector. Native ZIP/7z checks cover
both panes, EN/RU and normal/flat, preserving source sorting/marks/Refresh,
peer provider and input/archive bytes. Main-window reports use980×620/640×400;
Properties uses the existing ListViewDialog.rc list and OK/Cancel grouping at
540×360/360×240. Verify the complete table/viewport and separate exposed footer,
then Escape rejection without changing the provider or selection. At compact
width, long metadata values remain reachable through the existing scrollbars.
Qt collector geometry is separate from native Windows Properties execution and
rendered upstream parity. `native_column_workflow` writes directory-crc and
native-columns-crc reports; the source audit owns dated platform evidence.

### Changed nested archive Parent (P3-05.15)

Parent uses the source modified-archive Yes/No/Cancel question. No and Cancel
both exit and delete the spool without changing the outer archive. Yes updates
the original outer item; writer failure/Cancel retains the modified spool and
reports the source update error, but still returns to the outer folder.
Parent ignores its final Refresh HRESULT. Failed publication leaves an empty,
enabled list; Ctrl+R recovers that same outer provider. Check native2/301-row
outer archives with disk-extracted children, both panes, EN/RU, table/icon and selection modes using representative
cases at980×620/640×400. Peer bytes/provider and subsequent filesystem OnTimer
must survive. These checks do not establish rendered Windows parity.

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

### Native nested opening (P3-05.15)

OpenItemInArchive keeps original preferred provider stream, bounded memory
extraction and disk extraction paths. Native provider streams use original Agent
index mapping and keep the module/resource alive while read or seek is possible.
All successful opens ignore the subsequent Refresh result as upstream does;
S_FALSE/E_FAIL/E_ABORT during child row publication must not add a dialog.
Failed publication is empty/enabled; Ctrl+R restores the same child provider.
Parent restores the original outer provider/focused child name and removes its
owned spool. Source timer/notify/redraw remain usable, including filesystem
navigation and OnTimer afterwards. Check2/301 child rows, both panes, EN/RU,
table/icon and standard/alternative selection at980×620/640×400. Inspect failed
and recovered address/list groups; original archives and peer bytes stay intact.
Native open cancellation must retain the outer provider/rows and release owned
temporary files without a stale refresh or extra error. Rendered Windows and
native Windows/Linux execution remain separate acceptance requirements.

### Source panel creation (P3-05.15)

Native property failures during initial creation and OpenAsArc_Index retain the
source callers' ignored RefreshListCtrl result, without an extra error dialog.
The actual filesystem OpenItem caller must hand off its outer guards too;
after native recovery, filesystem navigation and OnTimer must still publish a
new private file. Direct OpenAsArc_Index alone does not prove this caller.
OnReload reports every non-S_OK result, including S_FALSE and E_ABORT, after
redraw and notification guards are restored. This policy also applies beyond
the asynchronous row boundary. A failed list is empty and enabled; a subsequent
Ctrl+R restores rows on the same native provider. Inspect failed and recovered
states in both panes, EN/RU, table/icon and standard/alternative selection at
980×620/640×400, with archive and peer bytes unchanged.
For real filesystem Copy/Move to a separate output folder, the archive peer's
unconditional reload uses that same policy. Its error precedes source selection
cleanup; terminal focus returns to the source view, output bytes are correct,
and the archive provider/bytes survive failure and retry. Check the same pane,
language, view and selection modes with a301-row native peer.

OnCreate starts list publication after its controls are constructed and before
the next panel opens. Initial and late second-panel creation must finish with
both lists/addresses visible and enabled, correct source modes, restored guards
and unchanged files/archive bytes. Check EN/RU table/icon and standard/alternative
selection at980×620/640×400, filesystem/transport/native ZIP/7z and2/301 rows.
Ctrl+R must actually publish and preserve marked/focused names; hide/show retains
the created peer. Original initial creation order starts with panel0 unless an
archive-only main-panel request chooses panel1; single-panel late creation covers
either role. Native rendered Windows parity remains unverified here.
Ordinary startup must use the requested main path rather than its saved path,
including a right main panel created second. Initial focus waits for the scans
to finish. Subsequent explicit address/list input or a public archive request
must retain the peer's focus instead of being overwritten by startup completion.
When a first scan completes during the second panel's binding, each source provider
must match its own visible rows. Public archive modal progress must return focus
to the selected address control, including its Qt combo focus proxy.
Showing a saved peer immediately after window show must retain its path and
source rows; pending initial focus cannot change projection during peer Create.
Source FM.cpp::MoveSubWindows fixes panel0 on the left and panel1 on the right,
including archive-first and late creation from an initially single right panel.

### Source startup window placement (P3-05.16)

Toolbar defaults retain source IsDialogSizeOK(200,200): when the dialog-unit
extent exceeds the available work area, default-mask bit31 disables labels.
Explicit saved labels/icons override this classification. Qt font metrics and
available screen geometry supply the original calculation; compact client
width alone does not change the saved/default policy. Actual large-font tests
check QAction/style without a rendered-geometry claim. Ordinary980×620/640×400
main-shell reports retain toolbar grouping/order, control visibility and menus.

FM.cpp::InitInstance restores a valid saved normal rectangle and maximized flag
before the caller exposes the window. Missing or malformed Position leaves the
default rectangle; Panels is admitted independently, with source panel-count and
focus defaults. Construction itself stays hidden under the QWidget lifecycle.
Close saves the normal rectangle while maximized; the next launch replays it.

The launch show command overrides saved maximization except Normal/Show/Default,
whose original InitInstance branch applies the saved flag. Explicit Maximize,
Minimize, Restore and no-activation retain their requested states. Hide binds
without exposing the window. Qt uses WA_ShowWithoutActivating for source
no-activation commands; native focus/taskbar behavior remains unverified.
Seven `launch-show-*` JSON reports inspect nonminimized states after the visible
QSizeGrip finishes deferred layout. Hidden/minimized cases check state only.

`window_placement` covers980×620/640×400 normal rectangles, normal/maximized
startup, one/two panels, focused pane, splitter publication, Close and restart.
Current JSON checks the visible panel and its controls on each launch. Qt uses
logical client coordinates and native frame/handle/content minimums; a native
rendered Windows reference remains required for frame and pixel parity.

The source splitter has a4-unit handle and a16-bit fixed-point ratio. Saved
coordinates are actual left-panel widths, excluding the handle; initial width300
remains300 at client widths980 and640. Expansion/return and F9 while one panel is
hidden retain the source ratio and rounding. Public splitter fixtures cover the
source120-unit bounds and halving below244 total width. Actual panel content can
impose a larger Qt minimum to prevent clipped address/list controls. Current
`splitter-source-{980,640}` JSON accompanies the normal/compact window cases;
native pixel/frame comparison remains open.

### Source Copy/Move destination (P3-05.15, required scope)

Copy history values follow source UStringVector/IsEqualTo_NoCase, retaining
source-distinct K/K spellings through save/reload and the actual editable combo.
Cancel preserves order; accepted CApp Copy moves its destination to the front.
`copy_dialog` and `app_copy_routing` assert those values; existing normal/compact
Copy/error geometry reports retain the same control structure.

Interarchive F5/F6 preserves the destination provider and pre-update marked/focused
names after success. Move Delete No/Cancel/error/engine cancellation restores
backup bytes and physically reopens the destination with that same saved state;
source rows/bytes remain intact. Source marks clear and focus returns to its list.
Inspect both-pane roles, EN/RU, table/icon on both panels, standard/alternative
selection at980×620/640×400, including301-row asynchronous destination publication.
Both views and addresses must be visible/enabled after terminal completion.

If physical rollback reopen fails, the destination displays its return filesystem
folder instead of stale archive rows; source rows/bytes survive. Failed backup
restore adds an explicit rollback warning; reopen E_ABORT stays silent. Check
both failed and recovered states at the same sizes/modes, including301-row
filesystem fallback. Reopening a repaired private destination and F5 retry must
restore normal selection/focus/notify/redraw and enabled controls.

Filesystem-to-archive F5/F6 keeps the bound destination provider and its pre-update
marked/focused names after native update. Source marks clear and focus returns
to the source list; failure/Cancel still publish the destination through the
existing Copy completion. Inspect both panes EN/RU, source table/icon, destination
table and alternative selection at980×620/640×400, including301-row asynchronous ZIP publication.
Both views and addresses must remain visible and enabled at terminal state.

Manual Refresh/Ctrl+R uses the bound archive provider, retaining source selection
and focus without an engine/reopen request. Inspect EN/RU both panes, ZIP/7z,
table/icon and standard/alternative selection at980×620/640×400; peer provider,
rows and private fixture bytes stay unchanged. Operation transport reopen is a
separate scenario.

Source RefreshListCtrl also publishes the bound list before restoring saved
selection. Archive Drop reloads only after successful CopyFromNoAsk; confirmation
No/Cancel and update failure/cancellation retain the list and both providers.
The peer archive is not reopened or published. Inspect EN/RU both panes,
table/icon and standard/alternative selection at980×620/640×400; terminal views
must be enabled, preserve marks/focused names and restore redraw.
For actual native writers, a later attribute S_FALSE/E_FAIL/E_ABORT after success
leaves an empty enabled list without changing the writer result; Ctrl+R recovers
the same provider. Finish the source list-focus request after asynchronous
publication, preserving the peer. Native Write failure and actual Cancel retain
the original archive and permit a second real Drop. Inspect representative2/301
rows in the default bounded native_drop_publication suite; geometry does not
establish rendered Windows parity.

After attempted external-file update, publish the owning archive list for success,
failure and cancellation; retain source selection, peer focus/provider and edited
temporary bytes on failure. Read-only worker refusal and changed-folder/missing-
item exits precede Refresh. Inspect EN/RU both panes, standard/alternative and
table/icon at980×620/640×400; terminal views must be enabled with redraw restored.

Select All/Deselect All must keep callback names available during selection and
mark-role signals, with the original source mark vector already authoritative.
An outer notify guard still suppresses uncached text. Inspect both panes, EN/RU,
table/icon and standard/alternative selection at980×620/640×400; peer focus and
provider remain stable. Selection projection does not change control geometry.

Activation uses source OnNotifyActivateItems for Return/double-click in table/icon:
Alt alone shows Properties; Shift alone opens outside; other Ctrl/Shift/Alt
combinations permit internal opening. Preserve both-pane ownership, selection,
peer and notify guards. Inspect EN/RU FS/real ZIP at980×620/640×400 and Qt Properties
at540×360/360×240; Cancel retains provider and selection. Native Windows filesystem
property sheets require their own native acceptance. Removed-directory activation
must show the source error and allow recovery without rebinding the owning panel.

Each source catalog entry gets a separate Qt section, including generic/raw
entries with the same PropID and raw entries for reserved Size properties.
Requested cells/alignment use that exact source descriptor, not the first ID.
Visible logical order and widths persist through the source save flow; duplicate
saved IDs retain upstream first-match initialization policy. Inspect both panes
EN/RU normal980×620/compact640×400, owning provider/selection/peer, header Cancel,
notification guards and Refresh. Duplicate-ID menu entries follow the native
catalog checks: hiding removes the first visible PropID, showing appends a new
instance even if that descriptor is already visible. Each visible instance has
its own logical index, section, requested cells and width. Inspect normal/compact
geometry before Refresh as well as after it, with selected rows and peer focus.
Sorting those sections uses the first catalog PropID's raw flag. Clicking any
section of the currently sorted PropID toggles the source direction, including
switching between generic/raw sections or appended copies. Keep the indicator
on the clicked section and map sort actions by PropID. Physical clicks focus the
owning list; explicit programmatic sorts retain their requested direction and
existing inactive-owner focus behavior. Inspect both panes EN/RU, empty raw data,
selection/peer state and copies/Refresh at980×620/640×400.
Re-enumerate a schema even for the same provider/type. Consume written generic/raw
counts as upstream does, skip individual info errors and generic IsDir, and hide
the reserved Qt Name section when no generic Name descriptor exists. A raw Name
keeps its own source section and Name-first logical order. Empty schemas show no
invented columns while preserving item/selection/status state; Qt collapses the
empty header to zero height. Counter outputs start at zero if a failed provider
leaves them unwritten; native behavior for unwritten outputs is unverified.
Inspect count-status, raw-Name/no-Name/empty/unwritten/recovery transitions with
header Cancel and both panes EN/RU at980×620/640×400.
Label-only highlight belongs to source visible logical subitem zero, including
raw Name or the first non-Name descriptor when reserved Qt Name is hidden.
Large/small icon and list modes show that same source subitem's exact callback
text and existing row icon, including raw Name hide/re-add, no Name, empty schema
and recovery. Appending Name does not change the existing first logical subitem.
Inspect only the affected icon/list view for appearance captures, with both panes
EN/RU at980×620/640×400; preserve selection and the focused peer across mutations.
With no source-visible descriptors, Qt retains physical-zero metadata captions
and row icons. Native rendering of this empty-catalog fallback is unverified.
Published cell text uses its source item index without another count call.
A later count error must not blank a valid name/generic/raw property; property
errors retain source generic error text or empty raw text. Preserve notify/cache
and provider guards, selection/peer focus, and invalidate old model indices on
rebind. Inspect both panes EN/RU detail/icon recovery and rebind at980×620/640×400.

Walk filesystem → real ZIP/7z → same-provider namespace schema changes →
filesystem → archive reopen. Fresh filesystem columns use source hidden-property
defaults; archives expose their reported properties. Name starts160px and other
columns100px. Persist filesystem Name width/order and shown ATime separately from
archive Name width. Unchanged initialization retains the visible descriptor
snapshot; changing only Type or only Name must rebuild it and update cell alignment
or the header caption, preserving saved widths and selection/peer focus.
Inspect all seven phases under EN/RU in both panes at980×620/640×400.

Language changes retain provider-defined column names, including Unicode names
with IDs outside the translated property catalog, in both panes. Standard
property headers still follow EN/RU. Reuse the owning source descriptor as the
property-name fallback without re-enumerating the provider.

Column initialization keeps Name at source logical index zero while retaining
the saved visual header order, including Name moved after other visible columns.
Refresh and reopening must retain that order and widths in both panes under
EN/RU at980×620/640×400. Direct InitColumns clears native marks before property
enumeration, including unchanged column catalogs; it does not clear published
Qt rows or their widget selection. Subsequent source restoration owns marks.
Save selection before direct InitColumns; restore that source snapshot afterwards.
Selection deltas update native marks immediately in their owning pane, including
the inactive peer. Notification guards and alternative mode suppress ordinary
selection deltas; SaveSelectedState reads marks without overwriting them. Preserve
parent exclusion, sorted source indices and peer focus through source Reload.
Inspect both pane roles EN/RU at980×620/640×400.

Column header menus retain source property order/checks/first-item protection,
toggle the owning panel's source visible catalog and append a newly shown column
after all visible columns. Hide/show after moving a column must follow that append
policy, while hiding the sorted column restores name/ascending metadata and the
header indicator, preserving row order until Refresh. Context, resize
and move callbacks on an inactive pane preserve active focus/provider/selection
and peer header state. Cancel changes nothing. Persist visible compact order and
width through source ViewSettings; inspect EN/RU both panes normal980×620 and
compact640×400 menus, address/list/status groups and reload/reopen consumers.
SaveListViewInfo updates visible width/order and persisted settings without
rewriting the property catalog. A menu hide/show re-adds the catalog width,
as native AddColumn does; resizing before Refresh still persists that width for
the next initialization/reopening. Catalog and visible logical indices remain
separate from header visual order.

Deleted archive items retain source OnCustomDraw's red text (RGB255,0,0), enabled
by default only when the archive reports deleted items. Use source item indices
and the owning provider for every visible table cell and icon label; parent and
ordinary items keep platform text color. Preserve alternative-selection pink
background, focus/marks, peer state and bytes. Suppress provider queries during
notification guards and reject a replaced provider. Verify on/off archive metadata,
reload/rebind/reset and both pane roles under EN/RU table/icon, standard/alternative
selection at980×620/640×400. Geometry reports do not establish color pixels;
inspect the affected view capture when needed. Rendered Windows parity remains
unverified without the matching native reference state.

Parent navigation to a namespace provider that fails during load, directory
validation, count or icon projection must leave an empty, usable view. Publish
the bound source address before loading unless CalledFromTimer suppresses it,
release the operation/notification guards, restore sorting/redraw and leave peer
provider/rows intact. A later explicit Refresh recovers the same provider.
Inspect both panes and status/address groups under EN/RU normal980×620 and
compact640×400, including load cancellation. Parent retains its source ignored
reload-result policy; explicit OnReload remains the HRESULT message owner.

Rename activation uses original RenameFile's focused row, independently of
operated marks; OnBeginLabelEdit rejects parent/read-only targets. The table/icon
editor must show that row's raw source name in either pane. Escape preserves
names, selection and bytes; a subsequent Rename can commit normally. Bounded
`panel_rename_state` consumers cover ordinary multiselection and alternative
marks on a different item. Existing editor geometry reports below apply.
Twenty current activation reports cover both panes/table-icon/selection modes
and two symbolic-link cases at980×620/640×400, with visible inline editors.
An address Return's queued list-focus request must not close a newer inline
Rename editor in the same pane when the next events or resize are delivered.

Cocoa table Rename reserves native focus-frame vertical margins inside the row.
The visible QFocusFrame must fit both the viewport and that row's vertical band;
the editor must retain its minimum text height. Icon-editor geometry is retained.
[Native RED/GREEN and platform checks](../upstream-port-audit/p3-06-rename-focus-2026-10-16.md).

Main-panel navigation contains one Parent button before the address field,
matching original Panel.cpp. Every visible panel retains its own source status
fields. The global operation-feedback row is hidden while empty and appears for
an operation/error message. Successful publication clears it; failed archive
completion retains its diagnostic. Four EN/RU normal980×620/compact640×400
one→two→one consumers inspect these groups and transient visibility.
[Rendered/source and three-platform receipt](../upstream-port-audit/p3-06-main-chrome-2026-10-16.md).

Rename completion uses original OnEndLabelEdit's saved selected names and raw
prefix/name, focusing the new name on success and the old name on error/Cancel.
Do not replace the saved selection with a single Qt path. After the worker,
source callback text is suppressed until the next load begins; raw item roles
and editor values stay available. Verify real filesystem success/collision/
load cancellation, standard/alternative marks, preserved peer bytes and recovery.
Existing name-view/editor normal/compact geometry remains the presentation
contract; source text suppression does not change grouping or widget geometry.

Native archive Rename additionally uses real F2 and the visible table/icon
inline editor, with official native writer success/Write failure/source Cancel
followed by S_OK/S_FALSE/E_FAIL/E_ABORT publication at2/301 rows. The posted
source reload ignores its HRESULT; explicit Ctrl+R owns recovery diagnostics.
Verify new/old focused name and source marks, same provider, unchanged peer,
payload bytes and successful retry after failure/Cancel. Inspect published,
recovered and retried states at980×620/640×400 in both panes and EN/RU.

Reference: pinned App.cpp::OnCopy/Reduce_Path_To_RealFileSystemPath,
PanelKey Shift consumers, CopyDialog.cpp/.rc and Panel/Agent/FSFolder CopyTo/
CopyFrom interfaces. Keep the original title/prompt/summary/history and focused
versus operated selection. Ordinary destination begins with the source opposite-
panel rule; one-panel reduction follows the original filesystem/root-prefix
algorithm. Qt must not replace an empty peer target with its own current folder.
Classify filesystem/ADS/archive/root/drives through source capabilities/prefixes,
preserving relative/direct/multiple-item normalization, path diagnostics and
unsupported root/ADS refusals before publication. Original copy options, naming,
selection/row refresh, title and source focus must survive the async continuation.
Accepted archive Move keeps its checked source Copy/Delete/rollback extension.
Verify both panels, F5/F6 and Shift routes, success/failure/Cancel, Unicode/spaces,
no unintended output/input changes and operation guards. Inspect EN/RU normal/
compact Copy dialog and resulting panel JSON using their existing contracts.
Native Windows ADS/device and rendered/cold-stable acceptance remain mandatory.
Path failures use original Panel.cpp prefix+LF+HRESULT formatting. Its platform
message box owns wrapping and autosizing, without a fixed upstream dialog
resource geometry; inspect text exposure/fit and standard button grouping under
both size probes. Keep Copy dialog's existing normal/compact dimensions separate.
Directory summary suffixes use original Add_PathSepar.
Copy/Move menu and ordinary F5/F6 do not require marked rows: retain original
CFileMenu availability and let OnCopy's OperSmart choose items. With no marks,
verify the complete summary, Cancel and untouched input bytes in filesystem and
archive views under EN/RU normal/compact geometry. Empty writable folders are
an enabled no-op; busy/read-only/provider restrictions still apply. Retained
timer/notify guards must survive async handoff and release before terminal
synchronous panel refresh.
After Copy publication verify original source/destination saved states: ordinary
Copy/Move clears source marks, preserves the captured current item and peer
selection, updates the source title and restores source list focus. Shift keeps
its source saved-selection semantics. Inspect both source-panel roles under
EN/RU1100×640/640×400, including both address/list groups and toolbar. Do not
let delayed progress destruction replace the restored title. Offscreen window
activation may be restored by the driver; native focus/rendered parity still
requires native evidence.
Ordinary Copy/Move refreshes the captured destination panel even when typed
output is outside it. Verify both source roles and bounded/301-item publication,
preserved peer selection, successful bytes, overwrite Cancel and dialog Cancel.
Disable polling in the regression so the operation itself must publish rows.
Inspect the resulting address/list groups at1100×640/640×400. Namespace OnReload
must republish its existing provider rows through the same renderer.
Its source load stage must reject invalid directory-property types and property/
load failures before publishing rows. A failed reload leaves no stale rows and
restores notification guards; a later valid reload recovers the same list.
Inspect the empty namespace/error state at980×620/640×400 through the existing
address/list/toolbar contract. No new dialog layout is introduced by validation.
OnReload retains the original HRESULT message for load/property failures,
including LoadItems E_ABORT. GetNumberOfItems uses its written count regardless
of HRESULT; an unwritten output is initialized to zero by the Qt adapter.
Count metadata E_ABORT is not a load-operation Cancel. Inspect EN/RU both-pane
detail/icon written-zero/written-two/unwritten Refresh/timer/recovery states at
980×620/640×400; retain peer, selection and bytes. Parent and post-Copy/Move must
publish written counts without reporting that metadata status as a load failure.
The post-Copy selection stage must return an actual load/property failure before
reading failed provider state. Check both source roles for Copy/Move: output bytes, input
retention/deletion, marks, notification guards, source focus/title and successful
recovery on the next valid peer reload.
Verify direct result, explicit reload, changed-folder timer and
namespace/filesystem Refresh command under EN/RU; close the OS-managed error and recover
through a valid reload. Inspect message text, standard button group and exposure
under normal/compact probes, retaining platform autosizing. Archive synchronous
load errors must also reach this callback.
Filesystem reload keeps its bound provider through controlled E_ABORT and
allocation failure, empties failed rows, then recovers after a valid reload;
inspect both source roles and the Copy/Move filesystem-peer completion consumer.
Worker failures must retain their original HRESULT/provider, clear failed rows
and show the source message before async completion. Check both panels, EN/RU,
normal/flat navigation and Refresh, old-generation rejection and successful
worker recovery. A Copy/Move peer failure must restore source focus/title only
after temporary projection ends; inspect resulting panels at1100×640/640×400.
When a bound folder disappears, Refresh/timer must clear native items/operated
indices as well as visible rows. Preserve provider/guards/command recovery and
the existing missing-folder status; restoring the directory recovers control
bytes. Inspect both panels/EN-RU/standard-alternative empty views at980×620/640×400.
Typed missing descendants must publish the source closest existing ancestor,
its prefix/title and list focus, preserving peer selection and input bytes.
Verify both panels/EN-RU/absolute-relative/trailing separator at980×620/640×400;
Windows also strips original outer quotes before Qt path classification.
Native quoted-route execution and whole RefreshListCtrl acceptance remain required.
Source CalledFromTimer suppresses address publication during reload. Preserve
the draft text, cursor, selected text and address focus while updating rows;
ordinary Refresh must publish the current folder instead. Check both panels,
EN/RU, small/bounded-worker/flat filesystem and bound namespace/archive callers,
with peer state, native prefix/title and private control bytes unchanged.
Inspect resulting address/list groups at980×620/640×400.
When ShowDots exposes the parent row, source reload selects it when fallback
focus is row0 and restores it for an explicit focused name of "..". Exclude the
sentinel from native marks/operated indices. Open/Enter and Open Outside must
remain available for that selected row through the original source handlers.
Check hidden-row controls, both panels/EN-RU/standard-alternative selection,
small/worker/flat reload and existing parent icon/tooltip, with peer and control
bytes preserved. Inspect both groups at980×620/640×400.
Row icons follow original RefreshListCtrl system-interface/fallback policy,
including slow filesystem/stream folders, positive-index validity, attribute
masking and directory-bit correction. Keep icon lookup inside bounded GUI row
publication. Check real filesystem/ZIP small and301-item rows with real icons
on/off, source indices and unchanged private bytes. Attribute failures/Cancel
must reach reload owners and recover on a valid reload; asynchronous publication
stops once with its HRESULT and ignores cancelled/replaced generations. Inspect
normal/compact address/list groups; native shell appearance still requires a
rendered Windows reference.
Name display uses original OnNotify/SetItemText fast-name formatting: replace
RLO with underscore, compress runs of five or more spaces, show underscore for
an empty fast name and retain the Windows trailing-space marker. Preserve
unchanged native operation names and editor values. Failed generic name/prefix
properties display the original "Error: " cell text. During source notification
suppression, keep cached names or blank first requests. Check callback text and
indices at980×620/640×400, plus real ZIP Rename exact names/bytes, Escape and
invalid-name refusal. Qt keeps existing NFC and flat-prefix display adaptations;
native Windows marker rendering still needs a rendered reference.

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
The root namespace's empty source prefix remains an actual selectable empty
history entry, including after restart; Cancel is distinct from accepting it.
Native Windows device/ADS/UNC bookmark/history/restart consumers inspect the
owning address and list at980×620/640×400 and preserve the peer's draft.

### Source root command target (P3-05.13)

Reference: pinned resource.rc IDM_OPEN_ROOT_FOLDER, MyLoadMenu, App.h inline
OpenRootFolder, PanelFolderChange::OpenDrivesFolder and Panel.cpp list WM_CHAR.
The command/backslash/slash opens the drive list: Windows Computer prefix and
original drives; POSIX retains the documented CFSDrives mounted-volume service.
It closes archive links and uses the focused panel, leaving the peer intact.
Do not substitute the separate root namespace's Computer/Documents/Network rows.
Verify EN/RU normal/compact address/list/status geometry, command and both key
routes, source provider choice, parent/back restoration, selection and input bytes.
Current namespace consumers are recorded in the
[address/Root acceptance](../upstream-port-audit/p3-05-address-root-2026-10-17.md).
Full native/rendered parent acceptance remains separate.
The integrated Qt command/key consumers use the retained source drive transition.
The separate root namespace is reached through Parent; Back from a filesystem
location restores its actual stored namespace prefix. After source selection
restoration, same-row Open remains available. Current checks belong to the audit.
Live EN→RU→EN updates both bound drive addresses and the focused window title.
Preserve a pending address draft, existing drive rows and selection; verify both
panels at normal/compact sizes. The header reload uses the source path/icon/title
stage without adding history or enumerating the drive list again.

### Source address dropdown (P3-05.12)

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
Windows shell queries retain the requested source attributes, including the
directory policy for an ADS colon. Current device/ADS/UNC selection, refusal,
focus and closed-editor icon checks are recorded in the
[address/Root acceptance](../upstream-port-audit/p3-05-address-root-2026-10-17.md).
Full rendered icon/style acceptance remains with the parent workflow audit.

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

### Shared ListView: Properties and Folder History (P3-06)

Reference: pinned FileManager/ListViewDialog.rc and OnInit/OnEnter/OnOK callers.
Initial client is496×336DLU with native font/minimum protection; root margins
and footer gap are8DLU, buttons64×16DLU with native caption protection. Preserve
one-column hidden History header and the blank two-column Properties header.
Native standard-button ordering is retained. `list_view_dialog` writes EN/RU
source-initial and540×360/360×240 reports, checking exposed list/footer without
changing stored full values, clipboard/detail or acceptance behavior.

### Link (P2-05)

Reference: pinned FileManager/LinkDialog.rc and source OnInit/OnButton_Link.
Initial client is304×230DLU, mapped with the native Qt font and protected
minimum sizes. Root margins are8DLU; Browse is20DLU wide. Source path labels
start at8/48DLU vertically, Current target at78, Link Type at104 with90DLU
height. The footer uses64×16DLU buttons with native caption protection.
The source resizable/minimize/maximize capabilities are requested through Qt
window flags; native window managers may suppress modal title-bar controls.
The resource has no context-help title-bar button.
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
scrolling and footer exposure, plus native-font source initial frames. A group
frame may exceed a compact viewport; its native title rectangle and every child
must each be fully reachable. Current native EN/RU reports on three OS and
rendered original initial states are recorded in the
[secondary-dialog receipt](../upstream-port-audit/p3-06-secondary-dialogs-2026-10-16.md).
Native font metrics, compact scrolling and modal chrome prevent a claim of
complete rendered Windows pixel equality.

### Benchmark (P2-04)

Reference: pinned `CPP/7zip/UI/GUI/BenchmarkDialog.rc` and its source state bodies.
POSIX failed/empty affinity uses original System.h normalized processor fallback
in the hardware-threads leading CPU count and choice limits; that count must not
be zero or retain InitST's one when a larger processor count is available.
Windows retains its native source helper. These system failures do not change
grouping or controls.
Dictionary/memory precede CPU threads, then Size→Speed→CPU Usage→Rating / Usage→
Rating columns; Compressing and Decompressing retain Current/Resulting rows.
Elapsed/Passes and Total Rating precede CPU/version/system information. In the
normal resource, Restart/Stop belong to the upper right of the left control
pane; Help/Cancel remain in that pane's footer. The separate static log begins
at the top of the right column and spans the normal dialog height. It uses
plain, nonwrapping dialog-font text, not the total mode's fixed-font edit.
Compressing/Decompressing are group boxes; resource RTEXT metric headers and
values are right-aligned. Initial clients are488×264DLU normal and376×276DLU
total, with Qt font/minimum-size protection. Current tests assert these initial
frames, roles/grouping, footer placement, right alignment and logical focus.
Source normal logical tab order is Restart→Stop→Help→Cancel→Dictionary→Threads→
Passes, with Restart initially focused. Total initially focuses its console.
Both source templates request minimize/maximize/resizable style and no
context-help title-bar button; Qt retains native modal-chrome conventions.
Total mode has elapsed time and a fixed-font console. Qt scrolls the normal control pane at
compact sizes and replaces Win32 rectangles/font setup with Qt layout services.

`benchmark_lifecycle` writes source-initial and normal/total EN/RU JSON at960×650 and640×400,
checks all named labels/inputs after scrolling, fixed footer exposure and live
language changes. Historical reports on all three OS were reviewed2026-10-03: normal compact
controls scroll vertically/horizontally, metrics captions fit; buttons remain
fully exposed. Total console is618×326 compact and938×576 normal. Cancel/Help use
the original standard language mapping402/409; control IDs2/9 are not language
IDs. No screenshot or rendered Windows reference has been used to claim pixel
parity. Windows spanning group/error/system labels retain natural width; wrapped
source text updates also set its required height, since a grid span's height
calculation otherwise assumes the wider span. The Windows console is618×332
compact/938×582normal; Linux618×330/938×580. Lower system rows are expected below
the initial compact viewport and are tested after scrolling. Current Windows
960×650 reports also leave3px of the empty final SYS2 row below the initial
viewport; its label fits and the same per-control scrolling check verifies
reachability. This is scroll-content geometry, not clipped text. P2-04 acceptance
is recorded in the audit; whole-plan platform/rendered checks remain P3.
The current source-frame/grouping correction has36 native JSON per OS and ten
original Windows EN/RU reference states across List/Link/Benchmark. See the
[current receipt](../upstream-port-audit/p3-06-secondary-dialogs-2026-10-16.md)
for precise checks and deviations; the older console dimensions above retain
their historical revision.

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

### File-list drag target (P1-16 WIP)

Reference: pinned `PanelDrag.cpp::PositionCursor`, its `LVIS_DROPHILITED`
paint state, native right-drop popup pairs/masks and Copy-to-archive confirmation.
External middle-button entry uses the same source popup; source transfer flags
retain left/right precedence. Button release at Drop must not erase the entry
decision. `drag-middle-menu.json` records the current-build popup geometry.
Qt preserves per-panel path→view→status order. Hover highlights the folder through
a separate paint role without changing selection; DragLeave/Drop removes it.
Refresh clears the source panel's drop-folder name/index before loading or
restoring rows, including failure/Cancel. A subsequent leave does not address
an old row number in the new list; a new hover resolves its current folder.
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

Native adaptation,2026-10-15: outer margins follow source m8 using current
dialog base units; both columns grow their nonfixed fields, including all six
shared-width compression combos. Complete combo/group widget rectangles must
fit the viewport; source buttons/checks remain reachable by Tab. Normal and
compact EN/RU source-add reports cover native styles and an explicit
FieldsStayAtSizeHint style, with distinct `-size-hints` report tags. The lower
memory usage/limit group is scrolled into view before asserting both Threads
and memory visibility. Padding-only address-icon clipping has its separate
glyph contract; it does not relax Add control bounds.
[Native Add receipt and limits](../upstream-port-audit/p3-03-2026-10-15.md#p3-03p3-06--native-add-geometry-tab-traversal-and-display-fixtures-2026-10-15).

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

For long/super paths the original custom Browse fallback retains its top
Parent/Create/folder row, three-column list, path edit, conditional filter and
OK/Cancel footer. Its source initial client is272×336DLU, with8DLU margins/gaps,
exact24DLU top-button widths with16DLU minimum height and64×16DLU footer
buttons with native caption protection. Path/filter heights have14DLU minimums.
Hidden FolderMode filter retains its vacant row; Cocoa uses its actual widget
rectangle to preserve that source space. It requests source minimize/maximize/
resize without context-help. Source demonstration rows initialize column widths
through content autosizing, including the native icon slot.
`source_browse_dialog` checks the initial EN/RU frame and visible controls;
existing normal/compact navigation/filter/create/accept/Cancel tests remain.
Parent/Create activate on Return/keypad Enter; Create cancellation leaves the
filesystem unchanged. Initial focus remains the path edit. Bounded original
EN/RU and current native results are in the
[browser receipt](../upstream-port-audit/p3-06-source-browsers-2026-10-16.md).

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
in the [current browser receipt](../upstream-port-audit/p3-06-source-browsers-2026-10-16.md).
Named controls remain exposed at normal/compact sizes; full workflow/pixel parity
is not inferred from those reports. Earlier P1-15 check.sh175/175 is historical.

### Offline Help and About (P1-15.1)

References: pinned HelpUtils/MyLoadMenu/CompressDialog topic calls and exact
official26.03 CHM HTML/CSS with release provenance in resources/7zip.
QTextBrowser replaces Win32 CHM presentation, retaining pages/links/fragments.
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
are recorded in the [presentation receipt](../upstream-port-audit/p3-06-primitives-2026-10-16.md).
Complete pixel parity remains unverified; P1-15 acceptance includes cleanup above.

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

Menu checkbox rows and Folders radio rows retain the resource's10-DLU height /
14-DLU pitch through a4-DLU gap mapped using the current font. Native QWidget
frames must remain separate at normal/compact sizes. Options geometry waits for
activation/configure and retains minimumSizeHint when a style requires a taller
frame. The [native Options receipt](../upstream-port-audit/p3-06-options-spacing-2026-10-16.md)
records RED/GREEN geometry and platform boundaries.

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
POSIX; Help and F1 open the source Options topic through the application service.

`compression-options-{zip|7z}-{520|440}.json` covers actual 520×400 and 440×340
sizes, exposed controls and footer. Current Qt reports show small explicit-value
checkboxes beside their labels, no clipping and separate confirmation buttons.
Source visibility is applied before Qt computes minimum size so hidden NTFS
controls cannot enlarge the compact dialog. No rendered Windows reference was
available; these checks establish structure and usability on macOS only.

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

### Main menu and toolbar (P0-09.1)

Source Panel.cpp::OnCreate creates status bars with WS_CHILD|WS_VISIBLE and no
SBARS_SIZEGRIP; Windows/Control/StatusBar.h forwards those flags unchanged.
Qt's per-panel and shared transient-message status bars must likewise have no
resize grip. `main_shell` checks EN/RU at980×620/640×400, one→two→one panels,
unchanged Unicode message text, exposed shared bar and absent visible QSizeGrip;
`status-no-grip-*` reports record each phase. The window border remains the
platform resize surface. These checks establish Qt structure, not rendered parity.
Set the Unicode message before each panel toggle: creating the second panel
must preserve it. Panel construction does not publish a new operation result.

Source SetButtonText supplies the seven short button captions AND tooltips;
shared File menu QAction captions do not replace them after language/selection
changes. `language_ui` inspects EN/RU normal980×620 and compact640×400 reports
`toolbar-text-{-,ru}-{980,640}.json`. Tooltip text is asserted directly, without
a popup-pixel claim. Native rendered tooltip appearance remains unverified.

Opening the parent View menu runs source OnMenuActivating before display. Its
time submenu caption is the current day; five current-time examples and UTC
reflect the focused source panel and shared UTC flag. Source view/sort/two-panel,
flat/toolbar/auto-refresh checks are projected without executing commands.
`view_menu` covers both panes and EN/RU at980×620 and640×400, writing24
`view-menu-*-main.json`, `view-menu-*.json` and `view-menu-*-time.json` reports.
Both addresses and the actual visible table/icon lists must fit. Menu order and
action bindings are retained; reports do not establish rendered Windows parity.
For the closed address's decorative image, verify the complete painted glyph.
Qt6.12 Cocoa can clip one transparent pixel of its QLineEdit side-widget padding
while exposing the full16×16 logical icon. Reports retain strict widget bounds
and additionally record `iconBounds`/`iconFullyVisible`; clipping or hiding the
glyph remains a failure. A source selection's pending list-focus request survives
popup visibility until Hide, while a newer editor/peer interaction supersedes it.

Reference: `CPP/7zip/UI/FileManager/resource.rc::IDM_MENU`,
`App.cpp::g_ArchiveButtons/g_StandardButtons` and `MyLoadMenu.cpp` at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Six top-level menus follow
File/Edit/View/Favorites/Tools/Help. File follows the RC command groups;
independent Qt archive-workflow aliases have been removed. Unsupported source commands remain
visible disabled. Archive and Standard toolbar groups use the seven exact
large/small BMP pairs and source mask; text/size/visibility derive from the
native toolbar flags. `main_shell` asserts menu action order, IDs, shortcuts,
enabled state, active-panel routing and resource icons, then writes
`main-shell-980x620.json`, `main-shell-640x400.json` and File/View popup reports
for both sizes. Qt QMenu may scroll when its source command list exceeds the
available screen; JSON records its geometry and action tests cover non-widget
QActions. The Windows rendered menu is not yet available for pixel comparison.

Resource-pixel checks explicitly request DPR1 and retain both original BMP sizes;
native rendering can choose the appropriate high-DPI resource.

### Per-panel context and status (P0-09.2)

`source_context_menu` also inspects both pane owners, EN/RU table/icon lists,
mouse/keyboard Cancel at980×620 and640×400, with32 popup JSON reports; four
additional empty/Parent reports cover keyboard no-item admission. Source
OnContextMenu uses exact(-1,-1), origin for no operated items and otherwise the
focused item's rectangle. Qt substitutes visual-item centre/viewport coordinates
and clamps its popup to the screen. Action order/state is asserted separately
from widget geometry. Rendered Windows pixel acceptance remains unverified.

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
objects, so their order/enabled state is asserted separately. System shell groups
are available through the Windows bridge and disabled on other platforms;
the independent SevenZip group remains disabled. The upstream Win32 resource/source
establishes structure, not rendered pixel dimensions on macOS.
Native popup fixtures observe the owned visible menu inside its nested loop,
after activating the window/list, for mouse, keyboard and compact peer consumers.
See the [current Cocoa receipt](../upstream-port-audit/p3-06-native-shell-2026-10-16.md).

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

Windows Computer/Documents/Network row and address-popup icons use the original
CSIDL shell indices through SysIconUtils's native QIcon bridge. The closed
Computer/Network address uses that same source image. Icon selection does not
alter the control order or geometry; POSIX retains platform icons. Public
panel_item_icons verifies native image data and small/large sizes separately from these geometry
contracts. Whole rendered/HiDPI Windows reference acceptance remains open.

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
The footer is a compact block aligned right. Five standard buttons have equal
native-text widths; Auto Rename uses the source 104/64 width proportion, enlarged
if its translated text requires it. Cancel has a standard width and aligns with
Auto Rename's right edge. Hidden extra buttons retain their resource columns and
gaps, so Yes/No/Cancel do not move. File details start beside the top of each icon.
`overwrite_layout` covers EN/RU, normal/compact widths, extra-action visibility
and the public hide transition. Its initial-frame case checks growth between
8pt and 12pt fonts: the source 356×216 dialog-unit frame maps through
`QtDialogUnits` and may expand to Qt's minimum size. Responsive Qt layouts and
native font metrics are deliberate adaptations of the fixed Win32 dialog;
they do not imply identical pixel dimensions. Rendered reference evidence is
recorded in the [P3-06 audit](../upstream-port-audit/p3-06-2026-10-15.md).
Qt supplies one file icon; the optional second Windows EXE/ICO icon is omitted
explicitly in the contract. Widths must fit labels and footer buttons without
clipping. Default and native result IDs are checked behaviorally.

Memory initially maps the336×216DLU client through its native font/minimum
hint, with eight-DLU root margins,72DLU message area and62DLU Action group.
The limit edit/spinner pair occupies50DLU with native minimum protection,
indented10DLU from the root margin; GB follows after8DLU. Remember has a10DLU
inset from the radio buttons. Continue/Cancel are64×16DLU with native caption
protection, with a minimum10DLU gap after Action. The source has no minimize/
maximize/context-help title-bar buttons.
Memory order is native requirement/limit/RAM message, change-limit checkbox,
independent edit/spinner and GB label, Action group containing Allow/Skip and
Remember, Continue/Cancel. Source RAM-dependent default and spinner range,
text validation, NeedSave/Remember/SkipArc and cancellation are preserved.
The text field remains independent of the spinner range, including source
acceptance of zero; invalid text does not close the dialog.
Invalid-limit errors use source MyMessages' Critical/OK and 7-Zip title, with
user-question suppression; macOS hides QMessageBox titles. The pending dialog
and raw text remain available for correction in either policy state.
Qt spinner buttons adapt the original edit/up-down pair, preserving
the original `UDS_ALIGNRIGHT` paired width and `UDS_ARROWKEYS` keyboard path:
enabled buddy edit ↑/↓ invokes the same step operation as the buttons; disabled
input is unchanged. Up/down buttons are not independent tab stops. The raw text
parser and source range policy remain separate. Native button-box optical
margins may shift its backing widget by one pixel; source footer gaps are
asserted through public layout-item geometry, with actual control visibility
checked separately. This corresponds to Qt's
[QWidgetItem rectangle conversion](https://github.com/qt/qtbase/blob/6.12/src/widgets/kernel/qlayoutitem.cpp#L456).
These checks establish
structure and local Qt usability. Initial focus is Change allowed limit, as in
the first source tab stop. Native PanelOpen integration is checked in P0-04.1.
The bounded EN/RU original/current rendered comparison is recorded in the
[source-prompts receipt](../upstream-port-audit/p3-06-source-prompts-2026-10-16.md);
whole workflow and pixel equality are not established by those initial views.

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
`CPP/7zip/UI/FileManager/PanelSort.cpp::CompareItems/CompareItems2`,
`CPP/7zip/UI/Agent/Agent.cpp::CompareItems/CompareItems2/ComparePrefixes` and
`PanelItems.cpp` list state. Directories group before files in either direction;
the chosen typed property sorts first, then base name, flat prefix and source
index. The Agent interface compares flat prefixes naturally: equal `common.txt`
names under `dir2` and `dir10` place `dir2` first. The generic property fallback
for providers without IFolderCompare retains lexical BSTR order. Raw archive
properties pass the original raw flag to the provider comparator. Header and sort action
remain in sync.
Sort commands use the source property toggle; a newly chosen Size, PackSize,
CTime, ATime or MTime starts descending, all other properties ascending. Qt
headers use the same source default. A repeated PropID toggles even through
another generic/raw section; the source catalog owns its first-match raw flag.
The Qt header completes its automatic mouse sort before the source handler's
result is projected. Explicit programmatic direction requests remain explicit.
Sorting retains native selected indices
and ensures the focused row is visible, including a row moved outside the old
viewport. Reload retains the chosen property and direction.
The source restoration phase sorts after matching the focused name and before
choosing a fallback row by position. It publishes all four status parts after
selection restoration even if Qt emits no focus/selection signal. The owning
pane's labels update without switching focus or altering the peer's status.
The list, address, toolbar and status keep their existing order and stay
unclipped at 980×620 and 640×400. `panel_sort` writes
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
The parent View menu fills these examples before the child popup opens.
Changing either setting redraws visible time columns in place; day-level ties
still sort by source `FILETIME`. `panel_columns` checks the menu text and state,
filesystem/archive cells, persistence and fully visible list at 980×620 and
640×400 in `source-time-*.json`. These Qt reports do not establish rendered
Windows parity.

## File list settings

Options applies pages in source System/Menu/Folders/Edit/Settings/Language order.
An association error shows one Critical message without changing the current tab
or vetoing OK/later pages. Invalid RAM selects Settings and keeps its raw input;
earlier page values may already be committed, as in the source property sheet.
MyMessages suppresses that error when user questions are disabled. Message-box
titles are native-hidden on macOS and retain `7-Zip` on Windows/Linux.
`options_integration` and `options_dialog` produce current normal/compact-owner
error reports; inspect every visible message label/button for fit and clipping.

Reference: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/SettingsPage2.rc` and `SettingsPage.cpp`.
The Qt page follows the resource order: Show `..`, real file icons, full-row
selection, grid lines, single click, alternative selection, system menu, large
pages, extraction-memory label and checkbox/edit/arrows/RAM row, then OK/Cancel.
Alternative selection is enabled and persists the source mark mode.
Refresh resets the source group anchor to the first list row, independently of
restored focus. The next Shift-click marks that original inclusive row range;
a shown `..` remains excluded from operated indices. Ctrl-click starts a new
anchor. Selection colours, single focused row, totals and peer state stay in
sync after direct restoration, menu Refresh and timer reload.
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
Its initial496×336DLU frame,8DLU margins/list-footer gap and64×16DLU footer
buttons use the native Qt font and protect native minimum sizes. Native button
ordering is retained. Source minimize/maximize/resizable capabilities are
requested with Qt flags; no context-help title-bar button is added.
There is no separate Delete button in the reference. Delete affects all
selected rows, Ctrl+A/C copy/select, Enter confirms the focused path.
Long paths remain reachable through scrolling, without eliding stored text.
SingleClick changes activation to one click and enables hover selection without
confirmation. Modified selection clicks do not activate. Interaction coverage
is in `list_activation`; this adaptation does not change the geometry contract.
`folder_history` verifies list/footer visibility and separation at 540×360
and 360×240 and writes JSON reports. Qt geometry establishes structure and
usability. The original rendered EN/RU history states are recorded in the
[secondary-dialog receipt](../upstream-port-audit/p3-06-secondary-dialogs-2026-10-16.md);
widget geometry does not establish pixel equivalence or physical input.

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
[properties audit](../upstream-port-audit.md#свойства-и-двухколоночный-список-2026-09-28).
CEditDialog initially maps the336×256DLU client through the native Qt font,
with eight-DLU root margins/gap and64×16DLU Close protected by its native hint.
It requests the source resize/minimize/maximize capabilities without a title-bar
context-help button. CEditDialog has one read-only multiline
edit above Close, with both scroll directions and no automatic wrapping.
Text/Title are copied without truncation. Close has initial focus and is the
default button: initial Enter closes. Tab reaches the edit, where Enter remains
inside the edit (ES_WANTRETURN). Geometry: 480×320 and 320×240,
visible, separate text area and footer. Coverage is described in
[testing](../testing.md). Rendered Windows parity remains unverified.

## Selected properties

Window Properties now reads the bound source folder/operated indices. File
packed size is visible for filesystem and archive owners; empty source values
are omitted. Rows follow the source property catalog, with the existing full
current-location footer. Archive Modified text remains sparse raw compatibility
data until native time parsing; it is not a precision/timezone acceptance claim.

Reference: ListViewDialog.rc; source scope and baseline are recorded in the
[properties audit](../upstream-port-audit.md#свойства-и-двухколоночный-список-2026-09-28).
Two columns (property/value) fill the body with a visible empty header,
followed by OK/Cancel. No label form or custom selection title remains.
The same496×336DLU source frame and native-font/margin/footer policy as folder
history apply; header visibility remains controlled by NumColumns.
The value shown in the list is limited to 1024 upstream wchar_t units,
then ` ...`; CRLF/LF become spaces. Copy and Enter/double-click detail
use the full value. Delete is disabled for properties.
Geometry: list/header/footer at 540×360 and 360×240, long Unicode paths and
horizontal scrolling. Test coverage is described in [testing](../testing.md).
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

## GUI early error messages

The standard early GUI errors have no custom upstream dialog resource:
UpdateGUI::ShowDialog and ExtractGUI call Explorer/MyMessages' MessageBox
presentation. `gui_error_policy` checks the corresponding Qt Critical/OK boxes
at requested 480×220/320×160 in EN/RU, including fitted message labels and visible
OK controls; each of the six visible states emits two current-build JSON reports.
Suppressed states emit no box. Qt owns any minimum size adjustment and native
button placement; macOS omits the window caption. The original hardcoded English
texts remain unchanged. This is a standard message adaptation, not rendered
Windows pixel parity or a contract for custom Extract/Compress dialog geometry.
Current macOS boxes auto-fit to 228/304×116, Linux to 204/274×98 and Windows to
293/400×96 for writer/output errors respectively. Both requested sizes resolve
to these platform sizes, with fitted labels and visible/enabled OK; the requested
compact width does not override QMessageBox's standard fit policy.

## Shared name/mask dialog

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ComboDialog.cpp/.rc` and
`BrowseDialog.cpp::Dlg_CreateFolder`. Implemented view: `ComboDialog.cpp`.

- The prompt precedes one editable combo, with OK/Cancel at the bottom.
  Initial Value remains visible after populating ordered Strings.
- The initial frame follows the256×80 DLU source client template through the
  native font and Qt minimum-size protection; it scales with the font instead
  of retaining an arbitrary360×150px initial size. Explicit normal/compact
  resizing below remains a separate adaptation.
- Folder/file creation uses source title, prompt and default-name language IDs;
  selection masks use the same view with their own prompts.
- CPanel::SelectSpec owns the Select/Deselect titles, mask prompt, initial `*`
  and Cancel return. `source_selection` checks both languages/panes and standard/
  alternative marks, with32 current360×150/260×150 Combo reports. The same
  source owner handles an inactive-pane direct call without changing its peer.
- OnOK preserves raw text, including spaces and an empty value. Validation
  belongs to the calling operation. Cancel leaves the public Value unchanged.
- `combo_dialog` checks folder/file/mask views at 360×150 and 260×150:
  fitted prompt, reachable combo/footer, control order and separate buttons.
  JSON reports accompany the checks; screenshots remain optional.
- `source_create` exercises actual CreateFolder/CreateFile callers in both
  panes/languages and normal/flat views. Forty-eight current 360×150/260×150
  reports reuse this contract; creation keeps the source first-component focus,
  alternative marks and distinct failure-refresh policy. Cancel preserves the
  pre-dialog state. Source provenance and checks are in the
  [receipt](../upstream-port-audit.md#p3-05--source-create-filefolder-scenario-2026-10-10).
- ZIP comment uses source `PanelOperations.cpp::ChangeComment` with the focused
  item name and localized Comment in its title, localized IDS_COMMENT2 prompt
  and the existing raw property value. `native_comment_ui` checks EN/RU at the
  same two sizes: current comment-en/ru-360/260 JSON reports preserve prompt,
  editable combo and separately reachable OK/Cancel. titleID=0 preserves the
  caller's dynamic title; source language ID0 is the application name.
- Original Set Comment Error/HRESULT remains the error presentation. On macOS,
  which omits QMessageBox window captions, the caption is the visible message
  heading and HRESULT detail is informative text. Tests exercise real writer
  refusal on both languages and ordinary/AES ZIP. Comment now uses the shared
  source panel HRESULT/caption helper; its source-owned refresh follows the
  error, while dialog Cancel returns before refresh. Existing Combo JSON and
  normal/compact geometry consumers retain the same controls and text contract.
- Filesystem comments use the same source dialog, focused name/title and
  IDS_COMMENT2 prompt. `filesystem_comment` checks existing value, EN/RU,
  normal360×150/compact260×150, File Comment and Ctrl+Z, OK and Escape, selected
  rows and unchanged file bytes. Eight filesystem-comment-en/ru-0/1-360/260 JSON
  show a fitted prompt, editable combo and separately visible/enabled OK/Cancel.
  The source FSFolder writer trims the accepted comment; ZIP's raw whitespace
  behavior remains separate. No new dialog or rendered Windows parity is claimed.
  The source Escape/Cancel path does not reload the folder. An externally added
  item while the dialog is open remains unpublished until explicit Refresh;
  selection, focus, provider and peer rows stay intact. Accepted Comment reaches
  the source reload phase even after operation failure. Load failure/cancellation
  leaves no stale rows and releases guards; a later explicit Refresh recovers.
  Preserve the original ignored reload-result policy in this operation caller.
  Inspect both panes in EN/RU at 980×620/640×400 after Cancel and terminal load.

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
- The source initial client template is304×112DLU. The volume combo remains
  left-aligned at96DLU, including after resize; Browse uses20DLU width with a
  native minimum hit target and the source `...` caption. Only the destination
  combo expands horizontally.
  Font mapping and Qt minimum sizes protect labels/footer on each platform.
- OK/Cancel remain at the bottom. Invalid volume input opens the source
  Critical/7-Zip/OK modal message and keeps Split open, including accept().
  OK or Escape dismisses only the message; raw input remains available for retry.
- Combine uses the original shared Copy dialog: destination, Browse,
  read-only scrolling summary and footer. The summary lists the count and
  size, source prefix, first two names, ellipsis when needed and final name.
- `volume_dialog` checks normal/compact Split at 480×240 and 340×240 in
  normal/error states, and Combine at 480×250 and 340×250 with 2/3/5 parts.
  It writes JSON and verifies order, reachable controls, fitted labels,
  separated path/Browse and separate OK/Cancel.

Qt layouts replace OnSize arithmetic; Qt native folder browsing replaces
MyBrowseForFolder. The earlier inline-error adaptation is removed2026-10-10;
no extra error control exists in the source resource. Separate current-build
message JSON checks Critical/plain text/OK and native auto-fit; macOS ignores
its caption. `language_dialog` also checks English/Russian prompts and raw input
across language changes. Windows volume/language consumers use native QPA/fonts
and the existing UI resource lock. The original parser and partial-failure
vector remain authoritative; invalid submission starts no operation or directory.
Reports establish structure/usability on macOS; a rendered Windows reference
was unavailable and pixel-level parity remains unverified.

## Copy/Move dialog

`native_archive_ads_copy` applies this same resource contract to a nested ZIP
source and opened native NTFS-stream peer. Both panes and EN/RU exercise Copy,
dialog Cancel, occupied-stream Skip/Cancel and original unsupported Move at
440×250/340×220, with main address/list pairs at980×620/640×400. It introduces
no new controls; JSON establishes Qt geometry, separately from exposed/rendered
Windows reference acceptance.

The filesystem-provider command consumer `fs_provider_command` reuses this
source CopyDialog resource and controls for the opened NTFS-stream peer. Its
EN/RU F5/F6/Cancel/Skip cases cover both source panes, path/Browse/summary/footer
visibility at440×250/340×220 and both main address/list pairs at980×620/640×400.
The same suite checks accepted case-only F2 and Escape for file/directory rows
in both panes/languages, retained provider/selection and protected bytes.
Widget JSON establishes Qt geometry; exposed native Windows reference and
pixel acceptance remain separate. No new destination control or stream-specific
dialog is introduced.

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/CopyDialog.rc` and `CopyDialog.cpp::OnSize`.
Implemented view: `src/CPP/7zip/UI/FileManager/CopyDialog.cpp`.

- Destination label precedes the editable path; Browse stays to its right.
- Browse retains the original twenty-DLU utility-button width instead of the
  native style's ordinary push-button minimum, leaving the row for the path.
- Initial client follows CopyDialog.rc336×160DLU with Qt font mapping and native
  minimum protection. Source NormalizeSize(true) applies once at first native
  Show when the frame overlaps and exceeds the work area. EN/RU Copy/Move
  regressions cover9/12/32pt initial/compact states. Source resizable chrome
  requests minimize/maximize, subject to native modal-window conventions.
- Selection counts and known sizes precede the source path and first five
  relative names; a larger selection ends with an ellipsis. Folder names end
  in a separator. The summary stays below the destination and above OK/Cancel.
- Names preserve line boundaries. Read-only scrolling makes the displayed
  summary and long filenames reachable without enlarging the footer.
  The summary has no Tab stop, as the original static control; destination,
  Browse and footer remain keyboard targets. Enter preserves raw destination
  spaces/Unicode; Escape preserves the original public value.
- At 440×250 and 340×220 in Copy and Move states, the label fits, path and
  Browse do not overlap, the selection area and footer stay fully visible,
  and OK/Cancel remain separate. Qt may enforce its style minimum size.

Adaptations: Qt layouts replace Win32 resize arithmetic; QPlainTextEdit with
no wrapping replaces the static text. `CPanel::GetItemsInfoString` supplies
the upstream summary and eleven-line limit from the active source folder. `copy_dialog`
writes four JSON reports and checks these relationships with a large selection.
`agent_stream_copy_ui` also produces 56 Copy/Move/Cancel reports for FS/ZIP→WIM
archived ADS, both panes/languages, at 440×250 and 340×220; its 32 main-window
reports inspect both address/list pairs at 980×620 and 640×400. Current evidence
applies the same CopyDialog RC controls/relations/assertions, with source Russian
prompt IDs 6002/6003. It verifies transitions and retained marks/focus/rollback
through public tests; JSON alone does not prove those behaviors or native pixels.
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
- OK, Cancel and Help stay visible,
  distinct, and inside the window at requested sizes 640×360 and 470×330;
  Qt may expand to its style-dependent minimum size. Static labels fit.

P2-07 source_extract_advanced additionally checks EN/RU700×390/600×340 exposed
controls, checkbox width and source grouping/order. Four JSON reports reviewed
2026-10-03: compact modes are286 pixels wide, Russian ElimDup requires285;
inspected bounds equal exposedBounds. Rendered Windows parity is unverified.

The source caller supplies the full single-archive path in the window caption;
multiple archives use the plain Extract caption. Language changes preserve
that path and pending input. Eliminate duplication remains enabled independently
of the caller. The source resource has no destination hint.

`dialog_path_rules` adds EN/RU × split on/off Create/Extract Cancel/confirm
reports at 700×390/600×340 (Extract) and 760×720/640×480 (Create).
It asserts complete destination, mode, password, duplication and visible
folder-name fields, archive-name input and footer exposure. Source grouping and
conditional folder-name visibility remain as above. Editable values are checked
by the workflow, not inferred from redacted JSON; native separators and NBSP
preservation change values without introducing controls or captions.

Current 2026-10-07 toolbar, language and advanced Extract/Copy reports were
reviewed on macOS, native Windows Qt and Linux. The mode/password columns,
checkbox text and footer remain exposed at normal/compact sizes. Requested
Russian470×330 expands to the style-dependent minimum (macOS525, Windows483
pixels wide). The standalone Windows GUI consumer also checks public UIA
exposure/enabled states and source caption. This does not establish original
Win32 pixel parity; [the audit](../upstream-port-audit.md#p3-05--toolbar-extracttest-source-consumers-2026-10-07)
owns the platform evidence.

Adaptations: the pinned desktop resource and Qt use combo boxes for modes.
Help uses the source topic. The blank folder-name checkbox uses an accessible
tooltip; Qt layouts and platform button ordering replace Win32 positioning.
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
- New and existing targets both expose Update mode above Path mode in the
  right column; Path mode offers Relative, Full and Absolute. Source
  CompressDialog.cpp::OnInit attaches/populates both combos without testing
  target existence, matching CompressDialog.rc and the current Qt constructor.
  The new-target native Linux state is recorded in the
  [native receipt](../upstream-port-audit.md#p3-03p3-04--native-linux-package-workflow-and-desktop-recovery-2026-10-14).
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

## Source long-path browser

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`FileManager/BrowseDialog.cpp/.rc`, `IDD_BROWSE`. The original custom browser
is active for long/super/device paths; it is distinct from the normal native
folder/file picker. Preserve Parent, Create Folder, current folder on the top
row; single-selection Name/Modified/Size list; editable path, filter, OK/Cancel.
Folder mode hides the filter; zero/one file filter disables it. Parent and
directories remain before files regardless of sorting direction. Changing the
filter reloads the list, keeping the source-selected filename where possible.
Backspace/Control+R/F7 and list Enter retain source parent/reload/create/open
handlers. OK outside the list resolves raw input through source GetFullPath;
Cancel preserves the caller's FilePath/filter. Folder acceptance adds a separator.
Ordinary native pickers use source CommonDialog flags: Open permits a new name,
Save does not add an overwrite prompt; callers own subsequent validation.
Source settings control dots and single-click behavior. Qt layouts, icons,
accessible button names and locale time presentation replace Win32 services.
The current-folder caption elides in the middle; tooltip and accessible name
retain the full source prefix, while the editable path keeps the raw value.
Normal 560×640 and compact 420×400 reports check grouping/order, fitting labels,
visible path/filter/footer and row values. Rendered Windows parity is unverified.

## Archive password prompt

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/PasswordDialog.cpp/.h/.rc` (`IDD_PASSWORD`).
`CPasswordDialog` shows a prompt, password edit, Show Password, then OK/Cancel.
Initial client is216×88DLU, with eight-DLU root margins. The prompt is8DLU
high, the edit begins at20DLU with14DLU minimum height, and Show Password begins
at42DLU with10DLU minimum height. Footer buttons are64×16DLU protected by native
metrics and use native standard-button ordering.
The minimum gap from Show Password to the footer layout item is12DLU; native
button-box optical margins remain platform geometry. The source has no minimize,
maximize or context-help title-bar buttons. Compact Qt resize remains an
accessibility adaptation of the fixed original form.
Initial focus is the password edit. Initial Password/ShowPassword are applied before display. Toggling copies the
current edit into public state, then changes echo mode without changing its text,
as in ReadControls/SetTextSpec. OK/Enter commit raw text; the caller discards a
rejected dialog and keeps the existing backend cancellation policy.

`password_dialog` checks masked/shown states at 360×170 and 260×170, control
order, label fitting, footer visibility and separation. `ui_parity_password`
maps all five resource controls to these current-build reports. Behavioral tests
cover initial values, Unicode/spaces, show/hide, OK/Enter and Cancel. Editable
password values stay out of reports. Russian captions and Qt button ordering
are documented adaptations. Windows rendering remains unverified.

`password_window` adds current-build 360×170/260×170 reports for accepted
Unicode, surrounding spaces and empty text during source archive opening.
It also checks the subsequent asynchronous process prompt, raw-value reuse,
Cancel/reopen and reset on another archive. `open_callback`/`update_callback`
cover empty and supplementary Unicode through the portable UString/BSTR boundary.
These add caller coverage; the resource layout and labels are unchanged.

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
states. Captions follow source language bindings; clipboard shortcuts also accept macOS Meta.
Background is the source default button and receives initial focus when native
priority capability is available; Enter on focused Pause toggles pause instead.
Real priority failure restores the previous caption/title and reports a source
Sync error. Cancel uses source YES/NO/CANCEL and Escape-as-Cancel, temporarily
pausing and restoring prior pause. Native completion while the question is open
is deferred until the answer; completed errors hide Pause/Background and focus
the default Close button. Native message boxes use plain text. Their Qt platform
layout is checked separately from the 26-control source resource; macOS ignores
message-box window titles. Source MB_YESNOCANCEL and success MB_OK have no icon
flags: both Qt boxes use NoIcon; MB_ICONERROR uses Critical. Cancel's existing
normal/compact answer matrix checks the public icon. Final-error/success reports
request480×200/320×200, but native QMessageBox auto-fit determines actual sizes;
these requests do not establish two distinct rendered sizes. On Windows all
four progress-report producers and Cancel use the native QPA/font database,
as the main dialog already does. The offscreen plugin's missing-font fallback
metrics are unsuitable for application geometry. Add in an open archive was the first operation consumer
under P0-10.1. P0-10.2 connects the other current archive, filesystem and volume
operations. Filesystem/volume workers expose cancellation and real background
priority, but Pause is disabled because they have no worker pause capability.

MessagesDialog.rc (`IDD_MESSAGES`) uses a two-column numbered/message list and
Close footer,456×176DLU initial client and resize/minimize/maximize capabilities
without context-help. Assigning a parent must preserve those source flags.
Close has initial focus, matching the first resource tab stop.
The Qt source port preserves AddMessageDirect/AddMessage and the
OnInit/OnSize control flow; `IDL_MESSAGE` is above `IDOK` at 760×450 and
640×380. Current-build JSON confirms both controls are visible, separated and
within the dialog. Create follows the source modal lifetime until Close/Enter
(IDOK) or Escape (IDCANCEL), retaining the stack-owned PanelDrag message vector.
The independent File-menu journal was removed under P2-09. Font-DLU scaling
may enlarge the Close button to its Qt size hint. Windows rendered appearance
remains unverified.
No Windows rendered reference; geometry does not establish pixel parity.
The original IconID branch loads the exact FM.ico through a Qt ID/resource
service, including native 16/32/48 frames; default negative IDs preserve the
existing icon. Image decoding tests do not verify Windows title-bar rendering.

When extending a view, update its specification and add the relevant geometric
relationship or conditional state to its focused test. Add contracts for other
views as they are changed rather than rewriting all UI tests at once.

Create compression method states follow pinned CompressDialog.cpp/.h/.rc:
Store has empty dictionary/order/solid selectors, with threads still available
according to the original no-method policy. TAR GNU/POSIX has empty numeric and
thread selectors and hides both memory rows. GZip has its single Auto dictionary
and no threads/solid choices. An unrecognized external method retains empty
dictionary/order and unknown memory values; it does not borrow Copy's zero.
Empty selectors remain visible and disabled. Returning to ordinary 7z restores
the choices and memory rows. The two memory QWidget wrappers are presentation
containers, not additional resource groups. `compression_method_state` probes
EN/RU at760×720/640×480, reaches each field by scrolling and writes28 JSON reports.
These establish conditional visibility, row order, text fit and footer geometry;
original rendered Windows appearance remains unverified.

Create validation messages retain the original native call policy: password,
Browse/path and invalid-volume errors use MyMessages' suppressible Critical/OK
box with the 7-Zip caption; memory refusal uses unconditional Critical/OK, and
the small-volume question uses unconditional Yes/No/Cancel. Qt/macOS omits the
message-box caption. `compression_error_policy` checks EN/RU at requested
480×220/320×160 with Qt auto-fit, full label text and reachable buttons. These
standard platform boxes have no separate CompressDialog.rc control group.
Their JSON is geometry evidence; rendered original Windows appearance and native
desktop input remain separate acceptance requirements.
