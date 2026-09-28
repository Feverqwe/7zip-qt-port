// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelSort.cpp at 0766b73.
// Copyright (C) 1999-2026 Igor Pavlov. GNU LGPL 2.1 or later.
// Pointer algorithm copied from upstream; char16_t replaces Windows wchar_t.
// QStringView conversion and Qt properties adapt the panel at its boundary.

#include "PanelSort.h"

#include <QTableWidgetItem>
#include <string>

namespace {
// Preserve Windows 7zFM UTF-16 code units on platforms with 32-bit wchar_t.
char16_t MyCharUpper(char16_t c) { return QChar(c).toUpper().unicode(); }
} // namespace

int CompareFileNames_ForFolderList(const char16_t *s1, const char16_t *s2) {
    for (;;) {
        char16_t c1 = *s1;
        char16_t c2 = *s2;
        if ((c1 >= '0' && c1 <= '9') && (c2 >= '0' && c2 <= '9')) {
            for (; *s1 == '0'; s1++)
                ;
            for (; *s2 == '0'; s2++)
                ;
            size_t len1 = 0;
            size_t len2 = 0;
            for (; (s1[len1] >= '0' && s1[len1] <= '9'); len1++)
                ;
            for (; (s2[len2] >= '0' && s2[len2] <= '9'); len2++)
                ;
            if (len1 < len2)
                return -1;
            if (len1 > len2)
                return 1;
            for (; len1 > 0; s1++, s2++, len1--) {
                if (*s1 == *s2)
                    continue;
                return (*s1 < *s2) ? -1 : 1;
            }
            c1 = *s1;
            c2 = *s2;
        }
        s1++;
        s2++;
        if (c1 != c2) {
            // Probably we need to change the order for special characters like in Explorer.
            char16_t u1 = MyCharUpper(c1);
            char16_t u2 = MyCharUpper(c2);
            if (u1 < u2)
                return -1;
            if (u1 > u2)
                return 1;
        }
        if (c1 == 0)
            return 0;
    }
}

int CompareFileNames_ForFolderList(QStringView s1, QStringView s2) {
    // Views need not be null-terminated. Own the strings before calling upstream.
    const std::u16string name1 = s1.toString().toStdU16String();
    const std::u16string name2 = s2.toString().toStdU16String();
    return CompareFileNames_ForFolderList(name1.c_str(), name2.c_str());
}

QStringView GetExtensionPtr(QStringView name) {
    const qsizetype dotPos = name.lastIndexOf(QLatin1Char('.'));
    return name.mid(dotPos < 0 ? name.size() : dotPos);
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
    int res = 0;
    // Name is the base name, even when the Qt presentation shows a relative path.
    if (left.column() != 0)
        res = CompareFileNames_ForFolderList(QStringView(left.text()), QStringView(right.text()));
    if (res == 0)
        res = CompareFileNames_ForFolderList(QStringView(left.data(Qt::UserRole + 3).toString()),
                                             QStringView(right.data(Qt::UserRole + 3).toString()));
    if (res == 0)
        res = CompareFileNames_ForFolderList(QStringView(left.data(Qt::UserRole + 5).toString()),
                                             QStringView(right.data(Qt::UserRole + 5).toString()));
    if (res == 0) {
        const int first = left.data(Qt::UserRole + 6).toInt();
        const int second = right.data(Qt::UserRole + 6).toInt();
        res = first < second ? -1 : first > second ? 1 : 0;
    }
    return res;
}
