#pragma once

#include <QTableWidget>

class FileTable : public QTableWidget {
  public:
    using QTableWidget::QTableWidget;
    bool filesystem = true;

  protected:
    QMimeData *mimeData(const QList<QTableWidgetItem *> &items) const override;
};

class ArchiveTableItem : public QTableWidgetItem {
  public:
    using QTableWidgetItem::QTableWidgetItem;
    bool operator<(const QTableWidgetItem &other) const override;
};
