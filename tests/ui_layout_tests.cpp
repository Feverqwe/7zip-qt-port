#include "mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QStatusBar>
#include <QSysInfo>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QtTest>

class MainWindowLayoutTests : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void copyDialogMatches7zfmControls() {
        CopyDialog dialog(false, "/initial", {"/tmp/имя с пробелом.txt"}, {"/previous", "/older"});
        QCOMPARE(dialog.objectName(), QString("copyDialog"));
        QCOMPARE(dialog.windowTitle(), QString("Копировать"));
        auto *destination = dialog.findChild<QComboBox *>("copyDestination");
        QVERIFY(destination);
        QVERIFY(destination->isEditable());
        QCOMPARE(destination->currentText(), QString("/initial"));
        QCOMPARE(destination->itemText(0), QString("/previous"));
        QVERIFY(dialog.findChild<QPushButton *>("copyBrowseButton"));
        QVERIFY(dialog.findChild<QLabel *>("copySelectionInfo")
                    ->text()
                    .contains(QStringLiteral("имя с пробелом.txt")));
        destination->setCurrentText("/changed");
        QCOMPARE(dialog.destination(), QString("/changed"));
        CopyDialog moveDialog(true, "/initial", {}, {});
        QCOMPARE(moveDialog.windowTitle(), QString("Переместить"));
    }

    void layoutAtSizes_data() {
        QTest::addColumn<QSize>("windowSize");
        QTest::newRow("default") << QSize(980, 620);
        QTest::newRow("compact") << QSize(720, 480);
        QTest::newRow("small") << QSize(640, 400);
    }

    void layoutAtSizes() {
        QFETCH(QSize, windowSize);

        MainWindow window;
        auto *path = window.findChild<QLineEdit *>("archivePath");
        auto *pathCombo = window.findChild<QComboBox *>("pathCombo");
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        QVERIFY(path);
        QVERIFY(pathCombo);
        QVERIFY(pathCombo->isEditable());
        QCOMPARE(pathCombo->lineEdit(), path);
        QVERIFY(table);
        QVERIFY(toolbar);
        QStringList toolbarActions;
        for (QAction *action : toolbar->actions())
            toolbarActions << action->objectName();
        QCOMPARE(toolbarActions,
                 QStringList({"toolbarAddAction", "extractArchiveAction", "testArchiveAction",
                              "toolbarGroupSeparator", "copyAction", "moveAction", "deleteAction",
                              "archiveInfoAction"}));
        QCOMPARE(toolbar->toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
        QVERIFY(!toolbar->actions().first()->icon().isNull());
        int navigationButtons = 0;
        for (QToolButton *button : window.centralWidget()->findChildren<QToolButton *>())
            if (button->defaultAction() == window.findChild<QAction *>("backAction") ||
                button->defaultAction() == window.findChild<QAction *>("upAction"))
                ++navigationButtons;
        QCOMPARE(navigationButtons, 2);

        path->setText(QString(180, QLatin1Char('a')) +
                      QStringLiteral("/имя с пробелом/archive.7z"));
        window.resize(windowSize);
        window.show();
        QCoreApplication::processEvents();

        QCOMPARE(window.size(), windowSize);
        QVERIFY(path->isVisible());
        QVERIFY(table->isVisible());
        QVERIFY(toolbar->isVisible());
        QVERIFY(window.statusBar()->isVisible());
        QCOMPARE(table->columnCount(), 7);
        QVERIFY(table->isColumnHidden(5));
        QVERIFY(table->isColumnHidden(6));

        const QRect content = window.centralWidget()->contentsRect();
        QVERIFY2(content.contains(pathCombo->geometry()), "Archive path escaped the content area");
        QVERIFY2(content.contains(table->geometry()), "File table escaped the content area");
        QVERIFY2(pathCombo->geometry().bottom() < table->geometry().top(),
                 "Archive path overlaps the file table");
        QVERIFY2(table->height() >= 120, "File table is too short to inspect archives");
        QVERIFY2(table->horizontalHeader()->sectionSize(0) > 0, "Name column is hidden");
        QVERIFY(table->isColumnHidden(1));
        QVERIFY(table->verticalHeader()->isHidden());

        const QString screenshotDir = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR",
                                                           QDir::currentPath() + "/ui-screenshots");
        QVERIFY(QDir().mkpath(screenshotDir));
        const QString tag = QString::fromLatin1(QTest::currentDataTag());
        const QString screenshot =
            screenshotDir + "/" + QSysInfo::productType() + "-" + tag + ".png";
        const QImage image = window.grab().toImage();
        QVERIFY2(image.save(screenshot), qPrintable(screenshot));
        QVERIFY(QFileInfo(screenshot).size() > 0);
    }

    void toolbarCustomizationPersists() {
        MainWindow window;
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        auto *archive = window.findChild<QAction *>("archiveToolbarOption");
        auto *standard = window.findChild<QAction *>("standardToolbarOption");
        auto *large = window.findChild<QAction *>("largeButtonsOption");
        auto *text = window.findChild<QAction *>("buttonTextOption");
        QVERIFY(toolbar && archive && standard && large && text);
        window.show();
        archive->setChecked(false);
        QVERIFY(!toolbar->widgetForAction(toolbar->actions().first())->isVisible());
        QVERIFY(toolbar->widgetForAction(window.findChild<QAction *>("copyAction"))->isVisible());
        large->setChecked(true);
        text->setChecked(false);
        QCOMPARE(toolbar->iconSize(), QSize(48, 36));
        QCOMPARE(toolbar->toolButtonStyle(), Qt::ToolButtonIconOnly);
        standard->setChecked(false);
        QVERIFY(!toolbar->isVisible());

        MainWindow reopened;
        QVERIFY(!reopened.findChild<QAction *>("archiveToolbarOption")->isChecked());
        QVERIFY(!reopened.findChild<QAction *>("standardToolbarOption")->isChecked());
        QVERIFY(reopened.findChild<QAction *>("largeButtonsOption")->isChecked());
        QVERIFY(!reopened.findChild<QAction *>("buttonTextOption")->isChecked());
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }

    void columnCustomizationPersists() {
        MainWindow window;
        window.show();
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *header = table->horizontalHeader();
        QVERIFY(!table->isColumnHidden(2));
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            for (QAction *option : menu->actions()) {
                if (option->text() == "Размер") {
                    option->trigger();
                    break;
                }
            }
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(header, "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(30, 5))));
        QVERIFY(table->isColumnHidden(2));
        MainWindow reopened;
        QVERIFY(reopened.findChild<QTableWidget *>("archiveTable")->isColumnHidden(2));
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
};

QTEST_MAIN(MainWindowLayoutTests)
#include "ui_layout_tests.moc"
