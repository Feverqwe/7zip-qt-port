#!/usr/bin/env bash
# Source options through the retained native reader and direct batch consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$#" != 0 ]]; then
    printf 'Usage: %s\n' "$0" >&2
    exit 2
fi
./scripts/build.sh dev archive_open_options_tests source_extract_backend_tests \
    source_extract_advanced_tests shell_archive_services_tests codec_creation_tests
./scripts/ctest.sh --preset dev --output-on-failure \
    -R '^(archive_open_options|source_extract_backend|source_extract_advanced|shell_archive_services|codec_creation)$'
