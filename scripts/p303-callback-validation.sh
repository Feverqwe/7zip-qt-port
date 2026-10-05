#!/usr/bin/env bash
set -euo pipefail
./scripts/build.sh dev panel_columns_tests panel_sort_tests large_list_window_tests panel_runtime_tests filesystem_selection_runtime_tests panel_property_tests flat_view_tests folder_size_view_tests
result=0
./scripts/ctest.sh --preset dev -R '^(panel_columns|panel_sort|large_list_window|panel_runtime|filesystem_selection_runtime|panel_property|flat_view|folder_size_view)$' --output-on-failure || result=$?
./scripts/check.sh || result=$?
exit "$result"
