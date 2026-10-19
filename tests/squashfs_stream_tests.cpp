#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QtEndian>
#include <QtTest>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    HRESULT failure = S_OK;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
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
} // namespace

class SquashfsStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_SQUASHFS_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject record_;
    QByteArray bytes_;

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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "SquashFS")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no SquashFS handler");
    }
    UInt32 index(const QString &name) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UInt32(-1);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QString::fromWCharArray(path.bstrVal) == name)
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
  private slots:
    void initTestCase() {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_SQUASHFS_PYTHON),
                        {QString::fromUtf8(SEVENZIP_SQUASHFS_FIXTURE), "--compressed"});
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        record_ = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QVERIFY(!record_.isEmpty());
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record_["image"].toString().toLatin1());
        bytes_ = qUncompress(packed);
        QVERIFY(!bytes_.isEmpty());
        createArchive();
    }
    void interleavedStreamsSurviveCloseReopenAndRelease() {
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto first = stream("alpha");
        auto second = stream(QStringLiteral("данные 空"));
        auto link = stream("link");
        QVERIFY(first && second && link);
        check(first, "alpha", 0, 53);
        check(second, QStringLiteral("данные 空"), 0, 53);
        check(first, "alpha", 8192, 10);
        check(second, QStringLiteral("данные 空"), 16384, 11);
        QCOMPARE(archive_->Close(), S_OK);
        CMyComPtr<IInStream> replacement = new Input(bytes_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        check(link, "link", 0, 5);
        check(second, QStringLiteral("данные 空"), 8192, 8192);
        check(first, "alpha", 8175, 27);
        check(second, QStringLiteral("данные 空"), 0, 8200);
        createArchive();
    }
    void sourceErrorsAndCancellationPropagateWithoutPoisoningRetry() {
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 item = index("alpha");
        QVERIFY(item != UInt32(-1));
        CMyComPtr<IArchiveExtractCallback> callback = new Extract;
        for (HRESULT error : {E_ABORT, E_FAIL}) {
            source->failure = error;
            QCOMPARE(archive_->Extract(&item, 1, 1, callback), error);
        }
        source->failure = S_OK;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QCOMPARE(static_cast<Extract *>(callback.Interface())->result,
                 Int32(NArchive::NExtract::NOperationResult::kOK));
        static_cast<Extract *>(callback.Interface())->cancel = true;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), E_ABORT);
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(SquashfsStreamTests)
#include "squashfs_stream_tests.moc"
