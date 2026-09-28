// Adapted from 7-Zip CPP/7zip/UI/FileManager/FSFolderCopy.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// QFile replaces NIO files, a progress function replaces IProgress, and bool
// replaces HRESULT. ErrorFileIndex / ErrorMessage retain their upstream roles.
#include "FSFolder.h"
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
} // namespace NFsFolder
