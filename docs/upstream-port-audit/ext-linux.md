# ext2/3/4 read-path review against Linux

Part of the [Linux format review](filesystems-linux.md), 2026-10-19. This first
integrated ext iteration covers classic indirect mapping, sparse reads, extent
tree boundaries, directory record boundaries and archive stream consumers.
The complete ext row remains open for the modern features listed below.

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
The existing short inline-data fallback is retained pending complete inline
support; it is not evidence that the full feature works. Conflicting inline and
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

## Remaining ext modernization

This iteration does not close the ext row. Source-visible work still includes
complete inline inode/xattr data, META_BG/distributed or larger group descriptors,
BIGALLOC geometry and accounting, uninitialized inode groups and large-directory
inode sizes. CRC32C/seed-based superblock, group, bitmap, inode, directory and
extent checksum handling is incomplete in the original handler. Inode time-extra
field boundaries and feature/creator-OS metadata rules need updating as part of
that integrated scope. Encryption/key and incompatible-feature semantics also
need explicit comparison so ciphertext or unknown layouts cannot masquerade as
ordinary successful reads. These are required follow-up work, not accepted
permanent limitations. Kernel VFS/page cache, writes and journal replay remain
outside the read-only archive contract.
