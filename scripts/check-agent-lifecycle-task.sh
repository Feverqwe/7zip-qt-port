#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev agent_lifecycle_tests agent_opening_tests agent_folder_tests \
    agent_real_indices_tests agent_capability_tests archive_opening_tests \
    archive_open_options_tests archive_property_catalog_tests codec_catalog_tests
./scripts/ctest.sh --preset dev --output-on-failure \
    -R '^(agent_lifecycle|agent_opening|agent_folder|agent_real_indices|agent_capability|archive_opening|archive_open_options|archive_property_catalog|codec_catalog)$'
if [[ "${OS:-}" != Windows_NT ]]; then
    ./scripts/build.sh dev archive_item_open_tests nested_archive_tests \
        archive_external_edit_tests nested_edit_chain_tests panel_columns_tests \
        panel_column_schema_tests panel_column_changes_tests native_column_workflow_tests
    ./scripts/ctest.sh --preset dev --output-on-failure \
        -R '^(archive_item_open|nested_archive|archive_external_edit|nested_edit_chain|panel_columns|panel_column_schema|panel_column_changes|native_column_workflow)$'
fi
