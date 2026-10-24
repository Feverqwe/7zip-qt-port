#include "7zCrc.h"
#include "7zip/Archive/Zip/ZipHandler.h"
#include "native_archive_fixture.h"
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QtTest>
#include <openssl/evp.h>

class CryptoZipTests final : public QObject {
    Q_OBJECT

    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

  private slots:
    void initTestCase() { CrcGenerateTable(); }
    void native_runtime_rejects_unavailable_crypto() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile config(fixture.filePath("openssl.cnf"));
        QVERIFY(config.open(QIODevice::WriteOnly));
        config.write("openssl_conf = init\n[init]\nalg_section = algorithms\n"
                     "[algorithms]\ndefault_properties = provider=sevenzip_missing\n");
        config.close();
        QFile input(fixture.filePath(QStringLiteral("данные with spaces.bin")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        input.write("private payload");
        input.close();
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert("OPENSSL_CONF", config.fileName());
        for (const QString &format : {QString("zip"), QString("7z")}) {
            QProcess process;
            process.setProcessEnvironment(environment);
            process.setWorkingDirectory(fixture.path());
            QStringList arguments = {"a", "-t" + format, "-psecret", "-mx=0"};
            if (format == "zip")
                arguments << "-mem=AES256";
            arguments << "--" << fixture.filePath("failed." + format) << input.fileName();
            process.start(QString::fromUtf8(SEVENZIP_CRYPTO_REFERENCE_7Z), arguments);
            QVERIFY(process.waitForFinished(10000));
            QCOMPARE(process.exitStatus(), QProcess::NormalExit);
            QVERIFY2(process.exitCode() != 0, process.readAllStandardOutput().constData());
        }
    }
    void unavailable_derivation_aborts_archive() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CMyComPtr2_Create<IOutArchive, NArchive::NZip::CHandler> writer;
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(writer.Interface()->QueryInterface(IID_ISetProperties, (void **)&properties),
                 S_OK);
        const wchar_t *names[] = {L"em"};
        NWindows::NCOM::CPropVariant value(L"AES256");
        QCOMPARE(properties->SetProperties(names, &value, 1), S_OK);
        CMyComPtr2_Create<IOutStream, COutFileStream> output;
        QVERIFY(output->Create_NEW(us2fs(fixture.filePath("failed.zip").toStdWString().c_str())));
        CMyComPtr<IArchiveUpdateCallback> callback = new NativeArchiveFixture::CWriter(
            {{QStringLiteral("данные with spaces.bin"), QByteArray("secret")}}, "password");
        QCOMPARE(EVP_set_default_properties(nullptr, "provider=sevenzip_missing"), 1);
        const HRESULT status = writer.Interface()->UpdateItems(output, 1, callback);
        QCOMPARE(EVP_set_default_properties(nullptr, nullptr), 1);
        QCOMPARE(status, E_FAIL);
    }
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
