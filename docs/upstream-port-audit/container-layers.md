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
the official runtime retains its original console module. No-catalog read
consumers now acquire native providers as recorded below. Materialized nested editing remains enabled;
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
| Catalog discovery | Native `LoadDll`/`LoadExternalModules`/`SetLibCodecs` supply providers and version metadata without executing the console. Deployment images retain provider lifetimes; optional console copying remains for unmigrated consumers. |
| Listing and testing | `ArchiveBackend::startNativeRead` uses official Open/Extract; its joined worker acquires native providers when no catalog was supplied. CLI dispatch and technical-listing parsers have been removed. |
| Extraction to a destination | `ArchiveBackend::extract` runs original `DecompressArchive` and `CArchiveExtractCallback` in the joined native worker. Original selection and overwrite replace the CLI passes; effective native selection is revalidated before writes. |
| Extraction of one source entry | The joined read worker acquires native providers as needed and extracts the selected native index into a private stream. CLI dispatch, text error decoding and console selection files have been removed. |
| Creation and public mutations | `archivebackend_creation.cpp` and `archivebackend_mutations.cpp` still dispatch creation stages, add, delete, rename and replace through the process transport. |
| Source updates | `archivebackend_sourceupdate.cpp` and `archivebackend_sourceplan.cpp` still dispatch file/add, mutation and staged plan operations. Native `UpdateArchive` and Agent update interfaces exist and need their callers connected. |

Direct consumers include file-only `QtArchiveFileResource` extraction/update,
source batch extraction/opening, standalone backend APIs and materialized nested
editing/publication. The retained native reader path must remain independent of
backend/window lifetime. Process completion owns continuations, temporary-file
cleanup and publication; migrating dispatch must migrate these responsibilities
together. Hashing and benchmark providers already use source/native workers.
The remaining write operations must migrate before
removing the shared CLI transport. Native discovery and retained runtime-image ownership
are now independent of console availability.

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

### Native catalog discovery

Starting at `cfb18b78`, discovery no longer launches `7zz i` or parses its banner
and library listing. Source reference: <https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/UI/Common/LoadCodecs.cpp/.h`: complete `CCodecs::Load`, `LoadDll`,
`IsSupportedDll`, `LoadDllsFromFolder`, provider registration, `SetLibCodecs`,
`CloseLibs` and their version/property and registry dependencies were compared
with the existing port before the change and reviewed again afterward.
The existing mixed-revision baselines remain authoritative; this comparison
does not relabel the whole adapted module as a new upstream revision.

Original native module ABI/version queries, directory enumeration, provider
registration and diagnostics remain the functional owners. The original
`GetBaseFolderPrefixFromRegistry` body is exposed through a narrow overload so
the Qt-selected deployment can supply its starting directory. The zero-argument
source caller retains its original starting point. The Qt adapter resolves its
deployment paths and schedules loading; it does not replace the Windows
registry search or infer codecs from console text. Module-only explicit loading
retains its version-check contract. Deployment loading accepts usable external
formats when the main module is missing and preserves the nonfatal diagnostic.

Immutable module/plugin assets, original-path diagnostics, joined cancellation,
deadlines and retained generations remain in the existing owners. Native DLL
entrypoints cannot be safely interrupted; cancellation discards their result
after the joined load returns. An optional console copy is retained solely for
the remaining process consumers, without making native discovery depend on it.
Console information output and its text transport are omitted from discovery.

The affected native listing also now uses original `GetFormatNamePtr` like
Console `List.cpp`, preserving the parser (`#`) type when no format index exists.
Existing parser behavior coverage detects this missing projection. Opening tests
now verify extracted stream bytes after caller/manager release and retained
catalog extraction after failed replacement; obsolete spool-presence and
process-listing hooks no longer describe those native paths.

Focused acceptance covers discovery without a console (including a hanging or
wrong-version console), module/provider warnings and failed replacement,
plugin-only deployments, Unicode/space names, native list/test/source extraction,
deadline/cancel/retry/destruction, opening and retained-layer consumers.
Source comparison establishes the selected loader/adapter scope separately
from runtime checks; ordinary destination extraction, writes and no-catalog
fallbacks remain migration work.

