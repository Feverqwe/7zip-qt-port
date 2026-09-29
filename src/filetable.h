#pragma once

#include "7zip/PropID.h"
#include "Common/MyWindows.h"
#include <QTableWidget>
#include <QVector>
class CPanel;

class FileTable : public QTableWidget {
  public:
    using QTableWidget::QTableWidget;
    bool filesystem = true;
    CPanel *sourcePanel = nullptr;
    QVector<PROPID> propertyIds;
    QVector<bool> rawProperties;
    PROPID propertyId(int column) const {
        return column >= 0 && column < propertyIds.size() ? propertyIds[column] : kpidNoProperty;
    }
    bool isRawProperty(int column) const {
        return column >= 0 && column < rawProperties.size() && rawProperties[column];
    }
    int columnForProperty(PROPID id) const {
        for (int column = 0; column < propertyIds.size(); ++column) {
            if (column != 1 && column != 5 && column != 6 && propertyIds[column] == id)
                return column;
        }
        return id == kpidExtension ? 5 : id == kpidNoProperty ? 6 : -1;
    }

  protected:
    QMimeData *mimeData(const QList<QTableWidgetItem *> &items) const override;
};

class ArchiveTableItem : public QTableWidgetItem {
  public:
    using QTableWidgetItem::QTableWidgetItem;
    bool operator<(const QTableWidgetItem &other) const override;
};
