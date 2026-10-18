#include "qttranslationcatalog.h"
#include <QCoreApplication>
#include <QGlobalStatic>
#include <QTranslator>

namespace {
// Source LangString is a module-wide service, including inside Explorer.
// The shell DLL can unload before its host QCoreApplication. Module ownership
// destroys catalogs before that DLL's embedded resources disappear, and also
// supports Explorer hosts that have no QCoreApplication at all.
Q_GLOBAL_STATIC(QTranslator, applicationTranslator)
Q_GLOBAL_STATIC(QTranslator, sourcePluralTranslator)
} // namespace

QTranslator *QtSourcePluralTranslator() { return sourcePluralTranslator(); }

QTranslator *QtApplicationTranslator() { return applicationTranslator(); }

QString QtTranslatedApplicationText(const char *context, const char *source) {
    if (QCoreApplication::instance())
        return QCoreApplication::translate(context, source);
    const QString translated = QtApplicationTranslator()->translate(context, source);
    return translated.isNull() ? QString::fromUtf8(source) : translated;
}
