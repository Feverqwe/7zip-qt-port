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
| `dir.c` | `exfat_extract_uni_name`, `exfat_get_uniname_from_ext_entry`, `exfat_readdir`, `exfat_get_entry_type`, entry location/read, entry-set loading/validation/release |
| `fatent.c` | `__exfat_ent_get`, `exfat_ent_get`, `exfat_count_num_clusters` |
| `balloc.c` | Bitmap range testing, allocation/loading/freeing and `exfat_test_bitmap` |
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
and is delivered as zero bytes. DOS conversion uses existing 7-Zip time helpers;
signed quarter-hour offsets follow `misc.c::exfat_adjust_tz`.

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
