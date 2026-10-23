#pragma once
#include <QStringList>

inline bool IsArchiveRecoveryFormat(const QString &format) {
    return QStringList{"7z", "zip", "rar", "rar5", "tar", "gzip", "bzip2", "xz", "zstd"}.contains(
        format.toLower());
}
