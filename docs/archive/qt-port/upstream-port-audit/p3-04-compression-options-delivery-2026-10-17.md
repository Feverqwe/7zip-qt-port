# P3-04 — Compression Options in private three-platform packages

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

2026-10-17. Product `7a8592f081756aa6f1e008c944a454a70ac1c794` includes
the accepted [Compression Options component](p3-06-compression-options-2026-10-17.md).
Release/BUILD_TESTING=OFF uses Qt6.12.0 and the existing official runtime/SFX
from https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

All1056 selected product/build inputs match committed blobs and remain unchanged
after packaging. Compared with the [preceding delivery](p3-04-options-delivery-2026-10-17.md),
only CMakeLists.txt and the two existing Compression Options presentation adapter
files differ. No source module, dependency, language or borrowed baseline changes.
The existing private macOS tree first matches its preceding1056 input hashes,
then receives those three committed inputs. Guests reuse current/build/release;
compiler caches and incremental build trees are preserved.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS arm64 ZIP | 12,535,403 | `9f2f3feae4260dd91646fbb20c375de55a91fd690236dd0219721cddb3430cbc` | 15 PASS |
| Linux x64 tar.gz | 34,216,647 | `fadfb4dfb62ab84a20b04b9f69b3c8af9fc0e1f0248ba2cbf876433949c821cd` | 14 PASS |
| Windows x64 ZIP | 28,575,060 | `6912bb6a6679c93cb4b9580dd492045dcb3b3ccc2dd8191223f57100cbecdaa7` | 14 PASS |

Relocated GUI create/test/extract and engine list verify ZIP/7z, Unicode/spaced
paths, binary/empty bytes, unchanged sources, broken archives and missing modules.
Fresh extractions pass payload constraints, all38 pinned Qt licenses, own/upstream
notices, applicable ICU/CRT notices and SFX provenance/hash checks. macOS strict
ad-hoc bundle signature verification passes; no public signing or publication.

Fresh dependency inspection resolves11 Mach-O objects/127 loads and the bundled
private QKeyMapper import. Linux resolves four bundled Qt libraries with relative
ELF runpaths and its matching private import. Windows26 PE objects have345 normal
imports and no delay imports;55 external basenames resolve to trusted system
locations through the existing no-DllMain inspection. Eight official runtime/SFX
inputs remain unchanged on each OS; both guest protected producers are reverified.
The macOS source-derived ICNS remains byte-identical to the accepted asset;
the earlier strict raw/native pixel failure is retained, without a new parity claim.
The three fresh packaged Windows executables again have16px native icon color
and transparency masks equal to the source ICO, checked under a Limited token
in run `20261017T145653Z-e1330341`; its disposable extraction is removed.

Evidence: `build/review/goal-delivery-7a8592f/`. Guest packaging run
`20261017T145406Z-c0dbd703` completes on both platforms with source/runtime guards.
Windows uses the ordinary Limited interactive session; Linux GUI smoke uses
Xvfb. Packaging and validation reuse the previous reviewed drivers with updated
committed inputs and evidence destinations. Archives, manifests and dependency/
payload proofs are retained; disposable validation extractions and duplicate
transport archives are removed after inspection. Existing build trees, compiler
caches and active/failure/reference evidence remain intact.
Host D: storage is checked before and after packaging and remains available.

The bounded package refresh is complete. P3-04 still depends on P3-03; Windows
cold discovery and remaining native workflow acceptance remain open. Clean-OS
and GitHub scope retain their explicit exclusions, rather than PASS results.
The parent count remains44/47. No push, publication, upstream or VM/disk change.
