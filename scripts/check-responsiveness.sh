#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
workers=(archive_preparation worker_progress archive_creation_lifecycle archive_volume_creation archive_mutation
    source_update_backend archive_backend filesystem_scanner archive_proxy language_source_update)
lists=(large_list_window flat_view panel_selected_state panel_key panel_auto_refresh
    nested_archive nested_edit_chain app_lifecycle_window binding_lifecycle ui_navigation bookmarks)
volumes=(archive_volume_creation sfx_creation sfx_lifecycle ui_archive_volumes)
files=(worker_window_progress ui_copy_same ui_operations language_file_transfer
    file_transfer volume_operations volume_workflow app_lifecycle_window)
hover=(list_activation list_view_dialog folder_history list_settings_runtime panel_key properties_runtime
    temp_cleanup hash_results progress_dialog)
case "${1:-all}" in
    workers) suites=("${workers[@]}") ;;
    lists) suites=("${lists[@]}") ;;
    volumes) suites=("${volumes[@]}") ;;
    files) suites=("${files[@]}") ;;
    hover) suites=("${hover[@]}") ;;
    all) suites=("${workers[@]}" "${lists[@]}" "${volumes[@]:1}" "${files[@]:0:7}") ;;
    *) printf 'Usage: %s [workers|lists|volumes|files|hover|all]\n' "$0" >&2; exit 2 ;;
esac
if [[ "$#" -gt 1 ]]; then
    printf 'Usage: %s [workers|lists|volumes|files|hover|all]\n' "$0" >&2
    exit 2
fi
targets=()
for suite in "${suites[@]}"; do
    if [[ "$suite" == bookmarks ]]; then targets+=(bookmark_tests)
    else targets+=("${suite}_tests"); fi
done
./scripts/build.sh dev "${targets[@]}"
pattern="$(IFS='|'; printf '%s' "${suites[*]}")"
./scripts/ctest.sh --preset dev -R "^($pattern)$" --output-on-failure
