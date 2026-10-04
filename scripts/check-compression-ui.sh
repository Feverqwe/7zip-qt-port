#!/usr/bin/env bash
# Focused Add dialog and its actual File Manager/shell consumers.
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev sevenzip_desktop source_compress_dialog_tests shell_menu_window_tests \
    shell_update_window_tests compression_tests compression_advanced_tests \
    compression_path_tests language_ui_tests codec_commands_ui_tests codec_creation_ui_tests \
    ui_geometry_tests filesystem_selection_runtime_tests ui_operations_tests compression_resource_tests \
    ui_update_modes_tests archive_metadata_ui_tests source_drop_add_tests drag_drop_add_ui_tests \
    source_archive_browse_tests modal_refresh_window_tests flat_view_tests compression_preferences_tests \
    shell_update_gui_tests sfx_creation_tests sfx_lifecycle_tests \
    compression_dictionary_tests compression_order_tests compression_solid_tests \
    compression_memory_tests compression_limit_tests \
    main_shell_tests panel_shell_tests panel_key_tests ui_navigation_tests ui_archive_states_tests \
    ui_archive_volumes_tests language_main_dialog_tests language_focus_tests properties_runtime_tests \
    file_menu_policy_tests source_properties_tests archive_error_properties_tests \
    command_policy_runtime_tests native_command_dispatch_tests nested_archive_tests nested_edit_chain_tests \
    archive_item_open_tests archive_external_edit_tests external_editors_ui_tests language_archive_command_tests
./scripts/ctest.sh --preset dev --output-on-failure \
    -R '^(source_compress_dialog|shell_menu_window|shell_update_window|shell_update_gui|sfx_creation|sfx_lifecycle|compression|compression_advanced|compression_path|compression_preferences|compression_resources|compression_dictionary|compression_order|compression_solid|compression_memory|compression_limit|language_ui|codec_commands_ui|codec_creation_ui|ui_geometry|filesystem_selection_runtime|ui_operations|ui_update_modes|archive_metadata_ui|source_drop_add|drag_drop_add_ui|source_archive_browse|modal_refresh_window|flat_view|main_shell|panel_shell|panel_key|ui_navigation|ui_archive_states|ui_archive_volumes|language_main_dialog|language_focus|properties_runtime|file_menu_policy|source_properties|archive_error_properties|command_policy_runtime|native_command_dispatch|nested_archive|nested_edit_chain|archive_item_open|archive_external_edit|external_editors_ui|language_archive_command)$'
