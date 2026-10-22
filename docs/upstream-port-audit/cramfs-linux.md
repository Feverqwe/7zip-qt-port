# Cramfs read-side modernization against Linux

## Optional whole-volume verification and limited reading — 2026-10-22

Recompared Linux `inode.c:cramfs_read_super`/`cramfs_finalize_super`, folio
reads and their source dependencies with SDK opening, CheckCrc, archive
properties and CLI/backend/native-folder consumers. Baselines and retrievable
repositories are recorded below. Linux reads fsid metadata but does not require
whole-volume CRC verification before ordinary file access. The archive adapter
added this scan; an I/O error in otherwise unused volume bytes refused all
opening. A completed CRC mismatch also set a fatal header flag, causing CLI
listing to fail even when the file bytes were readable.

The same CRC scan and SDK CRC implementation are retained. Ordinary scan Seek/
Read failure or a short read now leaves the checksum explicitly unverified;
E_ABORT/E_OUTOFMEMORY and all progress-callback failures remain fatal. No
partial scan is presented as a checked checksum or as a mismatch. A completed
mismatch and a source shorter than the declared volume remain diagnostics,
published as warnings rather than fatal opening errors. Actual metadata/path/
bounds checks and file read/extraction errors are unchanged. No source writes,
repair or replay are performed.

Named read-only limitations distinguish unavailable verification, mismatch and
short source. The existing native-folder UI requests explicit consent for the
selected opening, defaults to Cancel, retains the limited-access badge and
uses the existing refresh/reopen rules. Healthy controls have no limitations
or false prompt. EN/RU diagnostics reuse the existing translation flow. The
standalone overlay receives the same warning/property behavior; only the GPL
module's bounded source path runs the optional streaming CRC scan.

The ABI regression first reproduced refusal for both an I/O fault and a short
read in an unused source tail. It now verifies exact file data despite that
fault, fatal cancellation/OOM and progress-callback failure, cleared failure
state and healthy reopen without a stale limitation. CLI controls on both
engines and both byte orders retain exact payload after a checksum mismatch.
UI controls cover consent/refusal, mismatch, short source, healthy opening,
copying, image immutability, reopen and visible EN/RU normal/compact geometry.
Current Cocoa widget JSON confirms translated diagnostic text fits; native
Windows pixel parity and physical devices are not claimed.

Final six focused suites (`cramfs`, `cramfs_module`,
`filesystem_cramfs_read_ui`, `language`, `upstream_baseline`,
`upstream_inventory`) passed on macOS/Linux/Windows; test phases took
6.84/4.49/22.55 seconds. Runner receipt: `20261022T051434Z-c9c5b9a6`.
The incremental macOS application build and diff check passed. Both engine
overlays were rebuilt with existing caches; no full/slow suites or native
mounts were run. Disposable fixtures were cleaned by their owners, superseded
task UI reports removed, current review evidence and caches retained.

This completes the optional-verification scenario, not the full cramfs audit.
Contiguous metadata-prefix loading and directory-local errors still require
their own source comparison and useful-read adaptation where supported.

## Independent file streams — 2026-10-22

Rechecked the complete SDK `GetStream`, `ReadBlock`, `Close`/`Free`, allocation
and cached-stream consumers against the baseline below, plus the Linux folio
reader and the archive C source/decoder callbacks. A returned stream retained
the public archive handler, whose mutable source and buffer were cleared on
Close or replaced on reopen. Retaining the COM object alone did not retain
the selected file's readable state. Both the Linux and historical SDK profiles
failed an uncached read after Close in the public ABI regression.

The original SDK stream/cache/decoder flow and original Linux C folio body
are retained. Each returned stream now owns a private instance of the same
handler with its selected source, profile, bounds and decoding state. The
historical whole-image buffer has shared immutable ownership, so independent
streams do not copy the image per file. The Linux profile holds its bounded
source directly and does not need the archive's metadata buffer to read data.
Allocation paths and Free use the same MidAlloc/MidFree pair through shared
ownership. The standalone overlay receives the same SDK fix; no sibling or
Linux source is changed. No second format reader or decoder is introduced.

