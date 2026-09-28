#include "CPP/7zip/UI/FileManager/PanelSelect.h"
#include "mainwindow.h"

namespace {
CPanelSelection readSelection(const QTableWidget &table) {
    CPanelSelection selection;
    selection.Items.reserve(size_t(table.rowCount()));
    for (int row = 0; row < table.rowCount(); ++row) {
        const auto *item = table.item(row, 0);
        selection.Items.push_back({item->text(), item->data(Qt::UserRole + 1).toBool(),
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
    if (state_ != State::Filesystem && state_ != State::Viewing)
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
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    auto selection = readSelection(*table_);
    selection.SelectByType(table_->currentRow(), selectMode);
    UpdateSelection(*table_, selection);
}

void MainWindow::SelectAll(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    auto selection = readSelection(*table_);
    selection.SelectAll(selectMode);
    UpdateSelection(*table_, selection);
}

void MainWindow::InvertSelection() {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    auto selection = readSelection(*table_);
    selection.InvertSelection();
    UpdateSelection(*table_, selection);
}
