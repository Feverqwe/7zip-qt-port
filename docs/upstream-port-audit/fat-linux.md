# FAT read paths compared with Linux

Part of the [Linux format review](filesystems-linux.md), 2026-10-19.
The integrated iterations cover BPB-based FAT12/16/32 geometry, directory
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
verified the transferred function/table ranges against the pinned snapshot
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

## Volume state and useful read-only access — 2026-10-21

Compared complete Linux `inode.c:fat_read_bpb`, `fat_fill_super`,
`fat_set_state`, BPB types and `include/linux/msdos_fs.h:fat_valid_media` with
the original SDK header/open/FSINFO/stream/property bodies and current archive
opening, transport and consent consumers. The module previously ignored BPB
dirty state. The existing reader could retrieve files from a short source, but
its declared physical size made the common archive layer reject the opening as
unexpected EOF before useful access. These were separate reader/product gates.

The complete original `fat_read_bpb` C body is now retained in
`src/fs/fat/inode.c`, unchanged, alongside the earlier legacy BPB routines.
The original `fat_valid_media` helper and narrow little-endian/type services
supply its dependency closure. A bounded C bridge returns the dirty state using
the `fat_fill_super` FAT12/16 versus FAT32 branch. Legacy normalization uses a
private BPB with no extended state, as before. Write-side `fat_set_state` and
mount/VFS services are not copied. Notices and the existing GPL module/standalone
boundary remain intact; the sibling SDK remains read-only.

The dynamic FAT module reports dirty state and source truncation through the
shared read-only limitations contract. Opening needs explicit per-opening consent;
the badge persists across refresh, and a new opening asks again. Readable files
can be copied; source geometry still governs FAT/mapping validation. The physical
archive span is bounded by the actual input length, while the separate limitation
retains the declared-volume discrepancy. Missing required bytes remain DataError,
not successful empty/zero-filled output. An unusable FAT table is still a real
reader limitation. No source writes, repair, journal replay or forced unmounts
are introduced. Standalone retains its original short-source archive policy;
the application's Linux module owns the new limitations contract.

Related common fixes preserve abort/OOM from optional FSINFO seek/read and rewind
the stream before a fresh open, allowing reuse after a failed metadata read.
Other optional FSINFO failures remain warnings, with free space recounted from
the usable FAT rather than the failed cached count. These two integration fixes
also reach the standalone overlay without GPL linkage.

ABI checks cover clean/dirty/short/combined FAT12/16/32, successful exact payload,
cleared limitations after Close, genuinely incomplete extraction, FSINFO I/O,
abort/OOM and reuse. CLI checks actual extraction, empty files, diagnostics and
unchanged source bytes. The first integration run exposed the common unexpected
EOF gate; the bounded physical span fixes it without weakening that common layer.
Seven focused suites passed on macOS arm64 (53.41 s): `fat`, `fat_module`,
`language`, all three previous filesystem GUI suites and `filesystem_fat_read_ui`.
The new small GUI group preserves EN/RU accept/Cancel, exact copy, refresh/reopen,
disabled write actions, source preservation and compact geometry. Cocoa combined
warnings are 420×227 EN / 420×288 RU with translated text and no clipping.
The app incremental build and upstream verifier (422 exact / 242 adapted /
12 local), 21 Linux reference hashes and exact copied C body comparison passed.

Native Linux vfat `6.8.0-147-generic` read all nine dirty/short/combined controls
with exact payload and empty files, receipt `20261021T113154Z-3d9ca6e8`.
Readonly block/mount flags and unchanged SHA-256 were checked; all loops, mounts
and images were removed. This supplements pinned source comparison and does not
claim execution of that snapshot or physical-device acceptance.

Guest receipt `20261021T113302Z-13ace39b`: Linux 7/7 (23.74 s), Windows 6/7,
with an existing backend-discovery test reporting `Codec discovery timed out`.
The new ABI states, CLI and all four GUI groups passed. Focused unchanged
`fat_module` then passed on Windows, receipt `20261021T113706Z-48cca499`.
The original failure evidence is retained; its cause is not established, and
no discovery deadline, build flag or compiler cache was changed. All seven
related suites are therefore checked across macOS/Linux/Windows batches.
Full/slow suites and physical devices were not checked.

The directory failure boundary identified here is resolved by the next scenario.

