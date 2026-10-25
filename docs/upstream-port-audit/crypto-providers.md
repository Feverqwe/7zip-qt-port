# История интеграции OpenSSL (2026-10-24)

Этот документ сохраняет исторический scope интеграции из `44661863`.
В экспериментальной ветке от 2026-10-25 OpenSSL удалена; текущее решение,
возвращённые исходники и проверки — в [оценке backend](crypto-backend-assessment.md).
Следующие разделы описывают прежнюю сборку с OpenSSL.

## Источники и границы переноса

SDK, приложение и официальный native runtime требуют OpenSSL 3 libcrypto.
Опциональный backend и возврат к исходному вычислению удалены по запросу
пользователя от 2026-10-24. Ошибка библиотечной операции завершает операцию
архива; непреобразованные байты не выдаются как успешное шифрование.

7-Zip: <https://github.com/ip7z/7zip.git>, полный commit
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Исходные полные inventory units,
SHA256 и mapping к локальным владельцам записаны в существующем
[baseline](../upstream-baseline.json). Изменённые ревизии и извлечённые зависимости
остаются явными в том же manifest; новая параллельная inventory не создаётся.
Проверка и сравнение обновлений выполняются по [процедуре](../upstream-update.md).

OpenSSL: <https://github.com/openssl/openssl.git>, reference baseline 3.5.9 LTS,
commit `45e844fa2a14ec92d146bd8f5778ac130b6625fb`. Полные исходные API и
implementation units, их SHA256 и notice mapping — в
`docs/openssl-reference-baseline.json` в commit `44661863`. Алгоритмические bodies
OpenSSL не копируются: библиотека является внешней зависимостью, а новые
`src/crypto/openssl*` — локальная MIT integration публичного API.

| Исходный модуль / symbols | Локальный владелец и адаптация | Сохранённый или исключённый scope |
| --- | --- | --- |
| `CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp::Pbkdf2Hmac` | `src/CPP/7zip/Crypto/`, EVP KDF bridge | Исходный PBKDF2/HMAC loop удалён; подпись и callers сохранены. Явные длины, iterations и PKCS5 legacy-параметры передаются библиотеке. |
| `CPP/7zip/Crypto/MyAes.{h,cpp}` | Исходные filters, IV, streaming и key validation; обязательный EVP cipher | Исходный CBC/CTR software/HW dispatch исключён. Ошибки SetKey возвращают HRESULT, ошибки Filter прерывают coder через исходный `CSystemException`. |
| `C/Aes.{h,c}::AesCtr_Code`, `AesCbc_Init` | `src/C/`, язык C сохранён | Сохранён upstream pre-increment little-endian 64-bit counter и XOR loop WinZip. AES-блоки вычисляет EVP ECB. Исходные S-boxes, key schedule, CBC rounds и CPU dispatch удалены. |
| `C/Sha1.c`, `C/Md5.c` | `src/C/`, исходные state/buffering/padding/finalization API | Round functions и исходный SHA-1 CPU dispatch удалены; полные блоки вычисляет libcrypto. Copyable contexts, Final reset и `Sha1_GetBlockDigest` сохранены для archive/HMAC consumers. |
| `C/Sha256.c`, `C/Sha512.c` | Общие авторитетные C owners под `src/C/` | SDK использует только OpenSSL block-state API; исходные rounds/CPU branches компилируются только для GPL readers. SHA-512/224 и /256 framing/initial states сохранены. |
| `C/AesOpt.c`, `C/Sha1Opt.c` | Локальные зависимости удалены | Эти реализации больше не нужны ни одному локальному consumer. SHA-256/512 Opt остаются только для GPL targets. Native SDK исключает все эти Opt objects. |
| `CPP/Common/ComTry.h::COM_TRY_END` | `src/CPP/Common/ComTry.h` | Исходный HRESULT exception type распознаётся перед общим catch, поэтому отказ KDF даёт `E_FAIL`, а не ложный успех или неверный Out-of-memory result. Прочие exception branches сохранены. |

В выбранных upstream модулях готового libcrypto adapter нет. Прочитаны полные
исходные bodies, headers, FilterCoder, WzAes, RarAes/Rar5Aes, 7zAes, HMAC,
registry и native make callers. Новая логика ограничена библиотечной и error
границами; форматы, derivation, counter mode и decoding заново не реализуются.

## Почему часть исходников остаётся

- GPL-2.0-only filesystem readers не получают Apache-2.0 libcrypto dependency.
  Их SHA-2, fscrypt, fs-verity и асимметричная криптография остаются исходными.
  Общие SHA-2 файлы имеют одну source ownership; SDK compile role не содержит
  исходных compression functions и не может переключиться на них.
- SHA3/SHAKE хранит открытое состояние и no-failure C API. Opaque EVP contexts
  не заменяют этот контракт без изменения ownership/copy/lifecycle.
