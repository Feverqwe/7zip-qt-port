# Read-only EROFS from Linux

## C++23 dependency facade

The local Linux EROFS facade owns mounted volumes and file handles through RAII,
uses spans for file data and returns `std::expected` results. Read failures retain
their processed-byte count for the existing archive-stream accounting. Files
borrow the mounted volume; the reader and its source callbacks outlive it.
Source callbacks have explicit nonthrowing contracts. Existing missing-device
fallback, transport errors, cancellation and partial-read policy are preserved.
The filesystem decoding algorithms and C decompression dependencies are unchanged.

Linux GCC/UBSan found a host-C boundary issue in `erofs_listxattr_foreach`: a
normal xattr has no long-prefix infix, so the original Linux body calls `memcpy`
with a null source and zero length. The complete body, caller and prefix setup
were compared with the pinned Linux unit. The adaptation skips that empty copy;
xattr assembly, validation and original baseline hashes are preserved. Existing
module xattr fixtures exercise this path with sanitizers.

## Raw DEFLATE through official zlib — 2026-10-24

EROFS raw DEFLATE now uses the unchanged zlib 1.3.2 C subset already shared
with Zisofs and SquashFS. Repository, full commit, original-unit SHA256 and
local owners remain in the [shared manifest](../zisofs-zlib-baseline.json).
The repeatable reference-verify/reference-review commands in the
[Zisofs audit](isofs-linux.md) apply to this same baseline; future updates
must include all three consumers. Original notices remain intact, and the zlib
license is installed with the EROFS notices on Unix and Windows.

Before substitution, the complete pinned Linux `decompressor_deflate.c`,
`z_erofs_fixup_insize`, mapper/full-prefix flags, current decode callback,
SDK Deflate decoder and its buffer/finish services were compared. The original
Linux body already calls zlib, but its page switching, pool/workspace and kernel
API cannot serve a contiguous archive buffer directly. The small local
`erofs_decode_deflate` in `src/erofsnativedecoders.c` connects the public API;
it introduces no decoding algorithm. Linux source hashes and mapped owners
remain in [the existing inventory](../erofs-linux-baseline.json).

Both modes require exactly the requested output length. Full mode additionally
requires DEFLATE stream completion; a private one-byte guard rejects output
overflow while allowing an end marker after an exactly filled destination.
Prefix mode may stop before the stream ends. Neither mode requires all source
padding to be consumed. Existing config/header validation, leading-zero fixup,
read-only bounds, mapping, source transport and cancellation remain in their
owners. The C++ callback retains allocation-error translation. SDK Deflate,
Bitl and LzOutWindow bodies are removed from this module's build composition;
the general archive codecs retain their original implementations.

After integration the full/prefix callers and exact-size/error branches were
compared again. This codec substitution is source-complete within the existing
read-side scope; it adds no recovery mode. Corrupt-stream recovery differences
are permitted by the task, but this does not relax completion or validation.
Runtime validation is recorded separately below.

The six focused suites passed on macOS arm64, Linux x64 and Windows x64/MSVC.
The full/prefix/guard regression, 480 valid-stream comparisons, historical
archive ABI comparison, first Windows discovery timeout and successful final
rerun are recorded with receipts and scope in the
[shared zlib validation](squashfs-linux.md). Source transfer completeness and
runtime evidence remain separate; no recovery, full/slow, native mount,
release-install or performance acceptance is inferred.

## MicroLZMA through liblzma — 2026-10-24

EROFS now uses the unchanged public `lzma_microlzma_decoder`, `lzma_code` and
`lzma_end` APIs from XZ Utils 5.8.4, repository
`https://github.com/tukaani-project/xz.git`, commit
`d3e650e63c110e830fd5391e7f8b45df0b91d3da`. The original-file hashes and local
owners are recorded in [the liblzma baseline](../erofs-liblzma-baseline.json).
The selected C sources and headers under `third_party/xz` are unchanged and
retain their 0BSD license and author notices. The upstream CMake file is a
reference-only inventory unit. The local static target compiles MicroLZMA,
LZMA1/LZ and common stream/allocation services as C11; encoders, other formats,
other filters, tools, threaded services and standalone assembly modules are omitted. Public
headers include declarations for those unused APIs but their implementations
are not linked. Symbols are hidden on Unix and the Windows API uses static
linkage; no system liblzma or additional runtime DLL is required. Both install
layouts include the liblzma license.

