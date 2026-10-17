# P3-06 — native association commands and associated archive opening

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

2026-10-17, parent `e95cc8a` plus this integrated correction. This closes two
observed association workflow failures; it does not close the P3 parents.

## Source and ownership

Baseline https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed original
`CPP/7zip/UI/FileManager/SystemPage.cpp::GetProgramCommand/OnApply`,
`RegistryAssociations.cpp` complete read/add/delete bodies and their headers,
`CPP/7zip/UI/Agent/ArchiveFolderOpen.cpp::LoadFormats/OpenFolderFile`, and the
existing FM startup, Agent open, backend discovery and Qt event consumers.
The existing [body ledger](../omitted-scope-audit/ledger.md) retains the complete
module/dependency dispositions. No module is newly copied/omitted, no baseline
revision or source language changes, and original notices remain intact.

Original System registration constructs a quoted native module-directory path
to7zFM.exe. The adapted source uses the running Qt executable, whose pathname
has `/` separators. `QDir::toNativeSeparators` now supplies the native pathname
at this registry-command boundary. The source state cycle, quoting, Add/Delete,
icons, first-error policy and notification flow remain unchanged. The existing
Windows platform continues to leave protected UserChoice to Windows Settings.

Original independent FM startup loads its codec owner before archive binding.
The Qt command-line entry already awaited discovery, but the macOS file-open
consumer constructed a default window (starting discovery) and immediately
called openPath. Backend busy admission then returned E_FAIL. The corrected
entry adapter creates an independent window/backend, opens the target only
after codecCatalogReady success and presents discovery failure without starting
an archive read. Closing the window cancels/joins its owned discovery; another
window retains its independent state. The helper belongs to the existing
qtfilemanagermain owner, with substantial code outside main's connection lambda.
Upstream has no portable macOS QFileOpenEvent/discovery/lifetime implementation;
this narrow Qt entry adaptation reuses the existing source archive opening and
backend services instead of introducing another archive algorithm or transport.

## Windows native regression

`windows_system_registration` uses a fresh GUID extension and verifies absence
of its exact HKCU/HKLM keys before testing. Actual Options Cancel writes nothing;
source Apply publishes the handler, Windows ShellExecuteEx resolves its command
and launches the owned test executable, and a Unicode/spaced file reaches that
recipient byte-for-byte. Clear removes the registration. A later external
preference change returns ERROR_RETRY and remains intact. Every owned registry
key and process handle is cleaned up; the normal archive extensions, existing
handlers, HKLM and UserChoice are outside this fixture's mutation scope.
This tests real native registration/dispatch rather than asserting registry
strings or private implementation names. The existing fake-service coverage
could not detect the native command-resolution failure.

Actual old-product RED: source Apply succeeds, but ShellExecuteEx returns
error5/SE_ERR_ACCESSDENIED. Normalizing the same owned registration command
changes that call to success. Its diagnostic child did not finish within the
unchanged5s observation; that run establishes launch only, not received bytes.
The corrected regression completes normally and verifies the recipient bytes.
Native-path normalization of the input filename alone did not fix the command.
The first compile-only failure is a missing test conversion, not a product RED.

## macOS manual result and public regression

The user tested the actual7a8592f package and owned ZIP, reported working
associations but `E_FAIL : Unspecified error` on opening, and confirmed that
this was the prepared fixture and Archive Utility was restored through Clear/
Apply. Native NSWorkspace inspection independently finds the same original
public.zip-archive handler/path afterward; the ZIP and source payload hashes
are unchanged. Assignment/restoration is accepted in that exact manual scope;
no claim is made that a particular consent panel was observed. File opening on
the old package remains FAILED, separate from successful association mutation.

`associated_window` sends early and repeated QFileOpenEvents through the actual
window factory, creates a real ZIP with the official engine and checks both
independent windows' listed payload, archive/source preservation, close during
discovery, and failure without starting an archive read. The source-visible
busy/discovery gap and actual manual failure establish the old behavior;
the focused regression protects its complete Qt consumer rather than only the
previous signal-only event queue. Public Cocoa events do not become physical
Finder evidence. A refreshed package/manual retry is still required for that
specific failed user workflow.

The subsequent5ccf3f1 package retry is successful: the user reports the archive
opens, but also an ordinary directory window. This replaces the old E_FAIL
with a concrete remaining initial-launch UX correction; it does not establish
single-window acceptance. [Delivery and native identity/default observations](p3-04-native-associations-delivery-2026-10-17.md)
retain the exact artifact and observation boundaries.

## Checks and limits

Current macOS focused five suites pass in0.85s: associated_window, system_page,
association_services, options_integration and association_file_open. Native
Cocoa associated_window passes5Qt/0fail/0skip in1.466s, without screenshots,
Computer Use or OS input. Startup and upstream baseline/inventory checks pass
3/3 in3.46s. The integrated app builds incrementally.

Guest focused run `20261017T151351Z-93f39d89` passes all five suites on Linux
and six on Windows, including native HKCU registration. The final focused
Windows registration fixture extraction is checked again in native run
`20261017T151539Z-8cf0e695`: registration1/1 PASS1.12s. The same native run passes
associated_window5Qt/0fail/0skip on each guest: XCB0.370s, Windows1.071s.
All seven final source/build/test inputs match the current root and remain
unchanged after the native run. Linux uses private Xvfb/Openbox; Windows uses
the normal Limited interactive session. Existing incremental builds and caches
are reused. No slow suite or full gate is repeated solely for this bounded
entry/registration correction; unchanged parent cold failures are retained.

Evidence: `build/review/goal-native-associations-20261017/`, root
`build/review/windows-system-registration-*.log` and `associated-window-*.log`,
and `/private/tmp/sevenzip-system-registration-7a8592f/` manual fixture/baseline.
Historical harness and failing native attempts remain separate. Windows HKLM
mutation, protected UserChoice/OS Settings selection, refreshed packaged Finder
opening and remaining full native/cold acceptance are not established here.
Packages7a8592f predate this correction. Count44/47 remains; no push,
publication, upstream or VM/disk configuration change.
