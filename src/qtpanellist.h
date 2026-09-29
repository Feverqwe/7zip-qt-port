// Qt control adapter for the selection/focus portion of CPanel's CListView.
#pragma once
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include <QAbstractItemView>
#include <QHash>
#include <QPointer>

class QtPanelList {
  public:
    void Attach(QAbstractItemView *view) {
        view_ = view;
        itemRows_.clear();
    }
    bool IsBound() const { return view_ && view_->model() && view_->selectionModel(); }
    int GetItemCount() const;
    int GetSelectedCount() const;
    int GetFocusedItem() const;
    bool IsItemSelected(int row) const;
    UString GetItemName(unsigned row) const;
    QString GetItemRelPath(unsigned row) const;
    bool GetItemParam(unsigned row, quintptr &param) const;
    void PrepareItemIndices();
    void ClearSelection();
    void DeleteListItems();
    void SelectItem(int row);
    void SetFocusedItem(int row);
    void EnsureVisible(int row);

  private:
    QPointer<QAbstractItemView> view_;
    QHash<unsigned, int> itemRows_;
};
