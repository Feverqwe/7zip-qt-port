"""Native compressed RAM workspaces; no disk fallback."""

import ctypes
import json
import os
from pathlib import Path
import platform
import re
import shutil
import struct
import subprocess


def capacity_gib(default=4):
    value = os.environ.get("SEVENZIP_RAM_SIZE_GIB", str(default))
    if not value.isdecimal() or int(value) < 1:
        raise ValueError("SEVENZIP_RAM_SIZE_GIB must be a positive integer")
    return int(value)


def compression_enabled():
    value = os.environ.get("SEVENZIP_RAM_COMPRESSION", "ON").upper()
    if value not in ("ON", "OFF"):
        raise ValueError("SEVENZIP_RAM_COMPRESSION must be ON or OFF")
    return value == "ON"


def privileged(*args):
    return subprocess.check_output(["sudo", "-n", *map(str, args)], text=True).strip()


def create_zram(mount, gib):
    privileged("modprobe", "zram")
    # Allocate a new device, rather than formatting an existing idle device.
    number = privileged("cat", "/sys/class/zram-control/hot_add")
    if not number.isdecimal():
        raise ValueError("Invalid new zram device identifier")
    device = f"/dev/zram{number}"
    mounted = False
    try:
        privileged("zramctl", "--algorithm", "lz4", "--size", f"{gib}G", device)
        privileged("mkfs.ext4", "-q", "-m", "0", "-O", "^has_journal", "-E", "nodiscard", device)
        privileged("mount", "-t", "ext4", "-o", "nodev,nosuid,noatime,discard", device, mount)
        mounted = True
        privileged("rmdir", mount / "lost+found")
        privileged("chown", f"{os.getuid()}:{os.getgid()}", mount)
        privileged("chmod", "700", mount)
    except BaseException:
        if mounted:
            privileged("umount", mount)
        subprocess.run(["sudo", "-n", "tee", "/sys/class/zram-control/hot_remove"],
                       input=number, text=True, check=True, stdout=subprocess.DEVNULL)
        raise


def linux(mount, create):
    gib = capacity_gib()
    compressed = compression_enabled()
    if not mount.is_mount():
        if not create:
            raise ValueError("RAM workspace is not mounted")
        if mount.is_symlink() or mount.exists() and any(mount.iterdir()):
            raise ValueError("Refusing an occupied RAM mount point")
        mount.mkdir(parents=True, exist_ok=True)
        if compressed:
            create_zram(mount, gib)
        else:
            options = f"size={gib}G,nodev,nosuid,mode=700,uid={os.getuid()},gid={os.getgid()}"
            privileged("mount", "-t", "tmpfs", "-o", options, "sevenzip-ram", mount)
    record = json.loads(subprocess.check_output(
        ["findmnt", "-J", "-b", "-n", "-o", "TARGET,SOURCE,FSTYPE,SIZE", "--mountpoint", str(mount)], text=True))
    fs = record["filesystems"][0]
    if compressed:
        if fs["fstype"] != "ext4" or not re.fullmatch(r"/dev/zram[0-9]+", fs["source"]):
            raise ValueError("Compressed RAM workspace requires ext4 on zram; existing mounts are never replaced")
        settings = Path("/sys/class/block") / Path(fs["source"]).name
        if (int((settings / "disksize").read_text()) != gib * 1024**3
                or "[lz4]" not in (settings / "comp_algorithm").read_text()
                or (settings / "backing_dev").read_text().strip() != "none"):
            raise ValueError(f"Expected {gib} GiB LZ4 zram without disk writeback")
    elif fs["fstype"] != "tmpfs" or int(fs["size"]) != gib * 1024**3:
        raise ValueError(f"RAM workspace must be a {gib} GiB tmpfs mount")


