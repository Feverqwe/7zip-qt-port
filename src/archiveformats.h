#pragma once

#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace ArchiveFormats {
inline bool canOpen(const QString &path) {
    static const QStringList suffixes = {"7z",    "zip", "rar", "tar", "gz",  "gzip", "tgz", "bz2",
                                         "bzip2", "xz",  "txz", "zst", "cab", "iso",  "wim"};
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

inline bool canModify(const QString &path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "7z" || suffix == "zip";
}

inline QString openDialogFilter() {
    return QStringLiteral("Архивы (*.7z *.zip *.rar *.tar *.gz *.gzip *.tgz *.bz2 *.bzip2 *.xz "
                          "*.txz *.zst *.cab *.iso *.wim);;Все файлы (*)");
}
} // namespace ArchiveFormats
