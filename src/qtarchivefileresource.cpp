#include "qtarchivefileresource.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/CopyCoder.h"
#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

HRESULT QtArchiveFileResource::GetArchiveProperty(PROPID id, PROPVARIANT *value) const {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidPhySize && _physicalSize >= 0)
        prop = UInt64(_physicalSize);
    else if (id == kpidOffset)
        prop.Set_Int64(_offset);
    return prop.Detach(value);
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
    auto *outputSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> output = outputSpec;
    if (!outputSpec->Open_EXISTING(us2fs(path.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    RINOK(InStream_SeekToBegin(stream))
    CMyComPtr<ICompressProgressInfo> copyProgress = new CSpoolProgress(progress);
    RINOK(NCompress::CopyStream(stream, output, copyProgress))
    RINOK(outputSpec->Close())
    _temporary = path;
    EnginePath = path;
    temporary.setAutoRemove(false);
    return S_OK;
}
