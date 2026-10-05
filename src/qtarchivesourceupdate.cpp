#include "qtarchivesourceupdate.h"
#include "7zip/Archive/Common/ItemNameUtils.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/CopyCoder.h"
#include "7zip/IPassword.h"
#include "CPP/7zip/UI/Common/UpdateCallback.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/Windows/FileDir.h"
#include "Common/ComTry.h"
#include "Common/MyCom.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "Windows/TimeUtils.h"
#include "archivebackend.h"
#include "archivenativeopen.h"
#include "archiveprogresstransport.h"
#include "qtarchivebackendrequest.h"
#include "qtarchivefileresource.h"
#include "qtownedtempdir.h"
#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace {
class CResourceProgress final : public IQtProgressTransport {
    CMyComPtr<IUnknown> _resource;
    QtArchiveFileResource *_resourceSpec;
    ArchiveProgressTransport _transport;

  public:
    CResourceProgress(QtArchiveFileResource *resource, CProgressDialog &dialog,
                      ArchiveBackend &backend, bool nativeWorker)
        : _resource(resource), _resourceSpec(resource), _transport(dialog, backend, nativeWorker) {
        resource->SetUpdateProgress(&dialog.Sync);
    }
    ~CResourceProgress() override { _resourceSpec->SetUpdateProgress(nullptr); }
    HRESULT SetPaused(bool paused) override { return _transport.SetPaused(paused); }
    HRESULT SetBackground(bool background) override { return _transport.SetBackground(background); }
};
} // namespace

void QtArchiveFileResource::AttachProgressService(ArchiveBackend &backend) {
    _progress = [this, &backend](CProgressDialog &dialog, bool nativeWorker) {
        return std::make_unique<CResourceProgress>(this, dialog, backend, nativeWorker);
    };
}

void QtArchiveFileResource::AttachUpdateService(ArchiveBackend &backend, const QString &format) {
    AttachReopenService(backend, format);
    AttachProgressService(backend);
    _setProperties = [this, &backend](const wchar_t *const *names, const PROPVARIANT *values,
                                      UInt32 count) {
        return backend.setNativeArchiveProperties(_nativeOpen, names, values, count);
    };
    _update = [this, &backend, format](ISequentialOutStream *output, IInArchive *metadata,
                                       UInt32 count, IArchiveUpdateCallback *callback) -> HRESULT {
        _outputMTime.reset();
        if (auto *native = NativeItemProperties()) {
            CMyComPtr<IOutArchive> writer;
            const HRESULT query = native->QueryInterface(IID_IOutArchive, (void **)&writer);
            if (query != E_NOINTERFACE) {
                RINOK(query)
                if (const auto *source = dynamic_cast<CArchiveUpdateCallback *>(callback);
                    source && source->CommentIndex >= 0 && source->Comment) {
                    if (!HasCommentService())
                        return E_NOTIMPL;
                    return backend.updateNativeComment(_nativeOpen, output, count, callback);
                }
                if (_compressionOptions && !_nativePropertiesExplicit)
                    return backend.updateNativeArchive(_nativeOpen, output, count, callback,
                                                       *_compressionOptions);
                return backend.updateNativeArchive(_nativeOpen, output, count, callback);
            }
        }
        if (_compressionOptions &&
            (_compressionOptions->SymLinks.Val || _compressionOptions->HardLinks.Val ||
             _compressionOptions->AltStreams.Val || _compressionOptions->NtSecurity.Val)) {
            if (!NativeItemProperties())
                return E_NOTIMPL;
            return backend.updateNativeArchive(_nativeOpen, output, count, callback,
                                               *_compressionOptions);
        }
        if (const auto *source = dynamic_cast<CArchiveUpdateCallback *>(callback);
            source && source->CommentIndex >= 0 && source->Comment) {
            if (!HasCommentService())
                return E_NOTIMPL;
            return backend.updateNativeComment(_nativeOpen, output, count, callback);
        }
        return UpdateArchiveSourceStream(backend, EnginePath, format, output, metadata, count,
                                         callback, _sync,
                                         _compressionOptions ? &*_compressionOptions : nullptr);
    };
}

