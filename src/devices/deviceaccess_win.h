#pragma once
#include "Common/Common0.h"
#include "devices/deviceaccess.h"
#include <qt_windows.h>

// Native-only boundary. No filesystem handlers are linked into the reader.
HANDLE WindowsOpenExternalDevice(const ExternalDevice &device, DWORD &error);
std::shared_ptr<DeviceReadHandle> AdoptWindowsReadHandle(HANDLE handle);
DeviceAccessResult WindowsAuthorizeExternalDevice(const ExternalDevice &device,
                                                  const std::function<bool()> &cancelled);