def windows(mount, create):
    gib = capacity_gib()
    drive = mount.drive
    if len(drive) != 2 or drive[1] != ":" or str(mount).rstrip("\\/") != drive:
        raise ValueError("Windows RAM mount must be a drive root")
    if not mount.exists():
        if not create:
            raise ValueError("RAM workspace is not mounted")
        executable = shutil.which("imdisk")
        if not executable:
            raise ValueError("Install ImDisk and run the RAM workspace setup as administrator")
        subprocess.run([executable, "-a", "-t", "vm", "-s", f"{gib}G", "-m", drive,
                        "-p", "/fs:ntfs /q /y /v:SevenzipRAM"], check=True, stdout=subprocess.DEVNULL)
    # Query the driver's public IOCTL rather than trusting a drive label.
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32,
                                  ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
    kernel.CreateFileW.restype = ctypes.c_void_p
    kernel.DeviceIoControl.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_void_p,
                                      ctypes.c_uint32, ctypes.c_void_p, ctypes.c_uint32,
                                      ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p]
    kernel.CloseHandle.argtypes = [ctypes.c_void_p]
    handle = kernel.CreateFileW("\\\\.\\" + drive, 0, 3, None, 3, 0, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        data = ctypes.create_string_buffer(65536)
        returned = ctypes.c_uint32()
        # IOCTL_IMDISK_QUERY_DEVICE; IMDISK_CREATE_DATA's public ABI.
        if not kernel.DeviceIoControl(handle, 0x83722008, None, 0, data, len(data), ctypes.byref(returned), None):
            raise ValueError("RAM drive is not an ImDisk device")
        if returned.value < 48:
            raise ValueError("Invalid RAM drive geometry")
        size, flags = struct.unpack_from("<q", data.raw, 8)[0], struct.unpack_from("<I", data.raw, 40)[0]
        if size != gib * 1024**3 or flags & 0xF00 != 0x200:
            raise ValueError(f"Expected a {gib} GiB virtual-memory ImDisk drive")
    finally:
        kernel.CloseHandle(handle)
    filesystem = ctypes.create_unicode_buffer(32)
    kernel.GetVolumeInformationW.argtypes = [ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32,
                                            ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32),
                                            ctypes.POINTER(ctypes.c_uint32), ctypes.c_wchar_p, ctypes.c_uint32]
    if not kernel.GetVolumeInformationW(str(mount), None, 0, None, None, None, filesystem, len(filesystem)):
        raise ctypes.WinError(ctypes.get_last_error())
    if filesystem.value != "NTFS":
        raise ValueError("RAM workspace requires NTFS timestamp precision")


def windows_compression(mount, create):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.GetFileAttributesW.argtypes = [ctypes.c_wchar_p]
    kernel.GetFileAttributesW.restype = ctypes.c_uint32
    attributes = kernel.GetFileAttributesW(str(mount))
    if attributes == 0xFFFFFFFF:
        raise ctypes.WinError(ctypes.get_last_error())
    expected = compression_enabled()
    if bool(attributes & 0x800) != expected:
        if not create:
            raise ValueError("RAM directory compression differs from SEVENZIP_RAM_COMPRESSION")
        subprocess.run(["compact.exe", "/C" if expected else "/U", f"/S:{mount}", "/A", "/Q"],
                       cwd=mount, check=True, stdout=subprocess.DEVNULL)
        attributes = kernel.GetFileAttributesW(str(mount))
        if attributes == 0xFFFFFFFF:
            raise ctypes.WinError(ctypes.get_last_error())
        if bool(attributes & 0x800) != expected:
            raise ValueError("Unable to set NTFS RAM directory compression")


def ensure(root, create=True):
    if __package__:
        from .build_paths import project_identity, workspace_root
        from .cp2_install import install_lock
    else:
        from build_paths import project_identity, workspace_root
        from cp2_install import install_lock
    workspace = workspace_root(root)
    workspace.mkdir(parents=True, exist_ok=True)
    linux_name = "ram-zram" if compression_enabled() else "ram"
    mount = Path(os.environ.get("SEVENZIP_RAM_MOUNT", "R:/" if os.name == "nt" else workspace / linux_name))
    if mount.is_symlink():
        raise ValueError("RAM mount point must not be a symlink")
    with install_lock(workspace / ".ramdisk.lock"):
        (windows if platform.system() == "Windows" else linux)(mount, create)
        marker = mount / ".sevenzip-ram.json"
        identity = project_identity(root)
        if marker.is_symlink():
            raise ValueError("Invalid RAM ownership marker")
        if not marker.exists():
            # os.listdir also handles an empty ImDisk root on Python 3.13,
            # where pathlib's scandir-based iterator raises WinError 2.
            if not create or any(name not in ("System Volume Information", "$RECYCLE.BIN") for name in os.listdir(mount)):
                raise ValueError("RAM workspace is not owned by this project")
            marker.write_text(json.dumps(identity) + "\n")
        if json.loads(marker.read_text()) != identity:
            raise ValueError("RAM workspace belongs to another project")
        if platform.system() == "Windows":
            windows_compression(mount, create)
    return mount
