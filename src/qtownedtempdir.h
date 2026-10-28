#pragma once
#include "qtownedtempregistry.h"
#include <QTemporaryDir>

// Qt filename/cleanup service with a lease spanning the automatic cleanup.
class QtOwnedTempDir final {
    std::shared_ptr<QtOwnedTempLease> lease_;
    QTemporaryDir directory_;

  public:
    explicit QtOwnedTempDir(const QString &directoryTemplate = {});
    bool isValid() const { return directory_.isValid() && bool(lease_); }
    QString path() const { return directory_.path(); }
    QString filePath(const QString &name) const { return directory_.filePath(name); }
    std::shared_ptr<QtOwnedTempLease> lease() const { return lease_; }
};
