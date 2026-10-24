#include "CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "CPP/Common/Wildcard.h"
#include "archivebackend.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

bool IsSafePath(const UString &path);

bool ArchiveBackend::sourceUpdatePathIsSafe(const QString &path, bool allowAbsolute) {
    const auto value = path.toStdWString();
    const UString native(value.c_str());
    if (path.contains(QChar(0)) || !IsSafePath(native)) {
        if (!allowAbsolute || !QDir::isAbsolutePath(path))
            return false;
        const QString normalized = QDir::fromNativeSeparators(path);
        if (normalized.split('/').contains("..") || normalized.split('/').contains("."))
            return false;
        QString relative;
#ifdef Q_OS_WIN
        const QRegularExpression rootPattern(R"(^(?:[A-Za-z]:/|//[^/]+/[^/]+/))");
        const auto match = rootPattern.match(normalized);
        if (!match.hasMatch())
            return false;
        relative = normalized.mid(match.capturedLength());
#else
        relative = normalized.mid(1);
#endif
        return !relative.isEmpty() && sourceUpdatePathIsSafe(relative);
    }
    UStringVector parts;
    SplitPathToParts(native, parts);
    UStringVector corrected = parts;
    Correct_FsPath(false, false, corrected, false);
    if (corrected.Size() != parts.Size())
        return false;
    FOR_VECTOR(i, parts) {
        if (parts[i] != corrected[i])
            return false;
    }
    return true;
}