Source comparison covered the Linux `xz_dec_microlzma_*` wrapper, its LZMA
properties, range coder, dictionary and completion branches; the liblzma
MicroLZMA wrapper, LZMA1/LZ decoder and common stream services; and the EROFS
configuration, padding removal, extent map, native callback and archive stream
consumers. The existing configuration range, compressed extent bounds,
full-versus-prefix size contract and successful output-length check remain.
Linux and liblzma both accept `lc + lp <= 4` and `pb <= 4` here. Allocation
failure maps to `-ENOMEM`; other unsuccessful decodes map to `-EILSEQ`.
liblzma allocates a dictionary bounded by the existing EROFS 8 MiB limit,
whereas Linux's former `XZ_SINGLE` mode used the caller's output as dictionary.
No speed or reduced-memory claim is made.

The Linux XZ Embedded decoder, private/public headers and local host adapter
have been removed. Their original hashes remain as historical reference units
in [the Linux baseline](../erofs-linux-baseline.json), explicitly marked as
replaced dependencies. EROFS metadata/layout handling and the other codecs
are unchanged; general 7-Zip LZMA and XZ consumers are outside this replacement.
The earlier read-side source-closure boundaries below still apply.

The user explicitly permits differences in recovery behavior for this task.
This is not byte-for-byte equivalence on malformed input: with dictionary
65536, five-byte input `a2307ffc00` and a requested one-byte prefix, liblzma
can return `a` where Linux rejects the block. Prefix decoding does not verify
the remainder of a compressed extent, and must not be treated as a successful
full integrity test. Full decoding still requires stream completion and the
exact compressed/uncompressed lengths. Existing source-read, extraction-path,
bounds, cancellation and result-reporting boundaries remain in place. No new
EROFS recovery mode, repair, journal replay or source write is introduced.

Reproduce verification and upstream review using the existing tooling:

```sh
git clone https://github.com/tukaani-project/xz.git /tmp/erofs-xz-review
python3 scripts/upstream_baseline.py reference-verify \
  --manifest docs/erofs-liblzma-baseline.json --upstream /tmp/erofs-xz-review
python3 scripts/upstream_baseline.py reference-review \
  --manifest docs/erofs-liblzma-baseline.json --upstream /tmp/erofs-xz-review \
  --new-revision <proposed-full-commit> --output /tmp/erofs-xz-review.md
```

Review upstream dependencies/omissions and compare the baseline, proposed
revision and local owners before updating. Original hashes pin the original
bytes, independently of safe changes to the integration.

Validation on 2026-10-24, feature branch `feature/erofs-microlzma` based on
`4f3b80b4`: affected targets built and `erofs_reader`, `erofs_module` and
`erofs` passed on macOS arm64, Linux x64 and Windows x64. Existing official
creator images cover compressed layouts and CLI extraction; module regressions
now also exercise MicroLZMA source/output failures, cancellation/reuse and
seekable streams surviving archive close/reopen/release. A bounded decoder
regression checks full-versus-prefix completion, truncated/extra input,
incorrect output sizes and untouched output guard bytes.

An additional macOS differential diagnostic encoded 675 valid streams with
the pinned official liblzma encoder: every one of the 75 supported `lc/lp/pb`
combinations, dictionaries 4096/65536/8388608, and one-byte, repetitive and
mixed random/repeated payloads. Both the former Linux decoder and production
liblzma adapter returned the expected bytes for all 2475 full/prefix requests,
without writing beyond the output boundary. All 2025 full-decode mutations
(truncation, appended byte and invalid properties) were rejected. This is
bounded compatibility evidence, not exhaustive equivalence on corrupt input.
Local diagnostic sources/results are retained under
`build/review/microlzma`; they are not another shipped decoder.

