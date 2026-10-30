#pragma once

#include "Common/MyWindows.h"
#include "Windows/SecurityUtils.h"
#include <QByteArray>
#include <QString>

// Disposable, blank VHD owned by this fixture. The native attachment is always
// read-only, has no drive letter and lasts only as long as this handle.
class ReadOnlyVirtualDisk final {
    HANDLE disk_ = INVALID_HANDLE_VALUE;
    DWORD error_ = ERROR_SUCCESS;
    QString device_;
    bool attached_ = false;
    NWindows::NSecurity::CAccessToken token_;
    TOKEN_PRIVILEGES previousPrivilege_{};
    bool privilegeAdjusted_ = false;

  public:
    static constexpr quint64 Size = 32 * 1024 * 1024;
    explicit ReadOnlyVirtualDisk(const QString &path);
    ~ReadOnlyVirtualDisk();
    ReadOnlyVirtualDisk(const ReadOnlyVirtualDisk &) = delete;
    ReadOnlyVirtualDisk &operator=(const ReadOnlyVirtualDisk &) = delete;
    bool valid() const { return attached_ && !device_.isEmpty(); }
    DWORD error() const { return error_; }
    QString device() const { return device_; }
    QByteArray readDevice() const;
    DWORD detach();
};
