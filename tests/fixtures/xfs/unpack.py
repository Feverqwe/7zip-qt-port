#!/usr/bin/env python3
"""Expand checked-in XFS fixtures into disposable sparse regular files."""
import pathlib
import sys
import zlib
import os


def sparse(file):
    if os.name != "nt":
        return
    import ctypes
    import msvcrt
    from ctypes import wintypes
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    ioctl = kernel.DeviceIoControl
    ioctl.argtypes = [wintypes.HANDLE, wintypes.DWORD, ctypes.c_void_p, wintypes.DWORD,
                      ctypes.c_void_p, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), ctypes.c_void_p]
    ioctl.restype = wintypes.BOOL
    returned = wintypes.DWORD()
    if not ioctl(msvcrt.get_osfhandle(file.fileno()), 0x900C4, None, 0,
                 None, 0, ctypes.byref(returned), None):
        raise ctypes.WinError(ctypes.get_last_error())

source, output = map(pathlib.Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
zero = bytes(1024 * 1024)
for version in (4, 5):
    decoder = zlib.decompressobj()
    length = 0
    with (source / f"v{version}.zlib").open("rb") as packed, (output / f"v{version}.xfs").open("wb") as raw:
        sparse(raw)
        while chunk := packed.read(16384):
            while chunk:
                data = decoder.decompress(chunk, len(zero))
                chunk = decoder.unconsumed_tail
                length += len(data)
                if data == zero[:len(data)]:
                    raw.seek(len(data), 1)
                else:
                    raw.write(data)
        if not decoder.eof:
            raise ValueError("Truncated XFS fixture")
        raw.truncate(length)