Runtime acceptance on 2026-10-23 selected `codec_probe`, `codec_catalog`,
`external_codecs`, `native_listing`, `native_archive_test`,
`native_source_extract`, `agent_opening`, `native_folder_opening`,
`archive_layer_lifetime`, `associated_window` and `codec_capabilities`.
macOS passed the eleven-suite set (11.61 s), the expanded plugin-only external
codec cases (4.56 s), and the affected probe/opening rechecks after fixture
isolation. Initial Linux/Windows receipt `20261023T194744Z-58670aea` passed nine
suites and exposed two test fixtures accidentally discovering sibling build
plugins. The slow modules now live in private temporary deployments. Linux
passed both affected suites in `20261023T195103Z-e9baeeef` (9.65 s).
Windows passed both in desktop receipt `20261023T195213Z-954bf7bf`.
These runner checks cover DLL/so loading and the Windows registry boundary. Existing
incremental trees and compiler caches were retained. Private fixtures clean up
automatically; foreign runner artifacts were preserved. Host storage remained
healthy, with no VHDX growth. No slow suite, full gate or visual check was run.

### Native reads without a prepared catalog

Baseline `da0aae61`. Recompared original `LoadGlobalCodecs`/`CAgent::Open` and
`BindToRootFolder`, `ArchiveFolderOpen.cpp` factory/binding, `CCodecs::Load`,
and Console `Main.cpp` load/list/test/extract dispatch at upstream
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`
(<https://github.com/ip7z/7zip.git>), with current catalog, read worker,
folder manager, file resource, reopen and source extraction consumers.
Original Open/Extract, property enumeration, proxy binding and codec loading
remain authoritative; no decoder, format parser or new detection algorithm
was introduced. Existing source module baselines and licenses are unchanged.

The shared native discovery body can now run inside the joined read worker.
Standalone listing, testing and single-entry extraction always use that worker,
including when no catalog was supplied. It checks pause/cancel around native
loading and retains the immutable image until loading and opening have joined.
Successful provider acquisition is published before result receivers can start
another operation; cancelled acquisition is discarded. An explicit failed
catalog is not silently replaced. Provider warnings and archive damage remain
separate diagnostics. Automatic source extraction classifies recovery using
the opened native format, preserving closed owned output and typed damage
results without requiring a prior listing or an explicit type string.

Agent opening requests discovery before selecting its direct stream or CopyTo
boundary, preserving native streams and reader generations for callers that did
not preload providers. The existing typed backend metadata bridge remains for
alternate service implementations; ArchiveBackend's own reads always load the
official native engine. Discovery is an overridable backend command like the
other asynchronous operation commands, allowing alternate metadata services to
own their provider availability without restoring console reads.

Removed read CLI argument construction, technical-listing/header parsers,
printed-error classification, redirected source output, exact-selection list
files and their completion/cleanup branches. The process transport remains for
destination extraction and writes. Its progress/error/password handling still
has direct write consumers and is not removed in this iteration.

Affected regressions use archives produced by official writers for literal
unsafe names, Unicode/space names, alternate streams, passwords (including an
empty defined password), lifecycle and cancellation. Process regressions target
the remaining write transport. Binding cancellation uses source item-progress
callbacks rather than a listing hook. Native rollback/retry and nested writer
failure/cancel use the existing delegating format fixture; property-fault
selection is extended to required opening properties. Reader-only fixtures
replace the obsolete assumption that TAR cannot update. The full interarchive
matrix is opt-in after its old process hooks produced a 120 s timeout; the
default native subset covers reopen failure, cancellation and failed restore.

Encrypted nested editing exposed a genuine native reopen adapter gap:
`COpenCallbackImp` intentionally omits `IProgress` from QueryInterface. The Qt
adapter's failed progress query also discarded its crypto service, producing
`E_NOINTERFACE` after a protected update despite a correctly retained password.
Compared complete original `CArchiveLink::ReOpen` and `COpenCallbackImp`
progress/crypto delegation and the File Manager update/reopen callers at the
revision above. `CNativeOpenProgress` now accepts the existing archive-open
callback directly and delegates its file/byte progress, password and fscrypt-key
requests through the original interfaces. The source QueryInterface contract
and original callback bodies remain unchanged; no new password store or prompt
policy is added. Callback lifetime is still scoped to the joined opening.

Final macOS acceptance on 2026-10-23: the application and selected targets built;
26 suites passed together (45.16 s): backend, mutation, opening, progress,
native source extraction/listing/testing, Agent opening/lifecycle, source extract
backend/contract, probe/catalog/external codecs/capabilities, native folder
opening, retained layers, open options, nested navigation/editing/update,
device folders, archive recovery, native parent publication, associated windows
and the focused interarchive recovery subset. The nested editor suite passes
15 cases, including separate protected inner/outer passwords and actual native
writer failure/cancel. Reference extraction uses UTF-8 include files for its
canonically unique fixture names so QProcess filesystem argument normalization
does not misread native NFC renames; production selection remains literal.

This iteration changes shared Qt scheduling and interface adaptation, without
changing DLL/so search/loading or OS-specific services, so validation is macOS
only under the user's runner rule. Linux/Windows were not rerun and are not
accepted for this revision by earlier receipts. The full interarchive diagnostic
hit its old 120 s cap before its obsolete hooks were replaced; only the native
subset is accepted. No opt-in exhaustive matrix, full gate or visual comparison
was run. Incremental builds and caches were preserved; obsolete task-owned
timeout fixtures and the temporary stack sample were removed after diagnosis.

### Native extraction to a destination

Baseline `fd7d5229`; source review on 2026-10-24 against
<https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`:
`CPP/7zip/UI/Common/Extract.cpp/.h` (`Extract`, `DecompressArchive`),
`ArchiveExtractCallback.cpp/.h` (initialization, selection, paths, overwrite,
output/link metadata, item/block errors, memory requests and closing),
`IFileExtractCallback.h`, `Common/Wildcard.cpp/.h`,
`CPP/7zip/UI/Agent/Agent.cpp` (`CAgentFolder::Extract`) and their
GUI/ExtractGUI and FileManager callback consumers. Rechecked complete helper
and callback paths after integration. Existing source baselines, modules,
notices and decoder implementations are retained.

