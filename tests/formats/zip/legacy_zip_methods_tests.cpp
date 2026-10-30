#include "7zip/Archive/Zip/ZipHandler.h"
#include "7zip/Common/StreamObjects.h"
#include "C/7zCrc.h"
#include "archive/support/native_archive_fixture.h"
#include <QDataStream>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

namespace {
QByteArray archive(quint16 method, const QByteArray &packed, const QByteArray &plain) {
    const QByteArray name = QStringLiteral("данные 空 with spaces.txt").toUtf8();
    const quint32 crc = CrcCalc(plain.constData(), size_t(plain.size()));
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out << quint32(0x04034b50) << quint16(63) << quint16(0x800) << method << quint16(0)
        << quint16(0) << crc << quint32(packed.size()) << quint32(plain.size())
        << quint16(name.size()) << quint16(0);
    out.writeRawData(name.constData(), name.size());
    out.writeRawData(packed.constData(), packed.size());
    const quint32 central = bytes.size();
    out << quint32(0x02014b50) << quint16(63) << quint16(63) << quint16(0x800) << method
        << quint16(0) << quint16(0) << crc << quint32(packed.size()) << quint32(plain.size())
        << quint16(name.size()) << quint16(0) << quint16(0) << quint16(0) << quint16(0)
        << quint32(0) << quint32(0);
    out.writeRawData(name.constData(), name.size());
    const quint32 centralSize = quint32(bytes.size()) - central;
    out << quint32(0x06054b50) << quint16(0) << quint16(0) << quint16(1) << quint16(1)
        << centralSize << central << quint16(0);
    return bytes;
}

class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    Int32 Result = -1;
    CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> Output;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = Output;
        (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        Result = result;
        return S_OK;
    }
};
} // namespace

class LegacyZipMethodsTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { CrcGenerateTable(); }
    void readsLegacyEntries_data() {
        QTest::addColumn<quint16>("method");
        QTest::addColumn<QByteArray>("packed");
        QTest::addColumn<QByteArray>("plain");
        QTest::newRow("shrink") << quint16(1) << QByteArray::fromHex("4100") << QByteArray("A");
        QTest::newRow("implode") << quint16(6) << QByteArray::fromHex("03f5f5f5f503f5f5f5f58300")
                                 << QByteArray("A");
        // Produced by the baseline 7-Zip PPMd ZIP encoder from 30,000 'A' bytes.
        QTest::newRow("ppmd") << quint16(98)
                              << QByteArray::fromHex("070040fdc83f0000000000000000007ec2009ee000")
                              << QByteArray(30000, 'A');
        QTest::newRow("lzfse") << quint16(99)
                               << QByteArray::fromHex(
                                      "6276786e010000000a000000e141060000000000000062767824")
                               << QByteArray("A");
        QTest::newRow("zstd-method-20")
            << quint16(20) << QByteArray::fromHex("28b52ffd200109000041") << QByteArray("A");
    }
    void readsLegacyEntries() {
        QFETCH(quint16, method);
        QFETCH(QByteArray, packed);
        QFETCH(QByteArray, plain);
        const QByteArray image = archive(method, packed, plain);
        CMyComPtr2_Create<IInStream, CBufInStream> input;
        input->Init(reinterpret_cast<const Byte *>(image.constData()), image.size());
        CMyComPtr<IInArchive> reader = new NArchive::NZip::CHandler;
        QCOMPARE(reader->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        const UInt32 index = 0;
        QCOMPARE(reader->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(callback->Output->GetBuffer()),
                            qsizetype(callback->Output->GetSize())),
                 plain);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile file(dir.filePath(QStringLiteral("архив 空.zip")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(image), qint64(image.size()));
        file.close();
        QProcess engine;
        engine.start(QString::fromUtf8(SEVENZIP_ZIP_VERIFY_ENGINE),
                     {"x", "-so", "-bso0", "-bse2", "--", file.fileName()});
        QVERIFY(engine.waitForFinished(10000));
        QCOMPARE(engine.exitStatus(), QProcess::NormalExit);
        QCOMPARE(engine.exitCode(), 0);
        QCOMPARE(engine.readAllStandardOutput(), plain);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), image);

        // A decodable payload with a mismatched recorded CRC must not become
        // a successful extraction when restoring an old compression method.
        QByteArray wrongPlain = plain;
        wrongPlain[0] = char(wrongPlain[0] ^ 1);
        const QByteArray badCrcImage = archive(method, packed, wrongPlain);
        input->Init(reinterpret_cast<const Byte *>(badCrcImage.constData()), badCrcImage.size());
        QCOMPARE(reader->Open(input, nullptr, nullptr), S_OK);
        callback.SetFromCls(new Extract);
        QCOMPARE(reader->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(callback->Result, Int32(NArchive::NExtract::NOperationResult::kCRCError));
    }
    void removedWriterMethods_data() {
        QTest::addColumn<QString>("method");
        for (const QString &method :
             {"Shrink", "Implode", "PPMd", "LZFSE", "1", "6", "20", "98", "99"})
            QTest::newRow(qPrintable(method)) << method;
    }
    void removedWriterMethods() {
        QFETCH(QString, method);
        CMyComPtr<IOutArchive> writer = new NArchive::NZip::CHandler;
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(writer->QueryInterface(IID_ISetProperties, (void **)&properties), S_OK);
        const wchar_t *name = L"m";
        const auto wide = method.toStdWString();
        NWindows::NCOM::CPropVariant value(wide.c_str());
        QCOMPARE(properties->SetProperties(&name, &value, 1), S_OK);
        CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> output;
        CMyComPtr<IArchiveUpdateCallback> callback = new NativeArchiveFixture::CWriter(
            {{QStringLiteral("данные 空.txt"), QByteArray(30000, 'A')}}, QString());
        QCOMPARE(writer->UpdateItems(output, 1, callback), E_NOTIMPL);
        QCOMPARE(output->GetSize(), size_t(0));
    }
    void sidecarRejectsPpmdCreation() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString inputPath = directory.filePath(QStringLiteral("данные 空.txt"));
        QFile input(inputPath);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write(QByteArray(30000, 'A')), qint64(30000));
        input.close();
        const QString archivePath = directory.filePath(QStringLiteral("архив 空.zip"));
        QProcess engine;
        engine.start(QString::fromUtf8(SEVENZIP_ZIP_VERIFY_ENGINE),
                     {"a", "-tzip", "-mm=PPMd", "-bso0", "-bse2", "--", archivePath, inputPath});
        QVERIFY(engine.waitForFinished(10000));
        QCOMPARE(engine.exitStatus(), QProcess::NormalExit);
        QVERIFY(engine.exitCode() != 0);
        QVERIFY(engine.readAllStandardError().contains("Not implemented"));
        QFile output(archivePath);
        QVERIFY(!output.exists() || output.size() == 0);
        QVERIFY(input.open(QIODevice::ReadOnly));
        QCOMPARE(input.readAll(), QByteArray(30000, 'A'));
    }
};

QTEST_GUILESS_MAIN(LegacyZipMethodsTests)
#include "legacy_zip_methods_tests.moc"
