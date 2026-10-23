#include "archivebackend.h"
#include "archivereadpolicy.h"
#include "qtarchivereadoperation.h"
#include "qtlanguageservices.h"
#include "qtownedtempdir.h"
#include <QFile>
#include <QFileInfo>

void ArchiveBackend::finishSourceExtractOperation(const QtArchiveReadOperation &operation) {
    using namespace NArchive::NExtract::NOperationResult;
    sourceExtractOperation_ = operation.OutputOperation;
    if (sourceExtractOperation_ == kOK && !operation.Errors.isEmpty())
        sourceExtractOperation_ = operation.Errors.last().Result;
    if (sourceExtractOperation_ == kOK && !operation.OpenErrors.isEmpty()) {
        // The source stream API must not publish bytes as verified when opening
        // reported damage or trailing data. Preserve the CLI recovery contract
        // using CArcErrorInfo flags instead of inferring it from printed labels.
        UInt32 flags = 0;
        for (const auto &error : operation.OpenErrors)
            flags |= error.ErrorFlags | error.WarningFlags;
        sourceExtractOperation_ = flags & kpv_ErrorFlags_UnexpectedEnd  ? kUnexpectedEnd
                                  : flags & kpv_ErrorFlags_DataAfterEnd ? kDataAfterEnd
                                                                        : kHeadersError;
    }
    const bool partial = (lastResult_ == E_FAIL || lastResult_ == S_FALSE) &&
                         sourceExtractRecovery_ && operation.OutputCreated &&
                         operation.OutputClosed && sourceExtractOperation_ > kOK;
    if (partial)
        lastResult_ = S_FALSE;
    if (operation.OutputCreated && lastResult_ != S_OK &&
        (!partial || QFileInfo(createTemporary_).size() <= 0))
        QFile::remove(createTemporary_);
}

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
    sourceExtractEntry_ = entry;
    sourceExtractAltStream_ = isAltStream;
    createTemporary_ = QFileInfo(output).absoluteFilePath();
    sourceExtractRecovery_ = IsArchiveRecoveryFormat(format);
    if (format.isEmpty() && hasListedSourceArchive(archive_))
        sourceExtractRecovery_ =
            IsArchiveRecoveryFormat(listedSourceArchives_.value(archive_).Format);
    pendingPassword_ = password;
    if (startNativeRead(Job::SourceExtract, archive_, password, format))
        return;
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
