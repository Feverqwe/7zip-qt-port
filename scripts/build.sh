#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_root"
preset="${1:-dev}"
build_only=0
if [[ "${2:-}" == --build-only ]]; then
  build_only=1
fi

# Keep MSVC /showIncludes encoding consistent even without English language resources.
if [[ "${OS:-}" == Windows_NT ]]; then
  export VSLANG="${VSLANG:-1033}"
  windows_code_page_output="$(chcp.com)"
  if [[ "$windows_code_page_output" =~ ([0-9]+)[^0-9]*$ ]]; then
    windows_code_page="${BASH_REMATCH[1]}"
    chcp.com 65001 >/dev/null
    trap 'chcp.com "$windows_code_page" >/dev/null' EXIT
  else
    printf 'Could not read the Windows console code page.\n' >&2
    exit 1
  fi
fi

compiler_cache=""
if command -v ccache >/dev/null 2>&1; then
  compiler_cache="$(command -v ccache)"
  if [[ "${OS:-}" == Windows_NT ]]; then
    compiler_cache="$(cygpath -m "$compiler_cache")"
  fi
  if [[ -z "${CCACHE_DIR:-}" && "$(uname -s)" == Darwin ]]; then
    export CCACHE_DIR=/private/tmp/sevenzip-desktop-ccache
  fi
fi

configure() {
  if [[ "$build_only" == 1 ]]; then
    if [[ ! -f "$project_root/build/$preset/CMakeCache.txt" ]]; then
      printf 'Configure %s before using --build-only.\n' "$preset" >&2
      exit 2
    fi
    return
  fi
  if [[ "${OS:-}" == Windows_NT && -n "$compiler_cache" ]]; then
    # MSVC shared /Zi databases cannot be cached. Embedded /Z7 retains debug
    # information in each object and lets ccache restore complete outputs.
    set -- -DCMAKE_POLICY_DEFAULT_CMP0141=NEW -DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded "$@"
  fi
  local runtime_directory="${SEVENZIP_CODEC_MODULE_DIR:-$project_root/build/codec-runtime}"
  local runtime_module="$runtime_directory/7z.so"
  if [[ "${OS:-}" == Windows_NT ]]; then
    runtime_module="$runtime_directory/7z.dll"
  fi
  if [[ -f "$runtime_module" ]]; then
    set -- "-DSEVENZIP_CODEC_MODULE=$runtime_module" "$@"
  fi
  if [[ -n "${SEVENZIP_COMPACT_TEST_BINARIES:-}" ]]; then
    set -- "-DSEVENZIP_COMPACT_TEST_BINARIES=$SEVENZIP_COMPACT_TEST_BINARIES" "$@"
  fi
  if [[ -n "$compiler_cache" ]]; then
    set -- \
      "-DCMAKE_C_COMPILER_LAUNCHER=$compiler_cache" \
      "-DCMAKE_CXX_COMPILER_LAUNCHER=$compiler_cache" \
      "-DCMAKE_OBJCXX_COMPILER_LAUNCHER=$compiler_cache" "$@"
  fi
  local cache_file="$project_root/build/$preset/CMakeCache.txt"
  local signature_file="$project_root/build/$preset/.configure-signature"
  local requested_signature
  requested_signature="$(printf '%q ' "$preset" "$@" \
    "${CMAKE_PREFIX_PATH:-}" "${CC:-}" "${CXX:-}" "${OBJCXX:-}" \
    "${CFLAGS:-}" "${CXXFLAGS:-}" "${OBJCXXFLAGS:-}" "${LDFLAGS:-}" \
    "${CMAKE_TOOLCHAIN_FILE:-}" "${SDKROOT:-}" "${CPLUS_INCLUDE_PATH:-}" \
    "$(cksum "$project_root/scripts/build.sh" "$project_root/CMakePresets.json")" | cksum)"
  local existing_signature=""
  if [[ -f "$cache_file" && -f "$signature_file" && -f "build/$preset/build.ninja" ]]; then
    existing_signature="$(printf '%s\n%s\n' "$requested_signature" "$(cksum "$cache_file")")"
  fi
  if [[ -n "$existing_signature" && "$(cat "$signature_file")" == "$existing_signature" ]]; then
    # Ninja owns CMake input/glob dependencies. Regenerate only when needed,
    # before CTest selection; this target does not compile the application.
    cmake --build --preset "$preset" --target build.ninja -- --quiet
  else
    cmake --preset "$preset" --log-level=WARNING "$@" >/dev/null
  fi
  printf '%s\n%s\n' "$requested_signature" "$(cksum "$cache_file")" > "$signature_file"
}

if [[ "$preset" != dev && "$preset" != release ]]; then
    printf 'Usage: %s [dev|release] [target ...]\n' "$0" >&2
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
  fi

  if [[ -z "${CMAKE_PREFIX_PATH:-}" ]]; then
    export CMAKE_PREFIX_PATH="$project_root/build/qt/6.12.0/macos"
    if [[ ! -f "$CMAKE_PREFIX_PATH/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
      printf 'Install the official Qt 6.12.0 macOS kit into build/qt, or set CMAKE_PREFIX_PATH to its macos directory. See docs/development.md.\n' >&2
      exit 2
    fi
  fi
  # A different SDK must not inherit cached package locations from Homebrew.
  # Keep the build tree and compiler cache; refresh only Qt package discovery.
  configure -U 'Qt6*_DIR' "-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.4
else
  configure
fi

if [[ "$#" -gt 0 ]]; then
  shift
fi
if [[ "$build_only" == 1 ]]; then
  shift
fi
if [[ "${1:-}" == --configure-only ]]; then
  if [[ "$#" != 1 ]]; then
    printf '%s: --configure-only does not accept targets.\n' "$0" >&2
    exit 2
  fi
  exit 0
fi
if [[ "$#" -gt 0 ]]; then
  cmake --build --preset "$preset" --target "$@" -- --quiet
else
  cmake --build --preset "$preset" -- --quiet
fi
