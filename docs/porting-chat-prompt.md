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
P1-17: source ZIP per-item comment writer/ChangeComment/SetProperty и реальный
GUI consumer через ArchiveBackend. Native catalog/module lease и ZIP reader/writer
factories из принятого P2-01 теперь доступны; inspect original source writer и
dependencies перед интеграцией. Это ещё не перенос actual comment consumer.
P1-18 actual RAR memory consumer остаётся mandatory, не закрывать synthetic tests.
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
