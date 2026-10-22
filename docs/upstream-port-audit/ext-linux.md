# ext2/3/4 read-path review against Linux

Part of the [Linux format review](filesystems-linux.md), 2026-10-19. This first
integrated ext iteration covers classic indirect mapping, sparse reads, extent
tree boundaries, directory record boundaries and archive stream consumers.
The second integrates Linux metadata and inline-data readers. The ext row remains
open for the remaining read features listed below.

## Linux extent-tree inventory for archive consumers — 2026-10-22

Compared complete pinned `ext4_ext_precache`, its extent-cache/read/cleanup
helpers, `ext4_find_extent`, the read branches of `ext4_ext_map_blocks` and
`ext4_fiemap` with SDK `FillExtents` and all its consumers. The archive requires
an ordered inventory, including metadata extents beyond user `i_size` for verity;
Linux's existing depth-first precache walk supplies that complete traversal
without adding an independent tree decoder or repeatedly searching each extent.

Transferred `ext4_ext_path_brelse`, `ext4_ext_drop_refs`, `ext4_free_ext_path`,
`ext4_cache_extents`, `__read_extent_tree_block` and `ext4_ext_precache` into the
existing C `fs/ext4/extents.c` unit, with the required original path/header
accessors. The first five complete helper bodies match the pinned Linux bytes.
The source remains Torvalds Linux at
`58161ead29e50d70a317ea26b472e0c7508e69da`; the existing adapted unit and header
retain their original notices and C11 compilation. No new upstream unit is
needed. A private buffer/export adapter replaces block-device I/O and VFS extent
status caching, with at most one filesystem block per active tree level.

The precache control flow retains its original DFS/read/error/unwind branches.
The archive adds a leaf export hook, handles a depth-zero inode root through that
hook and uses the original no-cache flag so each validated leaf is exported once.
Inode semaphores are omitted for the private unpublished inode snapshot. Cleanup
depth is initialized explicitly so failure at any acquired level or visitor
releases every retained buffer. The maximum depth is checked before allocation.
Source HRESULTs, OOM and progress-callback failures remain distinct at the C++
boundary. Kernel allocation/write/journal mutation code is not transferred.

Original block/reference/checksum validation remains authoritative. The archive
also retains its existing parent upper-bound check, using the original path's
next index at each ancestor. It rejects an extent crossing that bound before
exporting it. Consistent non-root leaves must be nonempty, matching the original
`ext4_ext_map_blocks` read check. Only validated data/unwritten ranges are exported;
the existing stream owner derives gaps after the next range has validated, so a
failed child cannot create a fake zero tail. Readable prefixes retain the existing
limited-access behavior. Mandatory journal protection and complete EA/verity
validation remain strict at their existing boundaries.

Every GPL extent inventory consumer now uses this C walk: regular files, sparse
and unwritten reads, directory/htree range inventory, raw symlink storage,
allocated EA-inode values, journal protection and verity backing metadata.
Existing C++ streams still own transport/crypto/verification and retained mapping
vectors. The SDK recursive tree decoder is compiled only in the non-GPL engine
profile. This closes the selected eager tree-inventory transfer, not the whole
Linux extent module or broader ext namespace/source audit.

Existing behavior regressions cover malformed roots, empty children, depth five,
checksums, parent boundaries, first/later failed children without invented holes,
source Read/Seek failures, cancellation, Unicode/spaces, protected metadata,
retained streams, native crypto/EA/verity and directory fallback. Added small
journal fixtures cover external extent trees through depth five, exact owner
data/unwritten-zero/tail bytes, foreign-file alias rejection, failure at the
deepest leaf, empty failed-open state and successful reuse after cancellation.

Validation: macOS passed all eight ext/code/provenance suites, including the
new journal ABI cases, and the incremental application build. Linux and Windows
passed the same eight in `20261022T125042Z-19b13ca4`; prepared dynamic and
standalone SDK engines were rebuilt through the existing runtime workflow.
Source comparison confirms the SDK `FillExtents` branch is unchanged after
selecting its existing build profile, and all 303 pinned Linux hashes match.
The final source/provenance checks and fixture-remnant inspections passed;
no disposable ext fixtures remained locally or on either guest. Existing build
trees and compiler caches were reused. No full gate, slow suite, new native
kernel-mount/device check, UI appearance acceptance or release install was run.
Overall filesystem and ext source completeness remains open.

## Complete archive classic-map consumer integration — 2026-10-22

Rechecked the complete pinned `ext4_get_link`, `ext4_encrypted_get_link`,
`ext4_inode_is_fast_symlink`, indirect lookup and SDK stream factories/callers.
The remaining GPL consumers of SDK bitmap mapping were raw slow-symlink storage
and direct directory streams without range inventory. Both now use the same
original Linux lookup as regular files, allocated EA-inode data, directory
enumeration and journal protection. Fast/inline storage retains its existing
original inode/inline readers; it does not require a block lookup.

The raw symlink boundary still exposes exactly one initialized filesystem block,
not the inode's advertised file size. Holes and protected/out-of-image references
remain errors. Plain target termination and ownership stay in the transferred
Linux symlink reader. Encrypted raw storage bypasses regular-file data-unit
decryption and reaches the existing fscrypt target decoder unchanged, including
its storage-size and ciphertext-length checks and native no-key presentation.
Public target streams remain owned decoded buffers, so source/handler release
does not invalidate them. No reader-policy gate, source write, repair or journal
replay was introduced.

The SDK dense-map class and traversal are now compiled only into the non-GPL
engine profile. Dead GPL crypto/prefix adapters were removed from that fallback;
its algorithms and existing local fixes remain. Its seek initializer again has
the original no-argument form, with the same behavior as its former default-false
argument. This closes integration of the selected classic mapping service in
the GPL archive module, not all ext read features or source scope: extent mapping,
namespace presentation and other remaining audit entries are still open.

Existing ABI error/isolation/retry checks now run for both classic and extent
symlinks; the nesting/indentation change keeps the prior cases. Additional bounded
classic variants check first-block termination, embedded NUL, larger advertised
size, unread second storage block and retained target ownership. A public raw
directory stream returns exact noncontiguous physical-block contents, supports
backward seeks after Close and never reads the intervening unused block. A
checksum-correct bitmap variant of a native encrypted symlink, with inode size
reduced to one byte, returns exact keyed plaintext and the native no-key target.
Only its map/inode checksum changes; ciphertext and encryption context remain
those of the existing fixture. Existing CLI coverage retains mandatory source
invariance, EA/cluster fast-link classification and protected metadata rejection.

Validation: macOS passed the eight ext/code/provenance suites and incremental
application build. Linux and Windows passed all eight in
`20261022T122951Z-83fa54ba`, including both prepared SDK engine profiles and
the GPL module. Review strengthened the new crypto regression to select the
converted inode explicitly: native cases share a plaintext target. The final
test passed again on macOS and both guests in `20261022T123203Z-a6bf1e93`.
All 303 pinned Linux reference hashes match; no new Linux unit or dependency
was added, and the transferred mapper/symlink bodies were not changed.
Existing incremental runtime/build/compiler caches were reused. No full gate,
slow suite, new kernel-mount/device check, UI appearance acceptance or release
install was performed. Overall ext and filesystem source closure remains open.

## Linux indirect lookup for directory ranges — 2026-10-22

Compared the pinned Linux indirect lookup, its buffer validation/release branches
and directory mapping consumers with SDK `ReadDir`, `GetStream_Node`, allocated
range inventory and indexed-directory callbacks. Classic directories now use the
same original C lookup as regular files and journal bootstrap. The scan retains
allocated ranges rather than one entry per logical block; confirmed holes are
skipped and an unknown mapping tail is not presented as zero-filled directory
metadata. A failed lookup preserves already confirmed ranges and the existing
limited-read warning/consent flow. Htree and linear fallback consumers retain
their existing range checks, checksum/name handling and callback cancellation.
Directory blocks do not enter regular-file fscrypt data-unit decryption;
existing filename/context handling remains authoritative.

The private C boundary now owns an inode/root snapshot and up to three retained
indirect buffers per lookup object. Only unreferenced buffers can be replaced.
The original Linux helpers and map body are unchanged; the narrow buffer adapter
marks a buffer reusable only after its read and original block-reference check
succeed. Failed/invalid buffers remain unvalidated and are reread on retry.
Regular-file streams own this object together with their original source, volume
and crypto state. The journal bootstrap's transient wrapper retains its existing
callback lifetime and protection-tree behavior. Source bounds, protected-metadata
ownership and explicit read-only behavior remain in force; no write, repair or
journal-replay service was added.

A bounded 8 MiB image declares a classic directory ending at logical block
`UINT32_MAX`. The old dense map rejected its 2^32-block count before allocation;
the new reader lists the final entries and copies their exact small payloads.
The last allocated range's end is consumed with 64-bit arithmetic. This is a
short sample/directory test, not a maximum-file extraction or a slow suite.
Additional ABI rows cover direct directory data, indirect metadata Read/Seek
failures, sparse holes, protected-pointer corruption, callback errors, cancellation,
independent copying and healthy reopen. Retry regressions ensure a transient
metadata read error can recover and repeated invalid-reference reads never become
successful cached reads. Existing regular-file, EA-inode, encrypted-file,
journal-owner and retained-stream checks exercise the shared lookup boundary.

This transfers the selected classic directory mapping path, not all ext namespace
logic: SDK directory presentation and extent mapping remain under review, and
standalone retains its non-GPL reader.

Validation: all eight ext/code/provenance suites passed on macOS and Linux
(`20261022T121508Z-6f0d0c97` for Linux). The same Windows run passed seven
suites, while CLI `ext` reached its 45-second suite timeout. No child process
remained; its single disposable fixture directory was removed. An unchanged
verbose diagnostic ran all 50 CLI tests successfully in 37.651 seconds
(`20261022T121844Z-b4ccfe40`), then the ordinary Windows `ext` wrapper passed
with the original timeout (`20261022T122025Z-db20bcba`). The initial timing
failure was not reproduced; no timeout increase or functional workaround was
introduced. The incremental macOS application build passed. All checks reused
existing runtime/build/cache state. The three original indirect helper bodies
still match the pinned source bytes, and all 303 reference hashes match.
No new Linux unit was added. No full gate, slow suite, physical-device test,
new kernel-mount comparison, UI appearance acceptance or release install was
performed; the broader ext and filesystem audits remain open.

## Linux indirect lookup for journal system-zone bootstrap — 2026-10-22

Compared the pinned Linux `fs/ext4/block_validity.c` reserved-inode protection
loop and `fs/ext4/inode.c` mapping/validity callers with the archive's complete
`extblockreader.c` services and SDK bootstrap callback. The journal's classic
mapping still built a dense SDK pointer vector even though ordinary regular-file
streams already used Linux's indirect lookup. Bootstrap now calls that same
original C lookup for each requested range, including coalesced data and holes;
the dense journal vector and its separate indirect traversal are removed.
The original system-zone loop, ownership, overlap detection and publication
remain authoritative. Extent-based bootstrap retains its existing adapter.

Source acquisition reuses the read-only physical input adapter and preserves
progress cancellation and source HRESULTs through the C callback. The kernel's
journal-owner exception bypasses block-reference validation, so archive adapters
retain explicit source bounds before acquiring an indirect block and validate
the returned physical run with the existing Linux block-validity service.
This is necessary during bootstrap, before the system-zone tree is published,
as well as for later journal-owner stream reads. It introduces no source writes,
repair or journal replay. An unreadable mandatory journal map still prevents
publishing an unverified protection tree; this does not add a policy gate to
ordinary recoverable file reads.

Small disposable images cover single, double and triple indirect journal maps,
exact head/hole/tail reads by the journal owner, independent user data, rejection
of a foreign file aliasing journal data, metadata overlap and out-of-image data
and index pointers. ABI checks cover Read/Seek source failures, cancellation,
empty state after failure and successful reuse. CLI checks compare complete
single/double journal output, including all sparse zeros. Triple-indirect checks
read bounded samples from a 1 MiB physical image, without extracting its larger
logical journal. The new ABI test was corrected to accumulate legal short Reads
across map boundaries. Directory allocated-range inventory remains an SDK
consumer; this iteration does not claim closure of all ext mapping paths.

Validation: all eight ext/code/provenance suites passed on macOS and Windows
(`20261022T120225Z-ab7fcaff`). Linux exposed a missing explicit `<climits>`
include, corrected before its final eight-suite PASS in
`20261022T120435Z-510cf06c`. The final include was rebuilt and the ABI suite
passed on Windows again (`20261022T120434Z-adea81c4`); macOS repeated all eight
on the final source and its application build passed. Existing incremental
build trees, runtime snapshots and compiler caches were reused. The three
transferred indirect helper bodies remain byte-identical to the pinned Linux
source, and all 303 reference hashes match. No new Linux unit or dependency was
added. Disposable fixture directories were empty after validation. No full gate,
slow suite, physical-device check, new kernel-mount comparison or release install
was run. The broader filesystem goal and ext read-feature audit remain open.

