#pragma once

#include <QByteArray>
#include <QString>

// A newc payload with the current-directory entry and ./ names used by pkg
// payloads. Keep literal names: archive creation tools can strip the dots.
inline QByteArray DotPathCpioFixture(const QByteArray &root = ".") {
    QByteArray archive;
    quint32 inode = 1;
    const auto append = [&](const QByteArray &name, bool directory, const QByteArray &bytes) {
        archive += "070701";
        const quint32 fields[] = {inode++, directory ? 0040755u : 0100644u, 0, 0, 1,
                                  0,       quint32(bytes.size()),           0, 0, 0,
                                  0,       quint32(name.size() + 1),        0};
        for (quint32 value : fields)
            archive += QByteArray::number(value, 16).rightJustified(8, '0');
        archive += name;
        archive += '\0';
        while (archive.size() % 4)
            archive += '\0';
        archive += bytes;
        while (archive.size() % 4)
            archive += '\0';
    };
    append(root, true, {});
    append(root + "/ProxyBridge.app", true, {});
    append(root + QStringLiteral("/ProxyBridge.app/имя 空.txt").toUtf8(), false, "payload bytes");
    append(root + "/._ProxyBridge.app", false, "sidecar bytes");
    append("TRAILER!!!", false, {});
    return archive;
}
