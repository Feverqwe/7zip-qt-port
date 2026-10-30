# Shared liblzma CRC64

SDK XZ decoding/encoding, the CRC64 hash provider, and nested XZ streams in
SquashFS and Linux frames use the existing pinned liblzma dependency for CRC64.
Archive framing, decoding, recovery, integrity reporting and cancellation retain
their existing owners.

## Source comparison and provenance

The 7-Zip source is <https://github.com/ip7z/7zip.git> at
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Reviewed complete inventory units
`C/XzCrc64.c`, `C/XzCrc64Opt.c`, `C/XzCrc64.h`, their initialization/registration
in `CPP/Common/XzCrc64Init.cpp` and `XzCrc64Reg.cpp`, and the check lifecycle in
`C/Xz.c` and its encode/decode consumers. The existing
[SDK manifest](../upstream-baseline.json) records full original implementation
hashes and maps both adapted C units under `src/C/`. Original headers and
consumers remain unchanged.

`Crc64Update` retains the uncomplemented SDK seed/result contract through
inversion around the public `lzma_crc64` API. Empty updates preserve the seed.
`Crc64GenerateTable` retains its symbol but needs no initialization: liblzma's
portable tables are immutable. The SDK table generation, slicing routines and
endianness dispatch are removed. `XzCrc64Opt.c` retains only an empty translation
unit for the original native makefiles. There is no second CRC64 implementation
or runtime fallback. The independent original SFX still uses its unmodified
upstream build, as before.

The library is XZ Utils 5.8.4, <https://github.com/tukaani-project/xz.git>,
`d3e650e63c110e830fd5391e7f8b45df0b91d3da`, already used for EROFS MicroLZMA.
The [existing liblzma manifest](../erofs-liblzma-baseline.json) now also pins
complete original `check.h`, `crc_common.h`, `crc64_fast.c` and both generated
endian table headers, with unchanged local owners under `third_party/xz`.
The API, common definitions and portable integer helpers reuse their existing
inventory. No library revision is advanced. Sources retain 0BSD author notices;
the SDK units retain Igor Pavlov's public-domain notice.

The shared C11 target builds the portable slice-by-four CRC64 path, with
no mutable lazy CPU dispatch, threading or assembly. Existing EROFS decoding
stays in its separate target. Native Console/Format7zF use the same unchanged
source/header closure through the runtime manifest; their original makefiles
receive a narrow dependency compile rule with static C ABI and C11 settings.
The CMake target selects its detected target byte order; native snapshot rules
select the builder's byte order. The supported native macOS/Windows/Linux
arm64/x64 builders are little-endian; big-endian execution is not validated.
The library's 0BSD notice is installed at the shared runtime license root in
addition to its existing EROFS location. This change makes no speed claim.

The [later shared-primitives continuation](shared-checksums.md#crc32-and-gpl-hash-library-consolidation-2026-10-25)
also adds CRC32 to this CMake library; its native runtime closure remains CRC64-only.

## Verification and update review

Use the existing [update procedure](../upstream-update.md). Reproduce the
original SDK baseline with `scripts/upstream_baseline.py verify/review`, selecting
`src/checksums/crc64/XzCrc64.c` and `src/checksums/crc64/XzCrc64Opt.c` for review. Reproduce library originals
and proposed updates with `reference-verify/reference-review`, passing
`--manifest docs/erofs-liblzma-baseline.json` and a checkout containing the
recorded revision. Compare full original units and local adapters before
advancing either manifest; original hashes do not track local formatting.

Behavior coverage extends `shared_checksum` with frozen pre-change SDK outputs
for three seeds, alignment and block/tail boundaries, split updates and null
empty updates. `native_hash_bundle` exercises registered CRC64, split input and
reset to an empty file. XZ fixtures add a corrupt CRC64 trailer, exercised by
the console and direct stream consumers. A console-created CRC64 XZ stream is
also decoded and checked through Python's independent liblzma installation.
Existing XZ, SquashFS and Linux-frame
suites cover their success/error/cancellation paths. No source-text assertions
or configuration-value tests are added.

## Validation, 2026-10-25

Branch `feature/library-consolidation`, based on `d89dfe1f`:

- macOS arm64: application and official native runtime built. All 11 focused
  suites passed (21.64 s): checksum/hash providers, XZ, SquashFS, Linux frames,
  their stream consumers, upstream baseline/inventory and runtime cache tooling.
  The added XZ writer interoperability case passed separately (2.02 s).
  Final localized checksum/hash/XZ/tooling follow-up passed 4/4 (16.15 s).
- Linux x64: all 10 selected checksum/hash/XZ/SquashFS/Linux-frame/tooling suites
  passed (39.22 s), runner receipt `20261025T040802Z-6b5b2d94`. Added writer case
  passed in the XZ suite, receipt `20261025T041243Z-3c34aeaa`.
  Final localized checksum/XZ/tooling follow-up passed 3/3 (20.55 s), receipt
  `20261025T041438Z-3e093cbd`.
- Windows x64/MSVC: official native runtime and selected CMake targets built.
  The initial batch passed 9/10, including all checksum/hash/XZ/SquashFS/frame
  consumers; `upstream_baseline` exceeded its unchanged 120 s timeout (receipt
  `20261025T040820Z-1de05093`). Final checksum and XZ suites, including writer
  interoperability, passed; `upstream_baseline` timed out again (receipt
  `20261025T041851Z-79e84fbd`). These batches are not full PASS results.
  A bounded split diagnostic also exceeded 120 s in its first group, after ten
  passes and while executing the unchanged review-report test (receipt
  `20261025T042434Z-3e741697`); it does not establish completion of all 23 tests.
  The directly affected existing vendored-C/header-rebuild regression passed
  separately (receipt `20261025T042759Z-a7ae1e52`, one test 11.66 s). No default
  CTest timeout, registration or cache setting was changed.

SDK verification passed 396 exact, 271 adapted and 21 local source owners;
all 37 liblzma original inventory units verified. SDK and library review reports
were generated through the existing review commands. Native/app builds reuse
the installed OpenSSL, incremental output trees and compiler caches. Initial
runner configure attempts lacked an OpenSSL path in the new branch; their
retries explicitly reused the existing dependency installation.

Evidence is retained in task-local `build/review/library-consolidation` and the
shared runner receipt directories. Temporary vector-generator binaries and the
one-off runner configuration driver are removed; incremental trees/caches stay.
No full/slow suite, installed-package, physical-media, UI or performance
acceptance is claimed.
