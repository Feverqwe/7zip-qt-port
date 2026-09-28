#include "qtpanellist.h"
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

void QtPanelList::PrepareItemIndices() {
    itemRows_.clear();
    for (int row = 0; row < GetItemCount(); ++row) {
        quintptr param;
        if (GetItemParam(unsigned(row), param))
            itemRows_.insert(unsigned(param), row);
    }
}

QString QtPanelList::GetItemRelPathForItem(unsigned itemIndex) const {
    const int row = itemRows_.value(itemIndex, -1);
    if (row < 0)
        throw 1;
    return GetItemRelPath(unsigned(row));
}
void QtPanelList::ClearSelection() {
    if (IsBound())
        view_->selectionModel()->clearSelection();
}

void QtPanelList::DeleteListItems() {
    itemRows_.clear();
    if (IsBound())
        view_->model()->removeRows(0, GetItemCount());
}
void QtPanelList::SelectItem(int row) {
    view_->selectionModel()->select(view_->model()->index(row, 0),
                                    QItemSelectionModel::Select | QItemSelectionModel::Rows);
}
void QtPanelList::SetFocusedItem(int row) {
    view_->selectionModel()->setCurrentIndex(view_->model()->index(row, 0),
                                             QItemSelectionModel::NoUpdate);
}
void QtPanelList::EnsureVisible(int row) {
    if (row >= 0 && row < GetItemCount())
        view_->scrollTo(view_->model()->index(row, 0));
}