namespace {
class CCallbackProgress final : public ICompressProgressInfo, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressProgressInfo)
    IArchiveUpdateCallback *_callback;

  public:
    explicit CCallbackProgress(IArchiveUpdateCallback *callback) : _callback(callback) {}
    Z7_COM7F_IMF(SetRatioInfo(const UInt64 *inSize, const UInt64 *)) override {
        return _callback->SetCompleted(inSize);
    }
};

HRESULT FindChangedItem(IInArchive *metadata, UInt32 count, IArchiveUpdateCallback *callback,
                        UInt32 &changed, QString &name) {
    UInt32 originalCount;
    RINOK(metadata->GetNumberOfItems(&originalCount))
    // This engine service implements source UpdateOneFile only. Unsupported
    // callback plans must not silently become a different archive operation.
    if (count != originalCount)
        return E_NOTIMPL;
    changed = UInt32(-1);
    for (UInt32 i = 0; i < count; i++) {
        Int32 newData, newProps;
        UInt32 index;
        RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &index))
        if (index != i || newData != newProps)
            return E_NOTIMPL;
        if (!newData)
            continue;
        if (changed != UInt32(-1))
            return E_NOTIMPL;
        NWindows::NCOM::CPropVariant original, requested, directory;
        RINOK(metadata->GetProperty(index, kpidPath, &original))
        RINOK(callback->GetProperty(i, kpidPath, &requested))
        RINOK(callback->GetProperty(i, kpidIsDir, &directory))
        if (original.vt != VT_BSTR || requested.vt != VT_BSTR || directory.vt != VT_BOOL ||
            directory.boolVal != VARIANT_FALSE)
            return E_NOTIMPL;
        name = QString::fromWCharArray(requested.bstrVal);
        if (name != QString::fromWCharArray(original.bstrVal) ||
            !ArchiveBackend::sourceUpdatePathIsSafe(name))
            return E_NOTIMPL;
        changed = i;
    }
    return changed == UInt32(-1) ? E_NOTIMPL : S_OK;
}

struct CMutationPlan {
    ArchiveBackend::SourceMutation Operation = ArchiveBackend::SourceMutation::Delete;
    QStringList Paths;
};

HRESULT ReadPath(IInArchive *metadata, UInt32 index, QString &path, bool allowAbsolute = false);
HRESULT ReadNewPath(IArchiveUpdateCallback *callback, UInt32 index, QString &path,
                    bool allowAbsolute = false);

struct CAddedItem {
    UInt32 Index = 0;
    QString Path;
    bool Directory = false;
};

HRESULT FindAddPlan(IInArchive *metadata, UInt32 count, IArchiveUpdateCallback *callback,
                    QList<CAddedItem> &items) {
    UInt32 originalCount;
    RINOK(metadata->GetNumberOfItems(&originalCount))
    QSet<UInt32> kept;
    QSet<QString> names;
    for (UInt32 i = 0; i < count; ++i) {
        Int32 newData, newProps;
        UInt32 index;
        RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &index))
        if (index != UInt32(-1)) {
            if (index >= originalCount || kept.contains(index))
                return E_NOTIMPL;
            kept.insert(index);
        }
        if (!newData && !newProps) {
            if (index == UInt32(-1))
                return E_NOTIMPL;
            continue;
        }
        if (!newData || !newProps)
            return E_NOTIMPL;
        QString path;
        RINOK(ReadNewPath(callback, i, path))
        if (path.contains('*') || path.contains('?') || path.startsWith('@'))
            return E_NOTIMPL;
        if (index != UInt32(-1)) {
            QString original;
            RINOK(ReadPath(metadata, index, original))
            if (original != path)
                return E_NOTIMPL;
        }
        const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (names.contains(key))
            return E_NOTIMPL;
        names.insert(key);
        NWindows::NCOM::CPropVariant directory;
        RINOK(callback->GetProperty(i, kpidIsDir, &directory))
        if (directory.vt != VT_BOOL)
            return E_NOTIMPL;
        items.append({i, path, directory.boolVal == VARIANT_TRUE});
    }
    return kept.size() == int(originalCount) && !items.isEmpty() ? S_OK : E_NOTIMPL;
}

struct CSourceUpdatePlan {
    QStringList Removed;
    QStringList RemovedDirectories;
    QStringList RetainedPaths;
    QList<CAddedItem> Files;
    QList<CAddedItem> Directories;
};

