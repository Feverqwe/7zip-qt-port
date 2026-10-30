#include "filemanager/qtfilemanagerdrag.h"
#include "filemanager/window/mainwindow.h"
#include <QContextMenuEvent>
#include <QMouseEvent>

bool QtFileManagerDrag::HandleRightDrag(QWidget &widget, QEvent &event) {
    const unsigned panel = window_.panelIndexFor(&widget);
    if (revoked_ || panel >= kNumPanelsMax)
        return false;
    const auto &ui = window_.panelUi_[panel];
    if ((!ui.table || &widget != ui.table->viewport()) &&
        (!ui.iconList || &widget != ui.iconList->viewport()))
        return false;
    if (event.type() == QEvent::ContextMenu) {
        const auto &context = static_cast<QContextMenuEvent &>(event);
        if (context.reason() == QContextMenuEvent::Mouse) {
            if (rightPressWidget_ == &widget) {
                // macOS presents a context menu on press; defer until release
                // so a right-button drag can reach the source drop menu.
                contextDeferred_ = true;
                return true;
            }
            if (rightDragConsumed_)
                return true;
        }
    } else if (event.type() == QEvent::MouseButtonPress) {
        const auto &mouse = static_cast<QMouseEvent &>(event);
        rightDragConsumed_ = false;
        contextDeferred_ = false;
        rightPressWidget_ = mouse.button() == Qt::RightButton ? &widget : nullptr;
        rightPressPosition_ = mouse.position().toPoint();
    } else if (event.type() == QEvent::MouseMove && rightPressWidget_ == &widget) {
        const auto &mouse = static_cast<QMouseEvent &>(event);
        if ((mouse.buttons() & Qt::RightButton) && PanelEnabled(panel) &&
            (mouse.position().toPoint() - rightPressPosition_).manhattanLength() >=
                QApplication::startDragDistance()) {
            rightDragConsumed_ = true;
            contextDeferred_ = false;
            rightPressWidget_ = nullptr;
            StartDrag(panel, true);
            return true;
        }
    } else if (event.type() == QEvent::MouseButtonRelease) {
        const auto &mouse = static_cast<QMouseEvent &>(event);
        if (mouse.button() == Qt::RightButton && rightPressWidget_ == &widget) {
            rightPressWidget_ = nullptr;
            if (contextDeferred_) {
                contextDeferred_ = false;
                QContextMenuEvent context(QContextMenuEvent::Mouse, mouse.position().toPoint(),
                                          mouse.globalPosition().toPoint(), mouse.modifiers());
                QCoreApplication::sendEvent(&widget, &context);
            }
        }
    }
    return false;
}
