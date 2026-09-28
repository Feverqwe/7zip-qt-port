#!/bin/sh
set -eu

repo_root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
fixture=$(mktemp -d "${TMPDIR:-/tmp}/sevenzip-memory.XXXXXX")
trap 'rm -rf "$fixture"' EXIT HUP INT TERM

mkdir -p "$fixture/.agents/skills/sevenzip-setup/scripts" "$fixture/.agents/skills/sevenzip-setup/assets" "$fixture/docs"
cp "$repo_root/.agents/skills/sevenzip-setup/scripts/setup.sh" "$fixture/.agents/skills/sevenzip-setup/scripts/"
cp -R "$repo_root/.agents/skills/sevenzip-setup/assets/codex" "$fixture/.agents/skills/sevenzip-setup/assets/"
touch "$fixture/AGENTS.md" "$fixture/CMakeLists.txt" "$fixture/README.md"
git -C "$fixture" init --quiet
setup_script="$fixture/.agents/skills/sevenzip-setup/scripts/setup.sh"

sh "$setup_script"
test -s "$fixture/.codex/memory/README.md"
test -s "$fixture/.codex/templates/handoff.md"
test -L "$fixture/.codex/docs"
if git -C "$fixture" config --local --get core.hooksPath; then
  echo 'Setup unexpectedly installed Git hooks.' >&2
  exit 1
fi
git -C "$fixture" config --local core.hooksPath user-owned-hooks

# A populated index and task note are user-owned data, not setup templates.
printf '# Existing memory\n\n- [Research](../notes/research.md)\n' > "$fixture/.codex/memory/README.md"
printf 'Evidence and unresolved upstream question\n' > "$fixture/.codex/notes/research.md"
printf 'Local attachment\n' > "$fixture/.codex/attachments/evidence.txt"
cp "$fixture/.codex/memory/README.md" "$fixture/expected-index.md"
cp "$fixture/.codex/notes/research.md" "$fixture/expected-note.md"
cp "$fixture/.codex/attachments/evidence.txt" "$fixture/expected-attachment.txt"

sh "$setup_script"
cmp "$fixture/expected-index.md" "$fixture/.codex/memory/README.md"
cmp "$fixture/expected-note.md" "$fixture/.codex/notes/research.md"
cmp "$fixture/expected-attachment.txt" "$fixture/.codex/attachments/evidence.txt"
test "$(git -C "$fixture" config --local core.hooksPath)" = user-owned-hooks
echo 'Project memory initializes and survives repeated setup.'
