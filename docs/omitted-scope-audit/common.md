# Omitted source scope review — common

[Индекс](../omitted-scope-audit.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

### Common ZipRegistry persistence body/header disposition, 2026-10-07

Source: https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/Common/ZipRegistry.cpp/.h`. Complete active bodies, declarations,
Registry services and Create/Extract/WorkDir/ContextMenu consumers were compared.

| Source scope | Disposition |
| --- | --- |
| Registry key and synchronization services | Explicit application INI path and QSettings replace HKCU/7-Zip/CKey. Source g_CS now serializes complete Load/Save operations, including profile-group replacement, using QMutex. DWORD conversion validates its full range before narrowing; Qt bool text is also accepted because it is the adapter's own serialized representation. |
| NExtract CInfo and password/memory-limit functions | Source mode bounds, forced flags, default-true SplitDest, default-false metadata/password BoolPairs, history and defined-only writes remain. Undefined Extract pairs preserve previous keys. The all-ones memory limit deletes its key. Legacy Qt history remains a one-way read fallback. |
| NCompression CInfo/CFormatOptions | Source Level=5 default/global field, per-format fields, reset rules, history, source traversal and metadata defaults remain. Undefined compression pairs and all-ones per-format DWORDs delete keys; empty profile strings delete keys. Only the owned options group is replaced. INI cannot represent an empty subkey; an entirely default profile has no stored group and FindRegistryFormat_Always supplies its source defaults on use. |
| CMemUse/ParseMemUse | Original parser, overflow checks, suffix/percent rules, clear/default and byte calculation remain; QString-to-wide conversion replaces the native string boundary. |
| NWorkDir/CContextMenuInfo | Original mode/path fallback, removable-only default, context BoolPair defaults, flags and Zone sentinel policies remain. Invalid stored booleans keep their respective source defaults. |
| Inactive branches and storage differences | Commented dictionary-chain, MemLimit_Save/Load and email-mask alternatives remain inactive at this baseline. Registry prefix/open/create, recursive history keys and write-if-changed optimization are storage services; INI groups/string lists replace them. Portable upstream MethodProps/CBoolPair, strings, vectors, ExtractMode and numeric conversion remain dependencies; no archive algorithm is introduced. |

This receipt covers ZipRegistry's persistence owner. Other Common modules and
CompressDialog event/header reconciliation retain their separate required scope.
Regression/platform evidence belongs to the
[settings audit](../upstream-port-audit.md#p3-05--source-settings-persistence-2026-10-07).

### Common owner and adaptation reconciliation, 2026-10-08

The complete pinned `CPP/7zip/UI/Common` inventory contains25 `.cpp` and30
headers. All55 have exactly one source/vendor owner; none is missing or duplicated.
Six implementation files and15 headers are byte-exact vendor dependencies.
The remaining19 implementations and15 headers were compared against the full
original files with formatting normalized for review. Changed active bodies,
header contracts and their direct consumers were read separately; unchanged
algorithm bodies are established by the complete comparison, not by path names.
ZipRegistry's full persistence disposition above and the earlier functional
component receipts remain authoritative for their changes.

Paths in this table are relative to `CPP/7zip/UI/Common`. Adapted owners are
under `src/`; byte-exact owners are under `third_party/7zip/`.

| Complete file family | Copied/adapted scope and caller/dependency disposition |
| --- | --- |
| ArchiveOpenCallback, DefaultName, PropIDUtils, SetProperties, UpdateAction, UpdateProduce `.cpp/.h`; ArchiveCommandLine.h, ArchiveName.h, DirItem.h, ExitCode.h, ExtractMode.h, Property.h, SortUtils.h, StdAfx.h, UpdatePair.h | All21 files are byte-exact. Original COM/password/volume callbacks, default-name rules, property conversion, action tables and update production remain the sole implementations. OpenArchive/Extract/Update/Agent and GUI callers reuse these types and bodies. StdAfx is header/compiler transport, not an omitted runtime module. |
| ArchiveName.cpp, Bench.cpp/.h, EnumDirItems.cpp/.h, ExtractingFilePath.cpp/.h, SortUtils.cpp, TempFiles.cpp/.h, UpdatePair.cpp | Only include owners, notices and formatting differ. Complete source naming/collision, benchmark, enumeration, path correction, sorting, cleanup and pair-comparison algorithms remain. Create/Panel/drag use CreateArchiveName; Extract callback uses Correct_FsPath/path parts; Update/Agent reuse directory items and pair production. Benchmark's platform services are dependencies of its unchanged algorithm, covered by its lifecycle receipt. |
| ArchiveCommandLine.cpp | Full parser/options body remains. The POSIX argv0/global module-prefix tail is replaced by the single shared Windows/DLL.cpp module-path service; Qt supplies the executable path because the desktop has no console argv0 initializer. Original Windows mapping/large-page/device branches remain selected by their platform definitions. GUI dispatcher uses the original parser; no second command parser is introduced. |
| CompressCall.cpp/.h, CompressCall2.cpp | Complete native process/map/event/quoting and in-process Add/Extract/Test/Hash workflow bodies were compared. Windows shell consumers retain the process implementation; owner HWND becomes null and settings path is explicit. Portable quoting has one owner. Qt shell/panel/drop consumers use typed services in CompressCall2 for catalog, GUI workers, errors and parent windows. Original TRY/return/display policy remains, including CompressFiles' S_OK after its handled exception; the service retains operation error state. Native waitFinish/process lifetime and Qt joined modal lifetime are distinct documented transports. |
| ArchiveExtractCallback.cpp/.h, IFileExtractCallback.h | Source item/path/overwrite/stream/metadata/link algorithms remain. Optional IQtExtractPathPolicy checks the already corrected final path before CreateFolders/output and checks link targets before link creation; it does not rewrite archive names or link algorithms. Complete modified GetExtractStream/CreateHardLink2/SetLink2 and CreateFolders dependency were reviewed. CQtExtractPathPolicy delegates filesystem containment to ArchiveBackend; source multi-archive and shell consumers install scoped policies. Zone helpers are also compiled for POSIX virtual-file callbacks, where a colon names a file rather than an NTFS stream. Original native security/ADS/reparse and POSIX link/mode branches remain. |
| Extract.cpp/.h, OpenArchive.cpp/.h | Full Extract loop and Open/Open2 service branches retain source errors, abort, volume deduplication, totals and nested-chain policy. Optional IQtArchiveOpenService supplies process-backed archives through ArchiveBackend. The retained catalog clears this hook before entering native Open3, avoiding recursion. SetDefaultName is the original OpenStream block shared with process projection; detection/signature/handler algorithms remain unchanged. Native ArchiveOpenCallback, FileStreams/LimitedStreams and source CArc/CArchiveLink types remain dependencies. |
| HashCalc.cpp/.h | Source hash parsing, digest comparison, extraction/test and manifest writer bodies remain. AddDefaultMethod/Extract/UpdateItems use a per-handler CExternalCodecs provider instead of a mutable global pointer; ISetCompressCodecsInfo loads the original provider interface. The catalog owns modules until handlers/streams release; its destructor breaks module SetCodecs links before unloading. No new checksum/parser implementation replaces the original. Source HashGUI and native Hash archive consumers retain the same engine providers. |
| LoadCodecs.cpp/.h | Original loading, metadata, format ordering, errors and factory bodies remain. LoadExternalModules and SetLibCodecs factor the original folder/NeedSetLibCodecs/SetCodecs phases for ArchiveCodecCatalog's immutable runtime snapshot. Load still runs them before Formats.Sort. The catalog checks deployment/version and keeps its image alive through COM release; CloseLibs clears provider links before unload. Source external-codec/export/static/SFX conditionals remain; retained old SDK/CE configurations are not claimed as executed desktop targets. |
| Update.cpp/.h, UpdateCallback.cpp/.h | Complete modified Compress/UpdateArchive and extracted SFX, maximum-MTime and post-success deletion bodies retain original ordering/checks, stream ownership and failure/abort policy. Qt volume output, AgentOut and QtArchiveDeleteInputs call these source bodies. The deletion overload validates its explicit callback/buffer boundary; it preserves source size/time/reparse checks, deepest-first directories and finish callback. Native Unix SFX retains the archive extension before .001 despite having no .exe suffix. POSIX NewDataIsAltStream carries the Agent archive-ADS flag unavailable in POSIX CDirItem; Windows keeps its original field. Original MAPI remains Windows-only; Qt services explicitly refuse unsupported Unix email rather than report success. |
| WorkDir.cpp/.h, ZipRegistry.cpp/.h | WorkDir retains original mode/removable/temp selection, CreateTempFile and MoveToOriginal. Explicit settings path replaces implicit registry location; SetMTime delegates to source COutFileStream and OwnedLease projects the existing temp-file lifetime owner. ArchiveFolderOut applies time before close/publication and adopts only an already-owned spool after success. ZipRegistry's complete body/header/default/undefined/locking disposition is recorded above; there is no second settings implementation. |

Dependency/build review includes original `UI/GUI/makefile`, `UI/Console/Console.mak`,
`UI/FileManager/FM.mak`, `Bundles/Fm/makefile` and `7zip.mak`, plus current CMake
source lists, platform definitions and interface links. Common algorithms compile
as C++; their unchanged `C/` allocation/CPU/hash/thread dependencies still compile
as C. GUI/native shell entrypoint differences do not justify omitting Common
workflow bodies. Original commented/debug/old-code branches remain inactive;
Windows security/device/MAPI, POSIX links/owner/mode and external-codec branches
remain platform-dependent source behavior, not blanket N/A declarations.

This closes Common's file-owner and adaptation disposition at this checkout.
It does not assert execution of every retained conditional branch, native desktop
input, rendered Windows parity or final package acceptance. Agent's desktop
disposition is recorded above. Remaining FileManager bodies and P3-03/04/05/06
acceptance remain open. Current validation and reproducible review evidence are in the
[Common reconciliation audit](../upstream-port-audit.md#p3-05--common-owner-and-adaptation-reconciliation-2026-10-08).
