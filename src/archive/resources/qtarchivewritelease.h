#pragma once
#include <QStringList>
#include <expected>
#include <memory>
#include <vector>
class QLockFile;

// Per-user native-writer ownership shared by File Manager and independent 7zG.
// Locks live outside selected input directories, so they cannot enter an archive.
class QtArchiveWriteLease final {
  public:
    ~QtArchiveWriteLease();
    QtArchiveWriteLease(QtArchiveWriteLease &&) noexcept;
    QtArchiveWriteLease &operator=(QtArchiveWriteLease &&) noexcept;
    QtArchiveWriteLease(const QtArchiveWriteLease &) = delete;
    QtArchiveWriteLease &operator=(const QtArchiveWriteLease &) = delete;
    [[nodiscard]] static std::expected<QtArchiveWriteLease, QString>
    Acquire(const QStringList &outputs);

  private:
    QtArchiveWriteLease();
    std::vector<std::unique_ptr<QLockFile>> locks_;
};
