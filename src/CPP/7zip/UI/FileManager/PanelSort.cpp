// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSort.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// QStringView replaces the upstream wchar_t pointers for portable Qt strings.

#include "PanelSort.h"

#include <QTableWidgetItem>

int CompareFileNames_ForFolderList(QStringView s1, QStringView s2) {
    qsizetype i1 = 0;
    qsizetype i2 = 0;
    const auto at = [](QStringView text, qsizetype index) {
        return index < text.size() ? text[index] : QChar();
    };
    for (;;) {
        QChar c1 = at(s1, i1);
        QChar c2 = at(s2, i2);
        if ((c1 >= QLatin1Char('0') && c1 <= QLatin1Char('9')) &&
            (c2 >= QLatin1Char('0') && c2 <= QLatin1Char('9'))) {
            for (; at(s1, i1) == QLatin1Char('0'); ++i1)
                ;
            for (; at(s2, i2) == QLatin1Char('0'); ++i2)
                ;
            qsizetype len1 = 0;
            qsizetype len2 = 0;
            for (; at(s1, i1 + len1) >= QLatin1Char('0') && at(s1, i1 + len1) <= QLatin1Char('9');
                 ++len1)
                ;
            for (; at(s2, i2 + len2) >= QLatin1Char('0') && at(s2, i2 + len2) <= QLatin1Char('9');
                 ++len2)
                ;
            if (len1 < len2)
                return -1;
            if (len1 > len2)
                return 1;
            for (; len1 > 0; ++i1, ++i2, --len1) {
                if (at(s1, i1) == at(s2, i2))
                    continue;
                return at(s1, i1) < at(s2, i2) ? -1 : 1;
            }
            c1 = at(s1, i1);
            c2 = at(s2, i2);
        }
        ++i1;
        ++i2;
        if (c1 != c2) {
            // Probably we need to change the order for special characters like in Explorer.
            const QChar u1 = c1.toUpper();
            const QChar u2 = c2.toUpper();
            if (u1 < u2)
                return -1;
            if (u1 > u2)
                return 1;
        }
        if (c1.isNull())
            return 0;
    }
}

int CompareItems(const QTableWidgetItem &left, const QTableWidgetItem &right) {
    const bool isDir1 = left.data(Qt::UserRole + 1).toBool();
    const bool isDir2 = right.data(Qt::UserRole + 1).toBool();
    if (isDir1 != isDir2)
        return isDir1 ? -1 : 1;

    if (left.data(Qt::UserRole + 2).isValid() && right.data(Qt::UserRole + 2).isValid()) {
        const qint64 first = left.data(Qt::UserRole + 2).toLongLong();
        const qint64 second = right.data(Qt::UserRole + 2).toLongLong();
        if (first != second)
            return first < second ? -1 : 1;
    }
    int res = CompareFileNames_ForFolderList(QStringView(left.text()), QStringView(right.text()));
    if (res == 0)
        res = CompareFileNames_ForFolderList(QStringView(left.data(Qt::UserRole + 3).toString()),
                                             QStringView(right.data(Qt::UserRole + 3).toString()));
    return res;
}
