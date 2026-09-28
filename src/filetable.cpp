#include "filetable.h"
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
    return CompareItems(*this, other) < 0;
}
