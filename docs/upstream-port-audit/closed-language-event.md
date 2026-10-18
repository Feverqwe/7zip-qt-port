# Language notification after window close — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed original FileManager FM WM_CLOSE/WM_DESTROY, App ReloadLangItems and
ReleaseApp, Panel GetFolderTypeID and their Qt window close, quiescence,
destruction and language-event consumers.

The source close releases panel folders before destroying the native window.
Qt can keep a closed widget alive until deferred deletion, and a queued
LanguageChange can still reach it. During the concurrent localization work,
the default macOS checks crashed when retranslation queried a released panel's
folder type. A deterministic regression reproduced the same null dereference
by closing a populated window and then delivering a language event.

MainWindow retranslation now exits when its application state is closed. Folder
queries retain the source requirement that a current folder exists; the pending
drive-label translation branch also checks that requirement. The native Panel
API and close/release algorithms remain unchanged. This is a Qt event-lifetime
boundary; no reusable Win32 implementation handles delayed Qt notifications.
No source branch or dependency is omitted.

The existing app_lifecycle_window suite now checks a late language event on a
closed, still-alive window: it must remain hidden and start no backend read.
The pre-fix run crashed on macOS. After the fix, that suite and the existing
associated_window discovery/close cases pass. No layout or text contract changes
are part of this fix; rendered appearance comparison is unnecessary.

The drive-label branch belongs to the concurrent localization changes. Its
additional folder check remains with that pending branch; the independent
closed-state guard and regression can be reviewed separately.

The application build, app_lifecycle_window, associated_window and direct
language/lifecycle consumers passed on macOS arm64, Linux x64 and Windows x64
/ Qt 6.12. The focused batches passed eighteen suites on macOS, nineteen on Linux
(`20261018T104509Z-456be6c1`) and nineteen on Windows
(`20261018T104113Z-08a7e8e0`). An additional Windows native_shell_com shutdown
crash is separate and remains open. Existing incremental builds and compiler
caches were retained; no slow matrix or rendered UI comparison was requested.
