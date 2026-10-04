#include "7zip/UI/FileManager/resource.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"

void MainWindow::completeListSelection() {
    if (scanSelectedState_)
        app_.GetFocusedPanel().RefreshListCtrl(*scanSelectedState_);
    scanSelectedState_.reset();
    auto paths = std::move(scanSelectedPaths_);
    scanSelectedPaths_.reset();
    if (paths)
        selectPaths(*paths);
}

void MainWindow::restoreListSelection(const CSelectedState &selection) {
    if (scanningDisk_ || scanningArchive_)
        scanSelectedState_ = selection;
    else
        app_.GetFocusedPanel().RefreshListCtrl(selection);
}

void MainWindow::handleSelectionClick(int row) {
    if (!selectionClickLeft_ || row < 0 || !app_.GetFocusedPanel()._mySelectMode)
        return;
    app_.GetFocusedPanel().OnLeftClick(row, selectionClickModifiers_);
    refreshActions();
}

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
    const UInt32 titleID = selectMode ? IDS_SELECT : IDS_DESELECT;
    ComboDialog dialog(QtLangText(titleID), QtLangText(IDS_SELECT_MASK), "*", this);
    dialog.BindLanguage(titleID, IDS_SELECT_MASK);
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
