"""Native Linux tmpfs and Windows ImDisk provisioning; no disk fallback."""

import ctypes
import json
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess

CAPACITY = 4 * 1024**3


def linux(mount, create):
    if not mount.is_mount():
        if not create:
            raise ValueError("RAM workspace is not mounted")
        if mount.is_symlink() or mount.exists() and any(mount.iterdir()):
            raise ValueError("Refusing an occupied RAM mount point")
        mount.mkdir(parents=True, exist_ok=True)
        options = f"size=4G,nodev,nosuid,mode=700,uid={os.getuid()},gid={os.getgid()}"
        subprocess.run(["sudo", "-n", "mount", "-t", "tmpfs", "-o", options,
                        "sevenzip-ram", str(mount)], check=True, stdout=subprocess.DEVNULL)
    record = json.loads(subprocess.check_output(
        ["findmnt", "-J", "-b", "-n", "-o", "TARGET,FSTYPE,SIZE", "--mountpoint", str(mount)], text=True))
    fs = record["filesystems"][0]
    if fs["fstype"] != "tmpfs" or int(fs["size"]) != CAPACITY:
        raise ValueError("RAM workspace must be a 4 GiB tmpfs mount")


def windows(mount, create):
    drive = mount.drive
    if len(drive) != 2 or drive[1] != ":" or str(mount).rstrip("\\/") != drive:
        raise ValueError("Windows RAM mount must be a drive root")
    if not mount.exists():
        if not create:
            raise ValueError("RAM workspace is not mounted")
        executable = shutil.which("imdisk")
        if not executable:
            raise ValueError("Install ImDisk and run the RAM workspace setup as administrator")
        subprocess.run([executable, "-a", "-t", "vm", "-s", "4G", "-m", drive,
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
        if size != CAPACITY or flags & 0xF00 != 0x200:
            raise ValueError("Expected a 4 GiB virtual-memory ImDisk drive")
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


def ensure(root, create=True):
    if __package__:
        from .build_paths import project_identity, workspace_root
        from .cp2_install import install_lock
    else:
        from build_paths import project_identity, workspace_root
        from cp2_install import install_lock
    workspace = workspace_root(root)
    workspace.mkdir(parents=True, exist_ok=True)
    mount = Path(os.environ.get("SEVENZIP_RAM_MOUNT", "R:/" if os.name == "nt" else workspace / "ram"))
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
    return mount
