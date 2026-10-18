#include "qtlanguageservices.h"
#include "qttranslationcatalog.h"
#include "qtupstreamtranslations.h"
#include <QDir>
#include <QLocale>
#include <QTranslator>
#include <algorithm>
#include <limits>

static void InitTranslationResources() { Q_INIT_RESOURCE(sevenzip_filesystem_translations); }

QString QtSelectedItemsText(int count) {
    return QCoreApplication::translate("PanelSelection", "Selected objects: %Ln", nullptr, count);
}

QString QtSelectionStatusText(int selected, int total) {
    return QCoreApplication::translate("PanelSelection", "Selected: %1 / %2")
        .arg(QLocale().toString(selected), QLocale().toString(total));
}

namespace {
const QtUpstreamTranslation *Resource(UInt32 id) {
    for (const auto &entry : kQtUpstreamTranslations)
        if (entry.id == id)
            return &entry;
    return nullptr;
}
QString NormalizedCode(QString code) { return code.replace('_', '-').toLower(); }
} // namespace

QList<QtLanguageInfo> QtAvailableLanguages(const QString &directory) {
    InitTranslationResources();
    QList<QtLanguageInfo> languages{{"-", "English : English", {}}};
    QStringList paths{QString(":/translations")};
    if (!directory.isEmpty())
        paths.prepend(directory);
    else if (QCoreApplication::instance())
        paths.prepend(QCoreApplication::applicationDirPath() + "/translations");
    for (const QString &path : paths) {
        const QDir dir(path);
        for (const QString &file : dir.entryList({"sevenzip_*.qm"}, QDir::Files, QDir::Name)) {
            const QString code = file.mid(9, file.size() - 12);
            // English contains only the source language's grammatical forms.
            // The existing English choice already selects it as fallback.
            if (NormalizedCode(code) == "en")
                continue;
            if (std::any_of(languages.cbegin(), languages.cend(), [&](const auto &item) {
                    return NormalizedCode(item.code) == NormalizedCode(code);
                }))
                continue;
            QTranslator catalog;
            if (!catalog.load(dir.filePath(file)))
                continue;
            const QLocale locale(catalog.language().isEmpty() ? code : catalog.language());
            QString native = locale.nativeLanguageName();
            if (!native.isEmpty())
                native[0] = native[0].toUpper();
            QString name = QLocale::languageToString(locale.language()) + " : " + native;
            if (NormalizedCode(code).contains('-'))
                name += " (" + locale.nativeTerritoryName() + ')';
            languages.append({code, name, dir.filePath(file)});
        }
    }
    std::sort(languages.begin() + 1, languages.end(), [](const auto &a, const auto &b) {
        return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
    });
    return languages;
}

UString QtEnglishLanguageString(UInt32 id) {
    const auto *entry = Resource(id);
    return entry ? UString(QString::fromUtf8(entry->text.source).toStdWString().c_str())
                 : UString();
}

QString QtTranslatedResourceString(UInt32 id) {
    const auto *entry = Resource(id);
    auto *translator = QtApplicationTranslator();
    return entry && translator
               ? translator->translate("SevenZip", entry->text.source, entry->text.comment)
               : QString();
}

QString QtTranslatedResourceCount(UInt32 id, quint64 count) {
    const auto *entry = Resource(id);
    if (!entry)
        return {};
    // QTranslator's numerus argument is signed int. Keep larger source
    // quantities exact with a complete, non-inflected quantity label.
    if (count > quint64(std::numeric_limits<int>::max())) {
        const char *source =
            id == 3504   ? QT_TRANSLATE_NOOP("LargeQuantity", "%1 B")
            : id == 6105 ? QT_TRANSLATE_NOOP("LargeQuantity", "Delete this number of items: %1?")
            : id == 7308
                ? QT_TRANSLATE_NOOP("LargeQuantity", "Specified volume size: %1 B.\nAre you sure "
                                                     "you want to split archive into such volumes?")
                : QT_TRANSLATE_NOOP("LargeQuantity", "Split file into this number of volumes: %1?");
        return QtTranslatedApplicationText("LargeQuantity", source).arg(QLocale().toString(count));
    }
    QString text = QtApplicationTranslator()->translate("SevenZip", entry->text.source,
                                                        entry->text.comment, int(count));
    if (text.isNull())
        text = QtSourcePluralTranslator()->translate("SevenZip", entry->text.source,
                                                     entry->text.comment, int(count));
    if (text.isNull())
        text = QString::fromUtf8(entry->text.source);
    // Direct translator lookup also serves Explorer without QCoreApplication;
    // Qt's application-level substitution is therefore performed here.
    return text.replace("%Ln", QLocale().toString(count)).replace("%n", QString::number(count));
}

QString QtLoadTranslation(const QString &language, const QString &directory) {
    const auto languages = QtAvailableLanguages(directory);
    auto *sourcePlurals = QtSourcePluralTranslator();
    if (QCoreApplication::instance())
        QCoreApplication::removeTranslator(sourcePlurals);
    if (sourcePlurals->load(":/translations/sevenzip_en.qm") && QCoreApplication::instance())
        QCoreApplication::installTranslator(sourcePlurals);
    QStringList candidates;
    if (language.isEmpty())
        candidates = QLocale::system().uiLanguages();
    else
        candidates.append(language);
    const QtLanguageInfo *selected = &languages.first();
    for (const QString &candidate : candidates) {
        QString code = NormalizedCode(candidate);
        while (!code.isEmpty()) {
            const auto found =
                std::find_if(languages.cbegin() + 1, languages.cend(),
                             [&](const auto &item) { return NormalizedCode(item.code) == code; });
            if (found != languages.cend()) {
                selected = &*found;
                break;
            }
            const int separator = code.lastIndexOf('-');
            code = separator < 0 ? QString() : code.left(separator);
        }
        if (selected != &languages.first())
            break;
    }
    if (auto *translator = QtApplicationTranslator()) {
        if (QCoreApplication::instance())
            QCoreApplication::removeTranslator(translator);
        // A resource-only miss clears the previous catalog without searching the cwd.
        const QString file = selected->file.isEmpty()
                                 ? QString(":/translations/.source-fallback.qm")
                                 : selected->file;
        if (translator->load(file) && QCoreApplication::instance())
            QCoreApplication::installTranslator(translator);
    }
    return selected->code;
}
