// File-only 7zz service for the native IInArchive extraction callback contract.
#include "7zip/Archive/IArchive.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Compress/CopyCoder.h"
#include "7zip/IPassword.h"
#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "qtarchivebackendrequest.h"
#include "qtarchivefileresource.h"
#include <QFileInfo>
#include <QTemporaryDir>

namespace {
class CExtractStreamProgress final : public ICompressProgressInfo, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressProgressInfo)
    IArchiveExtractCallback *_callback;
    UInt64 _offset;

  public:
    CExtractStreamProgress(IArchiveExtractCallback *callback, UInt64 offset)
        : _callback(callback), _offset(offset) {}
    Z7_COM7F_IMF(SetRatioInfo(const UInt64 *input, const UInt64 *)) override {
        if (!input)
            return _callback->SetCompleted(nullptr);
        const UInt64 completed = _offset + *input;
        return _callback->SetCompleted(&completed);
    }
};

HRESULT ExtractItems(ArchiveBackend &backend, const QString &archive, const QString &format,
                     IInArchive *metadata, const UInt32 *indices, UInt32 count, Int32 testMode,
                     IArchiveExtractCallback *callback, CProgressSync *sync, bool encrypted) {
    if (!metadata || !callback || (count != UInt32(-1) && count && !indices))
        return E_INVALIDARG;
    QString password;
    if (encrypted) {
        CMyComPtr<ICryptoGetTextPassword> crypto;
        RINOK(callback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&crypto))
        CMyComBSTR value;
        RINOK(crypto->CryptoGetTextPassword(&value))
        password = QString::fromWCharArray(value);
        // Source GetStream can replace an existing file. Verify the password
        // before invoking that callback, as the 7zz file transport opens later.
        RINOK(RunArchiveBackendRequest(
            backend, [&] { backend.testSourceArchive(archive, password); }, callback, sync))
    }
    UInt32 total;
    RINOK(metadata->GetNumberOfItems(&total))
    if (count == UInt32(-1)) {
        count = total;
        indices = nullptr; // Native all-items contract ignores the supplied indices.
    }
    UInt64 unpacked = 0;
    for (UInt32 n = 0; n < count; ++n) {
        const UInt32 index = indices ? indices[n] : n;
        if (index >= total)
            return E_INVALIDARG;
        NWindows::NCOM::CPropVariant size;
        RINOK(metadata->GetProperty(index, kpidSize, &size))
        if (size.vt == VT_UI8) {
            if (size.uhVal.QuadPart > UInt64(-1) - unpacked)
                return E_INVALIDARG;
            unpacked += size.uhVal.QuadPart;
        }
    }
    RINOK(callback->SetTotal(unpacked))
    UInt64 completed = 0;
    QTemporaryDir staging;
    if (!staging.isValid())
        return E_FAIL;
    const Int32 mode =
        testMode ? NArchive::NExtract::NAskMode::kTest : NArchive::NExtract::NAskMode::kExtract;
    for (UInt32 n = 0; n < count; ++n) {
        const UInt32 index = indices ? indices[n] : n;
        RINOK(callback->SetCompleted(&completed))
        NWindows::NCOM::CPropVariant path, directory, size;
        RINOK(metadata->GetProperty(index, kpidPath, &path))
        RINOK(metadata->GetProperty(index, kpidIsDir, &directory))
        RINOK(metadata->GetProperty(index, kpidSize, &size))
        if (path.vt != VT_BSTR || directory.vt != VT_BOOL)
            return E_FAIL;
        const QString name = QString::fromWCharArray(path.bstrVal);
        // CLI selects names rather than archive indices. Ambiguous duplicate
        // entries require the codec/index transport; never concatenate them.
        for (UInt32 other = 0; other < total; ++other) {
            if (other == index)
                continue;
            NWindows::NCOM::CPropVariant otherPath;
            RINOK(metadata->GetProperty(other, kpidPath, &otherPath))
            if (otherPath.vt == VT_BSTR && name == QString::fromWCharArray(otherPath.bstrVal))
                return E_NOTIMPL;
        }
        CMyComPtr<ISequentialOutStream> output;
        RINOK(callback->GetStream(index, &output, mode))
        if (!output && !testMode && directory.boolVal == VARIANT_FALSE) {
            if (size.vt == VT_UI8)
                completed += size.uhVal.QuadPart;
            continue; // Native overwrite/skip decision.
        }
        RINOK(callback->PrepareOperation(mode))
        HRESULT result = S_OK;
        if (directory.boolVal == VARIANT_FALSE) {
            const QString streamPath = staging.filePath(QString::number(n));
            result = RunArchiveBackendRequest(
                backend,
                [&] { backend.extractSourceEntry(archive, name, streamPath, format, password); },
                callback, sync);
            if (result == S_OK && output) {
                CMyComPtr2<ISequentialInStream, CInFileStream> input;
                input.Create_if_Empty();
                if (!input->Open(us2fs(streamPath.toStdWString().c_str())))
                    result = GetLastError_noZero_HRESULT();
                else {
                    CMyComPtr<ICompressProgressInfo> progress =
                        new CExtractStreamProgress(callback, completed);
                    result = NCompress::CopyStream(input.Interface(), output, progress);
                }
            }
        }
        output.Release();
        // Native engines stop extraction on E_ABORT without reporting a data
        // error for the interrupted item. Preserve that cancellation contract.
        if (result == E_ABORT)
            return E_ABORT;
        const HRESULT reported = callback->SetOperationResult(
            result == S_OK ? NArchive::NExtract::NOperationResult::kOK
                           : NArchive::NExtract::NOperationResult::kDataError);
        if (result != S_OK)
            return result;
        RINOK(reported)
        if (size.vt == VT_UI8)
            completed += size.uhVal.QuadPart;
    }
    RINOK(callback->SetCompleted(&completed))
    return S_OK;
}
} // namespace

void QtArchiveFileResource::AttachExtractService(ArchiveBackend &backend, const QString &format) {
    AttachProgressService(backend);
    _extract = [this, &backend, format](IInArchive *metadata, const UInt32 *indices, UInt32 count,
                                        Int32 testMode, IArchiveExtractCallback *callback) {
        COM_TRY_BEGIN
        return ExtractItems(backend, EnginePath, format, metadata, indices, count, testMode,
                            callback, _sync, Info.encrypted);
        COM_TRY_END
    };
}
