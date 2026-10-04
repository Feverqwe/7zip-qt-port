// Qt presentation adapter for upstream NControl::CListView operations.
#pragma once
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include <QTreeWidget>

class QtFileListHover;

class CListView : public QTreeWidget {
    QtFileListHover *_hover;

  public:
    explicit CListView(QWidget *parent);
    // Qt adapters for LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT.
    void SetSingleClickActivation(bool enabled);
    void InsertItem(unsigned index, const UString &s) {
        auto *item = new QTreeWidgetItem;
        item->setText(0, ToQString(s));
        insertTopLevelItem((int)index, item);
    }
    void SetSubItem(unsigned index, int column, const UString &s) {
        topLevelItem((int)index)->setText(column, ToQString(s));
    }
    int GetNextSelectedItem(int index) const {
        while (++index < topLevelItemCount())
            if (topLevelItem(index)->isSelected())
                return index;
        return -1;
    }
    int GetFocusedItem() const { return indexOfTopLevelItem(currentItem()); }
    void SetItemState_FocusedSelected(int index) {
        setCurrentItem(topLevelItem(index), 0, QItemSelectionModel::ClearAndSelect);
    }
    void DeleteItem(unsigned index) { delete takeTopLevelItem((int)index); }
    void SetColumnWidthAuto(int column) { resizeColumnToContents(column); }
};
