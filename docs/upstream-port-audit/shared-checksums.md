# Shared checksum and hash primitives

Scope: XXH32 for EROFS xattr filters and LZ4 Frame; SDK XXH64 for hash
commands, Zstandard checksums and Btrfs; Adler32 in the SDK zlib wrapper
used by HFS and native APFS, DMG, XAR and compressed VMDK consumers. No format decoder,
recovery, framing, cancellation or checksum policy is replaced.

## Source comparison and ownership

The 7-Zip baseline is https://github.com/ip7z/7zip.git at
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Complete original inventory units
`C/Xxh64.c`, `C/Xxh64.h`, `CPP/Common/Xxh64Reg.cpp`,
`CPP/7zip/Compress/ZlibDecoder.cpp` and `.h`, their callers and dependencies
were reviewed before adaptation. `docs/upstream-baseline.json` maps the
adapted C and C++ owners and pins SHA256 of both original implementation units.
The original header and hash registration remain unchanged.

- `src/C/Xxh64.c` retains SDK symbols, state layouts, seed zero and the
  original streaming block/tail orchestration. Hash arithmetic and finalization
  use the existing pinned libzstd xxHash. A narrow adapter translates four SDK
  accumulator lanes, total length and pending bytes to the statically linked
  library state. This deliberately depends on the pinned state representation;
  review it on a library update. Digest does not mutate the caller's state.
  Architecture-specific SDK hash arithmetic is omitted.
- `src/lib/xxhash.c` retains Linux's seeded `xxh32` service for EROFS and
  delegates to the same XXH32. Its original Linux inventory/hash remains in
  `docs/erofs-linux-baseline.json`; no Linux hash arithmetic remains locally.
- LZ4 Frame retains the unchanged official framing implementation and consumes
  the shared xxHash header through its include path. The bundled LZ4 xxHash
  `.c` and `.h` are removed locally; original hashes remain inventory-only
  references in `docs/lz4-frame-baseline.json`.
- `src/CPP/7zip/Compress/ZlibDecoder.cpp` replaces only Adler32 arithmetic
  with `adler32_z`. Empty updates preserve the supplied SDK seed. Output
  accounting still follows bytes actually accepted by the destination.
  Optional absent trailers, rejection of partial/incorrect trailers, the
  DEFLATE decoder and all error/progress paths retain their upstream bodies.

The authoritative unchanged xxHash is part of https://github.com/facebook/zstd.git
at `f8745da6ff1ad1e7bab384bd1f9d742439278e99`, with original whole-file hashes
and owners in `docs/zstd-encoder-baseline.json`. LZ4's omitted copy is pinned
at https://github.com/lz4/lz4.git,
`ebb370ca83af193212df4dcbadcc5d87bc0de2f0`.
The unchanged Adler implementation is https://github.com/madler/zlib.git at
`da607da739fa6047df13e66a2af6b8bec7c2a498`, mapped and hashed in
`docs/zisofs-zlib-baseline.json`. No dependency revision was advanced.

The native engine/console overlay receives both adapted SDK units and the
unchanged Adler source/header closure through the existing runtime manifest.
Vendored runtime dependencies are mapped using the same prefix-removal rule
as local integration, with duplicate destination rejection. zlib C compiles
with `Z_PREFIX`, retaining its source language and avoiding symbol collisions.
The CMake HFS module links the existing zlib target. The adapter explicitly
uses the zlib C calling convention on MSVC; two undefined-feature-macro
warnings are suppressed only while including the unchanged zlib header under
the native SDK's `/Wall` policy. License notices for zlib
are installed for the shared runtime in addition to existing module notices.
The OpenSSL configure probe now includes `openssl/evp.h` for its EVP cipher
calls; the prior probe failed on a fresh configure despite a valid library.

## Review and verification

