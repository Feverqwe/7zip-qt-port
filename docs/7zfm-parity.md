# Сверка с Windows 7zFM

## P1-17 — комментарий элемента ZIP (2026-10-02)

Эталон — `PanelOperations.cpp::ChangeComment`, `ComboDialog.cpp/.rc`,
`SevenZip.rc` IDM_COMMENT/Ctrl+Z, `AgentOut.cpp::CommentItem` и
`ArchiveFolderOut.cpp::SetProperty` baseline0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
Команда действует на элемент под фокусом, даже если отмечено несколько строк.
Общий диалог сохраняет имя элемента в заголовке, исходное значение и пробелы;
Cancel не записывает ничего, пустое значение очищает comment. Порядок prompt,
editable combo, OK/Cancel следует исходному resource. Actual EN/RU geometry
проверена360×150/260×150, четыре JSON отчёта просмотрены; Windows rendering
не доступен и pixel parity не подтверждается.

Qt выполняет исходный SetProperty на source progress worker, обеспечивая pause
и отмену через исходные callbacks. Ошибку ZIP writer показывает исходный
Set Comment Error/HRESULT; на macOS заголовок сообщения размещён в его тексте,
поскольку ОС не показывает QMessageBox window caption. Ошибка и отмена не
заменяют исходный архив. Обычный/AES ZIP сохраняет bytes файлов; вложенный ZIP
проходит существующее подтверждение записи в родителя. Native writer и
совпадение native/transport item order обязательны для доступности команды;
остальные форматы и read-only цепочки её не предлагают. Windows/Linux runtime
пока не проверен. Source provenance и частичный DLL scope — audit § P1-17.

Здесь описаны текущие сценарии, команды, ограничения и отличия от Windows
7zFM. Исходники в таблице служат ссылками на эталон поведения;
происхождение реализации, ревизии и технические адаптации находятся в
[аудите переноса](upstream-port-audit.md). Порядок проверки UI описан в
[тестировании](testing.md) и [контрактах компоновки](ui-layout-contracts.md).
Совпадение поведения не подтверждает перенос исходника или проверку на всех ОС.

P2-01 accepted: Open filters now use the matching official codec module's native
extensions/aliases, including DOCX and .001, with All files fallback on failure.
The default app asynchronously probes its installed engine version; mismatched
or unavailable metadata is reported in the status bar. Source folders use native
default-name/AddExt rules and read-only state for mutation commands rather than
filename suffixes. Real ZIP with unknown suffix, gzip/TGZ and parser '#' folders
are checked on macOS. Source UpdateEnabled now drives update-service readiness
for the detected type; original Agent CopyFromFile and Add callback plans update
all seven writable formats with checked failure/cancellation rollback. The
official handler still determines whether a particular mutation is possible;
UpdateEnabled does not promise every operation on every stream format.
With a valid module, source opening now runs native Open3 in the application
worker before the process listing and retains its reader for native archive
properties. Wrong forced types and cancellation refuse before process start;
native encrypted-header passwords pass to the listing service. Folder Close
releases reader streams before source publication. Native properties and module
reload lifetime are checked through CFfpOpen; item listing/extraction/update and
reopen still have a 7zz integration boundary. Actual TGZ retains the gzip item
level (no seekable decoded native stream); extraction returns its original TAR.
Actual native Split→7z and its process extraction round trip are verified. Volume
sets/nested streams stay read-only for this process publication service, including
after reader Close. Remaining mutation/command UX acceptance is still pending.
Add uses native eligibility and the detected existing archive type. Empty or
ineligible source selection does not discard that detected type; selecting one
file restores gzip/bzip2/xz and their Deflate/BZip2/LZMA2 controls. Actual Add
success/cancel and Inside */# command routes are checked through MainWindow.
Single-item bzip2 Delete reports its handler refusal; XZ produces the source
empty stream and still presents one stream item. Copied mutation basename and
exact-name CLI mode preserve source item identity and extensionless spool paths.
Source KeepName and
main-extension naming/history bodies are compiled; a failed catalog disables
confirmation with an explanatory format tooltip. Native one-file/OriginalFileName/
KeepName eligibility is connected to source-picker changes, and native
CreateArchiveName supplies initial suggestions for metadata-configured callers.
New archive creation now exposes native eligible 7z/ZIP/gzip/bzip2/xz/tar/WIM;
KeepName formats disappear when a directory or multiple sources are selected.
The explicit type request permits gzip's .gz extension and aliases. Original
FormatChanged encryption flags disable the group for unsupported formats and
hide Encrypt file names outside 7z. TAR retains GNU/POSIX at level0; real POSIX
magic is verified, and source solid flags include xz. Formats refresh their
parameter state when a source change removes the selected format. Current Qt
760×720/640×480 reports establish control states, scrolling and exposed footer;
Source KeepName updates preserve the original input name and write a separate
result: a temporary copied bzip2/xz archive would otherwise acquire a different
default item name. The process Add action set replaces selected files directly;
only omitted items are deleted. MainWindow command/error/cancel consumers and
the full P2-01 gate passed188/188(74.09s), macOS arm64 Qt6.11.2 2026-10-02;
Windows/Linux runtime and rendered parity unverified.

P1-16 implementation accepted: обе table/icon панели используют исходные правила PanelDrag: внутренний
disk→disk Copy/Move с Ctrl/Shift и выбором подпапки, внешний filesystem drop→Add,
drop в архив→Copy с Yes/No/Cancel. Правое drop-меню сохраняет исходные порядок,
маски и Cancel; source right drag разрешён только из filesystem. Наведение и
SingleClick при source drag приостановлены. Qt на macOS откладывает context menu
с нажатия до отпускания правой кнопки, чтобы допустить этот жест.
Архивный source заранее извлекает реальные bytes до Qt QDrag, поскольку у Qt
нет переносимого OLE release-before-drop hook; отмена может произойти уже после
извлечения. Move наружу не удаляет исходные архивные элементы. Неподтверждённый
внешний получатель удерживает owned temp после завершения локального drag без
удаления по таймеру. Native promise/delayed materialization не заявлены.
Qt Add всегда ждёт свой backend/progress вместо отдельного nonmodal 7zG.
Автоматические native/Qt receiver tests и normal/compact JSON выполнены;
Finder ещё не проверен (отдельный обязательный P3-03.1): Computer Use не получил
доступ к sevenzip_desktop. Эта проверка требуется для итоговой приёмки P3-03/P3-06.
Windows/Linux runtime и rendered Windows reference остаются unverified.

P1-15.2 accepted: source Delete Temporary Files is connected. It lists only registered
objects directly under the current OS temp directory and allows browsing inside
those owned roots. Delete uses native confirmation/first-failure/reload behavior;
active or externally unconfirmed objects are visible and protected. Qt association
launches retain protection because their completion cannot be established.
POSIX native temp creation now respects TMPDIR/macOS per-user temp, matching Qt.
Enter/Backspace/Delete/F5/F1/Escape and Help/Close retain source meanings; the six
columns may scroll horizontally at compact width. Qt controls replace Win32 shell
images; ShowDots, SingleClick/track-select, FullRow and Grid retain source
settings through Qt parent rows, shared hover and painting. Parent/dot navigation
stops at OS temp, excluding non-owned ancestors. macOS composed/decomposed
filenames are matched with OS identity proof, keeping Unicode cleanup reachable.
Context new-window directory open is disabled until the P2-07 directory CLI
consumer; file self-open and OS Open Outside use Qt platform services. Runtime
Windows/Linux and rendered Windows parity remain unverified. Final focused18/18
passed12.43s; full unsandboxed check.sh175/175 passed66.82s, macOS arm64 Qt6.11.2
2026-10-01; EN/RU normal/compact JSON inspected. P1-15 parent accepted.

P1-15.1 принята: Contents/F1, Help/F1 текущих Options/Add/Extract/advanced
и About открывают исходные
offline-разделы официальной26.03 справки с исходными ссылками/якорями. Qt browser
заменяет CHM presentation на всех ОС; exact members имеют release provenance.
Справка английская, локальные окно/Back/error — EN/RU, Close — стандартный Qt.
Normal/compact JSON проверен на macOS; Windows/Linux runtime/rendered parity
не подтверждён. About сохраняет source version/date/copyright/free-software,
OK/homepage и logo; local app-version и license link добавлены явно, внешняя
engine version не выводится из source baseline. About EN/RU360×330/300×310
JSON проверен; справка сохраняет pending input и не выполняет Apply. Full
unsandboxed check.sh172/172PASS63.63s macOS arm64 Qt6.11.2 2026-10-01;
owned-temp cleanup/parent впоследствии приняты в P1-15.2 выше. Help будущих disabled
System/Benchmark идёт вместе с их P2-08/P2-04 owners.

