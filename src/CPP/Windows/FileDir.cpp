// Adapted from 7-Zip CPP/Windows/FileDir.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt adapts deletion services; GetCurrentDir retains upstream native bodies.
#include "Windows/StdAfx.h"

#include "FileDir.h"
#ifndef _WIN32
#include <limits.h>
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

bool DeleteFileAlways(const QString &path) {
#ifdef Q_OS_WIN
    const QFileInfo fi(path);
    if (!fi.isSymLink() && fi.isFile() && !(fi.permissions() & QFileDevice::WriteUser)) {
        if (!QFile::setPermissions(path, fi.permissions() | QFileDevice::WriteUser))
            return false;
    }
#endif
    return QFile::remove(path);
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
