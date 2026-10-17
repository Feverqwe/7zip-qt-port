# P3-05.14 — persisted root and native namespace locations, 2026-10-16

## Source and adaptation

Repository: <https://github.com/ip7z/7zip.git>, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Reviewed `CPP/7zip/UI/FileManager/PanelFolderChange.cpp`:
`LoadFullPathAndShow`, `FoldersHistory`, `SetBookmark`, `OpenBookmark`;
their `App.h`/Qt address callers; `App.cpp::CFolderHistory::AddString`,
`AddUniqueStringToHead`, `Normalize`; and
`CPP/Windows/Registry.cpp::CKey::SetValue_Strings/GetValue_Strings`.
The original history writer uses length-delimited `REG_BINARY` UTF-16 entries
and retains an empty entry. The current QSettings QStringList storage also
retains it. An empty source prefix identifies the root namespace.

`LoadFullPathAndShow` unconditionally adds the source prefix to history.
The local `MainWindow::recordVisitedPath` previously discarded empty paths,
so navigating to root could not subsequently select that location in history.
Removing that independent early return restores the original caller policy;
existing `CFolderHistory`, storage, dialog and source-address dispatch remain
authoritative. Cancel continues to use a null QString; an accepted empty entry
is a non-null empty QString and navigates to root. No label or alias is invented.

This is a two-line removal in the Qt caller, with no new functional module,
copied dependency, language conversion or upstream modification. Existing
source modules retain their baseline and notices. Windows registry presentation
remains adapted to the existing QSettings service.

## Integrated regression and native consumers

`persisted_location` adds EN/RU root history selection before and after restart.
It observes an exact empty history item through the public dialog, exercises
Computer → Parent → root and history dispatch, and retains backend-call guards.
Actual macOS RED: 11 Qt pass / 2 fail before removing the filter
(`mac-creation-and-root-red-v53`). GREEN: 13 Qt pass / 0 fail / 0 skip.

The Windows-only `native_persisted_locations` uses twelve rows covering
device/ADS/UNC × both panes × EN/RU. It opens the exact source namespace,
saves and reopens bookmark 5, selects its history item, restarts and reopens
the bookmark, and checks provider type, peer draft and unchanged fixture bytes.
Device coverage enumerates only the existing device namespace; it performs no
raw-device IO. ADS uses a disposable Unicode/spaced NTFS file. UNC uses an owned
UUID share restricted to the current SID, removed by the fixture. The scoped
Highest session is needed for that share only; no general elevated gate or
disk changes were performed. No archive engine operations are expected.

Evidence root: `build/review/final-availability-8e9343b-20261014`.

| Platform | Current focused result |
| --- | --- |
| macOS | `mac-persisted-root-green-v55`: six suites, 330 Qt pass; `build/review/final-root-history-v57`: four direct suites default/Cocoa, 62 Qt pass each |
| Linux | snapshot `20261016T204741Z-dadffbfb`, `linux-final-root-history-v57-evidence`: four direct suites default/XCB, 62 Qt pass each |
| Windows | snapshot `20261016T204646Z-a017b4d7`, `windows-final-native-persisted-v56-evidence`: persisted 13 + native namespaces 14 Qt pass, default/native runs each |

All listed runs have zero failures/skips and passing source/configuration guards.
The initial Windows build `20261016T203920Z-9b6d612e` failed before tests because
the new test lacked its moc include; the corrected fresh snapshot above passes.
No product acceptance is inferred from that initial failed build.

## UX and remaining scope

The existing history contract uses 540×360 and 360×240, with eight new EN/RU
initial/restart root-history reports per native producer. The empty row is
asserted directly; item contents are not serialized by the generic JSON walker.
Twenty-four Windows namespace reports cover 980×620 and 640×400; inspection
in `namespace-locations-geometry-v56.json` confirms the owning address and list
are enabled and fully visible. Forty-eight reports in the Windows receipt
include the ordinary persisted-location consumers. No screenshots or OS input
were used. Structure/geometry do not establish rendered Windows pixel parity.

Earlier accepted archive/nested/legacy bookmark behavior is retained.
This receipt closes the previously missing native namespace/history coverage
and root-history defect. Parent source/UX/platform acceptance, rendered state
comparison and repeated Windows cold-codec failures remain separately tracked;
packages built from product `a2254e5` precede this functional fix and need refresh.

## Current source-contract acceptance (2026-10-17)

