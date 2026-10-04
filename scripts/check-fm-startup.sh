#!/usr/bin/env bash
# Original File Manager startup and direct panel/window consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 ]]; then
    printf 'Usage: %s\n' "$0" >&2
    exit 2
fi
./scripts/build.sh dev sevenzip_desktop file_manager_startup_tests app_lifecycle_tests shell_menu_window_tests
./scripts/ctest.sh --preset dev -R '^(file_manager_startup|app_lifecycle|shell_menu_window)$' --output-on-failure
