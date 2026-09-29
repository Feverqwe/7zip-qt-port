#include "archivebackend.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

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
