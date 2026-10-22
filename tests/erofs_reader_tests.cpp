#include "erofs_special_fixture.h"
#include "erofsreader.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QtEndian>
#include <QtTest>
#include <cerrno>
#include <cstring>
#include <memory>

namespace {
QByteArray image() {
    const QString directory = QString::fromUtf8(SEVENZIP_EROFS_FIXTURES) + '/';
    QFile metadata(directory + "manifest.json"), file(directory + "plain.erofs.zlib");
    if (!metadata.open(QIODevice::ReadOnly) || !file.open(QIODevice::ReadOnly))
        return {};
    const auto record = QJsonDocument::fromJson(metadata.readAll()).object();
    QByteArray bytes(4, '\0');
    qToBigEndian(quint32(record["images"].toObject()["plain"].toObject()["size"].toInt()),
                 bytes.data());
    return qUncompress(bytes + file.readAll());
}
struct Reader {
    QByteArray bytes;
    bool fail = false;
    uint64_t failAt = UINT64_MAX;
    std::unique_ptr<erofs_volume, decltype(&erofs_close_volume)> volume{nullptr,
                                                                        erofs_close_volume};
    std::unique_ptr<erofs_file, decltype(&erofs_close_file)> root{nullptr, erofs_close_file};
    static int Read(void *opaque, unsigned device, uint64_t offset, void *data, size_t size) {
        const auto &self = *static_cast<Reader *>(opaque);
        if (self.fail || (self.failAt >= offset && self.failAt - offset < size) || device ||
            offset > uint64_t(self.bytes.size()) || size > self.bytes.size() - offset)
            return -EIO;
        std::memcpy(data, self.bytes.constData() + offset, size);
        return 0;
    }
    int Open() {
        erofs_source source{};
        source.context = this;
        source.size = uint64_t(bytes.size());
        source.read = Read;
        erofs_volume *v = nullptr;
        int result = erofs_open_volume(&source, &v);
        volume.reset(v);
        if (result)
            return result;
        erofs_volume_info info{};
        erofs_get_volume_info(v, &info);
        erofs_file *file = nullptr;
        result = erofs_open_file(v, info.root_nid, &file);
        root.reset(file);
        return result;
    }
};
struct Entries {
    struct Entry {
        uint64_t nid;
        unsigned type;
    };
    QMap<QByteArray, Entry> values;
    int stop = 0;
    static int Emit(void *opaque, const char *name, size_t length, uint64_t nid, unsigned type) {
        auto &self = *static_cast<Entries *>(opaque);
        self.values.insert(QByteArray(name, qsizetype(length)), {nid, type});
        return self.stop;
    }
};

qsizetype regularEntry(const QByteArray &bytes) {
    // Locate a dirent in the uncompressed creator fixture for fault injection.
    const auto blockSize = quint64(1) << quint8(bytes[1036]);
    const auto inode = qFromLittleEndian<quint32>(bytes.constData() + 1064) * blockSize +
                       qFromLittleEndian<quint16>(bytes.constData() + 1038) * 32;
    const auto format = qFromLittleEndian<quint16>(bytes.constData() + inode);
    const auto xattrs = qFromLittleEndian<quint16>(bytes.constData() + inode + 2);
    const auto directory =
        ((format >> 1) & 7) == 2
            ? inode + (format & 1 ? 64 : 32) + (xattrs ? 12 + (xattrs - 1) * 4 : 0)
            : qFromLittleEndian<quint32>(bytes.constData() + inode + 16) * blockSize;
    const auto names = qFromLittleEndian<quint16>(bytes.constData() + directory + 8);
    for (quint64 offset = 0; offset < names; offset += 12)
        if (quint8(bytes[qsizetype(directory + offset + 10)]) == 1)
            return qsizetype(directory + offset + 10);
    return -1;
}
} // namespace

