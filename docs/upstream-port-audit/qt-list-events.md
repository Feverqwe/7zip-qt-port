# File-list delayed events — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed FileManager `App.cpp::SetListSettings`, `ListViewDialog.cpp` setup,
activation and selection handlers, and the SingleClick setup in `BrowseDialog`
and `BrowseDialog2`. Upstream delegates hover tracking to the enabled native
ListView through `LVS_EX_TRACKSELECT`; Qt provides the timer adapter shared by
the main panels, history/properties and browse/temporary-file lists. Its callers,
view disabling during asynchronous operations, drag suspension, model-index
lifetime, modifier and visibility guards were reviewed together.

A queued hover could change selection after its view was disabled, and even
after a disable/enable cycle. Two behavior cases reproduced this on macOS
before the correction. The timer now requires an enabled view; viewport
EnabledChange cancels the pending target, including across quick re-enabling.
Mouse moves on a disabled view cannot arm tracking. Existing timing, activation,
selection owners and source commands are unchanged.

No upstream Qt timer implementation exists to reuse. This change is confined
to the Qt event-delivery boundary; no upstream body, dependency, algorithm or
command branch is copied, replaced or omitted. It changes selection state only,
with no layout/text/visibility or resize changes.

macOS arm64 / Qt 6.12, working tree based on `a0410fc`: both new cases failed
before the fix and passed afterward. Incremental application and affected test
targets built. `list_activation`, `list_settings_runtime`, `list_view_dialog`,
`browse_dialog` and `temp_cleanup` passed (5/5).

Run `20261017T214058Z-a902b999` passed all five suites on Linux x64 / Qt 6.12
and four on Windows x64 / Qt 6.12. Windows `temp_cleanup` failed only when its
existing symlink fixture lacked permission under the Limited interactive token.
Its completed empty leftover directory was inspected and removed; failure
logs were retained. Elevated interactive retry `20261017T214219Z-ca306c5e`
passed `temp_cleanup`. Both guests reused `current` and compiler caches.
These are focused behavior checks, without a full gate, slow suites or a
rendered Windows comparison.
