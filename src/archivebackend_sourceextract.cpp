#include "archivebackend.h"
#include "archivereadpolicy.h"
#include "qtlanguageservices.h"
#include "qtownedtempdir.h"
#include <QFile>
#include <QFileInfo>

Int32 ArchiveBackend::sourceExtractErrorOperation() const {
    using namespace NArchive::NExtract::NOperationResult;
    Int32 result = output_.contains("Open Errors:") ? kHeadersError : kDataError;
    // Reverse the official Console/ExtractCallbackConsole error labels.
    const struct {
        const char *text;
        Int32 result;
    } errors[] = {{"CRC Failed", kCRCError},
                  {"Data Error", kDataError},
                  {"Unsupported Method", kUnsupportedMethod},
                  {"Unavailable data", kUnavailable},
                  {"Unexpected end of data", kUnexpectedEnd},
                  {"There are some data after the end of the payload data", kDataAfterEnd},
                  {"Is not archive", kIsNotArc},
                  {"Headers Error", kHeadersError},
                  {"Wrong password", kWrongPassword}};
    for (QString line : output_.split('\n')) {
        // Console progress can share the physical line with this error.
        const auto errorStart = line.indexOf("ERROR: ");
        if (errorStart < 0)
            continue;
        line = line.mid(errorStart);
        for (const auto &error : errors)
            if (line.startsWith(QStringLiteral("ERROR: ") + error.text))
                result = error.result;
    }
    return result;
}

void ArchiveBackend::extractSourceEntry(const QString &archive, const QString &entry,
                                        const QString &output, const QString &format,
                                        const QString &password, bool isAltStream) {
    if (busy()) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceextract",
                                                         "Another operation is still running."));
        return;
    }
    lastResult_ = E_FAIL;
    sourceExtractOperation_ = -1;
    if (entry.isEmpty() || entry.contains(QChar(0)) || !QFileInfo(archive).isFile() ||
        output.isEmpty() || QFileInfo::exists(output) || format.contains(QChar(0))) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceextract",
                                                         "Invalid extraction file or stream."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    createTemporary_ = QFileInfo(output).absoluteFilePath();
    sourceExtractRecovery_ = IsArchiveRecoveryFormat(format);
    if (format.isEmpty() && hasListedSourceArchive(archive_))
        sourceExtractRecovery_ =
            IsArchiveRecoveryFormat(listedSourceArchives_.value(archive_).Format);
    pendingPassword_ = password;
    QStringList arguments{"x", "-y", "-so", "-bso0", "-bse2", "-bsp2", "-spd", "-ssc"};
    // Explicitly choose one payload: ordinary files must not implicitly include ADS.
    arguments.append(isAltStream ? "-sns" : "-sns-");
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
