# Linux exFAT read-only format module

## Sources and reproducible baseline

Selected on 2026-10-17 from the current mainline Linux tree, rather than the
historical Samsung release:

- Repository: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>.
- Full commit: `58161ead29e50d70a317ea26b472e0c7508e69da`.
- Inventory unit: the complete `fs/exfat` directory, including its headers,
  Makefile and Kconfig; all sixteen files were read or compared for dependencies.
- Original-file SHA256 values: [Linux baseline](../exfat-linux-baseline.json).
  Retrieve files from that commit when reviewing or updating the adaptation.
- Linux notices and GPL-2.0-or-later SPDX identifiers are retained. The source
  license text is in `third_party/linux/LICENSES/preferred/GPL-2.0` and is installed
  with the module. The root MIT license does not replace these notices.

The format ABI reuses the official 7-Zip implementation at commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
<https://github.com/ip7z/7zip.git>. `CPP/7zip/Archive/ArchiveExports.cpp` is copied
unchanged and registered in the existing [7-Zip baseline](../upstream-baseline.json).
`CreateObject`/the handler CLSID come from `Archive/DllExports.cpp` and
`GetModuleProp` from `Compress/CodecExports.cpp`; the module entrypoint adapter
retains only the archive ABI/version boundary. Existing portable 7-Zip COM,
properties, streams, Unicode and time helpers are reused.

## Compared scope, transfer and platform boundary

The source comparison covered the boot/mount path in `super.c`, directory
enumeration and entry-set consumers in `dir.c`/`namei.c`, FAT traversal in
`fatent.c`/`exfat_fs.h`, allocation-bitmap readers in `balloc.c`, timestamps and
checksums in `misc.c`, upcase handling in `nls.c`, and read mapping through
`inode.c`/`cache.c`/`iomap.c`. The complete driver is not ported.

Adapted code remains C under `src/fs/exfat/`:

| Upstream module | Transferred read scope |
| --- | --- |
| `super.c` | `exfat_calibrate_blocksize`, `exfat_read_boot_sector`, `exfat_verify_boot_region` |
| `dir.c` | `exfat_extract_uni_name`, `exfat_get_uniname_from_ext_entry`, `exfat_readdir`, `exfat_get_entry_type`, entry location/read, entry-set loading/validation/release, volume-label lookup/read |
| `fatent.c` | `__exfat_ent_get`, `exfat_ent_get`, `exfat_count_num_clusters` |
| `balloc.c` | Bitmap range testing, allocation/loading/freeing, `exfat_test_bitmap` and `exfat_count_used_clusters` |
| `misc.c` | `exfat_calc_chksum16`, `exfat_calc_chksum32`, `exfat_chain_set`, `exfat_chain_dup`; no mutation transport |
| `exfat_fs.h`, `exfat_raw.h` | Original constants, packed disk structures, read-facing types and geometry/cluster-chain helpers, including `exfat_cluster_walk`/`exfat_chain_advance` |

The platform header replaces kernel allocation, endian helpers and block reads.
Each volume has its own stream context; there is no kernel, FUSE, system mount,
raw-device discovery or write callback. Readahead is omitted as a kernel
performance service; reads remain demand driven. The bitmap word width is
explicitly 64 bits on LLP64 hosts. VFS-only inode/superblock fields are omitted.
Packing is applied around the original raw structures for MSVC and Unix compilers.

`exfatreader.c` supplies volume lifetime, the stream-backed `buffer_head` service,
directory/extraction cursors and the C/C++ boundary. Metadata assignments follow
`namei.c::exfat_find`; stream reads advance through the original cluster walker,
using the retained cursor rather than Linux's VFS/page-cache/extent-cache APIs.
The unwritten tail follows `iomap.c::__exfat_iomap_begin`'s `valid_size` distinction
and is delivered as zero bytes. Timestamp decoding now uses the original Linux
C functions described below; 7-Zip helpers only project UNIX time into FILETIME.

`exfathandler.cpp` implements the new `IInArchive` adapter: filesystem traversal,
properties, callback progress, extraction and test mode. There is no usable
upstream 7-Zip exFAT handler at its pinned revision, and Linux has no 7-Zip COM
adapter. The new code is restricted to that boundary and stream/lifetime services;
the filesystem readers themselves come from Linux. Widgets and `ArchiveBackend`
retain their existing generic format-discovery and operation paths.

Explicit adaptations for untrusted image streams:

