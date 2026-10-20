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
identity, symlink targets and public xattr/ACL values as described below.
Host ACL/LSM enforcement, VFS mounting, mmap/DAX and page-cache sharing services
remain outside this read-only archive API.

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

## Public xattr and ACL values — 2026-10-20

The initial implementation accounted for xattr storage when locating inode data
but omitted value reads. This iteration connects the pinned Linux `xattr.c`
initialization, inline/shared iteration, name/value copying, list/get operations,
handler map, long-prefix loading/cleanup and ACL getter. Complete bodies and
their `super.c`, inode, metadata-buffer and VFS callers were compared before
integration and again afterward. The selected function bodies remain identical;
only private visibility, host signed-size types, includes and VFS service types
are adapted. The source inventory now also pins the original Linux `xxh32`
implementation and POSIX ACL dependencies.

The Linux name-filter hash uses the original `lib/xxhash.c` round and one-shot
32-bit bodies. Streaming/64-bit hash interfaces are not needed; unaligned loads
are a narrow byte-order service. ACL conversion reuses the authoritative
`fs/posix_acl.c` serialization from ext, adding its original backing-store parser
and common value checks. Numeric namespace/allocation services are shared without
depending on ext4 disk/inode types. EROFS ACL values follow the VFS representation,
including named users/groups and validation of tags, version, size and IDs.
The ext consumers remain covered because their private ACL services moved to
the common header; their filesystem reader and ACL decoder are unchanged.
The original POSIX ACL UAPI headers retain their notices and paths; disk words
use the equivalent fixed-width host types. Unix/Windows installation includes
the LGPL text and Linux-syscall-note alongside the existing module notices.

Long prefixes are read from physical, packed or metabox metadata using the
original loader branches. Inline and shared xattrs can span filesystem blocks;
shared entries in metabox use the original feature selection. The name filter
honors its reserved byte. Immutable inode initialization and owned prefix/shared
arrays replace VFS allocation, locking and lifetime services. The no-ACL cache
hint and inode fingerprint/page-cache sharing are kernel cache services and are
omitted; they do not substitute for public value reads. Namespace handlers use
user/trusted/security/ACL metadata with privileged image-reader policy, without
host mount-option, capability or LSM filtering.

The C archive boundary calls the Linux list/get/ACL operations. A small separate
C++ value adapter owns bytes, escapes unsafe name bytes and disambiguates case
collisions. Existing 7-Zip buffer streams expose those values after archive
Close/reopen/Release. The handler publishes per-name metadata on files, directories
and hardlinks, plus a collision-free virtual root item. It provides alternate-
stream parent/type/name properties, independent inode/stream identities and
readonly file metadata. Archive projection has no usable kernel implementation;
it remains in the existing archive owner with the value adapter separated.
Extraction reports value bytes in progress and honors the final cancellation
callback; it does not apply stored xattrs or ACLs to the host filesystem.
The SDK's complete `CArc::OpenStream`/item-property callers also require the
archive-level alternate-stream marker before consulting individual items.
Both EROFS and ext now provide it, following the original HFS/NTFS contract.
CLI regressions check that `-sns-` excludes metadata while preserving an ordinary
ext filename containing a colon, and `-sns` exposes exact metadata values.

Six additional fixtures are produced by the same pinned erofs-utils. PAX input
supplies binary values, trusted/security namespaces and access/default ACLs
without host xattr dependencies. The creator's TAR path stores values inline
even with `-x0`; directory-input fixtures therefore separately exercise actual
shared entries, including shared metadata in metabox. Long-prefix variants cover
plain and packed locations and metabox. Values include empty/binary/Unicode and
unsafe names, case collisions, 9000-byte data and named user/group ACL entries.
A real `[ROOT]` filename checks virtual-root disambiguation. Fixture generation
uses disposable directories and records command inputs and hashes in the manifest.

CLI regressions compare every published value and preserve source images.
Mutations cover unsupported header-only bodies, inline bounds, shared counts/
IDs, ACL version/ID errors and the reserved-filter compatibility branch. ABI
checks cover owned empty/large streams, parent/type, shared-value read errors,
cancellation after actual metadata transport, output failure and reuse. A shared
metadata relocation keeps source failures distinct from earlier superblock and
inode reads. The ArchiveBackend consumer verifies listing and selected extraction
of large values, access/default ACLs, directory and root metadata. Tests check
observable bytes, errors and progress rather than source text or call counts.

Nine focused suites (`erofs`, `erofs_module`, `ext`, `ext_stream`,
`ext_xattr_backend`, `codec_probe`, `runtime_cache`, `upstream_baseline`,
`upstream_inventory`) passed on macOS arm64/Qt 6.12 (7.05 s), Linux x64
(17.84 s) and Windows x64/MSVC/Qt 6.12 (42.83 s), receipt
`20261020T181120Z-de95b22f`. After connecting archive-level stream filtering
and guarding UCRT's existing `min`/`max` macros, the seven direct format/source
suites passed again on all three platforms: macOS 3.80 s, Linux 12.48 s and
Windows 29.78 s, receipt `20261020T181619Z-ebaf5053`. The final Windows build
has no C4005 redefinition warnings; timeout and cache settings were not changed.

Native Linux `6.8.0-147-generic` matched all 93 xattr reads over four read-only
images: inline, shared, plain-prefix and packed-prefix layouts, receipt
`20261020T180830Z-572c4d70`. All image hashes stayed unchanged. New metabox
layouts are validated through the pinned-source module on three platforms,
not through that older native kernel. Temporary mounts/images and the review
script were removed locally and from the guest worktrees.

A real local macOS installation of the final runtime read nine exact metadata
values across inline, packed-prefix and shared-metabox images and honored
`-sns-` filtering. Installed LGPL/syscall notice bytes matched the retained
originals. The disposable installation and its log were removed. Incremental
build trees and compiler caches remain intact. No full/slow, release archive,
native authorization, physical-device or UI appearance gate is claimed.
