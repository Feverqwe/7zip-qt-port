#include "CPP/7zip/UI/FileManager/PanelSelect.h"
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

namespace {
CPanelSelection readSelection(const QTableWidget &table) {
    CPanelSelection selection;
    selection.Items.reserve(size_t(table.rowCount()));
    for (int row = 0; row < table.rowCount(); ++row) {
        const auto *item = table.item(row, 0);
        selection.Items.push_back({item->data(Qt::UserRole + 3).toString(),
                                   item->data(Qt::UserRole + 1).toBool(),
                                   table.selectionModel()->isRowSelected(row, QModelIndex())});
    }
    return selection;
}

void UpdateSelection(QTableWidget &table, const CPanelSelection &selection) {
    QItemSelection selected;
    for (int row = 0; row < table.rowCount(); ++row)
        if (selection.Items[size_t(row)].Selected)
            selected.select(table.model()->index(row, 0),
                            table.model()->index(row, table.columnCount() - 1));
    table.selectionModel()->select(selected,
                                   QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}
} // namespace

void MainWindow::SelectSpec(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    ComboDialog dialog(selectMode ? "Выделить" : "Снять выделение", "Маска:", "*", this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    auto selection = readSelection(*table_);
    const QString mask = dialog.value();
    selection.SelectSpec(QStringView(mask), selectMode);
    UpdateSelection(*table_, selection);
}

void MainWindow::SelectByType(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    auto selection = readSelection(*table_);
    selection.SelectByType(table_->currentRow(), selectMode);
    UpdateSelection(*table_, selection);
}

void MainWindow::SelectAll(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    auto selection = readSelection(*table_);
    selection.SelectAll(selectMode);
    UpdateSelection(*table_, selection);
}

void MainWindow::InvertSelection() {
    if (state_ != State::Filesystem && state_ != State::Viewing && state_ != State::Namespace)
        return;
    auto selection = readSelection(*table_);
    selection.InvertSelection();
    UpdateSelection(*table_, selection);
}