P1-14.2 принят (macOS, full170/17063.11s): Password/Copy/Split/Create-folder/Overwrite/Mem/Messages/Extract
получают captions из исходных language IDs. Смена языка обновляет текст без
повторной инициализации ввода и решений. Для папок Overwrite сохраняет исходную
общую подпись о файле. Split показывает исходную ошибку размера тома inline;
Mem использует исходную системную ошибку. Extract сохраняет два доступных
режима путей, исходные overwrite labels и исходный общий заголовок Extract.
Qt standard buttons/native pickers сохраняют платформенные тексты. Остальные
текущие формы/messages также подключены; Windows/Linux runtime не подтверждён.
Split/Combine используют исходные заголовки прогресса и подтверждение числа частей;
Open и продолжение hash — исходные IDS_OPENNING/IDS_CHECKSUM_CALCULATING.
Qt/7zz staging и контекст журнала получают локальные EN/RU-подписи; исходные
сообщения движка сохраняются. Source RC fallback, keyboard/buddy/collision и
normal/compact geometry входят в принятую .2; новые capabilities имеют свои owners.
Split/Combine worker показывает EN/RU; отмена определяется состоянием операции.
Подтверждение на100 частей сохраняет исходный текст и Yes/No/Cancel. На macOS
QMessageBox не показывает заголовок окна — документированное поведение
[Qt](https://doc.qt.io/qt-6/qmessagebox.html#setWindowTitle); сообщение и решения
проверяются в обоих языках. Windows/Linux runtime остаётся непроверенным.
Copy/Move, compression, Extract/Test, Open и hash получают заголовки исходных
owners. Локальные сообщения preview-limit и файлового staging получают EN/RU;
raw пути и сообщения движка сохраняются. FileTransferOperation различает отмену
с полным откатом и частичный сбой перемещения: предупреждение о частичном
результате остаётся видимым независимо от языка и запроса отмены.

Форма сжатия теперь показывает исходные B/KB/MB/GB и пометку `*  ` для
автоматических значений в обоих языках. Отказ по лимиту памяти использует
полный исходный CompressDialog formatter: требуемая память, RAM, заданный
лимит и ошибка; EN/RU popup проверен геометрией. Это preflight сжатия, реальные
decoder RAM callbacks остаются P1-18. Ошибки локальных filesystem/association/
mutation adapters получают EN/RU-контекст с сохранением raw путей и системных
сообщений. Progress Cancel сохраняет стандартные Qt Yes/No/Cancel и исходные
решения. Смена языка сохраняет реальные подсказки каталогов и обновляет только
специальную строку `..`; адрес получает локализованные placeholder/accessibility.
Закладки используют исходные Bookmark/`-`, сокращение длинного пути и shortcuts;
Qt сохраняет действия и полный путь в tooltip. Корневое представление меняет
исходные имена без сброса строк/выделения; POSIX Volumes — явная EN/RU-подпись
платформенного расширения. Все именованные поля прокручиваемой формы Add
проверены на доступность в EN/RU при760×720/640×480 с постоянным footer.

Отсутствующий перевод статической подписи сохраняет исходный RC-текст и его
акселератор, как LangSetDlgItemText; динамические сообщения используют LangString.
Поэтому английский заголовок формы — исходный «Add to Archive». Общая русская
клавиша `П` у Show password/Parameters сохранена: оба поля доступны через Tab,
Qt cycling проверяется, когда ОС включает мнемоники. macOS сохраняет свою
политику мнемоник; Windows/Linux runtime не подтверждён. Подписи составных
CPU/RAM-строк направляют фокус к реальным combo и имеют доступные имена.
Фильтр Open archive — Qt-расширение с явными EN/RU-подписями Archives/All files;
все прежние маски, включая первый том `.001`, сохранены.

P1-13.1: Options page2300 называется 7-Zip и сохраняет source MenuPage2.rc order:
две регистрации shell, Cascaded, Icons, ElimDup, Zone и 14 items. Только Zone
имеет настоящий FM consumer; Windows разрешает выбор No/Yes/Office/custom,
POSIX показывает disabled с причиной. Shell controls disabled на всех ОС до
P2-09; ElimDup не подменяет обычные FM extract defaults. Apply/Cancel и source
persistence сохраняют не изменённые shell values. Parent P1-13 остаётся открыт.
Normal640×390/compact520×380 JSON и geometry проверяются `menu_page`; Windows
ADS/runtime и rendered parity требуют отдельной проверки на Windows.

P1-12.1: Options сохраняет source порядок шести страниц и Apply/OK/Cancel.
Folders/Editor/Settings активны, System/Language disabled до своих owners;
текущий Menu scope указан в P1-13.1 выше.
Source RAM range/validation и working directory используются в consumers;
неверный ввод выбирает Settings/input, Apply обновляет обе панели с selection.
POSIX removable-only и Windows large-page privilege controls без capability
disabled. Child Windows engine получает large-page request, Create получает
working-directory path; исходный engine решает поддержку/ошибки операции.
Source FoldersPage2.rc/SettingsPage2.rc, normal640×390/compact520×380 widget JSON
и geometry проверены на macOS arm64 Qt6.11.2; прежняя compact height350 увеличена
для новых Settings controls. Windows/Linux runtime и rendered parity не проверены.
Parent Options acceptance остаётся P1-12.2 после остальных страниц.

P1-10.2: Properties показывает исходные item/raw/folder/archive levels,
MainItemProperty и ошибки non-open уровня. Undefined item value не подменяется
нулём; после separator отдельно идут folder totals. CRC/checksum форматируются
исходными helpers. Display может сокращать длинный текст, но clipboard/detail
сохраняют полный Unicode value. Qt заменяет Windows system Properties общей
source enumeration; original path block использует имя property kpidName.
File-only adapter сохраняет actual CLI fields, native UInt64/CRC и доступные
checksum/SHA raw bytes до64; большие raw/decoded security values остаются только
engine display text. CLI normalization уже может потерять исходные control
characters; generic CLI names/text не объявляются native raw metadata.
Проверены source ListViewDialog.rc grouping, current-build JSON540×360/360×240,
list/buttons geometry, macOS arm64 Qt6.11.2. Windows/Linux runtime и rendered
Windows parity не проверены; codec error-type lookup/P2-01 и ADS graph/P2-06
остаются отдельными source capabilities.

P1-10.1: исходный `PanelItems.cpp::EditItem(bool)` F3 считает все operated
directories через IFolderCalcItemFullSize; при выбранных папках focused file
не запускает viewer. F4 остаётся file-only. Qt перечитывает source column text
и totals без reload, сохраняя выделение/фокус. Исходный synchronous NULL-progress
расчёт больших деревьев остаётся для адаптации P2-10. Проверены real Unicode tree,
defined zero и normal980×620/compact640×400 JSON на macOS arm64 Qt6.11.2.
Windows reference — source body и исходная команда F3; rendered reference и
Windows/Linux runtime не проверены. Properties/raw remainder P1-10.2 открыт.

P1-09: исходный `PanelItemOpen.cpp::OpenParentArchiveFolder` предлагает обновить
изменённый внутренний архив при возврате в родителя. Yes записывает через
исходный OnOpenItemChanged; No и Cancel удаляют временную копию без записи.
При ошибке сохраняются внешний архив и изменённая временная копия. Native F4,
Rename/Delete/CreateFolder используют тот же parent-link path; каждый уровень
должен иметь update capability. TAR/GZip/tar.gz и тома не перепаковываются.
Маленький nested memory stream материализуется в source-owned файл из-за
file-only `7zz`; это дополнительный disk I/O относительно upstream memory path.
Дополнительный built-in text editor во вложенном архиве остаётся disabled.
Upstream reference — source Yes/No/Cancel MessageBox и parent-link bodies,
не rendered Windows screenshot. Current-build prompt386×116 с unclipped text
и тремя кнопками проверен при owner980×620/640×400, default Yes/escape Cancel,
parent address/list/focus восстановлены. Проверено macOS arm64 Qt6.11.2;
Windows/Linux runtime и pixel parity не проверены.

P1-07.1 Add exposes the native Parameters field, Options button and summary in
the source resource order after volumes. Options follows CompressOptionsDialog.rc
grouping with the original precision/capability state transitions: ZIP Windows,
UNIX and DOS; 7z Windows precision. Unsupported NTFS flags are hidden; explicit
UNIX/DOS ZIP precision hides creation/access controls. Profiles retain parameters,
precision and explicit time flags separately by format. Options Cancel discards
its copy; Add Cancel does not persist changes. The native property parser handles
method overrides; invalid and unsupported properties fail before publication.
Real mtime round-trips and normal/compact widget reports were checked on macOS.
P1-07.2: archive mtime now reaches multi-root create, native Add
publication and all output volumes. Preserve access is available on Windows;
the official engine does not implement it on POSIX, where the disabled control
explains that limitation. Windows preferences survive opening the form on POSIX
without enabling an ineffective request. Shared/Delete controls follow the
source resource order ahead of Encryption; shared writing is Windows-only.
Delete defaults off and is available for Add, disabled for Update/Freshen/Sync
with a reason. All create stages and all volumes must publish before source
deletion; native Add deletes after MoveToOriginal. Skipped or changed files are
retained by the original size/time checks. Cancel before publication preserves
sources; Cancel during deletion can leave the archive and partially deleted
sources. Physical deletion failures retain the file without undoing the successful
archive, following upstream. Create with links refuses deletion pending P2-05.
Compression limit rejection keeps Add open for retry; Cancel leaves profiles
unchanged. Actual RAR decoder memory prompts remain mandatory P1-18, requiring
native decoder callback transport; the existing memory form does not prove it.
Help remains P1-15. Windows/Linux runtime and
rendered Windows Options parity are unverified.

P0-12 Ask extraction uses the source `COverwriteDialog` and progress dialog.
Whole root archives, including encrypted and nested ones, use the original
`GUI/ExtractGUI` → `Common/Extract` archive loop. Selected items and a viewed
subfolder use `CPanel::CopyTo`/`CAgentFolder::Extract`; subfolder paths remain
relative to that folder. The file-only `7zz` service supplies bytes. A wrong
password is checked before an occupied target is opened; a correct retry can
replace it. Batch archives share one source overwrite decision state. Split
`.001` volumes are opened through the outer Split handler, and subsequent
parts are skipped in the source archive loop. Non-Ask modes still use the
backend extraction path. Real 7z/ZIP, nested and split fixtures were checked
on macOS; Windows/Linux runtime and rendered Windows dialog parity remain
unverified.

P0-05.1 filesystem rows now display the bound source folder's name and flat
prefix; Qt normalizes visible Unicode to NFC while retaining raw source names
and indices for operations. Hidden files and empty directories remain listed.
Flat enumeration retains a protected child row and continues to visible
siblings; cancellation still stops the scan. Filesystem Properties can read
source `descript.ion` comments and link/inode/change-time values. Complete
column/time formatting, source sort and system icons remain P0-06/P0-09.
The checked Qt layout and runtime are macOS-only; no rendered Windows parity
claim follows from them.

P0-05.2 presents large flat folders through the original CFSFolder item order.
Qt yields between 32-row batches so native icon lookup and row creation do not
monopolize one event-loop turn. Native count and shown-row status remain visible;
switching mode or location discards pending rows. The deterministic 2048-file
Unicode fixture and the 1500-file normal/compact window scenario verify this
local behavior. Windows/Linux runtime and Windows appearance are unverified.

P0-06.1 sorts bound filesystem, archive and root items by source properties and
source indices. The native rule groups directories first, compares the selected
typed property, then name, flat prefix and source index. An equal basename in
`dir10` and `dir2` uses source BSTR prefix order (`dir10` first), rather than
natural path sorting. The Qt header/action selection and four view modes keep
their current behavior; raw property columns and full column controls are
P0-06.2, time precision and UTC are P0-06.3. Normal/compact geometry is
checked locally, with no rendered Windows comparison.

P0-06.2 now projects the bound folder's original property catalog into the Qt
list. The header menu offers every reported property, with Name always shown;
width, order, visibility and sort restore separately for filesystem and archive
folder types. Raw values and sorting follow the source interface when a folder
provides it. The external `7zz` listing currently has no binary raw provider,
so this does not claim visible NTFS/security/reparse values for real archives;
full archive raw-value transport remains P1-10. Hidden columns are read when
shown, avoiding work during ordinary row publication. The normal/compact
four-view JSON confirms layout and control visibility, not pixel fidelity.
P0-06.3 adds the source View → Time menu examples for day, minute, second,
100 ns and nanosecond levels, plus UTC. Current panel time values use the
unchanged `PropVariantConv`/`PropIDUtils` formatter; changing the setting
redraws date cells without reparsing displayed text for sorting. The choice
survives reopening through Qt settings. Archive `7zz` local listing text is
converted back to `FILETIME` at the metadata boundary. Windows 7zFM keeps the
level across panels in memory; this Qt port also persists level and UTC between
launches. P0-06.4 adds the source View auto-refresh option and one-second
timer policy. Windows uses `CFSFolder` change notifications; on macOS/Linux
Qt polls the displayed filesystem folder and entries because upstream's POSIX
`WasChanged` returns false. The current selection and top visible item survive
refresh. Archive folders have no source `IFolderWasChanged`, so they require
manual refresh. Windows rendered reference and Windows/Linux runtime checks
remain outstanding.

The View menu now opens the source RootFolder through `IDM_OPEN_ROOT_FOLDER`;
backslash on the focused file list uses the same command. The Qt list presents
the provider's original item names and indices. On macOS, Computer opens `/`,
Parent from `/` returns to the virtual root, and Back returns through the prior
filesystem location. Typed relative and Unicode paths bind through CPanel.
The original Windows root has Computer, Documents, Network and Volumes; source
network enumeration is integrated, while Windows runtime/system icons remain
unverified. Root read-only
commands stay available while mutations are disabled. Current Qt geometry was
checked at 980×620 and 640×400; no rendered Windows comparison or Windows/Linux
runtime verification is claimed.

Current File/Edit/View/Favorites, toolbar and shared context commands use the
same native command dispatch as keyboard and panel activation. Read-only native
folders keep read commands and reject mutations; stale enabled actions cannot
bypass busy/closed, nested, encrypted or missing-writer restrictions. Open Inside
uses focus; ordinary/external open uses operated indices, including multiple
selected files. Toolbar switches retain native flags and save/reload order.
The window caption follows native CApp::RefreshTitle: focused folder prefix,
or `7-Zip` for an empty prefix. Menu size classification uses Qt system font
metrics and available screen geometry in the original dialog-unit calculation.
Qt widgets/icons and unavailable source commands remain the P0-09/P2-09 scope;
full source mutation/edit workflows remain P1-01/P1-03/P1-04/P1-08. Windows
rendered parity and Windows/Linux execution remain unverified locally.

P0-04.1 is accepted as an integrated prerequisite. Native component tests on macOS exercise PanelCopy into
disk/CVirtFileSystem, nested ZIP opening in memory and return to the same parent
folder, all-items extraction with cumulative progress, and changed-file update
after a stale item index. Prefix mismatch, missing item and released folder reject
the update without starting the engine. These are component integration checks;
MainWindow now uses the native parent stack for root/nested/address/parent
navigation; its older ArchiveParent stack is removed. Focused application
regressions cover TAR/GZip, unknown extension and nested Unicode/space ZIP,
externally updated archive refresh, selection and parent focus. After native
selection restoration, commands are refreshed explicitly because notifications
were disabled. Cancellation no longer presents an erroneous data-error result.
Nested archive addresses use their native virtual prefix; the existing Qt
internal-folder separator remains presentation only. Source parent navigation
returns to the archive's containing directory, and missing address suffixes bind
to their surviving ancestor. Root failure/cancel keeps the prior return-directory
workflow and records the backend result text in status/journal. Busy commands
and password/cancel assertions run during native modal opening. Full operation-state,
external-process/editor lifecycle and final application acceptance remain open.
The full formatting/build/CTest gate passes 119/119 on macOS arm64 (2026-09-29,
58.65 s). Native changed-nested return updates the outer ZIP on success and keeps
outer bytes plus changed temporary data after failed or cancelled update.
The 16 current archive-runtime JSON reports verify address/active view visibility
at normal/compact sizes across normal/flat and all four modes. This accepts the
prerequisite component, not full P0-04 command or rendered Windows parity.
The hash result dependency uses the native two-column ListViewDialog with
filename/size/digest, no initial selected row, native Delete flag and OK/Cancel.
Normal/compact current-build JSON confirms visible list and footer at 540×360
and 420×360. Native resourceGui STRINGTABLE now supplies previously missing
titles/messages; the current fixed Russian presenter translates checksum,
testing/moving/opening/scanning and no-error captions. Full language selection
remains P1-14. Full hash workflows remain P2-03, not completed by this dependency.
Rendered Windows parity and Windows/Linux runtime remain unverified.

P0-10.1 is accepted for the first Add/drop consumer: source modal/worker finish, confirmation with
YES/NO/CANCEL, temporary pause restoration and finish-during-prompt deferral are
implemented with focused native worker tests. Source Close after errors remains
available until acknowledged. Password callbacks now use the existing source
PasswordDialog on the GUI owner thread. On macOS Qt ignores QMessageBox window
titles; message bodies remain plain text, as required by the native workflow
([Qt window-title contract](https://doc.qt.io/qt-6/qmessagebox.html#setWindowTitle)).
First real ArchiveBackend consumer and process-control propagation are connected;
current 24-report contract/JSON review and full 101/101 gate passed on macOS.
No rendered Windows reference or Windows/Linux runtime verification is available.
Qt message-box button order follows the platform; source Yes default and
Cancel/Escape semantics are retained. Add/drop was the first modal consumer;
P0-10.2 connects the other current archive/filesystem/volume consumers.
External percentages update only bar/title; unavailable byte/file totals stay
empty. Native pause/background
failure is reported with state/policy rollback. A launch exception after process
start first quiesces the child, then uses source error composition.

## Аудит диалогов (2026-09-28)

Сверка выполнена с upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
Qt на macOS, checkout до изменений `66ecb5e78bede4a9573918afb1af95f9439db4f7`.
Прочитаны AGENTS.md, профильные skills, ресурсы и обработчики. Проектный
`check_ui_parity.py extract` извлёк Copy, Combo, Split, ListView, Edit, Extract
и Compress вместе с shared include/хешами в `build/dev/ui-reports/audit-*.reference.json`.
Compress extraction также содержит IDD_COMPRESS_OPTIONS; этот дополнительный
диалог пока не перенесён. Перед исправлениями все 28 выбранных CTest прошли:
успех локальных тестов не означает полного соответствия upstream.

| Диалог / эталон | Проверенный результат и оставшееся расхождение |
| --- | --- |
| Copy/Move, CopyDialog.rc/.cpp | Ресурсный контракт проходит: шесть controls, Copy/Move, 440×250 и 340×220. Адаптации: scrolling summary вместо static, Qt footer и Unicode ellipsis. |
| Имя/маска/создание папки, ComboDialog.rc/.cpp | Prompt → editable combo → OK/Cancel; начальное Value, raw OK, Cancel и геометрия 360×150/260×150 проверены. Пока нет отдельного ресурсного контракта. |
| Split/Combine, SplitDialog.rc/.cpp и PanelSplitFile.cpp | Destination/Browse → volume size → footer; error/normal и 2/3/5 частей, normal/compact проверены. Inline error заменяет MessageBox, папка выбирается native Qt dialog. |
| История/свойства, ListViewDialog.rc/.cpp | Один/два столбца, header, footer, Copy/Delete/Enter/detail и 540×360/360×240 проверены. Перенесена settings-driven one-click/hover ветка через Qt adapter; набор свойств ограничен snapshot. |
| Полное значение, EditDialog.rc/.cpp | Read-only multiline, без wrap, обе прокрутки, Enter в edit, Close; 480×320/320×240 проверены. |
| Extract, GUI/ExtractDialog.rc/.cpp | Destination, optional folder name, modes слева и password справа; name shown/hidden, 640×360/470×330, история/сохранение/отмена проверены. Ask открывает исходный OverwriteDialog с Yes/No/All/AutoRename/Cancel. Целый root, включая вложенный или зашифрованный архив, проходит source `ExtractGUI`/`Extract`; выбранные элементы и текущая папка — source FM callback. `7zz` поставляет байты через узкий адаптер. Явная запись каталога в «Без путей» остаётся пустой папкой по исходному callback. Отсутствуют абсолютные пути, ElimDup, NtSecurity и Help. |
| Create/Add, GUI/CompressDialog.rc/.cpp | Исправлены stacked layout и порядок основной формы: compression слева, encryption справа, source picker после archive/options. 760×720/640×480, 7z/ZIP, show/hide confirmation и reachability проверены. Это частичный порт: нет update/path modes, volumes, SFX, shared/delete options, parameters, CompressOptionsDialog и Help; Add не меняет compression options. |
| Запрос пароля, PasswordDialog.rc/.cpp | QInputDialog заменён source port CPasswordDialog: восстановлен Show Password и исходный ReadControls/SetTextSpec/OnOK. Контракт проверяет все пять controls в masked/shown при 360×170/260×170; Qt integration проверяет reuse/reset/cancel. |
| Настройки, OptionsDialog.cpp, SettingsPage2.rc и EditPage2.rc | CSettingsPage показывает семь list flags в исходном порядке, с AlternativeSelection из P0-07.3. Standalone 340×300/460×300 JSON сохранён; partial property sheet с Editor/Settings и общим OK/Cancel/Apply проверен при 640×390/520×350. Viewer/Editor/Diff сохраняют WasChanged/Browse flow. Memory controls и четыре недоступные страницы остаются обязательным scope; rendered Windows parity unverified. |
| Прогресс/сообщения, ProgressDialog2.rc/2a.rc/.cpp, MessagesDialog.rc/.cpp | Текущие архивные, файловые и томовые операции показывают исходный ProgressDialog со статистикой, ошибками, подтверждением отмены и Background. Pause реально управляет 7zz; для файловых/томовых workers отключена. Последние сообщения отображает исходный MessagesDialog с двумя колонками и Close; File menu открывает его modelessly как Qt-адаптацию. Геометрия 760×450/640×380 проверена на macOS, Windows rendered parity не подтверждена. |
| Информация об архиве / F3/F4 | Archive Info остаётся отдельной QLabel-формой. Disk/archive F3/F4 используют source внешний viewer/editor; ограниченный UTF-8 диалог оставлен отдельной дополнительной командой. Archive known/child-process lifetime и changed-file update подключены в P1-08.2. |

Copy и Password проверяются ресурсными контрактами с current-build fixture.
После исправлений `./scripts/check.sh` прошёл на macOS: formatting, сборка
приложения и все 51 CTest. Свежая extraction Copy полностью совпала с сохранённым
reference; оба resource comparator завершились без ошибок, по четыре отчёта.
Для остальных сопоставлены resources/source и свежие geometry JSON существующих
тестов; это не автоматическая проверка каждого RC control/state. Pixel parity,
tab order, Windows/Linux runtime и полный набор ветвей не проверены. Backend
недостающих режимов, прогресс и полные настройки требуют отдельных source ports;
декоративные неподключённые controls не добавлялись.

Избранное хранит исходный CFastFolders (FolderShortcuts): десять UI-слотов,
пустые промежуточные записи и полный путь панели. CPanel::SetBookmark и
OpenBookmark подключены к общему Qt navigateTo. Исчезнувшая папка внутри
уже открытого архива теперь выбирает существующего родителя, как история,
без повторного list. Неперенесённые временные вложенные архивы по-прежнему
не сохраняются в избранное. Старые настройки MVP history/paths и favorites/*
игнорируются; fallback не используется.

История папок использует исходный CFolderHistory: до 100 путей, последний
первым, удаление всех дублей без учёта регистра. Это заменяет предел 20
самостоятельного Qt MVP. Диалог следует
ListViewDialog.rc: список и OK/Cancel; Delete удаляет выделенные строки,
Ctrl+A выделяет все, Ctrl+C/Ctrl+Insert копируют полные пути. На macOS также
доступен Cmd. Enter/двойной щелчок подтверждают путь; Alt+Enter показывает текст.
Удаление сохраняется только после OK, Cancel оставляет историю неизменной.
Одноколоночный список проверяется при 540×360 и 360×240 по JSON/геометрии;
Windows rendering недоступен, визуальная идентичность не подтверждена.
Настройка одного щелчка действует и в общем списке истории/свойств.
Один щелчок подтверждает путь истории; Alt+щелчок и щелчок свойства показывают
полный текст. Ctrl/Shift/Meta сохраняют выбор без открытия. Наведение в течение
400ms выбирает всю строку без активации; leave/нажатие/модификаторы прекращают
ожидание. Qt использует фиксированный interval вместо системного Win32 hover time.
Основная файловая table/icon панель теперь использует тот же адаптированный
hover/track-select: 400 ms, без открытия, с отменой при Ctrl/Shift, drag,
leave и смене строки после scroll/removal. В 7zFM это системный Win32
track-select interval, а Qt использует фиксированное ожидание.
ShowDots добавляет исходный `..` в файловый и архивный список, но не в
виртуальный root. FullRow в табличном виде переключает окраску всей строки
либо только имени. На macOS/Linux ShowSystemMenu добавляет действие открытия
каталога в системном файловом менеджере; это уже, чем Windows shell menu.
Полная сверка контекстного меню остаётся P0-09. Альтернативное выделение
использует исходную модель отметок P0-07.3. Визуальная проверка двух состояний FullRow выполнена на текущем Qt
build; Windows rendered reference и Windows/Linux runtime не проверены.

Команды маски/типа/всех/инверсии теперь используют исходный индексный вектор
`CPanel::SelectSpec/SelectByType/SelectAll/InvertSelection`, включая выбор папок
по типу, последнюю точку имени и исключение строки `..`. Qt показывает
`CComboDialog` и переносит его принятую маску в исходный цикл; Cancel не меняет
выделение и фокус. Общая Qt selection model связывает все четыре режима списка.
Ctrl-click, Shift-click, Shift-arrow и Insert теперь используют исходные
`OnLeftClick`, `OnArrowWithShift`, `OnShiftSelectMessage`, `OnInsert` и отдельный
вектор отметок. Qt оставляет одну строку фокуса и показывает отмеченные строки
красным RGB(255,192,192), как upstream custom draw; table/icon делят модель.
Клавиатурная навигация значков следует геометрии Qt, поэтому стрелка вправо
переходит к соседнему значку, а в подробном/списочном виде вниз — к следующей
строке. Отметки сохраняются при смене вида и обновлении списка. Точные цвета,
отступы и пиксельное сходство с Windows не подтверждены без Windows runtime.
На macOS исходная desktop policy сопоставления масок без учёта регистра
восстановлена: Qt определяет `TARGET_OS_IPHONE=0`, и это больше не включает
iPhone-specific чувствительность к регистру.

Полное значение свойства и Alt+Enter истории открывают просмотр текста;
компоновка и взаимодействие описаны в [контракте](ui-layout-contracts.md#full-text-detail).

F5/F6 показывает перенесённую `CPanel::GetItemsInfoString`: число выбранных
папок/файлов, известные размеры, текущий путь и первые пять относительных
имён с многоточием при большем выделении. Неизвестный размер не подменяется
нулём: папки файловой системы до расчёта размера показывают только число.
Папки архива получают размеры через CAgentFolder: в normal это CProxyArc,
а в flat размер папки отсутствует по исходному GetProperty. Числа группируются пробелами
по оригинальному ConvertSizeToString из `PanelListNotify.cpp`.

Copy/Move summary reads the active source folder through original operated
indices and GetItemsInfoString. Missing sizes stay unknown, defined zero stays
defined, and flat archive folder totals remain absent. Cancel dispatches no operation.
Source summary structure was checked in 16 normal/compact reports on macOS;
Windows/Linux runtime and rendered Windows parity remain unverified.

Диалог F5/F6 перенесён из `CopyDialog.cpp/.h`: назначения выбираются из
редактируемой истории, Browse нормализует конечный разделитель, Cancel
оставляет Value без изменений. История CopyHistory читает только исходный
ключ; MVP copy/history игнорируется. Совпадения удаляются без учёта регистра,
предел — 20.
Список выделения прокручивается и не переносит строки; это адаптация
SS_LEFTNOWORDWRAP для длинных имён. Структура проверена по `CopyDialog.rc`
и геометрии Qt; пиксельная сверка с Windows и проверка Windows/Linux не выполнены.

Копирование дерева файловой системы использует перенесённые `CopyFolder`
и read/write ветку `CopyFile_Ask` из `FSFolderCopy.cpp` (`0766b73`).
В Qt занятые имена вызывают исходный OverwriteDialog с Да/Нет/для всех,
автоматическим переименованием и отменой. Решение для всех хранится лишь на
время операции. Qt worker ожидает ответ без обращения к widget, а при отмене
запрос закрывается. Заменённый объект держится как backup до успешного
завершения всей операции; при ошибке откатываются уже опубликованные объекты.
Совпадающие папки сливаются через исходный `CopyFolder`; для вложенных
занятых файлов исходный `AskWrite` показывает OverwriteDialog. Слияние
собирается во временной папке, поэтому отказ или отмена до удаления сохраняют
прежнюю цель. F6 удаляет лишь записанные файлы, а пропущенные остаются в
источнике. Ссылки и несовпадающие типы file/directory запрещены. P1-02.1
добавил staged fallback для F6 при отказе rename между томами.

## P1-02.1 — F6 при отказе rename между томами (принято, 2026-10-01)

Обычный F6 сначала пробует перенести источник во временное имя рядом с целью.
Если это невозможно, источник копируется через перенесённый `FSFolderCopy`
copy body, цель публикуется, и лишь затем источник удаляется. Занятая цель
сохраняет старые байты до ответа исходного OverwriteDialog. Отмена до удаления
откатывает опубликованные цели. Если удаление источника не удалось после
публикации, команда сообщает ошибку и возможный частичный результат, а новая
цель сохраняется, чтобы не потерять байты. Инъекция границы томов проверена
локально; настоящий второй том, Windows/Linux runtime и visual reference
не проверены. Исторические ограничения этой итерации сняты в P1-02.2 ниже.

## P1-02.2 — исходный FSFolderCopy и слияние папок (принято, 2026-10-01)

`CFSFolder::CopyTo` принимает прямое имя файла/новой папки и каталог с
завершающим разделителем, считает объём и вызывает исходные `CopyFile_Ask`,
`CopyFolder`, `CCopyState::CopyFile_Sys/MoveFile_Sys` через callback. Для
занятой папки обычный F5/F6 теперь сливает содержимое с отдельными решениями
по вложенным файлам. Старое дерево сохраняется до публикации слияния; отказ
или отмена восстанавливают его. F6 сохраняет пропущенные исходные файлы и
сообщает об этом. Alternate streams и ссылки отклоняются; их перенос остаётся
P2-05/P2-06. Normal/compact main и panel status JSON, прямой двухпанельный
UI consumer и полный `check.sh` 143/143 прошли на macOS arm64/Qt 6.11.2.
Windows/Linux runtime и rendered Windows reference не проверены.

## P1-06.2 — открытие, тест и извлечение томов

Первый `.001` доступен в стандартном archive filter и открывается исходным
panel binding workflow. Список показывает содержимое архива. «Тест» теперь
использует source `CPanel::TestArchives`: выбранные элементы либо все при
пустом выборе, пароль последнего archive link, общий source progress/results.
Ask extraction использует source GUI/Common callbacks; следующие тома batch
не извлекаются повторно. Неверный пароль проверяется до замены destination.
Отсутствующий том, повреждённые данные и отмена показываются в существующих
source error/progress views. Тома доступны для чтения; изменение набора
не поддерживается официальным CLI adapter и commands отключены.
Пауза/отмена и формат source progress не менялись. Native picker фильтр проверен
через Qt dialog; Windows/Linux runtime и rendered Windows reference непроверены.

## P1-06.1 — создание архивных томов (2026-10-01)

Add to Archive предлагает editable «Разбить на тома, байт» с исходными девятью
presets и произвольной последовательностью размеров; пустое поле создаёт один
архив. Последний размер повторяется, номера начинаются с `.001`. Исходный
CompressDialog OnOK проверяет размер и спрашивает Yes/No/Cancel ниже 100 KiB;
No/Cancel не сохраняют профиль. Существующий архив не предлагает создание томов.
ZIP/7z и пароль поддерживаются. Занятый numbered target не заменяется;
ошибка/отмена удаляет свои staged outputs. Вторая стадия показывает
«Создание архивных томов» и поддерживает паузу/отмену.

Qt размещает combo в прокручиваемой форме после compression controls;
CompressDialog.rc задаёт control/label и группу, но не этот адаптивный размер.
JSON `archive-volumes-760/640` подтверждает label/combo полностью видимыми,
девять presets, порядок и отдельный доступный footer на 760×720 и 640×480.
Для внешнего `7zz` сначала создаётся полный временный архив; требуется
дополнительное дисковое место. Windows/Linux runtime и rendered Windows
reference не проверены. Open/test/extract приёмка остаётся P1-06.2.

Текущие группировки Create/Extract и адаптации к Qt описаны в
[текстовых спецификациях окон](ui-layout-contracts.md). `ui_geometry`
проверяет порядок, доступность полей при прокрутке и кнопки при нормальном
и компактном размере; JSON-отчёты заменяют обязательные PNG в мигрированных
тестах. Геометрические проверки не подтверждают точное визуальное совпадение
с Windows. Скриншоты используются для неразрешённых вопросов рендеринга.

| Узел 7zFM | Исходник | Состояние Qt и точное расхождение |
| --- | --- | --- |
| Две группы панели Add/Extract/Test и Copy/Move/Delete/Info | `App.cpp`, `g_ArchiveButtons`, `g_StandardButtons`, `ReloadToolbars()` | Порядок, разделитель, цветные символы и подписи перенесены. Символы рисуются средствами Qt; bitmap-ресурсы upstream не включены в проект. |
| Настройки панели: две группы, крупные кнопки, подписи | `App.h`, `ReadToolbar()`, `SaveToolbar()`; `resource.rc`, View → Toolbars | Четыре переключателя в меню «Вид», сохранение в локальном INI через `QSettings`. |
| Заголовок списка и видимые свойства | `PanelItems.cpp`, `GetColumnVisible()`, `ShowColumnsContextMenu()` | Имя, размер, сжатый размер и изменение показаны без служебного полного пути и номеров строк; «Сжатый» скрыт в обычной папке. Видимость доступных столбцов настраивается из заголовка и сохраняется. Набор свойств пока фиксирован, а в 7zFM зависит от папки и формата. |
| Сравнение и сортировка | `PanelSort.cpp`, `CompareFileNames_ForFolderList()`; `resource.rc`, View → Arrange | Сравнение имён перенесено в `src/CPP/7zip/UI/FileManager/PanelSort.cpp`: `2.txt` идёт перед `10.txt`, сравнение без учёта регистра. Qt-таблица вызывает эту функцию из `src/filetable.cpp`. Меню и Ctrl+F3–F7 выбирают имя, тип, дату, размер или исходный порядок. Для архива исходный порядок берётся из загрузки элементов `CAgentFolder`, для обычной папки — из перечисления ОС. Правила `CompareItems()` и `SortItemsWithPropID()` сверены с upstream `0766b73`: папки остаются перед файлами в обоих направлениях, повторный выбор свойства меняет направление, размер/сжатый размер/дата начинают с убывания; свойство и направление сохраняются при обновлении и переходах. В плоском виде сравниваются базовое имя, префикс папки и индекс загрузки, а не отображаемый относительный путь; расширение включает последнюю точку, в том числе у dotfiles и папок (`FSFolder.cpp::GetExtensionPtr`, `Agent.cpp::GetExtension`). Точное сравнение всех свойств и режим сортировки двух панелей 7zFM ещё не перенесены. |
| Контекстное меню списка | `PanelMenu.cpp`, `OnContextMenu()`, `CreateFileMenu()` | Состав зависит от выделения и режима каталога/архива; добавлены команды фона и свойства одного объекта. Нет расширений оболочки Windows, 7-Zip submenu и полного набора операций. |
| Свойства выделения | `PanelMenu.cpp::Properties`, `ListViewDialog.cpp`, `UI/Agent/AgentProxy.cpp::CalculateSizes` | Двухколоночный список свойств, сводка и вложенные counts используют перенесённые методы. Для папок архива размеры берутся из CProxyArc; сводка суммирует все выбранные строки по upstream, включая выбранных потомков. Рекурсивный размер папок файловой системы, форматные и сырые свойства 7zFM пока не включены. |
| Клавиатура списка | `PanelKey.cpp`, `OnKeyDown()` | Enter открывает папку, архив или файл; Backspace, F2–F7, Delete/Shift+Delete, Ctrl/Cmd+A, Ctrl+N, Alt+F12, Ctrl+F3–F7 и обновление работают в реализованных режимах. F3/F4 disk/archive файла запускают external viewer/editor по native focused index. F3 folder-size disabled до P1-10. |
| Главное меню | `resource.rc`; `MyLoadMenu.cpp`, `OnMenuActivating()` | Работают основные «Файл», «Правка», «Вид», «Избранное», включая View/Edit и Open Inside/Outside с ограничениями ниже. Оригинал формирует File динамически и содержит больше команд: Diff, Link и Alternate Streams; также есть Tools и Help. |
| Контрольные суммы | `resource.rc`, `PanelCrc.cpp`, `MyLoadMenu.cpp` | Подменю CRC содержит все 11 методов оригинала. Для выделенных файлов и папок файловой системы запускается `7zz h`; выбранные записи архива сначала извлекаются через backend во временную папку. Вывод и итог доступны в журнале, отмена останавливает текущий этап. В Windows 7zFM отдельное окно итогов и потоковое хеширование архива без временных файлов. |
| Открытие, просмотр и правка файла | `PanelItems.cpp`, `EditItem()`; `PanelItemOpen.cpp`, `StartEditApplication()`/`OpenItemInArchive()` | Enter/двойной щелчок используют системное приложение; архивный файл извлекается во временную папку с source lifetime. Disk/archive F3/F4 используют настроенную внешнюю программу без ограничения размера/UTF-8. Windows сохраняет default notepad.exe; POSIX пустой viewer использует association, editor требует настройки. Known/child processes отслеживаются; после правки Yes обновляет архив, No/Cancel удаляют копию, read-only/error сохраняют её. Короткий unchanged process сохраняет copy до закрытия owner; активный process продолжает работать после закрытия окна. POSIX inherited group сохраняет lineage orphan child; daemon, покинувший группу, может не обнаружиться. Дополнительные текстовые команды Qt сохраняют прежние ограничения. VerCtrl подключён в P1-08.3. |
| Ver Edit/Commit/Revert/Diff | `VerCtrl.cpp`, `MyLoadMenu.cpp` | Исходные динамические пункты добавляются в конце File/context меню для одного filesystem файла при настроенных Diff и `FM/7vc`. Readonly файл допускает Edit, writable — Commit/Revert/Diff. Edit хранит backup и нумерованную историю, Commit обрабатывает mtime и ставит readonly; Revert с другими данными использует source OverwriteDialog с default No, без extra buttons. На POSIX readonly убирает write bits, снятие readonly возвращает owner-write; creation time не полностью переносим (Linux stat ctime), Windows использует оригинальные API. Source size limit256 MiB и partial-write/error semantics сохранены. Upstream не предоставляет UI записи `7vc`; отдельная настройка не придумана. |
| Открыть внутри / снаружи | `resource.rc`, `MyLoadMenu.cpp`, `PanelItemOpen.cpp::OpenItem()` | Ctrl+PgDn открывает папку или выбранный файл внутри панели, включая вложенные архивы после извлечения во временную папку. Неизвестное расширение при явном открытии проверяет `7zz` по содержимому; обычный двойной щелчок продолжает запускать системное приложение. Цепочка `.tar.gz` открывается последовательным переходом в TAR. Shift+Enter открывает обычный или извлечённый файл системным приложением. Переход назад из корня вложенного архива восстанавливает родительскую папку и выделение. Архивы кроме `.7z`/`.zip` и все вложенные архивы только для чтения. Выбор альтернативного обработчика через `*`/`#` ещё отсутствует. |
| Панели и режимы списка | `App.cpp`, `Panel.cpp`, `SetListViewMode()`; `resource.rc`, View | Крупные/мелкие значки, список и подробности доступны через Ctrl+1–4; режим сохраняется. Все представления делят модель и выделение, поэтому команды действуют на те же записи. Двухпанельный режим ещё отсутствует; раскладка значков Qt может отличаться от Windows ListView. |
| Плоский просмотр | `Panel.cpp`, `ChangeFlatMode()`; `FSFolder.cpp`, `LoadSubItems()`; `resource.rc`, View → Flat View | Вложенные файлы и папки показаны единым списком с относительными путями; режимы файловой системы и архива сохраняются отдельно. Для диска Qt рекурсивно перечисляет записи порциями без прохода по ссылкам; переключение режима или переход в другую папку прерывает обход. Для архива добавляются неявные промежуточные папки. Выделение и операции используют исходные полные пути. Перечисление большого дерева и окончательная сортировка по-прежнему выполняются в потоке UI, но порции ограничены по времени. |
| Адрес и история | `Panel.cpp`, `PanelFolderChange.cpp`, `FoldersHistory()`; `ListViewDialog.rc` | CFolderHistory хранит до 100 каталогов, корней и внутренних папок архивов в INI. Выбор записи восстанавливает папку без повторного чтения уже открытого архива; после нового чтения исчезнувшая подпапка заменяется ближайшей существующей родительской. Временные пути вложенных архивов в историю не записываются. Перенесённый CListViewDialog (Alt+F12) позволяет удалить выделенные записи через Delete, скопировать пути и перейти к текущей; история «Назад» отдельная. Qt показывает границу архива как ` / `; Windows использует разделители пути и системный ListView. |
| Создание файла и выбор по маске/типу | `PanelOperations.cpp`, `CreateFile()`; `PanelSelect.cpp`, `SelectSpec()`/`SelectByType()`; `ComboDialog.rc` | Команда Ctrl+N создаёт пустой файл в обычной папке через редактируемый диалог; в архиве отключена. Правила выбора перенесены в `src/CPP/7zip/UI/FileManager/PanelSelect.cpp`, сопоставление маски — в `src/CPP/Common/Wildcard.cpp`. «Правка» выделяет или снимает выделение по нечувствительной к регистру маске `*`/`?` либо по расширению текущего файла (для папки — все папки). Qt-диалог повторяет поля ComboDialog, но не хранит историю введённых масок. |
| Закладки | `PanelFolderChange.cpp`, `SetBookmark()`/`OpenBookmark()`; `PanelKey.cpp`; `resource.rc`, Favorites | Меню «Избранное» хранит 10 слотов в локальном INI, включая текущую папку архива. Alt+Shift+цифра записывает слот, Alt+цифра открывает его при фокусе таблицы. На Windows 7zFM также поддерживает правый Ctrl с цифрой; здесь используется Alt. |
| Архивные операции | `App.cpp`, `Panel.cpp`, диалоги FileManager | Qt поддерживает создание, добавление в открытый архив, извлечение, тест и основные изменения через `ArchiveBackend`. В Windows сценарии выбора файлов и форматы диалогов шире; здесь действуют ограничения из README. |
| Диалог Add to Archive: сжатие и шифрование | `../GUI/CompressDialog.rc`, `CompressDialog.cpp` | Маски уровней и таблицы методов перенесены из upstream `CompressDialog.cpp`: у 7z уровни 0–9, у ZIP — 0, 1, 3, 5, 7, 9; в ZIP доступен также PPMd. UI и backend используют общие таблицы. Потоки выбираются из исходного ряда с пределами метода/CPU, Auto учитывает память; показаны оценки сжатия и распаковки. Для LZMA/LZMA2 доступны словарь и размер слова, для Deflate/Deflate64 — размер слова, для PPMd — память и порядок, для BZip2 — словарь; для 7z — Auto/Non/Full solid и размер блока. Несовместимые параметры отключаются и отклоняются backend. Пароль вводится дважды; Show Password скрывает повтор. 7z использует AES-256 с отдельным шифрованием имён, ZIP — ZipCrypto/AES-256. Выбор источников остаётся отдельной верхней частью Qt-диалога. Для компактного окна источники и параметры прокручиваются вместе, кнопки подтверждения остаются видимыми. Пока нет томов, режима обновления, режима путей, SFX и произвольных параметров. При добавлении в существующий архив эти параметры не меняются. |
| Диалог Extract | `../GUI/ExtractDialog.rc`, `ExtractDialog.cpp`; `../GUI/Extract.rc` | Перенесены поле назначения с историей из 16 путей, кнопка выбора папки, полные пути или режим без путей, отказ от перезаписи, пропуск, перезапись и оба режима переименования при конфликте, поле пароля с показом. Новая папка назначения создаётся после проверки существующих родителей. Совпадающие имена при извлечении без путей отклоняются до запуска. В upstream доступны также абсолютные пути и запрос решения при конфликте; здесь они ещё не реализованы. Имя подпапки включается отдельным флажком и сохраняется; устранение дублирования корневой папки и восстановление прав пока отсутствуют. |
| Разбить файл / объединить части | `PanelSplitFile.cpp`, `SplitDialog.rc` | Нумерация частей использует перенесённый `CVolSeqName` и `SplitUtils.cpp` (`ParseVolumeSizes`, `GetNumberOfVolumes`, `AddVolumeItems`) в `src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp`. В обычной папке доступны команды для одного файла. Диалог Split содержит папку назначения и редактируемый размер тома; как `CSplitDialog::OnOK()` и `CThreadSplit::ProcessVirt()`, принимает последовательность размеров, применяет их по порядку и повторяет последний для оставшихся частей; Combine принимает первую часть по исходному `CVolSeqName::ParseName` — суффикс `01` с любым числом ведущих нулей, с точкой или без неё, проверяет непрерывность и собирает файл в выбранную папку. Операции идут в рабочем потоке с отменой и временными файлами; файлы с существующими именами не заменяются. Диалог Combine показывает число частей, общий размер в байтах, исходную папку, первые две и последнюю часть, как `CApp::Combine()`. Проверку списка и суммы выполняет `VolumeOperation::inspectParts`; рабочий поток повторно проверяет части перед сборкой. |
| Диалог Copy/Move | `CopyDialog.rc`, `CopyDialog.cpp`; `App.cpp`, `Copy()` | F5/F6 открывают общий диалог с редактируемой историей назначения до 20 путей, выбором папки и именами объектов. Рабочий поток показывает текущий файл, принимает отмену и сохраняет прежнюю цель до публикации. Обычный `CApp::OnCopy` маршрутизирует операции между двумя файловыми или верхними архивными панелями, включая disk↔archive и archive↔archive; F6 из архива удаляет источник после успешной записи цели. При отказе rename F6 копирует во временную цель и удаляет источник после публикации, сообщая о частичном результате при ошибке удаления. Совпадающие дисковые папки сливаются с отдельными решениями по вложенным файлам. Одиночный новый filename и Shift+F5/F6 для элемента под фокусом поддержаны. Вложенные и защищённые архивы имеют явные ограничения. |
| Сервис → Настройки | `resource.rc`, `OptionsDialog.cpp`, `SettingsPage2.rc`, `EditPage2.rc`, `FoldersPage2.rc`, `MenuPage2.rc`, `LangPage.rc` | Partial six-page container сохраняет source order/IDs; Folders/Editor/Settings, 7-Zip и Language доступны, System disabled. Apply/OK пишут изменённые source values, Cancel сохраняет уже применённое. Language Apply обновляет source menus/toolbar/property headers/status и Options labels без очистки input/selection; missing/malformed fallback — source English. Qt standard buttons сохраняют platform conventions. WorkDir/RAM, external editors и Windows Zone имеют consumers. Shell controls остаются disabled до P2-09; полная интеграция страниц остаётся P1-12.2/P1-13.2/P1-14/P2-08. |

## Следующие крупные блоки

Создание папки использует перенесённый `Dlg_CreateFolder` и общий
`CComboDialog`, с исходным default «Новая папка». Создание файла и маска
выделения используют ту же форму. OK возвращает сырой текст, Cancel не
подтверждает изменение; ограничения имени проверяет операция. Компоненты
следуют ComboDialog.rc, геометрия проверена в нормальном/компактном размере;
точное визуальное совпадение Windows остаётся непроверенным.

Split перенесён в `CSplitDialog` с исходным порядком destination/volume
и именем источника в title. Combine использует общий `CCopyDialog`, как
`CApp::Combine`, с исходной сводкой в прокручиваемой области. Назначение
может быть новой папкой: создаётся после OK и дополнительного подтверждения
Split для >=100 частей. Отмена dialog/подтверждения папку не создаёт.
Ошибка размера остаётся inline вместо MessageBox. Структура и компактная
геометрия проверяются JSON/тестами; rendered Windows reference отсутствует,
pixel parity не подтверждена. Ограничение 10000 частей и отказ занятых имён
сохраняются как интеграционные отличия.

Создание папки/файла и Rename используют исходные `CPanel` проверки имени,
`CorrectFsPath` и `CThreadFolderOperations` из `PanelOperations.cpp`/
`BrowseDialog.cpp` (`0766b73`), а файловые действия — `FSFolder.cpp`.
F2 редактирует имя прямо в строке или значке во всех четырёх режимах списка;
Enter подтверждает, Escape отменяет, не открывая объект. Папка принимает составной путь, файл — путь с уже
существующими родительскими папками. Исходный Windows path normalizer сохранён;
на POSIX `CorrectFsPath` возвращает ввод без преобразования, как upstream.
Qt-таблица перезагружается после результата и восстанавливает выбор; при
создании вложенной папки выбирается её первый компонент в текущем списке.
Создание по-прежнему использует Qt-адаптацию `ComboDialog` с порядком полей
`ComboDialog.rc`; normal/compact геометрия проверяется контрактом.
Обычный Delete передаёт объекты в системную корзину через
`QFile::moveToTrash`; Shift+Delete вызывает исходный `CThreadFolderOperations`
и постоянное `FSFolder::Delete`. Подтверждение `PanelOperations.cpp` различает
один файл, папку и несколько объектов. Это явная Qt-адаптация: в исходном
Windows пути подтверждение корзины показывает Shell, а у Qt native Trash
такого диалога нет. Исходный worker поддерживает отмену при обходе дерева и
сохраняет частичный результат. Панель обновляется после успеха, ошибки и
отмены. Вложенные symlink удаляются сами, их цели не обходятся;
непосредственно выбранные ссылки отклоняются. Диалоги ввода имени остаются
Qt-адаптацией. Рендеренный Windows-референс inline-редактирования и
подтверждений недоступен; pixel parity не подтверждена.

1. Для P0-08.1 перенесены disk/disk F9 и отдельные представления; mixed archive и назначения межпанельных операций из `App.cpp` остаются P0-08.2/P1-01.
2. Доработать раскладку режимов просмотра, сортировку по свойствам и динамические столбцы по `PanelItems.cpp`, `PanelSort.cpp` и `resource.rc`; для очень больших деревьев вынести обход и сортировку из потока UI.
3. Переносить команды File и контекстного меню с их состояниями доступности из `PanelMenu.cpp` и `MyLoadMenu.cpp`. Для Open Inside/Outside и обратной записи после внешнего редактирования определить безопасную временную распаковку и обновление архива.
4. Расширить постоянную историю адресов, диалоги свойств и настроек. Платформенные отличия системных меню и интеграции проверять отдельно на Windows, macOS и Linux.

Функциональные правила `CPanel::SelectSpec`, `SelectByType`, `SelectAll` и
`InvertSelection` из `PanelSelect.cpp` (0766b73) выделены в `CPanelSelection`
с теми же именами методов. Маска применяется и к папкам; выбор по типу папки
выбирает папки, а тип файла определяется последней точкой и не выбирает папки.
Имена без точки образуют отдельную группу; выбор вне группы сохраняется.
В плоском просмотре маска и тип используют базовое имя (`GetItemName()`),
а не отображаемый относительный путь: точки в родительских папках не меняют
тип файла, точная маска имени работает на любой глубине.
`mainwindow_selection.cpp` владеет диалогом, чтением текущего индекса и
применением результата к общей модели таблицы и списка значков. Диапазоны
мышью/клавиатурой обрабатывает Qt; альтернативный `_mySelectMode` 7zFM пока
отсутствует.

`PanelKey.cpp::OnKeyDown()` (`VK_ADD`, `VK_SUBTRACT`, `VK_MULTIPLY`)
перенесён в `mainwindow_selection.cpp::handleSelectionKey()`. Цифровые
`+`/`−` вызывают маску, Shift — выбор всех, Alt — выбор по типу,
`*` — инверсию. Приоритет Alt над Shift и подавление Ctrl+Plus следуют
upstream. Адаптация Qt использует `KeypadModifier`, не перехватывает ввод
в адресной строке и ограничивает команды их текущей доступностью.
Таблица и список значков используют один обработчик; фактические события
цифровых клавиатур Windows/Linux ещё требуют платформенной проверки.

`CPP/7zip/UI/FileManager/PanelKey.cpp::handlePanelKey()` объединяет Qt-адаптер команд
`PanelKey.cpp::OnKeyDown()` и `Panel.cpp::CMyComboBoxEdit::OnMessage()`.
Shift+F4 вызывает существующую CreateFile только при фокусе списка и
доступности команды. Alt+F1/F2 из списка или адреса раскрывают историю,
как `App.cpp::CPanelCallbackImp::SetFocusToPath()`; обе клавиши используют
активную панель и в disk/disk P0-08.1; раздельный выбор левой/правой цели
остаётся P0-08.2. Tab из поля
адреса возвращает фокус в активный режим просмотра без навигации.
Win32 ComboBox заменён редактируемым Qt ComboBox и его popup; модификаторы
проверяются точно, чтобы не перехватывать другие сочетания.

В режиме открытия одним щелчком Qt-жесты Ctrl/Shift для множественного
выделения (и Meta как платформенный модификатор) не активируют запись.
`mainwindow_panel.cpp::activateSingleClickedRow()` отделяет выбор от открытия
для таблицы и списка значков. Upstream включает `LVS_EX_ONECLICKACTIVATE`
в `App.cpp::SetListSettings()` и получает активацию через Win32
`LVN_ITEMACTIVATE` (`PanelListNotify.cpp`); Qt вместо этого получает каждый
щелчок через `clicked`, поэтому явно исключает жесты выделения.
Это адаптация Qt, а не перенос Win32 уведомлений. Поведение hover/track-select
`LVS_EX_TRACKSELECT` пока не перенесено. Описание исходных стилей:
[Microsoft List-View Styles](https://learn.microsoft.com/en-us/windows/win32/controls/extended-list-view-styles).

В списке значков Qt теперь выделяет полные строки общей модели, как таблица:
мышь с Ctrl/Shift формирует те же выбранные пути и состояния команд.
Раньше выделялась только колонка имени, невидимая для `selectedRows()`;
проверки, устанавливавшие выбор через таблицу, не обнаруживали этот разрыв.

`PanelKey.cpp::OnKeyDown()` обрабатывает `VK_BACK` без проверки модификаторов.
Qt-адаптер теперь следует этому правилу при фокусе списка, во всех четырёх
режимах. Поле адреса сохраняет редактирование текста; во время загрузки
и операций переход отключён состоянием команды. Файл адаптера расположен
по исходному пути `src/CPP/7zip/UI/FileManager/PanelKey.cpp`; Win32 уведомления
заменены Qt event filter. Остальные ветви оригинала ещё перенесены не полностью.

Combine последовательно читает части и проверяет полную запись, изменение
размера источника и ошибки завершения записи. Отмена доступна до первого
чтения и во время работы. Временный результат публикуется только при успехе;
ошибка или отмена убирает его.

Имя результата Combine берётся из `CApp::Combine()` (`0766b73`): от
`UnchangedPart` убираются все завершающие точки; для пустого имени используется
`file`. Этот блок выделен в `CVolSeqName::GetCombinedName()` и вызывается UI.
Доступность Combine и обнаружение частей используют один `ParseName`, без
дополнительного ограничения регулярным выражением `.001`. Проверка пропусков,
символьных ссылок и непустых частей остаётся адаптацией Qt.

Split использует заданную последовательность размеров и повторяет последний
размер для оставшихся частей. Изменение размера источника и ошибки записи
завершают операцию с откатом временных файлов. Предварительное выделение
места под том не выполняется; файл записывается последовательно.

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
показывают исходные MB/GB. Расчёт Auto и Full Solid перенесены последующей итерацией; Auto оставляет выбор движку.

Диалог сжатия ZIP теперь включает размер слова Deflate и Deflate64 по
`SetOrder2` upstream: 8–258 / 8–257, Auto 32/64/128 для уровней ниже 7,
7 и 9. Явный выбор сохраняется при смене уровня; Store отключает поле.
Qt сохраняет Auto как 0 и оставляет значение движку, вместо Win32 sentinel -1.
Расположение поля остаётся прежним; проверены снимки 760×720 и 640×480
на macOS offscreen. Windows/Linux и физический native UI не проверены.

PPMd в 7z и ZIP теперь позволяет выбрать порядок по соответствующим веткам
SetOrder2: 2–32 (исходный разреженный ряд) и 2–16. Auto показывает порядок
в зависимости от уровня; при явном выборе движок получает `o`, для LZMA и
Deflate сохраняется `fb`. Словарь/память PPMd пока не доступны для явного
выбора. Диалоги проверены по снимкам 760×720 и 640×480 на macOS offscreen;
native UI Windows/Linux остаётся непроверенным.

Словарь LZMA/LZMA2 следует SetDictionary2: 64 КиБ, 256 КиБ, 1 МиБ,
2 МиБ и исходный ряд степеней двойки с промежуточными значениями до
3840 МиБ на 64-bit (до 64 МиБ на 32-bit). Auto показывает исходную
формулу для уровня и архитектуры, а значение выбирает движок. Пользовательский лимит RAM добавлен последующей итерацией; автоматические ограничения
потоков и отказ по памяти добавлены последующей итерацией.
Размеры отображаются в исходных KB/MB по Combo_AddDict2; снимки 760×720 и
640×480 проверены на macOS offscreen. Windows/Linux и 32-bit не проверены.

PPMd для 7z теперь предоставляет исходный ряд памяти от 1 МиБ до 1024 МиБ
на 64-bit (512 МиБ на 32-bit); Auto показывает 16 МиБ при уровне 5,
256 МиБ при уровне 9. Поле остаётся Dictionary size перед Word size,
как в CompressDialog.rc; подписи сохраняют исходные MB. Выбор передаётся
7zz через mem=<bytes>b. Память ZIP PPMd и автоматический лимит RAM добавлены последующими итерациями.
Снимки memory-PPMd-7z-760/640 проверены на macOS offscreen; другие ОС
и 32-bit остаются непроверенными.

Явная память ZIP PPMd теперь доступна: 1,2,4,8,16,32,64,128,256 МиБ,
Auto=1<<(level+19), как в отдельной ветке PPMdZip upstream. Ряд не включает
3 МиБ из 7z PPMd. Поле стоит перед размером слова по CompressDialog.rc.
Снимки memory-PPMd-zip-760/640 просмотрены на macOS offscreen; Windows/Linux
и аппаратные ограничения памяти не проверены.

BZip2 в 7z и ZIP теперь предоставляет Dictionary size 100–900 КиБ
с шагом 100 КиБ. Auto показывает 100 КиБ на уровне 1, 500 на уровне 3,
900 при уровне >=5, как в исходной ветке. Размер слова остаётся отключён.
Расположение контролов сверено с CompressDialog.rc; снимки
memory-BZip2-{7z,zip}-{760,640} просмотрены на macOS offscreen.
Windows/Linux и RAM limits не проверены.

Deflate/Deflate64 показывают фиксированный автоматический словарь 32/64 КиБ
во всех уровнях ZIP; ручной выбор отключён, как в ветке upstream с одной
строкой Auto. Снимки memory-Deflate/Deflate64-zip-760/640 проверены.

Управление solid-блоком теперь использует один combo по CompressDialog.rc:
Auto с вычисленным размером, Non-solid, 1 МиБ–64 ГиБ, Full solid.
Отдельный Qt checkbox убран. Auto меняется при выборе метода/словаря/уровня;
Full и Non-solid сохраняются при смене словаря. Данные и параметры
операции следуют SetSolidBlockSize2/OnOK; Auto не задаёт -ms, Full передаёт
UInt64(-1) байт. ZIP и Store отключают список. Снимки всех трёх режимов
при 760×720 и 640×480 проверены на macOS offscreen; Windows/Linux не проверены.

Потоки CPU теперь следуют SetNumThreads2: ряд ограничен методом и числом
аппаратных CPU×2, Auto показывает число потоков с учётом памяти ZIP/LZMA2.
Для 7z PPMd/Store доступен только Auto(1); ZIP использует отдельный предел
32/128 по архитектуре. Справа показано процессное/аппаратное число CPU.
Под строкой потоков стоят Memory usage for Compressing / Decompressing
по порядку CompressDialog.rc. Оценка — алгоритм 7zFM, а не измерение RSS
процесса. Превышение исходного 80% RAM отклоняется до запуска. Выбор
другого лимита памяти и Registry пока отсутствует. Формы PPMd, ZIP и LZMA2
просмотрены при 760×720 и 640×480 на macOS offscreen; Windows/Linux,
32-bit и CPU groups не проверены.

Лимит памяти Add to Archive следует SetMemUseCombo/AddMemComboItem upstream
0766b73: Auto=80%, 10–100% с шагом 10 и ряд размеров 256/384/512/768 МиБ
и далее до 16 ТиБ на 64-bit (до 4 ГиБ на 32-bit). Исходные формулы и
архитектурный предел сохранены. Поле расположено в строке памяти для сжатия
перед памятью для распаковки, как в CompressDialog.rc; Qt выравнивает
оценку и список горизонтально, RAM/предел в байтах доступны в подсказке.
Предел пересчитывает Auto-потоки ZIP/LZMA2, явные потоки сохраняются.
Отказ при нехватке памяти оставляет диалог открытым; backend повторяет
проверку до создания stage и передаёт memuse движку. Добавление в
существующий архив сохраняет прежние параметры, как до этой итерации,
и не показывает новые контролы. Лимит хранится отдельно для форматов и сохраняется при OK между
запусками (CFormatOptions; Qt INI вместо Windows Registry). Отмена и
ошибка валидации сохраняют прежние настройки. Нестандартные значения
из сохранённого spec вставляются в исходный ряд, неверный spec даёт Auto.
Снимки memory-limit-760.png и -640.png проверены на macOS offscreen.
Windows/Linux и 32-bit требуют отдельных проверок.

Диалог извлечения теперь использует перенесённые OnInit/OnOK CExtractDialog
(0766b73). Порядок перезаписи исправлен на исходный: отказ (адаптация Ask),
перезапись, пропуск, переименование извлекаемого, переименование существующего.
История ограничена 16 элементами исходным кодом, при подтверждении выбранный
путь перемещается в начало, сравнение уникальности без учёта регистра.
Путь назначения получает конечный разделитель, как в 7zFM. Режимы и флажок
показа пароля сохраняются при OK; Cancel не меняет INI, пароль не сохраняется.
SplitDest добавлен последующей итерацией. Absolute paths, интерактивный
Ask, ElimDup и NtSecurity пока недоступны. Qt controls сохраняют две колонки ExtractDialog.rc; визуальные
снимки extract-port-640.png и -470.png проверяются на macOS offscreen.

SplitDest диалога Extract перенесён из OnInit/OnOK 0766b73. По умолчанию
включён: родительская папка в combo, последний компонент (включая конечный
разделитель) в Edit. Корень и одиночный относительный путь не дублируются.
Поле имени стоит перед Path mode слева, Password справа, как в rc.
Отключение флажка скрывает Edit и использует только папку combo; изменение
сохраняется при OK. Добавлены Qt tooltip и accessibleName пустому исходному
checkbox для доступности. Снимки split-dest-{0,1}-{640,470}.png просмотрены
на macOS offscreen. Windows/Linux и native folder picker не проверены.

### Шифрование Add to Archive

Пустой пароль создаёт архив без шифрования. Для 7z метод — AES-256;
шифрование имён включается отдельным флажком. Для ZIP исходный список —
ZipCrypto по умолчанию и AES-256; последний метод ZIP сохраняется при OK.
Show Password показывает первое поле и скрывает повтор с его подписью;
после отключения повтор снова виден и проверяется. Флажки показа и шифрования
имён сохраняются, пароль и повтор никогда не сохраняются. Cancel и ошибка
не записывают изменения. При смене 7z→ZIP header checkbox отключается,
при возвращении его выбор сохраняется.

ZIP принимает только символы 0x20–0x7F, как исходный IsAsciiString; для AES
максимум 99. Предел проверяется и диалогом, и backend до запуска операции.
7z допускает Unicode-пароли. Передача пароля 7zz остаётся через stdin.
Контролы password/reenter/show/method/headers проверены геометрией в обоих
состояниях показа при 760×720 и 640×480, включая длинный путь в истории.
Qt сохраняет вертикальную форму; Windows rendering и Windows/Linux runtime
ещё не проверены.

### Путь архива и история Add to Archive

Поле «Архив» — редактируемый список с полным именем и расширением, как
IDC_COMPRESS_ARCHIVE. При смене формата последнее расширение заменяется;
ведущая точка скрытого имени не удаляется. При OK сохраняются полный путь
и архиватор; следующий диалог открывается с последним форматом. История
ограничена 20, текущая цель идёт первой, дубли сравниваются без учёта регистра.
Выбор записи восстанавливает формат и папку. Cancel и ошибка не сохраняют
изменения; пароль в историю не попадает.

Qt сохраняет отдельное поле папки и source picker. Путь в combo может быть
абсолютным или относительным к папке; для backend допустимы только .7z/.zip
с выбранным форматом, существующим родителем и новой целью. Диалог добавления
в открытый архив не изменяет историю создания. Геометрия combo проверена на
macOS offscreen при 760×720 и 640×480; Windows/Linux и отрисованный Windows
эталон не проверены.

### Профили параметров Add to Archive

Доступные параметры сжатия сохраняются отдельно для 7z и ZIP при OK и
восстанавливаются при смене формата и повторном открытии: уровень, метод,
словарь/память, слово/порядок, solid, потоки и лимит RAM. Cancel и отказ
валидации не записывают профиль. Auto не превращается в явное вычисленное
значение. Неподдерживаемый сохранённый метод выбирает первый доступный;
его словарь/слово/solid/потоки не переносятся на другой метод.
QSettings заменяет Windows Registry. История и последний формат перенесены;
настройки времени ещё не перенесены.

При смене уровня сжатия прежние метод, словарь, слово/порядок, solid и
потоки сбрасываются к исходным значениям по умолчанию, как в
IDC_COMPRESS_LEVEL оригинального 7zFM. Лимит памяти сохраняется. Уровень
«Без сжатия» отключает зависимые контролы; возвращение к другому уровню
пересчитывает их Auto. Это заменяет прежнее удержание ручных значений Qt.

Смена метода больше не переносит ручные размеры другого метода. Возврат
к методу сохранённого профиля восстанавливает его параметры. При смене
словаря обычный ручной solid сбрасывается к Auto; Non-solid и Full остаются,
как в upstream. Ручные потоки сохраняются при смене словаря и лимита RAM.

Состояния диалога проверены на Qt/macOS при 760×720 и 640×480; порядок
level/method/dictionary/order/solid/threads/memory сопоставлен с
`CPP/7zip/UI/GUI/CompressDialog.rc`. Отрисованный Windows-эталон недоступен:
это проверка структуры и читаемости Qt, визуальная идентичность не подтверждена.

## P0-02 — owners, selection and lifetime

P0-02 принят: приложение хранит history/favorites в CAppState, а CApp/CPanel
владеют независимыми settings, focus, selection и folder lifetime. Референс —
AppState.h, App.cpp/.h, Panel.cpp/.h, PanelItems.cpp и FM.cpp WM_CLOSE из
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
[Source audit](upstream-port-audit.md#p0-02--accepted-owners-and-state) и
[исторические записи](history/p0-02-source-port.md) сохраняют происхождение и checks.

- Source keys имеют приоритет над migrated Qt preferences. Focus отделён от
  selection; refresh сохраняет source имена/indices и выбирает surviving row,
  если прежний current item исчез. Normal/flat views используют bound native owner.
- Properties/Copy summary читают source catalogs и operated indices: absent size
  остаётся пустым, defined zero — нулём; flat archive folder size не выдумывается.
  Typed properties сохраняют исходное форматирование и error-bit ordering.
  POSIX packed size использует upstream logical-size fallback.
- Qt worker/batch delivery сохраняет отзывчивость и отбрасывает stale results.
  Metadata остаются immutable до reload; display names нормализуются в NFC,
  raw names/indices сохраняются для операций. POSIX symlinks используют lstat;
  flat scan не следует directory links. Windows traversal guard предотвращает
  junction cycles; его Windows runtime остаётся непроверенным.
- Close сохраняет source soft-exit/revoke/save/release/window-save/hard-exit order.
  Qt дополнительно отменяет и joins asynchronous workers до удаления inputs/temp;
  каждый panel отключает notifications до освобождения первого folder owner.
  Native/modal open блокирует close; ordinary async close отменяет операцию.
  QSettings заменяет Registry, logical Qt geometry — WINDOWPLACEMENT.
- Background/Foreground, source default focus/Enter, statistics/resize/errors/copy
  и exact FM.ico сохраняются. Native priority failure откатывает состояние и
  показывает ошибку, которую upstream игнорировал. Windows idle/normal, Darwin
  background и Linux OTHER/BATCH — разные platform scheduling services.
- Windows taskbar — per-window; Cocoa — process-wide Dock badge с сохранением
  чужого badge; Linux Update/Query — progress/visibility/urgency без colored
  pause/error или indeterminate. sevenzip_desktop identity не закрывает packaging.
- Native temp cleanup удаляет owned file, затем пустой parent; POSIX remove()
  принимает empty directory, Windows DeleteFile — нет. POSIX library transfer
  переиспользует Windows Attach/Detach и loaded-copy guard. Qt font units,
  fixed Russian captions, macOS Meta и progress clamp — явные адаптации.

Navigation/update приняты в P0-04.1/.2, model в P0-05, list state в P0-06/07,
два panel в P0-08, shell в P0-09, modal progress и consumers в P0-10.1/.2.
Старые записи о disabled Background/Cancel или Qt ArchiveParent описывали
промежуточный код и не являются текущими ограничениями. Полные editor, language,
raw/ADS/reparse, shell/engine и packaging остаются у своих feature owners.
Исторические macOS arm64 gates: P0-02.1 54/54, .2 121/121, .3 122/122;
progress contract — 26 controls/24 JSON states, navigation — 16 reports.
Windows/Linux runtime, rendered Windows/Dock и реальный Linux launcher не проверены.

## P0-08.1 — два дисковых окна и F9 (2026-09-30)

Референс: `App.cpp::SwitchOnOffOnePanel`, `FM.cpp::MoveSubWindows`,
`PanelKey.cpp::OnKeyDown` и пункт View → 2 Panels (`resource.rc`, F9) из
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Qt показывает две независимые
дисковые панели слева/справа с общими меню и toolbar. F9 скрывает неактивную
панель и раскрывает активную; Tab переносит фокус списка в другую видимую
панель. Путь, выбор, режим просмотра и сортировка не переходят между панелями;
команды используют активную панель. Опрашиваются оба видимых каталога; при
смене панели во время плоского сканирования незавершённая порция отменяется и
чтение возобновляется при возврате. QSplitter заменяет исходное положение HWND
панелей. Проверены Qt JSON для двух размеров и скрытой панели; точная Windows
отрисовка и Windows/Linux runtime ещё не проверены. Смешанные панели с архивами,
Alt+F1/F2 и восстановление разделителя — P0-08.2, перенос файлов — P1-01.

## P0-08.2 — смешанные панели и восстановление (2026-09-30)

Референс: `PanelKey.cpp::OnKeyDown` и `App.cpp::CPanelCallbackImp::SetFocusToPath`
задают Alt+F1 для адреса левой панели и Alt+F2 для правой; в однопанельном
режиме оба открывают адрес видимой панели. `FM.cpp` читает/сохраняет
`CWindowInfo` и положение разделителя, `App.cpp::OnCopy` выбирает путь второй
панели по индексу. Qt повторяет эти правила через панели с отдельными
представлениями и QSplitter. Диск/архив и архив/архив удерживают свои пути,
списки, выбор и режимы при переключении. После запуска восстанавливаются
количество панелей, активная сторона, их сохранённые пути/режимы и разделитель;
скрытая панель сохраняет путь для следующего F9. Copy/Move/Extract предлагают
путь второй панели. Перенос в архив через Copy/Move и полный F5/F6 workflow
остаются P1-01; текущий filesystem worker отклоняет архив как папку назначения.
Qt ограничивает положение разделителя минимальными размерами контролов в
компактном окне. Проверены normal/compact и hidden Qt JSON; рендеринг Windows
и Windows/Linux runtime не проверены.

## P0-09.1 — главное меню и toolbar (2026-09-30)

`resource.rc` задаёт шесть верхних меню File/Edit/View/Favorites/Tools/Help,
группы команд и разделители. Qt повторяет их порядок, сохраняет исходные
shortcut команды через общий dispatch и показывает недоступные пункты
отключёнными. Существующие Qt Open/Create/Add/Extract Selected/Journal вынесены
в отдельную группу File, чтобы архивные сценарии оставались доступны; это
временное различие до source port их входов. Подписи пока русские из текущего
Qt слоя; Language P1-14 ещё обязателен. Toolbar повторяет исходный порядок
Archive Add/Extract/Test и Standard Copy/Move/Delete/Info, сохранённые
переключатели, размеры 24×24/48×36 и magenta mask исходных BMP. Общие QAction
передают команды активной панели при двухпанельном режиме. `main_shell`
проверяет группировку, сочетания, disabled пункты, BMP и normal/compact Qt
JSON для окна и File/View popup. Отчёты не доказывают пиксельное совпадение:
отрисованный Windows 7zFM reference и Windows/Linux runtime не проверены.
Контекст/статус/system icons остаются P0-09.2.

## P0-09.2 — контекст, status и иконки панели (2026-09-30)

Контекстный popup использует `CPanel::CreateFileMenu`/
`CFileMenu::Load(programMenu=false)` для исходного порядка и доступности
File-команд по выбору, типу папки и read-only состоянию. Клавиатурный вызов
размещается у текущего элемента. Win32 7-Zip extension и системный shell
submenu требуют отдельных платформенных services и видны отключёнными для
выбранных дисковых элементов (системная группа зависит от ShowSystemMenu).
При открытом архиве Qt сохраняет отдельную группу Add/Extract/Test/Info;
это отличие до полного переноса archive context. Прежняя Qt команда открытия
системного менеджера удалена из этого popup.

Как в `Panel.cpp` и `PanelListNotify.cpp`, под списком каждой панели находятся
четыре поля: выбранное/всего, суммарный размер выбранного, размер и дата
текущего выделенного элемента. Qt сохраняет отдельную общую строку для
сообщений сканирования/операций. При двух панелях значения независимы.
Кэш `SysIconUtils::CExtToIconMap` сохраняет source правила для атрибутов,
расширений и `.001`/`.002`; Qt берёт само изображение из системного
QFileIconProvider. Цвет/форма иконок зависят от ОС. `panel_shell` фиксирует
normal/compact Qt JSON и disk/archive popup, но Windows rendered reference,
Windows/Linux runtime и полноценные shell/namespace icons ещё не проверены.

## P1-05.4 — Synchronize и записи каталогов (2026-10-01)

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`AgentOut.cpp::DoOperation`, Common `UpdatePair.cpp`/`UpdateProduce.cpp`.
Add's source callback plan can now remove explicit parent directory records
without losing retained descendants. Synchronize with Full paths preserves the
archive's newer child bytes and removes obsolete entries; a kept explicit
subtree does not shield its separately omitted nested directory from deletion.
The transactional backend reports failure/cancel through the existing progress
workflow and leaves the input archive unchanged until successful publication.
No presentation change; existing mode/path controls and compact scrolling remain.
ZIP/7z regressions run on macOS arm64. Windows/Linux runtime and rendered Windows
appearance remain unverified; absolute extraction remains P2-07.

## P1-05.3 — Add в защищённый архив (2026-10-01)

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/PanelCopy.cpp::CPanel::CopyFrom` and
`CPP/7zip/UI/Agent/UpdateCallbackAgent.cpp::CryptoGetTextPassword2`.
Add is available on a writable protected top-level ZIP/7z. The Qt dialog
uses the password already entered to open the archive and preserves ZIP AES
or 7z header encryption for newly added files. It does not expose a separate
password or encryption-method field for an existing archive. Wrong-password
and cancelled updates leave the original archive intact. macOS arm64 source
and UI fixtures establish this workflow; Windows/Linux runtime and rendered
Windows appearance remain unverified. Directory filtering remains P1-05.4.

## P1-05.2 — режимы путей в Add (2026-10-01)

Reference: upstream `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.rc` path-mode row,
`CompressDialog.cpp::OnInit/OnOK`, and `CPP/Common/Wildcard.cpp::
CCensor::AddPathsToCensor`. The Qt Add dialog offers Relative, Full and
Absolute for a new or existing ZIP/7z. Relative stores selected names under
their own parent; Full stores the path from the filesystem root; Absolute
stores the leading root too. A source `CCensor` enumerates existing-archive
items, while the backend stages an atomic new archive or update. Simple
directory trees and empty folders are supported. The dialog's source picker
and scroll area remain Qt adaptations.

The `compression` normal/compact JSON reports at 760×720 and 640×480 show
the path-mode row and accessible footer on macOS. Rendered Windows appearance
and Windows/Linux runtime remain unverified. Existing-archive protected
Add is covered by P1-05.3; directory filtering with a kept descendant remains
P1-05.4. Absolute entries can be listed, but safe extraction currently rejects them; P2-07
owns the extraction policy.

## P1-05.1 — Add в существующий ZIP/7z (2026-10-01)

Reference: `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/GUI/CompressDialog.rc` update/path combo rows,
`CompressDialog.cpp::OnInit/OnOK`, `UpdateGUI.cpp::g_UpdateMode_Pairs` and
`AgentOut.cpp::DoOperation`. The Qt dialog now shows four update modes and
compression controls for an existing archive. The selected mode uses source
Add/Update/Freshen/Synchronize action sets, so newer/older/archive-only files
follow the original pair decisions. A source picker and scrolling compression
form remain the platform presentation adaptation. The original path-mode combo
and Full/Absolute names are still P1-05.2; only relative names are exposed.
Unsupported callback plans report failure rather than applying another mode.

At 760×720 and 640×480 the mode and compression controls can be brought into
view within the scroll area and the footer remains visible. Current-build
`update-options-normal.json`/`update-options-compact.json` confirm widget
structure and clipping on macOS arm64; Windows rendered pixel appearance and
Windows/Linux runtime are unverified. The existing-archive encryption panel is
still absent, and this dialog declines protected Add pending P1-05.2; source
F5 protected Add remains available.

## P1-04.1 — изменение незашифрованного архива (2026-09-30)

F7, F2 и Delete/Shift+Delete в верхнем 7z/ZIP теперь идут через те же
`CPanel` и `CAgentFolder`, что и исходный 7zFM. F2 редактирует базовое имя
прямо в списке; flat prefix остаётся только частью отображения. Исходные
тип/количество в подтверждении удаления, прогресс, отмена, ошибки и
восстановление папки подключены к Qt. После операции исходный native ReOpen
предшествует перерисовке, чтобы удалённое выделение не открывало архив дважды.
Отдельный Qt `QInputDialog` для архивного F2 удалён. Protected/nested write
commands остаются отключёнными до P1-04.2/P1-09. Проверены macOS arm64,
реальные ZIP/7z и current-build Qt UX; Windows/Linux runtime и rendered
Windows 7zFM parity не проверены.

## P1-04.2 — операции с защищённым верхним архивом (2026-09-30)

F7, F2 и Delete/Shift+Delete доступны в открытом верхнем 7z с зашифрованными
заголовками и ZIP AES. Пароль, введённый при открытии, проходит через исходный
`CPanel` update callback и повторное открытие; эти действия не показывают
второй диалог пароля. Отмена, ошибка и неверный пароль не публикуют частичную
копию. В ZIP 7zz создаёт пустую папку как entry без данных; уже существующие
файлы остаются зашифрованными. Add/Edit/Move, вложенные архивы и comment
остаются в P1-05/P1-08/P1-09/P1-17. ZIP comment выделен из прежнего P1-04.3
и заблокирован отсутствием in-process ZIP writer. Структура диалога пароля следует
`PasswordDialog.rc/.cpp`; геометрия и текст не менялись. Проверены macOS arm64
и Qt runtime; Windows/Linux runtime и rendered Windows reference не проверены.

## P1-11.1 — системные тома на macOS/Linux (2026-09-30)

В виртуальном корне добавлен пункт «Тома» после «Компьютер». Он открывает
список смонтированных файловых систем с именем, типом и доступными размерами;
путь выбранного тома открывается исходным `CFSFolder`. На Windows исходный
список Computer/drive/Volumes остаётся у `CRootFolder`/`CFSDrives`.
Неисчезнувший корень `/` показан даже при пустом ответе системного mount API;
исчезнувший или недоступный том нельзя открыть из устаревшего списка.
Верхнее меню, toolbar, адрес и status остаются видимыми на 980×620/640×400.
Network namespace и Browse dialogs перенесены в P1-11.3/.2; Windows/Linux
runtime и rendered Windows reference не проверены.

## P1-11.2 — выбор файла и папки (2026-09-30)

Кнопки обзора в Copy/Move, Split, Extract и Create Archive, а также команда
Open Archive теперь проходят через source `MyBrowseForFolder` или
`CBrowseInfo::BrowseForFile`. Системный Qt picker сохраняет привычный OS UX;
Cancel не меняет редактируемый путь, выбор Unicode/пробелов возвращает тот
же путь и сохранённый индекс фильтра. Copy/Split оставляют конечный `/` по
своему исходному workflow. В upstream desktop используются native folder/file
dialogs; `BrowseDialog.rc` для custom fallback не доказывает их pixel parity.
Windows super/device path fallback и runtime Windows/Linux остаются
непроверенными; сетевой namespace перенесён в P1-11.3.

## P1-11.3 — сетевые папки (2026-09-30)

Windows «Сеть» открывает исходный `CNetFolder` и перечисляет ресурсы через
`NWindows::NNet`; на macOS/Linux тот же пункт показывает уже смонтированные
SMB/NFS/AFP/SSH/WebDAV ресурсы из системного списка томов. Подключение нового
сервера и запрос credentials не выполняются: это ограничение platform service,
а не скрытая команда UI. Путь и provider берутся из source folder properties,
назад и родитель возвращают к виртуальному корню, stale/нечитаемую share
открыть нельзя. Menu, toolbar, address, list и status проверены на 980×620 и
640×400 в current-build JSON на macOS arm64. Реальные remote services,
Windows/Linux runtime и rendered Windows visual parity не проверены.

## P1-01.1 — F5/F6 между двумя панелями и из архива (2026-09-30)

F5 в открытом архиве теперь показывает тот же CopyDialog, что и файловая
система: путь второй панели по умолчанию, исходная сводка выделения,
история и кнопка выбора папки. Для текущей папки архива `CPanel::CopyTo`
использует `kCurPaths`: файл попадает прямо в указанную папку, без повторения
внутреннего пути архива. F6 из архива выполняет эту же копию и удаляет
исходную запись только после её успеха. Отмена overwrite оставляет исходную
запись и старые байты назначения. Вторая filesystem панель обновляется после
F5/F6; disk→disk F5/F6 также обновляет обе панели. Исходный
`CAgentFolder::CopyTo(moveMode)` сам возвращает `E_NOTIMPL`, поэтому
двухстадийный F6 — локальная адаптация, а полный `CApp::OnCopy` и
archive destination остаются P1-01.2/.3. Отдельная команда Extract сохраняет
свой ExtractDialog и параметры. CopyDialog resource order/тексты проверены
на 440×250 и 340×220 в current-build JSON; Windows rendered parity и
runtime Windows/Linux не проверены.

## P1-01.2 — F5/F6 из дисковой панели в открытый архив (2026-09-30)

Если справа открыта папка верхнего ZIP/7z, F5/F6 слева подставляет её
виртуальный путь в исходный CopyDialog. Подтверждение запускает исходный
`CPanel::CopyFrom` → `CAgentFolder::CopyFrom` с Add action set. В архив
попадают файлы и деревья с относительными именами из текущей дисковой
папки. F6 удаляет дисковые исходники после успешной публикации архива;
отмена не удаляет их. После успеха обе панели обновляются. Пароль уже
открытого защищённого верхнего архива передаётся source callback. Вложенные
архивы и путь другого архива, не открытого во второй панели, пока не являются
destination: соответствующая маршрутизация остаётся P1-01.3. Структура
CopyDialog не менялась; Windows/Linux runtime и rendered Windows visual
parity ещё не проверены.

## P1-01.3 — F5/F6 между двумя открытыми архивами (2026-10-01)

При двух верхних ZIP/7z панелях F5/F6 показывает исходный CopyDialog с путём
противоположной панели, затем копирует выбранные записи через временную папку
и `CopyFrom` второго архива. При F6 исходный архив изменяется после публикации
целевого; отмена удаления восстанавливает прежние байты целевого архива.
Защищённый исходный 7z использует сохранённый пароль панели. Запись во
вложенный архив и цель вне открытой второй панели показывают отказ. Списки
обеих панелей обновляются после успеха. Обычные F5/F6 disk↔disk,
archive→disk и disk→archive используют тот же исходный выбор/диалог/историю.

Исходные Shift+F5/F6 (`copyToSame`) и одиночное переименование destination
были подключены в P1-01.4 после filesystem copy/move branches P1-02.
Структура CopyDialog осталась от P1-01.1; новый отказ
проверен в реальном nested-source UI тесте. Прямой набор затронутых UI
сценариев 10/10 и полный `check.sh` 142/142 прошли на macOS arm64/Qt 6.11.2.
Точный Windows rendered reference и Windows/Linux runtime не проверены;
parent P1-01 принят после P1-01.4.

## P1-01.4 — Shift+F5/F6 и имя назначения (2026-10-01)

Shift+F5/F6 использует исходный `PanelKey` consumer и открывает CopyDialog с
именем элемента под фокусом, даже когда выделена другая строка. В файловой
панели ввод нового имени копирует или перемещает один объект по точному пути;
родительская папка создаётся по исходной ветке `App.cpp`. То же прямое имя
можно ввести при обычном F5/F6 для одного объекта. Существующая папка остаётся
папкой назначения. Самокопирование и цель внутри исходного каталога отклоняются.

В архивной панели абсолютный destination передаётся исходному Agent
`CopyTo`: это папка извлечения, поскольку Agent всегда нормализует путь как
каталог. Shift+F6 удаляет исходную запись только после успешного извлечения.
Если исходный extract service недоступен, показан отказ. Переименование записи
внутри архива этим маршрутом не поддерживается. Новый диалог и новые controls
не добавлялись; структура/размер CopyDialog соответствуют P1-01.1.
Windows/Linux runtime и rendered Windows reference не проверены.
