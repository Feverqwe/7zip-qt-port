# Modern LZ4 Frame

## C++23 dependency facade — 2026-10-26

The pinned complete single-threaded `LZ4IO_decompressLZ4F` body, current C loop,
native services, handler callers and direct tests were compared before changing
the boundary. The original inventory hash was verified. The C loop and all
library/framing/cleanup algorithms remain unchanged.

`sevenzip.dependencies.lz4` exposes the existing synchronous C loop through
nonthrowing services that read/write bounded spans. A typed source extent and
result carry input/output counters; native HRESULT and cancellation precedence
remain with the existing service owner. The shared callback bridge rejects a
read count larger than its supplied buffer before returning control to C.
The C++ stream adapter and its direct tests import the facade, with the same
header API for compilers without scanning. No virtual dispatch, intermediate
buffer allocation or additional copying is introduced. Existing C consumers
continue to link the same authoritative raw decoder.

Focused acceptance: the existing `lz4_frame` suite passed on macOS LLVM 23,
Windows MSVC, Linux GCC through the header facade and Linux Clang 18 through
the named module. GCC 16 built the facade and importing plugin. Clang-Tidy
lifetime checks passed; its remaining trailing-return suggestions concern the
C-compatible bridge declarations. A malformed service read-count regression
checks rejection before decoding/output and preservation of cumulative output.
This does not establish a new full application or performance acceptance.

## Sources and integration

Official library: <https://github.com/lz4/lz4.git>, v1.10.0, commit
`ebb370ca83af193212df4dcbadcc5d87bc0de2f0`.
[Baseline](../lz4-frame-baseline.json) pins complete original inventory units,
SHA256 values and local owners. `third_party/lz4/lib/` contains unchanged C
`lz4`, `lz4hc`, `lz4frame` sources/headers and the BSD-2-Clause license.
XXHash is shared with libzstd; the former LZ4 copy remains an inventory-only
reference. See [shared checksums](shared-checksums.md).
Source language, public types and algorithms are preserved.

Compared before integration: complete Frame header/decoder bodies, callers,
state transitions, block history, size/checksum checks and skippable handling.
Official 7-Zip 26.05 has no modern LZ4 handler/decoder. The existing Linux
legacy reader understands a different framing format.

`src/formats/lz4/lz4frameio.c::LZ4IO_decompressLZ4F` adapts the single-threaded
`programs/lz4io.c::LZ4IO_decompressLZ4F` loop to bounded native streams and HRESULT
I/O/progress cancellation. The original GPL-2.0-or-later notice is retained;
the combined module uses GPL-2.0-only. The loop remains C with explicit cleanup;
`LZ4F_getFrameInfo`/`LZ4F_decompress` retain all header, history, decoding and
integrity algorithms. `src/formats/lz4/lz4framestream.cpp` supplies narrow C++ COM services,
contains exceptions at the C boundary, and maps results. FILE services, process
termination, console preferences,
sparse output, threading and producer commands are omitted. The complete
`lib/lz4file.c` read API was also inspected: its FILE ownership/read-result
contract does not supply bounded archive results. It is reference-only.

`Lz4FrameHandler` reuses the existing `LinuxFrameHandler` single-item
Open/Extract/properties pattern, itself adapted from `BrotliHandler`.
Inherited provenance and LGPL notices are in [Linux frames](linux-frames.md)
and [Brotli](brotli.md). Exports reuse `brotlimodule.cpp`'s recorded official
7-Zip subset. Existing SDK ArchiveExports, DummyOutStream, StreamUtils,
ProgressUtils and property/string dependencies remain unchanged.

The isolated dynamic Formats module registers read-only `Lz4`, `.lz4`, normal
and all sixteen skippable signatures. Frames are decoded sequentially.
`Lz4Legacy` retains its own magic and Linux framing reader. `.tar.lz4` uses the
existing nested-file workflow: extract the compressed TAR, then open TAR with
the native engine. Creation is not advertised. The library's encoder code is
required by lz4frame.c and is also the test fixture producer; no LZ4 executable
is built or shipped.

LZ4 and Zstd share skippable magic values. The existing official
`ZstdHandler::Open2` parser retains its skip/data loops but returns S_FALSE
when initial skippable frames lead to modern LZ4 before any Zstd data frame.
This lets the native format chooser continue to LZ4 instead of claiming its
metadata as a complete Zstd archive. Zstd-only skippable inputs and trailing
data after actual Zstd frames keep their existing behavior. This narrow
adaptation is also in the prepared official runtime overlay.

