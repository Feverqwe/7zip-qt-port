#include "7zCrc.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Compress/ShrinkDecoder.h"
#include <QDataStream>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

namespace {
QByteArray pack(const QList<unsigned> &codes) {
    QByteArray bytes;
    quint32 bits = 0;
    unsigned count = 0;
    unsigned width = 9;
    bool control = false;
    for (unsigned code : codes) {
        bits |= code << count;
        count += width;
        while (count >= 8) {
            bytes.append(char(bits));
            bits >>= 8;
            count -= 8;
        }
        if (control) {
            if (code == 1)
                ++width;
            control = false;
        } else
            control = code == 256;
    }
    if (count)
        bytes.append(char(bits));
    return bytes;
}

// Minimal ZIP records for a method that current 7-Zip reads but does not write.
QByteArray archive(const QByteArray &compressed, const QByteArray &plain) {
    const QByteArray name = QStringLiteral("данные 空 with spaces.txt").toUtf8();
    const quint32 crc = CrcCalc(plain.constData(), size_t(plain.size()));
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out << quint32(0x04034b50) << quint16(10) << quint16(0x800) << quint16(1) << quint16(0)
        << quint16(0) << crc << quint32(compressed.size()) << quint32(plain.size())
        << quint16(name.size()) << quint16(0);
    out.writeRawData(name.constData(), name.size());
    out.writeRawData(compressed.constData(), compressed.size());
    const quint32 central = bytes.size();
    out << quint32(0x02014b50) << quint16(10) << quint16(10) << quint16(0x800) << quint16(1)
        << quint16(0) << quint16(0) << crc << quint32(compressed.size()) << quint32(plain.size())
        << quint16(name.size()) << quint16(0) << quint16(0) << quint16(0) << quint16(0)
        << quint32(0) << quint32(0);
    out.writeRawData(name.constData(), name.size());
    const quint32 centralSize = quint32(bytes.size()) - central;
    out << quint32(0x06054b50) << quint16(0) << quint16(0) << quint16(1) << quint16(1)
        << centralSize << central << quint16(0);
    return bytes;
}

class CancelProgress final : public ICompressProgressInfo, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressProgressInfo)
  public:
    Z7_COM7F_IMF(SetRatioInfo(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
} // namespace

class ZipShrinkTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { CrcGenerateTable(); }
    void decode_data() {
        QTest::addColumn<QByteArray>("compressed");
        QTest::addColumn<QByteArray>("plain");
        QTest::addColumn<bool>("valid");
        QTest::newRow("literal") << pack({65}) << QByteArray("A") << true;
        QTest::newRow("dictionary-reference") << pack({65, 257}) << QByteArray("AAA") << true;
        QTest::newRow("increased-code-width")
            << pack({65, 256, 1, 257}) << QByteArray("AAA") << true;
        QTest::newRow("maximum-code-width")
            << pack({65, 256, 1, 256, 1, 256, 1, 256, 1, 257}) << QByteArray("AAA") << true;
        QTest::newRow("clear-leaves") << pack({65, 66, 256, 2, 67}) << QByteArray("ABC") << true;
        QTest::newRow("nonzero-padding") << QByteArray::fromHex("4180") << QByteArray("A") << false;
        QTest::newRow("truncated-code") << QByteArray::fromHex("41") << QByteArray("A") << false;
        QTest::newRow("orphan-reference") << pack({65, 258}) << QByteArray("AAA") << false;
        QTest::newRow("initial-control") << pack({256, 2, 65}) << QByteArray("A") << false;
        QList<unsigned> codes;
        QByteArray plain;
        for (unsigned i = 0; i < 100000; ++i) {
            if (i && i % 200 == 0)
                codes << 256 << 2;
            codes << (i & 255);
            plain.append(char(i));
        }
        QTest::newRow("buffer-boundaries") << pack(codes) << plain << true;
    }
    void decode() {
        QFETCH(QByteArray, compressed);
        QFETCH(QByteArray, plain);
        QFETCH(bool, valid);
        CMyComPtr2_Create<IInStream, CBufInStream> input;
        input->Init(reinterpret_cast<const Byte *>(compressed.constData()), compressed.size());
        CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> output;
        CMyComPtr2_Create<ICompressCoder, NCompress::NShrink::CDecoder> decoder;
        QCOMPARE(static_cast<ICompressSetFinishMode *>(decoder.operator->())->SetFinishMode(1),
                 S_OK);
        const UInt64 inSize = compressed.size(), outSize = plain.size();
        const HRESULT result = decoder.Interface()->Code(input, output, &inSize, &outSize, nullptr);
        QCOMPARE(result, valid ? S_OK : S_FALSE);
        if (valid)
            QCOMPARE(
                QByteArray(reinterpret_cast<const char *>(output->GetBuffer()), output->GetSize()),
                plain);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("архив with spaces.zip"));
        const QByteArray image = archive(compressed, plain);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(image), image.size());
        file.close();
        QProcess engine;
        engine.start(QString::fromUtf8(SEVENZIP_ZIP_VERIFY_ENGINE),
                     {"x", "-y", "-o" + dir.filePath("out"), "--", path});
        QVERIFY(engine.waitForFinished(10000));
        QCOMPARE(engine.exitStatus(), QProcess::NormalExit);
        QCOMPARE(engine.exitCode() == 0, valid);
        if (valid) {
            QFile extracted(dir.filePath(QStringLiteral("out/данные 空 with spaces.txt")));
            QVERIFY(extracted.open(QIODevice::ReadOnly));
            QCOMPARE(extracted.readAll(), plain);
        }
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), image);
    }
    void cancellation() {
        QList<unsigned> codes{65};
        for (unsigned i = 0; i < 4200; ++i)
            codes << 256 << 2;
        const QByteArray compressed = pack(codes);
        CMyComPtr2_Create<IInStream, CBufInStream> input;
        input->Init(reinterpret_cast<const Byte *>(compressed.constData()), compressed.size());
        CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> output;
        CMyComPtr2_Create<ICompressCoder, NCompress::NShrink::CDecoder> decoder;
        CMyComPtr2_Create<ICompressProgressInfo, CancelProgress> progress;
        QCOMPARE(static_cast<ICompressSetFinishMode *>(decoder.operator->())->SetFinishMode(1),
                 S_OK);
        const UInt64 inSize = compressed.size(), outSize = 1;
        QCOMPARE(decoder.Interface()->Code(input, output, &inSize, &outSize, progress), E_ABORT);
    }
};

QTEST_GUILESS_MAIN(ZipShrinkTests)
#include "zip_shrink_tests.moc"
