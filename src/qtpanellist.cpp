#include "qtpanellist.h"
#include <QBrush>
#include <QItemSelectionModel>

int QtPanelList::GetItemCount() const { return IsBound() ? view_->model()->rowCount() : 0; }
int QtPanelList::GetSelectedCount() const {
    return IsBound() ? view_->selectionModel()->selectedRows().size() : 0;
}
int QtPanelList::GetFocusedItem() const {
    return IsBound() ? view_->selectionModel()->currentIndex().row() : -1;
}
bool QtPanelList::IsItemSelected(int row) const {
    return IsBound() && view_->selectionModel()->isRowSelected(row, QModelIndex());
}
UString QtPanelList::GetItemName(unsigned row) const {
    const QModelIndex index = view_->model()->index((int)row, 0);
    const QVariant name = index.data(Qt::UserRole + 3);
    return ToUString(name.isValid() ? name.toString() : index.data().toString());
}
QString QtPanelList::GetItemRelPath(unsigned row) const {
    return view_->model()->index((int)row, 0).data().toString();
}
bool QtPanelList::GetItemParam(unsigned row, quintptr &param) const {
    if (!IsBound())
        return false;
    const QModelIndex index = view_->model()->index(int(row), 0);
    if (!index.isValid())
        return false;
    const QVariant value = index.data(Qt::UserRole + 6);
    if (!value.isValid()) {
        param = row; // Fixtures/legacy snapshots without an explicit source param.
        return true;
    }
    bool ok;
    param = value.toULongLong(&ok);
    return ok;
}

void QtPanelList::ClearSelection() {
    if (IsBound())
        view_->selectionModel()->clearSelection();
}

void QtPanelList::DeleteListItems() {
    if (IsBound())
        view_->model()->removeRows(0, GetItemCount());
}
void QtPanelList::SelectItem(int row) {
    view_->selectionModel()->select(view_->model()->index(row, 0),
                                    QItemSelectionModel::Select | QItemSelectionModel::Rows);
}
void QtPanelList::SetItemState_Selected(int row, bool selected) {
    if (!IsBound() || row < 0 || row >= GetItemCount())
        return;
    view_->selectionModel()->select(
        view_->model()->index(row, 0),
        (selected ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) |
            QItemSelectionModel::Rows);
}
void QtPanelList::SelectOnly(int row) {
    if (!IsBound())
        return;
    view_->selectionModel()->select(view_->model()->index(row, 0),
                                    QItemSelectionModel::ClearAndSelect |
                                        QItemSelectionModel::Rows);
}
void QtPanelList::SetItemMarked(int row, bool marked) {
    if (!IsBound() || row < 0 || row >= GetItemCount())
        return;
    const QVariant brush = marked ? QVariant(QBrush(QColor(255, 192, 192))) : QVariant();
    for (int column = 0; column < view_->model()->columnCount(); ++column) {
        const QModelIndex index = view_->model()->index(row, column);
        if (index.data(Qt::BackgroundRole) != brush)
            view_->model()->setData(index, brush, Qt::BackgroundRole);
    }
}
void QtPanelList::SetSingleSelection(bool enabled) {
    if (view_)
        view_->setSelectionMode(enabled ? QAbstractItemView::SingleSelection
                                        : QAbstractItemView::ExtendedSelection);
}
void QtPanelList::SetFocusedItem(int row) {
    view_->selectionModel()->setCurrentIndex(view_->model()->index(row, 0),
                                             QItemSelectionModel::NoUpdate);
}
void QtPanelList::EnsureVisible(int row) {
    if (row >= 0 && row < GetItemCount())
        view_->scrollTo(view_->model()->index(row, 0));
}
