#include "qtarchivefileresource.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/CopyCoder.h"
#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "CPP/7zip/UI/Common/WorkDir.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "qtarchivedeleteinputs.h"
#include "qtownedtempregistry.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QTemporaryFile>

namespace {
class CSpoolProgress final : public ICompressProgressInfo, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressProgressInfo)
    IProgress *_progress;

  public:
    explicit CSpoolProgress(IProgress *progress) : _progress(progress) {}
    Z7_COM7F_IMF(SetRatioInfo(const UInt64 *inSize, const UInt64 *)) override {
        return _progress ? _progress->SetCompleted(inSize) : S_OK;
    }
};
} // namespace

QtArchiveFileResource::~QtArchiveFileResource() {
    if (!_temporary.isEmpty())
        QFile::remove(_temporary);
}

HRESULT QtArchiveFileResource::ApplyOutputMTime(CWorkDirTempFile &file) const {
    return _outputMTime ? file.SetMTime(*_outputMTime) : S_OK;
}

HRESULT QtArchiveFileResource::AdoptPublishedOwnedTemp(const CWorkDirTempFile &file) {
    // A user archive is never registered by publication. Only a private spool
    // already owned by this resource can receive the successfully moved object.
    if (_temporary.isEmpty())
        return S_OK;
    const QString destination = QString::fromWCharArray(fs2us(file.Get_OriginalFilePath()).Ptr());
    if (QDir::cleanPath(destination) != QDir::cleanPath(_temporary) || !file.OwnedLease())
        return E_INVALIDARG;
    auto replacement = file.OwnedLease();
    if (replacement->MatchesObjectAt(_temporary)) {
        if (!replacement->RelocateAfterMove(_temporary))
            return E_FAIL;
    } else {
        // Cross-volume source moves can copy into a different OS file object.
        replacement = QtOwnedTempRegistry().RegisterCreated(_temporary);
        if (!replacement)
            return E_FAIL;
    }
    _temporaryLease = std::move(replacement);
    return S_OK;
}

void QtArchiveFileResource::CaptureOutputMTime(const CFiTime &time, bool defined) {
    _outputMTime.reset();
    if (_compressionOptions && _compressionOptions->SetArcMTime.Val && defined)
        _outputMTime = time;
}

void QtArchiveFileResource::CaptureDeleteInputs(const CDirItems &items,
                                                const CByteBuffer &processed) {
    _deleteInputs.reset();
    if (_compressionOptions && _compressionOptions->DeleteAfterCompressing) {
        _deleteInputs = std::make_shared<QtArchiveDeleteInputs>();
        _deleteInputs->Capture(items, processed);
    }
}

HRESULT QtArchiveFileResource::DeleteInputsAfterPublication(IUpdateDeletionCallback &callback) {
    auto inputs = std::move(_deleteInputs);
    return inputs ? inputs->Delete(callback) : S_OK;
}

HRESULT QtArchiveFileResource::GetArchiveProperty(PROPID id, PROPVARIANT *value) const {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidPhySize && _physicalSize >= 0)
        prop = UInt64(_physicalSize);
    else if (id == kpidOffset)
        prop.Set_Int64(_offset);
    else if (id >= 0x10000 && id - 0x10000 < unsigned(Info.properties.size())) {
        auto i = Info.properties.cbegin();
        std::advance(i, id - 0x10000);
        prop = i.value().toStdWString().c_str();
    }
    return prop.Detach(value);
}

HRESULT QtArchiveFileResource::GetNumberOfArchiveProperties(UInt32 *count) const {
    *count = UInt32(Info.properties.size());
    return S_OK;
}

HRESULT QtArchiveFileResource::GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *id,
                                                      VARTYPE *type) const {
    if (index >= unsigned(Info.properties.size()))
        return E_INVALIDARG;
    auto i = Info.properties.cbegin();
    std::advance(i, index);
    const std::wstring key = i.key().toStdWString();
    *name = ::SysAllocStringLen(key.c_str(), unsigned(key.size()));
    *id = 0x10000 + index;
    *type = VT_BSTR;
    return *name ? S_OK : E_OUTOFMEMORY;
}

