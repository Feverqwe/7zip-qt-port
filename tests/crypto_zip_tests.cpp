#include "7zCrc.h"
#include "7zip/Archive/Zip/ZipHandler.h"
#include "native_archive_fixture.h"
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

class CryptoZipTests final : public QObject {
    Q_OBJECT

    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

  private slots:
    void initTestCase() { CrcGenerateTable(); }
    void officialReaderAcceptsNativeAes_data() {
        QTest::addColumn<QString>("method");
        for (const char *method : {"AES128", "AES192", "AES256"})
            QTest::newRow(method) << QString::fromLatin1(method);
    }
    void officialReaderAcceptsNativeAes() {
        QFETCH(QString, method);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString member = QStringLiteral("данные 空 with spaces.bin");
        const QByteArray payload = QByteArray("payload\0with tail", 17).repeated(2000);
        const QString password(99, 'x');
        const QString archive = fixture.filePath("encrypted.zip");
        CMyComPtr2_Create<IOutArchive, NArchive::NZip::CHandler> writer;
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(writer.Interface()->QueryInterface(IID_ISetProperties, (void **)&properties),
                 S_OK);
        const wchar_t *names[] = {L"x", L"em"};
        NWindows::NCOM::CPropVariant values[] = {
            NWindows::NCOM::CPropVariant(UInt32(0)),
            NWindows::NCOM::CPropVariant(method.toStdWString().c_str())};
        QCOMPARE(properties->SetProperties(names, values, 2), S_OK);
        CMyComPtr2_Create<IOutStream, COutFileStream> output;
        QVERIFY(output->Create_NEW(us2fs(archive.toStdWString().c_str())));
        CMyComPtr<IArchiveUpdateCallback> callback = new NativeArchiveFixture::CWriter(
            {{member, payload}, {"empty.bin", QByteArray()}}, password);
        QCOMPARE(writer.Interface()->UpdateItems(output, 2, callback), S_OK);
        QCOMPARE(output->Close(), S_OK);
        QVERIFY(!read(archive).isEmpty());

        const QString engine = QString::fromUtf8(SEVENZIP_CRYPTO_REFERENCE_7Z);
        QProcess reference;
        reference.start(
            engine, {"x", "-y", "-p" + password, "-o" + fixture.filePath("out"), "--", archive});
        QVERIFY(reference.waitForFinished(10000));
        QCOMPARE(reference.exitStatus(), QProcess::NormalExit);
        QVERIFY2(reference.exitCode() == 0, reference.readAllStandardError().constData());
        QCOMPARE(read(fixture.filePath("out/" + member)), payload);
        QVERIFY(QFileInfo::exists(fixture.filePath("out/empty.bin")));
        QCOMPARE(QFileInfo(fixture.filePath("out/empty.bin")).size(), qint64(0));

        reference.start(engine, {"t", "-pwrong", "--", archive});
        QVERIFY(reference.waitForFinished(10000));
        QCOMPARE(reference.exitStatus(), QProcess::NormalExit);
        QVERIFY(reference.exitCode() != 0);
    }
};

QTEST_GUILESS_MAIN(CryptoZipTests)
#include "crypto_zip_tests.moc"
