#include "formats/btrfs/support/btrfs_fixture.h"
#include <QCryptographicHash>
#include <QtEndian>
#include <QtTest>
using namespace BtrfsFixture;

class BtrfsReaderTests final : public QObject {
    Q_OBJECT
  private slots:
    void linuxImages_data() {
        QTest::addColumn<QString>("variant");
        const auto images = manifest()["images"].toObject();
        QVERIFY(!images.isEmpty());
        for (auto it = images.begin(); it != images.end(); ++it)
            QTest::newRow(qPrintable(it.key())) << it.key();
    }
    void linuxImages() {
        QFETCH(QString, variant);
        Reader reader;
        reader.bytes = image(variant);
        QVERIFY(!reader.bytes.isEmpty());
        QCOMPARE(reader.Open(), 0);
        QMap<QString, btrfs_file_info> items;
        QCOMPARE(walk(reader, items), 0);
        const auto expected =
            manifest()["images"].toObject()[variant].toObject()["expected"].toObject();
        const auto files = expected["files"].toObject();
        QStringList actual;
        for (auto it = items.begin(); it != items.end(); ++it)
            if ((it->mode & 0170000) == 0100000)
                actual.append(it.key());
        QCOMPARE(actual, files.keys());
        for (auto it = files.begin(); it != files.end(); ++it) {
            QVERIFY2(items.contains(it.key()), qPrintable(it.key()));
            QByteArray data;
            QCOMPARE(readFile(reader, items[it.key()], data), 0);
            QCOMPARE(data.size(), it->toObject()["size"].toInteger());
            QCOMPARE(QString::fromLatin1(
                         QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex()),
                     it->toObject()["sha256"].toString());
        }
        const auto symlinks = expected["symlinks"].toObject();
        for (auto it = symlinks.begin(); it != symlinks.end(); ++it) {
            QByteArray target;
            QVERIFY(items.contains(it.key()));
            QCOMPARE(readFile(reader, items[it.key()], target), 0);
            QCOMPARE(QString::fromUtf8(target), it->toString());
        }
        const auto attributes = expected["attributes"].toObject();
        for (auto it = attributes.begin(); it != attributes.end(); ++it) {
            QVERIFY(items.contains(it.key()));
            const auto &info = items[it.key()];
            int error;
            auto file = open(reader.volume.get(), info.root, info.ino, error);
            QCOMPARE(error, 0);
            Attributes values;
            QCOMPARE(btrfs_read_xattrs(file.get(), &values, Attributes::Emit), 0);
            QCOMPARE(values.values, it->toObject());
        }
        const auto &original = items[QString::fromUtf8("данные 空.bin")];
        QCOMPARE(original.ino, items["hardlink.bin"].ino);
        QCOMPARE(original.root, items["hardlink.bin"].root);
        QCOMPARE(items["subvolume/same-inode.txt"].ino, items["snapshot/same-inode.txt"].ino);
        QVERIFY(items["subvolume/same-inode.txt"].root != items["snapshot/same-inode.txt"].root);
    }
    void malformedCompression_data() {
        QTest::addColumn<QString>("variant");
        QTest::addColumn<QString>("path");
        for (const QString &codec : {"zlib", "lzo", "zstd"})
            for (const QString &path :
                 {QString("compressed-inline.txt"), QString::fromUtf8("данные 空.bin")})
                QTest::newRow(qPrintable(codec + '/' + path)) << codec << path;
    }
    void malformedCompression() {
        QFETCH(QString, variant);
        QFETCH(QString, path);
        Reader reader;
        reader.bytes = image(variant);
        reader.captureDecode = true;
        QCOMPARE(reader.Open(), 0);
        QMap<QString, btrfs_file_info> items;
        QCOMPARE(walk(reader, items), 0);
        reader.frames.clear();
        QByteArray expected;
        QVERIFY(items.contains(path));
        QCOMPARE(readFile(reader, items[path], expected), 0);
        QVERIFY(!reader.frames.isEmpty());
        const auto frame = reader.frames.first();
        for (bool truncated : {true, false}) {
            auto input = frame.input;
            if (truncated)
                input.truncate(1);
            else
                input[0] ^= char(0xff);
            QByteArray output(qsizetype(frame.output + 32), char(0x5a));
            QVERIFY(btrfs_decode_native(nullptr, frame.algorithm, frame.sector, frame.inlined,
                                        input.constData(), size_t(input.size()), output.data() + 16,
                                        frame.output) != 0);
            QCOMPARE(output.left(16), QByteArray(16, char(0x5a)));
            QCOMPARE(output.right(16), QByteArray(16, char(0x5a)));
        }
    }
    void corruptionTransportAndCancellation() {
        Reader reader;
        reader.bytes = image();
        QCOMPARE(reader.Open(), 0);
        QMap<QString, btrfs_file_info> items;
        QCOMPARE(walk(reader, items), 0);
        const auto info = items[QString::fromUtf8("данные 空.bin")];
        int error;
        auto file = open(reader.volume.get(), info.root, info.ino, error);
        QCOMPARE(error, 0);
        reader.reads.clear();
        QByteArray buffer(qsizetype(reader.info.sectorsize * 2), char(0x7f));
        size_t processed = 0;
        QCOMPARE(btrfs_pread_file(file.get(), 0, buffer.data(), reader.info.sectorsize, &processed),
                 0);
        QCOMPARE(processed, size_t(reader.info.sectorsize));
        uint64_t dataOffset = UINT64_MAX;
        for (const auto &request : reader.reads)
            if (request.size == reader.info.sectorsize) {
                dataOffset = request.offset;
                break;
            }
        QVERIFY(dataOffset < uint64_t(reader.bytes.size()));
        const char byte = reader.bytes[qsizetype(dataOffset + 17)];
        reader.bytes[qsizetype(dataOffset + 17)] = byte ^ 1;
        QVERIFY(btrfs_pread_file(file.get(), 0, buffer.data(), buffer.size(), &processed) != 0);
        QCOMPARE(processed, size_t(0));
        reader.bytes[qsizetype(dataOffset + 17)] = byte;
        reader.failAt = dataOffset + reader.info.sectorsize;
        QCOMPARE(btrfs_pread_file(file.get(), 0, buffer.data(), buffer.size(), &processed), -EIO);
        QCOMPARE(processed, size_t(reader.info.sectorsize));
        reader.failAt = UINT64_MAX;
        reader.cancel = true;
        QCOMPARE(btrfs_pread_file(file.get(), 0, buffer.data(), buffer.size(), &processed),
                 -ECANCELED);
        QCOMPARE(processed, size_t(0));
    }
    void rootOwnerCannotBeAnotherSubvolume() {
        Reader reference;
        reference.bytes = image();
        QCOMPARE(reference.Open(), 0);
        int error = 0;
        auto root = open(reference.volume.get(), 5, 0, error);
        QCOMPARE(error, 0);
        QByteArray original;
        for (const auto &read : reference.reads) {
            if (read.size != reference.info.nodesize)
                continue;
            auto block = reference.bytes.mid(qsizetype(read.offset), qsizetype(read.size));
            // Published Btrfs tree-block header: owner at byte 88.
            if (qFromLittleEndian<quint64>(block.constData() + 88) == 5) {
                original = block;
                break;
            }
        }
        QVERIFY(!original.isEmpty());
        auto malformed = original;
        qToLittleEndian(quint64(256), malformed.data() + 88);
        uint8_t checksum[32];
        QCOMPARE(btrfs_checksum_native(reference.info.checksum, malformed.constData() + 32,
                                       size_t(malformed.size() - 32), checksum),
                 0);
        memcpy(malformed.data(), checksum, sizeof(checksum));
        Reader corrupt;
        corrupt.bytes = reference.bytes;
        const int copies =
            replaceMetadataCopies(corrupt.bytes, original, malformed, reference.info.sectorsize);
        QVERIFY(copies >= 2);
        if (corrupt.Open() == 0) {
            auto rejected = open(corrupt.volume.get(), 5, 0, error);
            QVERIFY(error != 0);
            QVERIFY(!rejected);
        } else {
            QVERIFY(!corrupt.volume);
        }
    }
    void metadataMirrorsAndBounds() {
        Reader reference;
        reference.bytes = image();
        QCOMPARE(reference.Open(), 0);
        uint64_t offset = UINT64_MAX;
        for (const auto &request : reference.reads)
            if (request.size == reference.info.nodesize) {
                offset = request.offset;
                break;
            }
        QVERIFY(offset < uint64_t(reference.bytes.size()));
        const QByteArray original = reference.bytes.mid(qsizetype(offset), reference.info.nodesize);
        Reader single;
        single.bytes = reference.bytes;
        single.bytes[qsizetype(offset + 101)] = single.bytes[qsizetype(offset + 101)] ^ 1;
        QCOMPARE(single.Open(), 0); // The other DUP copy still supplies valid metadata.
        QByteArray malformed = original;
        // Published Btrfs tree-block header: nritems at byte 96. Rehash the
        // block so this detects bounds validation, independently of checksums.
        qToLittleEndian(UINT32_MAX, malformed.data() + 96);
        uint8_t checksum[32];
        QCOMPARE(btrfs_checksum_native(reference.info.checksum, malformed.constData() + 32,
                                       size_t(malformed.size() - 32), checksum),
                 0);
        memcpy(malformed.data(), checksum, sizeof(checksum));
        Reader all;
        all.bytes = reference.bytes;
        const int copies =
            replaceMetadataCopies(all.bytes, original, malformed, reference.info.sectorsize);
        QVERIFY(copies >= 2);
        QVERIFY(all.Open() != 0);
        QVERIFY(!all.volume);
    }
};
QTEST_GUILESS_MAIN(BtrfsReaderTests)
#include "btrfs_reader_tests.moc"
