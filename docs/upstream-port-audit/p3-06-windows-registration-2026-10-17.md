# P3-06 — native Windows registration scopes and nonmutating detection

2026-10-17, parentf7e9668 plus this correction. This completes the bounded
native HKLM registration scenario and fixes a detected preference-read side
effect. It does not establish Windows Settings/UserChoice selection or close
the parent acceptance packages.

## Source review and correction

Baseline https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Recompared original and adapted
SystemPage ChangeState/OnApply, RegistryAssociations ReadFromRegistry/Add/Delete,
Registry CKey Open/Create and their headers, native adapter and Qt services.
Original registry reads use KEY_READ; original two-hive mutation/state/error
flow remains authoritative. No borrowed body, module, source language or
baseline changes. Upstream has no Qt-specific optimistic revision snapshot;
the existing narrow Qt platform adapter owns that integration.

The revision snapshot used a QSettings registry root at the desired UserChoice
key. Qt6.12 opens its native root through createOrOpenKey even for allKeys/get,
creating an absent empty key. Its source was inspected from the existing
SHA256-pinned Qt archive. The adapter now anchors QSettings at the existing
hive and traverses Classes/UserChoice with read-only subgroup access. Snapshot
contents and external-change refusal remain; detection does not create missing
preference keys. No replacement registry algorithm or direct UserChoice write
is introduced.

## Native behavior

The existing windows_system_registration fixture now exercises both System
columns using fresh GUID extensions whose exact keys are absent in both hives
before each row. Opening Options and Cancel must leave all exact keys absent,
including FileExts/UserChoice. Actual Apply registers an HKCU or HKLM handler;
ShellExecute resolves it and the owned child receives unchanged Unicode/spaced
filename and bytes. Clear removes the registration; an external association
change is retained with ERROR_RETRY. A Limited HKLM attempt returns access
denied and leaves both scopes absent. Exact owned keys are removed and verified
in both hives. ShellExecute's legitimate per-user FileExts usage metadata is
distinguished from the product's unwanted detection mutation and cleaned up.

Machine-wide rows require explicit opt-in:
`SEVENZIP_TEST_MACHINE_REGISTRATION=allow` on the scoped administrator fixture,
or `deny` on the Limited fixture. The row checks the actual administrator token.
An ordinary full run keeps the current-user row only, including when elevated.
Build windows_system_registration_tests with the project wrapper before the
focused CTest selection. These two additional rows are small, not a slow matrix.

Actual initial RED in the Limited run: the HKLM attempt correctly refuses but
its peer contains an unexpected key. Inspection finds two owned extensions
with empty UserChoice subkeys. The source-visible Qt root creation explains
the mutation; after the adapter correction, Options/Cancel and refusal leave
those keys absent. The two historical empty keys are removed only after
verifying their exact recorded names, zero values and sole empty UserChoice
child. No normal archive extension or pre-existing user choice is altered.

One diagnostic rerun fails before testing because its evidence directory
already exists; subsequent evidence uses the runner token, without new build
trees. The first administrator fixture incorrectly rejects native ShellExecute
usage metadata in HKCU; distinguishing that metadata fixes the assertion and
retains absence checks before any recipient launch. These harness failures
remain separate from the product RED and final checks.

## Checks and limits

Final administrator run `20261017T155413Z-f018083d` passes native registration
1/1,4Qt/0fail/0skip,1.25s CTest. Final Limited run
`20261017T155442Z-c7b92fa6` passes registration plus association_services,
system_page and options_integration:4/4,38Qt/0fail/0skip,5.52s wrapper test phase.
Both final source inputs match their root SHA256 guards after collection.
Final scoped/native results and exact source guards are retained in
`build/review/goal-machine-registration-20261017/`; original runner/check logs
remain in build/runners/hyperv and the root machine-registration logs.
Windows-only adapter/target changes do not require macOS/Linux appearance runs;
their platform implementations are unchanged. Full/slow suites are not repeated
for this bounded registry boundary. Existing incremental trees/cache are reused.

The previous private packagesd0a74dc predate this correction. The manual macOS retry
still reports an extra window; its posted-event test is insufficient for native
startup acceptance. A native explicit-app LaunchServices diagnostic records the
selected d0a74dc bundle URL and one owned onscreen window, but its metadata
does not prove that it is the archive window or replace the negative Finder
result. ZIP remains Archive Utility. UserChoice/OS Settings selection, macOS
startup, remaining native workflows and Windows cold cause/stability remain
open. Count44/47; no push, publication, upstream or VM/disk changes.

## Private delivery after the correction

Product `aae78760798bd6741542727e6479d588d9698858` is now packaged on all
three OS using the existing Release/BUILD_TESTING=OFF trees and compiler caches.
All1056 selected product/build inputs match committed blobs before and after
packaging. Only the Windows platform source differs from the preceding d0a74dc
input set; macOS/Linux behavior is unchanged. This refresh includes the verified
Windows nonmutating-detection correction and does not resolve the failed Finder
window scenario.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS arm64 ZIP | 12,536,542 | `9afb4af9adb05b2483f8c9205fbd4aeb094f3c54f692f68d620dcafd9704c5b7` | 15 PASS |
| Linux x64 tar.gz | 34,217,977 | `fa78f25fcc14c65670caf83b5cfd1cee6eac6010292f26b9fdd6f55330df584f` | 14 PASS |
| Windows x64 ZIP | 28,576,933 | `29dc1af2cf161e20b6ec8e597e25e37c042fd5d6f315da75aa1f2620ce719733` | 14 PASS |

Fresh payload/license/notices/SFX/hash checks pass, including38 pinned Qt
licenses and macOS strict ad-hoc signatures. Relocated GUI ZIP/7z operations
and engine list cover Unicode/spaces, empty/binary bytes, broken archives,
missing modules and source preservation. Fresh dependency inspection resolves
11 Mach-O objects/127 loads, bundled private Qt symbols, four Linux Qt libraries
with relative runpaths,26 PE objects/345 imports/zero delay imports and55 trusted
external Windows system DLL names. Eight official engine/SFX inputs remain
unchanged per OS; both protected guest producers are reverified. Source ICNS
bytes and native packaged Windows executable icon color/masks pass their
existing bounded checks; no pixel equality claim is added.

Guest package run `20261017T160231Z-fa374edf` passes on both OS; Limited native
PE icon run `20261017T160419Z-36355668` passes. The first local collector calls
use an unresolved run placeholder and fail before network or output creation;
collection from the actual completed run then passes. This local harness error
is retained separately from product acceptance. Evidence is
`build/review/goal-delivery-aae7876/`. Disposable extractions and root one-shot
scripts are removed after archiving; current manual fixtures, archives/proofs,
incremental trees and caches are retained. Storage before/after remains OK.
Clean-OS/GitHub exclusions and the open native/cold prerequisites are unchanged.
