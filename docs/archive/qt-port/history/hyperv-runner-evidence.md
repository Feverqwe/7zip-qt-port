# Hyper-V historical evidence

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Historical setup/capacity/check receipts. Follow [the current runner procedure](../../../hyperv-runners.md), verify live readiness and space; these snapshots do not authorize storage changes.

Capacity preflight, 2026-10-07 MSK: both authenticated x64 guest/toolchain probes
pass with Qt 6.12.0 and clang-format 23.1.2. Refreshed at02:27MSK:
host D: has 648,527,106,048 bytes free; guest Linux root has 2,981,888 bytes and
Windows C: has 57,765,888 bytes free. Existing-volume inspection found no separate
guest build volume (Linux has its root/boot/EFI partitions; Windows has C: and EFI).
Fresh snapshot/runtime/all-target acceptance builds do not fit these guest
volumes. No cleanup, VHDX compaction/growth, disk change or new source sync was
performed. The runners skill forbids automatic deletion/compaction in response
to capacity warnings. The required storage helper records its monitoring sample
under host D:. Current evidence: build/review/drag-refresh-final/{hyperv-storage.log,
hyperv-probe.log,linux-guest-space.log,windows-guest-space.log,linux-volumes.log,
windows-volumes.log}. Earlier samples remain historical. This preflight
does not provide current application, package or rendered Windows acceptance.

Refreshed capacity preflight,2026-10-07 06:29MSK (03:29UTC): both authenticated
x64 probes still pass with Qt6.12.0/clang-format23.1.2. Host D: has
634,121,768,960 bytes free; Linux root has4,001,792 bytes free and Windows C:
54,661,120 bytes. Read-only volume inspection again found no separate guest
build volume. Existing Windows/Linux VHDX file sizes stayed100,231,282,688 /
83,273,711,616 bytes (zero delta); virtual sizes are103,079,215,104 /
85,899,345,920 bytes. Fresh source/runtime/build snapshots still do not fit.
No source sync, cleanup, compaction, disk growth or VM reconfiguration was done.
The runners skill says: “Do not automatically compact disks or delete files in
response to warnings.” Evidence: build/review/native-external-final/
{hyperv-probe.log,hyperv-storage.log,linux-guest-space.log,
windows-guest-space.log,linux-volumes.log}. This preflight proves connectivity
and capacity only; native application/package/rendered acceptance remains open.

Fresh authenticated preflight at2026-10-07 07:48MSK (04:48UTC) supersedes
the capacity limitation above: Linux root has44,568,260,608 bytes free and
Windows C: has28,375,638,016 bytes free. Host D: has631,519,203,328 bytes free;
the same VM IDs and single OS disk per guest remain. At this preflight no cleanup,
compaction, new disk, resize or VM reconfiguration had been performed.
Evidence: build/review/runner-workspace-final/{host-before.log,
linux-before.log,windows-before.log}. The earlier low-space samples remain
historical; they must not be used to infer current capacity.

### Linux project data volume

The first fresh all-target run subsequently occupied20,627,836,928 bytes,
leaving23,580,934,144 bytes on Linux root. On2026-10-07 a separate dynamic128GiB
VHDX was created at `D:\runner\linux\sevenzip-work-20261007\data.vhdx` and
hot-attached to the existing Linux VM's SCSI0:2. SCSI0:1 retains its DVD;
the first attachment attempt was rejected because that slot was occupied.
The created VHDX was reinspected and reused rather than recreated.
Its identifier is `bbf5c8b3-0f0a-443d-b6ed-212c5c05c249`.
The guest's full GUID-derived by-id, serial, size, LUN and before/after device
inventory identified the new unpartitioned, unmounted disk with no signatures.
Only that new disk was initialized as ext4. Existing OS disks, partitions,
files, VM identity and networking were preserved; no cleanup or compaction ran.