Use `scripts/upstream_baseline.py verify/review` for the SDK units and
the existing `scripts/upstream_baseline.py reference-verify/reference-review` workflow for the
zlib, Zstandard, LZ4 and Linux manifests (see each owning audit). Compare
proposed original whole units with their recorded baseline and local owners;
do not hash an adaptation as its original baseline.

`shared_checksum` checks frozen pre-change XXH32/64 results, seeded XXH32,
unaligned buffers, split updates, repeated digest followed by more input,
SDK block/tail calls, independent Adler vectors, partial writes and optional
Adler trailer outcomes. Existing native hash, filesystem and frame suites
exercise direct consumers. The runtime-tooling regression compiles a vendored
C dependency, modifies its header and verifies changed executable output.

## Validation, 2026-10-24/25

Feature `feature/shared-xxhash-adler32`, based on
`4466186335f168c0dc632bf793862426ec11df6c`:

- macOS arm64: affected libraries, application and native runtime built;
  all 17 focused suites passed (23.93 s). After localized tooling/include
  follow-ups, three tooling suites passed (14.76 s), three checksum/native
  consumer suites passed (4.07 s), and the final checksum suite passed again.
- Linux x64: all 17 suites passed (80.16 s), runner receipt
  `20261024T210209Z-d431d9dd`. Final includes/guards: shared_checksum, HFS and
  Zstd passed (8.94 s), receipt `20261024T210755Z-3aa33af0`.
- Windows x64/MSVC: native runtime and selected targets built. Initial full
  selection passed 16/17; linux_frames_stream::backendNestedExtraction failed
  at codec discovery before extraction (receipt `20261024T210436Z-042890e1`).
  Focused repeat passed checksum/HFS/Zstd but repeated that discovery timeout
  (`20261024T211555Z-da1c485a`). Both the retained older LZ4 build and the
  same current test environment with the pre-change OpenSSL core passed the
  control scenario (`20261024T212306Z-43041f87`, `20261024T212447Z-0b1712c0`).
  The final core was restored byte-for-byte; its unchanged linux_frames_stream
  suite then passed in 3.22 s (`20261024T212654Z-62990350`). No timeout was
  changed. Cause of the two earlier discovery timeouts is unconfirmed;
  reliable cold catalog startup is not established by this acceptance.

SDK verification passed 398 exact, 269 adapted and 21 local owners. All 14
zlib and 65 libzstd mapped units equal their pinned original hashes; all 12
LZ4 original inventory units verified, and reference-review was exercised.
No new library source or revision was introduced. Whitespace checks passed.
Review metadata and receipts are retained under task-local
`build/review/shared-hash` and the shared runner log root. Disposable control
sources, libraries and driver are removed; incremental trees/caches are kept.

No full/slow gate, release installation, physical media, UI/pixel acceptance
or performance claim is included. Checksum equivalence covers the frozen
vectors and selected consumers, not every possible corrupt input.

## CRC32 and GPL hash-library consolidation, 2026-10-25

This continuation examines checksum, hash, compression and text-conversion
implementations against libraries already present in the production build.
Similar names and disabled historical bodies are not treated as live duplicates.

### Implemented ownership

- `src/lib/crc/crc32-main.c::crc32_le` preserves Linux's raw seeded state API
  around public `lzma_crc32`, complementing the incoming and returned state.
  Empty updates retain the seed. LZOP keeps its existing header/data/compressed
  checksum branches. The duplicate IEEE loop/table are removed; the existing
  generator emits the retained BE and CRC32C tables. `crc32_be` and `crc32c`
  retain their bodies. Frame Adler calls use the existing size-aware `adler32_z`.
- The shared liblzma target now provides CRC32 and CRC64 with immutable tables
  and its existing endian configuration. Added unchanged XZ units are
  `src/liblzma/check/crc32_fast.c`, `crc32_table_le.h` and `crc32_table_be.h`.
  The native core still needs only its existing CRC64 source closure.
