# Read-only EROFS from Linux

## Original shared string services — 2026-10-22

Compared the complete pinned `lib/string.c::check_bytes8`/`memchr_inv`,
`mm/util.c::__kmemdup_nul`/`kstrndup`/`kmemdup_nul`, their declarations in
`include/linux/string.h`, and the EROFS compression-padding, volume-name and
inline-symlink consumers. The reader previously supplied independent versions
of these services despite usable original Linux bodies.

EROFS now compiles the existing original `lib/string.c` scanner, also used by
ext crypto. Independent exFAT boot-field and ext fs-verity reserved/tail-byte
scanners are removed and their consumers use that same implementation. A
selected declaration retains the original header path. The scanner's existing
strict-C byte-pointer arithmetic and LLP64 `uintptr_t` alignment adaptation are
preserved; its private scalar/export bridge no longer includes crypto services.
All scanner bodies otherwise match the pinned reference. Each separate module
links its own required object; this does not add a cross-module runtime service.

The original duplication helpers are transferred together into `src/mm/util.c`.
Their bodies are unchanged except that `__kmemdup_nul` retains the archive
bridge's existing `SIZE_MAX` allocation-overflow rejection. Host allocation
replaces `kmalloc_track_caller`, GFP flags are ignored, and export annotations
are omitted. A pointer-sign cast at the volume-name call accommodates the
original `const char *` parameter. Unused kernel memory-management and string
services are omitted. Complete reference units and selected/adapted scope are
recorded in the EROFS, exFAT and ext baselines at the repository/revision below.

This removes source-transfer debt; no incorrect file bytes were established in
the replaced implementations. Existing behavior suites cover extraction of
compressed files and symlinks, volume metadata, malformed compressed input,
exFAT boot validation and fs-verity verification/error propagation. No new
recovery mode, source write, repair or journal replay is introduced. Overall
EROFS source closure and the parent filesystem audit remain open.

Validation: `erofs_reader`, `erofs_module`, `erofs`, `exfat`, `exfat_backend`,
`ext_stream`, `ext_fscrypt`, `fsverity_signature`, `fscrypt_crypto`,
`upstream_baseline` and `upstream_inventory` passed on macOS, Linux and Windows
(guest receipt `20261022T171919Z-b1ac128e`). The incremental application build
passed. All 54 EROFS and 304 ext reference hashes, and the added exFAT dependency
hashes, match. Existing runtime snapshots, build trees and compiler caches were
reused; disposable task fixtures were absent on all three platforms afterwards.
No full gate, slow suite, new native mount/device check, UI appearance acceptance
or release installation was run.

## Linux UUID publication and archive volume metadata — 2026-10-22

Compared the pinned `super.c::erofs_read_superblock` UUID/name/geometry reads,
`include/linux/fs.h::super_set_uuid`, the UUID fields in
`include/linux/fs/super_types.h`, the C volume-info boundary and archive property
enumeration/consumers. The host UUID setter was a no-op, losing the disk UUID.
In addition, the handler returned no archive-property descriptors, hiding its
existing volume-label and block-size properties from CLI/property enumeration.

The original complete `super_set_uuid` helper is transferred into its original
`include/linux/fs.h` path with its body unchanged. The private superblock bridge
retains the original UUID/length field names and byte length type; a 16-byte
array replaces kernel `uuid_t`. Warning evaluation keeps the original bounds
clamp without kernel diagnostics. Other VFS definitions/services from that header
are omitted. Both reference units are hashed in the existing EROFS baseline at
the same Linux repository/revision below. The original EROFS parser call is
unchanged. The C API now publishes the raw UUID alongside existing volume info.

Archive `Id` presentation follows the pinned 7-Zip
`CPP/7zip/Archive/ExtHandler.cpp::IsEmptyData` and its `GetArchiveProperty(kpidId)`
branch: the helper and property convention are reused with the existing
`Common/IntToString.cpp::ConvertDataToHex_Lower`. Nil IDs remain absent. Original
7-Zip repository/revision remain those recorded below. The handler now advertises
its volume label, cluster size and ID through the standard archive-property table;
structural properties and physical-size queries keep their existing behavior.
No UUID byte-order conversion or independent identifier formatter is introduced.

