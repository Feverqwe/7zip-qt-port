# Shared by Unix entry points; explicit compiler and SDK choices take priority.
sevenzip_select_toolchain() {
  if [[ "$(uname -s)" == Darwin || "$(uname -s)" == Linux ]]; then
    local selected_environment
    selected_environment="$(python3 "$project_root/scripts/lib/toolchain.py" --root "$project_root" --preset "${1:-dev}" ${2:+--runtime-only})" || return
    eval "$selected_environment"
  fi
}
