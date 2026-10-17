# P3-06 — initial macOS document window

2026-10-17. Initial d0a74dc queue handling passed a posted-event test but failed
the manual Finder retry. The later native-completion correction below, on
parentd66577b, addresses the reproduced late document delivery.

## Initial d0a74dc source comparison and adaptation

Baseline https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Recompared original and adapted
`CPP/7zip/UI/FileManager/FM.cpp` WinMain/WinMain2/InitInstance, their startup
types/services and the desktop/event/window consumers. The existing body ledger
retains module dispositions. No module is copied or newly omitted; original
argv parsing, initialization, exception policy and independent archive owner
remain intact. The changed functional code is the existing Qt entry boundary;
upstream has no macOS Apple-event startup implementation to reuse.

Reviewed Qt6.12 Cocoa application delegate and event dispatcher from the
SHA256-verified qtbase source archive
`a951bd163c7b80fc6b8c88d7668fb56abf91c152373e13c10666763238131307`.
Its initial openFiles delivery occurs during application launch; the public
Qt dispatcher initializes NSApplication when processing events. The desktop
adapter therefore collects initial Cocoa document events before choosing normal
directory startup. Pending documents use the existing asynchronous event route
and independent window/backend factory. Later documents keep that same route;
normal argv startup remains in the source FM path. No launch delay, replacement
of existing windows, synthetic OS input or private Qt API is introduced.

## Behavior and validation

The macOS associated_window regression posts an initial local document before
the actual desktop entry/event loop. It checks one visible archive window,
then two after a later document, retention of the first, real ZIP contents and
unchanged archive bytes. Existing slots retain discovery failure/cancellation
and repeated independent-window coverage. The child entry probe now exercises
the same desktop route for normal and argv archive startup on all platforms.
These checks detect the reported extra window; signal-only queue coverage could
not detect it.

Final macOS focused associated_window passes, native Cocoa6Qt/0fail/0skip
passes3.281s. Earlier three-suite startup/event/window checks pass3/3 in2.15s.
Three current Cocoa widget JSON reports cover first980×620, compact640×400
and second980×620 states: toolbar, address, archive table and both status bars
are fully visible. Native menu presentation remains owned by macOS. No PNG or
pixel-parity claim. Final guest run `20261017T154034Z-ec84fb1a` rebuilds the
final associated_window target and passes1/1 on Linux0.635s and Windows2.043s.
The earlier three-suite entry/event/window runs pass on each guest:
`20261017T153406Z-c58f301f` Linux and `20261017T153409Z-d62cd24c` Windows.
Incremental trees/caches are retained; no slow/full gate is repeated
for this bounded entry correction.

Harness corrections before acceptance: dynamic_cast replaces an invalid
qobject_cast on a class without Q_OBJECT; the layout helper is linked explicitly.
Retention is observed before application quit, which legitimately destroys the
windows. A prior apparent CTest pass following a failed build is excluded;
the final build and focused check terminate successfully in sequence.

## User preference and limits

The user explicitly requested restoring Archive Utility after testing. A
command-line NSRunLoop-only setter did not deliver its completion within30s
and did not establish restoration. With the native AppKit application loop,
the setter completes without NSError and an independent NSWorkspace query
confirms public.zip-archive → com.apple.archiveutility at
`/System/Library/CoreServices/Applications/Archive Utility.app`. No OS input is
generated and no other file type is changed.

Evidence: `build/review/associated-bootstrap-*.log`,
`build/review/associated-bootstrap-native-reports/` and the owned manual fixture
`/private/tmp/sevenzip-system-registration-5ccf3f1/`. Refreshed packaged Finder
opening was retried by the user and FAILED: the extra window remains. No owned
Sevenzip process remains in the immediate native process queries, so the
executed bundle path cannot be independently confirmed. ZIP still resolves to
Archive Utility. The public posted-event regression is insufficient to accept
actual LaunchServices startup; further native event-order diagnosis is required.
HKLM/UserChoice, remaining native workflows and Windows
cold cause/stability are separate open acceptance scopes. Parent count44/47
is unchanged; no push, publication or upstream edit.

## Actual main preamble and late document delivery

A subsequent manual retry leaves both windows open. Native process inspection
now identifies PID45704 at the local build/dev bundle, despite selecting the
private app in the requested workflow. That binary contains the desktop entry
and its main calls it; this is not evidence of an older pre-correction entry.
The process has no document argv. The ZIP handler again resolves to build/dev;
the already-authorized AppKit restoration completes successfully and independent
query confirms Archive Utility. Only the owned test process/fixture is inspected.

The native Qt diagnostic producer initially omitted main's icon/name/desktop
identity setup. Adding that same preamble reproduces two actual MainWindows:
one lists the prepared ZIP payload and the other the containing directory.
Thus the earlier one-window producer was an insufficient acceptance oracle.
A temporary phase trace observes empty pending requests at Activate, normal
directory creation, then a native QFileOpenEvent reaching the ready receiver
and creating the archive window. This gives an actual native reproduction of
the late event, separate from directly posted Qt events.

Two diagnostic changes—installing the receiver before identity setup and using
AllEvents—still produce two windows. They are rejected and their product edits
are removed; no unverified workaround is committed. Temporary source
instrumentation is restored in a finally guard; its initial compile-only newline
escaping error is retained separately. Probe source, phase log and window/content
JSON remain under `build/review/goal-associated-launchservices-20261017/`.
The next correction must distinguish the native ordinary/document startup
request before committing to an ordinary window, without an arbitrary delay
or replacing an existing user's window. Physical Finder acceptance remains open.