The private mount is `/mnt/sevenzip-desktop`, filesystem UUID
`9fd6574a-fe35-4af5-82fc-f1b16d78afb8`. Its initial available space was
134,128,553,984 bytes. One UUID/nofail entry was appended to guest `/etc/fstab`,
preserving the previous bytes with a private backup on the new volume.
Ownership records are under that volume's `setup/` and the host data directory;
local evidence is build/review/runner-workspace-final/{host-attach-data.log,
linux-after-attach.log,linux-initialize-data.log}. No Windows data disk was added.

## Validation on 2026-10-03

Checked the current worktree based on `4721f9da34abcaa7edea6940cf583711c1ba1457`
with runner tooling/docs additions; application sources were unchanged.

| Platform | Result |
| --- | --- |
| macOS arm64, Qt 6.11.2 | Full local gate: formatting/build and 207/207 CTest, 79.01 s |
| Ubuntu 24.04.5 x64, GCC 13.3 / Qt 6.4.2 | Identity/formatter, source hash, official runtime/SFX and formatting passed; app/test build failed before CTest |
| Windows 11 x64, MSVC 14.44 / Qt 6.8.3 | Identity/formatter, source hash, official runtime/SFX and formatting passed; app build failed before CTest |

Linux snapshot: `/home/runner/sevenzip-desktop/runs/20261003T083145Z-9873cd5b`.
The existing `tests/ui_update_modes_tests.cpp:189` uses `QTimeZone::UTC`, absent
in guest Qt 6.4.2. The full gate requires a newer matching Qt kit or a separately
reviewed compatibility change.

Windows snapshot: `C:\runner\sevenzip-desktop\runs\20261003T083237Z-a48e8d7b`.
The existing `src/qtexternalprocess.cpp:38` fails MSVC compilation because
`QueryFullProcessImageNameW` is undeclared. Windows API declarations/build
configuration need separate investigation. Full logs remain as `check.log` in
these snapshot directories; local run records correctly report failure.

Final readiness probes and five offline runner tests passed after localized
transport/logging changes; Python syntax and `git diff --check` passed. Host
storage monitoring reported no warnings after builds (about 770.8 GiB free on D:).
Neither guest Debug gate is accepted; Release was not attempted, and desktop,
distribution and visual parity remain unverified. No application fixes were made
as part of runner setup.

Review of commit `1adbeac31ad8f17e370a64d6ed3e1d102f1be6b7` after external
network access became available: both identity/toolchain probes and all five
offline helper tests passed. Installed Qt versions remain Linux 6.4.2 and
Windows 6.8.3. Network access alone does not resolve either recorded compilation
blocker. No packages were installed and the application gates were not rerun
during this documentation review.

## General entry point validation on 2026-10-03

Twelve offline transport/orchestration tests passed on macOS, as did
`git diff --check`. Live Linux and Windows checks passed for Python script
execution and Bash script execution, using one SHA256-verified rsync snapshot.
A subsequent snapshot transferred 46,342 client bytes per guest for an
approximately 18 MB archive (29,280 unmatched bytes; 17,952,160 matched bytes).
An intentionally invalid `check_ui_parity.py` invocation returned guest exit 2
on both platforms, downloaded both full logs, and returned aggregate exit 1.
These are orchestration checks, not guest application-gate acceptance.

The local unsandboxed full gate passed formatting/build and 222/226 CTest
cases in 95.95 s. Failures were `ui_operations` (segfault), `panel_auto_refresh`,
`archive_external_lifetime` and `archive_external_security`. A focused sequential
rerun through `scripts/ctest.sh` passed all four in 16.58 s. The initial full
gate remains failed; this tooling change does not modify application sources
or those test suites, and their parallel-run instability remains unresolved.

## CLI refresh validation on 2026-10-04

Sixteen offline runner tests passed on macOS, including action-specific options,
parallel probe failure isolation, preserved discovery diagnostics and Release
fail-fast behavior through both generated guest payloads. Live `probe all`
passed on Linux and Windows, using Sevenzip's formatter 23.1.2. The unsandboxed
local `scripts/check.sh` passed formatting, build and all 229 CTest cases in
79.24 s; `git diff --check` passed. Guest application gates were not rerun.