## Linux indirect lookup and bounded regular-file streams — 2026-10-22

Compared complete pinned Linux `fs/ext4/indirect.c` bodies and callers with SDK
`FillFileBlocks`, `CClusterInStream2` and `GetStream_Node`. The archive reader
allocated one pointer per logical file block before exposing a stream, including
holes. Large sparse files therefore required memory proportional to their logical
size; a stream with exactly 2^32 logical blocks was rejected by the unsigned-count
conversion before reading any data. Linux's lookup visits only the requested
branch and reports the mapped run or known hole.
The pinned ext2 `ext2_max_size` and `ext2_block_to_path` were also compared:
the normal ext2 size ceiling derives from its 32-bit sector count and fits the
new lookup's logical range. This is not a new restriction on that supported range.

Transferred `Indirect`, `add_chain`, `ext4_block_to_path`, `ext4_get_branch` and
the read/cleanup paths of `ext4_ind_map_blocks` into their original C path. The
three helper bodies match the pinned source bytes. The map body retains its
read/coalescing/hole/error/cleanup branches; unused allocation locals and the
create/allocation/journal-write branch are omitted, with an explicit unsupported
result for that branch. The archive boundary calls it with flags zero. The source
repository remains Linux Torvalds at
`58161ead29e50d70a317ea26b472e0c7508e69da`, linked above; the full unit hash was
already inventoried and the unit is now classified as adapted. Original notices
and C11 compilation are retained.

Private synchronous buffers replace the block cache and inode semaphore on the
owned in-memory inode/root snapshot. At most three indirect blocks are acquired
for one lookup and freed on every success/error path. Original block-reference
checks protect inode and indirect arrays, filesystem bounds and system zones,
including the journal-owner exception. The original `ext4_map_blocks` type fields
are retained; buffer-state flags are private lookup results, not on-disk fields.
No new filesystem decoder, allocation or source-writing service is introduced.

The module's regular-file streams, including internal allocated EA-inode reads,
now use that lookup lazily and cache only the current mapped run/hole. Existing
fscrypt data-unit handling is reused; required-allocation reads still reject
holes and xattr publication still requires complete value/hash verification.
Metadata and data source HRESULTs remain distinguishable from corrupt mappings;
opening a stream can now defer a transport failure to Read. Extraction keeps its
per-file error, cancellation and independent-selection behavior. Only a confirmed
hole produces zeros. Every data read seeks to its mapped physical address, so
interleaved metadata acquisition cannot leave the data cursor at a stale offset.

Returned streams share ownership of the original C volume/system-zone state as
well as the source and crypto state. Close/reopen/release of the handler cannot
invalidate or retarget them. Original directory allocated-range inventory and
system-zone bootstrap mapping remain SDK consumers; this iteration does not claim
closure of all ext mapping/namespace code. Standalone retains its non-GPL reader.

The transferred mapper's logical index is Linux `ext4_lblk_t` (32 bits). A larger
declared bitmap-file size can still expose addressable data, but an out-of-range
Read fails rather than wrapping to the first block. The known limitation is
reported globally and on the item, using EN/RU per-opening consent and the visible
limited-access mode. An independent readable file remains copyable. The bounded
range regression uses an 8 MiB image declaring 256 TiB, reads only small head,
middle-hole and final-block samples, retains the stream after handler reuse and
checks the first unaddressable position. It does not allocate/extract the logical
file or introduce a slow maximum-file suite.

Validation: macOS passed the eight ext/code/provenance suites and the three
language/EN/RU filesystem-read UI suites; the incremental application build passed.
Linux passed all eleven in `20261022T112048Z-aef06afc`. A Win32 `min` macro
conflict in the new C++ glue was corrected with an explicit standard-library
template call; Windows then passed all eleven in `20261022T112459Z-a3086b04`.
The equivalent C++ change was rebuilt and `ext_stream` passed again on macOS and
Linux (`20261022T112500Z-14a2e3c2`). Existing runtime snapshots, build trees and
compiler caches were reused throughout.

Additional checksum-correct bitmap variants of native fixtures exercise this
new stream rather than the original extents transport. AES/default-unit and
512-byte-unit encrypted files return exact golden plaintext, sparse zeros and
tail bytes. Classic EA-inode values return exact bytes; holes remain rejected
even after forging correct hashes for an all-zero value, while independent data
stays readable. The affected `ext` and `ext_fscrypt_cli` suites passed on all three
platforms (final guest receipt `20261022T113806Z-23d94c23`). Existing regressions
retain source errors, per-file isolation, cancellation, special/journal ownership,
Unicode/spaces, malformed paths, backward/cross-range seeks and owned streams
after Close/reopen/release. Tests accept deferred read errors without requiring
an eager GetStream failure.

EN/RU UI checks cover confirmation/decline, visible mode, independent copying,
refresh/reopen scope and unchanged source bytes. Current Cocoa warning JSON is
420×214 (EN) and 420×275 (RU), with visible fitting reason labels; rendered Windows
pixel parity is not claimed. All 303 pinned Linux reference hashes match, with
the new adapted unit still owned by the existing source inventory. No full gate,
slow suite, physical-device check, new native kernel-mount comparison or release
installation was run. The broader ext/module and parent format audits remain open.

## Inode inventory isolation and useful read-only opening — 2026-10-22

Compared pinned Linux `__ext4_get_inode_loc`, `__ext4_iget`, inode checksum,
extra-inode/inline initialization and role checks with the complete SDK inventory,
reference lookup, directory scan and system-zone consumers. Linux's inode lookup
uses a table block and reports failure for the requested inode; the eager archive
inventory previously read a whole group's table in one operation and rejected
the volume when any allocated inode failed initialization. The Linux/SDK source
repositories and full revisions remain those recorded below. No inode parser,
checksum algorithm or filesystem decoder was replaced or newly implemented.
The adapted archive boundary is `CPP/7zip/Archive/ExtHandler.cpp`, principally
`CHandler::Open2`, limitation properties and reset. Original Linux C bodies
remain unchanged; Linux supplies per-inode operations rather than the archive's
whole-volume inventory lifecycle.

The module now reads table blocks independently through the existing read-only
source adapter. After a short or failed block read, allocated inodes in that block
are retried through full individual reads using the existing bounded raw-inode
service. Only a completely reread inode enters the original validation; a partial
block prefix or previous group's buffer contents never become inode metadata.
This can preserve a usable root or file record when a different record in the
same block is unavailable. Valid blocks remain usable. Allocated inodes whose SDK parse,
original Linux initialization or required inline file payload decoding fails are
omitted; allocated empty user-inode slots are also reported as unavailable.
Linux checksum, extra-size, flags, extent-root and inline checks still reject
the same data. Directory entries referencing an omitted inode follow the existing
incomplete-directory path, retaining later valid records and independent files.
Inline directories keep records already emitted by the original Linux reader
before an error, retain normal path validation, and report an incomplete directory;
the unparsed suffix is not fabricated as directory records. An inline directory
without any usable emitted record is omitted.
The inode inventory adds an explicit read-only limitation, translated through the
existing EN/RU confirmation and persistent per-opening mode.

Fatal results are distinct from unavailable data. C allocation failures propagate
as E_OUTOFMEMORY; source abort/OOM and every explicit progress callback failure,
including S_FALSE, abort opening. Progress is queried only when a callback exists.
The root reference must be present and nonnegative before system-zone setup or
other consumers use it. No fabricated root or inode metadata is substituted.
Missing mandatory journal/service dependencies still fail original system-zone
setup. Superblock, geometry, group descriptors and allocation bitmap checks remain
mandatory; this change does not claim recovery from those failures or relax a
later essential encryption-context or protected-root mapping check.

Source comparison is complete for the non-root inventory failure scenario, not
the full ext read audit. Existing standalone table loading remains unchanged;
the application module supplies the Linux-dependent validation and partial
inventory mode. Both engine profiles continue to compile the shared SDK source.

Validation: eleven focused suites passed on macOS, Linux and Windows: `ext`,
`ext_stream`, `ext_fscrypt`, `ext_fscrypt_cli`, `ext_fscrypt_backend`,
`ext_xattr_backend`, `upstream_baseline`, `upstream_inventory`,
`filesystem_read_ui`, `filesystem_read_ui_ru` and `language`. Guest receipts:
`build/runners/hyperv/20261022T103128Z-147dbae4/{linux,windows}`. After the
localized inline-directory prefix and individual-inode retry follow-ups, the
eight ext/code/provenance suites
were rebuilt and passed again on all three platforms; final guest receipts:
`build/runners/hyperv/20261022T104712Z-95b7430d/{linux,windows}`. Runtime snapshots,
build trees and compiler caches were reused. The incremental macOS application
build also succeeded; the final module/runtime was rebuilt by the focused checks.

Regressions cover corrupted/empty allocated inodes, invalid extra sizes and
flags/extent roots, surviving independent files and later links, table block
read/partial-read/seek failures, successful individual-inode retries preserving
the root or an independent file, fatal abort/OOM/callback results, healthy reopen,
unavailable root/journal dependencies and exact inline-prefix file bytes. Existing
security cases require rejection or explicit limited listing with the bad item
omitted, never filesystem metadata exported as data or a fabricated valid empty
file. EN/RU UI checks cover consent/decline, visible mode, exact copying, refresh,
reopen scope and source invariance. Current Cocoa warning JSON is 420×227 (EN)
and 420×301 (RU), including both namespace and inode limitations with fitting
visible labels. Rendered Windows pixel parity is not claimed. All 303 pinned
Linux reference hashes match. No full gate, slow suite, physical-device test,
new kernel-mount comparison or release installation was run. Broader ext source
closure remains open.

## Unavailable symlink targets and useful read-only opening — 2026-10-22

Recompared pinned Linux `ext4_get_link`, `ext4_encrypted_get_link`, inline target
handling, fscrypt presentation and block-reference checks with SDK eager target
discovery, properties, GetStream and extraction. The source repositories and full
Linux/SDK revisions remain those recorded below; no new dependency, link parser
or cryptographic decoder was introduced. Linux reports a target-read failure on
the inode operation; the archive's eager discovery previously failed the entire
opening after a decoded link's storage read or target presentation failed.

The application module now keeps the named link and independent namespace data
when its target is unavailable. Its target and exported size are left unknown,
with a per-item error and archive read-only limitation. Subsequent good links
are still decoded. Eager storage read/seek errors use the existing source-only
adapter; semantic target errors retain the original Linux reader result. Abort,
allocation failures, explicit callback errors and internal failures still abort
opening without exposing partial items. Both warning languages use the existing
per-opening consent, visible limited-access indication and source-write guard.

Symlink mapping checks formerly performed during global system-zone validation
are deferred to the target reader's existing full mapping validation. Protected
metadata aliases are rejected before target bytes can be read; source bounds,
allocation requirements, extent checks and ciphertext length validation remain
mandatory. Mandatory inode initialization, inode checksums, fast-link acceptance
and encryption-context validation are unchanged. This scenario starts from a
successfully initialized inode; it does not claim recovery from every malformed
inode or a missing essential cryptographic context.

Copying retries the target read. Persistent failure reports a per-link data error
without publishing partial target bytes, and processing continues to selected
healthy files/links. A successful retry exports the complete owned decoded target;
its actual stream size is used for extraction acceptance instead of the raw inode
size, which may exceed Linux's target termination bound. Direct GetStream keeps
the original source HRESULT. A healthy reopen clears the limitation. Standalone
retains its SDK profile; both engine profiles still compile the shared source.

Validation: the incremental macOS application build succeeded. Eleven focused
suites passed on macOS, Linux and Windows: `ext`, `ext_stream`, `ext_fscrypt`,
`ext_fscrypt_cli`, `ext_fscrypt_backend`, `ext_xattr_backend`, `upstream_baseline`,
`upstream_inventory`, `filesystem_read_ui`, `filesystem_read_ui_ru` and `language`.
Guest receipts are
`build/runners/hyperv/20261022T101603Z-ea092026/{linux,windows}`; existing runtime,
build trees and compiler caches were reused. New public behavior checks retain
an independent file and later valid link after a first target failure, reject
metadata aliases, exercise read/partial-read/seek errors, require exact retry
output and healthy reopen, and preserve fatal abort/OOM/callback results.
Native encrypted fixtures with invalid zero/oversized ciphertext lengths list
with a limitation both with keys and without keys; failed target extraction
emits no bytes, while the independent encrypted file decodes exactly. EN/RU UI
tests cover consent/decline, visible mode, exact copying, refresh, reopen scope
and unchanged source bytes. Current Cocoa warning JSON has fitting visible
reason labels at 420×214 (EN) and 420×275 (RU); rendered Windows pixel parity is
not claimed. All 303 pinned Linux reference hashes match. No full gate, slow
suite, physical device, new native kernel-mount comparison or release installation
was run. The broader ext audit remains open.

## Optional extended attributes and useful read-only opening — 2026-10-22

