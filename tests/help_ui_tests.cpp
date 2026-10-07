#include "createarchivedialog.h"
#include "extractdialog.h"
#include "qtfmoptionsdialog.h"
#include "window_state_support.h"
#include <QSettings>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTimer>

class HelpUiTests : public QObject {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void optionsHelpRoutesEachNativePageWithoutApplying() {
        const QString settingsPath = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QtFmOptionsDialog dialog(settingsPath);
        dialog.show();
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        auto *buttons = dialog.findChild<QDialogButtonBox *>("settingsButtons");
        auto *command = dialog.findChild<QLineEdit *>("viewerCommand");
        command->setText("pending viewer %1");
        QVERIFY(buttons->button(QDialogButtonBox::Apply)->isEnabled());
        const QStringList keysBefore = QSettings(settingsPath, QSettings::IniFormat).allKeys();
        const QStringList fragments = {"system", "sevenZip", "folders",
                                       "editor", "settings", "language"};
        for (int i = 0; i < pages->count(); ++i) {
            pages->setCurrentIndex(i);
            buttons->button(QDialogButtonBox::Help)->click();
            auto *help = pages->widget(i)->findChild<QDialog *>("helpWindow");
            QVERIFY(help && help->isVisible());
            auto *browser = help->findChild<QTextBrowser *>("helpBrowser");
            QCOMPARE(browser->source().path(), QString("/upstream/7zip/Help/fm/options.htm"));
            QCOMPARE(browser->source().fragment(), fragments[i]);
            help->close();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QCOMPARE(command->text(), QString("pending viewer %1"));
            QVERIFY(buttons->button(QDialogButtonBox::Apply)->isEnabled());
            QCOMPARE(QSettings(settingsPath, QSettings::IniFormat).allKeys(), keysBefore);
        }
        pages->setCurrentIndex(3);
        dialog.activateWindow();
        QTRY_VERIFY(dialog.isActiveWindow());
        QTest::keyClick(command, Qt::Key_F1);
        auto *help = pages->widget(3)->findChild<QDialog *>("helpWindow");
        QVERIFY(help && help->isVisible());
        QCOMPARE(help->findChild<QTextBrowser *>("helpBrowser")->source().fragment(),
                 QString("editor"));
        help->close();
        dialog.reject();
    }
    void addAndExtractHelpRetainPendingInput_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::newRow("button") << false;
        QTest::newRow("F1") << true;
    }
    void addAndExtractHelpRetainPendingInput() {
        QFETCH(bool, keyboard);
        QTemporaryDir fixture;
        CreateArchiveDialog add(fixture.path());
        add.show();
        add.activateWindow();
        QTRY_VERIFY(add.isActiveWindow());
        auto *name = add.findChild<QLineEdit *>("archiveName");
        QVERIFY(name);
        name->setText(QStringLiteral("новый архив с пробелом"));
        const auto showHelp = [keyboard](QDialog &dialog, const QString &topic) {
            auto *buttons = dialog.findChild<QDialogButtonBox *>();
            if (keyboard)
                QTest::keyClick(&dialog, Qt::Key_F1);
            else
                buttons->button(QDialogButtonBox::Help)->click();
            auto *help = dialog.findChild<QDialog *>("helpWindow");
            if (!help || !help->isVisible())
                return false;
            const bool matches = help->findChild<QTextBrowser *>("helpBrowser")->source().path() ==
                                 "/upstream/7zip/Help/fm/plugins/7-zip/" + topic;
            help->close();
            return matches;
        };
        QVERIFY(showHelp(add, "add.htm"));
        QCOMPARE(name->text(), QStringLiteral("новый архив с пробелом"));
        add.reject();
        ExtractDialog extract(fixture.path(), {});
        extract.show();
        extract.activateWindow();
        QTRY_VERIFY(extract.isActiveWindow());
        auto *destination = extract.findChild<QComboBox *>("extractDestination");
        destination->setEditText(fixture.filePath(QStringLiteral("папка с пробелом")));
        auto *password = extract.findChild<QLineEdit *>("extractPassword");
        password->setText("pending secret");
        const QString pendingDestination = destination->currentText();
        QVERIFY(showHelp(extract, "extract.htm"));
        QCOMPARE(destination->currentText(), pendingDestination);
        QCOMPARE(password->text(), QString("pending secret"));
        extract.reject();
    }
    void sourceAboutCommandUsesModalOwner() {
        MainWindow window;
        auto *action = window.findChild<QAction *>("aboutAction");
        QVERIFY(action && action->isEnabled());
        bool observed = false;
        QTimer close;
        connect(&close, &QTimer::timeout, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("aboutDialog");
            if (dialog && dialog->isVisible()) {
                observed = QApplication::activeModalWidget() == dialog;
                close.stop();
                dialog->accept();
            }
        });
        close.start(5);
        action->trigger();
        QVERIFY(observed);
        QVERIFY(!window.findChild<QDialog *>("aboutDialog"));
    }
    void sourceContentsCommandAndF1_data() {
        QTest::addColumn<bool>("keyboard");
        QTest::newRow("menu") << false;
        QTest::newRow("F1") << true;
    }
    void sourceContentsCommandAndF1() {
        QFETCH(bool, keyboard);
        MainWindow window;
        window.show();
        window.activateWindow();
        QTRY_VERIFY(window.isActiveWindow());
        auto *action = window.findChild<QAction *>("helpContentsAction");
        QVERIFY(action && action->isEnabled());
        QCOMPARE(action->shortcut(), QKeySequence(Qt::Key_F1));
        if (keyboard)
            QTest::keyClick(&window, Qt::Key_F1);
        else
            action->trigger();
        auto *help = window.findChild<QDialog *>("helpWindow");
        QVERIFY(help && help->isVisible());
        auto *browser = help->findChild<QTextBrowser *>("helpBrowser");
        QCOMPARE(browser->source().path(), QString("/upstream/7zip/Help/fm/index.htm"));
        QVERIFY(browser->toPlainText().contains("File Manager"));
        help->close();
    }
};
QTEST_MAIN(HelpUiTests)
#include "help_ui_tests.moc"
