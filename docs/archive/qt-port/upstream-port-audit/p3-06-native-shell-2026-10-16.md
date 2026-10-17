# P3-06 — Native Cocoa shell consumers (2026-10-16)

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Product remains `0e7c0eec339b1b324f3c57a7386f1513bd860fd1`; subsequent changes
are limited to two test fixtures. Original `App.cpp::g_ArchiveButtons`,
`g_StandardButtons`, `AddButton`, `ReloadToolbars`, `PanelMenu.cpp::OnContextMenu`
and their current Qt presentation/selection callers were inspected at upstream
https://github.com/ip7z/7zip.git revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Source toolbar pairs are24×24 and
48×36 pixels with a magenta mask. Keyboard context invocation requires the
focused list item when operated indices exist; mouse and keyboard admission
remain source-owned. No product algorithm, archive transport or resource changes.

The integrated current-build scenario covers main menus/toolbar, per-panel
context/status, View commands, language/focus, properties, source panel keys,
Browse/Temp, Overwrite, Progress, Split, standard buttons and live toolbar
language. Normal/compact and EN/RU assertions retain their respective fixtures.
All operations use disposable local files/archives and public Qt APIs; no macOS
OS input, Computer Use or screenshots are used.

## Actual failures and fixture corrections

Initial native v1 has12/14 passing CTest suites,208Qt/2fail/0skip. Its offscreen
phase passes14/14,200Qt/0fail/10skip. On Retina, implicit QIcon::pixmap chooses a
high-DPI resource: physical48×36 is not the expected unscaled24×24. The fixture
now requests DPR1 explicitly for both resource-size assertions and checks that
both original bitmap sizes remain available. Qt's [public pixmap contract](https://doc.qt.io/qt-6/qicon.html#pixmap)
distinguishes requested size/device pixel ratio. Native toolbar/layout consumers
remain in the test; this does not force the application's rendering to DPR1.

The popup fixture's zero timer can run before native QMenu::exec begins its
nested loop. A repeating scoped timer now observes the actual visible owned
panelContextMenu only after the loop level increases, asserts its original
actions/state/order and closes it through a scope guard. Both mouse positions,
keyboard(-1,-1) and the compact peer popup use the same observation boundary.
Actual exposed/active window and list focus are established before invocation.
An intermediate offscreen attempt loses activation after popup closure; explicit
reactivation before each invocation corrects the fixture. Deadlines, source
admission, action assertions and status/geometry checks are not relaxed.

Native v2 retains the remaining popup failure. Native v3 passes14/14 in61.97s,
210Qt/0fail/0skip and273JSON/0PNG; its offscreen activation failure is retained.
Final v4 after the localized activation correction passes both affected suites
offscreen2/2 in2.03s and Cocoa2/2 in1.23s,19Qt/0fail/0skip each,25JSON/0PNG.
Each invocation verifies unchanged source inputs and restores CTest configuration.
The final required unsandboxed macOS full gate passes324/324 in231.15s,
5187Qt/0fail/66skip. Eleven slow suites remain excluded. Guest whole gates
already running use the preceding fixtures; the updated fixtures' Windows/Linux
focused follow-up remains required and is not inferred from the Cocoa result.

## Appearance and acceptance boundary

Current main-shell/panel-shell normal/compact JSON, toolbar EN/RU and popup
reports are inspected for visible grouping, order, labels and fit. Hidden menu
actions are not treated as clipped visible controls. Cocoa's native global menu
bar has zero in-window height; widget reports do not observe hardware traversal
of that global bar. QMenu action order and public Qt consumers establish the
named behavior, not physical input or pixel equality. Windows rendered main
window/full workflow comparison and remaining platform acceptance stay open.

Ignored evidence root `build/review/final-availability-8e9343b-20261014` retains
`mac-final-shell-native-v1` through`v4`, their drivers/logs/source hashes and
`mac-final-shell-full-gate.log`/`mac-final-shell-full-LastTest.log`.
Historical failed attempts remain distinguishable
from final passing consumers. No push/publication/upstream changes occur.

## Manual macOS global-menu follow-up, 2026-10-17

The user checks the fresh private56c01ba bundle through macOS's global menu
bar: View switches one/two panels, Tools → Options opens settings, and Cancel
returns focus to the file list. Both commands and focus restoration pass by
manual attestation. This closes the concrete OS-menu delivery gap for those
commands, complementing the Qt action/order/state fixtures above. It does not
claim hardware traversal of every menu item, screenshots or pixel equality.
The source menu/command owners retain their baseline and the current main-chrome
receipt; no code, resource or archive-operation behavior changes in this follow-up.
