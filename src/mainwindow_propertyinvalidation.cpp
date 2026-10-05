#include "Windows/PropVariant.h"
#include "mainwindow.h"

// Keep the numeric size role used by interaction in sync with source stats.
// Display text is requested from the source panel when Qt paints a cell.
void MainWindow::redrawSourceProperties() {
    auto &panel = app_.GetFocusedPanel();
    if (!table_ || !panel.GetFolder() || app_.QtOperations().closed())
        return;
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const bool sorting = table_->isSortingEnabled();
    const QSignalBlocker block(table_);
    table_->setSortingEnabled(false);
    for (int row = 0; row < table_->rowCount(); ++row) {
        const auto *name = table_->item(row, 0);
        if (!name || !name->data(Qt::UserRole + 6).isValid())
            continue;
        const UInt32 index = name->data(Qt::UserRole + 6).toUInt();
        for (int column = 0; column < table_->columnCount(); ++column) {
            auto *item = table_->item(row, column);
            if (!item || column == 0 || column == 1 || column == 5 || column == 6)
                continue;
            item->setData(Qt::DisplayRole, QVariant());
            if (fileTable->propertyId(column) == kpidSize) {
                NWindows::NCOM::CPropVariant size;
                panel.GetFolder()->GetProperty(index, kpidSize, &size);
                item->setData(Qt::UserRole + 2,
                              size.vt == VT_UI8 ? QVariant::fromValue(quint64(size.uhVal.QuadPart))
                                                : QVariant());
            }
        }
    }
    table_->setSortingEnabled(sorting);
    table_->viewport()->update();
    refreshActions();
}
