# Криптография: возврат к исходникам 7-Zip

Решение интегрировано в master (`ccfdb678`, 2026-10-25). Ниже сохранены
история отдельной сборки, причины выбора и границы проверок; дальнейшее
объединение CRC32C/AES описано в [аудите общих библиотек](shared-checksums.md).

## Решение и source scope

2026-10-25, по запросу пользователя, ветка `feature/crypto-upstream` от
`d89dfe1f`: приложение и официальный native runtime возвращены к криптографии
7-Zip. Эксперимент сначала изолирован от master; последующая интеграция описана
ниже. Новая криптографическая библиотека пока
не подключается: преимущества рассмотренных кандидатов перед upstream для
приложения не установлены. Это выбор для экспериментальной сборки, а не
утверждение о непригодности сторонних библиотек.

Возвращены полные исходные AES/AesOpt, SHA-1/Sha1Opt, SHA-256/Sha256Opt,
SHA-512/Sha512Opt, MD5, MyAes и Pbkdf2HmacSha1. Удалены OpenSSL bridges,
обязательная libcrypto, её bootstrap, runtime metadata и packaging notice.
ComTry снова использует исходную обработку исключений; форматные callers,
HMAC, RNG, ZIP/7z/RAR и GPL readers сохраняют upstream поведение.
Изменения XXHash/Adler32 из `cd577e6a` сохранены.

Источник: <https://github.com/ip7z/7zip.git>, полный commit
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Полные inventory units,
оригинальные SHA256 и mapping владельцев возвращены в существующем
[baseline](../upstream-baseline.json). Криптографические исходники byte-exact
и совпадают с деревом до OpenSSL `717b358f`; новых алгоритмов и partial imports
нет. Лицензии исходного 7-Zip сохранены. Сверка исходных модулей дополняется
проверкой полного baseline; процедуры verify/review остаются прежними.

Сохранён полный исходный `CPP/Build.mak` с прежним SHA256 baseline.
Общие правила snapshot теперь сами предоставляют include-путь корня для zlib
и фильтруют дополнительные header prerequisites в NMake: раньше эти необходимые
общие настройки поставлялись через OpenSSL staging. Компиляция C остаётся C,
исходные optimized objects снова участвуют в native/SDK сборке.
Целевой минимум macOS 14.4 для native console/module также сохранён в общих
правилах, включая пересборку зависимостей при изменении сгенерированных flags.

Исторический scope и проверки OpenSSL сохранены в
[предыдущем аудите](crypto-providers.md).

## Рассмотренные альтернативы

Это source/API assessment, а не приёмка или benchmark новых providers.
Исходники сторонних алгоритмов не копируются в продукт.

| Вариант | Подходящие свойства | Цена перехода и вывод |
| --- | --- | --- |
| Исходный 7-Zip | Нативные copyable hash states, HMAC fast paths, CBC и специальный WinZip CTR; существующий CPU dispatch | Не добавляет зависимости и адаптеров. Выбран для этой сборки. Сам по себе возврат не доказывает улучшения безопасности или скорости. |
| Nettle | C API без выделения памяти; публичные contexts и compression functions; AES/CBC/PBKDF2 | Хороший технический кандидат. Понадобятся отдельные build/packaging и adapters; лицензирование отличается от OpenSSL. Готовой проверенной MSVC-интеграции в проекте нет. |
| Crypto++ 8.9 | Публичные SHA/MD5 Transform с внешним состоянием; PBKDF2 принимает байты с длинами | Устраняет потребность в deprecated OpenSSL hash API. Нужен C ABI bridge для C owners, аккуратные alignment/endianness boundaries и проверка CPU throughput. Выигрыш в размере/скорости и стоимость native сборки не измерены. |
| Botan | Hash copy_state, AES и PBKDF; доступен C FFI | Копирование хеша создаёт новый объект; FFI использует opaque handles и ошибки. Потребуется адаптация lifecycle/no-failure contracts. Сама библиотека требует C++20; C FFI позволяет сохранить C/C++17 consumers. |
| Mbed TLS / TF-PSA-Crypto | Отдельная криптографическая библиотека и PSA API | Переход 4.0 удаляет большинство legacy crypto API. Новую интеграцию вокруг старых публичных structs выбирать не следует; PSA потребует отдельной адаптации состояний и ключей. |

У всех альтернатив остаются format-specific derivation, WinZip counter semantics,
HMAC callers и границы GPL modules. Список поддерживаемых алгоритмов сам по себе
не устанавливает эквивалентность этим контрактам. Для следующего provider нужны
измеримые преимущества и сквозная проверка, а не только сокращение исходников.

### Источники оценки

