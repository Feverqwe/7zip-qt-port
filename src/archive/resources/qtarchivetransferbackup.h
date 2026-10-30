#pragma once
#include "platform/temp/qtownedtempdir.h"
#include <QString>
#include <expected>
#include <functional>
#include <memory>

enum class ArchiveTransferBackupError { TemporaryStorageUnavailable, CopyFailed };

// A leased destination snapshot spanning the source-owned Copy/Move workflow.
class QtArchiveTransferBackup final {
    QtOwnedTempDir directory_;
    QString destination_;
    explicit QtArchiveTransferBackup(QString destination);

  public:
    [[nodiscard]] static std::expected<std::unique_ptr<QtArchiveTransferBackup>,
                                       ArchiveTransferBackupError>
    Create(const QString &destination);
    bool restore(const std::function<bool()> &closeArchive = {}) const;
};
