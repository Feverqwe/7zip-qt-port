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

  sevenzip_output_log="$(mktemp "${TMPDIR:-/tmp}/sevenzip-script.XXXXXX")"
  exec 3>&1 4>&2
  exec >"$sevenzip_output_log" 2>&1
  trap 'sevenzip_finish_output "$?"' EXIT
}

sevenzip_finish_output() {
  local status="$1"
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
