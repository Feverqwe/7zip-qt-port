# Read-only EROFS from Linux

## Reference and ownership

Part of the [Linux format review](filesystems-linux.md), 2026-10-19.
Linux repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
[Source hashes](../erofs-linux-baseline.json) record the complete `fs/erofs`
inventory and copied CRC dependencies. The official 7-Zip archive sources and
`Bundles/Format7zF` at `9128b80e3a471108678e2b3ec3c985861c8d7b0f` have no EROFS
handler. A new stream/archive boundary is therefore needed; filesystem mapping
and decompression algorithms must come from usable upstream C implementations.

The integrated module connects the transferred read-side code, decoders, archive
ABI and application consumers. Source comparison covers the transferred mapping
branches; runtime evidence below covers the tested producer layouts. Other
filesystem/format rows in the parent review remain separate work.

## Transferred scope

- `fs/erofs/erofs_fs.h` is unchanged. Disk-layout assertions remain enabled.
- `inode.c` retains `erofs_fill_symlink` and `erofs_read_inode`; VFS inode
  publication, mmap/DAX, ioctl and mount registration are omitted. An explicit
  check rejects signed overflow in compact inode epoch/mtime addition.
- `data.c` retains `erofs_map_chunks` and `erofs_map_blocks`. Raw, inline,
  chunk/index arrays, sparse chunks and 48-bit block fields keep their C paths.
- `zmap.c` retains all format-mapping bodies through
  `z_erofs_map_blocks_iter`, including full/compact indexes, big pclusters,
  alternate algorithms, inline tails, fragments, partial references and extents.
  The iomap reporting wrapper is omitted. A guard rejects logical cluster shifts
  that would reach the signed `1 << 31` expressions in the original bodies.
  The initial logical-cluster number uses `u64` to preserve Linux's 64-bit
  `unsigned long` behavior on Windows's LLP64 ABI.
- `dir.c` retains `erofs_fill_dentries`; the adapter provides seekable block
  acquisition and an enumeration callback in place of VFS/page-cache readdir.
- `super.c` retains checksum verification, variable metadata acquisition and
  superblock parsing. Device slots and unified/external-device addresses are
  adapted to bounded streams; device opening is delegated to a callback.
- Decompression configuration validation and plain shifted/interlaced transforms
  retain their original module paths. Stream-pool allocation, cache/overlap
  scheduling and kernel acceleration services are omitted. Compressed extents
  call the original decoder implementations through bounded-buffer adapters.
- `lib/crc/crc32-main.c` retains the generic Linux CRC implementations. The
  original generator produces the checked-in tables; only its unused generated
  kernel-configuration include and polynomial-header path are adapted.
  `include/linux/crc32poly.h` is copied unchanged.

The userspace header provides inode/superblock/buffer services and preserves
relevant original field names. C stays C11. Original licensing and copyright
notices are retained. Metadata pages use owned buffers with a 64 KiB ceiling,
independent of host page size, while filesystem geometry follows the superblock.
The archive adapter sets filesystem block geometry during superblock parsing
so compression-config limits use filesystem blocks. This replaces the kernel's
later block-device geometry update. A seekable source establishes volume bounds
and supports cancellation, input failures and bounded fragment/metabox recursion.
Flat extent arithmetic is checked before a forged 48-bit start address can wrap
into the beginning of the source stream. Copied C bodies keep Linux formatting;
new C++ integration is formatted separately.

## Decoders and archive integration

Linux `lib/lz4` decoder bodies retain their original C paths, with unaligned word
loads, bit operations, builtins and host headers adapted narrowly. XZ Embedded
`lib/xz/xz_dec_lzma2.c`, its two private headers and public `include/linux/xz.h`
are unchanged; the supported userspace configuration boundary selects single-call
MicroLZMA. The existing 7-Zip Deflate and Zstd implementations are reused.
Zstd uses caller-owned bounded buffers, checks the configured dictionary limit
and preserves partial-reference decoding without trusting a frame's requested
allocation size. No independent compression algorithm is introduced.

