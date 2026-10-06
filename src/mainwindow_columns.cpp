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
    int extraCount = 0;
    FOR_VECTOR(i, columns) {
        const PROPID id = columns[i].ID;
        if (id != kpidName && id != kpidSize && id != kpidPackSize && id != kpidMTime)
            ++extraCount;
    }
    table_->setRowCount(0);
    fileTable->textFolder = panel.GetFolder();
    table_->setColumnCount(7 + extraCount);
    fileTable->propertyIds = {kpidName,  kpidPath,      kpidSize,      kpidPackSize,
                              kpidMTime, kpidExtension, kpidNoProperty};
    fileTable->rawProperties.fill(false, 7 + extraCount);
    QStringList labels;
    for (PROPID id : fileTable->propertyIds)
        labels << (id == kpidNoProperty ? QtLocalText("Order", "Порядок")
                                        : QString::fromWCharArray(GetNameOfProperty(id, nullptr)));
    int next = 7;
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
        else {
            column = next++;
            fileTable->propertyIds.append(prop.ID);
            labels << ToQString(prop.Name);
        }
        fileTable->rawProperties[column] = prop.IsRawProp;
        labels[column] = ToQString(prop.Name);
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
    for (int column : {2, 3, 4}) {
        if (columns.FindItem_for_PropID(fileTable->propertyId(column)) < 0)
            table_->setColumnHidden(column, true);
    }
    const CPropColumns &visible = panel.GetVisibleColumns();
    FOR_VECTOR(i, visible) {
        const int column = fileTable->columnForProperty(visible[i].ID);
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

void MainWindow::saveColumns() {
    if (configuringColumns_ || !table_)
        return;
    auto &panel = app_.GetFocusedPanel();
    if (panel.GetColumns().IsEmpty())
        return;
    const auto *fileTable = static_cast<FileTable *>(table_);
    CPropColumns visible;
    const CPropColumns &columns = panel.GetColumns();
    FOR_VECTOR(i, columns) {
        const int column = fileTable->columnForProperty(columns[i].ID);
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
        const int column = fileTable->columnForProperty(prop.ID);
        if (column < 0)
            continue;
        QAction *option = menu.addAction(ToQString(prop.Name));
        option->setCheckable(true);
        option->setChecked(prop.IsVisible);
        option->setEnabled(i != 0);
        connect(option, &QAction::toggled, this, [this, column, index = i](bool) {
            auto &source = app_.GetFocusedPanel();
            source.ShowColumnsContextMenu(index);
            const bool visible = source.GetColumns()[index].IsVisible;
            {
                const QSignalBlocker blocker(table_->horizontalHeader());
                table_->setColumnHidden(column, !visible);
                if (visible)
                    table_->horizontalHeader()->moveSection(
                        table_->horizontalHeader()->visualIndex(column), table_->columnCount() - 1);
            }
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
    const CPropColumns &columns = panel.GetColumns();
    const int index = columns.FindItem_for_PropID(fileTable->propertyId(column));
    return index >= 0 ? panel.GetItemText(itemIndex, columns[index]) : QString();
}

Qt::Alignment MainWindow::sourceColumnAlignment(int column) const {
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const CPanel &panel = app_.Panels[app_.GetFocusedPanelIndex()];
    const CPropColumns &columns = panel.GetColumns();
    const int index = columns.FindItem_for_PropID(fileTable->propertyId(column));
    return index >= 0 ? GetColumnAlign(columns[index].ID, columns[index].Type)
                      : Qt::AlignLeft | Qt::AlignVCenter;
}

void MainWindow::redrawTimestampColumns() {
    if (!table_ || !app_.GetFocusedPanel().GetFolder())
        return;
    const auto *fileTable = static_cast<const FileTable *>(table_);
    const CPropColumns &columns = app_.GetFocusedPanel().GetColumns();
    for (int column = 0; column < table_->columnCount(); ++column) {
        const int index = columns.FindItem_for_PropID(fileTable->propertyId(column));
        if (index < 0 || columns[index].Type != VT_FILETIME)
            continue;
        for (int row = 0; row < table_->rowCount(); ++row)
            if (auto *item = table_->item(row, column))
                item->setData(Qt::DisplayRole, QVariant());
    }
    table_->viewport()->update();
}
