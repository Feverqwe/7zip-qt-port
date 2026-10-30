// Windows/Control/Dialog.h
// Adapted from 7-Zip (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "ui/dialogs/qtdialogscreenservices.h"

class QWidget;

namespace NWindows {
namespace NControl {
class CDialog {
  public:
    static void NormalizeSize(QWidget &window, const IQtDialogScreenServices &services,
                              bool fullNormalize = false);
    static void NormalizePosition(QWidget &window, const IQtDialogScreenServices &services,
                                  const IQtDialogScreenServices &parentServices);
};
bool IsDialogSizeOK(int xSize, int ySize, const IQtDialogScreenServices &services);
} // namespace NControl
} // namespace NWindows
