#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

struct PropertyItem {
    QString name;
    QString path;
    QString modified;
    bool directory = false;
    qint64 size = 0;
    qint64 packedSize = 0;
};

class PropertiesDialog : public QDialog {
  public:
    explicit PropertiesDialog(const QVector<PropertyItem> &items, bool archive,
                              const QString &location, QWidget *parent = nullptr);
};
