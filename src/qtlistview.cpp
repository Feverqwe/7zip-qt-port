// Qt presentation adapter for Win32 CListView styles used by ListViewDialog.
#include "qtlistview.h"
#include "qtfilelisthover.h"

CListView::CListView(QWidget *parent) : QTreeWidget(parent), _hover(new QtFileListHover(*this)) {
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setTextElideMode(Qt::ElideNone);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void CListView::SetSingleClickActivation(bool enabled) { _hover->SetEnabled(enabled); }
