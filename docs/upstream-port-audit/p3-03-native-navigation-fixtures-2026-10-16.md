# P3-03 — Native navigation and selection fixtures (2026-10-16)

This integrated fixture correction changes `filesystem_reload_tests`,
`panel_column_changes_tests` and their test-only CMake registration. Product
sources remain `a2254e5b139632cb2427d57f085510d9fa1c28ea`; the delivered packages
retain their exact product-input receipt. No upstream bodies, archive algorithms,
platform services, deadlines or slow classifications change. Source ownership
remains the audited FileManager Panel refresh/column/selection bodies at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
[ip7z/7zip](https://github.com/ip7z/7zip).

## Integrated fixture boundary

The Cocoa navigation replay completes fifteen suites,488Qt/0fail/2skip, but
`filesystem_reload` stalls in an actual NSAlert. The owned process stack identifies
the scanner failure question; its timer searches visible QMessageBox widgets.
That mismatch is corrected by the existing scoped `WidgetDialogFixture` and
queued responder. Original HRESULT/message/provider, stale-generation suppression,
empty rows, recovery, selection, control bytes and EN/RU/both-pane/flat assertions
remain. Automated widget geometry does not validate NSAlert; the separate user
manual message acceptance remains attached to its original a2254e5 artifact.

Windows' resting selection paint comparison also inherits State_MouseOver from
the physical cursor. Windows 11 can paint a hovered unselected cell like a selected
cell. The two reference paints now clear hover along with selected/focus before
setting the requested selection state. Primary cells must still differ and
secondary cells must still match. The schema-change fixture explicitly activates
its owned window before waiting for activation; the preceding Cocoa replay's
three activation failures are retained. No production paint/focus behavior changes.

macOS default/native `filesystem_reload` passes34Qt in each mode,64JSON/0PNG.
Its subsequent unsandboxed full gate passes324/324 in349.06s,
5191Qt/0fail/66skip. After the localized paint/activation fixture correction,
default/native `panel_column_changes` passes26Qt each,256JSON/0PNG. Windows
snapshot `20261016T185638Z-487e3931` passes both whole suites in default and
native modes:60Qt/0fail/0skip each,320JSON/0PNG. All three current code/build
inputs match that snapshot; the differing root inputs are six subsequent
instruction/documentation files and private snapshot helpers only.

Linux `filesystem_reload` snapshot `20261016T175952Z-5f8c1853` passes34Qt in
each mode,64JSON/0PNG. Its subsequent broad XCB matrix at
`20261016T183743Z-9fbfa437` passes235/235 in560.14s,
4505Qt/0fail/37skip,5537JSON/0PNG. It selects246 QPA suites before excluding
the11 slow suites. This closes the earlier broad230/235 fixture failures for
that snapshot; it precedes the localized panel paint/activation correction.
Current focused snapshot `20261016T190841Z-45613947` then passes both whole
suites in default and native modes:60Qt/0fail/0skip each,320JSON/0PNG. All three
current code/build inputs match; remaining differences are documentation and
private snapshot helpers. No broad Linux repetition is needed for this localized
fixture-only follow-up.

Each receipt preserves actual LastTest, source hashes and restored generated
CTest configuration. Existing normal/compact JSON retains both languages,
panel/view transitions, names/header order, visible controls and error text.
The fixture changes introduce no new visual product contract or screenshot.

## Current full gates and limitations

The independent current default Linux gate at
`20261016T173635Z-e6432462` passes323/323 in515.14s,
5183Qt/0fail/66skip. The same run's **Limited** Windows gate executes341 suites:
323 pass,18 fail in2632.07s,5360Qt/68fail/20skip. One additional device suite is
disabled under the no-new-disk restriction, and11 slow suites are excluded.
LastTest incorrectly describes the empty disabled entry as `Test Passed`; it is
excluded from the323 accepted passes. Generated disk exclusions and stock CMake
restoration/rebuild are retained with the source guard.

Fourteen Windows failing suites require privileged disposable symlink/share
fixture setup (including project-memory symlinks). The remaining four are:
panel_column_changes resting paint; external_codecs incompatible-module ready
at6650ms versus5000ms; codec_probe delayed-native-process completion at8950ms
versus5000ms; panel_key_source physical-1 bookmark delivery. The paint fixture
is covered by the passing current focused receipt above. The other failures
remain unresolved; neither warm repetitions nor changed deadlines establish a
causal fix or cold stability.

Automatic approval review previously rejected a general elevated full gate as
beyond the disposable-symlink scope. The Limited alternative really ran and its
failures are retained. No indirect elevated full gate, security-policy change,
AV process/scan export, new disk or upstream modification is used here.

Evidence stays under ignored
`build/review/final-availability-8e9343b-20261014`: mac-filesystem-reload-native-v37,
mac-panel-column-paint-native-v41/v42, linux-final-current-full-v35-evidence,
windows-final-current-full-v35-evidence, linux-final-filesystem-reload-native-v38-evidence,
linux-final-linux-native-default-v40-evidence and both
linux/windows-final-navigation-fixtures-native-v43-evidence. Guest receipts include
private one-off helpers, not committed product sources. P3-03/04/05/06 and the
43/47 count remain open; native Cocoa editor matrix, remaining source-owner
acceptance and Windows cold/physical/privileged checks are separate obligations.
