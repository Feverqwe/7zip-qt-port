#include "archivebackend.h"
#include "qtlanguageservices.h"
#include "qtownedtempdir.h"
#include <QFile>
#include <QFileInfo>

void ArchiveBackend::extractSourceEntry(const QString &archive, const QString &entry,
                                        const QString &output, const QString &format,
                                        const QString &password, bool isAltStream) {
    if (busy()) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceextract",
                                                         "Another operation is still running."));
        return;
    }
    lastResult_ = E_FAIL;
    if (entry.isEmpty() || entry.contains(QChar(0)) || !QFileInfo(archive).isFile() ||
        output.isEmpty() || QFileInfo::exists(output) || format.contains(QChar(0))) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceextract",
                                                         "Invalid extraction file or stream."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    createTemporary_ = QFileInfo(output).absoluteFilePath();
    pendingPassword_ = password;
    QStringList arguments{"x", "-y", "-so", "-bso0", "-bse2", "-bsp2", "-spd", "-ssc"};
    // Original native kpidIsAltStream controls the Console transport. Unix
    // skips ADS without this switch; ordinary stdout must stay one payload.
    if (isAltStream)
        arguments.append("-sns");
    if (!format.isEmpty())
        arguments.append("-t" + format);
    sourceExtractSelection_.reset();
    QString selection = entry;
#ifdef Q_OS_MACOS
    // QProcess encodes arguments as filesystem names (NFD on macOS). Archive
    // item names are not filesystem arguments: retain their exact UTF-8 bytes.
    if (QFile::encodeName(entry) != entry.toUtf8()) {
        sourceExtractSelection_ = std::make_unique<QtOwnedTempDir>();
        QFile list(sourceExtractSelection_->filePath("selection.txt"));
        const QByteArray bytes = entry.toUtf8() + '\n';
        if (entry.contains('\n') || entry.contains('\r') || !sourceExtractSelection_->isValid() ||
            !list.open(QIODevice::WriteOnly) || list.write(bytes) != bytes.size()) {
            sourceExtractSelection_.reset();
            emit finished(
                false, QCoreApplication::translate("archivebackend_sourceextract",
                                                   "Cannot prepare exact archive item selection."));
            return;
        }
        arguments.append({"-scsUTF-8", "-i@" + list.fileName()});
        selection.clear();
    }
#endif
    arguments.append({"--", archive_});
    if (!selection.isEmpty())
        arguments.append(selection);
    start(Job::SourceExtract, arguments);
}