The baseline verifier validated all 32 original inventory units, all 31
vendored units match those bytes, and a reference-review against the same
revision exercised the original/local comparison path. macOS linkage inspection
found only system dependencies and no exported liblzma symbols. Incremental
builds/compiler caches were retained. Existing `upstream_baseline` and
`upstream_inventory` tooling suites also passed on macOS. Full/slow suites,
native mounts, release
installation, UI appearance and performance acceptance were not run.

## Read-side source audit closure — 2026-10-22

The read-side source audit is closed for this module's image/archive scenario
at Linux `58161ead29e50d70a317ea26b472e0c7508e69da`. The transferred scope below,
the compressed-read comparison and the dated corrections account for the format
readers, dependencies and C/archive consumers. This is source closure at that
revision, not a claim that the Linux VFS driver was ported wholesale or that
every possible image was exercised. Current multi-format status is owned by
the [parent audit](filesystems-linux.md).

The final comparison rechecked complete inode and symlink bodies, checksum and
superblock parsing, variable metadata reads, device-slot/mapping consumers,
the complete xattr transfer and shared ACL conversions. Inode/symlink bodies
match after the recorded strict-C pointer and signed-overflow adaptations.
Variable metadata reading matches unchanged; checksum/superblock differences
are pointer casts and early stream-block geometry setup. The xattr unit matches
after declaration/type adaptations and omission of the ACL-cache shortcut.
The source-visible gaps found during the audit, including UUID publication,
directory iteration and inline-metadata routing, are resolved by the recorded
iterations rather than hidden behind passing runtime checks.

No remaining format-read branch was identified in the reviewed scope. The
explicit boundaries are significant: archive indexing replaces VFS pathname
lookup and inode publication; stored modification time is exposed, while VFS
synthetic access/change-time aliases and DAX/statx I/O-alignment information are
not separate stored fields. Volume-label ioctl semantics are represented by
the archive label property. Kernel mount/sysfs controls, permission enforcement,
page-cache/ACL-cache management, scheduling and acceleration are not archive
format operations. New damaged-image partial-reading modes remain deferred
under the user's EROFS recovery scope; the concrete existing limitations above
and below remain applicable.

All 54 reference hashes, ten unchanged copied dependencies and the generated
CRC dependency hash match their recorded baselines. Runtime/build/test inputs
are unchanged since `e9a209ed`; its focused macOS `erofs_reader`, `erofs_module`,
`erofs`, `upstream_baseline` and `upstream_inventory` pass and application build
remain the current local evidence. The suites cover the C API, archive ABI,
CLI/backend consumers and producer layouts described below. Earlier Linux and
Windows results remain dated evidence; no remote validation was launched for
this closure. Physical-device/native-mount acceptance, UI appearance, release
installation, full and slow gates are not established by this iteration.
Only documentation changed, checked with `git diff --check`.

## Compressed read-side source comparison — 2026-10-22

Completed the selected comparison of compressed mapping, configuration, padding,
plain transforms, packed fragments and their stream/decoder consumers at the
pinned Linux revision below. The complete `zmap.c` read-side transfer matches
the reference after the recorded LLP64 type and shift-overflow adaptations;
only the iomap reporting wrapper is omitted. Complete `z_erofs_fixup_insize`,
`z_erofs_transform_plain` and `z_erofs_parse_cfgs` bodies match unchanged.
The complete LZ4 configuration body differs only in omitting kernel global-buffer
pool growth. The LZMA/Deflate/Zstd validation and decoder adapters retain their
previously recorded configuration and bounded-buffer boundaries.

Compared `zdata.c::z_erofs_read_fragment`, fragment dispatch in
`z_erofs_scan_folio`, compressed request construction in
`z_erofs_decompress_pcluster`, `fileio.c` consumers, `zutil.c` buffer services,
`ishare.c` and the corresponding `Kconfig`/`Makefile` choices. Packed fragments
reach the packed inode through the existing bounded inode-read callback; Linux's
folio copying/cache transport is not another format decoder. Global/reserved
page pools, asynchronous bio/work queues, in-place page sharing, page-cache
fingerprint sharing and hardware acceleration serve allocation/scheduling or
optional acceleration. Their omission does not remove an additional encoded
layout or codec. The earlier inline-metadata routing fix is required for both
ordinary and compressed inline data and remains part of this comparison.

