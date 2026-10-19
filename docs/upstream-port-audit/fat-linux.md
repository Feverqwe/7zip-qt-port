# FAT read paths compared with Linux

Part of the [Linux format review](filesystems-linux.md), 2026-10-19.
This integrated iteration updates BPB-based FAT12/16/32 geometry, directory-name
recovery and size-bounded file reads. It does not close the complete FAT row:
the additional compatibility work listed below remains in the active goal.

## Sources and retained ownership

Linux repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, revision
`58161ead29e50d70a317ea26b472e0c7508e69da`.
The [source inventory](../fat-linux-baseline.json) records complete `fs/fat` units.
Read-side comparison covered BPB decoding and superblock geometry in `inode.c`,
FAT entry loads in `fatent.c`, mapping and EOF handling in `cache.c`, LFN/short
name enumeration in `dir.c`, inode initialization, time conversion in `misc.c`
and supporting `fat.h` types. Write, allocation, truncate, NFS and VFS publication
are not transferred into the archive engine.

The authoritative handler is the complete original
`CPP/7zip/Archive/FatHandler.cpp` from <https://github.com/ip7z/7zip.git>, revision
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`, adapted under its original path in
`src/`. All archive properties, progress callbacks, directory recursion, streamed
extraction and registration remain connected. Original `CClusterInStream`,
`CRecordVector`, buffer, copy, progress, property and text helpers are reused.
The [7-Zip baseline](../upstream-baseline.json) selects this adaptation for both
the dynamic engine and standalone `7zz`. No sibling source checkout is changed.
Formatting follows the project style; functional comparison uses an independently
formatted copy of the pinned original to separate formatting from behavior.

## Integrated changes

BPB parsing no longer depends on executable bootstrap instructions, their jump
offset or zero-filled FAT32 reserved fields. FAT32 is selected by the FAT length
fields as in Linux, including a declared fixed-root area when present. Logical
sector and cluster checks allow Linux's 4096-byte sectors and 128-sector clusters.
Multiple FAT copies are bounded by volume geometry instead of an arbitrary limit
of four. Invalid media values are rejected; a disagreeing first FAT media entry
produces a warning instead of preventing access to otherwise readable files.

Metadata offsets and FAT capacity are calculated in 64 bits before narrowing.
The accessible cluster count is capped by the physical FAT capacity before its
range check. Invalid active-FAT indexes fall back to the first table, including
the former off-by-one case equal to the number of copies. Active-FAT selection is
a retained 7-Zip extension; the compared Linux read path uses the first table.
Header-size reporting also retains 64-bit arithmetic for directory clusters.
The selected FAT range must fit the source stream before its decoded table is
allocated; data-area truncation retains the original extraction error behavior.

Directory scanning skips invalid/orphan LFN slots and resynchronizes at a new
last-slot record. A bad sequence cannot hide subsequent short entries or prevent
opening the whole volume. Reserved LFN type/start fields do not discard an
otherwise usable name. Only the exact LFN attribute is treated as an LFN slot;
volume entries remain outside the file tree even with a stale directory bit.
The alias checksum uses the stored bytes before the escaped `0x05` lead byte is
converted to `0xE5` for display. Directory block acquisition and the original
warning reporting remain unchanged.

File reads stop at the directory entry's size, allowing preallocated empty files
and unused allocated tails. Missing required clusters, directory-cluster aliases
and repeated clusters within the required file prefix are rejected. The original
stream vector supplies data; a temporary copy uses the existing vector sort to
detect repeated cluster numbers without modifying shared FAT state. The number
of required clusters is checked before reservation. Unused chain tails are not
traversed or copied into output. These corruption guards do not claim identical
recovery behavior for every damaged filesystem Linux can mount.

## Validation

Deterministic local fixtures encode fragmented FAT12/16/32 files, Unicode LFNs,
empty files, sector-crossing FAT12 entries and LFN sequences, 4096-byte sectors,
512 KiB clusters, five FAT copies and FAT32 reserved/root-area variants. Process
checks exercise both supplied engines, exact extraction and source preservation.
Active-FAT checks preserve valid second-copy selection and cover fallback when
the index equals the copy count or exceeds it.
Recovery cases include invalid IDs/order/checksums, orphan/restarted sequences,
reserved LFN fields, escaped aliases and stale volume flags. Invalid geometry,
short/free/cyclic chains, oversized files and truncated data must report errors.
The tests check observable results rather than source text or private symbols.

Eight focused suites (`fat`, `cramfs`, `erofs`, `upstream_baseline`,
`upstream_inventory`, `runtime_cache`, `external_codecs`, `codec_probe`) passed
on macOS arm64/Qt 6.12, Linux x64 and Windows x64/MSVC/Qt 6.12. Guest receipt:
`20261019T074547Z-4d8060c3`. Localized checks of FAT and source provenance also
passed on all three platforms after the final C++ changes; guest receipt:
`20261019T075026Z-b8bdb4d7`.
The final active-FAT regression passed locally and on both guests in receipt
`20261019T075125Z-07319c31`. Upstream macro blocks retain their original readable
structure under local formatter exclusions; this presentation change is separate
from the functional comparison.

Supplementary read-only loop mounts using Linux `6.8.0-146-generic` confirmed
names and exact payload hashes for FAT12/16/32, unused chain tails, nonbootable
BPBs, reserved LFN fields, escaped aliases and invalid/restarted LFN sequences.
All nine mounted images returned the same expected file bytes. This older guest
driver is a runtime oracle for those fixtures; the pinned 2026 source remains the
modern reference. Mounts and temporary images were removed. No physical-device,
UI, full/slow suite or clean-package acceptance is claimed. Existing build trees
and compiler caches were retained.

## Remaining FAT compatibility work

The source comparison also identified Linux's optional DOS 1.x floppy geometry
fallback, which has no BPB and is a separate recognition/read scenario. It must
be integrated and checked before completing the FAT row. Linux does not require
the modern boot signature; the current archive detector still does. Recognition
must retain sufficient evidence to avoid claiming unrelated raw images as FAT.

Short-name codepage policy, full UTF-16 name conversion/padding behavior, invalid
date normalization and the remaining directory-size/record recovery boundaries
need comparison and behavioral checks. These are continuing work, not permanent
format restrictions accepted in place of the requested modernization. Kernel
mount-option policy, VFS services and writes remain outside the read-only goal.