Product sources remain `e3b3c652d4d2489d9143f3002c498057d6bd3c38`; the current
test/documentation parent is `8a14b67`. Reinspection of pinned upstream
`PanelFolderChange.cpp::SetBookmark/OpenBookmark/FoldersHistory` and the current
adaptation confirms that the original stored source prefix, reverse history
rebuild, selected item and Cancel distinction remain authoritative. The Qt
caller still records an empty root prefix and distinguishes accepted empty text
from null Cancel. No source module, dependency, algorithm or copied scope changes
in this validation iteration.

The integrated selection is `native_navigation_window`, `root_navigation`,
`address_combo`, `panel_address_keys`, `root_command`, `persisted_location`,
`panel_key_source` and `window_placement`. These cover both panes, EN/RU,
filesystem and real nested archives, source Root/Parent/bookmark/history routes,
focus, preserved control bytes and normal/maximized/splitter Close/restart.
Only these direct consumers are built and run through the project wrappers;
the declined `copy_archive_refresh` and other slow suites are not selected.

| Platform/current snapshot | Result |
| --- | --- |
| macOS Cocoa v210 |8/8 PASS31.58s,173Qt/0fail/0skip,171JSON/0PNG |
| Linux v211, `20261017T082805Z-af151aa6` |default8/8 PASS14.18s,163Qt/0fail/10skip; XCB8/8 PASS17.97s,173Qt/0fail/0skip |
| Windows native v212, `20261017T082807Z-693951ca` |8/8 PASS69.80s,171Qt/0fail/0skip,167JSON/0PNG |

All eight test owners match current checkout bytes on all three OSes;
source-input and configuration-restoration guards pass. XCB uses private
Xvfb/Openbox; it does not claim physical-console input. Current root history
reports additionally check24 views across three OSes: EN/RU initial/restart at
540×360 and360×240. The tree and visible footer controls are enabled and fully visible.
The empty history item's content is asserted by the public workflow test,
because generic widget JSON does not serialize tree items.
Linux default's ten skips are the native right-Control layout-mapping rows
without the corresponding offscreen QPA support; all ten execute in XCB.
Windows has two fewer RootNavigation rows because the POSIX-only namespace
cases are not compiled there, rather than skipped or counted as passes.

The rendered original EN/RU Folder History reference and shared ListView frame
remain in the [secondary-dialog receipt](p3-06-secondary-dialogs-2026-10-16.md).
Current reports establish the Qt contract and fit, not pixel equivalence or
physical OS input. Final native/rendered/cold-platform acceptance remains with
P3-03/P3-06; these checks do not silently close the other source-navigation IDs.

Ignored evidence root remains the one above, with `mac-navigation-native-v210`,
`mac-navigation-history-geometry-v210.json` and
`linux-final-navigation-v211-evidence`, `windows-final-navigation-v212-evidence`
and `navigation-current-input-proof-v212.json`;
`navigation-history-geometry-three-os-v212.json` records the24 fit inspections.
The scoped Windows namespace fixture v213 runs at
`20261017T083349Z-711e7c30`:1/1 PASS9.46s,14Qt/0fail/0skip,24JSON/0PNG.
Its actual token guard confirms Session1, Elevated=True, Normal priority32,
and a single selected suite, `native_persisted_locations`. Every device/ADS/UNC
row for both panes and EN/RU executes, including successful explicit removal
of each current-SID-only share. Device scope remains enumeration, with no raw
IO or new disk. Source/configuration guards pass. The four current source and
fixture input hashes match the checkout. All24 namespace reports retain enabled,
fully visible owning addresses and lists at980×620/640×400.
Evidence is `windows-final-persisted-native-v213-evidence` and
`persisted-native-geometry-v213.json` under the same root.

P3-05.14 is accepted for its source-prefix/bookmark/history/nested-location
contract. The earlier [source migration receipt](p3-05-2026-10-06.md#p3-0514--persisted-source-location-consumers-required-scope-2026-10-06)
retains the original RED, legacy stored-path conversion, open-chain reuse and
outer-archive fallback after restart; this acceptance does not claim automatic
reopening of a closed inner chain. Current native namespace and history gaps
are covered above. Whole native/rendered/cold acceptance and the other remaining
source IDs retain their own requirements; no parent package counter changes.

The previously refreshed [e3b3c65 packages](p3-04-2026-10-16.md#delivery-after-options-keyboard-activation-e3b3c65)
already contain the root-history fix on all three OSes. This validation and
documentation-only iteration adds no product delta requiring another package.
