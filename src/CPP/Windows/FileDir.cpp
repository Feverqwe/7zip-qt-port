// Adapted from 7-Zip CPP/Windows/FileDir.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Native current-directory and single-item cleanup bodies are upstream;
// Qt supplies path/cancellation boundaries for the remaining recursive adapter.
#include "Windows/StdAfx.h"

#include "Common/StringConvert.h"
#include "FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#ifndef _WIN32
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#else
#ifndef _UNICODE
extern bool g_IsNT;
#endif
#endif
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace NWindows::NFile::NDir {
using namespace NName;
bool GetCurrentDir(FString &path) {
#ifdef _WIN32
    const unsigned kBufSize = MAX_PATH + 16;
    path.Empty();
#ifndef _UNICODE
    if (!g_IsNT) {
        TCHAR s[kBufSize + 1];
        s[0] = 0;
        const DWORD len = ::GetCurrentDirectory(kBufSize, s);
        if (len == 0 || len >= kBufSize)
            return false;
        s[kBufSize] = 0;
        path = fas2fs(s);
        return true;
    } else
#endif
    {
        DWORD len;
        {
            WCHAR s[kBufSize + 1];
            s[0] = 0;
            len = ::GetCurrentDirectoryW(kBufSize, s);
            if (len == 0)
                return false;
            if (len < kBufSize) {
                s[kBufSize] = 0;
                path = us2fs(s);
                return true;
            }
        }
        UString temp;
        const DWORD len2 = ::GetCurrentDirectoryW(len, temp.GetBuf(len));
        if (len2 == 0)
            return false;
        temp.ReleaseBuf_CalcLen(len);
        if (temp.Len() != len2 || len - 1 != len2)
            return false;
        path = us2fs(temp);
        return true;
    }
#else
    path.Empty();
    char s[PATH_MAX + 1];
    char *res = getcwd(s, PATH_MAX);
    if (res) {
        path = fas2fs(s);
        return true;
    }
    {
#if defined(__GLIBC__) || defined(__APPLE__)
        res = getcwd(NULL, 0);
        if (res) {
            path = fas2fs(res);
            ::free(res);
            return true;
        }
#endif
        return false;
    }
#endif
}

#ifdef _WIN32
bool SetFileAttrib(CFSTR path, DWORD attrib) {
    /* win10:
      if (attrib == 0), it sets (FILE_ATTRIBUTE_NORMAL) attribute
      (FILE_ATTRIBUTE_DIRECTORY and some another attributes are ignored for files).
    */
#ifndef _UNICODE
    if (!g_IsNT) {
        if (::SetFileAttributes(fs2fas(path), attrib))
            return true;
    } else
#endif
    {
        IF_USE_MAIN_PATH
        if (::SetFileAttributesW(fs2us(path), attrib))
            return true;
#ifdef Z7_LONG_PATH
        if (USE_SUPER_PATH) {
            UString superPath;
            if (GetSuperPath(path, superPath, USE_MAIN_PATH))
                return BOOLToBool(::SetFileAttributesW(superPath, attrib));
        }
#endif
    }
    return false;
}

bool RemoveDir(CFSTR path) {
#ifndef _UNICODE
    if (!g_IsNT) {
        if (::RemoveDirectory(fs2fas(path)))
            return true;
    } else
#endif
    {
        IF_USE_MAIN_PATH
        if (::RemoveDirectoryW(fs2us(path)))
            return true;
#ifdef Z7_LONG_PATH
        if (USE_SUPER_PATH) {
            UString superPath;
            if (GetSuperPath(path, superPath, USE_MAIN_PATH))
                return BOOLToBool(::RemoveDirectoryW(superPath));
        }
#endif
    }
    return false;
}

bool DeleteFileAlways(CFSTR path) {
    /* If alt stream, we also need to clear READ-ONLY attribute of main file before delete.
       SetFileAttrib("name:stream", ) changes attributes of main file. */
    {
        DWORD attrib = NFind::GetFileAttrib(path);
        if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
            (attrib & FILE_ATTRIBUTE_READONLY) != 0) {
            if (!SetFileAttrib(path, attrib & ~(DWORD)FILE_ATTRIBUTE_READONLY))
                return false;
        }
    }

#ifndef _UNICODE
    if (!g_IsNT) {
        if (::DeleteFile(fs2fas(path)))
            return true;
    } else
#endif
    {
        /* DeleteFile("name::$DATA") deletes all alt streams (same as delete DeleteFile("name")).
           Maybe it's better to open "name::$DATA" and clear data for unnamed stream? */
        IF_USE_MAIN_PATH
        if (::DeleteFileW(fs2us(path)))
            return true;
#ifdef Z7_LONG_PATH
        if (USE_SUPER_PATH) {
            UString superPath;
            if (GetSuperPath(path, superPath, USE_MAIN_PATH))
                return BOOLToBool(::DeleteFileW(superPath));
        }
#endif
    }
    return false;
}
#else
bool RemoveDir(CFSTR path) { return (rmdir(path) == 0); }
bool DeleteFileAlways(CFSTR path) { return (remove(path) == 0); }
#endif

bool DeleteFileAlways(const QString &path) {
    const auto native = QDir::toNativeSeparators(path).toStdWString();
    return DeleteFileAlways(us2fs(native.c_str()));
}

bool RemoveDirWithSubItems(const QString &path,
                           const std::function<bool(const QString &)> &checkBreak) {
    if (checkBreak && !checkBreak(path))
        return false;
    bool needRemoveSubItems = true;
    {
        const QFileInfo fi(path);
        if (!fi.exists() && !fi.isSymLink())
            return false;
        if (!fi.isDir())
            return false;
        if (fi.isSymLink())
            needRemoveSubItems = false;
    }

    if (needRemoveSubItems) {
        QString s(path);
        if (!s.endsWith('/'))
            s += '/';
        const qsizetype prefixSize = s.size();
        // QDir has no Next(error) result. Reject an unreadable root before
        // enumerating; rmdir still detects entries missed by enumeration.
        if (!QFileInfo(path).isReadable())
            return false;
        const auto entries = QDir(path).entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::NoSort);
        bool isError = false;
        for (const QFileInfo &fi : entries) {
            s.truncate(prefixSize);
            s += fi.fileName();
            if (checkBreak && !checkBreak(s))
                return false;
            if (fi.isDir()) {
                if (!RemoveDirWithSubItems(s, checkBreak))
                    isError = true;
            } else if (!DeleteFileAlways(s)) {
                isError = false;
            }
        }
        if (isError)
            return false;
    }

    // A Windows reparse point is removed itself, without visiting its target.
    // QFile removes a Qt symbolic link to a directory in the same way.
    if (QFileInfo(path).isSymLink())
        return QFile::remove(path);
#ifdef Q_OS_WIN
    const auto permissions = QFileInfo(path).permissions();
    if (!QFile::setPermissions(path, permissions | QFileDevice::WriteUser))
        return false;
#endif
    return QDir().rmdir(path);
}
} // namespace NWindows::NFile::NDir
