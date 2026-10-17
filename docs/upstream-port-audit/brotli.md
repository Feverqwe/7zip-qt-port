# Read-only Brotli

## Исходники и baseline

Дата: 2026-10-17. Официальный runtime проекта остаётся
`https://github.com/ip7z/7zip.git`, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` (26.03).
В его `C/` и `CPP/` нет Brotli handler/decoder; `DOC/Methods.txt` только
резервирует идентификатор внешнего кодека. Новый алгоритм не реализуется.

Декодер: `https://github.com/google/brotli.git`, commit
`86ee1f65f62d6272345dee751d49e7d1f01c2b11`. Каталоги `c/common`, `c/dec`,
`c/include` и `LICENSE` скопированы без изменений в `third_party/brotli/`.
В сборку входят только common/decoder C translation units; encoder, CLI и
upstream tests не перенесены. C остаётся C99, публичные заголовки сохраняют
исходный C linkage. MIT notice поставляется вместе с приложением.

Обработчик и stream adapter основаны на `https://github.com/mcmilk/7-Zip-zstd.git`,
commit `964ba6a645545f47edc26452a35a18c726a5f1c4`:

| Исходная единица | Перенос и адаптация |
| --- | --- |
| `CPP/7zip/Archive/BrotliHandler.cpp` | `src/CPP/7zip/Archive/BrotliHandler.cpp`: исходные namespace, CHandler/IInArchive/IArchiveOpenSeq, single-item properties, callbacks Extract, IsArc_Brotli и регистрация формата. Сохранены интерфейсы официального baseline; убраны IOutArchive/ISetProperties, UpdateArchive/UpdateItems и encoder dependencies |
| `CPP/7zip/Compress/BrotliDecoder.h`, `.cpp` | Те же относительные пути в `src/`: CDecoder/ICompressCoder связывает 7-Zip streams с Google streaming API. Зависимость от BROTLIMT заменена узким raw-stream адаптером; свойства `.7z` и настройки threads не переносятся |
| `C/zstdmt/brotli-mt_decompress.c::st_decompress`, `st_finish_decompress` | Основа цикла raw stream: NEEDS_MORE_INPUT/OUTPUT, EOF, decoder result, output flush и освобождение state. Алгоритм декодирования остаётся в Google C; std::unique_ptr освобождает state на всех HRESULT/error exits |

Полные исходные SHA256 и retrievable commits записаны в
`third_party/brotli/upstream.json`. Для адаптированных fork-файлов это отдельный
baseline: список `local` официального `docs/upstream-baseline.json` означает
отсутствие этих путей в ip7z, а не независимую реализацию алгоритма.
LGPL-2.1-or-later notice fork сохранён в `7zip-zstd-LICENSE.txt` и поставляется
рядом с MIT notice. Собственный MT C не копируется и не компилируется.

Дополнительные зависимости официального baseline:

- `Archive/ArchiveExports.cpp`: неизменная регистрация, metadata и создание
  IInArchive, уже используемые другим format module проекта.
- `Archive/Common/DummyOutStream.{h,cpp}`: скопированы без изменений,
  включая test-mode discard/count. Добавлены в официальный exact manifest.
- `Common/StreamUtils`, `ProgressUtils`, `PropId`, PropVariant, MyWindows,
  MyString, IntToString, StringConvert, UTFConvert, Alloc: существующие
  переносимые зависимости; переиспользуются исходные bodies.
- `src/brotlimodule.cpp`: archive-only exports из исходного
  `Archive/DllExports2.cpp::CreateObject` и
  `Compress/CodecExports.cpp::GetModuleProp`. Coder/hasher/global external-codec
  entrypoints и Windows DLL services не нужны этому самодостаточному модулю.
  Windows `.def` экспортирует только его используемый ABI.

## Причины локальных изменений

Сравнены handler, decoder и raw/MT consumers до реализации. Исходный
`Bundles/Codec_brotli/makefile` не собирает archive handler: существующий codec
DLL сам по себе не даёт чтение `.br`. Отдельный format module сохраняет
официальный движок и существующий ArchiveBackend/source OpenArchive путь.

Исходный handler принимал поток в Open без проверки; теперь Open использует
исходную decoder-пробу IsArc_Brotli и отклоняет заведомо неверный/пустой вход.
Минимальная длина пробы допускает корректный поток пустого содержимого.
Признаки experimental framing не используются: реализован raw RFC 7932.
Сохранён ByExtOnlyOpen, поэтому общий поиск по содержимому не пробует Brotli
на произвольных текстовых/бинарных файлах. Явный native тип также доступен
через существующий механизм 7-Zip, без отдельного интерфейса выбора формата.

