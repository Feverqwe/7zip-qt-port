#include "archive/resources/archivenativeopen.h"
#include "archive/support/archive_reader_fixture.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

namespace {
class CImageOutput final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray Bytes;
    HRESULT WriteResult = S_OK;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (WriteResult != S_OK)
            return WriteResult;
        Bytes.append(static_cast<const char *>(data), qsizetype(size));
        if (processed)
            *processed = size;
        return S_OK;
    }
};

class CImageExtract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    CImageOutput *Output = new CImageOutput;
    CMyComPtr<ISequentialOutStream> Owner = Output;
    Int32 Result = NArchive::NExtract::NOperationResult::kDataError;
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

class ArchiveQcowReferencesTests final : public ArchiveReaderFixture {
    Q_OBJECT
    QJsonArray sets_;

    QByteArray Inflate(const QString &fixture) {
        QProcess process;
        process.start(engine_, {"x", "-so", "--", QStringLiteral(LAYER_FIXTURE_DIR "/") + fixture});
        if (!process.waitForFinished(10000) || process.exitCode())
            return {};
        return process.readAllStandardOutput();
    }

    bool Materialize(const QString &root, const QJsonObject &set,
                     QMap<QString, QByteArray> &originals) {
        const auto files = set["files"].toObject();
        for (auto file = files.begin(); file != files.end(); ++file) {
            const auto path = QDir(root).filePath(file.key());
            const auto bytes = Inflate(file.value().toString());
            if (bytes.isEmpty() || !QDir().mkpath(QFileInfo(path).absolutePath()) ||
                !Put(path, bytes))
                return false;
            originals[path] = bytes;
        }
        return true;
    }

