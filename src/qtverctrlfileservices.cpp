#include "qtverctrlfileservices.h"
#include "CPP/Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/TimeUtils.h"

#ifndef _WIN32
bool QtVerCtrlInFile::GetFileInformation(QtVerCtrlFileInfo *info) const {
    struct stat native;
    if (my_fstat(&native) != 0)
        return false;
    const UInt64 size = UInt64(native.st_size);
    info->nFileSizeLow = DWORD(size);
    info->nFileSizeHigh = DWORD(size >> 32);
    info->dwFileAttributes =
        NWindows::NFile::NFind::Get_WinAttribPosix_From_PosixMode(UInt32(native.st_mode));
#ifdef __APPLE__
    FiTime_To_FILETIME(native.st_birthtimespec, info->ftCreationTime);
#else
    FiTime_To_FILETIME(ST_CTIME(native), info->ftCreationTime);
#endif
    FiTime_To_FILETIME(ST_ATIME(native), info->ftLastAccessTime);
    FiTime_To_FILETIME(ST_MTIME(native), info->ftLastWriteTime);
    return true;
}

bool QtVerCtrlOutFile::Create_ALWAYS_or_NEW(CFSTR path, bool createAlways) {
    return createAlways ? Create_ALWAYS(path) : Create_NEW(path);
}
bool QtVerCtrlOutFile::Write(const void *data, UInt32 size, UInt32 &processedSize) {
    size_t processed = 0;
    const ssize_t result = write_full(data, size, processed);
    processedSize = UInt32(processed);
    return result != -1;
}
bool QtVerCtrlOutFile::SetTime(const FILETIME *creation, const FILETIME *access,
                               const FILETIME *modified) {
    CFiTime c, a, m;
    if ((creation && !FILETIME_To_timespec(*creation, c)) ||
        (access && !FILETIME_To_timespec(*access, a)) ||
        (modified && !FILETIME_To_timespec(*modified, m)))
        return false;
    return NWindows::NFile::NIO::COutFile::SetTime(creation ? &c : nullptr, access ? &a : nullptr,
                                                   modified ? &m : nullptr);
}
bool QtVerCtrlOutFile::SetMTime(const FILETIME *modified) {
    return SetTime(nullptr, nullptr, modified);
}
#endif

bool QtVerCtrlSetFileAttrib(CFSTR path, DWORD attributes) {
#ifdef _WIN32
    return NWindows::NFile::NDir::SetFileAttrib(path, attributes);
#else
    // Preserve source POSIX permission bits; the Win32 read-only toggle maps
    // to no write permissions, or restoring owner write permission.
    mode_t mode = mode_t(attributes >> 16);
    if (attributes & FILE_ATTRIBUTE_READONLY)
        mode &= ~mode_t(0222);
    else
        mode |= S_IWUSR;
    return chmod(path, mode) == 0;
#endif
}
