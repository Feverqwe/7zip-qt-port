#ifndef SEVENZIP_BTRFS_FIXTURE_H
#define SEVENZIP_BTRFS_FIXTURE_H
#include "btrfsreader.h"
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <cerrno>
#include <cstring>
#include <memory>

namespace BtrfsFixture {
inline QJsonObject manifest() {
    QFile file(QString::fromUtf8(SEVENZIP_BTRFS_FIXTURES) + "/manifest.json");
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
inline QByteArray image(const QString &variant = "plain") {
    QFile file(QString::fromUtf8(SEVENZIP_BTRFS_FIXTURES) + '/' + variant + ".img.qz");
    return file.open(QIODevice::ReadOnly) ? qUncompress(file.readAll()) : QByteArray();
}
inline QJsonObject multiManifest() {
    QFile file(QString::fromUtf8(SEVENZIP_BTRFS_FIXTURES) + "/multi/manifest.json");
    return file.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(file.readAll()).object()
                                          : QJsonObject();
}
inline QMap<quint64, QByteArray> devices(const QString &profile) {
    const auto records =
        multiManifest()["profiles"].toObject()[profile].toObject()["devices"].toObject();
    QMap<quint64, QByteArray> result;
    for (auto it = records.begin(); it != records.end(); ++it) {
        QFile file(QString::fromUtf8(SEVENZIP_BTRFS_FIXTURES) + "/multi/" + profile + "/devid-" +
                   it.key() + ".img.qz");
        if (!file.open(QIODevice::ReadOnly))
            return {};
        auto bytes = qUncompress(file.readAll());
        if (bytes.isEmpty())
            return {};
        result.insert(it.key().toULongLong(), std::move(bytes));
    }
    return result;
}
using File = std::unique_ptr<btrfs_file, decltype(&btrfs_close_file)>;
inline int replaceMetadataCopies(QByteArray &bytes, const QByteArray &original,
                                 const QByteArray &replacement, quint32 sector) {
    if (!sector || original.isEmpty() || original.size() != replacement.size() ||
        original.size() > bytes.size())
        return 0;
    int copies = 0;
    // Metadata on these Linux-created images is sector-aligned. Searching
    // entire zero-padded nodes byte by byte can become quadratic on Windows.
    for (qsizetype offset = 0; offset <= bytes.size() - original.size(); offset += sector) {
        if (memcmp(bytes.constData() + offset, original.constData(), size_t(original.size())) ==
            0) {
            memcpy(bytes.data() + offset, replacement.constData(), size_t(replacement.size()));
            ++copies;
        }
    }
    return copies;
}
inline File open(btrfs_volume *volume, uint64_t root, uint64_t ino, int &error) {
    btrfs_file *file = nullptr;
    error = btrfs_open_file(volume, root, ino, &file);
    return {file, btrfs_close_file};
}
struct Reader {
    struct Request {
        uint64_t offset;
        size_t size;
    };
    QByteArray bytes;
    QList<Request> reads;
    uint64_t failAt = UINT64_MAX;
    bool cancel = false;
    bool captureDecode = false;
    struct Frame {
        unsigned algorithm, sector;
        int inlined;
        QByteArray input;
        size_t output;
    };
    QList<Frame> frames;
    btrfs_volume_info info{};
    std::unique_ptr<btrfs_volume, decltype(&btrfs_close_volume)> volume{nullptr,
                                                                        btrfs_close_volume};
    static int Read(void *opaque, uint64_t, const uint8_t *, uint64_t offset, void *data,
                    size_t size) {
        auto &self = *static_cast<Reader *>(opaque);
        self.reads.append({offset, size});
        if ((self.failAt >= offset && self.failAt - offset < size) ||
            offset > uint64_t(self.bytes.size()) || size > uint64_t(self.bytes.size()) - offset)
            return -EIO;
        memcpy(data, self.bytes.constData() + offset, size);
        return 0;
    }
    static int Poll(void *opaque) { return static_cast<Reader *>(opaque)->cancel ? -ECANCELED : 0; }
    static int Decode(void *opaque, unsigned algorithm, unsigned sector, int inlined,
                      const void *input, size_t size, void *output, size_t capacity) {
        auto &self = *static_cast<Reader *>(opaque);
        if (self.captureDecode && self.frames.isEmpty())
            self.frames.append({algorithm, sector, inlined,
                                QByteArray(static_cast<const char *>(input), qsizetype(size)),
                                capacity});
        return btrfs_decode_native(opaque, algorithm, sector, inlined, input, size, output,
                                   capacity);
    }
    int Open() {
        btrfs_source source{};
        source.context = this;
        source.size = uint64_t(bytes.size());
        source.read = Read;
        source.poll = Poll;
        source.checksum = btrfs_checksum_native;
        source.decode = Decode;
        btrfs_volume *raw = nullptr;
        const int error = btrfs_open_volume(&source, &raw);
        volume.reset(raw);
        if (!error)
            btrfs_get_volume_info(raw, &info);
        return error;
    }
};
struct Entries {
    struct Entry {
        QByteArray name;
        uint64_t root, ino;
    };
    QList<Entry> values;
    static int Emit(void *opaque, const char *name, size_t length, uint64_t root, uint64_t ino,
                    unsigned) {
        static_cast<Entries *>(opaque)->values.append(
            {QByteArray(name, qsizetype(length)), root, ino});
        return 0;
    }
};
inline int walk(Reader &reader, QMap<QString, btrfs_file_info> &items) {
    struct Pending {
        QString path;
        uint64_t root, ino;
    };
    QList<Pending> pending{{QString(), 5, 0}};
    while (!pending.isEmpty()) {
        const auto current = pending.takeLast();
        int error;
        auto file = open(reader.volume.get(), current.root, current.ino, error);
        if (error)
            return error;
        btrfs_file_info info;
        btrfs_get_file_info(file.get(), &info);
        if (items.contains(current.path))
            return -EINVAL;
        items.insert(current.path, info);
        if ((info.mode & 0170000) != 0040000)
            continue;
        Entries children;
        error = btrfs_read_directory(file.get(), &children, Entries::Emit);
        if (error)
            return error;
        for (const auto &child : children.values)
            pending.append(
                {current.path + (current.path.isEmpty() ? "" : "/") + QString::fromUtf8(child.name),
                 child.root, child.ino});
    }
    return 0;
}
inline int readFile(Reader &reader, const btrfs_file_info &info, QByteArray &bytes) {
    if (info.size > (16U << 20))
        return -EFBIG;
    int error;
    auto file = open(reader.volume.get(), info.root, info.ino, error);
    if (error)
        return error;
    bytes.resize(qsizetype(info.size));
    size_t offset = 0;
    while (offset < size_t(bytes.size())) {
        const size_t size = qMin(size_t(7301), size_t(bytes.size()) - offset);
        size_t processed = 0;
        error = btrfs_pread_file(file.get(), offset, bytes.data() + offset, size, &processed);
        if (error || !processed || processed > size)
            return error ? error : -EIO;
        offset += processed;
    }
    return 0;
}
struct Attributes {
    QJsonObject values;
    static int Emit(void *opaque, const char *name, const void *value, size_t size) {
        auto &self = *static_cast<Attributes *>(opaque);
        if (self.values.contains(QString::fromUtf8(name)))
            return -EINVAL;
        self.values.insert(
            QString::fromUtf8(name),
            QString::fromLatin1(
                QByteArray(static_cast<const char *>(value), qsizetype(size)).toHex()));
        return 0;
    }
};
} // namespace BtrfsFixture
#endif