The archive reader decodes and caches a whole mapped compressed extent. Thus
damage outside a requested subrange can prevent returning an otherwise readable
prefix. Kernel page-cache demand decoding can behave differently here; adding
that damaged-image partial-reading scenario remains deferred under the user's
EROFS recovery scope. The partial-reference format flag is retained and does
not imply acceptance of corrupt data. Logical extent, source, metabox/fragment
recursion and path checks remain active; no source writes or replay are added.

No further code change was justified by this selected comparison. All 54 EROFS
reference hashes match. Runtime evidence remains the existing focused macOS
pass for the unchanged code recorded below; no repeat runtime run or remote
validation was needed for this documentation/instruction-only iteration.
`git diff --check` passed. This closes the selected compressed read-side source
comparison, not the remaining whole-module dependency/consumer audit or the
parent multi-format goal.

## Inline metadata and data-device address spaces — 2026-10-22

Compared the pinned `data.c::erofs_iomap_begin` inline-data acquisition,
`fileio.c::erofs_fileio_scan_folio`, `zdata.c::z_erofs_pcluster_begin`,
`erofs_map_dev`, metadata-buffer services and the archive's physical-read
consumer. Linux acquires `EROFS_MAP_META` bytes through metadata buffers for
both ordinary inline data and compressed inline tails. The archive adapter
instead sent primary inline metadata through the data-device mapper. An
auxiliary device's unified-address range could therefore redirect a directory
or file's metadata bytes into that device.

The transport boundary now uses the existing original `erofs_read_metabuf`
path for all metadata extents, selecting primary metadata or metabox mapping
from the inode. It releases the acquired buffer after copying and retains
explicit logical-source and filesystem-block bounds. Ordinary data extents
still use the original device mapper. No filesystem mapping or decompression
algorithm is replaced, and no new source unit is copied. The complete source
units remain hashed at the Linux repository/revision below.

A public C-reader regression combines an inline root directory and inline
regular file in primary metadata with an indexed file on an external device.
Its synthetic device range covers the numeric metadata coordinates. The test
checks exact namespace/file bytes, no external read for directory metadata,
actual external reads for file data and unchanged primary/auxiliary sources.
Existing producer-image module/CLI suites cover compressed inline tails,
metabox, multi-device reads, cancellation and errors. This fixes the existing
address-space boundary; it does not add a recovery mode or authorize writes,
repair or journal replay. Overall EROFS source closure and the parent audit
remain open.

Validation: `erofs_reader`, `erofs_module`, `erofs`, `upstream_baseline` and
`upstream_inventory` passed on macOS, Linux and Windows (guest receipt
`20261022T173439Z-53f7d81d`). The incremental application build passed. The
changed transport was compared again with the original inline-metadata consumers;
all 54 EROFS reference hashes match. Existing runtime snapshots, build trees and
compiler caches were reused, and disposable EROFS fixtures were absent on all
three platforms afterwards. No full gate, slow suite, new native mount/device
check, UI appearance acceptance or release installation was run.

## Original directory iterator and omitted-dot handling — 2026-10-22

Compared complete pinned `dir.c::erofs_readdir`/`erofs_fill_dentries`,
`inode.c::erofs_read_inode`'s `dot_omitted` field, the shared `dir_emit_dot`
contract, metadata-folio acquisition and C/archive enumeration consumers.
The archive boundary had an independent block loop and omitted Linux's
synthetic `.` entry for `EROFS_I_DOT_OMITTED_BIT` directories. This was a normal
directory-reading gap, separate from recovery of damaged images.

