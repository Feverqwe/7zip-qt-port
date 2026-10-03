# Сообщение для каждого нового чата

Скопируйте текст ниже целиком. Обычно ID менять не нужно: агент выбирает
следующий доступный шаг по чеклисту. Для конкретного шага замените предложение
о выборе на «Выполни шаг P0-02» с нужным ID.

```text
Продолжай портирование Sevenzip Desktop по docs/porting-plan.md и
docs/porting-checklist.md. Прочитай AGENTS.md, индекс .codex/memory/README.md
и только relevant notes; проверь git status, текущий код, upstream-port-audit
и 7zfm-parity. Не сбрасывай и не присваивай себе чужие изменения.

Выбери первый незавершённый незаблокированный шаг минимального приоритета, для
которого выполнены фактически нужные dependencies/capabilities. Назови его ID
и конкретный результат, затем выполни его до конца. Сейчас следующий ID —
P2-06: ADS/streams/CProxyArc2 по platform capabilities с исходными
AltStreamsFolder/FSFolder/ArchiveFolder/Agent proxies и Windows FileIO/FileFind.
Сначала inspect bodies и native Windows capability; Unix unsupported ветки
disabled с причиной. Подключить реальные list/open/copy/delete/update/Properties
consumers, failures/Cancel и EN/RU UX, сохранить baseline и source control flow.

P2-05 принят целой итерацией2026-10-03: complete LinkDialog/CApp::Link,
partial original FM privilege startup, raw Windows FSFolder properties,
native leaf Copy/Move/Delete и file/new-folder metadata. Actual File menu,
selection/create/Cancel, raw relative/absolute/dangling/cycle/directory links,
Windows junction, hardlink inode/bytes, privilege refusal и Linux real EXDEV
проверены. Full macOS unsandboxed format/build/205CTestPASS82.95s;
Windows7/7PASS30.24s, Linux7/7PASS0.15s zero focused skips;
12EN/RU normal/compact JSON reviewed, baseline382exact/173adapted/3local.
Counts34/46,12remain;P0 12/12,P1 16/18,P2 5/10,P3 1/6. Whole local commit;
no push/upstream writes. Finder отдельно mandatory P3-03.1, rendered/full P3
не закрыты. Archive storage/deletion link consumer остаётся P2-07 с явным guard.

P2-04 принят целой итерацией2026-10-03: complete Bench/BenchmarkDialog,
partial original CompressCall Benchmark entry, native backend providers/services,
actual Tools menu, source metrics/total, memory refusal/error/Restart/Stop/Close,
worker join, deferred MainWindow close and Help. Full macOS unsandboxed format/
all-target build/202CTestPASS89.68s; final localized affected3/3PASS5.82s.
Linux5/5PASS3.34s+final3/3PASS2.28s; Windows5/5PASS4.09s zero focused skips.
Eight normal/total EN/RU JSON per platform reviewed; baseline378exact/171adapted/
3local verified. Counts33/46,13remain; P0 12/12,P1 16/18,P2 4/10,P3 1/6.
Whole local iteration commit; no push/upstream writes. Native Finder separately
mandatory P3-03.1; rendered/full P3 still open. Earlier WIP evidence is superseded.

P2-03 принят целой итерацией2026-10-03: complete PanelCrc/HashCalc/HashGUI,
native Hash writer/reader/Test, source method providers/menu, scan/read/selection,
actual encrypted/wrong password/corrupt ZIP/error/Cancel и modal lifetime проверены.
Hash result JSON540/420 reviewed. Full macOS unsandboxed format/all-target build/
199CTestPASS79.48s; later localized MyWindows/Rename/lifecycle fixes5/5PASS2.79s.
Linux12/12PASS3.82s plus final5/5PASS3.28s, Windows build13targets/13/13PASS13.04s,
zero focused skips. Baseline374exact/165adapted/3local verified. Historical counts32/46,
14remain;P0 12/12,P1 16/18,P2 3/10,P3 1/6. Whole local iteration commit;
no push/upstream writes. FinderP3-03.1 and rendered/full P3 remain mandatory.
Historical pending checkpoints below are superseded by the accepted records.

P2-02 принят целой итерацией 2026-10-03: source SFX/LoadCodecs, оригинальные
runtime builders для macOS/Linux/Windows, private images/SetCodecs/native leases,
external codec/global-only facade/no-main Formats/assets/junction cycle,
single/split/AES/Add/delete/pause/cancel и EN/RU JSON проверены. Final macOS
unsandboxed formatting/all-target build/197 CTest PASS107.31s Qt6.11.2 arm64;
Linux ARM64 Qt6.4.2/GCC13.3 focused9/9PASS2.79s zero skips;
Windows11 ARM64 Qt6.8.3/MSVC14.44 clean build/focused7/7PASS13.76s.
Windows external/SFX zero skips; existing owned_temp symlink fixture остаётся
для P3. Baseline372exact/164adapted/3local verified. UTF-8 console в build.sh
исправляет Ninja/MSVC header dependencies без изменения VM/toolchain; prefix и
actual header records проверены. Counts31/46,15remain; P0 12/12,P1 16/18,
P2 2/10,P3 1/6. Whole local commit; no push/upstream writes. Rendered Windows
reference/full P3 и отдельный mandatory Finder P3-03.1 остаются открыты.
Далее historical P2-02 WIP checkpoints: pending/next/live statements superseded
этой приёмкой, не текущий backlog.

Historical P2-02 WIP on559102c: scripts/build-sfx.sh builds official native SFXCon from
baseline snapshot into build/sfx with SHA/source/host/version JSON and license;
native Mach-O arm64 Test/Extract disposableUnicode fixture PASS, source26.03.
Manual/script builds sameSHA ad64e6ffb725efdab3a2d3072ebffb6b20f74af10a34ed4ebb8c782cdc32b99b.
Source CompressDialog IsMethodSupportedBySfx/GetMethodNames filter,
CheckSFXControlsEnable/OnButtonSFX ported; Windows.exe/Unix empty native extension
follows Common/Update platform rule. Original CUpdateErrorInfo/PrepareSfxModule/
WriteSfxModule connected; runtime provenance/digest/version/native CPU validation,
backend create/split publication/+X and actual source checkbox/order/transitions
connected. Native self-Test/Extract single/multi-root/split/AES, missing/occupied/
cancel verified; EN/RU normal/compact4JSON reviewed. Unixvolume names mustuse
<self>.7z.NNN because original SFX reader cannotfind official7zz <self>.NNN.
Dynamic Console7z/Format7zF runtime built unchanged in pinned snapshot by
build-codec-runtime.sh; app selects prepared/deployed runtime, explicit
SEVENZIP_ENGINE_PATH, standalonefallback. Source Load directory/NeedSet body
factored into explicit-folder entry, native per-handler lease protocol; actual
external Copy-codec Create/List/Test/Extract/native leasedhandler, Unicode/space
pluginpath, ABI warning/mismatch/cancel and defaultengine checks verified.
About source codec-error branch connected to retainedcatalog; getterconst only,
new appcaption and normal/compact geometry verified. Baseline371/164/3 after
Windows COM.h/Codec.def dependencies; fourexact
export sources staged onlyforinventory, no helpercommit. No owncommit/fullgate/
P2acceptance. Private runtime images now connect the original SetCodecs loop
without clobbering retained catalogs; discovery and archive jobs exclude each
other. External Formats module without main DLL passes native/engine consumers.
Global-only external facade delegates original 7z handler with no per-instance
interface; exact decoding survives another confirmed catalog's release.
Implicit module uses engine Libs resolution. Matching-SHA non-executable SFX
refused before engine start. Latest codec2/2PASS4.47s, SFX/UI4/4PASS5.71s macOS
arm64. Nested Codecs/Formats assets retained after original removal and cyclic
directory refusal checked, codec/catalog/SFX3/3PASS5.31s. Windows build-sfx.ps1
and build-codec-runtime.ps1 compiled unchanged SFXWin/Console/Format7zF/Alone2
on Windows11 ARM64 with MSVC14.44; both clients report26.03. Earlier tools-missing
probe inspected PATH only; installed scoped developer environment is available.
Next: finish Windows Qt consumer build/checks and image isolation. Linux ARM64
Qt6.4.2/GCC13.3 focused6/6PASS1.52s zero skips; four SFX JSON controls reviewed.
FSFolderCopy callback pointer uses local original UInt64 instead of Qt quint64;
copy flow unchanged, mac filesystem2/2PASS0.27s. Windows SFX fixtures use GUI
-y/-o/-p extraction and separate engine Test, native execution still pending.
Actual Add acceptance and SFX delete-after-publication,
engine/volume-worker pause/resume, second-root/prepublication volume cancel pass;
latest affected4/4PASS2.63s macOS. Advanced method override preserves original
UpdateGUI property order; source allows explicit override of the selected method.
FM FilePlugins/RegistryPlugins external
branches are commented out upstream; ProgramLocation has no functional body.
Do not revive obsolete branches. Preserve current WIP; indexednote carries
details/temporarypaths. No ownlivehandles at checkpoint.

P1-18 принят целой итерацией: полный source Rar5Handler/native decoder/crypto,
actual RAR5/RAR7 required2MiB/allowed1MiB/flags через FM memory callback;
Allow/Skip/Cancel, whole/selected Extract/Test, batch remember/save/new-callback
reuse и CRC errors, compressed RAR5 size/CRC/independent libarchive проверены.
MainWindow root/selected/Test все шесть overwrite modes без console fallback,
traversal и потерянный item contract дают отказ. Four actual Mem JSON700/560
reviewed; baseline367exact/164adapted/3local verified,16,536 handlerfunctional
tokens identical. Финальный unsandboxed check.sh formatting/all-target build/
193 CTest PASS79.91s macOSarm64Qt6.11.2 2026-10-02; первый foundation linker
failure исправлен до приёмки. Whole commit subject:
Port source RAR decoder memory requests and GUI consumers (P1-18).
Historical counts after P1-18:30/46,16remain; P0 12/12,P1 16/18,P2 1/10,P3 1/6. Windows/Linux runtime/
rendered reference и additional compressedRAR7/encrypted/solid/multivolume
workflows unverified. Finder сохраняется отдельным mandatory P3-03.1.
Не разрешать giant dictionaries в tests: малого forced handler limit достаточно.
No push/publication/upstream edits.

P1-17 принят целой итерацией: source ZIP handler/writer/dependencies и original
CommentItem/SetProperty/ChangeComment, actual EN/RU plain/AES command, publication/
reopen/clear/error/cancel, nested parent Yes/No и native pause/cancel проверены.
Final unsandboxed full check.sh1910 на isolated own-scope snapshot: formatting,
all-target build/191 CTest PASS86.83s macOSarm64Qt6.11.2 2026-10-02, baseline351/
163/3. All162 own source/build-module/test files совпадают с checkout. Shared
gate43923 passed191/19288.64s: только чужой runner_vm missingfile; foreign hunks
сохранены и не входят в итерацию. Four Combo EN/RU360/260 JSON reviewed.
Historical counts after P1-17:29/46,17remain; P0 12/12,P1 15/18,P2 1/10,P3 1/6. Windows/Linux runtime и
rendered Windows parity unverified; native Finder отдельно mandatory P3-03.1.
Whole iteration commit subject: Port source ZIP item comments and GUI workflow (P1-17).
No push/publication/upstream edits.

Ниже historical P1-17/P2-01 checkpoints: их pending/live/disabled/next statements
superseded текущей приёмкой выше; это evidence, не текущий backlog.
P1-17 WIP: полный source ZIP handler/writer/dependencies уже скопирован;
sevenzip_zip и native_zip_writer_tests собираются, CTest1/1PASS0.51s (обычный/AES
ZIP × success/failure/cancel). Это собственный build source handler, не handler
из установленного DLL; metadata/Unicode comment/bytes/extraction проверены.
Next: actual ArchiveBackend leased native consumer, source Agent CommentItem/
SetProperty/ChangeComment, GUI/publication/reopen/error/cancel, потом full gate и
whole iteration commit. Команда disabled; accepted count28/46 не меняется.
Latest backend checkpoint supersedes that next statement: source CommentItem/
SetProperty/AGENT_OP_Comment and backend source ZIP consumer now connected;
verified source factory override in catalog, native item count/path order guard,
native properties and native reopen. Affected build90336 exit0; focused5/5 suites
PASS2.82s (native_comment/writer/folder_opening/codec_update/owned_temp_publication).
Plain/AES actual SetProperty publication/reopen/comment clearing, oversized
comment refusal/cancel/unchanged bytes tested. All-target build36443 still live
at checkpoint; do not restart while its handle is live. Next actual Panel/GUI,
broader regressions and full gate; no P1-17 acceptance or commit yet.
Subsequent36443 failed backend-only link without Agent: source g_ExternalCodecs
single empty ABI storage moved to local archiveexternalcodecs.cpp in source ZIP
library, Agent.cpp definition removed. All-target35574 terminal0; broader shared
consumer wrapper43406 live at checkpoint. Earlier36443 live status superseded.
Latest verified checkpoint:43406 terminal8,20/22pass, native parser cases
archive_opening/codec_commands_ui crashed because new hasCommentWriter looked up
source Parser FormatIndex=-1. LLDB20670 identified integration bounds bug; added
signed/bounds guard, source parser algorithm unchanged. All-target63060 terminal0;
same expanded wrapper67928 terminal0,22/22PASS14.39s; baseline351/163/3 verified,
diff checks pass. All earlier live handles terminal; no current live work.
Next actual source ChangeComment/GUI/capability/errors/refresh/geometry and full
gate/whole P1-17 commit. Accepted count28/46 unchanged; command disabled.
P2-01 принят целой итерацией: полный unsandboxed check.sh12893 formatting/build/
188 CTest PASS74.09s macOSarm64Qt6.11.2 2026-10-02, baseline224/161/3 verified.
Actual MainWindow Add seven formats success/cancel, Inside */#, missing/version
mismatch read-only fallback, native folder/password/cancel/leases, TGZ/Split
extract, bzip2 refusal/XZ empty-stream Delete/cancel и owned extensionless spool
publication проверены. Current14 Add760/640 JSON reviewed. Existing source format
and Deflate/BZip2/LZMA2 controls restore after empty→one-source transition. Native
DefaultName is preserved by copied mutation basename; source -sae retains exact
private archive paths. Whole iteration local commit subject:
Port native codecs and archive format workflows (P2-01). No push/upstream edits.
Counts28/46,18remain; P0 12/12,P1 14/18,P2 1/10,P3 1/6. Windows/Linux runtime
and rendered Windows parity remain mandatory. Finder separately P3-03.1.
Ниже исторические checkpoints P2-01: pending/next statements в них superseded
приёмкой выше, не являются текущим backlog или новой инструкцией.
Historical P2-01 bootstrap: full source LoadCodecs.cpp/.h, exact RegisterArc/Registry,
Qt executable-path service; actual installed7z.so26.03 typed metadata и
RAR5 reader/ZIP writer objects проверены. Focused codec/library/hash4/4PASS0.56s,
baseline220 exact/161 adapted/3 local — historical loader checkpoint; no helper commit.
All-target build passed; расширенный набор13 consumers PASS7.54s macOS arm64
Qt6.11.2 2026-10-02. Full gate для завершённого P2-01 ещё не выполнялся.
Earlier checkpoint: ArchiveBackend владеет immutable shared ArchiveCodecCatalog;
strict module-version/reload/native-handler lease проверены. Native ParseOpenTypes/
ParseComplexSize и exact DefaultName.cpp/.h перенесены, focused7/7PASS3.89s после
новых terminal builds, baseline222 exact/161 adapted/3 local.
Latest: asynchronous backend probe/discovery/version/timeout/cancel и queued safe
delivery подключены; default MainWindow запускает probe, injected backend caller-owned.
Metadata Open filters/IFolderManager extensions и source default-name body работают;
archiveformats.h удалён, menus/transfers используют source read-only state. Native
encoder flags не включают отсутствующий transactional transport (только actual ZIP/7z).
Real source CFfpOpen unknown-suffix ZIP/gzip/TGZ/AddExt/parser '#' и unchanged bytes
проверены; focused9/9PASS7.42s после terminal affected builds на macOS arm64 Qt6.11.2.
Latest Add continuation: native SetArchiveName2/SetArchiveName/metadata history,
ArcFormats/ArcIndices/KeepName/OriginalFileName inputs сохранены; Add удерживает
catalog и использует actual detected existing type, missing module disables OK.
ZIP/7z transport choices from native UpdateEnabled; standalone no-catalog legacy
compatibility не является discovery. После terminal affected + all-target builds
focused16/16PASS7.72s macOS arm64 Qt6.11.2, baseline222/161/3 verified. Native
archive-path760/640 и EN/RU Add JSON reviewed. Native gzip KeepName helper не
доказывает реальный gzip Add.
Latest selection/name: copied ShowDialog one-file/censor/FileInfo/eligibility block
в оригинальный UpdateGUI owner; Qt source picker add/remove обновляет OriginalFileName/
KeepName/ArcIndices, native CreateArchiveName задаёт initial suggestion. Native
one-file/multiple/directory/no-writer и live two-files→one naming/default-name cases
проверены, final11/11PASS7.53s after terminal consumer builds на macOS arm64 Qt6.11.2,
baseline222/161/3 verified. Current native760/640 JSON reviewed. Next: полный
writable-format creation/update transport scope, final live command/error UX и full gate;
P2-01 не принят, helper commit отсутствует. Native handlers не закрывают P1-17/18.
Latest backend creation: stable CompressionOptions.format selects actual native
name, source FindFormatForArchiveName/PrepareFormatSelection own eligibility,
source table owns method defaults. Real seven-format unknown-suffix create/list/
extract binary Unicode round trips pass; pathless bzip2/xz entries retained,
source DefaultName supplies their name, bzip2 undefined size remains undefined.
Partial-output failure/cancel preserve sources and remove staging. Terminal
all-target build and17/17PASS7.54s macOS arm64 Qt6.11.2 2026-10-02; baseline222/161/3
verified. UI projection still ZIP/7z, generic update transports and final UX/gate
next; no helper commit/acceptance. Do not edit a C++ test while its build is live:
an earlier all-target attempt failed on stale moc after our slot rename; the
subsequent terminal all-target rebuild above passed. No live build/test handles.
Latest GUI creation: native seven-format list, typed format DTO, source encrypt/
header flags, TAR GNU/POSIX at level0 and xz solid flags; source selection changes
refresh all format controls. codec_creation_ui covers14 real type/size creation
cases plus gzip→7z source transition and actual POSIX TAR magic. Top/encryption
JSON reviewed for all14 states; terminal all-target build20815 exit0,21/21PASS7.66s
macOS arm64 Qt6.11.2 2026-10-02. Current new-creation UI is no longer ZIP/7z-only;
existing update services remain ZIP/7z. Next: generic actual update transport,
success/failure/cancel and source callback consumers, final UX/full gate and whole
P2-01 commit. No helper commit or acceptance. No live build/test handles.
Latest source update: native UpdateEnabled replaces backend4guards/QtSourceUpdate
guard and Add existing-target projection. Actual CFfpOpen→Agent operations→source
publication CopyFromFile and CopyFrom/Add-plan pass for7formats under unknown
suffixes,42 success/failure/cancel scenarios. Source-plan projection deletes
omitted items only; native Add replaces referenced files. Native KeepName one-file
plans retain original input default-name and write separate output; copied
result.bzip2/xz changes unnamed item identity and is no longer used for matching.
Terminal all-target buildc3ad03 exit0, final23/23PASS8.61s (09b7a4), macOS arm64
Qt6.11.2 2026-10-02, baseline222/161/3 verified. Next: review/cover remaining
mutation and actual MainWindow command/error/read-only/type UX; review source
opening partial-port claims, final full gate and whole P2-01 commit. No helper
commit/acceptance or live handles. Native loader/writer objects do not close
P1-17/18: actual ZIP comment/RAR memory GUI consumers remain mandatory.
Latest native source: COMPLETE OpenArchive.cpp copied from recorded0766b733;
NParser/signature scanning/PrepareToOpen/OpenStream2/OpenStreamOrFile/nested
Open/Open2/ReOpen retained. Only canonical includes, provenance, optional existing
QtOpenService branch and deduplicated source default-name body differ from the
fresh upstream file formatted once with repo style. Exact SetProperties.cpp/.h
added; baseline224/161/3 verified. Native Open3 without Qt service tested on actual
ZIP/7z/gzip/TGZ/# handlers, wrong forced type, cancel/finished/unchanged bytes.
IMPORTANT: initialize native options.excludedFormats/props, and create fresh
options for a new open after releasing native streams; source mutates stream
pointers. Two initial test crashes came from missing exclusions and stale reused
stream options; no production source algorithm was changed to hide them.
Source-plan glue coherently moved to archivebackend_sourceplan.cpp (217lines,
validation/KeepName/orchestration functions); sourceupdate now253lines. Borrowed
modules not split. All-target7099 terminal0,25/25PASS9.06s b66793 macOSarm64Qt6.11.2
2026-10-02. App still uses Qt process-open service; native GUI/backend integration,
remaining actual mutation/MainWindow command UX and final full gate needed before
P201acceptance/whole commit. No helper commit or live handles.
Latest application integration: original native Open3 now executes in the
CFfpOpen/QtArchiveFolderManager worker through ArchiveBackend::openNativeArchive.
ArchiveNativeOpen holds the original link/catalog and a stable callback proxy,
detaching opening UI while volume callback references remain. Resource exposes
native archive properties and closes streams before publication/private cleanup.
Tests cover actual property IDs/catalog reload lease, wrong-type refusal before
process start, worker cancel and native encrypted-header password handoff. Real
TGZ remains on gzip level (native GetStream lacks seekable decoded stream), its
source callback extraction returns exact TAR bytes. Native Split→7z source opening
and extraction return actual Unicode-named input bytes. Expanded37-consumer run
caught split Delete/Rename readiness regression from source-read-only migration;
process writer now refuses volume sets/native nested chains and native read-only
handlers, preserving those flags after Close. Native nested process services retain
parsed opening intent (explicit *:r for empty native types). Final all-target82764
terminal0,37/37PASS15.88s macOSarm64Qt6.11.2 2026-10-02, baseline224/161/3 verified.
No full gate/helper commit/P201acceptance. Next: remaining actual MainWindow
command/error/read-only/type/mutation UX and whole-step gate/commit. Native
Open3/reader objects still do not close actual ZIP comment P117/RAR memory P118.
P1-16 implementation accepted: native OnDrag/Drop/transfer/CompressDropFiles и
CApp Begin/End, Qt MIME transport; focused6/6PASS0.99s macOS arm64 Qt6.11.2
2026-10-02. Actual ZIP binary bytes/known+unconfirmed receiver/refusal/drag+extract
cancel и native disk Copy/Move проверены. Live both views/lazy F9 panel, popup/
confirmation/Add, Yes/No/Cancel, close/SingleClick/hover guards уже подключены;
focused11/11PASS7.44s до последних naming/header fixtures, actual archive→disk/
archive также проверены. Полный unsandboxed check.sh formatting/build/179 tests
PASS67.66s macOS arm64 Qt6.11.2 2026-10-02. Нативная проверка Finder вынесена
в отдельный обязательный P3-03.1; она не блокирует приёмку реализации P1-16
или работу над P2, но требуется для P3-03/P3-06. Computer Use app access отказан,
запрос ожидает ответа; native transfer не выполнялся. Baseline218 exact/159 adapted/3 local.
Current audit/note содержат точный scope; eager materialization documented,
GUI-thread preparation/large-list responsiveness остаётся mandatory P2-10.
P1-15.2 и parent приняты: actual native BrowseDialog2/modal command, creation
ownership registry, native/Qt/nested/process leases, private spool publication,
Yes/No/Cancel/first failure/replaced identity и active child-editor protection.
Source ShowDots/SingleClick/hover/FullRow/Grid, context/props, navigation/Help/F1
подключены; EN/RU900×580/500×320 current-build JSON reviewed. Focused18/18
PASS12.43s; full unsandboxed check.sh formatting/build/175 tests PASS66.82s
macOS arm64 Qt6.11.2 2026-10-01; baseline217 exact/158 adapted/3 local verified.
User archives remain unowned. Cross-volume publication IO/late Finish cancellation
locally untested; Windows/Linux runtime/rendered parity unverified. Directory
Open Outside : 7-Zip disabled до P2-07 directory CLI consumer; это обязательный
сохранённый долг данного owner. File self-open/OS association подключены.
P1-15.1 принята: original HelpUtils/About и release HTML/CSS, все текущие
Options/Add/Extract/advanced contexts; focused11/11PASS4.70s, final F1/geometry
4/4PASS1.05s, final native About2/2PASS0.54s и full unsandboxed check.sh172/172
PASS63.63s macOS arm64 Qt6.11.2 2026-10-01; baseline215 exact/156 adapted/3 local.
Disabled System/Benchmark source Help callbacks закрываются с P2-08/P2-04;
cleanup context принят с P1-15.2.
P1-14.1/.2 и parent приняты: все текущие forms/messages/local context EN/RU,
static RC/dynamic LangString fallbacks, hot switch/restart/pending state, source
RAM/size/Auto/bookmarks, geometry normal/compact и keyboard/buddy/collision checks.
Full unsandboxed check.sh170/170PASS63.11s macOS arm64 Qt6.11.2 2026-10-01;
baseline210 exact/152 adapted/3 local verified. Final gate failure fixtures13/13
PASS3.00s, isolated modal consumers2/2PASS1.04s. Qt platform standard buttons/
macOS mnemonic policy сохранены, Windows/Linux runtime/rendered parity unverified.
Core language services — sevenzip_filesystem, widgets — sevenzip_panel; cancelled
result определяется HRESULT/typed completion, partial-move warning сохраняется.
Source baseline0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Счёт27/46, остаётся19.
P1-13 parent открыт до .2:
source shell policies/flags/registration требуют P2-09 consumer, controls disabled
с причиной. P2-09 зависит от P1-13.1 capability, не от открытого parent.
P1-12 parent открыт до .2 всех page owners; .1 capabilities не блокируют Language
и System. Windows ADS/runtime и Windows/Linux rendered parity не проверены.
ZIP comment P1-17 и реальные decoder RAM
prompts P1-18 заблокированы отдельно до source writer/decoder capability;
обе функции обязательны для полной итоговой приёмки.
Работай крупными итерациями: целый интегрированный компонент или пользовательский
сценарий вместе с зависимостями, адаптерами, подключением consumers, UX и тестами.
Если пакет велик, выдели несколько крупных результатов с простыми ID вида
P0-02.1, .2; только один уровень подшагов. Helpers, ресурсы и отдельные методы
остаются внутренними задачами одной итерации, без отдельных завершений/коммитов.
Следуй разделу «Размер итерации и коммита» плана. Можно завершать несколько
итераций за чат; если поручен весь план, продолжай весь доступный scope. Один
заблокированный пункт не останавливает другие пакеты. Не повторяй уже
подтверждённые переносы.

При перезапуске сначала сохрани historical IDs/commits/evidence и составь карту
оставшегося scope: несколько крупных результатов со ссылками на прежние открытые
строки. Не продолжай рекурсивное дробление и не закрывай старые строки только
ради новой структуры. Для текущего P0-02 проверь границы P0-04/P0-05/P0-10;
работу соседнего пакета учитывай под его ID с точной prerequisite-связью.
Сверь незакоммиченные изменения с текущим кодом и прежним handoff; не считай
их своими без проверки и не переписывай историю Git.

Нужен source port: сначала прочитай оригинальные bodies и dependencies из
точного upstream commit, скопируй функциональную основу с исходными путями,
именами, control flow и notice. Адаптируй только необходимые Qt/platform/7zz
границы. Совпадение поведения или имени файла не считается переносом.
../7zip-upstream остаётся read-only. ArchiveBackend остаётся границей операций;
widgets не строят 7zz arguments и не реализуют форматы/сжатие.

UX входит в этот же шаг: commands/menus/shortcuts, focus/selection, dialogs,
errors/progress/cancel и persistence. Не оставляй его на последок. Сверяй
исходные 7zFM resources и functional code. Для visual/text/resize changes
проверь text spec, normal/compact geometry и current-build widget JSON;
screenshots только по правилам AGENTS.md. Не заявляй Windows pixel parity
или проверку всех ОС без соответствующих свидетельств.

Добавь meaningful regressions, для операций success/failure/cancel и relevant
Unicode/space/unsafe-path cases. Собери affected targets и проверь focused
CTest/direct consumers. Перед завершением code/tests/build changes выполни
./scripts/check.sh по AGENTS.md; для docs-only — git diff --check.
Старые test results не подтверждают новый код.

Обнови docs/upstream-port-audit.md (revision, paths, symbols, copied/adapted,
omitted scope/dependencies и причины), docs/7zfm-parity.md и другие документы,
чьи claims изменились. Запиши evidence/status своего шага в чеклист, обнови
счётчик и useful local handoff, сохрани чужие записи. Закрой parent только
после всех подшагов и интеграции. Недоступную обязательную ветку не объявляй
done или N/A ради закрытия шага: укажи blocker и следующий prerequisite.
Если одна функция блокирует составной пакет, проверь её реальные consumers и
выдели самостоятельный blocked ID по правилу плана; сохрани исторические
ID/evidence, уточни scope и зависимости остальных пакетов, обнови счётчик и
продолжи первый доступный шаг. Parent принимай только если оставшийся scope
самостоятельно проверен. Финальную полную приёмку с обязательным blocked ID
не закрывай.

В конце сообщи: ID и результат, что скопировано/адаптировано, точные checks
и проверенную ОС, оставшиеся ограничения и следующий доступный ID. Продолжай
доступную работу без промежуточных подтверждений, даже если другая функция
заблокирована.
Каждую завершённую крупную итерацию коммить отдельно после проверок. Отдельный
helper, новый dependency или промежуточная запись приёмки не завершает итерацию.
Focused checks запускай по ходу реализации, полный gate — перед завершением
итерации согласно AGENTS.md. Граница ответа/compaction не требует коммита.
Включай только
свои файлы/hunks, сохраняй чужие изменения и указывай hash локального коммита.
Публикация, push/PR, release и изменение upstream в это поручение не входят.
```
