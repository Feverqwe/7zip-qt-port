# P3-03 — Bounded native external-editor coverage (2026-10-16)

The existing external-editor fixture now explicitly activates its owned window
before `qWaitForWindowActive` in the refused-update matrix. The first bounded
Cocoa outcome batch reproduces three activation failures:15Qt pass,3 fail.
This one-line test correction changes no product behavior, source body, data
matrix, assertions, timeout or slow classification. Product remains
`a2254e5b139632cb2427d57f085510d9fa1c28ea`. Source ownership is still
`CPP/7zip/UI/FileManager/PanelItemOpen.cpp` and its audited process/update
consumers at `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
[ip7z/7zip](https://github.com/ip7z/7zip).

The corrected complete default suite passes60Qt/0fail/0skip in12.72s on macOS.
All48 existing refused-update native rows then pass in three outcome batches
of16 rows:18Qt/0fail/0skip each including init/cleanup,46.35/45.69/28.31s.
The batches retain both panes, both languages, table/icon and standard/alternative
selection for controlled update failure, cancellation and read-only format.
All original archive/edited-copy bytes, provider/peer identity, selection,
notifications, error text, recovery and focus assertions execute.96 current
normal/compact JSON reports and no PNG are retained. The other native editor
slots have their separate earlier18Qt representative receipt; the product and
those test bodies are unchanged.

Generated CTest temporarily selects exact existing row names and native Cocoa;
the90s timeout remains unchanged and configuration is restored byte-for-byte.
All tracked source hashes remain unchanged across the run. The preceding whole
native-suite90s timeout remains historical evidence: these bounded results
establish row coverage, not a passing aggregate whole-suite execution. No new
large test matrix, slow-suite acceptance, NSAlert input or macOS screenshot is
claimed. Actual manual message acceptance retains its separate a2254e5 scope.

Linux snapshot `20261016T192000Z-6cc1a967` passes the whole default and native
editor suite:60Qt/0fail/0skip each,100JSON/0PNG. Source hashes match the current
fixture and product inputs; generated CTest restoration and unchanged-source
guards pass. Windows Limited snapshot `20261016T191848Z-eebac2e8` passes the
whole default and native editor suite:60Qt/0fail/0skip each,51.98/50.32s Qt times.
Its combined four-suite follow-up has124JSON/0PNG; all current code inputs match
and source/config guards pass. `codec_probe`17Qt and `panel_key_source`35Qt also
pass in both modes, including physical-1. The combined run **fails** in
external_codecs: initial readiness5300ms against5000ms in default mode; native
global-handler discovery times out and incompatible-module readiness5650ms
exceeds5000ms. Totals113/1 and123/2 Qt (pass/fail), no skips, represent the whole
four-suite results, not editor failures. The earlier full-gate key/probe failures
remain unexplained despite this successful follow-up; cold stability is open.

The accepted product packages require no rebuild for this
test-only change; default whole gates remain those recorded in the
[navigation receipt](p3-03-native-navigation-fixtures-2026-10-16.md).

Ignored evidence root `build/review/final-availability-8e9343b-20261014` retains
`mac-native-editor-outcomes-v45`: original failed log/source hashes,
default-LastTest, three exact row-selection files and native LastTest logs,
source hashes, JSON and restoration/result receipt. The same local build/dev
is reused. P3 parents remain open until their independent conditions pass.

## Separate privileged fixture follow-up

Windows Highest snapshot `20261016T191347Z-89330f3f` passes exactly twelve
disposable symlink suites in57.31s,191Qt/0fail/0skip: owned_temp, temp_cleanup,
source_extract_advanced, source_link_extract, link_dialog,
archive_creation_lifecycle, filesystem_link, panel_rename_state,
project_memory_setup, source_link_storage, file_manager_startup and
archive_metadata_ui. Source guards pass; the subsequent editor activation line
is irrelevant to these twelve consumers. This was separately approved scoped
validation, not an elevated whole gate. Its receipt is
`windows-final-symlink-fixtures-v44-evidence`. The two local-share suites remain
separate. Limited full failures are retained rather than converted into passes.
