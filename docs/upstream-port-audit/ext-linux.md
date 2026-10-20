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
layouts while retaining refusal of filesystem encryption. An explicit
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
features remain permitted by the read-only feature check. This does not supply
fscrypt decryption. Inode checksum rules already match Linux's creator-OS guard;
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
read bodies unchanged apart from explicit C pointer casts and replacement of
GNU's omitted ternary operand for MSVC. New dependencies are pinned in the
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
