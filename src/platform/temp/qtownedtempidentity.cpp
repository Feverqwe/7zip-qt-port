#include "platform/temp/qtownedtempidentity.h"
#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#include <QFile>
#include <QFileInfo>
#ifdef _WIN32
namespace {
class CIdentityFile final : public NWindows::NFile::NIO::CFileBase {
  public:
    bool Open(CFSTR path) {
        return Create(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT);
    }
};
} // namespace
#else
#include <sys/stat.h>
#ifdef __linux__
#include <QScopeGuard>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#endif

QString QtOwnedTempIdentity(const QString &path) {
    const QFileInfo file(path);
    if (file.isSymLink() || file.isJunction())
        return {};
#ifdef _WIN32
    // The same native handle information used by source FSFolder::ReadFileInfo.
    BY_HANDLE_FILE_INFORMATION info = {};
    const auto name = path.toStdWString();
    CIdentityFile handle;
    if (!handle.Open(us2fs(name.c_str())) || !handle.GetFileInformation(&info) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
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
    QString identity = QString("%1:%2:%3")
                           .arg(qulonglong(info.st_dev))
                           .arg(qulonglong(info.st_ino))
                           .arg(S_ISDIR(info.st_mode));
#ifdef __linux__
    // Linux can immediately reuse an unlinked inode, including at the same
    // source-generated temp path. Birth time distinguishes that new object
    // from a retained edit without changing when its contents are updated.
    struct statx extended = {};
    if (::statx(AT_FDCWD, QFile::encodeName(path).constData(), AT_SYMLINK_NOFOLLOW,
                STATX_INO | STATX_BTIME, &extended) == 0 &&
        (extended.stx_mask & STATX_BTIME)) {
        if (extended.stx_ino != qulonglong(info.st_ino))
            return {};
        identity +=
            QString(":%1:%2").arg(extended.stx_btime.tv_sec).arg(extended.stx_btime.tv_nsec);
    }
    // ext4 can reuse both an inode and its coarse birth timestamp immediately.
    // Its generation distinguishes the replacement without changing on writes.
    const int descriptor =
        ::open(QFile::encodeName(path).constData(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (descriptor >= 0) {
        const auto close = qScopeGuard([descriptor] { ::close(descriptor); });
        struct stat opened;
        if (::fstat(descriptor, &opened) != 0 || opened.st_dev != info.st_dev ||
            opened.st_ino != info.st_ino)
            return {};
        long generation = 0;
        if (::ioctl(descriptor, FS_IOC_GETVERSION, &generation) == 0)
            identity += QString(":g%1").arg(qulonglong(static_cast<unsigned long>(generation)));
    }
#endif
    return identity;
#endif
}
