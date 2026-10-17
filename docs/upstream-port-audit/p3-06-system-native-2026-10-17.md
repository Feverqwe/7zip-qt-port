# P3-06 — native Linux System associations and icons, 2026-10-17

## Source and ownership

Baseline: https://github.com/ip7z/7zip.git, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed original/adapted
`CPP/7zip/UI/FileManager/SystemPage.cpp/.h/.rc`, `SystemPageRes.h`,
`RegistryAssociations.cpp`, `FilePlugins.cpp/.h`, `OptionsDialog.cpp` and
`CPP/7zip/UI/Agent/ArchiveFolderOpen.cpp::GetIconPath` with their callers.
`CModifiedExtInfo::SetState/GetString`, `CAssoc::GetIconIndex`, mixed-selection
cycling and the source Apply loop remain authoritative and unchanged.
The Windows `AddIcon` environment expansion/indexed ExtractIconEx body remains
the loader when no native image is supplied. macOS retains its existing bundle/
file-icon fallback. No borrowed file, source revision, language or dependency is
added; existing notices and mirrored owners remain. Baseline verification passes
412exact/198adapted/3local paths at one revision.

The original page displays the selected handler's registered icon, independently
of its executable. Linux previously passed GIO's executable string through the
file-icon provider, losing the desktop entry's `GIcon`. Separate applications
sharing one executable consequently displayed the same generic icon; a quoted
Unicode/spaced executable is also not an icon filename.

The existing Linux platform adapter now obtains file/themed icon metadata from
`g_app_info_get_icon`, using GIO's file/theme names and Qt's native icon services.
The local association snapshot carries the image separately from mutation
identity/revision. The service retains each observed image by group/extension,
including shared-MIME refresh after Apply. Its optional own-application image
comes from the matching desktop entry. The source page accepts these images at
its existing image-list boundary and retains the original image-index selection.
This native GIO/Qt boundary has no portable upstream implementation: original
7zFM supplies Win32 HICON extraction, which is still reused on Windows. No
association algorithm, registry policy, MIME writer or archive logic is recreated.
GUI-less callers retain detection/mutation without constructing graphical icons.

## Deterministic native scenario

`linux_system_page_tests` uses actual GIO and the source System/Options owners,
with an injected two-extension `IFolderManager` fixture. It does not claim to
exercise the real catalog's full extension list. Each EN/RU × normal/compact
case has a separate process and private XDG configuration/data directories,
desktop entries and icon theme. Two external handlers share the same quoted,
Unicode/spaced executable but have different icons: an absolute Unicode/spaced
PNG and a themed icon. The application's own desktop entry has a third icon.
No handler is launched and no real desktop preference is mutated.

The UI must show both native handler IDs and their distinct decoded icons,
current native username and the disabled all-users action with its reason.
Selected-row Other→7-Zip→Clear→Other changes only pending state; preferences
and native revisions stay intact. Apply changes only the selected MIME default
and displays the own icon; the other handler/image remains unchanged. A pending
Clear followed by Cancel preserves the applied choice, including after reopening.

An external `gio mime` process then changes the private ZIP default. The fixture
waits for GIO's file-monitor delivery before pressing the actual Options Apply
button. Source Apply emits ERROR_RETRY, shows Critical/OK, continues later pages
as PSNRET_NOERROR requires and leaves Options open. The row displays the external
handler and its correct icon; external preference bytes remain identical.
The existing Qt widget responder acknowledges the message after its nested loop
is ready. This is Qt widget acceptance, not an OS default-app consent panel.

The linear test keeps these dependent before/after snapshots in one scenario;
fixture construction and report/row helpers are separate. Existing GUI-less
native Clear fixtures now also carry themed-icon metadata, covering the absence
of a QGuiApplication without changing their association contracts.

## Evidence and validation

Candidate: `3eba22e` plus this source/interface/test/workflow correction.
Evidence root: `build/review/goal-system-native-20261017/`.

- Actual RED `20261017T112921Z-1ee1d6b7`: all four cases fail at the first native
  icon assertion, showing generic black instead of the registered red image;
  total8Qt pass/4fail/0skip. Sources are hash-verified against the candidate
  before the product fix. Earlier `112718` fails to compile the new fixture's
  missing conversion includes and runs no tests.
- Final Linux default `20261017T114629Z-6f2331d2`:13/13 CTest PASS,
  96Qt/0fail/1skip. The skip is the Windows-only registry-revision contract.
  This covers native icon/state/error cases, GUI-less Clear, association services,
  the resource contract and all five direct Options/widget consumers.
- Native Linux existing five Options suites pass69Qt/0fail/0skip with86JSON in
  `20261017T113747Z-e9dcf985`. The final four GIO cases, expanded to the complete
  parent Apply/error presenter, pass12Qt/0fail/0skip and28JSON in
  `20261017T114907Z-75ea237c`. Only the changed fixture is replayed; product code
  and the five existing native consumers are unchanged. Together these provide
  81Qt and114 native JSON reports. XCB runs on Xvfb/Openbox, not a physical console.
- macOS default7/7 PASS,76Qt/0fail/1 Windows-only skip; Windows Limited default
  `20261017T113203Z-e84c7ea9`7/7 PASS,77Qt/0fail/0skip. The same five native
  widget suites pass69Qt/0fail/0skip and86JSON on each OS. Their compiled source
  owners match the final candidate; later changes are Linux-only fixture/helper
  registration. Native source guards pass on all three platforms; no PNG is made.
- Updated workflow run `20261017T114134Z-cf152caf` passes the actual Options
  wrapper13/13,117Qt/0fail/0skip, and associations wrapper9/9,43Qt/0fail/1skip.
  The Options profile plan selects16 applicable Linux tests and builds their
  required targets. The later fixture expansion retains those entry points and
  receives the final focused/default/native follow-up above.

Current native JSON inspection checks five required groups in24 parent states:
visibility, label fit and expected current/all-user availability. All four actual
Critical messages have visible fitted text/OK. The later fixture expansion adds
full error-presenter evidence; an earlier immediate query raced GIO's pending
file-monitor notification and displayed its old cached handler in three cases.
The corrected readiness wait preserves the existing5s waits and CTest caps;
production code does not pump or replace the event loop. All fixtures clean up
their own disposable XDG data. Incremental builds/caches and current failure,
reference and review evidence are retained. One-shot drivers are archived and
removed from tracked source directories after acceptance.

This closes the native Linux handler-image transport and its complete
Apply/Cancel/refusal scenario, with portable fallback regressions on three OS.
It does not accept Windows/macOS live association values/icons, real installed
default-app changes, pixel parity, the full native application walkthrough or
Windows cold-codec stability. Delivery09a6f6d precedes this correction and needs
a final refresh. P3-03/P3-04/P3-06 remain open at44/47; no push, publication or
upstream modification occurs.