The original iterator is now transferred into `src/fs/erofs/dir.c`, retaining
block/entry position handling, initial-position alignment, bounded last-block
length, callback stop, buffer release and final synthetic-dot handling. A narrow
file/inode and `dir_emit_dot` bridge replaces VFS file/dentry services; callback
identifiers retain the C API's raw disk NIDs rather than VFS inode-number
renumbering. The C entrypoint supplies its existing callback context and preserves
nonzero callback results, including positive stops. Archive enumeration continues
to ignore dot entries and uses opened inode metadata for actual item kinds.

Kernel readahead, task-signal and scheduler services are omitted; the existing
bounded folio reader polls archive cancellation and propagates stream failures.
Logical final-block name-offset validation retains the prior archive guard in
addition to Linux's block/alignment checks. Strict C byte-pointer arithmetic
replaces GNU void-pointer arithmetic. The VFS file-operations table and seekable
directory-handle API remain outside the whole-directory C callback boundary.
Original source hashes and repository/revision remain in the baseline below.

Public C-reader regressions cover compact/extended directories spanning two
blocks at 512-byte, 4 KiB and 64 KiB geometry. They check disk entries followed
by exactly one synthetic dot with the directory NID/type, callback stops on
that synthetic entry, I/O failure and successful retry, and unchanged source
bytes. Existing module/CLI suites cover full namespace enumeration, extraction,
metadata, errors and cancellation. No new recovery mode or source-writing
operation is added. Overall EROFS source closure and the parent audit remain open.

Validation: `erofs_reader`, `erofs_module`, `erofs`, `upstream_baseline` and
`upstream_inventory` passed on macOS, Linux and Windows (guest receipt
`20261022T172733Z-a697e434`). The incremental application build passed. The
iterator was compared again with the original for retained branches and the
adaptations above; the existing entry parser remains unchanged. All 54 EROFS
reference hashes match. Existing runtime snapshots, build trees and compiler
caches were reused, and disposable EROFS fixtures were absent on all three
platforms afterwards. No full gate, slow suite, new native mount/device check,
UI appearance acceptance or release installation was run.

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
- `dir.c` retains `erofs_fill_dentries` and `erofs_readdir`; bounded metadata
  folios and a file/inode/callback bridge replace VFS file/page-cache services.
- `super.c` retains checksum verification, variable metadata acquisition and
  superblock parsing. Device slots and unified/external-device addresses are
  adapted to bounded streams; device opening is delegated to a callback.
- `xattr.c`/`xattr.h` retain namespace/value iterators, shared/inline attributes,
  filters, long prefixes and ACL readers, using selected `fs/posix_acl.c`
  converters. The unused no-ACL cache shortcut is omitted; it does not replace
  public ACL value reads. Page-cache fingerprint sharing is disabled, while
  its on-disk prefix configuration remains understood by the xattr reader.
- Decompression configuration validation and plain shifted/interlaced transforms
  retain their original module paths. Stream-pool allocation, cache/overlap
  scheduling and kernel acceleration services are omitted. Compressed extents
  call the original decoder implementations through bounded-buffer adapters.
- `lib/crc/crc32-main.c` retains the generic Linux CRC implementations. The
  original generator produces the checked-in tables; only its unused generated
  kernel-configuration include and polynomial-header path are adapted.
  `include/linux/crc32poly.h` is copied unchanged.
- `namei.c`'s binary pathname lookup is omitted because archive consumers select
  indexed entries from full directory enumeration. `fileio.c`, `zdata.c`,
  `zutil.c`, `ishare.c` and `decompressor_crypto.c` provide the transport/cache/
  scheduling/acceleration boundaries described in the compressed-read comparison;
  their payload consumers use the retained mappers and decoders. `sysfs.c`,
  `Kconfig` and `Makefile` are reference inventory; the host CMake module and
  bounded callback services own build configuration and lifetime. Private
  `internal.h`/`compress.h` services expose only the required host types and
  read-side operations.

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

Raw LZ4 now uses the unchanged official library; its full/partial contracts and
the replacement of the earlier Linux decoder are recorded in the
[LZ4 source audit](lz4-frame.md). MicroLZMA uses the unchanged liblzma subset
described in the 2026-10-24 substitution above. Raw DEFLATE uses the shared
official zlib API described above; Zstd retains the existing 7-Zip integration.
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
