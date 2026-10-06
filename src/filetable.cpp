#include "filetable.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QUrl>

namespace {
class FileColumnHeader final : public QHeaderView {
  public:
    using QHeaderView::QHeaderView;
    bool columnClick = false;
    bool dispatchingClick = false;
    std::function<void()> sortClick;

  protected:
    void mousePressEvent(QMouseEvent *event) override {
        {
            const QScopedValueRollback<bool> click(dispatchingClick,
                                                   event->button() == Qt::LeftButton);
            QHeaderView::mousePressEvent(event);
        }
        finishSortClick();
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        {
            const QScopedValueRollback<bool> click(dispatchingClick,
                                                   event->button() == Qt::LeftButton);
            QHeaderView::mouseReleaseEvent(event);
        }
        finishSortClick();
    }

  private:
    void finishSortClick() {
        if (!sortClick)
            return;
        auto action = std::move(sortClick);
        sortClick = {};
        columnClick = true;
        action();
        columnClick = false;
    }
};

bool sameColumnIdentity(const CPropColumn &left, const CPropColumn &right) {
    return left.ID == right.ID && left.Type == right.Type && left.IsRawProp == right.IsRawProp &&
           left.Name == right.Name;
}
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
        const QVariant foreground = index.data(Qt::ForegroundRole);
        if (foreground.isValid())
            option.palette.setColor(QPalette::HighlightedText,
                                    qvariant_cast<QBrush>(foreground).color());
        if (index.data(Qt::UserRole + 19).toBool())
            option.state |= QStyle::State_Selected;
        auto *table = dynamic_cast<const FileTable *>(parent());
        const int primary = table ? table->columnForVisibleProperty(0) : -1;
        if (!table || table->fullRowHighlight() || index.column() == (primary < 0 ? 0 : primary)) {
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
    setHorizontalHeader(new FileColumnHeader(Qt::Horizontal, this));
    horizontalHeader()->setSectionsClickable(true);
}

bool FileTable::takeSourceColumnClick() {
    auto *header = dynamic_cast<FileColumnHeader *>(horizontalHeader());
    if (!header)
        return false;
    const bool clicked = header->columnClick;
    header->columnClick = false;
    return clicked;
}

bool FileTable::deferSourceColumnClick(std::function<void()> action) {
    auto *header = dynamic_cast<FileColumnHeader *>(horizontalHeader());
    if (!header || !header->dispatchingClick)
        return false;
    // Finish Qt's automatic sort before projecting the source click result.
    header->sortClick = std::move(action);
    return true;
}

const CPropColumn *FileTable::sourceColumn(int column) const {
    if (!sourcePanel || column < 0)
        return nullptr;
    if (column < visibleColumnIndices.size()) {
        const int index = visibleColumnIndices[column];
        const CPropColumns &visible = sourcePanel->GetVisibleColumns();
        if (index >= 0 && unsigned(index) < visible.Size())
            return &visible[unsigned(index)];
    }
    const CPropColumns &columns = sourcePanel->GetColumns();
    if (!sourceColumnIndices.isEmpty()) {
        const int index = column < sourceColumnIndices.size() ? sourceColumnIndices[column] : -1;
        if (index < 0 || unsigned(index) >= columns.Size())
            return nullptr;
        const CPropColumn &prop = columns[unsigned(index)];
        return prop.ID == propertyId(column) && prop.IsRawProp == isRawProperty(column) ? &prop
                                                                                        : nullptr;
    }
    // Standalone adapters without a publication map retain their ID binding.
    const int index = columns.FindItem_for_PropID(propertyId(column));
    return index >= 0 ? &columns[unsigned(index)] : nullptr;
}

int FileTable::columnForCatalogProperty(unsigned index) const {
    return sourceColumnIndices.indexOf(int(index));
}

int FileTable::columnForVisibleProperty(unsigned index) const {
    if (!sourcePanel || index >= sourcePanel->GetVisibleColumns().Size())
        return -1;
    if (!visibleColumnIndices.isEmpty())
        return visibleColumnIndices.indexOf(int(index));
    const CPropColumns &visible = sourcePanel->GetVisibleColumns();
    const CPropColumn &prop = visible[index];
    unsigned occurrence = 0;
    for (unsigned previous = 0; previous < index; ++previous)
        if (sameColumnIdentity(visible[previous], prop))
            ++occurrence;
    for (int column = 0; column < columnCount(); ++column) {
        const CPropColumn *candidate = sourceColumn(column);
        if (candidate && sameColumnIdentity(*candidate, prop)) {
            if (occurrence == 0)
                return column;
            --occurrence;
        }
    }
    return -1;
}

void FileTable::removeSourceColumn(unsigned index) {
    const int column = visibleColumnIndices.indexOf(int(index));
    if (column >= 0)
        setColumnHidden(column, true);
    for (int &visible : visibleColumnIndices) {
        if (visible == int(index))
            visible = -1;
        else if (visible > int(index))
            --visible;
    }
}

void FileTable::appendSourceColumn(unsigned catalogIndex) {
    const QSignalBlocker blocked(this);
    const QSignalBlocker selectionBlocked(selectionModel());
    const QModelIndexList selected = selectionModel()->selectedRows();
    int column = -1;
    for (int candidate = 0; candidate < sourceColumnIndices.size(); ++candidate)
        if (sourceColumnIndices[candidate] == int(catalogIndex) &&
            visibleColumnIndices[candidate] < 0) {
            column = candidate;
            break;
        }
    const unsigned index = sourcePanel->GetVisibleColumns().Size() - 1;
    const CPropColumn &prop = sourcePanel->GetVisibleColumns()[index];
    if (column < 0) {
        column = columnCount();
        propertyIds.append(prop.ID);
        rawProperties.append(prop.IsRawProp);
        sourceColumnIndices.append(int(catalogIndex));
        visibleColumnIndices.append(-1);
        setColumnCount(column + 1);
        auto *label = new QTableWidgetItem(ToQString(prop.Name));
        label->setData(Qt::InitialSortOrderRole, CPanel::GetDefaultSortAscending(prop.ID)
                                                     ? Qt::AscendingOrder
                                                     : Qt::DescendingOrder);
        setHorizontalHeaderItem(column, label);
        for (int row = 0; row < rowCount(); ++row) {
            const auto *primary = dynamic_cast<const ArchiveTableItem *>(item(row, 0));
            if (!primary)
                continue;
            auto *cell = new ArchiveTableItem(*primary);
            cell->setData(Qt::DisplayRole, QVariant());
            cell->setData(Qt::DecorationRole, QVariant());
            cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
            cell->setTextAlignment(GetColumnAlign(prop.ID, prop.Type));
            setItem(row, column, cell);
        }
    } else {
        for (int row = 0; row < rowCount(); ++row)
            if (auto *cell = item(row, column))
                cell->setData(Qt::DisplayRole, QVariant());
    }
    visibleColumnIndices[column] = int(index);
    setColumnWidth(column, qMax(32, int(prop.Width)));
    setColumnHidden(column, false);
    horizontalHeader()->moveSection(horizontalHeader()->visualIndex(column), columnCount() - 1);
    for (const QModelIndex &row : selected)
        selectionModel()->select(model()->index(row.row(), column), QItemSelectionModel::Select);
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

QVariant ArchiveTableItem::data(int role) const {
    if (role == Qt::DecorationRole && column() != 0) {
        const auto *table = dynamic_cast<const FileTable *>(tableWidget());
        if (table && column() == table->columnForVisibleProperty(0))
            if (const auto *primary = table->item(row(), 0))
                return primary->data(role);
    }
    if (role == Qt::ForegroundRole) {
        const auto *table = dynamic_cast<const FileTable *>(tableWidget());
        if (!table || !table->sourcePanel || !table->sourcePanel->ShouldProcessNotifications() ||
            !table->textFolder || table->sourcePanel->GetFolder() != table->textFolder)
            return {};
        const QVariant index = QTableWidgetItem::data(Qt::UserRole + 6);
        if (!index.isValid())
            return {};
        // Provider exceptions cannot escape Qt's paint callback. The source
        // property body retains its exception policy; failed paint uses the palette.
        try {
            const QColor color = table->sourcePanel->OnCustomDraw(index.toUInt());
            return color.isValid() ? QVariant(QBrush(color)) : QVariant();
        } catch (...) {
            return {};
        }
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QTableWidgetItem::data(role);
    // PanelItems inserts callback text; PanelListNotify formats only requested
    // cells. Keep the owning panel, rather than the currently focused panel,
    // authoritative when Qt paints either side of a two-panel window.
    const auto *table = dynamic_cast<const FileTable *>(tableWidget());
    const int col = column();
    if (table && table->sourcePanel && role == Qt::DisplayRole &&
        !table->sourcePanel->ShouldDisplayItemText())
        return QString();
    if (table && table->sourcePanel && col != 1 && col != 5 && col != 6 &&
        (col != 0 || role == Qt::DisplayRole)) {
        // Native OnNotify suppresses callbacks while archive reopening can
        // invalidate the old proxy inside a nested event loop.
        if (!table->sourcePanel->ShouldProcessNotifications())
            return sourceTextDefined_
                       ? sourceText_
                       : (col == 0 ? QVariant(QString()) : QTableWidgetItem::data(role));
        const QVariant index = QTableWidgetItem::data(Qt::UserRole + 6);
        const CPropColumn *property = table->sourceColumn(col);
        if (index.isValid() && property && table->textFolder &&
            table->sourcePanel->GetFolder() == table->textFolder) {
            if (!sourceTextDefined_) {
                sourceText_ = table->sourcePanel->GetItemText(index.toUInt(), *property);
                if (col == 0)
                    sourceText_ = (QTableWidgetItem::data(Qt::UserRole + 5).toString() +
                                   sourceText_.toString())
                                      .normalized(QString::NormalizationForm_C);
                sourceTextDefined_ = true;
            }
            return sourceText_;
        }
    }
    return QTableWidgetItem::data(role);
}

void ArchiveTableItem::setData(int role, const QVariant &value) {
    if (role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::UserRole + 6)
        sourceTextDefined_ = false;
    QTableWidgetItem::setData(role, value);
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
            return table->sourcePanel->CompareItemIndices(
                       first, second, table->propertyId(columnIndex),
                       table->sourceColumnIndices.isEmpty()
                           ? table->isRawProperty(columnIndex)
                           : table->sourcePanel->_isRawSortProp != 0) < 0;
        }
    }
    return QTableWidgetItem::operator<(other);
}
