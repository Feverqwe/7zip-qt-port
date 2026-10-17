#!/usr/bin/env bash
# Native association mutation, source System state and file-open consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev association_services_tests association_file_open_tests system_page_tests
if [[ "$(uname -s)" == Linux ]]; then
    ./scripts/build.sh dev linux_association_clear_tests linux_system_page_tests
fi
./scripts/ctest.sh --preset dev --output-on-failure \
    -R '^(association_services|association_file_open|system_page|linux_association_clear_[01]|linux_system_page_.*)$'
