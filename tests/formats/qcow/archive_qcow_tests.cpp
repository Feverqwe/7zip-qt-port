#include "archive/resources/archivenativeopen.h"
#include "archive/support/archive_reader_fixture.h"
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
             {QStringLiteral("qcow1-stored"), QStringLiteral("qcow1-deflate"),
              QStringLiteral("qcow1-deflate-short"), QStringLiteral("stored"),
              QStringLiteral("deflate"), QStringLiteral("zstd"), QStringLiteral("extended-stored"),
              QStringLiteral("extended-deflate"), QStringLiteral("extended-zstd")})
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
        QTest::newRow("extended L2 with too small clusters") << quint64(16 | 8);
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
    void damagedQcow1ClusterDoesNotReturnDiskBytes_data() {
        QTest::addColumn<bool>("truncate");
        QTest::newRow("invalid-deflate") << false;
        QTest::newRow("truncated-payload") << true;
    }
    void damagedQcow1ClusterDoesNotReturnDiskBytes() {
        QFETCH(bool, truncate);
        QByteArray bytes = Inflate(QStringLiteral(LAYER_FIXTURE_DIR "/qcow1-deflate.qcow2.gz"));
        QVERIFY(bytes.size() > 48);
        const unsigned bits = quint8(bytes.at(32));
        const auto l1 = qFromBigEndian<quint64>(bytes.constData() + 40);
        QVERIFY(l1 + 8 <= quint64(bytes.size()));
        const auto l2 = qFromBigEndian<quint64>(bytes.constData() + l1);
        QVERIFY(l2 + 8 <= quint64(bytes.size()));
        const auto entry = qFromBigEndian<quint64>(bytes.constData() + l2);
        QVERIFY(entry & (quint64(1) << 63));
        const auto offset = entry & ((quint64(1) << (63 - bits)) - 1);
        const auto length = (entry >> (63 - bits)) & ((quint64(1) << bits) - 1);
        QVERIFY(length > 1);
        QVERIFY(offset + length <= quint64(bytes.size()));
        if (truncate)
            bytes.truncate(qsizetype(offset + length - 1));
        else
            // Raw Deflate BTYPE=3 is reserved and cannot decode successfully.
            bytes[qsizetype(offset)] = char(quint8(bytes.at(qsizetype(offset))) | 6);
        RejectDiskBytes(bytes);
    }
    void extendedSubclustersDoNotExposeUnallocatedHostBytes() {
        QByteArray bytes = Inflate(QStringLiteral(LAYER_FIXTURE_DIR "/extended-stored.qcow2.gz"));
        QVERIFY(bytes.size() > 112);
        const auto l1 = qFromBigEndian<quint64>(bytes.constData() + 40);
        QVERIFY(l1 + 8 <= quint64(bytes.size()));
        const auto l2 = qFromBigEndian<quint64>(bytes.constData() + l1) & L2OffsetMask;
        QVERIFY(l2 + 16 <= quint64(bytes.size()));
        const auto host = qFromBigEndian<quint64>(bytes.constData() + l2) & L2OffsetMask;
        QVERIFY(host && host + 16384 <= quint64(bytes.size()));
        // Keep stale nonzero host bytes behind both explicit-zero and unallocated
        // subclusters. Only subcluster 1 is visible; reads cross all boundaries.
        memset(bytes.data() + host, 0x5a, 16384);
        qToBigEndian<quint64>((quint64(1) << 32) | 2, bytes.data() + l2 + 8);
        QTemporaryDir directory;
        const auto path = directory.filePath(QStringLiteral("подкластеры 空.qcow2"));
        QVERIFY(Put(path, bytes));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return backend_->openNativeArchive(path, "QCOW", nullptr, reader); }),
                 S_OK);
        QVERIFY(reader);
        CMyComPtr<IInArchiveGetStream> streams;
        QCOMPARE(reader->arc().Archive.QueryInterface(IID_IInArchiveGetStream, &streams), S_OK);
        CMyComPtr<ISequentialInStream> input;
        QCOMPARE(streams->GetStream(0, &input), S_OK);
        QVERIFY(input);
        QByteArray actual(16384, Qt::Uninitialized);
        UInt32 pos = 0;
        while (pos < UInt32(actual.size())) {
            UInt32 read = 0;
            QCOMPARE(input->Read(actual.data() + pos, UInt32(actual.size()) - pos, &read), S_OK);
            QVERIFY(read > 0);
            pos += read;
        }
        QByteArray expected(16384, '\0');
        memset(expected.data() + 512, 0x5a, 512);
        QCOMPARE(actual, expected);
        QCOMPARE(Bytes(path), bytes);
    }
    void malformedMetadata_data() {
        QTest::addColumn<QString>("variant");
        QTest::addColumn<int>("offset");
        QTest::addColumn<quint64>("value");
        QTest::addColumn<int>("width");
        const auto standard = QStringLiteral("zstd");
        QTest::newRow("short header") << standard << 100 << quint64(96) << 4;
        QTest::newRow("unaligned header") << standard << 100 << quint64(105) << 4;
        QTest::newRow("header outside first cluster") << standard << 100 << quint64(8192) << 4;
        QTest::newRow("missing compression field") << standard << 100 << quint64(104) << 4;
        QTest::newRow("missing compression flag") << standard << 72 << quint64(0) << 8;
        QTest::newRow("deflate with zstd flag") << standard << 104 << quint64(0) << 1;
        QTest::newRow("unknown codec") << standard << 104 << quint64(2) << 1;
        QTest::newRow("clusters smaller than a sector") << standard << 20 << quint64(8) << 4;
        QTest::newRow("L1 cannot address disk") << standard << 36 << quint64(1) << 4;
        QTest::newRow("unaligned L1") << standard << 40 << quint64(4097) << 8;
        // Negative offsets identify fields in the first L2 entry.
        const auto extended = QStringLiteral("extended-stored");
        QTest::newRow("allocated and zero") << extended << -8 << ((quint64(1) << 32) | 1) << 8;
        QTest::newRow("allocation without host") << extended << -1 << quint64(0) << 8;
        QTest::newRow("unaligned host") << extended << -1 << quint64(512) << 8;
        QTest::newRow("host outside image") << extended << -1 << (quint64(1) << 48) << 8;
        QTest::newRow("reserved host bits") << extended << -1 << (quint64(1) << 56) << 8;
        QTest::newRow("compressed bitmap")
            << QStringLiteral("extended-zstd") << -8 << quint64(1) << 8;
    }
    void malformedMetadata() {
        QFETCH(QString, variant);
        QFETCH(int, offset);
        QFETCH(quint64, value);
        QFETCH(int, width);
        QByteArray bytes = Inflate(QStringLiteral(LAYER_FIXTURE_DIR "/") + variant + ".qcow2.gz");
        QVERIFY(bytes.size() > 112);
        quint64 position = quint64(offset);
        if (offset < 0) {
            const auto l1 = qFromBigEndian<quint64>(bytes.constData() + 40);
            const auto l2 = qFromBigEndian<quint64>(bytes.constData() + l1) & L2OffsetMask;
            position = l2 + (offset == -8 ? 8 : 0);
        }
        QVERIFY(position + width <= quint64(bytes.size()));
        if (width == 8)
            qToBigEndian(value, bytes.data() + position);
        else if (width == 4)
            qToBigEndian(quint32(value), bytes.data() + position);
        else
            bytes[qsizetype(position)] = char(value);
        RejectDiskBytes(bytes);
    }

  private:
    static constexpr quint64 L2OffsetMask = Q_UINT64_C(0x00fffffffffffe00);
};
QTEST_MAIN(ArchiveQcowTests)
#include "archive_qcow_tests.moc"
