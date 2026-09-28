#!/bin/sh
set -eu

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH='' cd -- "$script_dir/../../../.." && pwd)
assets_dir="$script_dir/../assets/codex"
target_dir="$repo_root/.codex"

if [ ! -f "$repo_root/AGENTS.md" ] || [ ! -f "$repo_root/CMakeLists.txt" ]; then
  echo "Refusing setup: $repo_root is not the Sevenzip Desktop repository." >&2
  exit 1
fi

mkdir -p "$target_dir/.obsidian" "$target_dir/context" "$target_dir/notes" "$target_dir/templates" "$target_dir/attachments" "$target_dir/memory"

if [ ! -e "$target_dir/memory/README.md" ] && [ ! -L "$target_dir/memory/README.md" ]; then
  cp "$assets_dir/memory/README.md" "$target_dir/memory/README.md"
fi

cp "$assets_dir/Home.md" "$target_dir/Home.md"
cp "$assets_dir/context/README.md" "$target_dir/context/README.md"
cp "$assets_dir/templates/investigation.md" "$target_dir/templates/investigation.md"
cp "$assets_dir/templates/handoff.md" "$target_dir/templates/handoff.md"
cp "$assets_dir/.obsidian/app.json" "$target_dir/.obsidian/app.json"
cp "$assets_dir/.obsidian/appearance.json" "$target_dir/.obsidian/appearance.json"
cp "$assets_dir/.obsidian/core-plugins.json" "$target_dir/.obsidian/core-plugins.json"
cp "$assets_dir/.obsidian/templates.json" "$target_dir/.obsidian/templates.json"

ensure_link() {
  link_path=$1
  link_target=$2
  if [ -L "$link_path" ]; then
    ln -sfn "$link_target" "$link_path"
  elif [ -e "$link_path" ]; then
    echo "Refusing to replace non-symlink: $link_path" >&2
    exit 1
  else
    ln -s "$link_target" "$link_path"
  fi
}

ensure_link "$target_dir/README.md" "../README.md"
ensure_link "$target_dir/AGENTS.md" "../AGENTS.md"
ensure_link "$target_dir/docs" "../docs"

echo "Sevenzip Desktop Obsidian vault and task memory are ready at $target_dir"
