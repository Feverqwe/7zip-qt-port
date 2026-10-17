#!/usr/bin/env bash
set -u

failed=0
check_command() {
  if command -v "$1" >/dev/null 2>&1; then
    printf 'OK   %-12s %s\n' "$1" "$(command -v "$1")"
  else
    printf 'MISS %-12s\n' "$1"
    failed=1
  fi
}

check_command git
check_command cmake
check_command ninja
check_command c++
check_command 7zz
check_command clang-format

if [[ -d ../7zip-upstream/.git ]]; then
  printf 'OK   upstream     %s\n' "$(git -C ../7zip-upstream rev-parse --short HEAD)"
else
  printf 'MISS upstream     expected ../7zip-upstream\n'
  failed=1
fi

if command -v cmake >/dev/null 2>&1; then
  if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
    printf 'INFO Qt prefix    %s\n' "$CMAKE_PREFIX_PATH"
  elif [[ "$(uname -s)" == Darwin ]]; then
    qt_prefix="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/build/qt/6.12.0/macos"
    if [[ -f "$qt_prefix/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
      printf 'INFO Qt prefix    %s\n' "$qt_prefix"
    else
      printf 'MISS Qt           install official Qt 6.12.0 into build/qt or set CMAKE_PREFIX_PATH\n'
      failed=1
    fi
  elif command -v brew >/dev/null 2>&1 && brew --prefix qtbase >/dev/null 2>&1; then
    printf 'INFO Qt prefix    %s\n' "$(brew --prefix qtbase)"
  else
    printf 'INFO Qt prefix    use CMAKE_PREFIX_PATH if Qt is not found\n'
  fi
fi

exit "$failed"
