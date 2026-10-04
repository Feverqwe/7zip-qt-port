#!/usr/bin/env bash
# Original GUI/email flow with private MAPI providers; no real mail client.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 || "${OS:-}" != Windows_NT ]]; then
    printf 'Usage: %s (Windows only)\n' "$0" >&2
    exit 2
fi
./scripts/build.sh dev native_mapi_tests native_shell_email_tests
./scripts/ctest.sh --preset dev -R '^(native_mapi|native_shell_email)$' --output-on-failure
