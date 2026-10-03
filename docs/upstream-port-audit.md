# Ревизия происхождения кода 7zFM

Здесь хранится реестр происхождения кода: upstream-ревизии и исходники,
перенесённые символы, адаптации, пропущенные части и долг переноса.
Правила работы заданы в [AGENTS.md](../AGENTS.md); наблюдаемое поведение
и платформенные отличия — в [сверке с 7zFM](7zfm-parity.md).
Владельцы компонентов и интеграционные границы описаны в
[архитектуре](architecture.md). Эти документы ссылаются на реестр,
не дублируя сведения о происхождении и историю переноса.

## P2-07 — extraction integration (accepted 2026-10-03)

Whole iteration accepted: source extraction controls/persistence, bounded final
paths/links, native security/ADS/reparse restoration, metadata Add and processed
deletion, actual MainWindow consumers and directory detached startup. Full macOS
formatting/all-target/214 CTest passed81.10s; final affected4/4PASS1.45s and
lifecycle1/1PASS1.28s. Windows13/13PASS33.56s plus final lifecycle1/1PASS2.48s;
Linux final13/13PASS10.15s. Current EN/RU normal/compact Extract/Metadata JSON,
including four native WIM Options reports, reviewed. Baseline382 exact/175
adapted/3 local verified. Full Windows/Linux gates, rendered Windows reference
and Finder remain P3 obligations. Historical WIP checkpoints below are superseded
by this acceptance. Baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
`https://github.com/ip7z/7zip.git`. GUI `ExtractDialog.cpp/.h` restores original
GetBoolsVal/CheckButton_TwoBools/GetButton_Bools, ElimDup/NtSecurity and the third
path mode; Qt substitutes control reads/writes. Common ZipRegistry restores
defined-value persistence under original ElimDup/Security keys through QSettings.
ExtractGUI restores dialog→options metadata flow. Common Extract::DecompressArchive
remains the authoritative root-elimination algorithm, not a Qt replica.

Absolute extraction is bounded by the chosen destination. Raw traversal,
case/Unicode collisions and occupied link ancestors remain refused. IQtExtractPathPolicy in
CExtractNtOptions receives the source-corrected path from
ArchiveExtractCallback::GetExtractStream before folder creation; CreateFolders
moves below the check, with its body preserved. Qt validates containment and
filesystem ancestors, reporting a localized reason through source MessageError.
Batch-open validates each actual listing before its source extraction loop.
These checks are not an atomic defense against concurrent ancestor replacement.

Native link extraction retains original CLinkInfo normalization/rebasing,
SetLink/SetLink2 deferred creation, CLinkLevelsInfo safety and SetPostLinks
metadata flow. The same borrowed policy now checks the final origin and target
in CreateHardLink2 and the non-hard SetLink2 branch immediately before creation.
Qt only resolves a relative final target and validates containment/ancestors;
it does not implement TAR link parsing or upstream link-safety algorithms.
MainWindow and batch preflight permit archive links only after a matching native
service has been proved. A link remains a leaf for archive path conflicts,
including directory links; file-only transport continues to refuse links.
For Windows native ADS, preflight permits a separately validated base and stream
name only with a proven native service and the listing's Alternate Stream flag.
The final callback policy receives original CReadArcItem::IsAltStream alongside
the source-corrected path. ArchiveBackend validates the bounded base and its
occupied ancestors independently, including base links; ordinary colon paths
remain refused. Original CorrectPathParts/Correct_AltStream_Name, parent rename
mapping and stream creation remain unchanged. Whole native WIM extraction of
Unicode/spaced base and stream bytes passed Windows; the regression failed
before this integration adaptation. Original NtSecurity restoration passes a
Windows protected explicit DACL fixture with the option on/off; archive and
source rights remain intact. Native Add security-only passes as well. Adding
the first ADS exposed a stale CArc::Ask_AltStream after publication:
the source CProxyArc2 rejects its file parent when this old flag is false.
CAgent::ReOpen now refreshes ConfigureArc from the fresh native handler before
unchanged ReadItems. Windows ten focused suites pass (26.27s), including
independent security/ADS on/off, failure/Cancel preservation and processed-source
deletion after native publication.

Qt extraction destinations now use QDir::toNativeSeparators when entering batch
OutputDir, GUI dialog OutputDir and MainWindow CopyTo.folder. Source GetFullPath
preserves already absolute strings, and FillLinkData_WinLink embeds its input in
NT reparse data; forward slashes from Qt produced a visually resolved but unreadable
absolute target on Windows. Native WIM relative and absolute file/directory link
byte reads now pass without changing source link construction. Source Metadata
Options WIM EN/RU transaction/persistence/compact geometry passed Windows.
Last-access-time captions wrap and Qt vertical gaps are explicit, with source
order retained. Four current WIM JSON reports reviewed; RU compact dialog is
480×430 with475×423 size hint, metadata labels and footer fully exposed.
POSIX source_link_extract covers file/directory/dangling/hard links, common root
elimination, original absolute-link rebasing, unsafe targets, occupied ancestors
and overwrite Cancel with partial writes preserved. Windows native fixtures
pass, including WIM reparse links. Full macOS format/build/214 CTest passed
(81.10s); subsequent affected checks pass after localized Windows/Options fixes.
Current focused Windows11/11PASS40.19s and Linux13/13PASS10.51s do not establish
full platform or rendered Windows acceptance. Final expanded Windows checks
passed13/13 (33.56s), with final lifecycle1/1 (2.48s); Linux13/13 (10.15s).

QtArchiveFileResource proves native count/path order for any retained native
reader, then routes original IInArchive::Extract through ArchiveBackend. Levels
that cannot prove order retain the file service; RAR/hash mismatch still refuses
fallback. Hash keeps its safe-read guard. Source Agent Extract receives borrowed
CExtractNtOptions for selected-copy metadata/path checks while the modal worker
is joined. Whole-root extraction uses original ExtractGUI/Common Extract for
every overwrite mode. Selected/current-folder extraction preserves Agent source
semantics; ElimDup is disabled in that dialog context because Agent does not
perform Common root elimination.

Related BrowseDialog2 directory Open Outside : 7-Zip uses existing navigation.
Main.cpp supplies the launch path to openPath, resolving it against launch cwd
before native navigation. FM.cpp's active g_MainPath startup and BrowseDialog2's
original command are the reference; QApplication arguments replace Win32 parsing.
Detached launch retains unconfirmed external owned-temp protection.

The folder/process-plan bridge projects defined IsDir through original
Archive_IsItem_Dir, preserving its default/type rules for native handlers such
as gzip that omit this optional property. Other native fields remain authoritative;
missing values are not synthesized. This fixed the expanded gzip Add regression.

Standalone Windows ZIP/7z creation now refuses SymLinks before engine start.
The pinned `CPP/7zip/Archive/Zip/ZipRegister.cpp` and
`CPP/7zip/Archive/7z/7zRegister.cpp` do not advertise `kSymLinks`; the native
catalog follows those original capabilities. A real Windows standalone fixture
proved that file/dangling links survive CLI storage but a directory reparse link
becomes an ordinary directory, followed by deletion of the input link. This
integration guard prevents that loss; it does not assert all CLI links are
unsupported. POSIX standalone compatibility remains tested, and Windows native
TAR/WIM link storage uses source-advertised capabilities. The regression verifies
refusal, preserved raw links/target bytes and no output publication or engine start.

Creation now carries the original CompressDialog::CInfo SymLinks pair through
ArchiveCommandLine.cpp's kSymLinks/kMinus (`-snl`/`-snl-`) into the external official
engine. Catalog-backed creation requires the original Flags_SymLinks capability.
The pre-publication deletion snapshot uses Common/Update.cpp's CCensor plus
EnumerateItems sequence, preserving original leaf reparse-data handling (the
Agent EnumerateItems2 sequence omits it). QtArchiveDeleteInputs retains the
same SymLinks pair for the unchanged source UpdateArchive deletion body.
Links remain refused for deletion when storage was not requested. File-only
source-update staging explicitly refuses SymLinks because it cannot carry
reparse properties. Metadata Add now uses a native writer; Windows runtime
checks pass as recorded above.

CompressDialog restores original Combine_Two_BoolPairs/Set_Final_BoolPairs
bodies and four Info pairs (SymLinks/HardLinks/AltStreams/NtSecurity). Narrow
LoadMetadataOptions/SaveMetadataOptions entry points replace FormatChanged/OnOK
orchestration. Source CArcInfoEx flags determine support; original Compression
Security/AltStreams/HardLinks/SymLinks persistence uses QSettings. The Options
modal is a transaction; Add OK saves it. Unsupported requests clear their
command pairs while registry preferences survive format changes. Unix hides
NTFS streams/security.

AgentOut keeps its source update plan and callback bodies. Integration assigns
Common/Update.cpp's DirItems SymLinks/ReadSecure/ScanAltStreams and callback
StoreSymLinks/StoreHardLinks/StoreNtSecurity fields. Link-aware enumeration uses
source CCensor/EnumerateItems, including selected leaves; non-GUI relative names
retain their physical folder prefix. After successful writing, the existing
AgentOut processed-item loop additionally includes AreReparseData, matching
Common/Update.cpp::Compress's post-pass. TAR can write link properties without
requesting a stream, so this adaptation is necessary for original deletion.
Snapshot/pairs survive until publication and source no-follow deletion.

MainWindow scoped extraction now derives preflight names from the same source
Agent real indices used by CopyTo. Previously extracting a current folder with
an empty selection validated the entire archive listing and rejected unrelated
root entries. The existing GetRealIndices/property projection is reused; source
selection and extraction bodies are unchanged. The focused metadata UI suite
exercises actual Add/Options/native TAR update/processed deletion, selected
folder extraction and current-folder extraction in EN/RU, plus Add Cancel.
Windows/Linux native link fixtures use original FillLinkData/SetReparseData and
FileIO file identity, replacing Windows-wide test skips. Unix uses source
SetSymLink_UString and inode identity. Windows focused nine suites passed
(43.11s), including actual MainWindow metadata Add/Extract, native link storage,
cycle/dangling/directory links and source update consumers. This is not a full
Windows gate or whole-step acceptance; the later security and whole-archive ADS
checks recorded above supersede that checkpoint's open consumers.

At the Windows source-to-process mutation boundary, ReadPath/ReadNewPath now
call original NArchive::NItemName::ReplaceSlashes_OsToUnix. The source Sync
callback correctly retained the child while deleting parent directory records;
unnormalized native separators defeated FindMutationPlan's descendant guard,
causing recursive CLI deletion. Regression uses a real opened native archive,
observes original DeleteOperation, and verifies preserved child bytes for ZIP/7z.
QtPanelFolder projects transport paths with original GetOsPath/GetOsPath_Remove_TailSlash;
custom presentation names retain their own representation.

Options wraps PreserveATime and time-precision captions to preserve source
control order at compact Windows sizes. Add geometry settles the scroll layout
before inspecting the complete Parameters field; QLineEdit cursor visibility
alone does not prove whole-field exposure. Functional source option bodies are
unchanged. No rendered Windows reference has been compared.

Runner compilation prerequisites found by the current full snapshots: Qt6.4
has no QTimeZone::UTC enum, so fixture constructors use equivalent QTimeZone(0).
A Windows MOC/compiler mismatch in filesystem_cleanup is fixed by retaining
the slot on both platforms and guarding its POSIX-only body. Process FileName
loads QueryFullProcessImageNameW dynamically via original Z7_GET_PROC_ADDRESS,
following PanelItemOpen's loader pattern without raising inherited SDK targets;
this Qt service retains DOS-path output for file comparison. Upstream's PSAPI
helper returns device paths and cannot directly provide that contract.

ArchiveBackend shares its validated source CProperty projection between CLI
and native transports. Metadata Add calls original SetProperties then
IOutArchive::UpdateItems with the retained catalog's detected format/capabilities,
not an open-type hint or a later catalog. Native GetFileTimeType supplies source
update comparisons. File-only staging refuses unrepresentable metadata.
ZIP/7z Register.cpp omit kSymLinks despite CLI link support; GUI/native choices
preserve that distinction. TAR is the verified POSIX metadata consumer.

source_link_storage covers TAR Add/link roundtrip, target preservation,
publication then processed-input deletion, native failure/cancel preserving
archive/input bytes, hard-link inode identity, ZIP/7z capability refusal,
Options Cancel/OK, restart/format filtering and EN/RU540×480/480×430 geometry.
Tests caught the missing reparse post-pass and incorrect assumptions about
reopen listing counts and QLabel-only geometry helpers. Final focused six
suites PASS6.02s. Full unsandboxed format/all-target build/211 CTest PASS81.52s.
After the localized native-phase progress fix, four direct suites PASS2.55s:
source_link_storage, codec_update, native_comment, source_extract_advanced.
The metadata transport detects its native phase from retained settings because
PanelCopy uses the default progress-service call; source Sync owns Pause/Resume.
The fixture verifies idle backend/unchanged archive during pause before writing.

Latest macOS checks: extraction17/17 PASS7.67s; creation/direct-consumer6/6
PASS4.46s; final layout/split/TAR3/3 PASS1.00s. Full unsandboxed check.sh passes
formatting/all-target build/210 CTest81.51s after correcting legacy mode-count,
basic-CLI option and modal-completion test premises. Baseline382/175/3 verified;
four EN/RU700×390/600×340 Extract JSON reports inspected. Windows/Linux
native security/metadata, links/streams extraction, archive link storage and
processed-input deletion, Windows source-update failures, final UX/full gate
and acceptance remain mandatory. No whole-step commit yet. P2-06 below describes historical acceptance.

## P2-06 — source ADS/streams acceptance (2026-10-03)

Baseline for these modules is
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
`https://github.com/ip7z/7zip.git`. Complete original
`CPP/7zip/UI/FileManager/AltStreamsFolder.cpp/.h` is copied under `src/`:
enumeration, stream properties, binding, cloning, comparison, notification,
CreateFile, Rename, Delete, CopyTo and Windows FileIO/FileFind calls retain
their original bodies. Includes, formatting, sort overload and copy diagnostic
types adapt to the existing authoritative portability owners. Original
CreateFolder, SetProperty and active CopyFrom/CopyFromFile return E_NOTIMPL;
commented upstream import implementations remain commented, not invented.
The module is built only on Windows; Unix xattrs are not NTFS ADS.

`FSFolderCopy.cpp::CCopyStateIO::MyCopyFile(CFSTR,CFSTR,DWORD)` restores the
source 64 KiB CByteArr/native FileIO loop, original HRESULT/IProgress and source
deletion. POSIX uses the original FileIO ReadFull/write_full interfaces.
Create_NEW replaces upstream Create_ALWAYS for exclusive owned stages.
INVALID_FILE_ATTRIBUTES suppresses ADS base attribute/time changes. Ordinary
file metadata and link checks retain the previously accepted transaction
adaptation. The QString overload only supplies native paths/attributes and the
existing Qt cancellation predicate. One authoritative PanelSort remains in
sevenzip_panel; duplicate per-consumer source registration is removed.

`RootFolder.cpp` restores the source colon-tail namespace and
`PanelFolderChange.cpp::OpenAltStreams` retains its interface/bind/fallback flow.
The Win filesystem fallback is disabled on Unix. Qt only dispatches the source
command, renders folder names/properties/indices and refreshes the namespace.
Filesystem ADS Copy/Move/Delete/CreateFile/Rename use existing CPanel workers;
Properties uses the existing source dialog. CDisableNotify prevents stale row
callbacks during Move reload. Native item paths select newly created/renamed
streams; the ADS label editor uses the source stream name.

`Agent.cpp` restores the original CProxyArc2 branches in LoadFolder/LoadItems,
Init/count/name/prefix/full-prefix/properties, folder/parent/ADS binding and real
index expansion. Existing AgentProxy.cpp/.h remain the sole proxy algorithms.
The Qt GetItem wrapper preserves original flat/non-flat index mapping; invalid
public indices and unbound ownership are guarded. ReadItems retains original
GetRawProps+IsTree selection and source RAM checks; callback-null load progress
remains P2-10. ReOpen copies the active Restore FolderItem body from
ArchiveFolderOut.cpp::CommonUpdateOperation, applying it to the newly opened
owner and preserving the source root-alt/file-alt sentinel semantics.

QtArchiveFileResource/Properties retain native tree flags, raw parents and item
properties only after exact count/path-order proof. Arbitrary native binding
was rejected because TGZ levels differ; tree, ZIP-comment, RAR and hash readers
keep their explicit supported boundaries. Qt rows consume the folder interface,
not a CProxyArc-only pointer. Backend listing adds original Console `-sns` so
archive ADS are present for order proof. The CurrentPaths validation adapter
admits only declared non-link ADS in the matching colon folder and validates
the plain output stream name, retaining collision/link/unsafe-base guards.
Only separators are normalized for selected/current/native paths; raw metadata
and diagnostic names stay intact. Console password recognition accepts both
original UserInputUtils.cpp prompt variants, including Windows echo suppression.

The file-only Console extraction adapter carries native `kpidIsAltStream` into
an explicit `-sns` switch only for stream items. Unix otherwise skips ADS even
with stdout extraction. The original callback owns output naming; ordinary
payloads retain stream exclusion. A synthetic Windows WIM fixture (provenance
in tests/fixtures/streams/README.md) now checks actual ADS on Unix as well.
On macOS, archive names whose UTF-8 differs from QFile::encodeName are passed
in a private UTF-8 selection list instead of argv: QProcess filesystem encoding
decomposes names, which must not change archive matching. The list has an owned
temporary lease through process completion. Names containing line breaks cannot
use this list transport and receive an explicit error; native extraction remains
required for that combination. This is a process boundary adaptation, not a
change to source comparison or normalization rules.
The Qt boundary is documented by
[QFile::encodeName](https://doc.qt.io/qt-6/qfile.html#encodeName) and the pinned
[Qt 6.11.2 QProcess argument conversion](https://github.com/qt/qtbase/blob/v6.11.2/src/corelib/io/qprocess_unix.cpp).
Use `-i@list` before `--`: positional `@list` after `--` is treated literally.

Native tree output is connected through ArchiveBackend to the original
handler IOutArchive::UpdateItems and original Agent/ArchiveFolderOut callback
and transaction. This avoids flattening source parent/ADS indexes into a CLI
mutation plan. Portable actual WIM Rename/exact19byte extraction passes on
macOS, Linux and Windows. Windows actual ADS Rename/Delete Cancel/Yes retains
the source final-stream fallback and passes official archive Test/exact main
bytes. Final Windows archive suite1/1PASS6.60s Qt5/5zero skips after forcing
the transferred fixture's VM mtime: source SHA256 matched local, but older moc
had omitted the new slot. Final Linux nine suites PASS23.82s, all Qt cases zero
skipped. Sixteen EN/RU980×620/640×400 JSON reviewed across mac/Linux archive,
Windows archive and Windows filesystem ADS: address/table/status fully visible,
source grouping intact. Baseline382exact/175adapted/3local verified.
Final macOS arm64 Qt6.11.2 unsandboxed gate passed formatting, all-target build
and207/207CTest120.58s. Earlier failed checkpoints are superseded. Final VM
checks exposed a Qt integration bug in updateSourceAdd: canonical QFileInfo paths
use '/' on Windows too, so a native QDir::separator containment suffix rejected
prepared files. The narrow fix keeps the containment guard and source callbacks;
the actual archive-to-archive drag regression now checks added payload bytes.
Disposable drag rows isolate their ownership registries, retaining the original
unconfirmed-receiver protection assertions without leaving shared test records.
Final localized consumer checks and whole-step acceptance are recorded below.

P2-06 is accepted as a whole iteration. Final localized macOS drag/source-update
consumers2/2PASS4.43s; Linux drag/contract/source-update3/3PASS3.21s, zero Qt skips.
Windows final drag/contract2/2 passed (Qt9+4 zero skips); the seven relevant Add,
directory/multiple input, Move, Cancel, protected archive and unsafe-path cases
passed through the project CTest wrapper1/1PASS1.39s (Qt9 zero skips). Earlier
Windows14-suite regression passed12 suites61.23s and Linux17-suite passed16
45.60s; their drag/contract failures are superseded by these focused results.
The expanded Windows source_update_backend suite remains failing: slash-sensitive
listing assertions, synthetic Sync metadata yielding an empty archive, absolute
path expectations and a30s timeout. This is retained for P2-07/P3-02/P3-03;
no complete Windows gate or rendered Windows parity is claimed. Counts35/46;
nextP2-07. Finder remains the separate mandatory P3-03.1.

## P2-05 — source links/metadata acceptance (2026-10-03)

P2-05 is accepted as a whole iteration. Final unsandboxed macOS arm64Qt6.11.2
check.sh passed formatting, all-target build and205/205CTest82.95s. Final focused
Windows11 ARM64Qt6.8.3/MSVC14.44 seven suites PASS30.24s, all Qt cases zero
skipped; Linux ARM64Qt6.4.2/GCC13.3 seven suites PASS0.15s, zero skipped.
Twelve current EN/RU520×500/360×300 JSON across the three platforms reviewed;
all controls become exposed through compact scrolling and footer remains visible.
Baseline382exact/173adapted/3local verified. Counts34/46; nextP2-06. This is not
full P3 acceptance: native FinderP3-03.1, rendered/full platform checks remain.

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
`https://github.com/ip7z/7zip.git`. Complete original
`CPP/7zip/UI/FileManager/LinkDialog.cpp/.h` copied into mirrored source paths,
including CApp::Link, default link selection, GetSymLink reproduction validation,
path/type/nonempty-file checks, browse and original hard/reparse operations.
Qt replaces modal/combo/radio/control/message/resize services. Exact original
LinkDialog.rc/Res.h are copied unchanged. Win32 OnSize rectangles are replaced
by the original grouping/order in Qt layouts with a compact scroll pane.
English resource captions are retained by dynamic resource-language bindings;
Cancel uses the original standard402 mapping.

Existing adapted Windows/FileLink.cpp and FileDir.cpp already retain original
hardlink, reparse encoding/parsing/device-control and POSIX readlink/symlink/link
bodies. They are reused, not reimplemented. POSIX does not have Windows reparse
data: OnInit reads the original raw readlink target and classifies the target
only for the presentation's file/directory choice. A symlink inode itself is
exempt from the Windows stored-directory-attribute type check. Junction/WSL
radio controls are disabled outside Windows. Empty plain POSIX files are refused
instead of hiding their bytes behind a reparse attribute. Editing an existing
POSIX link creates a private same-parent link and uses the source rename adapter
atomically, preserving the old link on failure. Clearing it unlinks the POSIX
inode; Windows DeleteReparseData retains its original behavior.

Actual File menu dispatch for one operated filesystem item, selection restoration,
create/Cancel and multiple-selection/archive refusal are verified. Fixtures cover
raw relative/absolute/dangling/cyclic/directory links, Windows junction, hardlink
shared bytes and inode identity, retarget/clear, denied process privileges on
Windows and POSIX parent permissions, copy/move/delete/cancel and metadata.

FM.cpp's original `g_SymLink_Supported` and `Set_SymLink_Supported` are added
to its existing partial port, preserving WM_CLOSE/WM_COMMAND/IsLargePageSupported
bodies. Exact original `Windows/MemoryLock.cpp/.h` supply EnablePrivilege and
EnablePrivilege_SymLink, including the original process-token privilege checks;
Windows builds those files with consistent Unicode definitions. Qt calls this
source initialization once when creating the Link dialog. POSIX has symbolic
links without Windows privilege initialization. This does not port all FM
startup or enable the large-pages setting.

Integration retains source CFsFolderStat/CopyTo/
CopyFileSystemItems/CopyFolder control flow, replacing the previous blanket
link refusals with native IsOsSymLink leaf handling. This explicitly deviates
from upstream Windows recursive CopyFolder/CopyFileEx behavior to meet the
port's no-implicit-follow requirement: directory links/junctions do not recurse;
raw GetReparseData is cloned by source SetReparseData (Windows) or SetSymLink
(POSIX), then source SetLinkFileTime preserves link timestamps. POSIX raw target
bytes remain unchanged, including relative/non-UTF-8 targets. New owned-stage
platform glue is necessary because there is no upstream portable raw-link clone
operation. Same-filesystem Move uses original MyMoveFile/rename; transaction
fallback clones the object, publishes it, and deletes only the source link.
Destination links remain refused by the transaction boundary. Original Windows
FSFolder IArchiveGetRawProps bodies, Reparse member and LoadSubItems reparse/file
identity capture are restored; failed capture does not read uninitialized handle
information. POSIX reports no NT raw property. CalcItemFullSize skips link leaves.

Compact Windows EN/RU captions exceeded viewport minimum
width; existing languageWrapWidth binding now wraps radio captions at220 pixels.
Resource text/order retained; geometry is structural/usable proof, not rendered
7zFM pixel parity. Failed target-open assertions account for different platform
QFileInfo dangling-exists semantics. Windows EN/RU520/360 JSON reviewed: footer and radio content
exposed, long captions wrap within280-pixel compact controls; upper path controls
are naturally outside the final bottom scroll position but were individually
exposed by the geometry test. Link-copy now reserves Windows empty objects with
original CreateDir/Create_NEW before SetReparseData; a failed POSIX exclusive
symlink never removes a racing destination. POSIX target type validation uses
Find_FollowLink solely to classify the chosen target; mutation still stores raw
target and does not traverse it.

Source POSIX MyMoveFile delegates EXDEV to byte-copying MyMoveFile_with_Progress;
that path would follow a link. QtRenameFileSystemEntry therefore supplies only
rename on POSIX and reuses original MyMoveFile on Windows. Native link Move and
the staged consumer explicitly clone raw reparse/readlink data on rename refusal,
check cancellation before deleting the old link, and remove their new object on
failure. Linux tests use private temporary roots on different /tmp and /dev/shm
devices, verifying real EXDEV for the source primitive and staged consumer.
Ordinary buffered file copies now use the existing original SetDirTime and
SetFileAttrib_PosixHighDetect after closing/flushing output, preserving supported
timestamps and attributes/mode; metadata failures reach the copy error boundary.
New copied folders restore metadata after child creation. Existing merge folders
retain their own attribute policy. This is an explicit P2-05 adaptation: upstream
CopyFolder does not restore portable directory metadata. Unix inode identity,
ownership and ctime are OS-created values; symlink permissions are OS-specific,
and link timestamps use no-follow SetLinkFileTime. No ownership impersonation or
extract metadata policy is added here. Archive Add link storage/deletion and
extract security/metadata remain separate backend capabilities; the filesystem
Link command does not enable unavailable archive workflows.

## P2-04 — source Benchmark acceptance (2026-10-03)

P2-04 is accepted within the scope below; this final record supersedes all WIP
checkpoints in this section. Complete source Common/Bench and GUI/BenchmarkDialog
own the native calculations, callbacks, state, properties, memory policy and
worker lifecycle. Backend providers/services and Qt controls are connected to
the real Tools menu; success, failure, cancellation, Restart/Stop/Close, deferred
MainWindow close and source Help are checked. No CLI output formula substitutes
missing source metrics. Total mode remains callable through the original entry
without adding its UNDER_CE-only command to the desktop Tools resource.

Final evidence: unsandboxed macOS check.sh formatting/all-target build/202CTest
PASS89.68s, followed by localized text geometry/Help fixes' three affected suites
PASS5.82s. Linux fivefocused suites PASS3.34s, all Qt cases zero skipped, then
final three affected suites PASS2.28s. Windows fivefocused targets build and
CTest PASS4.09s, all Qt cases zero skipped. Baseline378exact/171adapted/3local
verified; all checks2026-10-03, Qt6.11.2/macOS arm64, Qt6.4.2/Linux arm64,
Qt6.8.3/MSVC14.44/Windows arm64. Eight EN/RU normal/total JSON per platform
reviewed:960×650/640×400, exposed footer/console, named controls reachable by
scrolling and fitting text. Compact top-state system rows are intentionally
below the viewport; geometry tests scroll to expose each one. Rendered Windows
pixel parity and whole-plan platform acceptance remain P3.

Windows found three integration issues and the final tests verify their fixes:
the original FM startup large-page flag dependency; spanning labels stretching
beyond a compact viewport; Total worker completion preceding its queued UI text.
The Qt label adapter retains natural-width group/system/error captions and the
wrapped label's required height after source SetItemText. The test waits for the
visible Total output before closing. The original numerical/state bodies remain
unchanged. This is a whole iteration; helper commits were not made.

### Source scope and earlier checkpoints (superseded by final evidence above)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
`https://github.com/ip7z/7zip.git`. Complete `CPP/7zip/UI/Common/Bench.cpp/.h`
copied into the mirrored source path. Only canonical includes, provenance and
required clang-format differ; original benchmark generation, engine invocation,
callbacks, timing, memory policy and ratings remain authoritative. Exact
`CPP/Windows/SystemInfo.cpp/.h` copied unchanged into third_party; existing
System/Registry/CreateCoder/MethodProps dependencies are reused. No new digest,
compression or rating implementation is written.

The bounded official CLI probe (`b -mmt=1 -md18 -mtimems=50 1`) succeeded, but
its text output lacks all Current UnpackSize values used by the original dialog.
The native Bench callback exposes these source metrics directly through the
retained official codec catalog. The new focused native_benchmark fixture probes
actual encode/decode callbacks and cancellation with256KiB/1thread/50ms settings;
it imposes no golden performance numbers. sevenzip_common and its focused test
target build successfully on macOS. Focused native_benchmark1/1PASS0.46s,
four Qt cases passed/zero skipped, macOS arm64Qt6.11.2. Its first discovery
attempt using the default engine location failed before Bench; the fixture now
selects the explicit official7zz as existing native catalog tests do. Baseline
376exact/167adapted/3local verified. GUI BenchmarkDialog port, menu consumer,
memory refusal, Stop/Restart/Close, geometry, platform checks and full gate remain
mandatory; this prerequisite does not accept P2-04 or advance package counts.

Subsequent WIP now copies complete GUI/BenchmarkDialog.cpp/.h: CSyncData,
CBenchProgressSync, CThreadBenchmark, CBenchCallback/CBenchCallback2/CFreqCallback,
property parsing, memory refusal, pass/ratings aggregation, UpdateGui and
Restart/Stop/Cancel/finish-join bodies are retained. Qt replaces CModalDialog,
combo/edit/font/control resizing and message/timer services through
CQtBenchmarkWindow; layouts and fixed-width fonts replace Win32 rectangles and
CMyFont. POSIX uses the original CProcessAffinity Get/processor getters because
the source Windows-only combined getter is absent there. GetTickCount uses a
monotonic QElapsedTimer. CBenchCallback2::CheckBreak now reads Exit under the
same original critical section used by SendExit, avoiding an unsynchronized
cross-thread Qt cancellation read. Exact BenchmarkDialog.rc/Res.h are vendored.
The RC has no STRINGTABLE; adding it to rc_strings.cmake was disproved by the
generator's required table contract. Its original captions are presentation
fallbacks in the Qt adapter; that generator is unchanged.

CompressCall2.cpp/CompressCall.h partially copy only the original Benchmark
entry/declaration. Its totalMode `m=*` properties and default iteration count
are retained; CREATE_CODECS/LOAD_EXTERNAL_CODECS/global HWND are replaced by
scoped backend catalog providers and QWidget. Other compression/extract/checksum
call wrappers from these files remain omitted; their existing workflow owners
are not accepted by this partial file. MyLoadMenu's original two-panel timer
guards are retained in Qt dispatch. Benchmark action is connected and enabled
only when source FindMethod_Index confirms native LZMA encoder/decoder. The
immutable catalog outlives all provider/callback references through worker join.

sevenzip_panel/native_benchmark target builds on macOS. Actual bounded native
GUI Benchmark completes, presents original resulting GIPS and joins before close;
focused1/1PASS5.57s at the first GUI checkpoint. Further deterministic lifecycle,
memory refusal, actual menu, language/geometry/JSON, cross-platform and final
gate checks remain open; no P2-04 acceptance or helper commit.

The backend now owns `IQtBenchmarkServices`: its production adapter delegates
unchanged arguments to original `Bench` and `NWindows::NSystem::GetRamSize`.
Only those two calls in BenchmarkDialog cross this narrow engine/platform seam;
the source property, memory, callback and result bodies remain authoritative.
Services/catalog live through worker join. Original CompressDialog.cpp is also
registered in sevenzip_panel to reuse its sole `SetErrorMessage_MemUsage`
implementation; the CMake consumer link rule excludes the owner itself.

Deterministic public workflow tests now cover original metrics, total console,
Restart→Stop→Close joins, memory refusal without an engine call, engine errors,
normal/compact geometry and EN→RU→EN while open. Qt construction is split into
normal pane, metrics and summary builders; resource text bindings preserve
source captions and original LangUtils standard button mapping402/409. A first
geometry test incorrectly expected a shortened Russian title; corrected to the
source translation. JSON review also caught control2 translated as language2;
the original control→language mapping fixed it with a Cancel assertion.

Actual MainWindow Tools→Benchmark tests use confirmed native providers and the
backend service seam, checking provider absence, cancel/error, modal lifecycle,
join before services release and window usability afterward. The fake RAM32MiB
initially caused the source's default-memory refusal before its worker; the
menu fixture now reports8GiB without allocating it. Latest focused3/3PASS5.57s
on macOS arm64Qt6.11.2 before the added active-modal MainWindow close assertion.
Current eight normal/total EN/RU widget JSON reports reviewed, resource structure
and Qt usability only. Cross-platform and full gate are still pending.

The macOS unsandboxed full gate subsequently passed formatting/build/202CTest,
89.68s. Linux final fivefocused suites passed3.34s, all Qt cases zero skipped;
normal/total EN/RU JSON reviewed with exposed footer/console. The later source
Help callback assertion passes macOS benchmark_lifecycle1/1,0.52s.
Windows first linking exposed `Bench.cpp::Add_LargePages_String`'s startup
dependency `g_LargePagesMode`. The original `FM.cpp` default definition(false)
is retained in the platform services adapter. Qt startup does not enable the
LockMemory privilege; the large-pages option is disabled. The rest of FM startup
and SetMemoryLock are not ported by this definition. Bench's diagnostic body is
unchanged. Windows rebuilding and final focused checks remain mandatory.

## P2-03 — source CRC/Hash/Test acceptance (2026-10-03)

Baseline for this package: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
retrievable from `https://github.com/ip7z/7zip.git`. This section supersedes the
earlier partial HashCalc/HashGUI debt statements for the bodies now copied.
P2-03 is accepted within this scope; whole-plan acceptance remains open.

- `CPP/7zip/UI/FileManager/PanelCrc.cpp`: copied CDirEnumerator, CThreadCrc,
  CApp::CalculateCrc2/CalculateCrc, scan/read passes, error counts, aggregation,
  progress and final result routing. Qt replaces HWND; POSIX uses the original
  CEnumerator CDirEntry/Fill_FileInfo and CInFile::ReadFull interfaces. Original
  Panel.h CMyBuffer is retained. Local external-provider parameter replaces
  LoadGlobalCodecs/global storage; its caller retains the immutable catalog.
  Filesystem symlinks/reparse objects remain refused at the existing port boundary.
- `CPP/7zip/UI/Common/HashCalc.cpp/.h`: complete source HashCalc, Hash bundle,
  line generation/parsing, NHash::CHashPair/CHandler reader/writer/verifier,
  PGP cleartext reader branch and Codecs_AddHashArcHandler are now copied.
  Includes/formatting are adapted. NHash::CHandler owns a CExternalCodecs context
  through ISetCompressCodecsInfo instead of reading a mutable global pointer;
  AddDefaultMethod receives that context explicitly. The original source has
  no cryptographic signing implementation: its PGP branch is read-only, not
  a new signature generator or a promise of signature authentication.
- `CPP/7zip/UI/GUI/HashGUI.cpp/.h`: complete CHashCallbackGUI/HashCalcGUI worker
  and original callbacks, result formatting and ListViewDialog flags/order.
  QWidget/OnInit/exec replace HWND/Create; functional hash bodies are retained.
- Exact dependencies `CPP/Common/DynLimBuf.cpp/.h` are vendored unchanged.
  CMake moves existing EnumDirItems, SortUtils, FileLink, UniqBlocks and
  ProgressUtils dependencies into sevenzip_common so the complete HashCalc
  module has no common-to-filesystem cycle. Paths and authoritative files stay
  unchanged; digest implementations are the previously borrowed original ones.

MainWindow now invokes source CalculateCrc2 with scoped catalog providers.
Archive selections use source CopyTo streamMode/hashMethods and original hash
extract callbacks. The engine-only transport can still spool one unnamed stream
privately before CopyStream feeds that callback; there is no application-level
extract-selection-then-CLI-hash workflow. Native Hash/RAR handlers use the typed
ArchiveBackend extraction boundary after item-order verification. CAgent Init
retains source hash-handler flag and physical hash-file directory prefix.
The local hash-read guard rejects unsafe manifest paths and inner symlinks/
reparse objects before source callbacks read external files. Qt only adapts
process/thread ownership, cancellation/pause and presentation; it implements no
digest algorithm. Backend hash/hashMethods now run original HashCalc in an owned
worker; provider/catalog lifetime outlives all retained IHasher instances.
Native workers accept the source progress dialog's background state in the host
process, just as the existing volume worker does. Windows build consumers exposed
two header dependencies: SplitDialog explicitly includes MyTypes for UInt64;
QtFileManagerCommands explicitly includes MyWindows before its service interface,
matching its workflow implementation's Win32 CreateFile macro spelling.
No original source symbol is renamed. A comparison with the exact baseline after
identical clang-format confirms that the three complete borrowed bodies differ
only at the recorded include, Qt/platform/provider and link-boundary adaptations.
The newly built Windows selection consumer also exposed a prior Rename hint
adapter that split source paths only on `/`. It now obtains the original panel's
GetItemName(kpidName), preserving the source folder's name policy on Windows and
Unix. The existing forged-display-path regression validates that result; the
transport lifecycle assertion checks cancellation HRESULT and a nonempty localized
message instead of assuming the host language is English.
Source method lookup determines enabled original menu entries; the upstream
menu's fixed command list/order is preserved. PanelCopy now propagates SetMethods
failure rather than silently running without the requested hashers.

Initial macOS arm64 checks: source workflow/bundle/results, checksum, selection
and binding lifecycle 6/6 PASS2.77s; subsequent checksum (encrypted/wrong password/
Cancel/corrupt ZIP) and native hash-file UI (good/CRC/missing/unsafe) 2/2 PASS0.77s,
zero skips. Source hash-file write/read/verify/error/cancel and multi-method/`*`
worker checks pass. These focused results are dated evidence, not a full gate.
Final unsandboxed macOS arm64 Qt6.11.2 check.sh passed formatting, all-target
build and199/199 CTest79.48s. Later localized MyWindows/Rename/lifecycle fixes
passed their five direct consumers2.79s. Linux ARM64 Qt6.4.2/GCC13.3 focused
12/12PASS3.82s and final five direct consumers5/5PASS3.28s, zero skipped cases.
Windows11 ARM64 Qt6.8.3/MSVC14.44 built13 targets and passed13/13 CTest13.04s,
zero skipped cases. Baseline374 exact/165 adapted/3 local verified. Current-build
hash-results540/420 JSON reviewed: list/footer/buttons fully visible, source
control order and flags retained. No rendered Windows/pixel parity claim.
Whole-iteration local commit follows; no push or upstream writes.

The first full gate passed formatting/all-target build and198/199 CTest; the
filesystem_selection_runtime fixture still expected old fake-backend hashing and
waited on the new modal results. It now checks actual source file totals/size and
absence of errors after forged display paths, across four ordinary/flat views;
it asserts no backend hash call. Its45s timeout bounds a future modal regression.
Intermediate targeted macOS3/3PASS1.64s and Linux3/3PASS1.35s then passed;
the successful repeated full gate and final platform results above supersede
this failed checkpoint. FinderP3-03.1 and remaining mandatory packages stay open.

## P1-17 — ZIP item comment acceptance (2026-10-02)

Accepted whole iteration after unsandboxed check.sh1910 on isolated own-scope
snapshot `/private/tmp/sevenzip-p117-review-h0ufqfu9`: formatting, all-target build
and191/191 CTest PASS86.83s macOS arm64 Qt6.11.2. All162 owned source/build-module/
test files match current checkout byte-for-byte; only unrelated runner hunks were
excluded. Shared checkout check.sh43923 passed formatting/build and191/192 suites
88.64s, failing solely foreign runner_vm because tests/runner_vm_tests.py was
removed concurrently. Foreign code/docs/index hunks preserved outside this commit.

Original source ZIP writer and dependencies, CommentItem/SetProperty/ChangeComment,
ArchiveBackend/native factory/property-order/transaction/reopen adapters and actual
GUI consumer are complete for per-item ZIP comments. Ordinary/AES success,
writer failure, cancellation, clear/raw Unicode/spaces, metadata/input bytes,
independent extraction, nested parent confirmation and native pause/cancel checked.
Four current EN/RU Combo JSON reports360×150/260×150 reviewed. Baseline351 exact,
163 adapted,3 local paths verified against recorded0766b733 revision. Detailed
source scope, deviations, omitted registration/export layer and historical
checkpoints below remain authoritative; earlier pending/disabled states superseded.
Windows/Linux runtime and rendered Windows appearance remain unverified P3 scope.
Counts29/46; next P1-18 source RAR memory consumer. No push/upstream mutation.

## P2-01 — native codecs/formats acceptance (2026-10-02)

P2-01 accepted after full unsandboxed check.sh12893: formatting, all-target build
and188/188 CTest PASS74.09s macOS arm64 Qt6.11.2, 2026-10-02. Baseline verified
224 exact/161 adapted/3 local paths at the recorded0766b733 revision. This whole
iteration is committed locally as `Port native codecs and archive format workflows (P2-01)`;
no helper commit, push, publication or sibling-upstream mutation. The evidence
below preserves intermediate checkpoints and corrected approaches; those earlier
pending-gate statements are superseded by this acceptance. Source code scope,
narrow Qt/platform/process adaptations and actual capability limits remain as
recorded. Windows/Linux runtime and rendered Windows parity are unverified and
mandatory in P3; native ZIP comment/RAR decoder GUI consumers P1-17/18 remain open.

Final MainWindow continuation uses actual native metadata in Add and source
Inside */# actions. `codec_commands_ui` covers all seven writers through the
real Add dialog (success/cancel, exact Unicode/spaced binary bytes), real content
opening/parser and read-only commands, missing/mismatched module read fallback,
and single-stream Delete results/cancellation. Add restoration retains the
detected existing type while a selection is empty/ineligible, then emits the
actual previous→current format transition so original parameter controls refresh.
This is Qt source-picker integration, not a replacement eligibility algorithm.
Fourteen current Add reports at760×720/640×480 were reviewed; fixed detected
format, source methods and exposed footer are checked. Windows rendered parity
and Windows/Linux runtime remain unverified.

Copied mutation input must retain its original basename in a private subdirectory:
changing pathless bzip2/xz DefaultName would make the CLI selector miss the source
item and incorrectly report success. The native handler algorithms are unchanged:
Bz2Handler.cpp::UpdateItems rejects zero items, while XzHandler.cpp::UpdateItems
calls Xz_EncodeEmpty and its GetNumberOfItems still reports one. Tests verify those
actual distinct outcomes and cancellation, with preserved source bytes on refusal.
Exact private paths use -sae, corresponding to original
ArchiveCommandLine.cpp::ParseArcNameMode and Update.cpp::CArchivePath::ParseFromPath
(k_ArcNameMode_Exact). This prevents CLI extension appending for extensionless
owned spools; a disposable actual-engine reproduction and owned_temp_publication
verified the adaptation. The first full gate found that spool regression; the
subsequent focused4/4PASS4.00s and final control/publication4/4PASS5.66s passed on
macOS arm64 Qt6.11.2 2026-10-02. The repeated full gate is pending at this checkpoint.
The small output-path adaptation remains in existing UpdateArchiveSourceStream;
splitting that existing broad orchestration is deferred to its native callback
transport work rather than combining a structural rewrite with this fix.

Latest native opening continuation copies the complete
`CPP/7zip/UI/Common/OpenArchive.cpp` from
[0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
superseding the earlier property/parser-only fragment. Original NParser handler,
signature ordering/scanning, CArc::PrepareToOpen/OpenStream2/OpenStreamOrFile,
zero-tail checking, native nested CArchiveLink::Open/Open2 and ReOpen bodies are
retained in their original owner and control flow. Exact SetProperties.cpp/.h
are added under third_party at the same source paths/revision; source notices and
the existing applicable license are preserved. Compile-time SFX/Windows branches
are retained but not claimed exercised by the local non-SFX macOS build.
Required deviations are canonical include owners, provenance notices, the
existing optional QtOpenService early branch, and sharing the already copied
default-name block via CArc::SetDefaultName. Native OpenStream calls that same
body; it is not duplicated. A fresh exact upstream snapshot formatted once with
the repository style was compared against the adapted file: only these recorded
changes remain. The sibling checkout was not changed.
`native_archive_opening` directly executes native Open3 with actual module
handlers, without QtOpenService: ZIP unknown extension, explicit 7z, gzip/TGZ
AddExt, concatenated ZIP parser, forced-type refusal and source callback
cancellation/finished. These direct tests alone do not prove GUI integration.
The application continuation now calls original native Open3 in the source
QtArchiveFolderManager worker through ArchiveBackend::openNativeArchive.
New local ArchiveNativeOpen is only a lifetime/callback adapter: it keeps the
source CArchiveLink and its exact immutable catalog together. Its stable proxy
forwards the opening UI during Open3 and detaches it afterward, because source
volume streams retain COpenCallbackImp after the opening dialog has gone.
QtArchiveFileResource exposes actual native archive properties and closes native
streams before publication/owned temporary cleanup. Item metadata/extraction,
updates and reopen remain process integration; this is not a claim that the
whole source Agent/native extraction consumer is complete. Missing/failed module
discovery preserves the documented process-only read fallback. A valid catalog
uses native detection and returns its refusal/cancellation before process start.
`native_folder_opening` covers source CFfpOpen native properties, catalog reload
lease, wrong forced-type refusal before process start, worker cancellation,
encrypted-header password propagation and unchanged original bytes. Together
with native_archive_opening and all42 codec_update scenarios it first passed3/3(2.74s).
The final stable proxy/lifetime continuation adds actual TGZ and Split→7z
source-folder extraction round trips. Source Open only descends when its original
IInArchiveGetStream→IInStream contract succeeds: the installed gzip handler keeps
its TAR item at the gzip level. The initial automatic-TAR test expectation was
incorrect and corrected after inspecting original Open and actual module behavior.
Native nested chains preserve parsed opening intent across process listing,
extract and reopen; empty native types use explicit *:r at that integration
boundary. Process writer readiness refuses volume sets/nested native chains,
even after native Close. The expanded consumer run caught a real split
Delete/Rename regression introduced by the earlier source read-only migration;
the resource now exposes that process publication constraint through IOutArchive
availability. No compression/format algorithm was rewritten for this refusal.
Native readers are closed before removing an unsuccessful owned stream spool,
which is necessary when platform file handles prohibit removal while open.
After terminal all-target build82764,37 direct suites passed37/37(15.88s), macOS
arm64 Qt6.11.2 2026-10-02; baseline224/161/3 verified. Final command/mutation UX
and full gate remain open; no P2-01 acceptance or helper commit is claimed.
Source COpenOptions callers must initialize excludedFormats/props, and use fresh
options after releasing native streams; the source API mutates stream pointers.
The latest direct native suite passed1/1(0.31s), macOS arm64 Qt6.11.2 2026-10-02.
Baseline verified224 exact/161 adapted/3 local. Full P2-01 acceptance remains open.
After terminal all-target build,25 direct suites passed25/25(9.06s), macOS arm64
Qt6.11.2 2026-10-02. This includes native opening/cancel and the42 source writer
transactions; it is not the final full repository gate.

Local source-plan transport was also extracted coherently into
archivebackend_sourceplan.cpp: validation, KeepName selection/output and stage
orchestration have separate functions; archivebackend_sourceupdate.cpp retains
single-file/add/mutation transports. Original borrowed modules were not split.
No backend behavior was intentionally changed by this extraction.

Latest source update continuation removes four ZIP/7z transport guards and the
QtArchiveSourceUpdate guard in favor of actual native UpdateEnabled metadata.
Source CAgentFolder/CAgent update and publication bodies remain unchanged; Qt
projects their callback plan into official ArchiveCommandLine actions, preserving
the original input and output transaction boundaries. Native explicit type is
passed to Delete/Rename too, so temporary/unknown filenames do not select a
different parser. Catalog-free compatibility remains ZIP/7z.
The local callback-plan projection now deletes only omitted items, because the
source Add action set overwrites referenced replacements. Delete-before-Add
incorrectly required an empty intermediate one-stream archive. Native KeepName
single-file plans use the existing separate-output update transport and source
properties; copying an unnamed bzip2/xz input to result.format would change its
source DefaultName before matching the replacement. This is process transport
adaptation, not a replacement of source update-state planning or archive codecs.
Actual source folder CopyFromFile and CopyFrom/Add-plan publication covers seven
types under unknown suffixes, binary Unicode/spaced paths and42 success/failure/
cancel scenarios. Controlled failures write partial output before exiting;
controlled cancellation observes actual50% output after writing. Original
archive and replacement source bytes remain unchanged after non-success.
Terminal all-target build and23 direct suites passed23/23(8.61s), macOS arm64
Qt6.11.2 2026-10-02; baseline222/161/3 verified. These checks do not prove every
mutation or final command/error UX. Full P2-01 gate/acceptance/commit remain open.

Latest GUI continuation removes the ZIP/7z projection for new creation, retaining
the native PrepareFormatSelection indices. Existing-target update choices still
require their implemented transactional service. Qt forwards the selected native
name in CompressionOptions and accepts native main extensions/aliases rather
than requiring the handler name as filename suffix. The source
CompressDialog.cpp::FormatChanged encryption flags now drive group enabled and
header visibility states. Source SetMethod2's TAR-at-level0 exception is retained
in the Qt method control and process property DTO; SetSolidBlockSize2 uses the
original Solid_ flag, including xz. A source-selection refresh emits one final
format transition after rebuilding the combo so method/password state follows
the actual choice. These changes adapt existing source state to Qt/CLI fields;
no compression algorithms are rewritten.
Focused codec_creation_ui covers seven types at760×720/640×480, real creation,
explicit selected type, source-changing gzip→7z, disabled retained passwords,
footer/control exposure and actual POSIX TAR magic (TarHeader.cpp constants).
Current top/encryption JSON reports reviewed for all14 states. After terminal
all-target build,21 direct suites passed21/21(7.66s), macOS arm64 Qt6.11.2,
2026-10-02. Generic update transport and final full P2-01 gate remain open;
no Windows/Linux runtime or rendered Windows parity is claimed.

Typed creation continuation: local `CompressionOptions::format` carries the
stable native handler name across the Qt/process boundary. Backend creation
uses `CCodecs::FindFormatForArchiveName` or the explicit native name, then the
ported `UpdateGUI::PrepareFormatSelection` for actual source eligibility. It
does not infer write capability from a filename. The existing staging,
publication, source lifecycle and official-engine compression remain owners;
method defaults now use the source format table rather than Deflate for every
non-7z format. Standalone callers without a catalog retain ZIP/7z compatibility.

`Console/List.cpp` transport records for bzip2/xz can omit Path; bzip2 can also
leave sizes undefined. The local listing parser retains these real records,
without inventing size/property values. Their display/extraction name comes
from the already copied `CArc::SetDefaultName` and exact `DefaultName.cpp`, at
the recorded upstream revision above. The original `GetItem_DefaultPath`
extension-property branches are not newly claimed by this transport projection.
Seven actual handlers (7z/ZIP/gzip/bzip2/xz/tar/WIM) round-trip binary Unicode/
spaced files under an explicitly selected type and unknown target suffix.
Failure after partial output and cancellation remove staging and preserve the
source; RAR and gzip-directory eligibility refuse before process launch.
After terminal all-target build, 17 selected suites passed17/17(7.54s), macOS
arm64 Qt6.11.2, installed official engine/module26.03, 2026-10-02. Baseline
verified222 exact/161 adapted/3 local. At this backend checkpoint UI choices projected ZIP/7z;
generic update transports, final UI consumers and the full gate remain pending.
P2-01 is not accepted and no helper commit has been made.

Selection/name continuation copies the active ShowDialog one-file/CFileInfo/censor
and format eligibility blocks into UpdateGUI.cpp::PrepareFormatSelection. The
fragment stays in its original owner; explicit CCompressDialog/CCodecs/censor
inputs replace the unavailable full CUpdateOptions/modal signature. Native Find,
Include, single-path/non-directory, UpdateEnabled, KeepName, hash/SWFC override,
OriginalFileName and KeepName decisions retain source bodies/control flow.
ArcIndices.Clear is the necessary repeated-picker adaptation; Qt handles error
presentation and method-control setup. The original user-codec SetMethods block
and remaining ShowDialog options/modal orchestration are not claimed by this
fragment; external codec methods remain P2-02.

Real Add source addition/removal rebuilds native selection inputs and keeps the
current format while projecting the implemented ZIP/7z transports. Naming uses
actual one-file/OriginalFileName/KeepName inputs before switching format. Initial
sources are prepared before constructing the form, and the already ported full
CreateArchiveName body supplies the suggestion instead of an independent generic
name for metadata-configured callers. Standalone no-catalog compatibility retains
its prior generic suggestion. Qt list/path projection owns no archive naming or
format eligibility algorithm. All-format creation/update transport is still
unfinished and the remaining writer choices are not enabled prematurely.

After terminal affected builds, six direct naming/Add/metadata/language suites
passed6/6(1.43s), macOS arm64 Qt6.11.2 2026-10-02: actual native one-file versus
multiple/directory eligibility, OriginalFileName, no-update-engine refusal, real
source-picker two-files→one KeepName transition and native initial archive name.
Current native archive-path760/640 JSON reviewed: Archive.7z suggestion,
path/format/path-mode order, exposed controls/preview/footer at both sizes.
After rebuilding direct source-update/source-extract/operations/advanced/encryption
consumers, the final selection/name checkpoint passed11/11(7.53s) on the same
platform. Baseline verified222 exact/161 adapted/3 local. This does not accept P2-01;
remaining writer transports, final live command/error UX and full gate are open.

Add/naming continuation copies CompressDialog.cpp::SetArchiveName2,
SetArchiveName and the metadata lookup portion of ArcPath_WasChanged from the
recorded baseline. ArcFormats/ArcIndices/Info.FormatIndex/m_PrevFormat,
OriginalFileName/KeepName and main-extension decisions retain source roles.
Qt supplies field/selection state instead of Win32 control reads; the combo
binding still owns SaveOptionsInMem/FormatChanged delivery. IsSFX/GetMethodSpec
are narrow input adapters; their SFX/hash consumers remain P2-02/P2-03, not an
acceptance of those workflows. Legacy standalone callers without metadata retain
their earlier two-format compatibility path; it is not codec discovery.

CreateArchiveDialog retains its catalog lease, populates implemented ZIP/7z
transports from actual UpdateEnabled metadata and passes native indices into
naming/history. Existing-target format comes from the opened archive's detected
type, so an unknown-suffix ZIP does not silently select7z. Failed metadata gives
an empty format list with an error tooltip and disabled confirmation. Qt format
projection/configuration is extracted into createarchivedialog_formats.cpp to
keep the existing construction owner below400 lines; no borrowed module is split.

Six focused naming/Add/metadata/language suites passed6/6(1.39s), macOS arm64
Qt6.11.2 2026-10-02 before the final IsSFX/GetMethodSpec source-symbol adaptation.
After subsequent affected and all-target terminal builds,16 direct naming/Add/
encryption/advanced/update/catalog/opening/extract/operations/drag/browse/language
suites passed16/16(7.72s) on the same platform. Baseline verified222 exact/161
adapted/3 local paths. Current-build archive-path760/640 JSON reviewed:
path→format→path-mode order, two native transport choices, exposed controls and
scroll viewport at both sizes. EN/RU Add760/640 JSON reviewed: footer/preview
remain visible while upper controls are outside the viewport after the test
scrolls to lower controls, as expected; the focused path reports inspect the top.
No rendered Windows reference or full P2-01 acceptance is claimed. Native gzip
KeepName and gzip→ZIP/7z transitions are helper behavior coverage, not an actual
gzip Add consumer. Source UpdateGUI::ShowDialog one-file/original-name/format
selection and full writable-format transports remain mandatory next work.

Latest integrated checkpoint: ArchiveCodecProbe is local process/platform glue;
upstream Console/Main.cpp prints its version but has no parser of its own output
or Qt asynchronous probe. It reads only the bounded official banner (not the
ambiguous Formats extension/signature table), finds the official module relative
to the canonical engine/app/install prefix, enforces ABI and exact version through
CCodecs, and owns start failure/nonzero exit/malformed output/deadline/cancel.
Backend delivery is queued so receivers can destroy the backend safely. Snapshots
are published with shared-pointer atomic operations because source workers read
them. The default MainWindow starts discovery; injected backends are caller-owned.

Open filters and IFolderManager::GetExtensions now project actual source metadata.
Native FilePlugins extension lookup remains its owner; Qt adapts the Windows icon-
resource extension service to typed metadata. Source CArc::OpenStream's default-
name block is copied into CArc::SetDefaultName: only codecs/index inputs are passed
explicitly. Process-owned arcs retain FormatIndex=-1 because their COpenOptions
do not share the in-process catalog; the resource retains the metadata lease and
applies the body using the detected handler, not a filename guess. Exact
DefaultName retains alias/AddExt/tilde/trim rules. The old completeBaseName and
archiveformats.h guesses are removed. Menus and panel transfers use source folder
read-only state. ZIP/7z are explicit implemented transactional transports; native
encoder presence does not enable other mutation paths. Without a catalog the
existing detected-type transport works; an explicitly failed catalog refuses it.

After terminal affected builds, nine codec/catalog/probe/opening/source-extract/
source-update/UI-operations/drag-archive/browse suites passed9/9(7.42s), macOS arm64
Qt6.11.2 2026-10-02. Real fixtures cover unknown-suffix ZIP, gzip with misleading
ZIP suffix, TGZ→.tar source naming and concatenated ZIP parser '#' read-only;
source CFfpOpen and folder kpidReadOnly are exercised, original bytes unchanged.
Earlier async/filter checkpoint passed5/5(1.10s). No full gate, iteration acceptance
or helper commit is claimed. Add metadata/KeepName and final live command/error UX
remain mandatory. Historical checkpoints below precede these consumer changes.

Latest checkpoint additionally copies OpenArchive.cpp::ParseComplexSize,
ParseTypeParams, ParseType and ParseOpenTypes from the same baseline, retaining
original symbols, branches, type/colon/dot/overflow/recursion rules. No native
opening/parser algorithm is claimed complete by that parsing fragment.
DefaultName.cpp/.h are exact reusable dependencies, with GetDefaultName3/2
extension removal, AddExt, no-extension tilde and trailing-space rules unchanged.

ArchiveBackend now owns an immutable shared ArchiveCodecCatalog. This is local
integration glue around CCodecs: explicitly supplied module and verified engine
version, native ABI/property load/error, strict module-version comparison, no
capabilities on failure, sorted native metadata and presentation extension list.
Lookup/update and open-type parsing delegate to source owners. Old retained
catalogs keep their module alive during replacement and native handlers must be
released before their retained catalog. No global singleton is introduced.
At that earlier checkpoint runtime discovery/probe and UI consumers were pending;
the latest integrated checkpoint above supersedes that status.

After successful terminal builds of app and codec suites, codec_capabilities/
codec_catalog passed2/2(0.40s), macOS arm64 Qt6.11.2 2026-10-02: native */#/chain/
offset/overflow and default names, missing module, busy/unverified-version guard,
real module metadata, mismatched version, reload and retained live RAR5 reader.
Earlier 13-consumer evidence below precedes this latest backend/parser checkpoint.
The latest seven codec/catalog/opening/source-extract/source-update/hash/native-
callback suites passed7/7(3.89s) after terminal builds. Baseline verified222 exact/
161 adapted/3 local paths. No full P2-01 gate or helper commit is claimed.

Baseline [0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17):
`CPP/7zip/UI/Common/LoadCodecs.cpp/.h` copied in full under original src paths.
Registration, codec/hasher/format property decoding, ABI compatibility, DLL
discovery/error/cleanup, extensions/signatures, lookup and handler creation
retain upstream bodies. Only includes and separately recorded formatting adapt.
The formerly exact header moves to src to resolve the single adapted DLL owner;
there is no duplicate implementation. RegisterArc.h and Windows/Registry.cpp/.h
are exact dependencies; Registry builds only on Windows with Unicode definitions.

DLL.cpp::GetModuleDirPrefix retains the source directory/fallback body.
MyGetModuleFileName uses Qt's executable path: upstream has only a Win32 resource
module lookup, while this application has no separate resource DLL. Existing
CLibrary bodies remain unchanged. No registry writes or registration are performed.
Z7_EXTERNAL_CODECS is enabled consistently across consumers. Existing hash tests
select builtin providers with nullptr. PanelCopy activates the source-commented
local CExternalCodecs lifetime because this port has no FM global singleton;
attaching runtime providers remains pending, without adding a mutable global.

codec_capabilities checks source alias/AddExt/case/path/type-chain behavior,
missing library, ordinary-library refusal and the installed official 7z.so26.03
arm64 metadata. DOCX resolves to ZIP; RAR5 is read-only; gzip retains KeepName/
tgz→.tar. Real RAR5 reader and ZIP writer objects are created and released before
unloading. This does not prove ZIP comment writing or decoder GUI callbacks;
P1-17/18 remain open. The installed engine module is not claimed to be built
here from the source baseline. Missing real-module coverage explicitly skips
and remains unverified on that platform.

After terminal builds, codec_capabilities/native_library/native_hash_bundle/
hash_results passed4/4(0.56s), macOS arm64 Qt6.11.2 2026-10-02. Baseline
verified220 exact/161 adapted/3 local paths. All-target consumer build passed;
13 codec/library/hash/drag/source-extract/source-update/UI-operations/opening/
lifecycle/focus consumers then passed13/13(7.54s). No full gate, accepted P2-01
iteration or helper commit is claimed.
Remaining mandatory scope: Add metadata/KeepName, final live Open Inside */# and
missing/incompatible-module UX checks, direct consumers and the full gate. The
installed native handler objects still do not accept P1-17/18.

## P1-16 — drag/drop source owner (accepted implementation, 2026-10-02)

Integrated implementation accepted after source-body/dependency review, real
archive bytes and live two-panel workflows, normal/compact widget reports and
the full unsandboxed formatting/build/179-test gate (67.66s), macOS arm64
Qt6.11.2. Baseline verification:218 exact/159 adapted/3 local paths. Historical
WIP evidence below records intermediate checks, not the final status.
Native Finder acceptance remains mandatory under separate P3-03.1; Windows/Linux
runtime and rendered Windows parity remain unverified under P3-03/P3-06.
Eager materialization and unsupported native promises remain explicit adaptations;
large-list/GUI-thread preparation review belongs to mandatory P2-10.

Baseline remains [0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
PanelDrag.cpp now copies CDataObject::CopyFromPanelTo_Folder, CDropSource's
QueryContinueDrag/GiveFeedback, CDropTarget PositionCursor/IsFsFolderPath/
GetEffect_ForKeys/GetEffect/GetTargetPath, and CApp CreateDragTarget/SetFocusedPanel/
DragBegin/DragEnd. CApp owns the drop target; the prior presentation forwarding
did not establish source drag state. Object creation occurs in the Qt CApp
constructor before any presentation can focus a panel. Native vectors/strings,
copy options and archive CopyTo consumers remain authoritative.

Declarations move into qtpaneldragmodel.h because Qt MIME/presentation needs the
native carrier/state interfaces; implementation remains in original PanelDrag.cpp.
OLE interface/refcount/storage/capture/cursor plumbing is replaced by
qtpaneldragtransport. Win32 point/window/list hit testing and enabled state are narrow
presentation services; POSIX same-drive detection must use actual storage volumes,
not treat all absolute paths as the same '/' drive. GetEffect and continuation
branch order remain source bodies. The source commented-out MOVE mask stays
disabled. Highlight fields move from the panel to its single drop-target owner;
ClearState removes presentation highlight before clearing references.

Source CPanel::OnDrag now retains support/right-button/operated-index gates,
native 7zE CTempDir and corrected filesystem names, DragBegin/End, source
CopyTo options/moveIsAllowed, transfer decision, error/messages and final selection
branches. CF_HDROP HGLOBAL/COM lifetime and DoDragDrop are replaced by a narrow
Qt transport interface and names carrier. On POSIX the Windows-only ADS-prefix
guard is omitted and normalization remains; missing E_UNEXPECTED uses its original
HRESULT value. Comments/debug/OLE clipboard-only calls are outside this scope.
The source SetTransfer carrier exposes native flags/mode/effect/command/folder and
typed coordinates; source SetData2 needProcessBySource and GetData/GetTransfer
flag construction remain in PanelDrag. OLE medium validation/version/ISA/COM
serialization is omitted for the trusted typed in-process carrier, never decoded
from an external MIME marker or serialized pointer. External sources supply URLs.
Native owned temp lease is provided to the receiver transport: protection must be
persisted before external delivery; positive receiver completion clears it,
otherwise OnDrag disables automatic root deletion. This replaces the explicitly
documented upstream delayed-recipient deletion hazard, not source archive logic.

Source Drop/DragEnter/Over/Leave and SendToSource_auto retain native command choice,
right-menu masks, external FS Add/internal FS Copy/open-archive Copy and confirmation,
target-path handshake, source-versus-target execution and flags. Win32 OLE delivery,
message box and popup presentation become narrow Qt services. Typed failure/cancel
results return from CompressDropFiles/CopyFromNoAsk so the source preserves
selection on target failure. Qt's release transition occurs after target execution,
so the transport restores its Copy_HRESULT after QueryContinueDrag's original
pre-drop reset. Failed typed GetData returns before exposing nonexistent paths.

Source CompressDropFiles retains parent/root-temp path choice, archive naming,
wait flags and CopyFsItems/CopyFromNoAsk branches. CreateArchiveName's entire body
is copied into original UI/Common/ArchiveName.cpp with include/formatting changes;
ArchiveName.h is exact vendored source. PanelCopy's previously unconnected
CopyFrom_Paths branch now uses its native NFsFolder::CopyFileSystemItems body,
whose owner was already ported; CopyFsItems and CopyFromNoAsk are copied from
Panel.h/PanelCopy.cpp. Archive/Add execution remains behind native folder/backend
owners. QtFileManagerDrag's compression callback opens the existing Add dialog,
uses native CCompressDialog::SetArchiveName for the selected format's suffix,
and awaits the existing backend progress consumer.

Qt transport prepares archive bytes with native CopyFromPanelTo_Folder before
QDrag::exec because Qt has no portable OLE release-before-drop hook. This can
extract for a subsequently cancelled drag and materialize before an internal
destination is known; it prevents native receivers seeing paths without bytes.
Temporary ownership is protected before exposing URLs, unknown successful
receiver completion stays durable, and known cancelled/finished recipients clear
protection. No native promise or delayed byte generation is claimed.

The live QtFileManagerDrag controller now binds table/icon sources and all
panel viewports, including the second panel created later by F9. Native popup
pairs/masks/order and Yes/No/Cancel confirmation become Qt callbacks; command
selection remains in PanelDrag. GetItemName is exposed to the production hit-test
adapter without changing its body. Source Begin/End and late CopyTo are guarded
for the entire drag; close is refused, hover suspended and single-click suppressed.
Qt's Static icon movement resets dragEnabled, so the common policy is reapplied
after a mode change. Rejected/cancelled drops do not reopen archives. Disabled
viewports, nonlocal URLs and unaccepted synthetic Drop events are rejected before
execution. Native right drag is filesystem-only; Qt defers a macOS press context
menu until release to allow that gesture. These parts form the integrated P1-16
implementation accepted above. Upstream OnDrag documents
unknown external recipients can still use bytes after DoDragDrop returns;
ownership protection must retain that uncertain lifetime rather than delete
by elapsed time. Native platform promises and file managers require separately
recorded platform evidence. EnumFormatEtc's OLE-only format enumerator has been
reviewed; Qt QMimeData supplies its local-URL format enumeration. No native
promise or portable delayed materialization branch is claimed. The Qt Add
consumer always awaits in-process completion even when the source permits a
separate nonmodal 7zG process; native wait flags remain recorded.

After terminal successful builds, focused drag_drop/app_lifecycle/panel_focus
passed3/3(0.58s), macOS arm64 Qt6.11.2 2026-10-02. Native OnDrag is exercised
with an actual Unicode/spaced filesystem file: recipient reads selected bytes,
Cancel preserves selection, successful delivery clears selection without deleting
the original, drag indices reset and notifications resume. Other cases check
opposite/source panel rejection, hovered-folder path/highlight removal, same-volume
effect fallback and archive continuation versus cancellation. These are partial
owner checks at the earlier checkpoint; actual archive bytes were subsequently
verified below. Baseline at that checkpoint217 exact/158 adapted/3 local; Windows/Linux runtime
unverified. No P1-16 acceptance or local commit yet.

Subsequent focused drag_drop/drag_archive/source_extract_backend passed3/3(0.96s)
after successful terminal builds, macOS arm64 Qt6.11.2 2026-10-02. A disposable
official-engine ZIP provides actual binary Unicode/spaced-file bytes to native
CPanel::OnDrag → CopyTo → source Agent/resource → Qt MIME/fake recipient. It checks
finished recipient cleanup, unconfirmed receiver protection after local leases end,
drag cancellation and actual engine extraction cancellation; returned Move never
deletes archive members or changes the original ZIP. Disk→disk ControlCopy and
ShiftMove run source target handshake and actual source CopyTo. Latest baseline
verified218 exact/159 adapted/3 local. Live widgets/UX/full gate/platform recipients
remain mandatory, not accepted by these transport fixtures.

Final focused drag_drop/drag_archive/source_extract_backend/app_lifecycle/
panel_focus/fs_folder_copy passed6/6(0.99s), after terminal successful build;
the final archive fixture additionally covers a recipient refusing delivery.

Live drag_drop_ui exercises both target views and both panel directions,
actual ControlCopy/ShiftMove bytes, folder highlight without selection,
source-panel refusal, native right popup Copy/Cancel, external archive Yes/No/
Cancel with actual ZIP update/re-extraction, and real source callbacks through
offscreen QDrag cancellation. Active roots cannot be claimed, close is refused,
SingleClick suppressed and temp roots removed after cancellation. Archive right
gestures retain source refusal. Hover resumes after DragLeave without opening.
Eight current-build JSON reports at980×620/640×400 were reviewed: visible panels,
paths, view stacks and status strips are exposed; Windows rendered fidelity
remains unverified. Focused11/11 passed7.44s macOS arm64 Qt6.11.2 2026-10-02,
before subsequent native archive→disk/archive fixtures (drag_archive passed0.65s).
Final filesystem→Add success/suffix regression passed1/1(0.55s). Full unsandboxed
check.sh passed formatting/build/all179 CTest cases(67.66s), macOS arm64 Qt6.11.2
2026-10-02. No code/test/build changes followed this passing gate.

The actual Finder attempt could not run: Computer Use returned
“Computer Use was not approved to use sevenzip_desktop”. The isolated app process
exited normally; the empty recipient directory is not success evidence. Access
has been requested; by user instruction the Finder check is now a separate
mandatory P3-03.1 under platform acceptance P3-03/P3-06. It does not block
implementation acceptance of P1-16 or independent P2 work. Native transfer
remains unverified; Windows/Linux runtime remains unverified.

## P1-15.2 — owned-temp cleanup acceptance (2026-10-01)

Accepted integrated cleanup and P1-15 parent: actual command/modal browser,
ownership and native/Qt lifetime, source confirmation/cancellation/first failure,
active child-editor protection, properties/preferences/navigation and Help/F1.
After terminal builds, 18 affected CTest suites passed (12.43 s); unsandboxed
check.sh passed formatting, app build and all 175 tests (66.82 s), macOS arm64
Qt 6.11.2. Four current EN/RU 900×580/500×320 widget JSON reports were reviewed:
all named controls remain exposed, including path/filter/footer at compact size.
Baseline verified: 217 exact / 158 adapted / 3 local. Count 26/46; next P1-16.
Windows/Linux runtime and rendered Windows parity remain unverified.

Pinned source remains [0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original BrowseDialog2::Reload filters OS temp names `7z[E/O/S]` plus eight hex
digits; this establishes a naming convention, not ownership. Its OnDelete
prefix guard/selected indices/one-or-multi confirmation/native formatters/
delete loop/first failure/reload and MyBrowseForTempFolder are the required
functional basis. They are now copied into mirrored BrowseDialog2.cpp/.h and
connected through native MyLoadMenu → CApp → QtFileManagerCommands.
BrowseDialog2.rc establishes Delete/Refresh, Parent/read-only path, list,
disabled filter and Help/Close ordering. The native cleanup command is enabled
when navigation is available and no modal consumer is active.

Copied functional scope: CBrowseItem/CBrowseEnumerator::Enumerate and its
200-dir/2000-file limits; native vectors and one-child aggregate; PrintProps*
and PrintFileProps; CompareItems, sorting transitions, Reload selection/focus,
GetParentPath/OpenParentFolder/OnItemEnter, single/multiple confirmation,
OnDelete's native per-item removal/first-failure return/reload; context command
IDs, ordering, selection constraints and properties; MyBrowseForTempFolder title
and original fm/temp.htm Help topic. The shared Browse_ConvertSizeToString body
is copied into its original BrowseDialog.cpp owner. Exact BrowseDialog2.rc/Res.h
are vendored at the pinned revision. Qt control construction/rows live in
qttempbrowsepresentation.cpp; public class declarations move from the original
CPP to its header for this adapter, without a second functional implementation.

Required adaptations: POSIX CEnumerator::CDirEntry + Fill_FileInfo(false),
IsOsSymLink/GetWinAttrib/FiTime conversion services; Qt controls/messages/cursors,
column sorting invokes the original comparator; registry lookup replaces only
top-level naming membership. Identity/root/no-link ancestry is revalidated under
the claimed root lease immediately before source deletion. Active/unconfirmed
external leases deny claims. A failed PrintFileProps reload invalidates source
selection references, so confirmation returns rather than targeting a new row.
POSIX MyGetTempPath now uses the actual Qt/OS temp service (TMPDIR/macOS per-user
temp), replacing upstream's explicit FIXME /tmp fallback; Win32 service remains
unchanged. Canonical OS temp aliases are resolved for this browser boundary.
Adjacent work/publication paths and prior roots outside current OS temp are
excluded, even if tracked for lifecycle purposes.

Qt OS association and detached self-open replace Win32 shell/process services;
protection is persisted before launch and cleared only on launch failure.
Completion without a waitable handle is deliberately unconfirmed. Directory
Open Outside : 7-Zip remains disabled with a reason until P2-07 supplies a
directory command-line consumer; current main.cpp opens archive files only.
Omitted source scope: drives/non-temp navigation (blocked by the temp boundary),
UNDER_CE layout, commented-out filters/path editor, Win32 shell image lists,
Win32 drawing/message plumbing. FM ShowDots/SingleClick/FullRow/Grid use Qt
parent rows, shared track-select hover and a paint adapter; Shell icons use
QFileIconProvider. The dot parent stops at OS temp just like the Parent command,
excluding the non-temp ancestor entry. Parent navigation and all six functional
columns remain. These presentation adaptations do not
claim pixel parity; directory new-window integration remains documented debt.
On macOS the membership/path key accounts for composed versus decomposed
enumeration names, while revalidating the actual root OS identity before deletion.
This fixed an observed missing registered Unicode-file row; other platforms
retain exact names. No ownership is inferred from Unicode-normalized names alone.

The original CTempDir and CTempFile creation/filename/Remove/MoveTo bodies stay
in mirrored Windows/FileDir.*. After source creation a narrow Core ownership
adapter registers the OS object and holds a lease through automatic cleanup.
Registration failure closes the output/removes the created object and returns
an error rather than leaving an untracked success. Native naming algorithms,
upstream types and deletion behavior remain unchanged. CTempFileInfo has a
shared lease copied with native nested CFolderLink/CTmpProcessInfo records;
DisableDeleting is not treated as proof of inactivity. PanelItemOpen's known
process wait/snapshot loop clears external protection only after its handles
finish. Unknown association/shutdown/wait-error paths retain protection.
Successful launch disables automatic deletion before native worker creation:
an allocation/thread failure must not delete a file already handed to a process.
The source wait/update/decision loops otherwise remain intact.

No upstream persistent ownership/lease registry exists: original temporary
cleanup relies on filename filtering and can browse arbitrary OS temp. New
qtownedtempregistry/identity/dir are necessary platform/lifetime glue, not a
replacement deletion algorithm. Records use atomic QSaveFile in a private
application cache directory (explicit private root for tests), per-object
QLockFile with age-based expiry disabled, native device/file ID and directory
kind, and durable unconfirmed-external protection. One generated UUID per OS
identity prevents a second registration from bypassing an existing lock.
ClaimForDeletion re-reads identity/protection under the lock. Links/junctions,
malformed records and replaced objects cannot establish ownership. Windows
identity uses source CFileBase handle information with read/write/delete sharing
and OPEN_REPARSE_POINT; POSIX lstat supplies device/inode/type. This is a narrow
identity service, following the existing FSFolder::ReadFileInfo platform boundary.

Qt-owned hash/view/nested extract/update/rollback/volume/language directories
use the same lease alongside QTemporaryDir automatic cleanup. Stream-to-file
spooling retains its source CopyStream body and holds a lease with its private
engine file. Adjacent archive staging paths are tracked for lifecycle purposes;
the source cleanup presentation retains the OS-temp boundary and
does not expose arbitrary filesystem deletion. Standard output/staging paths
owned by existing backend publication remain behind ArchiveBackend.

owned_temp passed1/1(0.33s), then direct external/nested/extract/update/viewer/
language/volume consumers passed14/14(10.60s), macOS arm64 Qt6.11.2 2026-10-01.
Additional actual child-editor/wait-error ownership and failed-registry fixtures
passed2/2(3.33s) at the earlier registry checkpoint. The final 18-suite and full
gate above include actual cleanup versus active child-editor fixtures. Windows/
Linux identity/lock/runtime behavior is unverified locally.

Publication adaptation: mirrored ArchiveFolderOut::CommonUpdateOperation retains
the source MoveToOriginal / MoveArc_Finish / Before_ArcReopen / ReOpen ordering.
Immediately after a successful physical move, QtArchiveFileResource transfers
the native CTempFile lease to its existing private spool destination. CTempFile
and CWorkDirTempFile expose only that lease; their move algorithms are unchanged.
For the same OS object the held identity lock and record are relocated; a
successful cross-volume copy registers the new object only at the already owned
private spool. Ordinary user archives return without registration. A stale
replaced-object record is pruned when its last lease dies. Ownership-adapter errors
retain source reopening and callback-error priority; a cancelled Finish after a
successful move still follows the ownership transfer. This is narrow integration
glue, not a replacement publication algorithm.

owned_temp_publication checks native CWorkDirTempFile publication for private
spools versus user archives, and actual CAgentFolder::CommonUpdateOperation
rename/reopen with a private spooled ZIP on success, engine failure and cancellation.
It observes changed identity, retained active lock, denied cleanup claim, original
user bytes and final spool removal. Passed1/1(0.67s), macOS arm64 Qt6.11.2.
Actual cross-volume IO and late MoveArc_Finish cancellation are not exercised by
these fixtures; failed physical moves do not register an unproven destination.
temp_cleanup additionally checks Yes/No/Cancel through actual message-box buttons,
Unicode/spaced file and directory deletion, unowned names, link navigation,
active/unconfirmed entries, replaced second-object identity stopping multi-delete,
refresh selection, context IDs/properties and source FM view preferences. OS launch
is intercepted for the SingleClick fixture. main_shell checks actual modal command/
Escape, and archive_external_lifetime opens cleanup during a real child editor.

## P1-15.1 — Help/About/current context acceptance (2026-10-01)

Accepted integrated scope: native Contents/F1, source About/version/license,
all current Options/Add/Extract/advanced Help callbacks, offline links/fragments,
unavailable error, pending input/dirty Apply preservation and EN/RU normal/compact
geometry. Final unsandboxed check.sh passed formatting, app build and172/172
CTest(63.63s), macOS arm64 Qt6.11.2 2026-10-01. Parent P1-15 remains open;
next P1-15.2 owned-temp cleanup, count25/46 unchanged. Windows/Linux runtime
and rendered Windows parity remain unverified.

Baseline [ip7z/7zip0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Mirrored HelpUtils.cpp/.h retains ShowHelpWindow, kHelpFileName and native FString
module-prefix + `7-zip.chm::/` + topic construction. Qt module-directory/offline
display replaces GetModuleDirPrefix/HtmlHelp/hh.exe; Qt parent supplies lifetime.
Win32/CE/MingW display branches use the common Qt adapter. Native MyLoadMenu
Contents `FM/index.htm` and COptionsDialog::OnHelp kHelpTopic_Options are wired
through their original command switch/functions. Main Contents/F1 and advanced
compression Help are actual consumers. Source Edit/Folders/Settings/Menu/Lang
OnNotifyHelp callbacks retain their original topic constants, including source
case and fragment. Qt Options dispatches the selected page's native callback;
F1 and Help never apply pending settings. CCompressDialog::OnHelp and native
ExtractDialog::OnHelp retain original topics; Qt parent is a presentation input.
Win32 CModalDialog::OnHelp's base notification has no Qt control-message counterpart.
Disabled System/Benchmark callbacks remain with P2-08/P2-04; cleanup Help is .2.

AboutDialog.cpp/.h preserves CAboutDialog::OnInit/OnHelp/OnButtonClicked,
kLangIDs, native UString version/date calls, start.htm and homepage switch.
Qt bindings replace LangSetWindowText/LangSetDlgItems; Qt modal positioning,
initialization/standard OK replace CModalDialog services. QDesktopServices
replaces Win32/CE ShellExecute variants. Original external-codec diagnostic
branch is omitted because g_CodecsObj is unavailable in the current external-7zz
adapter; its capability owner is P2-01/P2-02. Exact AboutDialog.rc/Res.h,
7zipLogo.ico, MyVersion.h and C/7zVersion.h are vendored at the full baseline.
The original logo/version/date/copyright/free-software/OK/homepage grouping
remains. Added local application version and explicit original license link
distinguish borrowed source version from external engine; no engine version is
inferred. Desktop version comes from CMake project metadata. EN/RU360×330/300×310
About JSON is reviewed: all labels/buttons exposed, source and local text fit.
Homepage tests intercept Qt URL handling; no live service is contacted.

Exact HTML/CSS comes from the official26.03 installer already used by Language,
SHA2560859c524b8a63551848f0c246abddcb1d0b7b656b0fbfe879f8d85e61a9e6edd.
resources/7zip/help-provenance.json records retrievable URL, CHM hash and all78
exact extracted members (70 HTML,8 CSS). CHM binary index streams are omitted;
QTextBrowser uses original relative links. Encoding/CRLF/content is unchanged.
The release help is English; local window/navigation/unavailable context is
EN/RU. Qt normalizes source topic filenames for native case-insensitive CHM
lookup, retaining fragment case. Offline Qt HTML replaces native CHM presentation
on all platforms; Windows/Linux runtime/rendering remains unverified. Explicit
root fixtures exercise unavailable help; HTTP(S) links launch only after a click.

help_source passed1/1 (0.44 s); actual Contents/F1 and advanced Help plus main
shell/language consumers passed5/5 (2.76 s), macOS arm64 Qt6.11.2. Current
EN/RU760×560/420×300 JSON inspected: browser, Back and standard Close exposed.
Additional integrated Help/About/current-context focused set passed11/11(4.70s);
final real Add/Extract F1 and geometry/resource consumers passed4/4(1.05s).
Baseline verified215 exact/156 adapted/3 local; all78 HTML/CSS bytes and original
CHM member hash independently compared with the extracted pinned installer.
Final native-UString About/actual UI consumers passed2/2(0.54s); full gate above
accepts .1 as one integrated result, without helper commits. Actual cleanup
owner is BrowseDialog2::MyBrowseForTempFolder/OnDelete; DeleteOldTempFiles is
commented out. Owned-temp lifetime/cleanup and parent acceptance remain mandatory.

## P1-14.2 — remaining dialog consumers and acceptance (2026-10-01)

**Accepted current scope:** all current app forms, source captions and dynamic
messages, Qt/integration context and Core-only worker messages support source
English/Russian. Pending input, selections, action identity, modal decisions,
raw paths/engine text and archive bytes remain intact. Static source RC fallback
and dynamic LangString fallback are distinct; partial/malformed files, long
labels, original mnemonic collisions, Tab/buddy reachability and normal/compact
EN/RU geometry are checked. Earlier partial checkpoints below are historical.
Final unsandboxed `./scripts/check.sh` passed formatting, app build and170/170
CTest (63.11 s), macOS arm64 Qt6.11.2, 2026-10-01. Failed-gate fixture regression
set passed13/13 (3.00 s); final isolated language command/main-dialog consumers
passed2/2 (1.04 s). Baseline/inventory verify210 exact/152 adapted/3 local at the
full pinned revision below. P1-14 parent is accepted, count25/46; next P1-15.
Windows/Linux runtime and rendered Windows parity remain unverified; macOS
native mnemonic/standard-button policy is retained. Future disabled Help/System/
Benchmark/shell/decoder capabilities retain their separate mandatory owners.

The final caption review distinguishes source `LangSetDlgItemText`/
`LangSetWindowText` from dynamic `LangString`: missing dialog translations retain
the original RC caption, including case and accelerator position. Local
`QtBindResourceText` implements that presentation service using the unchanged
`LangString_OnlyFromLangFile`; it does not change native lookup or parser logic.
Password/Split/Extract/Overwrite/Messages/Mem, Add/advanced, Progress labels and
Settings/Folders/Edit/Menu/Language static controls use their original desktop
RC literals. Dynamic titles/prompts selected by callers still use `LangString`.
Progress changes its binding from resource Cancel to dynamic IDS_CLOSE at
completion; its regression caught and fixed stale resource fallback on hot switch.

`language_focus` checks actual RU Add's original duplicate `П` mnemonic on Show
password and Parameters, retaining both controls and pending secrets. Qt cycles
focus where the platform enables mnemonics; Tab reachability is unconditional.
macOS can suppress mnemonics by native policy, so this is not Windows/Linux
runtime evidence. Composite CPU/RAM form rows now expose the real combo through
Qt focus proxies and accessible names; EN→RU→EN tests retain values. Partial,
blank-ID and malformed files are exercised in the actual Add form: original
`CLang::OpenFromString` skips blank lines and advances IDs, so a blank line does
not define an empty caption. An attempted native lookup API extension was
reverted after this source inspection. Original EN/RU assets remain unchanged.
Disabled Menu shell/Zone tooltips now have explicit local EN/RU context.

Latest direct RC/focus/Options/advanced consumers passed8/8 (5.69 s), macOS arm64
Qt6.11.2; the final Progress binding correction passed2/2 (9.68 s). Current compact
Add/advanced/Mem/Progress JSON was inspected with full footer/buttons and expected
scrolling exposure. Baseline verifies210 exact/152 adapted/3 local paths. The
whole-app final gate and residual picker-filter review remain open.

The first whole-app gate passed formatting/build and161/170 tests (208.02 s),
with nine failed/not-run suites caused by remaining fixed-Russian/old-Auto
fixtures: Copy/Extract titles, cancellation text, resource selectors and native
New Folder default. Progress Cancel geometry also required fixed Russian Yes/No;
it now checks complete platform standard buttons and native default/Escape and
decision behavior. Combo's failed timer assertion returned before rejecting the
modal dialog; that test process was terminated, the observation/assertion was
moved outside the timer and a30 s CTest timeout added. Failures remain subject
to rebuilt focused and full checks; this run is not acceptance.

Residual Open archive filter was a local Qt extension: upstream
BrowseDialog.cpp::CBrowseInfo::BrowseForFile accepts caller filter descriptions,
and EditPage supplies `*.exe`; no upstream owner builds this app's supported-
archive suffix filter. ArchiveFormats::openDialogFilter now supplies explicit
local EN/RU Archives/All files captions, retaining every existing mask, `.001`,
selection and FilterIndex. `browse_dialog` exercises the real picker and Unicode
first-volume selection in both languages. Codec capability/suffix detection is
unchanged migration debt outside this caption task.

The second gate passed formatting/build and168/170 tests (226.14 s), leaving
File Viewer modal waiting and Language Main Dialog timeout. A macOS process
sample proved File Viewer waited in `openFileViewer::exec` after a30 ms one-shot
test responder had already fired. Viewer/edit responders now wait for actual
visible dialogs and stop before acting, with assertions after modal completion;
the suite has a60 s bound. The Language sample showed MainWindow construction
sorting the populated build directory, not a language-lock deadlock. The existing
per-suite startup/TMP isolation loop ran before new Language targets were defined;
it is now applied after all targets, keeping startup filesystem input deterministic.
Its cancellation warning responder also has local bounded lifetime rather than
callbacks capturing expired loop variables. No production timer or sort body
was changed for these fixture failures.

Source baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
The `FileManager/{PasswordDialog,OverwriteDialog,MemDialog,MessagesDialog,
SplitDialog,CopyDialog,BrowseDialog,ComboDialog}` and `GUI/ExtractDialog` modules
retain their previously ported functional bodies. Source `LangSetWindowText`,
`LangSetDlgItems` IDs, `LangUtils.cpp` standard-control mappings, RC captions,
`App.cpp::CApp::OnCopy` and `PanelSplitFile.cpp` caller title/prompt IDs now feed
Qt presentation bindings. `PasswordDialogRes.h`, `CopyDialogRes.h` and
`SplitDialogRes.h` are copied unchanged into the exact-source manifest.

Bindings install a Qt LanguageChange filter on the window and update caption,
accessible name, optional tooltip/placeholder and source filename suffix only.
Pending passwords, path/name edits, history, selection and modal decision flags
are not reinitialized. Overwrite size/time text and Mem info are refreshed from
their original source formatting bodies; Messages updates the header/button
geometry without replacing existing rows. The independently written directory
overwrite caption is removed: upstream uses the same file-caption IDs for this
branch. Split invalid volume uses source ID7307 in the existing inline Qt error
label; Mem invalid arguments reuse upstream ErrorMsg (POSIX system text remains
English). Extract mode arrays and AddComboItems now use the original source IDs
and control flow; the unsupported absolute extraction mode remains omitted as
before. Hot switch changes item text in place, retaining data and selection.
The old local hint claiming that NoPaths refuses duplicate basenames is removed;
the source overwrite decision handles these collisions.

Add to Archive and the advanced Options presentation are being bound to
CompressDialog.cpp/RC IDs. `createarchivedialog_language.cpp` owns Qt form-label
bindings and text-only item refresh; source numeric settings and functional
owners remain unchanged. `QtLocalText`/`QtBindLocalText` are explicitly limited
to Qt/integration text without upstream IDs, with English fallback and Russian
selected from the source language-file metadata. No source translation IDs or
original distribution assets are invented or modified.

Focused `language_dialog/extract_prompt/password_dialog/volume_dialog` passed
4/4 (1.41 s); after adding Extract, `language_dialog/extract_dialog` passed 2/2
(0.65 s), macOS arm64 Qt6.11.2. This is partial evidence, not .2 acceptance.
Remaining operation/worker/local backend context, complete EN/RU
reachability/fallback/accelerators and the final full gate
are still mandatory. Windows/Linux runtime/rendered parity remain unverified.

Additional in-progress consumers: Add source field labels, password errors,
level/path/update and solid labels, unchanged numeric item data; its additional
Qt source picker/save-folder/footer use explicit local EN/RU bindings. Memory
default display now restores source `k_Auto_Prefix` (`*  `) rather than the
independent Russian prefix. Small-volume confirmation uses existing native
`FormatUtils::MyFormatNew`/`NumberToString` with source ID7308. Advanced Options
binds the original kLangIDs_Options. For hot switch, COptionsDialog::AddPrec
retains its original formatting body and accepts an optional text-only output:
the Qt event updates existing item text using the original payload without
calling SetPrec/OnInit or committing pending checkbox/time policy. Its Type
caption uses the original property owner and retains the existing format/method
suffix. Qt standard buttons replace previous independent RU overrides.
QTextLayout wraps the long source archive-time checkbox at 300 logical px:
QCheckBox lacks native wordWrap; this retains the original words/accelerator
and prevents RU compact minimum width expanding from440 to466px.
Latest focused eight dialog/language/compression consumers passed8/8 (3.06 s);
source baseline verified210 exact/152 adapted/3 local, macOS arm64 Qt6.11.2.
Current JSON reports inspected: core dialogs and advanced EN/RU no clipped
text, advanced compact440×340 retained. Add content outside the viewport is
scrollable and still requires explicit reachability checks before .2 acceptance.

Progress now copies upstream ProgressDialog2.cpp kLangIDs/kLangIDs_Colon and
binds them before its original OnInit flow. Initial Qt resource fallbacks are
English; the two colon captions follow the original service. A LanguageChange
handler refreshes the original cached Pause/Continue/Background/Foreground/
Paused strings and calls the existing SetPauseText/SetPriorityText/OnSize without
changing Sync, transport state or accumulated messages. Finished IDCANCEL keeps
source Close ID408 metadata after OnExternalCloseMessage. New hot-switch tests
cover paused/background state, unchanged percentage/messages and finished Close;
the transport fixture asserts that language events send no operation requests.
Native priority, worker, cancellation and resource contracts passed8/8 (12.81 s).
The existing fast-finish worker fixture now waits at most50×10ms for modal
activation before observing Close focus: it previously saw enabled/default and
focusWidget=IDCANCEL while hasFocus was still false; the same focus requirement
remains and failure closes the dialog. Static English caption expectations were
updated to exact source Pause/Background; no operation semantics changed.

PanelMenu.cpp::AddPropertyString retains its source enumeration/formatting body
and records property IDs plus copied BSTR fallback at the Qt row boundary.
ListViewDialog stores those IDs on existing items; hot switch calls original
GetNameOfProperty, updates Strings used by clipboard/item info, and keeps values,
row identity and selection. It performs no property/backend requery. Properties
title and full-value EditDialog title use source IDs. FoldersHistory and SelectSpec
caller title/prompt now use original source IDs6601/6402/6403/6404. These changes
do not replace their source history/mask algorithms. Actual Properties EN/RU
normal540×360/compact360×240 reports inspected; focused language_ui/folder_history/
panel_selection/list_view_dialog/properties_runtime passed5/5 (2.52 s).

The built-in FileViewerDialog is a local bounded UTF-8 editor. Upstream
PanelItemOpen.cpp launches external viewer/editor applications; EditDialog.cpp
only presents read-only list values and has no reusable file-read/write editor
body. Therefore its captions/accessible names/validation notices use explicit
local EN/RU text; file content, pending cursor/selection/document-modified state,
save/cancel flow and QFile/QSaveFile operations are unchanged. Read-only error
presentation retains raw QFile error detail and translates the local context on
hot switch. Focused language_viewer/file_viewer passed2/2 (0.92 s), macOS arm64
Qt6.11.2. These remain partial .2 checks: local main-window extras/messages,
backend context, whole-app reachability/accelerators/long labels and full gate
are still required; no intermediate component acceptance/commit is claimed.

Additional .2 work binds the Qt-only File menu group (Open/Create/Add to open
archive, selected Extract, last-operation log, built-in text viewer/editor),
Back and the toolbar window title to explicit EN/RU text. These entries have no
corresponding native command/resource caption; native commands still use
MyLoadMenu. Language changes retain QAction identity, enabled state and shortcut.
The pure QtLocalText lookup now lives in qtlanguageservices rather than the
widget adapter, so backend presentation context does not depend on controls.
The nine local validateExtraction rejection messages now use this service;
its existing checks/control flow, raw paths, engine arguments and listing parser
are unchanged. Source ExtractCallback.cpp/ExtractingFilePath.cpp do not provide
these transport preflight captions. This text-only change does not claim a
source port of the legacy safePath/validateExtraction implementation; its
functional migration debt remains subject to its source owner.
Focused archive_backend/language_ui passed2/2 (1.08 s), macOS arm64 Qt6.11.2.
The new actual backend fixture checks EN→RU→EN empty destination and unsafe
Unicode/spaced path rejection with unmodified raw path and no extraction effect.
An initial test fixture omitted the on-disk archive and was rejected by the
snapshot identity guard; the corrected fixture now reaches the intended checks.
MainWindow EN/RU reports at1100×640/640×380 check address/table/toolbar exposure.
Other local main-window/operation/backend context, accelerators, long-label and
scroll reachability checks and final full gate remain open.

The local Archive information summary now binds its existing metadata snapshot
to EN/RU window/label text, with no re-list. It is an existing Qt/7zz integration
summary, not the source Properties dialog; no additional native metadata algorithm
is claimed. Open Archive picker/system-open error and the legacy built-in-editor
operation context now select local EN/RU text. Its changed-file confirmation
uses the existing source FormatUtils::MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE,
RelPath), matching PanelItemOpen.cpp's message owner; the existing workflow is
unchanged. language_main_dialog/ui_navigation/language_ui passed3/3 (7.22 s),
macOS arm64 Qt6.11.2. Initial new fixture used an unrelated positional ArchiveInfo
aggregate; explicit files/unpackedSize fields corrected it before the passing run.
Archive summary EN/RU420×240/300×180 reports expose the complete metadata label.
This remains partial evidence; other operation/backend captions and .2 acceptance
are still pending.

ArchiveBackend.cpp's local process-start/busy/preflight/transaction/exit context,
password-redaction marker and completion captions now select EN/RU through
qtlanguageservices. Native diagnostic owners retain source IDs; these captions
describe the separate CLI integration and do not invent resource IDs. Listing
tokens, process arguments, exit parsing and raw engine detail are unchanged.
MainWindow completion no longer recognizes cancellation by a Russian string:
an explicit HRESULT comes from the actual completion owner. The backend signal
passes lastResult; native batch/PanelCopy passes its own result; the Add progress
caller uses CProgressThreadVirt::Result after successful Create. Source Create
returns S_OK for successful modal creation, independently of the worker result.
This adaptation preserves upstream result semantics at the Qt completion bridge.
An initial backend-only result lookup incorrectly warned for source overwrite
Cancel; a sampled ui_operations stack identified that path. The owned hanging
test was terminated, then explicit source results fixed the regression.
Direct archive_backend/language_main_dialog/ui_archive_states/ui_navigation/
ui_operations passed5/5 (12.79 s), macOS arm64 Qt6.11.2. The context-menu test's
stale Russian local Add expectation now uses default English. Controlled fixtures
report E_ABORT explicitly for cancellation. Further actual backend EN→RU→EN
success/preflight/engine-exit context checks preserve the raw engine tail, and
actual canceled Create runs in both languages with E_ABORT/no output effect;
archive_backend passed1/1 (1.33 s). No full .2 gate or acceptance yet.

Further .2 consumers: ArchiveBackend creation/volume lifecycle and their Qt
workers now use local EN/RU context for creation/deletion/progress, collisions,
publication failures and cancellation. Native EnumDirItems/MultiOutStream/
CopyStream bodies, transactional paths, flags and result codes remain unchanged.
These captions describe CLI staging, not native compression-format logic.
Actual variable ZIP/7z volumes, protected/unprotected success, occupied preflight/
post-compression destination, compression/volume cancellation and pause-cancel-
join now run in both languages and verify exact completion/error context with
unchanged Unicode/spaced paths and output cleanup. Six direct suites passed6/6
(6.77 s): archive_volume_creation/archive_creation_lifecycle/ui_archive_volumes/
volume_operations/ui_operations/language_dialog, macOS arm64 Qt6.11.2.
The native PanelSplitFile.cpp source title IDs7303/7402 and source size/confirmation
IDs7304/7305/7306 now feed the existing MainWindow Split/Combine caller. Original
MyFormatNew/NumberToString formats the part count; modal buttons/decisions remain.

MainWindow opening uses source FileFolderPluginOpen.cpp's IDS_OPENNING3303;
hash continuation uses source PanelCrc.cpp's IDS_CHECKSUM_CALCULATING7500.
Journal result/error and local archive-move continuation context now select EN/RU
while retaining the raw backend message. An actual source Open cancellation
regression runs EN→RU→EN and observes selected-language status and journal.
Direct ui_archive_states/ui_navigation/ui_operations/language_main_dialog/
archive_progress passed5/5 (48.24 s). Initial fixture set the language before
MainWindow reloaded its settings; language is now selected after construction.
The legacy move-out assertion expected Russian and returned before dismissing
the modal error, causing the diagnostic timeout; it now checks English and
dismisses before asserting. No production workaround or timeout relaxation.
Other operation/worker/backend captions, scrolling reachability, fallback and
accelerator/long-label review plus the integrated full gate remain mandatory.

Split/Combine worker chain now selects EN/RU for the existing QFile/transaction
adapter context in mirrored PanelSplitFile.cpp and VolumeOperation. The source
worker/sequence control flow, payload bytes, stage/publication paths and QFile
error detail are unchanged; only existing local captions change. Original source
Combine IDs7404/7405 and MyFormatNew(IDS_FILE_EXIST3008,path) replace independently
written preflight wording. Other safety/transaction captions have no native
resource and use explicit local pairs. Formatting is applied separately from
these text bindings; no functional body reorganization is included.
MainWindow suppresses cancellation error presentation using the operation's
atomic isCanceled state; it does not recognize a translated message string.
The public getter is consumed by production completion handling, not added for
test access. Existing callback/cleanup lifetime is retained.
Worker success, collision/gap, cancel rollback, removed/grown input parts and
changed split source now run in EN/RU, assert raw Unicode paths/bytes and no
partial output. Actual MainWindow Split→Combine also verifies selected-language
completion. Seven direct suites passed7/7 (6.46 s), macOS arm64 Qt6.11.2:
volume_operations/volume_workflow/combine_names/split_destination/volume_dialog/
language_dialog/ui_operations. Stale Russian source-change/name expectations
were corrected before this passing run.
The added EN/RU100-part confirmation matrix covers dialog Cancel and Yes/No/
Cancel in normal1100×640 and compact640×380 main windows. Source message text
and applicable title are asserted; twelve current JSON reports show the Qt
managed400×116 confirmation with all labels/buttons exposed. The latest focused
volume_workflow passed1/1 (1.18 s); prior worker/workflow2/2 passed0.91 s.
QMessageBox intentionally ignores window titles on macOS, documented by
[Qt](https://doc.qt.io/qt-6/qmessagebox.html#setWindowTitle); tests assert this
platform behavior and retain the source title assertion on other platforms.
No rendered Windows reference or Windows/Linux runtime verification is claimed.
MainWindow archive-command captions now use the original source IDs: PanelCrc
IDS_CHECKSUM_CALCULATING/INFORMATION, FileFolderPluginOpen IDS_OPENNING,
GUI compression3301 and extraction/testing3300/3302; PanelCopy's CopyTo branch
retains IDS_COPYING6004. Local bounded viewer/temporary-directory/update-result
context uses explicit EN/RU pairs; source messages and raw paths are unchanged.
References remain the full pinned revision above, including PanelCopy.cpp's
title selection and ExtractGUI.cpp/UpdateGUI.cpp. No source functional body,
argument, selection, metadata or archive algorithm changes are included here.
The existing mirrored GUI/resource2.h remains authoritative; baseline verification
caught an attempted duplicate exact copy, which was removed before acceptance.
Baseline remains210 exact/152 adapted/3 local.
The archive preview-limit regression uses actual MainWindow selection and
17-MiB metadata: EN→RU→EN at1100/640 widths retains selection/backend counts,
shows the complete local diagnostic and writes four400×116 popup reports.
language_archive_command passed1/1 (0.53 s); six direct archive/window suites
passed6/6 (46.23 s); after the authoritative-header correction the three affected
language_archive_command/archive_progress/ui_operations suites passed3/3 (7.41 s).

File-transfer presentation now uses source PanelCopy.cpp's IDS_COPYING6004 and
IDS_MOVING6005 plus explicit EN/RU pairs for existing Qt staging/safety/error
context in FileTransferOperation and mirrored FSFolderCopy.cpp. The native
CopyTo/AskWrite/recursive/progress bodies, rollback/publication/delete order,
bytes and raw paths remain unchanged. Source hardcoded English filesystem
diagnostics retain their English fallback; Russian is a local presentation
adaptation because those original diagnostics have no language resource ID.
Formatting is accounted separately; existing large owner files are retained for
this focused caption change, without introducing a second responsibility.
FileTransferOperation marks the actual overwrite Cancel answer and exposes a
typed completion kind. MainWindow no longer compares cancellation text.
Fully rolled-back cancellation is Canceled; the existing partial-move warning
is Failure even after cancellation, preserving its visibility. Atomic completion
state is written before emitting the existing result signal and used in production.
Core-only LangUtils/ResourceString/qtlanguageservices and the original bundled
resource move to sevenzip_filesystem alongside CLang/file I/O; Qt widget bindings
remain in sevenzip_panel. This avoids adding a Widgets dependency or circular
panel dependency to filesystem workers and keeps each module authoritative.
language_file_transfer covers EN/RU success Copy/Move, raw Unicode/spaced paths,
overwrite Cancel/rollback, true failure and partial-move cancellation retaining
published bytes. The direct seven suites passed7/7 (6.77 s), macOS arm64Qt6.11.2:
language_file_transfer/file_transfer/file_transfer_direct/fs_folder_copy/
ui_operations/language_ui/language_dialog. Initial build used the wrong resource
header for6004/6005; the existing resource.h/resourceGui.h fixed the declarations.
Other archive-transfer/backend/integration messages and the whole-app .2 gate
remain open; none of these partial checks closes .2 or the parent.

Archive-to-archive local warning/result context now uses EN/RU with source
IDS_COPYING/IDS_MOVING. CApp's copied staged CopyTo→CopyFrom flow derives from
pinned upstream App.cpp::Copy (lines780–824); the existing delayed source-delete
and destination rollback adaptation is retained, with no transaction changes.
The actual ZIP Copy/Move/declined-delete test now runs EN/RU and checks selected
language completion, extracted bytes, source membership and exact rollback.

archivebackend_sourceupdate.cpp's existing CLI/staging validation, no-op and
error context now selects EN/RU via Core language services. Source Update.cpp/
Agent Update callback/ArchiveCommandLine.cpp decisions, stage ordering, process
arguments, safety checks and raw paths are unchanged. This presentation-only
change does not claim a source port of the remaining independently written
integration validation. The dedicated language_source_update scenario checks
actual ZIP update/no-op/cancel, unsafe add/protected-policy rejection and raw
Unicode/spaced missing source path, preserving input/output bytes and HRESULT.
The final five direct suites passed5/5 (7.44 s), macOS arm64Qt6.11.2:
language_source_update/source_update_backend/archive_backend/ui_operations/
language_ui. The first archive-transfer direct run caught a stale Russian nested
archive rejection assertion; it now checks exact English after dismissing the
message, with no production workaround. Core language loading and dialog direct
consumers separately passed language/language_main_dialog/language_archive_command
3/3 (52.38 s) after the library ownership move. The integrated .2 gate and
remaining mutation/worker context remain pending at that checkpoint.

Further source presentation port, at the same full baseline above:
`CPP/7zip/UI/GUI/CompressDialog.cpp::{AddSize_MB,AddSize_MB_id,
SetErrorMessage_MemUsage}` (source lines1030–1063) now retain their original
UString bodies, overflow-aware ceil-MB conversion, resource IDs, line order and
blank lines. The formatter declaration is exposed in the mirrored header so
the existing compression UI and ArchiveBackend preflight share one body. The
original usage-caption fallback in OnOK (IDS_MEM_REQUIRED_MEM_SIZE, then
IDT_COMPRESS_MEMORY with colon removal) is retained. Qt supplies the final
QString and QMessageBox; existing RAM calculations and rejection checks are
unchanged. This is compression preflight, not the mandatory actual decoder
RAM callback of P1-18.

The same module restores original `k_Auto_Prefix`/`Modify_Auto(AString&)`,
`Combo_AddDict2`'s AString text body and `Add_Size(AString&,UInt64)` (source
lines1616–1625,1825–1836,2390–2402). Narrow QString adapters convert only the
native ASCII size/count display. Win32 combo insertion/item data remain adapted
by the existing Qt callers; original B/KB/MB/GB and `*  ` now replace the local
Russian unit/Automatic formatter. All existing algorithm/choice omissions in
the prior CompressDialog audit remain; this does not claim the entire module.
Hot switch retains these language-independent numbers and item payloads.

`archivebackend_mutations.cpp` local validation/staging context now uses EN/RU;
password and invalid-volume errors reuse source3805/3806/7307. No validator,
argument, stage, safety guard or archive algorithm changed. Source-extract busy/
invalid-stream context and Qt filesystem mutation/association notices also use
local EN/RU, preserving raw engine/system text and Unicode paths. FSFolder's
native mutation bodies remain authoritative. ExtractGUI error popup title is
the original ShowErrorMessage's `7-Zip`; its original hardcoded invalid-output
diagnostic is retained. Progress AskCancel keeps Qt platform standard buttons,
original Yes/No/Cancel control flow, default Yes and Escape Cancel.

Actual creation preflight tests cover native password rejection, local invalid
method, zero RAM limit and pending input; source-extract rejection and occupied
Unicode filesystem targets retain bytes/HRESULT. The native full RAM message
is checked EN→RU→EN at both Add sizes; four current popup JSON reports show
English400×226/Russian400×256 with complete message/buttons. Association failure
checks EN/RU, plaintext raw path, E_FAIL and current400×116 JSON. Eight direct
suites passed8/8 (7.80 s): language_archive_command/language_source_update/
compression_limit/language_file_transfer/fs_folder/native_fs_operations/
source_extract_backend/external_launch, macOS arm64Qt6.11.2.

Dictionary/memory/solid tests now use geometry/JSON instead of unconditional
PNG captures. Current760×720/640×480 reports expose the affected combo fields,
source units and automatic prefix; scrolling off-viewport controls are covered
separately by reachability requirements. A fifteen-suite direct run passed11
and failed4 on stale Russian cancellation/Automatic expectations (28.85 s).
After fixture corrections, order/dictionary/solid passed in the four-suite run
(3 passed,1 failed;3.31 s); its last Deflate/Deflate64 Automatic expectation
was then corrected and compression_memory passed1/1 (1.76 s). Cancellation
checks now assert exact English and E_ABORT, with no production workaround.
The eleven passing suites include language UI/commands/update, compression limit/
general compression, flat view, list settings, auto refresh, navigation and
both progress consumers. These remain focused evidence, not a full .2 gate.

Main-window local read/scan/count captions and the editable address placeholder
now select EN/RU. `QtArchiveReOpen` uses source FileFolderPluginOpen's
IDS_OPENNING for the existing metadata reopening lifecycle; request/HRESULT/
transport state are unchanged. Language refresh now identifies only the source
parent row by its existing UInt32(-1) item index, preserving real directory
path tooltips. A regression checks directory row identity, path, selection and
address input/placeholder across EN→RU→EN. Whole-app reachability, accelerator/
long-label/fallback acceptance and the final full gate remain open; baseline
scope remains210 exact/152 adapted/3 local and no helper commit is made.

The copied `MyLoadMenu.cpp::OnMenuActivating` bookmark branch (source508–546)
now supplies its two UString loops through a narrow MyLoadBookmarks Qt entry
point. It retains IDS_BOOKMARK,100/50 long-path truncation, empty `-` and original
shortcut suffixes. Qt updates existing QAction objects instead of removing and
appending HMENU items; actual QKeySequence/dispatch stay intact, tooltip keeps
the full path as a local presentation extension. MainWindow's independent
Russian `— пусто` formatter is removed; hot switch invokes the same source body.
Unused disabled temp-bookmark block remains omitted as in upstream.

RootFolder's POSIX Volumes extension has no source resource ID and now uses
explicit local EN/RU instead of unconnected QCoreApplication::translate.
Windows FSDrives's Computer path restores its original LangString(IDS_COMPUTER)
expression. MainWindow hot switch invokes native CRootFolder::Init and projects
its names into existing items/native-name metadata by index, with sorting
temporarily suspended; it performs no filesystem enumeration/backend request/
folder binding. Existing root/drive workflow omissions remain unchanged.
The regression checks source root name, item identity/selection and980×620/
640×400 EN/RU JSON. Bookmark regression checks exact long-path truncation,
source captions and action/shortcut identity without backend calls.

Add reachability now exposes every named atomic control through the existing
scroll area in EN/RU at760×720/640×480, checks labels/button widths and the
persistent footer at every position. Bottom-position JSON inspected shows
selected-source list, Remove and preview/footer exposed; controls above that
viewport were checked at their own scroll positions. First added address test
expected the macOS `/var` alias rather than canonical `/private/var`; corrected
fixture passed language_ui1/1 (0.80 s). Add reachability passed1/1 (0.82 s),
then bookmarks/language_ui/main_shell passed3/3 (1.24 s). Final root_navigation/
volume_mount/bookmarks/language_ui/main_shell passed5/5 (1.20 s), macOS arm64
Qt6.11.2. An initial root projection build called the private concrete method;
it now uses the existing public IFolderFolder interface. Full .2 gate and
whole-app accelerator/fallback/long-label review remain pending.

The residual inventory also replaced initial auxiliary table captions with
existing GetNameOfProperty and explicit local Order, and bound the disabled POSIX
removable-drive explanation to EN/RU. Native column enumeration/settings and
FoldersPage state/apply bodies are unchanged. `language_ui/options_dialog/
panel_columns` passed3/3 (1.66 s), checking initial Path/Order, hot tooltip and
existing column persistence. The first build named an unregistered
column_layout_tests target and ran no tests; the registered panel_columns target
was built successfully before this check. Diff/format inspected on owned files.
Mnemonic inventory of current Add JSON finds the original RU `П` shared by Show
password and Parameters; EN has no such duplicate. Its Qt focus/cycling behavior
and form-label buddy review remain open rather than altering original assets or
claiming accelerator acceptance from a caption inventory.

## P1-14.1 — accepted Language/Options/main window integration (2026-10-01)

The integrated .1 scenario is accepted; whole-app .2 and parent remain open.
Source module baseline remains
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, https://github.com/ip7z/7zip.git.
Common/Lang.cpp is copied unchanged and built with the existing FileIO in
sevenzip_filesystem; Lang.h was already vendored. Signature/UTF-8/escape/id/offset,
1 MiB limit and binary lookup bodies are unchanged. LangPage.cpp/.h preserve
native CLangInfo/CObjectVector, enumeration/filtering, sort, locale marks,
missing/extra ID comparison, 50-line/1500-character info limits and apply flow.
Qt replaces the property page/combobox/info/error services. POSIX enumeration
uses upstream CEnumerator::Next(CDirEntry,found)/Fill_FileInfo in place of the
Windows Next(CFileInfo) overload. Help remains P1-15; LangPage.rc/Res.h are exact.

LangUtils.cpp/.h now preserve load-once, source two-attempt default-language
selection, reload/path checks and lookup/fallback bodies. Windows retains the
original kLangs/FindShortNames and system/user LANGID logic; QLocale supplies
primary/regional codes only on POSIX where Windows LANGID services do not exist;
QRecursiveMutex protects source language storage when read by current worker
consumers. RegistryUtils Read/SaveRegLang retain source CuString calls through
the existing explicit FM INI service. No source-parser duplicate is introduced.

Language data is not in the upstream Git source checkout. Exact original
Lang/en.ttt and Lang/ru.txt come from the official 26.03 Windows archive, listed
by https://www.7-zip.org/download.html and retrieved without executing the
installer. Release URL, archive SHA-256/size and individual resource SHA-256
are pinned in resources/7zip/provenance.json; original resource headers and
distribution License.txt are retained. These assets have a separate archive
baseline, not a fictitious path/revision in the source Git manifest. Qt bundles
the files and stages them in an owned temporary directory only when no installed
module/Lang directory exists; the unchanged CLang file reader consumes that
directory. This does not write into the installation or the upstream checkout.

MyLoadMenu.cpp now copies k_LangID_TopMenuItems, CIDLangPair/FindLangItem and
MyChangeMenu's recursive control flow, popup inference, timestamp/bookmark skips,
OpenInside */#/Benchmark2 and tab suffix logic. QList/QAction replace HMENU/item
services. The source reset-to-English LoadMenu operation is folded into each
lookup's compiled STRINGTABLE/original en.ttt fallback; captions change in place
to preserve QAction identity, shortcuts, enabled/check state and connections.
The old fixed Russian ResourceString override is removed. Source English tables
have precedence over en.ttt, matching resource fallback. en.ttt is read by the
unchanged CLang parser and retained immutable for concurrent fallback readers.

OptionsDialog uses the original six page IDs/order and active CLangPage, source
modified/apply state, with Qt property-sheet services. Invalid Settings memory
input retains pending language until a successful Apply. Cancel preserves a
previous successful Apply. Qt LanguageChange replaces the original after-sheet
menu/toolbar/ReloadLangItems notification: current top-level widgets receive the
event after the source language is applied. MainWindow updates source menus,
toolbar button resource IDs, existing property headers and both panels' selected
status captions without changing row/selection/column state. CApp::ReloadLangItems
and AddSizeValue/AddValuePair1/2/GetItemsInfoString retain upstream native UString,
IDs and formatting bodies; QString conversion remains only at Qt presentation
boundaries. PanelSplitFile reuses the same source summary helper.

Qt text bindings store source IDs/fallbacks on presentation objects; editable
values are never rewritten. Options Settings/Folders/Edit labels and accessible
names use source IDs; Edit viewer colon and Menu other-bitness caption retain
source formatting. Menu item hot refresh reuses its source formatting body while
preserving flags/current zone. Folders Browse title now uses its source ID;
Edit native file filter masks replace independently written Russian filter names.
Settings invalid-argument errors use existing upstream ErrorMsg, including its
documented POSIX English system-code text. SettingsPageRes.h is copied exact.

Current focused language/language_ui/options_dialog/external_editors_ui/
source_properties/panel_shell6/6 passed3.91 s on macOS arm64 Qt6.11.2; additional
language_ui1/1 passed0.62 s after partial-language menu regressions. Tests cover
source parser failures and escapes, missing/traversal/malformed fallback, actual
bundled RU, Apply/Cancel/restart, invalid Settings with pending language, action/
shortcut/selection/header/toolbar state and pending editor commands. Partial
language tests preserve timestamp/CRC/bookmark labels, */# and tab suffixes;
missing language resets formerly Russian menus to English. EN/RU Options JSON
at640×390/520×380 for all five available pages inspected: grouping/order, bounds,
checkbox text widths and wrapping memory label fit; Qt standard buttons retain
platform text. These checks preceded the final gate recorded below. Parent P1-14
stays open for .2: other dialogs/messages, local main-window extras/tooltips without
source IDs, backend message context and whole-app accelerator/long-label review.
Windows/Linux runtime and rendered Windows reference parity remain unverified.

The first full unsandboxed gate built/formatted successfully but failed26/163
(464.16 s) after removal of the fixed RU table: source labels invalidated old
caption assertions and modal Close responders; properties_tests was explicitly
terminated after its stale-date responder left the modal open. No acceptance.
Tests now select actual Russian explicitly where that is their fixture, using
ru.txt's `Сжатый`/`Изменен`/`Тип Ошибки`; other default/fallback tests assert exact
source English. Progress modal responders now recognize source English Close.
Focused consumers22/25 passed25.20 s before remaining captions were corrected.
The full gate also exposed compact two-panel size: Panel.cpp's icon-only
VIEW_PARENTFOLDER control must not show its longer translated menu text. Qt
standard parent/back arrow buttons adapt that shared platform image service;
QAction menu captions remain intact. QtStatusLabel elides constrained status
text, retains full native text/accessibility/copy value and exposes it in tooltip;
it replaces Win32's bounded status part rendering only. Corrected focused checks
panel_key/properties_runtime/language_ui/panel_shell4/4 passed7.63 s after these
changes; current640×400 widget JSON inspected, all status fields/arrow controls
visible. Source Russian Prefix1030 is `Путь` (same translation as Path1003),
not the former independently written `Префикс пути`; fixture follows original IDs.
Navigation button images survive QAction::changed during reload. These focused
checks preceded the final gate below; .2/parent remain open.

The corrected full gate had162/163 passing tests in57.28 s; its remaining failure was the
Progress text contract's old fixed-RU Close/Foreground/Continue states. Those
expectations now use exact source English while keeping the existing grouping,
visibility, pause/error/close and resizing contracts. All code consumers passed
in that run. No global Russian override was restored or behavior checks removed.
The final full gate below followed that contract correction.

Final evidence: unsandboxed `./scripts/check.sh` PASS163/16358.87 s on macOS arm64
Qt6.11.2, format/build and source inventory/baseline checks passed (207 exact,
152 adapted,3 local). After the final contract correction its five source fixtures
passed10.33 s; latest panel_key/properties_runtime/language_ui/panel_shell4/4
passed7.63 s. Worktree/staged SHA-256 of all three original release assets match
provenance. No changes followed the successful full gate except acceptance docs.
P1-14.1 closes as one integrated user scenario; parent24/46 unchanged. P1-14.2
remains mandatory for other forms/messages/local Qt captions, accelerator and
long-label matrix before whole-app acceptance. No Windows/Linux/runtime or
rendered Windows parity claim is made from these macOS checks.

## P1-13.1 — source MenuPage/Options/Zone scenario (2026-10-01)

Immutable baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git. Copied/adapted
`CPP/7zip/UI/FileManager/MenuPage.cpp/.h`: CContextMenuItem/kMenuItems,
Clear_MenuChanged, LoadLang_Spec, OnInit list/Zone/default/custom-value bodies,
OnApply BoolPair/Flags/Zone flow, OnButtonClicked, OnCommand and OnItemChanged.
Native UInt32 flags and UString/MyFormatNew are retained. Qt property-page,
list/combobox/check controls and item check-state comparison replace Win32
messages; `_initMode` prevents initialization from dirtying the page. Options
keeps pageID2300 and source caption 7-Zip, adds its modified/apply handler.
Edit is the accepted P1-08.1/.2/.3 implementation, not a new rewrite.

Exact dependencies: Explorer ContextMenuFlags.h, resource.h/resource2.rc,
FileManager/MenuPageRes.h and GUI/ExtractDialogRes.h. ResourceString/CMake compile
the original Explorer STRINGTABLE into the existing Qt fallback service;
BITMAP entry is not a shell rendering port. Formatting is separate from the
functional adaptations. Provenance paths are recorded in upstream-baseline.json.

Reviewed consumers: original Explorer QueryContextMenu reads Cascaded/MenuIcons/
ElimDup/Flags; this application's ContextMenu.cpp currently ports only FindExt.
Its DLL/registry/service consumer is mandatory P2-09/P1-13.2. CShellDll native
DLL discovery, Check/SetContextMenuHandler/requery/error branches are omitted
until that adapter exists. Registration, Cascaded/Icons/ElimDup and item flags
therefore remain disabled with capability reasons on every platform. They are
not connected to the FM File menu or ordinary Extract defaults. Source Menu
ElimDup configures shell extraction, not FM extraction. Help is P1-15 and language
file/retranslation services P1-14; current captions come from source English.

WriteZone already has the original PanelCopy::CopyTo → IFolderSetZoneIdMode/File
→ CAgentFolder/Agent::Extract → CArchiveExtractCallback::CloseFile consumer.
The file-only 7zz transport feeds bytes into that source callback, which writes
Windows Zone.Identifier before timestamps. Only Windows enables this control;
POSIX preserves stored values without exposing a nonfunctional operation.
The constructor accepts that presentation-service capability for deterministic
tests; it is selected by IsZonePropagationSupported in the actual Options owner.

Relevant migration debt fixed: the old CContextMenuInfo Qt persistence removed
unchanged BoolPairs, unlike source Key_Set_BoolPair. It now writes only Def=true
pairs, preserves unmodified flags and deletes the WriteZone override for UINT32(-1)
as source Key_Set_UInt32 does. This avoids losing shell preferences on Zone-only
Apply. Source bodies remain in their existing authoritative modules.

Focused menu_page/context_menu_settings/options_dialog/external_editors_ui/
source_extract_backend passed5/5 in1.87 s. Full unsandboxed check.sh formatting,
build and161/161 CTest passed57.27 s on macOS arm64 Qt6.11.2. Inspected fresh
options-menu-640/520 JSON: source checkbox order, separate Zone label/combo,
120/110px list and exposed footer, no checkbox text clipping at640×390/520×380.
Exact baseline verification passed203 exact/149 adapted/3 local paths from the
pinned read-only checkout. Windows ADS assertion is a conditional test branch,
not an executed macOS check. Windows/Linux runtime and rendered appearance
remain unverified. P1-13 parent remains open for P1-13.2/P2-09, full goal24/46
unchanged; next available P1-14.

## P1-12.1 — Settings/Folders, work-dir и обе панели (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git. Скопированы и адаптированы
`CPP/7zip/UI/FileManager/FoldersPage.cpp/.h`: CFoldersPage, m_WorkDirInfo,
GetWorkMode/MyEnableControls/GetWorkDir/ModifiedEvent/OnButtonClicked/
OnFoldersWorkButtonPath/OnApply. Сохранены native CInfo/mode order, init/needSave
и apply flow; Win32 property-page controls заменены Qt, EN_CHANGE — textChanged,
Browse — существующий MyBrowseForFolder. FoldersPageRes.h скопирован без изменений.
Source help/lang services ещё не подключены; owners P1-15/P1-14. Только Windows
WorkDir поддерживает source removable-drive branch; POSIX checkbox disabled,
stored value сохранён, native device integration остаётся P2-05.

Partial OptionsDialog подключает Folders на source page index2/ID2400,
modified-state и Apply. Source WorkDir::CreateTempFile получает FmSettingsPath
по умолчанию вместо пустого settings path, чтобы существующий source update
consumer реально использовал сохранённую рабочую папку. Explicit settings path
по-прежнему доступен deterministic fixtures; GetWorkDir algorithm не изменён.

`SettingsPage.cpp/.h` возвращают исходные _largePages_wasChanged,
_memx_wasChanged, EnableSpin, RAM GB rounding/range и OnApply branches
(original111–267,271–320,417–478), включая native UString/ConvertStringToUInt32,
порядок general→large pages→memory save, UINT32(-1) для unset и source max1<<30.
Zero принимается исходным validation, но следующее OnInit трактует его как
unset. Commented legacy MemLimit combo/UnderCE/help/lang paths не объявлены
перенесёнными. QtMemoryLimitSpin заменяет Win32 edit/up-down: raw text до Apply
не исправляется, стрелки ограничены вычисленным source RAM range. QMessageBox
и возврат PSNRET_INVALID заменяют ShowErrorMessage/property-sheet notification;
Options остаётся открытым и выбирает Settings/input focus при неверном вводе.

`FM.cpp::IsLargePageSupported` (original486–503) добавлен к существующим
WM_COMMAND/WM_CLOSE bodies без их удаления. Source Windows capability сохранена,
POSIX возвращает false; большие страницы в Qt Settings disabled там, где нет
этой Windows privilege capability. RegistryUtils::ReadLockMemoryEnable/
SaveLockMemoryEnable используют прежние default/defined branches и Options INI
вместо Windows base registry. Gui EnablePrivilege_LockMemory заменён backend
передачей `-slp` в официальный engine: `Common/ArchiveCommandLine.cpp1124–1245`
проверяет risk и включает privilege внутри child process. Allocator/privilege
engine здесь не переписаны; Windows runtime не проверен локально.

`GUI/UpdateGUI.cpp::ShowDialog` working-dir block529–539 скопирован в прежний
partial module и минимально извлечён в GetCompressionWorkingDir: возвращается
WorkingDir field вместо ещё отсутствующего CUpdateOptions. Mode/current branch,
MyGetFullPathName, GetWorkDir/CreateComplexDir сохранены. Backend Create stages
передают actual source path через `-w`; native Console/Common Update owns temporary
archive prefix, final publication остаётся в существующей backend transaction.
FM update использует единственный CWorkDirTempFile. Его build ownership перенесён
из sevenzip_location в sevenzip_filesystem, чтобы GUI create и Agent update
использовали один модуль без library cycle; filename/path/functional body те же.

`App.h::RefreshAllPanels` (original172–181) скопирован с NumPanels/
LastFocusedPanel/OnReload loop. Applied consumer перечитывает source list flags
и обе видимые панели. Qt reload presentation для archive теперь LoadItems/
projection/selected-state restore; filesystem сохраняет прежний scanner owner.

Focused options_dialog/work_dir/external_editors_ui3/3 PASS1.26 s macOS arm64
Qt6.11.2. Folders geometry/JSON640×390/520×350 просмотрены: source radio order,
path/Browse row, removable checkbox, отдельный footer без clipping.
Проверены Cancel/Apply-then-Cancel, invalid config defaults и actual Unicode
temporary file/cleanup. Final normal640×390/compact520×380 Folders/Settings JSON
проверены; compact height выросла из350 из-за actual Settings minimum361.
Focused8/8 PASS5.72 s (options_window/options_dialog/fm_settings/settings/
panel_auto_refresh/archive_opening/external_editors_ui/work_dir), поздние
options_dialog/archive_backend2/2 PASS1.03 s. Actual Create с Unicode work dir
и отказ occupied work path, archive_mutation cancellation consumers проверены.
Первый gate160/160 PASS56.85 s предшествовал GUI Create consumer. Следующий
gate159/160 выявил test focus assertion: offscreen hasFocus зависит от foreground,
но pending input widget в dialog.focusWidget правильный. Проверка input focus
исправлена; focused options_dialog1/1 PASS0.45 s. Final unsandboxed check.sh
formatting/build/160 из160 CTest PASS55.99 s, macOS arm64 Qt6.11.2,2026-10-01.
P1-12.1 принят; следующий доступный ID P1-13.
Parent P1-12 остаётся открыт до .2 и остальных page owners; три pages disabled.
Windows/Linux runtime/rendered Windows parity не проверены.

## P1-10.2 — полный Properties flow и доступная metadata (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git. В mirrored `FileManager/PanelMenu.cpp`
восстановлены `kSpecProps`, AddSeparatorSmall и Properties item/raw/folder/archive
level/non-open bodies (original100–420). Сохранены UString/AString/CRecordVector,
GetPropertyInfo/GetRawPropInfo loops, пропуск undefined/empty, CRC/checksum upper
hex до8 bytes, lower hex для остальных, security formatting, data:N после256
bytes, source totals и reverse level ordering с MainItemProperty sections.
FormatUtils/PropertyName/PropIDUtils/ListViewDialog/EditDialog остаются прежними
единственными source implementations. Source localized selected-count format
восстановлен вместо QString::arg; временные Russian resources для двух captions
используют существующий ResourceString adapter до language owner P1-14.

Agent.cpp/.h восстанавливают исходные IFolderProperties, IGetFolderArcProps и
IFolderArcProps, kFolderProps/GetFolderPropertyInfo/CRC и GetArcNumLevels/
GetArcProp/GetArcNumProps/GetArcPropInfo/MainItemProperty bodies
(original1232–1379,1892–2002). Body/paths/names сохранены. Две codec-index lookup
ветки ErrorType не имеют file-only service и возвращают VT_EMPTY; их P2-01 scope
сохранён, формат ошибки не выдумывается. GetTypeOfArc текущего one-handler owner
использует actual engine ArchiveType; in-process codec catalog не объявлен
перенесённым. Windows-only InvokeSystemCommand("properties") заменён общим Qt
Properties и native IFolder enumeration для файловой системы; shell integration
остаётся P2-09. Modal construction вынесена в уже существующий PropertiesDialog.

File-only adaptation: backend сохраняет исходные listing bytes отдельно от
trimmed/redacted journal. Reverse parser для `Console/List.cpp` technical output
в upstream отсутствует; Qt glue сохраняет фактически полученные item/header
fields и brace multiline headers. CRC возвращается typed UI4; размеры сохраняют
полный UInt64, absent/invalid остаются undefined, zero остаётся defined.
Остальные доступные CLI display fields возвращаются named BSTR без выдуманных
native типов; это сохраняет engine representation, включая его normalization.
Generic properties и header names могут оставаться английскими до language
integration. Пароли не печатаются новым adapter в журнал.

`Console/List.cpp::CFieldPrinter::PrintItemInfo` raw branch (590–647) допускает
полный hex только до64 bytes. QtPanelFolder raw service восстанавливает фактически
полученные Checksum/SHA-1/SHA-256 bytes и хранит их в immutable metadata owner;
missing item data возвращает empty raw. Реальный SHA-256 manifest проверяет
`Common/HashCalc.cpp::NHash::CHandler::{GetNumRawProps,GetRawPropInfo,GetRawProp}`
(1059–1109): checksum — kpidChecksum/NPropDataType::kRaw. Handler/decoder bodies
здесь не копируются и не реализуются, Hash workflow остаётся P2-03. `data:N`,
decoded security/reparse и большие raw payloads сохраняются только как actual
display text; bytes, которых CLI не отдаёт, не синтезируются. Native Properties
raw branch отдельно сохраняет исходные limits/formatting для полноценного provider.

CProxyArc2 decision: полные copied AgentProxy.cpp/.h bodies и BSTR lifetime fix
сохраняются с прежним provenance. Process transport не отдаёт GetParent/raw names/
ADS graph; checksum-only raw service возвращает E_NOTIMPL для parent и не
активирует proxy2. Source CProxyArc остаётся authoritative path tree и считает
explicit/implicit directory totals/CRC. Alternate-stream dispatch/binary graph
сохраняется обязательным P2-06; reparse/security capabilities — P2-05. Нет
claims полного Agent.cpp/ArchiveFolderOpen.cpp или in-process codec port.

`source_properties` проверяет actual ZIP CRC и Hash checksum bytes, UInt64 max,
defined zero/undefined, sparse SHA raw catalog, source folder/arc levels и
MainItemProperty/non-open errors, длинный Unicode header, полный clipboard/detail.
Properties consumers обновлены с item-only expectations на исходную folder
summary и native path naming; unavailable path не заменяется snapshot prefix.
UI presentation properties и actual engine properties хранятся раздельно:
formatted snapshot не расширяет native archive catalog. Unsupported CRC у UI
snapshot сохраняет E_NOTIMPL. Runtime fixtures различают item и folder sections
с одинаковыми source names. Первый full gate выявил эти три consumer проблемы
и TAR extraction failure: после сохранения blank values пустая Symbolic Link
ошибочно считалась ссылкой. Link flag теперь требует непустого actual value;
existing nested_archive TAR и unsafe archive_backend regressions проверяют
извлечение обычного файла и отказ для настоящих unsafe paths/links.
Focused5/5 PASS1.03 s; final SHA sparse1/1 PASS0.50 s, macOS arm64 Qt6.11.2,
2026-10-01. Geometry/JSON540×360/360×240 просмотрены, source ListViewDialog.rc
задаёт list/buttons; rendered Windows reference и Windows/Linux runtime
не проверены. После исправлений focused6/6 PASS2.04 s (panel_folder,
archive_property_catalog, properties_runtime, source_properties, nested_archive,
archive_backend). Повторный unsandboxed check.sh: formatting/build и158/158
CTest PASS55.35 s. P1-10.2 и parent P1-10 приняты в описанном file-only scope;
P2-01/P2-05/P2-06/P2-09/P2-10 остаются обязательными. Следующий ID P1-12.

## P1-10.1 — исходный F3 folder-size workflow (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
https://github.com/ip7z/7zip.git, `CPP/7zip/UI/FileManager/PanelItems.cpp::EditItem(bool)`
lines1043–1085. Восстановлен полный исходный body: !useEditor QI
IFolderCalcItemFullSize, Get_ItemIndices_Operated, обход выбранных папок,
CalcItemFullSize(index,NULL), needRefresh/InvalidateList/return; иначе focused
native index и исходная file branch. Контейнеры, порядок и игнорирование HRESULT
сохранены; F4 папки не запускает. Уже принятый `FSFolder.cpp::{CalcItemFullSize,
CFsFolderStat::Enumerate}` и FolderStat_Defined остаются единственным владельцем
размеров/числа объектов. Archive Agent не предоставляет CalcItemFullSize:
его размеры explicit/implicit папок уже вычисляет source CProxyArc, новый
параллельный calculator не добавлен.

Win32 InvalidateList заменён narrow Qt invalidation presentation: существующие
строки перечитывают source properties и numeric sorting data без LoadItems или
повторного binding, которые уничтожили бы FolderStat_Defined. Callback ограничен
активной панелью, сохраняет selection/focus, обновляет status/actions.
Проверка Qt availability теперь разрешает F3 для operated folders только при
реальном IFolderCalcItemFullSize; F4 по-прежнему требует focused file.
Новый coherent `mainwindow_propertyinvalidation.cpp` содержит только projection.
Расчёт сохраняет исходный синхронный NULL-progress flow; responsive large-tree
adaptation остаётся обязательным P2-10. Прежние symlink/reparse ограничения
FSFolderStat не объявляются решёнными этим шагом, owner P2-05.

`folder_size_view` использует реальный CFSFolder, keyboard F3, Unicode/space tree,
пустую папку (defined zero), несколько выбранных папок с фокусом на файле;
проверяет source totals, displayed sizes, неизменность provider, selection/focus
и отсутствие archive operations. Normal980×620/compact640×400 current-build
JSON и geometry просмотрены. Focused5/5 PASS1.41 s macOS arm64 Qt6.11.2,
2026-10-01. Первый gate обнаружил missing source registration у root_navigation;
Qt projection подключён автоматически ко всем targets с mainwindow.cpp.
Focused root_navigation/folder_size_view2/2 PASS0.59 s. Финальный unsandboxed
check.sh formatting/build/157 из157 CTest PASS54.89 s. P1-10.1 принят;
parent P1-10 не закрыт:
Properties/raw/folder/archive levels/CRC/full-value acceptance остаются .2.
Windows/Linux runtime и rendered Windows parity не проверены.

## P1-09 — обратная запись вложенной цепочки (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Переиспользованы уже скопированные mirrored
`FileManager/PanelItemOpen.cpp::{OpenAsArc,OpenParentArchiveFolder,OnOpenItemChanged}`,
`PanelFolderChange.cpp::{CloseOneLevel,OpenParentFolder}`, `Agent/Agent.cpp::CanUpdate`,
`ArchiveFolderOut.cpp::CommonUpdateOperation`, `AgentOut.cpp::UpdateOneFile` и
`UpdateCallbackAgent.cpp`. Parent link, CTempFileInfo, FileInfo change check,
индекс записи, cached password, Yes/No/Cancel, source ReOpen и удержание файла
при ошибке остаются исходными. Новый алгоритм parent chain не введён.

Необходимая file-only адаптация: исходный маленький CVirtFileSystem stream
материализуется через исходные COutFileStream/Create_NEW/CopyStream в тот же
FilePath, которым владеет CTempFileInfo. Раньше Qt manager записывал этот stream
в отдельный private spool, изменения которого source parent link не видел.
`IQtArchiveStreamOpenServices` передаёт признак владения через существующий
FileFolderPluginOpen worker; после успешного открытия OpenAsArc снимает IsVirtual
и фиксирует реальную FileInfo. QtArchiveFileResource не удаляет owned файл;
исходный parent link остаётся единственным владельцем cleanup. Неуспешный probe,
неверный пароль, cancellation и copy failure удаляют только собственную копию;
это сохраняет исходный Create_NEW fallback FlushToDisk. Generic unowned streams
продолжают использовать private spool. Цена адаптации — disk I/O для небольшого
вложенного архива, который upstream мог держать только в памяти.

Capabilities проверяет source IsThereReadOnlyFolder по каждому parent level;
engine UpdateEnabled service разрешает только текущие ZIP/7z. Qt nested-only
guards сняты у Add/Rename/Delete/CreateFolder/Move; encrypted Move и дополнительный
built-in text editor сохраняют прежние ограничения. Native F4 работает с cached
password. TAR/GZip, составной tar.gz и остальные handlers без update service
не перепаковываются независимым packer. Source No и Cancel отклоняют изменения
и удаляют temp; ошибка update сохраняет изменённый temp и внешний archive.

Agent/ArchiveFolderOpen.cpp::OpenFolderFile изучен: его in-process archive->Open
и codec/icon DLL services остаются ранее указанным P2-01/P3-05 scope. Здесь
используется уже принятый QtArchiveFolderManager typed transport и исходный Agent
owner, а не заявляется полный порт ArchiveFolderOpen.cpp или OpenArchive::Open2.

`nested_edit_chain` проверяет реальные Unicode/space inner7z/outerZIP bytes,
F4 и Rename/Delete/CreateFolder, protected оба уровня, ошибки/отмену каждой записи,
No/Escape parent prompt, wrong/cancel inner password, read-only parent/child TAR,
cleanup/retention и parent listing/focus. `external_launch` также проверяет
rejected memory probe → обычный external fallback. Focused11/11 PASS7.70 s,
macOS arm64 Qt6.11.2, 2026-10-01. Prompt geometry/default/escape и current-build
JSON проверены при owner980×620 и640×400. Windows/Linux runtime и rendered
Windows parity не проверены; source MessageBox establishing workflow не является
pixel reference. Первое полное выполнение выявило устаревшее memory-only
ожидание source_extract_backend; тест теперь проверяет bytes materialized file
и source cleanup. Повторный focused2/2 PASS4.35 s; финальный unsandboxed
check.sh format/build/156 из156 CTest PASS54.56 s. Baseline verify PASS197 exact,
145 adapted, 3 local. P1-09 принят; следующая обязательная итерация P1-10.

## P1-08.3 — VerCtrl source workflow (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Copied the complete FileManager/VerCtrl.cpp
into its mirrored path: ConvertPath_to_Ctrl, CFileDataInfo::Read, 256 MiB buffer
limit, ParseNumberString, CreateComplexDir_for_File, WriteFile, FILETIME helpers
and CApp::VerCtrl. Original CByteBuffer, comparisons, branch order, readonly
guards, `_7vc/<filename>/001...` history enumeration/rename, Commit precision
loop, Revert default-No overwrite and Diff order remain. Source WriteFile's
void/error and partial-write semantics remain; this is not an atomic version store.
Formatting is the repository clang-format adaptation, separate from behavior.

Windows uses original BY_HANDLE_FILE_INFORMATION/CInFile/COutFile/CEnumerator
and SetFileAttrib. New QtVerCtrl file services project only unavailable POSIX
Win32 methods: upstream FileIO fstat/read_full/write_full/create/time APIs,
upstream FILETIME conversions and POSIX enumeration's explicit found output.
POSIX preserves high permission bits; readonly removes write permissions, and
clearing readonly restores owner-write. macOS birth time supplies creation time;
other POSIX uses stat ctime, and upstream POSIX SetTime cannot promise Windows
creation-time restoration. mtime/atime use upstream FILETIME precision. Windows
CE/legacy presentation is outside the Qt-supported targets. CPanel error/parent
methods regain upstream public visibility for the source WriteFile helper.

Copied RegistryUtils ReadReg_VerCtrlPath uses source `7vc` key through shared
FM INI. Upstream has no UI writer for this advanced setting; no new setup dialog
is invented. CFileMenu remains the only owner of Diff/7vc/one-file/readonly
visibility, names and ordering. Qt builds appended actions, binds native IDs,
reads 7vc for program/context projections and refreshes source selection and
availability after execution. Released/unbound panels produce no projection;
direct-consumer tests exposed and fixed an initially unguarded refresh call.

`verctrl` fixtures cover two backup versions, readonly/mtime preservation,
exact hour rounding, controlled Diff argv, identical Edit and timestamp-only
Revert, Yes/No/Escape, missing backup, timestamp disagreement, unavailable backup
root, source buffer limit, absent configuration and multiple selection. Revert
uses original OverwriteDialog with default No and hidden extra buttons; normal
700×440 / compact560×440 JSON and main-window980×620 / 640×400 geometry reviewed.
Focused7/7 PASS2.32 s; final compact/default-No assertions pass1/1, 0.63 s.
Unsandboxed check.sh PASS155/155, 49.53 s, format/build/inventory PASS;
baseline verify PASS197 exact/145 adapted/3 local. P1-08.3 and parent P1-08
accepted with integrated .1 disk/settings/Diff and .2 archive process/update
consumers in that gate. No omitted VerCtrl functional branch;
Windows/Linux runtime and rendered Windows visual parity remain unverified.

## P1-08.2 — archive external-process integration (2026-10-01)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Copied PanelItemOpen.cpp CChildProcesses
(284–431) and known-process MyThreadFunction (1133–1328) into the authoritative
mirrored module. Original containers, sorted PID set, parent/same-name discovery,
firstPass, 60-handle limit, two-second fallback, changed/unchanged checks,
read-only error, Yes update, No/Cancel cleanup and failed-update retention remain.
Handles and Win32 wait/snapshot calls use narrow IQtProcessServices adapters;
CSnapshotProcess is exported to the service header because both boundaries need
its record. QElapsedTimer supplies the monotonic GetTickCount interval.

QtNativeProcessServices supplies OS operations only. Windows reuses unchanged
Windows/ProcessUtils.cpp CProcess::Create (same pinned source), with Unicode
build selection, Toolhelp snapshots and waitable process handles. Supported Qt
Windows versions use QueryFullProcessImageNameW rather than legacy dynamic PSAPI.
POSIX uses posix_spawnp with literal argv, waitpid for owned children, and process
identity stamps for opened PIDs (macOS libproc start time, Linux proc start time).
Closing a live owned POSIX handle leaves the app running and starts a PID-only
reaper; it does not retain UI/archive objects. No shell is involved. A fresh
process group retains child lineage after POSIX orphan reparenting; snapshot
ParentId projects that group to the source-known PID set. Descendants that
deliberately leave the group still require observed native parent/same-name
capability; a controlled child surviving its parent verifies inherited lineage
on the local macOS runtime. Windows/Linux runtime remains unverified.
Platform references: [POSIX spawn](https://www.man7.org/linux/man-pages/man3/posix_spawn.3p.html),
[wait](https://www.man7.org/linux/man-pages/man3/wait.3p.html), macOS SDK libproc.h /
sys/proc_info.h and [Apple libproc implementation](https://github.com/apple-oss-distributions/xnu/blob/main/libsyscall/wrappers/libproc/libproc.c).

QtExternalGuiCall projects MessageBox/SendMessage through a queued, cancellable
request. Pending or checking calls keep worker records alive; GUI close sets the
panel-owned exit flag before joining. A delivered modal call retains its record
through completion and uses existing CanCloseNow. MainWindow defers requests
until backend/operation/modal owners are free; runExternalArchiveUpdate marks
source progress/operation ownership, suppresses ordinary backend completions and
refreshes the edited panel with source selection before restoring the active one.
StartEditApplication now obtains a known handle for archive launches; disk
detached launch and no-handle association remain distinct. Archive F3/F4 are
connected, including source read-only edit/error semantics. Wait handles expose
Running/Exited/Error separately, preserving source wait-error retention instead
of interpreting failed OS waits as normal process exit.

Real fixtures cover ZIP Yes/No/Escape-Cancel, F3/F4, metadata/selection refresh,
active-process and pending-request window close, failed/cancelled update,
read-only TAR preservation, busy deferral, stale location and another focused
panel. Native lifetime fixtures cover orphan child discovery, short/long unchanged
cleanup, launch failure and injected wait failure. Extraction failure/cancel and
password cancellation leave originals intact. Protected ZIP AES and 7z headers
are checked with correct and wrong passwords after updates.

The ZIP regression exposed an existing CLI adapter omission: input password
delivery alone did not define output encryption for newly compressed data.
Inspected pinned ZipHandlerOut.cpp 392–430 (CryptoGetTextPassword2,
Password_Defined, inherited AES mode), ZipUpdate.cpp SetEncrypted and CLI password
callback. ArchiveBackend::updateSourceFile now adds bare `-p` for protected ZIP;
the password stays on stdin. Existing 7z inherited encryption is retained.
This is a transport adaptation, not a new format implementation. The fixture's
ZIP password is ASCII because upstream rejects non-ASCII ZIP passwords;
7z uses a Unicode/space password. Existing no-handle and direct consumers pass.
Prompt text/geometry JSON reviewed at normal/compact owner sizes 980×620/640×400;
all labels/buttons fit. Focused 8/8 PASS, 4.85 s macOS arm64 Qt6.11.2.
Unsandboxed check.sh PASS154/154, 48.27 s; format/build and inventory PASS.
Exact baseline verify PASS197 exact/144 adapted/3 local paths. P1-08.2 accepted;
P1-08.3 VerCtrl and parent acceptance remain mandatory. Old Windows CE branches,
debug traces and legacy runtime API lookup are omitted for Qt-supported systems;
the original functional discovery/wait/update bodies remain authoritative.
Windows/Linux runtime and rendered Windows visual parity unverified.

## P1-08.1 — integrated disk View/Edit/Diff (2026-10-01)

Same pinned baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, source repository
https://github.com/ip7z/7zip.git. Inspected FM PanelItemOpen.cpp helpers
StartAppWithParams/StartEditApplication/EditItem (697–833), CApp::DiffFiles,
known/child-process worker (1133–1328), EditPage.cpp/.h/.rc/EditPage2.rc,
OptionsDialog.cpp six-page order, RegistryUtils Diff keys, VerCtrl.cpp and
Common/TempFiles.cpp. Existing no-handle association and native update bodies
remain authoritative, rather than being replaced by the built-in text editor.

Copied disk CPanel::EditItem and StartEditApplication bodies into the mirrored
PanelItemOpen.cpp. Native UString, SplitCmdLineSmart, program/parameter ordering,
per-mode settings and Windows directory/notepad default remain. CProcess Create
is a narrow injected Qt service; StartAppWithParams sends original parsed base
parameters and separate path vector instead of serializing path argv on POSIX.
Windows service retains native parameter string and original quoted path loop;
POSIX QProcess splits base arguments and appends literal paths, without shell.
Disk launches detach as source disk CProcess destructor closes its handle without
waiting. This service is explicitly unsuitable for archive known-process editing.
POSIX has no upstream WindowsDir/notepad default: an unconfigured viewer uses the
OS association, while Edit requires configuration and reports source launch error.
RegistryUtils adds copied SaveRegDiff/ReadRegDiff through existing FM INI service.

Copied CApp::DiffFiles overloads preserve selected-pair order, one-selected-item
opposite-panel selection and relative fallback, including flat-to-normal basename.
Copied PanelItems EditItem(bool) focused-file branch uses the native selection
owner; GetFocusedFileIndex shares its guards with Qt availability. F3/F4 now
route to native disk EditItem; configured Diff uses native menu projection and
is hidden when its command is empty. The existing limited text viewer/editor
remains an explicit extra File menu feature without F3/F4 shortcuts.

New mirrored EditPage.cpp/.h retains CEditPageCtrl, three controls, OnInit,
WasChanged/OnCommand/OnApply and Browse flow. QLineEdit signals project EN_CHANGE;
CBrowseInfo replaces Win32 file picking. Browse replaces the whole command,
including discarding old parameters as upstream does. POSIX uses an all-file
application filter; Windows retains exe/all-files. EditPageRes.h is copied
unchanged. All use the same pinned baseline above and are in the manifest.

OptionsDialog.cpp is a partial P1-12 prerequisite: original six-page order and
IDs are retained, with Editor and existing Settings functional, four unavailable
pages disabled. QtFmOptionsDialog names the presentation container to avoid the
existing GUI COptionsDialog class. Qt property-sheet services supply modified
state/Apply/OK/Cancel and call each source OnApply. CSettingsPage can be embedded
without its standalone footer; MainWindow applies committed list flags immediately.
Help, System/Menu/Folders/Language bodies and Settings memory/large-pages branches
remain with P1-12/13/14/15/P2-08. This is not a full OptionsDialog source-port claim.

Still omitted: archive known/child-process wait and complete changed-temp flow
(P1-08.2); VerCtrl.cpp workflow (P1-08.3); source F3 selected-folder
CalcItemFullSize branch (P1-10). Archive F3/F4 and folder F3 remain disabled.
Source process and property dependencies must be ported before those consumers
are enabled; no-handle association does not validate them. Parent remains open.

Focused current-tree build and external_launch PASS 1/1, 0.73 s macOS arm64,
Qt 6.11.2. Tests cover native disk EditItem with separate Viewer/Editor settings
and exact source path vector, real owned test executable at Unicode/space path,
literal argv including `$()`/semicolon, launch failure and Diff key independence.
macOS Qt process argument encoding yields decomposed Unicode; semantic fixture
comparison uses NFC, while the native injected service test checks exact UString.
Integrated external_editors_ui checks Apply/Cancel/Browse, F3/F4 focused-file
launch, pair/opposite/fallback Diff and normal/flat paths, plus launch failure.
Editor options geometry and reviewed current-build JSON cover 640×390 and
520×350 against EditPage2.rc grouping/order. Final focused 9/9 PASS, 2.18 s;
unsandboxed check.sh 151/151 PASS, 45.16 s on macOS arm64 Qt 6.11.2.
Initial gate exposed binding_lifecycle's old archive F3 assertion; it now
checks extra text-view availability and disabled archive native F3 separately.
Inventory PASS; exact baseline verification: 196 exact, 144 adapted, 3 local
paths across the pinned revision. No Windows/Linux runtime or rendered Windows
parity verification. P1-08.1 accepted; parent P1-08 remains open.
Windows/Linux runtime remain unverified.

## P1-07.2 — source lifecycle integration (2026-10-01)

Baseline remains `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
https://github.com/ip7z/7zip.git. Inspected source `GUI/UpdateGUI.cpp`
flag projection (429–468), `GUI/CompressDialog.cpp` PreserveATime capability,
Options and OnOK bodies, `Common/ZipRegistry.cpp/.h` global PreserveATime
BoolPair persistence, `Common/ArchiveCommandLine.cpp` `-ssp`/`-ssw`/`-stl`,
and `Common/Update.cpp` latest-time/volume assignment (847–908) and
post-publication deletion (1853–1930).

The existing copied Options bodies now expose archive mtime. The source global
PreserveATime BoolPair and registry save/load calls are added through the existing
INI service. Qt request DTO fields preserve source names; the backend translates
lifecycle flags to the original CLI switches outside method properties. The
official engine calculates latest mtime for multi-root create; CMultiOutStream
receives that completed-output MTime/MTime_Defined before writing volumes, as in
Update.cpp. For existing Add, `Common/Update.cpp::Compress`'s latest-time loop
(805–845) is copied into `GetUpdateArchiveMTime`, retaining native pairs,
full-precision disk times and retained archive properties. AgentOut merges the
original update callback's LatestMTime after success (source 883–897), then
transports CFiTime to CWorkDirTempFile before native MoveToOriginal publication.
This also covers Freshen without new data. CopyStream transfers bytes only;
SetMTime forwards to the existing upstream COutFileStream service. Original
transaction, failure and reopen branches remain intact; formatting is separate.
AgentOut.cpp also projects source PreserveATime/ShareForWrite into the original
CArchiveUpdateCallback used for staging original inputs, before the CLI reads
its temporary copy. ApplyOutputMTime lives in qtarchivefileresource.cpp so native
callback tests do not acquire dependencies on the complete CLI update service.

PreserveATime is a Windows capability: pinned `Windows/FileIO.cpp::CInFile::Open`
only performs SetFileTime suppression under `_WIN32`. Real macOS `-ssp` accepted
the flag but changed atime. POSIX Options therefore disables it with a reason;
direct backend requests fail before engine launch. A persisted Windows setting
is retained in INI but excluded from POSIX effective requests. No independent
post-read timestamp restoration is introduced. Source Shared/Delete controls
are projected in CompressDialog.rc order, ahead of Encryption. Shared writing
is enabled only on Windows, where the native file opening service implements
sharing modes. Delete defaults off and is not persisted; source action-set
checks reject Update/Freshen/Synchronize with deletion before any engine launch.

`Common/Update.cpp/.h` is a new **partial port**, inventoried at the same pinned
revision. Copied bodies: UpdateArchive's action-set check (1147–1153), deletion
phase (1853–1930), CDirPathSortPair/processedItems/live Size/MTime/CTime guards,
deepest-first empty-directory cleanup, and the latest-time loop above. The
full Compress/UpdateArchive writer, rename/censor/codec/multivolume pipeline and
their earlier helpers remain omitted. Check/helper extraction and a narrow
IUpdateDeletionCallback replace access to the monolithic writer's local state;
callback method names and functional control flow remain original. Added
snapshot-size/null validation protects the adapter contract. Native deletion
ignores physical DeleteFileAlways/RemoveDirAlways_if_Empty failures, as upstream;
it does not delete skipped or changed files. No separate deletion algorithm.

`QtArchiveDeleteInputs` holds native snapshots/processed flags. Native Add
captures original GetStream completion flags and runs deletion only after
MoveToOriginal; existing F6 moveMode flow remains separate. Create enumerates
original inputs with authoritative EnumDirItems in a pause/cancel QThread before
CLI stages. Exit-zero completion of every stage and final single/volume
publication precede marking those scanned inputs processed and deleting them.
Nonzero/warnings, launch/publication failure or early cancellation preserve
sources. Cancellation during deletion can leave the published archive and a
partially deleted source set, matching source phase ordering. Create with links
rejects delete-after-compress until the P2 link owner is ported. EnumDirItems and
SortUtils move from sevenzip_location to sevenzip_filesystem ownership; their
bodies are unchanged and compile once. Qt worker/mutex/queued progress and the
backend prepare/delete jobs are integration glue, not borrowed algorithms.

Compression preflight retry/cancel follows copied CompressDialog checks and is
covered through the Add form. The remaining real **decoder** memory-request
transport is mandatory P1-18 (historical P1-07.2 scope); parent P1-07 acceptance
excludes only this explicitly separated function, not the rest of the plan.

### P1-18 prerequisite — actual RAR decoder memory callbacks

Historical prerequisite investigation (2026-10-01); the capability gap below
is superseded by P1-18 implementation. Its older checks do not validate new code.

Pinned `Archive/Rar/Rar5Handler.cpp::CHandler::Extract` is the actual caller of
IArchiveRequestMemoryUseCallback (3009 and 3145). Its dictionary/header/solid
sequence logic supplies native required/allowed sizes and flags. Our mirrored
Rar5Handler currently ports only BLAKE2sp registration; the real reader/decoder
is external. `Console/ExtractCallbackConsole.cpp::RequestMemoryUse` (556–619)
prints an English diagnostic and returns Allow/Skip locally; it has no GUI
request/answer protocol. Parsing that text or guessing from Method strings is
not a source port. Existing FM RequestMemoryUse/CMemDialog tests exercise a
synthetic callback only. Prerequisite: copied native RAR handler/reader, decoder,
crypto/stream dependencies and one authoritative registration, routed through
ArchiveBackend to the existing source GUI callback (or an explicit source-derived
structured engine bridge). Revisit with that capability and deterministic real
RAR5/RAR7 fixtures testing Allow/Skip/Cancel/remember on worker callbacks, including
root/selected/Test/batch consumers. P1-12 Settings persistence and P1-08–16 do
not need a running decoder callback; final P3 traceability/omitted closure do.

Current-tree macOS arm64/Qt 6.11.2 focused checks (2026-10-01): 11/11 PASS,
3.97 s (creation lifecycle, update callback, volume creation/UI, advanced,
compression limit/memory/preferences, source update, UI update, extract prompt).
The earlier 149-suite gate belongs to the preliminary timestamp integration;
final unsandboxed check.sh passes formatting/build and 150/150 CTest (42.21 s).
The first final gate had 149/150 PASS (42.92 s), with only inventory rejecting
new files absent from Git index; staging them corrected inventory/baseline (2/2,
0.54 s), and the complete rerun passed. Current-build
Options/Add/lifecycle and memory-limit JSON at normal/compact sizes is reviewed;
Windows/Linux runtime and rendered Windows reference remain unverified.

## P1-07.1 — source Advanced Options и параметры (2026-10-01)

Baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Copied the complete OPTIONS section of `CPP/7zip/UI/GUI/CompressDialog.cpp`:
`COptionsDialog` Bool1/BoolBox helpers, precision construction, time capability
states, `OnInit/OnCommand/OnButtonClicked/OnOK`, and the original
`CCompressDialog::ShowOptionsString` helpers/body. Original names, native
CBoolPair/UString/CFormatOptions and control flow remain in the mirrored CPP.
`CompressDialog.h` receives original CBool1 and CInfo property fields/defaults.
The adapted COptionsDialog declaration is in `qtcompressoptionsdialog.h` because
the mirrored DTO header also serves Core-only engine consumers. This avoids
adding a Widgets dependency to those consumers; it is not a second implementation.

Copied `GUI/UpdateGUI.cpp`'s entire property-helper block: `AddProp_*`,
`SplitOptionsToStrings`, `IsThereMethodOverride`, `ParseAndAddPropertires`,
`SetOutProperties`. `GetCompressionProperties` is the narrow adapter for the
original ShowDialog call order. Native CObjectVector<CProperty> is projected
to individual `-m` argv entries by ArchiveBackend, replacing its independent
property construction. The existing QString encryption field needs a conversion
overload and `isEmpty`; other added fields retain native types. Widgets do not
parse parameters or construct engine switches. `UpdateGUI` and its original
UpdateAction dependency now compile once in sevenzip_panel so backend consumers
and sevenzip_location share the same authoritative body.

`Common/ZipRegistry.h/.cpp` gains original Options/TimePrec/CBoolPair profile
fields and their Save/Load order. Native registry DWORD operations use QSettings;
undefined pairs remove the key. The GUI resource header is an exact unchanged
copy at `third_party/7zip/CPP/7zip/UI/GUI/CompressDialogRes.h`, inventoried in the
pinned manifest. Source notices and license remain; formatting is clang-format.

`qtcompressoptionscontrols.*` supplies Qt controls and original control service
names; `createarchivedialog_advanced.cpp` projects pinned `7zRegister.cpp` and
`ZipRegister.cpp` time flags into the original CArcInfoEx owner. Qt applies
OnInit visibility before calculating minimum size. Resources
`CompressOptionsDialog.rc` and `CompressDialog.rc` establish grouping/order.
Options OK updates a temporary per-format profile; Cancel discards it, and Add
Cancel never writes INI. The source summary accompanies the Options button.

Partial scope remains explicit: no complete CompressDialog/ShowDialog/UpdateGUI
port is claimed. Shared writing, preserve access time, archive mtime,
delete-after-compress and memory retry/cancel remain P1-07.2; unsupported lifecycle
controls are disabled and SetArcMTime is rejected by the adapter. NTFS/link flags
are unsupported in this owner and hidden by native capability policy; their
functional owners remain P2-05. Retained tar/gzip Options branches are not exposed
by the current ZIP/7z Add form. Help remains disabled pending P1-15.

`compression_advanced` checks normal/compact geometry, source capability states,
Cancel and per-format persistence, method override, real ZIP/7z mtime precision
round-trips, omitted mtime, invalid/unsupported properties and cancellation with
no partial output. DOS rounding follows source TimeUtils on POSIX; Windows native
rounding is not locally verified. Focused seven-suite gate passed on macOS arm64,
Qt 6.11.2 (7/7, 4.06 s). Full gate evidence is recorded in the checklist.

## P1-05.4 — directory callback plans и приёмка Add (2026-10-01)

Baseline remains [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reviewed source bodies: `CPP/7zip/UI/Agent/AgentOut.cpp::EnumerateArchiveItems`,
`CAgent::DoOperation`, `CPP/7zip/UI/Common/UpdatePair.cpp::GetUpdatePairInfoList`,
`UpdateProduce.cpp::UpdateProduce`, `EnumDirItems.cpp::EnumerateItems`, and
`CPP/Common/Wildcard.cpp::CItem::CheckPath`. Their previously copied bodies and
module boundaries are retained; this iteration changes only the local
`qtarchivesourceupdate.cpp` and `ArchiveBackend` process adapters.

The callback may omit an explicit directory record while retaining a child.
The ordinary Delete recognizer now declines that shape, so it cannot silently
route Synchronize to recursive CLI deletion. The source-plan recognizer carries
removed directory records and retained original paths separately. The backend
deletes omitted files first, then omitted directories deepest first with exact
retained-descendant exclusions. The official CLI has no item-index deletion
interface; these stages adapt its path matching to the authoritative callback
plan without extracting/recompressing retained data. All stages use the existing
transaction copy, password, progress, cancellation and publish boundary.

Regressions exercise the source Agent's Full-path Synchronize on ZIP/7z with
Unicode/space directories, an older retained disk child and omitted directory
records. Direct backend cases retain an explicit subtree while removing its
unwanted nested directory, check byte contents, reject unsafe exclusions and
cancel during the second deletion stage with exact-original rollback.
No visible controls or layout change in this iteration. P1-05.1–.3 retain their
historical evidence; advanced options/formats, nested update, links/streams and
absolute extraction remain owned by P1-07/P2-01/P1-09/P2-05/P2-06/P2-07.
Duplicate/case-folded archive names and noncanonical unsafe paths remain rejected
by the integration guard; their broader source closure is reviewed in P3-05.
Windows/Linux runtime and rendered Windows appearance remain unverified.

Validation on macOS arm64/Qt 6.11.2, 2026-10-01: affected targets built;
unsandboxed focused `source_update_backend`, `ui_operations`, `ui_update_modes`,
`command_policy_runtime`, `filesystem_selection_runtime`, `binding_lifecycle`,
`panel_runtime` passed 7/7 (16.00 s); full unsandboxed `./scripts/check.sh`
passed formatting/build and 146/146 CTest (61.96 s). Fresh Add JSON reports
760×720/640×480 retain fully visible four-mode and three-path combos and footer.
Initial validation exposed slow startup sorting of build outputs and of the
shared temporary parent after a fixture's saved panel path disappeared; those
runs were stopped after a timeout. CMake now isolates current and temporary
directories per MainWindow suite. This is a test integration adaptation;
application navigation and sorting remain unchanged. Parent P1-05 is accepted
for the documented top-level ZIP/7z scope; the listed platform checks remain.

## P1-05.3 — защищённый Add в существующий архив (2026-10-01)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original paths and symbols: `CPP/7zip/UI/FileManager/PanelCopy.cpp`
`CPanel::CopyFrom`, `CPP/7zip/UI/Agent/AgentOut.cpp`
`CAgent::DoOperation`, `CPP/7zip/UI/Agent/UpdateCallbackAgent.cpp`
`CUpdateCallbackAgent::CryptoGetTextPassword2`, and
`CPP/7zip/UI/GUI/UpdateGUI.cpp` `FindUpdateMode`.
The source panel and Agent still produce the Add action plan and forward the
opened archive password through the original update callback. No functional
source body is replaced in this iteration.

The local `QtArchiveFileResource` callback adapter marks a plan protected
only when its callback supplies a password. `ArchiveBackend` keeps its
transactional copy and rejects an unmarked password-bearing plan. For 7z it
lets the existing archive's encryption and header mode carry through the
`7zz` update; explicitly passing `-p` would trigger a second password prompt
after opening the protected header, while `-mhe=off` would change the header
mode. For ZIP AES, `-p` is retained so added items are encrypted; the engine
retains the existing AES method. These switches and process details have no
portable upstream counterpart because the project uses the official CLI
engine as its update transport.

Qt command availability permits Add on a writable protected top-level
archive, and the dialog reuses its opened password without requesting it
again. Direct source tests verify encrypted old and Unicode new file bytes,
ZIP AES, 7z encrypted headers, wrong-password failure and cancellation
rollback. The UI test verifies one password prompt and successful protected
Add. The existing-target dialog still does not offer a separate password or
encryption-method control; it preserves the opened archive's protection.
Directory filtering with a retained descendant and remaining Add branches
are P1-05.4. Windows/Linux runtime and rendered Windows appearance remain
unverified. On macOS arm64/Qt 6.11.2, affected targets built, focused
`source_update_backend`, `ui_update_modes`, `ui_operations`, `app_state` and
`command_policy_runtime` passed after updating the policy expectation, and
the full unsandboxed `./scripts/check.sh` passed on 2026-10-01.

## P1-05.2 — режимы путей нового и существующего архива (2026-10-01)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original paths and symbols: `CPP/Common/Wildcard.cpp::CCensor::{AddItem,
AddPathsToCensor}`, `CPP/7zip/UI/Common/Update.cpp::UpdateArchive`,
`EnumDirItems.cpp::EnumerateItems`, `CPP/7zip/UI/Agent/AgentOut.cpp::
CAgent::DoOperation`, `ArchiveFolderOut.cpp::CAgentFolder::CommonUpdateOperation`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::OnInit/OnOK` and
`CompressDialog.rc::IDC_COMPRESS_PATH_MODE`.

The unchanged upstream `CCensor::AddPathsToCensor` and already adapted source
`EnumerateItems` decide logical names: Relative drops the selected item's
parent, Full retains the path without its volume/root, and Absolute retains
the absolute name. A narrow GUI-only branch in `CAgent::DoOperation` now calls
that original enumeration body; the existing `EnumerateItems2` route remains
for other callers. `CAgentFolder::UpdateFromPaths` supplies the absolute
selection and scoped path mode through local `IArchiveFolderInternal`, while
the original pair/action/callback operation remains the authority for update
decisions. Qt's `CreateArchiveDialog` adds the source path-mode choices and
passes the selection through `CPanel::CopyFrom`.

`ArchiveBackend` is platform integration: for a new archive it stages selected
parents separately for Relative, the filesystem root for Full, and absolute
names via `7zz -spf` for Absolute. The temporary archive is published only
after every stage succeeds. The existing-archive callback adapter stages
absolute item streams under safe temporary aliases, then asks the backend to
rename those names to their original absolute names after delete/add. The
backend validates both aliases and requested names, uses `-spf` for absolute
delete/rename, and keeps all mutations in the temporary archive. These
process and validation steps have no reusable upstream implementation because
the project bridges the original in-process callback to `7zz`.

Port scope: ZIP and 7z, new and existing archives, files and simple directory
trees including empty directories. `FindSourceUpdatePlan` still declines an
omitted explicit directory that contains an otherwise kept child; the CLI
`7zz d` command removes that child too. Protected Add is covered by
P1-05.3. Absolute archive entries are listed but the current safe extraction
path rejects them; extraction policy belongs to
P2-07. Full path root/UNC behavior needs Windows runtime validation. These
directory branches belong to P1-05.4 before parent acceptance.
`clang-format` affects only the edited portions of mirrored C++ files; source
notices remain. The baseline inventory now lists the two `UpdateGUI` files
introduced in P1-05.1. On macOS arm64/Qt 6.11.2 the full unsandboxed gate
passed formatting, build and 145/146 CTest cases; only `upstream_inventory`
failed because the prior commit omitted those two entries. After this JSON
correction, focused `upstream_inventory` and `upstream_baseline` passed.
Focused `source_update_backend`, `ui_update_modes` and `compression` also
passed. Windows/Linux runtime and rendered Windows appearance are unverified.

## P1-05.1 — существующий архив: update modes и compression (2026-10-01)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Original paths and symbols: `CPP/7zip/UI/GUI/UpdateGUI.cpp::g_UpdateMode_Pairs`/
`FindUpdateMode` and `UpdateGUI`, `CompressDialog.h::NUpdateMode`/
`CInfo`, `CompressDialog.cpp::k_UpdateMode_Vals`/`OnInit`/`OnOK`,
`CompressDialog.rc::IDC_COMPRESS_UPDATE_MODE`,
`CPP/7zip/UI/Agent/AgentOut.cpp::CAgent::DoOperation` and
`ArchiveFolderOut.cpp::CAgentFolder::CommonUpdateOperation`,
`CPP/7zip/UI/Common/UpdateAction.cpp`, `UpdatePair.cpp`, `UpdateProduce.cpp`.
The exact Common action sets and produce algorithm were already present; the
mirrored Agent operation retains the source pair calculation and callback.

`src/CPP/7zip/UI/GUI/UpdateGUI.cpp` copies the four original mode/action-set
pairs and lookup loop, omitting the unrelated Win32 update dialog and command
line setup. `CompressDialog.h` now carries the original enum and info defaults.
Qt `CreateArchiveDialog` shows the original four modes and existing compression
controls for an existing target; `CPanel::CopyFrom` passes the selected source
action set through local `IArchiveFolderInternal::UpdateFrom` to a narrow
`CAgentFolder` adapter, which calls original `SetFiles` and
`CommonUpdateOperation`. The interface keeps the portable panel library free
of a link dependency on the concrete Agent. `QtArchiveFileResource` carries the
selected options through this source callback route. The resource's local
callback-plan adapter reads source keep/new/remove decisions, stages the native
streams and timestamps, and asks `ArchiveBackend` to update a separate copy
using `7zz` delete/add stages. The backend owns validation, process switches,
transaction failure/cancel cleanup and compression option checks; Qt widgets
never build engine commands. The create/archive compression switch builder was
extracted without changing its validation. The selected archive resource is
kept alive through the synchronous source worker and refresh.

This is a partial `UpdateGUI.cpp`/`CompressDialog` port: the verified scope is
existing, top-level ZIP/7z files with relative names. Path modes, directory
edge cases and encrypted Add need P1-05.2; the larger Win32 dialog body and
format options unrelated to this workflow are omitted. Source callback plans
unsupported by the file-only `7zz` adapter still fail explicitly. The dialog
and plan backend reject protected Add until encryption preservation is
implemented; the pre-existing source F5 protected Add route is unchanged. Required
`clang-format` affects presentation of borrowed snippets only. Source notices
and LGPL reference remain in the mirrored files. macOS arm64 byte-level tests
cover four modes on ZIP and 7z, Unicode/staged replacement, rollback,
compression method and UI route; Windows/Linux and rendered Windows appearance
are unverified.
The older controlled-backend `ui_operations` fixture now checks the Add
dialog's Cancel path and mode availability; its separate drag/drop test still
checks `addSources`. The dialog's real source route is covered in
`ui_update_modes`.
Focused `source_update_backend`, `compression`, `ui_update_modes`, `app_state`
and `panel_folder` passed. The full unsandboxed `./scripts/check.sh` passed
146/146 CTest on macOS arm64/Qt 6.11.2, 2026-10-01.

## P1-03.1 — filesystem CreateFolder/CreateFile/inline Rename (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CThreadFolderOperations`,
`IsCorrectFsName`, `CPanel::CorrectFsPath`, `CreateFolder`, `CreateFile`,
`OnEndLabelEdit`, `RenameFile`; `BrowseDialog.cpp::CorrectFsPath`; already
accepted `FSFolder.cpp::CFSFolder::CreateFolder/CreateFile/Rename` and
`ComboDialog.cpp`.

The mirrored `PanelOperations.cpp` copies source operation enum, worker dispatch,
`CUpdateCallback100Imp` lifecycle, name validation and source path correction.
The `CREATE_FILE` worker branch is a local adaptation: upstream calls
`IFolderOperations::CreateFile` directly, while this Qt port keeps filesystem
mutations on the source progress thread. Source `OnEndLabelEdit` label control,
real-index/selection/reload sequence is adapted to `QTableWidget`: F2 begins
inline editing in all four view modes, Qt commits on Enter and cancels on Escape,
and `MainWindow`
reloads and selects the resulting path. `CreateFolder`/`CreateFile` use the
ported `ComboDialog` presentation. The path name is passed to source
`IFolderOperations`; creation of nested directories and existing-parent file
paths comes from the original FSFolder body. The Windows `CorrectFsPath` loop
is copied from BrowseDialog, and its POSIX passthrough branch is retained.
Qt replaces Win32 list control, HWND, messages and presentation state;
`CheckBeforeUpdate`, source symbols and module boundaries remain. Required
`clang-format` changed formatting, not functional bodies.

This is a partial `PanelOperations.cpp` port: `DeleteItems`, recycle-bin shell
path and confirmation, archive changes and `ChangeComment` remain P1-03.2/
P1-04. No source label-edit control or Windows shell recycle implementation
is claimed on macOS/Linux. A directly selected symlink remains rejected by
the existing Qt command policy; Windows and Linux runtime and rendered
Windows inline appearance remain unverified.

## P1-03.2 — filesystem Delete and native Trash (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CPanel::DeleteItems`,
`CThreadFolderOperations::ProcessVirt`/`DoOperation`, the existing
`FSFolder.cpp::CFSFolder::Delete`, and
`MyLoadMenu.cpp::WndProc` (`IDM_DELETE`, `!IsKeyDown(VK_SHIFT)`).
The mirrored `DeleteItems` retains the source timer guard, operated-index
collection, selected-state snapshot, single file/folder/multiple prompt choice,
`CDisableNotify`, `FOLDER_TYPE_DELETE` dispatch, title refresh and partial
result semantics. `MyLoadMenu` still supplies the bool for recycle versus
permanent deletion; Qt's list key adapter now sends Shift+Delete through that
same source command switch.

The original `SHFileOperationW` Windows shell call is replaced by the narrow
`IQtTrashService` boundary. Its production implementation delegates each
selected path to `QFile::moveToTrash`, which uses the platform desktop's
Trash/Recycle Bin. Qt does not present the source shell confirmation, so the
source type/count prompt is used before both routes. The source permanent
worker and `CUpdateCallback100Imp` run unchanged at the operation boundary;
`CFSFolder::CheckBreak` is connected to its progress `CheckStop` for recursive
cancellation. Qt reloads `FSFolder` and restores source selection after every
accepted result, including partial failure/cancel; this replaces Win32 list
refresh. The independent application delete worker was removed. Its test-only
driver moved under `tests/` to retain deterministic FSFolder callback,
recursive-cancel and partial-failure regressions. Directly selected symlinks
remain rejected; nested links are unlinked without following their targets.
Required `clang-format` formatting is separate from functional adaptation.

Only filesystem deletion is included. Archive deletion remains P1-04.
macOS test UI uses an injected Trash adapter to avoid modifying the user's
actual Trash; native service behavior and Windows/Linux runtime remain
unverified locally. No Win32 long-path shell limitation is claimed for Qt.
Unix permission-denied CreateFile/Rename/Delete and occupied inline Rename
are covered by focused regressions; the full macOS arm64 gate passed 137/137
CTest cases on 2026-09-30.

## P0-03 — integrated command policy/dispatch (accepted, 2026-09-29)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
https://github.com/ip7z/7zip. `MyLoadMenu.cpp/.h::CFileMenu::Load` and its fields,
constructor, g_Zvc_IDs/Strings are copied under original paths. The complete
functional loop retains context/program distinction, Diff/Benchmark filtering,
Split/Combine file eligibility, read-only and hash-folder switches, link count,
alt-stream support, compact disabled/separator omission, insert counters,
version-control file attributes/size checks and trailing-separator removal.
`IQtFileMenuServices` replaces source/destination CMenu, popup copying, Registry
settings and screen-size check. Item IDs/UString remain upstream; native portable
CFileInfo remains the file inspector. Source language/resource menu construction
remains mandatory P0-09/P1-14; the command switches are connected below.

`PanelMenu.cpp::CPanel::CreateFileMenu` copies operated-index collection,
directory classification, CFileMenu population, alt-stream capability query and
Load. Win32 shell/7-Zip COM menus remain P0-09/P2-09, disabled until implemented;
current Qt command items use the source-owned policy. `Panel.h` IO-FS/pure-drives
predicates and IsFolder_with_FsItems retain original FS/network branches (the
latter moves to Panel.cpp to contain its FileName/StringConvert dependency).
FindDir_InOperatedList retains the index loop but queries existing source
IsItem_Folder: full RefreshListCtrl directory cache loading remains P0-05.

`Agent.cpp::CAgentFolder::GetFolderProperty` ReadOnly/IsHash branches and
`Agent.h::Is_Attrib_ReadOnly`, `IsThere_ReadOnlyArc` are ported for this consumer.
Original long-path/attribute/read-only arc ordering and typed VARIANT_BOOL remain.
The source codec catalog FormatIndex/UpdateEnabled predicate is supplied by
ArchiveUpdateEnabled querying the metadata owner's actual IOutArchive capability;
the external engine transport has no in-process codec index. Complete catalog/
parser capabilities remain P2-01; this adaptation does not infer update support
from filename or rewrite compression. Portable CFileInfo.GetWinAttrib initializes
original _attrib at the current Init/Open bridge. Win32 INVALID_FILE_ATTRIBUTES
is DWORD(-1) on platforms without that header constant. Actual read-only file,
long-path change, read-only arc, absent/present writer and hash-property regression
fixtures are isolated; no engine command is invoked in capability tests.

Policy focused test passed 1/1 (0.28 s); agent_capability/agent_folder/
archive_opening/file_menu_policy passed 4/4 (1.31 s), macOS arm64 2026-09-29.
ExecuteFileCommand/OnMenuCommand bodies are now copied with original switch IDs,
method names/arguments, bookmark/time ranges and status refresh ordering.
IQtFileManagerCommandServices and its menu extension replace the global CApp's
platform/presentation entry points. `FM.cpp` WM_COMMAND and toolbar ExecuteCommand
retain original toolbar IDs and two-panel timer guards. `CApp::ExecuteCommand`
owns the entry, rejects a closed lifecycle, and protects synchronous native
callers with the P0-02 operation CallScope. Exit is deliberately outside that
scope so the source WM_CLOSE lifecycle can run. `QtFileManagerCommands` attaches
all existing File/Edit/View/Favorites actions, toolbar, context-menu shared
actions, panel activation and bookmark/selection shortcuts. Qt extensions use
IDs 1000–1006, below original plugin start 1100. Action enabled/check state is a
projection; execution recomputes the native policy even for a stale enabled
QAction. The former refreshActions matrix and independent callbacks are removed.
QtFileMenuProjection owns no widget or HMENU and uses CPanel::CreateFileMenu;
transport restrictions (busy/scan, nested/encrypted updates, currently supported
engine updates and current selected-only Copy/Move consumers) are local to the
application adapter. Full Copy/Move smart/all-items semantics remain P1-03.

`PanelMenu.cpp::CheckBeforeUpdate` and `Panel.h::DoesItSupportOperations` retain
the original interface test, reverse current/parent read-only chain and localized
error construction. Current Rename/Move/Delete/Create consumers call that source
preflight. Complete mutation workflow bodies remain P1-01/P1-03/P1-04; current
copy/edit/extract/hash transports are not declared fully source-ported here.
Unsupported Diff/version/link/streams/plugins/help/benchmark/second-panel/time
consumers have no available registry entry and cannot dispatch placeholders.

The original read-only policy exposed a missing provider prerequisite:
`CFSFolder` had no IFolderOperations. The native interface now copies source
GetAbsPath/CreateFolder/CreateFile/Rename/Delete functional bodies in FSFolder.cpp
and uses existing portable upstream CreateDir/CreateComplexDir/COutFile/MyMoveFile.
QString entry points delegate to those bodies; Qt progress callbacks adapt to
IProgress, preserving original SetTotal/SetCompleted order and cancellation.
Const GetAbsPath serves the existing const presentation bridge. POSIX EEXIST
replaces Win32 ERROR_ALREADY_EXISTS in the directory fallback branch. Rename
retains the existing no-replacement check, and direct selected symlinks remain
unavailable; recursive deletion uses the already adapted no-follow FileDir
service. Null progress/out-of-range indices are rejected before dereference.
CopyFrom/CopyFromFile E_NOTIMPL bodies are also upstream; full CopyTo/SetProperty
remain explicitly unavailable native branches pending P1-03/P1-04. This partial
interface is not completion of those mandatory workflows or all of FSFolder.

Toolbar prerequisites for native OnMenuCommand retain original CApp fields,
ReadToolbar/SaveToolbar/Switch* methods, bit mask/default high-bit rule and
SaveToolbarChanges Save→Reload→Move order. ViewSettings saves the original
Toolbars key via explicit INI path; invalid/out-of-range values use original
default. Qt legacy toolbar/* keys migrate only when Toolbars is absent. Qt owns
QToolBar construction, grouping/metrics/icons and layout activation; checkboxes
mirror source flags with blocked signals and route through CApp. Full source
toolbar/resource/icon presentation remains P0-09. Flat-view dispatch also uses
the command entry; full source model/timer refresh remains P0-05.

Additional copied bodies: App.cpp::RefreshTitle/RefreshTitlePanel retain the
focused-panel test, empty-prefix `7-Zip`, PrevTitle cache and update order; QWidget
setWindowTitle replaces SetText. Create clears the cache and command service on
reinitialization. Partial CPP/Windows/Control/Dialog.cpp/.h::IsDialogSizeOK retains
original integer division/multiplication and both work-area comparisons. Qt
system font metrics replace packed GetDialogBaseUnits and QScreen availableGeometry
replaces GetWorkAreaRect; other Dialog.cpp bodies are not ported by this change.
Qt policy refresh uses one snapshot; execution always recomputes it. Recursive
filesystem cancellation explicitly preserves E_ABORT across the portable service.
Delete's Qt error projection retains the failed native item path after partial
deletion, instead of replacing it with a generic message. SelectByType availability
uses the source focused item, including empty selection, not operated indices.
The fake backend now supplies typed fixture format/openFolder rather than
accidentally falling through to a real `/missing/7zz` reopen; explicit metadata
keeps priority. This test adaptation does not weaken native writer capability.
The same fake overrides source extractSourceEntry with queued completion/cancel;
native OpenItemInArchive no longer falls through to a real engine in UI fixtures.
Navigation tests explicitly select one item after Select All before single-item
Enter, because copied OpenSelectedItems operates the full selection. Both Up
presentations share original parent dispatch, including leaving archive root.
Current-build main-window JSON at 980×620, 720×480 and 640×400 shows toolbar,
address, list and status bar fully visible. Geometry establishes Qt usability,
not rendered Windows parity. Latest focused ui_navigation passed 1/1 (1.06 s);
screen/menu/native FS cancellation and nested/native navigation checks passed
their focused cases. The current complete gate remains pending.
First complete gate compiled successfully but failed 4/129 cases (125 passed,
247.68 s): missing deletion-path projection; unchanged priority child reply timeout;
viewer fixture typed format/source-stream/lifetime omissions; parent fixture and
SelectByType focused policy. After fixes the five direct filesystem/viewer/key/
policy consumers passed 5/5 (3.35 s), and unchanged priority retest passed 1/1
(3.78 s). Viewer success/failure/cancel remains covered; final file lifetime is
checked on native CopyTo's output, independently of disposed transport scratch.
These focused checks preceded the final full gate recorded below.

Final acceptance: ./scripts/check.sh formatting/build PASS and 129/129 CTest
PASS (87.96 s), macOS arm64/macOS 27/Qt 6.11.2, 2026-09-29. Complete current
command component and its existing consumers are accepted; all omitted feature
workflow/model/resource/shell scopes above remain mandatory in their owners.
Windows/Linux runtime and rendered Windows parity remain unverified.

Earlier intermediate checks: native_fs_operations/fs_folder passed; ui_state
passed after synchronizing a double-clicked unselected Qt row before the native
OpenSelectedItems loop. ui_state/ui_layout/app_lifecycle passed 3/3 (2.35 s).
Runtime policy fixture compares canonical native filesystem paths (macOS /var
aliases /private/var); its initial lexical comparison was corrected without
changing production path logic. These are implementation checks, not P0-03
acceptance; current full gate and remaining state matrix are still pending.
The same application policy is checked again before side effects so a stale
shortcut cannot bypass a disabled command. Plugin Invoke/release remain ordered
services, with actual plugin commands unavailable until P2-09. Qt service calls
replace HWND activation/close, Options/About/help/temp dialogs; future unsupported
features remain mandatory in their owners. All timestamp/menu/language construction
omissions are still explicit; copied switches do not imply implemented consumers.

P0-04 navigation prerequisite inside the command integration copies PanelItems.cpp
OpenSelectedItems/OpenFocusedItemAsInternal, PanelItemOpen.cpp OpenItem/
OpenFolderExternal and PanelFolderChange.cpp OpenFolder. These preserve native
operated/focused indices, 20-item limit, parent insertion, first internal-folder
break, multi-item external flow, archive/known-extension/virus/error/cancel branches,
BindToFolder → SetNewFolder → LoadFullPath → RefreshListCtrl. QtPanelList adapts
list focus/selection/repaint/EnsureVisible; source MessageBox_Error_LangID is copied
to Panel.cpp. Existing no-handle IQtExternalFileServices supplies OS association;
full known process/editor owners remain P1-08. No separate helper iteration or
P0-04 acceptance: production consumers and its full navigation gate remain open.

Production command registry/dispatch, state matrix/keyboard/
context integration, UX and full gate remain pending. No helper commit or package
acceptance. Source notices retained; formatting applied separately. Windows/Linux
and rendered Windows parity remain unverified.

## P0-02 — accepted owners and state

Parent P0-02 is accepted. Its former recursive helper chain is historical evidence,
not remaining work. The [archived source records](history/p0-02-source-port.md)
preserve all original paths/symbols, copied/adapted/omitted portions, project baselines
and checks verbatim. Every borrowed module uses
[ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Adapted application files retain upstream paths under src; unchanged dependencies
remain under third_party/7zip with original notices/license. Formatting is separate
from functional changes. This consolidation adds no source or runtime acceptance.

## P0-02.1 — application history/favorites owner

Copied FileManager/AppState.h CAppState fields and Read/Save order; the explicit
settings-path argument delegates to QSettings adapters. MainWindow reads the owner
before menus/panels; confirmed history/favorites edits retain immediate persistence.
Historical acceptance: project baseline `36fad59`, commit `eedf926`, macOS
2026-09-28, focused 5/5 and full formatting/build/54 CTest.

## P0-02.2 — application/panel lifecycle owner

Copied App.cpp/.h Create/CreateOnePanel/Save/ReleaseApp and Panel.cpp/.h
Create/OnCreate/ReleasePanel, native focus/index/tab callbacks, per-panel settings
and disable-all-before-any-release ordering. ViewSettings/RegistryUtils retain
source bit packing and keys; QSettings replaces Registry and source keys take
precedence over migrated legacy Qt keys. QtAppPresentation supplies widgets and
guarded targets; the former global app/launcher references become per-instance
owners. Save stores an open archive's outer parent. Historical acceptance:
macOS arm64 2026-09-29, full formatting/build/121 CTest (85.30 s), startup
success/failure/cancel, independent state and 16 normal/compact archive JSON reports.

## P0-02.3 — operation-state and window-close integration

Copied FM.cpp WM_CLOSE soft-exit/drop-revoke/Save/ReleaseApp/window-save/hard-exit
order through IQtFileManagerWindowServices. Qt additionally cancels/joins backend,
scanner, file/volume and per-panel external workers while inputs still live.
QtAppOperationState holds source CPanel processing guards across asynchronous
completion: Scan permits navigation, Open/Operation block consumers, completion
restores guards after rows/selection agree, and closing prevents late callbacks.
QSettings stores source binary Position/Panels using logical Qt coordinates in
place of WINDOWPLACEMENT. Historical acceptance: macOS arm64 2026-09-29,
focused 8/8 and full formatting/build/122 CTest (84.83 s).

### Integrated prerequisites and remaining boundaries

| Former P0-02 scope | Accepted result / authoritative audit section |
| --- | --- |
| Native IFolder/COM/property types, CAgent/CFSFolder ownership, source item indices, path/property/selection helpers and selected consumers | Archived module records; P0-04.1/.2 navigation, P0-05 model, P0-06 sort/columns/time, P0-07 selection |
| CTempFileInfo/CFolderLink, FileDir cleanup, CLibrary transfer, root/nested parent stack and changed-file update | Archived dependencies; P0-04.1/.2 integrated source owner and temporary lifetime |
| FormatUtils/resources, CProgressSync, native threading/errors, progress/statistics/icon/priority/taskbar/startup wait and update callbacks | Archived dependencies; P0-10.1/.2 integrated modal worker and all current consumers |
| Timer/notify/status guards, all-panel release and process/operation quiescence | P0-02.2/.3 and P0-06.4 |

The archive records narrow native/Qt adaptations, including POSIX library transfer
copied from the Windows class, Compare_FiTime, the 100 ms POSIX sleep and 500 ms
startup wait, strict resource extraction, native progress clamping, real priority
failure rollback, Cocoa badge ownership and Linux DBus transport/desktop identity.
They do not imply complete upstream modules. Full editor/known-process workflows
remain P1-08; language loading P1-14; raw/ADS/reparse capabilities P1-10/P2-05;
shell integration P2-09; packaging and remaining engine/platform branches P3-05.
Memory/zone and other omitted branches keep the owners specified by their module
and later integrated records. None becomes N/A through this cleanup.

Historical macOS checks establish local behavior, source contracts and geometry;
Windows/Linux runtime, rendered Windows/Dock fidelity and actual Linux launcher
rendering remain unverified. The accepted neighboring packages retain their own
evidence; parent counters do not change.

## P0-05.1 — filesystem read model and normal/flat consumers (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([FSFolder.cpp/h](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager),
same revision for `TextPairs.cpp/h`). `TextPairs.cpp/h` were copied to their
mirrored paths; only `StdAfx.h`/relative Common includes became project include
paths. `CPairsStorage` parse, sort, lookup, replacement and save bodies remain
the original implementation. Required clang-format is mechanical and separate
from the include adaptation. `CFSFolder::LoadComments`, lazy `kpidComment`
lookup/cutoff, `ReadFileInfo`, `ReadChangeTime`, `CalcItemFullSize`,
`CFsFolderStat::Enumerate` and `Clone` start from the original bodies. Previously
accepted `LoadItems`/`LoadSubItems` and typed-property bodies remain the owner.

The Windows link and NT change-time calls are retained under `_WIN32`; POSIX
uses `lstat` for the same inode/link/status-change metadata and the portable
7-Zip `CFiTime` conversion. POSIX `CEnumerator::Next`/`Fill_FileInfo` adapt the
source Windows iterator shape. In flat recursion the source continues past an
unreadable child; the local cancellation callback alone propagates `E_ABORT`.
The outer root still reports its own binding failure to Qt. Directory-size
enumeration retains source count/size/progress flow and does not follow POSIX
symlinks. `mainwindow_filelist.cpp` now takes visible names/prefixes and sort
roles from source properties/indices; NFC normalization is limited to Qt text.
QFileInfo remains for Qt icons and pre-existing timestamp presentation; native
time formatting and complete column/sort ownership are P0-06.

The read-model consumer checks cover quoted UTF-8 `descript.ion` comments and
reload, hard links/inode/change time, Unicode nested directory statistics with
cancellation, and hidden/empty/unreadable sibling enumeration. Normal/flat UI
and source properties consumers are exercised by `flat_view`,
`panel_selected_state` and `properties_runtime`. The incomplete source
`IFolderCompare`/fast item-name sort path and periodic model refresh belong to
P0-06, system icons to P0-09, comment writes/`SetProperty` to P1-03, raw
reparse/ADS interfaces to P2-05/P2-06. P0-05.2 still owns large asynchronous
scan, stale batches and full parent acceptance. Windows/Linux runtime and
rendered Windows comparison remain unverified.
Focused `fs_folder`, `flat_view`, `panel_selected_state`, `properties_runtime`
pass 4/4, 6.13 s; the full `./scripts/check.sh` formatting/build/all CTest gate
passes 130/130, 88.79 s on macOS arm64, 2026-09-29. Current-build 980×620 and
640×480 flat scan/presentation JSON retains visible toolbar, address, list and
status; these reports do not measure Windows rendering. P0-05 parent acceptance
is not claimed by this read-model component.

## P0-05.2 — large flat filesystem presentation and P0-05 acceptance (2026-09-29)

Baseline remains ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([FSFolder.cpp source](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/FSFolder.cpp)).
No borrowed functional body changes in this iteration: the accepted
`CFSFolder::LoadSubItems`/`LoadItems`, `GetProperty`/`GetRelPath` and native
item indices remain the sole flat enumeration model. `FilesystemFolderLoader`
still adapts worker scheduling and atomic cancellation; `FilesystemScanner`
transports its completed source folder to Qt and rejects stale generations.
The GUI batch cap changed from 128 to 32 because presentation can resolve
native icons per row after batch preparation. This bounds one delivery even
when the preparation timer has not expired. It does not change source order,
filters, metadata or the worker's count-progress cadence.

The deterministic nested Unicode fixture contains 2048 files and two parent
folders; it verifies 2050 ordered source indices, at most 32 rows per GUI
batch, an event-loop turn before completion and no failure. Existing scanner
tests cover restart during native enumeration, restart/cancel during row
publication, destruction and invalid roots. Existing source-model tests cover
hidden/empty entries, unreadable siblings, Unicode names, symlink-cycle
suppression, metadata and cancellation. The 1500-file `flat_view` scenario
checks normal/compact status, partial listing, cancel, mode switch and
navigation without stale rows. Current-build compact report at 640×480 keeps
the toolbar, address, table and status fully visible; its upstream reference
is the File Manager list/flat command structure, not a rendered Windows image.

The P0-05 source filesystem model, narrow async adapter and current normal/flat
consumers are accepted within the above scope. Source sort/column/time and
auto-refresh remain P0-06, native icons P0-09, write-side `SetProperty`/comment
editing P1-03, raw reparse/ADS P2-05/P2-06 and larger-operation responsiveness
P2-10. Windows/Linux runtime and rendered Windows appearance remain unverified.
Focused `fs_folder`, `filesystem_scanner` and `flat_view` pass 3/3 (2.51 s);
`./scripts/check.sh` passes formatting, app build and 130/130 CTest (85.77 s)
on macOS arm64, 2026-09-29. The updated current-build 640×480 JSON has a
fully visible toolbar, address, table and status. No pixel parity is claimed.

## P0-06.1 — typed source sort and current list consumers (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([PanelSort.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelSort.cpp),
same revision for `PanelItems.cpp` and `Panel.h`). Existing mirrored
`PanelSort.cpp` already contained the upstream natural filename pointer loop.
This iteration ports `CompareItems2`/`CompareItems` property iteration into
`CPanel::CompareItemIndices`: parent and folder priority, source `IFolderCompare`
dispatch, natural base-name/extension comparison, `CPropVariant` type/BSTR/value
comparison, name→prefix→source-index fallback. The Qt row comparator now passes
original source indices and a property ID; the previous independent Qt value/
text comparator was removed. `FileTable` holds only a non-owning panel pointer,
whose lifetime is managed by `CApp`; Qt still reverses property order for
descending and compensates folder grouping. Header/action changes synchronize
the native panel's `_sortID`/`_ascending` with the visible state. Root rows use
the same item type and source owner.

Necessary adaptations: upstream Win32 `LPARAM` list-item indices come from Qt
row roles; `wchar_t` names are converted at the existing Qt text boundary;
`SortPropIDForColumn` maps the current seven-column view. The original raw
property branch and `SetSortRawStatus` require the source column catalog and
remain P0-06.2. View column/time persistence and timer refresh remain
P0-06.2/.3. Windows `ListView_SortItems` and focus-scroll call are represented
by Qt sorting and the current shared selection model. There is no second
functional sort implementation for bound folders.

The old flat-prefix test expected natural digit order for `dir2`/`dir10`.
Upstream compares `kpidPrefix` as a BSTR, so the test now expects lexical
`dir10` before `dir2` after equal names. Focused source-sort/flat/selection/root
consumers and current-build 980×620/640×400 JSON validate the resulting Qt
behavior; Windows rendering/runtime and Linux runtime remain unverified.
Focused `panel_sort`, `flat_view`, `panel_selected_state` and `root_navigation`
pass 4/4 (5.64 s); `./scripts/check.sh` passes formatting, app build and
130/130 CTest (88.78 s) on macOS arm64, 2026-09-29. The current-build compact
sort report has fully visible toolbar, address, table and status. P0-06.2/.3
and parent acceptance are still mandatory.

## P0-06.2 — source column catalog, raw comparison and Qt header (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed original paths: `CPP/7zip/UI/FileManager/PanelItems.cpp` (`InitColumns`,
`SaveListViewInfo`, `GetColumnVisible/Width/Align`, `ShowColumnsContextMenu`),
`ViewSettings.h/.cpp` (`CColumnInfo`, `CListViewInfo::Save/Read`),
`PanelSort.cpp` (`CompareItems2` raw branch and `CompareFileNames_Le16`),
`PanelListNotify.cpp` (`GetItemText` property/raw branches), `Panel.h`, and
`CPP/7zip/UI/Agent/Agent.h/.cpp` (`CAgentFolder::GetNumRawProps`,
`GetRawPropInfo`, `GetRawProp`, `GetParent`). `PropertyName.cpp` and portable
`CPP/7zip/UI/Common/PropIDUtils.cpp` are existing reviewed dependencies.

The mirrored `PanelItems.cpp` now builds `CPropColumns` from each bound folder's
typed and raw property capabilities, merges the saved visibility/width/order,
forces Name visible and restores its property sort. The original
`SaveListViewInfo` column-order/hidden-column construction and binary
`CListViewInfo` v1 layout remain in their upstream owners. The former fixed
Qt `columns/2..4` flags are replaced by a per-FolderTypeID `QSettings` byte
record; hex-encoded UTF-8 type IDs prevent INI slash/Unicode key collisions.
Registry calls and Win32 `LVCOLUMN` reads are replaced only by a Qt header
snapshot. Qt `FileTable` holds the current source property/raw mapping, retains
hidden path/index transport cells, displays source-formatted property text and
sorts original item indices without parsing display strings. The dynamic header
menu lists all reported properties, keeps Name visible, restores shown columns
and resets a hidden sort column to Name. Four view modes share the same rows and
column state. The previous seven-position `SortPropIDForColumn` adapter was
removed. `GetColumnAlign` keeps the original property/type switch with Qt
alignment constants; copied modules use project clang-format separately from
functional comparison.

`PanelListNotify.cpp::GetItemText` retains the source size grouping, BSTR newline
replacement, `ConvertPropertyToShortString2` fallback, raw security/reparse
conversion and short-hex/long-data branches. The Name and flat-prefix Qt
presentation remains the accepted P0-05 adaptation. Time uses level 0 here;
five precision levels, UTC control and view-specific formatting belong to
P0-06.3. The source raw comparison's empty/data-type/reparse branch is copied;
the Qt row supplies its source index and raw flag. Malformed reparse bytes are
rejected before pointer comparison, a portability guard absent from the Win32
call site. `CAgentFolder` now implements the upstream raw interface and maps
folder rows to original archive indices; `qtarchiveproperties.cpp` forwards an
available raw provider across the `7zz` metadata adapter. The current external
`7zz` listing does not supply binary raw fields, so real archive raw values are
not claimed: the code path is checked with a deterministic provider fixture,
and complete archive raw-value transport remains mandatory P1-10. `_proxy2`
alternate streams remain P1-10/P2-05. There is one authoritative copy of each
borrowed functional body.

`panel_columns` checks Unicode/space filesystem values, raw catalog/text/sort,
all-column header control, independent filesystem/archive records,
width/order/sort restoration and eight current-build JSON reports (four modes
at 980×620 and 640×400); toolbar, address, list and status stay visible.
`view_settings_columns` checks exact binary bytes and malformed records;
`agent_folder`, `panel_sort` and `ui_layout` cover direct consumers. Focused
5/5 pass after the local alignment/test-cleanup edits; the app rebuild passes. The full
`./scripts/check.sh` before that edit passed formatting, app build and 132/132
CTest (93.36 s) on macOS arm64; the subsequent alignment-only change received
focused rebuild/tests. Windows/Linux runtime and rendered Windows visual parity
remain unverified. P0-06.3 time and P0-06.4 auto-refresh/parent acceptance are
still mandatory.

## P0-06.3 — source time precision and UTC View workflow (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed original paths/symbols: `CPP/7zip/UI/FileManager/Panel.h`
(`_timestampLevel`, MIN default), `App.h` (`_timestampLevels`,
`GetTimestampLevel`, `SetTimestampLevel`), `PanelListNotify.cpp::GetItemText`,
`MyLoadMenu.cpp` View/Time popup and `WndProc` time dispatch;
`CPP/Windows/PropVariantConv.h/.cpp` and
`CPP/7zip/UI/Common/PropIDUtils.cpp::ConvertPropertyToShortString2` are the
previously vendored, unchanged formatting dependencies. Portable
`CPP/Windows/TimeUtils.cpp` supplies the current UTC sample. The shared
`FormatUtils.cpp` has no time-formatting body and was reviewed but not changed.

`CPanel` now retains the source MIN default and supplies its selected level to
the copied `GetItemText` property branch. `CApp` owns the same all-panel level
update and menu-level vector as upstream. The source View/Time popup loop
formats five examples (DAY/MIN/SEC/NTFS/NS) with `ConvertUtcFileTimeToString`,
records the available levels, and uses the original `IDM_VIEW_TIME` range and
`IDM_VIEW_TIME_UTC` dispatch. Qt `QMenu`/`QAction` replace Win32 menu handles;
the existing `QtFileManagerCommands` boundary redraws visible typed time cells
without replacing the source formatter or converting displayed text back into
timestamps. The column comparator continues to read raw `FILETIME` properties.
Qt `QSettings` additionally persists precision and UTC between launches;
upstream retains these across panels but only in memory. The external `7zz`
local listing text is converted to `FILETIME` at the existing metadata adapter;
its actual precision cannot exceed the listing's supplied digits. No time
logic was copied into `FormatUtils`, and no duplicate formatter was introduced.

`panel_columns` now checks five menu examples, day/second/UTC transitions,
reopening, raw time sort despite equal day strings, Unicode/space filesystem
and archive values, and current-build 980×620/640×400 JSON. The reports show
fully visible toolbar, address, view stack and table. Focused
`view_settings_columns|ui_layout|command_policy_runtime|panel_sort|panel_columns`
passes 5/5; `./scripts/check.sh` passes formatting, app build and the full
CTest gate (133 tests registered) on macOS arm64. Windows/Linux runtime and
rendered Windows menu parity remain unverified. Auto-refresh and full P0-06 acceptance
remain mandatory P0-06.4.

## P0-06.4 — source timer policy and portable filesystem refresh (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed `CPP/7zip/UI/FileManager/App.h` (`AutoRefresh_Mode`,
`Change_AutoRefresh_Mode`, `SetPanels_AutoRefresh_Mode`), `Panel.h`/`Panel.cpp`
(per-panel mode, one-second timer), `PanelItems.cpp`
(`RefreshListCtrl_SaveFocused`, `OnReload`, `OnTimer`) and `FSFolder.cpp`
(`CFSFolder::WasChanged`). The app/panel mode, source timer guards,
`IFolderWasChanged` query and saved `CSelectedState` flow are copied into their
existing modules. Qt owns timer delivery, a View-menu check action, persisted
mode, and the callback that reloads the current filesystem view through the
source panel and restores the visible top path/scroll position. Notification
processing remains disabled during active operations and window teardown.

The original Windows `CFSFolder::WasChanged` consumes a native change
notification; its POSIX branch always returns false. The narrow Qt adapter
polls metadata of the displayed folder and source-enumerated entries every
second and marks the source panel changed. It skips active/flat scans, so it
does not read a partially published list. A missing current directory clears
stale visible rows and records its absent signature; recreation triggers a
fresh bind. Archive folders do not implement `IFolderWasChanged` upstream, so
archive refresh remains explicit. This does not port Win32 notification handles
or claim archive auto-refresh. Large-tree polling cost remains part of P2-10.

`panel_auto_refresh` checks Unicode file creation/removal, current item,
selection/scroll, disabling and persisting the View option, manual refresh,
and disappearing/reappearing folders on macOS. Earlier P0-05 flat scan
cancel/stale-batch and P0-06.1–.3 sort/columns/time and four-view geometry
evidence remains applicable. Windows/Linux runtime and rendered Windows visual
parity remain unverified. Focused `panel_auto_refresh` and the full
`./scripts/check.sh` gate pass (134/134 CTest) on macOS arm64. The first full
run was denied DBus socket creation and process-priority calls by the sandbox;
the rerun with those system permissions passed. A later one-line action-state
fix and selection assertion passed the affected build/focused CTest again.

## P0-07.1 — source list settings and main-panel activation (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source tree](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
Reviewed `CPP/7zip/UI/FileManager/SettingsPage2.rc` control order and
`SettingsPage.cpp::{OnInit,OnButtonClicked,OnApply}` for all seven list flags;
`RegistryUtils.cpp::CFmSettings::{Load,Save}` and `App.cpp::SetListSettings`
were already source-derived. Reviewed `PanelItems.cpp::InitList` (`showDots`
and `kParentIndex`), `PanelListNotify.cpp::OnNotifyList` (one-click activation,
track-select and item changes), `PanelSort.cpp` parent-index ordering, and
`PanelMenu.cpp::CreateFileMenu` (`ShowSystemMenu` only for filesystem).

The Qt `CSettingsPage` now follows the original seven-control order and
retains its source init/changed/apply policy. ShowDots, FullRow and
ShowSystemMenu are read and written through the existing `CFmSettings` owner;
cancel and an unchanged page do not write. AlternativeSelection is visible
but disabled, retains any pre-existing stored value, and remains mandatory
P0-07.3 because upstream uses an independent marked-item vector and
Ctrl/Shift range policy. Large pages, extraction RAM, six-page Options,
Help and language controls remain P1-12.

The source `showDots`/`kParentIndex` rule is presented in the bound Qt table,
including flat/normal filesystem and archive views; the virtual root omits it.
The parent item keeps its source sentinel through sort and navigation. Qt
preserves the source `FullRow` visual distinction with a delegate that paints
only the Name cell's selection when the option is off, without changing the
row-level selection model or source item indices. The `ShowSystemMenu` flag
adds an OS file-manager action only in the filesystem context; native Windows
shell verbs have no current cross-platform Qt equivalent and full context
menu parity remains P0-09. System icons and grid keep their prior consumers.

`qtfilelisthover.cpp` is new presentation glue for the source
`LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT` policy on both table and icon
views. It uses the already established 400 ms Qt dwell adapter, validates
the persistent model index and cursor position after scroll/removal, and
cancels on modifiers, drag, press and leave. It never opens on hover. Qt
single-click activation remains guarded against Ctrl/Shift/Meta gestures.
The fixed interval and platform file-manager action are explicit deviations.

`fm_settings` and `list_settings_runtime` exercise seven-control order,
disabled alternative mode, persistence/cancel, ShowDots on disk/archive/root,
normal/reverse sort, system-menu visibility, full-row paint difference,
single-click hover guards and selected-row retention. Current-build settings
340×300/460×300 and list 640×400/980×620 JSON show visible controls and
unclipped table/address. A targeted Qt screenshot confirmed the selection
paint difference; rendered Windows parity and Windows/Linux runtime remain
unverified. Source selection/range/alternative-mode acceptance remains P0-07.2/.3.
Focused direct-consumer CTest passes 8/8; `./scripts/check.sh` passes the full
135/135 CTest gate on macOS arm64 in the unsandboxed environment required by
AGENTS.md. A later localized tooltip and drag-guard test correction passed the
affected build and focused CTest. The first optional screenshot fixture wrote
inside its temporary directory and was corrected to use the build report
directory; it is not part of the product path.

## P0-07.2 — source selection commands (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([exact source](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelSelect.cpp)).
`src/CPP/7zip/UI/FileManager/PanelSelect.cpp` now holds the source bodies and
branch order of `CPanel::UpdateSelection`, `SelectSpec`, `SelectByType`,
`SelectAll` and `InvertSelection`, using the existing upstream
`CRecordVector<bool> _selectedStatusVector`, `GetRealItemIndex`,
`GetItemName`, `IsItem_Folder` and source `Wildcard.cpp` matcher.
The independent `CPanelSelection`/`QString` snapshot and second rule owner
are removed. `Panel.h` retains the upstream method names and `QtPanelList`
supplies one narrow selected-state setter and redraw adapter. Mask dialog
creation remains in `mainwindow_selection.cpp`: Qt has no Win32
`CComboDialog::Create`, so the accepted `UString` mask enters the source loop
as an argument. `SaveSelectedState` synchronizes Qt's current selection before
each command until the source item-notification/mark body is ported.
The native `UString` wildcard overload exposed an existing macOS integration
bug: Qt/AppKit headers define `TARGET_OS_IPHONE` as `0`, so the old `#ifdef`
selected iPhone case sensitivity on desktop macOS. `Wildcard.cpp` now tests
the macro value, retaining upstream's desktop case-insensitive rule and its
source matcher instead of reverting to the Qt UTF-16 selection wrapper.

The parent sentinel is skipped in selection projection and type commands;
source folder indices remain stable across sorted table/icon views. Source
`_enableItemChangeNotify` suppression maps to the existing `CDisableNotify`
guard while Qt selection signals fire; this prevents command availability
refresh from overwriting the source vector midway through a multirow command.
The window refreshes command availability after the completed projection.
`CPanel::KillSelection`, `OnLeftClick`,
`OnArrowWithShift`, `OnShiftSelectMessage`, `OnInsert` and alternative-mark
state remain P0-07.3. The old `OnUpWithShift`/`OnDownWithShift` bodies are
commented out in the baseline and are not active omissions. Current tests
cover the bound source vector, parent sentinel, mask cancellation, disk/archive
mask/type and keypad actions in four views. Windows/Linux runtime and
rendered Windows parity remain unverified. Focused direct consumers
`panel_selection`, `panel_key`, `ui_state`, `panel_selected_state`,
`archive_selection_runtime` and `filesystem_selection_runtime` pass 6/6;
the full unsandboxed `./scripts/check.sh` gate passes on macOS arm64
2026-09-29 (135/135 CTest). No layout or text changed in this iteration;
the P0-07.1 normal/compact geometry evidence remains applicable.

## P0-07.3 — source alternative marks, mouse and keyboard selection (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([PanelSelect.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelSelect.cpp),
[PanelKey.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelKey.cpp),
[PanelListNotify.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelListNotify.cpp)).
Copied the active control flow of `CPanel::OnLeftClick`,
`OnArrowWithShift`, `OnShiftSelectMessage`, `OnInsert` and `KillSelection`
into the mirrored
`PanelSelect.cpp`, plus their `_selectionIsDefined`, `_selectMark`,
`_startGroupSelect` and `_prevFocusedItem` fields in `Panel.h`. The Qt click
signal supplies row/modifier state where Win32 used `NM_CLICK`/`LVKF_*`;
the key adapter calls the source shift-range completion after Qt moves focus,
replacing `PostMsg(kShiftSelectMessage)`. `QtPanelList` projects the original
marked vector as RGB(255,192,192) backgrounds in the shared table/icon model,
and constrains Qt selection to the single focused row where upstream sets
`LVS_SINGLESEL`. Parent sentinel rows are skipped. `EnsureSelectionVectorSize`
is narrow Qt initial-load glue: this presentation can publish rows without a
`RefreshListCtrl` call, while upstream initializes its vector during list
construction. It also preserves marks during incremental row loading.

`CApp::SetListSettings` now calls the panel transition, while the original
`CSettingsPage` reads/writes AlternativeSelection through `CFmSettings` with
Apply/Cancel persistence. `SaveSelectedState` retains source marks in that
mode; `RefreshListCtrl` restores marked names independently from Qt focus.
Current command availability, hash/extract guards and Copy summary use source
operated indices, so a single focused Qt row does not replace multiple marks.
`CDisableNotify` prevents Qt selection notifications from resetting the source
vector mid-command. Unadapted Win32 custom-draw, `LVS_*` style bits and
`HWND` messages are presentation only; source mark/range bodies remain.
`PanelKey.cpp` still has other P0-08/P0-09/P1 command branches outside this
selection scope. The deleted-item custom-draw branch remains P0-09; two-panel
drag/drop remains P1-16. The inactive commented
`OnUpWithShift`/`OnDownWithShift` are not ported.

`panel_selection` tests the source mark/range/Insert/KillSelection bodies,
focus restoration and parent sentinel. `panel_key` runs disk/archive ×
normal/flat × four views, plus root, settings toggles, Copy summary,
filesystem row removal, scroll and hover. `fm_settings` verifies enabled
Apply/Cancel persistence. Current-build 32 normal/compact 980×620/640×400
JSON reports and one removal report show visible unclipped lists; targeted
table/icon viewport screenshots show red marks separate from the blue focused
row. Focused direct-consumer CTest passes 9/9 and the full unsandboxed
`./scripts/check.sh` gate passes 135/135 on macOS arm64 2026-09-29.
Windows/Linux runtime and rendered Windows visual parity remain unverified.

## P0-08.1 — source panel switch and disk/disk Qt projection (2026-09-30)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([App.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/App.cpp),
[App.h](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/App.h),
[FM.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/FM.cpp),
[PanelKey.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelKey.cpp),
[resource.rc](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/resource.rc)).
Copied `CApp::SwitchOnOffOnePanel` creation/show/hide/layout control flow
into mirrored `App.cpp` and retained the original two `CPanel` owners, paths,
list modes and focus callbacks. Qt's `IQtAppPresentation::ShowPanel` replaces
Win32 `Enable`/`Show`; QSplitter replaces `FM.cpp::MoveSubWindows` HWND geometry.
The Qt action presents the resource's View → 2 Panels F9 command. The list's
Tab adapter calls original `CPanelCallbackImp::OnTab`; panel focus events still
call `CApp::SetFocusedPanel`. On failed second creation, Qt rolls back NumPanels
to one so a rejected widget cannot leave an invisible source owner active.

Win32 has separate child controls for each panel. `mainwindow_twopanel.cpp`
provides their Qt equivalent: each panel retains its own widgets, visible
location/view/sort and refresh snapshot; the existing command presenter projects
the active panel. The original `CPanel` still owns folders and source selection
vectors. The Qt filesystem scanner is single-owner: switching during a flat
scan cancels its generation and rebinds that panel when reactivated, preventing
queued rows from reaching the other panel. The existing portable metadata poll
now visits both visible disk panels. A missing provider during rebinding leaves
source file commands disabled instead of entering `CreateFileMenu` with null.

Scope is disk/disk including separate paths, view modes, selection, focus,
normal/flat loading and inactive refresh. Archive/disk and archive/archive
projection, Alt+F1/F2 target, startup splitter/layout restore and Copy/Move
destination default remain P0-08.2; actual cross-panel transfer bodies remain
P1-01. This is a partial port of the broader App/FM/Panel layout workflow, not
acceptance of the P0-08 parent. Win32 splitter drawing, drag target and direct
`HWND` messages are Qt presentation or later P1-16 adapters. Required project
clang-format changes only formatting. `app_lifecycle` covers success/failure of
the switch, while `panel_key` covers F9/Tab, active commands, path/view/selection,
inactive refresh, flat scan and JSON normal/compact/hidden geometry. The full
unsandboxed `./scripts/check.sh` gate passes 135/135 on macOS arm64 2026-09-30.
Windows/Linux execution and rendered Windows parity remain unverified.

## P0-08.2 — mixed panels, source path focus and restored layout (2026-09-30)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([App.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/App.cpp),
[PanelKey.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/PanelKey.cpp),
[ViewSettings.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/ViewSettings.cpp),
[FM.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/FM.cpp),
[Panel.cpp](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/Panel.cpp),
[resource.rc](https://github.com/ip7z/7zip/blob/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager/resource.rc)).
The existing mirrored `CPanelCallbackImp::SetFocusToPath` retains the source
one/two-panel target rule; the `PanelKey.cpp` Qt key adapter now calls it for
Alt+F1/F2. The existing `CWindowInfo::Read/Save` byte layout remains upstream;
`MainWindow` uses its panel count/current index/splitter coordinate at startup
and close, while Qt's QSplitter provides geometry. The startup presenter
temporarily projects each CPanel when it creates controls and restores the
requested active panel after widgets enter the Qt event loop. Both bound
filesystem and archive folders render into their own Qt panel state; archive
metadata, folder path, list entries and view state remain with that projection.

The Qt Copy/Move/Extract dialog adapter takes the default from the source
`CApp::OnCopy` opposite-panel rule and `CPanel::GetFsPath`. It keeps the current
one-panel destination when only one panel is visible. Real cross-panel transfer
to a target archive and full F5/F6 operation routing remain P1-01; accepting a
non-directory target in the current filesystem transfer rejects it safely.
`CApp::Save` skips an uncreated hidden panel so its previously saved path/view
and flat mode survive one-panel sessions. Source `CApp::Save` writes both slots;
the difference is needed because this Qt application defaults to one visible
panel rather than 7zFM's two.

Focused `panel_key`, `app_lifecycle`, `app_lifecycle_window`, `panel_focus`,
`panel_runtime`, `native_navigation`, and `binding_lifecycle` CTest consumers
passed on macOS arm64. The new mixed disk/archive, archive/archive,
Alt+F1/F2, dialog defaults, restored/hidden-panel tests and normal/compact
widget JSON establish the Qt scope. The full unsandboxed `./scripts/check.sh`
gate passes 135/135 registered CTest cases on macOS arm64, 2026-09-30. P0-08
parent is accepted; Windows/Linux runtime and rendered Windows visual parity
remain unverified.

## P0-09.1 — главное меню и toolbar 7zFM (2026-09-30)

Точный baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/FileManager/resource.rc` (`IDM_MENU`, порядок,
разделители, сочетания, `IDB_*`), `MyLoadMenu.cpp::CFileMenu::Load`,
`ExecuteFileCommand`, `App.cpp::g_ArchiveButtons/g_StandardButtons`,
`FM.cpp::OnCommand` и `ViewSettings.cpp::Read/SaveToolbarsMask`.
`MyLoadMenu.cpp` и application command/toolbar-state bodies уже перенесены в
предыдущих P0-03/P0-08 итерациях; текущая итерация подключает их к общему
Qt QAction меню. `resource.rc` ранее скопирован в `third_party/7zip`.

Без изменений скопированы 14 bitmap-файлов из того же каталога и commit:
`Add[2].bmp`, `Extract[2].bmp`, `Test[2].bmp`, `Copy[2].bmp`, `Move[2].bmp`,
`Delete[2].bmp`, `Info[2].bmp` (каждая пара означает исходный большой и малый
ресурс). Проверено байтовое совпадение с baseline checkout; применимая
лицензия — `third_party/7zip/License.txt`. В Qt `mainwindow_menus.cpp`
собирает группы/порядок из `IDM_MENU`, а `qtfilemanagercommands.cpp` связывает
Open Inside `*`/`#` и Exit с уже перенесённым исходным dispatch и общим
active-panel policy. `QMenu`/`QAction` заменяют Win32 HMENU/accelerators;
`mainwindow_preferences.cpp` выбирает оба размера BMP и применяет исходный
magenta mask вместо прежних нарисованных Qt иконок. Это presentation adapter,
а не независимый functional command switch. Русские подписи остаются текущим
Qt переводом до полного source language port P1-14.

Comment, Diff, Link, Alternate Streams, Benchmark, Delete Temporary Files,
Help Contents и About показаны как disabled source пункты: их functional/UI
owners ещё открыты (P1-08/P1-15/P2-09/P3-05). Действующие Qt Open/Create/Add/
Extract Selected/Journal оставлены отдельной группой File для доступа к
архивным сценариям до переноса их исходного входа. Исходный File menu в
компактном Qt окне может прокручиваться по правилам QMenu/монитора; исходная
Win32 small-screen фильтрация уже перенесена в `CFileMenu::Load`, но menu
presentation пока показывает disabled пункты явно по требованию плана.
Контекстное меню, status totals и system icons остаются P0-09.2;
ни полный P0-09, ни rendered Windows parity эта итерация не закрывает.

`main_shell` проверяет source order/ID/недоступные команды/shortcuts,
байтово скопированные BMP в Qt и active-panel меню после переключения/Tab.
Отчёты `main-shell-980x620`, `main-shell-640x400` и отдельные file/view menu
JSON описывают текущую Qt геометрию. Focused 8/8 и полный unsandboxed
`./scripts/check.sh` 136/136 прошли 2026-09-30. Проверена только macOS arm64;
Windows/Linux runtime ещё не проверен.

## P0-09.2 — per-panel context/status и системные иконки (2026-09-30)

Точный baseline: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные пути и символы: `CPP/7zip/UI/FileManager/PanelMenu.cpp::CPanel::CreateFileMenu`,
`OnContextMenu`; `MyLoadMenu.cpp::CFileMenu::Load` и `ExecuteFileCommand`;
`PanelListNotify.cpp::CPanel::Refresh_StatusBar`, `ConvertSizeToString`;
`Panel.cpp::Create`, `OnSize` (четыре части status, под списком);
`SysIconUtils.cpp/.h::CExtToIconMap::GetIconIndex`, `FindInSorted_Attrib`,
`FindInSorted_Ext`; `resource.rc::IDM_MENU`.

Сохранённый ранее `CPanel::CreateFileMenu` содержит source policy для
operated indices, read-only, FS/hash/alt-stream folders и вызывает
`CFileMenu::Load`. В этой итерации контекстный Qt popup передаёт ему полный
source File action sequence с `programMenu=false` и строит результат по
исходному порядку и enabled состоянию. Win32 `OnContextMenu` служит
референсом позиционирования с клавиатуры: `(-1,-1)` заменяется центром
текущего элемента. Qt mouse/keyboard events и `QMenu::exec` заменяют HWND,
`TrackPopupMenu`. Отдельная Qt группа archive Add/Extract/Test/Info остаётся
после source file commands до переноса полного archive/context workflow.
`CreateSevenZipMenu` и `CreateSystemMenu` требуют Win32 COM/Explorer/extension
services; их группы на диске показаны disabled, системная — только при
ShowSystemMenu. Они остаются обязательными владельцам P1-02/P2-09/P3-05,
без объявления ready/N/A. Встроенная прежняя Qt команда открытия в системном
файловом менеджере удалена из context, так как подменяла source shell group.

Тело `CPanel::Refresh_StatusBar` скопировано в зеркальный `PanelListNotify.cpp`
в исходном порядке: operated/total, сумма выбранных размеров, размер и mtime
текущего выбранного элемента. `GetItemSize`, `GetRealItemIndex` и исходный
`ConvertPropertyToShortString2` остаются авторитетными; Qt адаптация меняет
только `_statusBar.SetText` на возвращаемые четыре `QString` поля и добавляет
переведённый префикс счётчика до P1-14. Каждый Qt panel размещает четыре
QLabel под списком; общий MainWindow QStatusBar несёт сообщения сканирования
и операций. Ранее разрозненная строка с числом объектов больше не является
единственным per-panel status.

В зеркальный `src/CPP/7zip/UI/FileManager/SysIconUtils.cpp/.h` перенесены
имена структур/класса, sorted binary searches, циклы attribute/extension
cache, лимит 65536 и shortcut числовых split-суффиксов. Win32
`SHGetFileInfo`/CSIDL/HIMAGELIST вместо копирования на macOS/Linux заменены
узким `QFileIconProvider` → `QIcon` adapter; исходные `Shell_GetFileInfo*`
функции остаются Windows-only и не вызываются Qt consumers. Диск и архив
запрашивают одну кэшированную карту при включённом ShowRealFileIcons; при
выключенном флаге остаются стандартные Qt значки. Источник сам комментирует
часть image-list integration в `PanelListNotify`; текущий Qt adapter
использует активную cache policy, но пиксельный вид системных иконок зависит
от ОС. Для особых provider icons (network/drive/shell namespace) нужны
P1-11/P2-10/P3-05.

`panel_shell` и прямые consumers проверяют disk/archive context, keyboard
anchor, status size/date/count и независимость двух панелей, кэш extension/
split и Qt JSON при 980×620/640×400. Первый полный gate выявил устаревшее
ожидание `ui_archive_states` о Refresh в контексте и скрытии disabled Rename;
тест обновлён по source меню, focused проверка прошла. После этого focused
direct consumers 11/11 и полный unsandboxed `./scripts/check.sh` 137/137
прошли на macOS arm64 2026-09-30. P0-09 parent принят; рендеринг Windows и
Windows/Linux runtime не подтверждены.

## P0-04.2 — root/address/parent window integration (2026-09-29)

Baseline: ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
([source](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17/CPP/7zip/UI/FileManager)).
The previously copied `RootFolder.cpp` (`CRootFolder::LoadItems`, `GetProperty`,
`BindToFolder`) and `PanelFolderChange.cpp` (`CPanel::OpenRootFolder`,
`OpenParentFolder`, `BindToPath`, `LoadFullPath`) remain the functional owner.
`PanelKey.cpp` now routes the resource `IDM_OPEN_ROOT_FOLDER` backslash command
through CApp; its other source key flow is retained. Qt's `mainwindow_root.cpp`
presents `IFolderFolder` item names and original indices, without a second root
enumeration or path algorithm. `mainwindow_archiveopening.cpp`,
`mainwindow_filelist.cpp` and `qtfilemanagercommandworkflows.cpp` bind root,
typed relative paths, parent and Back through the panel; Qt history uses an
empty marker for the source root prefix. `qtfilemanagercommands.cpp` derives
Parent availability from the native folder, including POSIX `/`, and prohibits
filesystem mutations on the read-only root. View-menu text, icon, list cells,
address clearing and normal/compact presentation are Qt adaptations.

The original Windows RootFolder has Computer/Documents/Network/Volumes; POSIX
has one Computer entry binding `/`. Windows drives/network namespace and Browse
providers remain P1-11, source icons/menu completeness P0-09, filesystem model
and polling P0-05/06, external editor P1-08. No full-module RootFolder claim is
made. Current root, relative/Unicode address, Parent/Back, busy stale-command
and source-index checks are in `root_navigation`; earlier P0-04.1 archive/nested,
password/cancel/update and TAR/GZip evidence is retained. Windows/Linux runtime
and rendered Windows comparison remain unverified. The current-build root JSON
reports at 980×620 and 640×400 establish Qt geometry on macOS only.
The first full gate found seven flat-filesystem crashes while CPanel had no
bound folder during scanning; the command projection now checks that transient
state before querying IsRootFolder. The affected eight CTest suites pass 8/8,
13.45 s; final formatting/build/all CTest gate passes 130/130, 87.96 s on
macOS arm64, 2026-09-29. This completes current P0-04 parent navigation scope;
the named P0-05/P0-09/P1-08/P1-11 owners retain their omissions.

## P0-04.1 — source navigation/parent-stack component (accepted prerequisite, 2026-09-29)

Latest internal scope (2026-09-29; supersedes earlier partial callback snapshots):

MainWindow root/nested/parent consumers now call the source CPanel BindToPath,
OpenItemInArchive and OpenParentFolder bodies. The Qt ArchiveParent stack and
its duplicate temporary-folder owner have been removed. Archive rows query the
current CAgentFolder properties; address presentation adapts the existing
separator to a native binding path. Ordinary disk binding also uses BindToPath.
The file-only engine resource retains ArchiveInfo for presentation beside its
existing authoritative metadata transport; it does not add a parent stack.
Source CAgent::ReOpen, GetPathParts and ordinary-folder restore control flow
back a narrow Qt worker/modal refresh bridge. Refresh failure must release
invalidated native proxies. CSelectedState crosses the delayed Qt row-loading
boundary as a presentation snapshot so source parent-focus requests survive.
These consumers complete the integrated P0-04.1 prerequisite. App/affected builds pass;
focused native_navigation_window, nested_archive, source_extract_backend,
panel_navigation, external_launch and source_update_backend pass 6/6, 3.37 s
on macOS arm64 2026-09-29. Real archive cases include unknown extension,
TAR/GZip, nested Unicode/space ZIP and externally updated ZIP refresh with
selection/parent focus. Native selection restoration suppresses notifications;
the Qt presenter must refresh command availability afterward (outside-open
regression covers this). A source transport cancellation now returns E_ABORT
without SetOperationResult(DataError), matching native engine callback semantics;
the regression verifies no erroneous result callback, idle backend and preserved
archive bytes. Native CPanel destructor CloseOpenFolders is retained in Panel.cpp
after stopping Qt notifications and joining per-panel external workers; a nested
memory temp-directory fixture verifies cleanup and preserved original archive.
Window listing fixtures now finish requests inside the source modal owner;
busy-command, keyboard, password, cancel, error, selection and mutation-refresh
assertions remain active during that lifecycle. Fixtures create owned local files
because source binding checks file existence. Bookmark disappearing-folder cases
check restore-to-surviving-ancestor control flow. A flat-mode renderer guard stops
old row selection notifications while the native provider changes its item count.
Deferred selection is consumed only after native row loading, then cleared on
folder release; fresh archive opens cannot inherit another owner's selection.
Root opening retains backend result text in the Qt status/journal and presents
failed opening after the native joined modal lifecycle. Native progress splits
directory and basename across lines; folder properties retain trailing separators.
Focused bookmark passes 1/1, 0.61 s; properties/panel_sort 2/2, 1.29 s;
panel_selected_state 1/1, 3.45 s; settings/panel_key 2/2, 3.82 s;
panel_runtime/archive_selection_runtime/binding_lifecycle pass individually
2.14/2.21/2.93 s; ui_operations/ui_archive_states/properties_runtime pass
3/3, 4.49 s. These macOS arm64 results precede the final formatting/full gate.
Final acceptance: check.sh formatting/build/all CTest PASS 119/119, 58.65 s,
macOS arm64 2026-09-29, /tmp/sevenzip-p0-04-1-check-final.log. Initial sandbox
run could not create private DBus sockets; the permitted local gate used required
socket/process permissions. The first unsandboxed run caught obsolete checksum
post-open waiting and timestamp catalog expectations; both were corrected and
the final complete gate passed. Additional nested_update drives native
OpenParentFolder/changed-file prompt through a real ZIP update, failure and cancel:
successful replacement cleans owned temp; unsuccessful update preserves both
outer archive bytes and changed temp. Focused nested_update/checksum pass 2/2,
0.90 s; timestamp catalog 1/1, 0.25 s. Native extraction callback-contract tests
have their own focused target (operation suite279 lines, contract137).
Current-build 16 archive-runtime JSON reports show address/active view fully
visible at 980×620 and 640×480 in all four normal/flat modes. No Windows rendered
reference or Windows/Linux runtime validation is claimed. All 88 newly vendored
files match the exact recorded upstream baseline byte-for-byte. Required formatting
of adapted src/tests is separate from functional adaptations. The prerequisite
was committed as `d0bc1d2`; full P0-04/P0-03 command acceptance,
known/editor/child-process workflows, model/transport and other parent omissions
remain mandatory under their recorded owners.

Commit scope review: all 248 staged paths belong to this integrated prerequisite,
including its retained unfinished work recorded in the sequential-goal handoff.
The scope contains 62 adapted upstream files, 46 Qt/engine integration files,
39 regression/consumer files, 88 exact upstream dependencies and 13 build/docs/UX
evidence files. Native extraction, update, hash callback and prompt dependencies
are needed by PanelCopy/OpenItemInArchive/changed-file return; they are not separate
completed packages. Every non-vendor source/test basename is accounted for in
this audit, the handoff or CMake registration. The index matches the checked
working tree; the 88 vendor files match the recorded baseline byte-for-byte.
No local memory, temporary files, plan/prompt edits or upstream changes are included.

External launch dependency now copies SplitCmdLineSmart and native editor/viewer
ReadRegEditor/SaveRegEditor key selection with explicit QSettings FM storage.
The native no-process-handle branch of CTmpProcessInfo/MyThreadFunction and
CTmpProcessInfoRelease is connected to an injected association service.
CExitEventLauncher fields/event/thread accounting remain source-derived; the
owner is panel-scoped instead of a mutable global. Native Thread_Wait_Close
replaces the Win32 wait/close calls and always joins before panel members die,
including hard exit (the original 100 ms timeout could outlive a Qt/POSIX owner).
Tracking capacity is allocated before launch/thread creation, preventing an
allocation exception from freeing a record already used by its worker.
IsReadOnlyFolder/IsThereReadOnlyFolder are copied at their original PanelMenu path.
Exact unchanged Common/AutoPtr.h supports the source unique-pointer ownership.

The association service intentionally returns no process handle; successful
launch therefore retains the temp until owner exit. The original tail checks
the baseline file size/time before deleting: changed data remains on disk.
Known/short-lived main processes, CChildProcesses snapshot/handle tracking,
StartEditApplication and their active changed-file update sender remain
mandatory P0-04/P1-08 scope. MainWindow now attaches the manager and association
service; the component is not accepted as the complete external-open workflow.
Focused external_launch uses real ZIP extraction and a fake association, never
starts an external application: successful unchanged cleanup, changed-file
preservation and failed-launch cleanup; independent editor/viewer settings and
source quoted/unquoted command split cases also pass. Latest external_launch
plus source_extract_backend passes 2/2, 1.37 s on macOS arm64 2026-09-29 before
the later localized explicit destructor join edit. Rebuilt affected extraction/
update/external consumers after that edit pass 3/3, 2.07 s. Added memory-probe
fallback verifies a plain file is first extracted into memory, returns native
S_FALSE from its archive probe, then flushes to disk for association launch;
external_launch passes after that addition (0.54 s).

QtAssociatedFileServices is narrow Qt/OS glue around the existing injectable QUrl
opener; it reports failure/throw as E_FAIL with a plain-text launch error.
MainWindow now attaches retained manager/association services to its source panel,
with service lifetime preceding CApp member lifetime. Its old navigation consumers
are still being migrated. The service exposes no process handle; original Win32
PIDL/shell handle and extensionless-dot branches of BrowseDialog2::StartApplication
and known/editor process services remain mandatory, not claimed by this Qt API.
Fake opener regression preserves Unicode/space/literal markup in the QUrl path
and verifies plain-text failure. Latest external_launch/application_starts pass
2/2, 0.76 s; app builds on macOS arm64 2026-09-29. No user applications launched.

PanelCopy.cpp now copies CPanelCopyThread, native archive CopyTo/test dispatch,
callback/password/messages/result lifecycle, hash/statistics setup and parent-zone
inheritance. CCopyToOptions and CPanelCallback declarations retain native fields
and method names. HWND/result presentation uses QWidget and the existing source
progress dialog; a scoped resource transport connects the joined worker to 7zz.
The callback receives the explicit settings path. CContextMenuInfo preserves
native defaults and defined-value Save/Load branches with QSettings replacing
the Options registry key. Native filesystem CopyFileSystemItems remains an
explicit mandatory P0-05/P1-02 dependency returning E_NOTIMPL in that branch;
later update/copy-from/delete PanelCopy bodies are not ported by this subset.
CApp callback implementation/title ownership remains mandatory P0-02 integration.

PanelItemOpen.cpp additionally copies OpenAsArc_Index, IsVirus_Message and the
internal OpenItemInArchive flow: direct stream, bounded CVirtFileSystem, disk
fallback, password inheritance, native corrected name, zone options, archive
probe and temp/link ownership. IsItem_AltStream is copied from PanelItems.cpp;
Explorer/ContextMenu.cpp contains only its original shared FindExt dependency,
not shell integration. RAM information is obtained through the native portable
System service per invocation instead of mutable globals; deep nesting guards
the source shift against an out-of-range shift. Error presentation uses native
MyFormatMessage and Qt plain-text prompts. The external no-handle association
subset is described above; known-process/editor tracking and changed-temp update
sender remain mandatory, and unavailable services report unsupported. MainWindow
still uses its old stack; this is internal WIP, not completed navigation parity.

Source extraction now implements the native all-items contract (ignores a
nonnull indices pointer for count == UInt32(-1)), validates selected indices
before side effects, reports total unpacked bytes and accumulates CopyStream
progress between entries. A skipped callback stream retains the native skip
contract. Public CPanel::CopyTo disk/memory and native nested open/parent return
are tested with real binary ZIP fixtures and Unicode/space names; the latest
rebuilt source_extract_backend passes on macOS arm64 2026-09-29 (0.75 s).
All-items/multiple-entry progress regression now passes: a deliberately invalid
nonnull index is ignored for the all-items sentinel, both binary files match,
and completed bytes remain monotonic through the combined total. Rebuilt direct
consumers pass 10/10, 3.41 s: extract_prompt, ui_parity_overwrite/ui_parity_mem,
source_extract_backend/source_update_backend, native_hash_bundle,
native_extract_callback, panel_navigation, archive_opening and
compression_preferences. Separate context_menu_settings passes (0.27 s),
including undefined flags/unrelated setting preservation and invalid values.
No full gate, accepted iteration or commit is claimed for this continuation.

The native OnOpenItemChanged message body now takes a typed CTempFileInfo/prefix/
password request instead of LPARAM/window handle. Source prefix guard, stale
index relocation by relative name, saved selection, disable-notify, update and
result flow are retained; a released-folder guard is the necessary Qt lifetime
adaptation. Real ZIP regression rejects mismatched prefix and removed item with
no engine start, relocates index 99 to the existing Unicode name, updates only
that file, then rejects a released owner. Latest affected extract/update passes
2/2, 1.71 s (macOS arm64 2026-09-29); external process sender remains mandatory.

Exact unchanged resourceGui.rc is now included in compiled native STRINGTABLE
inputs, resolving missing HashGUI/test/open/moving source messages. Fixed Russian
captions remain the presentation adaptation pending the full language owner.
hash_results checks real CRC32 rows, source title/flags/no initial selection and
list/footer/buttons at 540x360 and 420x360; current JSON has visible 518x307/398x307
lists and separate footer. The test uses QPushButton sizeHint for caption sizing
(the inspection labelFits helper is specifically for QLabel). Latest hash_results,
source_extract_backend/source_update_backend and resource_strings_generator
pass 4/4, 2.05 s, macOS arm64 2026-09-29. No PNG/rendered Windows parity claim.

Hash dependency scope for PanelCopy: MethodProps.cpp now copies the complete
original module (only canonical includes/formatting adapted). HashCalc.cpp ports
CHashBundle::SetMethods/InitForNewFile/Update/SetSize/Final, native AddDigests,
CHasherState::AddDigest/WriteToString and HashHexToString. The rest of the hash
file reader/writer/verifier/signer remains mandatory P2-03 scope. Exact unchanged
CreateCoder.cpp/FilterCoder.cpp/.h/RegisterCodec.h provide source factories.
HashGUI.cpp/.h port only native value/bundle result formatting and ShowHashResults;
the existing source-derived Qt ListViewDialog replaces HWND/Create. HashCalcGUI
and its separate worker remain mandatory with the hash owner.

Unchanged native C hash implementations/registration dependencies now supply
CRC32/CRC64/CKSUM/MD5/SHA1/SHA256/SHA384/SHA512/SHA512-256/SHA3/XXH64 and BLAKE2sp.
The global BLAKE2sp hasher/registration block is copied from Rar5Handler.cpp at
its original adapted path; the archive-handler body is not copied by this
dependency port. Applicable source/license notices are preserved. All dependencies
use the same recorded full upstream revision. CMake OBJECT/interface linkage
keeps the original registration constructors in final consumers instead of
silently discarding them from static archives. HashCalc is in sevenzip_common,
HashGUI in sevenzip_panel, preserving dependency direction; archive operations
continue through ArchiveBackend and the official external engine.

Native hash regression checks CRC32's fixed vector/statistics, incremental
MD5/SHA1/SHA256/SHA384/SHA512/SHA3-256 against Qt's independent digest provider,
wildcard registration including BLAKE2sp/CRC64/XXH64, and unsupported method
failure. App/affected targets build; native_hash_bundle plus source_extract_backend
passes 2/2, 1.03 s, macOS arm64 2026-09-29. Native IArchiveFolder and zone setter
interfaces are now advertised by CAgentFolder with their source bodies; the
complete IInFolderArchive owner and full source PanelCopy integration remain
mandatory. This dependency work does not close P0-04.1 or P2-03.

Agent.cpp now also contains the full native CAgentFolder::Extract body;
ArchiveFolder.cpp copies CopyTo and zone setters. Native path parts, extraction
callback setup/options, selected real indices, hard-link preparation and closer
remain source bodies. Native IArchiveFolder and zone setter COM interfaces expose
these bodies; the complete IInFolderArchive owner remains mandatory.
The Win32 alternate-stream prefix
check is guarded on POSIX, where a colon is a filename character. New source
fields retain upstream defaults. QtArchiveFileResource now attaches an extraction
service and advertises folder operations when that capability exists, even for
read-only archives; update remains independently guarded.

The file-only boundary invokes ArchiveBackend::extractSourceEntry per selected
native entry. Official 7zz binary stdout goes to an owned private staging file;
stderr carries process messages/progress/password prompts. Native CopyStream
then feeds the source callback stream with its progress/break contract. Subsequent
jobs restore normal channels. Original paths are CLI selectors, never staging
filesystem names; source callbacks own corrections and final filesystem names.
CLI selection cannot distinguish duplicate original names, so that unsupported
transport branch returns E_NOTIMPL before concatenating data and remains mandatory
codec/index transport debt. Error distinctions/metadata links and complete
multi-item progress remain under review, not accepted parity.

New source_extract_backend checks native public CopyTo into disk and bounded
virtual files, Unicode/spaces/binary bytes, binary-vs-message channel separation
and channel reset, existing output preservation, failed process start, cancelled
child cleanup/original preservation and malformed archive cleanup. App/affected
targets build; focused extraction/update/opening passes 3/3 (latest command
completion recorded in local memory). No P0-04.1 full gate/commit/acceptance.

FileManager `ExtractCallback.cpp/.h` now copies the full CExtractCallbackImp,
CGrowBuf/CVirtFile/CVirtFileSystem functional bodies from
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` at
https://github.com/ip7z/7zip. Includes and required formatting are adaptations.
OverwriteDialog.cpp/.h retains CFileInfo, ReduceString, AddSizeValue,
SetFileInfoControl text body, default/extra actions and native returned IDs;
MemDialog.cpp/.h retains AddSize_GB/AddInfoMessage_To_String, RAM/range/default
branches and OnContinue parsing/result fields. Win32 controls/layout/modal calls
become Qt Widgets; shell icons use QFileIconProvider with the optional second
EXE/ICO icon omitted. A separate edit with arrow buttons preserves typed values
outside the native spinner range. Native result constants have a narrow portable
header. New modal bodies execute completely on the GUI thread through a joined
source progress parent; Password delegates to the existing scoped presenter.
The global DisableUserQuestions switch becomes callback-owned state. Settings
use explicit Ini paths for native Read/Save_LimitGB and show-password branches.
POSIX filesystem times use native FiTime/FILETIME conversions; Win32 read-only
alternate-stream attribute restoration stays Windows-only. Common zone helper
bodies are compiled on POSIX for CVirtFileSystem too: a colon-suffixed POSIX
file is a named file, not a Windows alternate stream. Native extraction zone
activation remains Windows-guarded; OS-specific zone propagation still requires
integrated review, and is not declared N/A.

Exact dependencies added: Common/Lang.h, OverwriteDialogRes.h/.rc,
MemDialogRes.h and resourceGui.h. Overwrite STRINGTABLE is included in the same
compiled source resource owner with fixed Russian captions. The reference tool
now parses ICON, with a deterministic regression to avoid swallowing adjacent
resource statements. Generated pinned reference/contract pairs cover both new
views. New extract_prompt cases exercise native memory/spill bytes and filename
correction, callback stop and persisted memory limit, overwrite answers/default/
cancel/native formatting, and memory field enable/invalid/commit/cancel.

Evidence: app and affected targets build; focused
`^(extract_prompt|ui_parity_overwrite|ui_parity_mem|ui_parity_tool|native_extract_callback|source_update_backend|archive_opening)$`
passes 7/7, 2.04 s, macOS arm64 / Qt 6.11.2, 2026-09-29. Each view has four
current-build JSON reports at actual widths 700/560, no tested clipping; Windows
rendering and Windows/Linux runtime unverified. This is internal WIP, not a
completed iteration/commit. Full AgentFolder Extract/CopyTo, file-only engine
extract transport, PanelCopy/OpenItemInArchive and MainWindow's one native
navigation owner remain mandatory before P0-04.1 acceptance/full gate.

Baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, retrievable from
https://github.com/ip7z/7zip. P0-10.1 prerequisite is accepted at `d966b79`.
Inspected `CPP/7zip/UI/FileManager/PanelItemOpen.cpp` OpenAsArc/Name/Index,
OpenParentArchiveFolder, CThreadCopyFrom/OnOpenItemChanged and
OpenItemInArchive memory/temporary/password/zone branches; PanelFolderChange.cpp
BindToPath/LoadFullPath/OpenParentFolder/CloseOneLevel/CloseOpenFolders;
FileFolderPluginOpen.cpp CThreadArchiveOpen/CFfpOpen; OpenCallback.cpp;
Common/ArchiveOpenCallback.cpp; Agent/ArchiveFolderOpen.cpp and
ArchiveFolderOut.cpp CopyFromFile/CommonUpdateOperation. These bodies, required
dependencies and existing root/nested/address/parent consumers form one result.
The old Qt ArchiveParent stack remains migration debt until replaced; no helper
or newly copied dependency closes this iteration or its parent.

Current internal dependencies: unchanged FileStreams.cpp/.h, StreamUtils.cpp/.h,
UniqBlocks.h and ArchiveOpenCallback.cpp/.h copied under third_party from the
exact baseline. Native open/volume stream lifetime, counters, password and
reopen contracts retained. Full Common/Wildcard.cpp/.h replaces the previous
partial copy in its existing mirrored path. One source EnhancedMaskTest body
is parameterized for wchar_t/native case policy and UTF-16/Windows-style Qt
selection; Qt overloads only convert/present at that boundary. All targets now
link the one common implementation instead of compiling the same source again.
Canonical includes and mandatory formatting are separate adaptations.

ArchiveExtractCallback.cpp currently ports only complete CLinkLevelsInfo::Parse
and IsSafePath dependencies needed by native volume opening; all other extraction
bodies remain mandatory. FileDir.cpp gains original GetFullPathAndSplit, POSIX
FiTime_To_timespec/SetFileTime_Base/SetDirTime and C_umask bodies required by
unchanged FileIO/FileStreams. MyGetFullPathName's existing GetFullPath forwarding
is collapsed to that same native call; no filesystem path algorithm is replaced.
FileIO.cpp is now built on POSIX as well as Windows.

OpenCallback.cpp/.h copies all functional counter/break/finished/password bodies;
HWND/INT_PTR become guarded QWidget/qintptr. Qt password creation/retrieval runs
on the GUI thread through the existing callback service, including accepted
empty password and ShowPassword. Original Read_ShowPassword/Save_ShowPassword
use explicit settings path/QSettings instead of Registry and preserve their
default false/read/write scope. No password is persisted.

Status: active WIP. Affected dependency/selection/update targets build; integrated
source/consumer acceptance and focused checks pending. No full gate or commit
is appropriate before the integrated navigation result.

Further current WIP: FileFolderPluginOpen.cpp/.h retains complete active source
worker, error-level composition, callback creation/result/password/lifetime and
library transfer flow; injected native IFolderManager replaces only construction
of the in-process CArchiveFolderManager at the engine boundary. CPluginLibrary
becomes its native CLibrary base because disabled plugin-loading branches are
not executed. Settings path and QWidget parent are explicit Qt services.
PanelItemOpen.cpp now contains OpenAsArc/Msg/Name, OpenParentArchiveFolder and
CThreadCopyFrom/OnOpenItemChanged bodies; PanelFolderChange gains native folder
interface Release/QueryInterface, LoadFullPath and CloseOneLevel/CloseOpenFolders.
MainWindow still uses its former stack: runtime integration remains mandatory.
Source changed-temp YES updates, NO/CANCEL cleanup and update-error preservation
are retained; original CANCEL on this prompt does not cancel leaving the folder.
Native StreamObjects.cpp/.h and C/Alloc.c/.h are exact unchanged dependencies for
memory streams. POSIX ProcessWasFinished also signals the existing finished
latch for FileFolderPluginOpen's own worker entry point; Create still joins it.

Focused evidence so far: panel_selection/update_callback 2/2, 0.75 s;
open_callback 1/1, 0.34 s; panel_navigation 1/1, 6.04 s before the last POSIX
generic-worker latch extension. These verify component contracts only, not
completion of P0-04.1 or real engine/navigation consumers. macOS arm64 2026-09-29.

After the generic-worker latch extension, focused progress_worker/navigation/
open_callback/archive_progress/progress_cancel pass 5/5, 5.45 s; native navigation
fast-finish test now 0.34 s. Further WIP copies BindToPath and SetToRootFolder
functional bodies, with only CDirEntry replacing CFileInfo in the POSIX enumerator
boundary (Windows aliases them). RootFolder ordinary filesystem binding and
CFSFolder::InitToRoot are exact early prerequisites; full root/drives/network/
Browse namespace remains P1-11, ADS P0-07 and system icons P0-09. RootFolder's
network construction/fallback and ADS branches explicitly return E_NOTIMPL
until those required owners are ported, not done/N/A. Native drive provider
still has its documented unfinished volume capability. Names/indices, root and
ordinary path binding retain source flow; Qt DocumentsLocation substitutes the
Windows shell path service, icon indices remain -1 pending icon presentation.
This is not acceptance of the full RootFolder module or its namespace.

Further WIP in the same iteration: source BindToPathAndRefresh,
GetParentDirPrefix, OpenParentFolder and OpenRootFolder bodies are copied;
RefreshListCtrl's no-argument overload retains its source CSelectedState wrapper.
The existing selection-only overload still lacks the full list load/presentation
stage; MainWindow consumer replacement is mandatory before acceptance.

QtArchiveFolderManager now implements the injected native IFolderManager boundary
for the source CFfpOpen worker. GUI process requests, password requests and stop
are queued to ArchiveBackend; the worker observes completion even while paused
and waits for engine quiescence. The shared ArchiveProgressTransport supplies the
same native owned-process controls used by Add rather than another implementation.
Forced archive format is passed through ArchiveBackend::openFolder, not widgets.
Source COpenArchiveCallback and COpenCallbackImp retain password/volume contracts.
The engine's listing transport maps the exact Console/List.cpp
Print_OpenArchive_Error/Open_Strict S_FALSE labels at this baseline; unknown
output, start/I/O errors and crashes remain E_FAIL, cancellation E_ABORT.
Only error-label comparison uses canonical Unicode on macOS, whose engine emits
filesystem-native NFD; native/archive entry names are preserved.

Exact unchanged Compress/CopyCoder.cpp/.h and StdAfx.h are additionally vendored
from the same baseline. Native IInStream opening uses its CopyStream algorithm
and native COutFileStream at the file-only engine boundary. QtArchiveFileResource
supplies only a private temporary filename and cleanup; every bound folder retains
the source CAgent, which now retains the external engine resource in place of an
in-process archive stream owner. Failed opening removes the spool, successful
opening retains it until the last source folder is released. This does not yet
accept OpenItemInArchive extraction-to-memory, virtual-file update/writeback,
zone branches or real nested/MainWindow consumers; those remain mandatory.

Focused evidence: opening/progress/navigation/open_callback 4/4, 2.08 s after
GUI password/cancel and macOS label fixes; after memory-spool integration,
archive_opening/panel_navigation 2/2, 0.96 s. Real 7zz 26.03 (matching baseline
7zVersion.h) ZIP/Unicode-space file and native memory-stream fixtures pass,
including folder-retained spool lifetime, S_FALSE cleanup, empty password,
password cancellation, stop-confirm/quiescence and failed engine start.
After source parent/root-method integration and additional parent/pop/root
regressions, the same four focused tests pass 4/4, 2.34 s. All evidence is
macOS arm64 on 2026-09-29; Windows/Linux runtime and integrated final acceptance
remain unverified. No additional helper commit/full gate has been made.

Changed-file update integration has an additional exact dependency: existing
AgentProxy uses QString/std containers and listing-based Load in place of native
owned names, vectors, CArc/IInArchive properties and full memory/CRC/raw handling.
The old accepted evidence remains limited to its documented copied control-flow
subset; it does not validate the omitted native model. Source
ArchiveFolderOut::CommonUpdateOperation saves/restores path parts and reopens the
source CAgent after its work-directory transaction; AgentOut::UpdateOneFile
builds native CUpdatePair2 values for one real index and keeps original names.
These bodies cannot be replaced by calling the old delete-then-add Qt
replaceEntry workflow and described as a source port. Native model, update pair,
work-directory/temporary/move/reopen prerequisites must accompany this consumer;
their full parent-model/update acceptance remains under P0-03/P1 owners.

Current WIP replaces the earlier partial AgentProxy implementation with the full
baseline AgentProxy.cpp/.h: CProxyArc/CProxyArc2, native owned names/vectors,
property/default-name/CRC/memory-limit/raw-parent handling and path/real-index
algorithms. Qt path lookup used by remaining presentation consumers is now a
narrow adapter outside this borrowed module. QtPanelFolder exposes a read-only
IInArchive metadata view over the same engine catalog; a forwarding metadata
adapter supports existing IFolderFolder providers. Native CAgent::ReadItems calls
CProxyArc::Load(CArc, progress), rather than rebuilding the tree from a Qt list.
Full OpenArchive.h declarations and its item-boolean/default-path helpers are
copied; the remaining opening/parser/CArchiveLink bodies are still mandatory,
not accepted merely because their declarations are present. Exact unchanged
LoadCodecs.h, DirItem.h, Property.h and Archive/Common/ItemNameUtils.cpp/.h plus
StdAfx.h are additional dependencies from the same recorded baseline.

One necessary functional deviation fixes CProxyArc2::Load's BSTR fallback:
the baseline declares CPropVariant inside the fallback branch but consumes its
bstrVal pointer after that scope destroys the value. The value now remains alive
through AllocStringAndCopy. A deterministic native-tree fixture with Unicode,
spaces, invalid parents, memory-limit failure and reload covers this path.
Temporarily restoring the baseline scope makes the name assertion fail (empty
root name versus `root й space`); restoring the fix passes. Native model/opening/
navigation/property/agent focused tests pass 5/5, 1.61 s on macOS arm64,
2026-09-29; the application builds. Earlier historical Qt-container claims remain
evidence only for their recorded scope, superseded by this current unaccepted WIP.

Work-directory prerequisites now copy full Windows/FileDir.cpp/.h source bodies
and Common/WorkDir.cpp/.h, including CTempFile creation/exclusive collision retry,
work-mode selection, output lifetime and MoveToOriginal. CTempDir's unchanged
source Create/Remove flow is enabled on POSIX; its Win32-only recursive removal
dependency uses the existing Qt filesystem boundary there. QString/cancellation
cleanup overloads are retained for current consumers. Exact C_FileIO.h and
MyBuffer2.h are vendored dependencies. NWorkDir::CInfo preserves defaults and
load validation; QSettings replaces Registry using explicit settings paths and
original option names. CreateTempFile takes that explicit path instead of reading
implicit global settings. This is prerequisite scope inside P0-04.1, not full
filesystem or update acceptance. Focused work-directory/filesystem cleanup/
operations/opening/navigation tests pass 5/5, 1.59 s (macOS arm64, 2026-09-29).
They cover Unicode/space paths, specified/current/default work modes, missing
specified-path fallback, publication, abandoned-output removal, invalid work
directory preserving the original, and POSIX temp-directory symlink cleanup.

The same result now includes complete source EnumDirItems.cpp/.h and SortUtils.cpp
plus exact UpdateAction.cpp/.h, UpdatePair.h, UpdateProduce.h, SortUtils.h and
Common/StdAfx.h dependencies. Full UpdateCallback.cpp/.h and
Agent/UpdateCallbackAgent.cpp/.h are being connected next; native pair/property/
stream/password/error bodies are retained with canonical dependency includes.
Exact Windows/SecurityUtils.cpp/.h and original InitLocalPrivileges from
ArchiveExtractCallback.cpp supply the retained Windows security branches.
These newly copied callbacks are WIP dependencies, not accepted update consumers.
UpdateCallbackAgent uses native UInt64 instead of Windows-only UINT64 in its two
counter definitions. OpenArchive.cpp now additionally retains the complete native
CArc item path-to-parent/path/path2/item/size/time bodies and alt-stream colon
helper, needed by callbacks; codec-opening/parser/link bodies remain unported.
Focused archive_update_callback/work_dir/archive_proxy tests pass 3/3, 0.80 s.
The callback fixture checks original archive names alongside new disk data,
native update-pair index/flags, stream close notifications, accepted password,
failed-file messages, cancellation and Before_ArcReopen clearing stop status.
Current application and direct UI consumer targets build; archive selection,
panel runtime, binding lifecycle and compression/extract settings checks pass
5/5, 4.99 s. All results are macOS arm64, 2026-09-29. Full P0-04.1 consumer
replacement and required update/reopen transaction remain open; no helper commit
or full gate is appropriate at this point.

Further WIP removes the unused CAgent ArchiveEntry list and duplicate retained
folder-property pointer. CArchiveLink owns the authoritative native CArc metadata;
CAgentFolder and its remaining Qt selected-path consumer query that owner.
Original CArcErrorInfo::ClearErrors and CArchiveLink KeepModeForNextOpen/Close/
Release bodies are copied, along with the source RAM-limit policy for CProxyArc.
AgentOut.cpp currently copies only SetFolder, SetInArchiveInterfaces, CommonUpdate
and full UpdateOneFile; IArchiveFolderInternal/GetAgentFolder and CanUpdate use
their original bodies. SetFolder is temporarily a regular method on the partial
owner, rather than advertising an incomplete IOutFolderArchive interface.
All other AgentOut operations, full Agent interfaces/raw-folder branches and the
ArchiveFolderOut transaction/reopen consumers remain mandatory.

The source UpdateOneFile fixture passes (archive_update_callback, 0.37 s): only
one real index inside a bound subfolder changes, the archive name survives a
different disk filename, unrelated pairs remain unchanged, and invalid selection/
missing disk file/tail guard fail before the engine call. Later affected native
owner/opening/callback/backend checks pass 4/4, 1.67 s, macOS arm64 2026-09-29.
All newly vendored files compare byte-for-byte with the recorded baseline;
the sibling checkout has no modifications.

ArchiveBackend::updateSourceFile is a new narrow process transport for native
IOutArchive callbacks. The exact baseline ArchiveCommandLine.cpp
ParseUpdateCommandString/SetAddCommandOptions protocol supplies `-u-` and the
separate `!output` command; selected-file pair states force new data/properties,
while unselected entries are copied. It writes an uncommitted result for native
WorkDir publication and never invokes the old Qt delete-then-add workflow.
Current standalone engine fixtures pass for ZIP/7z, original bytes/other entries,
cancel after native process pause, failed start, existing-output preservation and
unsafe names. Source ExtractingFilePath.cpp/.h is copied in full for this new
boundary: IsSafePath and Correct_FsPath validate staging names. Names requiring
correction are rejected at the engine boundary because changing their name would
violate native KeepOriginalItemNames; other extraction's former safePath remains
migration debt in its own required scope. NUL cannot cross a native path boundary.
QtArchiveSourceUpdate now supplies the narrow file-only IOutArchive service.
The source AgentOut UpdateOneFile produces the native callback plan; the service
accepts only that plan, stages GetStream through unchanged CopyStream/FileStreams,
and copies the separate engine result into the caller's native output stream.
Qt only provides private names/directories and queued process services. The
archive property wrapper retains the resource and advertises IOutArchive only
when that service is attached. Read-only consumers do not acquire a backend link
dependency: the resource stores an injected service, not engine decisions.
The native worker fixture now calls CAgent::SetFolder/UpdateOneFile for a bound
subfolder with a differently named disk file; original archive bytes, original
entry name, sibling entry and closed source input streams are checked against
the real 7zz result. QtArchiveBackendRequest's modal worker fixture checks that
source Create waits for process completion. Both remain internal P0-04.1 WIP:
production transaction/reopen/MainWindow consumers are mandatory and incomplete.

CArc::ReadBasicProps and Archive_GetArcProp_UInt/Int are copied with their full
source bodies from the same baseline; the existing GetOpenArcErrorFlags owner is
reused. ArchiveInfo now transports the engine's numeric Offset header alongside
Physical Size. QtArchiveFileResource supplies only those typed properties and
the actual source file size; native ReadBasicProps computes tail/unexpected-end
and CanUpdate retains its native tail guard. Tests cover exact fit at offset 16,
tail size 32 and unexpected end. Prefix publication remains mandatory; the current
update service rejects nonzero offsets until native CommonUpdateOperation is
connected. Metadata catalog enumeration and remaining archive header properties
are still required. Focused source_update_backend/archive_update_callback/
archive_opening/archive_proxy pass 4/4, 1.70 s; app and affected targets build
on macOS arm64 2026-09-29. No P0-04.1 full gate or acceptance is claimed.

Further internal P0-04.1 work copies native CArc::ReOpen, CArchiveLink::ReOpen,
CAgent::ReOpen/Close and Agent.h KeepModeForNextOpen from that same baseline.
The unchanged LimitedStreams.cpp/.h and its StdAfx.h are vendored dependencies.
The existing native archive wrapper is retained across reopen; a queued file-only
service relists metadata into that wrapper, using the same catalog construction
as initial opening. It does not create another CAgent or duplicate folder tree.
Codecs are NULL at this integration boundary because the external engine owns
codec selection. The empty-link ReOpen→Open2 branch is explicitly unsupported
until full codec/open-owner capability is connected, and remains mandatory.
Nonzero-offset stream/property adjustment also remains required before accepting
the native SFX branch; the current update service rejects such archives.

ArchiveFolderOut.cpp now copies GetPathParts, the original empty-directory helper,
C_CopyFileProgress_to_FolderCallback_MoveArc, CommonUpdateOperation and
CopyFromFile. The CopyFromFile branch retains native WorkDir creation, output
stream ownership, callbacks, close/publish/reopen, proxy reset and path restoration.
CAgent's regular methods replace calls through its not-yet-complete native COM
owner interface. Other AgentOut command branches explicitly return E_NOTIMPL
and remain mandatory under their owning operation packages; this is a partial
module, not a complete ArchiveFolderOut port. POSIX directory inspection uses
upstream CEnumerator::DirEntry_IsDir(..., false); clearing the Windows read-only
attribute is Windows-only. CopyFromFile is now the native COM method on a
conditional IFolderOperations contract. That contract is available only with
an attached update service and native CanUpdate approval; the limited engine
service additionally disables it for nonzero offsets. Unavailable CreateFolder/
CreateFile/Rename/Delete/CopyTo/CopyFrom/SetProperty remain explicit E_NOTIMPL
adapters, not completed ports. The resource owns an injected progress factory
that retains the resource and sets/clears the nonowned CProgressSync pointer
through the source worker join. QtArchiveFolderManager attaches metadata/reopen
and ZIP/7z update services when the initial listing succeeds.

The real-engine source transaction fixture passed (source_update_backend,
0.82 s, macOS arm64 2026-09-29): it invokes CopyFromFile in a bound subfolder,
then observes the updated size through that same folder after native path
restoration, extracts updated bytes and checks the unchanged sibling. Earlier
standalone WorkDir/reopen fixture passed together with archive_opening (2/2,
1.42 s). These checks do not complete the integrated navigation iteration.

The current real panel fixture invokes CPanel::OnOpenItemChanged, its original
CThreadCopyFrom, conditional native IFolderOperations and native transaction
through the resource progress factory. Updated size is observed through the same
panel folder, and real extracted bytes plus the unchanged sibling are checked.
Native cancellation at engineStarted returns E_ABORT after quiescence and leaves
the original archive unchanged. Focused source_update_backend/
archive_update_callback/archive_opening/archive_proxy pass 4/4, 1.96 s on macOS
arm64 2026-09-29; app and affected targets build. MainWindow still uses its former
Qt ArchiveParent stack; remaining opening/extraction/memory/zone and production
navigation consumers are required before acceptance. No extra commit/full gate.

Common/ArchiveExtractCallback.cpp/.h is now copied in full from the baseline,
replacing the former link/path helper-only implementation. Its native algorithms,
stream/file/link/attribute/time/cancel/overwrite and callback control flow are
unchanged; adaptations are canonical includes and project formatting. Exact
ProgressUtils, FilePathAutoRename, CreateCoder.h and MethodId.h are vendored.
HashCalc.h and the previously partial MethodProps.h now carry full native
declarations/inline bodies with adapted includes; their remaining implementation
and active hash consumers remain mandatory in their owners. Common extraction,
native opening stream services and CArc bodies are compiled in the filesystem
library alongside their file/link dependencies, rather than making the common
string/contracts library depend on filesystem operations. Engine-owner targets
explicitly link that dependency. There is still one authoritative implementation.

The full callback exposed metadata debt in QtPanelFolder: IInArchive unavailable
properties must return S_OK/VT_EMPTY, rather than E_NOTIMPL, and MTime must be a
native FILETIME. The adapter now converts the local timestamp presentation emitted
by List.cpp to FILETIME with native precision metadata, preserving seven fractional
digits. There is no reverse CLI timestamp decoder in the borrowed application
module: this conversion is an external-engine data/Qt timezone boundary, not a
replacement of native CArc timestamp or extraction logic. Custom presentation
snapshots retain their former text contract. The native CArc test observes a
31-tick difference between two source timestamps and rejects malformed text to
VT_EMPTY. Metadata absent from the listing, including links/attributes, remains
explicitly unavailable and mandatory for the complete model/engine transport.

Native extraction callback fixtures exercise Unicode paths, source traversal
correction confined to destination, existing-file skip and cancelled progress.
These tests failed before the typed metadata fix and pass afterward. Affected
native_extract_callback/source_update_backend/archive_opening/archive_backend/
compression_preferences/open_callback pass 6/6, 3.33 s; app and affected targets
build on macOS arm64 2026-09-29. Full source FileManager ExtractCallback,
virtual filesystem/memory and actual engine Extract/CopyTo consumers are next
internal tasks. This is not P0-04.1 or P0-06/P0-10.2 acceptance.

## P0-10.1 — integrated progress/callback component (2026-09-29)

This integrated component is accepted in its stated scope. Historical evidence
below is retained; remaining consumers are grouped in the checklist's iteration map.
The reproducible source baseline is
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
[ip7z/7zip](https://github.com/ip7z/7zip).

Copied source functional bodies: FileManager/ProgressDialog2 modal creation,
native worker/Process/result/exception composition, finish and cancel-confirm;
GUI/UpdateCallbackGUI2 Init/operation/move/password state;
FileManager/UpdateCallback100 all eight COM callback interfaces and methods;
FileManager/ExtractCallback::SetExtractErrorMessage. Adapted application files
retain their upstream paths under src/CPP/7zip/UI. Exact unchanged interfaces
Archive/IArchive.h, IPassword.h and ICoder.h are under third_party/7zip/CPP/7zip.
ClassDefs remains the single GUID definition owner. Agent/IFolderArchive.h and
Common/IFileExtractCallback.h retain all interface declarations, with opaque
CCodecs/CArchiveLink declarations replacing heavyweight engine includes:
the current integration executes official 7zz through ArchiveBackend.

Qt adaptations: modal/event/message/default-button services; atomic worker→GUI
lifecycle flags; explicit per-dialog question policy; POSIX startup wait and
join; password widgets constructed/executed/destroyed on the GUI owner thread
through qtcallbackpassword. Current resource language provider uses original
non-Z7_LANG fallback; complete language-file ownership remains P1-14.
Source Before_ArcReopen clears stop only, retaining pause.

ArchiveProgressOperation is the first connected integration owner: MainWindow
Add/drop uses the original native modal worker and callbacks; its old QProgressDialog
owner is removed for this scenario. Remaining progress/journal consumers are
P0-10.2, not omitted completion. The bridge publishes external paths with source
Set_FilePath and polls completion even while paused: blocking a local callback
in CheckStop would otherwise miss independent engine completion. Unknown byte/file
totals remain unknown; a percent-only atomic fallback updates only bar/title.
Accepted empty password is sent as an empty line through backend stdin, preserving
source callback acceptance. Start exceptions return to original Process composition;
after-start exceptions first quiesce the child through cancelAndWait. Scoped
MainWindow ownership policy restores normal routing after modal completion.

processpause supplies the missing external-process service: POSIX SIGSTOP/CONT,
Windows owned thread handles/native CThread suspend/resume with bounded snapshot
rechecks and rollback. It does not contain archive logic. Relevant native contracts:
[SuspendThread](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread),
[Thread32Next](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-thread32next),
[POSIX signals](https://man7.org/linux/man-pages/man7/signal.7.html).
ArchiveBackend owns the process handle/PID lifetime, native controls and reset
after exit; widgets receive no process IDs or command syntax.

Source button pause failure restores sync state; child priority failure restores
parent policy and reports errors. Startup propagates an active parent background
policy; foreground remains normal spawn policy until an explicit live transition.
Already cancelled startup emits no policy request. POSIX stop/continue delivery
is asynchronous; native owned-child observations confirm stop/resume, not a fake
progress-only pause. Windows preserves another owner's suspend counts and retains
partial resume failures for retry; Windows/Linux runtime remains unverified.

Source/interface body comparisons passed for complete UpdateCallback100 and
IFolderArchive/IFileExtractCallback declarations ignoring only formatting/comments
and escaped macro line breaks. GUI2 matches except the reviewed password service.
The three added vendor headers byte-match the baseline; sibling checkout stays
clean. Current inventory: root C++45, mirrored C++44, third_party paths89
(86 prior tracked paths plus these three headers). Required formatting is separate
from functional adaptations.

Focused evidence on macOS arm64, Qt 6.11.2, 2026-09-29: progress_worker/cancel/dialog
3/3 in 11.45 s; update_callback 1/1 in 0.29 s; process_pause 1/1 in 0.97 s.
Final focused 9/9 passed in 15.81 s; later localized archive/consumer checks passed,
including after-start exception cleanup and cancel of Add preserving the original
archive. Full `./scripts/check.sh` passed formatting, app build and **101/101 CTest
in 72.05 s** on macOS arm64/Qt 6.11.2, 2026-09-29; log
`/tmp/sevenzip-p0-10-1-check.log`. Current-build parity passed all 24 reports/26
controls. Reviewed four new normal/compact Close reports: no clipping or unfit text,
correct hidden Pause/Background, error list and default Close; unknown totals empty.
Native confirmation and password behavior/geometry are covered separately.
Windows/Linux runtime and rendered Windows parity remain unverified and mandatory;
all remaining progress/journal consumers and parent P0-10 acceptance stay P0-10.2.

## P0-10.2 — all current progress/message consumers (2026-09-30)

Source baseline for each borrowed module:
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` from
[ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
The upstream `CPP/7zip/UI/FileManager/MessagesDialog.h/.cpp/.rc` and
`MessagesDialogRes.h` supply `CMessagesDialog::AddMessageDirect`, `AddMessage`,
`OnInit`, `OnSize`, two-column numbered messages and the Close footer. The
functional message-splitting/numbering loops are copied under the original
paths. Win32 `CModalDialog`/`CListView`/DLU services become a narrow Qt
`QDialog`/`CQtProgressListView` presentation; resource IDs and original 456×176
dialog units are retained. Font metrics may enlarge the Close control. The
File menu's local last-operation action reopens the dialog modelessly, so a
completed hash result stays inspectable; this action is a project extension.
The original source's `Create` modal dispatch is not reused for this local
menu workflow. `ResourceString` supplies the Russian `IDS_MESSAGE` caption.

All other current archive operations now use the existing source
`CProgressDialog` via `ArchiveProgressTransport`; filesystem copy/delete and
Split/Combine use it through `QtCallbackProgressTransport`. Their existing
`FileTransferOperation`, `FileDeleteOperation` and `VolumeOperation` workers
remain operation owners; this iteration replaces only the former
`QProgressDialog`/custom journal presentation. The generic transport forwards
confirmed source cancellation to the worker and applies the real native
process-priority service. Those workers have no cooperative pause mechanism,
so the source Pause button is disabled for them. Volume reports actual total
and completed bytes; file deletion reports total/completed items; file
transfer reports accumulated completed bytes, while its total and ETA remain
unknown rather than fabricated. The 7zz process transport retains true child
pause/background controls and the percent-only fallback from P0-10.1.

`CProgressDialog::OnCancel` invokes the narrow transport after the original
YES/NO/CANCEL decision. Unsupported pause is skipped during the source's
temporary cancel-question pause; its normal body and restoration remain.
`ArchiveProgressOperation::ProcessVirt` treats a confirmed stop as E_ABORT even
when synchronous backend completion wins the latch, avoiding a false final
error after cancellation. GUI-thread async consumers initialize source
controls before showing even when their parent window is hidden. Stage handoff
uses `QDialog::done` so a finished stage cannot be mistaken for a new cancel
request. The source close/final message is delivered before command
re-enablement; worker quiescence and partial deletion refresh remain under
the existing operation-state owner. Backend diagnostics and final errors feed
source `CProgressSync`; a single UStringVector retains last-operation messages
for the source MessagesDialog, with no independent QPlainTextEdit owner.

Omitted from this package: source FileManager drag-error caller branches outside
the current workflows, new pause capabilities in filesystem/volume workers,
full language-file loading (P1-14), future P1–P3 operations, rendered Windows
pixel comparison and Windows/Linux runtime checks. Those are not counted as
implemented by the P0-10 parent. Qt 6.11.2/macOS arm64 current-build
normal/compact MessagesDialog JSON reports show both controls fully visible,
ordered and separated, with a fitting Close caption. Focused direct consumers
`progress_cancel`, `checksum`, `ui_operations`, `ui_archive_states`,
`app_lifecycle_window`, `volume_workflow` passed 6/6 in 11.65 s. The final
unsandboxed `./scripts/check.sh` passed formatting, app build and all 137/137
CTest cases on macOS arm64, Qt 6.11.2, 2026-09-30. No Windows/Linux runtime
or rendered Windows reference was available locally. After a documentation-only
scope correction to the progress resource contract, its targeted
`ui_parity_progress` fixture passed unsandboxed (5/5 including dependencies);
the sandboxed native-priority fixture cannot exercise this macOS service.

## P0-12.1 — interactive Ask through the 7zz transport (2026-09-30)

Exact source baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
from [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reviewed `CPP/7zip/UI/GUI/ExtractDialog.cpp/.rc`,
`ExtractGUI.cpp::ExtractGUI`/`CThreadExtracting::ProcessVirt`,
`UI/FileManager/ExtractCallback.cpp::AskOverwrite`/`AskWrite`,
`UI/Common/ArchiveExtractCallback.cpp::CheckExistFile`, and
`UI/Common/Extract.cpp::DecompressArchive`/`Extract`. The existing source
mirrors retain `CExtractDialog` mode ordering/persistence,
`CExtractCallbackImp::AskOverwrite` answer mapping, and the complete original
`CArchiveExtractCallback::CheckExistFile` decision body. P0-12.1 connects the
same `COverwriteDialog` to the user-facing extraction path, sharing the
original native-button-ID→`NOverwriteAnswer` conversion with filesystem
Copy/Move. Qt file metadata is translated to source `CFileInfo` and the
source dialog owns presentation. No upstream file was modified.

The local official `7zz` 26.03 process exposes overwrite interaction only as
English text on stdout plus stdin answers in a disposable conflict probe;
its documented CLI has `-ao{a|s|t|u}` and `-y`, but no structured per-file
callback. Parsing prompt wording or translated paths is unsafe. The necessary
`ArchiveBackend` adapter therefore validates the listed entries, asks for
decisions before extraction, then runs `-aos` for unoccupied entries and
targeted `-aoa`/`-aou` passes for accepted conflicts. `No` leaves an occupied
file untouched; Yes/No to All and AutoRename update per-operation state in the
same order as source `CheckExistFile`. The callback lives in the GUI event
loop; cancellation closes the source dialog and aborts before any pass. Each
subsequent pass keeps password/cancellation state, and an error stops the queue.
This CLI selection/queue is integration code, not claimed as copied source
algorithm or an in-process invocation of `CheckExistFile`.

Differences and remaining mandatory scope: source `CheckExistFile` asks at the
moment of write; this adapter has a preflight-to-write race even after
revalidation. 7zz writes directly, so failure/cancel may leave partial output;
there is no transactional rollback. `ExtractGUI.cpp` and `Common/Extract.cpp`
are not yet source-ported into runtime orchestration; their option precedence,
batch/callback close semantics and relevant omitted branches remain P0-12.3.
The existing non-Ask RenameExisting/Extracted modes, selection validation and
password prompt stay enabled. Filesystem symlink, type and unsafe-path guards
remain in backend validation. P0-12 parent is open until the source integration
scope and final acceptance are checked.

Focused `extraction_options`, `ui_operations`, `extract_dialog`,
`ui_archive_states`, `file_transfer`, `archive_backend` passed 6/6 on macOS
arm64, Qt 6.11.2; real 7z/ZIP Ask, Unicode, NoPaths selected, wrong-password
retry, occupied auto-rename target, unavailable dialog and cancel are covered.
Unsandboxed `./scripts/check.sh` passed formatting, app build and 137/137
CTest cases; after a localized cleanup/fixture edit,
`extraction_options` was rebuilt and passed again. Windows/Linux runtime and
rendered Windows parity are unverified.

## P0-12.2 — root FM extraction through native callback (2026-09-30)

Exact original source is
[`ip7z/7zip` commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17):
`CPP/7zip/UI/FileManager/PanelCopy.cpp::CPanel::CopyTo` and
`CPanelCopyThread::ProcessVirt`, `CPP/7zip/UI/Agent/Agent.cpp::CAgentFolder::Extract`
and `GetRealIndices`, `CPP/7zip/UI/Common/ArchiveExtractCallback.cpp::CheckExistFile`,
`CPP/7zip/UI/FileManager/ExtractCallback.cpp::AskOverwrite` and
`CPP/7zip/UI/GUI/ExtractGUI.cpp::ExtractGUI`. The first five source bodies were
already copied under matching `src/` paths. This iteration connects the root
file-manager Extract command to those bodies; `PanelCopy.cpp` retains its source
thread/callback/error flow, with a narrow `CCopyToOptions::extractMode` branch
that supplies the source `IArchiveFolder::Extract` path/overwrite parameters and
uses the original Extract progress title. The external `7zz` still supplies
archive bytes through `QtArchiveFileResource::AttachExtractService` and
`ArchiveBackend::extractSourceEntry` (file-only CLI stdout staging), not an
in-process codec. No upstream checkout file was changed; notices and license
remain in the mirrored modules and `third_party/7zip/License.txt`.

`MainWindow::extractWithSource` selects this route only for a listed, root,
unencrypted file-backed archive and FullPaths/Ask. It passes source panel indices, or all
root indices, through the native folder proxy; output validation and symlink/
unsafe-name guards remain in `ArchiveBackend`. The source callback now asks at
`GetStream` before each file, so the P0-12.1 preflight-to-write interval is
removed for this route. Source progress owns the modal lifecycle and the
existing backend completion owner ignores each staged stream completion until
the native worker joins. The result then restores command state and journal.
Real UI tests assert Yes, selected No, AutoRename and Cancel, source dialog
metadata, preserved existing bytes and no `ArchiveBackend::overwriteRequired`
preflight signal. Controlled backend consumers retain their async transport.

This is a partial P0-12 port. Nested or encrypted archives, NoPaths and non-Ask
modes still use the P0-12.1 backend path. `ExtractGUI.cpp::ExtractGUI`/
`CThreadExtracting::ProcessVirt` and `Common/Extract.cpp::DecompressArchive`/
`Extract` batch orchestration have been reviewed but not copied or wired;
option precedence, multi-archive statistics, callback close/error composition,
volume handling and related dependencies remain P0-12.4. Source callback writes
directly; failure/cancel can leave partial output. Destination changes after
validation remain a race, and rendered Windows plus Windows/Linux runtime parity
are unverified.

Focused `ui_operations`, `source_extract_backend` and `extraction_options`
passed 3/3; after extending the UI decisions fixture, `ui_operations` passed
again. The full unsandboxed `./scripts/check.sh` passed C++ format, app build
and all 137 CTest cases on macOS arm64 with Qt 6.11.2 on 2026-09-30.

## P0-12.3 — root NoPaths through source FM callback (2026-09-30)

Exact baseline:
[`ip7z/7zip` commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Common/ArchiveExtractCallback.cpp::GetStream` path-mode switch,
`CPP/7zip/UI/Agent/Agent.cpp::CAgentFolder::Extract`, and
`CPP/7zip/UI/FileManager/PanelCopy.cpp::CPanelCopyThread::ProcessVirt`.
These source bodies remain in their existing mirrored paths. The Qt command
adapter now passes the dialog's NoPaths mode into the already connected native
`IArchiveFolder::Extract` route for a listed, unencrypted root archive. No
algorithm was rewritten and no upstream file modified. `ArchiveBackend` still
validates all selected entries, destination ancestry, link/type hazards and
flattened name collisions before the native worker starts. A real ZIP/Unicode
selected-folder test confirms the flattened file, source Ask at write time,
replacement bytes and absence of the P0-12.1 preflight signal. The source
callback retains an explicit archive directory entry as an empty folder in
NoPaths mode; this comes from its own path-mode body and is recorded as a
visible difference from the earlier CLI `e` transport.

Two related paths were investigated and remain P0-12.4. For an encrypted
archive, the file-only `7zz -so` adapter redirects stdout as binary and only
parses stderr for prompts; a real UI source-extract test timed out waiting for
password handling. The current transport has no verified structured prompt.
The source callback can also open or replace the output before password
verification. Encrypted user extraction therefore stays on the existing
preflight/test path until an authenticated stream contract prevents a hang and
wrong-password data loss. For selection inside a bound archive subfolder,
`CAgentFolder::Extract` passes current-folder path parts to the source callback,
which strips that prefix even with FullPaths; a real selected-item test found
the expected full destination conflict was not asked. The established backend
route is retained until the source root/batch owner supplies the correct path
base. `GUI/ExtractGUI.cpp` and `Common/Extract.cpp` remain mandatory source
orchestration, along with nested/encrypted acceptance. No unverified behavior
is marked complete.

Focused `ui_operations`, `archive_selection_runtime`, `ui_archive_states`
and `extraction_options` passed 4/4. The full unsandboxed
`./scripts/check.sh` passed formatting, app build and all 137 CTest cases on
macOS arm64 with Qt 6.11.2 on 2026-09-30. Windows/Linux runtime and rendered
Windows parity remain unverified.

## P0-12.4 — encrypted root and current archive folder (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Agent/Agent.cpp::CAgentFolder::Extract`,
`CPP/7zip/UI/Common/ArchiveExtractCallback.cpp::GetStream`/`CheckExistFile`,
`CPP/7zip/UI/FileManager/PanelCopy.cpp::CPanel::CopyTo` and
`ExtractCallback.cpp::CryptoGetTextPassword`/`AskOverwrite`. Their previously
copied bodies and source control flow stay authoritative. The Qt command adapter
now uses them for an encrypted root archive and for the current bound archive
folder. The source folder deliberately strips its current prefix for FullPaths:
the target is relative to the viewed folder, matching its original path policy.

`QtArchiveFileResource::ExtractItems` is only the external engine service. For
an encrypted archive it requests the source callback password, runs the already
listed archive through `ArchiveBackend::test`, then calls source `GetStream`.
This ordering prevents an invalid password from opening or replacing an
existing target. The same password is supplied to every file-only `7zz -so`
request before the process starts, avoiding the unhandled stdout prompt found
in P0-12.3. This is a narrow process-boundary adaptation, not a replacement
extraction algorithm. The backend still owns process arguments, validation,
path/symlink checks and cancellation. Wrong password returns through source
progress/error UX; the user can retry extraction with a new password. As with
the upstream direct writer, a later failure/cancel can leave already extracted
files; no atomic rollback is promised.

Real UI fixtures cover encrypted 7z and ZIP with Unicode/space names and
occupied output: wrong password preserves old bytes and never reaches overwrite;
the next correct attempt invokes `COverwriteDialog` and writes new bytes. A
bound ZIP subfolder fixture confirms source Ask at its relative destination and
no backend preflight. Native folder navigation, existing root NoPaths and
backend consumers remain separately covered. Local macOS arm64/Qt 6.11.2 checks
are recorded in the P0-12.4 checklist row: focused 5/5, five repeated
`ui_state`/`ui_operations` checks, and an unsandboxed full `./scripts/check.sh`
with 137/137 CTest cases. Windows/Linux runtime and rendered
Windows appearance are unverified.

At the P0-12.4 commit this was still a partial P0-12 port. Nested archive engine resources and
`CPP/7zip/UI/GUI/ExtractGUI.cpp::ExtractGUI`/
`CPP/7zip/UI/Common/Extract.cpp::Extract`/`DecompressArchive` multi-archive
orchestration remained P0-12.5 and were excluded from that commit. Their
integrated port is recorded below. The existing P0-12.1 backend fallback
continues for unsupported routes and non-Ask modes.

## P0-12.5 — source batch extraction and nested archives (2026-09-30)

Exact baseline: [ip7z/7zip commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Common/Extract.cpp/.h::Extract`/`DecompressArchive`,
`CPP/7zip/UI/GUI/ExtractGUI.cpp/.h::ExtractGUI`/`CThreadExtracting`,
`CPP/7zip/UI/Common/OpenArchive.cpp/.h::CArchiveLink::Open2`/`Open3`,
`CPP/7zip/UI/GUI/resource2.h` and existing `ExtractDialog.cpp/.h`.
The complete `Extract.cpp` archive loop and file selection/decompression bodies
are copied into their mirrored paths; `ExtractGUI.cpp` retains its thread,
options, result and progress sequence. Include paths, Qt dialog/message calls,
language resource lookup and the new file-only opener/progress arguments are
adapted. Required `clang-format` changes are mechanical and are not upstream
functional changes. `ExtractDialog::SetInitialOptions` maps upstream forced
options and persisted defaults to the existing Qt combo controls and password.
`resource2.h` supplies source IDs for archive-count text.

The full upstream `CArchiveLink::Open2` codec detection and recursive stream
opener cannot run with the external `7zz` engine. Its mirrored body is a narrow
delegation to `IQtArchiveOpenService`; `Open3` keeps the source `Open2` then
`Open_Finished` ordering. `QtArchiveBatchOpen` lists one file at a time through
`QtArchiveFolderManager`, creates source `CArc` metadata and preserves the
source `Extract` loop, wildcard censor, archive result callback, overwrite
state, cancellation, password and close. `QtArchiveBatchExtract` only converts
confirmed Qt options and archive paths to source arguments. The whole-root
`MainWindow` Ask consumer uses this route after the existing destination guard;
selected/current-folder Ask keeps `CPanel::CopyTo`. Nested resources now use
the same split. Backend signals for each `7zz` child remain hidden from the
outer window until the source modal worker joins. Source callback errors are
not reported as success when the upstream return HRESULT is `S_OK`.

For split `.001` archives the external `7zz -slt` listing reports `Volumes` and
`Total Physical Size`. The narrow adapter records those part paths and sizes in
source `CArchiveLink::VolumePaths`/`VolumesSize` and supplies the combined
logical stream length to `CArc::ReadBasicProps`. `-t7z` is omitted on the
file-only stream request so `7zz` can open the outer Split handler. The source
loop then skips later volumes and maintains total progress. The current adapter
supports numeric `.001` naming and rejects unsupported naming or missing parts;
the full in-process codec/volume opener and broader format handling remain
P3-05 migration debt, not a claimed port. Non-Ask extraction and additional
Extract modes/metadata remain P2-07.

Real macOS fixtures cover two ZIP archives with a shared Unicode/space path
and source overwrite Yes/Cancel, a split multi-volume 7z with one list/open and
exact extracted bytes, nested whole and selected archive Ask with source
overwrite, encrypted 7z/ZIP wrong-password preservation then correct retry,
and forced-versus-saved dialog modes. These are structural and behavioral
checks on macOS arm64/Qt 6.11.2; they do not establish rendered Windows parity
or Windows/Linux runtime behavior. Focused `extract_dialog`,
`source_extract_backend`, `nested_archive` and `ui_operations` passed 4/4;
the required full `./scripts/check.sh` passed formatting, app build and
137/137 CTest cases on 2026-09-30.

## P0-11 — filesystem overwrite decisions (2026-09-30)

Exact source baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`
from [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Reviewed `CPP/7zip/UI/FileManager/OverwriteDialog.cpp/.h/.rc`,
`FSFolderCopy.cpp::CopyFile_Ask`/`CopyFolder`,
`ExtractCallback.cpp::AskOverwrite`/`AskWrite`,
`UpdateCallback100.cpp`, and `CPP/7zip/Common/FilePathAutoRename.cpp/.h`.
The existing mirrored `COverwriteDialog` source `CFileInfo`, `ReduceString`,
`SetFileInfoControl`, `OnInit`, `OnButtonClicked` and original button IDs remain
the sole presentation implementation. Its native icon/control services are Qt
adapters; P0-11 adds directory-specific wording, since a whole-folder collision
now uses this prompt. `FilePathAutoRename.cpp` remains an unchanged, single
vendored implementation. The existing mirrored `CExtractCallbackImp::AskOverwrite`
switch supplies the exact Win32 button-ID to `NOverwriteAnswer` mapping reused
by the filesystem GUI adapter. The per-operation Ask/Skip/Overwrite/Rename
state and rename path follow its `AskWrite` decision branch; the worker's
transaction stages output before publication and keeps displaced entries as
backup until the complete source set succeeds. The backup/rollback, Qt
condition-variable request and worker ownership are local integration code,
not claimed as copied 7-Zip bodies.

`FileTransferOperation` now sends immutable source/destination paths to the
GUI; its worker waits without touching widgets. Cancel wakes the waiter,
closes an active source dialog and rolls back staged/published work. The same
source decisions cover files and whole directories. Whole-directory replacement
is an explicit transaction adaptation; upstream `CopyFolder` can merge a
directory recursively through `AskWrite` for each contained file. This
different merge workflow remains P1-02 along with native MoveFile/CopyTo,
cross-volume moves, and any nested source callback not used by current
stage-copy targets. Archive extraction Ask and `7zz` protocol remain P0-12;
this work does not mark them implemented. Symlinks and mixed file/directory
types remain rejected. A race that occupies an intended new/renamed target
before publication fails instead of replacing that new entry. Failed rollback
must be reported with its surviving backup; that hardening remains under the
filesystem transaction owner.

`file_transfer` covers Yes/No/YesAll/NoAll/AutoRename/Cancel, repeated
conflicts, occupied rename target, Unicode, file/directory conflict, byte
progress and replacement rollback. `ui_operations` covers F5/F6 dialog
integration, AutoRename, No and cancellation. `extract_prompt` and
`ui_parity_overwrite` cover source controls and file/directory text at 700×440
and 560×440; current-build folder JSON was inspected. Focused 4/4 passed on
macOS arm64, Qt 6.11.2. Unsandboxed `./scripts/check.sh` passed formatting,
app build and all 137/137 CTest cases on 2026-09-30.
Windows/Linux runtime and rendered Windows pixel parity are unverified.

## P0-01 — accepted source inventory

P0-01 is accepted as a documentation inventory, not functional port completion.
The [archived module inventory](history/p0-01-source-inventory.md) preserves all
21 original audit records and module tables verbatim: paths, symbols, copied/
adapted/omitted portions, dependencies, owners, decisions, project baselines and checks.
The exact retrievable source baseline is
[ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).

The 2026-09-28 snapshot covers **112 unique .cpp paths: 109 UI plus three
supporting modules**. Thirty mirrored modules (16 in .1 and 14 in .2) were reviewed
against upstream bodies and dependencies; all 82 then-missing UI modules were
inspected and assigned source-port decisions/feature owners in .3. FileManager,
Agent, GUI and Common coverage includes callers, build dependencies and platform
constraints. No missing mandatory branch was mechanically declared N/A. Inactive
comment-only/definition/build files were identified individually. Headers/resources
remain attached to their module records; familiar paths alone do not prove a port.

Historical verification: macOS 2026-09-28, docs-only path/coverage/diff checks;
33/33 vendored source/header/license files byte-matched the pinned baseline.
Inventory commits: .1 `cfd02a5`, .2 `d03de90`, final .3 coverage `415513c`,
.4 reconciliation `36fad59`. No app build, CTest or rendered/runtime verification
was performed for this inventory. Intermediate coverage counts are retained only
in the archive and do not describe today's checkout.

Later P0-02–P3 source records update their own modules and omissions; consult those
accepted results before acting on an archived finding. The inventory establishes
coverage and provenance, not completion of compression/engine, UI workflows or
Windows/Linux runtime. This documentation consolidation changes no parent count
or functional acceptance.

## Активация общего списка и выбор при наведении (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/UI/FileManager/ListViewDialog.cpp::{OnInit,OnNotify}`: перенесена
оставшаяся ветка CFmSettings.Load/SingleClick → ONECLICKACTIVATE/TRACKSELECT,
сохранён безусловный FULLROWSELECT. Активация делегирует существующему OnEnter:
один столбец подтверждает focused path, Alt или два столбца показывают detail.
Qt itemClicked/itemDoubleClicked заменяют LVN_ITEMACTIVATE/NM_DBLCLK и глобальный
флаг версии common controls; Ctrl/Shift/Meta исключены из активации, поскольку
Qt clicked приходит и для жестов выделения. Клавиатурная активация не изменена.

`qtlistview.cpp/.h` — новая узкая presentation-адаптация TRACKSELECT, для которого
Qt не имеет прямого аналога: dwell 400ms, выбор всей строки без открытия,
остановка при leave/press/modifiers/drag, QPersistentModelIndex защищает от
удаления hovered row, повторная проверка позиции — от scroll/reorder.
Win32 default hover time не запрашивается: фиксированный Qt interval является
явным платформенным отличием. Основная панель table/icon view пока не получила
hover/track-select. Функциональные тела списка/clipboard/delete/detail сохранены.

FmSettingsPath — общий Qt/INI path adapter, перенесён из MainWindow без изменения
политики override/AppConfigLocation/toolbar.ini; такого пути нет в registry upstream.
FoldersHistory передаёт explicit SettingsPath, остальные вызывающие используют
общий adapter. Не добавлена повторная реализация config location в dialog.
Проверки list_activation покрывают single/double, modifiers, Alt/properties detail,
hover on/off, leave и удаление row; прямые consumers — list_view_dialog,
folder_history, properties, settings, fm_settings. Layout не изменён.
Это evidence первоначального dialog adapter; основной table/icon hover
подключён позднее в § P0-07.1 выше.

## Настройки файловых списков (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/UI/FileManager/RegistryUtils.cpp/.h::CFmSettings::{Load,Save}`
перенесены в исходные пути `src/`: семь полей, defaults=false, имена ключей,
порядок чтения/записи и сохранение default при некорректном значении сохранены.
QSettings INI group FM заменяет HKEY_CURRENT_USER FM; путь передаётся явно.
ReadOption принимает bool/UInt32 и проверяет !=0 как UINT32ToBool вместо registry DWORD. Остальные функции
RegistryUtils (editor/language/flat view/large pages) не перенесены.
Старые самостоятельные ключи list/* не читаются и не мигрируются.

`SettingsPage.cpp/.h::{OnInit,OnApply,OnButtonClicked}` перенесены частично:
ShowRealFileIcons/ShowGrid/SingleClick, флаги init/changed, чтение при
инициализации и запись только изменённой страницы при подтверждении.
Qt QDialog/check boxes/signals заменяют CPropertyPage, CheckButton и notifications;
OnApply сохраняет snapshot пропущенных полей вместо чтения отсутствующих controls.
Cancel ничего не сохраняет. Отдельный Apply и OptionsDialog с шестью страницами
отсутствуют. Не перенесены controls/ветки ShowDots, FullRow, AlternativeSelection,
ShowSystemMenu, large pages, extraction RAM, help и Lang. Порядок трёх controls
взят из SettingsPage2.rc; самостоятельная settingsdialog.cpp удалена,
header оставлен как alias. MainWindow адаптирует поддерживаемые флаги к Qt.
CFmSettings принадлежит sevenzip_common, CSettingsPage — sevenzip_panel.
Notice/лицензия сохранены; форматирование адаптаций — clang-format.

Проверки: fm_settings (defaults, invalid INI, round trip, cancel/confirm,
сохранение пропущенных flags, отсутствие записи без изменений, геометрия),
settings (применение и один щелчок в четырёх режимах disk/archive).
Геометрия/JSON проверяют структуру Qt; Windows rendering не проверен.
Это исторический partial scope; расширение страницы и текущих consumers
описано в § P0-07.1 выше, AlternativeSelection остаётся P0-07.2.

## Запрос пароля и группировка Compress после сверки диалогов (2026-09-28)

Baseline: официальный [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

`CPP/7zip/UI/FileManager/PasswordDialog.cpp/.h/.rc`: перенесены
`CPasswordDialog::{ReadControls,SetTextSpec,OnInit,OnButtonClicked,OnOK}`
в исходные относительные пути `src/CPP/7zip/UI/FileManager/PasswordDialog.*`.
Сохранён порядок чтения Password/ShowPassword, обновления password char/text,
инициализации checkbox и чтения при OK. Win32 controls, resource attachment,
Create/parent HWND и routing buttonID заменены Qt controls/layouts, OnInit/exec
и отдельным соединением clicked. UString заменён QString на границе формы.
Lang/иконки/Win32 базовые обработчики не перенесены; подписи русские, кнопки
стандартные Qt. Модуль принадлежит sevenzip_panel, не backend.

QInputDialog в `mainwindow_backendevents.cpp` заменён этим диалогом. Прежние
правила cancel/empty request и lifetime пароля сохраняются; пароль/ShowPassword
не сохраняются в INI. Отмена после переключения ShowPassword отбрасывает
весь результат формы, хотя upstream ReadControls обновляет её публичные поля.
Generated reference содержит полный commit и хеши resource/shared include/source;
password contract сопоставляет все пять controls. Это структурная проверка,
не проверка Windows rendering. LGPL notice сохранён; лицензия —
`third_party/7zip/License.txt`.

`CPP/7zip/UI/GUI/CompressDialog.rc::IDD_COMPRESS`: исправлено Qt presentation
расположение поддерживаемых controls — compression слева, encryption справа.
Ресурс размещает password labels над edit; Qt encryption form теперь делает то же.
Дополнительный source picker следует за основной формой, чтобы не вытеснять
archive/options при открытии. Qt scroll area, wrapping длинных compression rows,
отдельное destination field, группировка и отсутствие неподдерживаемых controls
остаются явно ограниченным представлением исходного ресурса. Новые функциональные
алгоритмы Compress не добавлены, provenance уже перенесённых методов не меняется.

Проверки: password_dialog, ui_parity_password, ui_archive_states,
compression_encryption, ui_geometry; последний проверяет оба столбца на двух
размерах и в 7z/ZIP. Полная сверка и оставшийся долг —
[аудит диалогов](7zfm-parity.md#аудит-диалогов-2026-09-28).

## Шифрование и пароль Add to Archive (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/GUI/CompressDialog.cpp/.h/.rc`,
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h` и `GUI/UpdateGUI.cpp::SetOutProperties`.

В `src/CPP/7zip/UI/GUI/CompressDialog.cpp/.h` перенесены
`CCompressDialog::{SetEncryptionMethod,GetEncryptionMethodSpec,FindRegistryFormat}`
и `IsAsciiString`. Qt preferences делегируют поиск тому же исходному helper.
Исходный password-блок `OnOK` выделен в `CheckPassword`, чтобы UI и backend
использовали один алгоритм. Сохранены UString, порядок проверок ZIP ASCII
(0x20–0x7F), AES >99, сравнение повторного пароля только при скрытом пароле,
7z AES-only, ZIP ZipCrypto/AES, восстановление aes-prefix из CFormatOptions,
default-index и RemoveChar('-'). Qt QStringList/index заменяют ComboBox;
FindRegistryFormat использует snapshot CInfo вместо контролов.

`createarchivedialog_encryption.cpp` — Qt presentation/glue для
`OnInit`, `UpdatePasswordControl`, соответствующих частей `FormatChanged` и
`SaveOptionsInMem`: повторное поле и label скрываются при показе, значение
сохраняется; отдельный флажок EncryptHeaders доступен только для 7z.
Исходный порядок password/reenter/show/method/headers взят из CompressDialog.rc.
QFormLayout заменяет Win32 controls. Формирование таблиц/валидация остаются в
исходном GUI-модуле. CFormatOptions.EncryptionMethod и CInfo.ShowPassword/
EncryptHeaders сохраняются через QSettings вместо Registry; пароль не хранится.

Backend повторяет перенесённый password-блок до stage. `UpdateGUI::SetOutProperties`
передаёт em только для непустого spec; адаптер 7zz делает то же через -mem,
поэтому пустой ZIP spec даёт исходный ZipCrypto вместо прежнего принудительного
AES. Header encryption передаётся существующим -mhe. Валидация доступных em и
ZIP/header combinations — интеграционная проверка двух поддерживаемых форматов.
Исходные UI error messages заменены локализованными Qt-сообщениями; возврат error
enum заменяет ShowErrorMessageHwndRes, остальной OnOK остаётся частичным портом.
Нет password initialization из существующего архива, SFX/внешних кодеков и
других частей полного CompressDialog. Форматирование — clang-format.

История длинных путей больше не увеличивает minimum width всей формы:
AdjustToMinimumContentsLengthWithIcon сохраняет редактируемую строку и полный
текст в dropdown. Это необходимая Qt-адаптация resize, обнаруженная геометрией.

## Путь архива, история и архиватор (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Источники: `CPP/7zip/UI/GUI/CompressDialog.cpp/.h` и
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h`; адаптированные владельцы сохраняют пути в `src/`.

`CCompressDialog::{SetArchiveName,SetArchiveName2,GetFinalPath_Smart,
SetArcPathFields,ArcPath_WasChanged}` и `GetExtDotPos` перенесены для 7z/ZIP:
сохранены поиск точки после разделителя, исключение ведущей точки,
удаление последнего расширения при смене формата, Trim, fallback StartDirPrefix,
разбор выбранной истории и ветка `always`. Qt snapshot текста заменяет Win32
ComboBox; QString — тексты этой границы, UString сохранён для исходных
операций расширения; QDir/QFileInfo — адаптеры
NName::GetFullPath/GetFullPathAndSplit. Проверка NUL необходима на границе native API.
Метаданные двух доступных кодеков заменяют CArcInfoEx; оба имеют KeepName=false.
SFX, KeepName=true, OriginalFileName, hash handlers, внешние кодеки и
OnButtonSetArchive пока не перенесены.

`CCompressDialog::OnOK` содержит только исходный блок истории/ArcType: сначала
новый полный путь, затем прежние уникальные пути до 20. Чтение контролов и Save
остаются в Qt-обвязке. Локальный AddUniqueString сохраняет алгоритм одноимённого
helper из ExtractDialog.cpp без зависимости rules/backend от виджета извлечения.
Полный OnOK ещё не перенесён. `NCompression::CInfo::{Save,Load}` получили исходные
ArcPaths/ArcType, defaults и ключи ArcHistory/Archiver; QSettings INI заменяет
Registry, остальные группы сохраняются.

`createarchivedialog.cpp` остаётся presentation/integration glue с неперенесёнными
частями формы. Combo показывает полное имя с расширением; activated заменяет
CBN_SELCHANGE/PostMessage. Backend определяет формат по суффиксу, поэтому Qt
подтверждает только совпадающие с выбранным форматом .7z/.zip, существующего
родителя и незанятую цель; upstream допускает более общий путь.
Форматирование — обязательный clang-format.

## Свойства и двухколоночный список (2026-09-28)

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Пути ниже относительно `CPP/7zip/UI/FileManager/`; адаптированные файлы — в `src/`.

| Исходник и символы | Заимствование и адаптация |
| --- | --- |
| `EditDialog.cpp/.h/.rc::CEditDialog` | Перенесён весь модуль: UString Title/Text и OnInit; QPlainTextEdit/layout заменяют Win32 attachment/OnSize, Qt-локализация — LangSetDlgItems, event filter — ES_WANTRETURN. ShowItemInfo сохраняет обе ветки и Windows-only LF→CRLF при отсутствии CR. |
| `PanelMenu.cpp::{AddListAscii,AddSeparator,AddPropertyPair,CPanel::Properties}` | Перенесены перечисление свойств и сводка single/multiple selection с UStringVector и UInt64 totals. CPanelItem/CPanelProperty заменяют COM metadata снимком доступных полей, Qt — LangString и создание modal. Числа форматируют ConvertSizeToString/ConvertUInt64ToString. |
| `PanelItems.cpp::{GetItem_UInt64Prop,GetItemSize}` | Сохранены parent-index, пустое свойство, ошибка 2723400 и делегирование GetItemSize; snapshot заменяет COM/variant. |
| `ListViewDialog.cpp/.h::{OnInit,CopyToClipboard,ShowItemInfo,DeleteItems,OnEnter}` | Перенесены NumColumns/Values, усечение display copy, raw copy/detail и параллельное удаление Strings/Values. `qtlistview.h` адаптирует CListView к QTreeWidget; полный текст показывает перенесённый CEditDialog. Лимит 1024 wchar_t зависит от ширины wchar_t платформы. |

Не перенесены raw/security/flags/errors, полный ConvertPropertyToString2,
IFolderProperties/IFolderArcProps levels, non-open level, IFolderGetItemName
fast path, остальные команды PanelMenu/PanelItems, single-click/hover и
Win32 notifications. Qt-список заменяет filesystem shell properties.
`third_party/7zip/CPP/7zip/PropID.h` скопирован без изменений из того же baseline;
адаптированные файлы форматируются clang-format.

## История папок и общий список (2026-09-28)

Следующая итерация переносит `AppState.h::CFastFolders::{SetString,GetString,
Save,Read}` и `PanelFolderChange.cpp::CPanel::{SetBookmark,OpenBookmark}`
из того же полного baseline ниже. Сохранены sparse vector, заполнение пустыми
строками до индекса и отсутствие изменения state при GetString вне списка.
Qt adapter передаёт полный отображаемый prefix; OpenBookmark возвращает его
для общего navigateTo вместо Win32 BindToPathAndRefresh. Save/Read используют
`ViewSettings.cpp/.h::{SaveFastFolders,ReadFastFolders}`, ключ FolderShortcuts.
Критическая секция исключена по той же причине GUI-thread ownership.
Двухполевая структура Bookmark и самостоятельное ветвление открытия удалены.
Исчезнувшая внутренняя папка открывает ближайший существующий parent через
общую навигацию, включая уже открытый архив без повторного чтения.
MVP-настройки history/paths и favorites/* больше не читаются по указанию
пользователя; миграционного fallback нет. Остальной CAppState пока не перенесён.

Baseline: [ip7z/7zip, `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Из `CPP/7zip/UI/FileManager/AppState.h` перенесён `CFolderHistory`;
из `App.cpp` — `AddUniqueStringToHead`, `Normalize`, `AddString` с исходным
пределом 100 и сравнением `UString::IsEqualTo_NoCase`. Имена, вектор,
порядок удаления дублей и вставки сохранены. State принадлежит GUI thread,
поэтому Win32 critical section отсутствует; Save/Read принимают путь INI.
`ViewSettings.cpp/.h::{SaveFolderHistory,ReadFolderHistory}` сохраняют исходный
ключ FolderHistory; настройки прежнего Qt MVP игнорируются.

Из `ListViewDialog.cpp/.h/.rc` перенесена одноколоночная ветка:
OnInit, ListView_GetSelected, DeleteItems, CopyToClipboard, ShowItemInfo,
OnEnter, OnOK и клавиши OnNotify. QListWidget/event filter/layout/clipboard
заменяют Win32 controls/OnSize/notifications. Сохранены множественный выбор,
удаление первого выбранного до исчерпания выбора, флаг изменений, индекс
фокуса и платформенный перевод строки clipboard. Meta добавлен для macOS.
ShowItemInfo использует перенесённый CEditDialog.
NumColumns/Values и усечение значений перенесены следующей
итерацией выше; Settings single-click/hover пока пропущен.
Qt выбирает surviving current row после удаления; Win32 focus восстановить
буквально нельзя. Отдельная кнопка удаления MVP убрана по исходному rc.

`PanelFolderChange.cpp::CPanel::FoldersHistory` сохраняет ветки OK/Cancel,
обратную вставку отредактированного списка и выбор focused item. State/parent
передаются аргументами вместо _appState/GetParent, путь возвращается Qt adapter
вместо BindToPathAndRefresh. Остальные функции модуля пока не перенесены.
MainWindow хранит history отдельно от combo; combo — только presentation.

Неизменённые зависимости взяты из того же baseline в `third_party/7zip/`:
`CPP/Common/{MyString.h,MyString.cpp,IntToString.h,IntToString.cpp,
StringConvert.h,StringConvert.cpp,UTFConvert.h,UTFConvert.cpp,MyBuffer.h,
MyWindows.cpp}` и `C/CpuArch.h`. Они сохраняют upstream notices; остальные
зависимости MyVector/MyTypes/Common/7zTypes уже присутствовали.
MyWindows обеспечивает BSTR на POSIX, Windows использует oleaut32.
Преобразование QString↔UString ограничено Qt/settings boundary и учитывает
разную ширину wchar_t. Форматирование адаптированных файлов — clang-format,
vendored sources не переформатированы.

Воспроизводимый baseline новых итераций FSFolder/FileDir, Split/Combine и
Combo/Browse: официальный репозиторий
[`ip7z/7zip` на commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные пути, перенесённые symbols, пропущенные части и адаптации указаны
в разделах ниже. Для обновления каждого частичного модуля сравнивать этот
snapshot с новой ревизией, включая зависимые/пропущенные ветки; форматирование
рассматривать отдельно от functional diff. Соседний checkout остаётся read-only.

## Инструмент сверки UI (2026-09-28)

Инструмент UI-сверки описан в [docs/ui-reference.md](ui-reference.md).
Сохранённый Copy baseline извлечён из той же полной ревизии официального
`ip7z/7zip`: `CopyDialog.rc`, `CopyDialogRes.h`, `CPP/7zip/GuiCommon.rc` и
`CopyDialog.cpp::{OnInit, OnSize, OnButtonClicked, OnButtonSetPath, OnOK}`.
JSON сохраняет ресурсные данные и короткие навигационные выдержки, не
функциональную реализацию. Python-парсер/сверка — новый локальный tooling;
новых заимствованных алгоритмов или изменений Qt presentation в этой задаче нет.
Контракт покрывает шесть контролов и выбранные Copy/Move состояния; полный
source-port audit существующих модулей остаётся в соответствующих разделах.

## Общий ввод имени/маски и создание папки (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ComboDialog.cpp/.h/.rc::{CComboDialog, OnInit, OnOK,
OnSize}` и `BrowseDialog.cpp/.h::Dlg_CreateFolder`.
Адаптированные реализации сохраняют исходные пути и symbols. Прежний
самостоятельный combodialog.cpp удалён; combodialog.h — compatibility alias.

Перенесены Title/Static/Value/Strings, порядок заполнения и получение сырого
Value в OnOK. Qt-конструктор задаёт поля до OnInit; QDialog, QComboBox,
QLabel/layouts заменяют CModalDialog/Win32 controls/OnSize. Qt AddItem выбирает
первую запись, тогда как исходный AddString не меняет введённый Value:
после наполнения восстанавливается исходный edit text. accept проходит через
OnOK; getters — Qt compatibility glue. QStringList используется только на
presentation boundary; никакого нового functional parser не добавлено.

Dlg_CreateFolder скопирован с очисткой destName перед dialog, исходными
Title/Static/default Value, возвратом false при Cancel и Value при OK.
QWidget заменяет HWND, русские строки — LangString. CreateFolder теперь
использует эту функцию, как PanelOperations.cpp; создание файла и SelectSpec
переходят на исходный CComboDialog через alias. Остальная BrowseDialog,
CorrectFsPath, inline Rename и complex-name validation остаются долгом.
Notices/LGPL сохранены; clang-format обязателен. sevenzip_panel компилирует
общий dialog один раз и включает AUTOMOC для Qt meta-object.

## Split dialog и подготовка Combine (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/SplitDialog.cpp/.h/.rc`,
`PanelSplitFile.cpp::{CApp::Split, CApp::Combine, AddInfoFileName}` и
`App.cpp::AddValuePair2`. `CSplitDialog` сохранён в исходном относительном пути;
самостоятельные volumedialog.cpp/.h удалены.

Перенесены OnInit (имя в title, Path, AddVolumeItems, выбор первого размера),
OnOK (Path, Trim, ParseVolumeSizes, отказ пустого/ошибочного ряда),
OnButtonSetPath (browse, NormalizeDirPathPrefix, сброс индекса). VolumeSizes
использует исходный CRecordVector<UInt64>; временный std::vector нужен для
текущего адаптированного SplitUtils parser, Qt-worker принимает QList<qint64>.
Предел qint64 сохраняет допустимые смещения QFile. QComboBox/QDialog/layouts
заменяют CModalDialog/Win32 controls/OnSize; native QFileDialog заменяет
MyBrowseForFolder. Inline label заменяет modal error, accept вызывает OnOK.
Qt-конструктор передаёт публичные FilePath/Path перед OnInit.

Блок сводки CApp::Combine скопирован в CThreadCombine::GetInfo в том же
PanelSplitFile.cpp: сохранены AddInfoFileName, AddValuePair2, первые два имени,
условие многоточия и последний файл. Извлечение метода необходимо для
разделения Qt orchestration и исходной логики при отсутствии полной CApp.
AddValuePair2 экспортирован из существующего App.cpp, как в upstream.
CCopyDialog получил конструктор для Title/Static/Value/Info: Combine теперь
использует исходный общий диалог вместо отдельного Qt dialog.

mainwindow_volumes.cpp переносит подготовительные ветки CApp::Split:
повторную проверку источника после dialog, сравнение размера первого тома,
GetNumberOfVolumes и подтверждение >=100. Оба сценария нормализуют назначение
и вызывают CreateComplexDir (QDir::mkpath) после подтверждения. Сохранены
адаптерные ограничения на выбранные symlink, занятые имена, 10000 частей,
транзакционную публикацию и откат. Проверка пропусков и пустых частей остаётся
существующей integration adaptation; discovery loop не объявляется новым
source port. Полный CApp, взаимодействие двух панелей и Win32 progress — долг.
Форматирование — clang-format; notices/LGPL сохранены.

## Мутации файловой системы и рабочий поток удаления (2026-09-28)

Источник: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/FSFolder.cpp::{GetRelPath, GetAbsPath, CreateFolder,
CreateFile, Rename, Delete}`, `FSFolder.h::{CDirItem, CFSFolder}`,
`CPP/Windows/FileDir.cpp::{DeleteFileAlways, RemoveDirWithSubItems}`.
Адаптированные файлы сохранены по исходным путям; notices и LGPL сохранены.

Перенесены ветки mkdir → CreateComplexDir, Create_NEW, Rename с родительским
префиксом, индексный цикл Delete с SetTotal/SetCompleted и рекурсивный обход
RemoveDirWithSubItems. Сохранён исходный порядок файлов, накопление isError
в рекурсивном обходе и частичный результат удаления. Files/Folders и indices
используют исходные CObjectVector/CRecordVector. Остальные методы CFSFolder
не объявляются перенесёнными.

Необходимые изменения: QFile/QDir/QFileInfo заменяют NIO/Win32 filesystem
services; QString — платформенные пути, bool/ErrorMessage — HRESULT и last
error, callbacks — IProgress. AddPath и конструктор формируют снимок панели
Qt, включая Parent для плоского вида; это новая интеграционная обвязка.
Win32 FILE_ATTRIBUTE_READONLY заменён Qt permissions только на Windows:
на Unix удаление зависит от прав родительской папки. QFile удаляет сам symlink;
ветка HasReparsePoint сохраняет запрет обхода его цели. Qt enumeration не
имеет Next(error): проверяется читаемость папки, а rmdir выявляет оставшиеся
записи. Сохраняется существующая политика отказа для непосредственно
выбранного symlink и незамещения dangling symlink при Rename.

FileDeleteOperation и mainwindow_filedelete.cpp — Qt worker/presentation glue:
исходный прогресс i преобразуется в число i+1 для QProgressDialog, отмена
проверяется до удаления каждого объекта и в рекурсии, завершение всегда
обновляет панель, деструктор ожидает поток. Дополнительный CheckBreak нужен,
потому что исходный RemoveDirWithSubItems не принимает IProgress и иначе
большое дерево не реагировало бы на отмену. UI сохраняет прежнее ограничение
создания одним именем; CFSFolder поддерживает исходную ветку составного пути.
Корзина/SHFileOperation, alternate streams и остальные FSFolder методы — долг.
Форматирование — обязательный clang-format.

## Qt layout diagnostics

Reference inspected: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/ExtractDialog.rc` (`IDD_EXTRACT`) and
`CPP/7zip/UI/GUI/CompressDialog.rc` (`IDD_COMPRESS`).
`docs/ui-layout-contracts.md` records control grouping/order and current Qt
adaptations. `tests/layout_inspection.cpp` is new test-only Qt integration glue:
runtime QObject-tree inspection, ancestor-clipped QWidget geometry, and JSON
reports are specific to Qt, so no upstream implementation is copied for this
helper. It does not replace upstream dialog functional logic or establish
pixel-level parity. `ui_geometry` checks reachability, footer geometry, and
Extract column ordering on disposable fixtures.

Источник сравнения: `../7zip-upstream`, commit `0766b73`, прежде всего
`CPP/7zip/UI/FileManager/`. Эта таблица охватывает все производственные файлы
`src/` на 28 сентября 2026 года. «Собственный» означает, что реализация написана
в этом репозитории по поведению 7zFM, а не перенесена из исходного файла.

Цель и обязательные правила переноса заданы в `AGENTS.md`: исходная реализация
копируется с сохранением имени файла, относительного пути, символов и структуры
модуля. UI-представление адаптируется к Qt; остальная логика меняется только там,
где этого требуют платформенные или интеграционные зависимости. Это относится
и к функциональному коду внутри Win32/COM UI-модулей. Собственная реализация
с совпадающим поведением остаётся долгом переноса.

Адаптированные исходники приложения размещаются в `src/<путь-upstream>`,
неизменённые общие зависимости — в `third_party/7zip/<путь-upstream>`.
Для каждого переноса фиксируются commit, исходные пути и символы,
скопированные и адаптированные части, а также причины каждого существенного
отклонения. Одних совпадающих имён недостаточно для признания прямого переноса.
Сохраняются исходные уведомления и применимая лицензия; текст лицензии для уже
перенесённых фрагментов лежит в `third_party/7zip/License.txt` из того же checkout.
Архивные операции сейчас остаются за `ArchiveBackend` и официальным `7zz`.

| Наши файлы | Соответствующий исходник upstream | Текущее состояние |
| --- | --- | --- |
| `src/CPP/7zip/UI/FileManager/PanelSort.cpp`, `.h` | `CPP/7zip/UI/FileManager/PanelSort.cpp::CompareFileNames_ForFolderList`, `CompareItems` | Сравнение имён и порядок «папки перед файлами → свойство → базовое имя → префикс → индекс загрузки» перенесены с сохранением имён; Указательный компаратор скопирован напрямую с заменой Windows `wchar_t` на `char16_t`; перегрузка `QStringView` преобразует вход на границе. Qt item data заменяет свойства `IFolder`. Направление, начальный порядок свойств и расширение по последней точке адаптированы по `PanelSort.cpp`, `FSFolder.cpp` и `Agent.cpp` того же commit. Остальная сортировка по сырым и форматным свойствам пока не перенесена. |
| `src/CPP/Common/Wildcard.cpp`, `.h` | `CPP/Common/Wildcard.cpp::DoesWildcardMatchName`, `EnhancedMaskTest` | Указательный `EnhancedMaskTest` скопирован с заменой Windows `wchar_t` на `char16_t`; перегрузка `QStringView` преобразует вход, Qt выполняет `MyCharUpper`. Для выбора в панели применяется нечувствительное к регистру сравнение 7zFM. |
| `src/CPP/7zip/UI/FileManager/PanelSelect.cpp`, `.h`, `src/mainwindow_selection.cpp` | `CPP/7zip/UI/FileManager/PanelSelect.cpp::SelectSpec`, `SelectByType`, `SelectAll`, `InvertSelection` | Функциональная часть `CPanel` выделена в `CPanelSelection`: методы, порядок ветвей и вектор выбора сохранены. Компонент не зависит от виджетов; `mainwindow_selection.cpp` адаптирует `ComboDialog`, фокус и Qt selection model, публикуя выбор одним обновлением. Альтернативный режим выбора upstream и обработчики мыши/диапазона остаются за Qt и не перенесены. |
| `third_party/7zip/CPP/Common/StringToInt.cpp`, `.h` и их заголовки `Common*.h`, `My*.h`, `NewHandler.h`, `StdAfx.h`, `C/7zTypes.h`, `7zWindows.h`, `Compiler.h`, `Precomp.h` | Те же относительные пути upstream | Побайтовые копии без изменения алгоритмов и форматирования. Собирается `sevenzip_common`; Qt-код использует оригинальный `ConvertStringToUInt64`. |
| `src/CPP/7zip/UI/FileManager/SplitUtils.cpp`, `.h` | `CPP/7zip/UI/FileManager/SplitUtils.cpp` | Перенесены `ParseVolumeSizes`, `GetNumberOfVolumes`, список `k_Sizes` и `AddVolumeItems`. Алгоритмы сохранены; `UString` / `CRecordVector` заменены стандартными контейнерами, Win32 combo — `QComboBox`. Worker применяет последовательность размеров и повторяет последний; диапазон ограничен `qint64`. |
| `src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp`, `.h` | `CPP/7zip/UI/FileManager/PanelSplitFile.cpp::CVolSeqName`, `CThreadCombine::ProcessVirt`, `CThreadSplit::ProcessVirt` | Алгоритм нумерации и циклы Combine/Split перенесены с сохранением имён; `QString` / `QFile` / callback заменяют строки, NIO и progress sync. Проверки изменений входа и flush — платформенная адаптация. |
| `src/CPP/7zip/UI/GUI/CompressDialog.cpp`, `.h` | `CPP/7zip/UI/GUI/CompressDialog.cpp::CFormatInfo`, `g_Formats`, `g_*Methods`, `GetStaticFormatIndex`, `SetMethod2`, `SetDictionary2`, `SetOrder2`, `SetSolidBlockSize2`, `SetNumThreads2`, `GetMemoryUsage_Threads_Dict_DecompMem`, `OnOK` | Таблицы форматов, методов, уровней и флагов взяты из upstream. Qt-преобразование возвращает имена методов с исходным фильтром 7z. Один источник используется UI и backend; SFX и внешние кодеки не представлены в текущем UI. |
| `src/CPP/7zip/UI/Agent/AgentProxy.cpp`, `.h`, `Agent.cpp`, `Agent.h` | `CPP/7zip/UI/Agent/AgentProxy.cpp::CProxyArc`, `CProxyDir`, `Agent.cpp::CAgentFolder::LoadFolder`, `LoadItems`, `GetNumberOfItems` | Перенесены бинарный поиск и отложенное слияние подпапок, дерево явных/неявных папок, итеративный подсчёт размеров, раскрытие выбора в реальные индексы и плоский обход. Qt-строки и стандартные контейнеры заменяют upstream-контейнеры; данные `ArchiveBackend` — COM-свойства. CRC, сырые свойства, `CProxyArc2` и alternate streams не представлены процессным листингом. |
| `src/main.cpp`, `mainwindow.cpp`, `mainwindow.h`, `mainwindow_menus.cpp`, `mainwindow_panel.cpp` | `FM.cpp`, `App.cpp`, `App.h`, `Panel.cpp`, `Panel.h`, `PanelItems.cpp`, `resource.rc` | Qt-оболочка одной панели. Конструктор, сборка меню и сборка панели разделены; перенос правил меню и компонентов панели ещё не завершён. |
| `src/mainwindow_backendevents.cpp` | `PanelCopy.cpp`, `PanelCrc.cpp`, `PanelItemOpen.cpp`, `OpenCallback.cpp`, `ExtractCallback.cpp` | Адаптер асинхронных сигналов `ArchiveBackend`: переходы между этапами хеширования/перемещения, просмотр извлечённого файла и восстановление состояния. Он необходим для процессного backend; функциональные сценарии ещё требуют сверки с исходными callback и panel-модулями. |
| `src/mainwindow_filelist.cpp` | `PanelFolderChange.cpp`, `PanelItems.cpp`, `PanelOperations.cpp` | Собственные переходы по файловой системе, наполнение строк и создание. Нужен перенос по исходным функциональным модулям. Выбор по маске и типу вынесен в `PanelSelect.cpp`. |
| `src/mainwindow_archivelist.cpp` | `PanelItems.cpp`, `PanelSort.cpp`, `PanelMenu.cpp` | Построение дерева, раскрытие выбранных папок и плоский обход делегированы перенесённым `CProxyArc` / `CAgentFolder`. Строки таблицы адаптируют их данные к Qt. Собственные правила доступности команд ещё требуют переноса. |
| `src/mainwindow_archiveopening.cpp` | `PanelFolderChange.cpp`, `PanelItemOpen.cpp` | Собственная цепочка вложенных архивов с временными файлами и backend. Нужно сверить стек папок и открытие по содержимому. |
| `src/mainwindow_archivecommands.cpp` | `PanelCrc.cpp`, `PanelOperations.cpp`, `PanelCopy.cpp`, `PanelItemOpen.cpp` | Собственная связка действий с `ArchiveBackend`; выбор и правила команд требуют сверки. |
| `src/mainwindow_fileoperations.cpp`, `mainwindow_filedelete.cpp` | `PanelOperations.cpp`, `PanelCopy.cpp`, `FSFolder.cpp` | Qt-обвязка подтверждения, рабочего потока и команд. Создание, переименование и удаление делегированы перенесённым методам CFSFolder; остальные правила панели остаются долгом. |
| `src/CPP/7zip/UI/FileManager/PanelKey.cpp` | `PanelKey.cpp::OnKeyDown`, `Panel.cpp::CMyComboBoxEdit::OnMessage`, `App.cpp::CPanelCallbackImp::SetFocusToPath` | Qt-диспетчер клавиш списка и адреса: Shift+F4, Alt+F1/F2, Tab, закладки и делегирование команд выделения. Однопанельный режим сохраняет upstream-правило общей цели Alt+F1/F2. |
| `src/mainwindow_preferences.cpp` | `PanelMenu.cpp`, `PanelSort.cpp`, `PanelFolderChange.cpp`, `App.cpp`, `PanelKey.cpp` | Собственный код меню, сортировки, истории, закладок и событий. Это несколько upstream-модулей в одном файле. |
| `src/CPP/7zip/UI/FileManager/PanelMenu.cpp`, `PanelItems.cpp`, `src/mainwindow_dialogs.cpp`, `propertiesdialog.cpp`, `propertiesdialog.h` | `PanelMenu.cpp::Properties`, `ListViewDialog.cpp`, `PanelItems.cpp::GetItemSize/GetItem_UInt64Prop`, `UI/Agent/AgentProxy.cpp::CalculateSizes` | Перенесены цикл перечисления доступных snapshot properties, сводка выбранных объектов и двухколоночный ListViewDialog. Qt-обвязка только собирает snapshot; размеры папок — CProxyArc. Форматные/сырые свойства и остальные PanelMenu/PanelItems branches отсутствуют. |
| `src/mainwindow_volumes.cpp`, `volumeoperations.cpp/.h`, `src/CPP/7zip/UI/FileManager/SplitDialog.cpp/.h` | `PanelSplitFile.cpp`, `SplitDialog.cpp/.h/.rc`, `CopyDialog.cpp` | CSplitDialog OnInit/OnOK/browse перенесены, Combine использует общий CCopyDialog и исходную сводку. Нумерация, parser, число томов и worker-циклы перенесены ранее; транзакции, проверка путей и discovery остаются Qt-адаптацией. |
| `src/CPP/7zip/UI/FileManager/FSFolderCopy.cpp`, `FSFolder.h` | `CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CCopyStateIO::MyCopyFile`, `CCopyState`, `CopyFolder`, `CopyFile_Ask`, `IsDestChild` | Цикл чтения/записи, накопленный прогресс, рекурсивная copy-ветка и проверка вложенного назначения перенесены с сохранением имён. `QFile`, `QDir` и callbacks заменяют платформенные операции; stage создаётся с `NewOnly`, ошибки записи проверяются `flush`, сохраняется дата. Транзакционная обвязка, move и интерактивный overwrite остаются адаптацией/долгом. |
| `src/filetable.cpp`, `filetable.h` | `PanelItems.cpp`, `PanelSort.cpp`, `PanelDrag.cpp` | Qt-адаптер таблицы и drag-out. Функция сравнения вынесена в зеркальный `PanelSort.cpp`. |
| `src/filesystemscanner.cpp`, `filesystemscanner.h` | `FSFolder.cpp::LoadSubItems`, `Panel.cpp::ChangeFlatMode` | Собственный порционный обход через Qt. Нужна сверка фильтров, ссылок и отмены. |
| `src/filetransferoperation.cpp`, `filetransferoperation.h` | `FSFolderCopy.cpp`, `PanelCopy.cpp` | Qt-worker с временными файлами и откатом. Копирование дерева/байтов и проверка вложенного назначения делегированы перенесённым `CopyFolder`, `CopyFile_Ask`, `CCopyStateIO` / `IsDestChild`. Перемещение, предварительная проверка дерева для rename и транзакционное завершение остаются адаптацией. |
| `src/fileviewerdialog.cpp`, `fileviewerdialog.h` | `PanelItemOpen.cpp`, `EditDialog.cpp`, `EditPage.cpp` | Собственный Qt-редактор и интеграция с backend; в upstream есть отдельные режимы редактора и внешнего открытия. |
| `src/createarchivedialog.cpp`, `createarchivedialog_compression.cpp`, `createarchivedialog_resources.cpp`, `createarchivedialog.h` | `CPP/7zip/UI/GUI/CompressDialog.cpp`, `CompressDialog.rc`, `FileManager/PanelCopy.cpp` | Qt-форма выбора источников и сжатия. Правила уровней (`SetLevel2`) и методы берутся из перенесённого `GUI/CompressDialog.cpp`; Ветки размера слова LZMA/LZMA2, Deflate/Deflate64 и порядка PPMd/PPMdZip перенесены в `SetOrder2`; Словари доступных методов, Auto/Non/Full solid, потоки и оценки памяти перенесены; путь/история, шифрование, пользовательский лимит памяти и профили перенесены последующими итерациями выше; выбор источников и остальные поля полного OnOK остаются долгом. |
| `src/CPP/7zip/UI/GUI/ExtractDialog.cpp`, `.h`, `src/CPP/7zip/UI/Common/ZipRegistry.*` | `CPP/7zip/UI/GUI/ExtractDialog.cpp::CExtractDialog::OnInit`, `OnOK`, `AddComboItems`, `AddUniqueString`; `ZipRegistry.*::NExtract::CInfo` | Исходные таблицы режимов, история и сохранение режимов/показа пароля перенесены. Qt заменяет presentation; `src/extractdialog.h` — compatibility alias, прежняя самостоятельная реализация удалена. SplitDest и SplitPathToParts_Smart перенесены следующей итерацией; ElimDup и NtSecurity остаются долгом. |
| `src/CPP/7zip/UI/FileManager/CopyDialog.*`, `ViewSettings.*`, `App.cpp`, `Panel.h`, `PanelListNotify.cpp`, `src/copydialog.h` | `FileManager/CopyDialog.cpp`, `.h`, `.rc`; `ViewSettings.cpp::ReadCopyHistory`, `SaveCopyHistory`, `AddUniqueStringToHeadOfList`; `App.cpp::Copy`, `CPanel::GetItemsInfoString`; `PanelListNotify.cpp::ConvertSizeToString` | Перенесены CCopyDialog, история, сводка выделения и форматирование чисел. Qt layouts заменяют OnSize, QSettings — Registry, snapshot свойств — COM; старый header содержит только alias. Остальные App/Panel/ListNotify функции пока не перенесены. |
| `src/CPP/7zip/UI/FileManager/ComboDialog.cpp/.h`, `BrowseDialog.cpp/.h`, `src/combodialog.h` | `FileManager/ComboDialog.cpp/.h/.rc`, `BrowseDialog.cpp::Dlg_CreateFolder` | Перенесены OnInit/OnOK и создание папки через общий dialog; Qt заменяет presentation, header alias сохраняет существующих потребителей. Остальная BrowseDialog не перенесена. |
| `src/CPP/7zip/UI/FileManager/SettingsPage.*`, `RegistryUtils.*`, `settingsdialog.h` | `FileManager/SettingsPage.cpp/.h`, `RegistryUtils.cpp/.h`, `SettingsPage2.rc` | Частичный source port страницы и CFmSettings, подробно выше; OptionsDialog и остальные controls остаются долгом. |
| `src/archiveformats.h` | `FileManager/FilePlugins.cpp`, `PanelFolderChange.cpp`, `PanelItemOpen.cpp` | Собственный статический список расширений для эвристики двойного щелчка и фильтра диалога. В upstream расширения приходят от установленных обработчиков; движок сам определяет формат при явном открытии. |
| `src/archivebackend.cpp`, `archivebackend.h`, `archivebackend_listing.cpp`, `archivebackend_mutations.cpp`, `archivebackend_validation.cpp` | `CPP/7zip/UI/Console/Main.cpp`, `CPP/7zip/UI/Common/OpenArchive.cpp`, `Update.cpp` и движок `CPP/7zip/Archive/` | Адаптер процесса `7zz`, парсер его вывода, проверка путей и транзакционное обновление. Не является копией движка; переносить код форматов сюда нельзя. |

Вывод ревизии: строки со статусом «собственный» обозначают долг переноса, а не
принятую конечную архитектуру. Переименование файлов само по себе его не
исправит. Каждый такой блок требует сверки исходных функций, зависимостей и
тестов, переноса переносимой логики и Qt-адаптации платформенных вызовов.

Сравнение имён `PanelSort.cpp::CompareFileNames_ForFolderList` теперь использует
указательный алгоритм напрямую из upstream `0766b73`. Единственная замена
в теле алгоритма — `wchar_t` на `char16_t`, чтобы сохранить UTF-16 Windows
на macOS/Linux. `MyCharUpper` использует Qt; отдельная перегрузка преобразует
`QStringView` в строки с завершающим нулём, включая срезы без терминатора.

Цикл объединения `CThreadCombine::ProcessVirt()` перенесён из upstream
`PanelSplitFile.cpp` (`0766b73`) в тот же относительный путь. Сохранены порядок
частей, чтение до EOF блоками 1 МиБ, проверка полной записи и прогресс после
каждого блока. `QFile` и callback заменяют NIO/HRESULT/CProgressSync. Адаптация
добавляет проверку изменения размера, `flush` и отмену до первого чтения.
`VolumeOperation` владеет проверкой путей, временным именем, публикацией
и откатом; Split делегирует чтение/запись перенесённому `CThreadSplit::ProcessVirt`.

Правила первой части и имени результата Combine используют исходный
`CVolSeqName::ParseName` и перенесённый блок `CApp::Combine` в
`CVolSeqName::GetCombinedName`. Снято собственное ограничение `.001`;
поддерживаются `part01`, `part.01`, `.001` и числовые имена.

Цикл `CThreadSplit::ProcessVirt()` перенесён из `PanelSplitFile.cpp` (`0766b73`)
в тот же модуль: блоки 1 МиБ, ограничение чтения остатком тома, повтор последнего
размера, переход к следующему файлу и прогресс через 4 МиБ либо на границе тома.
Qt-адаптер передаёт список временных путей вместо `VolBasePath`, проверяет размер
источника и сообщает ошибки `flush`. Предварительное выделение длины из
`CPreAllocOutFile` не переносится: Qt пишет временный файл последовательно.
`VolumeOperation::copyBytes` удалён; транзакционная публикация и откат остаются
в `VolumeOperation`.

Ветка LZMA/LZMA2 `CCompressDialog::SetOrder2()` перенесена из upstream
`0766b73` в `src/CPP/7zip/UI/GUI/CompressDialog.cpp`. Сохранены генерация
8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 273 и автоматическое значение
32 для уровней ниже 7, иначе 64. UI и backend используют одну функцию.
Qt-часть формы сжатия выделена в `createarchivedialog_compression.cpp`;
явный выбор сохраняется при смене уровня, автоматический не передаёт `fb`
движку. Ветки порядка Deflate/PPMd подключены последующими итерациями, описанными ниже.

Числовой ряд `CCompressDialog::SetSolidBlockSize2` перенесён из `0766b73`
в исходный `GUI/CompressDialog.cpp`: степени 2^20–2^36. Qt хранит размеры
в МиБ вместо логарифмов; UI и backend используют один источник. Сохраняется
явное значение при смене уровня, для ZIP/Store блок отключён. Подписи
показывают МиБ/ГиБ. Расчёт автоматического размера и отдельный Full Solid
из upstream пока не перенесены; Auto оставляет выбор движку.

Скопированы проверки `fi.Solid_()`/нулевого уровня и числовой цикл `i = 20..36`.
Вместо вставки значений в Win32 `m_Solid` функция возвращает данные для Qt combo
и проверки `ArchiveBackend`; преобразование логарифмов в МиБ требуется текущим
полем `CompressionOptions::solidBlockMiB`. Registry, Add_Size, расчёт Auto,
Non Solid и Full Solid из этой функции не включены в данный перенос.
Форматирование изменено только по обязательному clang-format gate.

### Итерация: порядок Deflate / Deflate64

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetOrder2`.
Скопированы обе ветки Deflate: условия Auto 32/64/128, цикл и замена
последнего значения на 258/257. Сохранены имена метода, порядок вычислений
и границы; clang-format меняет только оформление. Win32 AddOrder и выбор
Registry заменены возвратом данных и текущим выбором Qt, как в ранее
перенесённой ветке LZMA. `GetMethodID` — узкий адаптер между строковым
`CompressionOptions` и исходными ID из таблицы форматов; upstream получает
ID из item data Win32 combo. Backend использует тот же ряд для проверки и
передаёт существующий параметр `fb` движку. Ветки порядка PPMd перенесены следующей итерацией.

### Итерация: порядок PPMd / PPMdZip

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetOrder2`,
`GetOrderMode`, `CPP/7zip/UI/GUI/UpdateGUI.cpp::SetOutProperties`.
Скопированы ветки PPMd (ряд 2,3,4,5,6,7,8,10,12,14,16,20,24,28,32 и Auto
4/6/16/32) и PPMdZip (2–16, Auto level+3), включая исходные циклы,
условия и формулы. Как в остальных ветках, AddOrder/Registry заменены
выдачей данных и текущим выбором Qt. `GetOrderMode` скопирован с передачей
ID вместо чтения Win32 combo; добавлен пустой default для clang -Wswitch.
Backend адаптирует исходный выбор `di.OrderMode ? "o" : "fb"` из
SetOutProperties к существующей строке аргумента QProcess. COM property
vector не используется при процессной границе с 7zz; функция целиком пока
не перенесена. Размер памяти PPMd и его dictionary branch остаются долгом:
UI отключает словарь, движок использует свой Auto. Подпись «Размер слова»
сохранена как в ресурсе upstream. Только clang-format меняет оформление.

### Итерация: словарь LZMA / LZMA2

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2`,
`Combo_AddDict2`. Скопированы формула Auto, цикл i=30..62, пропуски малых
значений, ограничение kLzmaMaxDictSize (15<<28) и архитектурный предел
kLzmaMaxDictSize_Up. Сохранены типы UInt32/size_t, условия и порядок
вычислений. Qt получает список вместо Win32 combo; Registry и выбор
ближайшего сохранённого значения заменены сохранением доступного текущего
выбора (как в остальных опциях). Пересчёт сохранённого Registry dictionary
>=3.75 GiB не нужен: отсутствуют Registry и произвольные размеры в Qt combo.
Перенесены проверки кратности и выбор единиц из Combo_AddDict2; форматирование
строки выполняет QString, подписи локализованы в КиБ/МиБ. UI и backend
используют один исходный ряд. CompressionOptions хранит байты (quint64)
вместо МиБ, чтобы не терять 64/256 КиБ; адаптер 7zz добавляет суффикс b.
Auto=0 не передаёт словарь движку. Другие dictionary branches, RAM limits,
расчёт памяти и Registry остаются долгом. clang-format меняет оформление.

### Итерация: память PPMd в 7z

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kPPMd)
и `CPP/7zip/UI/GUI/UpdateGUI.cpp::SetOutProperties`.
Скопированы Auto=1<<(level+19), константы kPpmd_Default_4g /
kPpmd_MaxDictSize_Up, цикл i=38..62, пропуск 1.5 МиБ, ограничение размера
и break по архитектуре. Формулы, типы и порядок вычислений сохранены;
Win32 AddDict2/Registry заменены выдачей значений и текущим выбором Qt.
Ряд не достигает границы 4g на поддерживаемых 32/64-bit, поэтому размер
для подписи совпадает с реальным размером. Backend переносит выбор
OrderMode ? "mem" : "d" в существующий QProcess аргумент; COM property
vector остаётся вне процессной границы. Qt включает словарь по наличию
исходных вариантов; backend проверяет тот же список без собственного
ограничения на LZMA. RAM limits, Registry и PPMdZip пока не перенесены.
Обязательный clang-format меняет только оформление исходных циклов.

### Итерация: память PPMdZip

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kPPMdZip).
Скопированы Auto=1<<(level+19) и цикл i=20..28 с UInt32 dict=1<<i.
AddDict/Registry заменены возвратом данных и Qt selection; исходные
формулы, границы и типы сохранены. Backend уже использует GetOrderMode
и mem=<bytes>b; одинаковая подпись PPMd разрешается в kPPMdZip по таблице
ZIP. RAM limits и выбор Registry остаются долгом. Форматирование — clang-format.

### Итерация: словарь BZip2

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetDictionary2` (kBZip2).
Скопированы ветки уровня >=5 / >=3 / ниже и значения 900/500/100<<10,
цикл i=1..9 и UInt32 dict=(i*100)<<10. Условия, типы и формулы сохранены;
Win32 AddDict и сохранённый Registry default заменены Qt combo данными
и сохранением доступного текущего значения. Backend использует исходный
список и существующий d=<bytes>b. Исходные значения в KiB сохранены,
хотя BZip2 движок нормализует размер до блоков по 100000 байт.
Это не собственный набор размеров. Форматирование — clang-format.

В этой же итерации завершены dictionary branches доступных методов:
скопирована ветка kDeflate/kDeflate64 с фиксированным Auto 1<<15 / 1<<16.
AddDict2(Auto) заменён одной строкой Qt; явный словарь не доступен,
backend отклоняет его, поскольку исходный список не содержит ручных вариантов.
Ветка kCopy не подключается: метод Copy не экспонируется нашей таблицей
7z-методов, Store сохраняет отдельное отключение контролов. Остальные
неподдерживаемые форматы и закомментированный ZSTD не включены.

### Итерация: управление solid-блоками

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::Get_Lzma2_ChunkSize`, `Add_Size`,
`CCompressDialog::SetSolidBlockSize2`, solid-блок `OnOK`,
`CompressDialog.h::NCompressDialog::CInfo` и `UpdateGUI.cpp::SetOutProperties`.
Скопированы Get_Lzma2_ChunkSize целиком, выбор единиц Add_Size, расчёт
Auto solid (включая нормализацию BZip2 и пределы 16 МиБ/4 ГиБ/16 ГиБ),
ряд логарифмов 20..36, Non Solid=0 и Full Solid=64, а также преобразование
OnOK в байты с UInt64(-1) для Full. Win32 insertion/Registry заменены
данными Qt combo и текущим выбором; названия, формулы, типы, условия
и порядок вычислений сохранены. Add_Size использует QString для строки
и локализованные единицы, без собственного алгоритма размеров.
Только solid-поля CInfo скопированы: CompressionOptions наследует этот
запись-адаптер, остальные поля CInfo ещё не перенесены. Qt задаёт
SolidIsSpecified по доступности поля перед вызовом перенесённого OnOK,
как FormatChanged upstream. Backend проверяет байты через тот же ряд
и преобразование, переносит s=<bytes>b из SetOutProperties в 7zz -ms.
Прежние bool solid / solidBlockMiB и отдельный checkbox удалены:
единственный список теперь следует исходному порядку контролов.
Полный OnOK, Registry и XZ UI остаются долгом; исходная XZ ветка Auto
сохранена, но формат создания остаётся ограничен 7z/ZIP.
Форматирование — обязательный clang-format.

### Итерация: потоки и расход памяти

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::CCompressDialog::SetNumThreads2`,
`GetMemoryUsage_Threads_Dict_DecompMem`, `AddMemUsage`, RAM-блок `OnInit`
и проверка памяти `OnOK`; `CompressDialog.h::GetDict2`;
`CPP/7zip/Common/MethodProps.cpp::Calc_From_Val_Percents`;
`CPP/Windows/System.cpp`, `.h`.

System.cpp/.h, Windows/StdAfx.h, Common/Defs.h, Common/MyVector.h и C/Threads.h
скопированы побайтово в third_party/7zip с исходными путями и notices.
Переиспользованы CProcessAffinity и GetRamSize, включая Windows processor
groups, POSIX affinity, sysctl/sysinfo: они уже переносимы, новые Qt/sysconf
аналоги не написаны. POSIX CProcessAffinity не имеет Windows метода
Get_and_return_NumProcessThreads_and_SysThreads; нормализация num1/num2
скопирована из его тела в узкий адаптер GetSystemInfo. RAM-блок OnInit
сохранён с 32-bit пределом 7<<28, минимумом 64 МиБ и исходными 80%.
Calc_From_Val_Percents скопирован целиком в зеркальный Common/MethodProps.cpp;
COM/строковые свойства остальных функций MethodProps пока не нужны
процессной границе, поэтому адаптированный заголовок содержит только
используемый интерфейс и исходные UInt64/UInt32, без новых вычислений.

SetNumThreads2 сохраняет исходные пределы LZMA=2/LZMA2=512/BZip2=64,
Auto-only для 7z PPMd/Copy, ZIP=32/128 по архитектуре, CPU*2 предел списка,
а также циклы уменьшения Auto для ZIP и LZMA2 по памяти. Registry и
Win32 вставка заменены текущим Qt выбором и возвратом данных. Скопирована
полная активная функция оценки памяти со всеми доступными ветками и
исходными арифметикой/условиями/порядком; вместо методов виджета её
параметры содержат format, level, method, dictionary и blockSizeLog.
AddMemUsage сохраняет единицы MB/GB/TB и округление, QString заменяет
UString insertion. Пустой default устраняет enum -Wswitch без смены поведения.

Qt-представление ресурсов вынесено в createarchivedialog_resources.cpp:
GetDict2 адаптирует исходное переключение Spec/Auto к combo=0; системные
данные читаются один раз на диалог. Spinbox 0..256 заменён combo Auto/ряд,
показаны процессные/аппаратные CPU и оценки сжатия/распаковки по порядку
CompressDialog.rc. Backend получает тот же набор через порт, проверяет
явные потоки, применяет условие memUsage>limit из OnOK перед запуском
и сохраняет -mmt только для явного выбора. При неизвестной RAM отказ
не применяется (адаптация недоступного системного значения); Auto по
памяти отключён, как в исходном SetNumThreads2. Пользовательский RAM
selector, Registry, ZSTD и UI XZ остаются долгом. Форматирование — clang-format.

### Итерация: исходный разбор лимита памяти

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::NCompression::CMemUse`,
`ZipRegistry.cpp::ParseMemUse`, `CMemUse::Parse` и
`CPP/Common/MyString.h::MyCharLower_Ascii(wchar_t)`.
Структура, Clear/GetBytes, весь указательный парсер и ASCII helper
скопированы с сохранением имён, типов, ветвей и проверки переполнения.
Адаптация: Parse принимает QString и передаёт нуль-терминированную
std::wstring исходному wchar_t парсеру. Это позволяет использовать уже
скопированный ConvertStringToUInt64 на всех ОС, без нового Qt-парсера.
CMemUse использует ранее перенесённый Calc_From_Val_Percents, включая
насыщение и округление. Остальной ZipRegistry (Windows Registry,
критические секции и настройки форматов) не нужен для этого ядра и
остаётся долгом; не добавлены платформенные Registry зависимости.
Форматирование — обязательный clang-format. Подключение пользовательского
предела к диалогу и backend выполняется следующей итерацией.

### Итерация: пользовательский лимит памяти от диалога до движка

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.cpp::AddMemSize`, `AddMemComboItem`,
`SetMemUseCombo`, `Get_MemUse_Bytes`, RAM-блок `OnInit`, память в `OnOK`,
`SetNumThreads2`; `CompressDialog.h::CInfo::MemUsage`;
`UpdateGUI.cpp::SetOutProperties` (ветка memuse).
Скопированы форматирование размеров, Auto/процентные/байтовые элементы,
оба цикла выбора и вставка нестандартного текущего значения, GetBytes
и fallback Auto. Алгоритмы, типы, пределы и порядок ветвей сохранены.
Вместо UString и Win32 combo функции возвращают Qt-строки/данные;
Registry CFormatOptions заменён параметром spec, операции SetCurSel
удалены из ядра — текущий выбор принадлежит Qt. Show/Enable по MemUse_
не нужен: оба экспонируемых формата имеют этот flag. Для байтов mu.Val
эквивалентен исходному GetBytes(_ramSize_Reduced). CSystemInfo получил
исходное _ramSize_Reduced после минимального размера 64 МиБ.

SetNumThreads2 теперь вызывает перенесённый Get_MemUse_Bytes вместо
фиксированного RamUsageAuto. При смене solid-блока Qt пересчитывает
Auto-потоки; backend восстанавливает log из проверяемого CInfo, чтобы
оценка соответствовала выбранному solid. Это адаптер байтовой process
границы, не новая формула оценки памяти. OnOK сохраняет проверку
memUsage!=UInt64(-1), затем memUsage>limit; Qt заменяет MessageBoxError.
Проверка применяется лишь к созданию: диалог добавления не экспонирует
сжатие и backend сохраняет существующие параметры.

CInfo передаёт исходную CMemUse, backend повторяет проверку и переносит
ветку SetOutProperties: проценты в memuse=<value>%, байты в memuse=<value>b,
Qt QProcess добавляет префикс -m. Неизвестная RAM сохраняет ранее описанную
адаптацию Auto без отказа; явный предел проверяется. Оценка/RAM в подсказке
вместо исходной длинной строки со slash — адаптация ширины Qt-формы.
Windows Registry, сохранение по формату и остальные поля CInfo остаются
долгом. Форматирование — обязательный clang-format.

### Итерация: сохранение лимита памяти по формату

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::CFormatOptions::{FormatID,MemUse}`,
`CInfo::Formats`, `ZipRegistry.cpp::CInfo::{Load,Save}` (пределы памяти);
`CompressDialog.cpp::FindRegistryFormat`, `FindRegistryFormat_Always`,
`Get_FormatOptions`, `SaveOptionsInMem`, `SetMemUseCombo`, FormatChanged
и Save в OnOK. Поля остальных compression options не объявляются
перенесёнными: эта итерация ограничена памятью.

Скопированы поиск формата без учёта регистра, поиск-or-add, доступ по индексу,
сохранение MemUse предыдущего формата до его смены и формирование исходного
выбранного индекса при вставке нестандартного spec. Имена, порядок ветвей,
возвращаемые индексы и CObjectVector сохранены; используется уже побайтово
скопированный Common/MyVector.h. Qt presentation-зависимые UString/
CSysString заменены QString, Get_ArcInfoEx/Info.FormatIndex — текущим
и предыдущим именем Qt combo. Тела принадлежат одному Qt адаптеру
createarchivedialog_preferences.cpp; нет второго алгоритма поиска форматов.

В ZipRegistry.cpp скопированы Clear при Load, обход formatIDs,
создание CFormatOptions, чтение MemUse и Formats.Add; Save сохраняет
исходный обход FOR_VECTOR и заменяет только группу options. Win32
Registry open/create/GetRegString/SetRegString заменены QSettings INI
в выделенной группе Compression/Options существующего toolbar.ini.
Имена MemUse32/MemUse64 сохраняют исходное разделение архитектур.
Win32 CS_LOCK не нужен: загрузка/сохранение вызываются только в GUI thread.
Методы принимают путь вместо глобального registry key, что обеспечивает
детерминированные фикстуры. Другие группы настроек не затрагиваются.
Load/Save переносят только память/FormatID; остальные поля исходного CInfo
и CFormatOptions остаются долгом. Save вызывается после всех проверок OK;
Cancel и неуспешная валидация не записывают изменения. Форматирование —
обязательный clang-format.

### Итерация: исходные режимы и история диалога извлечения

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/ExtractDialog.cpp::{kPathModeButtonsVals,kOverwriteButtonsVals,
AddComboItems,AddUniqueString,CExtractDialog::OnInit,OnOK}`, `.h`;
`CPP/7zip/UI/Common/ZipRegistry.*::NExtract::CInfo::{Load,Save}`;
`CPP/Windows/FileName.cpp::NormalizeDirPathPrefix`.
`UI/Common/ExtractMode.h` скопирован побайтово в third_party/7zip.

Класс CExtractDialog и адаптированные тела находятся в зеркальных GUI путях.
Исходные таблицы, исходный порядок overwrite Ask/Overwrite/Skip/Rename/
RenameExisting, циклы AddComboItems и AddUniqueString, предел 16,
обработка выбранного/введённого пути, исключение выбранного или последнего
элемента, Trim, NormalizeDirPathPrefix, порядок ветвей смены режимов и
показа пароля, очистка/пересборка истории и сохранение в OnOK скопированы.
Сохраняются CObjectVector, enum и CBoolPair upstream. Win32 controls,
строки локализации и сообщения заменены Qt; UI constructor — presentation
адаптация существующей формы по ExtractDialog.rc. Исходная нормализация
пути перенесена локальным QString helper: пустой путь, проверка последнего
разделителя, добавление разделителя. Qt принимает оба разделителя на Windows.

Qt editable combo может удерживать индекс при редактировании текста:
узкий адаптер переводит этот случай в CB_ERR до исходной ветки OnOK.
После addItem восстанавливается typed text, поскольку Qt автоматически
выбирает первый элемент. Backend API переводит исходные enum в свои
ExtractionOptions; параметры 7zz остаются за ArchiveBackend. Absolute
paths не добавляются в combo (граница backend их не поддерживает); kAsk
адаптирован к прежнему отказу при конфликте вместо интерактивного callback.

Load/Save переносят только пути, режимы/Force и ShowPassword из NExtract
CInfo; QSettings заменяет Registry, GUI-thread вызовы не требуют CS_LOCK.
Сохраняются оригинальные имена ключей в Extraction; старый history/extract
читается только как миграционный fallback. Пароль не записывается.
MainWindow лишился самостоятельного алгоритма истории, старый cpp удалён,
header содержит только alias ExtractDialog=CExtractDialog. Неиспользуемые
SFX, Lang, Help, NtSecurity, ElimDup, SplitDest пока остаются долгом.
Обязательный clang-format изменяет оформление скопированного тела.

### Итерация: SplitDest и исходное разделение пути

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/Common/Wildcard.cpp::SplitPathToParts_Smart`,
`CPP/7zip/UI/GUI/ExtractDialog.cpp::CExtractDialog::{OnInit,OnOK,
OnButtonClicked}` (ветки IDX_EXTRACT_NAME_ENABLE / SplitDest),
`UI/Common/ZipRegistry.*::NExtract::CInfo::SplitDest`.

Указательный обход SplitPathToParts_Smart скопирован в существующий
зеркальный Common/Wildcard.cpp; имя, start/p, пропуск одного конечного
разделителя и обратный поиск сохранены. QString/QChar заменяют UString/
wchar_t, left/mid заменяют SetFrom/присваивание p. IsPathSepar сохраняет
исходное разделение платформ: на Windows slash/backslash, POSIX slash.
Структура borrowed Wildcard не реорганизуется; это единственный helper.

Скопированы ветки OnInit: при SplitDest.Checked исходный путь разделяется,
пустой prefix заменяется name (корень и одиночное относительное имя), иначе
name попадает в отдельный Edit. OnOK сохраняет исходные Trim, +=pathName,
NormalizeDirPathPrefix, сравнение SplitDest со старым значением и Def=true
при изменении. Значение по умолчанию true и Key_Get_BoolPair_true перенесены
в QSettings Load/Save NExtract::CInfo; только изменённый bool сохраняется.
Name field показывается/скрывается Qt toggled вместо Win32 OnButtonClicked.
Отмена не записывает флажок/историю, поле имени само не сохраняется:
в истории хранится составной конечный путь, как в upstream.

Qt presentation повторяет checkbox/Edit перед Path mode слева, Password
справа по ExtractDialog.rc. У исходного пустого checkbox добавлены tooltip
и accessibleName; Edit получил accessibleName/placeholder. Qt getter
до OK собирает preview из тех же полей, после OK возвращает DirPath.
Параметры движка не меняются: backend проверяет составное назначение
существующей защитой путей/ссылок и создаёт его. NtSecurity, ElimDup,
интерактивный Ask остаются долгом. Форматирование — clang-format.

## Полный профиль доступных параметров сжатия (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.h::NCompression::CFormatOptions`,
`ZipRegistry.cpp::CInfo::Save/Load`, `Key_Set_UInt32/Key_Get_UInt32`;
`CPP/7zip/UI/GUI/CompressDialog.cpp::SaveOptionsInMem`, `FormatChanged`,
`SetLevel2`, registry branches `SetMethod2`, `SetDictionary2`, `SetOrder2`,
`SetSolidBlockSize2`, `SetNumThreads2`.

Скопированы поля Level/Dictionary/Order/BlockLogSize/NumThreads/Method,
инициализация и ResetForLevelChange/Reset_BlockLogSize. Save/Load сохраняют
исходный порядок полей и DWORD sentinel `UInt32(-1)` (отсутствующее значение).
Win32 Registry заменён узким QSettings INI адаптером; UString Method заменён
QString на границе Qt. У SaveOptionsInMem сохранены ветви преобразования
UInt64 dictionary в DWORD, включая `UInt32(-2)` для 4 GiB. Нулевые Qt Auto
словаря/слова/потоков преобразуются в исходный sentinel. Solid хранит исходный
логарифм, в том числе 0/64/Auto. Пока параметры времени, произвольные options
и encryption method CFormatOptions не подключены.

В существующем Qt presentation adapter createarchivedialog_preferences.cpp
FormatChanged восстанавливает level → method → dictionary/order → solid →
threads, с исходной проверкой совпадения метода перед восстановлением его
параметров. QSignalBlocker заменяет отсутствие Win32 CBN_SELCHANGE при
программном заполнении. Недоступные значения возвращаются к Auto; выбор
большого словаря ограничивается текущим исходным рядом. SetLevel2 теперь
использует исходные ветви persisted level (default 5, invalid >9 →9) и
nearest-selection цикл. Обработчики смены уровня/метода/словаря перенесены
следующей итерацией. Пароли в профиль не входят.

## Смена уровня сжатия (2026-09-28)

Источник того же commit: `CompressDialog.cpp::OnCommand`, ветка
`IDC_COMPRESS_LEVEL`, `CompressDialog.h::SetMethod/MethodChanged`,
`ZipRegistry.h::CFormatOptions::ResetForLevelChange`.
В Qt-обработчик скопирован вызов ResetForLevelChange перед заполнением
метода и его зависимых контролов. Исходный reset очищает Method и присваивает
sentinel Level/Dictionary/Order/BlockLogSize/NumThreads; MemUse сохраняется.
Программное заполнение выполняет уже адаптированный FormatChanged, с
записью текущего выбранного Level в in-memory профиль перед заполнением:
это необходимо, поскольку Qt adapter заполняет также level combo, тогда
как исходная ветка CBN_SELCHANGE использует уже выбранный Win32 level.
Расчёты solid/threads/memory выполняются в исходном порядке зависимостей.
SFX/checksum ветки не применимы к текущим поддерживаемым 7z/ZIP.
Уровень 0 выключает Qt-контролы; список методов оставлен видимым и
недоступным (upstream очищает его). Пароль/лимит памяти не сбрасываются.

В той же итерации перенесены ветви `IDC_COMPRESS_METHOD`,
`IDC_COMPRESS_DICTIONARY`, `IDC_COMPRESS_MEM_USE`. Qt MethodChanged
заполняет словарь и слово в исходном порядке, затем solid/threads/memory
по вызывающему обработчику upstream. Параметры восстанавливаются только
при совпадении Method с сохранённым профилем; другие методы получают Auto.
При смене словаря скопированы SaveOptionsInMem, проверка
`blockSizeLog != kSolidLog_NoSolid && != kSolidLog_FullSolid` и
Reset_BlockLogSize: обычный размер solid сбрасывается к Auto, специальные
0/64 сохраняются. SaveOptionsInMem при смене лимита сохраняет explicit threads
перед их пересчётом. Оба сохранения меняют только in-memory профиль до OK.

## Рекурсивное копирование файловой системы (2026-09-28)

Источник: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CCopyState::CallProgress`,
`CopyFile_Ask` (UseReadWriteMode), `CombinePath`, `CopyFolder`.
В том же зеркальном файле сохранены порядок CallProgress → IsDestChild →
создание папки → перечисление → рекурсивная ветка/CopyFile_Ask, имена
srcPath2/destPath2 и накопление StartPos после MyCopyFile.
Самостоятельный обход copyEntry и обвязка copyFile удалены из worker.

CCopyState вынесен в существующий FSFolder.h и namespace NFsFolder для
доступа Qt-worker; из CProgressInfo нужен только StartPos. COM callbacks
заменены функциями Progress/CurrentFile, HRESULT/RINOK — bool и проверкой
результата; ошибки передаются в ErrorMessage. QString/QFileInfo/QDir заменяют
FString/CFileInfo/CEnumerator и платформенные операции. Qt entryInfoList
не предоставляет ошибку Next; перед обходом проверяется доступность папки.
Скрытые и системные записи включены, ссылки отклоняются, stage создаётся
только в отсутствующей папке (mkdir вместо CreateComplexDir).

AskWrite адаптирован как отказ при конфликте до открытия файла, а не
интерактивный запрос. Неиспользуемые skip/TotalSize и Win32 CopyFileEx ветки
не подключены. MoveMode/MoveFile_Sys/RemoveDir остаются у транзакционного
worker: он использует rename и откат, копирование с удалением источника
нарушило бы текущую гарантию сохранности при ошибке. Межтомный перенос
остаётся неподдержанным. Mandatory clang-format меняет оформление.

## Диалог копирования/перемещения и история (2026-09-28)

Источник того же commit `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/CopyDialog.cpp`, `.h`, `.rc`,
`ViewSettings.cpp::SaveStringList`, `ReadStringList`, `SaveCopyHistory`,
`ReadCopyHistory`, `AddUniqueStringToHeadOfList`, `App.cpp::CApp::Copy`
(обновление истории назначения). Исходный класс CCopyDialog и поля
Title/Static/Value/Info/Strings сохранены в зеркальном пути. OnInit заполняет
историю перед Value и устанавливает Info; OnOK получает Value только при OK.
OnButtonSetPath сохраняет cancel → NormalizeDirPathPrefix → SetCurSel(-1)
→ SetText. Qt QFileDialog заменяет MyBrowseForFolder, slash-проверка после
fromNativeSeparators — платформенный NormalizeDirPathPrefix.

Win32 Attach/SetItemText/NormalizeSize/OnSize заменены Qt-контролами и layouts
с исходным порядком label → path/browse → info → OK/Cancel. Вместо Static
SS_LEFTNOWORDWRAP используется read-only QPlainTextEdit без переноса строк:
прокрутка сохраняет доступ к длинным именам и большому выделению. Входной
список имён и конструктор — существующая Qt интеграция, а не перенос полного
CPanel::GetItemsInfoString (property totals ещё не подключены). Исходный
kCopyDialog_NumInfoLines сохранён для будущей сводки.

Save/ReadStringList используют QSettings вместо Registry и explicit settings
path вместо HKEY; вызовы идут в GUI-thread, lock не нужен. Имя CopyHistory
сохранено; старый copy/history не читается и не изменяется (fallback удалён
в итерации истории/избранного по указанию пользователя). Исходный цикл
удаляет все совпадения без учёта регистра и вставляет новый путь в начало;
App copy workflow ограничивает список 20 записями и сохраняет после принятого
существующего назначения. Проверка/создание назначения и backend остались
на прежних границах; произвольное переименование цели/новая папка из CApp::Copy
пока не перенесены. Самостоятельный copydialog.cpp удалён; header — alias.
Форматирование изменено обязательным clang-format.

## Сводка выделения для Copy/Move (2026-09-28)

Источник `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/App.cpp::CPanel::GetItemsInfoString`,
`AddSizeValue`, `AddValuePair1`, `AddValuePair2`, `AddPropValueToSum`;
`PanelListNotify.cpp::ConvertSizeToString` (обе перегрузки, INT_TO_STR_SPEC);
`Panel.h::GetItemsInfoString`, `CopyDialog.h::kCopyDialog_NumInfoLines`.
Скопированы исходные циклы и ветви: суммы папок/файлов раздельно, неизвестное
значение UInt64(-1) распространяется без дальнейшего счёта, общий размер
выводится только при двух ненулевых определённых суммах, затем путь и первые
11−6 имён с разделителем для папок и многоточием для остатка. Сохраняются
имена CPanel, методов и _currentFolderPrefix. Форматирование UInt32/UInt64
скопировано целиком с промежуточным Byte[32], wchar_t[32] и группами по три
цифры; QString::fromWCharArray заменяет UString только на выходе.

В зеркальном Panel.h портирована только часть CPanel, владеющая сводкой.
IFolderFolder заменён snapshot CObjectVector<CPanelItem> с RelPath/IsFolder/
SizeDefined/Size; upstream CRecordVector<UInt32> используется без изменений.
AddPropValueToSum получает тот же индекс и свойство Size без COM/PROPVARIANT;
языковые resource строки заменены русскими Qt строками. QString заменяет
UString для UI текста, noexcept — throw(). Конструктор snapshot и optional
Info аргумент CCopyDialog — узкая интеграция: исходный App устанавливает Info
до Create, а Qt создаёт виджеты в конструкторе.

MainWindow::selectedCopyInfo только собирает свойства выбранных строк:
относительное имя из представления, размеры файлов из UserRole, папок архива
из CProxyArc. Для папок диска SizeDefined=false соответствует FSFolder.cpp
kpidSize до FolderStat_Defined. Missing-size свойства архивных файлов уже
нормализуются текущим backend в 0; различение отсутствующего Size остаётся
долгом parser/metadata, без изменения границы ArchiveBackend в этой итерации.
Сводка не устраняет пересечение выбранных папок/файлов: как upstream, суммы
относятся к выбранным строкам. `sevenzip_panel` собирает эти UI модули один
раз, не добавляя QWidget-зависимость к sevenzip_common. clang-format меняет
оформление. Предыдущий конструктор имён оставлен как compatibility fallback;
рабочие F5/F6 теперь всегда получают перенесённую сводку.

## P1-04.1 — незашифрованные архивные CreateFolder/Rename/Delete (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/Agent/AgentOut.cpp::CAgent::DeleteItems/CreateFolder/RenameItem`,
`CPP/7zip/UI/Agent/ArchiveFolderOut.cpp::CAgentFolder::Delete/CreateFolder/Rename`
и `CommonUpdateOperation`, `CPP/7zip/UI/FileManager/PanelOperations.cpp::
CPanel::CreateFolder/OnEndLabelEdit/DeleteItems` и `CThreadFolderOperations`.
Сохранены исходные пары `CUpdatePair2`, real-index expansion (включая
содержимое папки и альтернативные потоки), `DeleteOperation`, проверка
совпадения имени папки, перенос имени потомков, `CArchiveUpdateCallback`
и порядок temp → close → MoveToOriginal → ReOpen → восстановление папки.
Последний FM owner был перенесён в P1-03; в этой итерации подключены его
архивные consumers и source progress/cancel/confirmation.

`qtarchivesourceupdate.cpp` адаптирует только внешний `IOutArchive` transport:
проверяет исходный callback-план и имена через source `Correct_FsPath`, выводит
Delete/Rename/CreateFolder и передаёт их `ArchiveBackend::updateSourceMutation`.
Backend копирует исходный архив в отдельный output и применяет `7zz` с
`-spd`, не публикуя копию; публикация и откат остаются в оригинальном
`CAgentFolder::CommonUpdateOperation`. Empty directory создаётся во временном
source root. Original ZIP/7z writer исполнялся бы in-process; такой внешний
file-only adapter обязателен для текущего engine boundary. `SetAsDir` и
`GetCurUtc_FiTime` заменяют Win32-only поля `Attrib`/`FILETIME` на переносимые
исходные типы. Обязательный clang-format меняет только оформление.

Qt F7, F2 inline и Delete/Shift+Delete теперь вызывают source CPanel;
после native ReOpen список перерисовывается без второго ReOpen, сохраняются
выбор/фокус и путь папки. Flat editor показывает source basename, а не
отображаемый prefix. Существующий `ArchiveBackend::remove` пока нужен
архивному Move/transfer P1-01; `add`/CopyFrom, comment/SetProperty,
зашифрованные и вложенные архивы, общий SetProperties и handler writer
capabilities остаются открытыми P1-04.2/.3, P1-05/P1-09. Это частичный
порт файлов AgentOut/ArchiveFolderOut; зеркальные имена не означают
готовность остальных функций. Original notices сохранены, лицензия:
`third_party/7zip/License.txt`.

## P1-04.2 — пароль при source archive mutation (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CThreadFolderOperations::DoOperation`:
возвращены исходные строки наследования `UsePassword`/`Password` из
`_parentFolders.Back()` после `CUpdateCallback100Imp::Init`. Остальные
исходные тела `CAgentFolder::CommonUpdateOperation` и
`CUpdateCallback100Imp::CryptoGetTextPassword2` сохраняются от P1-04.1.
Отклонение — `GetParentFolders()` вместо прямого доступа к частному полю Qt
панели; clang-format меняет оформление.

Новый file-only adapter передаёт пароль из `ICryptoGetTextPassword2` в
`ArchiveBackend::updateSourceMutation` и stdin 7zz. При повторном открытии
оригинальный `COpenCallbackImp::ReOpenCallback` несёт update callback;
`QtArchiveFileResource` извлекает из него уже определённый пароль до
`openFolder`, сохраняя исходный порядок publish → ReOpen. Если пароль не
определён, прежний callback-путь запрашивает его. Win32/in-process 7-Zip
writer здесь заменён внешним file-only сервисом; 7z header encryption и ZIP
AES поддержаны 7zz, новый пустой ZIP folder entry сам по себе не содержит
шифруемых данных. Add/Edit/Move, nested updates и comment/SetProperty этим
частичным портом не покрыты. Source notices и лицензия сохранены в
`third_party/7zip/License.txt`.

## P1-17 — source ZIP writer and GUI consumer (details, 2026-10-02)

Current GUI checkpoint supersedes the historical disabled/next statements below:
original `PanelOperations.cpp::CPanel::ChangeComment` now owns focused real-index
selection, parent/read-only/hash guards, native comment property, item title,
shared `ComboDialog` and refresh. The original `ArchiveFolderOut::SetProperty`
and `CAgent::CommentItem` functional bodies remain the property/update owners.
Qt replaces Win32 list/control calls and runs SetProperty on the existing source
`CThreadFolderOperations` worker with `CUpdateCallback100Imp`, rather than the
original NULL-progress synchronous GUI call. This is the responsiveness adapter;
the source ZIP update plan, callbacks, work-dir publication and reopen are reused.

The narrow `IQtFolderPropertyCapabilities` interface exposes actual comment
support to Panel without coupling shared presentation targets to concrete Agent.
The CApp command route binds resource `IDM_COMMENT` and original Ctrl+Z shortcut.
Comment is available only for a focused real item with the validated native ZIP
writer and writable ancestor chain. `ComboDialog::BindLanguage` preserves a
caller-supplied dynamic title when titleID is zero: source language ID0 means
7-Zip, not an absent string. Prompt and editable value still use the shared view;
raw whitespace and empty comments remain valid.

The caller retains original Set Comment Error/HRESULT presentation. Qt macOS
message boxes omit window captions, so the original error caption is shown as
the message heading with the HRESULT detail below. Cancellation is quiet.
Native progress uses original Sync pause/stop callbacks; the local transport
accepts native pause requests without attempting to suspend a nonexistent child
process. A subsequent process-backed metadata reopen uses the existing controls.

Current GUI/callback changes built37772 exit0; wrapper22135 exit0,5/5 suites
PASS15.95s on macOS arm64 Qt6.11.2 (native_comment_ui/native_comment/
codec_commands_ui/combo_dialog/archive_progress). Actual EN/RU plain/AES command
success, editable cancellation, UTF-8 oversized-comment writer refusal/error,
focus, readback, bytes and extraction checked. Four current Combo JSON reports
cover360×150/260×150 EN/RU. Nested publication and native paused cancellation
tests are being added; no full gate or acceptance yet. Windows/Linux runtime and
rendered Windows appearance remain unverified.

Latest backend checkpoint (supersedes the prerequisite-only next statements
below): copied original `CAgent::CommentItem` body from
`CPP/7zip/UI/Agent/AgentOut.cpp`, original ZIP/VT_BSTR `CAgentFolder::SetProperty`
guard/body from `ArchiveFolderOut.cpp` and its `AGENT_OP_Comment` dispatch. No
comment plan algorithm rewritten; existing source CArchiveUpdateCallback owns
CommentIndex/Comment and metadata forwarding. The process-backed Agent's
GetTypeOfArc adapter resolves its retained detected ArchiveType because its
synthetic CArc FormatIndex cannot index the native/global catalog.

ArchiveCodecCatalog retains verified module metadata, flags, signatures and
extensions, replacing only the ZIP input/output factory hooks with the copied
source CHandler (`LibIndex=-1`). Source OpenArchive performs detection/opening
and per-instance SetCompressCodecsInfo as before; the immutable catalog/handler
lease owns external module lifetime. Other factories remain module-backed.
ArchiveBackend::updateNativeComment invokes that original handler IOutArchive;
the Qt resource dispatches only the source callback's explicit comment plan
to this route. Other update/extract/item-list transports remain explicit process
integration. No handwritten ZIP record editing or CLI comment workaround.

The source folder proxy currently uses CLI item order. Before native per-item
properties/comment updates, QtArchiveFileResource verifies count and each Path
in the same order (NFC comparison only at the Qt Unicode boundary). A mismatch
retains process properties and disables comment capability; native indexes
are never guessed. Once validated, original native properties preserve comment
and writer metadata. The source work-dir transaction closes all native readers
before publication; reopen now obtains a fresh native reader and validates order
again. The existing progress/open/password projection was extracted unchanged
to qtarchivenativeopenprogress.cpp/.h for initial open and reopen.

Actual native_comment tests exercise source IFolderOperations::SetProperty through
CAgent/CommonUpdate/ArchiveBackend/source ZIP writer, transaction publication and
reopen, Unicode comment read-back and empty-comment clearing. Ordinary/AES256
archives each cover success, original writer refusal of65536-byte comments, and
callback E_ABORT; original input/archive unchanged on non-success and no owned
staging remnants. Successful archives extract exact binary bytes through 7zz.
Final affected build90336 exit0; focused wrapper76216 exit0:5/5 suites PASS2.82s
(native_comment/native_zip_writer/native_folder_opening/codec_update/
owned_temp_publication), macOS arm64 Qt6.11.2, 2026-10-02. Full all-target build
36443 subsequently failed a backend-only link: its target does not contain Agent,
so the source g_ExternalCodecs fallback storage could not remain in Agent.cpp.
The single empty ABI storage definition now lives in the narrow standalone-library
integration unit `src/archiveexternalcodecs.cpp`, referenced by sevenzip_zip.
No global catalog is populated; per-handler leases remain authoritative. This
relocation is a build boundary adaptation of the original Agent global, not a
new codec algorithm. Agent.cpp no longer contains the duplicate definition.
Repeated all-target build35574 exited0 (only existing Qt date deprecations).
No full gate/GUI acceptance/commit.
Next: actual CPanel::ChangeComment command/capability/dialog/error/refresh and
UI geometry, broader shared-property/open/update regressions and full gate.

Subsequent shared regression checkpoint: wrapper43406 found20/22 passing, with
SIGSEGV in archive_opening and codec_commands_ui parser cases. LLDB20670 showed
the new local hasCommentWriter guard indexing Formats with source parser
FormatIndex=-1 (unsigned4294967295). The native parser algorithm was unchanged;
the integration capability guard now checks the signed index and catalog bounds
before lookup. Rebuilt all targets63060 exit0; repeated the same expanded wrapper
67928 exit0:22/22 suites PASS14.39s, macOS arm64 Qt6.11.2, 2026-10-02.
Current baseline verifier e684d2 PASS351 exact/163 adapted/3 local; diff check
passed. No live build/test/debug handles. This supersedes earlier live handles
and failed checkpoint results; GUI/full gate/iteration acceptance remain pending.

Baseline: https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Copied complete original
`CPP/7zip/Archive/Zip/ZipHandler.cpp`, `ZipHandlerOut.cpp`, `ZipIn.cpp`,
`ZipItem.cpp`, `ZipOut.cpp`, `ZipUpdate.cpp`, `ZipAddCommon.cpp` and the
codec/crypto/stream dependency closure into `third_party/7zip/` with their
original paths. Dependency units include original LZMA/XZ/PPMd encoders and
decoders, ZIP/AES crypto and C support. No compression or ZIP algorithm rewritten.
Registration (`ZipRegister.cpp`) is not copied/linked: the integration
explicitly constructs the source handler behind ArchiveBackend, while the existing
leased module catalog continues to own format discovery. This is partial bundle
integration, not a replacement of the entire upstream DLL/export layer.

129 newly borrowed files are recorded individually in upstream-baseline.json:
127 exact plus two adapted headers under `src/CPP/7zip/Archive/`:
`Common/HandlerOut.h` and `Zip/ZipCompressionMode.h`. Their only functional
adaptation is canonical include ownership to reuse the already adapted
`Common/MethodProps.h`; notices and project formatting are accounted separately.
The single original `g_ExternalCodecs` fallback definition is provided in local
`src/archiveexternalcodecs.cpp` for the source handlers' external-codec ABI. It remains empty;
future consumers must pass/retain their immutable catalog per handler, rather
than mutate shared global codec state. Shared dependencies have one authoritative
source path; the isolated static target may compile existing stream/time modules
also needed by other static libraries.

`cmake/native_zip.cmake` builds these original sources as `sevenzip_zip`.
`native_zip_writer_tests` constructs the source `NArchive::NZip::CHandler`, uses
the already copied `CArchiveUpdateCallback` comment/no-data update plan, and
verifies a Unicode/spaces comment, two entries, unchanged Path/Size/CRC/encryption,
and exact binary extraction through the independent installed official `7zz`.
Ordinary and AES256 ZIP each cover success, output write failure, and callback
cancellation; original archive/input bytes remain unchanged in all cases.
This is a prerequisite check, not the source CAgent::CommentItem or GUI consumer.

Checks: isolated source-library build52641 exit0; final native writer test build
85154 exit0; wrapper CTest307021 exit0, 1/1 suite PASS0.51s (six data scenarios),
macOS arm64/Qt6.11.2, 2026-10-02. Intermediate compile failures were exported
adapted-header include paths and a test using the wrong FileStreams API; link
failure identified the omitted original global fallback. No live test/build
handles, full gate, acceptance or iteration commit yet. Command remains disabled.
Baseline verifier e39244 passed351 exact/163 adapted/3 local paths across the
single pinned revision; git diff --check passed after documentation updates.
Next: backend-owned native handler/lease integration, actual source Agent comment
and SetProperty/ChangeComment GUI, publication/reopen/refresh/failure/cancel and
full checks. Windows/Linux runtime and rendered Windows parity remain unverified.

## P1-04.3 — prerequisite для ZIP item comment (2026-09-30; перенесён в P1-17)

Точная база исследования: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelOperations.cpp::CPanel::ChangeComment`,
`CPP/7zip/UI/Agent/ArchiveFolderOut.cpp::CAgentFolder::SetProperty` и
`CommonUpdateOperation(AGENT_OP_Comment)`, `AgentOut.cpp::CAgent::CommentItem`,
`CPP/7zip/Archive/Zip/ZipHandlerOut.cpp::CHandler::UpdateItems`.
Исходный FM получает `kpidComment`, открывает ComboDialog, затем передаёт
`VT_BSTR` в SetProperty; Agent помечает `NewProps` одного real index и отдаёт
`CommentIndex`/`Comment` исходному ZIP writer. Другие форматы SetProperty
отклоняет.

Установленный `7zz` 26.03 на macOS arm64 имеет CLI команды a/d/e/h/i/l/rn/
t/u/x и перечисленные в `7zz -h` ключи без операции изменения ZIP item
comment. Текущий проект не содержит исходный `ZipHandlerOut.cpp` и его
in-process writer dependencies; file-only `QtArchiveFileResource::Update`
не может выполнить `CommentIndex`/`Comment`. Не заменяем ZIP-структуры своей
реализацией и не называем операцию готовой. Prerequisite: портировать ZIP
`IOutArchive` handler и необходимые исходные зависимости в ArchiveBackend,
сохранив транзакционную публикацию, затем подключить исходные Agent/FM bodies
и Qt диалог, success/failure/cancel, Unicode и сохранность данных/пароля.
Никакие исходные bodies в этом исследовании не скопированы. Это историческое
исследование P1-04.3 теперь принадлежит самостоятельному blocked пакету P1-17;
parent P1-04 принят по ранее проверенным .1/.2 и не включает ZIP comment.

## P1-11.1 — корень и смонтированные тома (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/RootFolder.cpp/.h::CRootFolder::Init/LoadItems/
BindToFolder/GetProperty`, `FSDrives.cpp/.h::CFSDrives::LoadItems/GetProperty/
BindToFolder`, `FSFolder.cpp::CFSFolder::Init`. Исходные body структуры
`CRootFolder`/`CFSDrives` и перечисление/свойства Windows-дисков сохранены из
P0-04/P0-05. В этой итерации non-Windows root получает второй виртуальный
элемент «Тома», который привязывает `CFSDrives`; исходный «Компьютер» → `/`
сохранён. Это намеренная platform adaptation: 7zFM Win32 показывает drives,
а прежний non-Windows branch исходника только `/`.

`qtvolumemounts.cpp` — узкий platform service на `QStorageInfo`, передающий
путь, отображаемое имя, файловую систему и доступные размеры в source
`CDriveInfo`; корень `/` остаётся доступен, даже если ОС не вернула его в
перечне. `CFSDrives::BindToFolderSpec` заново проверяет присутствие mount и
доступность каталога до source `CFSFolder::Init`, чтобы исчезнувший или
нечитаемый том не открыл устаревший путь. Инъекция provider на экземпляр
нужна для локальных deterministic fixtures, без глобального состояния.
`MainWindow::renderRootFolder` представляет два исходных folder type в одном
Qt списке, показывает source свойства томов и блокирует mutation commands.

Не перенесены Windows `CNetFolder`, browse navigation и device-image
`CopyFileSpec/CopyTo` из `FSDrives.cpp`: network owner P1-11.3, Browse entry
points owner P1-11.2, image
операции — P2-05/P3-05. Системный `QStorageInfo` на macOS не доказывает
Windows/Linux mount behavior; фактические сети/credentials не тестируются.
Оригинальные notices сохранены, лицензия — `third_party/7zip/License.txt`.

## P1-11.2 — source Browse entry points и picker consumers (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/BrowseDialog.cpp/.h::MyBrowseForFolder`,
`CBrowseInfo::BrowseForFile`, уже перенесённые `Dlg_CreateFolder` и
`CorrectFsPath`; consumers `CopyDialog.cpp::OnButtonSetPath`,
`SplitDialog.cpp::OnButtonSetPath`, GUI `ExtractDialog.cpp::OnButtonSetPath`.
Сохранены имена entry points, очистка результата до вызова folder picker,
разделение исходного FilePath на каталог/имя, возврат прежнего значения при
Cancel и индекс выбранного фильтра. Qt `QFileDialog` заменяет Win32
`NShell::BrowseForFolder`/`CommonDlg_BrowseForFile`; его native OS picker
управляет контролами, а `CBrowseInfo` остаётся владельцем пути и результата.
Текущие CreateArchive и MainWindow Open Archive используют те же входы.

Существующие статические Qt filters передаются presentation-адаптеру;
исходное построение filters из handler registry остаётся P2-01. Win32
`CommDlgExtendedError`/super-path fallback не переносится на Qt/Unix picker;
для Windows длинных путей это непроверенная ветка. Внутренний
`CBrowseDialog` tree/list/Reload/OnItemEnter/FinishOnOK (USE_MY_BROWSE_DIALOG)
не строится: в текущем upstream desktop используется системный picker,
а визуальный custom fallback для super/device paths остаётся обязательным
при подтверждении такого Windows workflow. `BrowseDialog2` Delete Temporary
Files остаётся P1-15. Original notices сохранены, лицензия —
`third_party/7zip/License.txt`.

## P1-11.3 — сетевой namespace (2026-09-30)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/NetFolder.cpp/.h::CNetFolder::Init/LoadItems/
GetProperty/BindToFolder/BindToParentFolder/GetFolderProperty`,
`CPP/Windows/Net.cpp/.h::NWindows::NNet` и
`CPP/7zip/UI/FileManager/RootFolder.cpp::CRootFolder::BindToFolder`.
`CPP/Windows/Net.cpp/.h` скопированы без функциональных изменений в
`third_party/7zip/CPP/Windows/`, собираются только на Windows с `mpr`.
Windows тела `CNetFolder` сохранены под `_WIN32`, включая исходное перечисление
ресурсов, свойства, binding и родительскую иерархию; Qt presentation
использует тот же `IFolderFolder`. Исходные имена, границы модуля и порядок
операций сохранены; clang-format изменил только оформление.

На macOS/Linux Win32 `WNet*` не доступен. Узкий `QStorageInfo` service помечает
известные сетевые файловые системы; POSIX ветка `CNetFolder` сохраняет
source-интерфейс и свойства `Name`/`Provider`, выдаёт только уже
смонтированные ресурсы и передаёт открытие в source `CFSFolder`. Перед
binding повторно проверяются mount, тип пути и читаемость: stale/исчезнувший
или недоступный ресурс не открывается. `CRootFolder` добавляет «Сеть» после
«Тома», а `CPanel::OpenParentFolder` остаётся owner возврата; Qt command
availability и Back разрешены для дочернего namespace. Это намеренное
отклонение от Windows browse: обнаружение новых серверов, подключение,
credentials и remote discovery не заявлены на POSIX; тесты не обращаются к
сети. Иконки/детали provider зависят от Qt/ОС, Windows rendered reference и
runtime Windows/Linux не проверены. Device-image части `FSDrives` остаются
P2-05/P3-05. Source notices сохранены, лицензия —
`third_party/7zip/License.txt`.

## P3-01 — воспроизводимый baseline и review обновления (2026-09-30)

Полный текущий реестр `docs/upstream-baseline.json` закрепляет repository
`https://github.com/ip7z/7zip.git` и commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` для каждого указанного пути;
`overrides` позволяет записывать отдельные полные ревизии. 192 неизменённых
файла `third_party/7zip/` совпали побайтово, включая `License.txt` из
`DOC/License.txt`; 135 адаптированных `src/CPP/` имеют соответствующий
исходный путь; три локальных заголовка PanelSelect/PanelSort/PanelSplitFile
такого пути в исходном commit не имеют. Повторных authoritative source paths
между группами нет. Старые количественные snapshots в audit сохраняются как
историческое evidence; актуальный счёт берётся из manifest, не из числа строк
упоминаний в тексте.

`scripts/upstream_baseline.py verify` повторно извлёк все исходные bytes через
`git show <full-commit>:<path>` из независимого временного Git snapshot,
скопированного без hardlinks, без старого sibling path; missing/stale entries,
двойной owner и несовпадение exact files теперь дают ошибку. Проверка
публичного сетевого clone не выполнялась. `review` сравнивает сохранённую
ревизию с предложенной до переноса, отдельно маркирует exact/adapted. В
одноразовом потомке source commit два синтетических изменения —
`CPP/7zip/UI/FileManager/NetFolder.cpp` (adapted) и `CPP/Windows/Net.cpp`
(exact) — были обнаружены и описаны; проектный код и sibling checkout не
изменялись. Реальные upstream functional fixes в P3-01 не переносились.
Дальнейшая процедура, включая проверку omitted branches, dependencies,
локальных Qt отклонений и обновление provenance только после tests, описана в
`docs/upstream-update.md`. Эта текущая приёмка требует поддержания manifest
при каждом следующем source port. Исходные notices и LGPL/public-domain
условия остаются в соответствующих файлах и `third_party/7zip/License.txt`.
`upstream_baseline`/`upstream_inventory` и полный `check.sh` 142/142 прошли
на macOS arm64 2026-09-30; Windows/Linux runtime не проверялся.

## P1-01.1 — F5/F6 между filesystem и диском назначения (2026-09-30)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/App.cpp::CApp::OnCopy`,
`PanelItems.cpp::CPanel::Get_ItemIndices_All/Get_ItemIndices_OperSmart`,
`PanelCopy.cpp::CPanel::CopyTo`, `CopyDialog.cpp/.h/.rc` и
`CPP/7zip/UI/Agent/ArchiveFolder.cpp::CAgentFolder::CopyTo`.
`Get_ItemIndices_All/Get_ItemIndices_OperSmart` перенесены из оригинала с
исходными именами и порядком. Уже перенесённый `CPanel::CopyTo` теперь
используется F5/F6 из верхнего архива, а оригинальный `CAgentFolder::CopyTo`
выбирает `kCurPaths` для текущей папки. Для F6 это стадия копирования:
upstream `CAgentFolder::CopyTo(moveMode)` возвращает `E_NOTIMPL`, поэтому
существующий Qt/`ArchiveBackend` двухстадийный Extract → Delete остаётся
узкой функциональной адаптацией; исходные записи удаляются только после
успешной стадии копирования. Ошибка/отмена не запускают Delete.

Qt `MainWindow` передаёт F5 из архива общему `CCopyDialog`, выставляет
`CurrentPaths` для исходного `CopyTo`, сохраняет source summary/history и
обновляет активную и вторую filesystem панели после копирования/перемещения.
Проверка `ArchiveBackend::validateExtraction` отнимает текущий префикс перед
проверкой конфликтов в папке назначения и отвергает записи вне текущей папки.
В корне архива прежний `7zz` fallback эквивалентен по путям, если source
extract service отсутствует; в подпапке при отсутствии service операция
отклоняется. Filesystem worker и его overwrite/cancel остаются интеграционным
долгом P1-02; исходное `App.cpp::CApp::OnCopy` управление destination,
temp staging и `CopyFrom` ещё не перенесено и входит в P1-01.2/.3.
Ссылки/ADS, внешние handlers и вложенные архивы остаются у P2-05/P2-06,
P2-02 и P1-09. Никакой исходный модуль не объявлен полностью закрытым только
по этому сценарию. Исходные notices сохранены; лицензия —
`third_party/7zip/License.txt`.

Focused `ui_operations`, `archive_selection_runtime`, `extraction_options`,
`source_extract_backend`, `copy_dialog`, `ui_layout`, `panel_key` прошли на
macOS arm64; Copy/Move dialog JSON 440×250 и 340×220 просмотрены. Полный
`./scripts/check.sh` (format, app build, все CTest) прошёл на macOS arm64,
Qt 6.11.2, 2026-09-30. Windows/Linux runtime и rendered Windows parity
не проверены для этого изменения.

## P1-01.2 — disk→archive через исходный CopyFrom/update (2026-09-30)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные владельцы: `CPP/7zip/UI/Agent/Agent.h`,
`AgentOut.cpp::CAgent::SetFiles/DoOperation`, его
`EnumerateArchiveItems/EnumerateArchiveItems2/CAgUpCallbackImp/CDirItemsCallback_AgentOut`,
`ArchiveFolderOut.cpp::CAgentFolder::CopyFrom` и ветка `AGENT_OP_Uni`,
`CPP/7zip/UI/FileManager/Panel.h`, `PanelCopy.cpp::CPanel::CopyFrom/CThreadUpdate`,
`CPP/7zip/UI/Common/UpdatePair.cpp::GetUpdatePairInfoList` и
`UpdateProduce.cpp::UpdateProduce`. Тела исходных SetFiles, CopyFrom,
DoOperation и helper обхода/выбора пар перенесены в зеркальные пути;
`UpdateProduce.cpp` скопирован без изменений в `third_party/7zip`.
`UpdatePair.cpp` скопирован в `src/CPP/7zip/UI/Common` с изменёнными
include путями и проектным форматированием; её алгоритм сравнения времени,
сортировки и формирования пар сохранён.

Необходимые адаптации: `CAgent::DoOperation` принимает только уже открытый
архив, поскольку создание нового архива принадлежит отдельной операции
Qt/`ArchiveBackend`; его branch создания `IOutArchive` через `CCodecs`
возвращает `E_NOTIMPL`. Запись alternate streams в переносимом `CDirItem`
недоступна и явно возвращает `E_NOTIMPL`. Source property plan остаётся в
теле, но текущий UI не задаёт `m_PropNames/m_PropValues`. Исходный
`CThreadUpdate` адаптирован к имеющемуся `CProgressThreadVirt` и
`IQtProgressServices`, сохранив вызов `IFolderOperations::CopyFrom`, пароль
верхнего архива, сообщения и progress/cancel. `qtarchivesourceupdate.cpp`
проверяет source callback plan, готовит файлы/папки во временном каталоге,
переносит время через `SetDirTime` и передаёт набор в
`ArchiveBackend::updateSourceAdd`. Backend отвергает опасные пути и ссылки,
использует официальный `7zz` с исходным Add action set
`p1q1r2x2y2z2w2`, отдельным output и паролем; native
`CommonUpdateOperation` публикует результат через WorkDir, повторно
открывает архив и для F6 только после успешной публикации удаляет
обработанные исходники. Это узкая граница вместо недоступного встроенного
writer, не новая реализация архива.

Qt F5/F6 из filesystem панели маршрутизирует выбор в текущую папку
противоположной открытой ZIP/7z панели, используя исходные относительные
имена, `CCopyDialog` и путь второй панели по умолчанию. После успеха
обновляются обе панели; отказ, вложенный архив и произвольный другой
archive destination не объявлены поддержанными. Дерево папок, Unicode,
пробелы, байты, время файла, защищённый верхний 7z, F6 после публикации,
отмена и небезопасный путь проверяются в `source_update_backend`; сквозной
F5/F6 с двумя панелями и refresh проверяется в `ui_operations`. Полный
`CApp::OnCopy` dispatch, archive→archive staging и остальные destination
варианты остаются P1-01.3; filesystem `FSFolderCopy` — P1-02,
ADS/links/metadata completeness — P2-05/P2-06. Исходные notices и лицензия
сохранены в `third_party/7zip/License.txt`.

Проверка: `./scripts/build.sh dev sevenzip_desktop source_update_backend_tests
ui_operations_tests`, адресные `source_update_backend` и `ui_operations`
прошли; полный unsandboxed `./scripts/check.sh` (форматирование, сборка,
все CTest) прошёл на macOS arm64, Qt 6.11.2, 2026-09-30.
Windows/Linux runtime и rendered Windows parity для этого пакета не проверены.

## P1-01.3 — archive→archive и исходная маршрутизация Copy/Move (принято, 2026-10-01)

Точная база: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
Исходные тела и зависимости: `CPP/7zip/UI/FileManager/App.cpp::CApp::OnCopy`
и `CPanelCallbackImp::OnCopy`,
`PanelItems.cpp::CPanel::GetItemName_for_Copy`, уже перенесённые
`PanelCopy.cpp::CPanel::CopyTo/CopyFrom`, `Agent/ArchiveFolder.cpp::CopyTo` и
`Agent/ArchiveFolderOut.cpp::CopyFrom`. Функциональное ядро `OnCopy` сохранено
в зеркальном `App.cpp`: проверка источника, OperSmart/focused selection,
исходный `CCopyDialog` с summary/history, `CorrectFsPath`, выбор второй
панели и условий временного переноса. `CopyArchiveToArchive` сохраняет исходный
`CTempDir` → `CPanel::CopyTo` → имена `GetItemName_for_Copy` →
`CPanel::CopyFrom`. `PanelItems.cpp` использует исходное свойство `kpidOutName`
и коррекцию имени. Notices и лицензия остаются в зеркальных файлах и
`third_party/7zip/License.txt`; проектное `clang-format` учитывается отдельно.
`CopyDialog.cpp/.h` собираются один раз в `sevenzip_panel`, поскольку исходный
`CApp::OnCopy` стал непосредственным потребителем диалога; отдельные Qt-цели
связываются с этой библиотекой.

Qt-адаптация: `IQtAppPresentation::DefaultCopyDestination/ExecuteCopy`
связывает исходную маршрутизацию с панелями и текущими асинхронными
filesystem/ArchiveBackend операциями. Временная папка и публикация целевого
архива принадлежат исходным `CopyTo/CopyFrom` и транзакционному backend.
F6 удаляет источник только после успешной публикации; перед удалением Qt
сохраняет байты целевого архива и восстанавливает их при отказе/отмене.
Обновление обеих панелей, сохранение фокуса и сообщения остаются узкой Qt
границей. Для двух одновременно открытых архивов backend хранит отдельный
проверенный снимок списка (путь, размер, времена, entries, encryption),
восстанавливает нужный контекст перед Test/Extract/мутацией и сбрасывает
снимок после записи. Шифрованная исходная extraction callback вызывает
`testSourceArchive`, поскольку уже владеет исходными метаданными независимо
от последнего отображённого списка.

Это частичный перенос `OnCopy`: исходная ветка Shift+F5/F6 `copyToSame`,
переименование одной цели, создание произвольного filesystem destination и
полные `FSFolderCopy.cpp` copy/move branches пока не подключены; исходный
`PanelKey.cpp` вызывает `OnCopy(..., shift)` и этот consumer нельзя считать
неприменимым. Ветка зафиксирована как обязательный P1-01.4 после P1-02;
parent P1-01 до неё не принимается. Windows-specific alternate streams, links и
metadata остаются P2-05/P2-06. Windows/Linux runtime и rendered Windows
reference не проверены. Реальные ZIP/7z UI фикстуры проверяют F5/F6 между
двумя панелями, байты, отмену с точным откатом, зашифрованный источник,
невозможность записи из вложенного архива и обновление панелей. Backend
тестирует Test/Extract/Add по первому архиву после listing второго и отказ
для устаревшего снимка; фиктивные helper-тесты используют существующий
временный файл. Проверены `archive_backend`, `source_extract_backend`,
`source_update_backend`, `archive_progress`, `extraction_options`,
`panel_folder`, `copy_dialog`, прямой набор UI 10/10 и baseline verifier
193 exact/136 adapted/3 local. Полный unsandboxed `./scripts/check.sh`
прошёл 142/142 CTest на macOS arm64/Qt 6.11.2, 2026-10-01.

## P1-02.1 — cross-volume F6 staged fallback (принято, 2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp::CopyFolder/CopyFile_Ask`,
`CCopyStateIO::MyCopyFile`, `CCopyState::CallProgress` и исходная ветка
`CopyFolder` «MoveFile_Sys, затем recursive copy/delete при отказе».
Эти тела были частично перенесены ранее в зеркальный `FSFolderCopy.cpp`:
recursive copy и чтение/запись уже используются `FileTransferOperation`.
В этом результате добавлена необходимая cross-volume ветка в Qt transaction
adapter, а не заявлен перенос ещё отсутствующих `CFSFolder::CopyTo` и
`CCopyState::MoveFile_Sys`; они входят в P1-02.2.

Причина адаптации: исходный MoveMode удаляет файлы при копировании, а текущий
Qt worker публикует весь набор транзакционно и должен откатить его при ошибке
до удаления. Поэтому F6 сначала пробует rename в stage; если файловая система
отказывает (включая границу томов), тот же source `CopyFolder`/
`CopyFile_Ask` копирует в stage без удаления. После публикации всех целей
worker удаляет скопированные источники. Узкий `FileSystemAdapter` позволяет
детерминированно вызвать отказ rename и отказ удаления без второго диска.
До фазы удаления отмена и ошибка откатывают цели и сохраняют исходники.
Если удаление уже началось и завершилось частично, worker оставляет
опубликованные байты, сообщает неуспех и частичный результат; обещание
полной атомарности было бы ложным. Исходные Windows MoveFileWithProgress,
AskWrite per-entry callback, прямое имя цели и recursive merge не включены
и остаются P1-02.2. Ссылки и полные метаданные — P2-05/P2-06.

Тесты forced fallback с Unicode/пробелами, деревом, занятой целью,
отменой и частичным удалением проходят в `file_transfer`; прямые disk-panel
F5/F6 UI consumers проходят. Полный unsandboxed `./scripts/check.sh`
прошёл 142/142 CTest на macOS arm64/Qt 6.11.2, 2026-10-01.
Windows/Linux runtime и реальный второй том не проверены.

## P1-02.2 — full FSFolderCopy callback and directory merge (принято, 2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp::{CCopyState,CopyFile_Ask,
CopyFolder,CFSFolder::CopyTo,CopyFileSystemItems}` и `FSFolder.h`;
consumers `PanelCopy.cpp::CPanelCopyThread::ProcessVirt`,
`ExtractCallback.cpp::CExtractCallbackImp::AskWrite`. Эти исходные symbol,
обход/подсчёт, AskWrite→copy/move→progress, рекурсия и direct-path ветка
остаются в зеркальном модуле. `CFSFolder::CopyTo` перенесён из `FSFolder.cpp`
stub в `FSFolderCopy.cpp`; этот файл теперь единожды компилируется в
`sevenzip_filesystem`. `CopyFrom/CopyFromFile` остаются оригинальными
`E_NOTIMPL`.

Адаптации: `CFSFolder` собирает полный путь как `_path + GetRelPath(fi)`
вместо ещё не восстановленного исходного `GetFullPath`. Portable
`CFsFolderStat` принимает путь в конструкторе. Windows `CopyFile_Sys`
использует исходный `CopyFileExW` callback; POSIX не имеет его и использует
перенесённый `CCopyStateIO::MyCopyFile`. `MoveFile_Sys` делегирует в
адаптированный source `MyMoveFile_with_Progress`; для каталога Qt rename
возвращает управление исходной recursive ветке при отказе, поскольку POSIX
FileDir EXDEV fallback копирует только обычные файлы. POSIX `CEnumerator`
заполняет `CFileInfo` через `Fill_FileInfo`; решение AskWrite для stage
передаёт узкий `CFileTransferCopyCallback`, без widget в source module.
Исходный `CExtractCallbackImp::AskWrite` остаётся COM consumer для
`CPanel::CopyTo`. MainWindow F5/F6 сохраняет транзакционный Qt worker:
для занятого каталога он копирует прежнее дерево в stage, применяет тот же
`CopyFolder` с per-file AskWrite и публикует результат. F6 удаляет только
записанные исходные файлы; пропущенные остаются. Ошибка/отмена до удаления
откатывает stage, а отказ удаления после публикации сохраняет целевые байты
и сообщает частичный результат.

Alternate streams и ссылки явно отклоняются до переноса (P2-05/P2-06);
metadata и Windows super-path ветки требуют отдельных platform проверок.
Прямое имя цели проверено через native `CFSFolder::CopyTo`, но Shift+F5/F6
Qt consumer относится к P1-01.4. Проверены `fs_folder_copy`, `fs_folder`,
`file_transfer`, прямой two-panel filesystem UI consumer и normal/compact
`main_shell`/`panel_shell` JSON; полный unsandboxed `check.sh` прошёл 143/143
CTest на macOS arm64/Qt 6.11.2, 2026-10-01. Реальный второй том,
Windows/Linux runtime и rendered Windows reference локально не проверены.

### P1-18 implementation — native RAR memory consumers

Baseline: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`https://github.com/ip7z/7zip.git`. Existing adapted
`CPP/7zip/Archive/Rar/Rar5Handler.cpp` now retains the complete original handler,
including header parsing, Open/Extract, dictionary/memory request control flow,
properties and BLAKE2sp/arc registration. Changes are canonical include paths,
source notice and project formatting. It remains one authoritative module in
the forced hash/arc registration object; a second vendored handler is not built.
Direct baseline comparison after excluding includes/comments and normalizing
C++ backslash-newline splices found all 16,536 functional tokens identical.
Formatting changed macro line continuations, not their logical bodies.

Exact vendored dependencies are recorded in `docs/upstream-baseline.json`:
Archive/Common/FindSignature, Archive/HandlerCont, Rar5Handler.h/RarHeader/RarVol
and StdAfx headers, Compress/HuffmanDecoder and Rar5Decoder, Crypto/HmacSha256
and Rar5Aes. Existing source stream, AES and ZIP/common dependencies are reused.
`CPP/7zip/Compress/RarCodecsRegister.cpp` copies the original RAR5 factory and
registration macros; legacy RAR1/2/3 registrations and decoder dependencies are
omitted and remain supplied by the official codec module. The native RAR5
registration object is forced into consumers so original CreateCoder finds the
copied decoder before external codecs. `cmake/native_rar.cmake` groups this
dependency closure without reorganizing original source modules; common/RAR/ZIP
static archive dependencies are linked through CMake.
The first full gate exposed LoadCodecs filesystem symbols missing from
foundation-only consumers. Existing FileDir/FileName/FileFind/FileIO/TimeUtils
and FileDir's Qt owned-temp identity/registry dependencies now have their sole
build owner in sevenzip_common (QtCore only), alongside original FileStreams.
No module is copied again or rewritten; filesystem/application targets consume
that foundation. This closes the complete handler/hash registration dependency
graph for every consumer, not just archive UI targets.

Verified module metadata retains ownership of format capabilities; only Rar5
reader construction is replaced with the source factory. Native reader format
indices are bounds checked. QtArchiveFileResource validates every source item
index/path against the file-only listing before exposing native extraction.
Mismatch refuses extraction rather than silently routing memory decisions to
the console. ArchiveBackend calls original IInArchive::Extract; original Agent,
FM CExtractCallbackImp::RequestMemoryUse and CMemDialog own decisions. Native
progress uses cooperative source synchronization without requiring a child
process. Batch opening applies original SetProperties after Open and before
Extract because its file-only adapter has already opened the native handler;
RAR memx/crc/mt extraction properties retain original interpretation.

Deterministic stored RAR5/RAR7 fixtures declare a 2 MiB dictionary and force the
public handler memx limit to 1 MiB. The actual handler requests the original
required/allowed values and flags through the GUI callback: no synthetic memory
invocation or console text parsing. A pinned libarchive compressed fixture
checks the copied decoder against known payload size/CRC; its provenance and
distribution notice are under `tests/fixtures/rar/`. Installed official 7zz
26.03 refuses that compressed fixture with Unsupported Method; independent
libarchive extraction is used where available. These checks do not establish
RAR7 compressed interoperability, Windows/Linux runtime or rendered parity.
Encrypted/solid and RAR multivolume GUI workflows are not established by these
small fixtures. Original handler bodies are retained, but the file-only batch
adapter still refuses non-.001 volume sets; the source module's full scope is
not evidence of full workflow/platform coverage.
P1-18 acceptance is recorded below; the additional unverified workflows above
remain explicit limits, not evidence supplied by these fixtures.
MainWindow root/selected Extract and Test also use the native reader for all
overwrite modes. Original source overwrite enums remain authoritative; local
RejectExisting validation maps to source Skip against a newly appearing file.
Unavailable index contracts refuse commands explicitly. The existing broad
mainwindow_archivecommands glue is kept focused on this routing change; its
general extraction/state component split is deferred rather than reorganizing
unrelated commands in the source-port iteration.

#### P1-18 acceptance (2026-10-02)

`native_rar_memory` passes35 Qt cases (28 handler Allow/Skip/Cancel/CRC rows,
four batch remember/save/reuse/cancel cases, compressed decoder and setup).
`native_rar_ui` passes15 Qt cases (12 actual root/selected × six overwrite
mode/Test rows, traversal refusal and setup). Exact source requests carry
2 MiB required/1 MiB allowed and original flags through source GUI callbacks.
Skip preserves output, cancellation returns E_ABORT, original archives are
unchanged; source IsOK separately reports CRC/Skip errors even when HRESULT
is S_OK. Missing index contracts refuse actual source extraction with no output.
Independent libarchive bytes/known CRC establish compressed RAR5 decoding;
installed7zz still cannot supply that comparator. Actual memory dialog Extract/
Test reports at700×460/560×460 were inspected: all visible controls fit,
labels fit, grouping/order and single-archive hidden remember follow source.

Focused direct consumer gate8/8 PASS6.96s; after foundation dependency correction,
foundation/RAR4/4 PASS1.30s and final RAR2/2 PASS1.25s. Baseline verifier367exact/
164adapted/3local PASS. Final unsandboxed `./scripts/check.sh` passes formatting,
all-target build and193/193 CTest79.91s, macOS arm64 Qt6.11.2. First full gate's
foundation linker failure was fixed before this acceptance. Windows/Linux runtime
and rendered Windows reference remain unverified. Whole iteration local commit
subject: `Port source RAR decoder memory requests and GUI consumers (P1-18)`.
No helper commit, push, publication or upstream change.

### P2-02 acceptance — source SFX and external modules (2026-10-03)

Whole P2-02 is accepted. The investigation below records copied CompressDialog
method/name/control bodies, Update PrepareSfxModule/WriteSfxModule and LoadCodecs
directory/ABI/SetCodecs flow, narrow Qt/7zz/platform adapters and omitted obsolete
FilePlugins/RegistryPlugins/ProgramLocation branches. All borrowed modules use
the reproducible official repository baseline
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Five exact dependencies are added:
Compress CodecExports.cpp/DllExportsCompress.cpp/Codec.def, Windows COM.h and
7zip/Common/UniqBlocks.cpp. Verifier372 exact/164 adapted/3 local passes.

Official native SFXCon/SFXWin, Console/Format7zF and Windows/Linux Alone2 runtimes
are actually compiled in isolated baseline snapshots. Both Windows producer
scripts reject unowned artifacts and junction outputs without changing sentinels.
Actual Add/SFX single/split/AES/delete-after-publication, native extraction,
engine/volume pause, second-root/prepublication cancellation and module refusals
pass. External codec and global-only source-handler facade support real engine
Create/List/Test/Extract/native leases, another catalog's release, nested assets,
missing-main Formats, ABI warnings/mismatch/cancel and actual junction cycles.

Final checks: unsandboxed macOS arm64 Qt6.11.2 check.sh formatting/all-target
build/197 CTest PASS107.31s; Linux ARM64 Qt6.4.2/GCC13.3 nine focused suites
PASS2.79s with zero Qt skips; Windows11 ARM64 Qt6.8.3/MSVC14.44 clean build and
seven focused suites PASS13.76s. Windows external/SFX have zero skips; the
existing owned_temp linkedRootAndLinkedObjectAreRejected file-symlink fixture
remains skipped and belongs to platform P3 work. UTF-8 console compiler prefix
and actual Ninja header records are verified after the clean Windows build.
Current EN/RU SFX JSON normal/compact geometry is reviewed on all three OS.
This establishes structure/usability, not rendered 7zFM pixel parity.

One whole local iteration commit; no helper commits/push/publication/upstream
writes. Counts31/46,15 remain. P3 full platform acceptance, rendered Windows
reference and separate mandatory Finder P3-03.1 remain open. Next P2-03.
Historical pending/failure statements below are superseded by this acceptance.

### P2-02 investigation — SFX and external modules (historical, 2026-10-02)

Baseline `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`, repository
`https://github.com/ip7z/7zip.git`. FM FilePlugins.cpp uses its internal Agent
folder manager; external CPluginLibrary branches are commented out.
RegistryPlugins.cpp is entirely commented-out legacy discovery. ProgramLocation
contains no active functional implementation. Do not revive these as an invented
current plugin workflow. Actual external archive/codec discovery and compatibility
are original Common/LoadCodecs.cpp::Load/LoadDllsFromFolder/LoadDll/IsSupportedDll.
The immutable catalog now loads original Codecs/Formats directories when the
dynamic engine's source Libs output confirms the deployment. Standalone7zz lacks
Z7_EXTERNAL_CODECS and cannot supply this capability. The official Console7z and
Format7zF module are built unchanged from the same pinned snapshot by the new
runtime builder; Debug/deployed-engine location and explicit path selection are
local runtime glue. No compression or archive handler is recreated.

LoadCodecs.cpp::Load retains its main/registry/global-SetCodecs source branches;
its two folder calls and NeedSetLibCodecs decision are factored unchanged into
LoadExternalModules(baseFolder). The Qt catalog supplies the engine deployment
folder instead of its own executable folder. SetLibCodecs factors the original
Load SetCodecs loop without changing its body. A confirmed dynamic deployment
owns private copies of its engine, main DLL and Codecs/Formats trees in
ArchiveCodecImage; the catalog retains those files until its COM owner unloads.
Native readers and child engine operations use that same private deployment.
This isolates the DLL-global CExternalCodecs context between retained catalogs;
OpenArchive::PrepareToOpen retains the original per-instance fallback. Original
paths are restored in source diagnostics and deployment comparisons. This is
local runtime/lifetime glue, not a replacement loader or archive algorithm.
Discovery and archive jobs exclude one another so publication cannot change the
engine or remove its image during an operation. Local copied ZIP/RAR factory
hooks explicitly retain NeedSetLibCodecs after the source g_NumArcs decision,
because those replacement factories are not registered in g_Arcs. A main-only
dynamic deployment's copied ZIP handler reads an actual BZip2 ZIP via this context.
The current macOS fixtures verify real external codec decoding and retained
catalog isolation, plus a deployment with no main DLL and an unchanged official
module in Formats: native exact-byte decoding and engine Create/List/Test/Extract
pass while the missing-main warning remains visible. The test-only external
format facade delegates all IInArchive work to the unchanged official module,
reports a distinct read-only type and withholds ISetCompressCodecsInfo. Actual
decoding of the external F001 Copy method therefore uses the original DLL-global
SetCodecs; it retains exact bytes after a second confirmed catalog is created
and released. Forced-type backend Open/Test/Extract also passes. The fixture
borrows an already-loaded core module handle and owns no extra global loader
lease. This is synthetic test metadata/protocol glue, not a new archive algorithm.
The facade additionally requires a nested resource at object creation. Its
retained image opens another handler after that resource is removed from the
original deployment; native/engine tests pass. Folder copying retains assets,
while original LoadCodecs direct-module enumeration remains unchanged. Cyclic
directory links refuse preparation; external/system dependencies are still
resolved by the platform loader, not relocated by this adapter. Windows/Linux
image isolation still needs verification within P2-02.

ArchiveCodecProbe only parses the source Console Libs/version transport (including
CRLF); original LoadDll/IsSupportedDll own module metadata and ABI compatibility.
Implicit main-module selection transports the engine's Libs resolution, including
the Codecs/Formats base folder when the main module is missing; explicit module
requests retain mismatch refusal. The catalog refuses engine/native deployment
mismatch. Original nonfatal load
warnings retain working modules and reach startup status plus copied About's
GetCodecsErrorMessage/error-box branch. Qt substitutes the parent, modal service
and retained catalog for global g_CodecsObj. GetCodecsErrorMessage is const-qualified
for that read-only lease, with its functional body unchanged. About's local app
caption now describes the official engine rather than a fixed standalone client.

The focused sfx_lifecycle consumer accepts the actual Add form, passes its
public target/sources/options to ArchiveBackend, and verifies native extraction
after single/split creation and source deletion. Cancellation at the second
root and before volume publication preserves both sources and leaves no output;
engine and volume-worker pause/resume retain correct output. Two separate source
folders are required: same-folder files intentionally form one engine stage.
Small deterministic random inputs keep the real compression process alive for
pause testing; Store can finish before Qt delivers started. These tests exercise
the existing copied lifecycle and platform pause adapters. Latest affected four
suites pass2.63s on macOS arm64.

Advanced method override follows the original UpdateGUI.cpp::ShowDialog
SplitOptionsToStrings/IsThereMethodOverride/SetOutProperties/ParseAndAddPropertires
order at the recorded baseline. The copied GetCompressionProperties retains
this order. SFX UI filters the selected method, while explicit advanced properties
may override it as upstream permits; no new blanket restriction is introduced.
Supported defaults and their native SFX decoding are verified separately.

Linux ARM64 verification builds unchanged Console/Format7zF/SFXCon/Alone2 at the
recorded revision in an isolated guest snapshot. SFX SHA-256 is
de371b3bb5ff832d6a3196f5171638f978fd62f98a6ff38136c0e371cbf996e4.
Qt6.4.2/GCC13.3.0 on Linux6.8.0-142 compiles the affected six targets; all six
CTest suites pass1.52s with zero Qt skips. Private images, global-only handler,
nested assets, missing-main and source SFX workflows therefore have direct Linux
evidence. Four EN/RU SFX JSON reports expose ordered controls at normal/compact
sizes, with checkbox width exceeding size hints. No rendered Windows parity claim.

The build exposed a narrow FSFolderCopy.cpp adapter error: Qt quint64 and upstream
UInt64 have different underlying integer types on Linux. CCopyState::CallProgress
and its Qt IO progress lambda now pass a local UInt64 value to the original
IProgress::SetCompleted, preserving the callback contract and copy flow. Existing
filesystem/copy regressions pass on macOS2/2 and Linux; no source algorithm changes.

Windows fixtures now select the native GUI module and .exe naming. SfxWin.cpp
WinMain2 always extracts, accepts -y/-o/-p and sets eo.TestMode=false. Accordingly
the fixtures use a separate official engine for Test and omit the console x verb
when executing GUI SFX; split Test addresses the payload, not the bare module.
Windows native consumer execution remains unverified, not N/A.

The Windows Qt build exposed explicit dependency gaps: processpause.h,
processpriority.h and qtarchivevolumeoutput.h now include original MyTypes.h
for UInt32/UInt64 and MY_UNCOPYABLE rather than relying on Unix MyWindows.h.
QtOwnedTempIdentity uses a local CFileBase-derived identity handle to expose
the protected original Create service with its existing zero-access,
read/write/delete sharing and reparse-point flags. CInFile::OpenReparse cannot
replace it because its read-only sharing would reject concurrently held writable
temporary files. No original FileIO body changes. FSFolderCopy's local move
progress adapter is named QtMoveFileProgress to avoid the existing Windows
CopyProgressRoutine callback name; callback and copy/move flow are unchanged.
External fixture paths select .exe/.dll on Windows and original Unix names
elsewhere. Host owned-temp/pause/priority and filesystem regressions pass;
these are portability fixes, not Windows runtime acceptance.

Further Windows compile fixes retain the original constants when Windows already
defines CBN_SELCHANGE/IDCLOSE, keeping local declarations only on other hosts.
QtPanelFolder explicitly casts an already range-checked property index to
unsigned for CObjectVector's original overloads: Windows UInt32 is unsigned long.
PanelOperations keeps original CComInitializer; its missing CPP/Windows/COM.h
dependency is copied byte-for-byte from the recorded revision into third_party,
using the existing Windows/COM.h include-root convention.
Both Windows producer scripts actually refuse unowned artifact and junction
output-directory fixtures before compilation, preserving sentinel SHA-256 and
the junction target's single-file inventory. Positive compilation and these
failure paths do not establish interactive Windows SFX or pixel parity.

Windows BrowseDialog::CorrectFsPath qualifies original NFind::CFileInfo
(Unix excludes this branch). NetFolder's retained Windows icon calls now reach
SysIconUtils.cpp's copied Shell_GetFileInfo_SysIconIndex_for_Path Unicode branch
and original HRESULT wrapper. SHGetFileInfoW, default attributes, flags,
icon-index output and GetLastError_noZero_HRESULT control flow are retained;
obsolete ANSI/NT dynamic dispatch and other CSIDL/image-list helpers are omitted.
The Qt icon cache remains its separate presentation service. Only Windows links
shell32. The original symbols/source paths share the recorded full baseline
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17; no second icon algorithm is introduced.

Windows fixture linking exposed invalid symbols `=` and `!` in CMake's generated
export-all definition. The codec fixture now uses unchanged original
CPP/7zip/Compress/Codec.def from the same baseline. The two local facade/ABI
fixtures have explicit test-only .def lists of their public module functions;
internal COM/Qt symbols are not part of the plugin ABI. Unix exports and original
CodecExports/DllExportsCompress bodies are unchanged. Baseline verification now
passes371 exact/164 adapted/3 local paths; four exact dependency files are staged
only for inventory. Whole P2-02 gate/acceptance and commit remain outstanding.
QtLocationFolder's archive metadata adapter inserts raw-property entries with
explicit PROPID keys instead of assigning QMap<quint32> to QMap<PROPID>:
Windows typedefs use distinct C++ types of the same32-bit width. Values and
property ordering are retained; existing raw-property consumers remain covered.

Windows linking then exposed dependencies retained by original extraction/
enumeration: Convert_WinPath_to_WslLinuxPath, FillLinkData_WslLink/WinLink,
NIO GetReparseData/SetReparseData and CUniqBlocks::AddUniq. The FileLink.cpp
Windows bodies were already copied but gated by a temporary local capability
macro; that extra guard is removed now that their FileIO/FileDir services exist.
Their original _WIN32/UNDER_CE guards and functional bodies are unchanged.
UniqBlocks.cpp is copied exactly from the recorded baseline beside its existing
header and built by sevenzip_filesystem. Broad ADS/link/security workflow
acceptance remains P2/P3; supplying these dependencies is not that acceptance.
Baseline verification after UniqBlocks.cpp passes372 exact/164 adapted/3 local
paths. Five exact dependencies are staged for inventory, without a helper commit.

MSVC dumpbin identifies a concrete ReOpen ABI mismatch: its implementation used
class IFolderFolder while the caller used the original struct IFolderFolder.
QtArchiveFileResource and QtArchiveReopen forward declarations now match the
original struct COM-interface tags, including the open callback. This changes
neither interface IDs/vtables nor archive behavior; it makes Windows decorated
function names agree across translation units. Unix mangling ignored this tag
difference, so host success alone did not expose the bug.

Repeated linking after the header fix exposed a separate build prerequisite:
CMake had recorded a mojibake Russian msvc_deps_prefix, so Ninja ignored MSVC
include dependencies and reused an older object. Its timestamp predates the
changed header; dumpbin confirmed the old class-tag symbol. The isolated Windows
guest cache/object directory is recreated with process-local VSLANG=1033.
build.sh now defaults that diagnostic language on Windows when unset, preserving
explicit caller settings. Compiler diagnostics only; product localization and
global VM environment are unchanged. Existing malformed caches require a clean
configuration; this is documented rather than silently deleting user outputs.
The installed ARM64 compiler contains only1049 resources, so VSLANG=1033 alone
does not change its Russian output. A separate minimal CMake/Ninja fixture with
UTF-8 console (`chcp65001`) actually recompiles after a header changes VALUE7→8;
both exit values confirm dependency delivery. build.sh now uses that console
encoding as well, restoring the caller's previous code page on exit. Explicit
language settings remain respected; no language pack/toolchain installation.

The first native Windows focused run compiles all seven targets and passes
codec_catalog, sfx_lifecycle, owned_temp and process_pause; SFX creation's20
non-layout cases pass. Failures are not accepted: priority test transport expected
LF instead of native CRLF, external facade used a non-native full DLL path, and
geometry scroll ran before Windows show/layout events settled. Test adapters now
normalize CRLF, use native path separators and settle events before scrolling.
Original LoadDll intentionally ignores ERROR_BAD_EXE_FORMAT from Windows
LOAD_LIBRARY_AS_DATAFILE; invalid-file expectations now follow that branch,
while Unix retains its warning. The VM's facade discovery gets a15s test wait
(measured7.15s); real backend timeouts are unchanged. The Windows resource-cycle
fixture uses original FillLinkData_WinLink/SetReparseData/GetReparseData to create
and verify a disposable junction rather than skipping directory-link checks.
The fixture removes only the junction and retains its target. Windows JSON
reports are written before geometry assertions for failure diagnosis; no screenshots.

Follow-up Windows focused runs reach6/7 suites: creation24/24 (including four
EN/RU geometry rows), lifecycle, catalog, owned-temp, pause and priority pass.
Two external-module failures remain at that checkpoint. Diagnostic module lists
show native loading the facade while the official engine omits it. The facade
is a local fixture, not a copied archive implementation: Qt-only path/marker
helpers are replaced with dependency-free C++17 filesystem/stream glue so
original Console's C/DllSecur.c search policy can load it without Qt DLLs.
All format/codec operations still delegate the exact original module, with no
extra loader lease. Deployment mismatches now report both module lists and the
original codec errors, making this refusal actionable.

The actual Windows junction fixture exposes a snapshot cycle missed by pathname
comparison. ArchiveCodecImage now identifies Windows directory ancestors using
the original CFileBase::Create/GetFileInformation service, following junctions
with FILE_FLAG_BACKUP_SEMANTICS and read/write/delete sharing. Volume/file IDs
identify the target across aliases; Unix retains canonical-path ancestry.
This is local immutable-runtime/platform glue, not a new loader algorithm.
The retained source loader still enumerates its original directories and ABI.

Two unchanged test dependencies are vendored at the exact baseline:
CPP/7zip/Compress/CodecExports.cpp and DllExportsCompress.cpp. The test fixture
registers the already borrowed CopyCoder under a synthetic ID/name, preserving
its algorithm and original module exports. Actual Console Create/List/Test/Extract
and native7z handler extraction prove use of this external codec; a main-only
catalog reports UnsupportedMethod for the same archive. A retained native handler
survives replacement of another catalog. Unicode/space plugin path, ABI-incompatible
and non-library warnings, mismatched deployment, discovery cancel and default
engine-path selection/refusal are covered. Tests use disposable files only.
Runtime-builder unowned-artifact refusal preserves exact foreign bytes.
Baseline verifier:369 exact/164 adapted/3 local; the two exact sources are staged
only for its tracked-file inventory, not a helper commit.

Official `CPP/7zip/Bundles/SFXCon/makefile.gcc` builds a native macOS arm64 console
SFX in an isolated snapshot, without changing upstream. New build-sfx.sh archives
the exact recorded revision, uses its unchanged makefiles/engine, handles the
host SDK include-path workaround and produces module/license/provenance JSON
with SHA-256. Windows GUI SFX remains a separate official runtime dependency.
Both manual snapshot and script produced SHA-256
ad64e6ffb725efdab3a2d3072ebffb6b20f74af10a34ed4ebb8c782cdc32b99b, version26.03.
An official7zz26.03 disposable Unicode/space fixture made from this module passes
native self-Test and Extract. 7zz leaves the result non-executable: a Unix
publication permission adapter is required before offering native SFX execution.
This check used only temporary generated data; it does not accept an app consumer.

CompressDialog.cpp retains original IsMethodSupportedBySfx and SFX method filtering
from SetMethod, CheckSFXControlsEnable and OnButtonSFX. Qt substitutes editable
path text and a returned enable flag for Win32 control access. Windows keeps
`.exe`; Unix adapts GUI kExeExt to the empty extension already used by original
Common/Update.cpp, for the native console module rather than a Windows target.
Source names, method IDs and control flow are retained.

Current WIP copies CUpdateErrorInfo and its three error setters from Update.h/.cpp,
the UpdateArchive module-presence/path-fallback branch into PrepareSfxModule,
and Compress's separate-volume SFX stream/copy/close body into WriteSfxModule.
Runtime-directory injection replaces GetModuleDirPrefix; Qt staging replaces the
final output path, while CInFileStream/COutFileStream/CopyStream remain original.
The single-output prefix and rsfx property are executed by official7zz, not a new
archive writer. The volume consumer compresses the payload without an SFX prefix,
uses the original CMultiOutStream, and stages/publishes the module separately.
WriteSfx UI callback is represented by worker progress; module copy has no separate
progress callback, as original CopyStream uses NULL. Other Compress/UpdateArchive
branches are omitted or delegated to the existing engine adapter.

ArchiveSfxModule is local runtime/integration glue: reads the build's recorded
source revision/host/version and content digest with original C/Sha256; the catalog
uses original CArchiveLink/Open3 and MachO/ELF/PE handlers' kpidCpu to check native
format and architecture. The sidecar provides reproducible provenance/integrity,
not a signed authenticity guarantee. New build-sfx.ps1 snapshots the same pinned
revision and invokes unchanged SFXWin/makefile with MSVC nmake PLATFORM; version
comes from original 7zVersion.h, manifest/license from that exact snapshot. It
guards unowned/reparse artifacts and avoids PowerShell 5.1 binary pipelines.
Windows Read requires this builder's producer. Parser validation succeeds on
Windows11 ARM64 10.0.26300. The earlier missing-tools probe inspected PATH only:
installed VS2022/MSVC14.44.35207, SDK10.0.26100.0 and Git are available through
a process-local developer environment. Actual unchanged SFXWin compilation passes;
module SHA-256 c87deca002afe1616dd7ce24d60824f135f75c0c6e3a0b0ea74df75167103004.
The new build-codec-runtime.ps1 uses the same pinned snapshot and ownership guards
to compile unchanged Console/Format7zF/Alone2 Windows makefiles. Both clients
report26.03 arm64, with the dynamic client loading its prepared7z.dll. Native
Windows Qt consumers remain unverified. Unix publication adds owner execution
permission before atomic rename. Missing/incompatible modules are refused.

An actual official7zz26.03 Unix `-sfx -v` fixture creates `self.001`, but unchanged
SFX OpenArchive.cpp::OpenStreamOrFile looks for `self.7z.001`; self-Test fails until
the volume is renamed. Our Unix integration explicitly publishes `<self>.7z.NNN`
to match that original reader, while preserving the native module unchanged.
Windows uses the original `.exe` to `.7z.NNN` naming intent. This necessary Unix
deviation is not an upstream change.

The Qt form now places source IDX_COMPRESS_SFX before Shared/Delete in the original
Options group; native catalog/module capabilities gate it, and source method/name
branches drive toggles and format transitions. Existing-target updates keep SFX
disabled. Focused macOS arm64 checks cover single/multi-root, ordinary/split output,
AES/header encryption, native generated self-Test/Extract, missing module, occupied
output, cancellation, unchanged source/module, and normal/compact EN/RU geometry.
Remaining external-module consumers/protocols, Windows/Linux runtime and full gate remain open.
Focused sfx_options/sfx_creation/compression/archive_volume_creation4/4PASS2.91s;
after the localized SFX→ZIP method-list restoration, affected sfx_creation/
compression2/2PASS1.40s and git diff --check PASS, macOS arm64 Qt6.11.2.
No P2-02 acceptance, full gate or helper commit has been claimed.

External/runtime/About WIP checks2026-10-02 macOS arm64 Qt6.11.2:
sevenzip_desktop and affected targets build PASS; final external_codecs/
codec_catalog/help_source/codec_commands_ui/help_ui5/5PASS4.42s; baseline verifier
369/164/3 PASS; git diff --check PASS. Earlier SFX focused evidence above still
applies to its last checked consumers; a whole-component full gate remains due.
About appcaption fourEN/RU360/300 JSON reviewed. Standard codec-error QMessageBox
fits at actual400×121 for both600/400 resize requests; no two-size rendering
claim is made for that auto-fit dialog.

## P1-06.2 — source consumers томов (принято, 2026-10-01)

Та же точная база `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
Из `CPP/7zip/UI/FileManager/Panel.cpp::TestArchives` скопирована archive-folder
ветка: `Get_ItemIndices_OperSmart`, QueryInterface `IArchiveFolder`, исходные
`CCopyToOptions::{streamMode,showErrorMessages,testMode}`, `CopyTo` и error/abort
ветви. В `Panel.h` скопирован исходный трёхаргументный `CopyTo` overload,
который берёт пароль последнего `_parentFolders` и вызывает общий consumer.
Win32 presentation заменена существующим Qt service; архивная ветка не
переписана. Filesystem `::TestArchives(paths)` batch entry point и upstream
закомментированный старый `CThreadTest` сюда не перенесены: текущая команда
доступна в открытом архиве; filesystem entrypoint остаётся mandatory omitted
scope P3-05 для `Panel.cpp`, а не считается неприменимым.

MainWindow связывает production test command с этим source consumer; источник
без extract service сохраняет прежний transport fallback. Source `PanelCopy`,
Agent `ArchiveFolder/Agent`, `Common/Extract`, `GUI/ExtractGUI` и callbacks
переиспользуются. `QtArchiveFolderManager`/`QtArchiveBatchOpen` уже передают
`7zz` listing, embedded format и numbered VolumePaths в исходные владельцы:
внешний engine открывает Split handler, поэтому для extract `.001` нельзя
принудительно задавать inner `-t7z`. Native Common Extract loop исключает
следующие VolumePaths из повторного batch извлечения. Полные in-process
codec opening bodies `OpenArchive::Open/Open2` не подменяются claims этого
шага; их прежний omitted scope сохраняется в общем audit/P2-01/P3-05.

Новый integration тест создаёт тома через принятый source stream, затем открывает
их через `BindToPath`/`FileFolderPluginOpen`, проверяет исходный smart selection,
test callback и root Ask extraction. Пароль проверяется до overwrite GetStream;
missing/corrupt volumes показывают source errors. Cancel открытия, теста и byte
извлечения освобождает backend и возвращает commands. Существующие тома не
изменяются. Qt native picker получает `*.001` в локальном archive filter;
upstream Windows filesystem batch presentation остаётся отдельным scope.

Проверки: focused 9/9 6.14 с; unsandboxed `./scripts/check.sh` — formatting,
build и 148/148 CTest, 40.58 с, macOS arm64/Qt 6.11.2 2026-10-01.
Parent P1-06 принят; Windows/Linux runtime и rendered Windows reference
не проверены. Source creation Add normal/compact JSON принят в P1-06.1;
nonvisual test routing переиспользует прежний source progress, picker filter
проверен через native-service Qt fixture без заявления геометрии OS dialog.

## P1-06.1 — создание архивных томов (2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`CPP/7zip/Common/MultiOutStream.cpp` скопирован целиком в зеркальный `src/`
путь; изменены includes, provenance notice и clang-format. Header `.h`
сохранён побайтно в `third_party/7zip/`; baseline manifest фиксирует оба файла.
Сохранены `CMultiOutStream` numbering, переменные размеры, seek, ограничения
открытых файлов, `FinalFlush_and_CloseFiles` и cleanup. Переиспользуются исходные
`FileStreams`, `CopyCoder`, `FileIO`, `FileDir`, `FileFind` и `System` adapters.
`SplitUtils.cpp::{ParseVolumeSizes,GetNumberOfVolumes}` восстановлены с исходными
`UString/CRecordVector`, control flow и overflow checks; Qt остаётся только
consumer/preset presentation. Реализация одна, в `sevenzip_panel`.

`GUI/CompressDialog.cpp::OnOK` volume block перенесён с Trim/Clear/parser,
ошибкой размера и Yes/No/Cancel для последнего размера менее 100 KiB;
`CompressDialog.rc` задаёт combo/label, `AddVolumeItems` — исходные девять presets.
Qt `createarchivedialog_volumes.cpp` связывает этот результат с controls.
Исходник не сохраняет произвольную историю размеров между запусками; список
presets и пустое начальное значение сохранены. Сохранение профиля происходит
только после успешной проверки и подтверждения.

`Common/Update.cpp::Compress` задаёт исходную интеграцию `CMultiOutStream`.
Её encoder orchestration ещё не перенесена целиком: официальный внешний `7zz`
не изменяет многотомный архив, а Qt создание использует несколько source-root
стадий. Поэтому `ArchiveBackend` завершает один официальный временный архив,
затем `QtArchiveVolumeOutput` передаёт готовые bytes через исходный stream и
публикует numbered set. Это требует места для полного временного архива плюс
томов; encryption/format не реализуются заново. Worker поддерживает pause/cancel,
проверяет occupied targets и откатывает только свои опубликованные файлы.
Защищённый 7z на последующих стадиях использует пароль открытого архива вместо
повторного CLI new-password prompt. Изменение существующего набора томов
отклоняется; source open/list/test/extract acceptance остаётся P1-06.2, SFX — P2-02.

Проверки: focused 8/8 3.86 с; unsandboxed `./scripts/check.sh` — formatting,
build и 147/147 CTest, 41.04 с, macOS arm64/Qt 6.11.2 2026-10-01.
Baseline verify: 194 exact, 139 adapted, 3 local paths. Normal/compact JSON
проверены; Windows/Linux runtime и rendered Windows reference не проверены.

## P1-01.4 — Shift+F5/F6 и прямое имя назначения (принято, 2026-10-01)

База: [ip7z/7zip `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
`CPP/7zip/UI/FileManager/PanelKey.cpp::OnKeyDown` (Shift+F5/F6 →
`OnCopy(..., shift)`), `App.cpp::CApp::Copy` (focused index, default name,
`CorrectFsPath`, direct destination/parent creation), `PanelCopy.cpp::CopyTo`,
`FSFolderCopy.cpp::CFSFolder::CopyTo` и
`CPP/7zip/UI/Agent/ArchiveFolder.cpp::CAgentFolder::CopyTo`/
`Agent.cpp::CAgentFolder::Extract`. Исходные `App.cpp` и `PanelKey.cpp`
сохраняют зеркальные пути и symbols; изменены только Qt presentation/dispatch
части. Исходный `FSFolderCopy.cpp` и Agent bodies остаются единственными
реализациями в своих модулях.

`PanelKey` теперь передаёт Shift в source `OnCopy`, независимо от обычной
selection-gated доступности QAction: исходная ветка выбирает именно элемент
под фокусом. `App.cpp` сохраняет признак завершающего разделителя и создаёт
родительский каталог для одиночного прямого пути. В Qt transaction adapter
`FileTransferOperation` передаётся точный target вместо папки для одного
файла/каталога; staging, исходный `FSFolderCopy` AskWrite, backup/rollback и
F6 publication/delete остаются общими. Для обычного F5/F6 один новый путь
без завершающего разделителя тоже является прямым именем, а существующая
папка остаётся папкой назначения. Самокопирование и вложенная цель каталога
отклоняются до изменения данных.

Для archive source Shift направляет explicit focused index в исходный Agent
`CopyTo`; F6 удаляет этот index после успешного извлечения. Это не переименование
записи архива: исходный `CAgentFolder::Extract` нормализует destination как
каталог, поэтому абсолютный путь извлекает элемент в указанную папку. Если
source extract service недоступен, Shift показывает отказ без fallback на
текущий selection. Запись под новым именем внутри архива относится к иному
source update workflow, а не к этой ветке `CopyTo`.

Проверены `file_transfer_direct` (Unicode direct file, directory move,
occupied Yes/No, self/child refusal), `ui_copy_same` (Shift F5/F6 filesystem,
обычный F5 direct filename и parent creation, Shift F5/F6 с настоящим ZIP
и отличающимся выделением). Полный unsandboxed `./scripts/check.sh`
прошёл 145/145 CTest на macOS arm64/Qt 6.11.2, 2026-10-01.
Windows/Linux runtime и rendered Windows reference локально не проверены.
