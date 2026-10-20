# ext2/3/4 read-path review against Linux

Part of the [Linux format review](filesystems-linux.md), 2026-10-19. This first
integrated ext iteration covers classic indirect mapping, sparse reads, extent
tree boundaries, directory record boundaries and archive stream consumers.
The second integrates Linux metadata and inline-data readers. The ext row remains
open for the remaining read features listed below.

## Sources and comparison scope

Linux repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, revision
`58161ead29e50d70a317ea26b472e0c7508e69da`. The
[source inventory](../ext-linux-baseline.json) records complete `fs/ext2` and
`fs/ext4` units. Selected read paths were compared in `ext4/indirect.c`
(`ext4_block_to_path`, `ext4_get_branch`, lookup in `ext4_ind_map_blocks`),
`extents.c` (header/entry validation, lookup and unwritten/hole semantics),
`dir.c`/`ext4.h` (directory checks and record-length decoding), `inode.c`
(fast symlinks, ACL pointers and incompatible inode flags), `block_validity.c`
and superblock group/file-size calculations. The corresponding ext2 fast-link
and inode read rules were also checked. Write/allocation portions are reference
context; they are not archive operations.

7-Zip repository: <https://github.com/ip7z/7zip.git>, revision
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. The complete 2860-line
`CPP/7zip/Archive/ExtHandler.cpp`, including properties, registration, callers,
both stream classes and extraction, was inspected before adaptation. Its
authoritative copy remains under the same path in `src/`. Original vector,
buffer, copy/progress, stream, property, text and CRC16 helpers are reused.
The [7-Zip baseline](../upstream-baseline.json) selects it for both engine builds.
No sibling source is changed and no new filesystem decoder is introduced.
Project formatting is compared separately against an equivalently formatted
original; macro-delimited class/Open bodies retain their original structure.

## Integrated changes

The original commented-out zero-subtree branches now fill the appropriate
logical span with holes in both the inode's indirect pointers and nested tables.
Span arithmetic uses 64 bits, is clipped to the requested logical block count
and uses the vector's existing reservation. Direct and leaf-level zero blocks
retain their original stream behavior. Missing whole single/double/triple
subtrees and discontiguous data after them therefore read as exact zero bytes,
including random access to large logical files stored in small images.

Extent checks now enforce nonzero bounded header capacity, entry count, nonempty
internal/child nodes, nonzero lengths, ordering and nonoverlap. Child first keys
must agree with the parent index, and child ranges must fit their parent interval.
The latter is an explicit consistency guard against ambiguous cross-leaf maps.
Unwritten extents retain zero-filled output even when physical storage contains
stale nonzero bytes. The existing depth ceiling matches Linux's actual
`EXT4_MAX_EXTENT_DEPTH` of five, including its lookup check; the generic header
check's larger corruption ceiling does not imply support for deeper reads.

Directory lengths use Linux's extended encoding, including whole 64 KiB blocks
represented by zero or `0xffff`. Records must fit their individual filesystem
block, satisfy the minimum/alignment rule and contain their declared name.
Deleted/index/checksum records still follow the original walker. A malformed
record cannot cross into the next concatenated directory block.

Group count excludes `s_first_data_block`, fixing a complete final group in a
1 KiB-block filesystem. ACL block pointers are retained and range checked.
Fast-symlink detection accounts for external xattr storage in `i_blocks` and the
EA-inode feature, rather than treating a short regular file as a symlink buffer.
Standalone builds retain the short inline-data fallback; complete inline support
is connected through the dynamic module described below. Conflicting inline and
extent flags are rejected as in Linux.

Initial data seeks and stream-construction errors now propagate through Extract,
including cancellation and input failures. Sparse construction may report open
progress while opening directories; the borrowed open callback is cleared on
success, failure, exception and Close so later extraction cannot call a released
open callback. Returned streams retain their original source and copied mapping
when the archive closes or reopens.

## Behavior validation