`DecompressArchive` is now callable for an already opened reader. Its native
selection, AUX/alternate-stream rules, callback initialization and close/result
sequence are preserved. A narrow optional extraction service sends its final
indices through ArchiveBackend's existing serialized native extraction boundary;
the source batch caller keeps its original direct invocation. Optional source
path parts provide Agent-style CurrentPaths/ADS-folder prefix removal. Original
outer-file mtime fallback is applied before extraction. No selection, overwrite,
rename, metadata or archive algorithm is independently implemented in Qt.

The destination adapter supplies typed progress, password, overwrite and
item/block/filesystem diagnostics in the existing joined worker. Overwrite
answers wait off the GUI thread and cancellation wakes that wait. Missing
decision consumers fail without replacing the existing output. The existing
validation body is shared with the worker and checks the actual selected native
indices, including descendants of a selected directory, before extraction writes.
The existing bounded-path policy remains attached to ArchiveExtractCallback;
the destination is checked again after an overwrite decision in case the GUI
wait allowed a link/path substitution. Backend source-snapshot requirements and
the source-workflow-only advanced options remain unchanged.

Removed the independent preliminary overwrite walk, its skip/overwrite/rename
CLI passes, extraction process continuations and extraction-specific password
state. All production archive reads/extraction now avoid console dispatch.
Creation, public mutations and staged source writes remain pending; this does
not complete the all-CLI migration goal.

Observable source alignment: CurrentPaths now actually removes the selected
archive-folder prefix (the prior CLI backend silently used full paths). Original
overwrite happens during decoding, so a ZIP data-password failure can leave an
unverified output after overwrite approval; no preliminary decode/replay pass
has been retained. Item/data/open errors remain failures, with their native
diagnostics. Tests cover failure separately from a successful overwrite retry.

Focused macOS validation: app and nine affected targets built; `extraction_options`,
`archive_backend`, `native_source_extract`, `native_archive_test`, `native_listing`,
`source_extract_backend`, `source_extract_contract`, `source_extract_advanced`
and `native_extract_callback` passed (11.91 s). After the final path-prefix
normalization adjustment, the app and affected targets rebuilt and
`extraction_options`/`archive_backend` passed again (42.51 s).
Reference-fixture commands used
the prepared official runtime on PATH. Coverage includes 7z/ZIP passwords,
overwrite choices/all/rename/no consumer, pause/cancel ownership, selected
Unicode/space names, destination substitution, unsafe descendants, CPIO dot paths,
source batch extraction, metadata and existing advanced-source options.
Obsolete fake-console fixtures and launch-count assertions were replaced or
removed while preserving output/error assertions. CPIO's old module-only row
incorrectly expected a GPL format provider inside the SDK module; it now uses
native deployment discovery with that provider.

