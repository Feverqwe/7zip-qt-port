#pragma once

#include <QStringView>

class QTableWidgetItem;

int CompareFileNames_ForFolderList(const char16_t *s1, const char16_t *s2);
int CompareFileNames_ForFolderList(QStringView s1, QStringView s2);
QStringView GetExtensionPtr(QStringView name);
int CompareItems(const QTableWidgetItem &left, const QTableWidgetItem &right);
