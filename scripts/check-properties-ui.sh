#!/usr/bin/env bash
# Source archive/portable Properties and actual Windows property-sheet consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "${1:-}" == --native-only && "$#" == 1 ]]; then
    if [[ "${OS:-}" != Windows_NT ]]; then
        printf 'Native property-sheet checks require Windows.\n' >&2
        exit 2
    fi
    ./scripts/build.sh dev sevenzip_desktop native_properties_window_tests native_system_menu_tests \
        native_shell_data_tests native_shell_com_tests
    ./scripts/ctest.sh --preset dev --output-on-failure \
        -R '^(native_properties_window|native_system_menu|native_shell_data|native_shell_com)$'
    exit
fi
if [[ "$#" != 0 ]]; then
    printf 'Usage: %s [--native-only]\n' "$0" >&2
    exit 2
fi
./scripts/build.sh dev sevenzip_desktop properties_tests source_properties_tests \
    properties_runtime_tests language_ui_tests owned_temp_publication_tests drag_drop_tests \
    language_dialog_tests property_formatting_tests panel_property_tests panel_columns_tests \
    native_library_tests alt_streams_command_tests
case "${OS:-}" in
    Windows_NT) ./scripts/build.sh dev native_properties_window_tests native_system_menu_tests \
        native_shell_data_tests native_shell_com_tests ;;
esac
./scripts/ctest.sh --preset dev --output-on-failure \
    -R '^(properties|source_properties|properties_runtime|language_ui|native_properties_window|native_system_menu|native_shell_data|native_shell_com|alt_streams_command|owned_temp_publication|drag_drop|language_dialog|property_formatting|panel_property|panel_columns|native_library)$'
