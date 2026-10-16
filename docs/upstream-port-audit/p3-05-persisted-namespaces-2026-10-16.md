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
