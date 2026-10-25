#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

if [[ "$(uname -s)" == Darwin && -z "${CMAKE_PREFIX_PATH:-}" ]]; then
  export CMAKE_PREFIX_PATH="$project_root/build/qt/6.12.0/macos"
fi
python3 scripts/prepare-openssl.py --configure-app --preset release
python3 scripts/prepare-runtime.py
./scripts/build.sh release
python3 scripts/package.py --install-only "$project_root/dist"
printf 'Application output: %s/dist\n' "$project_root"