Compared pinned Linux `fs/ext4/xattr.c` getters, EA-inode role/refcount/hash
checks, `acl.c` conversion and `__ext4_iget`'s external-block address check with
the archive inode inventory, eager attribute export and fscrypt consumers.
The Linux baseline remains
`58161ead29e50d70a317ea26b472e0c7508e69da` from the repository linked above;
SDK integration remains based on
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. No new source dependency or
independent value decoder was introduced.

The dynamic module now confines optional attribute failures to the affected
value or external container. Main data, other inodes and independently validated
attributes remain available. Invalid EA-inode roles, link/reference counts,
sizes, mappings, hashes and ACL encodings do not publish a value. A failed value
does not suppress later entries in a structurally validated container. A failed
external block preserves valid in-inode attributes; invalid external entries or
checksums never become an unchecked list. Metadata source read/seek failures are
localized through the existing read-only input adapter. Cancellation, allocation
failure, explicit callback errors and internal interface failures still propagate.

The archive inventory defers external xattr validation to its consumers. The
original Linux `ext4_inode_block_valid` check is now performed before an external
attribute block read in the visitor and getter, rather than rejecting the entire
archive during inventory. This also protects the strict fscrypt getter. Bounds,
system-zone validation, container checksums and complete value verification remain
mandatory before export. This differs deliberately from Linux's inode-acquisition
refusal for an invalid `i_file_acl`: useful independent archive data can be offered
with explicit consent. Internal inline-data and encryption contexts are not
optional exported metadata; their decoding and validation have not been relaxed.

EA-inode link counts are checked by the original attribute getter, not reconciled
against directory entries, because these service inodes have no directory links.
Affected native items report unavailable attributes; valid exported attribute
streams do not inherit that error. The archive reports a read-only limitation,
using the existing per-opening confirmation, visible mode and source-write
prohibition. Healthy reopen clears the limitation. Standalone `7zz` retains the
non-Linux reader profile; the new partial attribute export belongs to the GPL
module, with shared SDK code still compiled in both engine profiles.

Source scope is complete for this optional export scenario, not for all ext
inode initialization, symlink or encryption failure isolation. In-inode structural
validation is still part of mandatory inode initialization; this change does not
assert recovery from an undecodable inode or unsupported essential payload.

Runtime validation: incremental application build succeeded on macOS. All eleven
focused suites passed on macOS, Linux and Windows: `ext`, `ext_stream`,
`ext_fscrypt`, `ext_fscrypt_cli`, `ext_fscrypt_backend`, `ext_xattr_backend`,
`upstream_baseline`, `upstream_inventory`, `filesystem_read_ui`,
`filesystem_read_ui_ru` and `language`. Guest receipts are
`build/runners/hyperv/20261022T100331Z-6c22fd26/{linux,windows}`. Both runtime
profiles were prepared with the existing build/cache directories before checks.
The CLI regressions retain exact main-file/alias and healthy attribute bytes
while omitting bad EA-inode values, ACLs and invalid external containers. They
also exercise protected metadata pointers without exporting metadata bytes as
attributes. ABI tests inject read/seek failures, cancellation, allocation errors
and explicit callback failures, verify per-item diagnostics, and require healthy
reopen to clear the mode. EN/RU UI checks cover consent/decline, persistent mode,
exact copying, refresh, per-opening scope and unchanged source bytes. Current
Cocoa warning JSON reports 420×214 (EN) and 420×262 (RU), with visible fitting
reason labels. This is geometry evidence, not rendered Windows pixel parity.
All 303 pinned Linux reference hashes match. No full gate, slow suite, physical
device, new kernel-mount comparison or release installation was performed.

## Incomplete directory namespace and useful read-only opening — 2026-10-22

Recompared pinned Linux `ext4_readdir`, directory checksum/record validation,
`dx_probe`/`ext4_htree_next_block`, the complete transferred index walker and
SDK ParseDir/ReadDir, parent/path construction, properties and extraction callers.
Linux skips checksum/bad-record blocks and confines directory I/O errors to the
inode operation; the SDK's eager scan instead failed the entire archive opening.
The archive adaptation now retains useful namespace data with explicit limits.
It does not claim identical damaged-tree recovery to a kernel mount.

The application module reads known mapped directory blocks independently.
Unavailable source blocks, checksum failures and invalid record suffixes retain
previous entries and permit other blocks/directories to be read. A known mapping
prefix is usable without filling the unknown tail with zeros. Initial directory
data seeks are deferred to the block read, preventing an unavailable first block
from suppressing later blocks. Already decoded inline directory payloads retain
only their valid record prefix.
Indexed traversal reuses the original walker; after a recoverable failure the
existing bounded linear path can read the remaining mapped blocks. Already emitted
blocks are tracked so fallback cannot publish the same entries twice. Normal
successful htree enumeration still ignores unreachable stale leaves.

An entry whose target is invalid or absent from the decoded inode inventory is
omitted without publishing a fabricated file or discarding the following valid
entries. The inode-number bound comes
from the superblock, not the shorter inventory ending at the last allocated
inode; an absent inventory entry is an unavailable target, not an invalid record
boundary. A regression places it between two readable entries in the same block.
Root identity comes from the
validated root inode; directory contents do not depend on readable dot records.
The original named-parent edges and parent-cycle/path guards remain in force.
Readable directories whose name was lost use the original auxiliary unknown
namespace, with a real parent item for their descendants rather than flat paths
that could collide with root files. This is an unverified namespace, not a repaired
directory tree. Recovery of conflicting/cyclic parent graphs remains outside this
iteration; their existing guards have not been relaxed. A root inode that cannot
be initialized under the declared format still cannot provide this opening mode.

Source EOF/ordinary Read/Seek failures are distinguished from progress callbacks.
Abort/OOM, callback failures (including S_FALSE), internal interface failures and
fscrypt context denials remain fatal. Context validation is not bypassed by the
linear path or unknown namespace. A failed physical seek also invalidates the
mapped stream's remembered cursor: a retry must seek again rather than return
bytes from the old position. This shared SDK stream correction reaches both
engines; partial namespace opening remains in the separate GPL module.

Global read-only limitations and per-directory Error properties report an
incomplete/outdated file list. Link counts cannot be reconciled against a partial
enumeration, so that condition no longer turns the usable opening into a fatal
listing error. Independent header/inode/checksum validation remains effective.
Selecting an incomplete directory reports DataError while its readable selected
files continue. The existing EN/RU per-opening consent, Cancel default, persistent
badge, refresh/reopen and exact-copy flow apply. No source writes, journal replay,
orphan processing or repair is introduced.

ABI regressions cover first/last root and child blocks, Read/Seek/EOF/Abort/OOM,
unknown parent names, per-directory results, exact healthy payloads, fatal callback
results, clearing diagnostics on reopen and mapped-stream seek retries. CLI
regressions now check the actual safety boundary: invalid inode/name/record entries
are absent, partial mode is visible and valid sibling data is exact. They no longer
require whole-volume rejection merely because one directory is incomplete.
All 303 pinned Linux reference hashes still match. The broader ext source audit
and parent format goal remain open.

Eleven focused suites passed on macOS arm64 in 40.07 seconds: ext CLI/ABI,
fscrypt CLI/ABI/backend, xattr backend, both provenance suites, language and both
EN/RU read-only UI suites. After the final inode-inventory-bound regression,
the eight code/provenance suites passed again in 8.24 seconds. Final Linux/Windows
CLI/source checks passed in `20261022T093650Z-99e5778f`; the four final ABI/backend
suites passed in `20261022T094100Z-b6462ad0`.

Linux also passed the seven ABI/backend/UI/language suites in
`20261022T092329Z-82db49b0`. Windows passed six in that batch; the first unchanged
exFAT UI case exceeded its 5-second codec-discovery wait (Qt observed readiness
after 6.4 seconds). The isolated unchanged Windows UI suite passed in
`20261022T092703Z-ce676ca0` without timeout/cache changes. The cause of the initial
delay is not established. Current Cocoa JSON confirms fitting translated labels
at 420×201 (EN) and 420×262 (RU) logical pixels; these checks do not establish
native Windows rendered parity. Consent/refusal, exact copying, persistent mode,
refresh/reopen and source preservation are exercised through the application.

The final incremental application build and diff check passed. Disposable local
fixture directories were checked empty; test owners clean their temporary
payloads. Current small receipts, compiler caches and build trees are retained.
No full/slow suites, physical-device, new native mount or release-installation
acceptance is claimed for this iteration.

## Readable prefixes before unavailable mapping metadata — 2026-10-22

Recompared Linux `ext4_get_branch`/`ext4_ind_map_blocks`, `ext4_find_extent`
and their read callers with the complete SDK indirect/extent materializers,
mapped streams and extraction loop. Linux requests the map for a selected range;
the SDK constructs a whole file map before returning its stream. A late
unreadable or invalid metadata block therefore discarded an already validated
file prefix. This follow-up extends the previous per-file I/O isolation without
introducing a second mapping algorithm.

The application's Extract path can now reuse the original materializers' known
contiguous prefix after a mapping DataError. The original stream is bounded to
that prefix; no unreadable indirect/extent tail is converted into sparse zeros.
Extent traversal rolls back a tentative gap before a child that supplied no
validated ranges. Earlier validated leaves, including real holes/unwritten
ranges, remain readable. Normal metadata, checksum, system-zone, path and range
checks still run. An incomplete map always reports per-file DataError, even when
its readable prefix happens to cover the declared file size. Healthy selected
siblings continue, while Abort/OOM and output/progress failures remain fatal.

This extraction mode covers regular files and fscrypt's complete data units.
Linux's policy validation bounds the data-unit size by the inode block size;
the known block prefix does not authorize reading an unknown crypto unit.
fs-verity still requires its complete validated mapping and verifier setup:
readable ciphertext/plaintext alone does not authorize bypassing verification.
Directories, EA/internal allocated-map consumers, symlink targets, public
GetStream and standalone retain their strict stream-construction contracts.
An incomplete map is not published as a valid directory, xattr or target string.
No source write, repair, journal replay or new consent bypass is introduced.
The supported result is a contiguous prefix, not arbitrary recovery of every
disconnected later range.

ABI fixtures cover direct/single/double-indirect boundaries, nested missing
subtrees, first/later invalid extent children, gaps before failed children,
verity metadata and a prefix equal to the full file size. Addressed Read/Seek,
EOF, Abort/OOM, test-only mode, destination/progress failures, strict public
GetStream and healthy retries remain covered. Existing metadata-alias and parent
range regressions now assert exact safe prefixes with an error, retaining the
prohibition on exposing filesystem/journal blocks. A child that has not validated
its preceding gap cannot add zero bytes to that prefix.

The existing 15 native synthetic fscrypt policy cases also run through the real
CLI with a checksummed first leaf and an invalid next leaf. Fourteen ordinary
encrypted files return their exact first plaintext block with an error; the
verity combination publishes no unverified bytes. A separately stored healthy
file still extracts exactly. Original fixture images remain unchanged; mutations
and CLI copies belong to automatically cleaned temporary directories. These
bounded tests are ordinary suites, not a new slow matrix. All 303 pinned Linux
reference hashes were rechecked without mismatches. Broader ext source review
remains open; this does not claim identical kernel recovery or native-mount
acceptance of the synthetic corrupt trees.

Local focused validation covered `ext`, `ext_stream`, `ext_fscrypt`,
`ext_fscrypt_cli`, both ext backend suites and both provenance suites. Seven
passed in the shared run; the ext CLI suite then passed in 4.88 seconds after
its old blanket-empty assertions were replaced by the exact-prefix contracts
described above. Final CLI/source checks passed on Linux and Windows in
`20261022T083858Z-352b5422` (14.087/43.377 seconds); module ABI and both backend
suites passed on both guests in `20261022T084026Z-39e995b5`. The application build
and diff check passed. Existing incremental builds/compiler caches and current
small receipts are retained; disposable fixture owners clean their temporary
data. No full/slow suites, native UI appearance, physical-device, new native-mount
or release-installation checks were run for this iteration.

## Per-file extraction source I/O isolation — 2026-10-22

Recompared the complete original SDK extraction/copy loop, both mapped streams,
indirect/extent construction and their consumers with Linux's per-inode
`ext4_file_read_iter`, `ext4_read_folio`/readpage acquisition and the connected
fscrypt, fs-verity and symlink adapters. An ordinary source HRESULT during one
selected file previously terminated the whole extraction, preventing an
independently readable next file from being copied. This is an archive batch
integration issue; Linux does not supply an archive extraction callback loop.

The dynamic module now uses a private per-file source/output adapter. Ordinary
source Read/Seek errors and premature source EOF become that item's DataError;
the next selected item is still processed. Explicit source parameters connect
the same adapter to indirect/extent metadata, initial/data seeks, mapped payloads,
symlink inode/block reads, fscrypt data-unit acquisition and fs-verity metadata
and data acquisition. The handler's shared source is not temporarily replaced.
Original mapping/decoding/verification algorithms and SDK CopyCoder are reused;
there is no alternate filesystem reader. The existing FAT extraction adaptation
provides the integration pattern rather than another independent batch algorithm.

