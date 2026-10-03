#pragma once
#include <QByteArray>
#include <QDir>
#include <QString>

// Use with the official engine's -sccUTF-8 output. Filesystem path separators,
// console newlines and macOS Unicode normalization differ between test hosts.
inline QByteArray NormalizeTechnicalListing(const QByteArray &output) {
    return QDir::fromNativeSeparators(QString::fromUtf8(output))
        .normalized(QString::NormalizationForm_C)
        .replace("\r\n", "\n")
        .toUtf8();
}
