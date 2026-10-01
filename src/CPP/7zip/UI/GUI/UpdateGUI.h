// Adapted from 7-Zip CPP/7zip/UI/GUI/UpdateGUI.cpp at 0766b73.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "7zip/UI/Common/Property.h"
#include "7zip/UI/Common/UpdateAction.h"
#include "CompressDialog.h"

namespace NCompressDialog {
const NUpdateArchive::CActionSet *FindUpdateActionSet(NUpdateMode::EEnum mode);
CObjectVector<CProperty> GetCompressionProperties(const CInfo &info, bool is7z);
} // namespace NCompressDialog
