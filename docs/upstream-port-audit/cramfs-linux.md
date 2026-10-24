# Cramfs read-side modernization against Linux

> CramFS support was removed on 2026-10-24. This document preserves historical
> source/runtime evidence; see [retired formats](retired-formats.md) for the current decision.

Scope входит в [общую проверку форматов](filesystems-linux.md). Датированные
разделы сохраняют evidence итераций; прежние открытые вопросы заменены поздними
исправлениями и итогом сверки ниже. Source completeness и runtime acceptance
остаются отдельными свидетельствами.

## Итоговая сверка read-side source scope — 2026-10-22

Сверка read features закреплённого Linux
`58161ead29e50d70a317ea26b472e0c7508e69da` и подключённого архивного reader
закрыта. Все семь reference hashes повторно совпали; полные тела, зависимости
и классификация copied/adapted/omitted приведены в [source comparison](#source-comparison).
SDK baseline — `9128b80e3a471108678e2b3ec3c985861c8d7b0f`, repositories и
retrieval manifests указаны там же. Перенесённый C read path остаётся C;
существующие SDK parser/decoder/cache/stream helpers переиспользуются.

После завершённых исправлений не осталось известного отсутствующего disk-read
feature этого Linux baseline: inode/directory metadata, обычные/прямые/raw
блоки, sparse/short pages, обе позиции superblock, fsid/size contracts и
частично доступные источники учтены. Ошибки отдельных файлов и каталогов не
выдаются за успешное восстановление; Abort/OOM и ошибки потребителей не
подавляются. Native block/MTD acquisition, VFS/page cache, mmap/XIP и запись
классифицированы как kernel/platform services, а не пропущенные декодеры.

Разница с Linux — marker scanner не всегда знает начало выбранного тома.
CRC и тот же metadata walker разрешают различимые варианты; при фактической
неоднозначности доступное чтение требует явного согласия, сохраняет индикатор
и сообщает, что выбранная интерпретация не проверена. Это поддерживаемый
ограниченный режим, а не утверждение о единственно возможном начале или полном
namespace повреждённого образа. Kernel zero-page fallback не используется
как доказательство успешно прочитанных файловых данных.

Последняя функциональная итерация проверена CLI/source и ABI/ENRU UI suites
на macOS, Linux и Windows; receipts и более ранние regressions приведены ниже.
Нативный mount в настроенном Linux guest недоступен из-за отсутствия driver;
runtime этих kernel/VM, физических устройств, full/slow suites и release
installation этим итогом не валидируется. Общий аудит остальных форматов и
задача EROFS этим source closure не объявляются завершёнными.

## Catalog origin selection and explicit ambiguity — 2026-10-22

Recompared the Linux selected-device origin, complete SDK marker/physical-size
consumers, the existing metadata walker and the read-only consent/refresh flow.
For marker opens without a verifiable v2 CRC, the same SDK directory walker is
now run against both bounded candidate origins in temporary handler state.
There is no second directory parser or root-address shortcut. A sole complete
catalog is preferred over an incomplete one; if the default catalog exposes
nothing but the alternative has a readable prefix, that prefix remains usable.
CRC diagnostics follow the selected candidate. Bounds, path validation,
directory/resource guards and fatal source/callback errors remain effective.

Two plausible catalogs cannot establish a unique origin. Nor does a failed
metadata read disprove the unavailable candidate. These cases retain an explicit
unverified-origin limitation rather than silently claiming a uniquely identified
volume or refusing supported reads. The existing per-opening EN/RU consent,
badge, cancellation and refresh scope apply. Published physical size describes
the bounded selected input window in this mode, not a verified filesystem end:
the SDK marker opener requires a known span to return an archive at all.
V1 physical-size initialization is reset after choosing a different origin,
preventing a stale boot-prefix extent from synthesizing an unexpected-end error.
Explicitly bounded opening from stream origin zero keeps the Linux 0/512 path.

Process fixtures check v1 and bad-CRC v2 ordinary/boot-prefixed embedded volumes,
two different valid catalogs sharing one header, and a partial alternative when
the default exposes no entries. Selected bytes, public offsets and uncertainty
are checked through the actual engine. The ABI test injects ordinary I/O failure
only into the alternative's metadata: the selected file stays readable, the
ambiguity remains visible, Abort/OOM propagate and a healthy reopen clears the
limitation. EN/RU UI checks cover consent/refusal, exact copying, mode visibility,
normal/compact badge geometry, refresh and unchanged source bytes. Current Cocoa
JSON shows fitting EN/RU warning labels (420×214/275 logical pixels).

An initial run exposed two integration issues: omitting physical size prevents
the SDK marker scanner from returning the archive, and the legacy v1 reader
retained its pre-selection header extent. Both are corrected rather than
relaxing the behavior checks. Local CLI/ABI/UI/source checks passed (five suites,
21.93 seconds). Linux/Windows CLI/source suites passed in
`20261022T071305Z-d10db8be`; final ABI and exact Cramfs UI suites passed in
`20261022T071422Z-fb4fcba3`. The incremental application build and diff check
passed. Native Cocoa and offscreen Linux/Windows UI behavior/geometry are
distinct from rendered Windows visual parity, which is not claimed. Temporary
fixture owners cleaned their data; current evidence, build trees and compiler
caches are retained. No full/slow suites were run. No source write, repair,
replay or missing-decoder consent bypass is introduced.

## Verified v2 volume origin after signature scanning — 2026-10-22

Recompared Linux's superblock-at-0/512 selection with the complete SDK handler
opening, registration and marker-scan/ReadBasicProps/ReOpen stream contract.
Linux already knows the selected device/partition origin. The archive scanner
instead supplies the whole bounded stream positioned at a matching header.
The adapter's unconditional interpretation of a header at byte 512 as a boot
prefix confuses an ordinary volume embedded at that address; an embedded
boot-prefixed volume can conversely be read relative to its header instead
of its true origin. The original SDK has no resolver for this added native
boot-prefix scenario.

For v2 marker opens, the adapter reuses its existing bounded incremental CRC
check to verify the current interpretation, then the alternative header-at-0
or header-at-512 interpretation if necessary. A verified alternative supplies
all subsequent metadata/file-read coordinates and the public archive offset.
Already verified CRC is reused in the Linux-profile opening path. No second
parser or root-offset heuristic is introduced: Linux permits a boot-prefixed
volume's directory before its superblock. The probe checks source/declaration
spans before scanning, preserves fatal cancellation/OOM/progress failures and
retains the original diagnostics if neither interpretation verifies. A CRC
mismatch or unavailable optional scan does not introduce an opening gate.
Opening from an explicit stream origin still follows the existing 0/512 path.

Eight initial process cases reproduced the wrong offset/catalog or refusal
before correction. Both engines now read the exact payload with the correct
origin and physical size, in either byte order, for ordinary volumes embedded
at 512 and boot-prefixed volumes embedded at 1024. A larger ordinary image
ensures the first candidate is actually scanned and rejected, rather than
merely lacking enough bytes. A native fixture with its root before the
superblock protects the Linux placement contract. The module ABI checks
embedded direct-block extraction, fatal checksum/callback errors, cleanup and
reuse. Existing optional-CRC failure/short-read cases now also open directly
at the native marker: readable files remain accessible and unverified status
is retained. No UI text or layout changes are introduced.

Local focused CLI/source and ABI/UI checks passed (five suites, 19.05 seconds);
the final expanded marker/optional-CRC ABI check passed separately (1.07 seconds).
Linux/Windows CLI/source checks passed in `20261022T065714Z-fe9a8d40`
(2.45/25.45 seconds), final ABI plus existing Cramfs UI checks in
`20261022T065817Z-a4701f6f`. The incremental application build and diff check
passed. No full/slow suites, new appearance acceptance, native mount or physical
device checks were run; fixture owners clean their data and build trees/compiler
caches/current review evidence are retained.
At this iteration, origin inference without a verifiable v2 CRC remained open.
The later catalog/explicit-ambiguity scenario above addresses that integration
boundary without claiming that ambiguous bytes have a uniquely provable origin.

## Incomplete block tables and readable file prefixes — 2026-10-22

Compared the pinned Linux folio mapping, direct/end-pointer dependencies and
bounded source reads with the C bridge, SDK GetStream/cache/CopyCoder and
per-file extraction callbacks. Linux reads the current pointer and only the
previous-pointer/header dependencies required by that page. The adapter instead
required the entire table inside the source before returning a file stream or
reading any page. That hides valid earlier direct blocks when the table lies
after their payloads and only the last table entries are unavailable.

The Linux profile now uses the existing mapper and bounded acquisition for
each requested page. Whole-table preconditions are removed from both C bridge
entry points and from Linux-profile GetStream. Arithmetic representability is
checked separately from source availability in the shared C mapper; file-size
rounding is widened before addition. The legacy reader retains its whole-table
validation because it indexes its complete buffer directly. No unchecked source
read or substitute bytes are introduced: an unavailable pointer fails that
page before its temporary buffer is copied, preserving earlier completed
output and the SDK per-file DataError result. Other selected files remain
accessible, with cancellation/OOM and output failures retaining their existing
fatal behavior.

Opening checks published inode table ranges without decoding payloads. A
known incomplete table uses the existing per-opening consent, read-only badge
and refresh behavior, with an EN/RU explanation that partial copies report an
error. Consent does not mark an incomplete file successful or authorize repair.
An image may have usable direct blocks despite this limitation; ordinary end
pointers continue to require all dependencies used by the selected page.

Small process fixtures relocate a table after its existing payloads and omit
its final pointer, covering raw/zlib direct blocks, v1/v2 and both byte orders
with and without a native boot prefix. They require the exact readable prefix
and a failing extraction result, rather than accepting fabricated file tails.
The ABI scenario verifies that a healthy sibling is copied, and that a failed
seek/read of the unavailable page does not prevent rereading the first page.
The EN/RU UI scenario covers consent/cancellation, mode visibility, partial
copy diagnostics, exact output and unchanged source bytes. Local focused
`cramfs`, `cramfs_module`, `filesystem_cramfs_read_ui`, `upstream_baseline` and
`upstream_inventory` checks passed (19.11 seconds). Linux/Windows CLI/source
checks passed in `20261022T064621Z-9a75d75f`; final ABI and exact Cramfs UI
checks passed in `20261022T064718Z-c314b9e4`. The application build and diff
check passed. Current Cocoa JSON for the EN/RU warning labels shows complete
visibility and fitting text (420×214/275 logical pixels); the UI scenario
checks the badge in normal/compact windows too. Linux/Windows UI behavior is
offscreen, without a rendered Windows visual reference. Temporary fixture
owners clean their data; current review/failure evidence, build trees and
compiler caches are retained. No full or slow suites were run.
The remaining Cramfs source audit and general embedded-origin inference remain
open; this is not native-kernel or physical-device acceptance.

## Declared size and available source bounds — 2026-10-22

Recompared the pinned Linux `cramfs_read_super`, `cramfs_blkdev_read`,
`cramfs_direct_read`, `cramfs_read_folio` and MTD acquisition with the complete
adapter opening, metadata, checksum, stream and property consumers. Linux's
block-device reader uses device bounds rather than treating `super.size` as an
inode-data boundary; the MTD direct reader does enforce that declared size.
The archive adapter previously imposed the latter restriction on every source,
including refusing a readable volume whose declared size did not cover its
header. The selected archive scenario now permits useful bounded reading with
an explicit indication when the declared size is unreliable.

The pointer-mapping segment of the original Linux folio implementation is
factored into `cramfs_map_inode_block` in the same C translation unit. Both
folio reads and optional extent inspection call that one implementation;
there is no independent C++ block mapper. Existing endian loads, overflow
guards, source bounds, hole handling and final raw-page correction remain.
Mapping failures are checked before decompression/copying the temporary page.
The narrow C bridge exposes the mapped extent without decoding payload data.

When available bytes extend beyond a v2 declared size, the adapter inspects
published file mappings against the actual bounded source. References beyond
the declaration, an undersized header declaration or an unavailable mapping
produce the existing per-opening read-only consent flow and persistent badge.
Ordinary metadata/mapping failures retain the supported readable subset;
cancellation, OOM and progress-callback failures remain fatal. An unavailable
mapping leaves physical size unknown rather than inventing a verified end.
Known extents extend physical-size accounting only as far as referenced data.
For an overdeclared volume, the published physical size is bounded by the
available source while the existing short-source warning remains. Publishing
the unavailable declared extent made the SDK opener synthesize a hard
unexpected-end error before the application could offer its read-only choice.
If all inspected references fit the declaration, private file streams retain
the declared bound: unrelated trailing bytes do not enable a recovery mode or
silently become readable after an on-source pointer changes. CRC still covers
only the declared volume; a declaration too short for its checksum header is
explicitly unverified. No write, repair or replay is introduced. The legacy
standalone profile keeps its existing declared-size acceptance.

Small process fixtures cover both endiannesses, native prefixes, zero and
underdeclared sizes, ordinary/raw/direct mappings, exact extracted bytes and
the clean trailing-data control. ABI tests inject source failure, Abort/OOM
and optional-inspection cancellation, verify reuse and unknown physical size,
and mutate a valid pointer after opening to prove the retained source bound.
The existing EN/RU UI scenario checks consent, cancellation, geometry, the
visible mode and copying without source modification for the new limitation.
An initial validation selection ran the general filesystem UI scenario rather
than the separately registered `filesystem_cramfs_read_ui`. The exact Cramfs
suite exposed the overdeclared-size refusal in EN/RU on all three platforms;
that failure drove the physical-size correction above.

Local final `cramfs`, `cramfs_module`, `filesystem_cramfs_read_ui`,
`upstream_baseline` and `upstream_inventory` checks passed (15.59 seconds).
Linux/Windows CLI/source checks passed in `20261022T064031Z-682e9e7a`
(2.14/13.58 seconds); ABI plus the exact Cramfs UI suite passed in
`20261022T064125Z-ef64c779` (4.82/14.03 seconds). Current Cocoa widget JSON
confirms that the EN/RU limitation labels fit; the existing scenario also
checks the persistent badge at normal and compact window sizes. The final
application build passed. A subsequent test-only clarification makes the
cancellation callback wait until a file is published, ensuring cancellation
reaches extent inspection rather than initial metadata loading; its local ABI
check and both runners passed (`20261022T064218Z-5cce7735`). These are native
Cocoa and offscreen Linux/Windows behavior and
geometry checks, not rendered Windows visual-parity acceptance.
This closes the declared-size scenario, while the final
remaining Cramfs source comparison and general embedded-origin inference remain
open. No full/slow suite or native-kernel/device acceptance is claimed.

## File I/O isolation and optional packed statistics — 2026-10-22

Recompared Linux folio/error completion and read services, the adapted C I/O
error latch, SDK cached Read, CopyCoder and Cramfs GetPackSize/GetProperty/
Extract callers. Baselines and retrieval remain below. A local read error
does not make another inode unreadable in Linux. The archive bridge returned
every source I/O failure as a fatal extraction HRESULT, preventing the existing
SDK loop from reaching a healthy sibling. Conversely, the boolean packed-size
helper discarded source E_ABORT/E_OUTOFMEMORY as an ordinary unknown property.

The same helpers/parsers, C folio body, SDK cache and copy loop are retained.
Packed-size lookup now distinguishes a known value, an unavailable optional
summary and fatal cancellation/OOM. Both property publication and extraction
progress preserve those fatal results. An invalid end pointer, an extent
beyond source bounds or encoded block flags is not published as a physical
byte count. Flagged blocks remain readable by the existing Linux data path,
including when the superblock did not advertise the optional extension bit.
An unknown optional summary is not a verified file or an opening refusal.

Ordinary Linux-profile source/decoding failures now use the SDK per-file
DataError flow. CopyCoder preserves only the bytes completed before the error,
reports failure for that file and continues other selected items. The C I/O
error latch is checked before publishing the temporary folio buffer: failed
reads never become successful zero-filled data. E_ABORT/E_OUTOFMEMORY remain
fatal; E_NOTIMPL retains unsupported-operation reporting. Output-write and
progress-callback failures retain their original fatal behavior. Invalid
nonempty stream mappings are DataError rather than UnsupportedMethod; missing
decoding is still unsupported. The COM Extract call's S_OK means the callback
sequence completed, not that every item succeeded.

New public ABI regressions first reproduced swallowed packed-stat cancellation
and whole-extraction refusal before a healthy sibling. They now verify exact
readable prefixes, per-item DataError/OK results, fatal Abort/OOM both in optional
statistics and file data, recovery on a later read, unknown invalid/flagged
packed sizes and exact flagged-block data. A RAM-only two-file image injects a
late failure in the first file while the second remains readable. Existing
retained-stream/source-error expectations now require S_FALSE/DataError for
ordinary file I/O, preserving cancellation and retry checks. No new gate,
repair/replay, source write or missing-decoder consent bypass is introduced.

CLI plus source suites passed on macOS/Linux; the initial Windows first CLI
process exceeded its existing 10-second test deadline on the direct-range
control. The unchanged isolated suite passed in
`20261022T060614Z-d25e540c` (2.07 seconds). The timeout's cause is unconfirmed;
no deadline, retry policy or product behavior was changed to pass it. Final
isolated ABI/backend checks passed on all three platforms, Linux/Windows
receipt `20261022T060834Z-f0a7def3` (0.33/17.70 seconds), macOS 1.86 seconds.
Both engines/module were rebuilt incrementally, the macOS application build
and diff check passed. No new UI text/geometry changed, so appearance acceptance
was not repeated. No full/slow suites, native mounts or physical devices were
used; temporary fixture owners cleaned their data and compiler caches/build
trees/current review evidence were retained.

This closes the selected file-error/statistics scenario, not the entire
cramfs source audit. Later sections record declared-size and volume-coordinate
corrections; the final read-side comparison remains open.

## Inode data presence and informational counts — 2026-10-22

Recompared Linux `cramino`/`get_cramfs_inode`, folio reads, superblock fsid
handling and `cramfs_statfs` with SDK GetDataSize/GetStream/GetPackSize,
Open2 and item-count consumers. Baselines/retrieval remain below. Linux
assigns no payload to zero-size inodes or special nodes, including buggy
images with a nonzero unused offset. A zero data offset also produces a
data-less inode even when its raw size field is nonzero: the existing
GetDataSize rule already matches that behavior and is retained. Generic
`fs/inode.c` initializes i_size to zero; `get_cramfs_inode` only fills it for
data-bearing inode references. The partial-directory adapter had incorrectly
diagnosed a zero-offset directory with nonzero raw size; that false limitation
is removed according to the same Linux rule.

The original SDK empty-buffer stream is now selected before validating unused
data locations. Known empty streams also publish packed size zero before any
extended-pointer/table lookup. Thus empty regular files, symlinks and special
nodes do not require nonexistent block tables or a readable source payload.
Real nonempty data keeps the existing source bounds, decoding and error paths;
device major/minor metadata remains decoded independently from its data-less
stream. No new inode parser or decoder was introduced.

Linux's fsid file/block counts are informational statfs fields. The archive
adapter rejected a large declared file count and reserved catalog memory from
that untrusted hint before reading entries. Both count-dependent operations
are removed in both engine profiles; the original vector grows from actual
published records. The actual catalog/item/depth and source/memory bounds
remain in place. Declared counts remain available as header metadata; they do
not prove a complete namespace, control iteration or authorize allocation.
There is no new limited-read warning for a harmless statistical hint.

Pre-fix regressions reproduced unsupported-method errors for empty streams
with unused high offsets, refusal of readable files with large fsid counts,
and missing known-zero packed size. CLI controls now check exact empty bytes
for regular/symlink/character inodes in both byte orders and both engines,
including unchanged device properties. Counter controls check exact nonempty
file data with zero/maximal wire counts without constructing a large image.
The ABI control disables all source reads after Open and retains the empty
stream after Close/release. An existing EN/RU UI scenario now verifies that
large counters do not cause refusal, a false limited-mode prompt or failed
copy of a healthy file. Source access stays read-only; repair/replay and
missing-decoder policy are unchanged.

Four CLI/UI/source suites passed on macOS/Linux/Windows, receipt
`20261022T054533Z-fab3c42f`; the ABI/backend suite then passed separately on
all three in `20261022T054736Z-9e60b446`. Scheduling it separately avoids
combining its existing codec-discovery deadline with the CLI/UI process load;
the deadline itself was not changed. After the zero-offset-directory follow-up,
the affected CLI and two source suites passed again on all three platforms,
receipt `20261022T054920Z-ebe7caac`; unchanged ABI/UI scenarios were not repeated.
Both engine overlays and the module were rebuilt incrementally with existing
compiler caches. Diff check passed. No full/slow suites, native mounts,
physical devices or new UI appearance acceptance were performed. Fixtures are
RAM-only or temporary-owned; build trees/caches and current evidence remain.
The incremental macOS application build passed.

This resolves empty-stream and count-hint dependencies, not the entire cramfs
audit. Declared-size/source-bound acceptance, prefix/physical-size reporting
and the final remaining read-side comparison still precede source closure.

## Partial directories and independent branches — 2026-10-22

Recompared complete Linux `cramfs_readdir`/`cramfs_lookup`, inode publication
and block-read dependencies with SDK OpenDir, item publication, properties,
GetStream and Extract. Source baselines/retrieval are below. Linux can emit
completed directory entries before a later error, and another directory's
lookup does not require a complete walk of the damaged branch. Loading an
entire directory and propagating its failure through recursive archive Open
introduced a stronger refusal in the archive adapter.

The same SDK parser/walker is retained. Its entry loop is factored into
ReadDirItems and, in the GPL Linux profile, requests the fixed header and name
through the existing addressed metadata service before publishing an item.
Only completed, validated entries are retained; malformed names/types, path
escapes and incomplete records never enter the catalog. A later local error
preserves the completed prefix. Recursive traversal still visits the original
range of immediate children and continues independent branches. It does not
guess a resynchronization position in a corrupt tail.

Active source-directory references prevent recursion cycles, while completed
aliases can be traversed at another archive path. Local parse/I/O failures and
resource/depth limits mark the selected directory incomplete. E_ABORT,
E_OUTOFMEMORY and all open-progress callback failures remain fatal and clear
partial state. An unavailable root directory can expose only header metadata
and an explicitly incomplete empty catalog; this is not a verified empty FS.
Empty directories need no read of an unused offset, matching Linux's empty
inode path. Historical core-only profiles retain strict namespace opening.

The archive warning/read-only limitations report missing files/names and use
the existing per-opening consent, Cancel default, persistent badge and reopen
flow. Each incomplete directory also exposes its public item Error property;
copying that directory reports DataError while readable files remain exact.
Copying an independent healthy file succeeds. No source writes, repair,
journal replay or missing-decoder bypass is introduced. EN/RU messages use
the existing diagnostic translation path.

Public ABI regression first reproduced whole-open refusal after a readable
child entry. It now checks that prefix and a healthy sibling, per-directory
copy error, late source Abort/OOM, fatal callback E_FAIL after publication,
cleanup and healthy reopen without stale limitations. Existing source-gap
tests now require a diagnosed incomplete catalog for ordinary required-root
I/O failure while retaining fatal cancellation/OOM checks. CLI controls cover
both byte orders, malformed directory tails, cycles, individual exact reads,
public item errors and exclusion of invalid/unsafe entries. An existing empty
root control now also checks an unused high offset. UI controls exercise
accept/refuse and healthy copying for partial directories and cycles in EN/RU.

The six focused suites (`cramfs`, `cramfs_module`,
`filesystem_cramfs_read_ui`, `language`, `upstream_baseline`,
`upstream_inventory`) passed on macOS and Linux. Windows's initial strict
runtime build required an explicit constructor for the cleanup scope (C4623);
warnings remain enabled. After that narrow portability fix, five Windows
suites passed, while the ABI suite exposed a test-only native path-separator
difference and an existing codec-discovery deadline. The path lookup is now
normalized in the fixture comparison; the product path contract and deadline
are unchanged. Final isolated ABI checks passed on all three platforms in
receipt `20261022T053828Z-bd70d33d`. The timeout's cause is unconfirmed; this
does not establish reliable concurrent acceptance of that deadline.

Following the constructor change, Cramfs CLI/ABI consumers were also rechecked
on macOS/Linux; the unchanged UI/source suites were not repeated. Incremental
macOS application builds and diff check passed. Current Cocoa widget JSON
confirms the new translated warning fits; Windows/Linux UI used offscreen Qt,
without pixel-parity claims. No full/slow suites, native mounts or physical
devices were used. Existing compiler caches/build trees and current review/
failure evidence were retained; temporary fixture owners clean their data.

This completes directory-local failure isolation, not the entire cramfs audit.
The inode/count scenario above subsequently resolves empty-data and fsid-hint
dependencies. Declared-size/source-bound acceptance and physical offset
reporting still need final source comparison before closure.

## Addressed metadata reads — 2026-10-22

Compared complete Linux `cramfs_read_super`, `cramfs_read`/block-device buffer
acquisition, `cramfs_readdir`/`cramfs_lookup`, the disk inode header and SDK
Open2/OpenDir/EnsureData plus all retained-item/path/property/stream consumers.
Baselines are recorded below. Linux reads metadata around its referenced
location, without depending on every byte preceding a directory. The archive
adapter instead loaded the contiguous source prefix to the greatest metadata
address, so an unrelated unreadable gap hid readable directories/files.

The original SDK directory walker and disk parsers are retained. The narrow
buffer service now appends only the selected source range; archive item offsets
refer to retained metadata and disk inode offsets still refer to source bytes.
The header cache begins at the chosen superblock, excluding its boot prefix.
Source range and retained-buffer capacity are checked before allocation/read;
physical/header-size accounting keeps source coordinates. Growth, ownership,
progress callbacks and recursive publication retain their existing structure.
Historical whole-image profiles retain their buffer/path; no Linux parser is
reimplemented and the original C folio data reader is unchanged.

The primary header candidate can also be unreadable while a supported header
at byte 512 is readable. The handler now tries that candidate after an ordinary
initial read failure/short read as it already did after a magic mismatch.
E_ABORT/E_OUTOFMEMORY remain fatal, and explicit nonzero starting positions
preserve their read failure rather than probing an unrelated location. This
is bounded archive-source adaptation; Linux block-buffer I/O granularity and
failure-to-zero behavior are not claimed to match the archive callback.

The old 272 MiB bound was sufficient for format address geometry: inode offsets
have 26 bits shifted by two, and a directory size has 24 bits. It was not a
missing higher directory-address encoding. The remaining retained-catalog
capacity/item/depth guards are resource limits; cache occupancy no longer grows
with unused source gaps. A virtual source regression places a directory at
the final addressable offset and file data at 1 GiB without creating a large
file. Both it and a smaller shifted-directory control previously failed on an
unrelated gap, and now extract exact bytes with the correct Unicode path.
Another pre-fix failure reproduces the unreadable boot prefix. Required
directory I/O failures, cancellation and OOM still propagate and clear partial
open state. No extra limited-read warning is fabricated for unrequested gap
bytes; whole-volume verification has its own explicit state described below.
Source remains read-only, with no repair or replay.

Final five focused suites (`cramfs`, `cramfs_module`,
`filesystem_cramfs_read_ui`, `upstream_baseline`, `upstream_inventory`)
passed on macOS/Linux/Windows in 6.79/3.85/19.06 seconds; runner receipt
`20261022T052236Z-3fc6c724`. Both engine overlays and the GPL module were
rebuilt incrementally with retained compiler caches. The macOS application
build and diff check passed. Existing CLI tests exercise both byte orders,
nested directories, legacy profiles and data errors; UI tests retain their
consent/badge/copy checks. No full/slow suites, native mounts, physical
devices or new UI visual acceptance were performed. Fixtures are RAM-only
or temporary-owned; incremental build trees and caches were retained.

This resolves unnecessary source-prefix dependencies. The partial-directory
scenario above subsequently isolates failures in required directory ranges.

## Optional whole-volume verification and limited reading — 2026-10-22

Recompared Linux `inode.c:cramfs_read_super`/`cramfs_finalize_super`, folio
reads and their source dependencies with SDK opening, CheckCrc, archive
properties and CLI/backend/native-folder consumers. Baselines and retrievable
repositories are recorded below. Linux reads fsid metadata but does not require
whole-volume CRC verification before ordinary file access. The archive adapter
added this scan; an I/O error in otherwise unused volume bytes refused all
opening. A completed CRC mismatch also set a fatal header flag, causing CLI
listing to fail even when the file bytes were readable.

The same CRC scan and SDK CRC implementation are retained. Ordinary scan Seek/
Read failure or a short read now leaves the checksum explicitly unverified;
E_ABORT/E_OUTOFMEMORY and all progress-callback failures remain fatal. No
partial scan is presented as a checked checksum or as a mismatch. A completed
mismatch and a source shorter than the declared volume remain diagnostics,
published as warnings rather than fatal opening errors. Actual metadata/path/
bounds checks and file read/extraction errors are unchanged. No source writes,
repair or replay are performed.

Named read-only limitations distinguish unavailable verification, mismatch and
short source. The existing native-folder UI requests explicit consent for the
selected opening, defaults to Cancel, retains the limited-access badge and
uses the existing refresh/reopen rules. Healthy controls have no limitations
or false prompt. EN/RU diagnostics reuse the existing translation flow. The
standalone overlay receives the same warning/property behavior; only the GPL
module's bounded source path runs the optional streaming CRC scan.

The ABI regression first reproduced refusal for both an I/O fault and a short
read in an unused source tail. It now verifies exact file data despite that
fault, fatal cancellation/OOM and progress-callback failure, cleared failure
state and healthy reopen without a stale limitation. CLI controls on both
engines and both byte orders retain exact payload after a checksum mismatch.
UI controls cover consent/refusal, mismatch, short source, healthy opening,
copying, image immutability, reopen and visible EN/RU normal/compact geometry.
Current Cocoa widget JSON confirms translated diagnostic text fits; native
Windows pixel parity and physical devices are not claimed.

Final six focused suites (`cramfs`, `cramfs_module`,
`filesystem_cramfs_read_ui`, `language`, `upstream_baseline`,
`upstream_inventory`) passed on macOS/Linux/Windows; test phases took
6.84/4.49/22.55 seconds. Runner receipt: `20261022T051434Z-c9c5b9a6`.
The incremental macOS application build and diff check passed. Both engine
overlays were rebuilt with existing caches; no full/slow suites or native
mounts were run. Disposable fixtures were cleaned by their owners, superseded
task UI reports removed, current review evidence and caches retained.

This completes the optional-verification scenario, not the full cramfs audit.
The addressed-read scenario above subsequently removes contiguous source-prefix
dependencies; the partial-directory scenario above subsequently isolates
directory-local errors.

## Independent file streams — 2026-10-22

Rechecked the complete SDK `GetStream`, `ReadBlock`, `Close`/`Free`, allocation
and cached-stream consumers against the baseline below, plus the Linux folio
reader and the archive C source/decoder callbacks. A returned stream retained
the public archive handler, whose mutable source and buffer were cleared on
Close or replaced on reopen. Retaining the COM object alone did not retain
the selected file's readable state. Both the Linux and historical SDK profiles
failed an uncached read after Close in the public ABI regression.

The original SDK stream/cache/decoder flow and original Linux C folio body
are retained. Each returned stream now owns a private instance of the same
handler with its selected source, profile, bounds and decoding state. The
historical whole-image buffer has shared immutable ownership, so independent
streams do not copy the image per file. The Linux profile holds its bounded
source directly and does not need the archive's metadata buffer to read data.
Allocation paths and Free use the same MidAlloc/MidFree pair through shared
ownership. The standalone overlay receives the same SDK fix; no sibling or
Linux source is changed. No second format reader or decoder is introduced.

The regression covers two streams, uncached reads after Close and reopen into
the other profile, release of the public handler/provider, source I/O failure
and E_ABORT with zero reported bytes, retry and independent seek/read. Linux
extended/raw and historical SDK Zlib controls exercise both ownership paths.
This is an archive ABI lifetime adaptation, not a claim that Linux offers this
archive interface or that a shared seekable source supports concurrent calls.
Source access remains read-only; no gate, consent, repair or replay changes.

Before the fix the new ABI rows both failed the first read after Close.
Afterwards `cramfs`, `cramfs_module`, `upstream_baseline` and
`upstream_inventory` passed on macOS, Linux and Windows. Linux/Windows receipt:
`20261022T050553Z-57a756bb`; test phases 2.05/27.68 seconds. The local test
phase took 2.60 seconds; the incremental application build also passed.
Both official engine overlays and the module were rebuilt incrementally with
the existing compiler caches. No full/slow suites, physical devices or native
mount acceptance are claimed; fixtures are RAM-only or temporary-owned and
removed by their existing owners. Diff check passed.

This iteration does not close the complete cramfs source/policy audit.
The optional-verification scenario above subsequently resolves the CRC opening
dependency. Addressed metadata reads above remove the contiguous-prefix
dependency; partial-directory handling is recorded separately above.

## Volume coordinates in the archive adapter

2026-10-22: compared `CHandler::Open2`, archive-property publication and the
7-Zip `CArc` marker-scan, `ReadBasicProps` and `ReOpen` consumers at the SDK
revision below. `UseGlobalOffset` supplies the whole bounded input stream at
the detected signature; the reported offset must identify the volume origin
within that stream. The adapter now reports that origin rather than a delta
from the signature cursor. This removes a false negative offset and trailing
512 bytes for a native boot-prefixed volume, and preserves the outer offset
for an embedded ordinary volume. Data-read positions are unchanged.

Small fixtures verify offset, physical size, absence of a false tail and exact
extraction through both engines, with automatic and forced recognition.
The existing Unicode/boot-prefix ArchiveBackend scenario also checks the
published volume coordinates. Before the correction, six CLI cases failed.
Afterwards `cramfs`, `upstream_baseline` and `upstream_inventory` passed locally
and on Linux/Windows (receipt `20261022T062607Z-cc27d7bc`); the local module ABI
suite and application build passed too. The module ABI suite passed on both
runners in receipt `20261022T062944Z-191d0d83`. These cases do not establish general inference of the
origin of an embedded boot-prefixed volume. Declared-size/source-bound behavior
is covered above; the remaining Cramfs source review remains open.

## Source comparison

2026-10-19, part of the [Linux format review](filesystems-linux.md).
Linux reference repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
Inventory unit: complete `fs/cramfs` (`inode.c`, `uncompress.c`, `internal.h`,
`README`, Makefile and Kconfig), plus `include/uapi/linux/cramfs_fs.h`.
[Reproducible source hashes](../cramfs-linux-baseline.json) cover these units.
The comparison covered feature acceptance, superblock placement, directory
entries, inode types, folio reads, direct/end pointers, holes and zlib boundaries.

2026-10-22 full inventory reread: all seven recorded reference hashes match.
The following classification accounts for complete Linux bodies and their
callers; it does not turn runtime checks into proof of a complete source port.

| Reference bodies / dependencies | Archive read-side treatment |
| --- | --- |
| `cramfs_read_folio`, block flags and inode wire widths | Original C read algorithm, shared pointer-mapping segment and bounded/endian C services. Safety/error and aligned-final-page deviations are recorded in the integrated scenario. |
| `cramino`, `get_cramfs_inode`, `cramfs_read_super`, `cramfs_finalize_super`, `cramfs_statfs` | SDK inode/header/property adapter: non-data-bearing inode handling, UID/GID, old device numbering, root placement and raw fsid counters. Root permission normalization, artificial zero timestamps, kernel inode identities and inode caching are VFS presentation, not additional stored fields. Shared data offsets remain readable through independent archive streams. |
| `cramfs_readdir`, `cramfs_lookup` and directory operation tables | Existing SDK walker/public paths, complete-entry publication, bounded names, partial directories and cycle/resource guards. Sorted lookup is a VFS name-search optimization; enumeration does not require it. Cross-platform extraction path validation is retained. |
| `cramfs_blkdev_read`, `cramfs_direct_read`, `cramfs_read` | Seekable bounded input services, including declared-size and partial-table scenarios above. Kernel block/MTD caching and read-ahead are omitted. Failed acquisition is reported as unavailable data instead of treating the kernel service's zero-page fallback as verified file bytes. |
| `cramfs_uncompress_block`, init/exit and `internal.h`, zlib Kconfig dependency | Existing SDK zlib/Deflate implementation and per-reader ownership provide complete-stream decompression and short-page zero fill. The kernel's single global stream/workspace lifecycle is not copied into an archive plugin. |
| `cramfs_get_block_range`, `cramfs_last_page_is_shared`, MMU/NOMMU mmap/get-unmapped-area/capabilities and physmem operation table | Native mapping/XIP optimization, omitted as a kernel platform service. The ordinary C folio reader retrieves the same raw/direct file bytes, including shared final-page layouts, without mapping physical memory. |
| blockdev/MTD fill-super, kill/reconfigure, get-tree/context/type registration and module init/exit; Makefile/Kconfig | Kernel device acquisition, lifecycle and module configuration are omitted. Archive ABI registration, input lifetime and separate GPL-module deployment supply the userspace boundary. Source handles remain read-only; no sync, repair or journal replay is transferred. |
| `fs/cramfs/README`, UAPI header | Disk layout, flags, byte order and direct-block contracts compared in full. The SDK's opposite-endian decoding and explicit historical LZMA/enlarged-block branches remain additional archive compatibility; Linux-only unsupported flags are not interpreted by the C reader. |

All pinned Linux read features are accounted for by these copied/adapted paths.
The subsequent catalog-origin/explicit-ambiguity integration completes the
selected adapter scope as summarized above. This classification does not imply
general recovery of an ambiguous image or native-kernel runtime acceptance.

7-Zip repository: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Complete
`CPP/7zip/Archive/CramfsHandler.cpp` bodies, registration consumers and
`Common/StreamObjects.cpp`'s cached stream were compared. The handler remains
the authoritative adapted source under its original path in `src/`.
Original zlib/Deflate dependencies were copied unchanged, with notices and
paths recorded in the [7-Zip baseline](../upstream-baseline.json).
Existing LZMA, copy, progress, property and stream helpers are reused.

## Integrated read scenario

The initial sparse fix (`b3c78fd`) retained the upstream cached stream and
zero-filled empty physical extents. Its generated process fixtures failed with
a data error before the fix and passed through both engines afterwards.
The subsequent integrated scenario adds the Linux extended-pointer read path,
superblocks at byte 512, metadata validation and application deployment.

`src/fs/cramfs/inode.c` retains the Linux `cramfs_read_folio` read algorithm,
with its pointer mapping factored into a shared C function as recorded above.
The userspace services replace VFS/folio mapping, locking,
bounded block acquisition and decompression. Native word loads become bounded,
endian-aware loads. Input uses the original seekable engine stream; metadata
is buffered separately, so direct extents beyond the old 272 MiB whole-image
limit remain readable. The original 7-Zip zlib decoder supplies the kernel
decompression contract, including zero-filled short pages. Linux's zlib wrapper
is not duplicated. C remains C, with a narrow C ABI in `cramfsreader.h`.

Two narrow safety/correctness deviations from the pinned folio body are explicit:
previous-direct-extent additions reject 32-bit overflow, and an aligned final
uncompressed direct block retains its full page length. The pinned source caps
that final block to `offset_in_page(i_size)` even when zero, which loses a valid
full final page; the accompanying Linux format README requires a full page
unless the final page is partial. Tests cover both full and partial endings.

The existing 7-Zip directory walker, item publication and archive stream are
retained. Linux's VFS lookup/readdir/inode publication do not implement an archive
enumeration interface. Their disk contracts inform narrow adaptations: root
directory/offset and inode-type validation, bounded names, UID/GID and device
properties, UTF-8 paths, regular/symlink payload sizes, shifted roots, both
endiannesses, v1/v2 headers and rejection of unsupported feature bits.
Recognition uses the two endian magic values, as Linux does, without requiring
the informational text signature. Version-2 CRC covers the volume from its
superblock, excluding an optional leading prefix, and is checked incrementally.

Open/read/extract errors and cancellation propagate through the engine ABI;
failed opens clear partial state, and handlers can be reused. Cached streams
retain their owning handler and their own file metadata. Formatting changes
within adapted C++ functions are separate from these functional changes; untouched
Linux C control flow and dependency organization remain recognizable.

## Module and platform boundaries

The GPL C reader is linked into a separate `sevenzip_cramfs` format module,
following the existing exFAT deployment boundary. That module has no unRAR code.
The dynamic engine omits its built-in Cramfs registration and loads this module
from `Formats`; CMake deployment and Unix/Windows installation include it and
its license notice. The same authoritative handler retains the explicit legacy
LZMA/enlarged-block branches, whose flags are outside Linux's feature set.

Standalone `7zz` retains its legacy Cramfs reader, including the sparse fix;
modern Linux Cramfs support belongs to the application's dynamic engine and
format module. A dynamic runtime copied without the module lacks Cramfs support.
Runtime overlays/provenance include the adapted handler in both engine builds.
No sibling checkout, widget or archive transport is modified.

Kernel VFS mounting, block/MTD acquisition, mmap/XIP and page-cache services are
outside this read-only archive adapter. The copied read path uses 4096-byte pages;
native kernel page sizes are not encoded independently in standard cramfs images.
Explicit historical 7-Zip enlarged-block formats keep their existing reader.
This scenario does not establish completion of the other filesystem modules or
EROFS in the parent review.

## Validation

Deterministic fixtures check exact extracted bytes for sparse, ordinary, raw,
direct and mixed blocks, discontiguous/shared extents, both byte orders,
v1/v2, shifted roots and 512-byte prefixes. Full and partial final pages,
short zlib output, explicit legacy LZMA/8 KiB blocks, empty directories/files,
symlink payloads, nested Unicode names and ownership metadata are covered.
Malformed root/name/type/features, bad pointers, lengths, compressed data and
truncation must fail. Magic-only automatic recognition is exercised through
the production engine without forcing the format.

Module ABI tests exercise cancellation, source/output errors, reuse, random
seeks, stream lifetime and a virtual seekable image whose direct payload lies
at 1 GiB. An input failure after open verifies extraction error propagation.
ArchiveBackend discovers the deployed format as read-only, lists, tests and
extracts a Unicode/prefixed image without modifying it. Temporary runtime copies,
images and output directories are removed by fixture ownership.

Local macOS arm64/Qt 6.12 focused checks passed for `cramfs`, `cramfs_module`,
`runtime_cache`, `upstream_baseline`, `upstream_inventory`, `developer_workflow`,
`exfat`, `exfat_backend`, `brotli`, `external_codecs` and `codec_probe`.
The source baseline verifies against the read-only sibling. Linux/Windows
incremental runtime build receipt: `20261019T063013Z-0e163113`.
All eleven focused suites passed on Linux x64; ten passed on Windows x64/MSVC
in receipt `20261019T063038Z-1cfda2d6`. Windows's new backend discovery case
hit its production deadline during that concurrent batch; its unchanged,
focused rebuild/test passed in `20261019T063232Z-8a90ac73`. The initial timeout's
cause is unconfirmed. The final local module rebuild/test also passed.

A supplementary native-kernel mount comparison was unavailable: the configured
Linux guest's `6.8.0-146-generic` kernel has no Cramfs filesystem driver. All
read-only loop-mount attempts and temporary fixtures were cleaned. This is not
kernel runtime acceptance; source comparison and module behavior are separate
evidence. No full, slow, physical-device, package or UI acceptance is claimed.
Incremental build trees and compiler caches are retained.
