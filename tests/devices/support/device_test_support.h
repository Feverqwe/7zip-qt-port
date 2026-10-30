#pragma once
#include "7zip/Common/StreamObjects.h"
#include "archive/codecs/archivecodeccatalog.h"
#include "devices/deviceaccess.h"
#include <QByteArray>
#include <QString>
#include <atomic>

namespace {
class CDeviceInput final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    CMyComPtr<IInStream> stream_;
    UInt64 position_ = 0;

  public:
    std::atomic_bool Disconnected{false};
    UInt64 FarthestRead = 0;
    explicit CDeviceInput(const QByteArray &data) {
        auto *input = new CBufInStream;
        stream_ = input;
        input->Init(reinterpret_cast<const Byte *>(data.constData()), size_t(data.size()));
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (Disconnected)
            return E_FAIL;
        UInt32 count = 0;
        const HRESULT result = stream_->Read(data, size, &count);
        position_ += count;
        FarthestRead = qMax(FarthestRead, position_);
        if (processed)
            *processed = count;
        return result;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        const HRESULT result = stream_->Seek(offset, origin, &position_);
        if (position)
            *position = position_;
        return result;
    }
};

class CAlignedHandle final : public DeviceReadHandle {
    CMyComPtr<IInStream> input_;
    quint32 sector_;

  public:
    explicit CAlignedHandle(IInStream *input, quint32 sector) : input_(input), sector_(sector) {}
    qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) override {
        if (offset % sector_ || size % sector_ || quintptr(data) % sector_)
            return E_INVALIDARG;
        RINOK(input_->Seek(Int64(offset), STREAM_SEEK_SET, nullptr))
        return input_->Read(data, size, &read);
    }
};

HRESULT DeviceInput(IInStream *input, quint64 size, IInStream **stream, quint32 sector = 4096) {
    ExternalDevice description;
    description.size = size;
    description.sectorSize = sector;
    return CreateDeviceInput(description, std::make_shared<CAlignedHandle>(input, sector), stream);
}

class COpenUi final : public IOpenCallbackUI {
  public:
    bool Cancel = false;
    QString Password;
    HRESULT PasswordResult = E_ABORT;
    HRESULT Open_CheckBreak() override { return Cancel ? E_ABORT : S_OK; }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_Finished() override { return S_OK; }
    HRESULT Open_CryptoGetTextPassword(BSTR *value) override {
        *value = nullptr;
        if (PasswordResult != S_OK)
            return PasswordResult;
        const auto text = Password.toStdWString();
        *value = SysAllocStringLen(text.data(), (UINT)text.size());
        return *value ? S_OK : E_OUTOFMEMORY;
    }
};

class CExtractUi final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)

  public:
    bool Cancel = false;
    Int32 ItemResult = -1;
    CDynBufSeqOutStream *Output = new CDynBufSeqOutStream;
    CMyComPtr<ISequentialOutStream> Lease = Output;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = Lease;
        (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        ItemResult = result;
        return S_OK;
    }
    QByteArray bytes() const {
        return QByteArray(reinterpret_cast<const char *>(Output->GetBuffer()),
                          qsizetype(Output->GetSize()));
    }
};
} // namespace
