#pragma once
#include <QStringList>
#include <memory>
#include <vector>
class QLockFile;

// Per-user native-writer ownership shared by File Manager and independent 7zG.
// Locks live outside selected input directories, so they cannot enter an archive.
class QtArchiveWriteLease final {
  public:
    QtArchiveWriteLease();
    ~QtArchiveWriteLease();
    bool acquire(const QStringList &outputs, QString &error);

  private:
    std::vector<std::unique_ptr<QLockFile>> locks_;
};
