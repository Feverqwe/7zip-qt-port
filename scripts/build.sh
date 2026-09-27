#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"
preset="${1:-dev}"

if [[ "$preset" != dev && "$preset" != release ]]; then
  printf 'Usage: %s [dev|release]\n' "$0" >&2
  exit 2
fi

if [[ "$(uname -s)" == Darwin ]]; then
  if ! printf '#include <type_traits>\n' | c++ -x c++ -std=c++17 -fsyntax-only - >/dev/null 2>&1; then
    sdk_path="$(xcrun --show-sdk-path)"
    sdk_cxx_headers="$sdk_path/usr/include/c++/v1"
    if [[ ! -f "$sdk_cxx_headers/type_traits" ]]; then
      printf 'C++ standard headers were not found in the macOS SDK.\n' >&2
      exit 1
    fi
    export CPLUS_INCLUDE_PATH="$sdk_cxx_headers${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
    printf 'Using C++ headers from %s\n' "$sdk_cxx_headers"
  fi

  if [[ -z "${CMAKE_PREFIX_PATH:-}" ]] && command -v brew >/dev/null 2>&1 && brew list --versions qtbase >/dev/null 2>&1; then
    cmake --preset "$preset" -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
  else
    cmake --preset "$preset"
  fi
else
  cmake --preset "$preset"
fi

cmake --build --preset "$preset"
