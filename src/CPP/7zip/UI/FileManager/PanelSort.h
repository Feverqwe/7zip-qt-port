#pragma once

#include <QStringView>

class QTableWidgetItem;

int CompareFileNames_ForFolderList(QStringView s1, QStringView s2);
int CompareItems(const QTableWidgetItem &left, const QTableWidgetItem &right);
