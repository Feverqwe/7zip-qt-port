#pragma once
#include "devices/deviceaccess.h"

// Re-enumerates and validates the selected external, unmounted device. Never
// accepts a path to an arbitrary file. Called independently by the helper.
int MacOpenExternalDeviceDescriptor(const ExternalDevice &device, QString &error);
DeviceAccessResult MacAuthorizeExternalDevice(const ExternalDevice &device,
                                              const std::function<bool()> &cancelled);
