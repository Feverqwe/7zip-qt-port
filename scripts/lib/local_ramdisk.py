"""Create and validate the project's RAM workspace."""

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
import shutil
import subprocess
import sys
from functools import lru_cache

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
    record = json.dumps({"mount": str(MOUNT), "uuid": info["VolumeUUID"]}, indent=2) + "\n"
    if destination.is_file() and destination.read_text() == record:
        return info
    temporary = destination.with_suffix(f".{os.getpid()}.tmp")
    temporary.write_text(record)
    temporary.replace(destination)
    return info


def start(root):
    if platform.system() != "Darwin":
        if __package__:
            from .ramdisk_platforms import ensure
        else:
            from ramdisk_platforms import ensure
        return ensure(root)
    if os.environ.get("SEVENZIP_RAM_MOUNT") and Path(os.environ["SEVENZIP_RAM_MOUNT"]) != MOUNT:
        raise ValueError("Unexpected macOS RAM mount point")
    with configuration_lock(root):
        return create_or_reuse(root)


@lru_cache(maxsize=16)
def volume(root):
    info = start(root)
    return MOUNT if platform.system() == "Darwin" else Path(info)


@lru_cache(maxsize=16)
def scratch(root):
    root = Path(root).resolve()
    mount = volume(root)
    identity = hashlib.sha256(str(root).encode()).hexdigest()[:16]
    destination = root / "b" if root.is_relative_to(mount) else mount / "w" / identity
    destination.mkdir(parents=True, exist_ok=True)
    if shutil.disk_usage(mount).free < 256 * 1024**2:
        raise ValueError("RAM workspace has less than 256 MiB free; remove completed RAM outputs or explicitly set SEVENZIP_RAMDISK=OFF")
    return destination


def environment(root):
    if os.environ.get("SEVENZIP_RAMDISK") == "OFF":
        return {}
    temporary, cache = scratch(root) / "tmp", scratch(root) / "ccache-tmp"
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
    if platform.system() != "Darwin":
        if __package__:
            from .ramdisk_platforms import capacity_gib, compression_enabled, ensure
        else:
            from ramdisk_platforms import capacity_gib, compression_enabled, ensure
        mount = Path(ensure(args.root, create=False))
        mode = "compressed" if compression_enabled() else "uncompressed"
        print(f"{mount}: {capacity_gib()} GiB RAM ({mode}), {shutil.disk_usage(mount).free / 1024**3:.2f} GiB free")
        return
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
