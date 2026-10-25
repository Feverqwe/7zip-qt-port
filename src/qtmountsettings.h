#pragma once
#include <QString>
class QWidget;
struct QtMountOptions {
    bool Accepted = false;
    quint64 Quota = 0;
    QString Password;
};
QtMountOptions QtAskMountOptions(QWidget *owner, const QString &source, const QString &password);
