// Adapted from 7-Zip CPP/7zip/UI/FileManager/FSFolderCopy.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// QFile replaces NIO files, a progress function replaces IProgress, and bool
// replaces HRESULT. ErrorFileIndex / ErrorMessage retain their upstream roles.
#include "FSFolder.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace NFsFolder {
bool CCopyStateIO::MyCopyFile(const QString &inPath, const QString &outPath,
                              QFileDevice::Permissions attrib) {
    ErrorFileIndex = -1;
    ErrorMessage.clear();
    CurrentSize = 0;
    {
        const qint64 kBufSize = 1 << 16;
        QByteArray buf(kBufSize, Qt::Uninitialized);
        QFile inFile(inPath);
        QFile outFile(outPath);
        if (!inFile.open(QIODevice::ReadOnly)) {
            ErrorFileIndex = 0;
            return true;
        }
        // The process adapter publishes staged files; a stage must never overwrite
        // another file. This replaces upstream Create_ALWAYS with Qt NewOnly.
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            ErrorFileIndex = 1;
            return true;
        }
        for (;;) {
            const qint64 num = inFile.read(buf.data(), kBufSize);
            if (num < 0) {
                ErrorFileIndex = 0;
                return true;
            }
            if (num == 0)
                break;
            const qint64 written = outFile.write(buf.constData(), num);
            if (written < 0) {
                ErrorFileIndex = 1;
                return true;
            }
            if (written != num) {
                ErrorMessage = "Write error";
                return true;
            }
            CurrentSize += quint64(num);
            if (Progress) {
                const quint64 completed = StartPos + CurrentSize;
                if (!Progress(completed))
                    return false;
            }
        }
        // QFile buffers writes, unlike upstream COutFile. Report flush errors
        // before the transaction can publish the stage, and preserve its timestamp.
        if (!outFile.flush()) {
            ErrorFileIndex = 1;
            return true;
        }
        outFile.setFileTime(QFileInfo(inPath).lastModified(), QFileDevice::FileModificationTime);
    }
    QFile::setPermissions(outPath, attrib);
    if (DeleteSrcFile) {
        if (!QFile::remove(inPath)) {
            ErrorFileIndex = 0;
            return true;
        }
    }
    return true;
}

bool IsDestChild(const QString &src, const QString &dest) {
    const qsizetype len = src.size();
    if (dest.size() < len)
        return false;
    if (dest.size() != len && dest[len] != '/')
        return false;
#ifdef Q_OS_WIN
    return QString::compare(dest.left(len), src, Qt::CaseInsensitive) == 0;
#else
    return QString::compare(dest.left(len), src, Qt::CaseSensitive) == 0;
#endif
}

bool CCopyState::CallProgress() {
    if (!Progress || Progress(StartPos))
        return true;
    ErrorMessage = QStringLiteral("Операция отменена.");
    return false;
}

bool CopyFile_Ask(CCopyState &state, const QString &srcPath, const QFileInfo &srcFileInfo,
                  const QString &destPath) {
#ifdef Q_OS_WIN
    const auto caseSensitivity = Qt::CaseInsensitive;
#else
    const auto caseSensitivity = Qt::CaseSensitive;
#endif
    if (QString::compare(destPath, srcPath, caseSensitivity) == 0) {
        state.ErrorMessage = QStringLiteral("Cannot copy file onto itself: %1").arg(destPath);
        return false;
    }

    // AskWrite adapter: unfinished overwrite interaction retains fail-on-conflict.
    if (!srcFileInfo.exists() || !srcFileInfo.isFile() || srcFileInfo.isSymLink() ||
        QFileInfo::exists(destPath) || QFileInfo(destPath).isSymLink()) {
        state.ErrorMessage = QStringLiteral("Источник или назначение недоступны: %1").arg(srcPath);
        return false;
    }
    if (state.CurrentFile)
        state.CurrentFile(srcPath);
    CCopyStateIO state2;
    state2.Progress = state.Progress;
    state2.StartPos = state.StartPos;
    if (!state2.MyCopyFile(srcPath, destPath, srcFileInfo.permissions())) {
        state.ErrorMessage = QStringLiteral("Операция отменена.");
        return false;
    }
    if (state2.ErrorFileIndex >= 0 || !state2.ErrorMessage.isEmpty()) {
        const QString errorName = state2.ErrorFileIndex == 0 ? srcPath : destPath;
        state.ErrorMessage = QStringLiteral("Ошибка чтения или записи файла: %1").arg(errorName);
        return false;
    }
    state.StartPos += state2.CurrentSize;
    return state.CallProgress();
}

static QString CombinePath(const QString &folderPath, const QString &fileName) {
    QString s(folderPath);
    if (!s.endsWith('/'))
        s += '/';
    s += fileName;
    return s;
}

bool CopyFolder(CCopyState &state, const QString &srcPath, const QString &destPath) {
    if (!state.CallProgress())
        return false;
    if (IsDestChild(srcPath, destPath)) {
        state.ErrorMessage = QStringLiteral("Cannot copy folder onto itself: %1").arg(destPath);
        return false;
    }
    // Do not follow links or merge a stage with an existing directory. Qt's
    // enumerator lacks the upstream Next error result; reject unreadable roots.
    const QFileInfo source(srcPath);
    if (!source.exists() || !source.isDir() || source.isSymLink() || !source.isReadable()) {
        state.ErrorMessage = QStringLiteral("Источник недоступен: %1").arg(srcPath);
        return false;
    }
    if (!QDir().mkdir(destPath)) {
        state.ErrorMessage = QStringLiteral("Cannot create folder: %1").arg(destPath);
        return false;
    }

    const QFileInfoList entries = QDir(srcPath).entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::NoSort);
    for (const QFileInfo &fi : entries) {
        const QString srcPath2 = CombinePath(srcPath, fi.fileName());
        const QString destPath2 = CombinePath(destPath, fi.fileName());
        if (fi.isDir()) {
            if (!CopyFolder(state, srcPath2, destPath2))
                return false;
        } else {
            if (!CopyFile_Ask(state, srcPath2, fi, destPath2))
                return false;
        }
    }
    return true;
}
} // namespace NFsFolder
