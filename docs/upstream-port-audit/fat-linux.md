# FAT read paths compared with Linux

Part of the [Linux format review](filesystems-linux.md), 2026-10-19.
The two integrated iterations cover BPB-based FAT12/16/32 geometry, directory
and file reads, then DOS 1.x recognition, names, dates and archive consumers.
Other filesystem and shared-format rows remain in the active goal.

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

## Linux compatibility module

The next integrated iteration connects the following original C implementations,
with full original bodies and required tables retained under their original paths:

- `fs/fat/inode.c`: `fat_bpb_is_zero`, `fat_read_static_bpb`, floppy defaults
  and the decoded BPB type. The userspace BPB adds the table's media value so the
  archive header can retain that property. A bounded adapter creates a private
  normalized BPB for the existing 7-Zip parser; the source is never rewritten.
- `fs/fat/dir.c`: `fat_parse_short`, short-name conversion helpers and bounded
  Unicode output. The archive selects Linux's default CP437 and WinNT display
  policy, honoring base/extension lowercase flags, including non-ASCII mappings.
- `fs/fat/misc.c`: `fat_time_fat2unix`, epoch/leap-year tables and timezone
  calculation. Local wall-clock output passes through the existing 7-Zip host
  FILETIME adapter; no kernel timezone or mount state is recreated.
- `fs/nls/nls_cp437.c`: the complete conversion/case tables and conversion
  functions. `fs/nls/nls_base.c` retains the UTF-8 encoding table,
  `utf32_to_utf8`, `get_utf16` and `utf16s_to_utf8s` dependency closure.
- `include/uapi/linux/msdos_fs.h`: disk structures and constants, with only
  host-header imports replaced. Layout assertions check the on-disk ABI.

C remains C11. The private compatibility header supplies only the required
kernel service/type boundary. Kernel NLS's 16-bit `wchar_t` is named
`fat_wchar_t` to avoid conflicting host C library types. Logging, module
registration, mount policy and VFS operations are omitted. One string-literal
pointer cast addresses host C compiler signedness diagnostics. Source comparison
verified the eight transferred function/table ranges against the pinned snapshot
with these explicit adaptations. Copyright and license notices are retained;
the UAPI syscall exception is included with installed notices.

The original 7-Zip FAT handler remains authoritative. `sevenzip_fat` links the C
compatibility routines and archive ABI without unRAR code, following the existing
GPL format-module boundary. Dynamic-engine FAT registration is omitted to avoid
duplicate handlers; CMake deployment and Unix/Windows installation supply the
module. Standalone `7zz` retains the common BPB/chain fixes and its classic reader;
DOS 1.x, Linux NLS and date normalization belong to the application's module.
No independent FAT decoder or compression algorithm is introduced.

Automatic recognition uses the original archive detector interface with a
start-of-stream rule. Modern volumes do not require executable boot code or the
`55 AA` signature. DOS 1.x additionally requires the original bootstrap pattern,
zero BPB fields and one of Linux's four known floppy sizes. Unrelated raw images
and incomplete metadata are rejected. The accepted FAT32 cluster-count boundary
also includes Linux's final addressable entry; maximum-size volumes were not
materialized for runtime testing.

LFNs end at the first UTF-16 NUL; trailing slot padding does not change a name.
Linux's converter combines surrogate pairs and skips isolated surrogates. Raw
unsanitized LFN slots are not returned as an alternate name: archive consumers
use the same canonical `GetProperty` name. The original parent/tree ABI remains.
Dates with zero day/month and the 2100 non-leap-year boundary follow Linux's
normalization. FSINFO sector zero falls back to sector one. Free-space reporting
recounts the decoded FAT as Linux's default statfs does, excluding the two
reserved entries and ignoring stale cached FSINFO counts.

The read-side comparison also reviewed fixed/clustered roots, directory size
limits, end markers, volume/deleted/dot entries, aliases and required chain ranges.
Existing archive recovery and unsafe-cycle guards remain explicit adaptations;
this does not promise identical recovery of every damaged tree. Codepage and
timezone choices are archive policy where the disk records no such choice.
Kernel mount-option configuration, VFS lookup/casefold policy, NFS, writeback,
ioctl/device management and writing are outside this read-only archive contract.

## Compatibility and consumer validation

The process suite exercises automatic detection of all four DOS 1.x geometries
and signatureless modern volumes, rejects unrelated legacy candidates, extracts
CP437/NT-case and supplementary-plane names, and checks lone-surrogate/padding
recovery. Date checks cover the zero date and 2100-02-29 normalization. Stale
FSINFO and zero reserved FAT entries exercise actual free-space output through
both engines. All process cases preserve their source images.

Module ABI checks cover cancelled/failed opens, extract cancellation, input/output
failures, reuse and exact output. A fragmented seekable stream retains its source
after archive Close/reopen/Release, including backward and cross-cluster reads.
The test honors the stream contract allowing short reads at cluster boundaries.
ArchiveBackend discovers read-only FAT and automatically lists, tests and extracts
a DOS 1.x image with Unicode/spaces in the image path and exact source preservation.

Thirteen related suites passed locally. On Linux/Windows, the initial batch's
other twelve suites passed, while a new ABI assertion incorrectly assumed one
Read would fill the requested buffer. After correcting that test, both FAT suites
passed on both guests (`20261019T080628Z-a0200428`). A subsequent Windows run timed
out during codec discovery. A read-only system snapshot taken afterward showed
available memory and low CPU load, which does not establish the incident's cause.
Both FAT suites passed locally and on both guests after the final free-space
changes (`20261019T081059Z-e4321383`). Source baseline/inventory checks also passed.
No production timeout was increased.

Eleven additional native read-only mounts on Linux `6.8.0-146-generic`, using
`dos1xfloppy`, CP437, WinNT name display and UTC dates, confirmed all four legacy
geometries, signatureless BPBs, CP437, surrogate/padding names and normalized
dates with exact file bytes. The mounts were unmounted and temporary images
removed. They supplement the pinned modern-source comparison; they do not claim
runtime execution of the 2026 kernel. No full/slow, physical-device, UI or
clean-package acceptance is claimed. Incremental builds and caches are preserved.
