#include "devicepipe_win.h"
#include "Windows/Handle.h"
#include <QMutex>
#include <QMutexLocker>
#include <QScopeGuard>

DWORD WindowsDeviceTransfer(HANDLE pipe, void *data, DWORD size, bool writing, bool overlapped,
                            const std::function<bool()> &cancelled) {
    auto *bytes = static_cast<char *>(data);
    while (size) {
        OVERLAPPED operation{};
        NWindows::CHandle event;
        if (overlapped) {
            event.Attach(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!event.IsCreated())
                return GetLastError();
            operation.hEvent = event;
        }
        DWORD count = 0;
        const BOOL completed =
            writing ? WriteFile(pipe, bytes, size, &count, overlapped ? &operation : nullptr)
                    : ReadFile(pipe, bytes, size, &count, overlapped ? &operation : nullptr);
        if (!completed) {
            const DWORD error = GetLastError();
            if (!overlapped || error != ERROR_IO_PENDING)
                return error;
            DWORD result = WAIT_TIMEOUT;
            {
                auto cancelPending = qScopeGuard([&] {
                    DWORD ignored = 0;
                    CancelIoEx(pipe, &operation);
                    GetOverlappedResult(pipe, &operation, &ignored, TRUE);
                });
                for (unsigned attempt = 0; attempt < 200 && result == WAIT_TIMEOUT; ++attempt) {
                    if (cancelled && cancelled())
                        break;
                    result = WaitForSingleObject(event, 50);
                }
                if (result == WAIT_OBJECT_0) {
                    if (!GetOverlappedResult(pipe, &operation, &count, FALSE))
                        return GetLastError();
                    cancelPending.dismiss();
                }
            }
            if (result != WAIT_OBJECT_0) {
                return cancelled && cancelled() ? ERROR_CANCELLED : ERROR_TIMEOUT;
            }
        }
        if (!count || count > size)
            return ERROR_BROKEN_PIPE;
        bytes += count;
        size -= count;
    }
    return ERROR_SUCCESS;
}

namespace {
class WindowsPipeHandle final : public DeviceReadHandle {
    NWindows::CHandle pipe_;
    NWindows::CHandle helper_;
    QMutex mutex_;

  public:
    WindowsPipeHandle(HANDLE pipe, HANDLE helper) {
        pipe_.Attach(pipe);
        helper_.Attach(helper);
    }
    ~WindowsPipeHandle() override {
        pipe_.Close();
        if (helper_.IsCreated() && WaitForSingleObject(helper_, 500) == WAIT_TIMEOUT)
            TerminateProcess(helper_, ERROR_CANCELLED);
    }
    qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) override {
        read = 0;
        if (!size || size > 1024 * 1024)
            return qint32(E_INVALIDARG);
        QMutexLocker lock(&mutex_);
        WindowsDeviceRequest request{WindowsDeviceCommand::Read, size, offset};
        WindowsDeviceReply reply;
        DWORD error = WindowsDeviceTransfer(pipe_, &request, sizeof(request), true, true);
        if (!error)
            error = WindowsDeviceTransfer(pipe_, &reply, sizeof(reply), false, true);
        if (!error)
            error = reply.error;
        if (!error && reply.size > size)
            error = ERROR_INVALID_DATA;
        if (!error && reply.size)
            error = WindowsDeviceTransfer(pipe_, data, reply.size, false, true);
        if (error) {
            pipe_.Close();
            return qint32(HRESULT_FROM_WIN32(error));
        }
        read = reply.size;
        return 0;
    }
};
} // namespace

std::shared_ptr<DeviceReadHandle> AdoptWindowsDevicePipe(HANDLE pipe, HANDLE helper) {
    try {
        return std::make_shared<WindowsPipeHandle>(pipe, helper);
    } catch (...) {
        CloseHandle(pipe);
        CloseHandle(helper);
        throw;
    }
}
