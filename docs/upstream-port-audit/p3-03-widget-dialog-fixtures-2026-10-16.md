# P3-03 — Widget dialog lifecycle in archive workflows (2026-10-16)

This component changes test presentation and response delivery only. The shipped
product remains0e7c0eec339b1b324f3c57a7386f1513bd860fd1. No upstream module,
algorithm, archive operation, language or platform bridge is replaced. Source
owners remain the recorded FileManager ProgressDialog2, PasswordDialog,
PanelItemOpen and BrowseDialog implementations at upstream
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17
([source repository](https://github.com/ip7z/7zip)).

`tests/fixtures/widget_dialogs.{h,cpp}` scopes Qt's widget-dialog preference to
the fixture and restores its previous value. Responders wait for the shown
dialog's nested event loop, exposure and activation. Queued timer delivery
allows another modal request to be answered while the previous callback is
still on the stack; stopped/destroyed timers suppress queued late callbacks.
Nine existing archive/password/progress/browser consumers share this fixture.
The browse test moves focus before public `selectFile`, as Qt6.12 deliberately
does not replace the filename editor's text while that editor has focus.

The protected external-editor consumer waits for update and temporary-copy
cleanup in a real bounded event loop. A Cocoa stack sample places the earlier
stall in PanelItemOpen's question `exec`, entered from QTest's manual
`processEvents` wait. The corrected suite passes without raising its90s cap or
changing password, cancellation, archive-byte or temporary-file assertions.
This is evidence for fixture delivery, not a claim about every Qt dispatcher.

macOS unsandboxed format/build/default gate passes324/324 in234.12s,
5187Qt/0fail/66skip. After the localized security-wait correction, all nine
affected default suites pass:151Qt/0fail/0skip,28.43s. Cocoa replay completes
eight whole suites,91Qt/0fail/0skip; the96-row external-editor matrix reaches
its90s cap. A separate representative editor replay passes18Qt/0fail/0skip
in35.16s, covering launch, all prompt answers, closing the owner, deferred owner
changes and six failure/read-only combinations across pane/language/view/
alternative-selection dimensions. Its whole default suite remains60Qt PASS.
The complete Cocoa matrix is not claimed as passing.

Linux official snapshot20261016T153616Z-ced3da6f passes nine default and nine
native XCB suites, each151Qt/0fail/0skip, with122 JSON reports and no PNG.
Windows passes the same nine default and nine native suites, each151Qt/0fail/
0skip, with122 JSON reports and no PNG. All twelve current code/build inputs
match both guest snapshots; complete source guards and restored CTest
configuration are verified. Both guests reuse eight rehashed official engine/
SFX inputs from protected producers; no new engine build is claimed here.

Actual macOS NSAlert/NSSavePanel/NSOpenPanel appearance and input are outside
these widget fixtures. The user's1f5e16a Add gutter and Save/Folder EN/RU
select/cancel attestations retain that revision. Separate actual message-panel
manual fixtures are prepared at `/private/tmp/sevenzip-native-alerts-0e7c0ee/`;
their result is pending. No Computer Use or OS-input workaround is used.

Ignored evidence root: `build/review/final-availability-8e9343b-20261014/`.
Full gate and copied LastTest are `mac-widget-dialog-fixtures-full-*`;
`mac-archive-widget-dialogs-security-v11`–`v14` retain targeted RED/GREEN traces;
`mac-widget-dialogs-v15` and `v16` retain whole and representative replays;
`{linux,windows}-final-widget-dialogs-v15-evidence/` retain the guest receipts. The v15
controller leaves an already-present `-v1` command unchanged, so its editor
replay is the whole matrix; v16 explicitly replaces either verbosity form and
its recorded command proves the selected slots. One diagnostic v12 source guard
detects concurrent documentation edits; v13 and final replays verify unchanged
inputs. Historical failures and source-guard limits remain visible.

Eleven opt-in slow suites remain excluded. This component does not close the
remaining native workflows, Windows cold-start stability or parent P3 acceptance.
