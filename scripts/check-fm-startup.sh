#!/usr/bin/env bash
# Original File Manager startup/GUI entry and direct panel/window consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 ]]; then
    printf 'Usage: %s\n' "$0" >&2
    exit 2
fi
targets=(sevenzip_desktop file_manager_startup_tests app_lifecycle_tests shell_menu_window_tests)
case "$(uname -s)" in
    Darwin|Linux) targets+=(unix_gui_entry_tests) ;;
esac
./scripts/build.sh dev "${targets[@]}"
./scripts/ctest.sh --preset dev -R '^(file_manager_startup|unix_gui_entry|app_lifecycle|shell_menu_window)$' --output-on-failure