Deterministic local volumes encode direct/single/double/triple maps, absent root
and nested subtrees, initialized/unwritten extents, multiple tree branches,
depth-five trees, complete block groups and fast symlinks with an external EA
block. Both production engines extract exact bytes from the bounded process
fixtures and preserve source images. Malformed tests reject extent capacities,
overlap/zero lengths, index/child inconsistencies, empty children, invalid physical
and ACL ranges, conflicting flags and malformed directory records. Positive
64 KiB directory cases cover both whole-block encodings.

ABI checks seek to the beginning, middle and tail of roughly 67 MiB logical
triple-indirect files in roughly 1 MiB images, without extracting a large matrix
of zeros. They cover stream lifetime after Close/reopen/Release, short reads,
direct-data and indirect-metadata seek errors, cancelled/failed opens, cancelled
extraction, output failure and reuse. Existing direct-device/folder/UI consumers
exercise the integrated engine on generated ext2, ext3 and ext4 samples. No real
user image or physical device is used.

Twelve read-only loop mounts using Linux `6.8.0-146-generic` verified the generated
classic/extent layouts, multiple branches, depth-five trees, sparse triple reads,
the complete group boundary and symlink target. Exact sampled bytes and file
sizes matched. All mounts and temporary images were removed. These older-driver
checks supplement the pinned modern source; they do not validate 64 KiB blocks
on that guest or execute the pinned 2026 kernel.

Nine focused suites (`ext`, `ext_stream`, `archive_device`, `device_folder`,
`device_ui`, `codec_probe`, `runtime_cache`, `upstream_baseline`,
`upstream_inventory`) passed on Linux x64 and Windows x64/MSVC/Qt 6.12 in receipt
`20261019T083318Z-683b084c`. On macOS arm64/Qt 6.12, the seven backend/source
suites passed first. The two device consumers initially found Homebrew `7zz`
26.03 while using the prepared 26.04 module; they passed with the prepared engine
first on PATH (2.45 s). Cocoa also required an unsandboxed run to access screens
and system services. No product timeout or runtime configuration was changed.
The new ext process/ABI checks remain a small focused suite; logical triple-file
size is not materialized as a large extraction matrix.
No full/slow, clean-package or physical-device acceptance is claimed. Existing
incremental builds and compiler caches are retained.

## Linux metadata and inline-data module

The same pinned Linux revision supplies the read-side bodies now adapted under
`src/fs/ext4/`: `super.c` superblock/GDT checksum verification and descriptor
location; `balloc.c` sparse-super placement; `bitmap.c` bitmap verification;
`inode.c` inode checksums; `inline.c` inline lookup, copying and directory
enumeration; `xattr.c` bounds, search and external-block checksum validation;
`dir.c` directory-entry validation; `namei.c` leaf/htree checksums; and
`extents.c` external extent-block checksums. Disk structs remain in the original
`ext4.h`, `ext4_extents.h`, `xattr.h` boundaries. The inventory also pins the
CRC16, CRC32C and table-generator dependencies in `lib/crc`. Existing EROFS CRC32C
code/tables are reused without another implementation. Full notices are retained,
including the LGPL inline-data source.

All borrowed filesystem/CRC implementations remain C. `userspace.h` and
`services.h` replace kernel endian, allocation, buffer, immutable inode/version,
locking and directory-emission services. Pointer arithmetic uses `char *` for
MSVC C; selected formerly static read helpers are exported inside the module.
The archive boundary owns bounded inode copies and opaque volume/node lifetimes.
The htree caller validates count/limit before the original checksum routine,
since the original VFS parser normally provides that prerequisite. Inline
directory diagnostics become archive errors instead of silent partial listings.
Writes, allocation, journal recovery, page cache, inode caching and VFS mutation
are omitted from this read-only component.

