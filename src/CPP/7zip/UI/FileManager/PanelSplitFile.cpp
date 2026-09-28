// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSplitFile.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// QString replaces UString; file I/O remains in the Qt platform adapter.

#include "PanelSplitFile.h"

void CVolSeqName::SetNumDigits(quint64 numVolumes) {
    ChangedPart = QStringLiteral("000");
    while (numVolumes > 999) {
        numVolumes /= 10;
        ChangedPart += QLatin1Char('0');
    }
}

bool CVolSeqName::ParseName(const QString &name) {
    if (name.size() < 2)
        return false;
    if (name.back() != QLatin1Char('1') || name[name.size() - 2] != QLatin1Char('0'))
        return false;

    qsizetype pos = name.size() - 2;
    for (; pos > 0 && name[pos - 1] == QLatin1Char('0'); --pos)
        ;
    UnchangedPart = name.left(pos);
    ChangedPart = name.mid(pos);
    return true;
}

QString CVolSeqName::GetNextName() {
    for (int i = ChangedPart.size() - 1; i >= 0; --i) {
        const QChar c = ChangedPart[i];
        if (c != QLatin1Char('9')) {
            ChangedPart[i] = QChar(c.unicode() + 1);
            break;
        }
        ChangedPart[i] = QLatin1Char('0');
        if (i == 0)
            ChangedPart.prepend(QLatin1Char('1'));
    }
    return UnchangedPart + ChangedPart;
}
