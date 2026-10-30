#include "Common/Common0.h"
#include "Windows/Handle.h"
#include "devices/deviceaccess_win.h"
#include <QMutex>
#include <QMutexLocker>
#include <limits>

namespace {
class WindowsReadHandle final : public DeviceReadHandle {
    NWindows::CHandle handle_;
    QMutex mutex_;

  public:
    explicit WindowsReadHandle(HANDLE handle) { handle_.Attach(handle); }
    qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) override {
        read = 0;
        if (offset > quint64(std::numeric_limits<LONGLONG>::max()))
            return qint32(E_INVALIDARG);
        QMutexLocker lock(&mutex_);
        LARGE_INTEGER position{};
        position.QuadPart = LONGLONG(offset);
        if (!SetFilePointerEx(handle_, position, nullptr, FILE_BEGIN))
            return qint32(HRESULT_FROM_WIN32(GetLastError()));
        DWORD count = 0;
        if (!ReadFile(handle_, data, size, &count, nullptr))
            return qint32(HRESULT_FROM_WIN32(GetLastError()));
        read = count;
        return 0;
    }
};
} // namespace

std::shared_ptr<DeviceReadHandle> AdoptWindowsReadHandle(HANDLE handle) {
    if (!handle || handle == INVALID_HANDLE_VALUE)
        return {};
    NWindows::CHandle source;
    source.Attach(handle);
    HANDLE readOnly = nullptr;
    // Remove write access even if a caller supplied a broader file handle.
    if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &readOnly,
                         FILE_GENERIC_READ, FALSE, 0))
        return {};
    try {
        return std::make_shared<WindowsReadHandle>(readOnly);
    } catch (...) {
        CloseHandle(readOnly);
        throw;
    }
}