The original 7-Zip C++ handler still owns archive metadata, mapping, properties,
progress, streams and extraction. The separate GPL `sevenzip_ext` format module
connects the C services and registers Ext in the dynamic application engine.
The dynamic core retains partition/container recognition helpers without a second
Ext registration. Standalone `7zz` retains its LGPL classic reader; modern Linux
features are provided by the application's dynamic engine. No unRAR source is
linked into the format module. Install rules include GPL-2.0 and the unmodified
7-Zip `DOC/copying.txt` LGPL-2.1 text, pinned to the same 7-Zip revision above.

Integrated paths include complete inode/xattr inline bytes and nested inline
directories, 64-bit/power-of-two descriptors up to Linux's maximum, distributed
META_BG placement, BIGALLOC geometry, uninitialized inode groups and LARGEDIR
sizes. Checksums cover superblock, descriptors, initialized inode/block bitmaps,
inodes, external xattr blocks, directory leaves/htree nodes and extent blocks,
including stored checksum seeds. Time fields are tested individually against
extra-isize boundaries; epoch zero remains defined. Huge-file block accounting
uses the feature flag as Linux does. Unknown incompatible layouts are rejected,
and encrypted content cannot be returned as successful plaintext.

Direct-device consumers discover the format module through the same dynamic
engine/probe as the product. Their feature policy accepts the implemented modern
layouts while retaining refusal of filesystem encryption. An explicit
[device session](direct-devices.md) may read a filesystem needing recovery without
replaying its journal; journal recovery itself is still omitted.
Creator images are read and extracted through the authorized device stream,
in addition to console/ABI tests. ABI lifetime checks reopen with different
inline bytes, close/release the handler and verify the original stream contents.
Timestamp checks compare FILETIME values directly, avoiding native console
timezone formatting as a proxy for the stored timestamp.

The small deterministic fixtures independently generate checksums, 128-byte
descriptors, inline payloads crossing i_block/system.data, nested inline
directories, partial timestamp extensions and indexed directories. Corruption
checks target each checksum class, malformed inline xattrs, directory records
and htree count/limit bounds. Official `mke2fs` 1.47.0 images cover inline data,
BIGALLOC and distributed META_BG/unused groups; compressed fixtures and creator
options are under `tests/fixtures/ext4`. Both files in each image were compared
byte for byte through read-only Linux `6.8.0-146-generic` loop mounts and the
production dynamic engine. Four deterministic checksum/seed/descriptor/inline
volumes and a directory with a root/internal htree node also mounted and returned
exact samples through that kernel. Temporary
images/mounts were removed. These runtime checks supplement the pinned 2026
source; they do not execute that newer kernel.

The five affected suites (`ext`, `ext_stream`, `archive_device`, `device_folder`,
`device_ui`) passed on Linux x64 and Windows x64/MSVC/Qt 6.12 in receipt
`20261019T091359Z-a93a285c`, and on macOS arm64/Qt 6.12. The final localized
metadata guard also passed the macOS ext/ABI rerun. The initial wider focused
batch additionally passed `codec_probe`, `runtime_cache`, `upstream_baseline`
and `upstream_inventory` on all three platforms. Its device failures exposed
tests loading only the core or standalone engine instead of the product's
module discovery; those tests now exercise the dynamic probe. The Windows
timestamp check initially depended on native local-time console formatting and
was replaced with exact public ABI timestamp validation. No timeout limit was increased.
The source inventory verifies 416 exact, 206 adapted and six local 7-Zip paths.
No full/slow, physical-device, native authorization or clean-package gate is
claimed; incremental build trees and compiler caches were retained.

## fs-verity data verification

The same pinned Linux revision supplies `fs/ext4/verity.c`'s metadata-position,
descriptor-location, descriptor-read and Merkle-page read bodies. The existing
7-Zip extent walk remains authoritative for physical mapping; its validated final
allocated endpoint supplies the location query used by ext4's descriptor reader.
Metadata beyond logical EOF is readable only inside the verifier; returned
streams retain the original logical file length.

