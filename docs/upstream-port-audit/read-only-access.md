# Аудит read-only и частичного доступа

Запрос от 2026-10-23: проверить и исправить соответствие расширенной политике
[Read-only filesystem access](../../AGENTS.md#read-only-filesystem-access),
коммитить целостные проверенные сценарии. Аудит продолжается; первая итерация
не означает приёмку всех повреждений всех форматов.

## Область и опорные свидетельства

Приоритет новых recovery-сценариев задаёт политика. Для NTFS, FAT/exFAT и ext
отправная точка — существующие записи [NTFS](ntfs-linux.md), [FAT](fat-linux.md),
[exFAT](exfat-linux.md), [ext](ext-linux.md) и
[общая интеграция ограниченного чтения](filesystems-linux.md#проверка-политики-ограниченного-чтения--2026-10-21).
Эти source audits и датированные проверки сохраняются; новый аудит проверяет
применение политики, а не повторяет реестр переноса Linux.

Для архивов и потоков эталон — <https://github.com/ip7z/7zip.git>, полный commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Рассматриваются 7z, ZIP, RAR/RAR5,
TAR, gzip, bzip2, XZ и Zstandard. Декодирование, recovery algorithms и
интерпретация формата остаются у официального engine. Согласие не заменяет
недостающий reader, пароль, том или кодек.

## Открытие доступного каталога — итерация 1

Сверены source owners `CPP/7zip/UI/Common/OpenArchive.{h,cpp}`:
`CArcErrorInfo`, `CArc::ReadBasicProps`, `CArchiveLink::Open/ReOpen`;
`CPP/7zip/UI/Agent/Agent.cpp`: `CAgent::Open/ReOpen`, загрузка proxy и arc
properties; `CPP/7zip/UI/Console/List.cpp`: сбор ошибок/предупреждений после
успешного открытия; FileManager `FileFolderPluginOpen.cpp`, `ExtractCallback.cpp`
и `Panel.cpp`: публикация папки, diagnostics и Copy/Test consumers.

В ZIP проверены `CPP/7zip/Archive/Zip/ZipIn.{h,cpp}`: `CInArchive::Open`,
`ReadHeaders`, чтение local records при отсутствии central directory;
`ZipHandler.{h,cpp}`: `Open`, archive properties и `Extract`, включая пофайловые
CRC/data/header errors и cancellation. Reader сохраняет доступный каталог с
`S_OK` и диагностикой; CLI List учитывает ту же диагностику в exit status.
Повторное требование успешного CLI List в Qt теряло этот доступный каталог.

Новый Qt boundary использует сохранённый reader для каталога и Copy/Extract/Test
из уже открытого архива, когда приоритетный архив сообщает ограничение открытия.
Индексы принадлежат этому reader, включая повторяющиеся имена; независимый CLI
каталог для их сопоставления не используется. Проверка путей получает metadata
этого же reader; защита фактических выходных путей остаётся в исходном extract
callback и Qt path policy. Ограниченное открытие ФС затем подключено к тому же
native reader в [итерации exFAT](exfat-linux.md#partial-directory-access-2026-10-23).

Batch extraction также передаёт исходный open callback и заимствованный progress
dialog в этот consent boundary. Диалог живёт до join extraction worker; новый
ресурс создаётся для каждого открытия. Предварительная проверка batch paths
использует metadata сохранённого reader, без зависимости от CLI listing.

Перед публикацией папки предлагается явное read-only открытие с диагностикой.
Cancel выбран по умолчанию, Escape отменяет. Ресурс открытия хранит ограничения,
запрещает update и показывает постоянный индикатор. ReOpen проверяет текущую
диагностику; новое открытие требует нового согласия. Полезные файлы можно
скопировать; ошибка отдельного файла передаётся исходному callback и не становится
успехом. Извлечение всего доступного каталога также использует выбранный reader.

Это адаптация Qt/integration glue. Новые parser/decoder bodies не добавлены;
исходные engine modules и portable dependencies не изменены, baseline сохранён.
Новый alert — необходимое отличие от Windows 7zFM по продуктовой политике;
он использует штатный Qt QMessageBox, EN/RU, без собственного оформления.

### Границы следующей сверки

Process-only fallback, предварительная проверка всего зашифрованного архива и
сохранение decoder output рассмотрены в следующей секции. Source-visible диагностика
перечисленных format handlers установлена; runtime примеры не являются доказательством
восстановления любого повреждения каждого формата. Файловые системы требуют адресной сверки gates по
текущим исходникам; прежние результаты не выдаются за новое runtime acceptance.

Найденный отказ всего exFAT открытия при ошибке одного каталога исправлен
[следующей целостной итерацией](exfat-linux.md#partial-directory-access-2026-10-23).
Bootstrap корня/bitmap, короткий источник и предварительная проверка всей файловой
цепочки рассмотрены в [следующей итерации exFAT](exfat-linux.md#partial-bootstrap-and-file-data-2026-10-23).
Дальнейшая сверка архивных gates и остальных приоритетных ФС остаётся в scope goal.
Для FAT устранён отдельный пробел результата полного извлечения неполного корня:
[сверка и исправление](fat-linux.md#incomplete-root-results--2026-10-23).

### Проверки

Новый `archive_recovery_ui` проверяет EN/RU отмену/согласие для усечённого ZIP
при открытии и batch extraction,
копирование Unicode/space имени, CRC failure другого файла, извлечение всего
доступного каталога, неизменность источника, ограничения команд, повторный запрос
и normal/compact widget geometry. Отдельный контроль проверяет отказ для
traversal path в восстановленном каталоге. Full/slow suites и physical devices не входят
в эту итерацию; rendered Windows visual parity не заявляется.

2026-10-23: incremental сборка приложения и affected targets прошла на macOS.
Начальный набор из девяти suites открытия/извлечения/limited-filesystem consumers
прошёл; после подключения batch callback прошли шесть затронутых suites:
`archive_recovery_ui`, `native_folder_opening`, `native_rar_memory`,
`source_extract_backend`, `source_extract_contract`, `native_extract_callback`.
Финальный новый набор включает traversal и обе точки входа, без skipped cases.
Проверены Cocoa EN/RU alert и badge JSON: текст и controls помещаются, badge
виден в 980×620 и 640×400. Screenshots, VoiceOver и Windows reference не снимались.

Linux прошёл десять focused suites, включая final batch cases и EN/RU обычные/ext
limited-filesystem consumers, receipt `20261023T024234Z-542f00c1`.
Windows прошёл тот же набор из десяти suites после исправления forward declaration,
receipt `20261023T024400Z-72e8dcb3`. Remote UI — offscreen. Начальные Windows builds
обнаружили `struct CArc` вместо исходного `class CArc`: MSVC mangling приводил к
неразрешённому символу нового метода проверки путей. Исправлены оба затронутых
forward declarations; это не сбой compiler cache. Build trees и caches сохранены.

Первый sandboxed macOS UI batch был непригоден: WindowServer недоступен, готовый
`7zz` отсутствовал в PATH. Успешные Cocoa runs выполнялись unsandboxed с текущим
`build/codec-runtime`; runtime не пересобирался. Одноразовые fixtures удаляются
через QTemporaryDir; сохранены только актуальные widget reports и небольшие
runner receipts/логи. `git diff --check` прошёл.

## Process fallback и частичный вывод — итерация 5

Сверены исходные `Console/List.cpp::ListArchives`, `Print_OpenArchive_Props`,
`ErrorInfo_Print`, `Console/ExtractCallbackConsole.cpp::SetOperationResult` и его
formatter, `UI/Common/ArchiveExtractCallback.cpp::SetOperationResult`, исходные
GetStream/Copy/Extract callers и Qt process/source adapters. Репозиторий и полный
7-Zip commit указаны выше; существующие engine/format baselines сохранены.
Console умеет перечислить доступные записи после успешного Open_Strict, но
возвращает ненулевой status из-за archive diagnostics. Обратного CLI parser в
upstream нет: новая логика ограничена существующим process transport.

Process metadata сохраняет читаемый каталог при штатном exit 1/2, когда официальный
technical listing содержит тип приоритетного архива, явную диагностику открытия и
доступные записи. Crash, отмена, неизвестный status, отказ открытия без каталога
и остальные форматы не превращаются в успешный recovery open. Диагностика также
сохраняется при exit 0 с предупреждением. Перед публикацией папки используется тот
же EN/RU запрос ограниченного чтения с default Cancel; после согласия виден badge,
изменение источника запрещено и на UI, и на process mutation boundary. Повторное
открытие запрашивает новое согласие. Копирование всего содержимого уже открытой
ограниченной папки сохраняет этот scope, вместо нового скрытого открытия.

Два независимых pretest всего зашифрованного архива удалены из process extraction
и file-only source callback. Исходный extractor проверяет пароль при чтении
выбранных данных; предварительный test всех записей не является требованием SDK.
Теперь повреждение постороннего файла не блокирует исправную выборку. Запрос
пароля, передача его движку, скрытие в журнале и отказ неверного пароля сохранены.
Как у исходного extractor, ошибка может оставить непроверенный выходной файл;
он не считается успешно извлечённым. Источник не исправляется и не перезаписывается.

File-only source transport сохраняет выданные движком байты при штатной ошибке
приоритетного формата. Они копируются через исходный CopyStream и штатную защиту
выходных путей. CRC, data, unexpected-end, unsupported/unavailable и password
diagnostics передаются по SDK per-item contract; неизвестная диагностика остаётся
DataError. Console progress может находиться в той же физической строке, поэтому
formatter label выделяется после progress. Ошибка отдельной записи не останавливает
независимые следующие записи, если callback принимает её. Ошибка callback/записи,
отмена, crash и нерезультативный запуск не превращаются в partial success.
Пустые failed staging files удаляются, остальные принадлежат временной lease
операции. Проверки путей и неоднозначных повторных имён не ослаблены.

У native handlers сохранены исходные per-item results и уже выданные данные:
проверены соответствующие extraction/error branches ZIP, TAR, gzip, bzip2, XZ,
Zstandard и source callbacks; RAR memory/CRC consumers проверяются существующим
набором. Для 7z/ZIP выборка продолжает использовать исходные CRC/data results.
Header errors process-команды могут дать ошибку даже при полностью прочитанном
выбранном файле: transport не выдаёт эти данные за отдельно проверенный успех.
Отсутствующие solid dependencies, тома, пароль или codecs остаются ограничениями
самого engine; согласие не создаёт недостающий decoder/recovery algorithm.
Эта итерация не добавляет форматных parser/decoder bodies и не является новым
переносом Linux/7-Zip модулей.

Регрессии проверяют усечённый ZIP с читаемой Unicode/space записью и CRC-ошибкой
другой записи, immutable source и запрет изменения; encrypted ZIP с исправной
выборкой при повреждении посторонней записи; усечённый TAR и gzip/bzip2/XZ/Zstd
с сохранением читаемых данных. Zstd берётся из существующей фиксированной libzstd
fixture, поскольку официальный 7zz не создаёт этот формат. Отдельный SDK callback
проверяет CRC для ZIP и 7z, точные непроверенные байты, следующую исправную запись
и прекращение по E_ABORT callback. UI покрывает оба транспорта, EN/RU,
cancel/accept, отдельное и batch открытие, полное/выборочное копирование и traversal.

На macOS десять focused suites прошли (26.72 s), затем три непосредственно
изменённых набора прошли повторно после локальных исправлений. Расширенный набор
из одиннадцати suites на Linux/Windows дал десять PASS и один отдельный отказ
`source_extract_advanced::dotPathsInCpio(native-handler)`, receipt
`20261023T042510Z-f7cd4484`. Он воспроизведён на macOS и в baseline executable,
собранном из последнего коммита без текущих изменений: file-service case проходит,
native Cpio сообщает «Cannot open file as archive». Текущие изменения после
проверки baseline восстановлены; CPIO code/fixtures не изменены. Этот отказ не
считается acceptance и не скрывается исключением из отчёта. Windows security/ADS
cases в этом наборе прошли; на macOS/Linux они пропущены по платформе.

Финальные три affected suites прошли на macOS Cocoa (10.96 s), Linux и Windows,
receipt `20261023T042947Z-57bccb95`. EN/RU warning JSON для native/process путей
проверены отдельно: текст и controls помещаются; normal/compact badge покрыт тем
же UI-набором. Upstream verifier прошёл: 418 exact / 263 adapted / 12 local.
После адресного lsof и проверки identity/lock удалены два точных неактивных
task-owned остатка диагностик и устаревшая ownership запись. Остальные временные
fixtures удаляются через QTemporaryDir/lease; активные runs, инкрементальные
build trees и compiler caches сохранены. Full/slow suites, physical media,
rendered Windows/Linux appearance и installed-package acceptance не заявляются.
Инкрементальные сборки приложения прошли на macOS и обоих guests; remote receipt
`20261023T043230Z-f9781e7b`. В финальной source-сверке сохранены owners consent,
metadata, callback и path validation; parsing process diagnostics остаётся в
source extraction transport, общая completion не получает отдельного decoder.

## Частичное чтение ext — итерация 6

Устранены безусловные отказы из-за короткого источника, недостоверных счётчиков
занятости и ошибки отдельных descriptor/bitmap. Сохранены проверки реальных
адресов и служебных областей; независимые группы и читаемые части файлов
доступны через существующий Linux reader. При утрате корневых имён используется
исходный orphan catalog. Полное копирование известного неполного каталога
сообщает ошибку, а результаты прочитанных файлов сохраняются отдельно.
Согласие на такое открытие, отмена по умолчанию и видимый режим остаются
в общем EN/RU boundary; свободное место при недостоверных метаданных неизвестно.
[Source comparison, адаптации и ограничения](ext-linux.md#partial-bootstrap-and-readable-data--2026-10-23)
отделены от runtime-проверок. Новые decoder/recovery algorithms не добавлены.

Оставшийся scope goal: source-сверка NTFS bootstrap, ограничений чтения метаданных
и результатов неполного каталога. Пройденные сценарии ext и архивов не являются
доказательством чтения любого повреждения каждого формата.
