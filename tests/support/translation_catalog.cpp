#include "support/translation_catalog.h"
#include "localization/qtlanguageservices.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QXmlStreamWriter>

bool WriteTranslationCatalog(const QString &directory,
                             const QList<QPair<UInt32, QString>> &messages, QString *error) {
    QFile source(QDir(directory).filePath("fixture.ts"));
    if (!source.open(QIODevice::WriteOnly)) {
        if (error)
            *error = source.fileName() + ": " + source.errorString();
        qWarning() << "Cannot write catalog fixture:" << source.fileName() << source.errorString();
        return false;
    }
    QXmlStreamWriter xml(&source);
    xml.writeStartDocument();
    xml.writeStartElement("TS");
    xml.writeAttribute("version", "2.1");
    xml.writeAttribute("language", "de_DE");
    xml.writeStartElement("context");
    xml.writeTextElement("name", "SevenZip");
    for (const auto &message : messages) {
        xml.writeStartElement("message");
        xml.writeTextElement("source",
                             QString::fromWCharArray(QtEnglishLanguageString(message.first)));
        xml.writeTextElement("comment", QString::number(message.first));
        xml.writeTextElement("translation", message.second);
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndElement();
    xml.writeEndDocument();
    source.close();
    QProcess release;
    // Windows lrelease converts argv through its ANSI code page. QProcess's
    // native Unicode working directory still lets this fixture exercise loading
    // a QM from a Unicode path without losing characters in the tool's argv.
    release.setWorkingDirectory(directory);
    release.start(TEST_LRELEASE_PATH, {"fixture.ts", "-qm", "sevenzip_de.qm"});
    if (!release.waitForFinished(10000) || release.exitStatus() != QProcess::NormalExit ||
        release.exitCode() != 0) {
        if (error)
            *error =
                QString("lrelease %1, exit %2: %3\n%4\n%5")
                    .arg(TEST_LRELEASE_PATH)
                    .arg(release.exitCode())
                    .arg(release.errorString(), QString::fromUtf8(release.readAllStandardError()),
                         QString::fromUtf8(release.readAllStandardOutput()));
        qWarning() << "lrelease fixture failed:" << TEST_LRELEASE_PATH << release.errorString()
                   << release.exitCode() << release.readAllStandardError()
                   << release.readAllStandardOutput();
        return false;
    }
    return true;
}
