# Selected native reader transfer

Source branch: `local-dev` at `d439ffdf29a0e3960942af7449aef2609b7c5761`.
Target baseline: `0d997ccad5e2500813aaf5f999500e631e76f404`.
This record covers the selected transfer requested on 2026-10-23; universal
extraction replay and a blanket restriction on nested editing are excluded.

## Progress controls

The completed transfer is recorded in [progress controls](progress-controls.md).

## Native enumeration and testing

Sources: `3e276e74cc04b80c11c1276827ed2f9640b6f3eb`,
`ae78a45a02f17a04c88a3514558f55cdba29aab6` and
`8c0c78acaecd82060dbc0ca2eaefe12ef2ecc2e1`, using their final combined
`QtArchiveReadOperation` rather than intermediate workers. Upstream source:
<https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/UI/Common/OpenArchive.cpp` (`CArchiveLink::Open`, `CArc` properties),
`ArchiveExtractCallback.cpp` (`GetStream`, `SetOperationResult`,
`ReportExtractResult`, `RequestMemoryUse`), `CPP/7zip/UI/Agent/Agent.cpp`
and `CPP/7zip/UI/Console/List.cpp` (`GetPropName`, `PrintTime`,
`PrintItemInfo`, `PrintPropertyPair2`, `Print_OpenArchive_Props`).
The test-callback projection in the source branch originally cites
`b88c6bc09339de32f7d18bbab900686b244f42ba`; the relevant bodies and dependencies
were compared again with the available 9128 baseline.

Copied the Console property-name table and retained its technical property/time,
attribute and raw NT metadata conversion. Omitted console drivers, formatting
and output transport. Qt owns typed snapshots and worker delivery. Open/Extract,
format detection, item mappings and decoding remain official engine operations.
The worker uses original item/block error projection, an operation-scoped
password service and cooperative pause/cancel checkpoints; memory requests keep
the already allowed engine budget. No memory-limit consent is invented for the
standalone non-dialog API. Open diagnostics do not turn a partial test into a
successful one, and incomplete/cancelled listing snapshots are not published.

Folder navigation keeps the native catalog/reader instead of reopening through
CLI text listing. This retains distinct indexes for duplicate member names.
The Console projection belongs only to `adapted`, not `runtime_adapted`:
the official runtime retains its original console module. Existing CLI fallback
remains for no-catalog consumers. Materialized nested editing remains enabled;
automatic direct container chains retain their existing update boundary.

Focused runtime acceptance on 2026-10-23: macOS passed native listing/test,
opening/options, native folder and RAR-memory consumers, update and nested
editing scenarios. The complete application target also built successfully.
Linux passed the 13-suite selected set in `20261023T181315Z-e4d9023f`;
Windows passed that set in desktop session `20261023T181906Z-5145d185`.
The Windows SSH session-0 attempt was not accepted: codec discovery and archive
progress required the desktop repeat. Test-only corrections cover canonical
macOS filenames, actual native indexes, unavailable CLI and repeated dismissal
of source error dialogs; existing nested-edit behavior checks are retained.
No slow suites or full gate were run. These checks accept file-based native
enumeration/testing; direct stream chains and retained generations are separate
scope below and are not established by this result.

## QCOW

Transferred the QCOW portion of
`d439ffdf29a0e3960942af7449aef2609b7c5761`. Upstream reference:
<https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/Archive/QcowHandler.cpp`, `HandlerCont.cpp/.h`,
`CPP/7zip/Archive/SquashfsHandler.cpp`, `C/ZstdDec.c/.h` and
`CPP/7zip/Common/CWrappers.cpp/.h`.

Compared the complete QCOW handler, image handler callers, tables, stream and
decoder dependencies. Formatting is separate from functional adaptation:
the handler retains upstream mappings, Deflate, sparse/zero reads and bounds
checks. The additional Zstandard cluster path uses the official caller-buffer
decoder, stops at the frame boundary before sector padding, and requires a full
cluster without extra output. External-data, extended-L2 and unknown incompatible
layouts are rejected; backing files and encryption retain their limitations.

One SDK adaptation is registered in both `adapted` and `runtime_adapted` in the
baseline manifest. Normal runtime preparation overlays that source onto the
official engine build; no second QCOW implementation is introduced. The existing
official decoder and LGPL notices remain authoritative. Test images use the
existing disposable MBR/ext4 source; the Zstandard image was produced by QEMU.

Runtime evidence: macOS, Linux and Windows `archive_qcow` pass all nine cases, including
stored/Deflate/Zstandard source-byte comparison, sparse tail and backward reads,
unsupported layout rejection and corrupted cluster failure. Linux and Windows
receipts are the focused runs above. Retained stream-chain validation is recorded
below.

## Retained native streams and generations

