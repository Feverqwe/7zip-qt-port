#include "7zip/PropID.h"
#include "archivebackend.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <limits>

HRESULT ArchiveBackend::listingFailureResult(int exitCode, QProcess::ExitStatus status) const {
    // Console/List.cpp (0766b733) prints these only for Open_Strict == S_FALSE.
    // Unknown engine output, process/I/O failures and crashes remain E_FAIL.
    if (status != QProcess::NormalExit || exitCode != 2)
        return E_FAIL;
    QString output = output_;
    QString archive = archive_;
#ifdef Q_OS_MACOS
    // The engine prints filesystem-native NFD names on macOS. Compare the
    // transport label canonically without changing source/archive entry names.
    output = output.normalized(QString::NormalizationForm_C);
    archive = archive.normalized(QString::NormalizationForm_C);
#endif
    const QStringList lines = output.split('\n');
    const QString prefix = QStringLiteral("ERROR: ") + archive + QStringLiteral(" : ");
    if (lines.contains(prefix + QStringLiteral("Cannot open the file as archive")) ||
        lines.contains(prefix + QStringLiteral("Cannot open encrypted archive. Wrong password?")))
        return S_FALSE;
    if (lines.contains(prefix + archive)) {
        static const QRegularExpression typeError(
            QStringLiteral("^Open ERROR: Cannot open the file as \\[.+\\] archive$"));
        for (const QString &line : lines)
            if (typeError.match(line).hasMatch())
                return S_FALSE;
    }
    return E_FAIL;
}

QList<ArchiveEntry> ArchiveBackend::parseListing(const QString &output) {
    QList<ArchiveEntry> result;
    ArchiveEntry current;
    current.size = current.packedSize = -1;
    bool havePath = false;
    auto flush = [&] {
        if (havePath)
            result.append(current);
        current = ArchiveEntry{};
        current.size = current.packedSize = -1;
        havePath = false;
    };
    const bool hasHeader = output.contains(QStringLiteral("----------\n"));
    bool inEntries = !hasHeader;
    for (const QString &line : output.split('\n')) {
        const QString trimmed = line.trimmed();
        if (!inEntries) {
            if (trimmed == "----------")
                inEntries = true;
            continue;
        }
        if (trimmed.isEmpty()) {
            flush();
            continue;
        }
        const int separator = line.indexOf(" = ");
        if (separator < 0)
            continue;
        const QString key = line.left(separator);
        const QString value = line.mid(separator + 3);
        if (key == "Path") {
            flush();
            current.path = value;
            havePath = true;
        } else if (key == "Size" || key == "Packed Size") {
            bool defined = false;
            const quint64 size = value.toULongLong(&defined);
            if (defined)
                (key == "Size" ? current.sizeValue : current.packedSizeValue) = size;
            (key == "Size" ? current.size : current.packedSize) =
                defined && size <= quint64(std::numeric_limits<qint64>::max()) ? qint64(size) : -1;
        } else if (key == "Modified")
            current.modified = value;
        else if (key == "Folder")
            current.directory = value == "+";
        else if (key == "Attributes") {
            current.directory = value.startsWith('D');
            current.link |= value.contains('l') || value.contains("Link", Qt::CaseInsensitive);
        } else if (key == "Symbolic Link" || key == "Hard Link" || key == "Reparse Point")
            current.link |= !value.isEmpty();
        if (key == "CRC") {
            bool defined = false;
            const quint32 crc = value.toUInt(&defined, 16);
            if (defined && value.size() == 8)
                current.crc = crc;
        } else if (key == "Checksum" || key == "SHA-1" || key == "SHA-256") {
            // Console/List.cpp raw branch prints <=64 bytes as lowercase hex;
            // larger/decoded security/reparse values cannot be reconstructed.
            static const QRegularExpression hex("^[0-9a-fA-F]{2,128}$");
            if (value.size() % 2 == 0 && hex.match(value).hasMatch())
                current.rawProperties.insert(
                    key == "Checksum" ? kpidChecksum : (key == "SHA-1" ? kpidSha1 : kpidSha256),
                    QByteArray::fromHex(value.toLatin1()));
            else
                current.properties.insert(key, value);
        } else if (key != "Path" && key != "Size" && key != "Packed Size" && key != "Modified")
            current.properties.insert(key, value);
    }
    flush();
    return result;
}

QMap<QString, QString> ArchiveBackend::parseArchiveHeaderProperties(const QString &header) {
    // Reverse only actual Console/List.cpp PrintPropVal_MultiLine output.
    // Upstream has no parser for its CLI presentation; this is transport glue.
    QMap<QString, QString> properties;
    const QStringList lines = header.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const int separator = lines[i].indexOf(" = ");
        if (separator < 0)
            continue;
        const QString key = lines[i].left(separator);
        QString value = lines[i].mid(separator + 3);
        if (value.isEmpty() && i + 1 < lines.size() && lines[i + 1] == "{") {
            i += 2;
            QStringList block;
            for (; i < lines.size() && lines[i] != "}"; ++i)
                block.append(lines[i]);
            value = block.join('\n');
        }
        if (key != "Path" && key != "Type" && key != "Physical Size" && key != "Offset")
            properties.insert(key, value);
    }
    return properties;
}
