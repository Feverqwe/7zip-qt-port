# P3-06 — Native Options geometry and source row spacing (2026-10-16)

The source baseline is https://github.com/ip7z/7zip.git commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Original MenuPage.cpp/.h,
MenuPage.rc/MenuPage2.rc, FoldersPage.cpp/.h, FoldersPage.rc/FoldersPage2.rc,
GuiCommon.rc control macros and the Options property-page callers remain the
authoritative owners. Their existing functional body/header/dependency
dispositions are in the omitted-scope audit. This correction changes only the
Qt presentation constructors of the two adapted `.cpp` modules. It adds no
algorithm, platform bridge, omission or duplicate implementation.

MenuPage2.rc's10-DLU checkboxes and FoldersPage2.rc's10-DLU radio buttons have
14-DLU vertical pitch. Menu's former4-pixel layout spacing yields Cocoa widget
frames20px high at y51/68: a3px overlap. Folders' native default spacing similarly
fails its existing strict row-separation regression. Both owners now map the
source4-DLU gap with the existing `QtDialogUnits(font())` helper. Cocoa compact
Menu checkboxes are20px high at y51/72/93, and Folders radios at y76/98/120.
The520×380 native frame fits all checked controls without overlap. No handler,
setting, command availability or archive behavior changes.

Fixture corrections are separate from this product defect. Native XCB can
deliver its initial configure after `resize`; the former single processEvents
call observes607×600 or520×400 instead of the requested compact frame. Tests
wait for window activation and the requested size before inspecting it. They
retain native minimumSizeHint, as the System-page test already does, rather
than forcing a frame below its minimum. Options' file/message responses and
the advanced compression-options modal use `WidgetDialogFixture`; System's
association-error responder uses queued delivery. Every existing fit, row-order,
width, setting-transaction and success/failure/cancel assertion remains.

Pinned original EN Menu and RU Folders rendered views from
`windows-original-options-v2-evidence/` were reviewed again. Their grouping/order
and four-DLU resource gaps support this adaptation; native Qt metrics/minimum
frames do not establish pixel equality. Cocoa current-build EN/RU normal/compact
JSON is inspected, without screenshots or OS input.

macOS affected four suites pass both default and Cocoa:48Qt/0fail/0skip,
22 native JSON reports. The required unsandboxed format/build/default gate
passes324/324 in427.38s,5187Qt/0fail/66skip, with unchanged source guards.
Linux snapshot20261016T155348Z-0652ce2b passes eight affected/direct suites in
default and XCB modes:88Qt/0fail/0skip each,128 JSON and no PNG. Windows Limited
passes seven suites; source_link_storage fails six disposable symbolic-link
setup rows because that token cannot create symlinks. Its Options geometry rows
pass. Snapshot20261016T155921Z-5af0a8e9 repeats only source_link_storage with
the authorized Highest token: default/native12Qt/0fail/0skip each, four JSON.
Together the seven Limited suites and that privileged fixture establish the
eight consumers,88Qt/0fail/0skip per mode; they are not one whole-gate run.
All seven current code/build inputs match both guest snapshots; uploaded-source
and restored-configuration guards pass. Eight official engine/SFX inputs are
rehashed and reused from protected producers without a fresh engine build.

The default Linux broad diagnostic's four geometry failures and Cocoa v17/v18
row-overlap/message-lifecycle failures remain available as RED evidence. v19
native geometry passes but its default fixed380px expectation fails against
the390px minimum; v20 uses the existing minimum-frame policy and passes.
No test cap or opt-in slow classification changes. Baseline verification passes
412 exact/198 adapted/three local paths across one revision.

Ignored evidence root: `build/review/final-availability-8e9343b-20261014/`:
`mac-options-native-v17`–`v20`, final v20 source/config guards and copied full
LastTest; `linux-final-options-native-v20-evidence/`,
`windows-final-options-native-limited-v20-evidence/` and
`windows-final-options-native-high-v21-evidence/`. The packages/manual NSAlert fixtures still
retain product0e7c0ee; these two presentation changes require a later package
refresh. Final main-window/native/stability acceptance remains open.