## Native launch completion correction

A temporary native delegate observer confirms that document launch returns from
the manual event flush after willFinishLaunching but before openFiles and
didFinishLaunching. Ordinary launch completes didFinishLaunching during that
same flush. Apple's [open-file contract](https://developer.apple.com/documentation/appkit/nsapplicationdelegate/application(_:openfile:))
places initial document delivery before didFinishLaunching; its public
[finishedLaunching property](https://developer.apple.com/documentation/appkit/nsrunningapplication/isfinishedlaunching?language=objc)
corresponds to the launch-completion notification.

The File Manager entry now awaits that native completion before activating
pending documents and choosing ordinary startup. The narrow qtapplicationstartup
bridge uses public AppKit property/notification APIs and a scoped Qt event loop;
its observer is removed before returning. Already-launched Cocoa and non-Cocoa
test applications continue immediately. Early application quit creates no
ordinary window. The existing Qt flush remains for already-posted requests.
There is no timer delay, private Qt API, delegate replacement or existing-window
replacement. Original FM/Agent parsing, initialization, archive/backend flows,
baseline and module dispositions remain unchanged. No upstream macOS body is
available to reuse. Objective-C++ is needed only for the AppKit lifecycle bridge;
borrowed C/C++ keep their languages. The bridge belongs to the shared File
Manager entry object owner and uses the consumers' existing AppKit link closure.

macos_document_startup compiles actual src/main.cpp with a test entry symbol
and observes public Qt windows, retaining production GUI entry/backend/engine.
Its private bundle/extension receives an actual NSWorkspace document request.
ZIP bytes use an owned unfamiliar suffix, so no real archive extension/default
is registered. It checks one archive window, ordinary LaunchServices and CLI
startup, exact probe identity and unchanged Unicode/spaced archive/source bytes.
After process termination it unregisters only that bundle and removes temporary
files. It generates no OS input or screenshots.

Sensitivity removes only the native wait temporarily: actual main creates two
windows with one successful archive, without a timeout. The regression fails;
current source is restored. Final affected targets rebuild and macOS four-suite
startup/event/window selection passes4/4 in2.92s. The first integrated run also
exposed lost already-posted requests on offscreen; retaining the existing event
flush fixes that consumer without weakening the native oracle.

Native Cocoa associated_window passes6Qt/0fail/0skip in3.539s, covering repeated
documents, discovery failure and joined close. Three current normal/compact
widget reports retain fully visible toolbar/address/table/status bars;0PNG.
Linux `20261017T163557Z-b330b6df` passes3/3 direct consumers in1.772s test phase;
Windows scoped startup run `20261017T163601Z-be600684` passes3/3 in9.652s.
All11 source/build/test hashes match the root after collection. Incremental
trees/cache are reused; no full/slow gate is repeated for this bounded correction.
Evidence remains in `build/review/goal-associated-launchservices-20261017/`:
native lifecycle traces, RED/accepted LastTest, source guards and widget JSON.
Temporary source tracing is removed. The delivery and manual retry below
supersede the pending-package/Finder boundary; parent count44/47 is unchanged.

## Private delivery and manual Finder acceptance

Product `56c01ba2a7f26bc33993fd7975f95b08abf49410` is packaged on all three
platforms with Release/testing-OFF and1058 matching committed product/build
inputs. Existing incremental trees and compiler caches are retained.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS arm64 ZIP | 12537567 | `04bc8bdfa918754c8b87b9d073a656742b0e3383c8debb7b256762ee396e71b7` | 15 |
| Linux x64 TAR | 34218152 | `cfe0c2c66ef34da7e951a976d037737e7da1ceac420cbf6fcd3f2f1e2075d99b` | 14 |
| Windows x64 ZIP | 28576930 | `cf412c540ead3ba5470acae799a913c122a0ad5dd1acca5a2f0c2af0ce29aab6` | 14 |

Fresh payload/license/notices/SFX checks pass, including38 bundled Qt license
files. macOS strict ad-hoc signature,11 Mach-O/127 load/5 ID inspection,
bundled Qt private-symbol resolution and original ICNS bytes pass. Linux ELF
bindings/runpaths and Windows26 PE/345 imports/55 resolved system dependencies
pass. Windows native extracted color/mask match the source ICO for all three
packaged executables. These checks make no pixel-parity claim. Eight retained
runtime producer hashes match on both guests. No full/slow gate is repeated.

Guest packaging run `20261017T164432Z-fb18e29e` passes on both guests. An
auxiliary icon attempt fails before readiness with invalid inventory JSON;
the simultaneous storage query reports a host System.OutOfMemoryException.
The subsequent read-only memory query succeeds; this does not establish the
cause. Sequential icon retry `20261017T165228Z-afec7579` and storage retry
both finish successfully. Neither package validation nor that failed auxiliary
attempt is relabelled. Evidence: `build/review/goal-delivery-56c01ba/`.

For the fresh private56c01ba bundle, the user reports one window on initial
Finder opening, then clarifies that opening an archive with a window already
open yields two. This accepts the reported extra-initial-window correction and
preservation of the existing window in that manual scenario. No Sevenzip process
remains at the immediate identity query, so the executed bundle URL cannot be
independently established. This limit remains separate from the exact-identity
actual-main native regression above. An independent NSWorkspace query confirms
ZIP still uses Archive Utility; no additional default change is needed.

Protected Windows UserChoice/OS Settings, remaining native workflow and Windows
cold cause/stability remain separate open scopes. No publication, push or
upstream modification is performed.
