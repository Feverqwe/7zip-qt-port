#pragma once
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QtEndian>
#include <QtTest>

namespace NtfsTestSupport {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    HRESULT failure = S_OK;
    HRESULT seekFailure = S_OK;
    bool eof = false;
    UInt64 failSeekAt = UINT64_MAX;
    UInt64 cancelAt = UINT64_MAX;
    bool cancelRequested = false;
    HRESULT failedSeekResult = E_FAIL;
    UInt64 failReadAt = UINT64_MAX, failReadEnd = UINT64_MAX;
    UInt32 failReadMinSize = 1, failedReads = 0;
    HRESULT failedReadResult = E_FAIL;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ <= cancelAt && cancelAt - position_ < size)
            cancelRequested = true;
        if (failure != S_OK)
            return failure;
        if (eof)
            return S_OK;
        if (size >= failReadMinSize && position_ < failReadEnd &&
            (position_ >= failReadAt || failReadAt - position_ < size)) {
            ++failedReads;
            return failedReadResult;
        }
        size = UInt32(qMin<UInt64>(
            size, position_ < UInt64(bytes_.size()) ? UInt64(bytes_.size()) - position_ : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (failure != S_OK)
            return failure;
        if (origin == STREAM_SEEK_SET && UInt64(offset) == failSeekAt)
            return failedSeekResult;
        if (seekFailure != S_OK) {
            const HRESULT result = seekFailure;
            seekFailure = S_OK;
            return result;
        }
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
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    bool cancel = false;
    Int32 result = -1;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = nullptr;
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        return S_OK;
    }
};
class CancelOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    HRESULT failure = E_ABORT;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return failure; }
};
class CancelEaOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    const Input *source = nullptr;
    HRESULT failure = E_ABORT;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source && source->cancelRequested ? failure : S_OK;
    }
};
} // namespace NtfsTestSupport

namespace NtfsTestSupport {
class Fixture {
  protected:
    QLibrary module_{QString::fromUtf8(SEVENZIP_NTFS_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject record_;
    QByteArray bytes_;
    QHash<QString, QJsonObject> fixtures_;

    void createArchive() {
        archive_.Release();
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto count =
            reinterpret_cast<Func_GetNumberOfFormats>(module_.resolve("GetNumberOfFormats"));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(count && property && create);
        UInt32 formats = 0;
        QCOMPARE(count(&formats), S_OK);
        for (UInt32 i = 0; i < formats; ++i) {
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "NTFS")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no NTFS handler");
    }
    UInt32 index(const QString &name) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UInt32(-1);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)) == name)
                return i;
        }
        return UInt32(-1);
    }
    CMyComPtr<IInStream> stream(const QString &name) {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        CMyComPtr<ISequentialInStream> sequential;
        if (provider->GetStream(index(name), &sequential) != S_OK || !sequential)
            return {};
        CMyComPtr<IInStream> result;
        sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
        return result;
    }
    void check(IInStream *stream, const QString &name, int offset, int size) {
        QVERIFY2(stream, qPrintable(name));
        const auto expected =
            QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1())
                .mid(offset, size);
        QCOMPARE(stream->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual;
        while (actual.size() < expected.size()) {
            char data[37];
            UInt32 count = 0;
            QCOMPARE(
                stream->Read(data,
                             UInt32(qMin(qsizetype(sizeof(data)), expected.size() - actual.size())),
                             &count),
                S_OK);
            QVERIFY(count > 0);
            actual.append(data, count);
        }
        QCOMPARE(actual, expected);
    }
    void loadFixture(const QString &codec) {
        if (!fixtures_.contains(codec)) {
            QProcess generator;
            QStringList arguments{QString::fromUtf8(SEVENZIP_NTFS_FIXTURE), "--kind=" + codec};
            generator.start(QString::fromUtf8(SEVENZIP_NTFS_PYTHON), arguments);
            QVERIFY(generator.waitForFinished(10000));
            QCOMPARE(generator.exitCode(), 0);
            const auto record = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
            QVERIFY(!record.isEmpty());
            fixtures_.insert(codec, record);
        }
        record_ = fixtures_.value(codec);
        QVERIFY(!record_.isEmpty());
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record_["image"].toString().toLatin1());
        bytes_ = qUncompress(packed);
        QVERIFY(!bytes_.isEmpty());
    }
    bool readLimitations(QString &text) {
        text.clear();
        UInt32 count = 0;
        if (archive_->GetNumberOfArchiveProperties(&count) != S_OK)
            return false;
        for (UInt32 i = 0; i < count; ++i) {
            BSTR name = nullptr;
            PROPID id;
            VARTYPE type;
            if (archive_->GetArchivePropertyInfo(i, &name, &id, &type) != S_OK)
                return false;
            const QString label = name ? QString::fromWCharArray(name) : QString();
            SysFreeString(name);
            if (label != "Read-only limitations")
                continue;
            NWindows::NCOM::CPropVariant value;
            if (archive_->GetArchiveProperty(id, &value) != S_OK)
                return false;
            if (value.vt == VT_BSTR)
                text = QString::fromWCharArray(value.bstrVal);
            return value.vt == VT_BSTR || value.vt == VT_EMPTY;
        }
        return false;
    }
};
} // namespace NtfsTestSupport
