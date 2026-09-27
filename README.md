# Sevenzip Desktop

Кроссплатформенный графический интерфейс в духе классического 7-Zip File Manager для Windows, macOS и Linux. UI и интеграционный код живут в этом Git-репозитории; официальный движок 7-Zip находится в отдельном соседнем checkout `../7zip-upstream`.

## Состояние проекта

Сейчас готов каркас окна на C++17 и Qt 6 Widgets. Команды архива пока отключены. План разработки — в [дорожной карте](docs/roadmap.md), граница UI и движка — в [архитектуре](docs/architecture.md).

## Быстрый старт

Установите CMake, Ninja, Qt 6 Widgets, `clang-format` и официальный `7zz`. На macOS:

```sh
brew install cmake ninja qtbase sevenzip clang-format
```

Из корня репозитория:

```sh
sh .agents/skills/sevenzip-setup/scripts/setup.sh
./scripts/doctor.sh
./scripts/check.sh
```

`check.sh` проверяет формат C++, собирает приложение и запускает CTest. Подробности по Windows, Linux и macOS — в [инструкции разработки](docs/development.md). О тестовой стратегии см. [тестирование](docs/testing.md).

## Агент и Obsidian

`AGENTS.md` задаёт правила для агента: менять код и документацию, добавлять регрессионные тесты для поведения и доводить задачи до проверенного результата. Повторяемые проектные навыки лежат в `.agents/skills/`:

- `$sevenzip-setup` — создаёт или обновляет локальную среду, Obsidian vault и Git-хук;
- `$sevenzip-context` — находит и поддерживает знания о проекте;
- `$sevenzip-regression` — добавляет тесты и проверяет изменения.

Скрипт настройки создаёт игнорируемую Git папку `.codex/`. Откройте **`<repo>/.codex` как vault** в Obsidian: «Open folder as vault», затем выберите скрытую папку через `Command-Shift-G` на macOS. В vault ссылки на актуальные `README.md`, `AGENTS.md` и `docs/`; локальные заметки и вложения сохраняются в `.codex/notes/` и `.codex/attachments/`. Повторный запуск настройки их не удаляет. В самой Obsidian можно открыть стартовую страницу командой:

```sh
open "obsidian://open?path=$(python3 -c 'from pathlib import Path; from urllib.parse import quote; print(quote(str(Path(".codex/Home.md").resolve()), safe=""))')"
```

После настройки локальный pre-commit хук запускает `check.sh` перед коммитом. GitHub Actions повторит проверки на macOS, Linux и Windows после публикации репозитория на GitHub. Сейчас удалённый репозиторий не настроен, поэтому CI ещё не выполнялся.

## Лицензии

7-Zip имеет собственные лицензии; подробности в `../7zip-upstream/DOC/License.txt`. У Qt Widgets доступны открытые лицензии LGPLv3/GPLv2 и коммерческая лицензия. Лицензию нашего UI нужно выбрать до распространения сборок.