Selected complete read-side bodies from `fs/verity/open.c`, `hash_algs.c`,
`verify.c` and `pagecache.c` provide descriptor checks, salted hash state, tree
topology and ascent/descent verification, including sparse/unwritten data and
EOF padding. Their headers stay at the original `fs/verity`, `include/linux`
and `include/uapi/linux` paths. The extended inventory pins all generic verity
units and both headers. All implementations remain C11. Original 7-Zip SHA-256
and SHA-512 C implementations provide the host hash services; no new hash or
Merkle algorithm is introduced. The separate GPL module remains isolated from
unRAR. Its install payload now also includes Linux-syscall-note for the UAPI
header notice.

The host represents pages as 64 KiB buffers so all ext block sizes fit. It uses
a bounded metadata-page cache with per-page verified-block masks, instead of the
kernel's per-inode/global page-cache bitmap. Consequently, the kernel's bitmap
allocation cap is omitted and file/tree indices retain Linux's 64-bit width on
LLP64 hosts. These adaptations affect cache/allocation services; the verification
path still checks the complete chain against the stored root. A failed block
is never added to the 7-Zip data cache or copied into the caller's buffer.

`CCachedInStream` supplies the unchanged 7-Zip read/seek/cache behavior; the local
stream adapter only acquires, pads and verifies blocks. Source failures and
`E_ABORT` cross the C service boundary unchanged. Streams own their mapping,
source, metadata and verification state after handler Close/reopen/Release.
The dynamic module verifies verity data; standalone `7zz` explicitly refuses
verity-marked file streams rather than claiming unchecked reads as verified.
Direct-device policy accepts the implemented verity data path.

Write/enable/metadata mutation, ioctl transport, global inode caching, scheduling,
readahead and kernel trust keyrings are omitted. The signature callback is the
original `CONFIG_FS_VERITY_BUILTIN_SIGNATURES=n` service: stored signature bounds
are checked, but this module makes no certificate/authenticity claim. Trust/key
policy still needs to be considered with the remaining fscrypt integration.

Deterministic fixtures independently produce SHA-256/SHA-512 trees using hashlib,
with/without salt, empty and short files, multiple tree levels, holes/unwritten
extents and partial EOF blocks. Metadata/data mutations cover descriptor version,
size, reserved fields, algorithm, salt, logical size, block size, signature bounds,
root, tree and file data. Valid descriptor padding remains accepted as in Linux.
ABI regressions exercise seeks and lifetime, failed metadata reads, failed block
reads, cancellation/retry, and an unchanged caller buffer on verification failure.
The tests assert operation results and bytes, not internal implementation names.

Eight read-only mounts on Linux `6.8.0-146-generic` accepted the valid deterministic
cases and rejected corrupted data/tree reads with `EIO`. A separate mke2fs 1.47.0
image was enabled through Linux `FS_IOC_ENABLE_VERITY` for SHA-256, salted SHA-512,
short and empty files; `FS_IOC_MEASURE_VERITY` results are recorded in the fixture
manifest. All bytes matched through a fresh read-only remount and the production
dynamic engine, including extraction through the authorized device stream.
Temporary images and mounts were removed; the compressed creator image remains
under `tests/fixtures/ext4`. These tests execute Linux 6.8; the source baseline
remains the pinned 2026 revision. Native 64 KiB pages/blocks are not claimed.

Eight focused suites (`ext`, `ext_stream`, `archive_device`, `device_folder`,
`device_ui`, `codec_probe`, `upstream_baseline`, `upstream_inventory`) passed on
macOS arm64/Qt 6.12 locally (6.42 s) and Linux x64 in receipt
`20261019T094203Z-fb47223f`.
Windows x64/MSVC/Qt 6.12 passed the same selection in
`20261019T094444Z-9bf69376` after the host header undefines Windows SAL's
`__reserved` annotation macro before the original Linux disk field declaration.
The initial Windows build failure was this name collision; the disk field and
layout remain unchanged. No timeout, cache or toolchain settings were altered.
The added standalone rejection regression passed in the localized `ext` rerun on
macOS and on both runners (`20261019T094646Z-6362f08b`).
No full/slow, package, physical-device or native authorization gate is claimed.

