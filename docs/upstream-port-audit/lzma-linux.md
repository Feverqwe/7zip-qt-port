# LZMA: dictionary, headers и source failures

## Source comparison

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Полный `CPP/7zip/Archive/LzmaHandler.cpp` перенесён под исходным путём в `src/`.
`CPP/7zip/Compress/LzmaDecoder.cpp` перемещён из exact `third_party/7zip` в
adapted `src/` без второй authoritative копии. Оба units включены в runtime
overlays динамического и standalone engines. Локальный ZIP target использует
тот же decoder; исходный include directory задан только для этого перенесённого
unit. [Manifest](../upstream-baseline.json) закрепляет source ownership,
revision и runtime fingerprints. Полное форматирование отделено от
функционального сравнения с форматированными оригиналами.

До изменений прочитаны целиком header parser/probe, Open/OpenSeq/Close,
Extract, LZMA86/BCJ branches и обе decoder модели Code/Read, их resume/property/
buffer/progress paths. Сравнены C LzmaProps_Decode, allocation и preview API;
проверены прямой ZIP wrapper и engine codec consumers. После изменений
повторно сравнены полные функциональные diffs и соответствующие callers.
Существующие parser и decoder 7-Zip сохранены; новый LZMA algorithm не добавляется.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
Прочитан полный `lib/decompress_unlzma.c`: header/range coder, literal/match,
dictionary wrapping, known size/EOS, fill/flush и error paths; также dispatch
из `lib/decompress.c` и вызов в `init/initramfs.c`.
[Reference hashes](../lzma-linux-baseline.json) закрепляют оригиналы.
Linux C bodies не копируются в production и не переводятся в C++.
Notices и лицензии исходного 7-Zip engine сохранены.

## Исправления

Archive parser принимает произвольное 32-bit dictionary field, которое умеют
читать Linux unlzma и существующий SDK decoder. Прежний список canonical
dictionary sizes больше не является условием отказа в Open и Extract.
У нестандартных headers probe переиспользует короткую decode-проверку из
исходного Open, сохраняя защиту от ложного распознавания формата без magic.
Новый результат IsArc или изменение публичного ABI для этого не вводятся.

Known unpack size читается как полный UInt64, за исключением исходного
unknown-size sentinel. Прежний порог 2^56 был эвристикой распознавания;
для таких headers также применяется decode preview. Это не ослабляет
проверку реального размера и EOS при извлечении. Удалена эвристика, которая
отвергала корректный пустой первый member с нулевыми properties при наличии
следующего member. Сохранены known/unknown sizes, original member loop и BCJ.

Open учитывает IArchiveOpenCallback, начинает чтение нового источника с начала
и очищает состояние после failure/cancellation. Preview остаётся ограниченным
исходным размером sample/output, а не декодирует весь архив при Open.
В общем decoder source E_FAIL/E_ABORT больше не подменяется S_FALSE/DataError
при неполном либо повреждённом partial read. Это относится к Code/CodeResume
и readable decoder interface; обычный malformed input по-прежнему сообщает
ошибку данных. Алгоритм, memory buffers и codec interfaces сохранены.

## Границы reference

Linux unlzma принимает все допустимые lc/lp/pb и произвольное dictionary field,
но initramfs dispatcher распознаёт LZMA по первым двум bytes `5d 00`.
Поэтому direct unlzma acceptance не выдаётся за automatic initramfs recognition
каждой комбинации properties/dictionary. Нулевой dictionary Linux нормализует
до 1; SDK decoder использует исходный minimum 4096. Проверенные small-dictionary
fixtures требуют только distance 1; дополнительные permissive SDK случаи
malformed distances не объявляются Linux parity.

Linux boot decoder завершает known-size output либо EOS; архивный reader
сохраняет более строгую проверку завершения и соответствия declared size.
Actual output в несколько петабайт не создавался: полный size field проверяется
через metadata и отказ при несовпадающем коротком payload. Это не acceptance
извлечения файла такого размера. Размеры dictionary сверх доступной памяти
ограничены исходными allocation errors; не объявляется проверка 4-GiB allocation.

LZMA86 — дополнительная совместимость 7-Zip, отсутствующая у Linux unlzma;
сохранены обе filter ветви и явный type selection. Отдельные LZMA concatenated
members не заменяют composite initramfs и его общий CPIO state. Этот scope,
оставшиеся bzip2/XZ/Zstd comparisons и census других shared formats остаются
в [едином реестре](filesystems-linux.md).

## Validation

CLI обоих shipped engines: 18 valid cases, automatic recognition `.bin` для
LZMA-alone, explicit LZMA86, exact output, test command и source immutability.
Проверены small/odd dictionary, wrapping, known/unknown/empty output,
крайние properties, пустой первый и другие concatenated members.
11 malformed cases проверяют normal code 2 для properties/range init,
truncation, declared-size mismatch, corruption, tail и неподдерживаемого filter.
До исправления handler эта suite воспроизвела 18 failures на двух engines.

ABI проверяет public probe, Open/OpenSeq, metadata, exact extraction,
source failures и cancellation, recovery/reuse и Code/Read с partial source
HRESULT. До исправления decoder body source E_FAIL воспроизведён как S_OK
плюс operation DataError. Прямой локальный ZIP handler извлекает независимый
ZIP-LZMA fixture через перенесённый decoder; ArchiveBackend извлекает nameless
LZMA entry из Unicode/spaces path, используя опубликованное имя listing.

Временный C reference harness использует исходные Linux unlzma bodies без
изменения алгоритма, userspace allocation/I/O adapters и fatal error callback.
13 одиночных valid fixtures дали точный payload, включая dictionary 0/3/4352/5001
и максимальные lc/lp/pb. Properties-max sample произведён pinned 7zz encoder
в 7z container и перенесён как raw pack stream с alone header.
Это compiled parser reference, не kernel boot acceptance. Disposable inputs,
outputs и producer archives удалены; source/diffs/небольшие evidence сохранены
локально. Incremental builds и ccache переиспользуются.

Focused suites `lzma`, `lzma_stream`, `gzip`, `gzip_stream`,
`native_folder_opening`, `codec_creation`, `external_codecs`, `runtime_cache`,
`runtime_package`, `upstream_baseline`, `upstream_inventory`: macOS arm64,
Linux x64 и Windows x64/MSVC — 11/11 PASS. Remote receipt
`20261020T082233Z-9a585cb3`. Exact baseline verification: 421 exact,
227 adapted, 11 local paths, одна revision. Shared decoder проверен прямыми
consumers и interface regressions; full/slow gate, UI appearance и реальные
устройства в этой итерации не проверялись.
