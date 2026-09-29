#pragma once

#include <QTableWidget>
class CPanel;

class FileTable : public QTableWidget {
  public:
    using QTableWidget::QTableWidget;
    bool filesystem = true;
    CPanel *sourcePanel = nullptr;

  protected:
    QMimeData *mimeData(const QList<QTableWidgetItem *> &items) const override;
};

class ArchiveTableItem : public QTableWidgetItem {
  public:
    using QTableWidgetItem::QTableWidgetItem;
    bool operator<(const QTableWidgetItem &other) const override;
};
