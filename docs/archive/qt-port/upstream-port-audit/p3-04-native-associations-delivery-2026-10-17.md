# P3-04 — private packages after native association corrections

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

2026-10-17. Product `5ccf3f11d64a9b6755d0b6fb6aaf0a7b1f9577ed` includes
the [Windows command and macOS file-open corrections](p3-06-native-associations-2026-10-17.md).
Release/BUILD_TESTING=OFF uses Qt6.12.0 and the existing official engine/SFX
from https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

All1056 selected product/build inputs match committed blobs before and after
packaging. Relative to [7a8592f delivery](p3-04-compression-options-delivery-2026-10-17.md),
only CMakeLists.txt, SystemPage.cpp, main.cpp, qtfilemanagermain.cpp and its startup
header differ. The existing private macOS tree first matches the preceding1056
input hashes, then receives those five inputs. Guests reuse current/build/release;
no compiler cache or incremental build tree is reset. No copied module,
dependency, source language or upstream baseline changes.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS arm64 ZIP | 12,536,174 | `ae3ab69ad24d3f8c9bdc7762acdc725a2bdee71918f6f222beacaf380c8db544` | 15 PASS |
| Linux x64 tar.gz | 34,217,682 | `a9eaa7bfd6f43c5f2dd47f33d197c88e52c959132e62399b19ffdeefc17238e4` | 14 PASS |
| Windows x64 ZIP | 28,576,594 | `dfc5fd13c30b88f40ace65476c9e198d606a778831015a2c6c03d126603913ef` | 14 PASS |

Fresh extractions verify relocated GUI ZIP/7z create/test/extract and engine list,
Unicode/spaced paths, binary/empty bytes, source preservation, broken archives
and missing modules. Payload constraints,38 pinned Qt licenses, applicable
own/upstream/ICU/CRT notices, SFX metadata/hash and macOS strict ad-hoc signatures
pass. Fresh dependency inspection resolves11 Mach-O objects/127 loads, matching
private Qt imports, four bundled Linux Qt libraries/relative runpaths, and26 PE
objects/345 imports/zero delay imports. All55 external Windows DLL basenames
resolve to trusted system locations through no-DllMain inspection.

Eight official runtime/SFX inputs remain unchanged per OS and both guest
protected producers are reverified. Current source-derived ICNS bytes remain
unchanged; the earlier strict raw/native pixel failure is retained. Three
packaged Windows executable16px icon color/masks match the source ICO in
Limited run `20261017T152349Z-67fce4ac`.

Guest package run `20261017T152123Z-a757c472` passes on both OS with source/runtime
guards. Linux GUI smoke uses Xvfb; Windows uses the ordinary Limited interactive
session. The first local Windows dependency post starts before its PE inspection
producer finishes and fails with FileNotFound; the ordered follow-up passes.
That harness-order error is not a product failure or an accepted initial check.

The user retries the prepared ZIP through Finder Open With and reports that
the archive opens, while a second ordinary directory window also appears.
Thus the reported E_FAIL is resolved in the manual retry, but initial native
document launch still needs the extra startup-window correction. The prepared
ZIP remains byte-identical. Native handler inspection after this retry resolves
org.sevenzip-desktop to the local build/dev bundle, unlike the preceding restored
Archive Utility state. No matching owned app remains running at inspection;
the exact executed bundle path cannot be independently recovered from that
process query. Default restoration/intent is pending user clarification; no
automatic preference write or unverified restoration claim is made.

Evidence: `build/review/goal-delivery-5ccf3f1/`, current native-association receipt,
and `/private/tmp/sevenzip-system-registration-5ccf3f1/`. Three disposable package
validation extractions and duplicate transport archives are removed; manual
fixtures remain for current review. Archives/proofs, failure/reference evidence,
build trees and caches are preserved. Host D: storage is checked afterward.

Bounded delivery is complete; clean-OS/GitHub exclusions remain exclusions.
P3-04 depends on P3-03, and extra startup-window/native/cold acceptance remains
open under P3-06. Count44/47; no push, publication, upstream or VM/disk changes.
