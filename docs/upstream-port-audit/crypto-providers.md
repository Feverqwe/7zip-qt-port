# Криптографические примитивы и библиотечные providers

## Область и исходная реализация

Задача от 2026-10-24: использовать оптимизированные библиотечные примитивы,
сохранив совместимость архивов и исходные реализации для неподдерживаемых
сценариев. Основной кандидат — OpenSSL libcrypto; Botan рассмотрен как
альтернатива. Реализация выполнена в `feature/openssl-crypto`.

7-Zip: <https://github.com/ip7z/7zip.git>, commit
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Полные исходные inventory units,
принадлежность и SHA256 адаптированных единиц — в
[baseline](../upstream-baseline.json). Проверка и сравнение обновлений выполняются
существующим `scripts/upstream_baseline.py`, по [процедуре](../upstream-update.md).

| Граница | Наблюдения по исходникам | Текущее состояние |
| --- | --- | --- |
| ZIP AES PBKDF2-HMAC-SHA1 | `WzAes.cpp::CBaseCoder::Init2` запрашивает единый буфер ключа AES, ключа MAC и password verifier. `Pbkdf2HmacSha1.cpp::Pbkdf2Hmac` копирует HMAC contexts и использует `GetLoopXorDigest1`. | Библиотечный вызов с исходным fallback подключён к приложению и native runtime. |
| AES-CBC и ZIP AES counter mode | `MyAes.cpp` владеет фильтрами, IV, остатком блока и выбором реализации. WinZip counter mode использует little-endian 64-битный счётчик с pre-increment; обычный EVP CTR не эквивалентен. | CBC default использует EVP с исходным fallback в приложении и runtime; CTR сохраняет upstream. |
| SHA-1/SHA-2, MD5 и HMAC | `Sha1_GetBlockDigest`, прямой доступ к состоянию, копирование contexts и специальные циклы derivation не эквивалентны opaque EVP contexts. | Для SDK и runtime подключена обработка полных блоков низкоуровневым API OpenSSL; исходные состояния/ветви сохранены. GPL readers сохраняют upstream. |
| Linux fscrypt, fs-verity и подписи | Отдельная GPL-2.0-only closure `sevenzip_fscrypt_crypto` и `sevenzip_ext`. | libcrypto к этим targets не подключён. |
| RNG | Исходный генератор владеет seeding, состоянием и блокировкой; SHA256 используется как primitive. | Workflow сохранён, блоки SHA256 используют выбранный SDK backend. |
| SHA3 / BLAKE2sp | `CSha3` хранит открытое 25-word состояние и SHAKE; `CBlake2sp` — восемь параллельных tree lanes. Opaque EVP SHA3 и последовательный BLAKE2s не заменяют эти контракты. | Исходные реализации сохранены; проверены SHA3 registry и RAR5 hash consumers. |

## PBKDF2

Полный `CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp` перенесён из unchanged dependency
в `src/CPP/7zip/Crypto/`. Сохранены символ, параметры, исходный цикл, HMAC
dependencies и все ветви исходного вычисления. Добавлена попытка вызвать
`EVP_KDF` PBKDF2/SHA1 перед исходным телом. Это адаптация библиотечной границы;
алгоритм PBKDF2 заново не реализуется.

