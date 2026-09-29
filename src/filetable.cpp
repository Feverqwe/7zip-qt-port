#include "filetable.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"

#include <QHeaderView>
#include <QMimeData>
#include <QUrl>

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

bool ArchiveTableItem::operator<(const QTableWidgetItem &other) const {
    // Qt reverses the whole comparison for descending order. Compensate only
    // the folder grouping: upstream reverses property order after this check.
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
