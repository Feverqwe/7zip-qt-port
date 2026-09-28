#pragma once

#include <QString>

struct CVolSeqName {
    QString UnchangedPart;
    QString ChangedPart = QStringLiteral("000");

    void SetNumDigits(quint64 numVolumes);
    bool ParseName(const QString &name);
    QString GetNextName();
};
