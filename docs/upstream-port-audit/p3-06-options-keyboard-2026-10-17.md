# P3-06 — Options source keyboard activation (2026-10-17)

Baseline: https://github.com/ip7z/7zip.git commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed original
`CPP/7zip/UI/FileManager/OptionsDialog.cpp`, `SystemPage.cpp/.h/.rc`, the five
other page resources and callers, `CPP/Windows/Control/PropertyPage.cpp/.h`
(`MyPropertySheet`, `CPropertyPage::OnNotify`/`OnSetActive`) and `CDialog::OnInit`.
The property sheet starts at System and delegates keyboard activation to native
dialog management. Existing source state, Apply, registry and association bodies
remain authoritative; their dependency and omitted-scope dispositions do not
change. Notices and baseline remain intact; upstream is read-only.

The Limited Windows original-reference probe checks its non-elevated token
before startup, guards foreground/input/focus by owned PID, uses an isolated
configuration key and exits through Cancel. Original focus observations are:
System Button101, Menu Button2302, Folders Button2402, Editor Edit100,
Settings Button2501 and Language ComboBox100. Reverse switching from Settings
returns to Viewer100, selects all21 pending characters and retains Editor102's
pending text. Selecting Folders Current2403 makes that checked radio the focus
target on return. Reference binaries/source inputs stay unchanged and the private
key is absent after exit. This establishes actual keyboard behavior, not pixels.

The Qt presentation now restores the first enabled tab stop at initial opening
and after Qt-owned keyboard page selection. A local QTabWidget adapter applies
the native dialog focus rule, including checked exclusive-radio members and
QLineEdit selection. Qt still owns page selection and platform key bindings.
System's two source buttons and existing sheet button tab stops use StrongFocus,
as the existing Add presentation does under QMacStyle. Intentional NoFocus and
the OS keyboard preference are preserved. Hide/show does not reinitialize focus.
No archive operation, association write or new platform bridge is introduced.
Only the existing adapted OptionsDialog.cpp/SystemPage.cpp owners change.

Focused validation covers `options_dialog`, `options_integration`,
`options_keyboard`, `options_window` and `system_page`. macOS default and Cocoa
each pass5/5,69Qt/0fail/0skip; Linux default and XCB each pass5/5 with the same
totals. Windows Limited native passes5/5,69Qt/0fail/0skip. Each native run emits
86 widget JSON reports and no PNG. All six candidate source/build/test inputs
match between OS; source/configuration guards pass. Geometry uses the existing
normal/compact contract and native minima. The extracted shared fake association
fixture never writes real registration. The integration wrapper includes the new
focused keyboard suite.

The updated macOS integration wrapper additionally passes all nine immediate
Options/page/language/editor/Help suites. Baseline verification passes412 exact,
198 adapted and three local paths across one revision. All302 source inventory
rows are rehashed; only the two existing Options presentation owners differ
from product17fab0f. No module/path ownership changes.

Actual RED evidence precedes the fix: the old initial-focus product fails four
offscreen rows; a native Cocoa baseline fails12 initial-focus rows and the old
page-switch expectation. The new page-activation regression with the adapter's
post-selection focus disabled fails five rows (selected radio and Settings
focus). The original-reference probe's first cross-process GetWindowText attempt
fails after focus observations; corrected WM_GETTEXT probes pass and establish
pending-text retention. That harness failure is not lost product input.

Ignored evidence root `build/review/final-availability-8e9343b-20261014/`:
`windows-final-original-options-keyboard-v175-evidence` (run
`20261017T054928Z-8e9d60a9`), `options-focus-red-v154`,
`mac-options-focus-baseline-v159`, `options-activation-red-v179`,
`mac-options-activation-v182`, `linux-final-options-activation-v183-evidence`
(run `20261017T061347Z-d21b20ac`) and
`windows-final-options-activation-v184-evidence`
(run `20261017T061347Z-22d859d6`). This localized component uses focused checks;
prior default gates and explicitly requested11 long suites retain their recorded
revision/scope. Live System association values/icons, full rendered UX acceptance
and the separate Windows cold codec timeout remain open. The subsequent
[three-platform e3b3c65 delivery](p3-04-2026-10-16.md#delivery-after-options-keyboard-activation-e3b3c65)
refreshes this committed component; older17fab0f artifacts retain their scope.
