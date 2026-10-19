#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QJsonArray>
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
    bool fail = false;
    HRESULT seekResult = S_OK;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (fail)
            return E_FAIL;
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
        if (seekResult != S_OK)
            return seekResult;
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
    Int32 result = -1;
    Extract() { output.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
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
class CancelOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
} // namespace

class ExtStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_EXT_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject bundle(const QString &layout) {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_EXT_PYTHON),
                        {QString::fromUtf8(SEVENZIP_EXT_FIXTURE), layout, "--bundle"});
        if (!generator.waitForFinished(10000) || generator.exitCode() != 0)
            return {};
        return QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
    }
    static QByteArray image(const QJsonObject &record) {
        QByteArray data(4, '\0');
        qToBigEndian(quint32(record["size"].toInt()), data.data());
        data += QByteArray::fromBase64(record["image"].toString().toLatin1());
        return qUncompress(data);
    }
    UInt32 fileIndex() {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UInt32(-1);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QString::fromWCharArray(path.bstrVal) == QStringLiteral("данные 空.bin"))
                return i;
        }
        return UInt32(-1);
    }
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "Ext")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no Ext handler");
    }
  private slots:
    void initTestCase() { createArchive(); }
    void sparseStreamsRetainSourceAfterCloseAndRelease_data() {
        QTest::addColumn<QString>("layout");
        for (const char *layout :
             {"double-hole", "double-subtree", "triple", "triple-hole", "extent-tree"})
            QTest::newRow(layout) << QString::fromLatin1(layout);
    }
    void sparseStreamsRetainSourceAfterCloseAndRelease() {
        QFETCH(QString, layout);
        const auto record = bundle(layout);
        QVERIFY(!record.isEmpty());
        const auto data = image(record);
        CMyComPtr<IInStream> input = new Input(data);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const auto index = fileIndex();
        QVERIFY(index != UInt32(-1));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(index, &sequential), S_OK);
        CMyComPtr<IInStream> stream;
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&stream)),
                 S_OK);
        QCOMPARE(archive_->Close(), S_OK);
        auto changed = data;
        const auto start = changed.indexOf("head");
        if (start >= 0)
            changed[start] = 'Z';
        CMyComPtr<IInStream> second = new Input(changed);
        QCOMPARE(archive_->Open(second, nullptr, nullptr), S_OK);
        provider.Release();
        archive_.Release();
        sequential.Release();
        input.Release();
        for (const auto &entry : record["samples"].toArray()) {
            const auto sample = entry.toObject();
            const auto expected = QByteArray::fromHex(sample["bytes"].toString().toLatin1());
            QCOMPARE(stream->Seek(qint64(sample["offset"].toDouble()), STREAM_SEEK_SET, nullptr),
                     S_OK);
            QByteArray actual;
            while (actual.size() < expected.size()) {
                char bytes[37];
                UInt32 count = 0;
                QCOMPARE(stream->Read(bytes, UInt32(expected.size() - actual.size()), &count),
                         S_OK);
                QVERIFY(count > 0);
                actual.append(bytes, count);
            }
            QCOMPARE(actual, expected);
        }
        createArchive();
    }
    void inputSeekFailuresPropagateThroughExtract_data() {
        QTest::addColumn<QString>("layout");
        QTest::newRow("direct-data-seek") << QString("direct");
        QTest::newRow("indirect-metadata-seek") << QString("double-hole");
    }
    void inputSeekFailuresPropagateThroughExtract() {
        QFETCH(QString, layout);
        const auto record = bundle(layout);
        const auto data = image(record);
        QVERIFY(!data.isEmpty());
        auto *source = new Input(data);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 index = fileIndex();
        QVERIFY(index != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        for (const HRESULT error : {E_ABORT, E_FAIL}) {
            source->seekResult = error;
            QCOMPARE(archive_->Extract(&index, 1, 0, callback), error);
        }
        source->seekResult = S_OK;
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        callback->output->fail = true;
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), E_FAIL);
        callback->output->fail = false;
        callback->output->bytes.clear();
        QCOMPARE(archive_->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        const auto expectedSize = qint64(record["file_size"].toDouble());
        QCOMPARE(callback->output->bytes.size(), expectedSize);
        QCOMPARE(callback->output->bytes.left(4), QByteArray("head"));
        QCOMPARE(callback->output->bytes.right(4), QByteArray("tail"));
    }
    void failedAndCancelledOpenCanBeReused() {
        const auto data = image(bundle("direct"));
        CMyComPtr<IInStream> input = new Input(data);
        CMyComPtr<IArchiveOpenCallback> cancel = new CancelOpen;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        auto *failed = new Input(data);
        input = failed;
        failed->fail = true;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        failed->fail = false;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(fileIndex() != UInt32(-1));
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(ExtStreamTests)
#include "ext_stream_tests.moc"
