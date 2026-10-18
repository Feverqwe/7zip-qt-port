#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog2.h"
#include "CPP/7zip/UI/FileManager/CopyDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
#include "qtstatuslabel.h"
#include "window_state_support.h"

#include <QMenuBar>
#include <QScopeGuard>
#include <QScreen>
#include <QSizeGrip>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QWindow>

class MainShellTests final : public QObject {
    Q_OBJECT

  private slots:
    void elidedStatusUsesInactiveAndDisabledColors() {
        QWidget window;
        window.resize(240, 120);
        QtStatusLabel label(&window);
        label.setText(QString(30, QLatin1Char('M')));
        QFont font = label.font();
        font.setPointSize(24);
        label.setFont(font);
        label.setGeometry(10, 10, 100, 50);
        QPalette colors = label.palette();
        colors.setColor(QPalette::Window, Qt::white);
        colors.setColor(QPalette::Active, QPalette::WindowText, QColor(170, 40, 40));
        colors.setColor(QPalette::Inactive, QPalette::WindowText, QColor(40, 50, 170));
        colors.setColor(QPalette::Disabled, QPalette::WindowText, QColor(40, 140, 50));
        label.setPalette(colors);
        label.setAutoFillBackground(true);
        window.show();
        QWidget other;
        other.show();
        other.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&other));
        QVERIFY(!window.isActiveWindow());
        for (bool enabled : {true, false}) {
            label.setEnabled(enabled);
            const QImage image = label.grab().toImage();
            bool expectedText = false;
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x) {
                    const QColor pixel = image.pixelColor(x, y);
                    expectedText |=
                        enabled
                            ? pixel.blue() > pixel.red() + 50 && pixel.blue() > pixel.green() + 50
                            : pixel.green() > pixel.red() + 50 && pixel.green() > pixel.blue() + 50;
                }
            QVERIFY(expectedText);
        }
    }
    void fileManagerWindowUsesSourceApplicationIcon() {
        ControlledBackend backend;
        MainWindow window(&backend);
        const QIcon original(QStringLiteral(":/upstream/7zip/UI/FileManager/FM.ico"));
        QVERIFY(!original.isNull());
        QVERIFY(!window.windowIcon().isNull());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(window.windowHandle());
        for (const QSize &size : original.availableSizes())
            QCOMPARE(window.windowHandle()->icon().pixmap(size).toImage(),
                     original.pixmap(size).toImage());
    }

    void startupToolbarUsesActualScreenMetrics_data() {
        QTest::addColumn<bool>("smallScreen");
        QTest::addColumn<quint32>("mask");
        QTest::addColumn<bool>("labels");
        QTest::newRow("default-normal") << false << quint32(0x8000000d) << true;
        QTest::newRow("default-small") << true << quint32(0x8000000d) << false;
        QTest::newRow("explicit-labels-small") << true << quint32(13) << true;
        QTest::newRow("explicit-icons-normal") << false << quint32(12) << false;
    }

    void startupToolbarUsesActualScreenMetrics() {
        QFETCH(bool, smallScreen);
        QFETCH(quint32, mask);
        QFETCH(bool, labels);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QByteArray oldSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const auto restoreSettings =
            qScopeGuard([&] { qputenv("SEVENZIP_SETTINGS_PATH", oldSettings); });
        const QString settings = fixture.filePath("screen toolbar.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings.toUtf8());
        SaveToolbarsMask(mask, settings);
        const QFont oldFont = QApplication::font();
        const auto restoreFont = qScopeGuard([&] { QApplication::setFont(oldFont); });
        if (smallScreen) {
            QVERIFY(QApplication::primaryScreen());
            QFont font = oldFont;
            font.setPixelSize(QApplication::primaryScreen()->availableGeometry().height() / 25 +
                              16);
            QApplication::setFont(font);
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        auto *option = window.findChild<QAction *>("buttonTextOption");
        QVERIFY(toolbar && option);
        QCOMPARE(option->isChecked(), labels);
        auto *add = qobject_cast<QToolButton *>(
            toolbar->widgetForAction(window.findChild<QAction *>("toolbarAddAction")));
        QVERIFY(add);
        QCOMPARE(add->toolButtonStyle(),
                 labels ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
        QCOMPARE(ReadToolbarsMask(settings), UInt32(mask));
    }

    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void sourcePanelChrome_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const QString &language : {QString("-"), QString("ru")})
            for (const QSize size : {QSize(980, 620), QSize(640, 400)})
                QTest::newRow(qPrintable(language + QString::number(size.width())))
                    << language << size;
    }

    void sourcePanelChrome() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        const UString oldLanguage = g_LangID;
        const auto restoreLanguage = qScopeGuard([&] {
            SaveRegLang(oldLanguage, settings);
            ReloadLang(settings);
            QtNotifyLanguageChanged();
        });
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        const QString left = fixture.filePath(QStringLiteral("левый каталог 空"));
        const QString right = fixture.filePath("right folder");
        QVERIFY(QDir().mkpath(left) && QDir().mkpath(right));
        SavePanelPath(0, ToUString(left), settings);
        SavePanelPath(1, ToUString(right), settings);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.resize(size);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QVERIFY(window.findChild<QAction *>("backAction"));
        auto *twoPanels = window.findChild<QAction *>("twoPanelsAction");
        QVERIFY(twoPanels);
        for (int phase = 0; phase < 3; ++phase) {
            if (twoPanels->isChecked() != (phase == 1))
                twoPanels->trigger();
            QApplication::processEvents();
            QStringList differences;
            if (window.statusBar()->isVisible())
                differences << "shared idle status bar";
            for (unsigned index = 0; index < (phase == 1 ? 2u : 1u); ++index) {
                auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(index));
                QVERIFY(panel);
                const auto buttons =
                    panel->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly);
                if (buttons.size() != 1 ||
                    buttons[0]->objectName() != QString("panelParentButton%1").arg(index))
                    differences << QString("panel%1 navigation buttons").arg(index);
                auto *status = panel->findChild<QStatusBar *>("panelStatusBar");
                QVERIFY(status && UiInspection::fullyVisible(*status, window));
                QVERIFY(UiInspection::fullyVisible(*panel->findChild<QComboBox *>("pathCombo"),
                                                   window));
            }
            QVERIFY(UiInspection::writeReport(window, QString("source-panel-chrome-%1-%2-%3")
                                                          .arg(language)
                                                          .arg(size.width())
                                                          .arg(phase)));
            QVERIFY2(differences.isEmpty(), qPrintable(differences.join(", ")));
        }
        window.statusBar()->showMessage(QStringLiteral("operation проверка 空"));
        QApplication::processEvents();
        QVERIFY(UiInspection::fullyVisible(*window.statusBar(), window));
        window.statusBar()->clearMessage();
        QApplication::processEvents();
        QVERIFY(!window.statusBar()->isVisible());
    }

    void statusBarsKeepNativeNoGripStyle_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (const auto &language : {QStringLiteral("en"), QStringLiteral("ru")})
            for (const QSize size : {QSize(980, 620), QSize(640, 400)})
                QTest::newRow(qPrintable(language + QString::number(size.width())))
                    << language << size;
    }

    void statusBarsKeepNativeNoGripStyle() {
        QFETCH(QString, language);
        QFETCH(QSize, size);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        const auto restoreLanguage = qScopeGuard([&] {
            SaveRegLang(UString(), settings);
            ReloadLang(settings);
            QtNotifyLanguageChanged();
        });
        ControlledBackend backend;
        MainWindow window(&backend);
        window.resize(size);
        window.show();
        auto *twoPanels = window.findChild<QAction *>("twoPanelsAction");
        QVERIFY(twoPanels);
        const QString message = QStringLiteral("status проверка 空");
        for (int phase = 0; phase < 3; ++phase) {
            window.statusBar()->showMessage(message);
            if (twoPanels->isChecked() != (phase == 1))
                twoPanels->trigger();
            QApplication::processEvents();
            QCOMPARE(window.statusBar()->currentMessage(), message);
            QVERIFY(UiInspection::fullyVisible(*window.statusBar(), window));
            for (auto *bar : window.findChildren<QStatusBar *>())
                QVERIFY2(!bar->isSizeGripEnabled(), "7zFM status bars have no resize grip");
            for (auto *grip : window.findChildren<QSizeGrip *>())
                QVERIFY(!grip->isVisible());
            QVERIFY(UiInspection::writeReport(window, QStringLiteral("status-no-grip-%1-%2-%3")
                                                          .arg(language)
                                                          .arg(size.width())
                                                          .arg(phase)));
        }
    }

    void nativeTemporaryFilesCommandUsesActualModalConsumer() {
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *action = window.findChild<QAction *>("tempDirAction");
        QVERIFY(action && action->isEnabled());
        bool observed = false;
        QTimer close;
        connect(&close, &QTimer::timeout, &window, [&] {
            auto *dialog = dynamic_cast<CBrowseDialog2 *>(QApplication::activeModalWidget());
            if (dialog && dialog->parentWidget() == &window) {
                observed = dialog->findChild<QTreeWidget *>("tempList") != nullptr;
                close.stop();
                QTest::keyClick(dialog, Qt::Key_Escape);
            }
        });
        close.start(5);
        action->trigger();
        QVERIFY(observed);
        QVERIFY(!window.findChild<QDialog *>("tempBrowseDialog"));
        QVERIFY(!backend.busy());
    }

    void sourceCommandsKeepAvailabilityAndShortcuts() {
        ControlledBackend backend;
        MainWindow window(&backend);
        for (const char *name : {"diffAction", "linkAction", "benchmarkAction"}) {
            auto *action = window.findChild<QAction *>(name);
            QVERIFY(action);
            QVERIFY(!action->isEnabled());
        }
        auto *streams = window.findChild<QAction *>("altStreamsAction");
        QVERIFY(streams);
#ifdef Q_OS_WIN
        QVERIFY(streams->isEnabled()); // Source allows opening the current folder's ADS.
#else
        QVERIFY(!streams->isEnabled());
#endif
        QVERIFY(window.findChild<QAction *>("helpContentsAction")->isEnabled());
        QVERIFY(window.findChild<QAction *>("aboutAction")->isEnabled());
        QVERIFY(window.findChild<QAction *>("tempDirAction")->isEnabled());
        QCOMPARE(window.findChild<QAction *>("openInsideAction")->shortcut(),
                 QKeySequence(Qt::CTRL | Qt::Key_PageDown));
        QCOMPARE(window.findChild<QAction *>("viewFileAction")->shortcut(),
                 QKeySequence(Qt::Key_F3));
        QCOMPARE(window.findChild<QAction *>("editFileAction")->shortcut(),
                 QKeySequence(Qt::Key_F4));
    }

    void toolbarCommandsFitBothSizes_data() {
        QTest::addColumn<QString>("language");
        QTest::newRow("en") << QStringLiteral("-");
        QTest::newRow("ru") << QStringLiteral("ru");
    }

    void toolbarCommandsFitBothSizes() {
        QFETCH(QString, language);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        const UString oldLanguage = g_LangID;
        const auto restoreLanguage = qScopeGuard([&] {
            SaveRegLang(oldLanguage, settings);
            ReloadLang(settings);
            QtNotifyLanguageChanged();
        });
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        SaveToolbarsMask(13, settings);
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        QVERIFY(toolbar);
        const char *actions[] = {"toolbarAddAction", "extractArchiveAction", "testArchiveAction",
                                 "copyAction",       "moveAction",           "deleteAction",
                                 "archiveInfoAction"};
        for (unsigned i = 0; i < 7; ++i) {
            auto *action = window.findChild<QAction *>(actions[i]);
            QVERIFY(action);
            auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
            QVERIFY(button);
            QVERIFY(!button->accessibleName().isEmpty());
            QVERIFY(button->toolTip().contains(button->accessibleName()));
            if (!action->shortcut().isEmpty())
                QVERIFY(button->toolTip().contains(
                    action->shortcut().toString(QKeySequence::NativeText)));
            QCOMPARE(button->toolButtonStyle(),
                     i < 2 ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
            QVERIFY(!button->icon().pixmap(toolbar->iconSize()).isNull());
        }
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            window.show();
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*toolbar, window));
            for (const char *name : actions) {
                auto *button = toolbar->widgetForAction(window.findChild<QAction *>(name));
                QVERIFY(UiInspection::fullyVisible(*button, window));
            }
            QVERIFY(UiInspection::writeReport(window, QStringLiteral("main-shell-%1-%2x%3")
                                                          .arg(language)
                                                          .arg(size.width())
                                                          .arg(size.height())));
            for (const char *name : {"fileMenu", "viewMenu"}) {
                auto *menu = window.findChild<QMenu *>(name);
                QVERIFY(menu);
                menu->popup(window.menuBar()->mapToGlobal(QPoint(0, window.menuBar()->height())));
                QApplication::processEvents();
                QVERIFY(menu->isVisible());
                QVERIFY(UiInspection::writeReport(*menu, QStringLiteral("main-shell-%1-%2x%3")
                                                             .arg(QString::fromLatin1(name))
                                                             .arg(size.width())
                                                             .arg(size.height())));
                menu->hide();
            }
        }
        auto *large = window.findChild<QAction *>("largeButtonsOption");
        QVERIFY(large);
        large->trigger();
        QCOMPARE(toolbar->iconSize(), QSize(48, 36));
        for (const char *name : actions) {
            auto *button = qobject_cast<QToolButton *>(
                toolbar->widgetForAction(window.findChild<QAction *>(name)));
            QVERIFY(UiInspection::fullyVisible(*button, window));
            QVERIFY(!button->icon().pixmap(toolbar->iconSize()).isNull());
        }
    }

    void fileMenuTracksTheFocusedPanelAfterToggleAndTab() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString left = fixture.filePath("left");
        const QString right = fixture.filePath("right with space");
        QVERIFY(QDir().mkpath(left));
        QVERIFY(QDir().mkpath(right));
        QFile file(QDir(right).filePath(QStringLiteral("имя.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        SavePanelPath(1, ToUString(right), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *leftPanel = window.findChild<QWidget *>("filePanel0");
        QVERIFY(leftPanel);
        auto *leftTable = leftPanel->findChild<QTableWidget *>("archiveTable");
        auto *leftAddress = leftPanel->findChild<QLineEdit *>("archivePath");
        leftAddress->setText(left);
        QTest::keyClick(leftAddress, Qt::Key_Return);
        QTRY_COMPARE(QFileInfo(leftAddress->text()).canonicalFilePath(),
                     QFileInfo(left).canonicalFilePath());
        auto *rename = window.findChild<QAction *>("renameAction");
        QVERIFY(rename);
        QVERIFY(!rename->isEnabled());
        window.findChild<QAction *>("twoPanelsAction")->trigger();
        auto *rightPanel = window.findChild<QWidget *>("filePanel1");
        QVERIFY(rightPanel && rightPanel->isVisible());
        auto *rightTable = rightPanel->findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(rightTable->rowCount() > 0);
        for (int row = 0; row < rightTable->rowCount(); ++row) {
            if (rightTable->item(row, 0)->text() == QStringLiteral("имя.txt")) {
                rightTable->selectRow(row);
                rightTable->setCurrentCell(row, 0);
                break;
            }
        }
        rightTable->setFocus();
        QTRY_VERIFY(rename->isEnabled());
        QVERIFY(window.findChild<QMenu *>("fileMenu")->actions().contains(rename));
        auto *copy = window.findChild<QAction *>("copyAction");
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(copy));
        QVERIFY(button && button->isEnabled());
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        button->setFocus();
        QTRY_VERIFY(button->hasFocus());
        QSignalSpy invoked(copy, &QAction::triggered);
        bool correctSource = false, correctDestination = false;
        QTimer cancel;
        connect(&cancel, &QTimer::timeout, &window, [&] {
            if (auto *dialog = dynamic_cast<CCopyDialog *>(QApplication::activeModalWidget())) {
                correctSource = ToQString(dialog->Info).contains(QStringLiteral("имя.txt"));
                correctDestination = QFileInfo(dialog->destination()).canonicalFilePath() ==
                                     QFileInfo(left).canonicalFilePath();
                cancel.stop();
                dialog->reject();
            }
        });
        cancel.start(5);
        QTest::keyClick(button, Qt::Key_Space);
        QCOMPARE(invoked.size(), 1);
        QVERIFY(correctSource && correctDestination);
        QVERIFY(QFileInfo::exists(file.fileName()));
        QVERIFY(!QFileInfo::exists(QDir(left).filePath(QStringLiteral("имя.txt"))));
        QTRY_VERIFY(rightTable->isEnabled());
        QApplication::processEvents();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        QTest::keyClick(rightTable, Qt::Key_Tab);
        QTRY_VERIFY(leftTable->hasFocus());
        QVERIFY(!rename->isEnabled());
    }
};

#include "review_test_main.h"
SEVENZIP_UI_TEST_MAIN(MainShellTests)
#include "main_shell_tests.moc"
