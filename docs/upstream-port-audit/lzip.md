# Read-only Lzip

The native module exposes `.lz` as a single unpacked file. `.tar.lz` uses the
existing nested TAR workflow. Listing, testing and extraction are supported;
creation and updates are omitted. Sources remain read-only. Partial output is
reported as an error, never as verified successful extraction.

## Source comparison and ownership

The selected 7-Zip 26.05 source baseline has no Lzip implementation. The format
author's [lzlib 1.16](https://www.nongnu.org/lzip/lzlib.html) supplies the decoder.
Complete original `lzlib.c`, included C modules and headers are unchanged under
`third_party/lzlib/`, compiled as C99. Its single translation unit includes
encoder functions; these are retained to preserve the upstream module, but no
archive update interface or creation command is exposed. BSD notices are retained
and installed. The combined module follows the project's GPL-2.0-only license,
retaining reused LGPL notices too.

The official project uses CVS. [The manifest](../lzip-baseline.json) records
the Git mirror `https://github.com/inikep/lzbench.git` at full commit
`2cc81a8ebccc0f43d4b7b7ff27847e5d39f96da9`, original inventory SHA256 values,
local owners and the official 1.16 release URL/SHA256. Every selected mirror unit
was compared byte-for-byte with that official tarball. The mirror supplies a
retrieval baseline, not an independent implementation. Unrelated benchmark
code is omitted. Original examples and fixtures are pinned too.

`src/lzipio.c` adapts `ffexample.c::ffdecompress`: bounded feed, read/write loop,
finish signal and completion check are preserved; native read/write/progress
callbacks replace stdio. The example permits unrestricted copying, distribution
and modification. All header, dictionary, LZMA, CRC32, data/member-size and
concatenation checks stay in lzlib. Source comparison covered complete
`LZ_decompress_read`, its lifecycle/callers, `decoder.c`, `decoder.h`, `cbuffer.c`
and `lzip.h`, including trailer, EOS/sync, truncation, distance and allocation
branches. Encoder dependencies remain included as required by the original
translation unit. Example CLI, signal/file management and resynchronization
services are omitted. No new damaged-input recovery mode is introduced for Lzip.

`src/lziphandler.cpp` reuses the existing LZ4/Brotli single-item native boundary:
Open/Close, properties, extraction callbacks/test mode and registration.
Inherited upstream symbols and original bytes are pinned by
`third_party/brotli/upstream.json` and its [audit](brotli.md).
The adaptation changes signature/decoder and delegates concatenation to lzlib.
`src/lzipstream.cpp` supplies native stream services; `cmake/Lzip.cmake` owns
build/runtime registration. Original `ffexample.c` and `bbexample.c` are pinned
for review of the partial decoder loop and test-fixture generation.

## Errors and update review

Truncation maps to UnexpectedEnd, decoder/trailer failures to DataError and
unrelated bytes after a verified member to DataAfterEnd. Unpacked size is
published only after success. Cancellation and native read/seek/write errors
propagate; test mode discards output. Existing engine path validation applies.
The module neither repairs source bytes nor guesses missing data. Supported
headers are version 1, as provided by lzlib; obsolete version 0 is not decoded.

Use the existing verifier/reviewer with a clone of the recorded mirror:

```sh
python3 scripts/upstream_baseline.py reference-verify \
  --manifest docs/lzip-baseline.json --upstream /path/to/lzbench
python3 scripts/upstream_baseline.py reference-review \
  --manifest docs/lzip-baseline.json --upstream /path/to/lzbench \
  --new-revision FULL_COMMIT --output /tmp/lzip-review.md
```

Compare proposed units with the official release too, then review dependencies,
omissions and local fixes before transfer. The manifest pins original bytes;
safe local refactoring does not require changing upstream hashes.

## Validation

2026-10-24: `lzip`, `lz4_frame` and `brotli` pass on macOS arm64, Linux x64
and Windows x64/MSVC. The final backend-error/native-stream changes also pass
`lzip`, `tar_stream` and `native_extract_callback` on all three platforms.
macOS additionally rebuilt the final header/name/attribution cleanup and reran
`lzip`. No UI controls changed; appearance acceptance was not required.

The new suite covers module discovery, signature detection, Unicode/spaced paths,
an independent official compressed fixture, dictionary variants, empty and
concatenated members, truncation, corrupted headers/payload/trailers, trailing
bytes, repeat extraction, test mode, cancellation, native read/seek/write failures,
bounded short reads and nested TAR through ArchiveBackend. It verifies source
immutability, absence of an update capability and backend failure for truncated
output; the independent sidecar extracts the same TAR bytes. Existing native
extraction callback tests cover malicious parent paths.

Original reference verification and the mapped adaptation review pass, including
byte-identical vendor units and the official binary fixture. The review helper
now reports binary local owners using SHA256 comparisons instead of attempting
UTF-8 decoding; text diffs remain available. Its full `upstream_baseline` suite
passes on macOS and Linux; the Windows run exceeds the existing 120-second suite
limit, separately from the passing archive suites. The focused binary-fixture
and existing text-change/deletion review tests both pass on Windows.

The macOS checks reused the prepared 26.05 runtime after verifying every recorded
adapted-source hash against this worktree. All platforms retained incremental
build trees and compiler caches. Native module/license installation rules were
configured; an installed user bundle was not replaced. Full/slow gates and
obsolete Lzip version-0 decoding are outside this validation.
