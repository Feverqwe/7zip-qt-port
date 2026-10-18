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
    void handleDefaultKeyPress(QKeyEvent *event);
    void invalidateLanguageText();
    bool fullRowHighlight() const { return fullRowHighlight_; }
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
    QMimeData *mimeData(const QList<QTableWidgetItem *> &items) const override;

  private:
    bool fullRowHighlight_ = false;
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

class ArchiveTableItem : public QTableWidgetItem {
    friend class FileTable;

  public:
    using QTableWidgetItem::QTableWidgetItem;
    QVariant data(int role) const override;
    void setData(int role, const QVariant &value) override;
    bool operator<(const QTableWidgetItem &other) const override;

  private:
    mutable QVariant sourceText_;
    mutable bool sourceTextDefined_ = false;
    mutable bool sourceTextStale_ = false;
};
