# Qt Linguist migration — 2026-10-18

## Final scope and acceptance

Application-owned interface text now uses Qt Linguist. The source review covers
all presentation `.cpp`, `.h` and `.mm` files, including platform-conditional
sources, resource-ID consumers, dynamic diagnostics and native shell consumers.
The English bridge retains the original 444 release language entries and 14
additional RC messages. Russian has 981 finished application messages; English
has five plural-only messages. Qt Base contributes standard-widget translations.
Qt's strict lrelease checks pass for placeholders, accelerators, surrounding
whitespace and punctuation, with no unfinished application translations.

The final literal review distinguished resource-bound initial/fallback captions
from displayed text that lacked a binding. It also followed archive diagnostic
properties from ZIP/RAR producers to Properties and extraction/open messages.
Missing-volume reasons translate before adding their unchanged filename; ZIP's
two header-overflow warnings translate as known complete diagnostics. Unknown
plugin text remains data. Properties retains original diagnostic sources/flags
and updates values in place at a language change, including clipboard values.
The original truncation/newline presentation algorithm remains. The last short
caption found was ExtractCallback's external-read phase; it now uses the catalog
without changing the original ask-mode switch or progress transport.

The reviewed diagnostic producers are
`CPP/7zip/Archive/Zip/ZipHandler.cpp::GetArchiveProperty` and
`CPP/7zip/Archive/Rar/Rar5Handler.cpp::GetArchiveProperty`; their archive code is
unchanged by this adaptation. Qt rendering is added at
`CPP/7zip/UI/FileManager/ExtractCallback.cpp::ErrorInfo_Print`,
`FileFolderPluginOpen.cpp`'s open-message assembly and
`PanelMenu.cpp::AddPropertyString`. `ListViewDialog.cpp` retains the complete
source value truncation/newline rules and adds Qt value bindings for reload.
`ExtractCallback.cpp::Init/PrepareOperation` retains its cached phase fields and
original operation dispatch. The new diagnostic adapters depend only on QtCore;
the existing dialog bindings use QtWidgets. Upstream has no Qt catalog or
LanguageChange implementation to reuse at these presentation boundaries.

The joined header-overflow diagnostic uses its explicit pair of known warning
keys, independent of other entries added to the translation list. The existing
language-change regression checks both translations and unchanged embedded
paths/plugin text. No producer or archive parsing branch changes.

Untranslated technical values are intentional: brands, codec/format names,
characteristic tokens, registry/DBus/COM keys, command protocol, paths, debug-only
traces and arbitrary external-tool/plugin output. Native OS error reasons retain
their original API conversion. Fixed timestamp syntax retains source precision,
UTC/local policy and transport compatibility; regional counts and quantities use
QLocale at the Qt presentation boundary. No application language-specific text
selection remains in C++.

| Evidence | Scope and result |
| --- | --- |
| Source provenance | Working-tree verifier: 410 exact,203 adapted,six local paths; original upstream remains read-only. |
| macOS arm64 / Qt6.12 | Default gate built all targets;332/340 passed. All eight failures were then repaired and their focused11-suite run passed6.77s. Subsequent diagnostic/reading checks passed; the later local edits use focused acceptance, not a claimed second full-gate PASS. |
| Linux x64 / Qt6.12 | Expanded localization/direct-consumer profile24/25 passed; the remaining Properties test incorrectly required translated diagnostic values to stay English. Its corrected rerun passed. |
| Windows11 x64 / Qt6.12 | Expanded profile25/26 passed; the same Properties correction passed afterward. Actual no-app COM captions and DLL unload pass. Unicode-path QM generation/load, numerus, native errors and Benchmark geometry pass. |
| Final callback scope | macOS four suites passed2.42s; final Linux/Windows reading/dialog reruns pass. |

No opt-in slow suites or hosted release gate were run. Current-build widget JSON
covers the affected normal/compact states; no rendered original Windows reference
was used to claim pixel parity. Source-port completeness and the bounded runtime
evidence above are separate conclusions. Historical checkpoints below retain
their original narrower scope and counts.

## Adaptations and historical checkpoints

