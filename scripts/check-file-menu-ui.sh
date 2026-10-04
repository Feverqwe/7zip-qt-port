#!/usr/bin/env bash
# Focused source File/plugin menus and external Open/View/Edit consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev sevenzip_desktop archive_item_open_tests archive_external_edit_tests nested_edit_chain_tests \
    shell_menu_window_tests shell_context_tests external_editors_ui_tests language_archive_command_tests \
    owned_temp_tests
case "${OS:-}" in
    Windows_NT) ./scripts/build.sh dev native_system_menu_tests native_properties_window_tests ;;
esac
./scripts/ctest.sh --preset dev --output-on-failure \
    -R '^(archive_item_open|archive_external_edit|nested_edit_chain|shell_menu_window|shell_context|external_editors_ui|language_archive_command|native_system_menu|native_properties_window|owned_temp)$'