## Directory failure isolation and reachable namespace — 2026-10-21

Compared complete pinned `dir.c:fat__get_entry/fat_get_entry/fat_get_entry_eod`,
`fat_parse_long`, `__fat_readdir`, `fat_readdir` and `fat_subdirs`,
`cache.c:fat_get_cluster/fat_bmap_cluster/fat_get_mapped_cluster/fat_bmap`,
`inode.c:fat_calc_dir_size/fat_validate_dir/fat_fill_inode` and callers/types,
with the original SDK `ReadDir`, cluster streams, item/tree properties, extraction
callback and current GUI/native-opening consumers. Linux keeps directory errors
local to an inode and can skip an unreadable directory block. The SDK recursively
opened the entire tree and propagated one bad child/chain or source read to a
failure of the whole image, discarding already readable records and siblings.

The original directory-record/LFN loop remains authoritative under its original
module path. Acquisition and recursive publication are separated into
`ReadDirItems` and the retained `ReadDir` entry point. The dynamic module acquires
logical sectors, preserving the SDK record parser and using the original stream
helpers; Linux VFS/page-cache/block services are not reimplemented. Known complete
records survive a missing suffix. A failed sector does not supply stale buffer
bytes and does not prevent trying the next mapped sector. Pending LFNs are reset
at a gap; metadata needs a complete 32-byte record, except a known first-byte NUL
which already proves end-of-directory. An EOD found in a short read does not
create a false incomplete-directory error.

Malformed start/continuation clusters, free/bad links, cycles and existing
item/size/depth bounds mark only the affected directory incomplete. Its known
records and discovered children are retained, and healthy siblings continue.
Source abort/OOM and all progress-callback failures remain fatal, including a
callback failure immediately after a recoverable source error. Active traversal
state is released on normal/error/exception exits. Physical metadata ownership
still prevents file streams from reading directory clusters. Reusing a directory
in another acyclic branch is readable at both names; an ancestor/chain cycle
remains blocked. Header accounting counts physical directory clusters once.

Incomplete directories expose item error metadata and return DataError when
extracted as a directory. Individual verified files remain readable. An incomplete
root namespace is reported through the global limitations contract. GUI consent,
badge and per-opening state use the existing mechanism; nothing writes to the
source, repairs its chain or replays a journal. Standalone retains its original
strict traversal; the common source factoring does not add GPL linkage or the
dynamic recovery semantics to that engine.

Twenty-four chain/alias controls cover FAT12/16/32. Additional range faults cover
root/child read and seek errors, unexpected EOF, short record/EOD prefixes,
LFN continuity, abort/OOM/callback and reopening. Checks compare the exact visible
file set, payloads and error results; no source-text assertions are added.
Existing stream/callback test helpers were shared without losing their earlier
module/backend tests. CLI checks copy known files and partial directory children,
while a requested incomplete-directory copy returns an error. The new small
`filesystem_fat_directory_ui` group copies both a healthy sibling and a known
file inside an incomplete directory, with EN/RU accept/Cancel, badge, refresh/
reopen, compact geometry and unchanged source. Cocoa warning geometry is
420×214 EN / 420×262 RU, with translated text and no clipping.

All nine focused suites passed on macOS arm64 (57.07 s). Final EOF/seek coverage
and the portable form of directory extraction also passed in focused FAT batches.
The first Windows build found preprocessor directives inside a macro argument;
the result is now selected before `RINOK`, without changing operation semantics.
App incremental build, upstream verifier (422 exact / 242 adapted / 12 local) and
21 pinned Linux source hashes passed. The C readers/NLS source bodies did not
change in this iteration.

Native vfat `6.8.0-147-generic` confirmed seven controls, receipt
`20261021T120537Z-267f96b2`: complete trees for FAT12/16/32, both alias paths,
and exact root/healthy-sibling files for free/cyclic/invalid child directories.
Recovery-prefix reading inside those invalid child directories is archive
adaptation, not a claim of VFS parity. Readonly block/mount flags and unchanged
SHA-256 were checked; all loops, mounts and images were removed. Pinned-kernel
runtime, physical media and full/slow gate acceptance are not claimed.

