# Btrfs: read-only Linux source port

## Source and current status — 2026-10-24

This feature is on `feature/btrfs-readonly`. The separate native Btrfs module
and C read-side library pass Linux-created image, ArchiveBackend, device-stream
and multi-device checks on macOS, Linux and Windows. Application startup and
the installed native module also pass on all three platforms. Evidence and
validation limits are recorded below.

Linux repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`. The
[baseline](../btrfs-linux-baseline.json) pins complete original inventory units
from `fs/btrfs`, the public disk structures and their dependencies, with a map
to adapted owners. Hashes describe original bytes, independently of local
formatting or refactoring. The sibling 7-Zip checkout is unchanged.

Use the existing [reference verification and review procedure](../upstream-update.md#external-reference-baselines):

```sh
python3 scripts/upstream_baseline.py reference-verify \
  --manifest docs/btrfs-linux-baseline.json --upstream /path/to/linux-git
python3 scripts/upstream_baseline.py reference-review \
  --manifest docs/btrfs-linux-baseline.json --upstream /path/to/linux-git \
  --new-revision <full-commit> --output /tmp/btrfs-review.md
```

Review complete units, dependent callers and omitted branches before advancing
the revision. A whole Linux module must not overwrite these partial adaptations.
The original snapshot is available in the shared harness investigation cache.
Independent verification against a sparse Git checkout passed for all 142
original units at the pinned commit. A same-revision `reference-review` records
current local differences; it does not claim comparison with a newer revision.

## Transferred code and adaptations

- `include/uapi/linux/btrfs_tree.h` → the corresponding `src/include/uapi/linux`
  header: complete disk-format definitions. Kernel includes are replaced by the
  private scalar bridge; packed attributes use a macro and MSVC packing scope.
  Flexible-array representation follows the UAPI convention, with its zero-size
  array spelling also used for MSVC. No ioctl interface or kernel API is exposed.
- `include/uapi/linux/btrfs.h` → `src/formats/btrfs/compat.h`: public UUID/label sizes
  feature flags and `enum btrfs_dev_stat_values`, needed by the complete tree ABI header.
  `include/uapi/linux/stddef.h::__DECLARE_FLEX_ARRAY` supplies the C wrapper and
  C++ zero-size spelling. Other bridge services are local: fixed-width types,
  allocation, errors and unaligned little-endian byte access.
- `fs/btrfs/ctree.c` → `src/formats/btrfs/ctree.c`: complete comparison bodies,
  binary-search algorithm, node-slot read, previous-leaf and neighbouring-item
  traversal. Folio mapping in binary search becomes a contiguous-buffer read;
  kernel logging/assertion becomes returned errors. Path allocation/release
  retains upstream ownership, using `calloc/free` without kernel locks.
  The plain-search branch of `btrfs_search_slot` and the ancestor/descendant walk
  of `btrfs_next_old_leaf(time_seq=0)` retain the upstream algorithms; COW,
  insert/delete, transaction setup, locking and concurrency retries are omitted.
  Nonzero write parameters are rejected. Input buffers have no local writers;
  this does not establish consistency of a live device being changed externally.
- `fs/btrfs/ctree.h`, `extent_io.h`, `disk-io.h` → `src/formats/btrfs/ctree.h`:
  selected path/root/extent-buffer/parent-check types and read-side declarations.
  Folios and kernel state are replaced by a private contiguous byte buffer and
  a borrowed filesystem context. Parent checks retain generation, owner, level
  and the first key.
- `fs/btrfs/accessors.h` → `src/formats/btrfs/accessors.h`: selected header, node,
  item/key, superblock, chunk, inode, root and extent read accessors. Buffer offsets use `size_t` to avoid the Win64
  `unsigned long` pointer-width mismatch. Disk structure layout is unchanged.
- `fs/btrfs/volumes.c` and `volumes.h` → their `src/fs/btrfs` owners:
  RAID geometry table (read fields), chunk-map comparison/search, system-array
  bootstrap, chunk-tree iteration and read-side address mapping. SINGLE, DUP,
  RAID0/1/1C3/1C4/10 and direct RAID5/6 data mapping retain the Linux algorithms;
  single-data-stripe RAID56 retains `set_real_chunk_type`. PID/live-device
  selection becomes explicit mirror numbers. There is no parity reconstruction,
  device replacement or kernel bio allocation. The existing adapted Linux RB-tree
  dependency is reused, with volume lifetime replacing map refcounts and locks.
  Duplicate system/chunk-tree mappings must agree; additions/physical addresses
  are checked for overflow. The last node pointer is validated as well.
- `fs/btrfs/disk-io.c` → `src/formats/btrfs/disk-io.c`: complete superblock and system
  array validation flow, with kernel diagnostics omitted, owned UUID state and
  host-independent 4–64 KiB buffer support. Superblock checksum and tree block
  validation use a required checksum callback, with unknown/missing algorithms
  rejected. Tree reads validate checksum, UUID, address, generation, owner,
  level and first key; failed mirrors can be read without repairing the source.
- `fs/btrfs/tree-checker.c` → `src/formats/btrfs/tree-checker.c`: complete common
  chunk validator and stripe-count checks; structural leaf/node ordering,
  bounds/overlap/packing checks and shared-subvolume ownership semantics.
  Item-specific validation is not all transferred: the consuming readers must
  validate variable-length bodies before interpreting them. Invalid profile
  bits are rejected before indexing the RAID geometry table.
- `src/formats/btrfs/btrfsreader.c`, `btrfsreader.h`: local C stream/lifetime adapter around
  the Linux super → system chunks → chunk tree → root tree sequence. Its ABI
  provides only reading, device lookup, polling, checksum and decompression;
  it has no source write operation. The checksum-size table is selected from
  `fs/btrfs/fs.c::btrfs_csums`. Public file identity includes subvolume and inode.
  File APIs now connect to inode/root/extent readers, native archive consumers
  and the concrete decompression boundary.
- `disk-io.c::btrfs_read_tree_root/read_tree_root_path` and the live-reference
  branch of `btrfs_get_root_ref` → `src/formats/btrfs/disk-io.c`: root-item lookup,
  checksum/generation/level validation and exact root-node owner validation.
  Kernel root allocation/cache becomes caller-owned compact state; traversal
  paths own their buffers. The generic descendant-owner rule still permits
  shared subvolume blocks, while the root-node owner must match exactly.
- `root-tree.c`, `inode-item.c`, `file-item.c`, `inode.c` → the corresponding
  C owners under `src/fs/btrfs`: complete inode/file-extent lookup bodies,
  root lookup and legacy root-item handling, inode metadata loading, directory
  index enumeration and the subvolume ROOT_REF checks. Root loading omits VFS
  caching and orphan cleanup; it does not synthesize a UUID for a legacy root.
  Names enter a subvolume only through a matching parent root/directory/name
  reference. Timestamps, modes, UID/GID, link count, flags and device IDs remain
  read metadata. Linux mode constants are selected from `include/uapi/linux/stat.h`.
- `inode.c::btrfs_get_extent`, `file-item.c::btrfs_extent_item_to_extent_map`,
  `btrfs_file_extent_end` and `extent_map.h` retain the extent lookup,
  backtrack/forward traversal, holes, inline, preallocation, compression and
  shared-extent offsets. Kernel extent-map caching becomes a caller-owned map
  and held path. `src/formats/btrfs/btrfsfilereader.c` supplies byte-buffer and file-lifetime
  services, keeping volume lifetime separate. Source decoding is bounded to
  Linux's compressed-extent sizes and validated before copying a shared slice.
- `file-item.c::btrfs_lookup_csum` plus the read-side checksum path supply data
  verification before byte delivery or decompression. The stream service reuses
  a held checksum leaf with the correct per-sector offset, checks the returned
  checksum range, and reports bytes delivered before a later error. NODATASUM
  inodes retain their on-disk semantics; they are not silently called verified.
- `tree-checker.c`: the consumed inode/root/directory/xattr/extent/checksum
  validators, their key checks, previous-inode and extent-overlap helpers are
  now connected after structural bounds checks. Diagnostic IO and write-mount
  restrictions are omitted, with read-only inode ro-compat handling retained.
  GNU statement-expression and void-pointer arithmetic spellings become portable
  C expressions; variable-length fields are checked before their consumers.
- `fs.c::btrfs_csum` keeps Linux's CRC32c/XXH64/SHA-256/BLAKE2b-256 dispatch.
  Existing Linux CRC32c and exact 7-Zip XXH64/SHA-256 primitives are reused;
  SHA-256 selects the software implementation without mutable CPU-init state.
  New `lib/crypto/blake2b.c` and `include/crypto/blake2b.h` transfer the complete
  generic Linux BLAKE2b path and API (GPL-2.0 OR MIT). Architecture dispatch and
  module registration are omitted. Local endian/rotation/explicit-clear services
  replace kernel primitives, and 64-bit IV enum members become macros for MSVC.
  Complete original units and their owner map are in the Btrfs baseline.
- `compression.c`, `zlib.c`, `lzo.c`, `zstd.c` → their `src/fs/btrfs` owners:
  read-side dispatch and inline/bio decompression flows with bounded contiguous
  buffers replacing kernel workspaces/folios. The callback carries filesystem
  sectorsize and inline-vs-regular framing. LZO retains total/segment lengths,
  sector-boundary header padding, safe decoding and early-output checks.
  zlib retains Linux's non-dictionary wrapper handling through the shared official
  zlib API. Zstandard retains the Btrfs 128 KiB bounds and first-frame
  behavior, using the shared 7-Zip API adapter to official libzstd; sector padding does not
  become another frame. Regular extents retain Linux's successful-decode zero
  filling of sector padding; inline extents retain their required output length.
  Existing LZO dependencies and their license/provenance owners are reused from
  the SquashFS/Linux-frame baselines; official zlib is owned by
  `docs/zisofs-zlib-baseline.json`, with its license shipped by the ISO package.
  The shared portable LZO
  header now includes its `size_t` declaration instead of depending on include order.
- `xattr.c::btrfs_listxattr/getxattr` → `src/formats/btrfs/xattr.c`: the read-only
  XATTR_ITEM walk and packed collision-chain name/data layout emit owned native
  attribute values through a callback; no xattr mutation code is transferred.
- Native presentation reuses the project's existing EROFS archive integration,
  with Btrfs-specific file identity, device validation, errors and properties.
  `btrfshandler.cpp` owns tree/attribute enumeration and extraction;
  `btrfsarchiveproperties.cpp` owns property projection; `btrfsarchivereader.cpp`
  owns transport/callback/device services. Shared seek/read and transport state
  are serialized for retained streams. Files retain the volume after handler
  Close/reopen; the native inode/stream identity pair includes the subvolume.
  Native extraction distinguishes checksum and unsupported-method results from
  generic data errors, preserving cancellation and allocation failures.
  Physical size describes the selected device, not aggregated filesystem size.
- The upstream `CArcInfo` fixed signature offset is 16 bits. The unchanged
  `IsArc` facility probes Btrfs's magic beyond 64 KiB, avoiding an ABI change or
  truncated signature offset. The separate GPL format module is deployed and
  installed beside other filesystem providers, with its notices; it does not
  enter the SDK-only runtime overlay. `archivedevice.cpp` includes Btrfs among
  filesystem-only readers, leaving archive operations behind the native backend.

Companion image lookup uses `devid-<id>.img` during opening, validating superblock
checksums, device ID/UUID and filesystem/metadata UUID against the primary image.
References are resolved while the opening callback is alive; retained streams
do not retain a potentially stale opening callback. Missing peers fail when
needed data cannot be read from another usable mirror. This is no host RAID
assembly or automatic raw-device scan. Linux-created multi-device samples
validate the image workflow, including missing and substituted members.

Shared dependencies keep their existing authoritative owners: RB-tree/CRC32c
in the [ext baseline](../ext-linux-baseline.json), LZO/inflate in the
[SquashFS](../squashfs-linux-baseline.json), [ISO](../isofs-linux-baseline.json)
and [Linux frame](../linux-frames-baseline.json) baselines. Their Linux revision
matches this port. The exact 7-Zip SHA-256/XXH64 and ABI dependencies use
the separate full revision in the [7-Zip baseline](../upstream-baseline.json).
The shared Zstd API adapter and libzstd implementation retain their existing
[source and validation owner](zip-zstd.md#общий-libzstd-encoderdecoder--2026-10-24).
They are shared implementations, not additional Btrfs copies.

Source formatting is applied separately with clang-format to the adapted C
units. The complete disk-format header retains upstream layout/formatting apart
from the documented include and packing adaptations. Current source comparison
is retained in the task worktree's ignored review directory.

Linux Btrfs code is GPL-2.0; UAPI headers carry
`GPL-2.0 WITH Linux-syscall-note`. Original copyright notices are retained.
Existing exact Linux license texts are in `third_party/linux/LICENSES/`.
The module remains isolated from the SDK-only runtime overlay,
as are the other GPL filesystem readers.

## Supported reading and limitations

The reader supports directory/file browsing, byte-exact extraction, subvolumes
and snapshots, hardlinks, symlinks, sparse/preallocated/shared extents, xattrs,
zlib/LZO/Zstd compression and CRC32c/XXH64/SHA-256/BLAKE2b-256 verification.
Chunk mappings support SINGLE, DUP, RAID0, RAID1, RAID1C3, RAID1C4, RAID10 and
direct reads of healthy RAID5/6 data stripes. Available mirrored copies may be
retried after a read/checksum failure; parity reconstruction is not implemented.
Browsing begins at the top-level subvolume, including its child subvolumes;
the filesystem's configured mount-default subvolume is not selected instead.

For multi-device image sets, open one member and provide the others in the same
directory as `devid-<id>.img`. Member ID/UUID and filesystem identity must match.
Direct device access supplies the selected stream; discovery of other physical
members is not implemented. Unavailable members may prevent opening or leave
individual files unreadable, depending on where their metadata/data reside.

The reader uses the primary superblock and committed trees. It does not replay
tree log; a pending log produces a warning that recent changes may be absent.
Remap-tree, RAID-stripe-tree, zoned volumes and extent-tree-v2 are unsupported.
When an unsupported feature is present in a recognized superblock, native/device
opening diagnostics identify it. Unknown incompatible features are also rejected.
Live-volume consistency, repair, parity recovery and damaged
superblock recovery are outside this port. Keep sources unmounted and stable.
Filesystem data checksums do not authenticate fs-verity Merkle trees or
signatures; that VFS verification path is not transferred. Xattrs are exposed
as archive attribute streams, not applied as native ACL/security metadata.

This is a selected Linux read-side transfer, not a transfer of the complete
kernel Btrfs driver. The source inventory below records its connected scope.

## Source comparison and integration

No existing local or 7-Zip Btrfs reader was found. The first comparison covered
tree search/navigation, root-item lookup, inode lookup, directory enumeration,
extent lookup and checksum/metadata validation entry points. These consumers
establish the required connected scenario, rather than a tree-only feature:

- `disk-io.c`, `tree-checker.c`, `fs.c`: image-based checksum and consumed-item
  validation evidence is recorded below. Current superblock opening uses the
  primary copy, with no recovery mode. Additional sector/node geometries remain
  untested.
- `volumes.c`: chunk mapping and native member identity checks pass against
  Linux multi-device samples. Unsupported feature consumers are excluded as
  described above; no independent replacement decoding is introduced.
- `root-tree.c`, `inode-item.c`, `inode.c`, `file-item.c`, `dir-item.c`, `xattr.c`:
  generated images cover roots, directories, file reads, native links and
  attributes. Further compatibility review remains separate from these samples.
- `compression.c`, `zlib.c`, `lzo.c`, `zstd.c`: normal/inline/shared compressed
  extents and malformed/truncated framing checks pass against Linux samples.
- Local integration: C/native/module/device consumers, retained streams,
  cancellation/progress and ArchiveBackend errors pass. The local feature
  compatibility reporter supplies reasons through standard native error props;
  this bridges the selected reader's capabilities to presentation, without
  replacing a Linux decoding algorithm. Installed validation is recorded below.

Writes, journal/tree-log replay, repair, allocation, balance, scrub repair,
send/receive and mount services are outside this read-only port. New damaged-image
recovery modes are not being added implicitly: Btrfs is outside the prioritized
NTFS/FAT/ext recovery policy. Technical decoding limitations must be reported
explicitly, without declaring partial output fully verified.

The selected read-side component is connected through file APIs, native format
discovery, ArchiveBackend, device streams and installation. Source-transfer
scope is distinct from runtime evidence: the exclusions above remain technical
limitations even where the checked scenarios pass.

## Intermediate build evidence — 2026-10-24

macOS: `./scripts/build.sh dev sevenzip_btrfs_core` PASS with the shared official
Qt kit and existing compiler cache; C11 diagnostics and `git diff --check` PASS.
At that first checkpoint, only the new static C library was built;
Linux/Windows and application/device/install acceptance had not yet run.

The first configure found an existing test registration requiring a prepared
codec runtime. The sibling upstream lacks the pinned 26.05 object, so the
runtime was reused from the harness after checking its pinned revision, every
adapted-source hash against this worktree, and runtime artifact hashes. Only
runtime files were copied; build trees and caches were not moved. No unrelated
CMake/test-registration changes were needed.

The library and native module also build after connecting file APIs, concrete
compression, native attributes/properties and device/install registrations.
A targeted SHA-256/BLAKE2b-256 diagnostic matched Python `hashlib` for empty,
partial-block, exact-block and multi-block data; disposable diagnostic sources
and binaries were removed. This checks checksum integration, not Btrfs images.

The official sidecar's native format-discovery diagnostic loads
`sevenzip_btrfs.so` and lists Btrfs. This establishes the module ABI/discovery
boundary, not image reading or application acceptance.

Independent `reference-verify` passed for all 142 original files. The completed
sparse Git checkout is at the pinned revision; no duplicate clone was needed.
The current `reference-review` replaces the earlier partial source comparison.

## Image-based validation — 2026-10-24

`scripts/generate-btrfs-fixtures.py` creates the checked-in compressed fixtures
using `mkfs.btrfs` and mounted Linux Btrfs. Each finished image is reopened
`ro,nologreplay`; expected file hashes, symlinks and user xattrs are captured
through Linux and the source image hash is checked unchanged. The fixture
manifest records tool/kernel versions, mkfs options and original image hashes.
Generated images and mount directories are disposable; fixtures retain the bytes
needed to repeat tests without mounts or privileged access.

macOS `btrfs_reader` and `btrfs_module` PASS on the expanded seven-image set:
CRC32c/XXH64/SHA-256/BLAKE2b-256, zlib/LZO/Zstd regular and inline compression,
nonzero offsets into shared compressed extents, sparse/preallocated files,
multiple B-tree leaves, Unicode names, hardlinks, subvolume/snapshot identity,
symlinks and xattrs. Native properties distinguish equal inode numbers in
different subvolumes. Retained/interleaved streams remain readable after
Close/reopen/release; cancellation and input/output failures are exercised.
ArchiveBackend discovery/list/Test/selected extraction and unchanged source bytes
pass. Aligned 4Kn device-stream tests cover direct volumes and Linux-reader MBR
partitions, bounded reads and opening cancellation. These use mocks, not physical
media or OS authorization.

The first real-image run found that regular compressed extents can declare
sector-rounded `ram_bytes` larger than the decoded final chunk. The adaptation
now retains Linux `btrfs_decompress_bio` successful-decode zero filling; inline
decoders retain their required-length checks. The fixtures detect the former
false corruption error for all three codecs. Corrupted data checksums, a failed
second sector with partial byte accounting, a bad DUP metadata copy and malformed
but rehashed tree bounds are checked separately.

Native test runtime copies both the Btrfs module and the Linux MBR module. Using
only the SDK core would not exercise the installed Linux 4Kn partition reader.
The tests reuse the existing shared backend source registration and compiler
cache; no console client performs application operations.

The macOS focused batch also passes `linux_frames`, `linux_frames_stream`,
`squashfs`, `squashfs_stream` and `squashfs_metadata`, checking the direct
consumers of the shared LZO include change. Native data corruption returns
`kCRCError` with no successful extraction result. The initial Linux two-suite
pass in `20261024T101040Z-fc770976` was followed by the complete three-suite
Linux/Windows pass in `20261024T102252Z-ec5cb54b`.

`btrfs_multidevice` covers Linux-created RAID0/1/1C3/1C4/10/5/6 images,
opening-callback release before extraction, absent members, substituted member
identities, opening cancellation and a missing RAID1 mirror. The fixture
generator reopens every member set through Linux `ro,nologreplay`, compares all
expected contents and checks unchanged member image hashes. Original chunk-tree
dumps are retained in task review evidence. Malformed/truncated regular and
inline compression frames are rejected without writing beyond output bounds.
Unsupported flags in recognized, correctly checksummed superblocks produce
specific native error properties, leave no partial file list, propagate to device
opening diagnostics and do not survive a later successful opening.

## Installed workflows and final checks — 2026-10-24

`scripts/check-btrfs-installed.sh` builds required targets, uses the existing
package installer, starts the installed application and reruns native module and
multi-device suites against installed Btrfs/MBR providers. It reuses build trees
and compiler caches. The macOS pinned SFX artifact was reused from the harness
after verifying its full source revision, platform and SHA256; no build tree was
copied or reset.

| Platform | Focused Btrfs suites | Installed startup and two native suites |
| --- | --- | --- |
| macOS arm64 | PASS | PASS, Cocoa outside sandbox |
| Linux x64 | PASS; final focused follow-ups below | PASS, `20261024T115230Z-1cf0cf1f`, Xvfb/XCB |
| Windows x64 | PASS; final focused follow-ups below | PASS, `20261024T115230Z-1cf0cf1f`, native limited desktop session |

The Windows installed checker explicitly builds the shell DLL required by the
existing package install. The existing interactive launcher originally rejected
branch-scoped worktrees; its allowlist now accepts generated branch roots and
continues rejecting escaping/nested paths. Its offline `hyperv_runner` regression
passes, and the installed Windows run exercises the actual scheduled-task path.

An initial Windows discovery timeout passed on the next focused run with the
same deadline. One later SSH stdin bootstrap stalled before preparing any new
guest command. Token/mtime checks and identical old log hashes distinguished it
from an executing build; only its identified bootstrap was stopped and its own
lock released after verification. The successful run above supersedes it.
No guest, unrelated task, toolchain or compiler cache was reset.

Final source review removed an allocating diagnostic copy from failure cleanup:
the guarded Open body prepares the error; cleanup only releases reader/items.
The two affected native suites pass on macOS and in Linux/Windows run
`20261024T111206Z-854255ec` after that local change.

The subsequent complete root-opening comparison found a missing Linux root-node
owner check, distinct from the looser shared-descendant rule. The transferred
root-read path now retains it. A correctly rehashed root with a different
subvolume owner is rejected; the generic shared-descendant owner rule remains
unchanged. macOS passes all three suites after this fix; Linux passes all three
in `20261024T112253Z-102066d2`, and both Windows native suites pass in that batch.
The new corruption test initially timed out on Windows while locating a long,
zero-padded node in its fixture. A sector-aligned replacement pass fixes test
preparation without changing the corruption/assertions or extending its timeout.
The final C-reader follow-up passes on macOS and Linux/Windows
`20261024T113133Z-ed184167`.

The real ArchiveBackend multi-device scenario also opens the secondary RAID0
member, resolves adjacent member images and extracts files from distinct
subvolumes with equal inode numbers into correct, different outputs. Both source
members remain byte-identical. This passes on macOS and Linux/Windows
`20261024T114427Z-7d2fe2ec`. Installation rules and dependencies remain unchanged
by these focused follow-ups. The installed workflows were then rerun with the
final modules and expanded backend case on all three platforms; macOS passes
both installed suites and Linux/Windows pass in `20261024T115230Z-1cf0cf1f`.

This is focused acceptance, not a full/release gate, slow-suite run, physical
media/OS authorization test, pixel-parity claim or exhaustive filesystem feature
matrix. Generated filesystem geometry is recorded in the fixture manifests;
other geometries and damaged-volume recovery remain unverified.

## Integration onto master — 2026-10-24

Rebase onto `403217f0` retains F2FS/XFS and the other merged format registrations
and installation entries. Btrfs now uses format ID `0xF9`: the original feature's
`0xF5` is already assigned to LZ4 Frame on master. Its Zstd consumer follows the
shared adapter selected by master, without another decoder implementation.
The removed kernel inflate dependency is replaced with master's existing official
zlib target (baseline `da607da739fa6047df13e66a2af6b8bec7c2a498`). Only the
allocator/API boundary changes: Linux Btrfs wrapper handling and output completion
rules remain in the adapted reader; no new decoder sources are imported.
The same removed private header was still referenced by Linux LZO/LZ4 boot
wrappers on master. Their Adler-32 adapter now uses the same official zlib target;
the wrapper algorithms and checksum contract are unchanged.

Focused macOS merge validation passes all eight suites: Btrfs reader/module/
multidevice, LZ4 Frame, Zstd stream, Linux frames/stream and the offline Hyper-V
runner checks. Linux/Windows results above describe the pre-rebase feature;
this integration was checked on macOS, without repeating full/slow gates.