class ErofsReaderTests final : public QObject {
    Q_OBJECT
  private slots:
    void chunkMetadataReadFailureCanBeRetried() {
        Reader reader;
        reader.bytes = erofsSpecialImage(0, 0100000, 0, 0);
        reader.bytes.resize(3 * 65536, '\0');
        const quint64 node = 2 * 65536 - 32;
        const quint64 nid = (node - 4096) / 32;
        qToLittleEndian(quint32(reader.bytes.size() / 4096), reader.bytes.data() + 1060);
        qToLittleEndian(quint32(4), reader.bytes.data() + 1104); // chunked-file feature
        qToLittleEndian(nid, reader.bytes.data() + 4096 + 32 + 24);
        qToLittleEndian(quint16(8), reader.bytes.data() + node); // compact chunk-based inode
        qToLittleEndian(quint16(0100600), reader.bytes.data() + node + 4);
        qToLittleEndian(quint16(1), reader.bytes.data() + node + 6);
        qToLittleEndian(quint32(4096), reader.bytes.data() + node + 8);
        qToLittleEndian(quint32(33), reader.bytes.data() + node + 32); // first chunk
        const QByteArray expected(4096, 'q');
        std::memcpy(reader.bytes.data() + 33 * 4096, expected.constData(), expected.size());
        const auto original = reader.bytes;
        QCOMPARE(reader.Open(), 0);
        erofs_file *raw = nullptr;
        QCOMPARE(erofs_open_file(reader.volume.get(), nid, &raw), 0);
        std::unique_ptr<erofs_file, decltype(&erofs_close_file)> file(raw, erofs_close_file);
        QByteArray data(4096, '?');
        reader.failAt = 2 * 65536;
        for (unsigned i = 0; i < 32; ++i) {
            size_t processed = 99;
            QCOMPARE(erofs_pread_file(raw, 0, data.data(), size_t(data.size()), &processed), -EIO);
            QCOMPARE(processed, size_t(0));
        }
        reader.failAt = UINT64_MAX;
        size_t processed = 0;
        QCOMPARE(erofs_pread_file(raw, 0, data.data(), size_t(data.size()), &processed), 0);
        QCOMPARE(processed, size_t(expected.size()));
        QCOMPARE(data, expected);
        QCOMPARE(reader.bytes, original);
    }
    void failedMetadataReadCanBeRetried() {
        Reader reader;
        reader.bytes = image();
        QVERIFY(!reader.bytes.isEmpty());
        const auto original = reader.bytes;
        reader.fail = true;
        for (unsigned i = 0; i < 32; ++i)
            QCOMPARE(reader.Open(), -EIO);
        reader.fail = false;
        QCOMPARE(reader.Open(), 0);
        Entries entries;
        QCOMPARE(erofs_read_directory(reader.root.get(), &entries, Entries::Emit), 0);
        QVERIFY(entries.values.contains("duplicate.bin"));
        QCOMPARE(reader.bytes, original);
    }
    void specialInodeMetadata_data() { erofsSpecialRows(); }
    void specialInodeMetadata() {
        QFETCH(unsigned, shape);
        QFETCH(unsigned, mode);
        QFETCH(unsigned, major);
        QFETCH(unsigned, minor);
        Reader reader;
        reader.bytes = erofsSpecialImage(shape, mode, major, minor);
        const auto original = reader.bytes;
        QCOMPARE(reader.Open(), 0);
        Entries entries;
        QCOMPARE(erofs_read_directory(reader.root.get(), &entries, Entries::Emit), 0);
        QVERIFY(entries.values.contains("node"));
        erofs_file *raw = nullptr;
        QCOMPARE(erofs_open_file(reader.volume.get(), entries.values["node"].nid, &raw), 0);
        std::unique_ptr<erofs_file, decltype(&erofs_close_file)> file(raw, erofs_close_file);
        erofs_item inode{};
        erofs_get_file_info(raw, &inode);
        QCOMPARE(inode.mode, mode | 0600U);
        QCOMPARE(inode.nlink, 1U);
        QCOMPARE(inode.size, quint64(0));
        const bool device = mode == 0020000 || mode == 0060000;
        QCOMPARE(inode.rdev, device ? (major << 20) | minor : 0U);
        QCOMPARE(reader.bytes, original);
    }
    void callbackTypesMatchLinuxDirentAbi() {
        Reader reader;
        reader.bytes = image();
        QVERIFY(!reader.bytes.isEmpty());
        QCOMPARE(reader.Open(), 0);
        Entries entries;
        QCOMPARE(erofs_read_directory(reader.root.get(), &entries, Entries::Emit), 0);
        QVERIFY(!entries.values.isEmpty());
        for (auto it = entries.values.cbegin(); it != entries.values.cend(); ++it) {
            erofs_file *raw = nullptr;
            QCOMPARE(erofs_open_file(reader.volume.get(), it->nid, &raw), 0);
            std::unique_ptr<erofs_file, decltype(&erofs_close_file)> file(raw, erofs_close_file);
            erofs_item inode{};
            erofs_get_file_info(raw, &inode);
            // Linux DT_* is the file-kind portion of stat.st_mode (bits 12..15).
            QCOMPARE(it->type, (inode.mode & 0170000U) >> 12);
        }
    }
    void callbackStopIsReturnedAndDirectoryCanBeReadAgain() {
        Reader reader;
        reader.bytes = image();
        QCOMPARE(reader.Open(), 0);
        Entries stopped;
        stopped.stop = -ECANCELED;
        QCOMPARE(erofs_read_directory(reader.root.get(), &stopped, Entries::Emit), -ECANCELED);
        QCOMPARE(stopped.values.size(), qsizetype(1));
        Entries complete;
        QCOMPARE(erofs_read_directory(reader.root.get(), &complete, Entries::Emit), 0);
        QVERIFY(complete.values.size() > stopped.values.size());
    }
    void unknownHintsStillEmitEntries_data() {
        QTest::addColumn<unsigned>("hint");
        QTest::newRow("unknown") << 0U;
        QTest::newRow("unsupported") << 255U;
    }
    void unknownHintsStillEmitEntries() {
        QFETCH(unsigned, hint);
        Reader original;
        original.bytes = image();
        QVERIFY(!original.bytes.isEmpty());
        QCOMPARE(original.Open(), 0);
        Entries before;
        QCOMPARE(erofs_read_directory(original.root.get(), &before, Entries::Emit), 0);
        const qsizetype offset = regularEntry(original.bytes);
        QVERIFY(offset > 0);
        const auto nid = qFromLittleEndian<quint64>(original.bytes.constData() + offset - 10);
        Reader changed;
        changed.bytes = original.bytes;
        changed.bytes[offset] = char(hint);
        const auto compat = qFromLittleEndian<quint32>(changed.bytes.constData() + 1032);
        qToLittleEndian(compat & ~1U, changed.bytes.data() + 1032); // optional superblock checksum
        QCOMPARE(changed.Open(), 0);
        Entries after;
        QCOMPARE(erofs_read_directory(changed.root.get(), &after, Entries::Emit), 0);
        QCOMPARE(after.values.keys(), before.values.keys());
        unsigned affected = 0;
        for (auto it = after.values.cbegin(); it != after.values.cend(); ++it) {
            QCOMPARE(it->nid, before.values[it.key()].nid);
            if (it->nid == nid && it->type == 0)
                ++affected;
        }
        QCOMPARE(affected, 1U);
    }
};
QTEST_GUILESS_MAIN(ErofsReaderTests)
#include "erofs_reader_tests.moc"
