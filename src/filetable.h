#pragma once

#include "7zip/PropID.h"
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "Common/MyWindows.h"
#include <QListView>
#include <QTableWidget>
#include <QVector>
#include <functional>
class CPanel;
struct CPropColumn;
QAbstractItemDelegate *CreateFileRowDelegate(QWidget *parent);

class FileTable : public QTableWidget {
  public:
    explicit FileTable(int rows, int columns, QWidget *parent = nullptr);
    void setFullRowHighlight(bool enabled);
    bool fullRowHighlight() const { return fullRowHighlight_; }
    bool filesystem = true;
    CPanel *sourcePanel = nullptr;
    CMyComPtr<IFolderFolder> textFolder;
    std::function<void()> dragStarter;
    QVector<PROPID> propertyIds;
    QVector<bool> rawProperties;
    QVector<int> sourceColumnIndices;
    const CPropColumn *sourceColumn(int column) const;
    int columnForCatalogProperty(unsigned index) const;
    int columnForVisibleProperty(unsigned index) const;
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
    void startDrag(Qt::DropActions actions) override;
    QMimeData *mimeData(const QList<QTableWidgetItem *> &items) const override;

  private:
    bool fullRowHighlight_ = false;
};

class FileIconList final : public QListView {
  public:
    using QListView::QListView;
    std::function<void()> dragStarter;

  protected:
    void startDrag(Qt::DropActions actions) override;
};

class ArchiveTableItem : public QTableWidgetItem {
  public:
    using QTableWidgetItem::QTableWidgetItem;
    QVariant data(int role) const override;
    void setData(int role, const QVariant &value) override;
    bool operator<(const QTableWidgetItem &other) const override;

  private:
    mutable QVariant sourceText_;
    mutable bool sourceTextDefined_ = false;
};
