#pragma once
#include "../third_party/7zip/C/7zCrc.h"
#include <QByteArray>
#include <QString>

// Deterministic stored RAR5/RAR7 records: no proprietary encoder, compressed
// data or decoder algorithm is used to create these disposable test fixtures.
namespace RarMemoryFixture {
inline QByteArray varInt(quint64 value) {
    QByteArray bytes;
    while (value >= 128) {
        bytes.append(char((value & 127) | 128));
        value >>= 7;
    }
    bytes.append(char(value));
    return bytes;
}
inline QByteArray crc(const QByteArray &bytes) {
    const UInt32 value = CrcCalc(bytes.constData(), size_t(bytes.size()));
    QByteArray result;
    for (unsigned shift = 0; shift < 32; shift += 8)
        result.append(char(value >> shift));
    return result;
}
inline QByteArray header(const QByteArray &body) {
    const QByteArray data = varInt(body.size()) + body;
    return crc(data) + data;
}
inline QString name() { return QStringLiteral("имя 空 with spaces.bin"); }
inline QByteArray payload() { return QByteArray("binary\0value\xff", 13); }
inline QByteArray archive(unsigned algorithm, const QString &firstName = name()) {
    QByteArray result("Rar!\x1a\x07\x01\0", 8);
    result += header(varInt(1) + varInt(0) + varInt(0));
    const quint64 method = algorithm | (quint64(4) << 10); // Declared dictionary: 2 MiB.
    for (const QString &path : {firstName, QStringLiteral("other.txt")}) {
        const QByteArray value = path == firstName ? payload() : QByteArray("other bytes");
        const QByteArray utf8 = path.toUtf8();
        result += header(varInt(2) + varInt(2) + varInt(value.size()) + varInt(4) +
                         varInt(value.size()) + varInt(0) + crc(value) + varInt(method) +
                         varInt(1) + varInt(utf8.size()) + utf8);
        result += value;
    }
    result += header(varInt(5) + varInt(0) + varInt(0));
    return result;
}
} // namespace RarMemoryFixture