HRESULT FindSourceUpdatePlan(IInArchive *metadata, UInt32 count, IArchiveUpdateCallback *callback,
                             bool allowAbsolute, CSourceUpdatePlan &plan) {
    UInt32 originalCount;
    RINOK(metadata->GetNumberOfItems(&originalCount))
    QStringList originalPaths;
    QList<bool> originalDirectories;
    QSet<QString> originalNames;
    for (UInt32 i = 0; i < originalCount; ++i) {
        QString path;
        RINOK(ReadPath(metadata, i, path, allowAbsolute))
        NWindows::NCOM::CPropVariant directory;
        RINOK(metadata->GetProperty(i, kpidIsDir, &directory))
        if (directory.vt != VT_BOOL)
            return E_NOTIMPL;
        const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (originalNames.contains(key))
            return E_NOTIMPL;
        originalNames.insert(key);
        originalPaths.append(path);
        originalDirectories.append(directory.boolVal == VARIANT_TRUE);
    }
    QSet<UInt32> referenced;
    QSet<QString> finalNames;
    for (UInt32 i = 0; i < count; ++i) {
        Int32 newData, newProps;
        UInt32 index;
        RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &index))
        if (index != UInt32(-1) && (index >= originalCount || referenced.contains(index)))
            return E_NOTIMPL;
        if (index != UInt32(-1))
            referenced.insert(index);
        QString path;
        bool directory = false;
        if (!newData && !newProps && index != UInt32(-1)) {
            path = originalPaths[int(index)];
            directory = originalDirectories[int(index)];
        } else if (newData && newProps) {
            RINOK(ReadNewPath(callback, i, path, allowAbsolute))
            NWindows::NCOM::CPropVariant directoryProperty;
            RINOK(callback->GetProperty(i, kpidIsDir, &directoryProperty))
            if (directoryProperty.vt != VT_BOOL)
                return E_NOTIMPL;
            directory = directoryProperty.boolVal == VARIANT_TRUE;
            if (index != UInt32(-1)) {
                if (originalPaths[int(index)] != path ||
                    originalDirectories[int(index)] != directory)
                    return E_NOTIMPL;
            }
            (directory ? plan.Directories : plan.Files).append({i, path, directory});
        } else
            return E_NOTIMPL;
        const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (finalNames.contains(key))
            return E_NOTIMPL;
        finalNames.insert(key);
    }
    for (UInt32 i = 0; i < originalCount; ++i) {
        // CLI Add uses the source Add action set and replaces selected files.
        // Deleting a replaced stream first would require an unsupported empty
        // intermediate archive for gzip/bzip2/xz. Delete only omitted items.
        if (!referenced.contains(i))
            (originalDirectories[int(i)] ? plan.RemovedDirectories : plan.Removed)
                .append(originalPaths[int(i)]);
        else
            plan.RetainedPaths.append(originalPaths[int(i)]);
    }
    return S_OK;
}

HRESULT ReadPath(IInArchive *metadata, UInt32 index, QString &path, bool allowAbsolute) {
    NWindows::NCOM::CPropVariant value;
    RINOK(metadata->GetProperty(index, kpidPath, &value))
    if (value.vt != VT_BSTR)
        return E_NOTIMPL;
    UString sourcePath(value.bstrVal);
    NArchive::NItemName::ReplaceSlashes_OsToUnix(sourcePath);
    path = QString::fromWCharArray(sourcePath.Ptr());
    return ArchiveBackend::sourceUpdatePathIsSafe(path, allowAbsolute) ? S_OK : E_NOTIMPL;
}

HRESULT ReadNewPath(IArchiveUpdateCallback *callback, UInt32 index, QString &path,
                    bool allowAbsolute) {
    NWindows::NCOM::CPropVariant value;
    RINOK(callback->GetProperty(index, kpidPath, &value))
    if (value.vt != VT_BSTR)
        return E_NOTIMPL;
    UString sourcePath(value.bstrVal);
    NArchive::NItemName::ReplaceSlashes_OsToUnix(sourcePath);
    path = QString::fromWCharArray(sourcePath.Ptr());
    return ArchiveBackend::sourceUpdatePathIsSafe(path, allowAbsolute) ? S_OK : E_NOTIMPL;
}

