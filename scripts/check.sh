#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

case "${CODEX_SANDBOX:-}" in
  ''|none|off|disabled|0) ;;
  *)
    printf 'check.sh: full regression gate requires an unsandboxed run (CODEX_SANDBOX=%s).\n' "$CODEX_SANDBOX" >&2
    exit 2
    ;;
esac

source "$project_root/scripts/agent-output.sh"
sevenzip_setup_output 'check.sh (format, build, CTest)'

if ! command -v clang-format >/dev/null 2>&1; then
  printf 'clang-format is required for the regression gate.\n' >&2
  exit 1
fi

for source_dir in src tests; do
  if [[ -d "$source_dir" ]]; then
    while IFS= read -r -d '' source_file; do
      clang-format --dry-run --Werror "$source_file"
    done < <(find "$source_dir" -type f \( -name '*.cpp' -o -name '*.mm' -o -name '*.h' -o -name '*.hpp' \) -print0)
  fi
done

git diff --check
git diff --cached --check
# The outer check.sh log already captures both commands in compact mode.
SEVENZIP_SCRIPT_OUTPUT=verbose ./scripts/build.sh dev
SEVENZIP_SCRIPT_OUTPUT=verbose ./scripts/ctest.sh --preset dev --output-on-failure