Selected source changes:
`a304ac2dce8b333973b034a883595879dcb938c2`,
`60ac293c1585ba12f2f2b546a559a7cec76065e8`,
`c11916e5fd515c1081a48f1b657296e2475a7e93`,
`33828e5a4d3849d59d2547c1cae919d80ad5e9e4`, native portions of
`5ce5784332c0c1567032a8856c46b8321581f895`, and final branch corrections.
Upstream reference: <https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`:

- `CPP/7zip/UI/Common/OpenArchive.cpp` complete `CArchiveLink::Open` stream descent,
  `CArc::OpenStreamOrFile` and reopen/volume callback dependencies;
- `CPP/7zip/UI/Agent/Agent.cpp/.h` stream-index/proxy mapping, update capability,
  reopen and folder ownership;
- `CPP/7zip/UI/FileManager/PanelItemOpen.cpp`, `FileFolderPluginOpen.cpp/.h`,
  `ExtractCallback.cpp/.h`: stream-first opening, CopyTo, virtual-file memory
  growth, spill, close, changed-file detection and parent-update prompt;
- `CPP/7zip/Common/StreamObjects.cpp`, `CPP/7zip/Common/LimitedStreams.cpp`,
  `CPP/7zip/Archive/7z/7zDecode.cpp::CLockedSequentialInStreamMT`: cursor/seek and
  serialized shared input patterns. The source branch originally cites
  `b88c6bc09339de32f7d18bbab900686b244f42ba`; these bodies were compared with
  the available 9128 baseline again.

The native handlers and their mappings are retained. Qt integration supplies
reader-generation leases, independent cursor positions and scoped operation
requests, with source seek/read control flow and bounds. Device generations
also receive independent authorized cursors; stream properties such as sector
size are forwarded when supplied. Native Extract/Open do not require a live
window backend. Remaining window/process service callbacks use guarded owners.
The native read-only boundary follows the input source, not total nesting depth.

The source branch's lazy factory advertises seek before resolving its native
stream, then falls back to replay. That factory was not copied. The original
`IInArchiveGetStream::GetStream` signature has no per-call progress argument,
so a narrow request-aware bridge preserves Agent's original index mapping and
calls the real native factory in the joined opening worker. Only an actual
native `IInStream` is exposed as seekable. No fallback decoder or synthetic
sequential-Extract seek implementation is introduced.

CopyTo retains its source memory allocation/spill logic. Its caller explicitly
requests the existing owned-file materialization when opening the extracted
memory result; arbitrary native streams remain direct. This preserves the
materialized child's update path and source modified-file prompt. The blanket
nested editing prohibition from `6c9ee3fc` is excluded.

Also excluded: replay input/cache/budget, replay seek probes and special open
results, replay-only memory/partial-result bridges and their tests, Alpine
tooling and the branch's historical migration log. TAR provides a real native
parent stream in the retained-chain regressions; ZIP remains an Extract-only
child in the lifetime case. Fixtures check byte results rather than source text.

Source comparison is complete for the selected stream, callback and caller
scope above. Native format mapping/decoder bodies were not independently
rewritten.

Runtime evidence, 2026-10-23: the selected 18 suites passed on macOS and Linux.
They cover stored/Deflate/Zstandard QCOW through MBR/ext to selected Unicode
file bytes, sparse/backward reads without full intermediate copies, current
request cancellation during native factory mapping and ancestor reads,
independent old/new-generation cursors, refresh after backend destruction,
retained extraction and final module release. Existing memory/spill, nested
edit/publication, external editing, native test/listing and device consumers
remain in the selected set. Linux receipt: `20261023T185403Z-5ca436fe`.

Windows passed 17 suites in that same desktop run. `codec_update` reached its
60-second limit there, then passed alone in 53.98 seconds, without source changes
(`20261023T190042Z-a3121d3a`). The earlier 15-suite run also passed that suite;
its new opening-options assertion needed native separator normalization, now
covered by the final run. This accepts the selected Windows scenarios while
retaining the timeout as a validation limitation. Incremental builds and compiler
caches were reused. The application target built on all three platforms; Linux
and Windows build receipts are `20261023T185917Z-15d6ed62` and
`20261023T190202Z-5f97b611`. Slow suites and a full gate were not run; no physical-device
or rendered-pixel parity is implied.

## Migration of remaining CLI operations

The migration starts at master `8a824d45ea46f1ef5ae212ee04c389e32d4317eb`.
This section extends the existing inventory; the selected native transfer above
does not establish completion of all archive operations.

Source inspection on 2026-10-23 identifies the remaining production dispatch:

| Operation | Current owner and remaining boundary |
| --- | --- |
| Catalog discovery | `ArchiveCodecProbe` runs `i` to obtain the engine version and deployment libraries before native loading. |
| Listing and testing | `ArchiveBackend::startNativeRead` uses official Open/Extract with a catalog; no-catalog consumers still use CLI fallback and technical-listing parsing. |
| Extraction to a destination | `ArchiveBackend::extract` still constructs CLI passes for selection and overwrite handling. Source Agent/GUI extraction already has native callbacks and path policy. |
| Extraction of one source entry | With a catalog, the joined read worker now opens and extracts the selected native index into a private stream. The no-catalog CLI fallback remains. |
| Creation and public mutations | `archivebackend_creation.cpp` and `archivebackend_mutations.cpp` still dispatch creation stages, add, delete, rename and replace through the process transport. |
| Source updates | `archivebackend_sourceupdate.cpp` and `archivebackend_sourceplan.cpp` still dispatch file/add, mutation and staged plan operations. Native `UpdateArchive` and Agent update interfaces exist and need their callers connected. |

Direct consumers include file-only `QtArchiveFileResource` extraction/update,
source batch extraction/opening, standalone backend APIs and materialized nested
editing/publication. The retained native reader path must remain independent of
backend/window lifetime. Process completion owns continuations, temporary-file
cleanup and publication; migrating dispatch must migrate these responsibilities
together. Hashing and benchmark providers already use source/native workers.
Codec discovery, runtime-image ownership and no-catalog consumers must migrate
before removing the shared CLI transport.

### Native extraction of one source entry

Upstream reference: <https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`:
`CPP/7zip/UI/Client7z/Client7z.cpp` (`CArchiveExtractCallback::GetStream`,
`PrepareOperation`, `SetOperationResult` and the `IInArchive::Extract` caller),
`CPP/7zip/UI/Common/ArchiveExtractCallback.cpp` (item/block diagnostics and
stream-output branch), `OpenArchive.cpp` (`CArc::GetItem_Path`),
`CPP/7zip/UI/Agent/Agent.cpp` (`CAgentFolder::Extract`) and
`CPP/7zip/Common/FileStreams.cpp/.h` (`COutFileStream`).

