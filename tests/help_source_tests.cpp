#include "CPP/7zip/UI/Common/LoadCodecs.h"
#include "CPP/7zip/UI/FileManager/AboutDialog.h"
#include "CPP/7zip/UI/FileManager/HelpUtils.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "qthelpservices.h"
#include "qtlangcontrols.h"
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextTable>
#include <QTimer>
#include <QtTest>

class HelpSourceTests : public QObject {
    Q_OBJECT
    QUrl openedUrl;
  public slots:
    void openUrl(const QUrl &url) { openedUrl = url; }
  private slots:
    void aboutKeepsNaturalSourceLogo() {
        CAboutDialog dialog;
        const QPixmap source(":/upstream/7zip/About/7zipLogo.ico");
        QCOMPARE(source.size(), QSize(110, 63));
        QCOMPARE(dialog.findChild<QLabel *>("aboutLogo")->pixmap().size(), source.size());
    }
    void aboutInitialFrameScalesWithSourceFont() {
        const QFont originalFont = QApplication::font();
        const auto restoreFont = qScopeGuard([&] { QApplication::setFont(originalFont); });
        QSize smallFrame;
        for (int points : {8, 12}) {
            QFont font = originalFont;
            font.setPointSize(points);
            QApplication::setFont(font);
            CAboutDialog dialog;
            dialog.show();
            QApplication::processEvents();
            if (points == 8)
                smallFrame = dialog.size();
            else {
                QVERIFY(dialog.width() > smallFrame.width());
                QVERIFY(dialog.height() > smallFrame.height());
            }
            for (auto *label : dialog.findChildren<QLabel *>()) {
                QVERIFY(UiInspection::fullyVisible(*label, dialog));
                QVERIFY2(UiInspection::labelFits(*label), qPrintable(label->objectName()));
            }
            QVERIFY(UiInspection::writeReport(dialog, QString("about-source-font-%1").arg(points)));
            dialog.reject();
        }
    }
    void aboutKeepsSourceFooterOrder() {
        CAboutDialog dialog;
        dialog.show();
        QApplication::processEvents();
        auto *homepage = dialog.findChild<QPushButton *>("aboutHomepage");
        auto *ok = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
        QVERIFY(UiInspection::bounds(*homepage, dialog).right() <
                UiInspection::bounds(*ok, dialog).left());
        QCOMPARE(homepage->width(), ok->width());
    }
    void aboutInitialReturnClosesSourceDialog() {
        CAboutDialog dialog;
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
        auto *ok = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
        QVERIFY(ok->hasFocus());
        QSignalSpy accepted(&dialog, &QDialog::accepted);
        QTest::keyClick(&dialog, Qt::Key_Return);
        QCOMPARE(accepted.count(), 1);
        QVERIFY(!dialog.isVisible());
        QVERIFY(!dialog.findChild<QDialog *>("helpWindow"));
    }

