#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/PropertyName.h"
#include "mainwindow.h"
#include "qtlanguageservices.h"
#include <QPointer>

void MainWindow::configureColumns(bool initialized) {
    auto &panel = app_.GetFocusedPanel();
    if (!initialized && panel.InitColumns() != S_OK)
        return;
    configuringColumns_ = true;
    const QSignalBlocker blocked(table_->horizontalHeader());
    auto *fileTable = static_cast<FileTable *>(table_);
    const CPropColumns &columns = panel.GetColumns();
    table_->setRowCount(0);
    fileTable->textFolder = panel.GetFolder();
    fileTable->propertyIds = {kpidName,  kpidPath,      kpidSize,      kpidPackSize,
                              kpidMTime, kpidExtension, kpidNoProperty};
    fileTable->rawProperties.fill(false, 7);
    fileTable->sourceColumnIndices.fill(-1, 7);
    fileTable->visibleColumnIndices.clear();
    QStringList labels;
    for (PROPID id : fileTable->propertyIds)
        labels << (id == kpidNoProperty ? QCoreApplication::translate("mainwindow_columns", "Order")
                                        : QString::fromWCharArray(GetNameOfProperty(id, nullptr)));
    FOR_VECTOR(i, columns) {
        const CPropColumn &prop = columns[i];
        int column = -1;
        if (prop.ID == kpidName)
            column = 0;
        else if (prop.ID == kpidSize)
            column = 2;
        else if (prop.ID == kpidPackSize)
            column = 3;
        else if (prop.ID == kpidMTime)
            column = 4;
        if (column < 0 || prop.IsRawProp || fileTable->sourceColumnIndices[column] >= 0) {
            column = fileTable->propertyIds.size();
            fileTable->propertyIds.append(prop.ID);
            fileTable->rawProperties.append(false);
            fileTable->sourceColumnIndices.append(-1);
            labels << ToQString(prop.Name);
        }
        fileTable->sourceColumnIndices[column] = int(i);
        fileTable->rawProperties[column] = prop.IsRawProp;
        labels[column] = ToQString(prop.Name);
    }
    table_->setColumnCount(fileTable->propertyIds.size());
    FOR_VECTOR(i, columns) {
        const CPropColumn &prop = columns[i];
        const int column = fileTable->columnForCatalogProperty(i);
        table_->setColumnWidth(column, qMax(32, int(prop.Width)));
        table_->setColumnHidden(column, !prop.IsVisible);
    }
    table_->setHorizontalHeaderLabels(labels);
    for (int column = 0; column < table_->columnCount(); ++column)
        table_->horizontalHeaderItem(column)->setData(
            Qt::InitialSortOrderRole, CPanel::GetDefaultSortAscending(fileTable->propertyId(column))
                                          ? Qt::AscendingOrder
                                          : Qt::DescendingOrder);
    for (int column : {1, 5, 6})
        table_->setColumnHidden(column, true);
    for (int column : {0, 2, 3, 4}) {
        if (!fileTable->sourceColumn(column))
            table_->setColumnHidden(column, true);
    }
    const CPropColumns &visible = panel.GetVisibleColumns();
    QVector<int> visibleIndices(table_->columnCount(), -1);
    FOR_VECTOR(i, visible) {
        const int column = fileTable->columnForVisibleProperty(i);
        if (column >= 0)
            visibleIndices[column] = int(i);
    }
    fileTable->visibleColumnIndices = visibleIndices;
    updateIconColumn();
    CIntArr columnOrder;
    const bool ordered = panel.GetColumnOrder(columnOrder);
    FOR_VECTOR(i, visible) {
        const int column =
            fileTable->columnForVisibleProperty(ordered ? unsigned(columnOrder[i]) : i);
        if (column >= 0)
            table_->horizontalHeader()->moveSection(table_->horizontalHeader()->visualIndex(column),
                                                    int(i));
    }
    sortColumn_ = fileTable->columnForProperty(panel._sortID);
    if (sortColumn_ < 0 || table_->isColumnHidden(sortColumn_))
        sortColumn_ = panel._sortID == kpidNoProperty ? 6 : 0;
    sortOrder_ = panel._ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
    configuringColumns_ = false;
}