- `src/lib/crypto/sha512.c::sha512_blocks_generic` delegates full blocks to the
  existing SDK `CSha512`, `Sha512_Init` and `Sha512_Update`. Linux keeps its
  SHA-384/SHA-512 IVs, buffering, 128-bit length accounting, padding, HMAC,
  precomputed states and finalization. SDK finalization is not used.
- `src/lib/crypto/sha3.c::sha3_keccakf_generic` keeps Linux endian conversion
  and delegates the permutation to the existing SDK SHA-3 implementation.
  Linux still owns absorb buffering, domain separation, HMAC and incremental
  SHAKE output. The bridge imports native lanes into `CSha3`, then absorbs one
  complete zero SHA3-256 rate block: XOR with zero preserves the lanes before
  the SDK permutation. It adds no padding and does not call SDK finalization.
- `src/lib/crypto/sdkhashblocks.c/.h` is local C integration. Its temporary SDK
  contexts are erased with existing `memzero_explicit`. The SDK bodies remain
  byte-exact. Their C objects now belong to `sevenzip_fscrypt_crypto`, also reused
  by ext/fs-verity consumers instead of compiling another set of those objects.

The [existing liblzma manifest](../erofs-liblzma-baseline.json) pins complete
original files and unchanged owners at <https://github.com/tukaani-project/xz.git>,
`d3e650e63c110e830fd5391e7f8b45df0b91d3da` (0BSD). The
[EROFS Linux manifest](../erofs-linux-baseline.json) keeps original CRC/generator
hashes and records the partial API adaptation and regenerated output. The
[ext Linux manifest](../ext-linux-baseline.json) pins complete original hash
units at <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
`58161ead29e50d70a317ea26b472e0c7508e69da`, with selected SDK adapter symbols and
retained Linux scope. Existing notices are preserved.

The [SDK manifest](../upstream-baseline.json) pins full original implementation
and API units at <https://github.com/ip7z/7zip.git>,
`b88c6bc09339de32f7d18bbab900686b244f42ba`, including public state layouts used
by the bridge and the units compared for rejected candidates. SDK sources stay
public domain and byte-exact under `third_party/7zip/C`; new glue is local GPL
integration. The rebased production source uses the restored original SDK crypto;
this continuation introduces no OpenSSL dependency or backend selection.

### Reviewed boundaries and remaining candidates

