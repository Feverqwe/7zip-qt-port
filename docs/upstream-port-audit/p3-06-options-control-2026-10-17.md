# P3-06 — Options page switching from all controls, 2026-10-17

Baseline: https://github.com/ip7z/7zip.git, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed original
`CPP/7zip/UI/FileManager/OptionsDialog.cpp`, all six page resources/callers,
`CPP/Windows/Control/PropertyPage.cpp/.h` and current Qt Options keyboard owner.
Original PropertySheet supplies window-wide Ctrl+Tab navigation and source
page activation. Page state/Apply/Cancel bodies remain unchanged.

The isolated actual original Windows reference, EXE SHA256
`0f00ac1e2b860632a97001440115d477ea8e3e5e7a19e08814cb6a753cd98eed`, confirms
six EN/RU observations in Limited interactive run
`20261017T124909Z-9854f631`. From Cancel footer2, forward System→Menu focuses
Button2302; reverse Menu→System focuses Button101; reverse System→Language
focuses ComboBox100. PID/foreground/key ownership guards remain active; private
reference configuration is removed on exit and fixture/source bytes are unchanged.
Only private Language settings are applied, never System associations.

Qt previously handled page cycling only when the key event reached QTabWidget.
The footer is outside that widget. On macOS physical Control is additionally
Qt MetaModifier, while QTabWidget6.12 handles logical ControlModifier only.
Qt documents its [macOS modifier mapping](https://doc.qt.io/qt-6.12/qt.html#KeyboardModifier-enum).
The pinned Qt6.12 source archive's QTabWidget::keyPressEvent and Cocoa key-view
mapping were inspected locally. A keyPress-only translation attempt does not
fix the issue because the child consumes Tab before that handler is reached.

The existing local QtOptionsPages adapter now installs window-context shortcuts,
including Cocoa physical Control, then delegates navigation to QTabWidget's
existing enabled/visible-page loop and retains source first-control activation.
It does not implement another navigation algorithm or globally swap modifiers.
The other platforms retain their logical Control key; existing synthetic logical
Control tests remain. Native Win32 PropertySheet has no portable Qt window-wide
shortcut implementation to reuse, so this remains a narrow presentation adapter.
No new dependency, borrowed module, archive/association logic or source revision.

## Regression and validation

Candidate: `1f1bb98` plus the existing Options adapter and regression correction.
Evidence: `build/review/goal-options-physical-control-20261017/`.

- Exact committed old product owner is hash-verified before integrated RED.
  Cocoa physical-Control-only initial RED6Qt pass/4fail; expanded page+footer
  RED2pass/8fail. Guests `20261017T125026Z-d9ec5862` each fail all four footer
  EN/RU normal/compact cases:19pass/4fail/0skip. These are actual behavioral
  failures, not assertions about private names or source text.
- Corrected source passes six focused CTest consumers on all three OS.
  macOS integration wrapper10/10 PASS. A subsequent Mac-only Backtab fixture
  strengthening passes final default keyboard1/1 and whole Cocoa keyboard.
- Native `20261017T125239Z-c7e5dae6` passes whole keyboard suite on Windows
  Limited and XCB, each23Qt/0fail/0skip and64JSON. Final Cocoa passes27Qt/0fail/
  0skip and112JSON, including both logical Command and physical-Control Qt-event
  routes, Shift+Tab/Backtab, all six pages/both directions, checked-radio focus,
  pending editor retention/select-all and footer switching. Cancel writes and
  notifications remain zero through the existing injected association fixture.
  No screenshots, Computer Use or OS input are used on macOS; this does not
  claim a physical hardware-key test. Linux uses private Xvfb/Openbox.
- Native guest compiled product/test hashes match the final root; source guards
  pass. Baseline retains412exact/198adapted/3local paths at one revision.
  C++ is clang-formatted; diff check passes. Temporary original/native drivers
  are archived and removed from source. Original copies/fixtures are cleaned
  after exit; current failure/reference evidence and incremental builds remain.

This closes the demonstrated Options key-routing/focus scenario. It does not
close physical Mac input, full native/rendered/cold acceptance or Windows codec
stability. Delivery40b3439 predates this correction and needs refresh after the
remaining UI scope settles. P3-03/P3-04/P3-06 remain open at44/47. No push,
publication or upstream modification occurs.
