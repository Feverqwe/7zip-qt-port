#pragma once
#include "qtownedtempdir.h"
#include <QString>

class ArchiveBackend;

// A leased destination snapshot spanning the source-owned Copy/Move workflow.
class QtArchiveTransferBackup final {
    friend class ArchiveBackend;
    QtOwnedTempDir directory_;
    QString destination_;
    explicit QtArchiveTransferBackup(QString destination);
    bool save();

  public:
    bool restore() const;
};