Source reference: <https://github.com/ip7z/7zip.git>, pinned baseline
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Reviewed
`CPP/7zip/UI/FileManager/LangUtils.cpp` load/reload, resource helpers and
fallback bodies, the corresponding language-page enumeration/apply caller,
`AboutDialog.cpp` initialization/help/homepage behavior and the Qt caption,
menu and options reload adapters. Their earlier port provenance remains in
the baseline manifest and archived audit.

The migration replaces bilingual Qt integration calls with
`QCoreApplication::translate` and context/source bindings for live captions,
tooltips and accessible names. Russian translations move from C++ to the
Linguist catalog; English remains the source fallback. CMake updates catalogs
with `lupdate`, compiles them with `lrelease`, and embeds the result. Extraction
includes platform-specific sources and sources compiled through object libraries,
independent of the current platform's target graph. Qt owns translator lifetime
under the application, and the existing language reload boundary selects it.
No archive engine, format, filesystem algorithm or source module boundary changes.

The About version and release date become translation parameters. Its wrapped
description derives geometry from the active translation rather than measuring
two hardcoded language variants. The dialog minimum height follows layout
height-for-width on resize, language and font changes; a live switch must also
fit the longer upstream free-software notice. Original Help/homepage behavior and the already
approved independent product identity remain.

Resource-ID helpers now read the same Qt catalog, keeping upstream caller names,
resource IDs, fallback branches, colon handling and standard-button mappings.
The English ID bridge imports all 444 original entries from the official 26.04
release assets. Original archive URL and hashes remain in
`resources/7zip/provenance.json`; the original files are no longer runtime assets.
Comparison with the pinned RC string tables identified 14 additional translatable
messages, now added to the bridge and Russian catalog. CRC/SHA algorithm names
and the codec-icon extension map remain data, not translated UI prose.

`LangPage` retains source Apply/Cancel, change notifications and Help behavior.
Qt catalog enumeration replaces native file enumeration, the Windows LANGID
table and missing-line report because TS/QM is the requested localization
boundary. A shared service discovers embedded/external catalogs, selects exact
or parent locale matches and supplies native language names. Old saved English
`-` and Russian `ru` choices remain valid. The page reports language/locale in
the original read-only report area; translation completeness belongs to Linguist.
The CLang runtime and temporary staging directory are removed. Source
`MyLoadString` still supplies original English RC captions for fallback; its
language-asset fallback now uses the compiled English resource bridge.

This initial checkpoint preceded the complete source review and acceptance
recorded above.

Selected-item presentation now keeps the source count calculation while passing
numeric selected/total values to Qt. The status ratio uses regional number
formatting, and the Properties count uses a Qt numerus message. English no longer
needs an `object(s)` construction; Russian uses its three cardinal forms. The
old cached resource-template field and its unused reload method are omitted
because Qt selects the form at presentation time. Reviewed the original
`PanelListNotify.cpp::Refresh_StatusBar`, Properties count branch in `PanelMenu.cpp`,
their status consumers and the application reload cache before this adaptation.
The four status parts, selection/focus calculation and property enumeration remain.

The temporary-file browser now refreshes translated headers, link captions,
protection tooltips, filter caption and source title in place. Source row identity,
sorting, selection, focus, paths and membership do not need re-enumeration for a
language change. Source column samples are measured after initializing translated
headers. This also avoids losing the intended initial column sizing while adapting
the source `BrowseDialog2.cpp` presentation; original enumeration/delete/link safety
branches remain. A native-link fixture verifies selection/focus survives RU/EN.

Additional local integration failures (association service, empty/failed volume
operation, archive path and executable SFX preparation) are catalog messages.
The upstream update error stores ASCII `AString` text, so its English source is
marked for extraction and translated when UpdateGUI constructs the Unicode final
message. The upstream error representation and filename handling are preserved.
Other upstream diagnostics still require a complete caller/boundary audit.

The Qt construction layer's 108 initial Russian caption literals now use English
source captions; the existing source/resource bindings still own their displayed
translations. Reviewed the main-menu ID bindings and Compress dialog form/item
bindings before the change. These construction changes do not replace any source
algorithm or command handler.

