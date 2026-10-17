# macOS application-menu exit — 2026-10-17

Source reference: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

Reviewed FileManager `resource.rc::IDM_MENU`, `MyLoadMenu.cpp` exit translation,
program/context-menu handling and `OnMenuCommand::IDCLOSE`, together with the Qt
action construction, menu projection, dispatcher and window close handler.
Upstream places Exit in File and sends WM_CLOSE. The port already dispatches
that command to the existing close handler, which retains its operation guards.

The Qt action now declares `QAction::QuitRole` before menu insertion. This is
a narrow Cocoa presentation adaptation: Qt can merge the existing action into
the native application menu without relying on its translated caption. The
existing platform Quit shortcut and command dispatch remain authoritative.
There is no upstream macOS application-menu implementation to reuse. No source
bodies or dependencies are copied or replaced, and no command branches omitted.

The follow-up disabled-state investigation also compared upstream `FM.cpp`
WM_COMMAND/WM_CLOSE with the port's synchronous-call lifetime adapter. A folder
refresh published disabled Exit while a `CallScope` was active; its destructor
then released the guard without republishing availability. A regression
reproduced Exit remaining disabled after Refresh. The adapter now notifies the
window when the outermost call returns. If Exit was disabled, the window queues
the existing command availability projection after stack unwinding. Execution
still rechecks the original close/lifetime guards. This notification is Qt
integration glue for asynchronous presentation, with no corresponding upstream
implementation to copy; no source algorithms or command branches change.

Follow-up validation: the added Refresh/Exit regression failed before the fix
and passed afterward. Incremental `Sevenzip`, `app_lifecycle_window_tests`,
`app_lifecycle_tests` and `command_policy_runtime_tests` builds passed; all three
focused suites passed. Lifecycle checks also cover Exit recovery after Copy
Cancel and archive open, successful action-driven close and rejection of a
forced enabled Exit trigger while the source open caller is still active.
These are macOS offscreen behavior checks, not native Cocoa menu acceptance.

Validation: macOS arm64 / Qt 6.12, incremental `Sevenzip`, `main_shell_tests`,
`language_ui_tests` and `app_lifecycle_window_tests` builds passed;
`language_ui` and `app_lifecycle_window` passed. `main_shell` had 17 passing
cases and two failures from existing expectations that exclude Open archive
and pin Rename's former menu index; both expectations and the extra menu row
are already present in HEAD. Native menu inspection was unavailable because
Computer Use access to Sevenzip was not approved. Windows/Linux and rendered
Windows parity are outside this check.
