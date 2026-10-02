#pragma once
#include "qtownedtempdir.h"
#include <QHash>
#include <QSet>
#include <QString>

// Distinct native images give original SetCodecs globals one immutable owner.
class ArchiveCodecImage final {
    QtOwnedTempDir directory_;
    QHash<QString, QString> origins_;
    QString module_, engine_;
    bool copyFile(const QString &source, const QString &relative);
    bool copyFolder(const QString &source, const QString &relative, QSet<QString> ancestors = {});

  public:
    bool prepare(const QString &module, const QString &engine);
    QString baseFolder() const { return directory_.path() + '/'; }
    QString module() const { return module_; }
    QString engine() const { return engine_; }
    QString originalPath(const QString &imagePath) const;
};
