#include "7zip/Common/LimitedStreams.h"
#include "7zip/PropID.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "deviceaccess.h"
#include <QByteArray>
#include <cstring>
#include <limits>

namespace {
class CDeviceReadStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    std::shared_ptr<DeviceReadHandle> handle_;
    quint64 size_;
    quint64 position_ = 0;
    quint32 sector_;

  public:
    CDeviceReadStream(std::shared_ptr<DeviceReadHandle> handle, quint64 size, quint32 sector)
        : handle_(std::move(handle)), size_(size), sector_(sector) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        COM_TRY_BEGIN
        if (processed)
            *processed = 0;
        if (!size || position_ >= size_)
            return S_OK;
        if (!data)
            return E_INVALIDARG;
        const quint32 prefix = quint32(position_ % sector_);
        const quint32 wanted =
            quint32(qMin<quint64>(qMin<UInt32>(size, 1024 * 1024 - prefix), size_ - position_));
        const quint32 aligned = (prefix + wanted + sector_ - 1) / sector_ * sector_;
        QByteArray buffer(qsizetype(aligned) + sector_, Qt::Uninitialized);
        auto *bytes = reinterpret_cast<Byte *>((quintptr(buffer.data()) + sector_ - 1) &
                                               ~(quintptr(sector_) - 1));
        quint32 count = 0;
        RINOK(handle_->readAt(position_ - prefix, bytes, aligned, count))
        if (count > aligned || count <= prefix)
            return E_FAIL; // Unexpected end/disconnect, not successful empty output.
        count = qMin(count - prefix, wanted);
        std::memcpy(data, bytes + prefix, count);
        position_ += count;
        if (processed)
            *processed = count;
        return S_OK;
        COM_TRY_END
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        // The enclosing original CLimitedInStream owns logical seek semantics,
        // including SEEK_END. It seeks its physical input only with SEEK_SET.
        if (origin != STREAM_SEEK_SET || offset < 0)
            return E_INVALIDARG;
        position_ = quint64(offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};

class CDirectDeviceInput final : public IInStream,
                                 public IStreamGetProp,
                                 public DirectDeviceInput,
                                 public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_3(ISequentialInStream, IInStream, IStreamGetProp)
    CMyComPtr<IInStream> input_;
    quint64 size_;

  public:
    const ExternalDevice Device;
    const std::shared_ptr<DeviceReadHandle> Handle;
    const quint64 Start;
    const bool IgnoreJournal;
    CDirectDeviceInput(IInStream *input, quint64 size, ExternalDevice device,
                       std::shared_ptr<DeviceReadHandle> handle, quint64 start, bool ignoreJournal)
        : input_(input), size_(size), Device(std::move(device)), Handle(std::move(handle)),
          Start(start), IgnoreJournal(ignoreJournal) {}
    quint64 deviceInputSize() const override { return size_; }
    bool ignoresDeviceJournal() const override { return IgnoreJournal; }
    Z7_COM7F_IMF(GetProperty(PROPID id, PROPVARIANT *value)) override {
        NWindows::NCOM::CPropVariant prop;
        if (id == kpidSectorSize)
            prop = UInt32(Device.sectorSize);
        prop.Detach(value);
        return S_OK;
    }
    Z7_COM7F_IMF(ReloadProps()) override { return S_OK; }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        return input_->Read(data, size, processed);
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        return input_->Seek(offset, origin, position);
    }
};
} // namespace

namespace {
qint32 CreateInput(const ExternalDevice &device, const std::shared_ptr<DeviceReadHandle> &handle,
                   quint64 offset, quint64 size, IInStream **stream, bool ignoreJournal = false) {
    // Each view has its own logical cursor. Sharing an original limited stream
    // between two open partitions would invalidate its cached physical cursor.
    CMyComPtr<IInStream> native = new CDeviceReadStream(handle, device.size, device.sectorSize);
    CMyComPtr<ISequentialInStream> limited;
    RINOK(CreateLimitedInStream(native, offset, size, &limited))
    CMyComPtr<IInStream> seekable;
    RINOK(limited.QueryInterface(IID_IInStream, &seekable))
    CMyComPtr<IInStream> result =
        new CDirectDeviceInput(seekable, size, device, handle, offset, ignoreJournal);
    *stream = result.Detach();
    return S_OK;
}
} // namespace

qint32 CreateDevicePartitionInput(IInStream *device, quint64 offset, quint64 size,
                                  IInStream **stream) {
    COM_TRY_BEGIN
    if (!stream)
        return E_INVALIDARG;
    *stream = nullptr;
    const auto *source = dynamic_cast<CDirectDeviceInput *>(device);
    if (!source || !size || offset > source->deviceInputSize() ||
        size > source->deviceInputSize() - offset || offset % source->Device.sectorSize ||
        size % source->Device.sectorSize)
        return E_INVALIDARG;
    return CreateInput(source->Device, source->Handle, source->Start + offset, size, stream,
                       source->IgnoreJournal);
    COM_TRY_END
}

qint32 CreateDeviceInputIgnoringJournal(IInStream *device, IInStream **stream) {
    COM_TRY_BEGIN
    if (!stream)
        return E_INVALIDARG;
    *stream = nullptr;
    const auto *source = dynamic_cast<CDirectDeviceInput *>(device);
    if (!source)
        return E_INVALIDARG;
    return CreateInput(source->Device, source->Handle, source->Start, source->deviceInputSize(),
                       stream, true);
    COM_TRY_END
}

qint32 CreateDeviceInput(const ExternalDevice &device, std::shared_ptr<DeviceReadHandle> handle,
                         IInStream **stream) {
    COM_TRY_BEGIN
    if (!stream)
        return E_INVALIDARG;
    *stream = nullptr;
    if (!handle || !device.size || device.size > quint64(std::numeric_limits<Int64>::max()) ||
        device.sectorSize < 512 || device.sectorSize > 64 * 1024 ||
        (device.sectorSize & (device.sectorSize - 1)) || device.size % device.sectorSize)
        return E_INVALIDARG;
    return CreateInput(device, handle, 0, device.size, stream);
    COM_TRY_END
}
