#!/usr/bin/env bash
# Build the pinned native Windows UI reference in a fresh disposable snapshot.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 || "${OS:-}" != Windows_NT ]]; then
    printf 'Usage: %s (authorized Windows runner only)\n' "$0" >&2
    exit 2
fi
source scripts/lib/windows-runner.sh
sevenzip_require_windows_runner
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File scripts/build-windows-reference.ps1
./scripts/build.sh dev native_rendered_system_menu_tests
export SEVENZIP_CAPTURE_SCREENSHOTS=1
./scripts/ctest.sh --preset dev -R '^native_rendered_system_menu$' --output-on-failure
