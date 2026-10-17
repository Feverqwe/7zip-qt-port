# P3-06 — initial macOS document window

2026-10-17, parent5a9a6f0 plus this correction. Manual5ccf3f1 retry opened
the ZIP successfully but also opened an ordinary directory window. The archive
discovery correction remains valid; this iteration removes that extra window.

## Source comparison and adaptation

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
