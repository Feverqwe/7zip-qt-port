#!/usr/bin/env bash
# Real source sender/map/event and native GUI helper archive workflows.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 || "${OS:-}" != Windows_NT ]]; then
    printf 'Usage: %s (Windows only)\n' "$0" >&2
    exit 2
fi
./scripts/build.sh dev native_gui_workflow_tests
./scripts/ctest.sh --preset dev -R '^native_gui_workflow$' --output-on-failure
