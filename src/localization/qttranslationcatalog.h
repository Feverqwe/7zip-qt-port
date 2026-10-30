#pragma once
#include <QString>
class QTranslator;

QTranslator *QtApplicationTranslator();
QTranslator *QtSourcePluralTranslator();
// Also serves the native shell host, which has no QCoreApplication.
QString QtTranslatedApplicationText(const char *context, const char *source);
