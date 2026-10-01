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

exec ctest "$@"
