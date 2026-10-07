#include "7zip/UI/FileManager/resource.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"

void MainWindow::completeListSelection() {
    if (scanSelectedState_)
        app_.GetFocusedPanel().RestoreLoadedSelection(*scanSelectedState_);
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
        app_.GetFocusedPanel().RestoreLoadedSelection(selection);
}

void MainWindow::handleSelectionClick(int row) {
    if (!selectionClickLeft_ || row < 0 || !app_.GetFocusedPanel()._mySelectMode)
        return;
    app_.GetFocusedPanel().OnLeftClick(row, selectionClickModifiers_);
    refreshActions();
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
