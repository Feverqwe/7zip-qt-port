# Regression strategy

The regression gate is `./scripts/check.sh`. It checks C++ formatting, builds the development preset, and runs CTest. A tracked Git pre-commit hook runs the same gate after `.agents/skills/sevenzip-setup/scripts/setup.sh` configures the local checkout. GitHub Actions will run it on macOS, Linux, and Windows after the repository has a GitHub remote.

## What to test

- UI wiring: command availability, selected files, dialog inputs, and user-visible error handling.
- Backend: construct arguments without a shell, parse output and errors, handle progress and cancellation.
- Archive integration: make tiny disposable archives with `7zz`, then verify listing, extraction, creation, wrong passwords, corrupted data, Unicode filenames, spaces, and unsafe paths as each feature is implemented.
- Platform behavior: paths, permissions, encodings, and process launch on each supported OS.

Use CTest for tests and keep fixtures under `tests/`. Use temporary directories for generated archives. A test must assert observable behavior. `archive_backend` tests `.7z` and `.zip` creation, listing, selected and full extraction, Unicode, spaces, nested folders, invalid archives, unsafe paths, missing `7zz` and cancellation. `ui_state` checks command availability and table contents for reopening, an empty archive, a real process launch failure, and cancellation during loading and extraction. `application_starts` is only a Qt startup check.

When a bug is found, first capture it with a focused regression test when practical, then fix it. Keep tests deterministic and independent of network access. Report the exact check command and result in the change summary. If a test cannot run on the current host, say why and rely on CI only after its result is available.

## Проверка компоновки Qt

`ui_layout` запускает настоящее окно через Qt Test в режиме `offscreen` и проверяет размеры 980×620, 720×480 и 640×400. Тест проверяет видимость панели, пути, таблицы и строки состояния, расположение без наложений и доступность столбцов. При каждом запуске сохраняет PNG в `build/dev/ui-screenshots/` с именем платформы и размера. Агент может открыть эти снимки для визуального осмотра. Сравнения с эталонными снимками и автоматического сообщения об изменении картинки пока нет.
