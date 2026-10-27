#!/usr/bin/env bash
set -euo pipefail
export PYTHONDONTWRITEBYTECODE=1

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

preset="${1:-release}"
if [[ "$#" -gt 1 || ( "$preset" != release && "$preset" != vcpkg-release ) ]]; then
  printf 'Usage: %s [release|vcpkg-release]\n' "$0" >&2
  exit 2
fi
source "$project_root/scripts/lib/toolchain.sh"
sevenzip_select_toolchain "$preset"
python3 scripts/prepare-runtime.py --preset "$preset" --sanitizer-runtime off
./scripts/build.sh "$preset" all
python3 scripts/package.py --build-dir "$SEVENZIP_BUILD_ROOT/$preset" \
  --install-only "$SEVENZIP_ARTIFACT_ROOT/install/$preset"
printf 'Application output: %s/install/%s\n' "$SEVENZIP_ARTIFACT_ROOT" "$preset"
