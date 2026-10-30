# Read-only XFS — 2026-10-24

Scope: browsing and copying files from XFS server/NAS filesystem images and
the existing read-only device streams. This is a new native format module;
the official 7-Zip baseline has no XFS handler. No source filesystem writes,
mounts, journal replay, repair or independent archive engine are introduced.

## Source selection and reproducible baselines

The C++23 GRUB XFS facade adds RAII volume ownership, borrowed span-based source
services, synchronous directory visitors and `std::expected` results. Read errors
retain their processed-byte count. The mounted volume borrows its archive reader
until it closes; names and directory entries are borrowed during each visit.
C++ source/progress exceptions become transport errors before returning to C,
allowing GRUB's normal error cleanup to run. The original C mount/directory/read
bodies, filesystem validation and source-language boundary are unchanged.
Existing XFS module/backend scenarios cover read results, damaged metadata,
cancellation, transport failure and retained streams.

The selected implementation is GNU GRUB's C read-only XFS driver:
`https://git.savannah.gnu.org/git/grub.git`, commit
`b07cc37ca64fe99260884e7325abc333374e098b`.
[The manifest](../xfs-grub-baseline.json) pins SHA256 of complete original
inventory units and maps adaptations to local owners. The retained
[GPL-3.0-or-later license](../../third_party/grub/COPYING) and original FSF
notices accompany the separate module.

Before editing, complete XFS structures and bodies, the fshelp reader, mount,
directory/open/read/symlink callers, memory/error services and registration were
compared. GRUB's data path is usable without its boot environment; Linux's
full `fs/xfs` driver requires VFS, iomap, buffers, transaction infrastructure
and many libxfs units. Linux remains the format/validation reference, rather
than a dependency copied into this GPLv3 module. The reviewed Linux snapshot
is `58161ead29e50d70a317ea26b472e0c7508e69da`, repository
`https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git`:
`fs/xfs/Makefile`, `libxfs/xfs_format.h`, `xfs_da_format.h`, `xfs_sb.c`,
`xfs_inode_buf.c`, `xfs_bmap_btree.c`, `xfs_cksum.h`, and the directory readers.
Linux GPL-2.0-only C bodies are not imported into GRUB's GPLv3 combination.

Also inspected: `https://github.com/libyal/libfsxfs.git`, commit
`cf474697de8a32f7eba4122e469b2ecd818054c6` (LGPL-3.0-or-later), including
its build dependencies, superblock checks, inode/data-stream readers and
feature restrictions. It requires the libyal library family and this revision
rejects META_UUID, among other flags; it is not imported. Search snippets
describing older libfsxfs releases were not used as the feature contract.

