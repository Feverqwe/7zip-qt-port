#!/usr/bin/env bash
# Build the official native console SFX from the recorded, immutable source.
set -euo pipefail
project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
upstream_checkout="${1:-$project_root/../7zip-upstream}"
artifact_dir="${2:-$project_root/build/sfx}"
case "$(uname -s)" in
  Darwin|Linux) ;;
  *) printf 'This script builds Unix console SFX. Windows uses the official GUI 7z.sfx module.\n' >&2; exit 1 ;;
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
module = directory / "7zCon.sfx"
manifest = directory / "7zCon.sfx.json"
license_file = directory / "7zCon-LICENSE.txt"
if any(path.exists() or path.is_symlink() for path in (module, manifest, license_file)):
    if any(path.is_symlink() for path in (module, manifest, license_file)):
        raise SystemExit("Refusing to replace a symlink SFX artifact")
    if not manifest.is_file() or json.loads(manifest.read_text()).get("producer") != "sevenzip-desktop/build-sfx":
        raise SystemExit("Refusing to replace an unowned SFX artifact")
PY
snapshot="$(mktemp -d "$artifact_dir/.build-sfx-XXXXXX")"
trap 'rm -rf -- "$snapshot"' EXIT
git -C "$upstream_checkout" archive "$revision" | tar -x -C "$snapshot"
if [[ "$(uname -s)" == Darwin ]]; then
  if ! printf '#include <type_traits>\n' | c++ -x c++ -std=c++17 -fsyntax-only - >/dev/null 2>&1; then
    task_sfx_sdk="$(xcrun --show-sdk-path)"
    export CPLUS_INCLUDE_PATH="$task_sfx_sdk/usr/include/c++/v1${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
  fi
fi
# Keep upstream makefiles and engine unchanged. Only host toolchain flags differ.
make -s -C "$snapshot/CPP/7zip/Bundles/SFXCon" -f makefile.gcc \
  CC="${CC:-cc}" CXX="${CXX:-c++}" LFLAGS_STRIP= -j4
python3 - "$project_root/docs/upstream-baseline.json" "$snapshot" "$artifact_dir" <<'PY'
import hashlib, json, pathlib, platform, re, shutil, sys
baseline = json.loads(pathlib.Path(sys.argv[1]).read_text())
snapshot, output = map(pathlib.Path, sys.argv[2:])
source = snapshot / "CPP/7zip/Bundles/SFXCon/_o/7zCon"
header = (snapshot / "C/7zVersion.h").read_text()
major, minor = [int(re.search(r"#define MY_VER_" + name + r"\s+(\d+)", header)[1])
                for name in ("MAJOR", "MINOR")]
digest = hashlib.sha256(source.read_bytes()).hexdigest()
manifest = {"schema": 1, "producer": "sevenzip-desktop/build-sfx",
            "repository": baseline["repository"], "revision": baseline["revision"],
            "source": "CPP/7zip/Bundles/SFXCon/makefile.gcc",
            "target": "native-console", "system": platform.system(),
            "architecture": platform.machine(), "version": (major << 16) | minor,
            "sha256": digest}
staged = snapshot / "7zCon.sfx"
shutil.copy2(source, staged)
staged.chmod(0o755)
staged_manifest = snapshot / "7zCon.sfx.json"
staged_manifest.write_text(json.dumps(manifest, indent=2) + "\n")
staged.replace(output / staged.name)
staged_manifest.replace(output / staged_manifest.name)
shutil.copy2(snapshot / "DOC/License.txt", output / "7zCon-LICENSE.txt")
print("Built official native console SFX:", output / "7zCon.sfx")
print("Upstream:", baseline["revision"], "SHA-256:", digest)
PY
