# NTFS: classic read paths по Linux NTFS3

Итерация входит в [общий scope форматов](filesystems-linux.md). Исправлено
существующее чтение boot/MFT, атрибутов, sparse и compressed streams.
Полная актуализация NTFS по Linux ещё не завершена.

## Исходники и границы переноса

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`, полный
`CPP/7zip/Archive/NtfsHandler.cpp`. Единственная адаптация находится по тому же
пути под `src/`; [baseline](../upstream-baseline.json) включает её в оба runtime
overlays. Сохранены классы, MFT enumeration, сборка base/extension records,
имена/ADS, recovery deleted records, security metadata, исходные stream/cache
и extraction flow. Полное C++ форматирование учитывается отдельно от
функционального сравнения; macro class boundary сохранена.

Linux: <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>,
commit `58161ead29e50d70a317ea26b472e0c7508e69da`.
[Reference manifest](../ntfs-linux-baseline.json) закрепляет полный snapshot
`fs/ntfs3`, включая library dependencies. Сравнены полные тела выбранного
classic read scenario и их callers/types: `super.c:ntfs_init_from_boot`,
`record.c:mi_enum_attr`, `fsntfs.c:ntfs_fix_post_read`,
`run.c:run_unpack`, `inode.c:ntfs_read_folio`,
`frecord.c:ni_read_frame`, определения в `ntfs.h`/`ntfs_fs.h` и LZNT decoder.
Это не означает полного переноса всего reference snapshot.

Из `fs/ntfs3/lznt.c` перенесены оригинальные C bodies `decompress_chunk`,
`decompress_lznt`, `parse_pair`, `s_max_off` и размер chunk. Их алгоритмы
сохранены; kernel includes заменены узким portable type/memory adapter.
Compression/match-search state, compressor и allocator не копируются:
модуль read-only. Нормализация положительного короткого decode с нулевым
хвостом соответствует `ni_read_frame`.

Linux C остаётся в отдельном GPL-2.0-only format-модуле `sevenzip_ntfs`.
Его LGPL 7-Zip closure: archive exports, portable stream buffers/utilities,
progress/properties, CopyCoder, DummyOutStream, MethodProps и Common/Windows
portable helpers. Локальная MethodProps-адаптация сохраняет собственный mixed
baseline. Модуль не содержит unRAR. Он устанавливается вместе с engine и
лицензиями; NTFS registration основного external-codec engine отключена,
чтобы фактическим владельцем формата был модуль. Standalone сохраняет
оригинальный C++ LZNT decoder без GPL C linkage; остальные исправления
обработчика действуют в обоих engines.

## Исправленный сценарий

Boot проверяет NTFS OEM и геометрию; не требует несущественных DOS полей,
boot jump и AA55. Размеры MFT/index используют signed low byte; MFT fixups
всегда применяются к 512-байтовым секторам независимо от logical sector size.
Недействительный primary boot допускает чтение последней 512-байтовой копии,
когда NTFS handler уже выбран. Отдельное обнаружение формата только по backup
signature не добавлялось. Ошибки seek/read и отмена сохраняют HRESULT.

Проверяются минимальные размеры атрибутов, nonresident flag, границы имени и
value/run data, initialized/data/allocated size, порядок активных атрибутов и
границы record/fixups. Sparse packed-size учитывается и при compression unit
0. Физические runs проверяются целиком внутри тома, включая signed LCN delta,
переполнение и недостаточную длину mapping pairs.

Sparse runs читаются как нули независимо от наличия compressed frames;
initialized tail не раскрывает выделенные данные. Сжатие определяется по
attribute flags, а не по оставшемуся unit sparse-атрибута. Linux LZNT1
обрабатывает частичные chunks/frames; ошибка decode не публикует cache tag.
Исходная salvage-ветвь удалённых файлов остаётся на C++ decoder.

Каждое физическое чтение восстанавливает source seek position. Потоки
сохраняют собственные mappings, buffers и source owner после Close, ReOpen
и освобождения архива. Неуспешный seek/read допускает повторение; неожиданный
EOF и неполная extraction не считаются успехом. Повреждение данных и
unsupported method имеют разные operation results.

Nonresident reparse attributes разрешаются через обычный bounded stream после
сборки extension records; archive raw property содержит payload, а не mapping
pairs. Некорректные reparse данные неактивной recovery-записи не мешают чтению
активных файлов. EFS и ещё не подключённый WOF unnamed stream возвращают явный
unsupported result; ciphertext и sparse-подстановка WOF не выдаются за файл.

## Проверки

Fixture создана официальными `mkntfs`/`ntfscp` ntfs-3g 2022.10.3; источник и
команды находятся [рядом с образом](../../tests/fixtures/ntfs/README.md).
Regressions проверяют реальные bytes/ошибки, не текст реализации:

- обоими engines: resident/Unicode, raw, sparse/head hole, initialized tail,
  compressed data, logical sector 4 KiB, boot fields и backup copy;
- GPL ABI: Linux short-frame zero fill, interleaved/lifetime streams,
  seek/read failures, retry, decode-cache failure, отрицательные/overflow seeks,
  nonresident reparse payload, отмена открытия/извлечения, failure cleanup;
- ArchiveBackend: list и selected extraction с Unicode/spaces, sparse/short
  compression, ошибочный mapping, readonly catalog и неизменность source image;
- native Linux NTFS3 `6.8.0-146-generic`: девять read scenarios (включая
  nonresident symlink), три ошибочных read paths, invalid initialized size и
  EFS; исходные images не изменяются. Штатный `linux-modules-extra` установлен
  для текущего ядра runner. Backup-boot variant этим старым kernel не открыт;
  fallback проверен по закреплённому source и обоим engines, без заявления
  native acceptance для этой ветви.

2026-10-19: `ntfs`, `ntfs_stream`, `upstream_baseline`, `upstream_inventory`,
`runtime_package`, `windows_package_verification` прошли на macOS ARM64,
Linux x64 и Windows x64. После финальных локальных исправлений повторены
`ntfs`/`ntfs_stream` на трёх ОС. Сборки используют существующие incremental
outputs и ccache. Полный gate и slow suites не запускались: изменения
ограничены NTFS, format deployment и provenance; затронутые consumers покрыты
focused проверками. macOS app target собран; install smoke подтвердил загрузку
модуля, единственную NTFS registration, short-frame extraction и notices.
Native runtime evidence относится к указанному Linux kernel; эталоном source
comparison остаётся закреплённый commit, не версия runner kernel.

## Открытый read scope

WOF LZX/XPRESS и его ADS frame-offset reader требуют следующей интеграции:
сейчас unnamed WOF stream явно отклоняется. Также открыт bootstrap `$MFT`
через attribute lists, когда дополнительные data extents находятся в extension
records. Их отсутствие не является завершением актуализации по Linux.
Остальные metadata/read features NTFS должны пройти оставшийся source audit.
VFS/page cache, mount services, запись/journal replay и EFS key service не
переносятся в archive handler; Linux сам не расшифровывает EFS при чтении.
