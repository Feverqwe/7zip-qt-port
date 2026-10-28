#include "qtarchivewritelease.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString Identity(QString path) {
    path = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    QStringList tail;
    QFileInfo info(path);
    while (info.canonicalFilePath().isEmpty()) {
        tail.prepend(info.fileName());
        const QString parent = info.absolutePath();
        if (parent == info.absoluteFilePath())
            break;
        info.setFile(parent);
    }
    path = info.canonicalFilePath();
    if (path.isEmpty())
        path = info.absoluteFilePath();
#ifdef Q_OS_WIN
    // QFileInfo preserves DOS short names; the source writer's Win32 calls
    // resolve them to the same output. Expand the existing ancestor first.
    const auto native = QDir::toNativeSeparators(path).toStdWString();
    const DWORD capacity = GetLongPathNameW(native.c_str(), nullptr, 0);
    if (capacity) {
        std::wstring longPath(capacity, L'\0');
        const DWORD size = GetLongPathNameW(native.c_str(), longPath.data(), capacity);
        if (size && size < capacity)
            path = QDir::fromNativeSeparators(QString::fromWCharArray(longPath.data(), int(size)));
    }
#endif
    for (const auto &part : tail)
        path = QDir(path).filePath(part);
#ifdef Q_OS_WIN
    auto parts = path.split('/');
    for (auto &part : parts)
        part.remove(QRegularExpression(QStringLiteral("[ .]+$")));
    path = parts.join('/');
#endif
    // A volume set shares ownership with any of its numbered output files.
    path.remove(QRegularExpression(QStringLiteral("\\.[0-9]{3,}$")));
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    // Conservative on case-sensitive macOS volumes: distinct case variants may
    // wait for each other, but case aliases on the default volume cannot race.
    path = path.toCaseFolded();
#endif
#ifdef Q_OS_MACOS
    path = path.normalized(QString::NormalizationForm_C);
#endif
    return path;
}
} // namespace

QtArchiveWriteLease::QtArchiveWriteLease() = default;
QtArchiveWriteLease::~QtArchiveWriteLease() = default;
QtArchiveWriteLease::QtArchiveWriteLease(QtArchiveWriteLease &&) noexcept = default;
QtArchiveWriteLease &QtArchiveWriteLease::operator=(QtArchiveWriteLease &&) noexcept = default;

std::expected<QtArchiveWriteLease, QString>
QtArchiveWriteLease::Acquire(const QStringList &outputs) {
    const QString cache = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    const QString root = cache + QStringLiteral("/sevenzip-desktop/archive-writers");
    if (cache.isEmpty() || !QDir().mkpath(root)) {
        return std::unexpected(
            QCoreApplication::translate("QtOperation", "Could not lock the archive for writing."));
    }
    QStringList identities;
    for (const auto &output : outputs)
        identities.append(Identity(output));
    identities.removeDuplicates();
    std::sort(identities.begin(), identities.end());
    QtArchiveWriteLease acquired;
    for (const auto &identity : identities) {
        const QString key = QString::fromLatin1(
            QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex());
        auto lock = std::make_unique<QLockFile>(root + '/' + key + ".lock");
        lock->setStaleLockTime(0); // Only a dead owner, never elapsed time, releases a live job.
        if (!lock->tryLock()) {
            return std::unexpected(QCoreApplication::translate(
                "QtOperation", "Another operation is writing to this archive."));
        }
        acquired.locks_.push_back(std::move(lock));
    }
    return acquired;
}