Qt Base's standard widget catalog is merged into each application QM with Qt's
`MERGE_QT_TRANSLATIONS`, so line-edit menus and other standard widgets follow the
selected application language. Official Qt Translations 6.12.0 source:
<https://download.qt.io/official_releases/qt/6.12/6.12.0/submodules/qttranslations-everywhere-src-6.12.0.tar.xz>,
SHA-256 `85929c0c30d6f273f23bd879bb69803ea17b010ab73cab2f8abf95357f6f6bbb`.
The matching module license information remains part of the Qt dependency
provenance; no Qt translation source is independently reimplemented. Build SDK
setup now includes Qt Tools and Qt Translations. Release CI definitions are
updated, but a hosted CI/release run has not been performed for this migration.

Reviewed original `PanelFolderChange.cpp` namespace item construction and current
address-combo icon consumers. The Qt icon adapter now receives resource identity
instead of recognizing translated Computer/Documents/Network names. Path/history
construction and the original source namespace path handling remain unchanged.
The regression checks that changing a caption keeps its icon, while identical
captions for different namespaces retain different icons. Windows native system
icon comparisons remain in their existing platform test.

Validation so far: macOS arm64, Qt 6.12.0, worktree based on
`a8d512697623f848dc9580b029601631102bfe9c`. Application and nine existing language
test targets build. The nine language suites pass, covering startup/live EN/RU,
fallback, pending-state preservation, dialog geometry and translated file transfer
results. The current catalog compiles 757 finished application messages. The existing
partial-language fixtures now compile actual QM catalogs with Qt's `lrelease`,
checking a third locale, missing/empty/malformed translations, Unicode/spaces
in the catalog directory, shortcut preservation and pending user input. Final
combined ten-suite rerun after this step: PASS (6.81s).
Macro-based upstream COM declarations produce parser warnings; they
do not affect extraction of the migrated messages. Windows/Linux and native
Windows rendered comparison have not been checked for this migration.
The additional `help_source` suite passes after detecting and correcting clipping
of the upstream notice on a live switch. Current normal/compact About JSON
reports for EN/RU show visible, fitting labels at both requested widths.
After merging standard Qt catalogs and changing initial captions, the ten language
suites pass (4.85s), including a new actual line-edit menu EN/RU check. The address
adapter's focused `panel_item_icons`, `address_combo` and `language_ui` pass (5.60s)
on macOS. The application rebuild also succeeds. Native Windows/Linux and hosted
CI remain unverified for these changes.
The selected-count and temporary-list changes pass 14 focused suites on macOS
(8.03s), including Properties, live selection preservation and current widget
geometry producers. Direct volume-operation and association suites also pass.
The SFX suite initially rejected a system 26.03 engine against the prepared 26.04
module; with the prepared project runtime selected in the child PATH it passes
(1.37s). No application or global environment setting was changed for that run.

Reviewed complete diagnostic producers and their sinks in `Update`,
`ArchiveExtractCallback`, `ExtractCallback`, `EnumDirItems`, `UpdatePair`,
`ArchiveCommandLine`, `Extract`, `HashCalc`, `LoadCodecs`, `OpenArchive`, and the
GUI/ProgressDialog exception handlers against the pinned upstream. Source
`AString` error fields and narrow exception types remain unchanged: constant
keys are marked for Linguist, and the Qt adapter translates before appending
paths or arguments. Update's independent diagnostic lines are translated at the
existing final-message boundary; its separate path list remains data. Codec
load errors likewise translate reasons separately from module paths and native
system errors. Commented-out diagnostics are outside executable scope.

`CPP/Common/CommandLineParser.cpp` moves from the exact vendor tree to the
matching adapted source path, with its complete source bodies retained. Its
five error keys gain extraction markers; callers still receive the original
English `AString` and translate the prefix before appending switch text. Headers,
types, switch grammar and parser control flow remain upstream. A one-off token
comparison excluding comments, formatting, includes and marker wrappers confirms
the source transfer. The working-tree baseline verifier passes for 413 exact,
200 adapted and six local modules, including this move, without changing the
Git index.

Additional reviewed presentation boundaries cover split/combine, filesystem
copy, temporary-folder creation, reparse/link failures, version-control errors,
benchmark thread/status/failure messages, invalid archive paths and context-menu
exception fallback. `MultiOutStream`'s zero-last-volume diagnostic retains its
original narrow throw. File operations and benchmark stop/restart/join sequencing
are unchanged. Qt timestamp menus now use their existing command/object identity;
upstream's year-caption recognition fallback is unnecessary with stable QAction
identity. Source drive-type property labels use the catalog; already rendered
type cells update in place, preserving source indices, names and selection.

