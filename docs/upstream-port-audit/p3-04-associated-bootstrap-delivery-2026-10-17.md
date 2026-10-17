# P3-04 — private packages after initial document-window correction

2026-10-17. Product `d0a74dc777f545edaf6ab0365325938a2bc67f57` includes the
[initial macOS document-window correction](p3-06-associated-bootstrap-2026-10-17.md).
Release/BUILD_TESTING=OFF uses Qt6.12.0 and the existing official engine/SFX
from https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

All1056 selected product/build inputs match committed blobs before and after
packaging. Six inputs differ from the preceding5ccf3f1 delivery, all confined
to CMake and the Qt desktop/file-open startup boundary. The persistent private
macOS tree first matches the previous1056 hashes and receives those six inputs;
guest current/build/release and all compiler caches are reused. No borrowed
module, source language or baseline changes.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS arm64 ZIP | 12,536,542 | `55ec5a30e61fc4ca7392056f6767b09ca41ae675800a216879f7718746606d6c` | 15 PASS |
| Linux x64 tar.gz | 34,217,973 | `d9625e4b22890ad5f2bcd9fd7b4792fec633c082faeacc2dc198ff4d55bafbae` | 14 PASS |
| Windows x64 ZIP | 28,577,014 | `ea57e8060c6c3e6c618fa7092e06c7fd859ee26c8357e3b14e7e2ca225015c64` | 14 PASS |

Fresh extractions pass payload checks,38 pinned Qt licenses, own/upstream and
applicable ICU/CRT notices, SFX metadata/hash and macOS strict ad-hoc signatures.
Relocated GUI ZIP/7z create/test/extract and engine list cover Unicode/spaces,
binary/empty bytes, source preservation, broken archives and missing modules.
Fresh dependency checks resolve11 Mach-O objects/127 loads, private Qt imports,
four bundled Linux Qt libraries/relative runpaths and26 PE objects/345 imports/
zero delay imports. All55 external Windows DLL basenames resolve to trusted
system paths without running DllMain. Eight official runtime/SFX inputs per OS
remain unchanged; both guest protected producers are reverified.

Guest package run `20261017T154226Z-52aec469` passes on both OS. Packaged
Windows executable16px icon color/masks match the source ICO in Limited run
`20261017T154354Z-16ec67f7`. The source-derived ICNS hash is unchanged; the
earlier raw/native pixel limitation remains. A Windows-only local validator was
initially copied to the macOS receipt directory; its license assertion failed.
The existing macOS validator replaces it, the wrong disposable extraction is
removed and the correctly selected checks pass. This was a harness selection
error, not a product failure or accepted initial check.

The fresh macOS app and unchanged owned Unicode ZIP/payload are prepared at
`/private/tmp/sevenzip-system-registration-d0a74dc/` for manual Finder retry.
Before this retry, the user's requested ZIP default restoration completes via
native AppKit and independent query confirms Archive Utility. Automated Cocoa
entry coverage establishes one initial archive window and independent repeated
documents; physical Finder acceptance remains pending.

Evidence: `build/review/goal-delivery-d0a74dc/`. Disposable validation extractions
and root one-shot scripts are removed after archiving the scripts; checked
archives/proofs, active manual fixtures, failure/reference evidence, incremental
trees and caches are preserved. Host D: is checked before/after and remains OK.
No full/slow gate is repeated for this bounded delivery. Clean-OS/GitHub
exclusions remain exclusions. P3-03 native/cold and P3-06 remaining acceptance
keep parent count44/47; no push, publication, upstream or VM/disk changes.
