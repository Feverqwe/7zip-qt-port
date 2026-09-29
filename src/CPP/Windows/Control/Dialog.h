// Windows/Control/Dialog.h
// Adapted from 7-Zip (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "qtdialogscreenservices.h"

namespace NWindows {
namespace NControl {
bool IsDialogSizeOK(int xSize, int ySize, const IQtDialogScreenServices &services);
}
} // namespace NWindows