HRESULT FindMutationPlan(IInArchive *metadata, UInt32 count, IArchiveUpdateCallback *callback,
                         CMutationPlan &plan) {
    UInt32 originalCount;
    RINOK(metadata->GetNumberOfItems(&originalCount))
    if (count > originalCount + 1)
        return E_NOTIMPL;
    QSet<UInt32> kept;
    QList<QPair<QString, QString>> renamed;
    QString created;
    for (UInt32 i = 0; i < count; ++i) {
        Int32 newData, newProps;
        UInt32 index;
        RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &index))
        if (index == UInt32(-1)) {
            if (i != originalCount || count != originalCount + 1 || !newData || !newProps)
                return E_NOTIMPL;
            NWindows::NCOM::CPropVariant isDir;
            RINOK(callback->GetProperty(i, kpidIsDir, &isDir))
            if (isDir.vt != VT_BOOL || isDir.boolVal != VARIANT_TRUE)
                return E_NOTIMPL;
            RINOK(ReadNewPath(callback, i, created))
            continue;
        }
        if (index >= originalCount || kept.contains(index) || newData)
            return E_NOTIMPL;
        kept.insert(index);
        if (newProps) {
            QString oldPath, newPath;
            RINOK(ReadPath(metadata, index, oldPath))
            RINOK(ReadNewPath(callback, i, newPath))
            if (oldPath == newPath)
                return E_NOTIMPL;
            renamed.append({oldPath, newPath});
        }
    }
    if (!created.isEmpty()) {
        if (kept.size() != int(originalCount) || !renamed.isEmpty())
            return E_NOTIMPL;
        plan.Operation = ArchiveBackend::SourceMutation::CreateFolder;
        plan.Paths = {created};
        return S_OK;
    }
    if (!renamed.isEmpty()) {
        if (count != originalCount || kept.size() != int(originalCount))
            return E_NOTIMPL;
        const QStringList oldParts = renamed.first().first.split('/');
        const QStringList newParts = renamed.first().second.split('/');
        if (oldParts.size() != newParts.size())
            return E_NOTIMPL;
        int changed = -1;
        for (int i = 0; i < oldParts.size(); ++i)
            if (oldParts[i] != newParts[i]) {
                if (changed != -1)
                    return E_NOTIMPL;
                changed = i;
            }
        if (changed < 0)
            return E_NOTIMPL;
        const QString oldRoot = oldParts.mid(0, changed + 1).join('/');
        const QString newRoot = newParts.mid(0, changed + 1).join('/');
        for (const auto &item : renamed)
            if (item.first != oldRoot && !item.first.startsWith(oldRoot + '/') ||
                item.second != newRoot + item.first.mid(oldRoot.size()))
                return E_NOTIMPL;
        plan.Operation = ArchiveBackend::SourceMutation::Rename;
        plan.Paths = {oldRoot, newRoot};
        return S_OK;
    }
    if (count >= originalCount || kept.size() != int(count))
        return E_NOTIMPL;
    for (UInt32 i = 0; i < originalCount; ++i) {
        if (kept.contains(i))
            continue;
        QString path;
        RINOK(ReadPath(metadata, i, path))
        // A CLI delete of this path would also delete retained descendants.
        // Such callback plans require the staged source-plan adapter below.
        for (UInt32 child : kept) {
            QString childPath;
            RINOK(ReadPath(metadata, child, childPath))
            if (childPath.startsWith(path + '/'))
                return E_NOTIMPL;
        }
        plan.Paths.append(path);
    }
    plan.Operation = ArchiveBackend::SourceMutation::Delete;
    return plan.Paths.isEmpty() ? E_NOTIMPL : S_OK;
}

