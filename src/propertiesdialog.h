#pragma once

#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include <QString>
#include <QVector>

struct PropertyItem {
    QString name;
    QString path;
    QString modified;
    bool directory = false;
    qint64 size = 0;
    qint64 packedSize = 0;
    int nestedFiles = 0;
    int nestedFolders = 0;
};

// Qt snapshot adapter; property enumeration/totals live in upstream CPanel.
class PropertiesDialog : public CListViewDialog {
  public:
    explicit PropertiesDialog(const QVector<PropertyItem> &items, bool archive,
                              const QString &location, QWidget *parent = nullptr);
};