    void aboutReportsSourceCodecErrors() {
        CCodecs codecs;
        auto &error = codecs.Errors.AddNew();
        error.Path = FTEXT("unusable plugin");
        error.Message = "the module is not compatible with program";
        UString expected;
        codecs.GetCodecsErrorMessage(expected);
        QVERIFY(!expected.IsEmpty());
        for (const QSize size : {QSize(600, 300), QSize(400, 250)}) {
            QString shown;
            bool geometry = false;
            QTimer::singleShot(0, this, [&] {
                auto *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                if (!message)
                    return;
                message->resize(size);
                QApplication::processEvents();
                shown = message->text();
                auto *label = message->findChild<QLabel *>("qt_msgbox_label");
                geometry = label && UiInspection::fullyVisible(*label, *message) &&
                           UiInspection::labelFits(*label) &&
                           UiInspection::writeReport(*message, "about-codec-error-" +
                                                                   QString::number(size.width()));
                message->accept();
            });
            CAboutDialog dialog(nullptr, &codecs);
            QCOMPARE(shown, QString::fromWCharArray(expected.Ptr()));
            QVERIFY(geometry);
        }
    }
    void aboutSourceVersionLicenseHomepageAndGeometry() {
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("language.ini");
        CAboutDialog dialog;
        dialog.show();
        auto *version = dialog.findChild<QLabel *>("aboutText101");
        QVERIFY(version->text().startsWith("7-Zip 26.03"));
        const QString sourceVersion = version->text();
        QCOMPARE(dialog.findChild<QLabel *>("aboutText102")->text(), QString("2026-09-03"));
        QVERIFY(dialog.findChild<QLabel *>("aboutText2901")->alignment() & Qt::AlignTop);
        QVERIFY(!dialog.findChild<QLabel *>("aboutLogo")->pixmap().isNull());
        QVERIFY(dialog.findChild<QLabel *>("aboutApplicationVersion")
                    ->text()
                    .contains("Sevenzip 0.1.0"));
        QDesktopServices::setUrlHandler("https", this, "openUrl");
        QVERIFY(!dialog.OnButtonClicked(999, nullptr));
        QVERIFY(dialog.OnButtonClicked(IDB_ABOUT_HOMEPAGE, nullptr));
        QDesktopServices::unsetUrlHandler("https");
        QCOMPARE(openedUrl, QUrl("https://www.7-zip.org/"));
        for (const char *language : {"-", "ru", "-"}) {
            SaveRegLang(UString(language), settings);
            ReloadLang(settings);
            QtNotifyLanguageChanged();
            QCOMPARE(dialog.windowTitle(), QtLangText(IDD_ABOUT, "About 7-Zip"));
            QCOMPARE(version->text(), sourceVersion);
            for (QSize size : {QSize(360, 330), QSize(300, 310)}) {
                dialog.resize(size);
                QTRY_COMPARE(dialog.size(), size.expandedTo(dialog.minimumSizeHint()));
                for (auto *label : dialog.findChildren<QLabel *>()) {
                    QVERIFY(UiInspection::fullyVisible(*label, dialog));
                    QVERIFY2(UiInspection::labelFits(*label), qPrintable(label->objectName()));
                }
                for (auto *button : dialog.findChildren<QPushButton *>())
                    QVERIFY(UiInspection::fullyVisible(*button, dialog));
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("about-%1-%2").arg(language).arg(size.width())));
            }
        }
        dialog.findChild<QPushButton *>("aboutLicense")->click();
        auto *help = dialog.findChild<QDialog *>("helpWindow");
        QVERIFY(help && help->isVisible());
        auto *browser = help->findChild<QTextBrowser *>("helpBrowser");
        QCOMPARE(browser->source().path(), QString("/upstream/7zip/Help/general/license.htm"));
        QVERIFY(browser->toPlainText().contains("GNU LGPL"));
        help->close();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        dialog.OnHelp();
        help = dialog.findChild<QDialog *>("helpWindow");
        QVERIFY(help && help->isVisible());
        QCOMPARE(help->findChild<QTextBrowser *>("helpBrowser")->source().path(),
                 QString("/upstream/7zip/Help/start.htm"));
        help->close();
    }
    void sourceTopicAndOfflineNavigation() {
        QWidget parent;
        ShowHelpWindow("FM/index.htm", &parent);
        auto *window = parent.findChild<QDialog *>("helpWindow");
        QVERIFY(window);
        auto *browser = window->findChild<QTextBrowser *>("helpBrowser");
        QVERIFY(browser);
        QCOMPARE(browser->source().path(), QString("/upstream/7zip/Help/fm/index.htm"));
        QVERIFY(browser->toPlainText().contains("File Manager"));
        browser->anchorClicked(QUrl("options.htm#language"));
        QCOMPARE(browser->source().path(), QString("/upstream/7zip/Help/fm/options.htm"));
        QCOMPARE(browser->source().fragment(), QString("language"));
        QVERIFY(browser->toPlainText().contains("Language"));
        browser->backward();
        QCOMPARE(browser->source().path(), QString("/upstream/7zip/Help/fm/index.htm"));
        window->close();
    }
    void sourceContextAndLanguageGeometry() {
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("language.ini");
        QWidget parent;
        ShowHelpWindow("fm/plugins/7-zip/add.htm#options", &parent);
        auto *window = parent.findChild<QDialog *>("helpWindow");
        QVERIFY(window);
        auto *browser = window->findChild<QTextBrowser *>("helpBrowser");
        QVERIFY(browser->toPlainText().contains("Add to Archive"));
        const QUrl source = browser->source();
        for (const char *language : {"-", "ru", "-"}) {
            SaveRegLang(UString(language), settings);
            ReloadLang(settings);
            QtNotifyLanguageChanged();
            QCOMPARE(browser->source(), source);
            QCOMPARE(window->windowTitle(), QString(language) == "ru"
                                                ? QStringLiteral("Справка 7-Zip")
                                                : QString("7-Zip Help"));
            for (QSize size : {QSize(760, 560), QSize(420, 300)}) {
                window->resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*browser, *window));
                auto *buttons = window->findChild<QDialogButtonBox *>();
                QVERIFY(UiInspection::fullyVisible(*buttons, *window));
                QVERIFY(UiInspection::writeReport(
                    *window, QString("help-%1-%2").arg(language).arg(size.width())));
            }
        }
        window->close();
    }
    void addHelpCrossChapterLinks_data() {
        QTest::addColumn<QString>("caption");
        QTest::addColumn<QString>("topic");
        QTest::addColumn<QString>("content");
        QTest::addColumn<bool>("keyboard");
        QTest::newRow("method") << "-m (Method)" << "method.htm"
                                << "Specifies the compression method." << false;
        QTest::newRow("sfx") << "-sfx (Create SFX archive)" << "sfx.htm"
                             << "Creates self extracting archive." << false;
        QTest::newRow("method-keyboard") << "-m (Method)" << "method.htm"
                                         << "Specifies the compression method." << true;
        QTest::newRow("sfx-keyboard") << "-sfx (Create SFX archive)" << "sfx.htm"
                                      << "Creates self extracting archive." << true;
    }
    void addHelpCrossChapterLinks() {
        QFETCH(QString, caption);
        QFETCH(QString, topic);
        QFETCH(QString, content);
        QFETCH(bool, keyboard);
        QWidget parent;
        ShowHelpWindow("fm/plugins/7-zip/add.htm#options", &parent);
        auto *window = parent.findChild<QDialog *>("helpWindow");
        QVERIFY(window);
        auto *browser = window->findChild<QTextBrowser *>("helpBrowser");
        const QUrl source = browser->source();
        const auto link = browser->document()->find(caption);
        QVERIFY(!link.isNull());
        QVERIFY(link.charFormat().isAnchor());
        browser->setTextCursor(link);
        browser->ensureCursorVisible();
        QApplication::processEvents();
        QTextCursor clickCursor = link;
        clickCursor.setPosition(link.selectionStart() + 2);
        const QPoint click = browser->cursorRect(clickCursor).center();
        QVERIFY(!browser->anchorAt(click).isEmpty());
        if (keyboard)
            QTest::keyClick(browser, Qt::Key_Return);
        else
            QTest::mouseClick(browser->viewport(), Qt::LeftButton, Qt::NoModifier, click);
        QCOMPARE(browser->source().path(), "/upstream/7zip/Help/cmdline/switches/" + topic);
        QVERIFY2(browser->toPlainText().contains(content), qPrintable(browser->toPlainText()));
        browser->backward();
        QCOMPARE(browser->source(), source);
        QVERIFY(browser->toPlainText().contains("Add to Archive"));
        browser->forward();
        QVERIFY(browser->toPlainText().contains(content));
        window->close();
    }
    void helpFollowsPaletteWithoutLosingNavigation() {
        const QPalette original = QApplication::palette();
        const auto restore = qScopeGuard([&] { QApplication::setPalette(original); });
        QWidget parent;
        ShowHelpWindow("start.htm", &parent);
        auto *window = parent.findChild<QDialog *>("helpWindow");
        auto *browser = window->findChild<QTextBrowser *>("helpBrowser");
        browser->anchorClicked(QUrl("general/formats.htm#zip"));
        const QUrl source = browser->source();
        const QString text = browser->toPlainText();
        for (bool dark : {true, false, true}) {
            QPalette colors = original;
            colors.setColor(QPalette::Base, QColor(dark ? "#202326" : "#ffffff"));
            colors.setColor(QPalette::Text, QColor(dark ? "#e8eaed" : "#202326"));
            colors.setColor(QPalette::AlternateBase, QColor(dark ? "#2c3035" : "#edf0f3"));
            colors.setColor(QPalette::Link, QColor(dark ? "#8cbcff" : "#205bb0"));
            colors.setColor(QPalette::Window, QColor(dark ? "#272b30" : "#f1f2f4"));
            colors.setColor(QPalette::WindowText, colors.color(QPalette::Text));
            colors.setColor(QPalette::Button, colors.color(QPalette::Window));
            colors.setColor(QPalette::ButtonText, colors.color(QPalette::Text));
            colors.setColor(QPalette::Mid, QColor(dark ? "#50565f" : "#b0b6be"));
            QApplication::processEvents();
            browser->verticalScrollBar()->setValue(40);
            const int scroll = browser->verticalScrollBar()->value();
            QApplication::setPalette(colors);
            QApplication::processEvents();
            QCOMPARE(browser->source(), source);
            QCOMPARE(browser->toPlainText(), text);
            QCOMPARE(browser->verticalScrollBar()->value(), scroll);
            QCOMPARE(browser->document()->rootFrame()->frameFormat().background().color(),
                     colors.color(QPalette::Base));
            auto cursor = browser->document()->find("Supported formats");
            QVERIFY(!cursor.isNull());
            QCOMPARE(cursor.charFormat().foreground().color(), colors.color(QPalette::Text));
            cursor = browser->document()->find("BZIP2");
            QVERIFY(cursor.currentTable());
            QCOMPARE(cursor.currentTable()->cellAt(cursor).format().background().color(),
                     colors.color(QPalette::Base));
            cursor = browser->document()->find("Creation");
            QCOMPARE(cursor.currentTable()->cellAt(cursor).format().background().color(),
                     colors.color(QPalette::AlternateBase));
            cursor = browser->document()->find("ZIP", 0, QTextDocument::FindWholeWords);
            QVERIFY(cursor.charFormat().isAnchor());
            QCOMPARE(cursor.charFormat().foreground().color(), colors.color(QPalette::Link));
            browser->verticalScrollBar()->setValue(0);
            for (QSize size : {QSize(760, 560), QSize(420, 300)}) {
                window->resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*browser, *window));
                QVERIFY(UiInspection::writeReport(
                    *window, QString("help-theme-%1-%2").arg(dark).arg(size.width())));
            }
        }
        browser->backward();
        QVERIFY(browser->toPlainText().contains("Welcome to 7-Zip"));
        browser->forward();
        QCOMPARE(browser->source(), source);
        browser->setSource(QUrl("qrc:/upstream/7zip/Help/cmdline/syntax.htm"));
        auto cursor = browser->document()->find("expression1");
        QVERIFY(!cursor.isNull());
        QCOMPARE(cursor.blockFormat().background().color(),
                 QApplication::palette().color(QPalette::AlternateBase));
        QCOMPARE(cursor.charFormat().foreground().color(),
                 QApplication::palette().color(QPalette::Text));
        window->close();
    }
    void unavailableTopicIsObservableAndDoesNotOpenWindow() {
        QTemporaryDir fixture;
        QWidget parent;
        QString warning;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &parent, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                warning = box->text();
                box->accept();
            }
        });
        answer.start(5);
        QtOpenHelpTopic("fm/index.htm", &parent, fixture.path());
        QCOMPARE(warning, QString("Help topic is unavailable: fm/index.htm"));
        QVERIFY(!parent.findChild<QDialog *>("helpWindow"));
    }
};
QTEST_MAIN(HelpSourceTests)
#include "help_source_tests.moc"
