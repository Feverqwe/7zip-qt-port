#pragma once
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivedevice.h"
#include "archivenativeopen.h"
#include "btrfs_fixture.h"
#include "device_test_support.h"
#include <QCryptographicHash>
#include <QDir>
#include <QLibrary>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>
namespace {
QString deployedModulePath(const QString &built) {
    const QString runtime = qEnvironmentVariable("SEVENZIP_BTRFS_TEST_RUNTIME");
    return runtime.isEmpty() ? built
                             : QDir(runtime).filePath("Formats/" + QFileInfo(built).fileName());
}
const QString name = QString::fromUtf8("данные 空.bin");
using BtrfsFixture::image;
using BtrfsFixture::manifest;
QByteArray payload() {
    QByteArray bytes(400000, '\0');
    for (qsizetype i = 0; i < bytes.size(); ++i)
        bytes[i] = char((i * 17 + i / 251) % 251);
    return bytes;
}
QByteArray incompatibleImage(quint64 feature) {
    auto bytes = image();
    if (bytes.size() < 65536 + 4096)
        return {};
    char *super = bytes.data() + 65536;
    // Published Btrfs superblock ABI: incompat_flags at 188, csum_type at 196.
    qToLittleEndian(qFromLittleEndian<quint64>(super + 188) | feature, super + 188);
    // A remap-tree filesystem also requires the block-group-tree feature.
    if (feature & (quint64(1) << 17))
        qToLittleEndian(qFromLittleEndian<quint64>(super + 180) | quint64(8), super + 180);
    uint8_t checksum[32];
    if (btrfs_checksum_native(qFromLittleEndian<quint16>(super + 196), super + 32, 4096 - 32,
                              checksum))
        return {};
    memcpy(super, checksum, sizeof(checksum));
    return bytes;
}
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    bool fail = false;
    UInt64 failAt = UINT64_MAX, notifyAt = UINT64_MAX;
    HRESULT atResult = E_FAIL;
    bool notified = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return E_FAIL;
        if (failAt >= position_ && failAt - position_ < size)
            return atResult;
        if (notifyAt >= position_ && notifyAt - position_ < size)
            notified = true;
        size = UInt32(
            qMin<UInt64>(size, position_ < UInt64(bytes_.size()) ? bytes_.size() - position_ : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : bytes_.size();
        if (offset < -base || (offset > 0 && base > INT64_MAX - offset))
            return E_INVALIDARG;
        position_ = UInt64(base + offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
class Output final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray bytes;
    bool fail = false;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return E_FAIL;
        bytes.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    CMyComPtr2<ISequentialOutStream, Output> output;
    bool cancel = false;
    bool cancelAfterOutput = false;
    UInt64 completed = 0;
    Int32 result = -1;
    Extract() { output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        if (value)
            completed = *value;
        return cancel || (cancelAfterOutput && !output->bytes.isEmpty()) ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32 mode)) override {
        *stream = nullptr;
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            *stream = output.Interface();
            (*stream)->AddRef();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        return S_OK;
    }
};
} // namespace