The existing read worker retains original Open/Extract and callback error
projection. The source file-stream/Extract/Close ownership is adapted to a
single private spool; the stream is returned only for the selected index in
extract mode, including when solid decoding requests skipped siblings. The
source caller retains final filesystem presentation and metadata handling.
The full Client7z filesystem extractor, console UI, directory construction,
attributes and overwrite policy are omitted because this API writes one
already selected private spool rather than a destination tree. Those features
remain in the original source extraction workflow.

The Qt boundary maps its exact name/ADS request to an actual native index;
the Console wildcard/censor interface is unsuitable for this literal selection
contract. Ambiguous duplicate names are rejected rather than concatenated;
index-based retained-reader extraction remains available for those entries.
Native `Create_NEW` protects an existing or raced output. Output created by the
operation is closed before completion and removed on cancellation or failure;
recovery-format item errors can retain unverified bytes with `S_FALSE` and the
decoder operation result or typed opening diagnostic. No text label is decoded to infer native CRC
or password failures, and partial output is never reported as complete success.

Source comparison covers the selected callback, stream, opener and caller scope;
format decoders and metadata/overwrite algorithms were not reimplemented.

Validation also exposed a retained-runtime dependency gap: after restoring the
pinned 26.04 header, Linux's older `DllExports2.o` still advertised 26.05 while
the console advertised 26.04. Its upstream make rule omits `C/7zVersion.h`.
The existing supplemental header-dependency generator now includes original
module-version consumers as well as adapted-header consumers. A regression
changes the upstream version baseline and executes the rebuilt consumer in the
same build tree. This repairs dependency tracking without clearing build outputs
or compiler caches, or forcing a cold build. The affected failed receipt is
`20261023T192213Z-50356b84`; its Windows selected set passed.

Runtime acceptance on 2026-10-23: macOS passed `native_source_extract`,
`native_listing`, `native_archive_test`, `source_extract_backend` and
`source_extract_contract` (4.78 s), plus `upstream_baseline` (7.30 s) and
`runtime_cache` (0.22 s). Linux and Windows passed the same seven-suite set in
`20261023T192554Z-cb0e1396`, with Windows running in its desktop session. The new
extraction suite passes 23 cases including initialization/cleanup, with no skips;
its backend has an unavailable CLI executable and a loaded official module.
Cases cover bytes for ZIP/7z/gzip/bzip2/xz, literal Unicode/space/wildcard/newline
names, duplicate rejection, solid siblings, decoder CRC and opening warnings,
passwords during both opening and decoding, cancellation/join, retry and
destruction while waiting for a password. Existing source consumers and native
listing/testing remain covered. Incremental build trees and compiler caches were
retained; disposable fixtures clean themselves up. No slow suite or full gate
was run. This accepts the selected native extraction path, not completion of
the remaining CLI operations listed above.
