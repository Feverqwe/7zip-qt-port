"""Source version time for build snapshots, including backdated edits/checkouts."""

import os


def version_time_ns(path, info):
    changed = info.st_ctime_ns
    if os.name == "nt":
        # Python's Windows st_ctime is creation time. NTFS ChangeTime tracks
        # edits even when an editor or checkout restores an older mtime.
        import ctypes
        from ctypes import wintypes

        class BasicInfo(ctypes.Structure):
            _fields_ = [(name, ctypes.c_longlong) for name in
                        ("creation", "access", "write", "change")] + [("attributes", wintypes.DWORD)]

        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                      wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
        kernel.CreateFileW.restype = wintypes.HANDLE
        kernel.GetFileInformationByHandleEx.argtypes = [wintypes.HANDLE, ctypes.c_int,
                                                       wintypes.LPVOID, wintypes.DWORD]
        kernel.GetFileInformationByHandleEx.restype = wintypes.BOOL
        kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        kernel.CloseHandle.restype = wintypes.BOOL
        handle = kernel.CreateFileW(str(path), 0x80, 7, None, 3, 0, None)
        if handle == wintypes.HANDLE(-1).value:
            raise ctypes.WinError(ctypes.get_last_error())
        try:
            basic = BasicInfo()
            if not kernel.GetFileInformationByHandleEx(handle, 0, ctypes.byref(basic), ctypes.sizeof(basic)):
                raise ctypes.WinError(ctypes.get_last_error())
            changed = basic.change * 100 - 11644473600000000000
        finally:
            kernel.CloseHandle(handle)
    return max(info.st_mtime_ns, changed)
