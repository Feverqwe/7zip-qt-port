#!/usr/bin/env bash
# Prepare the official dynamic console engine and module for external plugins.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
upstream_checkout="${1:-$project_root/../7zip-upstream}"
artifact_dir="${2:-$project_root/build/codec-runtime}"
case "$(uname -s)" in
  Darwin|Linux) ;;
  *) printf 'Unix runtime builder; Windows uses official 7z.exe and 7z.dll.\n' >&2; exit 1 ;;
esac
revision="$(python3 - "$project_root/docs/upstream-baseline.json" <<'PY'
import json, sys
print(json.load(open(sys.argv[1]))["revision"])
PY
)"
git -C "$upstream_checkout" cat-file -e "$revision^{commit}"
mkdir -p "$artifact_dir"
python3 - "$artifact_dir" <<'PY'
import json, pathlib, sys
directory = pathlib.Path(sys.argv[1])
files = [directory / name for name in ("7z", "7zz", "7z.so", "runtime.json", "LICENSE.txt")]
if any(path.exists() or path.is_symlink() for path in files):
    if any(path.is_symlink() for path in files):
        raise SystemExit("Refusing to replace a symlink codec runtime")
    manifest = directory / "runtime.json"
    if not manifest.is_file() or json.loads(manifest.read_text()).get("producer") != "sevenzip-desktop/build-codec-runtime":
        raise SystemExit("Refusing to replace an unowned codec runtime")
PY
snapshot="$(mktemp -d "$artifact_dir/.build-runtime-XXXXXX")"
trap 'rm -rf -- "$snapshot"' EXIT
git -C "$upstream_checkout" archive "$revision" | tar -x -C "$snapshot"
if [[ "$(uname -s)" == Darwin ]]; then
  if ! printf '#include <type_traits>\n' | c++ -x c++ -std=c++17 -fsyntax-only - >/dev/null 2>&1; then
    task_codec_sdk="$(xcrun --show-sdk-path)"
    export CPLUS_INCLUDE_PATH="$task_codec_sdk/usr/include/c++/v1${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
  fi
fi
for source in CPP/7zip/UI/Console CPP/7zip/Bundles/Format7zF CPP/7zip/Bundles/Alone2; do
  make -s -C "$snapshot/$source" -f makefile.gcc \
    CC="${CC:-cc}" CXX="${CXX:-c++}" LFLAGS_STRIP= -j4
done
python3 - "$project_root/docs/upstream-baseline.json" "$snapshot" "$artifact_dir" <<'PY'
import hashlib, json, pathlib, platform, re, shutil, sys
baseline = json.loads(pathlib.Path(sys.argv[1]).read_text())
snapshot, output = map(pathlib.Path, sys.argv[2:])
sources = {"7z": "CPP/7zip/UI/Console/_o/7z",
           "7zz": "CPP/7zip/Bundles/Alone2/_o/7zz",
           "7z.so": "CPP/7zip/Bundles/Format7zF/_o/7z.so"}
header = (snapshot / "C/7zVersion.h").read_text()
major, minor = [int(re.search(r"#define MY_VER_" + name + r"\s+(\d+)", header)[1])
                for name in ("MAJOR", "MINOR")]
files = {name: {"source": path, "sha256": hashlib.sha256((snapshot / path).read_bytes()).hexdigest()}
         for name, path in sources.items()}
manifest = {"schema": 1, "producer": "sevenzip-desktop/build-codec-runtime",
            "repository": baseline["repository"], "revision": baseline["revision"],
            "system": platform.system(), "architecture": platform.machine(),
            "version": (major << 16) | minor, "files": files}
for name, source in sources.items():
    staged = snapshot / name
    shutil.copy2(snapshot / source, staged)
    staged.replace(output / name)
shutil.copy2(snapshot / "DOC/License.txt", output / "LICENSE.txt")
record = snapshot / "runtime.json"
record.write_text(json.dumps(manifest, indent=2) + "\n")
record.replace(output / record.name)
print("Prepared official external-codec runtime:", output)
print("Upstream:", baseline["revision"], "version:", f"{major}.{minor:02}")
PY