- Establish volume/cluster/extent bounds that Linux normally obtains from a
  block device; reject unsupported major revisions. Two-FAT/TexFAT reads are
  restricted to the clean-volume scope described below.
- Preserve bounded FAT-chain counting before traversing root/child directories
  or extracting FAT-chained files; reject ancestor directory aliases/cycles.
- Require complete filename/stream entry sets and matching entry-set checksums;
  propagate malformed-set errors rather than silently omitting their files.
- Preserve UTF-16 names until the 7-Zip boundary and reject path components that
  could introduce traversal, separators or alternate streams during extraction.
- Propagate open/extract cancellation and source/output I/O errors; clean failed
  open state and callback leases before a handler is reused.

Omitted scope is intentional: all writes, allocation/free operations, repair,
deleted-file recovery, VFS mounting/options/inode publication, kernel caches and
bio/iomap scheduling. Upcase loading and case-insensitive lookup are not needed
because the module enumerates original names and 7-Zip selects indexed entries;
no filesystem name lookup API is exposed. The Linux bitmap reader remains part
of volume acceptance. `IOutArchive` and `IInArchiveGetStream` are not advertised;
ordinary extraction is the available file-data interface. This is a bounded
read-only source adaptation, not a complete source transfer of the Linux driver.

Formatting is separate: copied Linux bodies retain kernel formatting; new C++
and integration headers use the repository formatter. No sibling upstream files
are changed.

## Deployment and validation

The application build stages the separate format module into the existing
dynamic engine's `Formats` directory. Installation places it beside that engine
and installs its GPL notice. The standalone `7zz` executable cannot load it;
the application's existing dynamic `7z` runtime is required. An explicitly
overridden external engine needs its own compatible module deployment.

`exfat` exercises the exported ABI and the official process engine on a local,
deterministic volume. It covers nested/Unicode and maximum-length names, entry
sets crossing a fragmented directory boundary, fragmented file contents, empty
files, timestamps, unwritten tails, malformed boot/entry checksums, ranges,
cycles, traversal names, truncated input, open/extract cancellation, output errors
and source-image preservation. `exfat_backend` covers real application discovery,
read-only capability, listing, test and extraction using a private runtime.
Existing external-codec suites cover the shared module loader separately.

Focused validation on 2026-10-17, using the retained incremental builds and
compiler caches:

- macOS arm64, Qt 6.12: `exfat` and `exfat_backend` PASS (18 Qt cases including
  setup/cleanup, CTest 0.84 s). The final reader includes the 4096-byte logical
  sector case. `external_codecs` and `codec_probe` also passed in the immediately
  preceding four-suite run (6.97 s).
- Linux x64, Qt 6.12: final two exFAT suites PASS, CTest 0.29 s.
- Windows x64/MSVC, Qt 6.12: final two exFAT suites PASS, CTest 2.17 s, without
  task-owned compiler warnings. A previous CLI test failed because its output
  used the Windows console code page; the test now requests `-sccUTF-8`, matching
  the existing backend transport. Backend integration already passed that run.
- Both guests tested the same source snapshot SHA256
  `66de622532823f5b44c28437591d7de61f74128894df247eff51fbf9bdb1ad69`.
  Run receipt: `20261017T190349Z-1b610fc7`; the earlier loader/codec-probe run is
  `20261017T190123Z-1628baf1`. Those two unchanged shared-loader suites passed
  on both guests in the earlier run.
- Original Linux-file hashes and the byte-identical 7-Zip archive exports were
  checked against their recorded sources. Generated macOS install rules place
  the module under the bundle runtime's `Formats` and include its GPL notice.

The source transfer is limited to the read scenario described above. Runtime
evidence covers generated local images and actual official-engine/application
backend consumers on each OS; it does not establish recovery, physical-device,
all-image compatibility or installed-package acceptance. The macOS system
formatter requires a special device and did not format the temporary regular
file; no device was mounted or formatted. UI geometry/rendered parity and long
suites are outside this focused nonvisual validation. A full gate was not needed
because the format module is isolated and its direct loader/backend consumers
were covered.

## Consistent TexFAT images (2026-10-17)

The preceding single-FAT implementation was committed as `f356400`. This
follow-up extends the same read-only engine module; it does not implement
transaction replay or writes. The Linux source revision/baseline above is
unchanged.