This changes portable Qt/callback integration, not platform services or native
loading. Under the user's runner rule Linux/Windows were not rerun; native NTFS
security/ADS cases remain unverified for this iteration. No slow suite, full gate
or visual acceptance was run. Incremental build trees/caches were retained and
diagnostic logging/temporary fixtures removed.

### Return to standalone 7zG operations: source boundary review

On 2026-10-24 the requested direction changed from migrating every backend CLI
operation to restoring the upstream File Manager/7zG division. Completed native
reads and destination extraction remain the engine foundation. The new scope
is standalone test, extract, create/update, checksum and benchmark; live archive
folders, device inputs and parent-update chains retain their direct source path.
This is a requested boundary, not a claim that process isolation is implemented.

Reviewed against <https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`: Common `CompressCall.cpp`
(`Call7zGui`, `CreateMap`, archive grouping/test/extract/create/hash/benchmark),
`CompressCall2.cpp` in-process counterparts, FileManager `CPanel::TestArchives`,
GUI `Main2` and exception/exit policy, `ExtractGUI`/`CThreadExtracting`,
`UpdateGUI`/`CThreadUpdating` and their callbacks/services. Original source module
boundaries and the existing baselines remain authoritative.

The current `main.cpp` selects the GUI entry by the `7zG` executable name;
`qtguimain.cpp` owns a separate QApplication workflow, native discovery and
QtShellArchiveServices without a manager window. It disables quit-on-last-modal
to preserve sequential source dialogs. CMake creates the GUI alias next to the
application; packaging creates Unix aliases and signs the macOS bundle. This
existing entry can be reused; a lighter executable is not a prerequisite.

For test, GUI `Main2` retains source enumeration, native types, callbacks,
ExtractGUI and exit-code handling. QtShellArchiveServices passes to original
ExtractGUI with a retained native opener. Its selected read path does not need
another engine migration. GUI update likewise reaches the existing native
`updateSourceArchive` overload and original UpdateArchive, rather than the
separate legacy staged CLI mutation API. These are distinct consumers; do not
route the GUI back through the legacy process API during extraction into 7zG.

The manager's filesystem Test still calls the typed in-process CompressCall2
workflow through `runArchiveBatchCommand`; its modal scope holds manager state.
The source archive-folder branch instead uses CopyTo/testMode over live indices
and must remain direct. The existing virtual context-menu Test operation is a
dispatch boundary, but the panel currently calls the global typed overload.
This is the first consumer seam to connect to the standalone launcher.

The upstream Windows sender owns a UTF-16 selection mapping/event and waits for
mapping acknowledgement or process exit. That proves source selection transfer
and startup ownership, not the requested portable versioned channel or a final
structured result. The current GUI dispatcher returns source process exit codes;
it has no manager-job ACK/result channel. Its public shell CLI and mapping parser
must remain compatible. Structured jobs should enter the same source dispatcher,
not recreate test/extract/update workflows or parse progress-dialog text.

Source-visible work before accepting the first integrated Test scenario:
versioned local job/ACK/result transport, native child launch/observation whose
owner does not terminate accepted work when the manager closes, separate launch,
cancel, warning/error and crash handling, and manager consumer/result binding.
Original Windows CProcess closes its handle without killing the child; the
portable adapter must preserve that lifetime while observing exit. A socket
disconnect alone cannot prove that a child crashed and must not cancel its work.
No corresponding portable process/result implementation exists in the reviewed
upstream modules; this is the narrow Qt/OS service adaptation to implement.

Baseline runtime evidence: the current standalone GUI entry builds and its
existing create/test/extract/excluded-format/broken-input scenarios pass on macOS.
The regression is extended to run actual 7zG tests against a module-only private
deployment and an explicitly unavailable console, for healthy and broken ZIP
inputs with Unicode, spaces and quotes. Existing tests used an available console
and could not detect a regression that wrapped test operations in 7zz again.
This establishes the reusable native entry, not manager IPC/lifetime acceptance
or installed/cross-platform packaging. No runtime/OS implementation changed in
this review. `unix_gui_entry_tests` and its app dependency built incrementally;
the expanded `unix_gui_entry` suite passed on macOS (5.61 s), with the prepared
official runtime on PATH for reference-fixture creation. Private deployments
cleaned up automatically and the task-owned registry is empty. Runners, slow
suites and a full gate were not run.
