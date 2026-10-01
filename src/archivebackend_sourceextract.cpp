#include "archivebackend.h"
#include "qtlanguageservices.h"
#include <QFileInfo>

void ArchiveBackend::extractSourceEntry(const QString &archive, const QString &entry,
                                        const QString &output, const QString &format,
                                        const QString &password) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if (entry.isEmpty() || entry.contains(QChar(0)) || !QFileInfo(archive).isFile() ||
        output.isEmpty() || QFileInfo::exists(output) || format.contains(QChar(0))) {
        emit finished(false, QtLocalText("Invalid extraction file or stream.",
                                         "Недопустимый файл или поток извлечения."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    createTemporary_ = QFileInfo(output).absoluteFilePath();
    pendingPassword_ = password;
    QStringList arguments{"x", "-y", "-so", "-bso0", "-bse2", "-bsp2", "-spd", "-ssc"};
    if (!format.isEmpty())
        arguments.append("-t" + format);
    arguments.append({"--", archive_, entry});
    start(Job::SourceExtract, arguments);
}
