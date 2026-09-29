// Owned file-only engine resource for a source archive folder.
#pragma once
#include "Common/MyCom.h"
#include "archivebackend.h"
#include "qtprogresstransport.h"
#include <QString>
#include <functional>

class IInStream;
class IProgress;
class ArchiveBackend;
class CProgressSync;
class IInArchive;
class IArchiveUpdateCallback;
class IArchiveExtractCallback;
class ISequentialOutStream;
class IFolderFolder;
class IArchiveOpenCallback;
struct CArc;
class QtArchiveFileResource final : public IUnknown, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_0
    QString _temporary;
    std::function<HRESULT(ISequentialOutStream *, IInArchive *, UInt32, IArchiveUpdateCallback *)>
        _update;
    CProgressSync *_sync = nullptr;
    qint64 _physicalSize = -1;
    qint64 _offset = 0;
    std::function<HRESULT(IFolderFolder **, IArchiveOpenCallback *)> _reopen;
    std::function<HRESULT(IInArchive *, const UInt32 *, UInt32, Int32, IArchiveExtractCallback *)>
        _extract;
    std::function<std::unique_ptr<IQtProgressTransport>(CProgressDialog &)> _progress;

  public:
    QString EnginePath;
    ArchiveInfo Info;
    explicit QtArchiveFileResource(const QString &path) : EnginePath(path) {}
    ~QtArchiveFileResource();
    HRESULT Spool(IInStream *stream, IProgress *progress);
    void SetArchiveMetadata(qint64 physicalSize, qint64 offset) {
        _physicalSize = physicalSize;
        _offset = offset;
    }
    HRESULT GetArchiveProperty(PROPID id, PROPVARIANT *value) const;
    HRESULT ConfigureArc(CArc &arc) const;
    void AttachUpdateService(ArchiveBackend &backend, const QString &format);
    void AttachProgressService(ArchiveBackend &backend);
    void AttachExtractService(ArchiveBackend &backend, const QString &format);
    void AttachReopenService(ArchiveBackend &backend, const QString &format);
    HRESULT ReOpen(IFolderFolder **metadata, IArchiveOpenCallback *callback);
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport(CProgressDialog &dialog);
    // The caller owns the dialog until the native worker has joined.
    void SetUpdateProgress(CProgressSync *sync) { _sync = sync; }
    bool HasUpdateService() const { return bool(_update) && _offset == 0; }
    bool HasExtractService() const { return bool(_extract); }
    HRESULT Update(ISequentialOutStream *output, IInArchive *metadata, UInt32 count,
                   IArchiveUpdateCallback *callback);
    HRESULT Extract(IInArchive *metadata, const UInt32 *indices, UInt32 count, Int32 testMode,
                    IArchiveExtractCallback *callback);
};
