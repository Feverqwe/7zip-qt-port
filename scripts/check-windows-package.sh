#!/usr/bin/env bash
# Build, install and consume a private Windows package inside this snapshot.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 || "${OS:-}" != Windows_NT ]]; then
    printf 'Usage: %s (Windows only)\n' "$0" >&2
    exit 2
fi
source scripts/lib/windows-runner.sh
sevenzip_require_windows_runner
./scripts/build.sh dev native_shell_launch_tests file_manager_startup_tests
cmake --install build/dev --prefix "$PWD/build/windows-package"
export NATIVE_SHELL_TEST_INSTALL_DIR="$PWD/build/windows-package"
export NATIVE_SHELL_TEST_REGISTER=1
./scripts/ctest.sh --preset dev -R '^(native_shell_launch|file_manager_startup|windows_package_verification)$' --output-on-failure
