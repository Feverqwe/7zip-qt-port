#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"

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
./scripts/build.sh dev
ctest --preset dev --output-on-failure