## Read-only feature compatibility

The complete `ext4_feature_set_ok` and `ext4_check_feature_compatibility` bodies
in the pinned `fs/ext4/super.c`, their mount-call context and `ext4.h` feature helpers
were compared with `ext_linux_open`. The transferred C subset now owns the
incompatible-feature mask and rejects HURD with 64bit or EA_INODE, plus nonzero
encryption levels when the encryption feature is present. Previously these
combinations could extract successfully despite Linux rejecting the filesystem.
Accepted HURD 32-bit images, other creator OS values and ordinary unencrypted
files on encryption-capable filesystems remain readable.

The adaptation omits mount diagnostics, rev-0 warnings, ext2/ext3 driver probing,
DAX and read-write/quota checks. Archive reads are always read-only, perform no
casefold lookup and preserve stored directory names, so the kernel's dependency
on CONFIG_UNICODE for casefold lookup is not imposed. Unknown read-only-compatible
features remain permitted by the read-only feature check. This does not supply
fscrypt decryption. Inode checksum rules already match Linux's creator-OS guard;
they were reviewed without changing it. Modern LARGEDIR size decoding was also
already connected through the C inode reader.

The CLI regression checks actual extracted bytes and rejection without output
over eleven combinations. Before rebuilding the module it reproduced four
incorrect successful extractions. The source inventory revision remains unchanged;
the modified Linux units were already registered as adapted.

Focused acceptance covers `ext`, `ext_stream`, `archive_device`, `device_folder`,
`device_ui`, `codec_probe`, `upstream_baseline` and `upstream_inventory`.
Linux passed all eight in 17.29s (`20261020T162120Z-0fe78e55`). macOS passed
seven in the initial batch; `device_ui` lacked a screen inside the sandbox and
passed separately with native Cocoa access in 28.73s. Windows passed seven in
106.35s (`20261020T162024Z-25cfe890`); `codec_probe` timed out in its unchanged
deadline-delivery test, then passed alone without code/timeout changes
(`20261020T162256Z-958b16d6`). Incremental build trees and compiler caches were
retained. These checks do not establish a full/slow or physical-device gate.

Native Linux ext4 `6.8.0-147-generic` independently matched all eleven cases
(`20261020T162059Z-355afad5`): seven read-only mounts returned the exact payload,
and four unsupported combinations were rejected. All image SHA256 values stayed
unchanged. Temporary images, mounts and the synced review script were removed.
This running kernel is separate from the pinned source revision used for transfer.

## Extended attributes, EA-inode values and POSIX ACLs

The xattr comparison found an unconnected consumer: the archive reader checked
in-inode/external xattr structures without exposing their values. The dynamic
module now connects the complete read getter bodies from the pinned `xattr.c`,
including ibody-before-block precedence, EA-inode acquisition, exact size,
role/refcount and data/entry hashes. Both unsigned and legacy signed name hashes,
plus the original Lustre backpointer compatibility branch, are retained.
`check_igot_inode` comes from the same pinned `inode.c`. Recomparison found these
read bodies unchanged apart from explicit C pointer casts and replacement of
GNU's omitted ternary operand for MSVC. New dependencies are pinned in the
existing source inventory; no sibling checkout is modified.

Linux `acl.c` supplies the complete `ext4_acl_from_disk` body; `fs/posix_acl.c`
supplies `posix_acl_to_xattr`. Their original disk/in-memory/UAPI types and
numeric IDs are adapted through private allocation and initial-user-namespace
services. Published ACL bytes therefore match Linux VFS xattrs, including named
users/groups, rather than exposing the different on-disk ACL representation.
The original C modules and license notices remain at their upstream paths.