    HRESULT ReadImage(const QString &path, qsizetype size, QByteArray &actual) {
        std::shared_ptr<ArchiveNativeOpen> reader;
        RINOK(backend_->openNativeArchive(path, "QCOW", nullptr, reader))
        if (!reader)
            return E_FAIL;
        CMyComPtr<IInArchiveGetStream> streams;
        RINOK(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &streams))
        CMyComPtr<ISequentialInStream> input;
        RINOK(streams->GetStream(0, &input))
        if (!input)
            return E_FAIL;
        actual.resize(size);
        qsizetype offset = 0;
        while (offset < size) {
            UInt32 count = 0;
            RINOK(input->Read(actual.data() + offset, UInt32(qMin(qsizetype(8191), size - offset)),
                              &count))
            if (!count)
                return E_FAIL;
            offset += count;
        }
        CMyComPtr<IInStream> seekable;
        RINOK(input.QueryInterface(IID_IInStream, &seekable))
        // Cross a subcluster, explicit zero and the shorter raw parent's EOF,
        // then seek backwards through the retained chain.
        for (UInt64 position : {UInt64(511), UInt64(1023), UInt64(65535), UInt64(0)}) {
            RINOK(seekable->Seek(Int64(position), STREAM_SEEK_SET, nullptr))
            char bytes[513];
            UInt32 count = 0;
            RINOK(input->Read(bytes, sizeof(bytes), &count))
            if (!count || QByteArray(bytes, count) != actual.mid(qsizetype(position), count))
                return E_FAIL;
        }
        return S_OK;
    }

    static void SetBackingName(QByteArray &image, const QByteArray &name) {
        const auto offset = qFromBigEndian<quint64>(image.constData() + 8);
        qToBigEndian(quint32(name.size()), image.data() + 16);
        memcpy(image.data() + offset, name.constData(), size_t(name.size()));
    }

  private slots:
    void initTestCase() {
        InitializeRuntime();
        QFile file(QStringLiteral(LAYER_FIXTURE_DIR "/manifest.json"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        sets_ = QJsonDocument::fromJson(file.readAll()).object()["reference_sets"].toArray();
        QVERIFY(!sets_.isEmpty());
    }
    void dependentDiskBytes_data() {
        QTest::addColumn<QJsonObject>("set");
        for (const auto &set : sets_)
            QTest::newRow(qPrintable(set.toObject()["name"].toString())) << set.toObject();
    }
    void dependentDiskBytes() {
        QFETCH(QJsonObject, set);
        QTemporaryDir directory;
        QMap<QString, QByteArray> originals;
        QVERIFY(Materialize(directory.path(), set, originals));
        const auto expected = Inflate(set["expected"].toString());
        QVERIFY(!expected.isEmpty());
        QByteArray actual;
        const auto path = directory.filePath(set["entry"].toString());
        QCOMPARE(Worker([&] { return ReadImage(path, expected.size(), actual); }), S_OK);
        QCOMPARE(actual, expected);
        for (auto file = originals.begin(); file != originals.end(); ++file)
            QCOMPARE(Bytes(file.key()), file.value());
    }
    void unavailableDependencyDoesNotReturnSuccess_data() {
        QTest::addColumn<QJsonObject>("set");
        for (const auto &set : sets_)
            QTest::newRow(qPrintable(set.toObject()["name"].toString())) << set.toObject();
    }
    void unavailableDependencyDoesNotReturnSuccess() {
        QFETCH(QJsonObject, set);
        QTemporaryDir directory;
        QMap<QString, QByteArray> originals;
        QVERIFY(Materialize(directory.path(), set, originals));
        const auto files = set["files"].toObject();
        QString dependency;
        for (auto file = files.begin(); file != files.end(); ++file)
            if (file.key().endsWith(".raw"))
                dependency = directory.filePath(file.key());
        QVERIFY(!dependency.isEmpty());
        QVERIFY(QFile::remove(dependency));
        QByteArray actual;
        const auto path = directory.filePath(set["entry"].toString());
        QVERIFY(Worker([&] { return ReadImage(path, 4096, actual); }) != S_OK);
        QCOMPARE(Bytes(path), originals[path]);
    }
    void extractDependentImage_data() {
        QTest::addColumn<QJsonObject>("set");
        QTest::addColumn<HRESULT>("writeResult");
        for (const auto &set : sets_)
            for (const auto status : {S_OK, E_ABORT, E_FAIL})
                QTest::newRow(qPrintable(set.toObject()["name"].toString() + '-' +
                                         QString::number(quint32(status), 16)))
                    << set.toObject() << HRESULT(status);
    }
    void extractDependentImage() {
        QFETCH(QJsonObject, set);
        QFETCH(HRESULT, writeResult);
        QTemporaryDir directory;
        QMap<QString, QByteArray> originals;
        QVERIFY(Materialize(directory.path(), set, originals));
        const auto path = directory.filePath(set["entry"].toString());
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); }),
                 S_OK);
        QVERIFY(reader);
        auto *observer = new CImageExtract;
        CMyComPtr<IArchiveExtractCallback> callback = observer;
        observer->Output->WriteResult = writeResult;
        const UInt32 index = 0;
        const auto result =
            Worker([&] { return reader->arc().Archive->Extract(&index, 1, 0, callback); });
        QCOMPARE(result, writeResult);
        if (writeResult == S_OK) {
            QCOMPARE(observer->Result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(observer->Output->Bytes, Inflate(set["expected"].toString()));
        } else
            QVERIFY(observer->Result != NArchive::NExtract::NOperationResult::kOK);
        for (auto file = originals.begin(); file != originals.end(); ++file)
            QCOMPARE(Bytes(file.key()), file.value());
    }
    void cyclicOrEscapingBackingIsRejected_data() {
        QTest::addColumn<bool>("escaping");
        QTest::newRow("cycle") << false;
        QTest::newRow("outside selected directory") << true;
    }
    void cyclicOrEscapingBackingIsRejected() {
        QFETCH(bool, escaping);
        const auto set = sets_.first().toObject();
        QTemporaryDir directory;
        const auto root = directory.filePath("selected");
        QMap<QString, QByteArray> originals;
        QVERIFY(Materialize(root, set, originals));
        const auto path = QDir(root).filePath(set["entry"].toString());
        auto image = Bytes(path);
        const auto name =
            escaping ? QByteArray("../outside.qcow2") : set["entry"].toString().toUtf8();
        SetBackingName(image, name);
        QVERIFY(Put(path, image));
        // Use the declared backing type: an unsafe opener would succeed,
        // rather than fail later because a raw file is not a QCOW image.
        const auto outside = Inflate(QStringLiteral("stored.qcow2.gz"));
        QVERIFY(!outside.isEmpty());
        QVERIFY(Put(directory.filePath("outside.qcow2"), outside));
        QByteArray actual;
        QVERIFY(Worker([&] { return ReadImage(path, 4096, actual); }) != S_OK);
        QCOMPARE(Bytes(path), image);
        QCOMPARE(Bytes(directory.filePath("outside.qcow2")), outside);
    }
    void externalOffsetMustMatchGuest() {
        const auto set = sets_[1].toObject();
        QTemporaryDir directory;
        QMap<QString, QByteArray> originals;
        QVERIFY(Materialize(directory.path(), set, originals));
        const auto path = directory.filePath(set["entry"].toString());
        auto image = Bytes(path);
        const auto l1 = qFromBigEndian<quint64>(image.constData() + 40);
        const auto l2 =
            qFromBigEndian<quint64>(image.constData() + l1) & Q_UINT64_C(0x00fffffffffffe00);
        qToBigEndian((quint64(1) << 63) | 4096, image.data() + l2);
        QVERIFY(Put(path, image));
        QByteArray actual;
        QVERIFY(Worker([&] { return ReadImage(path, 4096, actual); }) != S_OK);
    }
};

QTEST_MAIN(ArchiveQcowReferencesTests)
#include "archive_qcow_references_tests.moc"
