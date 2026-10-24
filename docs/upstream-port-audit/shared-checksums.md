# Shared XXHash and Adler32

Scope: XXH32 for EROFS xattr filters and LZ4 Frame; SDK XXH64 for hash
commands, Zstandard checksums and Btrfs; Adler32 in the SDK zlib wrapper
used by HFS and native APFS, DMG, XAR and compressed VMDK consumers. No format decoder,
recovery, framing, cancellation or checksum policy is replaced.

## Source comparison and ownership

The 7-Zip baseline is https://github.com/ip7z/7zip.git at
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Complete original inventory units
`C/Xxh64.c`, `C/Xxh64.h`, `CPP/Common/Xxh64Reg.cpp`,
`CPP/7zip/Compress/ZlibDecoder.cpp` and `.h`, their callers and dependencies
were reviewed before adaptation. `docs/upstream-baseline.json` maps the
adapted C and C++ owners and pins SHA256 of both original implementation units.
The original header and hash registration remain unchanged.

- `src/C/Xxh64.c` retains SDK symbols, state layouts, seed zero and the
  original streaming block/tail orchestration. Hash arithmetic and finalization
  use the existing pinned libzstd xxHash. A narrow adapter translates four SDK
  accumulator lanes, total length and pending bytes to the statically linked
  library state. This deliberately depends on the pinned state representation;
  review it on a library update. Digest does not mutate the caller's state.
  Architecture-specific SDK hash arithmetic is omitted.
- `src/lib/xxhash.c` retains Linux's seeded `xxh32` service for EROFS and
  delegates to the same XXH32. Its original Linux inventory/hash remains in
  `docs/erofs-linux-baseline.json`; no Linux hash arithmetic remains locally.
- LZ4 Frame retains the unchanged official framing implementation and consumes
  the shared xxHash header through its include path. The bundled LZ4 xxHash
  `.c` and `.h` are removed locally; original hashes remain inventory-only
  references in `docs/lz4-frame-baseline.json`.
- `src/CPP/7zip/Compress/ZlibDecoder.cpp` replaces only Adler32 arithmetic
  with `adler32_z`. Empty updates preserve the supplied SDK seed. Output
  accounting still follows bytes actually accepted by the destination.
  Optional absent trailers, rejection of partial/incorrect trailers, the
  DEFLATE decoder and all error/progress paths retain their upstream bodies.

The authoritative unchanged xxHash is part of https://github.com/facebook/zstd.git
at `f8745da6ff1ad1e7bab384bd1f9d742439278e99`, with original whole-file hashes
and owners in `docs/zstd-encoder-baseline.json`. LZ4's omitted copy is pinned
at https://github.com/lz4/lz4.git,
`ebb370ca83af193212df4dcbadcc5d87bc0de2f0`.
The unchanged Adler implementation is https://github.com/madler/zlib.git at
`da607da739fa6047df13e66a2af6b8bec7c2a498`, mapped and hashed in
`docs/zisofs-zlib-baseline.json`. No dependency revision was advanced.

The native engine/console overlay receives both adapted SDK units and the
unchanged Adler source/header closure through the existing runtime manifest.
Vendored runtime dependencies are mapped using the same prefix-removal rule
as local integration, with duplicate destination rejection. zlib C compiles
with `Z_PREFIX`, retaining its source language and avoiding symbol collisions.
The CMake HFS module links the existing zlib target. The adapter explicitly
uses the zlib C calling convention on MSVC; two undefined-feature-macro
warnings are suppressed only while including the unchanged zlib header under
the native SDK's `/Wall` policy. License notices for zlib
are installed for the shared runtime in addition to existing module notices.
The OpenSSL configure probe now includes `openssl/evp.h` for its EVP cipher
calls; the prior probe failed on a fresh configure despite a valid library.

## Review and verification

Use `scripts/upstream_baseline.py verify/review` for the SDK units and
the existing `scripts/upstream_baseline.py reference-verify/reference-review` workflow for the
zlib, Zstandard, LZ4 and Linux manifests (see each owning audit). Compare
proposed original whole units with their recorded baseline and local owners;
do not hash an adaptation as its original baseline.

`shared_checksum` checks frozen pre-change XXH32/64 results, seeded XXH32,
unaligned buffers, split updates, repeated digest followed by more input,
SDK block/tail calls, independent Adler vectors, partial writes and optional
Adler trailer outcomes. Existing native hash, filesystem and frame suites
exercise direct consumers. The runtime-tooling regression compiles a vendored
C dependency, modifies its header and verifies changed executable output.

## Validation, 2026-10-24/25

Feature `feature/shared-xxhash-adler32`, based on
`4466186335f168c0dc632bf793862426ec11df6c`:

- macOS arm64: affected libraries, application and native runtime built;
  all 17 focused suites passed (23.93 s). After localized tooling/include
  follow-ups, three tooling suites passed (14.76 s), three checksum/native
  consumer suites passed (4.07 s), and the final checksum suite passed again.
- Linux x64: all 17 suites passed (80.16 s), runner receipt
  `20261024T210209Z-d431d9dd`. Final includes/guards: shared_checksum, HFS and
  Zstd passed (8.94 s), receipt `20261024T210755Z-3aa33af0`.
- Windows x64/MSVC: native runtime and selected targets built. Initial full
  selection passed 16/17; linux_frames_stream::backendNestedExtraction failed
  at codec discovery before extraction (receipt `20261024T210436Z-042890e1`).
  Focused repeat passed checksum/HFS/Zstd but repeated that discovery timeout
  (`20261024T211555Z-da1c485a`). Both the retained older LZ4 build and the
  same current test environment with the pre-change OpenSSL core passed the
  control scenario (`20261024T212306Z-43041f87`, `20261024T212447Z-0b1712c0`).
  The final core was restored byte-for-byte; its unchanged linux_frames_stream
  suite then passed in 3.22 s (`20261024T212654Z-62990350`). No timeout was
  changed. Cause of the two earlier discovery timeouts is unconfirmed;
  reliable cold catalog startup is not established by this acceptance.

SDK verification passed 398 exact, 269 adapted and 21 local owners. All 14
zlib and 65 libzstd mapped units equal their pinned original hashes; all 12
LZ4 original inventory units verified, and reference-review was exercised.
No new library source or revision was introduced. Whitespace checks passed.
Review metadata and receipts are retained under task-local
`build/review/shared-hash` and the shared runner log root. Disposable control
sources, libraries and driver are removed; incremental trees/caches are kept.

No full/slow gate, release installation, physical media, UI/pixel acceptance
or performance claim is included. Checksum equivalence covers the frozen
vectors and selected consumers, not every possible corrupt input.
