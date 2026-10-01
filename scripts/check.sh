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
sevenzip_setup_output 'check.sh (format, build)'

if [[ "$#" -ne 0 ]]; then
  printf 'Usage: %s\n' "$0" >&2
  exit 2
fi

ctest_jobs="${SEVENZIP_CTEST_JOBS:-4}"
if [[ ! "$ctest_jobs" =~ ^[1-9][0-9]*$ ]]; then
  printf 'SEVENZIP_CTEST_JOBS must be a positive integer.\n' >&2
  exit 2
fi
check_formatting() {
  if ! command -v clang-format >/dev/null 2>&1; then
    printf 'clang-format is required for the regression gate.\n' >&2
    return 1
  fi

  for source_dir in src tests; do
    if [[ -d "$source_dir" ]]; then
      while IFS= read -r -d '' source_file; do
        clang-format --dry-run --Werror "$source_file" || return
      done < <(find "$source_dir" -type f \( -name '*.cpp' -o -name '*.mm' -o -name '*.h' -o -name '*.hpp' \) -print0)
    fi
  done

  git diff --check || return
  git diff --cached --check
}

sevenzip_run_command 'clang-format checks and git diff --check' check_formatting
# The outer check.sh log already captures both commands in compact mode.
sevenzip_run_command './scripts/build.sh dev' env SEVENZIP_SCRIPT_OUTPUT=verbose ./scripts/build.sh dev
if [[ -n "${sevenzip_output_log:-}" ]]; then
  trap - EXIT
  sevenzip_finish_output 0
  exec 1>&3 2>&4
fi
env SEVENZIP_SCRIPT_OUTPUT=verbose ./scripts/ctest.sh --preset dev --output-on-failure --parallel "$ctest_jobs"
