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
        // In one-panel mode both upstream shortcuts target the active panel.
        pathCombo_->setFocus(Qt::ShortcutFocusReason);
        pathCombo_->showPopup();
        return true;
    }
    if (address) {
        if (key->key() == Qt::Key_Tab && key->modifiers() == Qt::NoModifier) {
            pathCombo_->hidePopup();
            viewStack_->currentWidget()->setFocus(Qt::TabFocusReason);
            return true;
        }
        return false;
    }
    if (key->key() == Qt::Key_F4 && key->modifiers() == Qt::ShiftModifier) {
        if (createFile_->isEnabled())
            createFile_->trigger();
        return true;
    }
    if (handleSelectionKey(key))
        return true;
    if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 && key->modifiers() == Qt::AltModifier) {
        openBookmark(key->key() - Qt::Key_0);
        return true;
    }
    if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 &&
        key->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
        saveBookmark(key->key() - Qt::Key_0);
        return true;
    }
    if (!(key->modifiers() &
          (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier))) {
        if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) &&
            openItem_->isEnabled()) {
            openItem_->trigger();
            return true;
        }
        if (key->key() == Qt::Key_Backspace && parent_->isEnabled()) {
            parent_->trigger();
            return true;
        }
    }
    return false;
}
