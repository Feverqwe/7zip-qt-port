#include "7zCrc.h"
#include "7zip/Archive/Zip/ZipHandler.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Compress/ZstdDecoder.h"
#include "CPP/7zip/Compress/ZstdEncoder.h"
#include "source_creation_fixture.h"
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray incompressibleBytes() {
    QByteArray bytes(1 << 20, Qt::Uninitialized);
    quint32 state = 0x7f4a7c15;
    for (char &byte : bytes) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        byte = char(state);
    }
    return bytes;
}
class CFailedInput final : public ISequentialInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialInStream)
    Z7_COM7F_IMF(Read(void *, UInt32, UInt32 *size)) override {
        *size = 0;
        return E_FAIL;
    }
};
class CFailedOutput final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
    Z7_COM7F_IMF(Write(const void *, UInt32, UInt32 *size)) override {
        *size = 0;
        return E_FAIL;
    }
};
class CCancelProgress final : public ICompressProgressInfo, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressProgressInfo)
    Z7_COM7F_IMF(SetRatioInfo(const UInt64 *input, const UInt64 *)) override {
        return input && *input ? E_ABORT : S_OK;
    }
};
} // namespace

class ZipZstdTests final : public QObject {
    Q_OBJECT
    QString engine_ = QString::fromUtf8(SEVENZIP_ZIP_VERIFY_ENGINE);

