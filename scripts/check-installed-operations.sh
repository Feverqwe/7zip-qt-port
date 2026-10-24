#!/usr/bin/env bash
# Consume a private native runtime with the existing operation regressions.
set -euo pipefail
cd "$(dirname "$0")/.."
entry_only=false
if [[ "$#" == 1 && "$1" == --entry-only ]]; then
    entry_only=true
elif [[ "$#" != 0 ]]; then
    printf 'Usage: %s [--entry-only]\n' "$0" >&2
    exit 2
fi

interpreter="${SEVENZIP_RUNNER_PYTHON:-python3}"
install="$PWD/build/7zg-operation-install"
if [[ "${OS:-}" == Windows_NT ]]; then
    source scripts/lib/windows-runner.sh
    sevenzip_require_windows_runner
    install="$PWD/build/windows-package"
    gui="$install/bin/7zG.exe"
    fm="$install/bin/7zFM.exe"
    native=windows
    if "$interpreter" -c 'import ctypes, sys; sys.exit(0 if ctypes.windll.shell32.IsUserAnAdmin() else 1)'; then
        entry_targets=(file_manager_startup_tests)
        entry_tests=file_manager_startup
        printf 'Elevated entry scope: startup, including native symlink fixtures.\n'
    else
        entry_targets=(native_shell_launch_tests)
        entry_tests='native_shell_launch|windows_package_verification'
        printf 'Limited entry scope: registered shell launch and package verification.\n'
        printf 'Run --entry-only elevated separately for native startup/symlink acceptance.\n'
    fi
else
    case "$(uname -s)" in
        Darwin) gui="$install/Sevenzip.app/Contents/MacOS/7zG"; fm="$install/Sevenzip.app/Contents/MacOS/7zFM"; native=cocoa ;;
        Linux) gui="$install/bin/7zG"; fm="$install/bin/7zFM"; native=xcb ;;
        *) printf 'Unsupported native runtime platform.\n' >&2; exit 2 ;;
    esac
    entry_targets=(unix_gui_entry_tests)
    entry_tests=unix_gui_entry
fi

if [[ "$native" == xcb && -z "${DISPLAY:-}" ]]; then
    # Exercise the native X11 plugin and deployed dependencies. A virtual display
    # establishes no physical-desktop or pixel-parity claim.
    exec xvfb-run -a "$0" "$@"
fi
targets=("${entry_targets[@]}")
tests="$entry_tests"
if ! "$entry_only"; then
    targets+=(operation_launcher_tests operation_extraction_tests operation_creation_tests
        operation_benchmark_tests benchmark_command_tests manager_hash_workflow_tests)
    tests+='|operation_launcher|operation_extraction|operation_creation|operation_benchmark|benchmark_command|manager_hash_workflow'
fi
./scripts/build.sh dev "${targets[@]}"
"$interpreter" scripts/package.py --build-dir build/dev --install-only "$install"
export SEVENZIP_OPERATION_GUI="$gui" SEVENZIP_OPERATION_QPA="$native"
export PATH="$PWD/build/codec-runtime:$PATH"
if [[ "$native" == windows ]]; then
    export NATIVE_SHELL_TEST_INSTALL_DIR="$install" NATIVE_SHELL_TEST_REGISTER=1
fi
QT_QPA_PLATFORM="$native" "$fm" --smoke-test
./scripts/ctest.sh --preset dev -R "^($tests)$" --output-on-failure
