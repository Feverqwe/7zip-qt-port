#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev menu_page_tests menu_policy_tests context_menu_settings_tests options_dialog_tests shell_context_tests language_ui_tests
case "${OS:-}" in
    Windows_NT) ./scripts/build.sh dev native_menu_integration_tests ;;
esac
./scripts/ctest.sh --preset dev -R '^(menu_page|menu_policy|context_menu_settings|options_dialog|shell_context|language_ui|native_menu_integration)$' --output-on-failure
