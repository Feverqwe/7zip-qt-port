#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

source "$project_root/scripts/agent-output.sh"
sevenzip_setup_output "ctest${*:+ $*}"
sevenzip_run_command "ctest${*:+ $*}" ctest "$@"
