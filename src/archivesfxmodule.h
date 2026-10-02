#pragma once
#include <QString>
#include <QtGlobal>

// Runtime asset provenance/host adapter. Archive and SFX algorithms remain in
// the official engine; source Update owns its module-presence decisions.
class ArchiveSfxModule final {
    QString path_, error_;
    quint32 version_ = 0;

  public:
    static ArchiveSfxModule Read(const QString &path, quint32 expectedVersion = 0);
    static ArchiveSfxModule Discover(quint32 expectedVersion = 0);
    bool available() const { return !path_.isEmpty(); }
    const QString &path() const { return path_; }
    const QString &error() const { return error_; }
    quint32 version() const { return version_; }
};
