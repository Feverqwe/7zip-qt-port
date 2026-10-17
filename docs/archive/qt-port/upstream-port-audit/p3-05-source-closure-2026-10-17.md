# P3-05 — source and integration closure, 2026-10-17

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

## Scope and source

Product `5d9cc8e`, source repository <https://github.com/ip7z/7zip.git>, full pin
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. This iteration changes documentation
only. The existing [109-unit body/dependency ledger](../omitted-scope-audit/ledger.md)
and its FileManager/Agent/GUI/Common receipt indexes own module dispositions;
the baseline owns exact/adapted file provenance. Those receipts distinguish
inactive/CE/entrypoint/PCH branches from required desktop functionality.

All302 original/local source, header and resource rows were rehashed against
the pinned Git blobs and current committed owners. All109 cpp units retain
8 exact/96 adapted/five absent dispositions; all104 present cpp owners remain
in CMake. Since the `e3b3c65` inventory, the sole borrowed-owner delta is the
[documented source shell-icon query](p3-05-address-root-2026-10-17.md).
The Qt Root history caller lies outside that borrowed inventory. No module,
source revision, language conversion, dependency or duplicate implementation
is introduced. Inventory evidence supplements the existing complete body maps.

## P3-05.15 — Copy and publication

Reinspected the original complete `App.cpp::OnCopy` body and current `OnCopy`,
`RefreshCopyPanel`, `CompleteCopy`, `QtAppPresentation::ExecuteCopy` and
`CompleteCopyIfReady`. Source ownership retains selected/focused indices,
destination reduction/classification, ADS/root/drives admission, directory and
history errors, saved selections, CopyTo/CopyFrom options and completion order.
The retained source refresh callback publishes the source for Move/copy-to-same
and the destination unconditionally for ordinary Copy/Move; reload HRESULTs are
ignored by this caller as upstream. Notification restoration, KillSelection,
title and source focus follow publication through the single-use continuation.

The earlier [caller/result-policy body map](../omitted-scope-audit/filemanager.md#app-copy-publication-caller-reconciliation-2026-10-10),
[proxy lifetime correction](p3-05-proxy-lifetime-2026-10-17.md) and
[native Copy receipt](p3-05-copy-native-2026-10-17.md) retain original RED cases,
success/failure/Cancel, filesystem/archive/ADS/device/zone, both-pane and
writer/Parent/Drop consumers. The source Copy/publication owners are unchanged
between `17fab0f` and `5d9cc8e`; later source changes are Options/System keyboard,
shell-icon presentation and Root history. Consequently the accepted native
archive/metadata and original eleven long-suite evidence remain applicable to
their named source behavior. They are not relabeled as a new full-product gate.

Current focused checks below additionally exercise `app_copy_routing`,
`panel_copy_reload`, `copy_publication_order`, `native_parent_publication` and
`native_drop_publication`. P3-05.15 is accepted for the complete source Copy and
publication contract. The declined twelfth slow `copy_archive_refresh` is not
run, and its current expanded full matrix remains unverified under P3-03.
No new waiver or large-matrix acceptance is inferred.

## P3-05.16 — startup and window placement

Reinspected original `FM.cpp::InitInstance`, `SaveWindowInfo`, `CSplitterPos`,
their active create/resize/close callers, and the current source bodies with
`MainWindow` startup/close and splitter adapters. The earlier
[placement/splitter receipt](p3-05-2026-10-10.md#p3-0516--source-initinstance-window-placement-2026-10-10)
retains actual geometry/ratio RED cases, settings admission, show commands,
normal/maximized restart, F9, resize, handle movement and the documented Qt
logical-coordinate/content-minimum adaptations. Those source owners are unchanged.

The old Session0 visible-window failures are not current desktop acceptance.
Current Windows Session1 `file_manager_startup` passes all20Qt cases, including
the actual `7zFM.exe` forced-type success/refusal windows and both native symlink
Copy cases. Only this suite uses the scoped elevated token required to create
its disposable symlinks; it does not change account/security policy. Every
platform passes all30 window-placement cases, including current normal/compact
geometry and source splitter state. P3-05.16 is accepted for its source
startup/placement contract; pixel parity and complete physical input remain
separate P3-06 claims.

## Current focused checks

| Platform / snapshot | Result |
| --- | --- |
| macOS arm64 Qt6.12 |10/10 PASS16.02s,162Qt/0fail/0skip |
| Linux `20261017T104614Z-4f151267` |default10/10 PASS; XCB9/9 PASS;162/151Qt,0fail/0skip |
| Windows `20261017T104437Z-75fe25a8`, Limited Session1 |9/9 PASS74.33s,146Qt/0fail/0skip |
| Windows `20261017T104642Z-5380eff5`, scoped startup suite |1/1 PASS7.97s,20Qt/0fail/0skip |

The ten-scope selection adds `window_placement`, `main_shell`, `memory_lock`,
`startup_path` and `file_manager_startup` to the five Copy consumers above.
`memory_lock` is QtCore-only and needs no native QPA replay. macOS current Cocoa
placement and shell consumers also passed in the preceding
[address/Root iteration](p3-05-address-root-2026-10-17.md).
Linux's initial selector stops after ten passing default suites because it
incorrectly requires a QPA property from `memory_lock`; the corrected one-off
driver runs the nine GUI suites under private Xvfb/Openbox and restores CTest
configuration. No product or test assertion changes are needed.

Snapshots are checked by the runner SHA manifest before execution. The final
startup snapshot additionally contains an unrelated uncommitted test-report
metadata change in `tests/layout_inspection.cpp`; it changes no product input
or startup assertion and is preserved outside this documentation commit.
The other current consumer snapshots use the committed reporting helper.
No full gate, slow suite, new screenshot or physical OS-input check is claimed.
Evidence: `build/review/final-availability-8e9343b-20261014/source-closure-20261017/`,
including `source-inventory-5d9cc8e.json` and exact default/native logs.

## Package boundary

P3-05 closes the required source/module/dependency/integration reconciliation;
all its sixteen substeps now have accepted source contracts. Native/rendered
whole-workflow acceptance, Windows cold-codec stability and refreshed final
delivery remain owned by P3-03/P3-04/P3-06. The plan therefore advances to44/47,
without claiming final acceptance or a complete native walkthrough.
