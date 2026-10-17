#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev options_integration_tests options_keyboard_tests options_dialog_tests options_window_tests system_page_tests menu_page_tests language_ui_tests external_editors_ui_tests help_ui_tests
if [[ "$(uname -s)" == Linux ]]; then
    ./scripts/build.sh dev linux_system_page_tests
elif [[ "$(uname -s)" == Darwin ]]; then
    ./scripts/build.sh dev mac_system_page_tests
fi
./scripts/ctest.sh --preset dev -R '^(options_integration|options_keyboard|options_dialog|options_window|system_page|mac_system_page|menu_page|language_ui|external_editors_ui|help_ui|linux_system_page_.*)$' --output-on-failure