The macOS parser regression exercises real parsing with invalid Unicode/spaced
arguments. An actual UpdateGUI error dialog verifies independent translated
reasons while preserving a Unicode filename containing an English diagnostic
key. The error-policy/native-callback/volume/link suites pass (five suites,
1.43s). Benchmark/menu/live-language suites pass (four suites, 1.34s), including
an actual Russian decoding error. Drive navigation/mount/live-language checks
pass (three suites, 1.01s), including unchanged row identity and selection after
EN/RU switches. The catalog now compiles 906 finished application messages,
2692 with Qt Base. The benchmark extraction warning comes from pre-existing
invalid debug-only code under disabled `PRINT_ITER_TIME`; the migrated messages
are extracted. These checks remain macOS evidence; Windows/Linux verification
and the full-application source audit are pending.

Further source review covers the remaining LinkDialog validation/reparse warning,
ADS self-copy prefix, comment-operation caption, expected-archive-type warning
in Properties and conditional version-control menu captions. Their source file
attributes, validation branches and callback ordering remain. Diagnostic row
bindings refresh the Properties warning name/value in place while preserving
selection; archive type values remain data. Existing real link refusal and
Properties tests now cover EN/RU messages and retained file bytes/row identity.
Four direct Properties/link suites pass (2.02s); normal/compact warning JSON is
also produced by the subsequent focused rerun.

Reviewed Bench's display producers, PrintLeft/PrintRight and GUI text-buffer
consumers against upstream. Benchmark report labels now use a shared catalog
context and UTF-8 in the existing narrow print stream. Numeric computation,
timing, aggregation, codecs, technical identifiers and branch order remain
unchanged. Padding counts Unicode text instead of UTF-8 bytes; the Qt console's
accumulation buffer uses AString explicitly on all platforms and its existing
UTF-8 SetText overload. Property values are encoded as UTF-8 at this same report
boundary rather than through a platform ANSI code page. Normal-mode report
headings and the thread-frequency caption also use the catalog, with the thread
number passed as a parameter. Historical report output is a snapshot of its
generation language. Existing limited real-engine callbacks now check the actual
EN/RU report and successful measured results. Together with benchmark lifecycle
and Properties, the focused rerun passes three suites (16.32s); no opt-in slow
benchmark matrix was run. The catalog contains 947 application messages after
the conditional menu-caption extraction. Full source and platform acceptance
remain pending.

The native Explorer host has no QCoreApplication. Reviewed the DLL factory,
QueryContextMenu's language-load call, FillCommand/resource lookup and the native
GetTitle consumer before adapting this path. Qt catalog ownership is now a small
QtCore service shared by diagnostic and resource adapters. Translators have
module lifetime, preserving upstream's global LangString contract without creating
an application inside Explorer. This also ensures a DLL destroys its translators
before unregistering embedded resources, even when the host has its own Qt
application. Application parenting left a translator referencing unloaded DLL
resources and caused an exit crash in the real COM test. This narrowly scoped
cache is necessary for repeated native
menu lookups; there is no separate dictionary or language-file parser. Native
ReloadLang resolves external catalogs relative to the existing source DLL-path
service. Resource enumeration avoids applicationDirPath when no application
exists. The obsolete Z7_LANG compile guard is removed around QueryContextMenu's
source LoadLangOneTime call because this Qt port always supports localization.

An app-less QtTest process exercises actual resource and diagnostic translation,
EN/RU reload, unavailable-language fallback and continued absence of an application.
It and the language/file-menu-policy suites pass on macOS (three suites, 0.42s).
The existing Windows host fixture now verifies the exact translated child caption
through the real DLL/COM boundary in isolated EN/RU processes, retaining its
object identity, file-content and unload checks. That Windows extension test has
been updated from source but has not yet been built or run for this migration.