Archive byte, raw-inode and buffer services replace VFS/cache ownership.
EA-inode data uses the existing 7-Zip mapping with holes/unwritten extents
rejected inside logical EOF; unused preallocation after EOF is permitted.
Kernel cache insertion, locks and quota state have no archive counterpart.
The namespace/list adapter follows Linux's entry traversal and handler map,
then calls the original getter/converters for each value. It exposes public
user/trusted/security/Hurd/ACL metadata as a privileged image reader, without
applying the host's mount options, capability or LSM policy. Internal inline-data
and fscrypt contexts are not published as public xattrs.

The authoritative Ext handler retains archive properties, extraction and parent
relationships. Owned values appear as alternate streams on files, directories
and each hardlink; root metadata uses a collision-free virtual root item.
Names escape unsafe bytes and disambiguate case/ordinary-filename collisions.
These archive naming/ownership adapters have no usable kernel implementation:
Linux supplies VFS xattr operations, not archive item/stream interfaces. Keeping
them in the existing handler preserves its module boundary and portable SDK
types. Returned metadata streams survive Close/reopen/Release; failed reads and
cancellation propagate through Open without leaving partially usable items.
Standalone LGPL `7zz` keeps its existing reader and does not publish this GPL
metadata path.

The retained creator fixture was made with mke2fs 1.47.0 and Linux
`6.8.0-147-generic` xattr operations, then read through a fresh read-only loop
mount. All listed values, including 9000-byte shared EA-inode values and access/
default ACLs, matched `getxattr`; the image hash stayed unchanged. The manifest
records the creator, native comparison and reproducible generation command's
inputs. Temporary images/mounts were removed. This running kernel supplements
the pinned 2026 source; it does not execute that revision.
An additional native read-only comparison accepted all 27 values for each of
the ordinary, signed-hash and Lustre-compatible variants, with unchanged image
hashes (`20261020T174331Z-894cb0af`). Its temporary review script was removed
locally and from the guest after completion.

CLI tests compare every value for ordinary, signed-hash and Lustre-compatible
images. Mutations reject bad data/hash/entry hash, missing EA flag, zero/multiple
links, zero references, wrong size, self-reference, unwritten EA data and invalid
ACL encoding. The unwritten fixture has valid hashes for zero bytes so it detects
accidental sparse zero filling. ABI tests cover owned/empty streams, parent/type,
input errors, cancellation and reuse. The ArchiveBackend test lists alternate
streams and extracts selected large, ACL, directory and root values through the
real process transport. It caught and fixed property ordering that otherwise
lost the alternate-stream marker during listing.
The subsequent [EROFS metadata iteration](erofs-linux.md#public-xattr-and-acl-values--2026-10-20)
also connected the archive-level marker required by the SDK's item-property
caller and shared the numeric ACL services/UAPI notices. Its focused checks
cover ext and verify metadata filtering separately from ordinary colon filenames.

Nine focused suites (`ext`, `ext_stream`, `ext_xattr_backend`, `archive_device`,
`device_folder`, `codec_probe`, `runtime_cache`, `upstream_baseline`,
`upstream_inventory`) passed on macOS arm64/Qt 6.12 (10.27 s) and Linux x64
(19.74 s, `20261020T174135Z-226768c4`). Windows x64/MSVC/Qt 6.12 passed eight
in that receipt; `device_folder` exceeded its unchanged catalog-readiness wait
and passed alone (4.07 s, `20261020T174404Z-a0be8676`) without a code/timeout
change. After removing an unused external getter wrapper, the five ext/source
suites passed again on macOS and both runners (`20261020T174504Z-3e3fe0e4`).
Incremental build trees and compiler caches were retained. No full/slow, UI,
package, physical-device or native authorization gate is claimed for this change.

## Remaining ext modernization

This iteration does not close the ext row. Encryption still needs a key-service
contract and the corresponding fscrypt read/name/symlink paths; the current
explicit unsupported result prevents ciphertext from masquerading as plaintext.
Remaining feature/creator-OS read semantics and key/trust service integration need
a complete source comparison and integration. These remain required follow-up work,
not accepted permanent limitations. Kernel VFS/page cache, writes and journal
replay remain outside the read-only archive contract.
