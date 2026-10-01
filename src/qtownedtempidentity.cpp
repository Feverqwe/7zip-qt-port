#include "qtownedtempidentity.h"
#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#include <QFile>
#include <QFileInfo>
#ifndef _WIN32
#include <sys/stat.h>
#endif

QString QtOwnedTempIdentity(const QString &path) {
    const QFileInfo file(path);
    if (file.isSymLink() || file.isJunction())
        return {};
#ifdef _WIN32
    // The same native handle information used by source FSFolder::ReadFileInfo.
    BY_HANDLE_FILE_INFORMATION info = {};
    const auto name = path.toStdWString();
    NWindows::NFile::NIO::CFileBase handle;
    if (!handle.Create(us2fs(name.c_str()), 0,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT) ||
        !handle.GetFileInformation(&info) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        return {};
    return QString("%1:%2:%3:%4")
        .arg(info.dwVolumeSerialNumber)
        .arg(info.nFileIndexHigh)
        .arg(info.nFileIndexLow)
        .arg((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
    struct stat info;
    if (::lstat(QFile::encodeName(path).constData(), &info) != 0 ||
        (!S_ISREG(info.st_mode) && !S_ISDIR(info.st_mode)))
        return {};
    return QString("%1:%2:%3")
        .arg(qulonglong(info.st_dev))
        .arg(qulonglong(info.st_ino))
        .arg(S_ISDIR(info.st_mode));
#endif
}
