#pragma once
#include "qtownedtempdir.h"
#include <QHash>
#include <QSet>
#include <QString>
#include <expected>
#include <memory>

// Distinct native images give original SetCodecs globals one immutable owner.
class ArchiveCodecImage final {
    QtOwnedTempDir directory_;
    QHash<QString, QString> origins_;
    QString module_;
    ArchiveCodecImage() = default;
    bool prepare(const QString &module);
    bool copyFile(const QString &source, const QString &relative);
    bool copyFolder(const QString &source, const QString &relative, QSet<QString> ancestors = {});

  public:
    enum class Error { TemporaryStorageUnavailable, RuntimeCopyFailed };
    [[nodiscard]] static std::expected<std::unique_ptr<ArchiveCodecImage>, Error>
    Create(const QString &module);
    QString baseFolder() const { return directory_.path() + '/'; }
    QString module() const { return module_; }
    QString originalPath(const QString &imagePath) const;
};
