#include "archive/backend/archivebackend.h"
#include "archive/resources/qtarchivetransferbackup.h"

std::expected<std::unique_ptr<QtArchiveTransferBackup>, ArchiveTransferBackupError>
ArchiveBackend::saveArchiveTransferDestination(const QString &path) const {
    return QtArchiveTransferBackup::Create(path);
}
