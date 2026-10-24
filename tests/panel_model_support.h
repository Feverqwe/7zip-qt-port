#pragma once
#include <QAbstractItemModel>
#include <QAbstractItemView>

inline bool SetPanelData(const QModelIndex &index, const QVariant &value, int role) {
    return const_cast<QAbstractItemModel *>(index.model())->setData(index, value, role);
}

inline bool PanelIndexSelected(const QModelIndex &index) {
    const auto *view = qobject_cast<QAbstractItemView *>(index.model()->parent());
    return view && view->selectionModel()->isSelected(index);
}

inline QModelIndexList PanelMatches(const QAbstractItemView &view, const QString &text) {
    return view.model()->match(view.model()->index(0, 0), Qt::DisplayRole, text, -1,
                               Qt::MatchExactly);
}