The regression covers two streams, uncached reads after Close and reopen into
the other profile, release of the public handler/provider, source I/O failure
and E_ABORT with zero reported bytes, retry and independent seek/read. Linux
extended/raw and historical SDK Zlib controls exercise both ownership paths.
This is an archive ABI lifetime adaptation, not a claim that Linux offers this
archive interface or that a shared seekable source supports concurrent calls.
Source access remains read-only; no gate, consent, repair or replay changes.

Before the fix the new ABI rows both failed the first read after Close.
Afterwards `cramfs`, `cramfs_module`, `upstream_baseline` and
`upstream_inventory` passed on macOS, Linux and Windows. Linux/Windows receipt:
`20261022T050553Z-57a756bb`; test phases 2.05/27.68 seconds. The local test
phase took 2.60 seconds; the incremental application build also passed.
Both official engine overlays and the module were rebuilt incrementally with
the existing compiler caches. No full/slow suites, physical devices or native
mount acceptance are claimed; fixtures are RAM-only or temporary-owned and
removed by their existing owners. Diff check passed.

This iteration does not close the complete cramfs source/policy audit.
The optional-verification scenario above subsequently resolves the CRC opening
dependency. Contiguous metadata-prefix loading and directory-local errors
remain outside this lifetime fix and require useful-read comparison.

## Source comparison

2026-10-19, part of the [Linux format review](filesystems-linux.md).
Linux reference repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
Inventory unit: complete `fs/cramfs` (`inode.c`, `uncompress.c`, `internal.h`,
`README`, Makefile and Kconfig), plus `include/uapi/linux/cramfs_fs.h`.
[Reproducible source hashes](../cramfs-linux-baseline.json) cover these units.
The comparison covered feature acceptance, superblock placement, directory
entries, inode types, folio reads, direct/end pointers, holes and zlib boundaries.

7-Zip repository: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Complete
`CPP/7zip/Archive/CramfsHandler.cpp` bodies, registration consumers and
`Common/StreamObjects.cpp`'s cached stream were compared. The handler remains
the authoritative adapted source under its original path in `src/`.
Original zlib/Deflate dependencies were copied unchanged, with notices and
paths recorded in the [7-Zip baseline](../upstream-baseline.json).
Existing LZMA, copy, progress, property and stream helpers are reused.

## Integrated read scenario

The initial sparse fix (`b3c78fd`) retained the upstream cached stream and
zero-filled empty physical extents. Its generated process fixtures failed with
a data error before the fix and passed through both engines afterwards.
The subsequent integrated scenario adds the Linux extended-pointer read path,
superblocks at byte 512, metadata validation and application deployment.

`src/fs/cramfs/inode.c` retains the complete C body of Linux
`cramfs_read_folio`. The userspace services replace VFS/folio mapping, locking,
bounded block acquisition and decompression. Native word loads become bounded,
endian-aware loads. Input uses the original seekable engine stream; metadata
is buffered separately, so direct extents beyond the old 272 MiB whole-image
limit remain readable. The original 7-Zip zlib decoder supplies the kernel
decompression contract, including zero-filled short pages. Linux's zlib wrapper
is not duplicated. C remains C, with a narrow C ABI in `cramfsreader.h`.

Two narrow safety/correctness deviations from the pinned folio body are explicit:
previous-direct-extent additions reject 32-bit overflow, and an aligned final
uncompressed direct block retains its full page length. The pinned source caps
that final block to `offset_in_page(i_size)` even when zero, which loses a valid
full final page; the accompanying Linux format README requires a full page
unless the final page is partial. Tests cover both full and partial endings.

