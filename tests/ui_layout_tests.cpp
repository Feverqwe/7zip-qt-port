#include "layout_inspection.h"
#include "mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QSysInfo>
#include <QTableWidget>
#include <QTemporaryDir>
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
        QCOMPARE(dialog.windowTitle(), QString("Copy"));
        auto *destination = dialog.findChild<QComboBox *>("copyDestination");
        QVERIFY(destination);
        QVERIFY(destination->isEditable());
        QCOMPARE(destination->currentText(), QString("/initial"));
        QCOMPARE(destination->itemText(0), QString("/previous"));
        QVERIFY(dialog.findChild<QPushButton *>("copyBrowseButton"));
        QVERIFY(dialog.findChild<QPlainTextEdit *>("copySelectionInfo")
                    ->toPlainText()
                    .contains(QStringLiteral("имя с пробелом.txt")));
        destination->setCurrentText("/changed");
        QCOMPARE(dialog.destination(), QString("/changed"));
        CopyDialog moveDialog(true, "/initial", {}, {});
        QCOMPARE(moveDialog.windowTitle(), QString("Move"));
    }

    void splitAndCombineCommandsFollowSelection() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        for (const QString &name : {"file.bin", "file.bin.001", "file.bin.002"}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("part");
        }
        const QString previous = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temp.path()));
        MainWindow window;
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *split = window.findChild<QAction *>("splitFileAction");
        auto *combine = window.findChild<QAction *>("combineFilesAction");
        QVERIFY(table && split && combine);
        QVERIFY(!split->isEnabled());
        QVERIFY(!combine->isEnabled());
        for (int row = 0; row < table->rowCount(); ++row) {
            if (table->item(row, 0)->text() == "file.bin.001") {
                table->selectRow(row);
                break;
            }
        }
        QVERIFY(split->isEnabled());
        QVERIFY(combine->isEnabled());
        table->clearSelection();
        for (int row = 0; row < table->rowCount(); ++row) {
            if (table->item(row, 0)->text() == "file.bin") {
                table->selectRow(row);
                break;
            }
        }
        QVERIFY(split->isEnabled());
        QVERIFY(!combine->isEnabled());
        QVERIFY(QDir::setCurrent(previous));
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }

    void extractDialogMatches7zfmControls() {
        ExtractDialog dialog("/initial", {"/previous", "/older"});
        QCOMPARE(dialog.objectName(), QString("extractDialog"));
        QCOMPARE(dialog.windowTitle(), QString("Extract"));
        auto *destination = dialog.findChild<QComboBox *>("extractDestination");
        QVERIFY(destination);
        QVERIFY(destination->isEditable());
        QCOMPARE(destination->currentText(), QString("/"));
        QCOMPARE(dialog.findChild<QLineEdit *>("extractPathName")->text(), QString("initial"));
        QCOMPARE(destination->itemText(0), QString("/previous"));
        QVERIFY(dialog.findChild<QPushButton *>("extractBrowseButton"));
        auto *pathMode = dialog.findChild<QComboBox *>("extractPathMode");
        auto *overwriteMode = dialog.findChild<QComboBox *>("extractOverwriteMode");
        QVERIFY(pathMode && overwriteMode);
        QCOMPARE(pathMode->currentText(), QString("Full pathnames"));
        QVERIFY(pathMode->isEnabled());
        QVERIFY(overwriteMode->isEnabled());
        QCOMPARE(pathMode->count(), 3);
        QCOMPARE(overwriteMode->count(), 5);
        pathMode->setCurrentIndex(1);
        overwriteMode->setCurrentIndex(1);
        QCOMPARE(dialog.options().pathMode, ExtractionOptions::PathMode::NoPaths);
        QCOMPARE(dialog.options().overwriteMode,
                 ExtractionOptions::OverwriteMode::OverwriteExisting);
        overwriteMode->setCurrentIndex(3);
        QCOMPARE(dialog.options().overwriteMode, ExtractionOptions::OverwriteMode::RenameExtracted);
        overwriteMode->setCurrentIndex(4);
        QCOMPARE(dialog.options().overwriteMode, ExtractionOptions::OverwriteMode::RenameExisting);
        auto *password = dialog.findChild<QLineEdit *>("extractPassword");
        auto *showPassword = dialog.findChild<QCheckBox *>("extractShowPassword");
        QVERIFY(password && showPassword);
        QCOMPARE(password->echoMode(), QLineEdit::Password);
        password->setText("secret");
        QCOMPARE(dialog.password(), QString("secret"));
        showPassword->setChecked(true);
        QCOMPARE(password->echoMode(), QLineEdit::Normal);
        dialog.findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
        destination->setCurrentText("/changed");
        QCOMPARE(dialog.destination(), QString("/changed"));
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
        QVERIFY(table->columnCount() >
                7); // Source folder property catalog extends the transport columns.
        QVERIFY(table->isColumnHidden(5));
        QVERIFY(table->isColumnHidden(6));

        const QRect content = window.centralWidget()->contentsRect();
        QVERIFY2(content.contains(pathCombo->geometry()), "Archive path escaped the content area");
        const QRect tableInContent(table->mapTo(window.centralWidget(), QPoint(0, 0)),
                                   table->size());
        QVERIFY2(content.contains(tableInContent), "File table escaped the content area");
        QVERIFY2(pathCombo->geometry().bottom() < tableInContent.top(),
                 "Archive path overlaps the file table");
        QVERIFY2(table->height() >= 120, "File table is too short to inspect archives");
        QVERIFY2(table->horizontalHeader()->sectionSize(0) > 0, "Name column is hidden");
        QVERIFY(table->isColumnHidden(1));
        QVERIFY(table->verticalHeader()->isHidden());

        const QString tag = QString::fromLatin1(QTest::currentDataTag());
        QVERIFY(UiInspection::writeReport(window, QSysInfo::productType() + "-" + tag));
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
                if (option->text() == "Size") {
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

    void viewModesShareSelectionAndPersist() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString original = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temporary.path()));
        QFile file(temporary.filePath(QStringLiteral("имя с пробелом.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();

        MainWindow window;
        window.show();
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *list = window.findChild<QListView *>("iconList");
        auto *stack = window.findChild<QStackedWidget *>("fileViewStack");
        QVERIFY(table && list && stack);
        QCOMPARE(stack->currentWidget(), static_cast<QWidget *>(table));
        QCOMPARE(list->model(), table->model());
        QCOMPARE(list->selectionModel(), table->selectionModel());

        for (int mode = 0; mode < 3; ++mode) {
            auto *action =
                window.findChild<QAction *>(QStringLiteral("viewMode%1Action").arg(mode));
            QVERIFY(action);
            action->trigger();
            QCOMPARE(stack->currentWidget(), static_cast<QWidget *>(list));
            QVERIFY(action->isChecked());
            QCOMPARE(list->viewMode(), mode == 2 ? QListView::ListMode : QListView::IconMode);
            QCOMPARE(list->iconSize(), mode == 0 ? QSize(48, 48) : QSize(16, 16));
        }
        QVERIFY(table->rowCount() > 0);
        list->selectionModel()->select(list->model()->index(0, 0),
                                       QItemSelectionModel::ClearAndSelect |
                                           QItemSelectionModel::Rows);
        QCOMPARE(table->selectionModel()->selectedRows().size(), 1);
        list->setFocus();
        const QKeyCombination selectAll = QKeySequence(QKeySequence::SelectAll)[0];
        QTest::keyClick(list, selectAll.key(), selectAll.keyboardModifiers());
        QCOMPARE(table->selectionModel()->selectedRows().size(), table->rowCount());
        window.findChild<QAction *>("viewMode3Action")->trigger();
        QCOMPARE(stack->currentWidget(), static_cast<QWidget *>(table));
        QCOMPARE(table->selectionModel()->selectedRows().size(), table->rowCount());

        window.findChild<QAction *>("viewMode1Action")->trigger();
        MainWindow reopened;
        QCOMPARE(reopened.findChild<QStackedWidget *>("fileViewStack")->currentIndex(), 1);
        QVERIFY(reopened.findChild<QAction *>("viewMode1Action")->isChecked());
        QVERIFY(QDir::setCurrent(original));
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
};

QTEST_MAIN(MainWindowLayoutTests)
#include "ui_layout_tests.moc"
