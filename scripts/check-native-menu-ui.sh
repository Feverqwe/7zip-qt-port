#!/usr/bin/env bash
# Native HMENU presentation and real File/System consumer on the Windows desktop.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 || "${OS:-}" != Windows_NT ]]; then
    printf 'Usage: %s (Windows only)\n' "$0" >&2
    exit 2
fi
./scripts/build.sh dev native_menu_presentation_tests native_system_menu_tests shell_menu_window_tests
./scripts/ctest.sh --preset dev -R '^(native_menu_presentation|native_system_menu|shell_menu_window)$' --output-on-failure
