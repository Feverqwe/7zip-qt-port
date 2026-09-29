// Windows/Control/Dialog.cpp
// Adapted from 7-Zip (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "Dialog.h"

namespace NWindows {
namespace NControl {
bool IsDialogSizeOK(int xSize, int ySize, const IQtDialogScreenServices &services) {
    // it returns for system font. Real font uses another values
    const QSize v = services.GetDialogBaseUnits();
    const int x = v.width();
    const int y = v.height();

    const QRect rect = services.GetWorkAreaRect();
    const int wx = rect.width();
    const int wy = rect.height();
    return xSize / 4 * x <= wx && ySize / 8 * y <= wy;
}
} // namespace NControl
} // namespace NWindows