Linux's nine suites passed in `20261021T120701Z-ee7a2f34` (25.60 s); that
receipt also retains the initial MSVC compile diagnostic. After the portable
macro fix, all nine passed on Windows, `20261021T120918Z-e229cdd7` (94.41 s).
Linux's three direct FAT suites passed again on the final source,
`20261021T120917Z-1a271559` (2.22 s); the corresponding macOS follow-up also
passed. Existing incremental outputs/cache and all test timeouts were retained.

Read-side closure still requires the remaining module/consumer comparison.
The shared clustered-stream boundary identified here is addressed below;
directory acceptance alone did not establish stream independence.

### Independent clustered file cursors — 2026-10-21

Compared the complete original SDK `CPP/7zip/Common/LimitedStreams.cpp` and
`.h`, `CClusterInStream::Read`/`Seek`/`InitAndSeek`, FAT `GetStream`/extraction,
and the other consumer, `CPP/7zip/Archive/ComHandler.cpp` regular-stream mapping
and extraction. Both handlers retain one shared physical input. Ext uses a
different class, `CClusterInStream2`; other classes in the common source are
unchanged. The SDK revision/repository recorded above also owns these modules.
Linux `fs/fat/file.c` exposes `generic_file_read_iter`/`generic_file_llseek`:
each opened file has an independent logical position, regardless of shared
device storage and fragmented FAT mapping. This comparison adds no Linux C
units and does not transfer VFS/page-cache services.

The old clustered reader assumed its cached physical position still matched
the shared input. Creating or reading another file could therefore return that
file's bytes; a failed seek could also make a retry read from a stale location.
The regression reproduced both failures before the change. The retained SDK
reader now restores its mapped physical position before each nonempty read.
Its cluster vector, bounded contiguous-block batching, file-size limit,
logical cursor updates, error propagation and EOF result remain unchanged.
Failed seeks leave the logical position and processed byte count unchanged,
and retry the seek. The single adaptation is included in both engine overlays
and the FAT module; the original Compound handler itself is unchanged.

The new `fat_stream` ABI suite alternates two fragmented files for FAT12/16/32,
retains both streams after archive/source release, seeks backward across block
boundaries and checks exact bytes, EOF and retry after seek/read error, abort,
OOM or zero-byte source reads. `compound_stream` checks both engines with two
regular Compound streams and an intentionally truncated source: complete data
and the available partial prefix are exact, while truncation retains a nonzero
result and the incomplete file reports Data Error. No source writes, repairs,
journal replay or consent/bounds bypasses are added.

Native vfat `6.8.0-147-generic` independently confirmed alternating file handles
and backward reads for all three FAT widths, receipt
`20261021T123343Z-bdc5273d`. Read-only block/mount flags and unchanged SHA-256
were checked; all synthetic images, loops and mounts were removed. An initial
reference run used the mount's default charset and could not address the Unicode
filename; explicit `iocharset=utf8` corrected that test setup. This is native
runtime evidence, not a claim that the pinned Linux revision was executed.

