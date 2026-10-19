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
layouts while retaining refusal of journal recovery and filesystem encryption.
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

## Remaining ext modernization

This iteration does not close the ext row. Encryption still needs a key-service
contract and the corresponding fscrypt read/name/symlink paths; the current
explicit unsupported result prevents ciphertext from masquerading as plaintext.
Fs-verity verification and remaining feature/creator-OS read semantics need a
complete source comparison and integration. These remain required follow-up work,
not accepted permanent limitations. Kernel VFS/page cache, writes and journal
replay remain outside the read-only archive contract.
