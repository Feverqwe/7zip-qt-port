#pragma once

#include "Common/MyTypes.h"
#include <string>
#include <vector>

class QComboBox;

bool ParseVolumeSizes(const std::wstring &s, std::vector<UInt64> &values);
void AddVolumeItems(QComboBox &volumeCombo);
UInt64 GetNumberOfVolumes(UInt64 size, const std::vector<UInt64> &volSizes);
