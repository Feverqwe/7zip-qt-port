#pragma once
#include "Common/MyString.h"
#include <QCoreApplication>
#include <QList>
#include <QString>
struct QtLanguageInfo {
    QString code;
    QString name;
    QString file;
};
QList<QtLanguageInfo> QtAvailableLanguages(const QString &directory = {});
UString QtEnglishLanguageString(UInt32 id);
// Null means the selected catalog has no translation; preserve the resource caption.
QString QtTranslatedResourceString(UInt32 id);
QString QtTranslatedResourceCount(UInt32 id, quint64 count);
// Returns the selected catalog code, or "-" for the English source fallback.
QString QtLoadTranslation(const QString &language, const QString &directory = {});
QString QtSelectedItemsText(int count);
QString QtSelectionStatusText(int selected, int total);