HRESULT StageCallbackStream(const QString &root, const QString &name, UInt32 index,
                            IArchiveUpdateCallback *callback) {
    const QString path = QDir(root).filePath(name);
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return E_FAIL;
    CMyComPtr<ISequentialInStream> input;
    RINOK(callback->GetStream(index, &input))
    if (!input)
        return E_FAIL;
    auto *file = new COutFileStream;
    CMyComPtr<ISequentialOutStream> output = file;
    if (!file->Create_NEW(us2fs(path.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    CMyComPtr<ICompressProgressInfo> progress = new CCallbackProgress(callback);
    RINOK(NCompress::CopyStream(input, output, progress))
    return file->Close();
}

HRESULT StageAddedItems(const QString &root, const QList<CAddedItem> &items,
                        IArchiveUpdateCallback *callback, QStringList &names,
                        QList<QPair<QString, QString>> *renames = nullptr) {
    QList<QPair<QString, CFiTime>> directories;
    for (const CAddedItem &item : items) {
        const QString stageName = renames && QDir::isAbsolutePath(item.Path)
                                      ? QStringLiteral("absolute-item-%1").arg(item.Index)
                                      : item.Path;
        const QString path = QDir(root).filePath(stageName);
        if (item.Directory) {
            if (!QDir().mkpath(path))
                return E_FAIL;
        } else
            RINOK(StageCallbackStream(root, stageName, item.Index, callback))
        NWindows::NCOM::CPropVariant modified;
        RINOK(callback->GetProperty(item.Index, kpidMTime, &modified))
        if (modified.vt == VT_FILETIME) {
            CFiTime time;
#ifdef _WIN32
            time = modified.filetime;
#else
            if (!FILETIME_To_timespec(modified.filetime, time))
                return E_FAIL;
#endif
            if (item.Directory)
                directories.append({path, time});
            else if (!NWindows::NFile::NDir::SetDirTime(us2fs(path.toStdWString().c_str()), nullptr,
                                                        nullptr, &time))
                return E_FAIL;
        }
        names.append(stageName);
        if (renames && stageName != item.Path)
            renames->append({stageName, item.Path});
    }
    for (auto it = directories.crbegin(); it != directories.crend(); ++it)
        if (!NWindows::NFile::NDir::SetDirTime(us2fs(it->first.toStdWString().c_str()), nullptr,
                                               nullptr, &it->second))
            return E_FAIL;
    return S_OK;
}
} // namespace

HRESULT UpdateArchiveSourceStream(ArchiveBackend &backend, const QString &archive,
                                  const QString &format, ISequentialOutStream *output,
                                  IInArchive *metadata, UInt32 count,
                                  IArchiveUpdateCallback *callback, CProgressSync *sync,
                                  const CompressionOptions *options) {
    COM_TRY_BEGIN
    if (!output || !metadata || !callback)
        return E_INVALIDARG;
    // File-only staging cannot carry the original callback's reparse data.
    // Require a native writer before exposing this choice for archive updates.
    if (options && (options->SymLinks.Val || options->HardLinks.Val || options->AltStreams.Val ||
                    options->NtSecurity.Val))
        return E_NOTIMPL;
    if (!backend.supportsArchiveUpdate(format))
        return E_NOTIMPL;
    // Prefix publication is owned by native CommonUpdateOperation. Until that
    // consumer is connected, this file service accepts only a zero-offset arc.
    NWindows::NCOM::CPropVariant offset;
    RINOK(metadata->GetArchiveProperty(kpidOffset, &offset))
    if ((offset.vt == VT_I8 && offset.hVal.QuadPart != 0) ||
        (offset.vt == VT_UI8 && offset.uhVal.QuadPart != 0) ||
        (offset.vt != VT_EMPTY && offset.vt != VT_I8 && offset.vt != VT_UI8))
        return E_NOTIMPL;
    QtOwnedTempDir stage;
    if (!stage.isValid())
        return E_FAIL;
    UInt32 changed;
    QString name;
    const HRESULT fileUpdate =
        options ? E_NOTIMPL : FindChangedItem(metadata, count, callback, changed, name);
    CMutationPlan mutation;
    QList<CAddedItem> added;
    CSourceUpdatePlan sourcePlan;
    ArchiveBackend::SourceUpdatePlan sourcePlanBackend;
    bool sourcePlanSelected = false;
    HRESULT mutationResult = E_NOTIMPL;
    if (fileUpdate == E_NOTIMPL)
        mutationResult = FindMutationPlan(metadata, count, callback, mutation);
    else if (fileUpdate != S_OK)
        return fileUpdate;
    if (fileUpdate != S_OK && mutationResult == E_NOTIMPL) {
        const HRESULT addResult =
            options ? E_NOTIMPL : FindAddPlan(metadata, count, callback, added);
        if (addResult == E_NOTIMPL) {
            added.clear();
            RINOK(FindSourceUpdatePlan(metadata, count, callback,
                                       options && options->PathMode == NWildcard::k_AbsPath,
                                       sourcePlan))
            sourcePlanSelected = true;
        } else
            RINOK(addResult)
    } else if (mutationResult != S_OK && fileUpdate != S_OK)
        return mutationResult;
    const QString sourceRoot = stage.filePath("source");
    QStringList addedNames;
    if (fileUpdate == S_OK)
        RINOK(StageCallbackStream(sourceRoot, name, changed, callback))
    else if (!added.isEmpty())
        RINOK(StageAddedItems(sourceRoot, added, callback, addedNames))
    else if (sourcePlanSelected) {
        const QString directoryRoot = stage.filePath("directories");
        sourcePlanBackend.allowAbsolute = options && options->PathMode == NWildcard::k_AbsPath;
        RINOK(StageAddedItems(sourceRoot, sourcePlan.Files, callback, sourcePlanBackend.files,
                              &sourcePlanBackend.renames))
        RINOK(StageAddedItems(directoryRoot, sourcePlan.Directories, callback,
                              sourcePlanBackend.directories, &sourcePlanBackend.renames))
        sourcePlanBackend.fileRoot = sourceRoot;
        sourcePlanBackend.directoryRoot = directoryRoot;
        sourcePlanBackend.removed = sourcePlan.Removed;
        sourcePlanBackend.removedDirectories = sourcePlan.RemovedDirectories;
        sourcePlanBackend.retainedPaths = sourcePlan.RetainedPaths;
    } else if (mutation.Operation == ArchiveBackend::SourceMutation::CreateFolder &&
               !QDir().mkpath(QDir(sourceRoot).filePath(mutation.Paths.front())))
        return E_FAIL;
    CMyComPtr<ICryptoGetTextPassword2> crypto;
    callback->QueryInterface(IID_ICryptoGetTextPassword2, (void **)&crypto);
    QString password;
    if (crypto) {
        Int32 defined = 0;
        CMyComBSTR value;
        RINOK(crypto->CryptoGetTextPassword2(&defined, &value))
        if (defined)
            password = QString::fromWCharArray(value);
    }
    sourcePlanBackend.allowProtected = !password.isEmpty();
    // Mutations operate on a copied archive. Preserve its basename because
    // pathless native handlers derive the selected item's DefaultName from it.
    // The private subdirectory avoids collision with prepared source files.
    const bool copiedMutation = fileUpdate != S_OK && added.isEmpty() && !sourcePlanSelected;
    const QString result = copiedMutation
                               ? stage.filePath("mutation/" + QFileInfo(archive).fileName())
                               : stage.filePath("result." + format);
    if (copiedMutation && !QDir().mkpath(QFileInfo(result).absolutePath()))
        return E_FAIL;
    if (fileUpdate == S_OK) {
        RINOK(RunArchiveBackendRequest(
            backend,
            [&backend, archive, name, sourceRoot, result, format, password] {
                backend.updateSourceFile(archive, name, sourceRoot, result, format, password);
            },
            callback, sync))
    } else if (!added.isEmpty()) {
        RINOK(RunArchiveBackendRequest(
            backend,
            [&backend, archive, addedNames, sourceRoot, result, format, password] {
                backend.updateSourceAdd(archive, addedNames, sourceRoot, result, format, password);
            },
            callback, sync))
    } else if (sourcePlanSelected) {
        RINOK(RunArchiveBackendRequest(
            backend,
            [&backend, archive, sourcePlanBackend, result, format, password, options] {
                backend.updateSourcePlan(archive, sourcePlanBackend, result, format, password,
                                         options ? std::optional<CompressionOptions>(*options)
                                                 : std::nullopt);
            },
            callback, sync))
    } else {
        RINOK(RunArchiveBackendRequest(
            backend,
            [&backend, archive, result, format, mutation, sourceRoot, password] {
                backend.updateSourceMutation(
                    archive, result, format, mutation.Operation, mutation.Paths,
                    mutation.Operation == ArchiveBackend::SourceMutation::CreateFolder ? sourceRoot
                                                                                       : QString(),
                    password);
            },
            callback, sync))
    }
    auto *inputSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> input = inputSpec;
    if (!inputSpec->Open(us2fs(result.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    CMyComPtr<ICompressProgressInfo> progress = new CCallbackProgress(callback);
    RINOK(NCompress::CopyStream(input, output, progress))
    return callback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
    COM_TRY_END
}