Потоковый мост ограничивает размер рабочих буферов, учитывает реально
потреблённый input, передаёт HRESULT чтения/записи и progress callback;
ошибки/отмена не превращаются в OperationResult success. NEEDS_MORE_INPUT
при EOF означает UnexpectedEnd; decoder error — DataError; остаток после
SUCCESS, включая следующий байт input, — DataAfterEnd. Для повторного Extract
seekable stream возвращается в начало. Test mode использует исходный dummy
output и не пишет файл. Размер распакованного элемента остаётся неизвестным
до успешного декодирования.

Не перенесены: запись, compression properties, BROTLIMT container, Large Window
extension, регистрация Brotli-метода внутри `.7z`, alias `.tbr` и dictionary HTTP
encodings. Это сознательный extraction-only scope, не полный перенос fork.
Обычный `.tar.br` может отдать tar как содержимое, но nested tar workflow
отдельно здесь не принимался.

## Интеграция и проверяемое поведение

`cmake/Brotli.cmake` собирает модуль и добавляет его в `Formats/` dev-runtime;
обычная сборка приложения зависит от его доставки. Unix/Windows install rules
устанавливают модуль и notices. Google sources vendored, сетевой загрузки при
configure/build нет. Существующие ArchiveCodecProbe/Catalog/Image отвечают за
одинаковое deployment у console/native consumers и lifetime загруженных модулей.
Новая UI/backend ветка декодирования не добавляется.

`brotli` проверяет native и console/backend consumers: byte equality, пустое и
короткое содержимое, input/output больше рабочих буферов, повторный Extract,
test mode, Unicode/spaces, default extracted name, malformed/truncated/trailing
input, cancellation после начала вывода, write failure, отказ создания и
сохранение занятого destination при Reject/Skip. Existing external_codecs и
codec_catalog покрывают прямые shared consumers.

Fixtures произведены Google Brotli CLI 1.2.0, `brotli -q 5 -o <name>.br <input>`.
`small`: UTF-8 `Brotli: Привет, мир! 空` с LF; `empty`: пустой payload;
`large`: 2 MiB `A`; `chunks`: 96 KiB low bytes xorshift32 со seed 0x12345678,
сдвиги 13/17/5. Это локальные синтетические данные, не пользовательские архивы.
Проверка обнаруживает структурные ошибки, но не обещает обнаружить всякое
повреждение: raw Brotli не содержит checksum.

Source review завершён для указанного raw read-only scope. Runtime результаты
и непроверенные области фиксируются ниже отдельно; source scope не означает
автоматическую приёмку платформ.

## Runtime validation, 2026-10-17

- macOS arm64, Qt 6.12.0: `build.sh dev brotli_tests external_codec_tests
  codec_catalog_tests sevenzip_desktop`; focused CTest
  `^(brotli|external_codecs|codec_catalog)$` — 3/3 PASS. После финальной сверки
  адаптера модуль и приложение пересобраны, `^brotli$` повторно PASS (1.22 s).
- Linux x64 и Windows x64, существующие Hyper-V guests: тот же focused набор
  через `verify.py --tests ... --platform all --skip-runtime`. Linux 3/3 PASS;
  Windows external_codecs/codec_catalog PASS, первый Brotli прогон обнаружил
  ошибку fixture: повторная запись файла при удерживаемом native handle.
  Для trailing case выделен отдельный файл, без изменения production semantics.
  Повторный Windows Brotli PASS. Финальная сверка source выполнена до последнего
  `^brotli$` на обеих платформах: Linux PASS, Windows PASS.
  Evidence run `20261017T192108Z-8fde80cf`; предыдущие shared-consumer результаты
  `20261017T191913Z-4dcd1882`, Windows fixture fix
  `20261017T192015Z-c7531596`. Использованы постоянные build trees и ccache.
- macOS `cmake --install` в task-owned staging: модуль и обе лицензии входят
  в bundle; его собственный `Resources/7zip/7z x -so` распаковал fixture
  с точным совпадением байтов. Staging удалён после проверки.
- Google vendor и новые exact upstream helpers сверены побайтно с baseline;
  task-owned C++ отформатирован, `git diff --check` PASS.

Это focused module/backend acceptance. UI appearance, installed packages на
Linux/Windows, HTTP dictionary encodings и полный/slow gate не проверялись.
