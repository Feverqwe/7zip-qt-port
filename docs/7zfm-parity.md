# Сверка с Windows 7zFM

Этот документ описывает сценарии и отличия от Windows 7zFM. Разделы по ID
и датам сохраняют границы проверенного поведения; их порядок не определяет
текущий статус. Для выбора работы использовать [чеклист](porting-checklist.md),
для provenance — [source audit](upstream-port-audit.md), для открытых UX-коррекций —
[UX audit](ux-port-audit.md). Нужный сценарий искать по команде, символу или ID,
затем читать соответствующий раздел и ссылки.

### Source GUI path/history consumers (P3-05, 2026-10-07)

Create/Extract теперь сохраняют исходные правила UString::Trim: убирают
краевые пробелы, LF и tab, сохраняя NBSP в имени архива, папки и истории.
CompressDialog использует исходное разрешение полного пути и отказ при
неразрешимом переходе выше корня; Extract добавляет системный разделитель.
Общая история обоих диалогов использует исходное IsEqualTo_NoCase, включая
его отличие от Qt case folding для K/Kelvin. Embedded NUL в Create по-прежнему
отклоняется на границе Qt-текста. Новые controls или тексты не добавлены.

Четыре EN/RU × SplitDest сценария проходят Create/list/Extract через официальный
движок с Unicode/NBSP, точными бинарными байтами, сохранённой историей и Escape
без записи настроек в обоих диалогах. JSON проверяет видимость и исходную
группировку при normal/compact размерах; Windows использует native Qt fonts/style.
Это не rendered reference оригинального Windows 7zFM и не приёмка физического
desktop-ввода. [Source scope и проверки](upstream-port-audit.md#p3-05--source-gui-path-and-history-consumers-2026-10-07).

### Source Agent provider lifecycle (P3-05, 2026-10-07)

Directory CRC now reaches the list and selected Properties through the original
path-proxy property branch. It uses the source additive UInt32 aggregate, so it
is not a checksum of concatenated directory bytes. Unknown file CRC can leave
the directory value undefined. Explicit-record failures retain source Error
text in the list and omit only the failed item property; the separate folder
summary can still show its cached aggregate. Original written-value/local
variant policy is preserved. This changes property data without adding controls.
See the [property projection audit](upstream-port-audit.md#p3-05--source-archive-directory-property-projection-2026-10-07).

Closed archive folders refuse loading before reading their proxies, including
after a failed ReOpen. Successful repair binds a fresh root and child provider;
callers must discard the old folders. Cancellation during initial proxy binding
publishes no folder and permits a retry through the same manager. Descriptor
and load failures release partial proxies for that retry. These source lifecycle
rules introduce no dialog or layout change. Exact adaptations and platform
coverage are in the [lifecycle audit](upstream-port-audit.md#p3-05--source-agent-provider-lifecycle-and-metadata-binding-2026-10-07).

### Source Agent opening/factory (P3-05, 2026-10-07)

Archive-folder opening now reaches the original Agent/factory workflow.
An unknown forced format returns source S_FALSE before listing; a recognized
malformed archive can return a diagnostic folder with the same result, so the
existing error UI can read NonOpen_ErrorInfo. Root binding receives the caller's
progress/Cancel interfaces. The returned owner retains streams and the exact
codec catalog after the manager is released or the backend catalog is replaced.
File-only transport still spools nested input and services process work through
ArchiveBackend. No dialog text, layout or visibility changes are introduced.
Current platform acceptance is recorded in the
[source audit](upstream-port-audit.md#p3-05--source-agent-opening-and-archive-folder-factory-2026-10-07);
native GUI/rendered and whole-plan acceptance remain open.

### Native zone Copy and bound filesystem publication (P3-05.15, 2026-10-07)

F5/F6 retain the source registry-zone default: None preserves the stored
Zone.Identifier, All applies the parent-container zone, and Office applies it
to Office names. Named alternate streams remain source engine output. The
separate Extract dialog owns its explicit metadata options. Copy/Move refreshes
the existing filesystem provider, including normal/flat worker loading; its
native notification owner remains on the GUI thread. Both panes may finish
their own scans before the original saved selection and source focus continuation.
Root ADS initialization failure again returns the original E_INVALIDARG.
The dated port audit owns bounded platform/geometry evidence. Rendered Windows
and whole-plan acceptance remain separate.

### Native bound archive Drop (P3-05.15, 2026-10-07)

CopyFromNoAsk retains success-only Refresh and source list focus. Actual writer
failure/Cancel and confirmation No/Cancel leave the list/provider and archive
bytes intact. A later native attribute publication failure does not reverse a
successful write: the list becomes empty/enabled and Ctrl+R reloads the same
provider. Qt publishes the reached source phase after synchronous guards unwind
and completes source list focus after asynchronous rows settle. Original native
writer-failure diagnostics remain in source progress and CopyFromNoAsk; no extra
Qt warning or physical reopen is added. Eight representative2/301-row cases
cover both panes/languages/views/selection modes, actual retry and subsequent
filesystem OnTimer. The bounded native component and current normal/compact
geometry pass on macOS, Linux and Windows; evidence is in the dated audit.
Rendered Windows and whole-plan acceptance remain separate.

### Native interarchive rollback file lifetime (P3-03, 2026-10-07)

The existing Copy/Delete/backup extension closes the destination's source
archive streams before restoring backup bytes, then uses the existing physical
reopen and saved selection. This permits Windows replacement under native
reader sharing. Source deletion refusal/error/Cancel still retains source bytes;
failed reopen still returns to the destination's filesystem folder. The narrow
Close bridge reuses CAgent::Close; source copy and deletion algorithms are unchanged.
The native Windows two-case RED and subsequent bounded three-OS verification
are tracked in the dated port audit; this is not whole-platform acceptance.

### Changed nested archive Parent (P3-05.15, 2026-10-07)

For disk-extracted native children, Parent retains the original Yes/No/Cancel
policy. No and Cancel both
discard the changed child and leave the outer archive unchanged. Yes updates
the outer item; writer failure/Cancel retains the modified spool and reports
the source update error, then still returns to the parent. Its final Refresh
result is ignored. Empty/enabled failed publication recovers on Ctrl+R using
the same outer provider. Real native writer retry preserves the old recovery
copy while deleting the new successful spool. Actual F2/native Rename creates
the changed child and retry. Five representative2/301-row cases cover both
panes, languages, views and selection modes, child focus, archive/peer bytes
and later filesystem OnTimer. Geometry reports do not prove rendered parity.
Qt callback publication now keeps a non-owning provider identity, so original
ReleaseFolder closes native archive streams before Parent deletes its spool.
The prior extra COM owner caused actual Windows cleanup failures. Source
algorithms and cleanup order remain unchanged. The five Windows cleanup failures
now pass, as do the affected native consumers on macOS and Linux; detailed
revision, geometry and gate evidence is in the dated port audit.

### Native Rename error ownership (P3-05.15, 2026-10-07)

Native F2 Rename now keeps the source progress dialog as the sole operation-error
owner; Qt no longer adds a generic second warning after the worker. Early invalid
input still rejects the editor with its validation message. Original new/old
focus, saved marks and unconditional posted reload after success/error/Cancel
are preserved. Reload publication errors remain ignored; explicit Ctrl+R recovers
the same provider. Real native writer retry, archive/peer bytes and guard release
are covered at2/301 rows in both panes/languages/views/selection modes. Normal/
compact reports establish control geometry only; native Windows/Linux and rendered
Windows reference acceptance remain open.
Qt's periodic two-panel traversal now honors source notification/status guards
before activation and status reads, preserving native WM_TIMER's guarded access
during archive proxy reopen. This fixes an actual native Rename crash.

### Native external editor update (P3-05.15, 2026-10-07)

The native editor-completion consumer preserves original OnOpenItemChanged's
unconditional ignored-result Refresh after writer success/failure/Cancel.
An ignored native property publication error does not turn a successful update
into an external-update failure: the actual payload changes and its editor copy
is deleted. Writer failure/Cancel preserve archive bytes and the edited copy,
then actual F4 retry succeeds. Source marks, owning provider, peer focus and later
OnTimer are checked in both panes/languages/views/selection modes with2/301 rows.
Current evidence uses macOS; native Windows/Linux and rendered reference remain
mandatory. This iteration adds consumer verification without changing production
algorithms, layout or control flow.

### Native nested opening (P3-05.15, 2026-10-07)

Original Agent::GetStream index mapping and OpenItemInArchive's preferred stream,
bounded memory and disk paths are retained. Qt adds only the native module/input
lease and timer-guard handoff needed by deferred row publication. All three
success branches ignore RefreshListCtrl results as upstream does. Actual native
Cancel Yes retains the outer folder and removes owned temporaries; No/Cancel
continue opening, followed by normal Parent cleanup. TAR explicit directory
records may provide empty streams; implicit non-leaf directories keep the
original no-stream policy. The resource proves structure and control flow;
macOS tests do not establish rendered Windows or three-platform acceptance.

## P3-05.15 — исходный Copy/Move routing (обязательная сверка, 2026-10-06)

Нативная ошибка свойств большого списка сохраняет политику source caller:
OnReload сообщает любой non-S_OK, включая S_FALSE/E_ABORT; OnCreate и
OpenAsArc_Index игнорируют Refresh HRESULT. Qt доставляет результат после
восстановления redraw/notification guards, до продолжения Copy. OpenAsArc_Index
передаёт владение guards из bind-фазы в Refresh перед асинхронной публикацией.
Filesystem OpenItem передаёт и внешние guards; ошибочный open сохраняет
timer suppression до завершения исходного error/external-fallback tail.
После ошибки список пуст и доступен; Ctrl+R восстанавливает тот же provider.
Native GUI property и CLI listing различаются по PID в частной тестовой fixture.
Это проверка control flow на macOS; rendered Windows и current native3OS
приёмка остаются открытыми.

Начальная публикация панели теперь принадлежит source `Panel.cpp::OnCreate`,
как при initial `App::Create`, так и при позднем `SwitchOnOffOnePanel`.
Reload-сервис подключается вместе с Qt-контролами до исходного Refresh;
отдельные render из конструктора/toggle удалены. Qt допускает параллельную
публикацию больших списков обеих панелей, сохраняя исходную политику ignored
initial Refresh HRESULT. Filesystem worker по-прежнему публикует свой provider.
Creation/Refresh/hide-show и EN/RU normal/compact проверяются отдельной матрицей;
current Windows/Linux и rendered Windows приёмка остаются открытыми.
CApp::Create сохраняет исходный main-panel choice отдельно от текущей Qt-проекции: ordinary
startup с правой main panel теперь получает requested path, а не её saved path.
Initial focus завершается однократно после scans; последующий явный ввод в
адрес/список или public archive request отменяет этот отложенный фокус.
Это необходимая Qt async-адаптация исходного синхронного Create/focus workflow.
Отложенный focus создаётся только для ещё читающего стартового списка;
синхронный Create уже установил его и не требует повторного действия.
Немедленное раскрытие сохранённой скрытой панели проверяется отдельно.
QSplitter вставляет контролы по source index, как FM.cpp::MoveSubWindows:
панель0 слева, панель1 справа независимо от порядка создания.
Public archive opening восстанавливает выбранный address control после modal
progress; слабая ссылка учитывает уничтожение и недоступность контрола.

Успешный Copy/Move между архивами также публикует тот же bound destination через
source RefreshListCtrl с pre-update selection. Принятая композиция Move через
Copy/Delete/rollback сохранена: original CAgentFolder::CopyTo(moveMode) возвращает
E_NOTIMPL. Отказ/ошибка/Cancel удаления источника восстанавливают backup и требуют
настоящего повторного открытия назначения, с выбором до записи. Эта ветка намеренно
заменяет provider. Both-pane EN/RU/table-icon/standard-alternative consumers
проверяют native ZIP/7z и transport phases. Отдельный interarchive_recovery
проверяет failed backup restore/reopen/engine Cancel, filesystem fallback и F5
после исправления временного назначения. Restore failure показывает отдельное
предупреждение, reopen E_ABORT остаётся без сообщения. Source Move по-прежнему
расширение адаптера; current Windows/Linux/rendered приёмка открыта.

Copy/Move из файловой панели в архив больше не открывает второй provider после
успешного CopyFrom. Как original OnCopy, список обновляется с состоянием,
сохранённым до операции; ArchiveFolderOut уже выполнил engine ReOpen и восстановил
proxy. Прежние отмеченные имена и фокус назначения сохраняются при изменении
индексов. Ошибка/Cancel продолжают existing unconditional destination reload и
source completion. Native ZIP/7z,301-row ZIP и transport success/failure/Cancel
проверяются отдельной матрицей; Windows/Linux и rendered Windows приёмка открыты.

Refresh/Ctrl+R теперь использует original CApp::RefreshView/OnReload также для
архивов. Команда перезагружает bound provider без повторного открытия физического
файла. Прежняя Qt-ветка вызывала ReOpen, меняя provider и добавляя engine-запрос.
Повторное открытие после остальных transport operations остаётся отдельной границей;
эта команда не заменяет закрытие и новое открытие внешне изменённого архива.

Прямой source RefreshListCtrl теперь загружает и публикует список, затем
восстанавливает выделение. После Drop исходный CopyFromNoAsk делает это только
при успехе; ошибка и Cancel возвращаются раньше. Удалён общий Qt ReOpen всех
видимых панелей после drag/drop, включая отменённый archive drag. Исходный
ArchiveFolderOut уже владеет ReOpen при обновлении архива; FS timer наблюдает
изменения файловой системы. Остальные operation/renderer/native boundaries
остаются обязательными.

После попытки записи внешней правки список архива публикуется также при ошибке
и Cancel, как unconditional RefreshListCtrl в OnOpenItemChanged. Qt переносит
исходный saved state до освобождения operation guards; результат записи остаётся
отдельным bool. Read-only worker и ранний отказ при смене папки/исчезновении
элемента не запрашивают Refresh. Ошибка загрузки не продолжает восстановление
выбора; отдельная диагностика ignored reload HRESULT не добавлена.
Неактивная панель публикуется через существующий withPanelProjection. Qt-adapter
сохраняет weak focused-control pointer до progress и возвращает фокус после
включения terminal UI: отключение адреса может очистить remembered focus.
48 EN/RU both-pane table/icon standard/alternative cases и normal/compact
проверены на macOS; нативная Windows/Linux focus/rendered сверка остаётся открытой.

UpdateSelection восстановил исходный узкий `_enableItemChangeNotify` guard:
синхронные Qt-сигналы выбора больше не скрывают callback names. Внешний
CDisableNotify сохраняет подавление. Цвет альтернативных отметок проецируется
Qt-ролями отдельно от исходного цикла выбора. EN/RU обе панели, table/icon,
standard/alternative и normal/compact проверены на macOS; Windows/Linux и
пиксельная сверка с rendered Windows остаются обязательными.

Refresh теперь использует записанный GetNumberOfItems count независимо от HRESULT,
как upstream. Qt сохраняет этот numItems между load/publication/selection; поздний
count не меняет опубликованные source indices. Для незаписанного output Qt задаёт
ноль вместо исходного неинициализированного значения. Parent и post-Copy/Move
публикуют валидный count без ложной ошибки. E_ABORT самого LoadItems сохраняет
отказ/Cancel; E_ABORT метаданных count следует отдельной исходной политике.
EN/RU обе панели, detail/icon, Refresh/timer/recovery и bytes проверены на macOS;
current Windows/Linux и исходный unwritten-output runtime не подтверждены.

App::OnCopy сохраняет начальные проверки, выбор, Copy dialog и исходные
destination/default/refusal/classification branches. Пустая цель peer остаётся
пустой, однопанельный путь сокращается исходным reducer. Multi-directory и parent
создаются до execution; ошибку сопровождает исходный путь и HRESULT.
FS/ADS/archive flags и captured indices проходят через consumers; staged Copy
сохраняет исходные alternate-stream/zone defaults. Summary добавляет native
separator к папкам. Qt MessageBox автоматически выбирает размер и wrapping
двухстрочной диагностики, как platform-owned message box исходника; это не
фиксированная Copy dialog geometry. Полная загрузка/refresh completion
и native ADS/device/zone evidence ещё требуют сверки.
Принятый архивный Move остаётся отдельной требуемой композицией Copy/Delete/
rollback: оригинальный Agent Move возвращает E_NOTIMPL. P3-05.15 открыт;
проверенные ранее сценарии не доказывают полного покрытия OnCopy. Полная сверка
App и границы адаптаций записаны в omitted-scope/port audit.
Copy/Move без отмеченных строк доступны по исходному CFileMenu; OperSmart
выбирает исходные элементы, пустая writable-папка даёт no-op. Busy/provider/
readonly ограничения сохраняются. Qt переносит timer/notify guards от modal
стека к владельцу операции; synchronous archive publication начинается после
окончания engine/rollback и освобождения guard. Это адаптация асинхронного
transport, а не новый selection algorithm. Полная completion-сверка остаётся.
Исходные source/destination CSelectedState сохраняются после modal, а исходный
post-operation title/selection/notify/focus body остаётся в CApp и выполняется
один раз после terminal execution и публикации списков. Ordinary two-panel FS
Copy сохраняет source rows/current item; Move/Shift/shared single panel публикуют
источник. Отложенный Qt progress destructor больше не заменяет итоговый title.
EN/RU оба source-panel roles, peer selection, bytes и normal/compact state
проверяются public consumers; offscreen activation не доказывает native focus.
Полная загрузка RefreshListCtrl и native/platform приёмка ещё обязательны.
Destination panel теперь обновляется и при цели вне peer после Copy/Move или
Cancel в overwrite dialog. Cancel самого Copy dialog сохраняет прежний список.
Qt учитывает уже начатую публикацию и ждёт сканирования перед исходной
continuation. Namespace OnReload повторно публикует существующий native provider.
Эти consumers проверены отдельно от оставшейся полной загрузки RefreshListCtrl;
текущие Windows/Linux и rendered evidence ещё требуются.
Загрузка namespace/archive/filesystem теперь использует исходные stages
RefreshListCtrl: удаление старых строк до загрузки, flat mode, columns,
проверенный directory cache и deleted-item flag. Worker уже загруженной FS-папки
не перечисляет её повторно. Ошибка directory property или Cancel загрузки
namespace оставляет пустой список; последующая успешная загрузка восстанавливает
его. Полные HRESULT presentation, icons/dots/timer и native-проверки остаются.
Синхронный HRESULT namespace/archive renderer теперь возвращается в исходный
OnReload; ошибку сопровождает его исходное сообщение, включая E_ABORT.
Namespace и filesystem Refresh идут через тот же handler. Direct/explicit/timer/command
consumers проверены под EN/RU с восстановлением списка после ошибки.
После ошибки загрузки исходная post-Copy continuation теперь получает тот же
HRESULT до восстановления выделения. Повторный reload пустого списка не читает
невалидный count provider. Copy/Move consumers проверяют output, очистку marks,
focus/title, notification guards и последующую успешную загрузку peer.
Синхронный FS reload сохраняет уже привязанный CFSFolder и возвращает load
HRESULT, включая Cancel и преобразованное COM allocation exception; ошибочное
выделение не восстанавливается. Обе роли панелей и реальный FS peer после
Copy/Move проверены с повторной загрузкой и контрольными байтами.
FS worker failure теперь передаёт исходный HRESULT в load stage до properties/
selection, сохраняет provider и показывает source error перед завершением
async publication. После временной peer projection исходная OnCopy continuation
восстанавливает source title/focus. Реальный worker failure, superseded generation,
normal/flat, EN/RU и Copy/Move consumers проверяют восстановление и bytes.
Missing-folder reload теперь очищает native items/marks через load stage,
сохраняя provider и последующее восстановление. Исходный Windows Next(fi)
завершает перечисление при ошибке; POSIX adapter повторяет этот контракт.
Прямой CFSFolder::Init missing path по-прежнему отказывает. Проверены обе панели,
EN/RU, Refresh/timer и standard/alternative selection, с контрольными байтами.
Ввод missing descendant использует исходный BindToPath и открывает closest
existing ancestor с его address/title и list focus; peer и control bytes
сохраняются. Qt address transport теперь использует тот же исходный Windows
outer-quote block, что BindToPathAndRefresh. Absolute/relative/trailing separator
проверены под EN/RU с обеих панелей; Windows quoted runtime ещё обязателен.
Остальные post-Copy failures требуют сверки;
текущие Windows/Linux и rendered проверки остаются обязательными.
Timer reload теперь передаёт исходный CalledFromTimer в Qt renderers:
набранный адрес, cursor/selection и focus сохраняются при обновлении списка.
Обычный Refresh публикует текущий путь. Малый/worker/flat filesystem и bound
root/ZIP callers покрыты отдельно; native three-platform acceptance обязательна.
Иконки строк теперь выбирает исходная policy RefreshListCtrl: slow-folder guard,
positive system index и fallback через исходные атрибуты с archive mask и
directory-bit correction. Вычисление выполняется внутри порций Qt publication.
Windows shell image list преобразуется в QIcon; POSIX CFSFolder использует
узкий Qt lookup с существующим GetWinAttrib. Root сохраняет прежние portable
namespace icons; Windows drive/network/stream rows используют source icon.
Ошибки атрибутов доходят до reload owner, публикация прекращается и очищает
строки; успешный reload восстанавливает список. Это Qt atomic-row adaptation:
исходный Win32 цикл мог оставить часть строк при раннем возврате HRESULT.
Real FS/ZIP small/301 и ShowRealFileIcons on/off проверены на Mac; native shell
runtime и rendered Windows reference ещё обязательны.
Name callback теперь сохраняет исходный fast-name formatter: RLO заменяется
на `_`, серии от пяти пробелов сокращаются, пустое fast name показывается как
`_`, Windows добавляет исходный trailing-space marker. Agent возвращает исходные
name/prefix/size interfaces; FS сохраняет original Unicode-FString conditional,
поэтому POSIX FS использует generic BSTR path. Generic name/prefix failures
показывают исходное `Error: ` в ячейке. Qt сохраняет существующие NFC/flat-prefix
display adaptations; операции и редактор используют исходные имена. Завершение
Rename читает EditRole, как original OnEndLabelEdit читает текст редактора.
Исходные блокировки таймера и уведомлений восстановлены внутри OnEndLabelEdit:
вложенные события GUI во время worker Rename не запускают reload. Success,
failure и E_ABORT восстанавливают предыдущее состояние, включая внешние
блокировки; реальные filesystem bytes проверены на macOS. После операции
сохраняются исходные selected names, новое/прежнее имя фокуса и SelectFocused;
Qt больше не заменяет выбор одним путём. `_dontShowMode` подавляет callback-текст
до начала загрузки, включая Qt-кэш, сохраняя raw roles/EditRole. Ранний отказ
имени возвращает старый текст и выбор без reload. EN/RU и обе панели проверены
через inline editor; native Windows/Linux/ADS проверка остаётся открытой.
Deleted-item foreground теперь использует исходные IsItem_Deleted и OnCustomDraw:
красный RGB255,0,0 при исходном default marking и archive-level deleted flag.
Qt ForegroundRole и HighlightedText сохраняют цвет в table/icon, включая обычное
выделение; alternative pink background не заменяется. Guards и несовпавший
provider блокируют запрос, ошибка paint использует палитру Qt; исходный property
getter сохраняет exception policy. EN/RU обе роли/reload/rebind/current captures
проверены на macOS. Реальные deleted archives и rendered Windows/native3OS
приёмка остаются открытыми.
Enter и двойной щелчок table/icon теперь вызывают original OnNotifyActivateItems:
Alt без Ctrl/Shift — Properties, только Shift — Open Outside, остальные
Ctrl/Shift/Alt сочетания разрешают внутреннее открытие. Parent следует тому же
source OpenSelectedItems. Qt декодирует модификаторы события и предотвращает
повторную QAction activation; rename/notify/operation/modal guards сохраняются.
Обе панели, EN/RU, FS/real ZIP, отмена Properties, удалённая папка и recovery
проверены на macOS. Outside archive callback сохраняет source virtual prefix;
это не приёмка OS association для архивной папки. Native Windows shell Properties,
current Windows/Linux и rendered parity остаются открытыми.
Изменение выбора теперь вызывает исходный OnItemChanged в owning CPanel,
включая неактивную панель. Qt передаёт source index и изменение selected-state;
guard/alternative mode сохраняют независимые marks. SaveSelectedState снова
только читает исходное состояние. Qt attachment и deferred publication имеют
явные точки инициализации; direct InitColumns очищает marks и требует snapshot,
сохранённый до вызова. EN/RU обе роли/Reload/peer/bytes проверены на macOS;
current native/rendered и полная OnNotify приёмка остаются открытыми.
Реальные FS/ZIP/7z теперь проверены одним сценарием с отдельными settings по
FolderTypeID: source defaults, Name width/order и ATime visibility восстанавливаются
после namespace-перехода и повторного открытия архива. Смена только Type или
только Name обновляет source visible descriptors, alignment и caption; ширина,
selection и peer focus сохраняются. EN/RU обе панели и normal/compact geometry
проверены на macOS; текущие Windows/Linux и rendered Windows остаются открытыми.
Проверка не меняет production поведение; [evidence](upstream-port-audit.md#source-real-provider-column-workflow-2026-10-07).
Header column menu теперь передаёт выбранный индекс исходному
ShowColumnsContextMenu: порядок/отметки и запрет первого пункта берутся из source
catalog. Показ добавляет колонку в конец; скрытие сортируемой меняет metadata
на Name/ascending, сохраняя строки до Refresh. Qt держит скрытые logical sections
и сохраняет компактный порядок только видимых. Popup/move/resize используют
владельца заголовка, выход из popup восстанавливает прежний widget focus.
EN/RU обе панели и active/inactive consumers проверены на macOS; текущие native
Windows/Linux и rendered Windows проверки остаются открытыми.
InitColumns теперь очищает исходные marks также при прямом вызове и сохраняет
исходный equal-column early return/rebuild. Логический Name остаётся первым,
а визуальный порядок берётся из исходного проверенного массива; перенос Name
в конец сохраняется после Refresh и reopening. Qt применяет массив после
создания физических секций, сохраняя rows/widget selection в их прежних owners.
Direct marks/restoration и EN/RU обе панели/order/width/bytes проверены на macOS;
raw/duplicate schemas и текущая native/rendered приёмка остаются открытыми.
SaveListViewInfo больше не переписывает каталог `_columns` при Qt move/resize:
обновляются visible snapshot и исходные persisted settings. Повторный показ через
меню берёт Width из каталога, как native AddColumn; прежняя Qt-проверка сохранения
173px после hide/show исправлена. Оставленный resize сохраняется и применяется
при следующем InitColumns/reopening. Catalog/visible/saved metadata и EN/RU обе
панели с active/inactive headers проверены на macOS; native/rendered checks открыты.
Qt publication теперь сохраняет source catalog index для каждой секции: generic
и raw CRC/Size с одинаковым ID получают отдельные текст/alignment/width. Namespace
rows используют исходный GetColumnAlign, как остальные списки. Сохранение видимых
колонок учитывает Type/raw/name/occurrence; исходная first-match политика saved
duplicate IDs остаётся. EN/RU обе панели с peer focus, guards, RawProp error/Cancel
и Refresh проверены на macOS. Broader schema и текущая
native/rendered приёмка остаются открытыми.
Повторные catalog-ID пункты меню снова доступны по исходной политике: скрытие
удаляет первый visible ID, показ добавляет новый экземпляр, даже если raw descriptor
уже видим. Qt отдельно хранит visible logical index и при необходимости создаёт
дополнительную секцию с callback cells. Ширины экземпляров, selection/peer focus,
повторные hide/show и normal/compact состояния до/после Refresh проверены на Mac.
Сортировка повторных PropID также следует source: raw-флаг берётся у первого
catalog entry, любой клик того же PropID переключает направление, включая другую
generic/raw секцию и append-копию. Qt завершает автоматический mouse sort перед
source handler, затем показывает его направление на нажатой секции. Физический
клик фокусирует owning list; явные программные запросы сохраняют direction и
inactive-owner поведение. Отметка Size определяется PropID.16 EN/RU обе панели,
пустой raw/selection/peer/Refresh и64 normal/compact reports проверены на Mac;
реальные схемы архивов и текущая native/rendered приёмка остаются открытыми.
InitColumns снова использует записанные generic/raw count без дополнительного
HRESULT gate, сохраняя пропуск individual info errors и generic IsDir. Один
provider/type повторно перечисляется при смене схемы. Qt скрывает резервный Name,
если generic descriptor отсутствует; raw Name остаётся отдельной Name-first
source секцией. Пустая схема не создаёт столбцов и сохраняет rows/selection/status,
а Qt сворачивает пустой header до0px. Count остаётся initialized0 для незаписанного
выхода; поведение native uninitialized count не подтверждено.12 EN/RU обе панели
и72 normal/compact reports проверены на Mac; native/rendered проверки открыты.
Выделение без FullRow рисуется в первом source logical subitem, в том числе
raw Name и no-Name; скрытый физический Name больше не поглощает highlight.
Large/small icon и list modes также используют первый source subitem для текста
и существующий значок строки. После hide/re-add raw Name новая append-копия не
заменяет logical zero. EN/RU обе панели и четыре режима проверяются одним
lifecycle; текущая native/rendered Windows приёмка остаётся открытой.
При пустом каталоге Qt сохраняет metadata captions/icon физической колонки0;
native appearance этой fallback-схемы не подтверждён.
Callback text больше не требует повторного item count перед GetItemText: поздний
S_FALSE/E_FAIL/E_ABORT счётчика не гасит valid published name/generic/raw cells.
Ошибки самих свойств сохраняют native Error/empty text; load-time validation и
notify/provider guards сохранены. EN/RU обе панели detail/icon recovery/rebind
проверяются отдельно от archive-operation cancellation и native приёмки.
После Parent ошибка namespace load/count/icon теперь завершает Qt-состояние
операции и оставляет обе панели доступными для Refresh. Адрес связанного
provider публикуется до загрузки, кроме таймерного обновления, как в исходном
RefreshListCtrl. Сортировка и уведомления восстанавливаются и после Cancel;
частичные строки очищаются. Это адаптация завершения глобального Qt busy-state:
исходный ранний RINOK до SetRedraw(true) не считается исправленным в upstream.
Исходное игнорирование результата reload в Parent сохраняется, OnReload остаётся
владельцем сообщения об ошибке. Детерминированные EN/RU сценарии обеих панелей
и normal/compact geometry проверены на macOS; rendered Windows не проверен.
Пока callbacks запрещены, имя остаётся cached или пустым. Реальный ZIP проверяет
точное новое имя/bytes, Escape и invalid-name refusal. Windows marker appearance
и native conditional interface требуют текущей Windows/rendered проверки.
Восстановление selection теперь использует исходный fast-name-first walk,
переиспользует имя при matching и читает prefix только в flat mode. NULL от fast
interface включает исходный property fallback; exception policy не заменяется
новой HRESULT validation. Сохранённые native names/marks/focus не зависят от
форматирования display name или порядка Qt rows. Без provider Refresh сохраняет
исходный ранний S_OK после сброса drop target: callback после освобождения папки
не меняет выбор, фокус, guards или статус и не ставит pending restore. Snapshot
transport имеет настоящий QtPanelFolder и сохраняет обычное восстановление.
Copy/reload потребители остаются
частью обязательной текущей платформенной приёмки.
После matching исходные SetMemMode_Enable/Restore разрешают callbacks перед
восстановлением фокуса. Qt current-cell signal может запросить uncached имя
и обновить доступность команд в этой фазе. Ошибка загрузки и Cancel до неё
сохраняют защиту внешнего guard. Это порядок source notifications, без изменения
геометрии или форматирования имён.

## P3-05.14 — закладки и история source locations (обязательный пропуск, 2026-10-06)

Save хранит source prefix активной
панели, namespace/nested locations входят в историю, пустой Open привязывает
root namespace. Старый display-разделитель преобразуется только после реального
файла архива; внутренние пробелы и сохранённые записи остаются целыми.
Исходный BindToPath повторно использует открытую nested chain; после её закрытия
и при перезапуске останавливается на содержащем внешнем архиве, поскольку
CAgentFolder::BindToFolder привязывает папки, а не открывает файлы архивов.
Native/runtime/rendered приёмка остаётся открытой; проверки и зависимости — audit.

## P3-05.13 — Root/drive view (проверка платформ открыта, 2026-10-06)

Root-команда теперь вызывает исходные CApp::OpenRootFolder/CPanel::OpenDrivesFolder:
закрывает архивные уровни и открывает существующий CFSDrives provider. Ordinary
slash/backslash и keypad slash используют тот же dispatcher. Windows показывает
исходный Computer/диски, Unix — ранее принятый mounted-volume сервис с prefix
Volumes/Тома. Отдельный root namespace доступен через исходный Parent, а Back
восстанавливает сохранённый prefix namespace вместо его подмены пустым root marker.
После восстановления source selection Qt обновляет централизованные команды
вне notification guard; выбранная уже текущая строка остаётся доступной для Open.
Public consumers проверяют обе панели, ключи/команду, Parent/Back, ZIP/nested ZIP
и байты. При смене языка уже открытые drive addresses и focused title обновляются
исходным path/icon/title этапом. Qt сохраняет набранный черновик, строки и
выделение без повторного перечисления дисков — адаптация live language events.
Native Windows/Linux, отрисовка и итоговая приёмка ещё не подтверждены;
текущие проверки и происхождение записаны в port audit.

## P3-05.12 — исходный адресный список (проверка платформ открыта, 2026-10-06)

Выпадающий список адреса теперь строит исходный CPanel: текущая иерархия пути,
границы архивов и отдельные пути привязки, затем виртуальные namespace entries.
Выбор имени родительского компонента открывает полный исходный путь, а история
посещений остаётся в Folders History. Enter/Escape используют исходные edit
handlers; Escape возвращает текущий source prefix с его native separator.
Qt рисует indentation/иконки и доставляет отложенный фокус списка. Запрос
фокуса не перехватывает уже выбранную другую панель или control — адаптация
порядка Qt events. Unix использует смонтированные тома и отдельный Volumes;
Windows сохраняет исходные Documents/Computer/drives/Network.

macOS public consumers проверяют диск, ZIP/nested ZIP, обе панели, выбор,
Escape и сохранность байтов; четыре EN/RU normal/compact JSON проверены.
Полный gate250/250 прошёл. Native Windows UNC/device/refusal, Linux runtime,
rendered Windows reference и итоговая
приёмка остаются открытыми; geometry/source resources не доказывают pixels.

В следующей итерации перенесён отдельный LoadFullPathAndShow: значок закрытого
адреса определяется исходным native prefix и границами архивов. Qt показывает
его через leading action поля ввода вместо ComboBoxEx edit item; filesystem,
archive и namespace renderers вызывают исходный этап своей панели. QFileIconProvider
и Qt namespace icons заменяют shell image list, source extension/directory
fallback сохранён. Прежнее отображение archive separator и исключение nested
virtual paths из истории сохранены явными аргументами. Mac regression проверяет
папки, ZIP/nested ZIP, возврат к внешнему архиву/диску и видимость значка;
полный Mac gate этой итерации прошёл250/250 за103,85с. Четыре итоговых EN/RU
JSON проверены: порядок, размеры combo, видимость редактора и leading icon.
Внутренняя кнопка значка22×18 следует геометрии закрытого редактора и вмещает
значок16×16; её standalone size hint не задаёт размер поля. Native Windows/Linux,
device/UNC, rendered icon acceptance и итоговая приёмка остаются открытыми.

Следующая регрессия проверяет stale-popup: после открытия списка удаляется
только собственная пустая иерархия, выбор мышью/Enter в EN/RU приводит к ближайшей
существующей папке — исходный успешный fallback, а не ошибка. Список, команды,
контрольные байты и повторно построенный popup согласованы. При временно пустом
application focus Qt использует сохранённый window focus widget: исходный запрос
возвращает фокус списка, поздний выбор другой панели не перехватывается.
Offscreen fixture повторно активирует только окно; production его не активирует.
Mac full250/250PASS104,82с, address13Qt/0skip и четыре итоговых JSON проверены.
Native/platform/refusal acceptance остаётся открытой.

Сверка callers также выявила другой обязательный пропуск P3-05.13: исходная
Root-команда и slash/backslash открывают drive view, тогда как текущий Qt service
открывал отдельный root namespace. Исходная CFSDrives/provider уже была;
интеграция OpenDrivesFolder/callers выполнена в следующей итерации выше.

## P3-05.5 — native archive diagnostics (принят scope свойств, 2026-10-06)

Исходные Agent GetArcProp/GetTypeOfArc и Properties теперь читают retained native
levels и точный catalog owner. Type показывает принятый handler, Error Type —
отвергнутый; Parser сохраняет исходное имя. Замена backend catalog не меняет
значение индексов уже открытого архива. NonOpen-уровень использует исходные
diagnostics и CFfpOpen warning flow; diagnostics успешно открытого уровня
остаются в Properties, как в оригинальном GetFolderError/CFfpOpen.
Accepted outer chain с отказом внутреннего binding остаётся доступной через
точные принятые handlers; CLI не повторяет отвергнутый native level.
Process-only metadata сохраняет прежний fallback. Public properties и normal/
compact geometry проверены на macOS, Linux и нативном Windows desktop; full macOS
проходит. Fresh full Linux/Windows выполняются; rendered parity пока не принята.

## P3-05.4 — Alt-arrow folder synchronization (принят scope команд, 2026-10-06)

Эталон — pinned App.cpp OnSetSameFolder/OnSetSubFolder и PanelKey.cpp
VK_UP/LEFT/RIGHT. Alt+Up назначает другой панели текущую папку; Alt+Left/Right
назначает выбранную папку или parent. Одна панель, отсутствие focus item,
обычный файл и отказ binding сохраняют исходные no-op ветви. Стрелка затем
передаётся list control, как в исходном OnKeyDown. Фокус остаётся в источнике.

Qt публикует большие списки асинхронно отдельно для каждой панели: Scan
отключает только её список и оставляет адрес доступным; при отключении списка
фокус переходит в адрес той же панели, чтобы не изменить владельца незавершённого
запуска. Смена фокуса не отменяет публикацию. Новый callback отменяет только
предыдущую публикацию назначения; modal Open/Operation по-прежнему блокирует
обе панели. Прямо связанная архивная папка сохраняет исходный относительный
prefix без копирования parent stack; Qt не добавляет перед ним пустой separator.
Native resource остаётся владельцем engine path после ухода источника из архива.
Normal/compact JSON и public regressions проверены на macOS, Linux и нативном
Windows desktop. Full macOS и Linux проходят; fresh full Windows250/251 имеет
таймаут external_codecs, подробности — source audit/P3-03.
Rendered Windows parity этих состояний пока не подтверждена.

## P3-05.1 — Unix source startup/dispatch (принят scope callers, 2026-10-05)

Unix production entry теперь использует исходные FM format/path/refusal handlers
и GUI parser/dispatch. Windows сохраняет native command-line tokenizer; Unix
передаёт декодированные argv без удаления буквальных кавычек и пробелов пути.
Исходная `-y` policy подавляет progress questions и на Unix. Win32 messages здесь
заменены QMessageBox с исходными текстами; layout новых форм не вводится.
LaunchServices event receiver сохраняет отдельные окна с существующим openPath
consumer. Qt helper и offscreen subprocess tests не подтверждают native Finder
или rendered Windows parity; эти проверки остаются в P3-03/P3-06.

## P2-11.5 — native update transport (принят, 2026-10-05)

При доказанной привязке metadata к native indexes исходные update callbacks
передаются обработчику напрямую; его ошибка или Cancel не запускают повторную
CLI mutation. Исходные explicit properties и reset сохраняются. Для одного
native SFX исходный ReOpen создаёт tail stream, а исходный publisher сохраняет
префикс. Process-only offsets, nested chains и volume sets остаются ограничены
возможностями transport. Переход к native writer не меняет layout диалогов.

Qt metadata transport пока не доказывает физические indexes при повторяющихся
именах: такой архив доступен для чтения, изменение отключено. Это отличие от
непосредственного native index owner Windows 7zFM. Канонически эквивалентные
Unicode-строки также не считаются доказательством тождественности indexes.
Маркированные NTFS streams проходят отдельные исходные правила colon/name и
общую проверку базового пути. Компонент принят на трёх ОС; evidence — audit
§ P2-11.5. Общая rendered UX/platform приёмка остаётся в P3.

## P2-11.1 — единый hover-selection (принят, 2026-10-05)

Исходный ListViewDialog.cpp OnInit применяет LVS_EX_TRACKSELECT вместе с
SingleClick. Диалоги history/properties/hash/messages теперь используют тот же
QtFileListHover, что table/icon lists и временный browser: задержка400мс,
выбор без активации, отмена при press/leave/wheel/hide/disable и удалении строки.
Source OnEnter/clipboard/Delete/confirmation остаются у исходных callers.
Public wheel/hide-show regression сначала воспроизводит два сбоя; общий adapter
устраняет их. Нормальная/компактная структура и тексты не изменялись. Текущие
24 Mac,24 Windows и24 Linux JSON history/hash/temp/progress проверены: видимые затронутые
контролы полностью помещаются в owner, порядок и разделение сохранены.
Scope принят на трёх ОС (audit § P2-11.1); rendered pixel parity остаётся P3-06.
Прежний Windows skip temp-cleanup directory-link fixture устранён отдельным
проверенным шагом: native fixture создаёт настоящий directory link, а текущие
полные Windows запуски выполняют suite без этого пропуска. Глобальная матрица
платформенных проверок остаётся в P3-02.

## P2-10.4 — filesystem/split progress (принят, 2026-10-05)

Filesystem Copy/Move и Split/Combine обновляют исходный CProgressSync из worker,
как CExtractCallbackImp и CThreadSplit/Combine; timer исходного dialog обновляет
Qt controls. Удалена промежуточная GUI очередь каждого chunk; суммы bytes и
native cancellation callbacks сохранены. Result/close joins предшествуют
освобождению dialog. Layout, текст и interval200ms исходного progress не меняются.
Public real-file regressions и приёмка на трёх ОС находятся в audit § P2-10.4.
P2-10 parent принят; итоговая rendered UX приёмка остаётся в P3-06.

## P2-10.3 — volume progress (принят, 2026-10-05)

Создание томов использует тот же latest-state worker transport, что Hash и
creation scan/deletion: промежуточные callbacks исходного CopyCoder объединяются
до GUI timer, pause/cancel checkpoints выполняются в исходных точках. Joined
completion завершает delivery до terminal result; generation исключает события
предыдущего worker при повторном запуске. CopyCoder, CMultiOutStream, exclusive
publication и rollback не меняются. Это Qt transport adaptation; layout/text
Progress/Messages сохранены. Scope принят на трёх ОС (audit § P2-10.3);
filesystem Copy/Move и Split/Combine progress приняты отдельно в P2-10.4.

## P2-10.2 — большие Qt списки (принят, 2026-10-05)

Исходные FSFolder/AgentFolder owners, indices, property/selection и comparator
сохранены. Windows ListView refresh заменён постепенной публикацией Qt rows;
для обычных папок выше 256 элементов исходный LoadItems выполняется в worker.
Во время заполнения address/navigation доступны, команды списка ждут coherent
model. Refresh и выбор созданного файла восстанавливаются после завершения,
переключение панели отменяет старую публикацию и сохраняет pending selection.
Сортировка при включении Qt выполняется один раз с исходным property/order.
Для disk и archive списка сохраняется исходная политика SetRedraw: Qt updates
выключены до завершения сортировки и восстановления выделения. Статус и address
продолжают обрабатывать события. Исправление P3-03.3 от 2026-10-05 включает
disk projection в ту же политику: source `RefreshListCtrl` не перерисовывает
частично заполненный список. Qt batches продолжают возвращаться в event loop,
а окончательная таблица отображается после source sort/selection; navigation
и ошибки восстанавливают updates. Предыдущее исключение для filesystem redraw
не устранило Windows timing failures и заменено этой source policy.
Смена Qt view сохраняет panel owner и фокус списка: upstream меняет style
того же ListView. Отмена scan при смене панели освобождает guards для команд
готовой панели. Обычные Windows folders обновляются через исходный watcher;
root recovery и flat descendant fallback остаются в Qt transport.
При Open/Operation адресные combo отключены вместе со списками: исходный
progress использует modal DialogBoxParam. Это исключает переход фокуса на
неактивную панель, пока source guards подавляют соответствующее уведомление.
После завершения combo вновь доступны; при Scan навигация остаётся доступной.

Layout/grouping/order не меняются. Public 4096-item disk/archive fixtures
проверяют heartbeat, normal 980×620 и compact 640×480 JSON, navigation/close и
stale turns. Scope принят на трёх ОС; parent принят после P2-10.4. Эти проверки структуры
не доказывают pixel parity с rendered Windows reference.

## P2-10.1 — worker scheduling (принят, 2026-10-04)

Подготовка archive update теперь использует исходный CCopyStateIO в worker;
Pause/Cancel действуют до запуска engine, незавершённая копия имеет отдельный
private lifetime. Последовательность source mutation/plan и engine stages
сохранена. Qt latest-state delivery соответствует исходному принципу
CProgressSync и timer presentation: промежуточные имена и проценты могут
объединяться, итоговые результаты/ошибки HashCalc сохраняются. События завершённой
фазы не доставляются после её terminal result. Это transport adaptation без
изменения структуры Progress/Messages и без утверждения pixel parity.
Полная отзывчивость Qt rows/icons принята в P2-10.2; parent принят после P2-10.4.
Windows native separators приводятся исходным ReplaceSlashes_OsToUnix только
для comparison/validation совместимых mutations; raw listing не меняется.
Итоговый macOS gate236/236 PASS95.26s; snapshot90ef1e7f Windows9/9 PASS42.83s,
Linux9/9 PASS9.38s. Это backend/native-fixture coverage, без rendered UI parity.

## P1-12.2 — Options container (принят, 2026-10-04)

Исходный OptionsDialog/MyPropertySheet открывает System, а Qt выбирал Settings.
Стартовая вкладка исправлена; порядок и source IDs шести страниц сохранены.
Общий Apply/OK/Cancel/restart scenario проверяет pending/committed значения,
ассоциации через изолированный сервис, callbacks и посторонние настройки.
Реальный MainWindow обновляет обе панели с сохранением выделения. Ctrl+Tab
использует обычный Qt focus cycle: Windows может посетить footer перед полями.
F3/F4 проверены с ожиданием активного окна и сигнала действия, а затем аргументов
реального helper-процесса. 72 current EN/RU normal/compact page JSON трёх ОС
просмотрены без clipping. macOS full234/234 PASS96.26s; Windows59cf7420 8/8
PASS21.05s; Linux4b9f3d65 8/8 PASS2.91s до локальных test-only corrections.
Pixel correspondence и полная строка Options в UX audit остаются открыты.

## P2-09.3 — Menu integration (принят, 2026-10-04)

Восстановлены исходные имена adjacent shell DLL. Cascaded/Icons/ElimDup и
список команд теперь доступны для перенесённых source consumers, Apply меняет
исходный QueryContextMenu/InvokeCommand, Cancel сохраняет прежние flags.
Windows показывает доступную native registration и отключает отсутствующую
другую bitness; Unix оставляет native registration/Zone unavailable при
доступных in-app policies. Двенадцать current EN/RU normal/compact Menu JSON
трёх ОС просмотрены: grouping/order/exposure сохранены. Windows607eb66a
проверил настоящую регистрацию, DLL activation и обнаружение системным Shell
provider с последующей очисткой. Итоговый snapshot1f968069 проходит Windows7/7
9.68s и Linux6/6 1.10s: реальная DLL получает только Extract Here, flat menu и
source bitmap. Свежий INI regression проверяет чтение true/false после restart;
исправлен только транспорт значений. macOS full233/233 PASS96.35s. Приняты
P2-09.3/P1-13.2 и родители P2-09/P1-13; общая Options приёмка — P1-12.2.
Pixel parity Menu page и глобальная UX-приёмка здесь не заявляются.

## P2-09.2 — Native Windows component (принят, 2026-10-04)

Итоговая приёмка компонента заменяет промежуточные ограничения ниже.
Проверены native Properties, COM/DLL, GUI/map, email, startup и установленный
consumer. Реальная регистрация Explorer handlers и Menu policies остаётся
P2-09.3. Provenance и отдельные результаты —
[audit](upstream-port-audit.md#p2-092--native-windows-component-accepted-2026-10-04).

System presentation проверена с реально запущенным оригинальным7zFM pinned
revision:22 строки исходного HWND/HMENU, raw query и Qt совпадают по порядку,
состояниям и группировке. Два PNG affected меню просмотрены; исправлена
вертикальная инверсия Share icon. Отдельные GDI regressions проверяют обе
ориентации DIB, прозрачность, owner-drawn строки/подменю, callback icons и
IContextMenu2/3 keyboard/mouse messages. Windows1c7b58c0 проходит3/3 suites,
reference0a46f9dd —1/1 без skips.

Повтор на Qt6.12 Windows11 x64 2026-10-05 (snapshot27c52b66) прошёл1/1,
3 Qt cases без skips. В текущем состоянии providers меню содержит21 строку;
native HWND/HMENU, raw query и Qt projection снова совпадают по порядку,
группам, состояниям, default action и подменю. Оба affected PNG просмотрены:
Open, Defender/Share icons, separators и labels видны без clipping. Это
повтор bounded System-menu reference; остальные native dialogs не приняты.

При96DPI обе стороны используют Segoe UI9pt; QWindows11Style оставляет свои
24px строки, отступы, цвет фона и скругление вместо native22px строк/9px
separators. Это Qt style adaptation; точное совпадение пикселей и другие DPI
не заявлены. Системные verbs сохраняют язык Windows независимо от app locale.
Воспроизводимый reference и изоляция настроек описаны в
[Windows UI reference](windows-ui-reference.md#pinned-runtime-system-menu-reference).

Исходные IContextMenu/IShellExtInit/IExplorerCommand/IEnumExplorerCommand и DLL
factory теперь подключены к одному command owner. Windowsfcf2fe1c прошёл2/2
native suites1.79s: реальный DLL consumer, legacy selection/HMENU, nested children
после освобождения root, icon extraction и native host без QCoreApplication.
Это source/runtime evidence; installed Explorer, оставшиеся GUI/map consumers, native System
rendering и полная UX-приёмка компонента остаются открытыми. Проектный CLSID
отдельный от официального7-Zip; filename7-zip.dll сохранён в каталоге проекта.

Windows email теперь использует исходный Update.cpp/MAPI, включая Unicode-first,
ANSI fallback, восстановление cwd и lifetime временного вложения. Пять процессных
сценариев с частным тестовым provider прошли; реальные почтовые аккаунты не
используются. Три исходные File Manager email-команды доступны по native update
capabilities; snapshotf9834a5a прошёл оба email suites2/2 за11.47s, включая14
File-menu rows: success/selection, visible Add Cancel и missing-export refusal,
без skips. Текущие двенадцать
EN/RU normal/compact JSON сохраняют исходный порядок трёх строк, Unicode captions
и fullyVisible=true: menu334×78 EN и373×78 RU, rows24px. Это проверка геометрии,
а не rendered comparison с Windows7zFM. На Unix применимой MAPI-ветви нет.

Windows FM startup теперь передаёт исходные path/type options прямо в CApp::Create
при создании окна. Сохраняются original quoted-string splitting, активный суффикс
`-t`, saved-panel fallback и WM_CREATE refusal: ошибка до появления File Manager,
тихий E_ABORT и FALSE/0. Qt отображает исходное сообщение через QMessageBox и
выполняет event loop; окно не открывается повторно после default startup.
Desktopb6182130 подтвердил фактический7zFM.exe: Unicode ZIP с `-tzip`, отказ с
`-t7z`, visible state и нормальное завершение обоих своих процессов. Ранее2ed23e18
проверял только in-process startup, поскольку MOC пропустил process slots.
Это runtime/command evidence; installed extension, native System rendering и
полная UX-приёмка остаются открытыми.

Эталон — pinned PanelMenu.cpp::Properties/InvokeSystemCommand и исходные
interface tables FSFolder/AltStreamsFolder. File Properties и toolbar Info теперь
входят в CPanel::Properties: Windows filesystem вызывает системный property sheet
через исходный IContextMenu verb, архив — исходный CListViewDialog collector.
Системные подписи следуют языку Windows, а не языку приложения. Windows ADS
сохраняет исходную no-op ветвь: provider не имеет IGetFolderArcProps и исключён
из InvokeSystemCommand. Прежнее дополнительное Qt-окно здесь удалено из workflow.
На Windows11 fixture подтвердил HWND, правильный заголовок и visible=0.
Явный SW_SHOWNORMAL в snapshot0296e0a7 не устранил скрытое состояние;
предположение о нулевом nShow как причине не подтвердилось. Причина и
доступность системного окна пользователю остаются предметом диагностики.
Последующая диагностика отделила SSH session0 от активного console desktop:
тот же file consumer проходит в session1 с exposed owner и visible native sheet.
Полный16-row consumer и missing-selection refusal затем прошли через
воспроизводимый `--interactive` runner в snapshotd12dce13; завершённая задача
удалена. Это runtime evidence всех four-view/flat/file/folder состояний,
а не rendered pixel-parity приёмка.
Unix не имеет Win32 COM property provider и использует прежний portable source
collector для filesystem. Нативный Windows fixture и остальные .2 consumers
ещё проверяются; runtime/visual приёмка не заявлена. Provenance и проверки —
в [audit](upstream-port-audit.md#p2-092--native-windows-component-accepted-2026-10-04).

## P2-09 — Shell popup (в работе, 2026-10-03)

Native System WIP 2026-10-04: перенесены исходные PanelMenu PIDL/query bodies и
CMINVOKECOMMANDINFOEX invocation; QtNativeShellMenu удерживает COM/HMENU на одну
генерацию и показывает исходное дерево в System submenu на Windows. На Unix
Windows System transport остаётся unavailable. Snapshot13edb0ee прошёл Windows
7/7 suites36.79s zero skips; восемь EN/RU normal/compact JSON просмотрены:
29 root rows с System первым и19 child rows без clipping. Подписи native verbs
следуют языку ОС (EN), независимо от RU app labels; это системный provider.
Native icons, owner-drawn handlers и rendered System comparison ещё не приняты;
COM extension/installation/email остаются открытыми. Старые записи о disabled
System ниже относятся к снимкам до этого подключения.
Текущий Windows snapshotc1b68eed прошёл8/8 suites35.54s zero skips: настоящий
native Copy передаёт выбранный Unicode-файл в clipboard и снимает выделение;
устаревшие QAction не вызывают новую команду. Повторно просмотрены восемь JSON
текущего build: исходный порядок root29/child19, без clipping. Это не заменяет
rendered comparison native icons/owner-drawn handlers и installed COM UX.

2026-10-04, уточнение обязательной UX-сверки: независимые File-команды журнала
и встроенного Text View/Edit удалены вместе с FileViewerDialog и его preview/
update wrappers. Исходные F3/F4 и Progress/Messages сохранены. Полный macOS
format/build/229CTest PASS97.44s подтверждает эту миграцию. Последующее исправление
Archive caption и независимой строки памяти распаковки проверено шестью suites
(9.06s), восемь новых EN/RU top/bottom JSON просмотрены. Новый snapshot
20261004T032721Z-e0a5aaaa проверяет49 suites на Windows/Linux; итог ещё неизвестен.
Это не приёмка P2-09 и не заявление о полном визуальном соответствии.

Эталон — pinned PanelMenu::CreateFileMenu/CreateSevenZipMenu/InvokePluginCommand
и Explorer ContextMenu::QueryContextMenu/InvokeCommandCommon. Дисковое меню
теперь использует исходный генератор путей, labels, verbs, IDs и grouping.
Qt рисует исходный MenuLogo.bmp; Add to7z/ZIP без диалога и SHA256 generation
используют source CompressCall2/UpdateGUI/UpdateArchive. Native System,
email остаются disabled; generic Add теперь открывает fixed-selection source
modal. Source SFX теперь получает проверенный native module из retained catalog;
обычный, encrypted и split SFX действительно запускаются на macOS. Новый
platform snapshot ещё проверяется; прежний Windows v10 показывает старый код.
Обычный toolbar Add использует исходный CPanel::AddToArchive, включая refusal
в архиве и пустое выделение; встроенный picker удалён из всех ветвей. Drag Add
использует исходные CompressCall2/UpdateGUI и native update. Семь форматов
toolbar Add и unknown-extension update через исходный two-panel Copy проверены.
Snapshot20261003T231240Z-201ad78d:Windows16/16PASS85.76s иLinux16/16PASS29.23s;
этот снимок предшествует следующему browse port и исправлению test fixture.
Обработчик `...` теперь переносит исходный OnButtonSetArchive: filters/index,
extension/format/alias/SFX/Cancel. Платформенный QFileDialog отображает исходные
descriptions, скрывая дополнительную Qt mask suffix. macOS expanded18/18
PASS15.10s; свежие full/Windows/Linux проверки текущего browse ещё выполняются.
2026-10-04: snapshot20261004T000254Z-d1a630c3 прошёл Linux19/19(31.68s),
Windows18/19(78.74s): native archive Move больше не падает, source browse проходит;
sole modal-refresh test сравнивал Windows caption с Qt `/` без нормализации.
Проверка исправлена; повтор platform validation текущих дальнейших изменений
остаётся открытой. macOS current20/20PASS18.93s включает flat-scan cancellation.

Настоящий Qt Add v10 снят и просмотрен рядом с исходным Windows 7zFM:
`build/runners/ui-reference-20261003/qt-application-add-v10/application-add.png`,
JSON того же имени, native642×546, executable SHA256
`a8cf7ce10b300a142c7abd1b15fac23548250da8aba0e9b10a86531beecad0e3`.
Capture task result0; собственная задача удалена, диалог отменён без создания
архива. Picker отсутствует, две колонки, encryption method в одной строке,
decompression value справа и footer доступны. Original reference остаётся
`20261003T203028687Z-f7f64f.png`, pinned7zFM26.03. SFX в оригинале доступен,
в Qt source consumer пока disabled; `*` у automatic compression method пока
не перенесён. Это открытые отличия, а не подтверждённое полное visual parity.
Снимок v10 предшествует удалению неиспользуемой existingTarget ветви и widget
preflights, но показывает production source form того же layout.

Последующая source SFX сборка прошла macOS full230/230(92.14s), а fresh snapshot
`20261004T010728Z-b6571bdb` — Windows24/24(114.99s), Linux24/24(35.68s).
Включены настоящий source toolbar SFX plain/password/volumes/Cancel и нативный
запуск созданного executable. Windows flat-view separator/cascade тоже больше
не воспроизводится. Эти результаты предшествуют переносу method selector.
Теперь исходные SetMethod2/GetMethodSpec дают automatic `*`, пустой source spec
и очищение Store; локально13affectedPASS10.49s, новая VM проверка идёт.
Оригинальный Windows переход SFX→ZIP дополнительно выполнен: остаётся только
LZMA, SFX выключен/disabled. Такое исходное состояние сохранено и проверяется,
прежнее Qt ожидание Deflate удалено. Capture result0, Cancel без создания архива,
completed task удалён; JSON — original-method-transition-20261004/states.json
в той же локальной директории reference. Новый default runtime PNG ещё требуется.

Actual Qt v11 default Add теперь снят и просмотрен рядом с оригиналом:
`qt-application-add-v11/application-add.png` и JSON в локальном reference,
snapshot98243ecb, native642×546, executable SHA256
`8be93735b703202d59634bcca77acdd37d68decc3b74663fe501747792fbb9de`.
Capture result0; свой completed task удалён, modal отменён. SFX доступен,
automatic `*  LZMA2` присутствует, picker отсутствует, footer доступен.
Этот снимок проверяет default state; он предшествует исправлению отсутствующего
method selection при SFX→ZIP, которое не меняет этот default. Native v2 probe
дополнительно установил selection -1 после перехода; Qt больше не навязывает
первый item, local13affectedPASS12.59s. Source-модель и runtime validation
этого последующего исправления фиксируются отдельно от older29/29 snapshot.
В v11 найдено ещё одно структурное отличие: folder prefix занимает всю ширину,
а source.rc задаёт его и Archive combo в одной колонке справа от Archive label.
Также Windows display prefix использует `/`; оба отличия исправлены и проверены
в следующем native v12. Начальное полное выделение имени тоже восстановлено.
Исходный порядок System → 7-Zip → File
сохраняется без дополнительного Qt separator, отсутствующего в source body.

Shell Open запускает отдельный File Manager. Вместо нового OS process Qt
создаёт отдельное окно с независимым backend и общей immutable module lease;
родительская папка не заменяется. Тип архива передаётся отдельно и сохраняется
до BindToPath. Отказ archive probe не выдаётся за успех привязки к папке.
Отмена пароля не показывает вторую ошибку и не сбрасывает selection; успешный
source InvokePluginCommand очищает selection. Native Open success/refusal/cancel
проверены через действующее popup; байты архива сохранены.

EN/RU normal/compact action geometry и четыре текущих widget JSON проверены на
macOS; один PNG только для новой bitmap rendering также просмотрен. Source
resources не подтверждают Windows pixel parity. Native extension, program File
menu и toolbar aliases, email и installed Windows checks ещё
обязательны; P2-09 не принят.

2026-10-04: current Add snapshot `20261004T020838Z-07e2aeb5` прошёл
Windows29/29PASS123.49s и Linux29/29PASS38.34s. Actual Qt v12
`qt-application-add-v12/application-add.png/.json` лично просмотрен рядом с
original PNG: native642×546, SHA256
`6cb83ba8cbc609ccf9acb013915d5e3ae6ac453cfb4b6d0d37b89851b5001ce9`.
Путь и имя делят одну колонку, Windows prefix использует `\`, имя выделено,
SFX и automatic `*` доступны, footer не перекрыт. Capture result0, собственная
задача удалена, Cancel без создания архива. Отступы, ширина compression controls
и расположение Archive caption всё ещё отличаются от native reference;
это не подтверждение полного визуального соответствия.

Программное File теперь создаётся через исходный
`MyLoadMenu.cpp::OnMenuActivating` File branch и `CPanel::CreateFileMenu`.
Qt сохраняет static resource actions отдельно от disposable shell rows и
retained plugin context; `CApp::ExecuteCommand/ExecuteFileCommand` вызывает и
освобождает исходный plugin. Reopen не дублирует строки, старый QAction не
вызывает новый context, смена focused panel отклоняет старую команду.
Native program-menu Open success/forced-type refusal/password Cancel и
Add success/broken-existing/Cancel проходят на macOS. Четыре EN/RU980/640
File JSON и дочерние shell JSON просмотрены: source order, Exit последним,
лишних aliases в открытом меню нет, все action rectangles доступны. Сами
legacy actions и workflows пока существуют; их удаление остаётся обязательным.
Новые File edits проверяются отдельно свежим snapshot0b1bdcab и full gate.

Позднейшее исправление Info: исходный `App.cpp::g_StandardButtons` связывает
Info с IDM_PROPERTIES, теперь эта же команда используется в Qt. Самостоятельный
Archive information screen и extension удалены. Properties открывает исходные
item/folder/archive sections и для пустого архива, как PanelMenu::Properties;
label кнопки остаётся исходным Info. Actual item/empty archive и filesystem
consumer во всех flat/view режимах, EN→RU→EN, unchanged backend calls проходят:
five direct suites3.16s и nine policy/source/menu consumers9.76s на macOS.
Четыре current540/360 EN/RU widget JSON просмотрены: list, buttons и footer
полностью доступны. Native Windows System Properties всё ещё требует source
System transport; Windows runtime сравнение этого последующего изменения открыто.
Предыдущий full230PASS95.22s и snapshot0b1bdcab предшествуют Info correction.

Direct Add/generation popup проверен на Unicode/spaced файлах: official7zz
подтверждает выходы, успешный source invoke очищает selection, результат
появляется в списке через существующий auto-refresh. Повреждённый existing7z
отклоняется с одной ошибкой и сохранением bytes/selection. GUI worker Cancel
останавливает source callback и удаляет свой выход. Восемь EN/RU normal/compact
root/hash JSON с текстом/verbs/geometry/enabled просмотрены; Qt bounds287×92
EN/316×92 RU и hash355×301 fully exposed. Свежие focused Hyper-V runs проверили
direct popup и source worker на Linux/Windows; общий Windows run выявил четыре
сбоя в baseline/context/SFX consumers. Повторные focused runs после fixes и
source ShowDialog прошли: Windows17/17PASS56.10s, Linux17/17PASS13.10s.
Installed native extension UX и rendered Windows parity не подтверждены;
screenshots для новых command-only изменений не потребовались.
Восемь current-build EN/RU normal/compact root/hash JSON каждой VM также
просмотрены: source order/verbs/enabled сохранены, все видимые action rectangles
exposed; ширины Windows455/503/hash563, Linux273/305/hash342.

Source UpdateGUI::ShowDialog теперь сохраняет исходный options roundtrip,
action sets, password и properties. PresentCompressDialog подключён к настоящему
fixed-selection Qt modal с retained catalog/external methods; success, broken
existing archive и Cancel проверены с native UpdateArchive. Новые исправления
Add по замечанию пользователя ещё проходят текущие gates.

Windows runtime эталон Add получен 2026-10-03: 7zFM26.03 собран без изменений из
commit0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17, executableSHA256
310fc41187fe3482de653a12cb3fc27d2826f9d18e95bd9a7835bf44cfc54151.
Снимок `application-runtime`, 52 controls,96DPI, client624×546 показывает
folder label над Archive+`...`, две колонки и OK/Cancel/Help, без выбора источников,
Save in и preview. PNG и JSON реально просмотрены, локальные артефакты —
`build/runners/ui-reference-20261003/20261003T203028687Z-f7f64f.{png,json}`.
Это эталон одного состояния, не доказательство совпадения Qt или остальных форм.
Qt-форма отрисована нативным Windows-плагином и реально просмотрена в
`build/runners/ui-reference-20261003/qt-rendered-add/source-add-{en,ru}-{760,640}.png`
с JSON и нижними scroll states. Это shared форма из тестового executable,
а не проверка всех команд собранного приложения. Source fixture специально
задаёт Freshen/Full paths/password, поэтому значения этих полей не сравниваются
с исходными defaults эталонного окна. Сверка обнаружила сокращённую строку
памяти и неверное положение её label; исходный PrintMemUsage и верхний label
исправлены и проверены в новых native Windows reports. SFX в source modal остаётся отключённым
и не принят. Наше дополнительное отключение Delete при смене update mode удалено:
исходный OnInit оставляет checkbox доступным, UpdateArchive проверяет action set
после OK. Успех, Cancel и отказ с сохранением источника проверены.
Отличия шрифтов и отрисовки Qt не означают pixel parity.
Обычный filesystem Add теперь получает panel selection без picker; column
grouping, folder label и footer приведены к этому состоянию. Legacy Add в
открытый архив и browse filters ещё требуют source/workflow исправления.
Все подобные допущения обязательны к разбору в [ux-port-audit.md](ux-port-audit.md);
прежняя запись Qt adaptation не закрывает их.

При нативной Windows проверке Copy из вложенного архива Qt после закрытия
диалога активировал противоположную панель, и consumer молча отбрасывал запрос.
Исходный OnCopy сохраняет source/destination refs. QtAppPresentation теперь
восстанавливает проекцию захваченного источника до ExecuteCopy; изолированная
регрессия Windows проходит1/1 в1.37s. Свежий snapshot
20261003T215547Z-2dad9bab прошёл Windows13/13PASS65.39s и Linux13/13PASS25.17s;
macOS full227/227PASS81.24s включает это исправление.

Настоящий Qt application-runtime Add через toolbar также снят и просмотрен:
`build/runners/ui-reference-20261003/qt-application-add-v8/application-add.{png,json}`,
snapshot20261003T212905Z-62e3141a, appSHA256
473f207e78581b6427ed91e9ec66db3e9e4da55a05c4a0748278ec48d94a5ef.
Выбор панели и полная строка памяти видны. Оставшийся default760×720 скрывает
footer под Windows taskbar; это не допустимое отличие. Размер по содержимому
и доступной области экрана исправлен с новой проверкой без test resize.
На macOS новые focused13/13PASS14.50s, default640×571 JSON просмотрен:
колонки, параметры и footer доступны. Свежий Windows snapshot
20261003T221959Z-6da11f81 прошёл13/13PASS70.86s. Повторный настоящий
application-runtime v9 PNG/JSON просмотрен: окно642×578 полностью помещается,
все три кнопки доступны. AppSHA256
c4d9055f35ed16398677f2ed3815573d18579818a94a846c259e2c13c0c6b712;
артефакты в `build/runners/ui-reference-20261003/qt-application-add-v9/`.
Собственная capture task завершилась0 и удалена. Этот sizing snapshot
предшествует последующему переносу CPanel::AddToArchive, не подтверждает его
workflow. Последняя полная проверка также предшествует новым Add source edits.

## P2-08 — System page (принят 2026-10-03)

Эталон — pinned SystemPage.rc/.cpp: File type, current-user и All users columns,
две кнопки «+» и исходные правила выбора/переключения. CSystemPage перенесён
на Qt с исходными state/apply bodies и подключён к Options/MainWindow.
Список расширений и Windows DLL icon lookup берутся из исходного владельца
ArchiveFolderOpen, а не из всех detection aliases. EN/RU normal/compact System
geometry и resource contract проверены на macOS, Windows и Linux: по четыре
актуальных отчёта. Заголовок текущего пользователя получает native username,
как в исходном Windows OnInit, с локализованным fallback. Qt Icons/native
provider не подтверждают pixel parity.

Windows сохраняет исходную регистрацию HKCU/HKLM и UserChoice: для выбора
эффективного default app может потребоваться Windows Settings, о чём сообщает
tooltip кнопки. Linux поддерживает current-user GIO MIME associations только
при installed desktop entry, указывающем на этот executable; неизвестные типы
отключены. macOS12+ требует .app bundle и использует native consent/completion;
Clear восстанавливает сохранённый предыдущий handler, а отсутствие истории
даёт отказ. All users на macOS/Linux disabled с причиной. Изменение native
MIME/UTType действует на все aliases этого типа. External revision перед Apply
проверяется; обнаруженный конфликт обновляет строку и требует нового выбора.
Проверка не является atomic OS transaction, а native ошибка может последовать
за частичным изменением. Options остаётся открытым для просмотра ошибки.
Windows revision detection now omits an absent ProgID instead of enumerating
the entire Classes root. Unrelated registrations no longer invalidate that
association; repeated Options initialization avoids the root scan. The native
missing-association regression and two Options openings pass on Windows;
the P2-11.4 prerequisite in the port audit records the current full-gate status.
Исходные проверки этого шага использовали fake registration services; OS associations
в разработке не менялись. macOS full gate219/219PASS67.46s, последующие
локальные проверки9/9PASS3.21s; Windows12/12PASS25.26s и Linux12/12PASS3.17s.
Полные Windows/Linux gates и установленная native регистрация остаются P3.

macOS .app объявляет source association extensions как Viewer/Alternate и
принимает QFileOpenEvent, включая ранние и повторные local requests. Запрос
идёт через существующий openPath в отдельном окне, чтобы не менять активную
операцию другого окна; Windows upstream запускает отдельный экземпляр для
association command. На первом native запуске может оставаться начальное окно
приложения: это явно отмеченная Qt/native adaptation, не pixel parity claim.
Command-line startup сохраняет свой прежний путь. Fake consumer/bundle checks
не регистрируют реальные defaults. Этап принят в этом явно описанном scope;
rendered Windows reference и отдельная native Finder приёмка остаются P3.

## P2-07 — Extract и directory startup

Эталон — pinned ExtractDialog.rc/ExtractDialog.cpp, ExtractGUI.cpp,
Common/Extract.cpp, ArchiveExtractCallback.cpp, FM.cpp и BrowseDialog2.cpp.
Qt восстанавливает Absolute paths, Eliminate duplication of root folder между
Path mode и Overwrite mode и Restore file security под Password. NtSecurity
доступен только в Windows; Unix показывает disabled control с причиной.
Для выбранных объектов и текущей архивной папки ElimDup disabled: исходный
Agent workflow не выполняет Common root elimination. Persistence сохраняет
source precedence/defined-value rules; Cancel не записывает изменения.

Absolute extraction допускает только пути внутри выбранной папки назначения;
исходный 7zFM допускает произвольные абсолютные пути. Это явное ограничение
интеграции перед созданием source-corrected output. Проверенный native handler
восстанавливает archive links через исходные deferred callbacks; вычисленная
цель должна оставаться внутри назначения и не проходить через существующую
ссылку. Absolute link targets перебазируются по исходным правилам, включая
ElimDup. File-only transport ссылки отклоняет. POSIX fixtures проверены;
Windows native link fixtures тоже проверены, включая cyclic links; Unix TAR
absolute link target на Windows отклоняется по исходным правилам. Positive
WIM relative/absolute file и directory reparse targets проверены чтением байтов.
Cancel на overwrite prompt сохраняет уже
завершённый файл и существующий конфликтующий файл; последующая ссылка
не создаётся. Исходный архив и входные файлы сохраняются.

Add → Options использует исходные SymLinks/HardLinks/AltStreams/NtSecurity
bool pairs и флаги выбранного handler. ZIP/7z в upstream не объявляют kSymLinks
для GUI, хотя CLI может сохранять ссылки; эта разница сохранена. TAR показывает
symbolic/hard links, NTFS streams/security доступны только Windows. Options
Cancel не меняет Add, Add OK сохраняет настройки, смена формата фильтрует
неподдерживаемые запросы. Source TAR writer и processed-input deletion
проверены на macOS и Linux, включая actual MainWindow Add/Options и Extract
выбранной/текущей папки в EN/RU. Cancel Add сохраняет архив и входные ссылки.
Windows actual MainWindow metadata consumer и девять focused CTest прошли.
Restore file security проверен с опцией on/off на настоящем NTFS DACL.
Native WIM Add сохраняет security/ADS по независимым опциям, failure/Cancel
сохраняют источники, успешная публикация предшествует processed deletion.
После первого ADS source proxy получает обновлённый native capability flag.

Standalone ZIP/7z на Windows отклоняет запрос сохранения ссылок до запуска
engine: фактический CLI round-trip теряет directory reparse metadata. При
Delete after compressing это приводило бы к потере исходной ссылки. Native
TAR/WIM использует объявленные исходным handler capabilities; POSIX standalone
ZIP/7z сохраняет прежний проверенный режим.

Open Outside : 7-Zip для owned temp directory больше не disabled. Аргумент
проходит через существующую native navigation; detached запуск сохраняет
unconfirmed external protection, поскольку подтверждённого exit handle нет.
P2-07 принят с focused runtime checks всех трёх платформ. Полные Windows/Linux
gates и rendered Windows parity не заявлены; resources и widget JSON
подтверждают структуру, порядок и доступность controls, а не pixel fidelity.

## P2-06 — Alternate streams (принят, 2026-10-03)

Эталон — IDM_ALT_STREAMS в FileManager.rc, исходный CPanel::OpenAltStreams,
AltStreamsFolder и Agent/CProxyArc2. File → Alternate streams открывает потоки
выбранного файла либо текущей папки. Имена, размеры, Packed size и Properties
получаются из исходного folder interface. Copy/Move/Delete/CreateFile/Rename
для NTFS вызывают исходные CPanel workers и AltStreamsFolder; CreateFolder и
импорт в эту FS-папку недоступны, как в активных исходных E_NOTIMPL methods.
Base bytes/attributes сохраняются при операциях над потоком.

На macOS/Linux filesystem command недоступен: xattrs не выдаются за NTFS ADS,
файлы с двоеточием не создаются как имитация потоков. Архивная ADS-папка и
native raw/tree properties переносимы. Архивные WIM Rename/Delete используют
исходные Agent callbacks и нативный IOutArchive, сохраняя родительские индексы.
После удаления последнего потока исходный Restore FolderItem возвращается к
обычной папке владельца: ADS-ветка больше не существует.

Windows actual EN/RU archive ADS Copy/Rename/Delete Cancel/Yes, проверка `7zz t`
и байты основного файла прошли. Filesystem Create Cancel/Create/Rename, Copy/Move,
Delete Cancel/Yes и Properties проверены на NTFS. EN/RU980×620/640×400 JSON
сохраняют порядок toolbar→address→table→status; geometry подтверждает Qt
структуру, но не rendered 7zFM pixel parity. macOS gate207/207PASS120.58s;
финальные direct consumers macOS2/2, Linux3/3 и Windows drag/contract2/2
плюс семь Add-сценариев прошли без пропусков. Расширенный Windows source-update
набор остаётся открытым для P2-07/P3 (absolute/Sync assertions и таймаут).

На macOS те же archive ADS операции проверены с синтетическим Windows WIM,
включая Unicode-имена, точные байты потока и сохранение основного файла.
Console transport включает `-sns` по исходному `kpidIsAltStream`. Имя элемента
передаётся через приватный UTF-8 список, когда Qt преобразовал бы argv в NFD;
сравнение имён в архиве сохраняется точным. Имена с переводами строк в такой
комбинации требуют нативного extract service и получают явный отказ.

Связанные codec regressions учитывают исходное ограничение gzip: GzHandler
кодирует сохранённое имя через CP_ACP на Windows. Произвольное Unicode-имя
в этом поле не гарантируется; путь к самому архиву и входной папке остаётся
Unicode. Алгоритм и формат заголовка для обхода ограничения не изменены.

## P2-05 — Link и filesystem metadata (принят, 2026-10-03)

Эталон — исходные LinkDialog.cpp/.h/.rc и CApp::Link. File → Link доступен для
одного filesystem item; defaults, Link from/to, browse, current target, пять
radio types и Link/Cancel сохраняют source order и validation. Nonempty plain
file нельзя скрыть reparse data. Qt заменяет Win32 controls и координаты;
compact форма прокручивается, длинные EN/RU radio captions переносятся.
Normal/compact geometry проверены на macOS, Windows и Linux; rendered 7zFM
pixel parity этой проверкой не подтверждается.

Windows сохраняет исходные hardlink/symlink/junction/reparse и process privileges.
Unix поддерживает hard/symbolic links, Junction/WSL видны с причиной недоступности.
POSIX edit атомарно заменяет link inode через private same-parent stage; Clear
удаляет inode. Windows Clear сохраняет пустой исходный file object после удаления
reparse attribute. POSIX target classification следует target только для выбора
file/directory type; создание и перенос сохраняют raw target.

Copy/Move/Delete обращаются к ссылке как к leaf object, включая dangling/cycle
и Windows directory junction. Это явная адаптация source copy flow: directory
links не обходятся. Raw target и поддерживаемые link timestamps сохраняются;
destination links остаются запрещены transaction boundary. Linux real EXDEV и
forced fallback на остальных ОС проверяют copy/publish/delete. Новые файлы и
папки сохраняют поддерживаемые timestamps/attributes/mode. Unix ownership,
inode/ctime и link permissions определяются ОС. Archive storage/extraction links
и security policy остаются отдельными backend workflows; filesystem Link не
включает их. Full macOS gate205/205PASS82.95s; focused Windows7/7PASS30.24s
и Linux7/7PASS0.15s без пропусков;12current-build JSON reviewed.

## P2-04 — Benchmark (принят, 2026-10-03)

Эталон — исходные BenchmarkDialog.cpp/.rc, Common/Bench.cpp и Tools dispatch.
Qt сохраняет Dictionary/Threads, Current/Resulting для Compressing/Decompressing,
исходные Size/Speed/Usage/RPU/Rating, elapsed/passes/total и Restart/Stop/Cancel.
Числа рассчитывает исходный Bench и callbacks: текст CLI не содержит весь набор
метрик окна. Нативный worker и исходные отказ по памяти, остановка и join
сохранены; backend удерживает codecs/hashers и engine/platform services.

Qt layouts заменяют Win32 координаты, fixed-font console — исходный CMyFont.
Компактный normal pane прокручивается; footer остаётся видимым. Total mode
сохраняет elapsed/console, но не добавляется в обычное Tools menu: в исходном
ресурсе Benchmark2 относится к UNDER_CE. EN/RU captions обновляются в открытом
окне, включая исходное сопоставление стандартных кнопок402/409.
Проверены normal/total EN/RU JSON всех трёх платформ; labels доступны при
прокрутке, footer остаётся видимым. Spanning captions имеют естественную ширину,
переносимые системные строки сохраняют нужную высоту при source text update.
Full macOS gate202/202PASS89.68s + финальные affected3/3PASS5.82s;
Linux5/5PASS3.34s + финальные3/3PASS2.28s; Windows5/5PASS4.09s, zero focused
Qt skips. Rendered Windows pixel parity и полная приёмка всего плана остаются P3.

## P2-03 — CRC/Hash/Test (принят, 2026-10-03)

P3-05.8 подключает исходный расчёт к Windows logical/device namespaces и ADS.
Logical-drive Copy остаётся недоступной; Hash дисков проверяет availability без
чтения дисков пользователя. Фактический device SHA256, отмена, отказ чтения и
повторная попытка проверяются только на собственном read-only/no-letter VHD.
Нативное чтение использует исходный CInFileStream с границей размера и выравниванием;
это устраняет sector-not-found при достижении конца устройства. Unix список томов
без operations-интерфейса сохраняет disabled Hash. Native11Qt/0skip и20EN/RU
normal/compact result JSON проверены; Linux full и общая приёмка ещё открыты.

Эталон — исходные PanelCrc.cpp, PanelCopy.cpp, HashGUI.cpp и Agent.cpp.
Команда CRC использует исходные выбранные индексы, scan/read и отдельное окно
результатов с двумя колонками. Внутри архива исходный hash callback получает
распакованный поток; общий этап извлечения выделения и второй CLI hash удалены.
File-only engine adapter может временно сохранить один поток в собственной
папке, прежде чем передать его callback; именами записей файлы staging не называются.
Методы меню включаются по исходному реестру hashers. Множественные методы и `*`
обрабатывает исходный HashCalc/CHashBundle; новых digest algorithms нет.

В обычном виде выбранная папка включает содержимое. В плоском виде upstream
Agent::Extract передаёт includeFolderSubItemsInFlatMode=false: для расчёта
дочерние файлы нужно выбрать отдельно. Это исправляет прежнее собственное
рекурсивное расширение плоской папки. Hash-файлы открываются исходным Hash
handler, Test читает файлы относительно папки manifest. Небезопасные пути и
внутренние ссылки/reparse objects отклоняются интеграционной границей порта.
PGP cleartext reader не означает генерацию или криптографическую проверку подписи.

Qt modal-worker удерживает владельца панели до join: закрытие главного окна
во время source-вызова откладывается. Ошибки/пароль/Cancel используют исходные
progress и extract callbacks; при ошибке не показывается успешный hash result.
Полный macOS gate199/199 PASS79.48s; последние локальные поправки прошли
пять direct consumers2.79s. Windows13/13 PASS13.04s без пропусков;
Linux12/12 и финальные5/5 PASS3.28s без пропусков. JSON540/420 проверены.
Rendered Windows reference и итоговая межплатформенная проверка не заявляются.

## P2-02 — SFX и внешние модули (принят, 2026-10-03)

Эталон — CompressDialog.rc Options: SFX, Shared, Delete, затем Encryption;
CompressDialog.cpp ограничения методов и переходы имени, Update.cpp отдельный
SFX-модуль при создании томов. Флажок использует source IDX_COMPRESS_SFX.
Новый архив 7z поддерживает SFX при совместимом официальном модуле и методе;
ZIP/BZip2, отсутствующий модуль и обновление существующего архива отключают его.
Windows использует `.exe`, macOS/Linux — нативный console SFX без расширения.
Windows GUI SFXWin собран из исходного baseline; native execution проверен.

Unix-адаптация томов: `<self>.7z.NNN`, поскольку исходный SFX reader ищет именно
это имя, а 7zz26.03 `-sfx -v` создаёт `<self>.NNN` и self-Test не проходит.
Сам модуль сохраняется отдельно и получает owner execution permission до
публикации. Для single/multi-root ordinary/split output проверены реальные
native self-Test/Extract, AES/header encryption, missing/occupied/cancel и
сохранность источника/модуля на macOS arm64 Qt6.11.2.

Четыре current-build JSON EN/RU760×720/640×480 reviewed: SFX/Shared/Delete
расположены в исходном порядке и полностью видимы, тексты помещаются. В compact
окне нижняя часть Encryption доступна через существующую общую прокрутку;
пиксельная Windows parity не подтверждена. External codecs и focused consumers
проверены на трёх платформах; полный macOS gate197/197 PASS107.31s.

Filesystem File → Comment теперь вызывает исходные FSFolder::SetProperty/
SaveComments и CPairsStorage. Сохраняются trim, удаление пустого значения,
UTF-8/BOM и другие пары descript.ion; source файл не меняется. Ctrl+Z, OK,
Escape, focused item и выделенные строки проверяются через общий source dialog.
Для flat nested item исходный setter возвращает E_NOTIMPL. Исходный setter
игнорирует false от SaveComments: при отказе записи возвращается S_OK, а reload
показывает старые данные. Эта особенность не заменена новой error policy.
Escape/Cancel в Comment теперь сохраняет исходный ранний выход без reload:
внешний файл, добавленный во время диалога, появляется после явного Refresh.
После принятия выполняется загрузка перед восстановлением исходного выбора;
отказ/отмена загрузки очищает старые строки и допускает последующее обновление.
Результат reload здесь по-прежнему не вызывает нового сообщения об ошибке,
как в исходном ChangeComment. EN/RU, обе панели и normal/compact состояния
проверены на macOS; текущая native Windows/Linux проверка этого изменения открыта.
Windows сохраняет исходные file attributes; POSIX использует portable source
Create_ALWAYS/write_full и сохраняет permissions существующего файла. Windows/
Linux execution и rendered Windows reference этого компонента ещё не проверены.

Текущая external-codec интеграция использует официальный dynamic Console7z;
standalone7zz внешние модули не загружает. Codecs/Formats enumeration, ABI checks
и warnings сохраняют исходные LoadCodecs bodies. Несовпадение native/engine
deployment вызывает отказ. Working plugins сохраняются при нерабочем модуле;
Qt codec discovery запускает version process и загружает каталог/runtime snapshot
в одном owned worker, включая блокирующее Windows CreateProcess; публикация
остаётся в GUI-потоке. GUI deadline5000ms не продлён; process wait проверяет
cancel/deadline каждые20ms и завершает/reaps child после возврата native create.
Cancel/deadline отбрасывают результат после join. Уже исполняющийся DLL
entrypoint или native create безопасно прервать нельзя: закрытие владельца ждёт
завершения worker и освобождает его результаты после join.
source About повторно показывает codec errors, startup status показывает их
без открытия модального окна. Immutable native readers удерживают собственные
DLL images: исходный SetCodecs и per-instance fallback используют этот каталог,
а child engine — тот же снимок runtime. Native reader и Create/List/Test/Extract
без main DLL, с исходным модулем в Formats, проверены на macOS; warning о main
сохраняется. Global-only external handler (test facade исходного 7z.so без
ISetCompressCodecsInfo) декодирует точные bytes после освобождения другого
подтверждённого каталога. Автоматический выбор модуля использует Libs исходного
engine; explicit mismatch сохраняет отказ. Те же consumers проходят Windows/Linux.
Реальные external Copy-codec Create/List/Test/Extract и native leased handler,
Unicode/space module path, wrong ABI, warnings, mismatch/cancel проверены на
macOS arm64. About caption EN/RU360/300 current JSON помещается; source error
box geometry600/400 проверена. Итоговая приёмка всего P0–P3 ещё открыта.
Actual Add acceptance создаёт single/split native SFX с delete-after-publication;
engine/volume-worker pause/resume и отмена second-root/prepublication volume
проверены на disposable данных. Advanced method override сохраняет исходный
UpdateGUI порядок properties; SFX фильтрует выбранный метод, explicit override
остаётся экспертной upstream возможностью.
Linux ARM64 Qt6.4.2 проверяет те же SFX/external consumers: focused6/6 без skips,
EN/RU normal/compact controls ordered/visible и captions fit. Windows SFX tests
используют GUI -y/-o/-p из SfxWin.cpp и отдельный engine Test; реальный Windows
запуск проверен: final clean Windows7/7PASS13.76s, Linux9/9PASS2.79s.
External/SFX Windows без skips; existing owned_temp symlink case требует
отдельной Windows fixture в P3. Четыре Windows EN/RU SFX JSON760/640 проверены:
source order, полная видимость и size hints. Эти проверки не принимают весь P3.
У стандартного QMessageBox оба resize requests дают auto-fit400×121 на этой
платформе; source text помещается, отдельная compact rendering parity не заявлена.

## P1-18 — запрос памяти RAR (2026-10-02)

Эталон — `Rar5Handler.cpp::CHandler::Extract`, FM
`ExtractCallback.cpp::RequestMemoryUse`, `MemDialog.cpp/.rc` baseline
0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Required/allowed bytes и flags поступают
из исходного обработчика. Allow, Skip и Cancel сохраняют исходные решения;
remember применяется к текущему пакету, сохранённый GB-лимит — к следующей
операции. Single-archive запрос без item index скрывает remember как в source;
MultiArcMode показывает его. Округление небольшого тестового лимита до 1 GB
принадлежит исходному диалогу, не означает выделения такого объёма памяти.

Root Extract использует исходный ExtractGUI/batch, selected — PanelCopy/Agent,
Test — Panel::TestArchives. Все режимы перезаписи RAR используют source пути;
RejectExisting проверяется до начала операции и сохраняет вновь появившийся
файл через source Skip. Несовпадение native item order вызывает явный отказ.
Native progress синхронизируется с исходным worker без child process.
Qt адаптирует presentation и хранение настроек; Windows/Linux runtime и
rendered Windows parity остаются непроверенными. Четыре actual Mem JSON
700×460/560×460 Extract/Test inspected, видимые controls и labels помещаются.
Полный gate formatting/build/193 CTest PASS79.91s на macOS arm64 Qt6.11.2;
это приёмка P1-18, не итоговая платформенная приёмка всего плана.

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
Refresh сбрасывает исходные поля подсвеченной подпапки в CPanel до загрузки
и восстановления, включая ошибку/Cancel. Qt очищает paint role и при сохранённых
строках; последующий DragLeave не обращается к старому индексу. Новое наведение
определяет текущую подпапку. Проверены обе панели и table/icons в EN/RU на macOS;
текущие Windows/Linux и rendered-сравнение остаются обязательными.
Архивный source заранее извлекает реальные bytes до Qt QDrag, поскольку у Qt
нет переносимого OLE release-before-drop hook; отмена может произойти уже после
извлечения. Move наружу не удаляет исходные архивные элементы. Неподтверждённый
внешний получатель удерживает owned temp после завершения локального drag без
удаления по таймеру. Native promise/delayed materialization не заявлены.
Qt Add всегда ждёт свой backend/progress вместо отдельного nonmodal 7zG.
Автоматические native/Qt receiver tests и normal/compact JSON выполнены;
Ручная Finder-проверка 2026-10-05 выявила -8058: временный файл удалялся до
получения Finder. macOS transport сохраняет временные bytes даже при нативном
IgnoreAction: такой результат Qt не подтверждает отмену внешнего получения.
Подтверждённая отмена controlled executor/typed target допускает cleanup;
нативный Escape может сохранить защищённый temp вместо его удаления.
Ручной повтор исправления успешен: received bytes совпадают по SHA-256,
исходный ZIP не изменён. Пользователь подтвердил нативный Escape; папка
назначения отменённого переноса пуста, временные bytes сохранены под external
protection. P3-03.1 принят вручную на macOS 2026-10-05; Computer Use недоступен.
Остальные требования итоговой приёмки P3-03/P3-06 остаются открытыми.
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
Первоначальная приёмка оставляла standard buttons/native pickers с платформенными
текстами. P3-05.9 исправляет двенадцать исходных LangSetDlgItems consumers:
Copy/Split/Combo/Password/Memory/Edit/ListView/About/Extract/Compress, compression
options и BrowseDialog2. OK/Cancel/Yes/No/Close/Help/Continue теперь используют
исходную таблицу выбранного языка; отсутствующий перевод сохраняет resource
caption. Memory сохраняет Continue при смене языка. Qt определяет порядок и
button roles; native pickers и Options property-sheet footer сохраняют своего
платформенного владельца. Текущие language cases и normal/compact footer JSON
проверены на macOS/Linux/native Windows; исходный полный Windows gate завершён
с четырьмя сбоями вне этих language suites, которые учитывает P3-03;
rendered и parent/final acceptance открыты. Остальные текущие формы/messages
также подключены; исходная приёмка
P1-14.2 сама по себе не подтверждает их Windows/Linux runtime.
P3-05.10 подключает исходные colon/remove-colon/window helpers к начальной
загрузке и смене языка: Benchmark показывает исходное «Скорость», Edit сохраняет
resource `&View:`, Progress применяет исходное добавление двоеточия. Отсутствующий
перевод сохраняет RC. Обычные кнопки Benchmark/Link/Progress используют ту же
исходную таблицу, а завершённый Progress меняет привязку Cancel на IDS_CLOSE.
Mac full249 и актуальные48 footer/8 Link JSON проверены; окончательные native
семь consumers и48 footer JSON проверены, полный gate этой native версии
прошёл256/256. Linux и rendered/parent acceptance остаются открыты.
Split/Combine используют исходные заголовки прогресса и подтверждение числа частей;
Open и продолжение hash — исходные IDS_OPENNING/IDS_CHECKSUM_CALCULATING.
Qt/7zz staging получает локальные EN/RU-подписи; независимая команда журнала
и встроенный редактор удалены в P2-09. Ошибки показывают source Progress/Messages, исходные
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
Историческое Qt-расширение Open archive с отдельным picker/Ctrl+O удалено
2026-10-04: его нет в исходном File menu. Сохраняются source Open/Open Inside,
address navigation и reusable BrowseDialog/filter component checks.

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
archive, following upstream. Create with links still refuses deletion until its
archive storage/deletion backend consumer is ported (P2-07); filesystem Link
support in P2-05 does not remove that guard.
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
`dir10` and `dir2` uses the source Agent's natural prefix order (`dir2` first).
Providers without IFolderCompare retain the generic BSTR fallback. Agent now
exposes its original comparator; raw columns carry the source raw flag through
Qt sorting. Header/actions use the source default/toggle policy:
newly chosen Size/PackSize/CTime/ATime/MTime
starts descending. Sorting ensures focused-row visibility. Inactive-header
signals belong to their own source panel and preserve the peer/focus/menu state.
Source Refresh sorts after named focus and before positional fallback. Its final
status publication updates all four parts even when Qt emits no selection/focus
signal, including a restored inactive pane, without changing peer labels/focus.
The four view modes retain their existing behavior; raw columns and full controls are
P0-06.2, time precision and UTC are P0-06.3. Normal/compact geometry is
checked locally, with no rendered Windows comparison.

P0-06.2 now projects the bound folder's original property catalog into the Qt
list. The header menu offers every reported property, with Name always shown;
width, order, visibility and sort restore separately for filesystem and archive
folder types. Raw values and sorting follow the source interface when a folder
provides it. The file transport exposes only actual raw checksum fields supplied
by the technical listing; it does not synthesize NTFS/security/reparse values.
Native codec metadata retains its raw provider and tree capability. Current
native platform acceptance remains in P3. Filesystem/archive metadata text
is requested through the source panel when Qt displays a cell, following upstream
callback text. Hidden columns do not trigger bulk reads when shown. Timestamp and
calculated-statistics changes invalidate cached text even in hidden columns;
source notification guards suppress reads during archive proxy replacement.
The normal/compact
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
OpenBookmark подключены к source-address Qt navigateTo. Исчезнувшая папка внутри
уже открытого архива теперь выбирает существующего родителя, как история,
без повторного list. Вложенные архивы сохраняют стабильный virtual prefix;
материализованные engine paths не входят в избранное. Поведение после закрытия
цепочки описано в P3-05.14 выше. Старые настройки MVP history/paths и favorites/*
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
виртуальный root. Reload сохраняет исходный выбор родительской строки при
fallback focus0 и восстанавливает её по явному имени `..`. Native marks/operated
indices не включают sentinel. Qt command availability допускает Open/Enter и
Open Outside для выбранного родителя через исходные handlers. Проверены обе
панели/EN-RU/standard-alternative/hidden dots/small-worker-flat consumers;
native/rendered acceptance остаётся открытой.
В альтернативном выборе Refresh сбрасывает начало Shift-группы на первую
строку, независимо от восстановленного фокуса. Последующий Shift-click следует
исходному диапазону; `..` исключён из marks/operated indices. Проверены прямое
восстановление, menu Refresh и таймер для обеих панелей, EN/RU, table/icons и
показанной/скрытой строки `..` с сохранением файлов и соседней панели.
FullRow в табличном виде переключает окраску всей строки
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

## P3-05 — Create validation policy (2026-10-07)

Saved compression profiles also retain original nearest-entry selection,
large-dictionary clamp, BZip2's decimal comparison and solid boundary behavior.
Unavailable explicit CPU thread counts leave no selection, matching source
SetNumThreads2; automatic values still select Auto. Qt inserts the source value
table and applies its returned selected index. Current EN/RU geometry and real
writer consumers are recorded in the
[profile audit](upstream-port-audit.md#p3-05--source-compression-profile-selection-2026-10-07).

Восстановлены исходные порядок проверок CompressDialog::OnOK и политика
MyMessages: ошибки пароля, пути/Browse и размера тома подавляются исходным
g_DisableUserQuestions, сохраняя отказ операции. Ошибка памяти и вопрос о малом
томе показываются независимо от этого флага. Ошибки используют Critical/OK и
заголовок 7-Zip; Qt/macOS скрывает заголовок стандартного message box.
Сохраняется исходная проверка несовпадающих паролей после отключения controls
при смене формата. Неизвестная RAM использует исходный fallback-лимит.
SaveOptionsInMem выполняется до проверки пути/томов, но INI и target публикуются
только после всех проверок. EN/RU normal/compact geometry, отказ/восстановление
и проверки платформ описаны в
[audit](upstream-port-audit.md#p3-05--source-create-validation-error-policy-2026-10-07).
Это не подтверждает rendered Windows parity или native desktop input.

## P3-05 — Level and SFX control events (2026-10-08)

Changing compression level retains text currently entered in Parameters while
resetting method-dependent defaults. SFX on/off retains the current method and
its dependent controls when supported, including an unavailable saved thread
selection, and updates only source naming and memory display. It does not trim
Parameters or initialize the whole format. EN/RU 7z/ZIP/XZ level and SFX state,
Cancel/no-write and existing real SFX operation evidence belongs to the
[control lifecycle audit](upstream-port-audit.md#p3-05--compress-control-specific-event-lifecycle-2026-10-08).
Rendered original Windows/native desktop acceptance remains open.

## P3-05 — Format and Hash filename controls (2026-10-08)

The archive-format combo follows source CBS_SORT ordering even if registration
order changes. Initial selection retains the first native candidate fallback,
saved name and explicit-handler priority; sorting cannot silently select another
handler. Browse filters use displayed order and native item data. Qt's system
locale supplies collation and existing lowercase format IDs remain its spelling;
native names supply filter descriptions. Hash method selection refreshes the
source filename extension to SHA1 or Auto SHA256 after control updates. EN/RU
normal/compact, Cancel/restart, Browse and real ZIP workflow evidence belongs to
the [format audit](upstream-port-audit.md#p3-05--format-selection-browse-and-hash-names-2026-10-08).
Empty/single-entry level controls retain source Auto in operation/profile specs;
Hash emits no unsupported level property and its source modal writer produces
the selected SHA1 manifest. TAR restart preserves the same Auto sentinel.
Original rendered Windows/system-locale/native desktop acceptance remains open.

## P3-05 — Options timestamp selection (2026-10-08)

Options preserves the original ComboBox contract: adding precisions does not
select one, and no selection yields CB_ERR/all-ones Auto. If a handler advertises
Unix/DOS but omits its Windows default, opening and accepting Options retains
Auto until the user explicitly chooses a value. Source capability/precision
algorithms, profile publication and resource grouping remain unchanged. The
Qt adapter now preserves that source behavior instead of selecting Unix itself.
EN/RU normal/compact probes cover empty/default/custom/explicit states, OK/Cancel,
persistence and keyboard selection. This is control/runtime acceptance; rendered
original Windows and native desktop acceptance remain required.

## P3-05 — Compress event and memory phases (2026-10-08)

Solid changes now preserve the original thread selection and Auto text, including
an unavailable saved selection. Memory display and OK refusal share original
GetNumThreads2's cached Auto count; dictionary/memory-limit events deliberately
regenerate it. This can preserve an upstream refusal after a Solid change that
would fit with a newly calculated thread count. Operation NumThreads remains the
all-ones Auto spec and the official engine owns its execution calculation. EN/RU
normal/compact control checks cover these states; native desktop input/rendered
Windows parity remains separate. Evidence and provenance belong to the
[event audit](upstream-port-audit.md#p3-05--compress-event-and-memory-phases-2026-10-08).

## P3-05 — Source settings persistence (2026-10-07)

Create/Extract, WorkDir and context-menu settings now retain upstream default,
defined and all-ones sentinel policies at the INI boundary. Undefined extraction
choices preserve previous settings; undefined compression options delete them.
Malformed password/split/removable preferences keep source defaults; values wider
than DWORD are refused instead of truncated. Global compression Level and empty
profile-string deletion are restored. Source whole-operation locking is preserved.
HKCU keys/history subkeys remain explicit INI groups/string lists; entirely empty
INI profile groups are implicit defaults recreated on use. No layout/resources
change. Dialog regressions cover explicit changes and restart; this does not
establish native desktop input or rendered Windows parity. Source/provenance and
current validation belong to the
[settings audit](upstream-port-audit.md#p3-05--source-settings-persistence-2026-10-07).

## P3-05 — Benchmark source/platform lifecycle (2026-10-07)

Benchmark's POSIX hardware-thread text and choices now use original System.h
normalization after failed/empty affinity queries. Windows retains its native
helper. Closing/restarting/stopping retains the source joined-worker lifecycle;
detached completion is the source successful no-op. Desktop/total resource
structure, Qt compact scrolling and live language/Help remain as specified in
the [Benchmark contract](ui-layout-contracts.md#benchmark-p2-04). The
[whole-body audit](upstream-port-audit.md#p3-05--benchmark-source-and-platform-lifecycle-2026-10-07)
does not establish rendered original Windows pixels or native desktop input.

## P3-05 — compression method/control states (2026-10-07)

The Qt Create dialog now retains original numeric method IDs for calculations.
Store leaves dictionary/order/solid empty while retaining original thread choices;
TAR GNU/POSIX leaves numeric/thread lists empty and hides compression/decompression
memory rows. Unsupported external methods show unknown memory instead of Copy's
zero dictionary. Copy itself has one zero dictionary item without Auto. Single
and empty controls retain original enabled rules, and switching back to 7z restores
the controls. QWidget memory-row containers adapt source joint visibility without
introducing another resource group. Original source bodies, header helpers and
resources are the structural reference; Qt normal/compact JSON does not establish
rendered Windows pixel parity or native desktop input. See the
[source audit](upstream-port-audit.md#p3-05--compression-method-and-control-states-2026-10-07).

## P3-05 — source CopyFrom into archived ADS (2026-10-07)

Agent now retains the original CopyFrom enumeration branch for an archive's
alternate-stream folder: regular files become ADS, and directories return
E_NOTIMPL before writing. Windows filesystem ADS prefixes retain the colon
without adding a path separator. On Unix, a narrow update-callback metadata
adapter supplies the Windows-only source item flag to the official WIM writer;
this does not add Unix filesystem ADS enumeration. Existing source transaction,
failure/Cancel handling and folder restoration remain authoritative. No dialog
layout or text changed. Public provider/worker checks do not establish native
desktop input or rendered Windows parity; the
[source audit](upstream-port-audit.md#p3-05--source-agent-alternate-stream-updates-2026-10-07)
owns the bounded implementation and platform evidence.

The actual peer ADS destination now has F5/F6 coverage from filesystem and ZIP
sources, both panes and EN/RU, including Copy Cancel and archive Delete Cancel
with byte-exact rollback. The Qt publication switch retains original OnCopy's
CDisableNotify until source RefreshListCtrl can load the replaced Agent proxy;
old focused rows cannot trigger premature metadata queries. Saved target
marks/focus and source focus/notification restoration are checked. This extends
the provider-level ADS evidence without claiming native desktop input or rendered
Windows equivalence; see the
[publication audit](upstream-port-audit.md#p3-05--source-copymove-publication-after-agent-updates-2026-10-07).

## P3-05 — source GUI early errors (2026-10-07)

The source GUI early-error policy is retained for missing update engines and
incorrect Extract output paths. They return original results before work starts;
g_DisableUserQuestions suppresses their messages as upstream, including GUI -y.
Qt Critical/OK presentation uses the original hardcoded English strings in both
EN/RU. Offscreen tests cover visible/suppressed states and later modal recovery;
native CLI/desktop/rendered and installed helper acceptance remain separate.
See the [error-policy audit](upstream-port-audit.md#p3-05--source-gui-early-error-policy-2026-10-07).

## P3-05 — toolbar Extract/Test source routing (2026-10-07)

Extract внутри открытого архива использует исходный Copy To с destination
противоположной панели. В файловой панели Extract показывает исходный Extract
для выбранного архива или группы архивов; Test проверяет выбранные архивы через
тот же source batch owner. Qt дополнительно отключает файловый batch route при
отсутствии native engine services, а также во время операции/закрытия окна.
Пустой открытый архив сохраняет доступный Extract и исходный пустой OnCopy.

Заголовок Extract содержит полный путь единственного архива и сохраняет его
при смене языка. Для группы архивов остаётся только исходный caption.
Удалены отсутствовавшие в upstream destination hint и selected-scope отключение
Eliminate duplication. Qt showEvent заменяет поздний Win32 OnInit после задания
параметров caller; доступность Windows security и системный порядок кнопок
остаются платформенными адаптациями. После модальных окон Qt восстанавливает
активность исходного окна, pane и focus. Source scope и проверенные платформы
фиксируются в [module audit](upstream-port-audit.md#p3-05--toolbar-extracttest-source-consumers-2026-10-07).

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
| Клавиатура списка | `PanelKey.cpp`, `OnKeyDown()` | Enter открывает папку, архив или файл; Backspace, F2–F7, Delete/Shift+Delete, Ctrl/Cmd+A, Ctrl+N, Alt+F12, Ctrl+F3–F7 и обновление работают в реализованных режимах. F3/F4 disk/archive файла запускают external viewer/editor по native focused index. Alt-Up/Left/Right используют исходные callbacks синхронизации папок двух панелей; проверка и Qt-адаптации описаны в разделе P3-05.4. F3 folder-size disabled до P1-10. |
| Главное меню | `resource.rc`; `MyLoadMenu.cpp`, `OnMenuActivating()` | Работают основные «Файл», «Правка», «Вид», «Избранное», включая View/Edit и Open Inside/Outside с ограничениями ниже. Оригинал формирует File динамически и содержит больше команд: Diff, Link и Alternate Streams; также есть Tools и Help. |
| Контрольные суммы | `resource.rc`, `PanelCrc.cpp`, `MyLoadMenu.cpp` | Подменю CRC содержит все 11 методов оригинала. Для выделенных файлов и папок файловой системы запускается `7zz h`; выбранные записи архива сначала извлекаются через backend во временную папку. Вывод и итог доступны в журнале, отмена останавливает текущий этап. В Windows 7zFM отдельное окно итогов и потоковое хеширование архива без временных файлов. |
| Открытие, просмотр и правка файла | `PanelItems.cpp`, `EditItem()`; `PanelItemOpen.cpp`, `StartEditApplication()`/`OpenItemInArchive()` | Enter/двойной щелчок используют системное приложение; архивный файл извлекается во временную папку с source lifetime. Disk/archive F3/F4 используют настроенную внешнюю программу без ограничения размера/UTF-8. Windows сохраняет default notepad.exe; POSIX пустой viewer использует association, editor требует настройки. Known/child processes отслеживаются; после правки Yes обновляет архив, No/Cancel удаляют копию, read-only/error сохраняют её. Короткий unchanged process сохраняет copy до закрытия owner; активный process продолжает работать после закрытия окна. POSIX inherited group сохраняет lineage orphan child; daemon, покинувший группу, может не обнаружиться. Дополнительные текстовые команды Qt сохраняют прежние ограничения. VerCtrl подключён в P1-08.3. |
| Ver Edit/Commit/Revert/Diff | `VerCtrl.cpp`, `MyLoadMenu.cpp` | Исходные динамические пункты добавляются в конце File/context меню для одного filesystem файла при настроенных Diff и `FM/7vc`. Readonly файл допускает Edit, writable — Commit/Revert/Diff. Edit хранит backup и нумерованную историю, Commit обрабатывает mtime и ставит readonly; Revert с другими данными использует source OverwriteDialog с default No, без extra buttons. На POSIX readonly убирает write bits, снятие readonly возвращает owner-write; creation time не полностью переносим (Linux stat ctime), Windows использует оригинальные API. Source size limit256 MiB и partial-write/error semantics сохранены. Upstream не предоставляет UI записи `7vc`; отдельная настройка не придумана. |
| Открыть внутри / снаружи | `resource.rc`, `MyLoadMenu.cpp`, `PanelItemOpen.cpp::OpenItem()` | Ctrl+PgDn открывает папку или выбранный файл внутри панели, включая вложенные архивы после извлечения во временную папку. Неизвестное расширение при явном открытии проверяет `7zz` по содержимому; обычный двойной щелчок продолжает запускать системное приложение. Цепочка `.tar.gz` открывается последовательным переходом в TAR. Shift+Enter открывает обычный или извлечённый файл системным приложением. Переход назад из корня вложенного архива восстанавливает родительскую папку и выделение. Архивы кроме `.7z`/`.zip` и все вложенные архивы только для чтения. Выбор альтернативного обработчика через `*`/`#` ещё отсутствует. |
| Панели и режимы списка | `App.cpp`, `Panel.cpp`, `SetListViewMode()`; `resource.rc`, View | Крупные/мелкие значки, список и подробности доступны через Ctrl+1–4; режим сохраняется. Все представления делят модель и выделение, поэтому команды действуют на те же записи. Двухпанельный режим ещё отсутствует; раскладка значков Qt может отличаться от Windows ListView. |
| Плоский просмотр | `Panel.cpp`, `ChangeFlatMode()`; `FSFolder.cpp`, `LoadSubItems()`; `resource.rc`, View → Flat View | Вложенные файлы и папки показаны единым списком с относительными путями; режимы файловой системы и архива сохраняются отдельно. Для диска Qt рекурсивно перечисляет записи порциями без прохода по ссылкам; переключение режима или переход в другую папку прерывает обход. Для архива добавляются неявные промежуточные папки. Выделение и операции используют исходные полные пути. Перечисление большого дерева и окончательная сортировка по-прежнему выполняются в потоке UI, но порции ограничены по времени. |
| Адрес и история | `Panel.cpp`, `PanelFolderChange.cpp`, `FoldersHistory()`; `ListViewDialog.rc` | CFolderHistory хранит до 100 каталогов, корней и внутренних папок архивов в INI. Выбор записи восстанавливает папку без повторного чтения уже открытого архива; после нового чтения исчезнувшая подпапка заменяется ближайшей существующей родительской. Временные пути вложенных архивов в историю не записываются. Перенесённый CListViewDialog (Alt+F12) позволяет удалить выделенные записи через Delete, скопировать пути и перейти к текущей; история «Назад» отдельная. Qt показывает границу архива как ` / `; Windows использует разделители пути и системный ListView. |
| Создание файла и выбор по маске/типу | `PanelOperations.cpp`, `CreateFile()`; `PanelSelect.cpp`, `SelectSpec()`/`SelectByType()`; `ComboDialog.rc` | Команда Ctrl+N создаёт пустой файл в обычной папке через редактируемый диалог; в архиве отключена. Правила выбора перенесены в `src/CPP/7zip/UI/FileManager/PanelSelect.cpp`, сопоставление маски — в `src/CPP/Common/Wildcard.cpp`. «Правка» выделяет или снимает выделение по нечувствительной к регистру маске `*`/`?` либо по расширению текущего файла (для папки — все папки). Qt-диалог повторяет поля ComboDialog, но не хранит историю введённых масок. |
| Закладки | `PanelFolderChange.cpp`, `SetBookmark()`/`OpenBookmark()`; `PanelKey.cpp`; `resource.rc`, Favorites | Меню «Избранное» хранит 10 слотов, включая текущую папку архива. Alt или правый Ctrl с цифрой открывает слот, с Shift — записывает; эта ветвь имеет приоритет над Ctrl+1–4. На macOS используется правый Command как Qt primary modifier. Реальный Windows ввод требует отдельной native проверки; протокол Qt проверяется локальной регрессией. |
| Архивные операции | `App.cpp`, `Panel.cpp`, диалоги FileManager | Qt поддерживает создание, добавление в открытый архив, извлечение, тест и основные изменения через `ArchiveBackend`. В Windows сценарии выбора файлов и форматы диалогов шире; здесь действуют ограничения из README. |
| Диалог Add to Archive: сжатие и шифрование | `../GUI/CompressDialog.rc`, `CompressDialog.cpp` | Маски уровней и таблицы методов перенесены из upstream `CompressDialog.cpp`: у 7z уровни 0–9, у ZIP — 0, 1, 3, 5, 7, 9; в ZIP доступен также PPMd. UI и backend используют общие таблицы. Потоки выбираются из исходного ряда с пределами метода/CPU, Auto учитывает память; показаны оценки сжатия и распаковки. Для LZMA/LZMA2 доступны словарь и размер слова, для Deflate/Deflate64 — размер слова, для PPMd — память и порядок, для BZip2 — словарь; для 7z — Auto/Non/Full solid и размер блока. Несовместимые параметры отключаются и отклоняются backend. Пароль вводится дважды; Show Password скрывает повтор. 7z использует AES-256 с отдельным шифрованием имён, ZIP — ZipCrypto/AES-256. Выбор источников остаётся отдельной верхней частью Qt-диалога. Для компактного окна источники и параметры прокручиваются вместе, кнопки подтверждения остаются видимыми. Пока нет томов, режима обновления, режима путей, SFX и произвольных параметров. При добавлении в существующий архив эти параметры не меняются. |
| Диалог Extract | `../GUI/ExtractDialog.rc`, `ExtractDialog.cpp`; `../GUI/Extract.rc` | Перенесены поле назначения с историей из 16 путей, кнопка выбора папки, полные пути или режим без путей, отказ от перезаписи, пропуск, перезапись и оба режима переименования при конфликте, поле пароля с показом. Новая папка назначения создаётся после проверки существующих родителей. Совпадающие имена при извлечении без путей отклоняются до запуска. В upstream доступны также абсолютные пути и запрос решения при конфликте; здесь они ещё не реализованы. Имя подпапки включается отдельным флажком и сохраняется; устранение дублирования корневой папки и восстановление прав пока отсутствуют. |
| Разбить файл / объединить части | `PanelSplitFile.cpp`, `SplitDialog.rc` | Нумерация частей использует перенесённый `CVolSeqName` и `SplitUtils.cpp` (`ParseVolumeSizes`, `GetNumberOfVolumes`, `AddVolumeItems`) в `src/CPP/7zip/UI/FileManager/PanelSplitFile.cpp`. В обычной папке доступны команды для одного файла. Диалог Split содержит папку назначения и редактируемый размер тома; как `CSplitDialog::OnOK()` и `CThreadSplit::ProcessVirt()`, принимает последовательность размеров, применяет их по порядку и повторяет последний для оставшихся частей; Combine принимает первую часть по исходному `CVolSeqName::ParseName` — суффикс `01` с любым числом ведущих нулей, с точкой или без неё, проверяет непрерывность и собирает файл в выбранную папку. Операции идут в рабочем потоке с отменой и временными файлами; файлы с существующими именами не заменяются. Диалог Combine показывает число частей, общий размер в байтах, исходную папку, первые две и последнюю часть, как `CApp::Combine()`. Проверку списка и суммы выполняет `VolumeOperation::inspectParts`; рабочий поток повторно проверяет части перед сборкой. |
| Диалог Copy/Move | `CopyDialog.rc`, `CopyDialog.cpp`; `App.cpp`, `Copy()` | F5/F6 открывают общий диалог с редактируемой историей назначения до 20 путей, выбором папки и именами объектов. Рабочий поток показывает текущий файл, принимает отмену и сохраняет прежнюю цель до публикации. Обычный `CApp::OnCopy` маршрутизирует операции между двумя файловыми или верхними архивными панелями, включая disk↔archive и archive↔archive; F6 из архива удаляет источник после успешной записи цели. При отказе rename F6 копирует во временную цель и удаляет источник после публикации, сообщая о частичном результате при ошибке удаления. Совпадающие дисковые папки сливаются с отдельными решениями по вложенным файлам. Одиночный новый filename и Shift+F5/F6 для элемента под фокусом поддержаны. Вложенные и защищённые архивы имеют явные ограничения. |
| Сервис → Настройки | `resource.rc`, `OptionsDialog.cpp`, `SystemPage.rc`, `SettingsPage2.rc`, `EditPage2.rc`, `FoldersPage2.rc`, `MenuPage2.rc`, `LangPage.rc` | Six-page container сохраняет source order/IDs; все страницы доступны после P2-08. System использует source state/apply и native registration services, с явными ограничениями платформ. Apply/OK пишут изменённые source values, Cancel сохраняет уже применённое. Language Apply обновляет source menus/toolbar/property headers/status и Options labels без очистки input/selection; missing/malformed fallback — source English. Qt standard buttons сохраняют platform conventions. WorkDir/RAM, external editors и Windows Zone имеют consumers. Shell controls остаются disabled до P2-09; полная интеграция страниц остаётся P1-12.2/P1-13.2. |

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

`PanelKey.cpp::OnKeyDown()` сохраняет исходные ветви
`VK_ADD`, `VK_SUBTRACT`, `VK_MULTIPLY`. Цифровые
`+`/`−` вызывают маску, Shift — выбор всех, Alt — выбор по типу,
`*` — инверсию. Приоритет Alt над Shift и подавление Ctrl+Plus следуют
upstream. Адаптация Qt использует `KeypadModifier`, не перехватывает ввод
в адресной строке и ограничивает команды их текущей доступностью.
Таблица и список значков используют один обработчик; фактические события
цифровых клавиатур Windows/Linux ещё требуют платформенной проверки.

`mainwindow_panelkey.cpp::handlePanelKey()` объединяет Qt-адаптер исходного
`CPanel::OnKeyDown()` и `Panel.cpp::CMyComboBoxEdit::OnMessage()`.
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
и операций переход отключён состоянием команды. Win32 уведомления заменены
Qt event filter, а исходный обработчик сохранён по пути
`src/CPP/7zip/UI/FileManager/PanelKey.cpp`.

Ctrl+C и Ctrl+Insert копируют имена реально отмеченных source indices через
CRLF, включая альтернативный выбор; пустой выбор даёт пустой текст без
подстановки фокусной ячейки. Ctrl+X/V и Shift+Insert сохраняют исходные no-op.
Ctrl+A/C/N/R/W/Z и Ctrl+1–4 учитывают Ctrl даже с дополнительными Alt/Shift;
Ctrl+W откладывает Close, как исходный PostMessage. Ctrl+PgDn в исходном list
handler поглощается без открытия: строка меню объявляет shortcut, но активного
accelerator для него в resource.rc нет. Явная команда Open Inside остаётся
доступной через меню. Ctrl+F3–F7 сохраняют исходный sort switch. Поля адреса и
редактор имени сохраняют собственные текстовые shortcuts. Проверки и точные
платформенные границы — в audit § source keyboard dispatcher.

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

LZMA/LZMA2 сохраняет явный выбор порядка при смене уровня; автоматический
выбор не передаёт `fb` движку. Исходный числовой ряд, defaults и общий
владелец `CCompressDialog::SetOrder2()` записаны в
[source audit](upstream-port-audit.md#qt-layout-diagnostics).

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

## P3-05.7 — Windows device-image Copy (2026-10-06)

Исходный `\\.\` namespace использует `CFSDrives`: логические тома и первые
16 физических устройств, свойства размера и имена образов определяет source
provider. F5 передаёт выбранные source indices в исходный `CPanel::CopyTo`;
Qt отображает устройства файлами по kpidIsDir. Сохранены AskWrite, progress,
4MiB buffer и исходная политика частичного файла при Cancel. Move, создание,
удаление, переименование и запись в устройство не поддерживаются upstream.
Hash логических дисков и устройств отключён до подключения namespace к его
Qt dispatcher. Copy логических дисков также отключён: исходный CopyTo
отказывает вне physical-device mode, а Qt consumer этого режима ещё отсутствует.
Unix mount namespace сохраняет прежний platform adapter и возможности.

Native read-only VHD регрессии старого продукта воспроизвели шесть direct
и четыре EN/RU UI отказа. Исправленный компонент проходит четыре native suite
на Windows, без skip; проверены16 EN/RU normal/compact JSON. Дополнительная
регрессия доступности logical drives проходит RED→GREEN на Windows. Финальный
macOS gate247PASS; Linux affected4PASS после full temp_cleanup timeout, Windows
full252/254 с process_pause/large-list failures сохранён. Компонент Copy принят;
полные platform gates и rendered 7zFM reference остаются обязательными в P3.
Ресурсные Main Shell/Copy contracts подтверждают
структуру, но не pixel parity устройства на Windows.

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

P3-03.5 проверяет собственную локальную Windows share; P3-05.11 восстанавливает
исходный RootFolder fallback для введённого UNC-сервера. CPanel выбирает
CNetFolder даже при Qt-классификации адреса как каталога, в том числе при
включённом Flat View. Открытие собственной share, родитель с сохранением фокуса,
отказ после удаления и обновление проверены в реальном окне EN/RU. Qt отключает
Copy без source IFolderOperations, используя ту же границу, на которой исходный
CApp::OnCopy отказывает в операции. Copy операций файлов/архивов и устройств
сохраняется; metadata providers не получают независимого Copy workflow.
Восемь обычных/компактных native JSON подтверждают структуру и геометрию.
Внешние серверы, credential prompts и rendered Windows pixel parity этим
локальным fixture не проверены; полная platform/parent приёмка остаётся открытой.

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
двухстадийный F6 — требуемое планом расширение: копирование и удаление
после успешной записи цели, а не идентичное поведение Windows 7zFM.
Проверка original App.cpp::OnCopy подтверждает передачу moveMode прямо в
этот отказ. Backend rollback и Cancel этого расширения проверены в P2-11.8.
На дату исходного шага полный `CApp::OnCopy` и
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
