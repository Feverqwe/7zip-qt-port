// Adapted from 7-Zip CPP/7zip/UI/FileManager/FSFolderCopy.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt supplies portable stage/file services; the source CopyTo, AskWrite,
// recursive copy/move and progress control flow remain in this module.
#include "CPP/Windows/FileDir.h"
#include "Common/StringConvert.h"
#include "FSFolder.h"
#include "Windows/FileName.h"
#include "Windows/TimeUtils.h"
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
    if (Callback && Callback->SetCompleted(&StartPos) != S_OK) {
        ErrorMessage = QStringLiteral("Операция отменена.");
        return false;
    }
    if (!Progress || Progress(StartPos))
        return true;
    ErrorMessage = QStringLiteral("Операция отменена.");
    return false;
}

DWORD CCopyState::ReportSystemProgress(UInt64 total, UInt64 current) {
    if (total > FileSize) {
        TotalSize += total - FileSize;
        FileSize = total;
        if (Callback)
            ProgressResult = Callback->SetTotal(TotalSize);
    }
    const UInt64 completed = StartPos + current;
    if (ProgressResult == S_OK && Callback)
        ProgressResult = Callback->SetCompleted(&completed);
    if (ProgressResult == S_OK && Progress && !Progress(completed))
        ProgressResult = E_ABORT;
    return ProgressResult == S_OK ? PROGRESS_CONTINUE : PROGRESS_CANCEL;
}

#ifdef _WIN32
static DWORD CALLBACK CopyProgressRoutine(LARGE_INTEGER totalFileSize,
                                          LARGE_INTEGER totalBytesTransferred,
                                          LARGE_INTEGER /* streamSize */,
                                          LARGE_INTEGER /* streamBytesTransferred */,
                                          DWORD /* streamNumber */, DWORD /* callbackReason */,
                                          HANDLE /* source */, HANDLE /* destination */,
                                          LPVOID data) {
    return static_cast<CCopyState *>(data)->ReportSystemProgress(
        UInt64(totalFileSize.QuadPart), UInt64(totalBytesTransferred.QuadPart));
}
#endif

bool CCopyState::CopyFile_Sys(const QString &oldFile, const QString &newFile,
                              QFileDevice::Permissions attrib) {
#ifdef _WIN32
    Q_UNUSED(attrib)
    const std::wstring oldWide = QDir::toNativeSeparators(oldFile).toStdWString();
    const std::wstring newWide = QDir::toNativeSeparators(newFile).toStdWString();
    FileSize = quint64(QFileInfo(oldFile).size());
    ProgressResult = S_OK;
    BOOL cancelFlag = FALSE;
    const bool copied = ::CopyFileExW(oldWide.c_str(), newWide.c_str(), CopyProgressRoutine, this,
                                      &cancelFlag, COPY_FILE_FAIL_IF_EXISTS) != FALSE;
    if (copied) {
        StartPos += FileSize;
        return true;
    }
    ErrorMessage = ProgressResult == S_OK
                       ? QStringLiteral("Не удалось скопировать файл: %1").arg(oldFile)
                       : QStringLiteral("Операция отменена.");
    return false;
#else
    CCopyStateIO state2;
    const bool targetExistedBefore = QFileInfo::exists(newFile);
    state2.StartPos = StartPos;
    state2.Progress = [this](quint64 completed) {
        if (Callback && Callback->SetCompleted(&completed) != S_OK)
            return false;
        return !Progress || Progress(completed);
    };
    if (!state2.MyCopyFile(oldFile, newFile, attrib)) {
        if (!targetExistedBefore)
            QFile::remove(newFile);
        ErrorMessage = QStringLiteral("Операция отменена.");
        return false;
    }
    if (state2.ErrorFileIndex >= 0 || !state2.ErrorMessage.isEmpty()) {
        if (!targetExistedBefore)
            QFile::remove(newFile);
        const QString errorName = state2.ErrorFileIndex == 0 ? oldFile : newFile;
        ErrorMessage = QStringLiteral("Ошибка чтения или записи файла: %1").arg(errorName);
        return false;
    }
    StartPos += state2.CurrentSize;
    return true;
#endif
}

