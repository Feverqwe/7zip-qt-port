#include "7zip/UI/Explorer/resource.h"
#include "CPP/7zip/UI/FileManager/LangPage.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include <QComboBox>
#include <QFile>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace {
bool writeFile(const QString &path, const QByteArray &data) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
FString nativePath(const QString &path) { return us2fs(UString(path.toStdWString().c_str())); }
QByteArray language(const QByteArray &name = "7-Zip") {
    return ";!@Lang2@!UTF-8!\n; fixture comment\n0\n" + name +
           "\nFixture\nNative name\n2322\ntranslated\\nsecond\\tfield\\\\end\n";
}
} // namespace

class LanguageTests : public QObject {
    Q_OBJECT
  private slots:
    void sourceParserAndFailedOpenClear() {
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("язык с пробелом.txt"));
        QVERIFY(writeFile(path, language()));
        CLang lang;
        QVERIFY(LangOpen(lang, nativePath(path)));
        QCOMPARE(lang._ids.Size(), 4u);
        QCOMPARE(QString::fromWCharArray(lang.Get(2322)),
                 QString("translated\nsecond\tfield\\end"));
        QVERIFY(lang.Get(2321) == nullptr);
        QCOMPARE(lang.Comments.Size(), 1u);
        QVERIFY(!LangOpen(lang, nativePath(directory.filePath("missing.txt"))));
        QVERIFY(lang.IsEmpty());
        QVERIFY(writeFile(path, language("other app")));
        QVERIFY(!LangOpen(lang, nativePath(path)));
        QVERIFY(lang.IsEmpty());
        QVERIFY(writeFile(path, ";!@Lang2@!UTF-8!\n0\n7-Zip\n10\nlast\n2\nbackwards\n"));
        QVERIFY(!LangOpen(lang, nativePath(path)));
        QVERIFY(lang.IsEmpty());
        QVERIFY(writeFile(path, QByteArray("bad signature")));
        QVERIFY(!LangOpen(lang, nativePath(path)));
        QVERIFY(writeFile(path, QByteArray((1 << 20) + 1, 'x')));
        QVERIFY(!LangOpen(lang, nativePath(path)));
    }
    void sourceReloadAndFallback() {
        QTemporaryDir directory;
        const QString settings = directory.filePath("options.ini");
        QVERIFY(writeFile(directory.filePath("fixture.txt"), language()));
        SaveRegLang(UString("fixture"), settings);
        ReloadLang(settings, directory.path());
        QCOMPARE(QString::fromWCharArray(LangString(IDS_CONTEXT_OPEN)),
                 QString("translated\nsecond\tfield\\end"));
        UString only;
        LangString_OnlyFromLangFile(IDS_CONTEXT_EXTRACT, only);
        QVERIFY(only.IsEmpty());
        QCOMPARE(QString::fromWCharArray(LangString(IDS_CONTEXT_EXTRACT)),
                 QString("Extract files..."));
        SaveRegLang(UString("missing"), settings);
        ReloadLang(settings, directory.path());
        QVERIFY(g_Lang.IsEmpty());
        QCOMPARE(QString::fromWCharArray(LangString(IDS_CONTEXT_OPEN)), QString("Open archive"));
        SaveRegLang(UString("../fixture"), settings);
        ReloadLang(settings, directory.path());
        QVERIFY(g_Lang.IsEmpty());
        SaveRegLang(UString("-"), settings);
        ReloadLang(settings, directory.path());
        QVERIFY(g_Lang.IsEmpty());
    }
    void bundledLanguagePageApplyCancelAndGeometry() {
        QTemporaryDir directory;
        const QString settings = directory.filePath("options.ini");
        SaveRegLang(UString("-"), settings);
        ReloadLang(settings);
        {
            CLangPage cancelled(settings);
            auto *choice = cancelled.findChild<QComboBox *>("languageChoice");
            QCOMPARE(choice->count(), 2);
            QCOMPARE(choice->currentData().toUInt(), 0u);
            QVERIFY(choice->itemText(0).contains("English"));
            QVERIFY(choice->itemText(1).contains(QStringLiteral("Русский")));
            choice->setCurrentIndex(1);
            QVERIFY(cancelled.IsModified());
            QVERIFY(cancelled.findChild<QPlainTextEdit *>("languageInfo")
                        ->toPlainText()
                        .startsWith("ru :"));
        }
        UString saved;
        ReadRegLang(saved, settings);
        QCOMPARE(QString::fromWCharArray(saved), QString("-"));
        CLangPage page(settings);
        page.findChild<QComboBox *>("languageChoice")->setCurrentIndex(1);
        QCOMPARE(page.OnApply(), 0L);
        QVERIFY(!page.IsModified());
        QVERIFY(page.LangWasChanged);
        ReadRegLang(saved, settings);
        QCOMPARE(QString::fromWCharArray(saved), QString("ru"));
        QCOMPARE(QString::fromWCharArray(LangString(IDS_CONTEXT_OPEN)),
                 QStringLiteral("Открыть архив"));
        CLangPage restarted(settings);
        QCOMPARE(restarted.findChild<QComboBox *>("languageChoice")->currentData().toUInt(), 1u);
        page.show();
        for (QSize size : {QSize(600, 320), QSize(480, 280)}) {
            page.resize(size);
            QApplication::processEvents();
            auto *choice = page.findChild<QWidget *>("languageChoice");
            auto *info = page.findChild<QWidget *>("languageInfo");
            QVERIFY(UiInspection::fullyVisible(*choice, page));
            QVERIFY(UiInspection::fullyVisible(*info, page));
            QVERIFY(UiInspection::separate(*choice, *info, page));
            QVERIFY(UiInspection::writeReport(page, QString("language-page-%1").arg(size.width())));
        }
        SaveRegLang(UString("-"), settings);
        ReloadLang(settings);
    }
};
QTEST_MAIN(LanguageTests)
#include "language_tests.moc"
