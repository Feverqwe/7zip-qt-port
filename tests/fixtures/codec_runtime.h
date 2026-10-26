// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>

inline bool CopyCodecRuntime(const QDir &source, const QDir &destination) {
#ifdef Q_OS_WIN
    QStringList files{source.filePath("7z.exe"), source.filePath("7z.dll")};
#else
    QStringList files{source.filePath("7z"), source.filePath("7z.so")};
#endif
    files += qEnvironmentVariable("SEVENZIP_TEST_RUNTIME_LIBRARIES").split('|', Qt::SkipEmptyParts);
    files.removeDuplicates();
    return std::ranges::all_of(files, [&](const QString &path) {
        return QFile::copy(path, destination.filePath(QFileInfo(path).fileName()));
    });
}
