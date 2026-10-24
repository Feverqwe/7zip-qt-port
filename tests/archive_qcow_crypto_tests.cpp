#include "archive_reader_fixture.h"
#include "archivenativeopen.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
class CPassword final : public IOpenCallbackUI {
  public:
    UString Text;
    HRESULT Result = S_OK;
    unsigned Requests = 0;
    HRESULT Open_CheckBreak() override { return S_OK; }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *) override { return S_OK; }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *) override { return S_OK; }
    HRESULT Open_Finished() override { return S_OK; }
    HRESULT Open_CryptoGetTextPassword(BSTR *password) override {
        Requests++;
        if (Result != S_OK)
            return Result;
        return StringToBstr(Text, password);
    }
};
class COutput final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray Bytes;
    HRESULT Failure = S_OK;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (Failure != S_OK)
            return Failure;
        Bytes.append(static_cast<const char *>(data), qsizetype(size));
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class CExtract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    COutput *Output = new COutput;
    CMyComPtr<ISequentialOutStream> Owner = Output;
    Int32 Result = -1;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        CMyComPtr<ISequentialOutStream> copy = Owner;
        *stream = copy.Detach();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        Result = result;
        return S_OK;
    }
};
} // namespace

class ArchiveQcowCryptoTests final : public ArchiveReaderFixture {
    Q_OBJECT
    QJsonArray sets_;
    QByteArray Inflate(const QString &name) {
        QProcess process;
        process.start(engine_, {"x", "-so", "--", QStringLiteral(LAYER_FIXTURE_DIR "/") + name});
        if (!process.waitForFinished(10000) || process.exitCode())
            return {};
        return process.readAllStandardOutput();
    }
    static HRESULT ReadAll(IInStream *stream, QByteArray &bytes) {
        qsizetype offset = 0;
        while (offset < bytes.size()) {
            UInt32 size = 0;
            RINOK(stream->Read(bytes.data() + offset,
                               UInt32(qMin(qsizetype(8191), bytes.size() - offset)), &size))
            if (!size)
                return E_FAIL;
            offset += size;
        }
        return S_OK;
    }
    bool PutParent(const QString &path, const QJsonObject &files) {
        return !files.contains("parent") || Put(QFileInfo(path).dir().filePath("qcow2.qcow2"),
                                                Inflate(files["parent"].toString()));
    }
    void Rows() {
        QTest::addColumn<QJsonObject>("set");
        for (const auto &set : sets_)
            if (!set.toObject().contains("offset"))
                QTest::newRow(qPrintable(set.toObject()["name"].toString())) << set.toObject();
    }
  private slots:
    void initTestCase() {
        InitializeRuntime();
        QFile file(QStringLiteral(LAYER_FIXTURE_DIR "/manifest.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        sets_ = QJsonDocument::fromJson(file.readAll()).object()["encrypted_sets"].toArray();
        QVERIFY(!sets_.isEmpty());
    }
    void decryptedBytes_data() { Rows(); }
    void decryptedBytes() {
        QFETCH(QJsonObject, set);
        const auto files = set["files"].toObject();
        const auto image = Inflate(files["image"].toString());
        const auto expected = Inflate(files["expected"].toString());
        QVERIFY(!image.isEmpty() && !expected.isEmpty());
        QTemporaryDir dir;
        const auto path = dir.filePath(QStringLiteral("шифрованный диск 空.qcow2"));
        QVERIFY(Put(path, image));
        QVERIFY(PutParent(path, files));
        CPassword password;
        password.Text = set["password"].toString().toStdWString().c_str();
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(
            Worker([&] { return backend_->openNativeArchive(path, "QCOW", &password, reader); }),
            S_OK);
        NWindows::NCOM::CPropVariant encrypted, warning;
        QCOMPARE(reader->arc().Archive->GetProperty(0, kpidEncrypted, &encrypted), S_OK);
        QCOMPARE(encrypted.vt, VARTYPE(VT_BOOL));
        QCOMPARE(encrypted.boolVal, VARIANT_TRUE);
        QCOMPARE(reader->arc().Archive->GetArchiveProperty(kpidWarning, &warning), S_OK);
        QCOMPARE(warning.vt, VARTYPE(VT_BSTR));
        QVERIFY(!QString::fromWCharArray(warning.bstrVal).isEmpty());
        CMyComPtr<IInArchiveGetStream> factory;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &factory), S_OK);
        CMyComPtr<IInStream> streams[2];
        for (auto &input : streams) {
            CMyComPtr<ISequentialInStream> sequential;
            QCOMPARE(Worker([&] { return factory->GetStream(0, &sequential); }), S_OK);
            QVERIFY(sequential);
            QCOMPARE(sequential.QueryInterface(IID_IInStream, &input), S_OK);
        }
        for (UInt64 pos : {UInt64(511), UInt64(4093), UInt64(16381), UInt64(32767), UInt64(0)}) {
            for (auto &input : streams) {
                QCOMPARE(input->Seek(Int64(pos), STREAM_SEEK_SET, nullptr), S_OK);
                char bytes[513];
                UInt32 size = 0;
                QCOMPARE(input->Read(bytes, sizeof(bytes), &size), S_OK);
                QVERIFY(size > 0);
                QCOMPARE(QByteArray(bytes, size), expected.mid(qsizetype(pos), size));
            }
        }
        if (files.contains("snapshot")) {
            CMyComPtr<ISequentialInStream> sequential;
            QCOMPARE(Worker([&] { return factory->GetStream(1, &sequential); }), S_OK);
            QVERIFY(sequential);
            CMyComPtr<IInStream> snapshot;
            QCOMPARE(sequential.QueryInterface(IID_IInStream, &snapshot), S_OK);
            const auto saved = Inflate(files["snapshot"].toString());
            QByteArray actual(saved.size(), Qt::Uninitialized);
            QCOMPARE(Worker([&] { return ReadAll(snapshot, actual); }), S_OK);
            QCOMPARE(actual, saved);
        }
        factory.Release();
        QCOMPARE(reader->arc().Archive->Close(), S_OK);
        reader.reset();
        for (auto &input : streams) {
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QByteArray actual(expected.size(), Qt::Uninitialized);
            QCOMPARE(Worker([&] { return ReadAll(input, actual); }), S_OK);
            QCOMPARE(actual, expected);
        }
        // Retained image streams reuse the supplied key without another prompt.
        QCOMPARE(password.Requests, 1u);
        QCOMPARE(Bytes(path), image);
        if (files.contains("parent"))
            QCOMPARE(Bytes(dir.filePath("qcow2.qcow2")), Inflate(files["parent"].toString()));
    }
    void passwordCancellation_data() { Rows(); }
    void highSectorNumberUsesFull64BitIV() {
        const auto set = sets_.last().toObject();
        QVERIFY(set.contains("offset"));
        const auto image = Inflate(set["files"].toObject()["image"].toString());
        QVERIFY(!image.isEmpty());
        QTemporaryDir dir;
        const auto path = dir.filePath("high.qcow2");
        QVERIFY(Put(path, image));
        CPassword password;
        password.Text = set["password"].toString().toStdWString().c_str();
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(
            Worker([&] { return backend_->openNativeArchive(path, "QCOW", &password, reader); }),
            S_OK);
        CMyComPtr<IInArchiveGetStream> factory;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &factory), S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(Worker([&] { return factory->GetStream(0, &sequential); }), S_OK);
        CMyComPtr<IInStream> stream;
        QCOMPARE(sequential.QueryInterface(IID_IInStream, &stream), S_OK);
        QCOMPARE(stream->Seek(set["offset"].toInteger(), STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual(512, Qt::Uninitialized);
        QCOMPARE(Worker([&] { return ReadAll(stream, actual); }), S_OK);
        QCOMPARE(actual, QByteArray(512, char(0xa7)));
        QCOMPARE(Bytes(path), image);
    }
    void wrongPasswordCannotBeAuthenticatedByThisFormat() {
        const auto set = sets_[1].toObject();
        const auto image = Inflate(set["files"].toObject()["image"].toString());
        QVERIFY(!image.isEmpty());
        QTemporaryDir dir;
        const auto path = dir.filePath("unverified.qcow2");
        QVERIFY(Put(path, image));
        CPassword password;
        password.Text = L"deliberately wrong fixture password";
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(
            Worker([&] { return backend_->openNativeArchive(path, "QCOW", &password, reader); }),
            S_OK);
        QVERIFY(!reader->arc().ErrorInfo.WarningMessage.IsEmpty());
        CMyComPtr<IInArchiveGetStream> factory;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &factory), S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(Worker([&] { return factory->GetStream(0, &sequential); }), S_OK);
        CMyComPtr<IInStream> input;
        QCOMPARE(sequential.QueryInterface(IID_IInStream, &input), S_OK);
        const auto expected = Inflate(set["files"].toObject()["expected"].toString());
        QByteArray actual(expected.size(), Qt::Uninitialized);
        QCOMPARE(Worker([&] { return ReadAll(input, actual); }), S_OK);
        QVERIFY(actual != expected);
        QCOMPARE(Bytes(path), image);
    }
    void passwordCancellation() {
        QFETCH(QJsonObject, set);
        const auto image = Inflate(set["files"].toObject()["image"].toString());
        QTemporaryDir dir;
        const auto path = dir.filePath("cancel.qcow2");
        QVERIFY(Put(path, image));
        QVERIFY(PutParent(path, set["files"].toObject()));
        CPassword password;
        password.Result = E_ABORT;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(
            Worker([&] { return backend_->openNativeArchive(path, "QCOW", &password, reader); }),
            E_ABORT);
        QVERIFY(!reader);
        QCOMPARE(Bytes(path), image);
    }
    void extraction_data() {
        QTest::addColumn<int>("result");
        QTest::newRow("success") << int(S_OK);
        QTest::newRow("cancel") << int(E_ABORT);
        QTest::newRow("output-failure") << int(E_FAIL);
    }
    void extraction() {
        QFETCH(int, result);
        const auto set = sets_[1].toObject();
        const auto image = Inflate(set["files"].toObject()["image"].toString());
        QTemporaryDir dir;
        const auto path = dir.filePath("extract.qcow2");
        QVERIFY(Put(path, image));
        CPassword password;
        password.Text = set["password"].toString().toStdWString().c_str();
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(
            Worker([&] { return backend_->openNativeArchive(path, "QCOW", &password, reader); }),
            S_OK);
        auto *callback = new CExtract;
        CMyComPtr<IArchiveExtractCallback> owner = callback;
        callback->Output->Failure = HRESULT(result);
        QCOMPARE(Worker([&] {
                     return reader->arc().Archive->Extract(nullptr, UInt32(-1), false, callback);
                 }),
                 HRESULT(result));
        if (result == int(S_OK)) {
            QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(callback->Output->Bytes,
                     Inflate(set["files"].toObject()["expected"].toString()));
        } else
            QCOMPARE(callback->Result, -1);
        QCOMPARE(Bytes(path), image);
    }
};

QTEST_MAIN(ArchiveQcowCryptoTests)
#include "archive_qcow_crypto_tests.moc"
