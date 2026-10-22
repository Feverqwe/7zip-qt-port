#pragma once
#include <QByteArray>
#include <QtEndian>
#include <QtTest>
#include <cstring>

// Metadata-only images: no host special files or device handles are created.
inline QByteArray erofsSpecialImage(unsigned shape, unsigned mode, unsigned major, unsigned minor) {
    QByteArray bytes(3 * 4096, '\0');
    auto u16 = [&](int at, quint16 value) { qToLittleEndian(value, bytes.data() + at); };
    auto u32 = [&](int at, quint32 value) { qToLittleEndian(value, bytes.data() + at); };
    auto u64 = [&](int at, quint64 value) { qToLittleEndian(value, bytes.data() + at); };
    u32(1024, 0xe0f5e1e2);
    bytes[1036] = 12;
    u64(1040, 2); // inode count
    u32(1060, 3); // image blocks
    u32(1064, 1); // metadata block
    const int root = 4096, node = shape == 2 ? 8192 - 32 : root + 128;
    u16(root, 4); // compact inode, inline directory
    u16(root + 4, 0040755);
    u16(root + 6, 2);
    u32(root + 8, 43);
    const int directory = root + 32;
    for (int i = 0; i < 3; ++i) {
        u64(directory + i * 12, i == 2 ? (node - root) / 32 : 0);
        u16(directory + i * 12 + 8, i == 0 ? 36 : i == 1 ? 37 : 39);
        bytes[directory + i * 12 + 10] = char(i == 2 ? 0 : 2);
    }
    std::memcpy(bytes.data() + directory + 36, "...node", 7);
    u16(node, shape ? 1 : 0); // extended inode, optionally crossing a block
    u16(node + 4, quint16(mode | 0600));
    if (shape)
        u32(node + 44, 1);
    else
        u16(node + 6, 1);
    // EROFS disk encoding, deliberately distinct from Linux's internal dev_t.
    u32(node + 16, (minor & 0xff) | (major << 8) | ((minor & ~0xffU) << 12));
    return bytes;
}

inline void erofsSpecialRows() {
    QTest::addColumn<unsigned>("shape");
    QTest::addColumn<unsigned>("mode");
    QTest::addColumn<unsigned>("major");
    QTest::addColumn<unsigned>("minor");
    for (unsigned shape = 0; shape < 3; ++shape) {
        for (unsigned mode : {0020000U, 0060000U, 0010000U, 0140000U}) {
            for (auto pair : {qMakePair(0U, 0U), qMakePair(1U, 3U), qMakePair(0xabcU, 0x54321U),
                              qMakePair(0xfffU, 0xfffffU)}) {
                const auto label = QByteArray::number(shape) + '-' + QByteArray::number(mode) +
                                   '-' + QByteArray::number(pair.first) + ':' +
                                   QByteArray::number(pair.second);
                QTest::newRow(label.constData()) << shape << mode << pair.first << pair.second;
            }
        }
    }
}
