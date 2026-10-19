#include "squashfsmetadata.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSet>
#include <QtEndian>
#include <QtTest>
#include <memory>

namespace {
struct Source {
    QByteArray bytes;
    quint64 failBlock = UINT64_MAX;
    static int read(void *opaque, uint64_t offset, void *output, size_t size) {
        auto *source = static_cast<Source *>(opaque);
        if (offset > quint64(source->bytes.size()) || size > quint64(source->bytes.size()) - offset)
            return -5;
        memcpy(output, source->bytes.constData() + offset, size);
        return 0;
    }
    static int metadata(void *opaque, uint64_t offset, void *output, unsigned *size,
                        uint64_t *next) {
        auto *source = static_cast<Source *>(opaque);
        if (offset == source->failBlock || offset > quint64(source->bytes.size()) ||
            2 > quint64(source->bytes.size()) - offset)
            return -5;
        const unsigned header = qFromLittleEndian<quint16>(source->bytes.constData() + offset);
        const unsigned length = (header & 0x7fff) ? header & 0x7fff : 32768;
        if (!(header & 0x8000) || length > *size ||
            length > quint64(source->bytes.size()) - offset - 2)
            return -5;
        memcpy(output, source->bytes.constData() + offset + 2, length);
        *size = length;
        *next = offset + 2 + length;
        return 0;
    }
};
using Reader =
    std::unique_ptr<sevenzip_squashfs_metadata, decltype(&sevenzip_squashfs_metadata_destroy)>;
} // namespace

class SquashfsMetadataTests final : public QObject {
    Q_OBJECT
    QByteArray fixture_;
    Reader create(Source &source) {
        sevenzip_squashfs_metadata *reader = nullptr;
        uint64_t directoryEnd = 0;
        const auto used = qFromLittleEndian<quint64>(source.bytes.constData() + 40);
        if (sevenzip_squashfs_metadata_create(used, Source::read, Source::metadata, &source,
                                              &reader, &directoryEnd) != 0)
            return Reader(nullptr, sevenzip_squashfs_metadata_destroy);
        return Reader(reader, sevenzip_squashfs_metadata_destroy);
    }
    static QByteArray largeValue() {
        QByteArray value;
        for (int i = 0; i < 36; ++i)
            for (int byte = 0; byte < 251; ++byte)
                value.append(char(byte));
        return value;
    }
  private slots:
    void initTestCase() {
        QProcess generator;
        generator.start(QString::fromUtf8(SEVENZIP_SQUASHFS_PYTHON),
                        {QString::fromUtf8(SEVENZIP_SQUASHFS_FIXTURE), "--xattrs", "--export"});
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        const auto record = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record["image"].toString().toLatin1());
        fixture_ = qUncompress(packed);
        QVERIFY(!fixture_.isEmpty());
    }
    void namesInlineBinaryAndSharedOutOfLineValues() {
        Source source{fixture_};
        auto reader = create(source);
        QVERIFY(reader);
        unsigned id = 0;
        QCOMPARE(sevenzip_squashfs_id(reader.get(), 0, &id), 0);
        QCOMPARE(id, 1000u);
        uint64_t reference = 1;
        QCOMPARE(sevenzip_squashfs_inode(reader.get(), 1, &reference), 0);
        QCOMPARE(reference, uint64_t(0));
        QVERIFY(sevenzip_squashfs_inode(reader.get(), 0, &reference) < 0);
        QVERIFY(sevenzip_squashfs_inode(reader.get(), 5, &reference) < 0);
        const int length = sevenzip_squashfs_xattrs(reader.get(), 0, nullptr, 0);
        QVERIFY(length > 0);
        QByteArray names(length, '\0');
        QCOMPARE(sevenzip_squashfs_xattrs(reader.get(), 0, names.data(), size_t(names.size())),
                 length);
        auto list = names.split('\0');
        list.removeAll(QByteArray());
        QCOMPARE(QSet<QByteArray>(list.cbegin(), list.cend()),
                 QSet<QByteArray>({"user.inline", "security.label", "trusted.large",
                                   "user.duplicate", "user.empty"}));
        for (const auto &name : list) {
            const QByteArray expected = name == "user.empty"       ? QByteArray()
                                        : name == "user.inline"    ? QByteArray("v\0a\0lue", 7)
                                        : name == "security.label" ? QByteArray("test-label")
                                                                   : largeValue();
            QCOMPARE(sevenzip_squashfs_xattr(reader.get(), 0, name.constData(), nullptr, 0),
                     expected.size());
            QByteArray value(expected.size(), '!');
            QCOMPARE(sevenzip_squashfs_xattr(reader.get(), 0, name.constData(), value.data(),
                                             size_t(value.size())),
                     expected.size());
            QCOMPARE(value, expected);
        }
        QCOMPARE(sevenzip_squashfs_validate_xattrs(reader.get(), 0), 0);
    }
    void shortBuffersMissingKeysAndInvalidIdsFail() {
        Source source{fixture_};
        auto reader = create(source);
        QVERIFY(reader);
        QByteArray value(2, '!');
        QCOMPARE(sevenzip_squashfs_xattr(reader.get(), 0, "user.inline", value.data(), 2), -34);
        QCOMPARE(value, QByteArray("!!"));
        QVERIFY(sevenzip_squashfs_xattr(reader.get(), 0, "user.missing", nullptr, 0) < 0);
        QVERIFY(sevenzip_squashfs_xattr(reader.get(), 1, "user.inline", nullptr, 0) < 0);
    }
    void metadataFailureAcrossBlocksCanBeRetried() {
        Source source{fixture_};
        auto reader = create(source);
        QVERIFY(reader);
        const auto idTable = qFromLittleEndian<quint64>(source.bytes.constData() + 56);
        const auto start = qFromLittleEndian<quint64>(source.bytes.constData() + idTable);
        source.failBlock = start + 8194;
        QByteArray value(largeValue().size(), '!');
        QCOMPARE(sevenzip_squashfs_xattr(reader.get(), 0, "trusted.large", value.data(),
                                         size_t(value.size())),
                 -5);
        source.failBlock = UINT64_MAX;
        QCOMPARE(sevenzip_squashfs_xattr(reader.get(), 0, "trusted.large", value.data(),
                                         size_t(value.size())),
                 value.size());
        QCOMPARE(value, largeValue());
    }
};
QTEST_GUILESS_MAIN(SquashfsMetadataTests)
#include "squashfs_metadata_tests.moc"