class CopyProgressRoutine final : public NWindows::NFile::NDir::ICopyFileProgress {
  public:
    explicit CopyProgressRoutine(CCopyState &state) : state_(state) {}
    DWORD CopyFileProgress(UInt64 total, UInt64 current) override {
        return state_.ReportSystemProgress(total, current);
    }

  private:
    CCopyState &state_;
};

bool CCopyState::MoveFile_Sys(const QString &oldFile, const QString &newFile,
                              QFileDevice::Permissions attrib) {
    Q_UNUSED(attrib)
    // POSIX MyMoveFile_with_Progress copies EXDEV files; directories must
    // return to CopyFolder's recursive source branch instead.
    if (QFileInfo(oldFile).isDir())
        return QDir().rename(oldFile, newFile);
    const std::wstring oldWide = QDir::toNativeSeparators(oldFile).toStdWString();
    const std::wstring newWide = QDir::toNativeSeparators(newFile).toStdWString();
    FileSize = quint64(QFileInfo(oldFile).size());
    ProgressResult = S_OK;
    CopyProgressRoutine progress(*this);
    const bool moved = NWindows::NFile::NDir::MyMoveFile_with_Progress(
        us2fs(oldWide.c_str()), us2fs(newWide.c_str()), &progress);
    if (ProgressResult != S_OK)
        ErrorMessage = QStringLiteral("Операция отменена.");
    else if (!moved)
        ErrorMessage = QStringLiteral("Не удалось переместить объект: %1").arg(oldFile);
    if (moved)
        StartPos += FileSize;
    return moved;
}

