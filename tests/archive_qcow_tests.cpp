#include "archive_reader_fixture.h"
#include "archivenativeopen.h"
#include <QtEndian>

class ArchiveQcowTests final : public ArchiveReaderFixture {
    Q_OBJECT
    QByteArray Inflate(const QString &path) {
        QProcess decoder;
        decoder.start(engine_, {"x", "-so", "--", path});
        if (!decoder.waitForFinished(10000) || decoder.exitCode())
            return {};
        return decoder.readAllStandardOutput();
    }
    QByteArray Fixture() { return Inflate(QStringLiteral(LAYER_FIXTURE_DIR "/zstd.qcow2.gz")); }
    void RejectDiskBytes(const QByteArray &bytes) {
        QTemporaryDir directory;
        const QString path = directory.filePath("unsupported.qcow2");
        QVERIFY(Put(path, bytes));
        std::shared_ptr<ArchiveNativeOpen> reader;
        const HRESULT result =
            Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); });
        if (result == S_OK) {
            QVERIFY(reader);
            CMyComPtr<IInArchiveGetStream> streams;
            QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &streams), S_OK);
            CMyComPtr<ISequentialInStream> input;
            const HRESULT streamResult = Worker([&] { return streams->GetStream(0, &input); });
            if (input) {
                QCOMPARE(streamResult, S_OK);
                char buffer[512];
                UInt32 actual = 0;
                QVERIFY(Worker([&] { return input->Read(buffer, sizeof(buffer), &actual); }) !=
                        S_OK);
                QCOMPARE(actual, UInt32(0));
            } else
                QVERIFY(streamResult == S_OK || streamResult == S_FALSE ||
                        streamResult == E_NOTIMPL);
        }
        QCOMPARE(Bytes(path), bytes);
    }

  private slots:
    void initTestCase() { InitializeRuntime(); }
    void diskBytesAndSparseTail_data() {
        QTest::addColumn<QString>("variant");
        for (const QString &variant :
             {QStringLiteral("stored"), QStringLiteral("deflate"), QStringLiteral("zstd")})
            QTest::newRow(qPrintable(variant)) << variant;
    }
    void diskBytesAndSparseTail() {
        QFETCH(QString, variant);
        const QByteArray expected =
            Inflate(QStringLiteral(LAYER_FIXTURE_DIR "/../devices/mbr.img.gz"));
        QVERIFY(!expected.isEmpty());
        const QByteArray image =
            Inflate(QStringLiteral(LAYER_FIXTURE_DIR "/") + variant + ".qcow2.gz");
        QVERIFY(!image.isEmpty());
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("диск 空.qcow2"));
        QVERIFY(Put(path, image));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); }),
                 S_OK);
        QVERIFY(reader);
        CMyComPtr<IInArchiveGetStream> streams;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &streams), S_OK);
        CMyComPtr<ISequentialInStream> input;
        QCOMPARE(Worker([&] { return streams->GetStream(0, &input); }), S_OK);
        QVERIFY(input);
        CMyComPtr<IInStream> seekable;
        QCOMPARE(input.QueryInterface(IID_IInStream, &seekable), S_OK);
        // Exercise complete source bytes including allocated, sparse and explicit-zero
        // clusters, then backward/unaligned reads and the large unallocated tail.
        QByteArray actual(expected.size(), Qt::Uninitialized);
        QCOMPARE(Worker([&]() -> HRESULT {
                     qsizetype offset = 0;
                     while (offset < actual.size()) {
                         UInt32 size = 0;
                         RINOK(input->Read(actual.data() + offset,
                                           UInt32(qMin(qsizetype(8191), actual.size() - offset)),
                                           &size))
                         if (!size)
                             return E_FAIL;
                         offset += size;
                     }
                     return S_OK;
                 }),
                 S_OK);
        QCOMPARE(actual, expected);
        for (quint64 offset : {quint64(4093), quint64(0), (quint64(1) << 30) - 513}) {
            QCOMPARE(seekable->Seek(Int64(offset), STREAM_SEEK_SET, nullptr), S_OK);
            char bytes[513];
            UInt32 size = 0;
            QCOMPARE(input->Read(bytes, sizeof(bytes), &size), S_OK);
            // The handler can return a cluster boundary short read.
            QVERIFY(size > 0);
            QCOMPARE(QByteArray(bytes, size), offset < quint64(expected.size())
                                                  ? expected.mid(qsizetype(offset), size)
                                                  : QByteArray(size, '\0'));
        }
        QCOMPARE(Bytes(path), image);
    }
    void unsupportedLayout_data() {
        QTest::addColumn<quint64>("flags");
        QTest::newRow("external data") << quint64(4 | 8);
        QTest::newRow("extended L2") << quint64(16 | 8);
        QTest::newRow("unknown layout") << (quint64(1) << 40 | 8);
    }
    void unsupportedLayout() {
        QFETCH(quint64, flags);
        QByteArray bytes = Fixture();
        QVERIFY(bytes.size() > 112);
        qToBigEndian(flags, bytes.data() + 72);
        RejectDiskBytes(bytes);
    }
    void damagedZstdClusterDoesNotReturnDiskBytes() {
        QByteArray bytes = Fixture();
        QVERIFY(bytes.size() > 112);
        const auto l1 = qFromBigEndian<quint64>(bytes.constData() + 40);
        QVERIFY(l1 + 8 <= quint64(bytes.size()));
        const auto l2 = qFromBigEndian<quint64>(bytes.constData() + l1) & ~(quint64(1) << 63);
        QVERIFY(l2 + 8 <= quint64(bytes.size()));
        const auto entry = qFromBigEndian<quint64>(bytes.constData() + l2);
        QVERIFY(entry & (quint64(1) << 62));
        const unsigned bits = qFromBigEndian<quint32>(bytes.constData() + 20);
        const auto offset = entry & ((quint64(1) << (62 - (bits - 8))) - 1);
        QVERIFY(offset < quint64(bytes.size()));
        bytes[qsizetype(offset)] = char(bytes.at(qsizetype(offset)) ^ 1);
        RejectDiskBytes(bytes);
    }
};
QTEST_MAIN(ArchiveQcowTests)
#include "archive_qcow_tests.moc"
