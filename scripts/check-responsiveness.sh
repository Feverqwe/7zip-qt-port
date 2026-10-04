#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
workers=(archive_preparation worker_progress archive_creation_lifecycle archive_mutation
    source_update_backend archive_backend filesystem_scanner archive_proxy language_source_update)
lists=(large_list_window flat_view panel_selected_state panel_key panel_auto_refresh
    nested_archive nested_edit_chain app_lifecycle_window binding_lifecycle ui_navigation bookmarks)
case "${1:-all}" in
    workers) suites=("${workers[@]}") ;;
    lists) suites=("${lists[@]}") ;;
    all) suites=("${workers[@]}" "${lists[@]}") ;;
    *) printf 'Usage: %s [workers|lists|all]\n' "$0" >&2; exit 2 ;;
esac
if [[ "$#" -gt 1 ]]; then
    printf 'Usage: %s [workers|lists|all]\n' "$0" >&2
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
