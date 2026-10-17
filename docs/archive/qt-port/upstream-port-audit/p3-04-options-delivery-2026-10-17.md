# P3-04 — Options correction in current private packages, 2026-10-17

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Product `2a7303c1379f051154878947bb9062c1b9cac4bb` is packaged Release with
BUILD_TESTING=OFF on macOS arm64, Linux x64 and Windows x64. This adds the
accepted window-wide/footer and Cocoa physical-Control Options correction to
the [previous native icon/System delivery](p3-04-native-icons-delivery-2026-10-17.md).
Qt remains6.12.0; the official engine/SFX source remains
https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

All1056 product/build inputs match committed Git blobs before and after
packaging. Only OptionsDialog.cpp differs from the preceding40b3439 product.
The existing private Mac tree first matches its preceding1056 inputs, then
receives that one committed file. Guests reuse persistent current/build/release;
their full/default checks finish and release current locks before packaging.
The later language-fixture correction changes no shipped input.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS ZIP | 12,534,055 | `73a4416c030ba849c59dfdb1677659577091e0e14d817c6b079d9d358c9cf3bb` | 15 PASS |
| Linux tar.gz | 34,213,002 | `ef2fe5b0dd01109028e6eca3bdbfe22ac0f94eeebe8d6a7554fccbad3f02fc29` | 14 PASS |
| Windows ZIP | 28,570,419 | `7f8659389e8e816aa8382a234fa47a1ef2ef5f63ce4af110eadad4848e78cf82` | 14 PASS |

Relocated GUI create/test/extract and engine list cover ZIP/7z, Unicode/spaced
paths, binary/empty bytes, source preservation, broken archives and missing
module. Each fresh extraction passes payload constraints, all38 pinned Qt
licenses byte-for-byte, own/upstream/ICU/CRT notices and SFX provenance/hash.
Windows includes five CRT DLLs; macOS strict deep ad-hoc signature checks pass.
These are private artifacts, without public signing or publication.

Fresh dependency inspection resolves11 Mach-O objects/127 loads, the bundled
private QKeyMapper import, four Linux Qt libraries and origin-relative ELF
runpaths. Windows26 PE objects have345 normal imports/zero delay imports;
all55 external basenames resolve to trusted system locations using the existing
no-DllMain inspection. Eight official runtime/SFX inputs remain unchanged on
each platform; guest protected producers are verified again.

The three packaged Windows EXEs again have native16px color and transparency
masks equal to the original ICO. Their disposable native extraction is removed
in finally. The fresh Mac ICNS remains byte-identical to the source-derived
asset, SHA256 `e16ae4d76296a98a138db5a8e87b870f9dd74c39aaef22a45e6e1a6401285e95`.
The previous strict raw/native Mac pixel-probe failure remains retained in the
linked receipt; no new pixel-parity claim or system cache flush is made.

Evidence: `build/review/goal-delivery-2a7303c/`. Guest package runs are Linux
`20261017T133630Z-7a244b61` and Windows `20261017T135234Z-ffbf440e`, both with
source/runtime guards and successful relocation receipts. Windows packaging
and native icon checks use Limited interactive tokens. Linux GUI smoke uses
Xvfb. Archives, manifests, dependency/payload proofs and task drivers are retained;
three completed validation extractions, duplicate transport tars and archived
one-shot source scripts are removed. Persistent builds/caches, prior archives
and current failure/reference evidence remain. Host D: storage is checked OK;
no VM, disk or upstream changes occur.

This completes the current bounded delivery refresh. Clean-OS is waived by the
user, not passed. P3-04 still depends on the open P3-03 cold/native acceptance;
P3-06 and the44/47 parent count are unchanged. No push or publication.
