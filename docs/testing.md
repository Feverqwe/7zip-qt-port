# Regression strategy

The regression gate is `./scripts/check.sh`. It checks C++ formatting, builds the development preset, and runs CTest. A tracked Git pre-commit hook runs the same gate after `.agents/skills/sevenzip-setup/scripts/setup.sh` configures the local checkout. GitHub Actions will run it on macOS, Linux, and Windows after the repository has a GitHub remote.

## What to test

- UI wiring: command availability, selected files, dialog inputs, and user-visible error handling.
- Backend: construct arguments without a shell, parse output and errors, handle progress and cancellation.
- Archive integration: make tiny disposable archives with `7zz`, then verify listing, extraction, creation, wrong passwords, corrupted data, Unicode filenames, spaces, and unsafe paths as each feature is implemented.
- Platform behavior: paths, permissions, encodings, and process launch on each supported OS.

Use CTest for tests and keep fixtures under `tests/`. Use temporary directories for generated archives. A test must assert observable behavior. `archive_backend` tests `.7z` and `.zip` creation, listing, selected and full extraction, Unicode, spaces, nested folders, invalid archives, unsafe paths, missing `7zz` and cancellation. `ui_state` checks command availability and table contents for reopening, an empty archive, a real process launch failure, and cancellation during loading and extraction. It also navigates a temporary filesystem with Unicode and spaces through the address bar, double click and Back, opens an archive from that list, and checks the return path. `application_starts` is only a Qt startup check.

For stage 3.2, `ui_state` checks direct and implicit archive folders, an explicit empty folder, a long Unicode filename, file double-click behavior, Up and Back, numeric column sorting, multiple selection and selected extraction paths after sorting. It also checks reopening, cancellation, failure and empty archives after folder navigation. The actual missing-`7zz` process test remains in `ui_state`; the folder-state cases use a controlled backend for deterministic signal timing.

For stage 3.3, `ui_state` sends Enter, Backspace, and the platform's Select All key sequence to the file table. It checks keyboard traversal of folders and archives with Unicode and spaces, unsupported files, selection, disabled actions while loading or extracting, and restored command availability after reopening, a real `7zz` launch failure, cancellation, and an empty archive. The Open action retains Qt's platform-specific standard Open shortcut; its file dialog is not exercised by the keyboard test.

For stage 4.1, `ui_state` exercises the single creation dialog with a Unicode file and folder, cancellation before submission, command availability during creation and after cancellation, failure, reopening and an empty archive. `archive_backend` checks that cancellation after writing partial output removes the temporary file, a failed `7zz` launch leaves no target, retry succeeds, and an existing target is preserved. The existing `.7z` and `.zip` round trips verify successful creation and listing with Unicode paths and spaces. These checks run locally on macOS; Windows and Linux remain unverified.

For stage 4.2, `archive_backend` updates disposable `.7z` and `.zip` archives with a nested Unicode folder containing a 5 MiB file, then lists and extracts the result. It adds to an empty ZIP, rejects a matching name, and checks that failed `7zz` launch, simulated write failure and cancellation leave the original bytes intact and remove temporary copies. `ui_state` checks the distinct add dialog, empty archive availability, disabled commands during update and reread, and recovery after cancellation, failure and reopening. The Qt test saves an add-dialog screenshot. These checks run locally on macOS; Windows and Linux remain unverified.

For stage 1.2, `archive_backend` also runs a disposable helper process that fragments percent and filename output across writes, ends with an unterminated error, supports cancellation, and emits a 3000-entry listing while a timer checks event-loop responsiveness. `ui_state` checks the visible progress label, nonmodal dialog, accessible last-operation log, failure text, and replacement of the previous log. These tests do not establish performance for archives larger than the fixture or verify Windows and Linux.

For stage 1.3, `archive_backend` feeds disposable listings with parent and drive paths, link metadata, duplicate names, file/directory collisions, and Windows reserved names. It also verifies refusal of missing, linked, and occupied destinations while preserving an existing Unicode filename. The earlier round trips cover successful Unicode paths and spaces. These tests run locally on macOS; Windows and Linux path semantics and filesystem behavior still need their own checks. Concurrent changes between validation and extraction are not covered.

When a bug is found, first capture it with a focused regression test when practical, then fix it. Keep tests deterministic and independent of network access. Report the exact check command and result in the change summary. If a test cannot run on the current host, say why and rely on CI only after its result is available.

## Проверка компоновки Qt

`ui_layout` запускает настоящее окно через Qt Test в режиме `offscreen` и проверяет размеры 980×620, 720×480 и 640×400. Тест проверяет видимость панели, адресной строки, таблицы и строки состояния, расположение без наложений и доступность столбцов. При каждом запуске сохраняет PNG в `build/dev/ui-screenshots/` с именем платформы и размера. `ui_state` сохраняет там же снимки открытой папки архива в тех же размерах, диалога создания и журнала. Агент может открыть эти снимки для визуального осмотра. Сравнения с эталонными снимками и автоматического сообщения об изменении картинки пока нет.
