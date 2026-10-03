#pragma once
#include "Common/MyTypes.h"
#include "qtlanguageservices.h"
#include <QString>
class QObject;
class QWidget;
QString QtLangText(UInt32 id, const QString &englishFallback = {});
void QtBindLocalText(QObject *object, const QString &english, const QString &russian,
                     bool accessibleOnly = false);
void QtBindLocalToolTip(QObject *object, const QString &english, const QString &russian);
// Presentation bindings retain original English captions and change text only.
void QtBindLangText(QObject *object, UInt32 id, const QString &englishFallback = {},
                    bool accessibleOnly = false);
// LangSetDlgItems keeps the original dialog resource caption when no translation exists.
void QtBindResourceText(QObject *object, UInt32 id, const QString &resourceCaption,
                        bool accessibleOnly = false);
void QtReloadLangTexts(QWidget &root);
QString QtWrapControlText(const QWidget &widget, const QString &text);
void QtNotifyLanguageChanged();
