#include "localization/LangUtils.h"
#include "localization/qtlanguageservices.h"
#include "settings/RegistryUtils.h"
#include "ui/diagnostics/qtdiagnostictext.h"
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

class NativeLanguageHostTests final : public QObject {
    Q_OBJECT
  private slots:
    void catalogSwitchAndFallbackWithoutApplication() {
        QVERIFY(!QCoreApplication::instance());
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QByteArray oldSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const bool hadSettings = qEnvironmentVariableIsSet("SEVENZIP_SETTINGS_PATH");
        const QString settings = directory.filePath("language.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings.toUtf8());
        const auto restore = qScopeGuard([&] {
            if (hadSettings)
                qputenv("SEVENZIP_SETTINGS_PATH", oldSettings);
            else
                qunsetenv("SEVENZIP_SETTINGS_PATH");
        });
        SaveRegLang(UString("ru"), settings);
        LoadLangOneTime();
        QCOMPARE(QString::fromWCharArray(LangString(402)), QString::fromUtf8("Отмена"));
        QCOMPARE(QString::fromWCharArray(QtDiagnosticText("Incorrect link type")),
                 QString::fromUtf8("Неверный тип ссылки"));
        for (const char *language : {"-", "missing"}) {
            SaveRegLang(UString(language), settings);
            ReloadLang(settings);
            QCOMPARE(QString::fromWCharArray(LangString(402)), QString("Cancel"));
            QCOMPARE(QString::fromWCharArray(QtDiagnosticText("Incorrect link type")),
                     QString("Incorrect link type"));
        }
        SaveRegLang(UString("ru"), settings);
        ReloadLang(settings);
        QCOMPARE(QString::fromWCharArray(LangString(402)), QString::fromUtf8("Отмена"));
        QVERIFY(!QCoreApplication::instance());
    }
};

QTEST_APPLESS_MAIN(NativeLanguageHostTests)
#include "native_language_host_tests.moc"