Before implementation, complete bodies and consumers were compared again:
`super.c::exfat_read_boot_sector`, `exfat_fs.h::FAT_ENT_OFFSET_SECTOR`,
`fatent.c::__exfat_ent_get`/`exfat_ent_get`, `balloc.c::exfat_load_bitmap`/
`exfat_allocate_bitmap`/`exfat_test_bitmap_range`, the root/directory and file
cluster walkers, plus `dir.c`'s entry classification/validation. The selected
Linux implementation stores two FAT offsets but reads FAT1 and selects only
bitmap identifier zero; its bitmap loader assumes contiguous physical sectors.
There is no usable active-pair selection implementation in that module to copy.

The narrow additions implement the public Microsoft
[exFAT specification](https://learn.microsoft.com/en-us/windows/win32/fileio/exfat-specification),
sections 3.1.13.1, 3.1.16, 4, 7.1 and 7.10:

- Retain `ActiveFat` from the **Main** Boot Sector; backup VolumeFlags are stale.
  All FAT reads, including root and bitmap chains, use the selected FAT offset.
- Enumerate and require the appropriate bitmap identifiers without duplicates;
  load only the bitmap paired with the selected FAT. The inactive pair is stale
  and is neither interpreted nor used as an error fallback.
- Follow the active bitmap's FAT chain with the existing Linux chain walkers,
  including fragmented bitmap extents. Bound its declared allocation by the
  cluster heap; check chain length/cycles and the allocation of its actual
  clusters rather than assuming a contiguous bitmap.
- Continue skipping benign primary entries, including TexFAT Padding, through
  the existing upstream `exfat_get_entry_type`/`exfat_readdir` branches. Unknown
  critical secondary entries remain unsupported; security metadata is not
  exposed or applied to extracted files.
- Reject two-FAT images marked `VolumeDirty` or `MediaFailure`, and reject the
  invalid single-FAT/second-active combination. Do not clear flags, switch pairs,
  infer a replacement active pair or attempt repair.

Here, consistent-image support means clean status flags plus the retained
structural checks for the selected data view. It is not a proof of every
filesystem invariant or validation/recovery of an interrupted transaction.

The fixture set distinguishes the two copies with different data-cluster chains
and bitmap contents, and deliberately gives backup VolumeFlags a different
selection. Tests exercise either active pair, fragmented bitmaps with an
allocation bit in a later bitmap cluster, 4096-byte sectors, padding, nested
Unicode files and original-image preservation. Failure cases cover dirty/media
status, missing/duplicate bitmap entries, active root/bitmap cycles, an
unallocated active root, refusal to use stale data after an active-file error,
and cancellation followed by reuse. The official process engine and
`ArchiveBackend` exercise both choices with actual listing/test/extraction.

Focused validation passed on macOS, Linux and Windows:

- macOS: incremental build of `exfat_tests`, `exfat_backend_tests` and
  `sevenzip_exfat_runtime`; `exfat` and `exfat_backend` passed, 44 Qt cases,
  4.37 seconds. Local receipt: `build/review/texfat/local-last-test.log`.
- Linux/Windows: `scripts/verify.py --tests '^(exfat|exfat_backend)$'
  --platform all --skip-runtime` built the selected targets in the existing
  guest worktrees and passed both suites, 1.12/3.95 seconds respectively.
  Run receipt: `build/runners/hyperv/20261017T191621Z-00d9c19f/run.json`;
  both guests verified source SHA-256
  `9d79cb5628f8c0dd585c9e01f243779a4fdfcc14db8aa796b542140ceaea707a`.

Validation used generated images, not real Windows CE volumes. No rendered UI,
installed-package, full-suite or long-suite acceptance is claimed. Existing
incremental builds and compiler caches were retained; no new archive transport
or UI changes are involved.

## Cached FAT cancellation (2026-10-19)

The [Linux filesystem review](filesystems-linux.md) revisited the same pinned
Linux revision. For this scenario, complete bodies and callers were compared:
`fatent.c::__exfat_ent_get`, `exfat_ent_get`, `exfat_count_num_clusters`,
`exfat_fs.h::exfat_cluster_walk`/`exfat_chain_advance`, directory entry-set
loading, bitmap chain consumers, and the local open/extract stream adapters.
The upstream traversal, cluster validation, loop bound and buffer ownership are
retained. The additional userspace adaptation checks the existing cancellation
service in `exfat_ent_get`, including reads satisfied from its FAT-sector cache.
Kernel traversal has no archive callback to copy; checking only in `sb_bread`
left cached cyclic walks without a cancellation checkpoint.

The ABI regression requests cancellation when input data returns during
extraction of a file with a cyclic FAT chain. It requires `E_ABORT`, no published
file bytes and successful reuse for a valid image. It does not assert a callback
count or source structure. With the new checkpoint removed, the regression
failed: extraction completed with a data-error result instead of cancellation.
With the checkpoint restored, the focused ABI/backend suites passed on macOS.
The remaining runtime/platform evidence is recorded in the review entry.

## Timestamp decoding (2026-10-21)

Compared complete `misc.c:exfat_get_entry_time`, `exfat_adjust_tz`,
`exfat_tz_offset`, their `namei.c:exfat_find` callers, inode timestamp consumers,
`super.c:exfat_init_fs_context` defaults and `kernel/time/time.c:mktime64`.
The previous independent C++ DOS conversion hid zero/normalized dates and
increments above 199; timestamps without UTC-valid also depended on the host
timezone instead of the driver's default zero offset.

Original `exfat_get_entry_time` and `exfat_adjust_tz` bodies are now compiled
as C in `src/fs/exfat/misc.c`, using the existing authoritative
`src/kernel/time/time.c:mktime64`. The latter retains its GPL-2.0 notice and
Linux source baseline; its original source hash is added to the exFAT manifest.
All seventeen previous baseline source/license hashes were verified again.
The module remains a separate GPL format module; standalone 7zz is unchanged.

The only mount-boundary adaptation is `exfat_tz_offset`: archive reads have
neither explicit mount options nor `sys_tz` services, so it returns the original
zero-initialized default offset. Explicit disk offsets retain both signed
branches. The C/C++ adapter preserves little-endian input and seconds/nanoseconds;
the existing SDK UNIX-time conversion supplies FILETIME. All three public
timestamp properties use the same decoder, with no increment for access time.
Write-side timestamp conversion, VFS truncation and mount option services are
omitted. Existing payload/chain/bitmap readers and TexFAT adaptations are retained.

Post-change comparison confirmed complete selected decode branches and all
three property consumers. ABI regressions assert exact FILETIME for unknown
offsets (including ignored offset bits), both signed offset extremes, 10ms
fractions, increment 255, zero date, leap-day and clock overflow. Separate child
processes repeat these checks with `TZ=UTC0` and `TZ=HST10`, protecting against
host-dependent conversion. Existing extraction/cancellation/malformed-volume
and process-engine/backend checks remain part of the focused suites.

`exfat` and `exfat_backend` passed on macOS arm64/Qt 6.12 (2.54 s), Linux x64
and Windows x64/MSVC/Qt 6.12, remote receipt `20261021T014812Z-4572b3cc`.
Native exFAT `6.8.0-147-generic` confirmed all eight images through read-only
loop mounts: exact mtime/atime and file payload, receipt
`20261021T014934Z-8faff0f4`. Images remained unchanged and mounts/temporary
fixtures were removed. The older running kernel supplements pinned source
comparison; native creation-time export was not checked. Incremental builds
and compiler caches were preserved. Full/slow suites, UI and physical devices
were not checked; this scenario does not close the overall format audit.

## Volume-label read feature (2026-10-21)

The pinned Linux driver also implements `dir.c:exfat_get_volume_label_dentry`
and `exfat_read_volume_label`, consumed by `file.c:exfat_ioctl_get_volume_label`.
Complete bodies, types, root-chain setup/validation, entry classification,
cached-entry lifetime and `nls.c` output consumers were compared before adding
this previously absent read feature. All eighteen baseline hashes were verified.

Both original C readers now live in `src/fs/exfat/dir.c`; root-chain walking,
first-match selection, deleted/unused entry handling, length limit, endian
conversion and buffer release branches are retained. Omitted scope consists of
write-side free-entry hints and their root VFS inode, plus the VFS mutex:
the archive owner serializes its per-volume source. Root chain counting still
precedes the scanner, and the existing FAT adapter uses the selected active pair.
The write-label function and ioctl userspace transport are not copied.

The C volume owner reads and retains the label during Open. The existing 7-Zip
adapter exposes `kpidVolumeName` through the standard archive-property table;
the previously available physical size is now discoverable through that table
too. UTF-16 is retained until the SDK string boundary, including surrogate pairs.
The first NUL terminates presentation as in Linux's label ioctl converter.
Absence is an empty label; after Close or failed Open the property is undefined.
This is eager archive metadata validation: a malformed first label or a source
error rejects Open, whereas Linux reports that error when its label ioctl runs.
Source failure/cancellation HRESULTs survive the C reader and cleanup.

Post-change source comparison covered the transferred readers, their existing
dependencies and every new volume/ABI consumer. Twelve label scenarios cover
first/fragmented-root placement, both TexFAT active pairs, 4096-byte sectors,
Unicode/maximum/empty/absent/deleted labels, first-match duplicates, unused
termination and embedded NUL. ABI checks verify label, unchanged file payload,
failed-open cleanup, source failure/abort and reuse. The official process engine
lists the Unicode label and tests/extracts the image for exFAT and both TexFAT
pairs. Existing backend consumers remain covered.

Final `exfat`/`exfat_backend` PASS on macOS arm64/Qt 6.12 (1.91 s), Linux x64
(1.20 s) and Windows x64/MSVC/Qt 6.12 (3.65 s), receipt
`20261021T015503Z-20b73622`. Incremental builds and compiler caches were
preserved. Full/slow suites, UI appearance and physical devices were not checked.

Native `6.8.0-147-generic` lacks this ioctl: a read-only mounted fixture returned
ENOTTY, receipt `20261021T015619Z-6a18da24`. Native label acceptance is therefore
unverified; the implementation is compared against the newer pinned source.
The image remained unchanged and mount/temporary exporter/fixtures were removed.
No package installation was needed. This feature does not close the remaining
format audit or change standalone 7zz's exFAT boundary.

## Allocation statistics and geometry (2026-10-21)

Compared complete `balloc.c:exfat_count_used_clusters`, bitmap loader and
root/active-pair consumers, `super.c:exfat_statfs`/`__exfat_fill_super`,
`include/linux/bitmap.h:BITMAP_LAST_WORD_MASK`, `bitops.h:hweight_long` and
`lib/hweight.c` software helpers before integration. Post-change comparison
covered every selected branch and the archive-property consumers. All twenty-one
source/license hashes in the extended manifest were verified.

The original count loop now runs after loading the selected bitmap and stores
`used_clusters` in the original volume state. Sector transitions and the final
partial-word mask are retained. Bitmap word/pointer/stride and mask constants
use the existing explicit 64-bit service on LLP64 as well as LP64. A per-sector
archive cancellation checkpoint also covers cached scans. Original
`__sw_hweight32`/`__sw_hweight64` bodies live in `src/lib/hweight.c`, compiled
as C with their GPL-2.0 notice. Kernel export/include transport is omitted;
the fixed word-width service chooses the software 64-bit branch without CPU
instruction requirements. The 8/16-bit helpers are not dependencies and are
not copied. All new Linux code remains in the separate GPL exFAT module.

The C/C++ adapter publishes free bytes, cluster size and sector size through
the standard archive-property table. Free bytes use the original `exfat_statfs`
data-cluster/free-cluster expressions; kernel device fsid, VFS inode/dentry
publication and statfs transport are omitted. Boot PercentInUse is not an
allocation count and is not substituted for the bitmap. Existing chain bounds,
bitmap acceptance, payload and TexFAT behavior remain in place.

Nine ABI scenarios cover 512/4096-byte sectors, both active FAT/bitmap pairs,
fragmented multi-sector bitmaps, a partial final word with deliberately allocated
padding outside the heap, and the last valid cluster inside that word. Tests
assert free bytes and geometry, including a 1024-byte cluster with a 512-byte
sector, scaling with allocation units and clearing properties on Close.
Existing source error/cancellation, malformed input,
process-engine listing/extraction and ArchiveBackend checks are retained.

Final `exfat`/`exfat_backend` PASS on macOS arm64/Qt 6.12 (1.97 s), Linux x64
(1.26 s) and Windows x64/MSVC/Qt 6.12 (3.56 s), receipt
`20261021T020231Z-59be93bd`. Earlier eight-scenario checks also passed on
all three platforms before adding the distinct cluster/sector geometry case.

Native exFAT `6.8.0-147-generic` confirmed three read-only images through
`statvfs`: base, partial-word padding and the last valid cluster, receipt
`20261021T020105Z-499c71eb`. Exact free-cluster counts, cluster size and file
payload matched; images remained unchanged, and mounts/temporary exporter/
fixtures were removed. This native check uses 512-byte sectors/clusters and
does not establish native TexFAT acceptance. Incremental builds and compiler
caches were retained. Full/slow suites, UI and physical devices were not checked.
