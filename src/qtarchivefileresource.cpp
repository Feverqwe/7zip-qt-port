#include "qtarchivefileresource.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/CopyCoder.h"
#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "CPP/7zip/UI/Common/WorkDir.h"
#include "Common/StringConvert.h"
#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"
#include "archivecodeccatalog.h"
#include "archivenativeopen.h"
#include "qtarchivedeleteinputs.h"
#include "qtownedtempregistry.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>

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
    CloseNativeOpen();
    if (!_temporary.isEmpty())
        QFile::remove(_temporary);
}

HRESULT QtArchiveFileResource::CloseNativeOpen() {
    _nativeItemOrder = false;
    auto reader = std::move(_nativeOpen);
    return reader ? reader->Close() : S_OK;
}

void QtArchiveFileResource::AttachNativeOpen(std::shared_ptr<ArchiveNativeOpen> reader) {
    _nativeItemOrder = false;
    _nativeNested = reader && reader->hasNestedStream();
    _nativeReadOnly = reader && reader->arc().IsReadOnly;
    _nativeOpenType = reader ? reader->transportType() : QString();
    _nativeOpen = std::move(reader);
}

bool QtArchiveFileResource::RequiresOuterOpen() const {
    return Info.volumeCount > 1 || _nativeNested;
}

QString QtArchiveFileResource::TransportFormat() const {
    return _nativeNested ? _nativeOpenType : (Info.volumeCount > 1 ? QString() : Info.format);
}

bool QtArchiveFileResource::HasUpdateService() const {
    // The process writer publishes one physical archive, not a volume set or
    // a reconstructed chain of native container streams. Source folder
    // read-only state must reflect this transport constraint as well.
    return bool(_update) && _offset == 0 && !RequiresOuterOpen() && !_nativeReadOnly &&
           (!_nativeOpen || !_nativeOpen->arc().IsTree || NativeItemProperties());
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
        _deleteInputs->SymLinks = _compressionOptions->SymLinks;
    }
}

HRESULT QtArchiveFileResource::DeleteInputsAfterPublication(IUpdateDeletionCallback &callback) {
    auto inputs = std::move(_deleteInputs);
    return inputs ? inputs->Delete(callback) : S_OK;
}

HRESULT QtArchiveFileResource::GetArchiveProperty(PROPID id, PROPVARIANT *value) const {
    if (_nativeOpen)
        return _nativeOpen->arc().Archive->GetArchiveProperty(id, value);
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
    if (_nativeOpen)
        return _nativeOpen->arc().Archive->GetNumberOfArchiveProperties(count);
    *count = UInt32(Info.properties.size());
    return S_OK;
}

HRESULT QtArchiveFileResource::GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *id,
                                                      VARTYPE *type) const {
    if (_nativeOpen)
        return _nativeOpen->arc().Archive->GetArchivePropertyInfo(index, name, id, type);
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

HRESULT QtArchiveFileResource::ConfigureArc(CArc &arc, const CCodecs *codecs) const {
    const QFileInfo file(EnginePath);
    if (!file.isFile())
        return E_FAIL;
    arc.Path = EnginePath.toStdWString().c_str();
    if (_nativeOpen) {
        const CArc &native = _nativeOpen->arc();
        // A native index belongs to its retained catalog. Map the handler name
        // into the caller's metadata table for source IsHashHandler; never copy
        // an index between different codec owners.
        if (codecs)
            if (const CArcInfoEx *format = _nativeOpen->formatInfo())
                arc.FormatIndex = codecs->FindFormatForArchiveType(format->Name);
        arc.DefaultName = native.DefaultName;
        arc.IsReadOnly = native.IsReadOnly;
        arc.FileSize = native.FileSize;
        arc.ArcStreamOffset = native.ArcStreamOffset;
        arc.GetRawProps.Release();
        arc.GetRootProps.Release();
        arc.IsTree = false;
        arc.Ask_Deleted = false;
        arc.Ask_AltStream = false;
        arc.Ask_Aux = false;
        arc.Ask_INode = false;
        if (NativeItemProperties()) {
            arc.Archive->QueryInterface(IID_IArchiveGetRawProps, (void **)&arc.GetRawProps);
            arc.GetRootProps = native.GetRootProps;
            arc.IsTree = native.IsTree;
            arc.Ask_Deleted = native.Ask_Deleted;
            arc.Ask_AltStream = native.Ask_AltStream;
            arc.Ask_Aux = native.Ask_Aux;
            arc.Ask_INode = native.Ask_INode;
        }
        return arc.ReadBasicProps(arc.Archive, 0, S_OK);
    }
    if (_codecs && _codecs->result() == S_OK)
        arc.SetDefaultName(_codecs->nativeCodecs(), _codecs->formatIndex(Info.format));
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
QtArchiveFileResource::CreateProgressTransport(CProgressDialog &dialog, bool nativeWorker) {
    if (_compressionOptions && NativeItemProperties()) {
        const auto &options = *_compressionOptions;
        nativeWorker = nativeWorker || options.SymLinks.Val || options.HardLinks.Val ||
                       options.AltStreams.Val || options.NtSecurity.Val;
    }
    return _progress ? _progress(dialog, nativeWorker) : nullptr;
}

HRESULT QtArchiveFileResource::Spool(IInStream *stream, IProgress *progress) {
    if (!_temporary.isEmpty())
        return E_FAIL;
    // 7zz accepts files only. The source stream and copy algorithm are retained;
    // Use the original temp-name/create service with the final output handle.
    // QTemporaryFile::close can retain its OS handle, preventing a native
    // writable reopen on Windows. This also removes the second file open.
    FString prefix;
    if (!NWindows::NFile::NDir::MyGetTempPath(prefix))
        return GetLastError_noZero_HRESULT();
    prefix += FTEXT("sevenzip-open");
    auto *outputSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> output = outputSpec;
    outputSpec->ProcessedSize = 0;
#ifndef _WIN32
    // Preserve the private spool permissions of the former Qt temp service.
    outputSpec->File.mode_for_Create = 0600;
#endif
    AString postfix;
    if (!NWindows::NFile::NDir::CreateTempFile2(prefix, true, postfix, &outputSpec->File))
        return GetLastError_noZero_HRESULT();
    prefix += postfix;
    const QString path = QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(prefix)));
    std::shared_ptr<QtOwnedTempLease> lease;
    bool completed = false;
    const auto cleanup = qScopeGuard([&] {
        outputSpec->Close();
        if (!completed)
            QFile::remove(path);
    });
    lease = QtOwnedTempRegistry().RegisterCreated(path);
    if (!lease)
        return E_FAIL;
    RINOK(InStream_SeekToBegin(stream))
    CMyComPtr<ICompressProgressInfo> copyProgress = new CSpoolProgress(progress);
    RINOK(NCompress::CopyStream(stream, output, copyProgress))
    RINOK(outputSpec->Close())
    _temporary = path;
    _temporaryLease = std::move(lease);
    EnginePath = path;
    completed = true;
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
