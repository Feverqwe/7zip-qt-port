#include "7zip/Common/StreamObjects.h"
#include "Windows/PropVariant.h"
#include "archive_backend_test_support.h"
#include "archivecodeccatalog.h"
#include "archivenativeopen.h"
#include <QStandardPaths>

namespace {
class CPathUpdateCallback final : public IArchiveUpdateCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveUpdateCallback)
    CMyComPtr<IInArchive> archive_;
    UString path_;
    bool stream_;

  public:
    CPathUpdateCallback(IInArchive *archive, const QString &path, bool stream)
        : archive_(archive), path_(path.toStdWString().c_str()), stream_(stream) {}
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetUpdateItemInfo(UInt32, Int32 *data, Int32 *props, UInt32 *index)) override {
        *data = 0;
        *props = 1;
        *index = 0;
        return S_OK;
    }
    Z7_COM7F_IMF(GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) override {
        NWindows::NCOM::CPropVariant property;
        if (id == kpidPath)
            property = path_;
        else if (id == kpidIsAltStream)
            property = stream_;
        else
            return archive_->GetProperty(index, id, value);
        property.Detach(value);
        return S_OK;
    }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialInStream **stream)) override {
        *stream = nullptr;
        return S_OK;
    }
    Z7_COM7F_IMF(SetOperationResult(Int32)) override { return S_OK; }
};
} // namespace

class NativeUpdatePathTests final : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native update path checks unverified");
#else
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        version_ = backend.codecCatalog()->version();
#endif
    }
    void malformedStreamPathDoesNotReachWriter_data() {
        QTest::addColumn<QString>("path");
        for (const QString &path :
             {"../outside:stream", "/absolute:stream", "safe/../file:stream", "safe:stream/child",
              "safe:stream\\child", "safe:stream:other", "safe:", "safe"})
            QTest::newRow(qPrintable(path)) << path;
    }
    void malformedStreamPathDoesNotReachWriter() {
        QFETCH(QString, path);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        writeFile(fixture.filePath("seed.bin"), "original bytes");
        const QString archive = fixture.filePath("original.zip");
        QProcess creator;
        creator.setWorkingDirectory(fixture.path());
        creator.start(engine_, {"a", "-tzip", "--", archive, "seed.bin"});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        QFile original(archive);
        QVERIFY(original.open(QIODevice::ReadOnly));
        const QByteArray before = original.readAll();
        original.close();
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(backend.openNativeArchive(archive, "zip", nullptr, reader), S_OK);
        QVERIFY(reader);
        CMyComPtr<IArchiveUpdateCallback> callback =
            new CPathUpdateCallback(reader->arc().Archive, path, true);
        CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> output;
        QCOMPARE(backend.updateNativeArchive(reader, output, 1, callback), E_INVALIDARG);
        QCOMPARE(output->GetSize(), size_t(0));
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), before);
    }
};
QTEST_MAIN(NativeUpdatePathTests)
#include "native_update_path_tests.moc"