## Shared raw decoder replacement

The Linux-derived `src/lib/lz4/` decoder, its compatibility layer and copied
Linux LZ4 header are replaced with the official library. Compared complete
implementations and every caller: legacy/SquashFS use bounded
`LZ4_decompress_safe`; EROFS also uses `LZ4_decompress_safe_partial`, with equal
target/capacity after integer bounds validation. Both versions retain those
full/partial contracts. Unused PREBOOT fast decoding stays excluded. No old
streaming state crosses the boundary. Public library headers replace copied
declarations; Linux framing/filesystem algorithms and bounds remain intact.

One C static target compiles the authoritative raw decoder; isolated modules
link it and retain Linux local symbol binding. HC/Frame checksum dependencies
are separate. Linux manifests retain original hashes as historical references
and explicitly record replacement dependencies. Native installs include LZ4
notices alongside the affected GPL/LGPL terms.

## Error and scope boundaries

Truncation reports UnexpectedEnd, invalid headers/blocks/checksums DataError,
and unrelated trailing bytes DataAfterEnd. Partial output is not successful;
unpacked size is published only after complete success. Test mode discards
output. Native read/write/seek and cancellation errors propagate. Sources
remain read-only; extraction-path handling stays in the existing engine.

Dict-ID is only a lookup hint and alone does not reject a frame. Actual external
dictionary references require the dictionary bytes; the archive API currently
has no LZ4 dictionary selection. The library's normal decoder validates bounds
and reports unresolved references as errors. No dictionary is guessed and no
unverified success is reported. Dictionary selection and encoding commands are
omitted from this read-only integration.

## Baseline verification and update review

```sh
python3 scripts/upstream_baseline.py reference-verify \
  --manifest docs/lz4-frame-baseline.json --upstream /path/to/lz4
python3 scripts/upstream_baseline.py reference-review \
  --manifest docs/lz4-frame-baseline.json --upstream /path/to/lz4 \
  --new-revision FULL_COMMIT --output /tmp/lz4-review.md
```

Existing tooling verifies original bytes, compares a proposed commit with the
baseline and compares mapped local owners with their original units. Review
the partial loop and its omitted services separately; do not replace it with
the complete CLI module. Recheck filesystem/legacy consumers on raw updates.

## Validation

2026-10-24, macOS arm64 and Linux x64: eleven focused suites pass:
`lz4_frame`, `linux_frames`, `linux_frames_stream`, `squashfs`,
`squashfs_stream`, `erofs`, `erofs_reader`, `erofs_module`, `zstd`,
`zstd_stream`, `cpio_stream`. The final C loop and modern/legacy coexistence
check were rebuilt and passed in `lz4_frame` on both platforms.

The new suite checks all block sizes, linked/independent history, content-size
metadata, block/content checksums, stored and compressed blocks, empty and
concatenated frames, all skippable magics, common-extension format selection,
truncation, checksum corruption and trailing data. It checks bounded short-read
input and exact consumed boundary, native read/seek/write failures, cancellation,
repeat extraction, test-mode discard and absence of successful size after errors.
Unicode/spaced paths and source immutability are checked. ArchiveBackend
lists/extracts `.tar.lz4`, then opens/extracts its TAR payload; the native console
extracts the exact TAR bytes. Existing EROFS stream seek/read coverage and
creator fixtures exercise the raw decoder's partial/full consumers; legacy,
SquashFS, Zstd skippable-only/leading/intermediate frames and CPIO remain covered.

Windows x64/MSVC: the initial run passed ten suites; EROFS's backend-discovery
case timed out, with its other 58 cases passing. The focused repeat passed
both the complete EROFS module suite and final-C LZ4 suite, including coexistence.
Thus all eleven selected suites have passing evidence on all three platforms.

The official library's original bytes, SDK baseline inventory and source hashes
were verified. The existing reference-review command was exercised against the
pinned commit, including the local partial-loop diff. Native module and license
installation rules were configured; the macOS dynamic runtime advertises both
modern and legacy LZ4 alongside the filesystem modules. No installed user bundle
was replaced. Full/slow gates and UI appearance acceptance were not run; this
change adds no UI controls. Build trees and compiler caches were retained.
