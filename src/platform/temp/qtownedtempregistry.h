#pragma once
#include <QString>
#include <QVector>
#include <memory>
class QLockFile;

struct QtOwnedTempEntry {
    QString key;
    QString path;
    QString identity;
    bool externallyProtected = false;
    bool active = false;
};

// Creation/active-session ownership only. Native BrowseDialog2 owns deletion.
// There is no source registry: Win32 BrowseDialog2 enumerates arbitrary OS temp.
class QtOwnedTempLease final {
    friend class QtOwnedTempRegistry;
    QString root_;
    QtOwnedTempEntry entry_;
    std::unique_ptr<QLockFile> lock_;
    QtOwnedTempLease(QString root, QtOwnedTempEntry entry, std::unique_ptr<QLockFile> lock);
    bool setExternalProtection(bool value);

  public:
    ~QtOwnedTempLease();
    bool ProtectExternal();
    bool ExternalFinished();
    bool MatchesObjectAt(const QString &path) const;
    bool RelocateAfterMove(const QString &path);
    const QString &path() const { return entry_.path; }
};

class QtOwnedTempRegistry final {
    QString root_;

  public:
    explicit QtOwnedTempRegistry(const QString &root = {});
    // Called immediately after an application-owned object is created, never
    // to discover ownership from a name prefix or archive-provided marker.
    std::shared_ptr<QtOwnedTempLease> RegisterCreated(const QString &path) const;
    QVector<QtOwnedTempEntry> Entries() const;
    // Claims one inactive object and revalidates its persisted OS identity.
    std::shared_ptr<QtOwnedTempLease> ClaimForDeletion(const QtOwnedTempEntry &entry) const;
    const QString &root() const { return root_; }
};