HRESULT QtArchiveFileResource::ConfigureArc(CArc &arc) const {
    const QFileInfo file(EnginePath);
    if (!file.isFile())
        return E_FAIL;
    arc.Path = EnginePath.toStdWString().c_str();
    // 7zz reports the logical stream across all split parts; source
    // ReadBasicProps compares kpidPhySize with that combined stream size.
    arc.FileSize = UInt64(Info.volumeCount > 1 && Info.totalPhysicalSize >= file.size()
                              ? Info.totalPhysicalSize
                              : file.size());
    arc.ArcStreamOffset = 0;
    return arc.ReadBasicProps(arc.Archive, 0, S_OK);
}

HRESULT QtArchiveFileResource::Update(ISequentialOutStream *output, IInArchive *metadata,
                                      UInt32 count, IArchiveUpdateCallback *callback) {
    if (!_update)
        return E_NOTIMPL;
    return _update(output, metadata, count, callback);
}

HRESULT QtArchiveFileResource::ReOpen(IFolderFolder **metadata, IArchiveOpenCallback *callback) {
    return _reopen ? _reopen(metadata, callback) : E_NOTIMPL;
}

HRESULT QtArchiveFileResource::Extract(IInArchive *metadata, const UInt32 *indices, UInt32 count,
                                       Int32 testMode, IArchiveExtractCallback *callback) {
    return _extract ? _extract(metadata, indices, count, testMode, callback) : E_NOTIMPL;
}

std::unique_ptr<IQtProgressTransport>
QtArchiveFileResource::CreateProgressTransport(CProgressDialog &dialog) {
    return _progress ? _progress(dialog) : nullptr;
}

HRESULT QtArchiveFileResource::Spool(IInStream *stream, IProgress *progress) {
    if (!_temporary.isEmpty())
        return E_FAIL;
    // 7zz accepts files only. The source stream and copy algorithm are retained;
    // Qt supplies solely the private filename/cleanup service at this boundary.
    QTemporaryFile temporary(QDir::tempPath() + "/sevenzip-open-XXXXXX");
    if (!temporary.open())
        return E_FAIL;
    const QString path = temporary.fileName();
    temporary.close();
    auto lease = QtOwnedTempRegistry().RegisterCreated(path);
    if (!lease)
        return E_FAIL;
    auto *outputSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> output = outputSpec;
    if (!outputSpec->Open_EXISTING(us2fs(path.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    RINOK(InStream_SeekToBegin(stream))
    CMyComPtr<ICompressProgressInfo> copyProgress = new CSpoolProgress(progress);
    RINOK(NCompress::CopyStream(stream, output, copyProgress))
    RINOK(outputSpec->Close())
    _temporary = path;
    _temporaryLease = std::move(lease);
    EnginePath = path;
    temporary.setAutoRemove(false);
    return S_OK;
}

HRESULT QtArchiveFileResource::SpoolToOwnedFile(IInStream *stream, const QString &path,
                                                IProgress *progress) {
    if (!stream || !_temporary.isEmpty() || !QDir::isAbsolutePath(path))
        return E_INVALIDARG;
    auto *outputSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> output = outputSpec;
    if (!outputSpec->Create_NEW(us2fs(path.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    bool completed = false;
    const auto cleanup = qScopeGuard([&] {
        outputSpec->Close();
        if (!completed)
            QFile::remove(path);
    });
    RINOK(InStream_SeekToBegin(stream))
    CMyComPtr<ICompressProgressInfo> copyProgress = new CSpoolProgress(progress);
    RINOK(NCompress::CopyStream(stream, output, copyProgress))
    RINOK(outputSpec->Close())
    EnginePath = path;
    completed = true;
    return S_OK; // The source CTempFileInfo owns cleanup of this file.
}