- [Nettle manual](https://www.lysator.liu.se/~nisse/nettle/nettle.html):
  low-level C API, memory ownership, AES/CTR/PBKDF2, GPL-2.0-or-later либо
  LGPL-3.0-or-later. Дополнительно прочитаны установленные headers Nettle 4.0:
  sha2.h и pbkdf2.h; в продукт они не переносятся.
- [Crypto++ source](https://github.com/weidai11/cryptopp/tree/843d74c7c97f9e19a615b8ff3c0ca06599ca501b),
  tag `CRYPTOPP_8_9_0`, commit `843d74c7c97f9e19a615b8ff3c0ca06599ca501b`:
  sha.h, md5.h, pwdbased.h, License.txt; локальный read-only reference checkout
  в task-owned build. Проверены публичный Transform и PBKDF2 с явными длинами.
- [Botan hash API](https://botan.randombit.net/handbook/api_ref/hash.html),
  [FFI](https://botan.randombit.net/handbook/api_ref/ffi.html),
  [building](https://botan.randombit.net/handbook/building.html).
- [Mbed TLS 4.0 migration](https://github.com/Mbed-TLS/mbedtls/blob/development/docs/4.0-migration-guide.md):
  разделение repositories, PSA и удаление legacy crypto API.

## Проверки

Сохраняются поведенческие crypto suites: известные CBC/CTR/PBKDF2/hash vectors,
потоковая обработка, cloning/reset, rekey, ZIP AES128/192/256 и неверный пароль.
Удаляются только сценарии отказа/внутренние вызовы уже отсутствующего OpenSSL
provider; проверки результата архивных операций сохраняются.

2026-10-25: macOS arm64 — профиль `crypto` 13/13 PASS; отдельно
`upstream_baseline`, `developer_workflow`, `hyperv_runner` 3/3 PASS.
После сохранения minOS 14.4 все эти 16 suites повторно PASS (26.44s).
Окончательный hash test сохраняет независимые hashlib references для всех
прежних padding boundaries без libcrypto; адресный повтор PASS.
Baseline verify: 410 exact, 259 adapted, 21 local.

Собрана и установлена отдельная Release `dist/Sevenzip.app` в task worktree.
Проверены `codesign --verify --deep --strict`, encrypted 7z round-trip через
установленный sidecar, отсутствие EVP symbols и прямой libcrypto dependency
у приложения, `7z` и `7z.so`. Дополнительный обмен архивами между прежним
OpenSSL runtime и новым upstream runtime: 8/8 PASS — 7z и ZIP AES128/192/256
в обе стороны, точные байты, Unicode/пробелы в именах, пустой файл и неверный
пароль. Для ZIP использован допустимый ASCII password; Unicode password
проверен у 7z. Временные архивы удалены автоматически.

Linux x86_64 и Windows x64 — профиль `crypto` 13/13 PASS на каждой ОС,
runner evidence `20261025T041730Z-65abc5c3`. Финальный Linux repeat:
`crypto_hash`, `upstream_baseline`, `developer_workflow`, `hyperv_runner`
4/4 PASS (`20261025T042531Z-9efa4896`). В финальном Windows repeat
`crypto_hash`, `developer_workflow`, `hyperv_runner` PASS; `upstream_baseline`
достиг 120s timeout (`20261025T043019Z-d89d9339`), отдельный повтор неизменённого
теста PASS (`20261025T043604Z-f63a3f90`).
Последнее изменение flags ограничено Darwin и не меняет Linux/Windows rules.
Исходная полнота не выводится из PASS:
восстановленные crypto modules отдельно сверены с исходным baseline и
деревом до OpenSSL.

Compiler caches и incremental build trees сохранены. В release workflow
сохранены установка ccache и compiler launchers; удалён только OpenSSL
bootstrap и его специальные инструменты. YAML разобран, hosted workflow
не запускался. Full gate, slow suites, Windows/Linux release packaging,
кроссплатформенные performance benchmarks и UI appearance acceptance не
выполнялись. Предварительная локальная проба ZIP Store AES256 не использована
для выбора backend: она шумная и предшествует финальной проверке minOS.

## Интеграция в master (2026-10-25)

По запросу пользователя ветка совмещена с master `6f52da72`. Разрешены
пересечения source owners и native build rules: сохранены liblzma CRC64,
новые изменения FileManager/Agent и исходная криптография 7-Zip без OpenSSL.
Baseline verify после совмещения: 408 exact, 261 adapted, 21 local.

macOS arm64: 11/11 focused suites PASS — crypto primitives/ZIP, native hash,
extraction/update, shared checksums, XZ/streaming и baseline tooling.
Дополнительно проверена итоговая CMake configuration с сохранённым master
guard для ещё не созданных test targets. Native console/module сохраняют
minOS 14.4.

Linux x86_64 и Windows x64: по 5/5 PASS — `crypto_aes`, `crypto_zip`,
`shared_checksum`, `xz_stream`, `native_hash_bundle`; runner evidence
`20261025T044310Z-41dcb511`. Проверки повторно использовали те же branch build
trees и compiler caches. Full gate и slow suites не запускались.

Ранее установленный отдельный `.app` сохранён как проверенный артефакт
`d6761528`; после совмещения с master он не пересобирался. Worktree оставлен
для этого артефакта и текущих свидетельств. Push не выполнялся.
