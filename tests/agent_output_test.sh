#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fixture="$(mktemp -d "${TMPDIR:-/tmp}/sevenzip-output-test.XXXXXX")"
trap 'rm -rf "$fixture"' EXIT
export SEVENZIP_OUTPUT_HELPER="$repo_root/scripts/agent-output.sh"
export TMPDIR="$fixture"

compact_success="$(CODEX_SESSION_ID=test SEVENZIP_SCRIPT_OUTPUT=auto bash -c '
  source "$SEVENZIP_OUTPUT_HELPER"
  sevenzip_setup_output sample
  printf "build details\n"
')"
[[ "$compact_success" == 'sample: OK' ]]
[[ -z "$(find "$fixture" -type f -print -quit)" ]]

forced_compact="$(CODEX_SESSION_ID= SEVENZIP_SCRIPT_OUTPUT=compact bash -c '
  source "$SEVENZIP_OUTPUT_HELPER"
  sevenzip_setup_output sample
  printf "build details\n"
')"
[[ "$forced_compact" == 'sample: OK' ]]

verbose_success="$(CODEX_SESSION_ID=test SEVENZIP_SCRIPT_OUTPUT=verbose bash -c '
  source "$SEVENZIP_OUTPUT_HELPER"
  sevenzip_setup_output sample
  printf "build details\n"
')"
[[ "$verbose_success" == 'build details' ]]

human_success="$(CODEX_SESSION_ID= SEVENZIP_SCRIPT_OUTPUT=auto bash -c '
  source "$SEVENZIP_OUTPUT_HELPER"
  sevenzip_setup_output sample
  printf "build details\n"
')"
[[ "$human_success" == 'build details' ]]

if failure_output="$(CODEX_SESSION_ID=test SEVENZIP_SCRIPT_OUTPUT=auto bash -c '
  source "$SEVENZIP_OUTPUT_HELPER"
  sevenzip_setup_output sample
  printf "first detail\n"
  for number in {1..90}; do printf "line %s\n" "$number"; done
  exit 7
' 2>&1)"; then
  printf 'A failed command unexpectedly succeeded.\n' >&2
  exit 1
else
  status="$?"
fi
[[ "$status" -eq 7 ]]
[[ "$failure_output" == *'sample: FAILED (exit 7)'* ]]
[[ "$failure_output" == *'line 90'* ]]
[[ "$failure_output" != *'first detail'* ]]
log_path="${failure_output##*Full log: }"
[[ -f "$log_path" ]]
grep -q '^first detail$' "$log_path"

mkdir -p "$fixture/bin"
cat > "$fixture/bin/ctest" <<'EOF'
#!/usr/bin/env bash
printf '%s\n' "$@" > "$FAKE_CTEST_ARGS"
printf 'fixture CTest output\n'
exit "${FAKE_CTEST_STATUS:-0}"
EOF
chmod +x "$fixture/bin/ctest"
export PATH="$fixture/bin:$PATH"
export FAKE_CTEST_ARGS="$fixture/ctest-args"

wrapper_success="$(CODEX_SESSION_ID=test SEVENZIP_SCRIPT_OUTPUT=auto bash \
  "$repo_root/scripts/ctest.sh" --preset dev -R '^agent_output$')"
[[ "$wrapper_success" == "ctest --preset dev -R ^agent_output$: OK" ]]
[[ "$(cat "$FAKE_CTEST_ARGS")" == $'--preset\ndev\n-R\n^agent_output$' ]]

if wrapper_failure="$(CODEX_SESSION_ID=test SEVENZIP_SCRIPT_OUTPUT=auto FAKE_CTEST_STATUS=9 bash \
  "$repo_root/scripts/ctest.sh" --preset dev 2>&1)"; then
  printf 'A failed CTest wrapper unexpectedly succeeded.\n' >&2
  exit 1
else
  status="$?"
fi
[[ "$status" -eq 9 ]]
[[ "$wrapper_failure" == *'ctest --preset dev: FAILED (exit 9)'* ]]
[[ "$wrapper_failure" == *'fixture CTest output'* ]]

if sandbox_output="$(CODEX_SANDBOX=seatbelt bash "$repo_root/scripts/check.sh" 2>&1)"; then
  printf 'The full check unexpectedly ran in a sandbox.\n' >&2
  exit 1
else
  status="$?"
fi
[[ "$status" -eq 2 ]]
[[ "$sandbox_output" == 'check.sh: full regression gate requires an unsandboxed run (CODEX_SANDBOX=seatbelt).' ]]

printf 'Agent output keeps successes short and failures diagnosable.\n'
