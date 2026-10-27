#!/usr/bin/env bash
set -euo pipefail

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
./scripts/build.sh "$preset"
python3 scripts/package.py --build-dir "$project_root/build/$preset" \
  --install-only "$project_root/dist"
printf 'Application output: %s/dist\n' "$project_root"