- BLAKE2sp использует восемь tree lanes; последовательный BLAKE2s не эквивалентен.
- HMAC framing использует copyable SHA states и no-failure `GetBlockDigest`;
  opaque EVP MAC context не является прямой заменой этого контракта.
- Legacy format ciphers, derivation и RNG seeding/state/locking остаются у
  официального engine; подходящие SHA/MD5/AES primitives внутри них заменены.
- Официальный самостоятельный SFX строится без SDK overlay из закреплённого
  upstream. Это независимый переносимый артефакт, а не запасной backend приложения.

Botan рассматривался как BSD-2-Clause альтернатива с C FFI. Он также требует
адаптации opaque contexts; второй backend и зависимость не вводятся.

## Библиотечная и build граница

SHA/MD5 adapter использует low-level OpenSSL Init/Update над полными блоками:
их открытые chaining words позволяют сохранять исходный copyable state API.
Эти вызовы не выделяют contexts и не имеют пути отказа для валидных входов.
Deprecated API изолирован в C adapter и проверяется CMake как обязательный.
Сборка без необходимого API или с неподходящим macOS minimum OS отклоняется.
Это использование библиотеки не заявляет FIPS compliance.

CBC использует EVP без padding и in-place update. EVP ECB context для WinZip
принадлежит C++ filter; исходные C counter/XOR steps получают borrowed context
через своё opaque storage. Счётчики передаются в EVP небольшими пакетами, чтобы
не вызывать библиотеку на каждый блок; порядок счётчика и partial-block contract
сохранены. C++ boundary проверяет результат до сообщения о числе
обработанных байтов. Нет стадирования для fallback, исходной AES key schedule
или второго вычисления при ошибке. OpenSSL выбирает CPU implementation;
принудительные 7-Zip SW/HW selectors отклоняются как неподдерживаемые.

CMake экспортирует обязательные dependency inputs для исходных native makefiles.
Runtime fingerprint учитывает библиотеку, API headers и локальные adapters;
замена headers без изменения version/library bytes также требует пересборки.
Конфигурация предшествует подготовке runtime в focused wrapper, runners и CI.
Статическая зависимость предпочтительна для portable bundles; произвольные
shared libraries остаются системными зависимостями.

Полный upstream `CPP/Build.mak` сохранён byte-exact под `third_party/7zip/CPP/`
и записан в baseline. Snapshot adaptation сохраняет исходные compile/link
commands и batch compilation. NMake VS 2022 filters исключают metadata из
`$**` и ненужные crypto Opt objects; `.S` ARM64 inputs сохраняются. Для library
adapters `/Gr` заменяется на `/Gd`, сохраняя C ABI. C компилируется как C11,
локальная C++ integration — как C++17. SFX не получает эти изменения.

Лицензии и notices 7-Zip сохраняются. OpenSSL Apache-2.0 notice устанавливается
всегда. Проверка reference и подготовка обновления воспроизводимы:

```sh
python3 scripts/upstream_baseline.py reference-verify \
  --manifest docs/openssl-reference-baseline.json --upstream <openssl-checkout>
python3 scripts/upstream_baseline.py reference-review \
  --manifest docs/openssl-reference-baseline.json --upstream <openssl-checkout> \
  --new-revision <full-commit> --output <review.md>
```

## Проверки текущего изменения

Проверки предыдущего опционального backend не являются приёмкой этого изменения.
Focused проверка включает независимые CBC vectors AES128/192/256,
WinZip counter wrap и partial calls, SHA/MD5 padding/clone/reset, PBKDF2 vectors
с NUL/пустыми/длинными secrets и отказ при недоступном KDF/cipher. SDK ZIP writer
и native `7z` завершают операцию ошибкой при недоступных EVP algorithms;
обычные зашифрованные архивы проверяются независимым reader.

На macOS проба ZIP store AES256 с cached input 128 МиБ, одним warmup и пятью
измерениями дала медиану 103 мс против 105 мс у предыдущего optional backend.
Это проверка отсутствия наблюдаемого замедления в одном сценарии, а не обещание
общего ускорения. Вызов EVP на каждый CTR-блок давал 140 мс и заменён пакетным.

2026-10-24: профиль `crypto` прошёл на macOS arm64, Linux x86_64 и Windows x64.
Отдельно прошли `developer_workflow` и `hyperv_runner`; последняя CTR-адаптация
проверена через AES/ZIP и native extract/hash consumers на всех трёх ОС.
На macOS собраны приложение и GPL Btrfs core; `otool` подтверждает отсутствие
внешней libcrypto dylib у приложения и native runtime. Проверены исходные
7-Zip и OpenSSL baselines; source-port полнота устанавливается сравнением
bodies/callers отдельно от runtime результатов.

Полный gate, slow suites и release packaging не запускались; изменения hosted
release workflow проверены как YAML, но не исполнены в GitHub Actions. Windows
проверялась со статической OpenSSL без ASM; измерений её скорости и скорости
Linux здесь нет. Release bootstrap устанавливает NASM для сборки OpenSSL на Windows.
