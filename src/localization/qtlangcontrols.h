#pragma once
#include "Common/MyTypes.h"
#include "localization/qtlanguageservices.h"
#include <QString>
#include <QStringList>
class QObject;
class QWidget;
QString QtLangText(UInt32 id, const QString &englishFallback = {});
void QtBindLocalText(QObject *object, const char *context, const char *sourceText,
                     bool accessibleOnly = false, const QStringList &arguments = {});
void QtBindLocalToolTip(QObject *object, const char *context = "", const char *sourceText = "");
void QtBindMemoryLimitUnitText(QObject *object, bool ramDefined, quint64 ramGB);
// Presentation bindings retain original English captions and change text only.
void QtBindLangText(QObject *object, UInt32 id, const QString &englishFallback = {},
                    bool accessibleOnly = false);
// LangSetDlgItems keeps the original dialog resource caption when no translation exists.
void QtBindResourceText(QObject *object, UInt32 id, const QString &resourceCaption,
                        bool accessibleOnly = false);
void QtBindStandardResourceText(QObject *object, UInt32 controlID, const QString &resourceCaption);
void QtReloadLangTexts(QWidget &root);
QString QtWrapControlText(const QWidget &widget, const QString &text);
void QtNotifyLanguageChanged();
