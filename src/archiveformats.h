#pragma once

#include "qtlanguageservices.h"
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
    return QtLocalText("Archives", "Архивы") +
           QStringLiteral(" (*.7z *.zip *.rar *.tar *.gz *.gzip *.tgz *.bz2 *.bzip2 *.xz "
                          "*.txz *.zst *.cab *.iso *.wim *.001);;") +
           QtLocalText("All files", "Все файлы") + QStringLiteral(" (*)");
}
} // namespace ArchiveFormats