void MainWindow::updateIconColumn() {
    if (iconList_)
        iconList_->setModelColumn(
            qMax(0, static_cast<FileTable *>(table_)->columnForVisibleProperty(0)));
}

void MainWindow::saveColumns() {
    if (configuringColumns_ || !table_)
        return;
    auto &panel = app_.GetFocusedPanel();
    if (panel.GetColumns().IsEmpty())
        return;
    const auto *fileTable = static_cast<FileTable *>(table_);
    CPropColumns visible;
    const CPropColumns &columns = panel.GetVisibleColumns();
    FOR_VECTOR(i, columns) {
        const int column = fileTable->columnForVisibleProperty(i);
        if (column < 0 || table_->isColumnHidden(column))
            continue;
        CPropColumn prop = columns[i];
        prop.IsVisible = true;
        prop.Order = 0;
        for (int visual = 0; visual < table_->horizontalHeader()->visualIndex(column); ++visual)
            if (!table_->isColumnHidden(table_->horizontalHeader()->logicalIndex(visual)))
                ++prop.Order;
        prop.Width = UInt32(table_->horizontalHeader()->sectionSize(column));
        visible.Add(prop);
    }
    panel.SaveListViewInfo(visible);
}

void MainWindow::showColumnsMenu(const QPoint &pos) {
    auto &panel = app_.GetFocusedPanel();
    const CPropColumns &columns = panel.GetColumns();
    auto *fileTable = static_cast<FileTable *>(table_);
    QMenu menu(this);
    FOR_VECTOR(i, columns) {
        const CPropColumn &prop = columns[i];
        const int column = fileTable->columnForCatalogProperty(i);
        if (column < 0)
            continue;
        QAction *option = menu.addAction(ToQString(prop.Name));
        option->setCheckable(true);
        option->setChecked(prop.IsVisible);
        option->setEnabled(i != 0);
        connect(option, &QAction::toggled, this, [this, index = i](bool) {
            auto &source = app_.GetFocusedPanel();
            const int removed =
                source.GetColumns()[index].IsVisible
                    ? source.GetVisibleColumns().FindItem_for_PropID(source.GetColumns()[index].ID)
                    : -1;
            source.ShowColumnsContextMenu(index);
            const bool visible = source.GetColumns()[index].IsVisible;
            {
                const QSignalBlocker blocker(table_->horizontalHeader());
                auto *view = static_cast<FileTable *>(table_);
                if (visible)
                    view->appendSourceColumn(index);
                else if (removed >= 0)
                    view->removeSourceColumn(unsigned(removed));
            }
            updateIconColumn();
            sortColumn_ = static_cast<FileTable *>(table_)->columnForProperty(source._sortID);
            sortOrder_ = source._ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
            {
                const QSignalBlocker blocker(table_->horizontalHeader());
                table_->horizontalHeader()->setSortIndicator(sortColumn_, sortOrder_);
            }
            saveColumns();
        });
    }
    const QPointer<QWidget> focused = QApplication::focusWidget();
    menu.exec(table_->horizontalHeader()->mapToGlobal(pos));
    if (focused)
        focused->setFocus(Qt::PopupFocusReason);
}

QString MainWindow::sourceColumnText(UInt32 itemIndex, int column) {
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const auto &panel = app_.GetFocusedPanel();
    const CPropColumn *prop = fileTable->sourceColumn(column);
    return prop ? panel.GetItemText(itemIndex, *prop) : QString();
}

Qt::Alignment MainWindow::sourceColumnAlignment(int column) const {
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const CPropColumn *prop = fileTable->sourceColumn(column);
    return prop ? GetColumnAlign(prop->ID, prop->Type) : Qt::AlignLeft | Qt::AlignVCenter;
}

void MainWindow::redrawTimestampColumns() {
    if (!table_ || !app_.GetFocusedPanel().GetFolder())
        return;
    const auto *fileTable = static_cast<const FileTable *>(table_);
    for (int column = 0; column < table_->columnCount(); ++column) {
        const CPropColumn *prop = fileTable->sourceColumn(column);
        if (!prop || prop->Type != VT_FILETIME)
            continue;
        for (int row = 0; row < table_->rowCount(); ++row)
            if (auto *item = table_->item(row, column))
                item->setData(Qt::DisplayRole, QVariant());
    }
    table_->viewport()->update();
}