The final short-label pass includes MemDialog's file caption, memory units in
MemDialog/SettingsPage/CompressDialog and the archive-moving callback, property
read failures, Agent update warning/error prefixes, LinkDialog's Windows read
error prefix, four switch-value diagnostics and registered archive-type titles.
Compared their complete producers and direct consumers with the pinned source.
Memory rounding, thresholds, limits, source input grammar and all operation
decisions remain; Qt formats displayed quantities through QLocale and catalog
templates. Memory labels retain source keys for live refresh. Processor-group
report text now uses a complete parameterized caption, avoiding a count followed
by a language-specific plural suffix. Technical algorithm identifiers, raw
property representations, engine protocol fields, debug traces and native OS
diagnostic contents remain data. The source's configured ISO timestamp precision
and UTC/zone presentation remain its existing user contract.

`CPP/Windows/ErrorMsg.cpp` moves from the exact vendor tree to its matching
adapted source path. The original complete Win32/native-errno/code-fallback
bodies remain authoritative. Its own internal/HRESULT reasons and hexadecimal
fallback caption use the same diagnostic catalog; native FormatMessage/strerror
results retain their source conversion, error-code handling and line trimming.
A provenance inventory check against
`9128b80e3a471108678e2b3ec3c985861c8d7b0f` passes on the current working tree
(410 exact, 203 adapted, six local paths, including concurrent ownership work).
No second ErrorMsg implementation remains. New presentation dependencies are
QtCore and the shared diagnostic/catalog adapters.

Qt's cached table-cell projection now marks text stale at a language reload.
It keeps the last displayed value while the original source notification guard
blocks provider access and recomputes it when access resumes. Rows, selection,
source indices, pending input and folder membership remain unchanged. The real
panel-property failure regression checks both languages and this guarded live
transition without rebuilding the list. The current catalog compiles 973 finished
application messages (2759 including Qt Base).

All 32 official Qt Base catalogs from the already verified Qt Translations 6.12.0
source have been compiled with the matching lrelease and installed in the local,
Linux and Windows SDKs. The guest SDKs retain source TS files, licenses and a
per-catalog SHA-256 receipt in `share/qttranslations`. The transferable package
SHA-256 is `ec62041d7cf42cba27fdbce10e198ee1d17b7de1a937bdb479a55c17d9204da8`.
Every installed guest catalog matches its receipt. This SDK preparation changes
neither runner worktrees nor compiler caches and does not constitute platform
build/runtime acceptance.

Reviewed upstream `FormatUtils.cpp` bodies, resource producers and all formatting
callers, including both delete paths, overwrite/property byte sizes, split-file
volume count and CompressDialog's small-volume confirmation. Resource templates
use Qt `%1` parameters; `MyFormatNew` retains its source API and delegates to
QString::arg. Original compiled RC fallback text normalizes its legacy placeholder
at the resource adapter boundary. User paths are arguments and retain literal
`%1` and `{0}` characters. The numeric overload receives the original quantities,
with no changes to selection, size calculations, confirmation decisions or I/O.
Qt numerus chooses grammatical forms and QLocale formats displayed quantities.
The English plural-only catalog supplies correct source-language forms and remains
the fallback beneath a selected language. For values outside Qt's signed-int
numerus range, complete quantity labels preserve UInt64 values without introducing
a separate plural-selection algorithm. Current catalogs contain 977 finished
application messages and five English plural messages.

The Open-as-archive clarification now retains a local translation binding instead
of only assigning its current text. Otherwise a later LanguageChange restored
the competing original resource caption. The existing real menu test catches this
regression. Benchmark's CPU information label now receives its complete source
row width; the label keeps right text alignment. Grid-level right alignment had
shrunk the widget to a narrow size hint and clipped a three-line hardware name on
both guests. Qt layout/font services remain the platform adaptation.

The first broad macOS default check built all targets but failed 17 suites;
it is not acceptance evidence. Focused repairs distinguish actual presentation
bugs from fixtures assuming English while the application correctly selects the
system language. Progress responders identify the source Close action instead
of comparing its translated caption. Source Add geometry checks follow the
documented editable-name alignment and deliberate long-path elision, rather than
pinning a font-dependent midpoint or requiring an elided path to fit in full.
The repaired direct consumers pass focused macOS checks. The next Windows run
passes six of seven selected suites, including actual COM host/unload, Benchmark,
native no-app language handling and source Add. The remaining Unicode-path test
fixture failure and Linux rerun are under investigation. These are focused
results, not complete three-platform localization acceptance.
