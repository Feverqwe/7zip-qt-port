#include "qtdiagnostictext.h"
#include "Common/StringConvert.h"
#include "qttranslationcatalog.h"
#include <QStringList>

UString QtDiagnosticText(const char *source) {
    const QByteArray key = QString::fromWCharArray(GetUnicodeString(source)).toUtf8();
    const std::wstring text =
        QtTranslatedApplicationText("SevenZipDiagnostics", key.constData()).toStdWString();
    return UString(text.c_str());
}

UString QtDiagnosticText(const char *source, unsigned code) {
    const std::wstring text =
        QtTranslatedApplicationText("SevenZipDiagnostics", source).arg(code).toStdWString();
    return UString(text.c_str());
}

UString QtDiagnosticLines(const AString &source) {
    UString result;
    const auto lines = QString::fromLatin1(source.Ptr()).split('\n');
    for (qsizetype i = 0; i < lines.size(); ++i) {
        if (i)
            result.Add_LF();
        result += QtDiagnosticText(lines[i].toLatin1().constData());
    }
    return result;
}

UString QtArchiveDiagnosticText(const UString &source) {
    const QString text = QString::fromWCharArray(source);
    static const QString missingPrefix = QStringLiteral("Missing volume : ");
    if (text.startsWith(missingPrefix)) {
        const QString message = QtTranslatedApplicationText(
            "SevenZipDiagnostics", QT_TRANSLATE_NOOP("SevenZipDiagnostics", "Missing volume: %1"));
        return UString(message.arg(text.mid(missingPrefix.size())).toStdWString().c_str());
    }
    const char *const warnings[] = {
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "32-bit overflow in headers"),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics", "16-bit overflow for number of files in headers"),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Volume was not properly unmounted. Some data may be corrupt."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "Medium has reported failures. Some data may be lost."),
        QT_TRANSLATE_NOOP(
            "SevenZipDiagnostics",
            "The HFS wrapper lacks protection flags. Embedded volume data may be inconsistent."),
        QT_TRANSLATE_NOOP("SevenZipDiagnostics",
                          "The free-space count exceeds the volume size. Free space is unknown.")};
    for (const char *warning : warnings)
        if (text == QString::fromLatin1(warning))
            return QtDiagnosticText(warning);
    if (text == QString::fromLatin1(warnings[0]) + ' ' + QString::fromLatin1(warnings[1])) {
        UString result = QtDiagnosticText(warnings[0]);
        result.Add_Space();
        result += QtDiagnosticText(warnings[1]);
        return result;
    }
    return source;
}
