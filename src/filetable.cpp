#include "filetable.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"

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
    return CompareItems(*this, other) < 0;
}
