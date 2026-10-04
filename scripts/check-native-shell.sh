#!/usr/bin/env bash
# Native shell selection, generated HMENU and source verb consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 || "${OS:-}" != Windows_NT ]]; then
    printf 'Usage: %s (Windows only)\n' "$0" >&2
    exit 2
fi
./scripts/build.sh dev native_shell_data_tests native_shell_com_tests native_shell_launch_tests native_archive_command_line_tests
./scripts/ctest.sh --preset dev -R '^(native_shell_data|native_shell_com|native_shell_launch|native_archive_command_line)$' --output-on-failure