  private slots:
    void initTestCase() { CrcGenerateTable(); }
    void staticZipWriterRoundTrip() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("данные 空 with spaces.txt");
        const QByteArray payload = QByteArray("static ZIP writer payload\n").repeated(15000);
        const QString archive = fixture.filePath("static.zip");
        CMyComPtr2_Create<IOutArchive, NArchive::NZip::CHandler> writer;
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(writer.Interface()->QueryInterface(IID_ISetProperties, (void **)&properties),
                 S_OK);
        const wchar_t *names[] = {L"m", L"x"};
        NWindows::NCOM::CPropVariant values[] = {NWindows::NCOM::CPropVariant(L"ZSTD"),
                                                 NWindows::NCOM::CPropVariant(UInt32(5))};
        QCOMPARE(properties->SetProperties(names, values, 2), S_OK);
        CMyComPtr2_Create<IOutStream, COutFileStream> output;
        QVERIFY(output->Create_NEW(us2fs(archive.toStdWString().c_str())));
        CMyComPtr<IArchiveUpdateCallback> callback = new NativeArchiveFixture::CWriter(
            QList<NativeArchiveFixture::Item>{{name, payload}, {"second.txt", payload}}, QString());
        QCOMPARE(writer.Interface()->UpdateItems(output, 2, callback), S_OK);
        QCOMPARE(output->Close(), S_OK);
        const QByteArray bytes = readFile(archive);
        QVERIFY(bytes.size() > 10);
        QCOMPARE(quint8(bytes[8]) | (quint16(quint8(bytes[9])) << 8), 93);
        QProcess extract;
        extract.start(engine_, {"x", "-y", "-o" + fixture.filePath("out"), "--", archive});
        QVERIFY(extract.waitForFinished(10000));
        QCOMPARE(extract.exitCode(), 0);
        QCOMPARE(readFile(fixture.filePath("out/" + name)), payload);
        QCOMPARE(readFile(fixture.filePath("out/second.txt")), payload);
    }
    void nativeCreateAndUpdate_data() {
        QTest::addColumn<QString>("encryption");
        QTest::addColumn<UInt32>("threads");
        QTest::addColumn<QString>("initialMethod");
        QTest::newRow("plain-serial-reuse") << QString("none") << UInt32(1) << QString("ZSTD");
        QTest::newRow("plain-parallel") << QString("none") << UInt32(2) << QString("ZSTD");
        QTest::newRow("AES256") << QString("AES256") << UInt32(1) << QString("ZSTD");
        QTest::newRow("ZipCrypto") << QString("ZipCrypto") << UInt32(1) << QString("ZSTD");
        QTest::newRow("update-Deflate-to-Zstd")
            << QString("none") << UInt32(1) << QString("Deflate");
    }
    void nativeCreateAndUpdate() {
        QFETCH(QString, encryption);
        QFETCH(UInt32, threads);
        QFETCH(QString, initialMethod);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString first = fixture.filePath(QStringLiteral("данные 空 with spaces.txt"));
        const QString second = fixture.filePath("retained.txt");
        const QString empty = fixture.filePath("empty.bin");
        const QString random = fixture.filePath("incompressible.bin");
        const QByteArray randomBytes = incompressibleBytes();
        const QByteArray payload = QByteArray("Zstandard ZIP payload\0", 22).repeated(50000);
        const QByteArray retained = QByteArray("second entry\n").repeated(30000);
        QVERIFY(writeFile(first, payload));
        QVERIFY(writeFile(second, retained));
        QVERIFY(writeFile(empty, {}));
        QVERIFY(writeFile(random, randomBytes));
        const QString archive = fixture.filePath(QStringLiteral("архив 空 with spaces.zip"));
        const QString password = encryption == "none" ? QString() : QString("fixture-password");
        CompressionOptions options;
        options.format = "zip";
        options.Method = initialMethod.toStdWString().c_str();
        options.Level = 5;
        options.NumThreads = threads;
        if (encryption == "AES256")
            options.EncryptionMethod = encryption;
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(SourceCreationFixture::Create(backend, archive, {first, second, empty, random},
                                               password, false, options),
                 S_OK);
        QVERIFY(!readFile(archive).isEmpty());
        QProcess listing;
        listing.start(engine_, {"l", "-slt", "--", archive});
        QVERIFY(listing.waitForFinished(10000));
        QCOMPARE(listing.exitCode(), 0);
        const QByteArray properties = listing.readAllStandardOutput();
        QVERIFY(properties.toLower().contains(initialMethod.toLower().toLatin1()));
        if (encryption != "none")
            QVERIFY(properties.contains("Encrypted = +"));

        // Replace one existing entry and add another; untouched data must survive.
        const QByteArray updated = QByteArray("replacement\n").repeated(25000);
        const QString added = fixture.filePath("added.txt");
        const QByteArray addedBytes = QByteArray("new entry\n").repeated(10000);
        QVERIFY(writeFile(first, updated));
        QVERIFY(writeFile(added, addedBytes));
        options.Method = L"ZSTD";
        QCOMPARE(SourceCreationFixture::Create(backend, archive, {first, added}, password, false,
                                               options),
                 S_OK);
        listing.start(engine_, {"l", "-slt", "--", archive});
        QVERIFY(listing.waitForFinished(10000));
        QCOMPARE(listing.exitCode(), 0);
        const QByteArray updatedProperties = listing.readAllStandardOutput().toLower();
        QVERIFY(updatedProperties.contains("zstd"));
        if (initialMethod == "Deflate")
            QVERIFY(updatedProperties.contains("deflate"));
        const QByteArray original = readFile(archive);
        QCOMPARE(SourceCreationFixture::Create(backend, archive, {first}, password, false, options,
                                               true),
                 E_ABORT);
        QCOMPARE(readFile(archive), original);

        const QString output = fixture.filePath("out");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.openFolder(archive, "zip", password);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        backend.extract(archive, output, {}, password);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(readFile(QDir(output).filePath(QFileInfo(first).fileName())), updated);
        QCOMPARE(readFile(QDir(output).filePath("retained.txt")), retained);
        QCOMPARE(readFile(QDir(output).filePath("added.txt")), addedBytes);
        QVERIFY(QFileInfo::exists(QDir(output).filePath("empty.bin")));
        QCOMPARE(readFile(QDir(output).filePath("incompressible.bin")), randomBytes);
        QCOMPARE(readFile(archive), original);
        QCOMPARE(readFile(first), updated);
        QCOMPARE(readFile(second), retained);
        if (encryption != "none") {
            QProcess wrongPassword;
            wrongPassword.start(engine_, {"t", "-pincorrect", "--", archive});
            QVERIFY(wrongPassword.waitForFinished(10000));
            QVERIFY(wrongPassword.exitCode() != 0);
        }
    }

    void encoderFailuresAndReuse() {
        CMyComPtr2_Create<ICompressCoder, NCompress::NZSTD::CEncoder> encoder;
        const QByteArray plain = QByteArray("payload\n").repeated(50000);
        CMyComPtr2_Create<IInStream, CBufInStream> input;
        CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> output;
        CMyComPtr2_Create<ISequentialInStream, CFailedInput> failedInput;
        output->Init();
        QCOMPARE(encoder.Interface()->Code(failedInput, output, nullptr, nullptr, nullptr), E_FAIL);
        input->Init(reinterpret_cast<const Byte *>(plain.constData()), plain.size());
        CMyComPtr2_Create<ISequentialOutStream, CFailedOutput> failedOutput;
        QCOMPARE(encoder.Interface()->Code(input, failedOutput, nullptr, nullptr, nullptr), E_FAIL);
        input->Init(reinterpret_cast<const Byte *>(plain.constData()), plain.size());
        output->Init();
        CMyComPtr2_Create<ICompressProgressInfo, CCancelProgress> progress;
        QCOMPARE(encoder.Interface()->Code(input, output, nullptr, nullptr, progress), E_ABORT);

        // Reuse after failures and across full/empty entries without concatenated frames.
        for (const QByteArray &bytes :
             {plain, incompressibleBytes(), QByteArray(), QByteArray("last entry")}) {
            input->Init(reinterpret_cast<const Byte *>(bytes.constData()), bytes.size());
            output->Init();
            QCOMPARE(encoder.Interface()->Code(input, output, nullptr, nullptr, nullptr), S_OK);
            CMyComPtr2_Create<IInStream, CBufInStream> compressed;
            compressed->Init(output->GetBuffer(), output->GetSize());
            CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> decoded;
            decoded->Init();
            CMyComPtr2_Create<ICompressCoder, NCompress::NZstd::CDecoder> decoder;
            decoder->FinishMode = true;
            const UInt64 packed = output->GetSize(), unpacked = bytes.size();
            QCOMPARE(decoder.Interface()->Code(compressed, decoded, &packed, &unpacked, nullptr),
                     S_OK);
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(decoded->GetBuffer()),
                                decoded->GetSize()),
                     bytes);
        }
    }
};
QTEST_MAIN(ZipZstdTests)
#include "zip_zstd_tests.moc"
