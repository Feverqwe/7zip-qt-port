#!/usr/bin/env python3
"""Exercise an installed runtime with system paths and disposable user data."""

import argparse
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
sys.dont_write_bytecode = True
import tempfile

from lib.build_paths import checked_output, directory

ROOT = Path(__file__).resolve().parents[1]


def environment(fixture, binaries, system):
    # Keep desktop/session access, but no compiler, loader, Qt SDK or engine
    # overrides. The package must discover its own libraries and codecs.
    inherited = ("SystemRoot", "WINDIR", "USER", "LOGNAME", "LANG", "LC_ALL",
                 "LC_CTYPE", "DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY",
                 "XDG_RUNTIME_DIR", "DBUS_SESSION_BUS_ADDRESS", "SECURITYSESSIONID",
                 "__CF_USER_TEXT_ENCODING")
    result = {key: os.environ[key] for key in inherited if key in os.environ}
    home = fixture / "home"
    temporary = fixture / "tmp"
    home.mkdir()
    temporary.mkdir()
    result.update(HOME=str(home), USERPROFILE=str(home), TMPDIR=str(temporary),
                  TMP=str(temporary), TEMP=str(temporary),
                  SEVENZIP_SETTINGS_PATH=str(fixture / "settings.ini"),
                  SEVENZIP_OWNED_TEMP_REGISTRY=str(fixture / "owned-temp"),
                  XDG_CONFIG_HOME=str(home / "config"), XDG_CACHE_HOME=str(home / "cache"),
                  XDG_DATA_HOME=str(home / "data"), APPDATA=str(home / "AppData/Roaming"),
                  LOCALAPPDATA=str(home / "AppData/Local"))
    if system == "Windows":
        windows = result.get("SystemRoot") or result.get("WINDIR")
        if not windows:
            raise ValueError("Windows system directory is unavailable")
        paths = [binaries, Path(windows) / "System32", Path(windows)]
    else:
        paths = [binaries, "/usr/bin", "/bin", "/usr/sbin", "/sbin"]
    result["PATH"] = os.pathsep.join(map(str, paths))
    return result


def run(program, arguments, fixture, env):
    command = [str(program), *map(str, arguments)]
    try:
        result = subprocess.run(command, cwd=fixture, env=env, capture_output=True, timeout=60)
    except subprocess.TimeoutExpired as error:
        sys.stderr.buffer.write((error.stdout or b"") + (error.stderr or b""))
        raise
    if result.returncode:
        sys.stderr.buffer.write(result.stdout + result.stderr)
        raise ValueError(f"Installed {program.name} failed with exit code {result.returncode}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", type=Path, required=True,
                        help="existing package directory inside the project build workspace")
    args = parser.parse_args()
    install = checked_output(ROOT, args.install)
    system = platform.system()
    if system == "Linux" and not (
            os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        display = shutil.which("xvfb-run", path="/usr/bin:/bin")
        if not display:
            raise ValueError("A desktop session or system xvfb-run is required")
        # Let the display wrapper own the entire check. If a GUI times out,
        # Python kills that child and exits normally through Xvfb's cleanup.
        return subprocess.call([display, "-a", sys.executable, str(Path(__file__).resolve()),
                                *sys.argv[1:]], cwd=ROOT)
    if system == "Darwin":
        bundle = install / "Sevenzip.app/Contents"
        binaries = bundle / "MacOS"
        console = bundle / "Resources/7zip/7z"
        suffix = ""
    elif system in ("Linux", "Windows"):
        binaries = install / "bin"
        suffix = ".exe" if system == "Windows" else ""
        console = binaries / ("7z" + suffix)
    else:
        raise ValueError(f"Unsupported runtime platform: {system}")
    fm, gui = (binaries / (name + suffix) for name in ("7zFM", "7zG"))
    temporary = directory(ROOT, "tmp")
    temporary.mkdir(parents=True, exist_ok=True)
    fixture = Path(tempfile.mkdtemp(prefix="installed-runtime-", dir=temporary))
    try:
        env = environment(fixture, binaries, system)
        source = fixture / "данные 空 with spaces.txt"
        payload = "Installed C++23 runtime: Unicode 密碼\n".encode()
        source.write_bytes(payload)
        archive = fixture / "архив 空 with spaces.7z"
        output = fixture / "извлечено 空"
        run(fm, ["--smoke-test"], fixture, env)
        run(console, ["a", "-y", "--", archive, source.name], fixture, env)
        original_archive = archive.read_bytes()
        run(console, ["t", "--", archive], fixture, env)
        run(gui, ["t", "-y", "--", archive], fixture, env)
        run(gui, ["x", "-y", "-o" + str(output), "--", archive], fixture, env)
        if (output / source.name).read_bytes() != payload:
            raise ValueError("Installed GUI extraction differs from its source")
        if source.read_bytes() != payload or archive.read_bytes() != original_archive:
            raise ValueError("Installed runtime modified its input")
        # Reuse the independent lzlib fixture already exercised by the decoder
        # suite. This also requires the deployed external-format module.
        stream = fixture / "сжатый 空 with spaces.lz"
        compressed = (ROOT / "tests/fixtures/lzip/fox.lz").read_bytes()
        stream.write_bytes(compressed)
        external_output = fixture / "external codec output"
        run(gui, ["t", "-y", "--", stream], fixture, env)
        run(gui, ["x", "-y", "-o" + str(external_output), "--", stream], fixture, env)
        extracted = [path.read_bytes() for path in external_output.rglob("*") if path.is_file()]
        if extracted != [b"The quick brown fox jumps over the lazy dog.\n"]:
            raise ValueError("Installed external codec extraction differs from its fixture")
        if stream.read_bytes() != compressed:
            raise ValueError("Installed external codec modified its input")
    except BaseException:
        print(f"Failure fixture retained: {fixture}", file=sys.stderr)
        raise
    else:
        shutil.rmtree(fixture)
    print("Installed runtime: startup, console, GUI and external codec PASS (installed/system paths)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(f"check-installed-runtime: {error}", file=sys.stderr)
        raise SystemExit(1)
