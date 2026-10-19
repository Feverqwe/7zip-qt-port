#!/usr/bin/env bash
# Generates ordinary fixture files only. Never attaches or mounts a device.
set -euo pipefail
destination=${1:?usage: generate.sh OUTPUT_DIRECTORY}
mkdir -p "$destination"
fixture=$(mktemp -d)
trap 'rm -rf "$fixture"' EXIT
mkdir -p "$fixture/input/папка 空"
printf 'device ext4 payload\n' > "$fixture/input/папка 空/данные с пробелом.txt"
truncate -s 8M "$fixture/ext4.img"
mke2fs -q -t ext4 -b 1024 -I 256 -O ^has_journal \
    -d "$fixture/input" -F "$fixture/ext4.img"
truncate -s 12M "$fixture/mbr.img"
printf 'label: dos\nstart=2048, size=16384, type=83\n' |
    sfdisk --quiet "$fixture/mbr.img"
dd if="$fixture/ext4.img" of="$fixture/mbr.img" bs=1M seek=1 conv=notrunc status=none
truncate -s 16M "$fixture/gpt.img"
printf 'label: gpt\nstart=2048, size=16384, type=L, name="Linux data"\nstart=18432, size=4096, type=L, name="Unsupported data"\n' |
    sfdisk --quiet "$fixture/gpt.img"
dd if="$fixture/ext4.img" of="$fixture/gpt.img" bs=1M seek=1 conv=notrunc status=none
for layout in ext4 mbr gpt; do
    gzip -n -c "$fixture/$layout.img" > "$destination/$layout.img.gz"
done
