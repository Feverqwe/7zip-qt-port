// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSelect.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// Qt selection model and ComboDialog replace the upstream Win32 controls.

#include "../../../../mainwindow.h"
#include "../../../Common/Wildcard.h"

void MainWindow::SelectSpec(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    ComboDialog dialog(selectMode ? "Выделить" : "Снять выделение", "Маска:", "*", this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString mask = dialog.value();
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (DoesWildcardMatchName(QStringView(mask), QStringView(table_->item(row, 0)->text())))
            table_->selectionModel()->select(
                table_->model()->index(row, 0),
                (selectMode ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
                    QItemSelectionModel::Rows);
    }
}

void MainWindow::SelectByType(bool selectMode) {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    const int focusedItem = table_->currentRow();
    if (focusedItem < 0)
        return;
    const auto *focused = table_->item(focusedItem, 0);
    const QString name = focused->text();
    const bool isItemFolder = focused->data(Qt::UserRole + 1).toBool();
    if (isItemFolder) {
        for (int row = 0; row < table_->rowCount(); ++row) {
            if (table_->item(row, 0)->data(Qt::UserRole + 1).toBool() == isItemFolder)
                table_->selectionModel()->select(
                    table_->model()->index(row, 0),
                    (selectMode ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
                        QItemSelectionModel::Rows);
        }
    } else {
        const int pos = name.lastIndexOf('.');
        const QString mask = pos < 0 ? QString() : QStringLiteral("*") + name.mid(pos);
        for (int row = 0; row < table_->rowCount(); ++row) {
            const auto *item = table_->item(row, 0);
            if (!item->data(Qt::UserRole + 1).toBool() &&
                (pos < 0 ? item->text().lastIndexOf('.') < 0
                         : DoesWildcardMatchName(QStringView(mask), QStringView(item->text()))))
                table_->selectionModel()->select(
                    table_->model()->index(row, 0),
                    (selectMode ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
                        QItemSelectionModel::Rows);
        }
    }
}
