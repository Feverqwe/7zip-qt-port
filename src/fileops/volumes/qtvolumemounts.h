#pragma once

#include <QList>
#include <QString>
#include <functional>

struct QtVolumeMount {
    QString path;
    QString name;
    QString fileSystem;
    qint64 totalBytes = -1;
    qint64 availableBytes = -1;
    bool network = false;
};

using QtVolumeMountProvider = std::function<QList<QtVolumeMount>()>;
QList<QtVolumeMount> QtSystemVolumeMounts();
