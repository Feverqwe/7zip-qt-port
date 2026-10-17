# P3-05.12/.13 — address and Root namespace consumers, 2026-10-17

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

## Source and boundary

Source repository: <https://github.com/ip7z/7zip.git>, full commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
The earlier [source migration receipt](p3-05-2026-10-06.md)
retains the copied/adapted address construction, indentation, stored binding
paths, selection and source Root command bodies. Reinspection covers
`PanelFolderChange.cpp::LoadFullPathAndShow`, `OpenRootFolder`,
`OpenDrivesFolder`, their `CApp::OpenRootFolder` and Qt consumers, and
`SysIconUtils.cpp::Shell_GetFileInfo_SysIconIndex_for_Path_attrib_iconIndexRef`
with its HRESULT wrapper and system image-list dependencies.

The original shell query uses `SHGFI_USEFILEATTRIBUTES`. The Windows branch of
the Qt path-icon adapter now calls that existing copied query and existing
`GetSystemIcon` bridge. Previously QFileIconProvider selected the existing
filesystem object's icon regardless of the requested attributes. No source
algorithm or cache policy is replaced; non-Windows QFileIconProvider presentation
is unchanged. No dependency or source-language conversion is introduced.

The source address algorithm removes a trailing backslash conditionally. An ADS
colon remains in the query and uses the original directory-attribute policy.
An earlier proposed unconditional removal was incorrect and was discarded;
it is absent from this change. Its earlier passing candidate runs are not
acceptance evidence for the current product.

The Qt-only Back history also lost namespace prefixes when Root was invoked
from Network, ADS or the device namespace. The integration now retains the
source folder's actual prefix when Root changes its destination. Repeating
Root at the same destination does not insert another history entry. Original
folder binding, drive/root transition and refresh bodies remain authoritative.
Upstream has no Qt application's Back-stack owner to reuse; this narrow caller
adaptation extends the existing filesystem/archive history service.

## Regression evidence

The public `root_navigation` workflow records Network → Root → repeated Root →
Parent → Back. Before the caller fix, macOS v228 reports7Qt pass/1fail: Back
publishes an empty address instead of `Network/`. The corrected workflow passes.

`address_combo` queries the public icon map for a disposable Unicode/spaced
file with directory and archive attributes and compares against an independent
native HICON query. It also checks cache reuse and unchanged bytes. Windows
`20261017T103310Z-0959457f` restores the previous adapter temporarily in the
runner worktree: the new attributes regression fails; the current adapter is
restored, rebuilt and the suite passes. RED is16Qt pass/2fail, including the
actual ADS-colon icon; GREEN is18Qt pass/0fail/0skip. No modified upstream
checkout is used.

The Windows-only `native_address_navigation` has12 rows:
device/ADS/UNC × both panes × EN/RU. It checks actual provider/stream contents,
the closed editor icon, hierarchy groups and indentation, public popup selection,
Escape, Root/Back, and refusal after removing an owned share. Peer draft,
Unicode/spaced control bytes and absence of archive-backend calls are retained.
The share is UUID-named, restricted to the current SID and explicitly removed;
device coverage enumerates the existing namespace without raw-device IO.
Its native QPA and common desktop resource lock are required for popup/focus
checks. The initial offscreen fixture timeout and subsequent native timeout
are retained as failed runs. The latter exposes an invalid fixture precondition:
it opens the popup programmatically while the list owns focus. The fixture now
focuses the address first, as a user opening that control would; the assertion
that selection returns focus to the list and the90s cap remain unchanged.

## Current checks and remaining acceptance

Evidence root: `build/review/final-availability-8e9343b-20261014`, with current
results under `navigation-acceptance-20261017` and persistent runner records
under `build/runners/hyperv`.

| Platform / scope | Current result |
| --- | --- |
| macOS final eight navigation consumers | default8/8 PASS6.43s,165Qt/0fail/10skip; Cocoa8/8 PASS26.58s,175Qt/0fail/0skip |
| Linux `20261017T103418Z-d5c5f3f3`, eight navigation consumers | default8/8 PASS; XCB8/8 PASS;165/175Qt respectively,0fail,10/0skip |
| Windows `20261017T103054Z-f552357c`, eight navigation consumers |8/8 PASS94.84s with configured platform policies, ordinary Session1 |
| Windows `20261017T103846Z-fba7ae46`, namespace fixture only |two native1/1 PASS10.91/10.21s, each14Qt/0fail/0skip;48JSON/48cropped PNG |
| Windows `20261017T103953Z-219e70b9`, address and direct icon consumers |3/3 PASS,74Qt/0fail/0skip, ordinary Session1 |
| Linux `20261017T103642Z-4a4b6079`, direct icon consumers |default/XCB2/2 PASS each,54Qt/0fail/2skip each |
| macOS direct icon consumers |2/2 PASS4.59s |

The eight final macOS consumers are `native_navigation_window`,
`root_navigation`, `window_placement`, `address_combo`, `panel_address_keys`,
`root_command`, `persisted_location` and `panel_key_source`. The Linux/Windows
eight-consumer selection substitutes `address_navigation` and `ui_navigation`
for the native-window/placement pair already covered by their earlier current
receipts. Offscreen keyboard-layout rows skip on macOS/Linux and all execute
under Cocoa/XCB. Two icon-provider rows are Windows-only and retain their skips
on Linux. No skip is converted into a pass.

The first Linux build is rejected by the kernel OOM killer before tests; a
single-process build passes. A first XCB attempt without DISPLAY also fails
before workflows. The successful native run uses private Xvfb/Openbox. Both
failures remain recorded separately. Windows namespace runs use only the
scoped elevated token required by the owned share, never an elevated full gate.

The final macOS run retains an unchanged1437-file compiled-input hash guard.
The final Windows source manifest matches all eight changed build/source/test
inputs exactly. The normal runner verifies its complete supplied SHA manifest
before execution. `upstream_baseline.py verify` passes412exact/198adapted/3local
paths across the pinned revision. Formatting and whitespace checks pass.

All48 current Windows address JSON roots are visible, enabled and fully exposed.
Their cropped images were inspected alongside the six original EN/RU v224
captures: device and UNC-server use the computer icon, ADS uses a folder icon.
The UNC-share itself uses the source shell folder/share icon. Pixel equivalence
and full original popup text are not inferred from these closed-editor images.
Normal/compact owning lists and address icons are also asserted by the fixture.

P3-05.12 and P3-05.13 are accepted for their source address/Root contracts,
including namespace selection/refusal and retained Back prefixes. The earlier
receipts retain the filesystem/archive/nested and slash/backslash command
consumers. No parent package count changes through accepting these substeps.
P3-03/P3-06 cold-codec, remaining native/rendered workflows and final delivery
remain mandatory. No full gate or slow suite was selected for these focused
navigation/icon changes. Existing original Windows address captures v224 cover
six EN/RU ADS/device/UNC-server closed-editor states. They establish the
rendered reference for that field, not complete popup labels or pixel parity.