static bool ReportError(CCopyState &state, const QString &message) {
    state.ErrorMessage = message;
    if (state.Callback)
        state.Callback->ShowMessage(message.toStdWString().c_str());
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

    if (!srcFileInfo.exists() || !srcFileInfo.isFile() || srcFileInfo.isSymLink())
        return ReportError(state, QStringLiteral("Источник недоступен: %1").arg(srcPath));

    QString destPathNew = destPath;
    if (state.Callback) {
        NWindows::NFile::NFind::CFileInfo nativeInfo;
        const std::wstring wide = QDir::toNativeSeparators(srcPath).toStdWString();
        if (!nativeInfo.Find(us2fs(wide.c_str())))
            return ReportError(state, QStringLiteral("Источник недоступен: %1").arg(srcPath));
        CMyComBSTR destPathResult;
        Int32 writeAskResult = 0;
        FILETIME sourceTime;
        FiTime_To_FILETIME(nativeInfo.MTime, sourceTime);
        const std::wstring sourceWide = srcPath.toStdWString();
        const std::wstring destWide = destPath.toStdWString();
        if (state.Callback->AskWrite(sourceWide.c_str(), BoolToInt(false), &sourceTime,
                                     &nativeInfo.Size, destWide.c_str(), &destPathResult,
                                     &writeAskResult) != S_OK)
            return ReportError(state, QStringLiteral("Операция отменена."));
        if (!IntToBool(writeAskResult)) {
            if (state.TotalSize >= nativeInfo.Size) {
                state.TotalSize -= nativeInfo.Size;
                if (state.Callback->SetTotal(state.TotalSize) != S_OK)
                    return ReportError(state, QStringLiteral("Операция отменена."));
            }
            return state.CallProgress();
        }
        destPathNew = QString::fromWCharArray((LPCOLESTR)destPathResult);
        if (state.Callback->SetCurrentFilePath(sourceWide.c_str()) != S_OK)
            return ReportError(state, QStringLiteral("Операция отменена."));
    } else if (QFileInfo::exists(destPath) || QFileInfo(destPath).isSymLink()) {
        return ReportError(state, QStringLiteral("Назначение недоступно: %1").arg(destPath));
    }
    if (state.CurrentFile)
        state.CurrentFile(srcPath);
    const bool copied = state.MoveMode
                            ? state.MoveFile_Sys(srcPath, destPathNew, srcFileInfo.permissions())
                            : state.CopyFile_Sys(srcPath, destPathNew, srcFileInfo.permissions());
    if (!copied)
        return ReportError(state, state.ErrorMessage);
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
    const QFileInfo source(srcPath);
    if (!source.exists() || !source.isDir() || source.isSymLink() || !source.isReadable())
        return ReportError(state, QStringLiteral("Источник недоступен: %1").arg(srcPath));
    if (state.MoveMode && state.MoveFile_Sys(srcPath, destPath, source.permissions()))
        return true;
    if (QFileInfo(destPath).isSymLink() ||
        !(state.Callback ? QDir().mkpath(destPath) : QDir().mkdir(destPath)))
        return ReportError(state, QStringLiteral("Cannot create folder: %1").arg(destPath));

    NWindows::NFile::NFind::CEnumerator enumerator;
    const std::wstring sourceWide = QDir::toNativeSeparators(srcPath).toStdWString();
    FString prefix = us2fs(sourceWide.c_str());
    prefix.Add_PathSepar();
    enumerator.SetDirPrefix(prefix);
    for (;;) {
        NWindows::NFile::NFind::CFileInfo fi;
        bool found = false;
#ifdef _WIN32
        if (!enumerator.Next(fi, found))
            return ReportError(state,
                               QStringLiteral("Не удалось прочитать папку: %1").arg(srcPath));
#else
        NWindows::NFile::NFind::CDirEntry entry;
        if (!enumerator.Next(entry, found))
            return ReportError(state,
                               QStringLiteral("Не удалось прочитать папку: %1").arg(srcPath));
        if (found && !enumerator.Fill_FileInfo(entry, fi, false))
            return ReportError(state,
                               QStringLiteral("Не удалось прочитать папку: %1").arg(srcPath));
#endif
        if (!found)
            break;
        const QString name = QString::fromWCharArray(fs2us(fi.Name));
        const QString srcPath2 = CombinePath(srcPath, name);
        const QString destPath2 = CombinePath(destPath, name);
        if (QFileInfo(srcPath2).isSymLink())
            return ReportError(state,
                               QStringLiteral("Ссылки пока не поддерживаются: %1").arg(srcPath2));
        if (fi.IsDir()) {
            if (!CopyFolder(state, srcPath2, destPath2))
                return false;
        } else {
            if (!CopyFile_Ask(state, srcPath2, QFileInfo(srcPath2), destPath2))
                return false;
        }
    }
    if (state.MoveMode && !QDir().rmdir(srcPath))
        return ReportError(state, QStringLiteral("Cannot remove folder: %1").arg(srcPath));
    return true;
}

