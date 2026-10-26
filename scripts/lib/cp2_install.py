"""Install the pinned cp2 build without changing system PATH."""

from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tarfile
import tempfile
import time
import urllib.request



def version(binary, pin):
    try:
        result = subprocess.run([str(binary), "--version"], text=True, capture_output=True, timeout=15)
        return result.returncode == 0 and result.stdout.strip() == pin["banner"]
    except (OSError, subprocess.TimeoutExpired):
        return False


@contextmanager
def install_lock(path):
    """A process-owned lock, automatically released even after a failed build."""
    with path.open("a+b") as handle:
        handle.write(b"\0")
        handle.flush()
        deadline = time.monotonic() + 600
        while True:
            try:
                if os.name == "nt":
                    import msvcrt
                    handle.seek(0)
                    msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
                else:
                    import fcntl
                    fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise TimeoutError("Another cp2 installation is still running")
                time.sleep(1)
        try:
            yield
        finally:
            if os.name == "nt":
                handle.seek(0)
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(handle.fileno(), fcntl.LOCK_UN)


def download(url, checksum, destination):
    if destination.exists() and hashlib.sha256(destination.read_bytes()).hexdigest() == checksum:
        return
    with urllib.request.urlopen(url, timeout=120) as response:
        data = response.read()
    if hashlib.sha256(data).hexdigest() != checksum:
        raise ValueError("Download checksum mismatch: " + url)
    destination.write_bytes(data)


def unpack(archive, destination):
    with tarfile.open(archive) as source:
        for member in source.getmembers():
            path = Path(*Path(member.name).parts[1:])
            if path.is_absolute() or ".." in path.parts:
                raise ValueError("Unsafe cp2 archive path")
            if not path.parts or path.parts[0] not in (
                    "src", "setup", "Cargo.toml", "Cargo.lock", "build.rs", "LICENSE", "NOTICE"):
                continue
            if not member.isfile():
                if member.isdir():
                    continue
                raise ValueError("Unexpected cp2 archive entry")
            target = destination / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(source.extractfile(member).read())


def rust_environment(tools, directory, pin):
    windows = os.name == "nt"
    suffix = ".exe" if windows else ""
    environment = dict(os.environ, CARGO_HOME=str(tools / "cargo"),
                       RUSTUP_HOME=str(tools / "rustup"),
                       RUSTUP_INIT_SKIP_MSVC_CHECK="yes")
    temporary = directory / "temp"
    temporary.mkdir(exist_ok=True)
    environment.update(TEMP=str(temporary), TMP=str(temporary), TMPDIR=str(temporary))
    cargo = tools / "cargo/bin" / ("cargo" + suffix)
    rustup = cargo.with_name("rustup" + suffix)
    environment["PATH"] = str(cargo.parent) + os.pathsep + environment.get("PATH", "")
    if not rustup.exists():
        machine = {"amd64": "x86_64", "arm64": "aarch64"}.get(platform.machine().lower(), platform.machine().lower())
        system = {"Windows": "pc-windows-msvc", "Darwin": "apple-darwin", "Linux": "unknown-linux-gnu"}[platform.system()]
        triple = machine + "-" + system
        checksum = pin["rustup"]["sha256"].get(triple)
        if checksum is None:
            raise ValueError("No pinned rustup bootstrap for " + triple)
        bootstrap = directory / ("rustup-init" + suffix)
        download(pin["rustup"]["url"].format(triple=triple, suffix=suffix), checksum, bootstrap)
        bootstrap.chmod(0o755)
        subprocess.run([str(bootstrap), "-y", "--profile", "minimal", "--no-modify-path",
                        "--default-toolchain", pin["rust"]], env=environment, check=True)
    installed = subprocess.check_output([str(rustup), "toolchain", "list"], env=environment, text=True)
    if not any(line.startswith(pin["rust"] + "-") for line in installed.splitlines()):
        subprocess.run([str(rustup), "toolchain", "install", pin["rust"], "--profile", "minimal"],
                       env=environment, check=True)
    return cargo, environment


def compile_tool(tools, directory, pin, vcvars):
    cargo, environment = rust_environment(tools, directory, pin)
    windows = os.name == "nt"
    suffix = ".exe" if windows else ""
    archive = directory / "source.tar.gz"
    download(pin["archive"]["url"], pin["archive"]["sha256"], archive)
    source = directory / "source"
    unpack(archive, source)
    for edit in pin.get("source_edits", []):
        relative = Path(edit["source"])
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError("Unsafe cp2 adaptation path")
        target = source / relative
        content = target.read_bytes()
        for change in edit["replacements"]:
            before, after = change["before"].encode(), change["after"].encode()
            if not before or content.count(before) != 1:
                raise ValueError("Source adaptation context does not match exactly once")
            content = content.replace(before, after, 1)
        target.write_bytes(content)
    arguments = [str(cargo), "+" + pin["rust"], "build", "--release", "--locked", "--bin", "cp2"]
    if windows:
        if not vcvars:
            raise ValueError("Windows cp2 build requires configured vcvars")
        batch = directory / "build.cmd"
        batch.write_text('@echo off\ncall "' + vcvars + '"\nif errorlevel 1 exit /b %errorlevel%\n' +
                         subprocess.list2cmdline(arguments) + '\nexit /b %errorlevel%\n')
        subprocess.run(["cmd.exe", "/d", "/c", str(batch)], cwd=source, env=environment, check=True)
    else:
        subprocess.run(arguments, cwd=source, env=environment, check=True)
    return source / "target/release" / ("cp2" + suffix)


def artifact_directory(tools, pin):
    directory = Path(tools) / "cp2" / pin["revision"]
    if pin.get("edits"):
        digest = hashlib.sha256(json.dumps(pin["edits"], sort_keys=True).encode()).hexdigest()[:16]
        directory /= "patched-" + digest
    return directory


def install(tools, pin, prebuilt=None, vcvars=None):
    tools = Path(tools).resolve()
    directory = artifact_directory(tools, pin)
    build_directory = tools / "cp2" / pin["revision"]
    binary = directory / ("cp2.exe" if os.name == "nt" else "cp2")
    if version(binary, pin):
        return {"path": str(binary), "status": "present"}
    directory.mkdir(parents=True, exist_ok=True)
    with install_lock(build_directory / "install.lock"):
        if version(binary, pin):
            return {"path": str(binary), "status": "present"}
        if prebuilt:
            source = Path(prebuilt)
        else:
            cached = build_directory / "source/target/release" / binary.name
            source = cached if version(cached, pin) else compile_tool(tools, build_directory, pin, vcvars)
        if not version(source, pin):
            raise ValueError("cp2 build does not match the pinned version/fingerprint")
        with tempfile.TemporaryDirectory(dir=directory) as temporary:
            staged = Path(temporary) / binary.name
            shutil.copy2(source, staged)
            staged.chmod(0o755)
            staged.replace(binary)
        (directory / "installed.json").write_text(json.dumps({
            "revision": pin["revision"], "sha256": hashlib.sha256(binary.read_bytes()).hexdigest()
        }, indent=2) + "\n")
    return {"path": str(binary), "status": "installed"}
