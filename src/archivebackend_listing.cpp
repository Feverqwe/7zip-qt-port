#include "archivebackend.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

QList<ArchiveEntry> ArchiveBackend::parseListing(const QString &output) {
    QList<ArchiveEntry> result;
    ArchiveEntry current;
    bool havePath = false;
    auto flush = [&] {
        if (havePath)
            result.append(current);
        current = ArchiveEntry{};
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
        const int separator = trimmed.indexOf(" = ");
        if (separator < 0)
            continue;
        const QString key = trimmed.left(separator);
        const QString value = trimmed.mid(separator + 3);
        if (key == "Path") {
            flush();
            current.path = value;
            havePath = true;
        } else if (key == "Size")
            current.size = value.toLongLong();
        else if (key == "Packed Size")
            current.packedSize = value.toLongLong();
        else if (key == "Modified")
            current.modified = value;
        else if (key == "Attributes") {
            current.directory = value.startsWith('D');
            current.link |= value.contains('l') || value.contains("Link", Qt::CaseInsensitive);
        } else if (key == "Symbolic Link" || key == "Hard Link" || key == "Reparse Point")
            current.link = true;
    }
    flush();
    return result;
}
