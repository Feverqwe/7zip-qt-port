# P3-06 — source application icon at native entry boundaries, 2026-10-17

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Baseline: https://github.com/ip7z/7zip.git, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed original
`CPP/7zip/UI/FileManager/FM.cpp::InitInstance`, `resource.rc`, `resourceGui.h`,
`FM.ico`, GUI resource/entry callers and corresponding Qt startup, presentation,
resource-link and bundle/deployment owners. Original InitInstance assigns
LoadIcon(IDI_ICON) before window-class registration. File Manager and GUI use
identical FM.ico bytes, SHA-256
`649f8e81cce9d8e3ab9a0d06db31788f5ced3c6ed99a78304a87dba35ecf06cc`.
The unchanged asset is already mirrored and linked in this repository.

Qt previously left application/window icons empty. Windows omitted the product
EXE icon resource, while macOS lacked a bundle icon. Startup now supplies the
existing source Qt resource as application/window icon. Windows compiles the
original IDI_ICON entry with build-local absolute paths; copied7zFM/7zG retain
the resource. macOS build-time sips converts the authoritative ICO to its required
ICNS container, included in native bundle metadata/resources. No converter ships
or runs at runtime. Linux uses the Qt application/window image; no desktop entry
or theme icon is installed by this correction.

No functional source algorithm, routing, geometry, settings, association writer,
borrowed revision or image content changes. Original 7zFM has no portable Qt
application-icon/macOS bundle implementation: these narrow native presentation/
build adaptations replace Win32 class/resource services. No new third-party
dependency or borrowed file is introduced; original notices/licenses remain.

## Evidence

Candidate: `2419a0e` plus this correction. Evidence root:
`build/review/goal-application-icon-20261017/`.

- Public `main_shell` regression RED:2Qt pass/1fail because windowIcon is null.
  GREEN checks the shown QWindow's decoded image at all source-supported sizes.
  Final whole Mac shell passes; Cocoa/XCB/Windows single-case native runs each
  pass3Qt/0fail/0skip. XCB uses private Xvfb/Openbox, not a physical console.
- macOS focused7/7 PASS covers app smoke, startup, placement, shell and
  association/bundle consumers. Later shown-window fixture strengthening passes
  whole main_shell and Cocoa separately. Native AppKit obtains the built app's
  NSWorkspace icon, rasterizes offscreen at48px and compares with the original
  ICO decoded by AppKit: equal pixels, distinct from generic app. No screenshots,
  Computer Use or OS input. An initial one-shot oracle compile fails on an
  unavailable C++ header; stdio.h corrects only that harness.
- Guests `20261017T122659Z-da962180`: Linux5/5 PASS; Windows Limited4/5 PASS,
  only two startup rows fail creating symlink fixtures. Actual failure retained.
  Previously authorized scoped Highest `20261017T122944Z-09089094` builds the
  final Windows resource and passes complete startup1/1. This is separate
  privileged prerequisite coverage, not a passing Limited whole gate.
- Native guests `20261017T122837Z-4388787f` verify shown-window icons with
  unchanged compiled product inputs. Later Windows resource-only observation
  `20261017T123103Z-c30c44e9` extracts sevenzip_desktop.exe/7zFM.exe/7zG.exe
  icons with ExtractIconExW/GetDIBits:16px color pixels and transparency masks
  exactly match the original ICO. These independent native artifact checks
  avoid new file-presence/configuration tests.
- Baseline remains412exact/198adapted/3local paths at one revision. Task C++
  is clang-formatted and diff-checked. One-shot producers are archived and removed
  from source; disposable generic fixtures are removed. Incremental builds and
  current RED/failure evidence remain.

This accepts source application/window identity and current build artifacts'
native icon metadata. It does not establish full screenshot parity, installed
Linux launcher/theme integration, default-handler consent/mutation or Windows
cold stability. Packages09a6f6d predate this and both System image corrections;
final refresh remains. P3-03/P3-04/P3-06 stay open at44/47. No push, publication
or upstream modification occurs.
