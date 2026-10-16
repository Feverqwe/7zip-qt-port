# P3-05.15 — retired native archive proxies and Copy completion, 2026-10-17

## Source ownership defect

Repository: <https://github.com/ip7z/7zip.git>, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Reviewed `CPP/7zip/UI/Agent/Agent.cpp/.h::CAgent::ReOpen`, folder initialization,
properties and binding; `AgentOut.cpp::SetFolder`;
`ArchiveFolderOut.cpp::CommonUpdateOperation`; original
`UI/FileManager/App.cpp::OnCopy`, `PanelItems.cpp::RefreshListCtrl` and their
current Qt reload/activation/command callers.

Original `ReOpen` deletes the owned proxies before opening their replacements.
Published `CAgentFolder` objects borrow those pointers. On failed reopening,
the previous directory pointer could survive and the local `IsBound` check
dereferenced it. Cocoa Copy cancellation exposed a real main-thread SIGSEGV in
`GetFolderProperty` → `GetFolderTypeID` → command availability during destination
activation. The filtered own-process stack is
`mac-codec-native-crash-stack-v69.json` under the final availability evidence root.
Adding a public stale-folder property check to the existing lifecycle regression
also produces SIGSEGV (`mac-proxy-lifetime-red-LastTest-v70.log`).

## Narrow adaptation

The source agent now retires its proxy generation before deletion. Folder
initialization records that generation, and `IsBound` rejects an older binding
before reading directory memory. Successful `CommonUpdateOperation` restoration
updates the current folder's generation alongside the existing proxy assignments.
Stale update/reopen entrypoints refuse invalid bindings. This preserves the
original algorithms, symbols, module boundaries and successful rebind policy;
it also prevents pointer-address reuse from reviving an old binding.
The existing operation/notification guards still serialize live proxy access;
the generation is a lifetime check, not a replacement for those guards.

The stable source type (`7-Zip.` plus the already recorded archive type) remains
available after directory data is retired. It is independent of proxy contents
in the original property switch and allows Qt to retain archive presentation
while its load reports failure. Directory path/data getters refuse stale data.
The lifecycle regression checks failed/repaired reopening of ZIP/WIM folders,
old root/child rejection, stable type and preserved input/archive bytes.

Qt reload activation now holds the target's existing notification guard until
the renderer can clear/reload the old rows. Without this transition guard,
availability tried to save selection against retired source indices, producing
a second native abort (`mac-codec-native-abort-stack-v72.json`). The guard ends
before deferred scan ownership is captured, preserving existing async restoration.

The upstream raw-pointer bodies do not provide this retired-binding guard.
This is a necessary local lifetime adaptation, using portable `UInt64` and a
private C++ atomic generation. No archive format/compression implementation,
source-language conversion, copied dependency, upstream modification or new
external ABI is introduced. Notices and the original baseline remain intact.

## Native cancellation fixture alignment

The old command fixture stopped at the first visible progress event and assumed
publication had not started. Cocoa had already published all seven tiny native
updates at that point. Source `ArchiveFolderOut` explicitly permits finishing
an entered publication despite a late stop and clears stop before reopening.
The fixture now records that source phase from the synchronized destination
path or already committed archive bytes. Before publication it still requires
the original archive byte-for-byte. Once publication has started, unchanged
bytes or an archive that extracts the exact requested replacement are the only
accepted results. The dedicated native update suite retains its deterministic
pre-publication failure/Cancel byte-preservation cases for all formats.

Copy/question responders reuse `WidgetDialogFixture` queued delivery and
readiness; native focus waits precede pane selection. All22 existing data rows
remain, with no deadline increase or large-payload substitution. This also
resolves the native Delete prompt hang. The temporary v74 CMake registration
was placed before target creation and failed configuration; v75+ register it
after `qt_add_executable`. That failed build is retained, not counted as checks.

## Current verification boundary

macOS core six suites pass127Qt in `mac-proxy-lifetime-green-LastTest-v71.log`.
The subsequent final type/activation/fixture changes pass the complete Cocoa
command consumer:24Qt/0fail/0skip,32.83s under the original60s cap,
`build/review/final-proxy-lifetime-v79`. Its14widget JSON reports require no PNG.
The final integrated fifteen suites pass683Qt each on macOS
(`build/review/final-integrated-native-v80`,178.38s) and Linux
(`20261016T223148Z-48422e83`). Linux native XCB commands also pass24Qt
(`20261016T224329Z-83ca39fa`,14JSON/0PNG). These include all source lifecycle,
folder/proxy/capability, native update, interarchive recovery, Parent and three
nested routes. Current Windows snapshot `20261016T222256Z-67c427ba` passes
13/15 suites, including lifecycle8Qt, commands24Qt, native update73Qt,
recovery84Qt, Parent7Qt, nested stream137Qt and memory137Qt. External codec
discovery still times out; disk times out before producing a completed-row
report and requires separate diagnosis. Neither failure is counted as acceptance.

The subsequent disk diagnosis identifies a test-only artificial size leaking
into original Windows preallocation. Its corrected full disk/direct-consumer
follow-up passes167Qt on Windows; corresponding six-suite Mac/Linux follow-ups
pass441Qt each. The exact source diagnosis, regression and unchanged-cap results
are in the [long-suite receipt](p3-03-long-acceptance-2026-10-16.md).

The final macOS default gate passes324/324 in264.75s, with source guard passing
(`mac-current-full-v83`). It excludes the eleven explicitly requested slow suites,
whose separate receipts remain in the long-suite audit. All302 borrowed owners
are rehashed against the previous591174e inventory; only the four documented
Agent source/header paths differ (`source-inventory-proxy-lifetime-candidate-v87.json`).
The109 cpp disposition and original pin remain unchanged. Linux's current broad
gate then passes323/323 in513.57s,5186Qt pass/66platform skips, with source guard passing
(`20261016T225934Z-73529f4a`). It precedes only the subsequent test-only disk
metadata correction, covered by focused follow-ups. The fresh7dadf6d macOS
package also passes its bounded runtime/payload/dependency checks. Windows broad
gate, remaining failures and guest package refresh still bound final P3
acceptance; this receipt establishes the narrow retired-proxy correction only.
