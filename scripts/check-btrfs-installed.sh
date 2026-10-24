#!/usr/bin/env bash
# Exercise the installed Btrfs module and application with existing regressions.
set -euo pipefail
cd "$(dirname "$0")/.."
interpreter="${SEVENZIP_RUNNER_PYTHON:-python3}"
install="$PWD/build/btrfs-install"
targets=(sevenzip_desktop btrfs_module_tests btrfs_multidevice_tests)
if [[ "${OS:-}" == Windows_NT ]]; then
    source scripts/lib/windows-runner.sh
    sevenzip_require_windows_runner
    targets+=(sevenzip_shell)
    runtime="$install/bin"
    fm="$runtime/7zFM.exe"
    native=windows
else
    case "$(uname -s)" in
        Darwin)
            runtime="$install/Sevenzip.app/Contents/Resources/7zip"
            fm="$install/Sevenzip.app/Contents/MacOS/7zFM"
            native=cocoa ;;
        Linux)
            runtime="$install/bin"
            fm="$runtime/7zFM"
            native=xcb ;;
        *) printf 'Unsupported platform.\n' >&2; exit 2 ;;
    esac
fi
if [[ "$native" == xcb && -z "${DISPLAY:-}" ]]; then
    exec xvfb-run -a "$0" "$@"
fi
./scripts/build.sh dev "${targets[@]}"
"$interpreter" scripts/package.py --build-dir build/dev --install-only "$install"
QT_QPA_PLATFORM="$native" "$fm" --smoke-test
export SEVENZIP_BTRFS_TEST_RUNTIME="$runtime"
./scripts/ctest.sh --preset dev -R '^btrfs_(module|multidevice)$' --output-on-failure
