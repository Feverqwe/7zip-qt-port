# Shared by Unix entry points; explicit compiler and SDK choices take priority.
sevenzip_select_toolchain() {
    local selected_environment
    selected_environment="$(python3 -B "$project_root/scripts/lib/toolchain.py" --root "$project_root" --preset "${1:-dev}" ${2:+--runtime-only})" || return
    eval "$selected_environment"
}
