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

Process-only fallback, предварительная проверка всего зашифрованного архива,
повреждения, обнаруживаемые только при decoding, и сохранение частичного вывода
требуют отдельной сверки. Source-visible диагностика перечисленных format
handlers установлена, но полная recovery-сверка их branches и consumers первой
итерацией не закрыта. Файловые системы требуют новой адресной сверки gates по
текущим исходникам; прежние результаты не выдаются за новое runtime acceptance.

Найденный отказ всего exFAT открытия при ошибке одного каталога исправлен
[следующей целостной итерацией](exfat-linux.md#partial-directory-access-2026-10-23).
Bootstrap корня/bitmap, короткий источник и предварительная проверка всей файловой
цепочки рассмотрены в [следующей итерации exFAT](exfat-linux.md#partial-bootstrap-and-file-data-2026-10-23).
Дальнейшая сверка архивных gates и остальных приоритетных ФС остаётся в scope goal.

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