The pre-fix module regression returned VT_EMPTY for a nonzero UUID. CLI
enumeration showed neither the label nor cluster size. Direct C API coverage now
checks raw UUID bytes from a source image; archive ABI coverage checks nonzero/nil
IDs and Close state. CLI cases use creator images with 512-byte, 4 KiB and 64 KiB
blocks, check label/geometry/ID output and unchanged source bytes. Existing suites
retain extraction, layout, lifetime, error and cancellation coverage. This is
normal metadata reading, with no new recovery mode or source-writing service.
Overall EROFS source closure and the parent filesystem audit remain open.

Validation: `erofs_reader`, `erofs_module`, `erofs`, `upstream_baseline` and
`upstream_inventory` passed on macOS, Linux and Windows (guest receipt
`20261022T170859Z-7ebfb358`). The incremental application build passed. The
transferred UUID helper matches Linux byte-for-byte and all 51 EROFS reference
hashes match. The expected pre-fix ABI/CLI failures establish sensitivity to the
lost UUID and unadvertised metadata. Existing runtime snapshots, build trees and
compiler caches were reused; disposable EROFS fixtures were absent afterwards
on all three platforms. No full gate, slow suite, new native mount/device check,
UI appearance acceptance or release installation was run.

## Original device mapping and slot-reader boundary — 2026-10-22

Compared complete pinned `erofs_fill_from_devinfo`, `erofs_map_dev`,
`erofs_init_device`, `erofs_scan_devices` and their uncompressed/compressed stream
consumers. The archive reader had an independent device-selection implementation.
The original mapper and fill helper are now transferred into their original
`fs/erofs/data.c` module and used by physical stream reads. Primary-device default,
explicit IDs, unified-image address adjustment and implicit unified-range lookup
retain Linux's branch order. Immutable bounded array lookup/iteration replaces
the kernel IDR and lock; stream descriptors replace file/block-device handles.
Existing checked addition and subtraction-based interval checks retain overflow
protection at the archive boundary.

The original `erofs_init_device` boundary now owns metadata acquisition, tag and
32/48-bit slot fields, total-block accounting and cursor advancement in
`fs/erofs/super.c`. Existing safe-name/read-only stream callbacks replace kernel
path allocation and file/bdev opening. The scanner still owns the immutable
stream table and its bounds; missing-volume fallback and cancellation remain
archive transport behavior. This iteration removes source-transfer debt; the
previous device selection was not shown to return wrong data for the tested
layouts.

New public C-reader cases use two auxiliary devices with a gap between their
unified ranges. Both external streams and a unified image are read through
explicit IDs and implicit unified addresses. Invalid IDs and exact-end addresses
return errors with the exact readable first-chunk prefix. Callback routing and
unchanged source bytes are checked. Existing native creator-image ABI/CLI tests
cover named volume acquisition, cancellation, missing-volume fallback and all
other EROFS layouts. No recovery mode or write service is added.

Validation: five focused EROFS/provenance suites passed on macOS, Linux and
Windows (guest receipt `20261022T163641Z-3cb006df`); the incremental application
build passed. All 49 EROFS reference hashes match. Existing incremental build
trees, runtime snapshots and compiler caches were reused; disposable EROFS
fixtures were absent on all three platforms afterwards. No full gate, slow
suite, new native mount/device check, UI appearance acceptance or release
installation was run. Overall EROFS source closure and the parent filesystem
goal remain open.

## Recovery priority — 2026-10-22

The user's narrowed [recovery scope](../../AGENTS.md#read-only-filesystem-access)
defers new partial-access modes for damaged EROFS images. Source comparison found
that an unreadable inode xattr currently fails archive opening through
`ReadAttributes`, even when ordinary file bytes could be read. That recovery
scenario is deferred; no code change was made for it. Correct reading of supported
EROFS layouts and their Linux source comparison remain in scope.

## Original metadata-buffer ownership and failed-read cleanup — 2026-10-22

Compared the complete pinned `data.c` metadata-buffer helpers, their
`erofs_map_chunks`, inode, superblock/configuration, xattr/prefix and zmap callers,
and the local stream/page services. Linux callers can return directly after a
failed initial metadata read: `read_cache_folio` returns an error without
transferring a folio to the buffer. The independent userspace `erofs_bread`
allocated its buffer before reading and retained it on failure, violating that
ownership contract. Failed superblock opens and chunk-table reads could leak one
metadata allocation each.

