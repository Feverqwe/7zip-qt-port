"""Manage opt-in macOS RAM scratch space without relocating incremental builds."""

import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import platform
import plistlib
import re
import shlex
import subprocess
import sys

if __package__:
    from .project_paths import harness_root
else:
    from project_paths import harness_root


MOUNT = Path("/Volumes/SevenzipRAM")
SECTORS = 8388608


def configuration(root):
    return harness_root(root) / ".codex/local-ramdisk.json"


def ram_device(device):
    images = plistlib.loads(subprocess.check_output(["hdiutil", "info", "-plist"]))
    return any(image.get("image-path") == f"ram://{SECTORS}" and
               any(entity.get("dev-entry") == device
                   for entity in image.get("system-entities", []))
               for image in images.get("images", []))


def checked_volume(mount, expected_uuid=None):
    if not mount.is_mount():
        raise ValueError("RAM disk is not mounted; run ./scripts/ramdisk.sh start")
    info = plistlib.loads(subprocess.check_output(["diskutil", "info", "-plist", str(mount)]))
    if info.get("FilesystemType") != "apfs":
        raise ValueError("RAM scratch requires APFS for precise build timestamps")
    containers = plistlib.loads(subprocess.check_output(["diskutil", "apfs", "list", "-plist"]))
    container = next((item for item in containers.get("Containers", [])
                      if item.get("ContainerReference") == info.get("APFSContainerReference")), {})
    stores = container.get("PhysicalStores", [])
    devices = []
    for store in stores:
        backing = plistlib.loads(subprocess.check_output(
            ["diskutil", "info", "-plist", store["DeviceIdentifier"]]))
        devices.append("/dev/" + backing.get("ParentWholeDisk", backing["DeviceIdentifier"]))
    if len(devices) != 1 or not ram_device(devices[0]):
        raise ValueError("Scratch destination is not the expected 4 GiB RAM device")
    if expected_uuid and info.get("VolumeUUID") != expected_uuid:
        raise ValueError("RAM volume changed; run ./scripts/ramdisk.sh start to activate it")
    return info


@contextmanager
def configuration_lock(root):
    import fcntl
    settings = configuration(root)
    settings.parent.mkdir(parents=True, exist_ok=True)
    with settings.with_suffix(".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield


def create_or_reuse(root):
    if MOUNT.exists() and not MOUNT.is_mount():
        # An unmounted volume can leave an empty mount point. Never remove
        # files or a nonempty directory to make room for a RAM volume.
        if MOUNT.is_symlink() or not MOUNT.is_dir():
            raise ValueError("RAM mount point is occupied")
        MOUNT.rmdir()
    if not MOUNT.is_mount():
        result = subprocess.check_output(
            ["hdiutil", "attach", "-nomount", f"ram://{SECTORS}"], text=True)
        device = result.strip().split()[0]
        if not re.fullmatch(r"/dev/disk[0-9]+", device) or not ram_device(device):
            raise ValueError("Refusing to format an unverified device")
        try:
            subprocess.run(["diskutil", "eraseDisk", "APFS", MOUNT.name, device],
                           check=True, stdout=sys.stderr)
        except BaseException:
            subprocess.run(["diskutil", "eject", device], check=False, stdout=sys.stderr)
            raise
    info = checked_volume(MOUNT)
    destination = configuration(root)
    temporary = destination.with_suffix(f".{os.getpid()}.tmp")
    temporary.write_text(json.dumps({"mount": str(MOUNT), "uuid": info["VolumeUUID"]},
                                    indent=2) + "\n")
    temporary.replace(destination)
    return info


def start(root):
    if platform.system() != "Darwin":
        raise ValueError("Local RAM disk setup is supported on macOS only")
    with configuration_lock(root):
        return create_or_reuse(root)


def environment(root):
    if platform.system() != "Darwin" or os.environ.get("SEVENZIP_RAMDISK") == "OFF":
        return {}
    settings = configuration(root)
    if not settings.is_file():
        return {}
    with configuration_lock(root):
        configured = json.loads(settings.read_text())
        mount = Path(configured["mount"])
        if mount != MOUNT:
            raise ValueError("Unexpected local RAM mount point")
        if not mount.is_mount():
            create_or_reuse(root)
            configured = json.loads(settings.read_text())
        checked_volume(mount, configured["uuid"])
    identity = hashlib.sha256(str(root.resolve()).encode()).hexdigest()[:16]
    scratch = mount / "workspaces" / identity
    temporary, cache = scratch / "tmp", scratch / "ccache-tmp"
    for directory in (temporary, cache):
        directory.mkdir(parents=True, mode=0o700, exist_ok=True)
    return {"TMPDIR": str(temporary), "TMP": str(temporary), "TEMP": str(temporary),
            "CCACHE_TEMPDIR": str(cache)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("start", "status", "env"))
    parser.add_argument("--root", type=Path, required=True)
    args = parser.parse_args()
    if args.action == "env":
        for key, value in environment(args.root).items():
            print(f"export {key}={shlex.quote(value)}")
        return
    if args.action == "start":
        start(args.root)
    configured = json.loads(configuration(args.root).read_text())
    mount = Path(configured["mount"])
    checked_volume(mount, configured["uuid"])
    space = os.statvfs(mount)
    print(f"{mount}: 4 GiB RAM, {space.f_bavail * space.f_frsize / 1024**3:.2f} GiB free")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"ramdisk: {error}")
