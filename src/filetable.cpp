#include "filetable.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QUrl>

namespace {
class FileRowDelegate final : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;
    bool eventFilter(QObject *editor, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            const auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                emit commitData(static_cast<QWidget *>(editor));
                emit closeEditor(static_cast<QWidget *>(editor));
                return true;
            }
        }
        return QStyledItemDelegate::eventFilter(editor, event);
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &original,
               const QModelIndex &index) const override {
        QStyleOptionViewItem option(original);
        if (index.data(Qt::UserRole + 19).toBool())
            option.state |= QStyle::State_Selected;
        auto *table = dynamic_cast<const FileTable *>(parent());
        if (!table || table->fullRowHighlight() || index.column() == 0) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }
        if (option.state.testFlag(QStyle::State_Selected))
            painter->fillRect(option.rect, option.palette.base());
        QStyleOptionViewItem narrowed(option);
        narrowed.state &= ~QStyle::State_Selected;
        QStyledItemDelegate::paint(painter, narrowed, index);
    }
};
} // namespace

QAbstractItemDelegate *CreateFileRowDelegate(QWidget *parent) {
    return new FileRowDelegate(parent);
}

FileTable::FileTable(int rows, int columns, QWidget *parent) : QTableWidget(rows, columns, parent) {
    setItemDelegate(CreateFileRowDelegate(this));
}

void FileTable::setFullRowHighlight(bool enabled) {
    if (fullRowHighlight_ == enabled)
        return;
    fullRowHighlight_ = enabled;
    viewport()->update();
}

QMimeData *FileTable::mimeData(const QList<QTableWidgetItem *> &items) const {
    auto *data = new QMimeData;
    if (!filesystem)
        return data;
    QList<QUrl> urls;
    for (QTableWidgetItem *item : items) {
        if (item->column() == 1)
            urls << QUrl::fromLocalFile(item->text());
    }
    data->setUrls(urls);
    return data;
}

void FileTable::startDrag(Qt::DropActions actions) {
    if (dragStarter)
        dragStarter();
    else
        QTableWidget::startDrag(actions);
}
void FileIconList::startDrag(Qt::DropActions actions) {
    if (dragStarter)
        dragStarter();
    else
        QListView::startDrag(actions);
}

bool ArchiveTableItem::operator<(const QTableWidgetItem &other) const {
    // Qt reverses the whole comparison for descending order. Compensate only
    // the folder grouping: upstream reverses property order after this check.
    const bool firstParent = data(Qt::UserRole + 6).toUInt() == UInt32(-1);
    const bool secondParent = other.data(Qt::UserRole + 6).toUInt() == UInt32(-1);
    if (firstParent || secondParent) {
        if (firstParent == secondParent)
            return false;
        return !tableWidget() ||
                       tableWidget()->horizontalHeader()->sortIndicatorOrder() == Qt::AscendingOrder
                   ? firstParent
                   : secondParent;
    }
    const bool folder = data(Qt::UserRole + 1).toBool();
    if (folder != other.data(Qt::UserRole + 1).toBool() && tableWidget())
        return tableWidget()->horizontalHeader()->sortIndicatorOrder() == Qt::AscendingOrder
                   ? folder
                   : !folder;
    auto *table = static_cast<FileTable *>(tableWidget());
    if (table && table->sourcePanel && table->sourcePanel->GetFolder()) {
        const int columnIndex = column();
        if (columnIndex >= 0 && columnIndex < table->columnCount()) {
            const UInt32 first = data(Qt::UserRole + 6).toUInt();
            const UInt32 second = other.data(Qt::UserRole + 6).toUInt();
            return table->sourcePanel->CompareItemIndices(first, second,
                                                          table->propertyId(columnIndex),
                                                          table->isRawProperty(columnIndex)) < 0;
        }
    }
    return QTableWidgetItem::operator<(other);
}
