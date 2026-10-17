# P3-04 — Private delivery after modal owner correction

2026-10-17. Product `3a0b3b7da437ee97392d2ab7be712582a4baab27`
contains the [modal owner correction](p3-03-owner-publication-2026-10-17.md).
Release/BUILD_TESTING=OFF uses Qt6.12.0 and the existing official engine/SFX
from https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

All1058 selected product/build inputs match committed blobs, exported directly
from Git, before and after packaging. Only CMakeLists.txt and the Qt reload
adapter differ from the preceding56c01ba product inputs. Original source bodies,
module boundaries, source languages and dependencies retain their audit scope.
The existing private macOS source/build first matches the previous1058 hashes
and receives exactly those two updated files. Guest current/build/release and
all compiler caches are reused; no cold build, cache reset or engine rebuild.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS arm64 ZIP | 12,537,504 | `ee75fc8822e7ebad3ed13f1f4f61647b563baf9f1850b6df22e4db6367689ad5` | 15 PASS |
| Linux x64 tar.gz | 34,218,003 | `2ab6f1686f6b8855b9f1504dbf022104af748e5e9aba73eae3b4615495a7ad30` | 14 PASS |
| Windows x64 ZIP | 28,576,855 | `2495c193e7a4b6a0bc03517bfc941a9b7cf4c3ea608138a3013d4a4496480b11` | 14 PASS |

Fresh extractions pass payload validation, all38 pinned Qt licenses,
own/upstream and applicable ICU/CRT notices, SFX metadata/hash and macOS strict
ad-hoc signatures. Relocated GUI ZIP/7z create/test/extract and engine list cover
Unicode/spaces, binary/empty bytes, source preservation, broken archives and
missing modules. Each original runtime/SFX set retains all eight hashes.

Fresh dependency checks resolve11 Mach-O objects/127 loads, the private Qt
import against bundled QtGui, four bundled Linux Qt libraries/relative runpaths
and26 PE objects/345 imports/zero delay imports. External Windows DLL names
resolve to trusted system paths without running DllMain. Both guest protected
runtime producers are independently reverified.

Guest package run `20261017T183014Z-f83e770a` passes on both OS. Limited
interactive run `20261017T183301Z-7cfd0b70` confirms native16px color/masks of
all three packaged Windows EXEs match the original ICO. Source-derived macOS
ICNS bytes retain SHA256
`e16ae4d76296a98a138db5a8e87b870f9dd74c39aaef22a45e6e1a6401285e95`.
The earlier strict raw/native macOS pixel mismatch remains recorded separately.
Read-only LaunchServices queries after all checks return Archive Utility for
both ZIP viewer/all roles, preserving the user's requested default restoration.

Evidence: `build/review/goal-delivery-3a0b3b7/`. Checked archives, committed-input
proofs, runtime/dependency/license/icon receipts and exact drivers are retained.
Three completed validation extractions, duplicate transport tars and root one-shot
drivers are removed after verification. Incremental builds/caches, active manual
fixtures and current failure/reference evidence are preserved. Host D: storage
is checked OK. No VM/disk changes, push, publication or upstream modification.

This completes the bounded delivery refresh. Clean-OS/GitHub exclusions and
parent acceptance are reconciled in the
[current matrix](../ux-port-audit.md#current-final-acceptance-matrix-2026-10-10).