The original `erofs_unmap_metabuf`, `erofs_put_metabuf`, `erofs_bread` and
`erofs_read_metabuf` now live in their original `fs/erofs/data.c` module. Unmap,
put and the read wrapper are byte-exact. The original bread cache-hit, release,
acquire, publish and map control flow is retained; adaptations resolve the host
stream context, retain source bounds/cancellation checks, replace cache loading
with the bounded host service and cast byte-pointer arithmetic for portable C.
The separate independent implementations are removed.

The host cache service allocates an owned single-page folio and publishes it
only after a complete bounded read; errors release that allocation locally.
Metadata folios use the existing 64 KiB page ceiling, including zero padding of
a valid final partial page. The shared contiguous page adapter also serves the
existing plain decompression transform. Actual VFS page-cache sharing,
readahead, asynchronous I/O and mapping services remain outside the archive
reader. Stream mapping selection retains primary/metabox/packed-inode handling,
and the transport still owns any embedded-image base offset. No decoding or
filesystem mapping algorithm is reimplemented.

A public C-reader scenario repeats failed superblock opens and then enumerates
the recovered image. A second synthetic chunk-based image places its inode and
chunk table on different metadata pages, injects repeated table-read failures,
then reads exact payload bytes through the same file handle. Both verify source
preservation. Existing module/CLI suites cover all creator layouts, xattrs and
prefixes, embedded-image offsets, retained streams, failure/cancellation and
reuse. macOS `leaks --atExit` on the initial-open scenario found 32 leaked
allocations (2,621,440 allocator-accounted bytes) before the fix and zero after
the fix. This is a resource-lifetime defect; the ordinary operation-result test
alone already passed before the fix.

Source access remains read-only and bounded, with no new opening restriction,
writes, repair or journal replay. The EROFS source-closure audit and parent
filesystem goal remain open.

Validation: `erofs_reader`, `erofs_module`, `erofs`, `upstream_baseline` and
`upstream_inventory` passed on macOS, Linux and Windows (guest receipt
`20261022T135553Z-f04a080c`). The incremental application build passed. The full
56-case direct C-reader run also reported zero leaks under macOS `leaks --atExit`,
including both repeated-read-failure scenarios. Final comparison confirms all
four transferred metadata functions after the explicit host adaptations above;
all 49 EROFS reference hashes match. Existing incremental build/runtime/cache
state was retained. Disposable EROFS fixtures were absent on all three platforms
after validation. No full gate, slow suite, new native mount/device check,
UI appearance acceptance or release installation was run.

## Linux device-number decoding and special inode metadata — 2026-10-22

Compared the pinned `fs/erofs/inode.c` compact/extended inode readers and special
file branches, `include/linux/kdev_t.h`, the C metadata boundary and every EROFS
consumer of `rdev`. The host services had replaced `new_decode_dev` with an
identity macro, leaving disk-encoded device numbers in `i_rdev`. The archive
handler independently decoded those disk numbers, so its major/minor properties
were already correct; the C reader did not expose Linux's internal device-number
representation.

The identity macro is removed. The original inode branches now call the existing
shared Linux `new_decode_dev` adaptation, already used by other filesystem
modules. Its original function body and Linux MAJOR/MINOR/MKDEV definitions are
retained; fixed-width `uint32_t` and ordinary inline linkage replace kernel types
and attributes. Unused encoders and VFS printing remain omitted. The EROFS
baseline now records that shared header at the same repository/revision below.
The C boundary documents the resulting Linux 12-bit major/20-bit minor `rdev`
representation. The archive handler uses the shared MAJOR/MINOR definitions,
removing its independent disk decoder without changing archive properties.

Metadata-only synthetic images exercise compact inodes, extended inodes and an
extended inode spanning two metadata blocks. Character/block devices cover
zero, ordinary, mixed high-bit and maximum device numbers; FIFO/socket cases
confirm that their disk union is ignored. Direct C tests verify decoded metadata
and unchanged source bytes. Module ABI tests verify major/minor property values
and absence for non-devices, plus empty streams for all four special types.
No host special files are created or opened. Before the fix, the new C regression
returned 259 for device 1:3 instead of Linux dev_t 1048579, including both extended
inode paths; archive property/stream tests already passed.

