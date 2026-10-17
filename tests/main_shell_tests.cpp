#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog2.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "qtlangcontrols.h"
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

    static QStringList actionNames(const QMenu &menu) {
        QStringList names;
        for (QAction *action : menu.actions())
            names << (action->isSeparator() ? QStringLiteral("|") : action->objectName());
        return names;
    }

  private slots:
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
        QCOMPARE(toolbar->toolButtonStyle(),
                 labels ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
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
        QCOMPARE(action->property("nativeCommandID").toUInt(), unsigned(IDM_TEMP_DIR));
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

    void sourceMenusKeepOrderAndUnavailableCommandsVisible() {
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *file = window.findChild<QMenu *>("fileMenu");
        auto *edit = window.findChild<QMenu *>("editMenu");
        auto *view = window.findChild<QMenu *>("viewMenu");
        auto *favorites = window.findChild<QMenu *>("favoritesMenu");
        auto *tools = window.findChild<QMenu *>("toolsMenu");
        auto *help = window.findChild<QMenu *>("helpMenu");
        QVERIFY(file && edit && view && favorites && tools && help);
        for (const char *name : {"openArchiveAction", "createArchiveAction", "addArchiveAction",
                                 "extractSelectedAction", "operationJournalAction",
                                 "builtinTextViewAction", "builtinTextEditAction"})
            QVERIFY(!window.findChild<QAction *>(name));
        QCOMPARE(window.menuBar()->actions().size(), 6);
        QCOMPARE(actionNames(*file), QStringList({"openItemAction",
                                                  "openInsideAction",
                                                  "openInsideOneAction",
                                                  "openInsideParserAction",
                                                  "openOutsideAction",
                                                  "viewFileAction",
                                                  "editFileAction",
                                                  "|",
                                                  "renameAction",
                                                  "copyAction",
                                                  "moveAction",
                                                  "deleteAction",
                                                  "|",
                                                  "splitFileAction",
                                                  "combineFilesAction",
                                                  "|",
                                                  "selectedPropertiesAction",
                                                  "commentAction",
                                                  "hashMenu",
                                                  "diffAction",
                                                  "|",
                                                  "createFolderAction",
                                                  "createFileAction",
                                                  "|",
                                                  "linkAction",
                                                  "altStreamsAction",
                                                  "|",
                                                  "exitAction",
                                                  "verEditAction",
                                                  "verCommitAction",
                                                  "verRevertAction",
                                                  "verDiffAction"}));
        QCOMPARE(actionNames(*edit),
                 QStringList({"selectAllAction", "deselectAllAction", "invertSelectionAction",
                              "selectMaskAction", "deselectMaskAction", "|", "selectTypeAction",
                              "deselectTypeAction"}));
        QCOMPARE(actionNames(*view), QStringList({"viewMode0Action",
                                                  "viewMode1Action",
                                                  "viewMode2Action",
                                                  "viewMode3Action",
                                                  "|",
                                                  "sort0Action",
                                                  "sort1Action",
                                                  "sort2Action",
                                                  "sort3Action",
                                                  "sort4Action",
                                                  "|",
                                                  "flatViewAction",
                                                  "twoPanelsAction",
                                                  "timestampMenu",
                                                  "toolbarsMenu",
                                                  "openRootAction",
                                                  "upAction",
                                                  "foldersHistoryAction",
                                                  "refreshAction",
                                                  "autoRefreshAction"}));
        QCOMPARE(actionNames(*favorites).first(), QStringLiteral("addFavoriteMenu"));
        QCOMPARE(actionNames(*tools),
                 QStringList({"settingsAction", "|", "benchmarkAction", "|", "tempDirAction"}));
        QCOMPARE(actionNames(*help), QStringList({"helpContentsAction", "|", "aboutAction"}));
        for (const char *name : {"diffAction", "linkAction", "benchmarkAction"}) {
            auto *action = window.findChild<QAction *>(name);
            QVERIFY(action);
            QVERIFY(!action->isEnabled());
            QVERIFY(action->property("nativeCommandID").isValid());
        }
        auto *comment = window.findChild<QAction *>("commentAction");
        QVERIFY(comment);
        QCOMPARE(comment->property("nativeCommandID").toUInt(), unsigned(IDM_COMMENT));
        QCOMPARE(comment->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_Z));
        auto *streams = window.findChild<QAction *>("altStreamsAction");
        QVERIFY(streams);
#ifdef Q_OS_WIN
        QVERIFY(streams->isEnabled()); // Source allows opening the current folder's ADS.
#else
        QVERIFY(!streams->isEnabled());
#endif
        QVERIFY(streams->property("nativeCommandID").isValid());
        QVERIFY(window.findChild<QAction *>("helpContentsAction")->isEnabled());
        QVERIFY(window.findChild<QAction *>("aboutAction")->isEnabled());
        QVERIFY(window.findChild<QAction *>("tempDirAction")->isEnabled());
        QCOMPARE(window.findChild<QAction *>("openInsideOneAction")
                     ->property("nativeCommandID")
                     .toUInt(),
                 unsigned(IDM_OPEN_INSIDE_ONE));
        QCOMPARE(window.findChild<QAction *>("exitAction")->property("nativeCommandID").toUInt(),
                 8u);
        QCOMPARE(window.findChild<QAction *>("openInsideAction")->shortcut(),
                 QKeySequence(Qt::CTRL | Qt::Key_PageDown));
        QCOMPARE(window.findChild<QAction *>("viewFileAction")->shortcut(),
                 QKeySequence(Qt::Key_F3));
        QCOMPARE(window.findChild<QAction *>("editFileAction")->shortcut(),
                 QKeySequence(Qt::Key_F4));
        QCOMPARE(window.findChild<QAction *>("twoPanelsAction")->shortcut(),
                 QKeySequence(Qt::Key_F9));
    }

    void toolbarUsesUpstreamBitmapsAndFitsBothSizes() {
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        QVERIFY(toolbar);
        const char *names[] = {"Add", "Extract", "Test", "Copy", "Move", "Delete", "Info"};
        const char *actions[] = {"toolbarAddAction", "extractArchiveAction", "testArchiveAction",
                                 "copyAction",       "moveAction",           "deleteAction",
                                 "archiveInfoAction"};
        for (unsigned i = 0; i < 7; ++i) {
            const QPixmap source(QStringLiteral(":/upstream/7zip/UI/FileManager/%1%2.bmp")
                                     .arg(QString::fromLatin1(names[i]), QStringLiteral("2")));
            QVERIFY(!source.isNull());
            auto *action = window.findChild<QAction *>(actions[i]);
            QVERIFY(action);
            // Check resource pixels independently of the screen's scale.
            // QIcon's default overload requests a native high-DPI pixmap.
            const QPixmap actual = action->icon().pixmap(QSize(24, 24), 1.0);
            QVERIFY(!actual.isNull());
            QCOMPARE(actual.size(), QSize(24, 24));
            QCOMPARE(actual.toImage().pixelColor(0, 0).alpha(), 0);
            QVERIFY(action->icon().availableSizes().contains(QSize(24, 24)));
            QVERIFY(action->icon().availableSizes().contains(QSize(48, 36)));
        }
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            window.show();
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*toolbar, window));
            QVERIFY(UiInspection::writeReport(
                window, QStringLiteral("main-shell-%1x%2").arg(size.width()).arg(size.height())));
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
        QCOMPARE(window.findChild<QAction *>("toolbarAddAction")
                     ->icon()
                     .pixmap(QSize(48, 36), 1.0)
                     .size(),
                 QSize(48, 36));
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
        QCOMPARE(window.findChild<QMenu *>("fileMenu")->actions().at(8), rename);
        QTest::keyClick(rightTable, Qt::Key_Tab);
        QTRY_VERIFY(leftTable->hasFocus());
        QVERIFY(!rename->isEnabled());
    }
};

QTEST_MAIN(MainShellTests)
#include "main_shell_tests.moc"
