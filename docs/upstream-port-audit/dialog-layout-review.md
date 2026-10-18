# Dialog layout review — 2026-10-17

## Extract folder-name toggle — 2026-10-18

Screenshot review compared desktop `ExtractDialog.rc` and the complete
`CExtractDialog::OnInit`, `OnButtonClicked` and `OnOK` with the current Qt
constructor, initial state, destination preview, confirmation and persistence
consumers. Source baseline/repository are recorded below. Native Windows hides
the name edit in a fixed resource rectangle. Qt's equivalent visibility change
collapsed that row and shifted the modes below it.

The requested presentation correction keeps the name edit visible but disabled
when SplitDest is off. Toggling retains pending text and control geometry;
the source checkbox still controls whether the name contributes to the output
path. Path splitting, normalization, history, persistence, cancellation and
extraction algorithms are unchanged. This is an intentional Qt presentation
difference from native Windows visibility, with no omitted functional branches.

Normal/compact geometry now checks repeated toggles, unchanged rectangles,
pending text and visible/enabled state. Existing split-destination and path-rule
tests cover output paths and persistence. Focused macOS/Qt 6.12 suites:
`ui_geometry`, `split_destination`, `extract_dialog`, `dialog_path_rules`.
Native Cocoa light/dark screenshots are refreshed in the review gallery.
Windows/Linux and rendered original 7zFM comparison remain unverified.

Source reference: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Working tree based on
`b9d13fbca9f43fe3b9971e0a0ef6545b8fb99ec7`, with the preceding
[compression Options correction](compress-options-layout.md).

Reviewed hard-wrapping/checkable-control candidates in the existing Qt
presentation and historical widget JSON, using the existing UI contracts and
port audit for scope. Compared MenuPage.rc/MenuPage2.rc and complete
MenuPage.cpp initialization, language, Apply and event handlers with the port,
MenuPage.h and OptionsDialog's page consumer. SettingsPage2.rc/SettingsPage,
FoldersPage2.rc/FoldersPage and LinkDialog.rc/Qt link controls were also inspected
for similar spacing/wrapping risks. Existing source ownership is unchanged.

Confirmed defect: MenuPage's Zone.Id caption inserted a line break at a fixed
pixel width and left unused space before the combo. The Qt label now uses that
space and wraps only when necessary. No original functional bodies, registry
logic, control order or capability branches were copied anew, replaced or
omitted. Dependencies remain Qt Widgets and existing language services.
The EN/RU normal/compact regression uses a native-sized font and checks actual
caption fit and separation; it failed before the fix on both Cocoa and offscreen.

The Add default-size producer initially captured JSON before nested native
layouts settled. Its screenshot showed the complete RAM text. Processing pending
layout events before that report removes the stale measurement; no Add product
layout or memory calculation changes were needed.

Validation scope: macOS arm64 / Qt 6.12.0. Seventeen existing focused suites
passed before the caption change. Native producers cover Add/Extract, Copy/Move,
name/folder entry, Split/Combine, Password, Overwrite, memory prompt, messages,
details, properties/history, Link, Progress, normal/total Benchmark, About/Help,
and Options pages in EN/RU normal/compact states where the producer supports
them. Scrollable forms are inspected at the relevant scroll positions; hidden
or unsupported controls are not treated as visible clipping. Fixed multiline
memory/file-information text is intentional. This is the declared producer
scope, not every possible dialog state or OS panel.

Six maintained RC/Qt comparisons passed: Copy, Password, Overwrite, Mem,
Progress and System. Recorded adaptations (native footer order/captions,
scrolling summary and Qt control replacements) were reviewed, not suppressed.
Progress's process-priority fixture required an unsandboxed retry; that retry
passed. The original sandbox failure was environmental, not a product fix.
The final affected consumer checks are Menu page, Options, Options keyboard,
language UI, System page and UI geometry. Native evidence is retained under
`build/dev/dialog-layout-review/`, separate from offscreen fixture reports.

Final results: 5/5 affected Options suites PASS (1.87s), UI geometry PASS (0.39s),
16 native producer batches PASS. All 243 retained Cocoa reports were inspected:
no clipping outside the declared scroll-content boundaries, fitting visible
labels, and no checkable widgets stretched beyond their natural line height.
Affected Options and Add screenshots were reviewed against the JSON; the RAM
measurement now agrees with the rendered text. Incremental release app build,
`python3 scripts/package.py --install-only dist` and strict deep signature
verification passed; the local `dist/Sevenzip.app` includes the correction.

No full or slow suite is required for the local label/producer change.
Windows/Linux runtime and fresh rendered original 7zFM comparison remain
unverified; resource contracts do not establish pixel equality. Source review
completeness applies to the changed Menu presentation boundary, not a renewed
whole-application source-port audit.
