# Opening menu captions — 2026-10-17

Source reference: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed FileManager `resource.rc::IDM_MENU`,
`MyLoadMenu.cpp::ExecuteFileCommand`, `PanelMenu.cpp::CreateFileMenu`,
`PanelItemOpen.cpp::OpenItem`, Explorer
`ContextMenu.cpp::QueryContextMenu/InvokeCommand`, and Common
`OpenArchive.cpp::ParseType/ParseTypeParams/CArchiveLink::Open`, including the
Qt command and shell-window consumers. FileManager Open Inside navigates the
current panel; the Explorer Open command creates an independent window.
Explicit `*` stops after one archive layer, `#` selects the parser, and `#:e`
enables its each-position search.

The user requested clearer names while retaining the 7-Zip arrangement.
Only Qt presentation captions are adapted, in English and Russian: archive
navigation, external launching, independent-window opening, and specialist
types receive descriptive names. All original entries, ordering, grouping,
command IDs, shortcuts, enablement and source dispatch remain. The archive
picker also stays in its existing position. The Windows Explorer extension
and upstream language resources are unchanged. No new upstream code is copied,
no functional body is replaced and no source branch is omitted.

Dependencies: existing Qt program/context projections, shell command map and
local language services. Source transfer is unchanged; this is a requested
presentation adaptation, not an updated upstream baseline.

Validation: macOS arm64 / Qt 6.12.0, working tree based on
`b9d13fbca9f43fe3b9971e0a0ef6545b8fb99ec7`. Incremental dev build of `Sevenzip`,
`shell_menu_window_tests`, `source_context_menu_tests` and `language_ui_tests`;
focused CTest `shell_menu_window`, `source_context_menu`, `language_ui`: 3/3 PASS.
Existing scenarios exercise independent-window success, forced-type refusal,
password cancellation, picker outcomes and panel ownership. EN/RU at 980×620
and 640×400 checks renamed rows and expanded format-submenu bounds/text widths.
Current offscreen JSON shows all renamed rows exposed; format submenus measure
214×161 EN and 258×161 RU. No Windows/Linux or rendered pixel comparison is
claimed; no full or slow suite was run.

The existing incremental Release `Sevenzip` target built successfully;
`python3 scripts/package.py --install-only dist` refreshed the local macOS app
with verified ad-hoc signatures. Package staging was removed automatically.