Source Abort/OOM, output errors (including S_FALSE) and progress/callback failures
still stop the batch. Output status is checked independently before interpreting
a copy result, so a simultaneous source error cannot suppress a destination
failure. SDK CopyCoder retains the successfully reported plaintext prefix,
including bytes returned together with a source error. Crypto/verity paths still
withhold incomplete data units/blocks; unavailable metadata does not fabricate
a mapping or symlink target. This iteration isolates failed files, without
claiming recovery of every readable range inside an unmappable file.

The public GetStream interface retains its original source and HRESULTs; stream
ownership, bounds, verification/trust and context/key restrictions are unchanged.
Standalone receives the explicit-source plumbing but retains its existing strict
source-failure behavior without GPL dependencies. No source write, repair or
journal replay is introduced. A completed Extract call with per-file DataError
does not mean successful extraction of those files.

Behavior checks inject Read/Seek/EOF/Abort/OOM faults into direct/indirect data,
external extent metadata, symlink inode/block storage and fs-verity metadata/data.
Each injection must actually be reached. Extract and test-only modes distinguish
DataError from the healthy sibling's OK result; exact bytes, partial-error prefixes,
fatal destination/progress errors, public GetStream failures and healthy retries
are checked. Native synthetic fscrypt fixtures exercise source errors and a
separately stored next encrypted file across all 15 existing policy/data-unit
cases, including verity combinations; exact plaintext hashes remain the oracle.
The generators use bounded in-memory images and add no persistent test payloads.

All 303 pinned reference-unit hashes were rechecked without mismatches. Eight
focused local suites (`ext`, `ext_stream`, `ext_fscrypt`, `ext_fscrypt_cli`,
`ext_fscrypt_backend`, `ext_xattr_backend`, `upstream_baseline`,
`upstream_inventory`) passed on macOS arm64 in 8.09 seconds after both engines
were prepared in the existing incremental trees. Source-port completeness for
the broader ext read-feature scope is still under review.

