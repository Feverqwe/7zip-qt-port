// Adapted from 7-Zip UI/FileManager/FSFolder.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include <QFileDevice>
#include <QFileInfo>
#include <QString>
#include <functional>

namespace NFsFolder {
struct CCopyStateIO {
    int ErrorFileIndex = -1;
    QString ErrorMessage;
    quint64 CurrentSize = 0;
    quint64 StartPos = 0;
    bool DeleteSrcFile = false;
    std::function<bool(quint64)> Progress;
    bool MyCopyFile(const QString &inPath, const QString &outPath, QFileDevice::Permissions attrib);
};
bool IsDestChild(const QString &src, const QString &dest);

// Read/write branch of upstream CCopyState. The transaction owner retains
// move/rollback; this state copies into an unpublished destination only.
struct CCopyState {
    quint64 StartPos = 0;
    QString ErrorMessage;
    std::function<bool(quint64)> Progress;
    std::function<void(const QString &)> CurrentFile;
    bool CallProgress();
};
bool CopyFile_Ask(CCopyState &state, const QString &srcPath, const QFileInfo &srcFileInfo,
                  const QString &destPath);
bool CopyFolder(CCopyState &state, const QString &srcPath, const QString &destPath);
} // namespace NFsFolder
