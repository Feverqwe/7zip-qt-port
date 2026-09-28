#pragma once

#include <QStringView>

class QTableWidgetItem;

int CompareFileNames_ForFolderList(QStringView s1, QStringView s2);
QStringView GetExtensionPtr(QStringView name);
int CompareItems(const QTableWidgetItem &left, const QTableWidgetItem &right);