The new GPL `sevenzip_erofs` module implements the original 7-Zip archive and
seekable-stream ABIs. It presents file types, ownership, timestamps, inode/link
identity, symlinks and device numbers. Existing local exFAT archive-boundary
patterns and upstream Ext property conventions are reused. Per-open reader state
is shared with returned streams, so closing/reopening the archive cannot retarget
an existing stream's source. Auxiliary volumes use the engine's volume callback;
unified images fall back to primary-stream device addressing. Unsafe auxiliary
names are not opened as host paths. Directory cycles, duplicate names and unsafe
archive path components are rejected at the archive boundary. Text conversion
uses UTF-8 with the existing 7-Zip fallback for invalid byte sequences.

CMake deployment and Unix/Windows install rules now include the format module
and notices. This extension uses the application's dynamic engine; standalone
`7zz` has no built-in EROFS handler. The module contains no unRAR implementation
and has no external compression-library runtime dependency.

The archive contract exposes contents, modes, UID/GID, timestamps, inode/link
identity and symlink targets. Linux xattr values, ACL/LSM policy, VFS mounting,
mmap/DAX and cache-sharing services are outside this archive API. Like the
upstream 7-Zip Ext handler, it has no generic Linux xattr publication interface.
On-disk inline/shared/long-prefix xattrs remain compatible with file reading;
their size participates in locating subsequent inode data. A creator fixture
with those attributes checks that boundary. This does not claim xattr export.

## Behavior validation

Official erofs-utils 1.9.4 at
`f36cadb5c563995ab3aa8572a60ed6b721b9557d`, source repository
<https://git.kernel.org/pub/scm/linux/kernel/git/xiang/erofs-utils.git>, generated
the fixtures in `tests/fixtures/erofs`. Their generator and manifest record
options and image hashes; original contents are deterministic local test data.
The tool was built from unchanged source bodies with a local CMake/configuration
adapter matching its Automake source inventory and available host libraries.
No system packages or installed tools were changed.

Production `7z` automatically recognized all 25 images and listed ownership
and symlink properties, then extracted exact file bytes without changing the
images. Coverage includes all four algorithms, full/compact indexes, big
pclusters, fragments, inline compressed tails, metabox, compressed directories,
chunk maps/indexes, sparse holes and 48-bit layout, plus 512-byte and 64 KiB
filesystem blocks, subpage compression and shared/long-prefix xattrs. Auxiliary
device images test both separate named blobs and a unified primary stream.
The generator records its device-tag/checksum adaptation for the named blob.

The process suite rejects bad checksum/features/geometry, root types, negative
sizes, timestamp overflow, path traversal, directory cycles, truncation and data
extents outside the image. Corrupt LZ4, MicroLZMA, Deflate and Zstd blocks must
report an error through the production engine, with the source unchanged.

ABI tests check open/extract/device cancellation, input/output failures and reuse,
missing auxiliary streams, flat-extent overflow and cleanup of a partial tree.
A seekable stream opened at a nonzero source offset retains its original reader
after archive Close/reopen/Release, including backward/cross-cluster/tail reads.
ArchiveBackend discovers EROFS as read-only, lists and tests a metabox image and
extracts the selected Unicode file with exact contents and source preservation.

Focused suites `erofs`, `erofs_module`, `cramfs`, `cramfs_module`, `exfat`,
`exfat_backend`, `brotli`, `external_codecs`, `codec_probe`, `runtime_cache`,
`upstream_baseline`, `upstream_inventory` and `developer_workflow` passed on
macOS arm64/Qt 6.12, Linux x64 and Windows x64/MSVC/Qt 6.12. Guest receipt:
`20261019T072532Z-e4626bd2`. Subsequent localized EROFS checks covered the added
geometry, sparse/xattr fixtures and narrow safety changes. Both EROFS suites
passed locally after the last source change and on Linux/Windows in final receipt
`20261019T073629Z-a49a6cc9`. The 7-Zip baseline and exact Linux source hashes
verify locally.
An earlier Windows process timeout did not recur in focused reruns; its cause
remains unconfirmed. No production timeout was increased.

Supplementary native-driver comparison on Linux `6.8.0-146-generic` read the
plain and LZ4 images with matching data, UID/GID and symlink target. That guest's
EROFS configuration rejected MicroLZMA, so this is a limited older-kernel oracle,
not runtime validation of the pinned 2026 kernel. Temporary read-only loop mounts
were unmounted and removed. The pinned source remains the reference for modern
layouts. No full, slow, package/clean-OS, UI or physical-device acceptance is
claimed. Incremental build trees and compiler caches were preserved; disposable
images, private runtimes and extraction directories were removed by their owners.
