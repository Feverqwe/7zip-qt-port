#include "mainwindow.h"

// Adapted from PanelKey.cpp::OnKeyDown(): VK_ADD, VK_SUBTRACT, VK_MULTIPLY.
// Qt identifies the numeric keypad with KeypadModifier on every platform.
bool MainWindow::handleSelectionKey(QKeyEvent *key) {
    const auto modifiers = key->modifiers();
    if (!modifiers.testFlag(Qt::KeypadModifier) || modifiers.testFlag(Qt::MetaModifier))
        return false;
    QAction *command = nullptr;
    if (key->key() == Qt::Key_Asterisk)
        command = invertSelection_;
    else if (key->key() == Qt::Key_Plus || key->key() == Qt::Key_Minus) {
        const bool select = key->key() == Qt::Key_Plus;
        if (modifiers.testFlag(Qt::AltModifier))
            command = select ? selectType_ : deselectType_;
        else if (modifiers.testFlag(Qt::ShiftModifier))
            command = select ? selectAll_ : deselectAll_;
        else if (!select || !modifiers.testFlag(Qt::ControlModifier))
            command = select ? selectMask_ : deselectMask_;
        else
            return true;
    } else
        return false;
    if (command->isEnabled())
        command->trigger();
    return true;
}

void MainWindow::SelectSpec(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    ComboDialog dialog(selectMode ? "Выделить" : "Снять выделение", "Маска:", "*", this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selected;
    panel.SaveSelectedState(selected);
    panel.SelectSpec(ToUString(dialog.value()), selectMode);
    refreshActions();
}

void MainWindow::SelectByType(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selected;
    panel.SaveSelectedState(selected);
    panel.SelectByType(selectMode);
    refreshActions();
}

void MainWindow::SelectAll(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selected;
    panel.SaveSelectedState(selected);
    panel.SelectAll(selectMode);
    refreshActions();
}

void MainWindow::InvertSelection() {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selected;
    panel.SaveSelectedState(selected);
    panel.InvertSelection();
    refreshActions();
}
