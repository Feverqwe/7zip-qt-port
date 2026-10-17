#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

python3 scripts/prepare-runtime.py
./scripts/build.sh release
python3 scripts/package.py --install-only "$project_root/dist"
printf 'Application output: %s/dist\n' "$project_root"
