// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelKey.cpp at 0766b73.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "7zip/UI/FileManager/resource.h"
#include "mainwindow.h"

// Qt adapter for PanelKey.cpp::OnKeyDown and Panel.cpp::CMyComboBoxEdit.
bool MainWindow::handlePanelKey(QObject *watched, QKeyEvent *key) {
    const bool address = watched == path_ || watched == pathCombo_;
    const bool list = watched == table_ || watched == table_->viewport() || watched == iconList_ ||
                      (iconList_ && watched == iconList_->viewport());
    if (!address && !list)
        return false;
    if ((key->key() == Qt::Key_F1 || key->key() == Qt::Key_F2) &&
        key->modifiers() == Qt::AltModifier) {
        app_.m_PanelCallbackImp[app_.GetFocusedPanelIndex()].SetFocusToPath(
            key->key() == Qt::Key_F1 ? 0 : 1);
        return true;
    }
    if (address) {
        if (key->key() == Qt::Key_Tab && key->modifiers() == Qt::NoModifier) {
            pathCombo_->hidePopup();
            app_.GetFocusedPanel().SetFocusToList();
            return true;
        }
        return false;
    }
    if (key->key() == Qt::Key_Tab && key->modifiers() == Qt::NoModifier) {
        app_.m_PanelCallbackImp[app_.GetFocusedPanelIndex()].OnTab();
        return true;
    }
    CPanel &panel = app_.GetFocusedPanel();
    if (panel._mySelectMode) {
        if (key->key() == Qt::Key_Shift) {
            panel.BeginShiftSelection();
            selectionShiftSequence_ = true;
        } else if (key->key() == Qt::Key_Insert && key->modifiers() == Qt::NoModifier) {
            panel.OnInsert();
            refreshActions();
            return true;
        } else if (key->modifiers() == Qt::ShiftModifier &&
                   (key->key() == Qt::Key_Up || key->key() == Qt::Key_Down ||
                    key->key() == Qt::Key_Left || key->key() == Qt::Key_Right)) {
            if (!selectionShiftSequence_) {
                panel.BeginShiftSelection();
                selectionShiftSequence_ = true;
            }
            panel.OnArrowWithShift();
            QTimer::singleShot(0, this, [this] {
                if (!app_.QtOperations().closed()) {
                    app_.GetFocusedPanel().OnShiftSelectMessage();
                    refreshActions();
                }
            });
        } else if (!key->modifiers().testFlag(Qt::ShiftModifier))
            selectionShiftSequence_ = false;
    }
    if (key->key() == Qt::Key_F4 && key->modifiers() == Qt::ShiftModifier) {
        if (createFile_->isEnabled())
            createFile_->trigger();
        return true;
    }
    // PanelKey.cpp handles VK_BACK without testing Alt, Ctrl or Shift.
    if (key->key() == Qt::Key_Backspace) {
        if (parent_->isEnabled())
            parent_->trigger();
        return true;
    }
    if (key->key() == Qt::Key_Backslash && key->modifiers() == Qt::NoModifier) {
        app_.ExecuteCommand(IDM_OPEN_ROOT_FOLDER);
        return true;
    }
    if (handleSelectionKey(key))
        return true;
    if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 && key->modifiers() == Qt::AltModifier) {
        app_.ExecuteCommand(830 + key->key() - Qt::Key_0);
        return true;
    }
    if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 &&
        key->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
        app_.ExecuteCommand(810 + key->key() - Qt::Key_0);
        return true;
    }
    if (!(key->modifiers() &
          (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier))) {
        if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) &&
            openItem_->isEnabled()) {
            openItem_->trigger();
            return true;
        }
    }
    return false;
}
