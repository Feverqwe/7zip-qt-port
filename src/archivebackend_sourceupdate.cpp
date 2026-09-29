#include "CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "CPP/Common/Wildcard.h"
#include "archivebackend.h"
#include <QDir>
#include <QFileInfo>

bool IsSafePath(const UString &path);

bool ArchiveBackend::sourceUpdatePathIsSafe(const QString &path) {
    const auto value = path.toStdWString();
    const UString native(value.c_str());
    if (path.contains(QChar(0)) || !IsSafePath(native))
        return false;
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

void ArchiveBackend::updateSourceFile(const QString &archive, const QString &entry,
                                      const QString &sourceRoot, const QString &output,
                                      const QString &format, const QString &password) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if ((format != "7z" && format != "zip") || !sourceUpdatePathIsSafe(entry) ||
        !QFileInfo(archive).isFile() || !QFileInfo(QDir(sourceRoot).filePath(entry)).isFile() ||
        output.isEmpty() || QFileInfo::exists(output)) {
        emit finished(false, QStringLiteral("Недопустимый файл или путь обновления архива."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    createTemporary_ = QFileInfo(output).absoluteFilePath();
    pendingPassword_ = password;
    // ArchiveCommandLine.cpp ParseUpdateCommandString: '-' suppresses modification
    // of the input; '!' selects the separate output. All matching states compress
    // the source file, preserving the native UpdateOneFile NewData/NewProps choice.
    start(Job::SourceUpdate,
          {"u", "-y", "-bsp1", "-bb1", "-spd", "-t" + format, "-u-",
           "-up1q1r2x2y2z2w2!" + createTemporary_, "--", archive_, entry},
          sourceRoot);
}
