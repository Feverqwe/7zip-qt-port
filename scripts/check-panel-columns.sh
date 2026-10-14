#!/usr/bin/env bash
# Source column catalogs, publication, menus, sorting and language consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev panel_columns_tests panel_column_schema_tests \
    panel_column_initialization_tests panel_column_changes_tests panel_column_menu_tests \
    panel_column_sort_tests language_ui_tests
./scripts/ctest.sh --preset dev --output-on-failure \
    -R '^(panel_columns|panel_column_schema|panel_column_initialization|panel_column_changes|panel_column_menu|panel_column_sort|language_ui)$'
