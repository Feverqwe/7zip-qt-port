#!/usr/bin/env bash

# Source this file from a Bash script, then call sevenzip_setup_output once.
sevenzip_setup_output() {
  sevenzip_output_label="$1"
  case "${SEVENZIP_SCRIPT_OUTPUT:-auto}" in
    compact) ;;
    verbose) return 0 ;;
    auto)
      [[ -n "${CODEX_SESSION_ID:-}" ]] || return 0
      ;;
    *)
      printf 'SEVENZIP_SCRIPT_OUTPUT must be auto, compact, or verbose.\n' >&2
      return 2
      ;;
  esac

  sevenzip_output_owner_depth="$BASH_SUBSHELL"
  sevenzip_output_log="$(mktemp "${TMPDIR:-/tmp}/sevenzip-script.XXXXXX")"
  exec 3>&1 4>&2
  printf '%s: running (output buffered; wait for exit)\n' "$sevenzip_output_label" >&3
  exec >"$sevenzip_output_log" 2>&1
  trap 'sevenzip_finish_output "$?"' EXIT
}

sevenzip_run_command() {
  local command_label="$1"
  shift
  if [[ -z "${sevenzip_output_log:-}" ]]; then
    "$@"
    return
  fi

  (
    sleep "${SEVENZIP_PROGRESS_DELAY_SECONDS:-15}" >/dev/null 2>&1 3>&- 4>&- &
    sleeper_pid="$!"
    trap 'kill "$sleeper_pid" 2>/dev/null || true; exit 0' TERM INT
    trap 'kill "$sleeper_pid" 2>/dev/null || true' EXIT
    wait "$sleeper_pid" || exit 0
    printf '%s: still running: %s\n' "$sevenzip_output_label" "$command_label" >&3
  ) &
  local notice_pid="$!"
  local status=0
  "$@" || status="$?"
  kill "$notice_pid" 2>/dev/null || true
  wait "$notice_pid" 2>/dev/null || true
  return "$status"
}

sevenzip_finish_output() {
  local status="$1"
  # A notifier can receive TERM before installing its own traps. Its inherited
  # EXIT trap must not finalize or remove the parent shell's buffered log.
  [[ "$BASH_SUBSHELL" == "$sevenzip_output_owner_depth" ]] || return 0
  if [[ "$status" -eq 0 ]]; then
    printf '%s: OK\n' "$sevenzip_output_label" >&3
    rm -f "$sevenzip_output_log"
  else
    printf '%s: FAILED (exit %s)\n' "$sevenzip_output_label" "$status" >&4
    printf 'Last 30 log lines:\n' >&4
    tail -n 30 "$sevenzip_output_log" >&4
    printf 'Full log: %s\n' "$sevenzip_output_log" >&4
  fi
}
