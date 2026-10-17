// Windows/Control/Dialog.cpp
// Adapted from 7-Zip (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "Dialog.h"
#include <QWidget>

namespace NWindows {
namespace NControl {
void CDialog::NormalizeSize(QWidget &window, const IQtDialogScreenServices &services,
                            bool fullNormalize) {
    const QRect workRect = services.GetWorkAreaRect();
    if (!workRect.isValid())
        return;
    const QRect rect = window.frameGeometry();
    if (!rect.isValid())
        return;
    int xs = rect.width();
    int ys = rect.height();
    if (!rect.intersects(workRect))
        return;
    const int xsW = workRect.width();
    const int ysW = workRect.height();
    if (xs <= xsW && ys <= ysW)
        return;
    if (fullNormalize) {
        // Qt can call source OnInit before exec/show. Request the state without
        // exposing a partially initialized modeless dialog.
        window.setWindowState(window.windowState() | Qt::WindowMaximized);
        return;
    }
    int x = workRect.left();
    int y = workRect.top();
    if (xs < xsW)
        x += (xsW - xs) / 2;
    else
        xs = xsW;
    if (ys < ysW)
        y += (ysW - ys) / 2;
    else
        ys = ysW;
    const QSize frame = rect.size() - window.size();
    window.resize(QSize(xs, ys) - frame);
    window.move(x, y);
}

void CDialog::NormalizePosition(QWidget &window, const IQtDialogScreenServices &services,
                                const IQtDialogScreenServices &parentServices) {
    const QRect workRect = services.GetWorkAreaRect();
    if (!workRect.isValid())
        return;
    QRect rect2 = workRect;
    bool useWorkArea = true;
    if (QWidget *parent = window.parentWidget()) {
        const QRect workRectParent = parentServices.GetWorkAreaRect();
        if (!workRectParent.isValid())
            return;
        if (workRectParent == workRect) {
            rect2 = parent->isWindow() ? parent->frameGeometry()
                                       : QRect(parent->mapToGlobal(QPoint()), parent->size());
            if (rect2.isValid() && workRect.contains(rect2))
                useWorkArea = false;
        }
    }
    const QRect rect = window.frameGeometry();
    if (!rect.isValid())
        return;
    if (useWorkArea) {
        if (workRect.contains(rect))
            return;
        if (!rect.intersects(workRect))
            return;
        rect2 = workRect;
    }
    const int xs = rect.width();
    const int ys = rect.height();
    const int xs2 = rect2.width();
    const int ys2 = rect2.height();
    if (xs <= xs2 && ys <= ys2) {
        const int x = rect2.left() + (xs2 - xs) / 2;
        const int y = rect2.top() + (ys2 - ys) / 2;
        if (x != rect.left() || y != rect.top())
            window.move(x, y);
        return;
    }
}

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
