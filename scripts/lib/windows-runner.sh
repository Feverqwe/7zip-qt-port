# Shared guard for commands that use the configured Windows test machine.
sevenzip_require_windows_runner() {
    local expected="${SEVENZIP_WINDOWS_RUNNER_HOST:-}"
    if [[ -z "$expected" ]]; then
        local interpreter="${SEVENZIP_RUNNER_PYTHON:-python}"
        if [[ -z "${SEVENZIP_RUNNER_PYTHON:-}" ]] && command -v python3 >/dev/null 2>&1; then
            interpreter=python3
        fi
        expected="$("$interpreter" scripts/lib/runner_config.py windows-host)" || return
    fi
    if [[ -z "${COMPUTERNAME:-}" || "$(printf '%s' "$COMPUTERNAME" | tr '[:lower:]' '[:upper:]')" != "$(printf '%s' "$expected" | tr '[:lower:]' '[:upper:]')" ]]; then
        printf 'This command requires the Windows test host configured in runners.local.json.\n' >&2
        return 2
    fi
}
