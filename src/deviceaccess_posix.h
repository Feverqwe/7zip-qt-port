#pragma once
#include "deviceaccess.h"

// Takes ownership of an O_RDONLY descriptor, including on failure.
std::shared_ptr<DeviceReadHandle> AdoptReadOnlyDeviceDescriptor(int descriptor);
