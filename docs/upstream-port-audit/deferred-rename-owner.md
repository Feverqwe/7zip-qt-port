# Deferred Rename ownership — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed `CPP/7zip/UI/FileManager/PanelOperations.cpp` label-edit handlers,
`PanelListNotify.cpp` notification dispatch, panel folder/selection ownership,
and their Qt table/icon editor, panel projection and backend completion callers.
Borrowed module baselines remain unchanged.

The source notification addresses its sending CPanel. Qt defers completion until
the editor has closed; its previous callback instead used whichever panel was
focused at delivery, carrying an index from the earlier folder. Switching panes
lost the intended rename. Navigating in flat mode could rename an unrelated file
at that index in the destination folder. Both failures were reproduced before
the fix for both source panes.

The Qt editor now retains the sending panel and folder, a persistent model index
and the original source path. Delivery checks that the item still belongs to that
folder and list, then runs the existing source handler under the sending panel's
projection. Modal progress and native backend completion retain that owner.
Navigation, list replacement and closed/busy operations retire the callback.
Selection-marker changes do not consume an outstanding text edit. Table and icon
editors share this boundary. Source name validation, folder operations, error
handling and refresh selection remain authoritative; no rename algorithm or
source branch was replaced or omitted.

The focused regression checks filesystem results and bytes after pane switching,
flat navigation, same-folder reload and alternative-selection changes, plus a
real native ZIP rename with a peer archive left intact. Existing rename,
selection, focus, navigation, reload, copy and application-lifecycle suites cover
the direct consumers. This changes event ownership, not dialog layout or text.

The application built and all 12 selected suites passed on macOS arm64, Linux
x64 and Windows x64 / Qt 6.12. Guest run `20261018T084859Z-625b8806` reused the
persistent worktrees and compiler caches; Windows used the elevated interactive
desktop. The missing official Linguist tools and their QtQml dependency were
added to the existing guest kits for the concurrently changed build requirements.
No full gate, opt-in slow rename matrix or visual comparison was run.
