# P3-06 — List, Link and Benchmark presentation (2026-10-16)

[Index](../upstream-port-audit.md). This integrated correction covers the shared
Properties/Folder History list, Link and normal/total Benchmark presentation.
Other secondary dialogs and whole P3 acceptance remain open.

## Source and adaptation boundary

Baseline: https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Reviewed `CPP/7zip/UI/FileManager/ListViewDialog.cpp/.h/.rc`,
`LinkDialog.cpp/.h/.rc`, `CPP/7zip/UI/GUI/BenchmarkDialog.cpp/.h/.rc`, their
resource headers and `CPP/7zip/GuiCommon.rc`. Callers include Properties and
Folder History in Panel code, `PanelOperations::CreateLink`,
`Common/CompressCall.cpp::Benchmark/Call7zGui` and the GUI benchmark dispatcher.
Original bodies, state initialization, notifications, link services, benchmark
calculations/worker/callbacks and dependencies retain the existing source port.
The read-only upstream checkout is unchanged; notices remain in adapted files.

Changes replace presentation in adapted `src/CPP/7zip/UI/FileManager/ListViewDialog.cpp`
and local narrow `src/qtlinkcontrols.*`/`src/qtbenchmarkcontrols.*` adapters.
No new archive algorithm or command transport is introduced, and no module is
copied or omitted. Existing `QtDialogUnits` maps resource dimensions using the
native Qt font, with native control minimum protection. Source resizable,
minimize/maximize capabilities are requested; native modal window chrome may
suppress title-bar controls. No context-help title-bar button is added.

List initial client is496×336DLU, eight-DLU root margins/gap and64×16DLU
footer buttons. One-column History hides its header; Properties retains the
original blank two-column header. Qt native standard-button ordering remains.
Strings, clipboard, detail view, deletion and acceptance behavior are unchanged.

Link initial client is304×230DLU; path labels/rows, current target and90DLU
Link Type group follow resource order/spacing. Browse is20DLU, footer64×16DLU.
The unnecessary nested margin/scroll frame is removed. Compact scrolling and
the existing translated-radio wrapping protect native font metrics. POSIX
Junction/WSL remain disabled. The private `BuildLinkTypes` extraction keeps
construction cohesive; source link operations and callbacks are unchanged.

Benchmark initial clients are488×264DLU normal and376×276DLU total.
Restart/Stop and Help/Cancel belong to the left pane; the right static log begins
at the top and spans the height. It uses plain nonwrapping dialog-font text;
total retains its fixed-font edit. Compress/Decompress are group boxes, metric
headers/values are right aligned, and CPU/version share their source row.
The compact left pane scrolls while its footer stays exposed. Preferred width
includes the actual native scrollbar hint/extent. Initial focus/logical tab
chain follows the resource: Restart→Stop→Help→Cancel→Dictionary→Threads→Passes;
total focuses the console. Tests assert Qt focus/chain, not physical OS input.

## Rendered reference and regressions

Ten original Windows EN/RU states were rendered: archive Properties, Folder
History, Link, stopped normal Benchmark and initial total Benchmark. Original
7zFM SHA256 is`0f00ac1e2b860632a97001440115d477ea8e3e5e7a19e08814cb6a753cd98eed`;
7zG is`a3f3743ea3b13bee0f2cb033091c5f86a0ea803362c8d28cd825223f69534c16`.
Both run Limited/Elevated=False before startup and private registry writes;
private keys are absent afterward and fixture bytes are preserved. FM launches
its companion7zG for Benchmark; reference collection uses the same pinned GUI
dispatcher directly. The earlier missing-companion attempt is retained as
partial evidence and does not count as a completed reference.

Actual pre-fix REDs cover Benchmark initial frames (four failures), initial
focus (two), List/Link initial frames (eight), and missing source window flags
(twelve). Geometry tests retain normal/compact EN/RU and live-language checks.
For groups taller than a compact viewport, the public native style's title
rectangle and each child are independently scrolled into view; the entire
frame is not required to fit simultaneously. Browse verifies source width and
actual native button content/text fit; footer native size-hint guards remain.

Three-platform focused/native checks use exact product/CMake inputs:

| Platform | Focused consumers | Native presentation | Reports |
| --- | --- | --- | --- |
| macOS arm64 Qt6.12 |10/10,126Qt/0fail/0skip,11.65s |3/3,22Qt/0fail/0skip,1.94s |36JSON, no screenshots |
| Linux x64 Qt6.12 |10/10,126Qt/0fail/0skip,11.25s |3/3,22Qt/0fail/0skip,2.41s |36JSON/36PNG |
| Windows x64 Qt6.12 |10/10,124Qt/0fail/2skip,38.48s |3/3,22Qt/0fail/0skip,3.68s |36JSON/36PNG |

Windows skips are the two POSIX-only affinity fallback cases; native source
affinity remains in use. Windows runs in Session1/Highest/Elevated=True with
verified normal priority. Linux uses private Xvfb/Openbox, without changing HOME
or desktop settings; this does not prove physical-console input. macOS uses
public Cocoa Qt tests/JSON without OS input or screenshots. Its sandbox attempt
has no screen and fails reversible priority calls; retained separately, it is
not native UI evidence. An unsandboxed compact Link frame assertion failed
before the group-title reachability correction; that original failure remains.

The required unsandboxed macOS full gate passes323/323 in262.65s,
5160Qt/0fail/66skip, excluding eleven slow suites. This validates this product
component and the separate runner-priority prerequisite, not a new Windows
whole gate, current packages, pixel equality or complete native acceptance.

A localized follow-up adds an explicit zero horizontal-scroll range assertion
for the normal source-initial Benchmark. macOS offscreen passes1/1 in3.71s.
The current full native Cocoa Benchmark consumer also passes1/1 in1.41s,
16Qt/0fail/0skip with source inputs unchanged and generated configuration
restored; no screenshot is captured.
Windows snapshot `20261016T063803Z-fa19311e` passes the complete native Benchmark
and unchanged external_codecs/codec_probe/process_pause/process_priority
consumers5/5 in32.36s,53Qt/0fail/2POSIX-only skips. The visible native scrollbar
strip does not represent a nonzero horizontal range. This warm bounded result
does not establish cold codec stability. The preceding one-off attempt
`20261016T063516Z-cff875cd` stops before tests on a misspelled build-target name;
it is retained as a harness failure, not a product regression. One-off drivers
are removed from the source checkout after snapshot freeze and retained in
ignored evidence. No passing whole gate is repeated for this localized test.

Evidence root: ignored `build/review/final-availability-8e9343b-20261014`:
`windows-original-secondary-v2-evidence`, `*-secondary-native-v2-evidence`,
`secondary-native-v2-v3-product-proof.json`, `list-link-red-evidence`,
`secondary-flags-red-evidence`, `secondary-presentation-red`, focus RED logs,
and `mac-secondary-priority-full-*`. Final Cocoa receipt is
`build/review/secondary-presentation-native-v3`; guest run is
`20261016T062702Z-f814576a`. Generated CTest configurations are restored.
