// Adapted from 7-Zip CPP/7zip/UI/FileManager/FSFolder.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Only the supported filesystem mutation methods are ported here. QString and
// QFile/QDir replace filesystem strings/services, callbacks replace IProgress,
// and bool/ErrorMessage replace HRESULT/GetLastError.
#include "FSFolder.h"
#include "CPP/Windows/FileDir.h"
#include <QDir>
#include <QFile>

namespace NFsFolder {
CFSFolder::CFSFolder(QString path) : _path(QDir::cleanPath(std::move(path))) {
    if (!_path.endsWith('/'))
        _path += '/';
}

void CFSFolder::AddPath(const QString &path) {
    const QFileInfo info(path);
    CDirItem item;
    item.Name = info.fileName();
    item.Directory = info.isDir();
    const QString relative = QDir(_path).relativeFilePath(info.absolutePath());
    if (relative != ".")
        item.Parent = Folders.Add(relative + '/');
    Files.Add(item);
}

QString CFSFolder::GetRelPath(const CDirItem &item) const {
    if (item.Parent < 0)
        return item.Name;
    return Folders[item.Parent] + item.Name;
}

void CFSFolder::GetAbsPath(const QString &name, QString &absPath) const {
    absPath.clear();
    if (!QDir::isAbsolutePath(name))
        absPath += _path;
    absPath += name;
}

bool CFSFolder::CreateFolder(const QString &name) {
    QString absPath;
    GetAbsPath(name, absPath);
    if (QDir().mkdir(absPath))
        return true;
    const QFileInfo info(absPath);
    if (!info.exists() && !info.isSymLink())
        if (QDir().mkpath(absPath))
            return true;
    ErrorMessage = QStringLiteral("Не удалось создать папку: %1").arg(absPath);
    return false;
}

bool CFSFolder::CreateFile(const QString &name) {
    QString absPath;
    GetAbsPath(name, absPath);
    QFile outFile(absPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        ErrorMessage = QStringLiteral("Не удалось создать файл: %1").arg(absPath);
        return false;
    }
    return true;
}

bool CFSFolder::Rename(UInt32 index, const QString &newName) {
    if (index >= Files.Size())
        return false;
    const CDirItem &fi = Files[index];
    QString fullPrefix = _path;
    if (fi.Parent >= 0)
        fullPrefix += Folders[fi.Parent];
    // Qt rename never replaces a target, including a dangling symlink.
    const QString target = fullPrefix + newName;
    if (QFileInfo(target).isSymLink() || !QDir().rename(fullPrefix + fi.Name, target)) {
        ErrorMessage = QStringLiteral("Не удалось переименовать объект: %1").arg(target);
        return false;
    }
    return true;
}

bool CFSFolder::Delete(const UInt32 *indices, UInt32 numItems) {
    if (SetTotal && !SetTotal(numItems))
        return false;
    for (UInt32 i = 0; i < numItems; i++) {
        const UInt32 index = indices[i];
        if (index >= Files.Size())
            return false;
        bool result = true;
        {
            const CDirItem &fi = Files[index];
            const QString fullPath = _path + GetRelPath(fi);
            if (CheckBreak && !CheckBreak(fullPath))
                return false;
            // Keep the existing panel policy for a directly selected link.
            // Links found inside a directory are unlinked without traversal.
            if (QFileInfo(fullPath).isSymLink())
                result = false;
            else if (fi.IsDir())
                result = NWindows::NFile::NDir::RemoveDirWithSubItems(fullPath, CheckBreak);
            else
                result = NWindows::NFile::NDir::DeleteFileAlways(fullPath);
            if (!result) {
                ErrorMessage = QStringLiteral("Не удалось удалить объект: %1").arg(fullPath);
                return false;
            }
        }
        const quint64 completed = i;
        if (SetCompleted && !SetCompleted(completed))
            return false;
    }
    return true;
}
} // namespace NFsFolder
