// Qt presentation adapter for Win32 CListView styles used by ListViewDialog.
#include "qtlistview.h"
#include <QApplication>
#include <QMouseEvent>

CListView::CListView(QWidget *parent) : QTreeWidget(parent) {
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setTextElideMode(Qt::ElideNone);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    _hoverTimer.setSingleShot(true);
    // Qt has no LVS_EX_TRACKSELECT: use a local 400ms dwell interval.
    _hoverTimer.setInterval(400);
    connect(&_hoverTimer, &QTimer::timeout, this, [this] {
        if (_singleClick && _hoverIndex.isValid() &&
            indexAt(_hoverPosition).siblingAtColumn(0) == _hoverIndex &&
            QApplication::mouseButtons() == Qt::NoButton &&
            QApplication::keyboardModifiers() == Qt::NoModifier)
            selectionModel()->setCurrentIndex(_hoverIndex, QItemSelectionModel::ClearAndSelect |
                                                               QItemSelectionModel::Rows);
    });
}

void CListView::SetSingleClickActivation(bool enabled) {
    _singleClick = enabled;
    setMouseTracking(enabled);
    viewport()->setMouseTracking(enabled);
    _hoverTimer.stop();
    _hoverIndex = QPersistentModelIndex();
}

void CListView::mouseMoveEvent(QMouseEvent *event) {
    QTreeWidget::mouseMoveEvent(event);
    if (!_singleClick)
        return;
    const QPersistentModelIndex index(indexAt(event->position().toPoint()).siblingAtColumn(0));
    _hoverPosition = event->position().toPoint();
    if (event->buttons() != Qt::NoButton || event->modifiers() != Qt::NoModifier ||
        !index.isValid()) {
        _hoverTimer.stop();
        _hoverIndex = QPersistentModelIndex();
    } else if (index != _hoverIndex) {
        _hoverIndex = index;
        _hoverTimer.start();
    }
}

void CListView::mousePressEvent(QMouseEvent *event) {
    _hoverTimer.stop();
    _hoverIndex = QPersistentModelIndex();
    QTreeWidget::mousePressEvent(event);
}

bool CListView::viewportEvent(QEvent *event) {
    if (event->type() == QEvent::Leave) {
        _hoverTimer.stop();
        _hoverIndex = QPersistentModelIndex();
    }
    return QTreeWidget::viewportEvent(event);
}