The existing 7-Zip directory walker, item publication and archive stream are
retained. Linux's VFS lookup/readdir/inode publication do not implement an archive
enumeration interface. Their disk contracts inform narrow adaptations: root
directory/offset and inode-type validation, bounded names, UID/GID and device
properties, UTF-8 paths, regular/symlink payload sizes, shifted roots, both
endiannesses, v1/v2 headers and rejection of unsupported feature bits.
Recognition uses the two endian magic values, as Linux does, without requiring
the informational text signature. Version-2 CRC covers the volume from its
superblock, excluding an optional leading prefix, and is checked incrementally.

Open/read/extract errors and cancellation propagate through the engine ABI;
failed opens clear partial state, and handlers can be reused. Cached streams
retain their owning handler and their own file metadata. Formatting changes
within adapted C++ functions are separate from these functional changes; untouched
Linux C control flow and dependency organization remain recognizable.

## Module and platform boundaries

The GPL C reader is linked into a separate `sevenzip_cramfs` format module,
following the existing exFAT deployment boundary. That module has no unRAR code.
The dynamic engine omits its built-in Cramfs registration and loads this module
from `Formats`; CMake deployment and Unix/Windows installation include it and
its license notice. The same authoritative handler retains the explicit legacy
LZMA/enlarged-block branches, whose flags are outside Linux's feature set.

Standalone `7zz` retains its legacy Cramfs reader, including the sparse fix;
modern Linux Cramfs support belongs to the application's dynamic engine and
format module. A dynamic runtime copied without the module lacks Cramfs support.
Runtime overlays/provenance include the adapted handler in both engine builds.
No sibling checkout, widget or archive transport is modified.

Kernel VFS mounting, block/MTD acquisition, mmap/XIP and page-cache services are
outside this read-only archive adapter. The copied read path uses 4096-byte pages;
native kernel page sizes are not encoded independently in standard cramfs images.
Explicit historical 7-Zip enlarged-block formats keep their existing reader.
This scenario does not establish completion of the other filesystem modules or
EROFS in the parent review.

## Validation

Deterministic fixtures check exact extracted bytes for sparse, ordinary, raw,
direct and mixed blocks, discontiguous/shared extents, both byte orders,
v1/v2, shifted roots and 512-byte prefixes. Full and partial final pages,
short zlib output, explicit legacy LZMA/8 KiB blocks, empty directories/files,
symlink payloads, nested Unicode names and ownership metadata are covered.
Malformed root/name/type/features, bad pointers, lengths, compressed data and
truncation must fail. Magic-only automatic recognition is exercised through
the production engine without forcing the format.

Module ABI tests exercise cancellation, source/output errors, reuse, random
seeks, stream lifetime and a virtual seekable image whose direct payload lies
at 1 GiB. An input failure after open verifies extraction error propagation.
ArchiveBackend discovers the deployed format as read-only, lists, tests and
extracts a Unicode/prefixed image without modifying it. Temporary runtime copies,
images and output directories are removed by fixture ownership.

Local macOS arm64/Qt 6.12 focused checks passed for `cramfs`, `cramfs_module`,
`runtime_cache`, `upstream_baseline`, `upstream_inventory`, `developer_workflow`,
`exfat`, `exfat_backend`, `brotli`, `external_codecs` and `codec_probe`.
The source baseline verifies against the read-only sibling. Linux/Windows
incremental runtime build receipt: `20261019T063013Z-0e163113`.
All eleven focused suites passed on Linux x64; ten passed on Windows x64/MSVC
in receipt `20261019T063038Z-1cfda2d6`. Windows's new backend discovery case
hit its production deadline during that concurrent batch; its unchanged,
focused rebuild/test passed in `20261019T063232Z-8a90ac73`. The initial timeout's
cause is unconfirmed. The final local module rebuild/test also passed.

A supplementary native-kernel mount comparison was unavailable: the configured
Linux guest's `6.8.0-146-generic` kernel has no Cramfs filesystem driver. All
read-only loop-mount attempts and temporary fixtures were cleaned. This is not
kernel runtime acceptance; source comparison and module behavior are separate
evidence. No full, slow, physical-device, package or UI acceptance is claimed.
Incremental build trees and compiler caches are retained.