`src/crypto/opensslkdf.*` — локальная C++17 integration, без заимствованных
OpenSSL bodies. Она использует публичный API OpenSSL 3, собственные RAII contexts,
явные длины пароля и соли, 64-битный параметр iterations и `size_t` длину ключа.
Режим PKCS5 сохраняет совместимость коротких legacy-паролей и солей; это
документированный [контракт OpenSSL](https://docs.openssl.org/3.0/man7/EVP_KDF-PBKDF2/).
Отказ provider возвращает управление исходному 7-Zip вычислению. Частичный ключ
не считается успешным результатом. Ни пароли, ни ключи не входят в диагностику.

Адаптация подключена к статическому ZIP consumer и официальным sidecar/module.
CMake экспортирует выбранные dependency inputs для исходных native makefiles;
runtime cache учитывает библиотеку, заголовки и compile definitions.

`SEVENZIP_CRYPTO_PROVIDER=AUTO|OPENSSL|UPSTREAM` выбирает доступный backend
на этапе CMake. AUTO сохраняет upstream, если библиотека недоступна или не
совместима с deployment target. OPENSSL требует пригодную библиотеку.
macOS link probe запрещает зависимость от библиотеки с более новой минимальной
ОС, чем у приложения. Статическая поставка проверена на macOS; Windows native
dependency closure проверена отдельно. Произвольные shared libraries остаются
системными зависимостями и не входят в эту portable-bundle приёмку.

## Лицензионная граница и Botan

OpenSSL 3 использует Apache-2.0
([официальный источник](https://www.openssl-library.org/source/)); GPL-2.0-only
модули Linux не должны получать эту link dependency. Лицензии исходных модулей
сохраняются; новый provider target не подключается к GPL reader targets.

Botan предоставляет BSD-2-Clause библиотеку и
[C FFI](https://botan.randombit.net/handbook/api_ref/ffi.html), включая AES,
хэши, MAC и PBKDF2. C FFI позволяет сохранить C language boundary Linux-кода,
но opaque objects также требуют корректного копирования и освобождения
состояния; они не являются заменой Linux structs или 7-Zip block-state API.
Вторая библиотека пока не вводится. Решение о сохранении либо замене конкретных
Linux primitives должно учитывать полный source contract и используемые режимы;
наличие похожего имени алгоритма не доказывает возможность замены.

## AES-CBC и SHA block state

Полные `CPP/7zip/Crypto/MyAes.{h,cpp}` и `C/Sha{1,256,512}.c` имеют по одному
авторитетному владельцу под `src/`, с исходными hashes в общем baseline.
Сохранены notices, язык исходников и алгоритмические bodies. Перед изменением
сопоставлены AES key schedule, CBC/CTR loops, C++ filters и selectors,
`FilterCoder` read/write/finish, `WzAes`, `ZipStrong`, `Rar5Aes`, QCOW callback,
SHA update/final/prepare и registry/HMAC consumers. Повторное сравнение после
изменений не выявило удалённых исходных ветвей. Native runtime использует тот же
overlay; проверенные платформы и границы приёмки перечислены ниже.

В выбранных исходных модулях 7-Zip нет готового libcrypto provider adapter.
Новая local integration ограничена вызовами библиотечного API и передачей
состояния; криптографические алгоритмы заново не реализуются.

`CAesCoder::Code` подключает локальный `SevenzipOpenSslAesCbc` на границе CBC
primitive. EVP работает без padding; требования к длине, остатки, `Init`, IV и
ключевая политика остаются у исходного фильтра. После каждого успешно
преобразованного фрагмента сохраняется IV для исходной реализации. Library
output применяется только после проверки результата и длины, поэтому при
отказе оставшийся неизменённый suffix можно обработать upstream. Временные
байты очищаются, контекст освобождается RAII. Это необходимо, поскольку
`CFilterCoder` трактует `Filter() == 0` как допустимые непреобразованные байты,
а не как crypto failure. Исходный выбор конкретной software/HW реализации
по-прежнему работает; OpenSSL применяется при default selector. CTR filter,
его счётчик и buffering не изменены.

`src/crypto/opensslhash.c` — C11 integration, использующая опубликованные
`SHA_CTX`, `SHA256_CTX`, `SHA512_CTX` и низкоуровневые Init/Update calls только
для полных блоков. Источники OpenSSL подтверждают отсутствие allocation и
отказов этих calls для корректных contexts/данных. Chaining words копируются
через `memcpy`, без aliasing несовместимых 64-битных типов. Счётчики, buffering,
padding, digest serialization, Final reset и нестандартные SHA-512/224 и /256
остаются в исходных модулях 7-Zip. Работают копирование промежуточного состояния
и `Sha1_GetBlockDigest`, необходимые для исходных derivation loops.

Deprecated API изолирован в одном C adapter: opaque EVP contexts не сохраняют
этот контракт без изменения upstream module boundaries и no-failure API.
CMake проверяет наличие типов, полей и функций; сборка без совместимого API
сохраняет исходный SHA. Software/HW selectors не удалены. Общие C owners
компилируются для GPL readers без `SEVENZIP_OPENSSL_HASHES`; отдельный provider
target связан только с SDK/application closure.

## Проверки

2026-10-24, macOS: `crypto_pbkdf2` PASS с Homebrew OpenSSL 3.6.5, включая
прямой библиотечный derivation и fallback при недоступном provider. Независимые
ожидаемые байты вычислены Python `hashlib.pbkdf2_hmac`: короткая соль,
iterations=1/4096, NUL в пароле и соли, пустые значения, длинный пароль,
несколько блоков и усечённый последний блок.

Та же проверка PASS с upstream после автоматического отклонения несовместимой
Homebrew libcrypto: Mach-O `minos 27.0`, target приложения 14.4. Эта системная
библиотека не подходит для поставки. Для совместимой локальной библиотеки
собран официальный OpenSSL 3.5.9 LTS, commit
`45e844fa2a14ec92d146bd8f5778ac130b6625fb`, как статическая libcrypto
с `-mmacosx-version-min=14.4` и существующим compiler cache. Его link probe
и focused `crypto_pbkdf2`, `crypto_zip` PASS. Прямой статический ZIP writer
создаёт AES128/192/256 архивы с непустым и пустым entry; исходный sidecar
из baseline читает точные байты с правильным паролем и отказывает с неверным.
Оба focused suite также PASS с upstream provider.

[OpenSSL reference baseline](../openssl-reference-baseline.json) хранит полные
исходные API/algorithm units и SHA256, revision и local notice mapping.
Алгоритмические bodies OpenSSL не копируются в локальный код; это внешняя
библиотечная зависимость. Лицензия сохраняется без изменений и устанавливается
при подключении provider. Существующие команды дают воспроизводимые проверки:

```sh
python3 scripts/upstream_baseline.py reference-verify \
  --manifest docs/openssl-reference-baseline.json --upstream <openssl-checkout>
python3 scripts/upstream_baseline.py reference-review \
  --manifest docs/openssl-reference-baseline.json --upstream <openssl-checkout> \
  --new-revision <full-commit> --output <review.md>
```

OpenSSL reference verify PASS охватывает полные исходные единицы из manifest,
включая AES/SHA/MD5 API и implementation units для новых adapters. Reference review
показывает изменения upstream; contract/API differences рассматриваются вместе
с локальной integration и её consumers до изменения зависимости.

Проверка исходного 7-Zip baseline после переноса AES/SHA/MD5 и build reference PASS.
Исходный `Pbkdf2Hmac` сравнен целиком; удалённого scope в нём нет.
Это свидетельство source provenance, а не полной runtime-приёмки crypto.

Текущая macOS проверка с совместимой OpenSSL 3.5.9: шесть focused suites
`crypto_aes`, `crypto_hash`, `crypto_pbkdf2`, `crypto_zip`, `fscrypt_crypto`,
`fsverity_signature` PASS. AES-CBC проверен с 16/24/32-байтными ключами,
encrypt/decrypt, staging/streaming boundaries, IV-before-key, repeated Init,
rekey и переключением implementation selector. SHA проверен на padding
границах, clone частичного состояния, reset после Final, разных реализациях,
SHA-512 variants и исходном HMAC/PBKDF2 loop при недоступном EVP provider.
Пересобраны ext, LinuxFrames и SquashFS с общими C owners. У macOS ext и
SquashFS не появилось динамической libcrypto dependency. Проверены также
link commands ext/SquashFS/LinuxFrames: provider/library не входит в их link
closure. Те же шесть suites PASS с UPSTREAM provider.

Дополнительные macOS проверки после подключения native runtime: PASS
`crypto_runtime_tooling`, `runtime_cache`, `archive_qcow_crypto`,
`native_hash_bundle`, `native_hash_workflow`, `native_source_extract` и
`source_update_backend`. Последний запускался вне sandbox: Qt write lease
использует стандартный macOS cache directory. Native `7z` и `7z.so` собраны
с OpenSSL 3.5.9 и deployment target 14.4. После добавления MD5 PASS
`crypto_hash`, `crypto_aes`, `crypto_pbkdf2`, `crypto_zip`, `crypto_runtime_tooling`;
native runtime пересобран с MD5. MD5 проверен по независимым digest vectors,
clone и reset.

Linux x64, GCC 13.3, Qt 6.12.0, статическая OpenSSL 3.5.9: PASS десять
focused suites — `crypto_aes`, `crypto_hash`, `crypto_pbkdf2`, `crypto_zip`,
`crypto_runtime_tooling`, `runtime_cache`, `fscrypt_crypto`, `fsverity_signature`,
`archive_qcow_crypto`, `native_hash_bundle`. Перед тестами собраны официальный
native runtime и исходный SFX с общим compiler cache. Межплатформенная проверка
Windows x64, MSVC 19.44, Qt 6.12.0, статическая OpenSSL 3.5.9: PASS весь
focused-профиль `crypto` с теми же primitive/consumer suites, а также
`native_rar_memory`, `native_source_extract`, `source_update_backend`.
OpenSSL собрана без ASM, поскольку NASM на проверенной VM отсутствует;
это проверка совместимости, а не доказательство ускорения Windows.
Финальные make-glue изменения дополнительно проверены через
`crypto_runtime_tooling` и `native_hash_bundle`. Дополнительно на Linux PASS `native_rar_memory`,
`native_source_extract`, `source_update_backend`. После исправления cache
fingerprint PASS `crypto_runtime_tooling` и `runtime_cache`: замена API header
без изменения версии или library bytes действительно меняет результат
пересобранного consumer. Неизменённая конфигурация сохраняет готовые объекты.

Установленная macOS сборка подготовлена существующим `scripts/package.py`.
Из её runtime созданы и прочитаны зашифрованные 7z и ZIP AES256 с Unicode/spaces
в именах и пароле; извлечённые байты совпали. Проверены подписи bundle,
сохранённая лицензия OpenSSL и отсутствие внешней libcrypto dependency у
приложения, sidecar и module. Это приёмка статической зависимости; поставка
произвольной системной shared libcrypto этим не проверена.

Повторное сравнение полных перенесённых модулей с закреплённым upstream
показало только provider boundaries, dispatch и необходимые includes; исходные
алгоритмические bodies не удалены. Прочитаны также полные `7zAes`, `RarAes`,
`RandGen`, SHA3 и его registry, BLAKE2sp header и RAR5 hash consumers. Форматные
legacy ciphers и derivation loops остаются у официального engine; их AES/SHA/MD5
primitives используют выбранный SDK backend, где контракт допускает адаптер.
Сохранённые fallback и GPL consumers по-прежнему требуют переносимых исходников;
удаление crypto source dependencies этой работой не заявляется.

macOS `native_rar_memory` также PASS. Focused-профиль `scripts/verify.py crypto`
собирает прямых consumers и проверяет success/failure/cancellation. Windows
`dumpbin /dependents` подтвердил отсутствие libcrypto DLL у `7z.exe`, `7z.dll`
и статического ZIP consumer; native `7z` запускается с PATH только к Windows
System32. Runtime metadata подтверждает выбранный OpenSSL backend.
Полная release-поставка Windows/Linux и UI/pixel parity не заявляются.
Slow suites и полный gate не запускались: выбранные suites покрывают изменённую
primitive/build границу и её прямых consumers.

## Native Windows build boundary

Полный upstream `CPP/Build.mak` сохранён без изменений в
`third_party/7zip/CPP/Build.mak`; полный SHA256 и source mapping добавлены в
существующий baseline. В snapshot `scripts/crypto_provider.py::stage` ограничивает
список source arguments для `COMPL_*`, `CCOMPL_*`, `COMPL_ASM*` и resource recipe:
NMake `$**` включает также header/config dependencies, которые MASM пытается
ассемблировать. Команды, флаги, исходный batch compilation и прочие ветви
сохраняются; фильтр NMake VS 2022 пропускает C/C++/ASM/RC inputs, включая ARM64
`.S` из `Asm.mak`. Изменения для ARM64 проверены по исходникам; Windows runtime
проверяется только на x64. Для OpenSSL adapters исходный `/Gr` заменяется на
`/Gd`, сохраняя C ABI библиотеки; сочетание этих флагов MSVC отвергает.
SFX остаётся
исходным и не получает этот provider overlay. Это узкая build adaptation,
без копирования нового алгоритма. Проверка использует настоящие MSVC, MASM и
linker, включая повторную сборку при замене header/library и сохранение объекта
при неизменной конфигурации. Git Bash linker исключается выбором tool directory
MSVC. Лицензия и notices upstream 7-Zip сохраняются.