Z7_COM7F_IMF(CFSFolder::CopyTo(Int32 moveMode, const UInt32 *indices, UInt32 numItems,
                               Int32 /* includeAltStreams */, Int32 /* replaceAltStreamColon */,
                               const wchar_t *path, IFolderOperationsExtractCallback *callback)) {
    if (numItems == 0)
        return S_OK;
    if (!indices || !path || !callback)
        return E_INVALIDARG;

    const FString destPath = us2fs(path);
    if (destPath.IsEmpty())
        return E_INVALIDARG;
    bool isAltDest = false;
#ifdef Q_OS_WIN
    isAltDest = NWindows::NFile::NName::IsAltPathPrefix(destPath);
#endif
    if (isAltDest)
        return E_NOTIMPL; // Alternate streams require the P2-05 platform adapter.
    const bool isDirectPath = !IsPathSepar(destPath.Back());
    if (isDirectPath && numItems > 1)
        return E_INVALIDARG;

    CFsFolderStat stat(FString(), callback);
    for (UInt32 i = 0; i < numItems; i++) {
        const UInt32 index = indices[i];
        if (index >= Files.Size())
            return E_INVALIDARG;
        const CDirItem &fi = Files[index];
        const FString srcPath = _path + GetRelPath(fi);
        if (QFileInfo(QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(srcPath))))
                .isSymLink())
            return E_NOTIMPL;
        if (fi.IsDir()) {
            stat.Path = srcPath;
            RINOK(stat.Enumerate())
            stat.NumFolders++;
        } else {
            stat.NumFiles++;
            stat.Size += fi.Size;
        }
    }
    RINOK(callback->SetTotal(stat.Size))
    RINOK(callback->SetNumFiles(stat.NumFiles))
    UInt64 completedSize = 0;
    RINOK(callback->SetCompleted(&completedSize))

    CCopyState state;
    state.TotalSize = stat.Size;
    state.Callback = callback;
    state.MoveMode = IntToBool(moveMode);
    state.IsAltStreamsDest = isAltDest;
    state.UseReadWriteMode = isAltDest;
    for (UInt32 i = 0; i < numItems; i++) {
        const CDirItem &fi = Files[indices[i]];
        FString destPath2 = destPath;
        if (!isDirectPath)
            destPath2 += fi.Name;
        const FString srcPath = _path + GetRelPath(fi);
        const QString source = QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(srcPath)));
        const QString target =
            QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(destPath2)));
        if (fi.IsDir()) {
            if (!CopyFolder(state, source, target))
                return E_ABORT;
        } else if (!CopyFile_Ask(state, source, QFileInfo(source), target)) {
            return E_ABORT;
        }
    }
    return S_OK;
}

HRESULT CopyFileSystemItems(const UStringVector &itemsPaths, const FString &destDirPrefix,
                            bool moveMode, IFolderOperationsExtractCallback *callback) {
    if (itemsPaths.IsEmpty())
        return S_OK;
    if (destDirPrefix.IsEmpty() || !callback || !IsPathSepar(destDirPrefix.Back()))
        return E_INVALIDARG;
    bool isAltDest = false;
#ifdef Q_OS_WIN
    isAltDest = NWindows::NFile::NName::IsAltPathPrefix(destDirPrefix);
#endif
    if (isAltDest)
        return E_NOTIMPL;

    CFsFolderStat stat(FString(), callback);
    FOR_VECTOR(i, itemsPaths) {
        const FString path = us2fs(itemsPaths[i]);
        NWindows::NFile::NFind::CFileInfo fi;
        if (!fi.Find(path))
            continue;
        if (QFileInfo(QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(path)))).isSymLink())
            return E_NOTIMPL;
        if (fi.IsDir()) {
            stat.Path = path;
            RINOK(stat.Enumerate())
            stat.NumFolders++;
        } else {
            stat.NumFiles++;
            stat.Size += fi.Size;
        }
    }
    RINOK(callback->SetTotal(stat.Size))
    UInt64 completedSize = 0;
    RINOK(callback->SetCompleted(&completedSize))

    CCopyState state;
    state.TotalSize = stat.Size;
    state.Callback = callback;
    state.MoveMode = moveMode;
    state.IsAltStreamsDest = isAltDest;
    state.UseReadWriteMode = isAltDest;
    FOR_VECTOR(i, itemsPaths) {
        const FString path = us2fs(itemsPaths[i]);
        NWindows::NFile::NFind::CFileInfo fi;
        if (!fi.Find(path)) {
            const UString error = UString(L"Cannot find the file : ") + itemsPaths[i];
            RINOK(callback->ShowMessage(error))
            continue;
        }
        FString destPath = destDirPrefix;
        destPath += fi.Name;
        const QString source = QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(path)));
        const QString target = QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(destPath)));
        if (fi.IsDir()) {
            if (!CopyFolder(state, source, target))
                return E_ABORT;
        } else if (!CopyFile_Ask(state, source, QFileInfo(source), target)) {
            return E_ABORT;
        }
    }
    return S_OK;
}
} // namespace NFsFolder
