#pragma once
#include "devices/deviceaccess_win.h"

enum class WindowsDeviceCommand : quint32 { Open = 1, Read = 2 };
struct WindowsDeviceRequest {
    WindowsDeviceCommand command = WindowsDeviceCommand::Open;
    quint32 size = 0;
    quint64 offset = 0;
};
struct WindowsDeviceReply {
    quint32 error = ERROR_SUCCESS;
    quint32 size = 0;
};

DWORD WindowsDeviceTransfer(HANDLE pipe, void *data, DWORD size, bool writing, bool overlapped,
                            const std::function<bool()> &cancelled = {});
// Takes ownership of pipe and helper process. Closing the last reader ends the
// elevated session. The helper accepts bounded read requests only.
std::shared_ptr<DeviceReadHandle> AdoptWindowsDevicePipe(HANDLE pipe, HANDLE helper);