| Examined implementation | Finding and disposition |
| --- | --- |
| Linux SHA-1 and SHA-224/SHA-256 versus SDK | Linux explicitly erases its round workspaces. SDK public update APIs do not expose private workspaces for erasure. Prototypes matched digests but could not retain that property through the public API. Original Linux bodies remain; they are not counted as completed consolidation. |
| SDK CRC32 (`7zCrc.h`, `CrcReg.cpp`) | The seeded API includes byte updates via `g_CrcTable` and explicit SW/HW hash selectors through `z7_GetFunc_CrcUpdate`. A generic library call would not preserve all those controls. Keep the existing SDK implementation for these consumers. |
| CRC32C in filesystems/VHDX/XFS | zlib/liblzma CRC32 uses a different polynomial. A subsequent review completed the common LGPL VHDX extraction described [below](#shared-crc32c-service), preserving GPL module separation. |
| Commented CRC32C in `ExtHandler.cpp` | The body and former caller are disabled history. Live metadata checking belongs to Linux; no functional duplicate was removed or claimed here. |
| Linux and SDK AES | Linux modes now use the SDK key/block adapter [below](#shared-aes-blocks-with-linux-modes), retaining state/erasure contracts and the explicitly accepted omission of Linux table prefetch. |
| SDK raw LZMA/XZ and liblzma | liblzma restricts `lc + lp` to four; SDK supports the broader legacy property range. Random XZ blocks, consumed-input accounting, recovery prefixes and cancellation are further contracts. The completed MicroLZMA migration does not establish general replacement. |
| SDK DEFLATE and zlib | Filesystem DEFLATE already uses zlib. SDK also serves Deflate64 and NSIS, and its zlib wrapper supports optional absent Adler trailers. Removing it would lose contracts or require further adaptation. |
| BLAKE2 | Linux BLAKE2b and SDK BLAKE2sp are different algorithms. Sequential BLAKE2s does not replace the tree hash; current production dependencies offer no matching replacement API. |
| Base64 and UTF conversion | SDK archive Base64 includes discovery, whitespace and tail contracts; Linux Base64 serves filesystem name variants. GPL/module and independent-console boundaries preclude a direct swap. SDK UTF conversion also preserves raw non-UTF-8 names and escape modes beyond ordinary Qt Unicode conversion. |
| Pinned dependency internals | Private CRC/LZMA helpers inside lzlib and other unchanged libraries have no public primitive-delegation API. Replacing them would create a dependency fork. Existing archive/filesystem library assessments retain their format/recovery compatibility limits. |

These are dispositions for the reviewed scope, not a claim that future
compatible consolidation is impossible. The common CRC32C service and AES
key/block adapter are completed below with their adaptation boundaries.

### Verification

Use existing `upstream_baseline.py verify/review` for SDK units and
`reference-verify/reference-review` with the owning Linux/liblzma manifests.
Compare proposed full original units, their pinned baseline and local adapters
before advancing revisions. Regenerate the retained CRC tables with the recorded
C generator. Original source hashes do not follow local formatting changes.

`linux_checksum` freezes pre-change raw CRC outputs across seeds, alignment,
split updates and empty calls, and distinguishes IEEE/BE/Castagnoli results.
LZOP fixtures add independently corrupted CRCs for header, decoded data and
compressed data. Existing asymmetric/fscrypt/fs-verity suites cover digest,
HMAC/HKDF, signatures, certificates and encrypted image reading. Additional
SHAKE128/256 vectors check copied contexts and split output across rate blocks
against Python hashlib. Regenerate with
`scripts/generate-asymmetric-hash-fixtures.py --shake`.

The final focused selection contains 29 suites covering raw checksums, native
hashes, Linux frames/CPIO/XZ, filesystem readers/modules and asymmetric/fscrypt
consumers. macOS arm64 passed all 29 (14.94 s). Linux x64 passed all 29
(105.86 s), receipt `20261025T052520Z-c8332c7f`.

Windows x64/MSVC built the runtime and selected targets. The combined selection
passed 26/29 (206.29 s), receipt `20261025T052417Z-d891dcf6`: Btrfs and nested
frame backend discovery timed out, and the ext CLI suite exceeded its unchanged
45-second limit. An isolated repeat passed both backend suites while ext repeated
the timeout, receipt `20261025T053255Z-30e4ebba`. A control restored the complete
CRC/SHA bodies and CRC table from rebased master in the same incremental build;
ext also timed out (45.39 s), receipt `20261025T053558Z-3e666611`. The control
driver restored the adapters, but its redundant final CMake configuration stalled
and was stopped; `--build-only` encountered the same regeneration stall
(`20261025T054349Z-46d849cd`). Restoration then reused an unchanged temporary copy
of the existing Ninja graph in the same build tree. The current ext module rebuilt,
and `ext_stream` plus `ext_fscrypt` passed, receipt `20261025T054952Z-fdc56e86`.
No compiler flags, dependency revisions or cache settings changed. Thus 28 selected Windows
suites passed, but complete Windows ext CLI acceptance remains unverified;
the timeout is not attributed to the new primitives. No timeout was increased.

SDK baseline verification passed 408 exact, 261 adapted and 21 local owners;
all 40 liblzma original units verified. Existing reference verification/review
also checked the selected complete Linux CRC/generator and SHA-512/SHA-3
original units against their pinned hashes. This is not a full Linux-manifest
verification. CRC table and SHAKE fixture regeneration matched retained bytes.

After rebasing onto restored SDK crypto, the existing native make outputs still
contained objects compiled against the former OpenSSL AES ABI and, on Windows,
an incompatible PCH. Their linker/loader diagnostics identified the obsolete
objects/PCH for removal; affected objects were rebuilt with the existing compiler
cache. Build trees, toolchains and caches were retained.

No full/slow gate, release installation, UI, performance or big-endian runtime
acceptance is included. Source consolidation covers the selected primitives;
Linux streaming/finalization remains an intentional adaptation boundary.


## Shared CRC32C service

The next continuation extracts the existing 7-Zip VHDX CRC32C implementation into
`src/CPP/7zip/Archive/VhdxCrc.cpp`, with a C ABI in `VhdxCrc.h`. The complete original
`CPP/7zip/Archive/VhdxHandler.cpp` is pinned in the existing SDK manifest at
`b88c6bc09339de32f7d18bbab900686b244f42ba`, repository
<https://github.com/ip7z/7zip.git>, LGPL-2.1-or-later. Its SHA256 is
`2956f3c02e39b94db8a47e6321f1dde1244446022ba5c02e79733c35af736fe6`.
The original handler and `DOC/copying.txt` are byte-identical to the former XFS
checksum baseline `9128b80e3a471108678e2b3ec3c985861c8d7b0f`; comparison retained
both complete inventory units before aligning that manifest with the engine.

### Source ownership and retained consumers

- `Crc32c_GenerateTable` and `CrcUpdateT1_vhdx` retain the original Castagnoli
  polynomial, table entries and byte-update loop. A function-local const table
  replaces the global table/initializer pair so concurrent first calls are
  synchronized and subsequent access is read-only. Empty/null updates return
  the incoming raw state before pointer arithmetic. The C ABI explicitly uses
  cdecl on MSVC because native nmake compiles with a different default convention.
- `src/CPP/7zip/Archive/VhdxHandler.cpp` retains the complete original reader;
  only checksum implementation/initialization moves. `Crc32c_Calc` still applies
  the original initial/final complements. All header, region and log callers,
  checksum-field handling, parsing, extent/BAT, parent and stream behavior remain.
  The newly owned full reader is formatted to repository style; comparison with
  the equivalently formatted original keeps this separate from the extraction.
  The disabled log replay branch remains disabled. No new VHDX recovery behavior
  or source writes are introduced.
- `src/xfschecksum.cpp` retains the bounds check and temporary zero/restore of
  its caller-owned metadata checksum. Its duplicate table and loop are removed.
- Linux `crc32c` preserves its raw seeded C API around the same service. Its
  duplicate Castagnoli table/loop and generator branch are removed; IEEE CRC32
  still uses liblzma, and the BE CRC32 body remains. The existing Linux manifest
  retains original complete-unit hashes and records the adapter/generated output.
- CMake compiles the extracted implementation once into `sevenzip_crc32c` for
  the direct Linux/XFS consumers. The native Format7zF closure compiles the same
  C++ owner through the existing runtime dependency tooling. The console/SFX
  closures do not gain a VHDX dependency. Linux and XFS readers remain in their
  separate GPLv2/GPLv3 modules; neither acquires the other's code or license.

The SDK `extracted`, runtime selection and complete-original hashes, the existing
XFS reference mapping, and the Linux library-adapter records describe this scope.
Use the existing `verify/review` and `reference-verify/reference-review` commands
for proposed revisions; compare the full original reader and the extracted owner
together. The new header and raw-state wrapper are local integration; there is no
new checksum algorithm, production dependency or upstream revision update.

### AES review disposition

The review covers Linux `lib/crypto/aes.c`, its key/state and mode headers,
`aesprovider.c` callers, and SDK `C/Aes.c`, `C/Aes.h`, its table/key/block bodies,
CBC/CTR callers and hardware fallback dispatch. Complete original Linux and SDK
units remain pinned in their existing manifests. Linux's mode bodies are kept;
the key/block adapter below replaces its generic primitives.

The SDK prepared-key prefix, forward decryption key order, aligned CBC state and
in-place blocks differ from Linux's reversed inverse keys and unaligned block API.
Those differences can be adapted: for example, one CBC block with a zero IV can
provide a block operation while Linux keeps its mode semantics. They alone are
not grounds to declare sharing impossible. SDK CTR itself cannot replace Linux
CTR/XCTR: counter byte order, increment timing and partial-block handling differ.
Linux CTS/XTS/HCTR2/Adiantum consumers also require their current mode boundaries,
state copying and temporary-state erasure.

The compatibility review identified Linux's
`aes_prefetch` before table-based encryption/decryption. It performs volatile
reads and a compiler barrier as basic cache hardening; it does not claim full
constant-time AES. SDK software `Aes_Encode`/`Aes_Decode` have no equivalent step,
and their T/D tables are private. The user explicitly accepted omitting this
table-prefetch measure for this application on 2026-10-25. With that scoped
decision, the adapter can reuse the unchanged public SDK API. State and temporary
memory erasure remain part of the retained contract.

### Focused behavior checks

`linux_checksum` extends the frozen pre-change Linux answers to arbitrary CRC32C
seeds, unaligned input, split updates and empty/null calls. Existing filesystem
suites check valid metadata and rejection of corrupted checksums. The native VHDX
fixture reads a small allocated disk through the prepared core and independently
checks rejection of damaged primary/secondary header and region-table CRCs. It
protects the checksum caller/closure, rather than source structure or symbol names.
No log-replay/recovery acceptance follows from this fixture.

macOS CRC selection passed 13/13; explicit SDK-header and formatting follow-ups
passed their direct consumers. Linux passed 13/13 (28.40 s), receipt
`20261025T060330Z-a6671dea`, then the include follow-up passed 3/3. Windows caught
the missing explicit SDK types include during native compilation; adding it
preserved provider bodies and fixed the build. Final combined acceptance is below.

## Shared AES blocks with Linux modes

Linux's duplicate AES S-box/round tables, key expansion and generic encryption/
decryption rounds are replaced by SDK primitives already present in the build.
The complete original Linux `lib/crypto/aes.c` and `include/crypto/aes.h` remain
pinned in the ext manifest at `58161ead29e50d70a317ea26b472e0c7508e69da`.
The existing SDK manifest pins complete `C/Aes.c`, `C/Aes.h`, and `C/AesOpt.c`
at `b88c6bc09339de32f7d18bbab900686b244f42ba`; these providers remain byte-exact
public-domain code from Igor Pavlov. `CpuArch.c` is the existing dispatch
dependency. No SDK backend, revision or production library is added.

`sdkaesblocks.c/.h` is narrow local GPL integration. It calls `Aes_SetKey_Enc`/
`Aes_SetKey_Dec`, snapshotting the input key before writing prepared output,
keeping Linux's native-word round-key layout and reversing the
SDK inverse-key sequence for Linux's prepared decryption state. For one block,
it reconstructs an aligned SDK context with a zero IV and invokes
`g_AesCbc_Encode`/`g_AesCbc_Decode`. One zero-IV CBC block supplies the block
operation; SDK CTR semantics do not replace Linux's CTR/XCTR. Input is copied
before output so unaligned and overlapping buffers retain their behavior.
Temporary prepared keys, SDK contexts and block buffers are explicitly erased
with the existing Linux `memzero_explicit`.

`sdkaesinit.cpp` owns synchronized first-use `AesGenTables` initialization for
the GPL library's SDK instance. It is local C++ integration because C++ provides
portable thread-safe initialization across the existing toolchains. Linux AES,
the bridge and SDK providers retain C compilation. Original Linux table prefetch
is intentionally omitted as accepted above; SDK normal dispatch and its software
fallback supply their existing behavior. SDK algorithm bodies are not adapted.

Linux `aes_expandkey`, prepare-key and block APIs retain validation, key lengths
and prepared-state structure. Existing ECB/CBC/CTS/XTS/CTR/XCTR bodies, IV/counter/
tweak updates, partial-block behavior, weak-key checks and temporary erasure are
unchanged. Existing ESSIV/HCTR2/Adiantum/fscrypt consumers keep those mode APIs.
Unused declarations of the removed private round tables are removed from the local header;
there are no transferred consumers of those tables. The ext manifest maps
selected replaced symbols, retained scope and all local owners to the provider.

The existing `sevenzip_fscrypt_crypto` target compiles the same SDK C owners used
elsewhere, together with the local adapter; GPL modules do not acquire core/unRAR
code, and the native core does not acquire Linux GPL code. No reader/write policy,
password requirement, extraction path validation or UI contract changes.

Existing independent AES-128/192/256 vectors exercise Linux public block/mode
APIs with unaligned keys/buffers, in-place processing, encryption/decryption,
IV results and stream continuation. The suite now also selects the SDK's public
software CBC dispatch functions and repeats those behavior vectors, restoring
normal dispatch afterwards. This detects fallback-specific adapter mistakes
even on machines whose normal SDK dispatch uses CPU instructions. Existing
invalid-key, mode, policy and encrypted filesystem fixtures remain in use.
No test pins key-array bytes, private symbols or source structure.

macOS AES selection passed 11/11 (5.79 s); normal/software dispatch vectors also
passed separately. The final combined 21-suite selection covers CRC, native VHDX,
XFS/Btrfs/EROFS modules/readers, frames, ext streams/recovery/xattrs/fscrypt and
asymmetric/hash/fs-verity consumers. macOS arm64 passed 21/21 (6.39 s), and Linux
x64 passed 21/21 (36.69 s), receipt `20261025T062924Z-b5fa91bb`.

Windows x64/MSVC built the native runtime and affected targets. Its combined run
passed 20/21 (154.55 s), same receipt; `xfs_module` exceeded its unchanged 45-second
limit. An isolated verbose repeat passed every XFS case (2.043 s of QTest work),
receipt `20261025T064133Z-1e5ce139`, and a subsequent ordinary CTest repeat passed,
receipt `20261025T064933Z-3f6fbb1d`. Earlier codec-discovery timeouts did not recur
in the combined run. Their cause is unconfirmed; reliable cold catalog startup
is not established by successful repeats. No timeout or compiler-cache policy
was changed. Thus all selected Windows scenarios passed across these runs.

Final review also compared preparation with input key material overlapping its
prepared-key storage. The previous Linux implementation passed that control;
the initial adapter reread overwritten input for decryption-key expansion.
Snapshotting and subsequently erasing the raw input key fixed it. The control
then passed; its representation-specific placement remains a disposable manual
diagnostic rather than a permanent test tied to key internals. After this local
follow-up, all 11 AES/crypto consumer suites passed on macOS (6.79 s), Linux
(13.82 s) and Windows (22.66 s), receipt `20261025T070316Z-0fc73b6e`.

SDK baseline verification passed 408 exact, 262 adapted and 23 local owners.
XFS original units and the selected complete Linux AES unit verified; existing
review tooling compared their original baselines/providers/adaptations. The
remaining BE table regenerated exactly. Full original VHDX reader comparison
after equivalent formatting confirmed retained bodies/callers unchanged, and
Linux mode/state/erasure bodies remained byte-identical after the replaced
generic primitives. Source completeness and runtime evidence are separate:
only the recorded primitive scope is consolidated, with the accepted prefetch
omission and the retained mode/reader boundaries above.

No full/slow gate, release installation, UI/pixel, performance or big-endian
runtime acceptance is included. Disposable controls/generators/drivers and
completed timeout fixtures are removed; small review evidence, incremental
build trees and compiler caches are retained.
