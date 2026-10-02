#include "archivecodecimage.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#ifdef _WIN32
#include "Common/StringConvert.h"
#include "Windows/FileIO.h"
#endif

namespace {
QString DirectoryIdentity(const QString &source) {
#ifdef _WIN32
    class CDirectoryFile final : public NWindows::NFile::NIO::CFileBase {
      public:
        bool Open(CFSTR path) {
            // Follow directory junctions to identify their immutable snapshot target.
            return Create(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
        }
    };
    CDirectoryFile handle;
    BY_HANDLE_FILE_INFORMATION info{};
    const auto path = QDir::toNativeSeparators(source).toStdWString();
    if (!handle.Open(us2fs(path.c_str())) || !handle.GetFileInformation(&info))
        return {};
    return QString("%1:%2:%3")
        .arg(info.dwVolumeSerialNumber)
        .arg(info.nFileIndexHigh)
        .arg(info.nFileIndexLow);
#else
    return QFileInfo(source).canonicalFilePath();
#endif
}
} // namespace

bool ArchiveCodecImage::copyFile(const QString &source, const QString &relative) {
    const QString destination = directory_.filePath(relative);
    const QFileInfo info(source);
    if (info.isSymLink() && !info.exists()) {
        // Keep the original loader's nonfatal broken-link diagnostic.
        if (!QFile::link(info.symLinkTarget(), destination))
            return false;
    } else if (!QFile::copy(source, destination))
        return false;
    origins_.insert(QDir::cleanPath(destination), info.absoluteFilePath());
    return true;
}

bool ArchiveCodecImage::copyFolder(const QString &source, const QString &relative,
                                   QSet<QString> ancestors) {
    const QString canonical = DirectoryIdentity(source);
    if (canonical.isEmpty() || ancestors.contains(canonical) || ancestors.size() >= 128)
        return false; // Refuse cycles rather than retain mutable external directory links.
    ancestors.insert(canonical);
    if (!QDir(directory_.path()).mkpath(relative))
        return false;
    for (const QFileInfo &entry : QDir(source).entryInfoList(
             QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
        const QString destination = relative + '/' + entry.fileName();
        if (entry.isDir()) {
            if (!copyFolder(entry.absoluteFilePath(), destination, ancestors))
                return false;
        } else if (!copyFile(entry.absoluteFilePath(), destination))
            return false;
    }
    return true;
}

bool ArchiveCodecImage::prepare(const QString &module, const QString &engine) {
    if (!directory_.isValid())
        return false;
    const QFileInfo main(module);
    if ((main.exists() || main.isSymLink()) && !copyFile(module, main.fileName()))
        return false;
    module_ = directory_.filePath(main.fileName());
    origins_.insert(QDir::cleanPath(module_), main.absoluteFilePath());
    const QDir source(main.absolutePath());
    for (const QString &folder : {"Codecs", "Formats"}) {
        const QDir plugins(source.filePath(folder));
        if (!plugins.exists())
            continue;
        // Source enumeration still skips directories; loaded plugins can read
        // their own nested assets, which must belong to the same immutable image.
        if (!copyFolder(plugins.absolutePath(), folder))
            return false;
    }
    if (!engine.isEmpty()) {
        const QString executable = QFileInfo(engine).isFile()
                                       ? QFileInfo(engine).absoluteFilePath()
                                       : QStandardPaths::findExecutable(engine);
        const QFileInfo program(executable);
        if (executable.isEmpty() || !copyFile(executable, program.fileName()))
            return false;
        engine_ = directory_.filePath(program.fileName());
    }
    return true;
}

QString ArchiveCodecImage::originalPath(const QString &imagePath) const {
    return origins_.value(QDir::cleanPath(imagePath), imagePath);
}
