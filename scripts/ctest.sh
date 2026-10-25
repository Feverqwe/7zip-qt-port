#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

if [[ -z "${CTEST_PARALLEL_LEVEL:-}" ]]; then
  ctest_jobs="${SEVENZIP_CTEST_JOBS:-4}"
  if [[ ! "$ctest_jobs" =~ ^[1-9][0-9]*$ ]]; then
    printf 'SEVENZIP_CTEST_JOBS must be a positive integer.\n' >&2
    exit 2
  fi
  export CTEST_PARALLEL_LEVEL="$ctest_jobs"
fi

invoke_ctest() {
  if [[ -n "${SEVENZIP_RUNNER_TIMING:-}" ]]; then
    "$SEVENZIP_RUNNER_PYTHON" scripts/lib/runner_timing.py --ctest "$@"
  else
    ctest "$@"
  fi
}

run_ctest() {
  case "${SEVENZIP_INCLUDE_SLOW_TESTS:-0}" in
    1) invoke_ctest --no-tests=error "$@"; return ;;
    0) ;;
    *) printf 'SEVENZIP_INCLUDE_SLOW_TESTS must be 0 or 1.\n' >&2; return 2 ;;
  esac

  local excluded_labels='^slow$'
  local argument
  local forwarded=()
  while [[ "$#" -gt 0 ]]; do
    case "$1" in
      -LE|--label-exclude)
        if [[ "$#" -lt 2 ]]; then
          printf '%s requires a label regex.\n' "$1" >&2
          return 2
        fi
        argument="$2"
        shift 2
        ;;
      --label-exclude=*) argument="${1#*=}"; shift ;;
      -LE?*) argument="${1:3}"; shift ;;
      *) forwarded+=("$1"); shift; continue ;;
    esac
    if [[ -n "$argument" ]]; then
      excluded_labels="$excluded_labels|($argument)"
    fi
  done
  # Bash 3 with nounset needs the guarded expansion for an empty array.
  invoke_ctest --no-tests=error ${forwarded[@]+"${forwarded[@]}"} --label-exclude "$excluded_labels"
}

run_ctest "$@" | awk '
  /^[[:space:]]*Start[[:space:]]+[0-9]+:/ { next }
  /^[[:space:]]*[0-9]+\/[0-9]+[[:space:]]+Test[[:space:]]+#?[[:space:]]*[0-9]+:.*[[:space:]]Passed[[:space:]]+[0-9.]+[[:space:]]+sec[[:space:]]*$/ { next }
  { print; fflush() }
'