Source reads remain bounded and read-only; no writes, repair, journal replay,
device-node extraction or new opening restriction is introduced. Overall EROFS
source closure and the parent filesystem audit remain open.

Validation: `erofs_reader`, `erofs_module`, `erofs`, `upstream_baseline` and
`upstream_inventory` passed on macOS, Linux and Windows (guest receipt
`20261022T134745Z-bff06d4c`). The incremental application build passed.
All 49 EROFS reference hashes match; the shared decoder body and original
special-inode branches match the pinned Linux source. Existing build trees,
runtime snapshots and compiler caches were reused. Disposable EROFS fixtures
were absent locally and on both guests after validation. No full gate, slow
suite, new native mount/device check, UI appearance acceptance or release
installation was run.

## Original Linux dirent conversion and C-reader callback ABI — 2026-10-22

Compared complete pinned `fs/fs_dirent.c`, `include/linux/fs_dirent.h`,
`erofs_fill_dentries`, the archive directory emitter and its Qt consumer. The
EROFS host adapter replaced `fs_ftype_to_dtype` with identity conversion, so the
public C enumeration callback received disk `FT_*` values rather than Linux
`DT_*`. The Qt consumer already ignores that hint and derives folder/file
classification from the inode's actual mode. The defect affected the C-reader
callback and source fidelity.

The new direct C-reader regression failed before the fix: a real creator-image
directory was emitted as type 2 instead of Linux DT_DIR (4). The original full
`fs/fs_dirent.c` implementation is now compiled as C11 into the existing EROFS
core, with its tables and all three function bodies unchanged. Only includes and
kernel symbol-export registration are adapted. The complete original header
retains its ABI definitions/declarations; portable fixed-width `umode_t` and the
Linux file-kind mask replace kernel type/stat includes. Original notices remain.
Repository and revision remain the Linux source below, with full unit/header
hashes and adapted scope in the existing baseline.

The identity macro is removed and `erofs_fill_dentries` now calls the real
conversion. ext uses the same adapted ABI header instead of a second constant
bridge; its original `get_dtype` behavior is retained. Unknown encoded types
become DT_UNKNOWN while names/inode IDs remain available. The Qt handler still
uses validated inode mode, with unchanged bounds/path/cycle checks, source
read-only access and copy behavior. No source-writing or repair service is added.

A focused C API consumer enumerates the native plain image and compares callback
types with each inode's Linux stat mode, checks unknown/unsupported hints without
lost names, returns a callback stop error and reads the directory again. CLI
coverage copies exact Unicode-named file data with missing/unsupported type hints
and verifies unchanged source bytes. Existing module/backend coverage continues
to exercise all established creator variants and transport/error boundaries.
The overall EROFS source-closure audit and parent filesystem goal remain open.

Validation: all eleven focused EROFS/ext/provenance suites passed on macOS,
Linux and Windows (guest receipt `20261022T132809Z-99cb7f9d`). The incremental
application build passed. Full-unit comparison confirms that the shared C unit
differs only in include/export services, while all three original function bodies
and both tables are retained. All 48 EROFS and 304 ext reference hashes match.
The expected pre-fix callback-type failure establishes regression sensitivity.
Existing build trees, runtime snapshots and compiler caches were reused;
disposable EROFS/ext fixtures were absent after checks. No full gate, slow suite,
new native mount/device check, UI appearance acceptance or release install was run.

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
- `data.c` retains metadata-buffer ownership/read helpers, `erofs_map_chunks`
  and `erofs_map_blocks`. Raw, inline,
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

The subsequent [SquashFS xattr iteration](squashfs-linux.md#публикация-xattr-values--2026-10-20)
reuses EROFS's ownership/name projection through a shared GPL-only source.
EROFS process/ABI/backend suites are included in that change's focused validation.

A real local macOS installation of the final runtime read nine exact metadata
values across inline, packed-prefix and shared-metabox images and honored
`-sns-` filtering. Installed LGPL/syscall notice bytes matched the retained
originals. The disposable installation and its log were removed. Incremental
build trees and compiler caches remain intact. No full/slow, release archive,
native authorization, physical-device or UI appearance gate is claimed.