The seven focused FAT/Compound/module/UI suites passed on macOS arm64 (11.62 s)
and Linux (`20261021T123408Z-ba1ab7c5`, 5.54 s). Windows initially passed five:
the backend module and one GUI row failed during codec discovery, before file
reads (one signal arrived after 6.65 s versus the test's 5 s wait). The unchanged
product/timeouts passed the four-suite targeted repeat, including the final
stream/Compound fixtures, `20261021T123516Z-dabdb436` (9.51 s). The discovery
timing cause remains unproven; these failures are retained as diagnostics rather
than attributed to clustered reads or hidden by a deadline increase.

The application incremental build, upstream verifier (422 exact / 242 adapted /
12 local) and 21 pinned FAT source hashes passed. A final stream follow-up
checks the whole requested range after backward seek rather than accepting only
its first block fragment. Compiler caches, build trees and test deadlines were
retained. Only the two handlers using this class and their direct consumers are
affected; full/slow gates, installation and physical media were not run. The
broader filesystem/format review remains active.

The final boundary/EOF follow-up passed on macOS (`fat_stream`, 0.98 s) and
both runners (`fat_stream`/`compound_stream`,
`20261021T123657Z-252e85d9`). The last synchronization also removed the
disposable reference/cleanup helper; no temporary helper is shipped.

That cursor iteration identified a separate read-policy boundary: `GetStream`
discarded an entire file when a later required FAT link was free, ended early
or looped, despite known earlier data clusters. The following iteration addresses
it; the old invalid-chain CLI tests established only an error result.

### Readable prefixes of incomplete file chains — 2026-10-21

Compared the complete SDK FAT `GetStream`, `Extract`, item/archive properties,
database opening/clearing and their consumers, plus pinned Linux
`cache.c:fat_get_cluster`/`fat_bmap_cluster`/`fat_get_mapped_cluster`, `fat_bmap`,
the `file.c` file operations and `inode.c` mapping/direct-I/O callers. Linux
can map earlier blocks before a later mapping request returns EIO; a bad later
link need not invalidate every earlier byte. The original SDK traversal is
factored into a shared helper, used by stream construction and the dynamic
module's opening diagnostics. No new FAT decoder or Linux C unit is added.

Streams now expose the prefix before the first invalid, repeated or known
directory-owned cluster. A private in-memory traversal flag is cleared by a
scope guard, so one file or callback failure cannot poison another mapping.
Capacity remains bounded by the file size and available data-cluster count.
Empty files and unused allocated tails remain readable; safe shared file chains
are independent. An invalid starting cluster yields no stream. Declared size
remains unchanged, so the original copy coder's actual-byte comparison reports
Data Error for an incomplete file and extraction continues to healthy selections.
Both shipped engines use this portable adaptation. The dynamic module additionally
reports affected-file Error and archive limitations before UI consent. EN/RU
warnings, the per-opening choice and persistent badge use the existing service;
no source writes, repair, journal replay or validation bypass is introduced.

Thirty-three ABI controls cover all three FAT widths with free/early-EOC/bad/
out-of-range links, self/back cycles, a directory-owned cluster, invalid start,
oversized declared size and complete tail/shared-chain controls. Checks cover
exact prefixes, original size/error properties, EOF/seek/retry, retained streams,
multi-file extraction/test mode with per-item Data Error/OK, cancellation,
destination failure and clean reopening. CLI checks both engines, including
healthy sibling extraction. The new small `filesystem_fat_file_ui` group checks
EN/RU Cancel/accept, partial bytes with actual visible extraction diagnostics,
healthy copying, badge, compact geometry, refresh/reopen and unchanged source.
Native Cocoa warning JSON is 420×214 EN / 420×262 RU, with translated limitations
and no clipped labels. The shared GUI test helper reuses the established
completed-progress Close predicate when dismissing expected diagnostics.

Native vfat `6.8.0-147-generic` confirmed 21 controls, receipt
`20261021T124757Z-f841e535`: aligned O_DIRECT reads return the exact 1024/1536-byte
prefix before EIO for free/EOC/bad/range/oversized chains, while tail/shared
controls return complete data. Buffered page-cache acceptance is not inferred
from these direct reads. Cycle/metadata boundaries remain explicit archive
validation adaptations; no native acceptance of them is claimed. Read-only
device/mount flags and unchanged SHA-256 were checked, with all synthetic images,
mounts and loops removed. The installed runtime kernel differs from the pinned
source revision; physical media and pinned-kernel runtime remain unverified.

The first UI checks timed out on expected extraction diagnostics because the new
automation handled message boxes but not the completed progress window; test
automation now follows the existing progress-dialog consumer. No product timeout
was increased. MSVC's SDK /Wall /WX rejected an implicit deleted default
constructor on the reference-bearing traversal scope; an explicit constructor
fixes that portable build issue without changing semantics. All 11 focused suites
passed on macOS arm64 (50.99 s) and Linux (`20261021T124831Z-2b334cab`).
The app incremental build, upstream verifier (422 exact / 242 adapted / 12 local)
and 21 pinned FAT hashes passed. Full/slow suites and installation were not run;
incremental build trees and compiler caches were retained. The broader review
remains active.

Windows passed ten suites in `20261021T124757Z-0ea11bd9` (130.59 s); the
remaining `fat_module` case hit the previously observed codec-discovery deadline,
before filesystem opening. With unchanged source and deadlines, its isolated
repeat passed in `20261021T125209Z-bce1da0a`. The discovery timing cause is still
unproven and the initial diagnostic is retained. Thus all eleven selected suites
have passing results on each OS, without claiming a single all-green Windows
batch. Final task-owned fixture cleanup passed on both guests,
`20261021T125129Z-4c9e1ee3`; the disposable helper was removed and synchronized
away on both guests. All jobs reached terminal state.

That file-prefix iteration left initial FAT-table acquisition requiring complete
reads. The following iteration compares the eager loader with Linux's on-demand
FAT-sector errors and separates unknown metadata from verified links.

### Localized FAT-table I/O failures — 2026-10-21

Compared complete pinned `fs/fat/fatent.c` entry addressing, sector acquisition,
FAT12 two-buffer access, 12/16/32-bit decoding, `fat_ent_read` and
`fat_count_free_clusters`, with their `cache.c` mapping and `inode.c` callers.
Compared the original SDK's full table loading, media/free-space checks,
directory traversal, file mapping, properties and open/clear/extract consumers.
The original SDK entry decoders remain authoritative, including FAT32 high-bit
masking. No new Linux C units, independent decoder or VFS/cache services are
introduced; the adaptation is the archive input-acquisition boundary.

The dynamic module retains original healthy bulk reads. After an actual input
failure it preserves reported prefix bytes and isolates unavailable logical
sectors, continuing to later readable sectors. Whole entry values require all
their bytes, including both halves of a FAT12 boundary entry. Unknown entries
have a separate internal state; they are never interpreted as zero/free space
or a confirmed directory EOF. A known first file cluster can still provide data
before its unknown next link, and a directory with a verified end marker can
be complete without following that link. Otherwise known directory entries and
children remain accessible with an incomplete-directory error. Existing metadata
ownership, cycle, geometry and path bounds remain in force.

Archive limitations describe the table failure before the existing per-opening
consent/badge flow. Exact free-space publication is withheld if required table
bytes are unavailable, instead of trusting an incomplete count or stale FSINFO.
Unavailable alignment padding after the last real entry does not create a false
limitation. Abort, OOM and open-callback failures remain fatal and clear the
unpublished archive; reopening restores normal properties. The selected table
remains authoritative, without guessing links from another FAT copy. Standalone
retains strict table acquisition; its existing readable-prefix mapping remains
unchanged. The physical table-end size check is retained: for this geometry the
root/data region lies after the required table, so an image ending inside it
does not contain useful file data to expose. No source writes or repair occur.

The new `fat_table` public ABI suite covers first/middle sector faults, short
reads/EOF and E_FAIL, partial cells and FAT12 straddles, a directory without a
known end, missing padding, initial seek failure, and a large FAT32 fault in a
later bulk chunk. It checks exact file/prefix bytes and per-item results across
multiple selections, Error/limitation/free-space properties, callback failure,
abort/OOM, and clean reopening. Fault inputs now stop successful reads at a fault
boundary even when a larger request spans it. A discarded early test assumption
expected a Seek to every sector; a healthy bulk read legitimately performs none
of those intermediate seeks. Tests retain actual I/O failure scenarios rather
than fixing the private read order.

The same fixture/module setup drives `fat_table_prompt`: EN/RU accept/Cancel on
actual handler limitations, default Cancel through Return, exact copy results
after consent and native text geometry. Cocoa JSON is 420×253 EN / 420×301 RU,
with both file-chain and table reasons translated and fully visible. Initial
new-test compilation issues (explicit class-pointer access and QLayout include)
were corrected before acceptance. The existing FAT UI consumers and language
suite remain covered. This is fault-injected archive ABI evidence; no installed
kernel/device I/O-fault injection or physical-media acceptance is claimed.

All ten selected suites have passing results on macOS arm64 across focused
batches (seven FAT/prompt/language suites, the added large-table controls, and
three FAT UI suites). Linux passed the ten-suite batch,
`20261021T131055Z-20bb9016` (10.86 s). In that same receipt Windows passed nine
(60.02 s); `fat_module` hit the previously observed codec-discovery deadline,
before filesystem Open. The unchanged isolated repeat passed in
`20261021T131602Z-7998823a`. The cause of discovery latency remains unproven;
initial diagnostics are retained and no deadline was increased.

Application incremental build, upstream verifier (422 exact / 242 adapted /
12 local) and all 21 pinned FAT source hashes passed. Task-owned fixture cleanup
passed on both runners, `20261021T131356Z-52d781e9`; the disposable helper was
deleted and synchronized away on both. All jobs reached terminal state. Existing
build trees and compiler caches were retained; full/slow gates and installation
were not run. This closes the selected table-I/O scenario, not the remaining
module/consumer review or the broader filesystem/format goal.
