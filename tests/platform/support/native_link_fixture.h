#pragma once
#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#include "fileops/native/FileDir.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include "Windows/MemoryLock.h"
#else
#include <sys/stat.h>
#endif

inline FString NativeFixturePath(const QString &path) {
    return us2fs(QDir::toNativeSeparators(path).toStdWString().c_str());
}

inline bool CreateNativeSymbolicLink(const QString &path, const QString &target) {
#ifdef Q_OS_WIN
    NWindows::NSecurity::EnablePrivilege_SymLink();
    const QString resolved = QDir::isAbsolutePath(target)
                                 ? target
                                 : QDir(QFileInfo(path).absolutePath()).filePath(target);
    CByteBuffer data;
    NWindows::NFile::FillLinkData(data, QDir::toNativeSeparators(target).toStdWString().c_str(),
                                  true, false);
    return NWindows::NFile::NIO::SetReparseData(
        NativeFixturePath(path), QFileInfo(resolved).isDir(), data, DWORD(data.Size()));
#else
    return NWindows::NFile::NIO::SetSymLink_UString(NativeFixturePath(path),
                                                    UString(target.toStdWString().c_str()));
#endif
}

inline bool NativeFilesShareIdentity(const QString &first, const QString &second) {
#ifdef Q_OS_WIN
    BY_HANDLE_FILE_INFORMATION a{}, b{};
    if (!NWindows::NFile::NIO::CFileBase::GetFileInformation(NativeFixturePath(first), &a) ||
        !NWindows::NFile::NIO::CFileBase::GetFileInformation(NativeFixturePath(second), &b))
        return false;
    return a.dwVolumeSerialNumber == b.dwVolumeSerialNumber &&
           a.nFileIndexHigh == b.nFileIndexHigh && a.nFileIndexLow == b.nFileIndexLow;
#else
    struct stat a{}, b{};
    return ::stat(QFile::encodeName(first).constData(), &a) == 0 &&
           ::stat(QFile::encodeName(second).constData(), &b) == 0 && a.st_dev == b.st_dev &&
           a.st_ino == b.st_ino;
#endif
}
