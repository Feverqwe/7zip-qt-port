#include "7zip/MyVersion.h"
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
#include <cmath>

namespace {
qreal textContrast(const QColor &foreground, const QColor &background) {
    const auto luminance = [](const QColor &color) {
        const auto linear = [](qreal value) {
            return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
        };
        return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) +
               0.0722 * linear(color.blueF());
    };
    const qreal first = luminance(foreground), second = luminance(background);
    return (qMax(first, second) + 0.05) / (qMin(first, second) + 0.05);
}
} // namespace

class HelpSourceTests : public QObject {
    Q_OBJECT
    QUrl openedUrl;
  public slots:
    void openUrl(const QUrl &url) { openedUrl = url; }
  private slots:
    void tableParameterNamesStayReadable() {
        QWidget parent;
        ShowHelpWindow("cmdline/switches/method.htm#7Z", &parent);
        auto *window = parent.findChild<QDialog *>("helpWindow");
        auto *browser = window->findChild<QTextBrowser *>("helpBrowser");
        const QString parameter = "{N}={MethodID}[:param1][:param2][..]";
        for (QSize size : {QSize(760, 560), QSize(420, 300)}) {
            window->resize(size);
            QApplication::processEvents();
            const auto token = browser->document()->find(parameter);
            QVERIFY(!token.isNull());
            QTextCursor first = token;
            first.setPosition(token.selectionStart());
            QTextCursor last = token;
            last.setPosition(token.selectionEnd() - 1);
            QCOMPARE(browser->cursorRect(first).top(), browser->cursorRect(last).top());
            const auto *table = token.currentTable();
            const auto header = table->cellAt(0, table->columns() - 1);
            first = header.firstCursorPosition();
            last = header.lastCursorPosition();
            last.movePosition(QTextCursor::PreviousCharacter);
            QCOMPARE(browser->cursorRect(first).top(), browser->cursorRect(last).top());
            browser->setTextCursor(last);
            // Qt can publish the document's new scroll range on the next event
            // turn, after laying out the nonbreaking table cells.
            QTRY_VERIFY2((browser->ensureCursorVisible(),
                          browser->cursorRect(last).left() < browser->viewport()->width()),
                         qPrintable(QString("cursor %1, viewport %2, document %3, scroll %4/%5")
                                        .arg(browser->cursorRect(last).left())
                                        .arg(browser->viewport()->width())
                                        .arg(browser->document()->size().width())
                                        .arg(browser->horizontalScrollBar()->value())
                                        .arg(browser->horizontalScrollBar()->maximum())));
            const int horizontal = browser->horizontalScrollBar()->value();
            browser->scrollToAnchor("7Z");
            browser->horizontalScrollBar()->setValue(horizontal);
            QVERIFY(UiInspection::writeReport(
                *window, QString("help-method-table-right-%1").arg(size.width())));
            browser->horizontalScrollBar()->setValue(0);
            browser->scrollToAnchor("7Z");
            QVERIFY(UiInspection::writeReport(*window,
                                              QString("help-method-table-%1").arg(size.width())));
        }
        window->close();
    }
    void commandBlocksPreserveSourceText_data() {
        QTest::addColumn<QString>("topic");
        QTest::newRow("syntax") << "cmdline/syntax.htm";
        QTest::newRow("examples") << "cmdline/commands/add.htm";
        QTest::newRow("nested-examples") << "cmdline/switches/method.htm#SevenZipYV";
        QTest::newRow("indented-examples") << "cmdline/switches/method.htm#MemUse";
    }
    void commandBlocksPreserveSourceText() {
        QFETCH(QString, topic);
        QWidget parent;
        ShowHelpWindow(topic.toUtf8().constData(), &parent);
        auto *browser = parent.findChild<QTextBrowser *>("helpBrowser");
        QTextBrowser reference;
        reference.setSource(browser->source());
        // Presentation must retain the upstream whitespace and line structure,
        // including PRE blocks nested inside table cells and definitions.
        QCOMPARE(browser->toPlainText(), reference.toPlainText());
        auto *window = parent.findChild<QDialog *>("helpWindow");
        for (QSize size : {QSize(760, 560), QSize(420, 300)}) {
            window->resize(size);
            browser->scrollToAnchor(browser->source().fragment());
            const QString needle = topic.contains("SevenZipYV") ? "yv=1600"
                                   : topic.contains("MemUse")   ? "memuse=p60"
                                                                : "";
            if (!needle.isEmpty()) {
                const auto code = browser->document()->find(needle);
                QVERIFY(!code.isNull());
                browser->setTextCursor(code);
                browser->ensureCursorVisible();
                browser->verticalScrollBar()->setValue(browser->verticalScrollBar()->value() +
                                                       browser->cursorRect(code).center().y() -
                                                       browser->viewport()->height() / 2);
                QVERIFY(textContrast(code.charFormat().foreground().color(),
                                     code.blockFormat().background().color()) >= 4.5);
                auto caret = browser->textCursor();
                caret.clearSelection();
                browser->setTextCursor(caret);
            }
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*browser, *window));
            QVERIFY(UiInspection::fullyVisible(*window->findChild<QDialogButtonBox *>(), *window));
            QVERIFY(UiInspection::writeReport(
                *window,
                QString("help-code-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        window->close();
    }
    void aboutInitialFrameScalesWithFont() {
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
    void aboutIdentityAttributionLinksAndGeometry_data() {
        QTest::addColumn<bool>("dark");
        QTest::addColumn<bool>("systemPalette");
        QTest::newRow("system") << false << true;
        QTest::newRow("light") << false << false;
        QTest::newRow("dark") << true << false;
    }
    void aboutIdentityAttributionLinksAndGeometry() {
        QFETCH(bool, dark);
        QFETCH(bool, systemPalette);
        const QPalette original = QApplication::palette();
        const auto restorePalette = qScopeGuard([&] { QApplication::setPalette(original); });
        QPalette colors = original;
        colors.setColor(QPalette::Window, QColor(dark ? "#272b30" : "#f1f2f4"));
        colors.setColor(QPalette::WindowText, QColor(dark ? "#e8eaed" : "#202326"));
        // Native macOS can supply a bright AlternateBase in dark appearance.
        colors.setColor(QPalette::AlternateBase, QColor(dark ? "#8e8e8e" : "#ffffff"));
        colors.setColor(QPalette::Button, colors.color(QPalette::Window));
        colors.setColor(QPalette::ButtonText, colors.color(QPalette::WindowText));
        colors.setColor(QPalette::Link, QColor(dark ? "#8cbcff" : "#205bb0"));
        if (!systemPalette)
            QApplication::setPalette(colors);
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("language.ini");
        CAboutDialog dialog;
        dialog.show();
        auto *version = dialog.findChild<QLabel *>("aboutText101");
        QVERIFY(version->text().contains("7-Zip " MY_VERSION));
        QVERIFY(dialog.findChild<QLabel *>("aboutText102")->text().contains(MY_DATE));
        QVERIFY(
            dialog.findChild<QLabel *>("aboutUpstreamCopyright")->text().contains("Igor Pavlov"));
        auto *applicationVersion = dialog.findChild<QLabel *>("aboutApplicationVersion");
        QCOMPARE(applicationVersion->text(), QString(SEVENZIP_DESKTOP_VERSION));
        QDesktopServices::setUrlHandler("https", this, "openUrl");
        const auto resetHandler = qScopeGuard([] { QDesktopServices::unsetUrlHandler("https"); });
        QVERIFY(!dialog.OnButtonClicked(999, nullptr));
        dialog.findChild<QPushButton *>("aboutProjectHomepage")->click();
        QCOMPARE(openedUrl, QUrl("https://github.com/Feverqwe/7zip-qt-port"));
        dialog.findChild<QPushButton *>("aboutHomepage")->click();
        QCOMPARE(openedUrl, QUrl("https://www.7-zip.org/"));
        for (const char *language : {"-", "ru", "-"}) {
            SaveRegLang(UString(language), settings);
            ReloadLang(settings);
            QtNotifyLanguageChanged();
            QCOMPARE(
                dialog.windowTitle(),
                QString(language == QByteArray("ru") ? "О программе Sevenzip" : "About Sevenzip"));
            QVERIFY(version->text().contains("7-Zip " MY_VERSION));
            QCOMPARE(applicationVersion->text(), QString(SEVENZIP_DESKTOP_VERSION));
            for (QSize size : {QSize(360, 330), QSize(300, 310)}) {
                dialog.resize(size);
                for (auto *label : dialog.findChildren<QLabel *>()) {
                    // Language changes can queue a second layout pass for the
                    // upstream group's newly wrapped free-software notice.
                    QTRY_VERIFY(UiInspection::fullyVisible(*label, dialog));
                    QTRY_VERIFY2(UiInspection::labelFits(*label), qPrintable(label->objectName()));
                }
                for (auto *button : dialog.findChildren<QPushButton *>()) {
                    QVERIFY(UiInspection::fullyVisible(*button, dialog));
                    QVERIFY(button->width() >= button->sizeHint().width());
                }
                const auto *heading = dialog.findChild<QLabel *>("aboutApplicationName");
                QVERIFY(UiInspection::bounds(*heading, dialog).bottom() <
                        UiInspection::bounds(*version, dialog).top());
                QVERIFY(UiInspection::separate(*heading, *applicationVersion, dialog));
                auto *attribution = dialog.findChild<QWidget *>("aboutUpstream");
                const QPixmap rendered = dialog.grab();
                const QImage surface = rendered.toImage();
                // Sample the empty top padding inside the rounded panel so
                // contrast includes the stylesheet's painted background.
                const QPoint sample = attribution->mapTo(&dialog, QPoint(12, 12));
                const QColor background =
                    surface.pixelColor(qRound(sample.x() * rendered.devicePixelRatio()),
                                       qRound(sample.y() * rendered.devicePixelRatio()));
                for (auto *label : attribution->findChildren<QLabel *>())
                    QVERIFY(textContrast(label->palette().color(QPalette::WindowText),
                                         background) >= 4.5);
                QVERIFY(UiInspection::writeReport(dialog, QString("about-%1-%2-%3")
                                                              .arg(language)
                                                              .arg(size.width())
                                                              .arg(QTest::currentDataTag())));
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
            const QColor header =
                cursor.currentTable()->cellAt(cursor).format().background().color();
            QVERIFY(header != colors.color(QPalette::Base));
            QVERIFY(textContrast(colors.color(QPalette::Text), header) >= 4.5);
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
                browser->setSource(QUrl("qrc:/upstream/7zip/Help/cmdline/syntax.htm"));
                const auto syntax = browser->document()->find("expression1");
                QVERIFY(!syntax.isNull());
                QVERIFY(syntax.blockFormat().background().color() != colors.color(QPalette::Base));
                QVERIFY(textContrast(syntax.charFormat().foreground().color(),
                                     syntax.blockFormat().background().color()) >= 4.5);
                QCOMPARE(syntax.charFormat().foreground().color(), colors.color(QPalette::Text));
                QVERIFY(browser->toPlainText().contains(
                    "expression1 | expression2 | ... | expressionN"));
                browser->verticalScrollBar()->setValue(0);
                QVERIFY(UiInspection::writeReport(
                    *window, QString("help-syntax-%1-%2").arg(dark).arg(size.width())));
                browser->backward();
            }
        }
        browser->backward();
        QVERIFY(browser->toPlainText().contains("Welcome to 7-Zip"));
        browser->forward();
        QCOMPARE(browser->source(), source);
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
