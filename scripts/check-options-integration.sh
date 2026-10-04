#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./scripts/build.sh dev options_integration_tests options_dialog_tests options_window_tests system_page_tests menu_page_tests language_ui_tests external_editors_ui_tests help_ui_tests
./scripts/ctest.sh --preset dev -R '^(options_integration|options_dialog|options_window|system_page|menu_page|language_ui|external_editors_ui|help_ui)$' --output-on-failure
