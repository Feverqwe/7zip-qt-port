#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev archive_preparation_tests worker_progress_tests archive_creation_lifecycle_tests archive_mutation_tests source_update_backend_tests archive_backend_tests filesystem_scanner_tests archive_proxy_tests language_source_update_tests
./scripts/ctest.sh --preset dev -R '^(archive_preparation|worker_progress|archive_creation_lifecycle|archive_mutation|source_update_backend|archive_backend|filesystem_scanner|archive_proxy|language_source_update)$' --output-on-failure