CRC32C reuses 7-Zip C++ bodies from `CPP/7zip/Archive/VhdxHandler.cpp`,
commit `b88c6bc09339de32f7d18bbab900686b244f42ba`, repository
`https://github.com/ip7z/7zip.git`, under LGPL-2.1-or-later.
[Its separate manifest](../xfs-sevenzip-baseline.json) pins the complete
original file and maps selected `Crc32c_GenerateTable` and
`CrcUpdateT1_vhdx` bodies to the shared `VhdxCrc.cpp` extraction. The complete
original VHDX unit and license are byte-identical to the former
`9128b80e3a471108678e2b3ec3c985861c8d7b0f` baseline. The immutable table now has
synchronized first-use initialization. The XFS wrapper still temporarily zeros
and restores its caller-owned checksum field. It does not link the VHDX reader
or Linux GPLv2 code. The [shared checksum audit](shared-checksums.md#shared-crc32c-service)
records the common native/GPL provider, source mapping and validation.

## Transferred scope, dependencies and adaptations

- `grub-core/fs/xfs.c` → `src/dependencies/grub/include/grub/fs/xfs.c`: superblock/version predicates,
  inode addressing and loading, extent decoding, bmap btree descent, inline
  and block/data directory iteration, symlink reads, inode timestamp helpers
  and file block mapping. Legacy signed seconds are retained as signed values.
  The XFS structures, upstream symbols and read control flow are retained.
- `grub-core/fs/fshelp.c` → `src/dependencies/grub/include/grub/fs/fshelp.c`: selected
  `grub_fshelp_read_file`, including sparse zero filling. Bootloader pathname
  lookup, symlink-following traversal and its stack are omitted: the existing
  archive layer owns paths and extraction safety.
- Selected types, fshelp callback declarations and checked-arithmetic contract
  from `include/grub/{types,fshelp,safemath}.h` → `xfs_compat.h`. Standard C
  types, packing, endian helpers, bounded source reads and per-reader errors
  replace GRUB CPU configuration, disk cache, allocator/error globals and
  bootloader logging. No GRUB disk writes or boot platform are linked.
- Local integration: `xfsreader.h`/`xfsreader.c` and the inode API appended to
  the C reader, `XfsArchiveReader`, 7-Zip archive/stream presentation, CMake,
  runtime deployment and device format selection. Byte sources are read-only;
  retained file streams keep their reader/module owner after handler Close.
  Reader state is serialized per source and progress callbacks carry cancellation
  and source failures through the C boundary.
- Existing LGPL 7-Zip archive exports, streams, properties, Unicode/time,
  allocation and ABI initialization are reused. `exfatmodule.cpp` retains its
  GPL-2.0-or-later terms and is usable under GPLv3 in this module. There is no
  unRAR or GPL-2.0-only Linux object in the XFS target.

Source comparison identified corrections needed before using GRUB on untrusted
images: geometry/shift bounds; inode and data-fork bounds; 54-bit file offsets;
unwritten extent zero filling; btree record/pointer bounds and descending
levels; bounded inline/free/name records; block/data-directory magic instead
of extent-count inference; failure propagation instead of clearing unreadable
inodes. Inline directory bounds use the declared data length, not inode padding.
XFS v5 metadata checksums and metadata UUID/owner checks supplement
GRUB's layout recognition. Source limits stay within the selected stream and
volume, including when the input is a partition or nested disk image.
These validations are narrow host/metadata boundaries around the transferred
reader; they do not replace extent/directory algorithms with a new filesystem
implementation. A second comparison after adaptation accounts for these
changes separately from omitted bootloader callers. GRUB C remains C11;
the reused checksum and local archive integration remain C++17.

## Behavior and explicit boundaries

The reader exposes XFS v4/v5 directories and regular file data, inline links,
single-block v5 remote links, sparse/unwritten ranges, extent/bmap files,
bigtime and large extent counts. Read-only device selection includes XFS;
normal archive discovery, Test, Copy/Extract and retained nested streams use
the native module. Installation includes the module and dependency notices.

This does not implement the complete Linux filesystem driver. External realtime
device data and multi-block v5 remote symlinks are unsupported and return a
specific unsupported-method result; headers cannot be silently emitted as link
data. Extended attributes/ACL streams, quotas, realtime metadata, journal
inspection/replay, repair and damaged-volume recovery are outside this change.
Unknown incompatible formats are rejected. Listing does not accept an
incomplete tree as success. Regular data has no XFS per-file content checksum;
metadata validation does not establish authenticity or consistency of a live
or improperly unmounted source. Use offline images or consistent snapshots.
Physical NAS RAID/LVM assembly is not part of an XFS filesystem reader.

The current text-name adapter accepts UTF-8 names and rejects unsafe path
components; arbitrary non-UTF-8 XFS byte names are not yet exposed. Directory
cycles and excessive traversal depth are rejected. These boundaries are not
a claim that all Linux-readable XFS features have been transferred.

## Baseline verification and update review

Use a separate GRUB checkout containing the pinned and proposed commits:

```sh
git clone https://git.savannah.gnu.org/git/grub.git /tmp/xfs-grub-review
python3 scripts/upstream_baseline.py reference-verify \
  --manifest docs/xfs-grub-baseline.json --upstream /tmp/xfs-grub-review
python3 scripts/upstream_baseline.py reference-review \
  --manifest docs/xfs-grub-baseline.json --upstream /tmp/xfs-grub-review \
  --new-revision <full-commit> --output /tmp/xfs-grub-review.md
```

The same commands accept `docs/xfs-sevenzip-baseline.json` with a 7-Zip
checkout. They verify original bytes and compare complete original units,
the proposed revision and current local adaptations; safe local refactoring
does not change original hashes. Review dependencies and omitted scope, retain
the bounds/checksum/error fixes, and validate before advancing either baseline.
Do not replace the selected fshelp body with its entire bootloader module.

## Validation

2026-10-24: both baseline verifications and adaptation reports PASS. macOS arm64,
Qt 6.12: incremental module/backend builds and focused `xfs_module` /
`xfs_backend` PASS. The fixtures come from official xfsprogs 6.6.0 `mkfs.xfs`
without mounting; [fixture reproduction](../../tests/fixtures/xfs/README.md)
records the generator. Checks cover file contents, Unicode/spaces, large
directories, symlink metadata, independent retained streams, cancellation,
source/output failures, malformed metadata/names, unwritten ranges, distant
file offsets and multilevel bmap files. Security regressions reject bytes beyond
the declared filesystem even when they exist in the backing stream, and reject
inline records beyond the declared directory length. Backend checks exercise discovery, Test, extraction
and aligned read-only device streams.

The final focused suites also PASS on Linux x64/GCC and Windows x64/MSVC,
both with Qt 6.12. Runner receipt: `20261024T094026Z-1978e45e`.
macOS application build PASS; local final focused runtime is 1.00 s.
The earlier receipt `20261024T092728Z-ac83a845` covers the first integrated
reader; `20261024T093701Z-4b993f83` covers the additional long-link, sparse
fixture and adjacent-data checks. The final receipt additionally includes
logical inline-directory bounds. The malformed xfsprogs prototype sample was
diagnosed from `mkfs/proto.c::newfile` in xfsprogs v6.6.0 and Linux remote-link
verification; validation was retained and the successful sample was corrected.

Incremental build trees and compiler caches were retained. Disposable fixtures
are removed by the test harness; resolved diagnostic images and generator outputs
were cleaned separately. Physical devices, native permission flows, manual UI
acceptance, full/slow suites and release installation were not run. Successful
source/baseline checks establish the selected transfer, not a complete Linux
XFS implementation; the omitted scope above remains explicit.

Integration, 2026-10-24: rebased onto master `f56c7a56` with ZIP/Zstandard,
Lzip, LZ4 Frame and F2FS changes retained. Conflict resolution preserves both
F2FS and XFS device candidates and their documentation. The XFS reader bodies
are unchanged. Incremental macOS builds and focused `xfs_module`, `xfs_backend`,
`f2fs_module`, `f2fs_native` all PASS (2.63 s). Linux/Windows evidence above
belongs to the original reader validation; those platforms were not rerun for
this documentation/device-registry integration.
