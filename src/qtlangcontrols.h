#pragma once
#include "Common/MyTypes.h"
#include <QString>
class QObject;
class QWidget;
QString QtLangText(UInt32 id, const QString &englishFallback = {});
// Presentation bindings retain original English captions and change text only.
void QtBindLangText(QObject *object, UInt32 id, const QString &englishFallback = {},
                    bool accessibleOnly = false);
void QtReloadLangTexts(QWidget &root);
void QtNotifyLanguageChanged();