Linux CLI/source checks passed in `20261022T082221Z-5e264864` (four suites,
12.714 seconds). Windows passed three suites in that batch; one existing invalid
bitmap CLI extraction exceeded its subprocess timeout of 10 seconds. The isolated
unchanged ext suite passed in `20261022T082359Z-2d6268ab`, without timeout/cache
changes; the cause of the initial timeout is not established. Final module ABI
and fscrypt/xattr backend suites passed on both guests in
`20261022T082438Z-96422880`. The final sibling-fixture refinement also passed
both local ABI suites. The application build and diff check passed. Fixtures
are owned/cleaned by their tests; current small receipts, incremental trees and
compiler caches are retained. No new native mount, physical-device, UI appearance,
full/slow-suite or release-installation acceptance is claimed.

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
layouts; the later [fscrypt integration](#fscrypt-software-reads) supplies explicit
key/no-key handling. An explicit
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
readahead and kernel trust keyrings are omitted. The enabled signature path and
explicit archive trust are described below; they use owned certificates rather
than a host kernel keyring.

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
features remain permitted by the read-only feature check. The later encoding
gate below validates the declared casefold charset; fscrypt reads are described
in their own section. Inode checksum rules already match Linux's creator-OS guard;
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
read bodies unchanged apart from explicit C pointer casts, replacement of
GNU's omitted ternary operand for MSVC and the deferred external-block address
guard described above. New dependencies are pinned in the
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

### fscrypt software reads

The integrated scenario covers encrypted ext4 directory, file and symlink reads
with explicit raw master keys, plus Linux's no-key view. The pinned inventory
includes complete `fs/crypto` units and the selected generic crypto dependency
closure; the inventory does not establish completed source transfer.

Initial body/caller comparison covers policy/context conversion and validation,
v1/v2 KDF branches, per-mode selection, IV generation, data-unit decryption,
filename presentation and symlink reads. Pinned Linux supports AES-XTS/CTS,
AES-CBC-ESSIV/CTS, SM4-XTS/CTS, Adiantum and AES-HCTR2 policy pairs. All remain
in the transferred scope, alongside direct-key/inode-IV policies, configurable
data units, no-key names and existing fs-verity composition.

The 2026 Linux library already provides usable portable AES CBC/CTS/XTS/XCTR
bodies. Selected original bodies, SHA-2/HMAC and exact fscrypt HKDF
remain under their original paths, with notices. A C11 static target supplies
the dynamic format module and a standalone C check. Portable Crypto API services
replace kernel allocation, mapping and registration at the boundaries described
below. The LGPL standalone engine does not link GPL fscrypt code and explicitly
refuses encryption-flagged nodes.

The generic AES and SHA algorithm bodies retain the pinned control flow.
Host adaptations replace kernel includes/exports and compiler barriers, provide
explicit key zeroization, endian/unaligned access and overflow services, and
name the initial AES encryption-key member for C11/MSVC. The encryption argument
uses the common initial member address instead of GNU transparent unions.
Pointer alignment calculations use `uintptr_t` on LLP64. Kernel acceleration,
FIPS initialization/registration and unused AES MAC/AEAD modes are omitted.
The copied XTS field multiplication and weak-key checks remain original;
kernel skcipher registration wrappers are omitted. Crypto utility routines use
their generic paths rather than compiler-specific constant-size shortcuts.
The generic SHA-224/SHA-256 and SHA-384/SHA-512 modules retain their HMAC APIs;
the archive scenario needs SHA-256 and HMAC-SHA512. Generic word accesses use
the kernel's compiler aliasing contract (`-fno-strict-aliasing` with GCC/Clang).

The selected dependency group includes original SM4 block/key functions,
SipHash2-4 and base64url. SM4 aliases for assembly consumers are omitted.
SipHash uses its alignment-independent path; fixed-width helpers are retained,
while unused HalfSipHash and compiler constant-size dispatch are omitted.
Base64 tables retain the original values with sequential C11 initialization
instead of GNU designated ranges. Decoder bit assembly uses unsigned shifts
to retain rejection of negative lookup entries without signed-shift undefined
behavior on host compilers. The encode/decode control flow remains original.

Original HCTR2 setkey/hash/crypt/finish and request completion bodies now run
through a synchronous owned-request Crypto API boundary. Its POLYVAL/GHASH
dependency uses the original generic 32-bit carryless multiply fallback.
HCTR2 initialization selects AES and AES-XCTR directly instead of kernel
template/spawn registration. The original cleanup and request-size branches
are retained. GNU omitted-middle conditionals use an explicit error local;
kernel algorithm-name negotiation, async providers and module registration are
omitted. This dependency layer is still separate from the ext handler.
The host Crypto API retains the installed-key requirement: a request without a
usable key returns `ENOKEY`, including after a failed key installation. Request
bounds are checked before the original mode body accesses its memory segments.

Prepared AES providers now expose the original ECB/CBC/XTS/CTR/XCTR library
through that Crypto API, including the ECB primitive needed by the v1 KDF.
XTS uses a private evolving tweak to retain the kernel driver's unchanged-IV
contract. CTS uses the original `crypto/cts.c` request bodies with the CBC
provider: the library CTS API deliberately clobbers IV differently, so direct
substitution would lose the kernel API state contract. CTS alignment annotations
use C11 and its GNU omitted-middle conditionals use explicit error locals;
the selected setkey/crypt/completion bodies otherwise remain original.
Original ESSIV skcipher setkey/crypt/request and cleanup bodies are transferred
for the fscrypt `essiv(cbc(aes),sha256)` instantiation. The SHA-256 hash service
and fixed dependencies replace kernel allocation/registration; dm-crypt AEAD
instantiations and algorithm-name negotiation are outside this ext read scope.

SM4 now uses the original generic block callbacks and CBC request functions,
the original ECB block iterator, and the original generic XTS key/tweak/CTS
functions. A fixed-buffer bridge replaces lskcipher/kernel registration and
selects the existing raw SM4 callbacks. XTS initialization selects ECB-SM4 and
the raw SM4 tweak cipher; the original allocation failure cleanup is retained.
Its two GNU error chains use equivalent explicit short-circuit statements.
Aligned owned buffers replace the kernel walk mappings, copying only processed
bytes before the original XTS tail handling and wiping temporary storage.
Portable request-context alignment follows the generic `u64` members; no SIMD
kernel context is used. This avoids relying on `max_align_t`, absent from the
selected MSVC C11 headers.

Adiantum now has its original setkey, NH/Poly1305 hashing, two-pass crypt,
request completion and cleanup bodies, with the generic ChaCha/HChaCha, NH and
32-bit Poly1305 dependency closure. Original `crypto/chacha.c` request bodies
preserve the XChaCha subkey and nonce/stream-position mapping. Generic Poly1305
uses the default non-architecture schedule size; full MAC buffering APIs are
not needed by the Adiantum core. Kernel algorithm registration and architecture
acceleration are omitted. Initialization selects XChaCha12/AES for fscrypt;
the original XChaCha20 variant is also available through the same bodies.
Two mapped-memory pointers use byte types for C11 arithmetic and ChaCha's local
alignment annotation uses C11. Empty Poly1305 initializers use `{0}`.

Host scatterlists now retain page/offset views as well as flat data bounds.
Ordinary coherent memory replaces page mapping/cache flushes; the synthetic
4096-byte page boundary selects the original fast or streaming hash path.
This is a crypto mapping optimization, not the filesystem's policy/page geometry.
The original Adiantum hashing branches remain present. Sized zeroed allocation
and sensitive release replace kernel slab ownership for derived-key buffers.
The selected stream providers are synchronous, satisfying Adiantum's original
provider requirement; no asynchronous device completion service is introduced.

The focused crypto suite compares encryption and decryption with selected
original `crypto/testmgr.h` known answers, including in-place/unaligned buffers,
continuation and final IVs. SHA/HMAC cases include split updates and long HMAC
keys. Separate fscrypt HKDF answers use Python's standard HMAC-SHA512 with RFC
5869 expansion, covering context separation and multiple output blocks.
Additional cases cover SHA-256, SM4, SipHash reference vectors, independent
base64url answers and invalid input bytes, plus HCTR2's complete kernel vector
set with both aligned and partial final blocks. These checks validate the host
crypto boundary, not encrypted filesystem reads.
The Crypto API adapters also run the AES vectors, preserving the CBC/CTR final
IVs recorded by the kernel vectors and the XTS/XCTR unchanged IV contract.
Original ESSIV vectors exercise the complete SHA-256-to-IV-to-CBC composition.
The forbidden-weak-key request flag must reject identical XTS key halves for
both AES and SM4. SM4-CBC/CTS/XTS use the original kernel vectors, including
the 189-byte XTS ciphertext-stealing case and recorded CBC final IVs.
Adiantum and ChaCha variants use their complete kernel vector sets. The
4096-byte Adiantum case also uses a nonzero source/destination page offset,
checking the generic streaming path against the same independent known answer.
On 2026-10-20 the prepared AES Crypto API/ESSIV, generic SM4, HCTR2 and supporting
hash/KDF/name-codec checks passed on macOS, Linux and MSVC Windows. The Windows
context-alignment compile error was fixed and the expanded suite rerun after
the other worktree owner completed. The subsequent Adiantum/ChaCha expansion
also passed on all three platforms before archive integration.

The context/policy read subset is now transferred as well: original context
size/version/nonce helpers, policy fields and accessors, context conversion,
mode-pair/flag/data-unit/geometry validation, policy comparison and key specifier
selection. The selected ten policy function bodies match Linux except for the
inline-crypto page-size comparison, which uses explicit archive geometry rather
than the host OS constant. Callers must supply filesystem maximum file size,
stable-inode capabilities and the relevant page geometry; current file length
cannot substitute for those limits. The handler now supplies these services
through the original `ext4_max_size` calculation and ext4 crypto capabilities.
The mode table from `keysetup.c` uses sequential C11 initialization for MSVC,
retaining its original mode indices. The borrowed blk-crypto
enum remains under its original header path and copyright notice; no inline
hardware service is provided. UAPI policy/key-specifier declarations exclude
unneeded ioctl/write interfaces. C11 assertion statements and the Windows SAL
name boundary preserve the on-disk structure layout.
Focused context/policy checks now also pass on all three platforms: truncated,
oversized and unknown contexts; invalid mode/flag/reserved fields; casefold/v1
restrictions; and unsafe data-unit/inode-IV combinations under supplied geometry.
These are parser/policy acceptance checks, not encrypted archive acceptance.

The read-side key preparation and name/data readers are now transferred:
`keysetup.c` mode selection, software cipher preparation/destruction, master-key
size checks and all v2 derivation branches; `keysetup_v1.c` nonce-keyed AES KDF
and direct-key selection; and the raw-key/HKDF identifier preparation from
`keyring.c`. The original SipHash inode-IV and directory-hash key derivations
are retained. An immutable archive lookup callback replaces kernel keyrings;
each prepared inode owns its transform. Global direct-key/per-mode caches,
kernel key claims, lazy shared inode-hash initialization, VFS key links, RCU and
revocation are omitted. The inode-hash key is prepared before publication;
mode/UUID/nonce/HKDF inputs and IV generation remain unchanged. Kernel inline
encryption and hardware unwrapping are not host services; no wrapped raw-key
input is accepted. The original unsupported-key/policy error distinctions remain.

`crypto.c` retains the complete IV generation and single-data-unit cipher body.
`fname.c` retains filename decryption, buffer ownership, dot-name handling and
the complete Linux no-key name representation, including strong-hash shortening.
`hooks.c` retains symlink header/bounds/empty-target checks and its name-reader
call. Symlink caching is omitted: every result has an explicit owner instead
of a shared VFS `i_link`. Heap cipher requests replace kernel stack-request
macros and add allocation-error handling; C byte/character pointer conversions
and an unaligned symlink length read form narrow host boundaries.

The opaque C boundary in `fscryptreader` snapshots inode metadata and filesystem
geometry during construction, prepares keys, then releases all borrowed registry
inputs. Its inode can outlive the original raw/master key and context buffers.
It restricts content decryption to regular files and name/symlink readers to
their appropriate inode types; allocated units must be complete and within
the filesystem data-unit limit. The archive handler retains these owned objects
in its mapped streams, independently of archive Close/reopen/Release.

The synthetic native fixture generator creates an ext4 image using Linux
ioctls and e2fsprogs, then obtains ciphertext extents and private contexts with
FIEMAP/debugfs. Linux reads back the known plaintext before unmounting, and a
read-only remount without keys supplies independent opaque-name/symlink answers.
The image hash remains unchanged during that read-only inspection. Kernel
`6.8.0-147-generic` accepted all thirteen selected cases: v1/v2 AES, ESSIV and
Adiantum; v1/v2 direct-key; inode-IV 64/32; HCTR2; SM4; and 512-byte data units.
The standalone C checks compare complete decrypted-unit hashes and native name/
symlink bytes, including a sparse gap, padded tail, unaligned/in-place buffers,
missing keys, malformed metadata, bounded output and released constructor inputs.
On 2026-10-20 these native-byte composition checks passed on macOS, Linux and
MSVC Windows. This establishes the prepared C reader, not archive extraction.
Directory hashes are actual inputs: native htree readdir supplies them even for
a single-block directory; the ext4 archive caller must preserve that behavior.

Required archive boundaries identified from current callers include binary
on-disk directory names before presentation, context values from ext4's private
encryption xattr, and full allocated last-block reads before logical EOF clipping.
Sparse holes must remain zero rather than being decrypted as ciphertext.
For encrypted fs-verity files, decryption must precede hash verification for
both data and verifier-only metadata reads. Inode/UUID and data-unit limits
must come from ext4 geometry, not from the archive's currently requested byte
range or the host OS page size.

The archive key boundary uses the existing `ICryptoGetTextPassword` callback
with an explicit fscrypt envelope. The [user contract](../ext-encryption.md)
defines v1 descriptors, v2 keys, multiple keys and the no-key view. Empty callback
text also selects the no-key view. Callback cancellation/errors propagate; raw
key/text buffers are released through wiping owners. Duplicate v1 descriptors
are rejected rather than choosing between conflicting master keys. No SDK
fscrypt key parser exists to transfer; this syntax/ownership bridge is new
integration code, and delegates hex conversion to the original 7-Zip parser.
Login/protector passwords are not interpreted as master keys. Hardware wrapping,
kernel keyring permissions/RCU and kernel I/O acceleration remain platform
services. No real user keys or archives are used by the native fixtures.

`ext4/crypto.c::ext4_get_context` reads the private encryption xattr through the
existing original xattr bodies. `super.c::ext4_max_size` supplies the original
maximum DUN geometry (the VFS block-count typedef assertion is omitted; archive
block counts are explicitly 64-bit). `hash.c` retains all original hash bodies;
its Unicode lookup-folding configuration is disabled because readdir hashes
stored ciphertext or uses hashes stored in casefold dirents. A narrow bridge
supplies byte/rotation services and the original fscrypt SipHash name reader.
Default/index hash versions, filesystem seed and signedness follow Linux;
casefolded encrypted records use the stored aligned hash pair. Prepared casefold
SipHash keys are also checked against native dirent answers. Original
`fscrypt_get_policy`, `fscrypt_has_permitted_context` and `fscrypt_require_key`
enforce key and parent/child policy requirements. Nonces do not have to match
between parent and child; policies do. Per-path access checks survive hardlink
aliases, and derived streams retain their own prepared inode.

The original ExtHandler mapping and hole branches remain authoritative. Only
initialized allocated regular-file bytes go through an owned data-unit cache;
the complete allocated unit is acquired before EOF clipping. Directory blocks
remain structural metadata, and binary names are decrypted/encoded before string
and path checks. Symlink payloads go through the transferred hooks reader and
their presented size drives properties, copy accounting and Extract acceptance.
Verity wraps the already-decrypting mapped stream, so both data and hidden
metadata are decrypted before verification. The direct-device feature policy
now admits ENCRYPT through this same reader, retaining journal-consent rules.

The expanded native image includes encrypted casefold and fscrypt+verity files,
bringing the selected policies/scenarios to fifteen; Linux accepted both new
cases and verified the known file content. Native inputs use 16/32-byte minimum
keys where permitted and retain 64-byte v1/v2 cases. Dynamic SDK and CLI checks
read all fifteen exact payloads and symlinks, and compare no-key names. They also
cover retained streams, failed/cancelled opens, invalid raw-key input, incomplete
input reads and retry without publishing a partial unit. A foreign file policy
with an available master key is refused before any output; malformed directory
policy edges are rejected before traversing the tree. Backend checks list
and extract all native files, reopen without keys, and preserve an existing
destination on failure. Aligned synthetic-device streams read each native tail
and preserve authorization/cancellation and journal-recovery boundaries.
On 2026-10-21 all eight focused crypto/archive/CLI/backend/device suites passed
on macOS, Linux and MSVC Windows using the current incremental builds and
prepared runtime. The Windows backend comparison uses native path separators;
this was a test expectation correction, not a changed path-publication contract.
A private macOS CMake installation also reads all native plaintext hashes and
no-key names through its installed CLI with only system paths, preserves the
image and carries the ext GPL notice. Its module adds no external crypto runtime
dependency. This is scoped installed-CLI evidence; real encrypted media,
hardware keys, native UI interaction and release-package acceptance are unverified.

This iteration does not close the ext row. Remaining feature/creator-OS read
semantics still need their complete source comparison and integration; they are
required follow-up work, not accepted permanent limitations. The subsequent
verity trust integration is described below. Kernel VFS/page cache, writes and
journal replay remain outside the read-only archive contract.

### Builtin signatures and explicit archive trust

The same pinned Linux revision supplies the enabled builtin-signature path in
`fs/verity/signature.c`, its `open.c` consumer, the selected verification entry
points in `certs/system_keyring.c`, and the complete PKCS#7 parser, verification
and trust traversal. X.509 parsing, signature preparation and self-signature
checks retain their original control flow. `public_key.c` supplies software
algorithm/encoding/hash selection, parameter packing and fresh-transform
verification; `signature.c` supplies subtype dispatch and cleanup. Original
asymmetric identifier generation, exact/partial/name matching, lookup and the
required second-ID check are retained. The baseline manifest pins every borrowed
source/header/schema; each source retains its notices and original C boundary.

The original enabled policy checks every present signature, including when
signatures are optional. An empty archive trust set rejects signed files before
parsing PKCS#7. Requiring signatures rejects unsigned verity files. The original
fixed-endian formatted digest binds the signature to fs-verity, its digest
algorithm and file digest. Descriptor, signature and Merkle verification must
all succeed before the file stream becomes available. File-content verification
and the standalone `7zz` refusal described above remain in force.

Dependencies are transferred at their original paths:

- `lib/asn1_decoder.c`, ASN.1 headers and the C host compiler
  `scripts/asn1_compiler.c`; unchanged PKCS#7, X.509, AKID, RSA and ECC grammars.
  Generated machine/callback tables match the original compiler. Host changes
  cover CRT binary I/O, compiler annotations/GNU expressions, byte offsets and
  equivalent sequential C11 table initialization.
- `lib/oid_registry.c`, original OID definitions and `lib/build_OID_registry`.
  The checked C11 generator retains every original index, encoded byte and
  search entry. Ordinary builds need no Perl/bc. Scalar bitfields, byte-pointer
  arithmetic and LLP64 numeric formatting are the narrow adaptations.
- Original generic SHA-1/SHA-2/SHA-3/SHAKE, SM3 and Streebog routines and selected
  synchronous `shash.c` buffering/default callbacks. Fixed owned provider
  metadata replaces global Crypto API registration. Calendar conversion reuses
  the existing authoritative `kernel/time/time.c`; original string/hex helpers
  supply identifier conversion and digest lookup.
- RSA, RSA ASN.1 helpers, PKCS#1 v1.5, synchronous akcipher requests and all generic
  MPI arithmetic units. MPI follows native `unsigned long` limbs, including
  32-bit limbs on LLP64 Windows. Original generic mixed-word arithmetic is used
  without architecture assembly. Compiler type modes, inline linkage, zero
  initialization and three GNU conditionals use equivalent C11 forms.
- ECC arithmetic, full/partial point validation, all four Linux NIST curves,
  both ECDSA encodings, and all five implemented GOST curves. The two GOST
  parameter sets unsupported by Linux retain rejection. Compiler carry/borrow
  services select the original generic path with portable unsigned overflow
  operations; half-word multiplication and modular reductions are unchanged.
- The generic ML-DSA verifier and Crypto API wrappers for all three parameter
  sets. Only includes, parameter-table/zero initialization, pointer-width inline
  selection and actual explicit workspace cleanup replace compiler services.
  Original `lib/string.c::memchr_inv` replaces the old independent fscrypt loop.

Original `lib/scatterlist.c` page/mapping iterator bodies replace the earlier
repeating-span adapter, retaining partial consumption, resume and termination.
Actual mappings address a bounded owned memory span; kernel chained/DMA pages
are not provided. Original generic bit scans supply MPI and replace the fscrypt
bit-count loop. Crypto API contexts and instances remain at their original
internal header boundaries. Constructors replace kernel discovery/registration;
original algorithm bodies and key-state behavior are retained. X.509 cleanup
attributes become real cleanup on all exits, and its GNU time expressions use
C11 statements. PKCS#1 requests are explicitly released on all allocated exits.
The zero-length `none` prefix uses a C11 sentinel with encoded length zero.

The archive-specific storage adapter owns copied certificate DER, parsed keys
and identifiers in an immutable trust object. Lookup uses the original matcher;
each synchronous result owns a separate handle borrowing immutable payload while
its caller retains the ring. Concurrent checks do not mutate shared reference
counts or global key state. PKCS#7 parsing/verification state and crypto transforms
are fresh for each check. Trust is established by the original traversal and
cryptographic verification of the trusted variant, never by names/IDs alone.

Kernel key quotas/credentials, global registration, builtin/secondary/platform
trust stores, system revocation lists and global inode caches are outside this
userspace archive boundary. The selected original
`CONFIG_SYSTEM_BLACKLIST_KEYRING=n`, `CONFIG_SYSTEM_REVOCATION_LIST=n` and
`CONFIG_SECURITY=n` services do not consult host stores or notify kernel LSMs.
Missing/special global-store requests are rejected; only the explicit owned ring
is available. No system trust store or kernel keyring is modified. No FIPS claim
is made. ECC private-key generation/validation, public-key generation and ECDH
random services are omitted from this read-only boundary. RSA private operations
remain source-transferred but are not runtime-accepted as an archive feature.

The GPL handler implements the original SDK `ISetProperties` transport, following
`NtfsHandler::SetProperties` conversion/ignored transport-property pattern from
7-Zip `9128b80e3a471108678e2b3ec3c985861c8d7b0f`. It reuses the existing authoritative
`MethodProps.cpp` boolean conversion; its mixed baseline remains recorded in the
[7-Zip manifest](../upstream-baseline.json). The local policy owner has no usable
upstream userspace counterpart. Property configuration is atomic on failure;
streams retain their policy and inputs after handler Close/reopen/Release.
Native file, device and selected-partition opening pass original `COpenOptions`
properties to the same handler. Widgets gain no certificate-editor workflow.

The public properties are `verity.require` (SDK boolean or `on`/`off` string) and
repeatable `verity.trust` (ASCII hexadecimal DER certificate). Trust inputs are
copied; Linux's original hex decoder handles the text boundary. For the dynamic
CLI, use `-mverity.require+ -mverity.trust=<certificate-DER-as-hex>`; omit the
require switch or use `-mverity.require=off` for optional signatures. Optional
signatures still require valid explicit trust whenever present. These options do
not load OS trust roots or interpret protector passwords.

Behavior evidence covers 228 independent Linux AF_ALG hash answers with block,
rate, padding and unaligned/split boundaries; published Linux ML-DSA, RSA and ECC
answers; and corrupt keys/messages/encodings and failed key replacement. The RSA
fixture generator independently reproduces the published CAVS answer and checks
its positive results with OpenSSL. Its public outputs cover all six Linux
non-FIPS modulus sizes and SHA-1/SHA-2/SHA-3 prefixes. No private test keys are
emitted. Published X.509/PKCS#7 fixtures exercise actual self/internal signature
verification, rather than accidentally deferring checks on a cert-less message.

Deterministic chain fixtures use published Linux test keys, independent Python
integer signing and OpenSSL verification. Tests cover embedded/external roots,
trusted leaves/intermediates, missing chains, altered content, empty/foreign
trust, second-ID mismatch and a valid certificate with identical identifiers but
a different key. The last two cases must not acquire trust. Factory failure
cannot publish a partially configured ring; verification survives input wipe.

OpenSSL also verifies the SHA-256/SHA-512 formatted-digest signatures in the
signed ext fixtures, including authenticated attributes. SDK and dynamic CLI
checks read exact content hashes, reject missing/foreign trust and signature
corruption, and enforce unsigned/required policy. SDK checks cover retained
streams, failed property replacement and cancellation/retry at signature reads;
aligned device streams receive the same opening properties. Input images stay
unchanged. Actual kernel signature acceptance with installed trust roots has not
been run: the host kernel's global trust state is deliberately untouched. Native
unsigned Merkle/fscrypt evidence above and the original Linux signature vectors
remain separate from these archive trust checks.

The thirteen focused crypto/ext/stream/backend/device suites passed on macOS,
Linux and MSVC Windows on 2026-10-21 with incremental builds and existing caches.
The additional authenticated-attribute image passed localized SDK/CLI checks on
all three platforms. The application was rebuilt, and a private macOS CMake
installation read all four image cases with a system-only PATH, rejected missing
or foreign trust, enforced required signatures and left input images unchanged.
The installed module has no external crypto runtime dependency; its payload
retains the LGPL MPI, BSD ECC and IETF ASN.1 license texts and specific notices.
Installed Linux/Windows packages, full/slow suites, native UI, release packages
and physical devices were not checked in this iteration. Broader ext
feature/creator-OS read semantics remain in the parent scope.

### Extra inode initialization and timestamps

The same Linux pin supplies the complete `fs/ext4/inode.c`
`ext4_iget_extra_inode` body and the extra-inode initialization branch of
`__ext4_iget`, with their callers, checksum ordering, timestamp macros and
`fs/ext2/inode.c` read path reviewed. The helper changes only linkage and GNU
void-pointer arithmetic for the C11 boundary. The original inode-state enum is
retained; archive-owned state bits replace kernel atomic bit operations while
the inode is unpublished. Xattr, inline-data and Lustre state remain distinct.
Other `__ext4_iget` validation branches are outside this transfer.

An on-disk zero extra size now receives Linux's standard in-memory extension
after raw checksum verification, without discovering inode xattrs in that
branch. Both engines expose each timestamp field according to its available
extension size, including partial extensions and valid epoch-zero timestamps;
absent birth time remains absent. This fixes lost epoch/nanosecond and birth-time
information in the existing SDK parser as well as the dynamic module.

Native Linux 6.8.0-147-generic read-only loop mounts confirmed both generated
partial-extension and zero-extension fixtures on 2026-10-21. In the latter,
atime/mtime reach 2106 and birth time reaches 2242 with nanoseconds preserved.
Payload hashes and input-image hashes matched; temporary mounts were removed.
This runtime kernel is separate from the pinned source reference. Seven focused
ext/stream/fscrypt/xattr/backend/device suites passed on macOS, Linux and MSVC
Windows, including rebuilt SDK and standalone runtimes and their public CLI
timestamp comparison. Runner receipt: `20261020T231817Z-c3cfe8ab`; native mount
receipt: `20261020T231747Z-e420d403`. Full/slow suites, installed packages and
physical devices were not checked for this iteration. Remaining ext read
validation and the parent format census remain open.

### System zones and block references

At the same Linux pin, `fs/ext4/block_validity.c` supplies complete zone insertion,
merging, reserved-inode protection, setup/release, range lookup and block-reference
checks. `balloc.c` supplies the complete descriptor/backup metadata geometry
helpers; `super.c` supplies descriptor-address accessors and the complete
`ext4_check_descriptors` body. Their original mount, inode, extent, indirect and
xattr callers were compared before integration. Descriptor checks retain their
read-only branches; existing archive checksum validation remains in force.

The dynamic module now builds the original tree before directory/content reads,
including journal mappings and their owner identity. Direct and indirect block
references, extent data/index ranges, unwritten extents and external xattrs use
the original validity checks. Journal blocks remain readable by their owning
inode. Original sparse-super/sparse-super2, reserved GDT, meta_bg and flex_bg
geometry determines the protected metadata ranges. This closes successful
extraction of metadata bytes through corrupt file pointers. Standalone `7zz`
retains its SDK-only checks; the GPL reader is a separately deployed module.

The complete `lib/rbtree.c` and its three Linux headers are pinned dependencies.
Host adaptations replace pointer-sized `unsigned long` storage/casts with
`uintptr_t` for MSVC, retain natural pointer alignment, and replace GNU statement
expressions used at the boundary with C11 equivalents. Kernel slab initialization,
shutdown and optional debug printing are omitted. Archive-owned allocation and
postorder destruction replace slab services; an unpublished immutable tree uses
immediate RCU publication/destruction without kernel locks or grace periods.
The journal mapping adapter reuses the SDK's existing extent/indirect maps and
preserves source/callback errors. It does not read or replay journal transactions.
No usable kernel userspace stream/ownership implementation exists for that
boundary. New original units and their full source hashes are in the existing
baseline manifest; notices remain in the copied sources.

CLI regressions cover 48 corrupt mapping/metadata combinations, overlapping
descriptors and journal-owner reads. SDK checks cover cancellation, empty state
after partial-tree or file-reference failure and reuse.
Existing creator-generated ext images exercise journal, inline, bigalloc and
distributed-descriptor consumers. Native Linux 6.8.0-147-generic read-only loop
mounts separately confirmed a valid control read and rejection of direct,
indirect, extent and unwritten-extent aliases of the inode table. Input images
were unchanged and temporary mounts removed (receipt
`20261020T232844Z-71fd0a16`). Native runtime evidence is distinct from the pinned
source comparison. Other inode acceptance/read-feature branches remain in the
parent audit; this transfer does not complete the overall ext review.

Seven focused ext/stream/fscrypt/xattr/backend/device suites passed on macOS,
Linux and MSVC Windows on 2026-10-21 (remote receipt
`20261020T232857Z-5068045a`). The MSVC run includes the C11 replacement for a GNU
rbtree statement macro found during the first Windows build. Full/slow suites,
installed packages, native UI and physical devices were not checked.
The subsequent overlapping-descriptor cleanup regression passed the two focused
SDK/CLI suites on all three platforms (remote receipt
`20261020T233013Z-ca8056ef`).

### Original extent validation and symlink inode semantics

The same Linux pin supplies complete `extents.c` root/block capacity helpers,
`ext4_valid_extent`, `ext4_valid_extent_idx`, `ext4_valid_extent_entries`,
`__ext4_ext_check` and `ext4_ext_check_inode`, together with their original
`ext4_extents.h` address/depth accessors. Complete original bodies and callers in
inode loading, tree lookup and block mapping were compared. The generic header
ceiling of 32 does not replace the lookup ceiling of five; empty non-root leaves
remain rejected by the mapping path, as in `ext4_ext_map_blocks`.

The dynamic reader validates inode extent roots during inode loading, repeats
their block checks after system-zone publication, and uses the same original
validator for child nodes and checksums. Empty file contents cannot bypass a
corrupt root. SDK traversal, parent-range guards and stream lifetime remain in
their existing owner. The C boundary bounds each supplied buffer before invoking
the original validator; exported linkage and owned inode state replace VFS
allocation. These helpers add no write, cache or journal-replay services.

Complete original `inode.c` `ext4_inode_is_fast_symlink` and `ext4_inode_blocks`
bodies now supply cluster-aware EA accounting and huge-file block conversion.
The latter takes the already-owned inode explicitly instead of embedding a VFS
inode in its private state, and spells kernel `blkcnt_t` as `u64` to avoid a
collision with the host POSIX typedef. Selected `__ext4_iget` symlink branches
reject append/immutable flags and invalid fast-target length/termination;
orphan recovery and VFS cached-link publication are outside the archive boundary.
Other inode acceptance branches remain outside this selected transfer.

Both engine builds receive the narrow SDK field/accounting corrections for
BIGALLOC, huge-file and EA_INODE symlinks, including a fast target carrying an
EXTENTS flag. Standalone retains SDK code and does not link the GPL helpers.
Production CLI tests cover all these positive cases, slow-link metadata aliases,
invalid symlink flags/targets and corrupt empty-file roots. Existing checksum,
fscrypt/verity, stream, backend and device consumers remain covered.

Seven focused suites passed on macOS, Linux and MSVC Windows on 2026-10-21;
remote receipt: `20261020T233701Z-038b41e8`. The final unsigned-width spelling
also passed localized macOS SDK/CLI checks. Native read-only Linux 6.8.0-147
mounts confirmed all four symlink geometry cases, rejected append/immutable flags,
a corrupt empty-file root and an empty non-root extent leaf. That older runtime
accepted malformed fast-target termination and zero length; the pinned reference
contains the stricter checks used by this port. Its acceptance is not attributed
to the newer source pin. Native receipt: `20261020T233904Z-031edc21`; images were
unchanged and mounts/temporary fixtures removed. Full/slow suites, installed
packages, UI and physical devices were not checked. The parent ext review and
overall format goal remain open.

### Inode acceptance, sparse sizes and special-file boundaries

At the same Linux pin, the complete `ext4_inode_info_init`,
`ext4_max_bitmap_size`, `ext4_isize`, `ext4_get_maxbytes`, `ext4_valid_inum` and
`is_special_ino` bodies supply original inode geometry, sparse-size and identity
rules. The initialized first-inode value preserves revision-zero semantics;
extra-size requirements are checked by the original initializer. The read-side
`__ext4_iget` retains its original illegal
inode, unlinked inode, size, casefold and `check_igot_inode` branches. Its archive
signature validates an already-owned decoded inode and returns an error code;
kernel pointer-error returns are converted at that boundary. Known-mode VFS
operation binding becomes mode acceptance without installing operation tables
or creating device nodes. VFS allocation/cache lookup, namespace services and
orphan recovery remain omitted. A per-copy bad-inode state retains the original
boot-loader/check_igot handling without mutating inventory objects.

The owned superblock initializes both original maximum-size calculations. Raw
inode inventory can still include inactive reserved slots written by mkfs;
normal namespace, special-inode and EA-inode consumers select their respective
Linux validation contracts. Invalid linked targets cannot become public items,
and unlinked inventory slots cannot become recovered `[UNKNOWN]` file contents.
The internal EA stream reader selects the EA-inode contract, preserving valid
large xattr values. Journal protection selects the special-inode contract.
All added helpers/branches and their callers were compared with pinned source;
the existing baseline already owns their complete original units.

Both SDK engines now decode the high size field when LARGEDIR requires it.
Device/FIFO/socket inode encoding is metadata, not an indirect data-block map:
empty special files produce empty streams; nonempty special-file stream requests
fail explicitly without opening any native device. Standalone retains its SDK
implementation and does not gain the GPL inode validator. No claim of native
device/service access or newly complete large-directory runtime coverage follows
from the metadata size correction.

Behavior checks exercise sparse-size boundaries and overflow, unknown/zero
modes, unlinked targets, casefold without its feature, reserved/quota aliases,
unlinked inventory and special-inode payload requests. Existing EA/xattr,
fscrypt/verity, stream, backend and device consumers remain part of the focused
selection. Native read-only Linux 6.8.0-147 stat/listing confirmed all fourteen
fixture cases, including exact bitmap/extent boundary sizes and metadata-only
device identities (receipt `20261020T234719Z-79830de5`). No device was opened;
images were unchanged and temporary mounts/fixtures removed. Native runtime
evidence remains distinct from the pinned source comparison. The broader ext
read-feature review and parent format goal remain open.

The final initializer/legacy and stale-unlinked-slot variants also passed native
read-only checks (`20261020T235334Z-b10c414b`), including valid legacy content and
rejected min/want extra-size requirements. Unreadable inventory slots do not
enter post-publication block validation as active files. Seven focused suites
passed on macOS, Linux and MSVC Windows on 2026-10-21 with existing incremental
trees/caches (remote receipt `20261020T235121Z-2f567c0d`). Both engine runtimes
were prepared for the shared SDK changes. Full/slow suites, installed packages,
native UI and physical devices were not checked in this iteration.

### Sparse directories, record validation and declared encoding

The pinned `fs/ext4/dir.c` `ext4_readdir` mapping, checksum and emission paths,
the complete `__ext4_check_dir_entry`/`is_fake_dir_entry` bodies, their inline
callers and the SDK stream/ParseDir consumers were compared. Original Linux
readdir skips missing mappings before checksum/record interpretation. The SDK
mapper now passes allocated ranges with the directory result; holes and
unwritten ranges are skipped by both engines, including the dynamic module's
checksum pass. No second physical mapper or content-based zero-block heuristic
is introduced. Allocation bookkeeping reuses validated extent/indirect maps and
does not allocate one flag per logical block. Written zero-filled blocks still
fail metadata checksums. The original C record validator is now connected to
external directory records as well as its existing inline consumer; the owned
buffer boundary checks available bytes before its original accesses.

The syscall emission callers were also compared: complete original
`fs/readdir.c` `verify_dirent_name` now validates presented names after optional
fscrypt conversion. Its linkage is exported inside the GPL module and the
kernel PATH_MAX definition is retained independently of the host limit; the
helper and UAPI limit header have full pinned hashes. Raw ciphertext is not
checked as a plaintext path component. Standalone's existing SDK parser gains
the equivalent narrow empty/slash guard without linking GPL code. Tests reject
invalid names in ordinary and inline directories; existing embedded-NUL length
checks remain unchanged. User-copy/syscall, VFS lookup and signal services are
not ported as archive operations.

The complete original `super.c` encoding table and `ext4_sb_read_encoding`
helper, with scalar `include/linux/unicode.h` age macros, now supply the declared
casefold encoding gate. `ext4_encoding_init` retains that selected acceptance
branch and its feature-off bypass. Archive readdir preserves stored names;
Unicode VFS map allocation, module loading/unloading and case-insensitive lookup
are omitted instead of being represented by fake Unicode operations. Unknown
encoding values cannot be treated as supported casefold volumes. Encoding fields
without the casefold feature and unused flag bits retain Linux's acceptance.
The Unicode header's full baseline hash is recorded; no external Unicode or
crypto runtime dependency is added. Standalone does not link the GPL gate.

CLI regressions read aliases after holes through classic block mapping, extents
and unwritten ranges; reject written-zero/corrupt checksummed blocks; and verify
encoding acceptance/refusal with exact original names/content. Seven focused
ext/stream/fscrypt/xattr/backend/device suites passed on macOS, Linux and MSVC
Windows on 2026-10-21 (remote receipt `20261021T000259Z-d1fe5c66`). Both engine
runtimes were rebuilt incrementally for the shared SDK changes.

Native Linux 6.8.0-147 read-only mounts confirmed ten directory/encoding cases
(`20261021T000545Z-450ba46e`): readdir names, content hashes, known/unknown encoding
and feature-off handling. Native content was read through the first hard link to
the same inode. The older kernel's VFS lookup after an unwritten directory range
returned EBADMSG, while its readdir emitted both names; ordinary holes allowed
lookup. The archive reconstructs paths from validated readdir records and does
not reproduce VFS lookup services. These observations are not a claim of native
post-unwritten-range lookup or identical corruption-recovery policy. Images
were unchanged and temporary mounts/fixtures removed.

Full/slow suites, installed packages, native UI and physical devices were not
checked. At that iteration SDK directory materialization still imposed a memory
boundary; the subsequent streaming LARGEDIR work below addresses it. The parent
read-feature scope remains open.

The final emission-name integration passed the same seven focused suites on all
three platforms (`20261021T000926Z-5ce7849f`; macOS 5.68s). Native read-only
ordinary/inline controls and empty/slash-name failures also passed
(`20261021T001123Z-eb21c386`), with matching content and unchanged images. This
separately validates the VFS emission contract copied into the GPL module.

### Streaming LARGEDIR directory payloads

The complete original SDK `ParseDir`, `ExtractNode`, Open/cleanup callers and
stream types were compared with pinned Linux `ext4_readdir` block traversal,
checksum/emission and seek behavior. The SDK had no usable directory streaming
orchestration: Open materialized the complete logical directory before parsing.
The narrow `ReadDir` adapter now uses its existing seekable stream and validated
allocated ranges to read one filesystem block at a time, jumping over holes.
Physical mapping and record interpretation remain in their existing original
owners; no second filesystem decoder, VFS page cache or syscall layer is added.

The original record loop retains its branches and updates. Its self/parent-link
state, directory-item vector and root hash version now live across block calls;
final dot/parent validation runs after the complete directory. Inline directories
reuse their existing bounded converted payload. Checksums and original Linux
record/name validators precede publication of a successful archive. A late read,
checksum or callback error leaves no items visible through failed Open and allows
reuse. Progress/cancellation is checked between directory blocks. Both engines
receive the shared SDK adaptation; GPL validation remains separately linked.
Kernel inode-version/readahead services are outside immutable archive reading.

The permanent Linux CLI regression opens a sparse LARGEDIR whose tail entry is
past 4 GiB while each child process has a 512 MiB address-space cap. Both engines
list that entry and extract its exact content; this detects the former whole-size
allocation without allowing the failing case to exhaust host memory. macOS and
Windows skip that platform-specific resource-limit test. SDK regressions on all
platforms cover a late block returning E_FAIL/E_ABORT, first-block cancellation,
late checksum failure, empty failed-Open state and successful reuse. Existing
indexed/inline, encrypted/no-key, fscrypt+verity, xattr/backend/device consumers
remain in the seven focused suites, which passed on macOS, Linux and MSVC Windows
on 2026-10-21 (remote receipt `20261021T002228Z-5454c8c3`).

Separate current-module CLI smoke checks read the same post-4-GiB entry through
both engines on macOS and Windows as well as capped Linux
(`20261021T002544Z-2c1b42ac` for the runners). Native read-only Linux
6.8.0-147 confirmed the directory's 64-bit size and a linear readdir cookie after
seeking to the high-offset tail, with the same inode/content as its first hard
link. That is scoped native stat/seek/read evidence, not a dense 4-GiB directory
walk or htree hash-cookie test. Images were unchanged; temporary CLI copies,
mounts and fixtures were removed. Builds reused existing incremental trees and
caches. Full/slow suites, installed packages, native UI, physical media and very
large dense directory populations were not checked. The parent format review
remains active.

The same review found a checksum-dispatch defect: a non-root indexed leaf whose
first short-name record occupied 12 bytes was mistaken for an htree root. The
archive checksum boundary now receives the logical block number and mirrors
`__ext4_read_dirblock`/`is_dx` classification: both DIR_INDEX and the inode index
flag are required, and only logical block zero identifies the root. For a
non-root full-block record, Linux explicitly warns that an empty leaf can look
like an index node. Linear archive traversal lacks the VFS caller's INDEX hint;
the adapter accepts that ambiguous block only if either original leaf or index
checksum validator succeeds. It does not import the mutable VFS htree traversal
or skip checksum verification. This is an archive-boundary adaptation, not a
claim that the complete `__ext4_read_dirblock` body is transferred.

Permanent CLI regressions extract both a one-character hard link and its Unicode
alias, list them across a second empty htree leaf, and reject damaged checksums
in either leaf. The final seven focused suites passed on macOS (5.65s), Linux
and MSVC Windows (`20261021T003229Z-3f6e923e`), including the earlier streaming
cases. Native read-only Linux 6.8.0-147 mounted both short-name controls, with and
without the empty leaf, enumerated the same names and read identical content
(`20261021T003306Z-f1b164d4`). Images were unchanged and temporary mounts/fixtures
were removed. The native control establishes valid-image compatibility; the
archive regression separately establishes strict rejection of corrupt checksums.

### Superblock geometry and input addressability

The complete pinned `ext4_check_geometry` body in `fs/ext4/super.c`, its
`ext4_block_group_meta_init`/`__ext4_fill_super` callers, descriptor/inode/cluster
types and `generic_check_addressable` dependency in `fs/libfs.c` were compared
with SDK header parsing and Open. SDK rounded block-group and inode-group counts
could agree without an exact inode inventory; it also obtained input size
without applying Linux's complete volume geometry contract before reading GDT.
No usable SDK equivalent of the complete selected check exists.

Both original C bodies are now compiled into the isolated GPL ext module;
`ext4_check_geometry` is exported privately without changing its branches. The
archive owns the initialized cluster ratio/inodes-per-group and supplies its
actual input byte length for the block-device-size service. The original check
runs before GDT allocation/reads and validates reserved GDT count, addressability,
volume/device span, first-data-block geometry, group ceiling and exact total
inode count. Existing checksum/descriptor/system-zone consumers remain in place.

The `fs/libfs.c` dependency is recorded in the source baseline with its complete
unit hash and original GPL notice. Its original function body remains unchanged.
The narrow overflow service performs a checked unsigned 64-bit shift; archive
sector/page indices are explicitly 64-bit on all hosts, with the existing 64 KiB
archive page geometry. No native page cache or block-device access is created.
Kernel logging/export services are replaced at the existing private boundary.
The SDK's earlier signed-stream-size and basic parsing guards remain. Standalone
does not acquire the GPL geometry implementation.

Public CLI checks cover a correct image with trailing non-filesystem bytes,
one-byte physical truncation, excessive reserved GDT blocks, an invalid 1 KiB
first-data block and an inconsistent exact inode count in a real multi-group
mkfs META_BG image. Existing coverage retains valid classic/legacy, BIGALLOC,
distributed descriptors, sparse/LARGEDIR, inline, xattr, crypto and backend
consumers. Source comparison confirms both transferred bodies match the pinned
original after private linkage/include adaptations; runtime acceptance remains
separate from this source evidence.

On 2026-10-21 the seven focused ext/stream/fscrypt/xattr/backend/device suites
passed on macOS, Linux and MSVC Windows (`20261021T003632Z-d048776b`). Native
read-only Linux accepted the padded control with exact content and refused all
four invalid geometry cases (`20261021T003708Z-cba469e1`). Images remained
unchanged; all temporary mounts/images were removed. The arithmetic division
service also retains Linux's remainder-return contract; its caller uses the
updated quotient. Builds reused incremental trees and compiler caches. Full/slow
suites, installed packages, UI and physical devices were not checked; the parent
format/read-feature review remains active. After the division-service refinement,
all seven local suites passed again (7.66s), and focused `ext`/`ext_stream`
reruns passed on both runners (`20261021T003822Z-0853a693`). That final source
synchronization also removed the disposable native helper from runner current.

### Allocation bitmap structure and group initialization

The pinned `ext4_read_inode_bitmap`, `ext4_read_block_bitmap_nowait` and
`ext4_validate_block_bitmap` callers were compared with SDK group enumeration,
the existing checksum boundary and system-zone setup. Checksums alone did not
connect Linux's structural block-bitmap checks. SDK enumeration also skipped
reading classic block bitmaps altogether and accepted root-group UNINIT flags.

The complete original `num_clusters_in_group`, `ext4_valid_block_bitmap` and
`ext4_valid_block_bitmap_padding` bodies are now transferred into their original
`fs/ext4/balloc.c` unit (only private export changes). They check allocation of
the block/inode bitmaps and the whole inode table, cluster-relative bounds and
the entire padding through the end of the bitmap block. FLEX_BG retains the
original metadata-location exemption while padding remains checked. The archive
initializes the original cluster-bit and inode-table geometry before consuming
these helpers. Bitmap block addresses use the original read caller's bounds.
The group number is passed explicitly rather than inferred from bitmap bytes.

The selected generic `_find_next_zero_bit` in `lib/find_bit.c` supplies the scan;
its `FIND_NEXT_BIT` expression is expanded to C11 statements for MSVC, retaining
the original masking, loop, bounds, first-set-bit helper and branch flow. The
existing complete generic `__ffs` header is reused. Native unsigned-long width
is retained (32 bits on LLP64 Windows, 64 on the checked Unix hosts). The dynamic
bitmap-size path is used, without the constant-size optimization or unrelated
search functions. The selected branch is Linux's little-endian host path, as on
the supported checked arm64/x64 hosts; big-endian runtime is not established.
Full dependency unit hashes for `find_bit.c`, `find.h`, `bitmap.h` and endian
bitops are recorded, and original notices remain. No independent scan algorithm
or format decoder is added.

Initialized bitmaps now pass their original checksum and structural predicates
before archive publication, including classic filesystems without descriptor
checksums. Checksummed non-root UNINIT groups retain their existing immutable
archive treatment: no stale bitmap bytes are consumed and empty inode groups
produce no inventory. Both UNINIT flags in the root group are rejected according
to the original read callers. Without descriptor checksums the flags remain
ignored. Kernel bitmap cache, allocation-group state, locking, corruption marks
and failure injection are not simulated. In particular, native read-only file
lookup need not load allocation bitmaps; the archive eagerly validates the
selected metadata and does not claim identical lazy corruption discovery.

Behavior regressions clear metadata and padding bits in classic and independently
checksummed images, preserving valid checksums so a checksum-only reader would
miss the corruption. They cover the end of the inode table, the first and final
padding bits, BIGALLOC cluster addressing and root/nonchecksummed flag handling.
The older synthetic BIGALLOC symlink fixture is corrected to encode cluster
allocation rather than retaining its previous block bitmap. Existing real mkfs
BIGALLOC/META_BG/inline images remain positive controls. Public ABI tests inject
E_FAIL/E_ABORT specifically at a classic block-bitmap read and verify exact
errors, no published items, successful reuse and a visible file after recovery.

On 2026-10-21 all seven focused suites passed on macOS (final 4.76s). Linux and
MSVC Windows passed the five unaffected suites in `20261021T004344Z-7853d3de`;
the two fixture-consuming stream/xattr suites then passed with the corrected
generator in `20261021T004518Z-aec3176b`. That generator fix supplies optional
fault offsets only for constructed images; existing native xattr fixtures have
no such builder field. Both runtimes and module builds reused incremental trees
and caches. The source predicates were compared again in full and all 301
baseline hashes matched. Adapted classifications include both generic read-side
dependency units `fs/libfs.c` and `lib/find_bit.c`.

No new native-kernel allocation-bitmap corruption run is claimed: read-only VFS
lookup need not reach these allocation validators. The Linux driver source is
the oracle for the transferred predicates, and public archive behavior is tested
separately. Full/slow suites, installed packages, UI and physical devices were
not checked. The broader format/read-feature review remains active.

### Reachable htree namespace and directory hash initialization

The remaining inode/creator-OS read branches were compared with their existing
owners, then directory enumeration callers were traced through pinned
`ext4_htree_fill_tree`, `htree_dirblock_to_tree`, `dx_probe`,
`ext4_htree_next_block` and `ext4_dx_readdir` fallback. SDK linear enumeration
visited every initialized allocated block of an indexed directory. That could
publish stale records from an allocated leaf which the htree no longer reaches,
or reject an otherwise readable tree because that unreachable leaf had a bad
checksum. Native htree enumeration does not visit that block.

The GPL module now uses original `dx_probe`, `dx_release`,
`ext4_htree_next_block`, getter/limit helpers and the complete
`__ext4_read_dirblock` body under their original `fs/ext4/namei.c` path.
`dx_probe` retains its hash-version, hash-in-dirent, flags, depth, count/limit,
binary search, cycle, frame and cleanup branches. Its optional filename-lookup
parameter and hash computation are omitted: this caller starts enumeration at
hash zero and supplies no lookup name. Complete copied bodies were compared
again with the pinned units; they are not inferred from passing tests.

The archive boundary follows the original fill-tree probe/root/leaf/next/release
call order, visiting every reachable leaf with `HASH_NB_ALWAYS`. SDK `ParseDir`
continues to interpret records and own names, metadata, item/ref projection and
sorting; fscrypt presentation remains in its existing owner. The kernel's
batch hash-cookie/rbtree publication and filename lookup are not recreated.
There is no usable upstream userspace callback boundary for the existing SDK
stream/parser, so narrow read/emit callbacks replace VFS buffer transport and
dirent-tree storage. Classic, inline and nonindexed directory paths retain the
existing bounded reader. Standalone retains its original linear SDK projection
and does not link GPL traversal.

Read callbacks query the existing validated mapping, preserve holes and exact
HRESULT failures, and check progress/cancellation between index reads. C buffers
and frame lifetimes have explicit owners; callbacks catch C++ exceptions before
crossing the C ABI. A bounded visited-bit set rejects repeated active blocks,
preventing repeated subtree expansion in malformed graphs. INDEX reads retain
bounded count/limit preconditions even without metadata checksums. Root limits
use the original declared info length: legacy extended root info remains accepted
when checksums are absent, while the original checksummed root parser requires
its supported layout. Reached leaves retain strict archive checksum validation,
including the empty-leaf ambiguity. `ERR_BAD_DX_DIR` during the initial probe
selects the existing linear fallback before any item has been emitted; other
read/checksum failures do not publish partial directories.

The complete original `ext4_hash_info_init` body and original hash constants now
initialize default hash version, seed and signedness. Invalid default versions,
including SIPHASH as the default, are rejected as in Linux, independently of
whether a particular directory uses an index. Creator-OS checksum/feature guards
remain in their existing readers. No device, journal recovery or namespace
mutation service is added.

CLI regressions cover an allocated unreachable leaf and its corrupt-checksum
variant, three htree levels with LARGEDIR, transitions into another internal node,
continuation-hash traversal, masked logical block values, legacy no-checksum root
layouts, initial-probe fallback, active holes/repeated subtrees and invalid default
hash versions. ABI tests inject E_FAIL/E_ABORT and callback cancellation while
probing a deeper index or moving to a later index, then verify empty failed-Open
state, late index checksum failure and successful handler reuse.

Seven focused ext/stream/fscrypt/xattr/backend/device suites passed on macOS,
Linux and MSVC Windows (`20261021T005458Z-ddb79393`). After the bounded legacy
root-info refinement, all seven local suites passed again (5.36s); final
`ext`/`ext_stream` reruns, including fallback, passed locally (4.87s) and on both
runners (`20261021T005720Z-134ec541`). Both official runtimes and module builds
reused existing incremental trees and compiler caches.

Native read-only Linux 6.8.0-147 confirmed twelve controls: reachable names and
exact bytes for the nine positive layouts (including an unreachable corrupt
leaf and legacy extended root info/fallback), plus refusal of three invalid
default hash versions (`20261021T005748Z-e46cd857`). Repeated-subtree/hole failure
controls were confined to the archive tests. Images remained unchanged and all
temporary mounts/images/helper sources were removed. No full/slow suites,
installed-package, native UI, physical-device or VFS hash-cookie ABI acceptance
is claimed. The parent format/read-feature audit remains active.

The final fscrypt caller comparison also resolves two hash-selection gaps:
the original initializer accepts unsigned default versions 3/4/5, and original
linear `ext4_readdir` supplies zero name hashes after fallback unless hashes are
stored in casefolded encrypted dirents. The existing hash boundary now accepts
the unsigned defaults and receives the actual linear-fallback state, preserving
the stored-hash branch. CLI regressions list/decrypt names and extract exact
plaintext with each unsigned default. A conversion of the native one-group
fixture to legacy metadata without checksums exercises encrypted htree fallback
with keys and
the public no-key name containing zero hashes. Its casefolded record tail is
converted to the valid non-metadata-csum geometry; no production validation is
relaxed to accommodate the fixture.

After these localized integrations, seven local suites passed (8.51s), and
`ext`/`ext_stream`/`ext_fscrypt_cli` passed on both runners
(`20261021T010051Z-17749cd8`). Native evidence above is specifically for the
twelve htree/default-hash controls, not execution of this additional fscrypt
fallback fixture by the kernel.
The additional encrypted fallback regression passed locally (0.90s) and on
both runners (`20261021T010537Z-4a8fd221`), with exact plaintext and zero-hash
no-key presentation. Temporary helper deletion was synchronized to both runner
current trees in `20261021T005910Z-fb3176cf`; no temporary native helper remains.

### Symlink storage, termination and owned target streams

The complete pinned `ext4_get_link`, `ext4_free_link`,
`ext4_read_inline_link`, `nd_terminate_link`, delayed-call ownership helpers and
`kfree_link` were compared with SDK symlink discovery, properties and streams.
The original plain block reader consumes only logical block zero, terminates
at `min(i_size, blocksize - 1)` and returns a C string. The inline reader instead
bounds termination by the actual inline payload. SDK full-i_size materialization
could expose bytes beyond that target or fail an inline link which Linux reads.

The complete C bodies now reside in their original `fs/ext4/symlink.c`,
`fs/ext4/inline.c`, `fs/libfs.c` and `include/linux/namei.h` paths; the original
`include/linux/delayed_call.h` is copied unchanged. Hashes/classifications are
recorded in the ext baseline and original notices retained. An owned inode and
bounded callback buffer replace block-device transport. The synchronous archive
caller supplies a non-null lookup token; unsupported RCU cache-only requests
retain the original ECHILD retry behavior, without constructing VFS dentries or
an inode cache. Fast plaintext links retain the existing original inode
validation and its validated i_data target. All borrowed implementations remain C.

The existing SDK mapping owners supply exactly one complete initialized block
for a slow symlink, or the complete 60-byte inode storage for an encrypted fast
link. This private raw-storage mode does not enlarge public regular-file streams.
Encrypted target presentation continues through the existing original fscrypt
reader; its ciphertext length is bounded by storage rather than a possibly
shorter i_size, matching `ext4_encrypted_get_link`. Inline symlinks preserve the
actual inline capacity before invoking the original reader, including an i_size
larger than that capacity. No new filesystem or encryption decoder is introduced.

Properties, extraction sizes and GetStream use the same presented target.
Archive payload size describes those target bytes (as already done for encrypted
links), whereas native lstat can retain a malformed raw i_size. Embedded NULs,
bytes after the first block and unused inline padding are not returned as target
data. Returned streams copy their owned target before Linux callbacks release
their buffers and survive Close/reopen/release. The later unavailable-target
iteration above confines ordinary eager source errors to the link with explicit
read-only consent. E_ABORT and explicit callback errors still propagate; failed
Open exposes no partial items. Public GetStream preserves source HRESULTs.
Standalone retains its SDK fallback and does not gain the GPL readlink reader.

Public CLI regressions cover first-block bounds, embedded NUL, unwritten tails,
large declared size, inline continuation/NUL/oversize/empty targets and first-block
holes. The constructed inline-xattr fixture now aligns arbitrary-sized values
as required by the format. An independently checksummed native encrypted fixture
with symlink i_size reduced to one byte still returns exact plaintext with keys
and the native no-key target via its actual encoded path. ABI tests enforce that
the second data block is not read, inject first-block errors/cancellation, check
empty failed-Open state/reuse and read an owned target after handler/source release.

Seven focused ext/stream/fscrypt/xattr/backend/device suites passed on macOS
(5.62s) and Linux in `20261021T012203Z-deb961e4`. Windows passed six in that
receipt; one subprocess in the prior bitmap-corruption test exceeded its
unchanged 10-second timeout. Isolated `ext` then passed
(`20261021T012418Z-f7809b9c`); the cause of the single shared-batch timeout is
not established. After final original-header placement, `ext`/`ext_stream`
passed on all three platforms (macOS 5.61s;
`20261021T012538Z-4b6b289a` on the runners). The latter source synchronization
also removed the disposable native helper from both current trees. An initial
new ABI-test release/lifecycle error was corrected; handler reuse and the
subsequent tests now pass. UCRT's existing `min` definition is respected.

Native read-only Linux 6.8.0-147 confirmed ten controls with unchanged images
(`20261021T012415Z-74c2e629`): exact block/inline targets, embedded NUL and
size clamps, empty inline target, refusal of a hole and the expected no-key
encrypted target despite i_size=1. That kernel reports EUCLEAN for the hole;
the archive reports a format/read failure through its existing error boundary.
All temporary mounts/images/helper sources were removed. The full copied bodies,
exact delayed-call header and all 303 baseline hashes were checked separately.
Incremental builds and caches were retained. Full/slow suites, installation,
native UI and physical devices were not checked; the parent review remains active.

### Read-only policy: image openings and recovery diagnostics

Review on 2026-10-21 found that direct-device recovery consent did not cover
ordinary ext images: the handler exposed feature declarations but neither
`kpidReadOnly` nor the limitation property consumed by the shared opening service.
Compared the original `CHeader::Parse`, archive properties and `CArc::OpenStream`
with their current consumers, then the pinned Linux `super.c` recovery/NOLOAD,
feature compatibility and mount-state branches, and `ext4.h` disk fields.
The original parser/property implementation remains authoritative. Two previously
unused superblock fields are read through its existing bounded little-endian
services; new code only reports recovery, pending orphan processing, unclean/error
state and unverified readonly-compatible declarations. There is no upstream
7-Zip consent/property implementation to reuse for these declarations.

Both engine builds expose the read-only property and concrete limitations.
The existing EN/RU opening service supplies per-opening consent, Cancel by default,
the persistent badge and copying. Direct-device policy consumes the same reader
reasons while preserving its incompatible-feature decoding restrictions. Unknown
readonly declarations retain useful unverified reading; consent never bypasses
missing decoding, source bounds or path checks. No source repair, writes, orphan
cleanup or journal replay are introduced. This reports an archive reading subset;
it does not port Linux mount/recovery services or assert consistency.

Eight deterministic image states exercise clean/recovery/orphan/error/unknown
declarations and combined warnings through CLI listing and exact extraction in
both engines, verifying unchanged input bytes. EN/RU native Cocoa image tests
cover cancellation, consent, copy, badge/diagnostic geometry, refresh and fresh
opening. Device tests cover error state and the legacy pending-orphan pointer
in addition to existing feature/decoding checks. The six focused suites `ext`,
`ext_stream`, `archive_device`, `device_ui`, `filesystem_read_ui` and `language`
passed on macOS arm64 in 50.71 seconds and Linux in 31.94 seconds
(`20261021T074719Z-876845c3`). Windows passed five suites in that batch; the
CLI suite reached its existing 45-second timeout. An isolated repeat with
unchanged source and timeout passed (`20261021T075203Z-e5ff5022`); the cause
of the shared-batch timeout is not established. Native Cocoa JSON confirms
translated combined reasons fit at 420×279 (EN) and 420×366 (RU); other guest
UI checks do not establish native visual parity. The application was rebuilt.
No new physical-device or native-kernel mount acceptance is claimed by this
policy check. The larger Linux-reference review remains active.
