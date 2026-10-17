# P3-03 — Windows shared-geometry whole gate (2026-10-17)

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Product `17fab0f86d7706795f7f5cea62a98c277c75d5ee`, test revision94a1998;
snapshot `20261017T042605Z-065e1f08`, Windows11Pro x64, Qt6.12.0,
VisualStudio2022 17.14. Interactive Session1, Limited, Normal priority.
This is the first whole Windows gate after the shared CDialog/Copy change.
It precedes the subsequent Options keyboard working candidate.

## Actual result

`scripts/check.sh` passes formatting and the all-target build. Default CTest
finishes in2298.39s:327/343 executed suites pass,16 fail;
5406Qt pass/67fail/20platform skips. LastTest contains328 passed records, but
one is an empty command for the separately disabled device suite; it is excluded
from executed acceptance. No passing whole gate or final P3 acceptance is claimed.

Fifteen failures are privilege/setup consumers:
owned_temp, temp_cleanup, source_extract_advanced, source_link_extract,
link_dialog, network_folder, archive_creation_lifecycle, filesystem_link,
panel_rename_state, project_memory_setup, native_persisted_locations,
native_network_navigation, source_link_storage, file_manager_startup and
archive_metadata_ui. Their separate scoped Highest coverage remains in the
[shared-geometry receipt](p3-06-modal-geometry-2026-10-17.md); the corrected
Link fixture passes separately. No general elevated gate is retried.

The remaining failure is actual external codec discovery:
`ExternalCodecTests::initTestCase` receives `Codec discovery timed out` after
5215ms under the unchanged5000ms application deadline. This is a failed result,
not merely a readiness assertion overshoot. `codec_probe` passes all18Qt checks
in9.48s, including process/native deadline, cancellation and heartbeat consumers;
that pass does not fix or establish a cause for the external discovery failure.

## Scope and integrity

Eleven slow suites are excluded here and retain their separately verified
[three-OS all-eleven receipt](p3-03-long-acceptance-2026-10-16.md#current-shared-geometry-product-all-eleven-pass-on-three-os).
The existing no-new-VHD constraints are unchanged: fs_drives_device is disabled;
fs_drives_device_ui and namespace_hash execute only the recorded existing-drive/
alternate-stream subsets. No new disk or substitute raw OS-disk fixture is used.

All original CMake/source bytes are restored after the gate and stock
external-codec/process-pause targets reconfigure/rebuild successfully.1047
product inputs match committed17fab0f; all1472 compiled inputs match94a1998.
Eight official runtime/SFX hashes are rechecked after validation in both the
snapshot and protected producer. Source/runtime and no-new-disk receipts remain
in `build/review/final-availability-8e9343b-20261014/windows-final-current-full-v151-evidence/`.

This closes the missing current17fab0f whole-run evidence, while retaining the
actual cold failure, scope exclusions and separate privilege coverage. The
remaining native/rendered workflow acceptance and parent packages stay open.
