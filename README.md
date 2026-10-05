# Sevenzip Desktop

Кроссплатформенный перенос Windows 7-Zip File Manager для Windows, macOS и Linux.
Функциональная основа переносится из исходного кода 7-Zip; интерфейс и обвязка
используют C++17 / Qt 6 Widgets. Архивные форматы и сжатие обеспечивает
официальный движок через native-модули и адаптер `7zz`.

## Состояние проекта

Работают две панели, файловая и архивная навигация, вложенные архивы, история
и избранное, режимы списка, сортировка и выделение. Перенесены файловые
Copy/Move/Create/Rename/Delete, Split/Combine, свойства, внешние View/Edit/Diff,
контрольные суммы, Benchmark, Help/About и страницы Options.

Создание и изменение архивов используют исходные планы обновления, параметры
сжатия, шифрование, тома и SFX. Доступные форматы и операции определяются
capabilities проверенного официального модуля. Извлечение поддерживает исходные
запросы пароля и перезаписи; ошибка или отмена может оставить уже извлечённые
файлы. Обратная запись вложенного архива требует возможности записи всей цепочки.
Платформенные metadata, links, ADS и associations имеют отдельные ограничения.

Портирование продолжается: shell/context integration P2-09 и общий UX-аудит
ещё открыты. Полная платформенная приёмка, установленная OS integration
и поставка остаются частью плана. Текущие статусы и счётчик ведутся только
в [чеклисте](docs/porting-checklist.md); подробности доступных сценариев
и ограничений — в [сверке с 7zFM](docs/7zfm-parity.md).
Датированные проверки и происхождение переносов сохранены
в [аудите](docs/upstream-port-audit.md); прошлый результат не подтверждает
новые изменения или другую платформу.

## Быстрый старт

Нужны C/C++17 compiler, CMake, Ninja, Qt 6 Widgets, `clang-format`
и официальный `7zz`. На macOS:

```sh
brew install cmake ninja qtbase sevenzip clang-format
```

Из корня репозитория:

```sh
./scripts/doctor.sh
./scripts/check.sh
```

`check.sh` проверяет формат C++, собирает приложение и запускает CTest.
На macOS откройте `build/dev/sevenzip_desktop.app`; из терминала:

```sh
build/dev/sevenzip_desktop.app/Contents/MacOS/sevenzip_desktop /path/to/archive.7z
```

`7zz` должен находиться в `PATH`; на macOS также проверяются стандартные
пути Homebrew. Зависимости других платформ, подготовка native codec/SFX runtime,
выбор engine и команды отдельных targets описаны в
[руководстве по разработке](docs/development.md).
Для Windows/Linux проверок используются общие x64
[Hyper-V runners](docs/hyperv-runners.md).

## Документация и агент

[Карта документации](docs/README.md) связывает план, статусы, provenance,
архитектуру, UI specifications и проверки. Правила работы — в
[AGENTS.md](AGENTS.md), профильные workflows — в `.agents/skills/`.

Локальная память необязательна. Команда
`sh .agents/skills/sz-setup/scripts/setup.sh` создаёт игнорируемый Git
Obsidian vault `.codex/` со ссылками на документацию и сохраняет существующие
заметки при повторном запуске. Порядок чтения и handoff —
в [протоколе памяти](docs/agent-memory.md).

## Лицензии

Лицензии перенесённых исходников сохранены в
[third_party/7zip/License.txt](third_party/7zip/License.txt).
У Qt Widgets доступны LGPLv3/GPLv2 и коммерческая лицензия.
Собственный Qt/UI и integration-код Sevenzip Desktop распространяется под
[MIT](LICENSE). Эта лицензия не заменяет лицензии перенесённого 7-Zip,
Qt и остальных зависимостей; их notices сохраняются отдельно.
