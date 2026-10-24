#pragma once

#include "7zip/PropID.h"
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "Common/MyWindows.h"
#include "filepanelmodel.h"
#include <QListView>
#include <QTableView>
#include <QVector>
#include <functional>
class CPanel;
struct CPropColumn;
QAbstractItemDelegate *CreateFileRowDelegate(QWidget *parent);

class FileTable : public QTableView {
    Q_OBJECT
  public:
    explicit FileTable(int columns, QWidget *parent = nullptr);
    void setFullRowHighlight(bool enabled);
    void handleDefaultKeyPress(QKeyEvent *event);
    void invalidateLanguageText();
    bool fullRowHighlight() const { return fullRowHighlight_; }
    FilePanelModel *panelModel() const { return model_; }
    int rowCount() const { return model_->rowCount(); }
    int columnCount() const { return model_->columnCount(); }
    void clearRows() { model_->removeRows(0, rowCount()); }
    int currentRow() const { return currentIndex().row(); }
    void setCurrentCell(int row, int column) { setCurrentIndex(model_->index(row, column)); }
    void setCurrentCell(int row, int column, QItemSelectionModel::SelectionFlags flags) {
        selectionModel()->setCurrentIndex(model_->index(row, column), flags);
    }
    void sortItems(int column, Qt::SortOrder order = Qt::AscendingOrder) {
        sortByColumn(column, order);
    }
    bool filesystem = true;
    CPanel *sourcePanel = nullptr;
    // Published callback identity only. CPanel owns the provider and must be
    // able to close its archive streams before deleting nested temporary files.
    IFolderFolder *textFolder = nullptr;
    std::function<void()> dragStarter;
    QVector<PROPID> propertyIds;
    QVector<bool> rawProperties;
    QVector<int> sourceColumnIndices;
    QVector<int> visibleColumnIndices;
    const CPropColumn *sourceColumn(int column) const;
    int columnForCatalogProperty(unsigned index) const;
    int columnForVisibleProperty(unsigned index) const;
    void removeSourceColumn(unsigned index);
    void appendSourceColumn(unsigned catalogIndex);
    bool takeSourceColumnClick();
    bool deferSourceColumnClick(std::function<void()> action);
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

  private:
    bool fullRowHighlight_ = false;
    FilePanelModel *model_;
};

class FileIconList final : public QListView {
  public:
    using QListView::QListView;
    void SetListViewMode(int mode);
    void handleDefaultKeyPress(QKeyEvent *event);
    std::function<void()> dragStarter;

  protected:
    void startDrag(Qt::DropActions actions) override;
    void changeEvent(QEvent *event) override;

  private:
    int mode_ = -1;
};
