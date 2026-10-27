#!/usr/bin/env bash
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "$#" == 0 ]]; then set -- status; fi
exec python3 "$project_root/scripts/lib/local_ramdisk.py" "$@" --root "$project_root"
