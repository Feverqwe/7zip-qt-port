#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "filemanager/support/window_state_support.h"
#include "filemanager/views/filetable.h"
#include "filemanager/window/mainwindow.h"
#include "localization/LangUtils.h"
#include "localization/qtlangcontrols.h"
#include "support/widget_dialogs.h"
#include "ui/progress/ProgressDialog2.h"
#include "ui/support/layout_inspection.h"
#include <QScopeGuard>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLineEdit>
#include <QSettings>
#include <QStatusBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class FlatViewTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    WidgetDialogFixture widgetDialogs_;

    static QAction *action(MainWindow &window, const char *name) {
        auto *result = window.findChild<QAction *>(name);
        Q_ASSERT(result);
        return result;
    }
    static FileTable *table(MainWindow &window) {
        auto *result = window.findChild<FileTable *>("archiveTable");
        Q_ASSERT(result);
        return result;
    }
    static int rowWithPath(MainWindow &window, const QString &path) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->model()->index(row, 0).data(Qt::UserRole).toString() == path)
                return row;
        }
        return -1;
    }
    static int filesystemRow(MainWindow &window, const QString &name) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->model()->index(row, 0).data().toString() == name)
                return row;
        }
        return -1;
    }
    static void openRow(MainWindow &window, int row) {
        QVERIFY(row >= 0);
        QVERIFY(
            QMetaObject::invokeMethod(table(window), "doubleClicked", Qt::DirectConnection,
                                      Q_ARG(QModelIndex, table(window)->model()->index(row, 0))));
    }

  private slots:
    void savedDiskFlatViewRestoresDescendants() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(QDir(fixture.path()).mkdir(QStringLiteral("nested папка")));
        const QString path = fixture.filePath(QStringLiteral("nested папка/inside δ.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("retained flat view"), qint64(18));
        file.close();
        for (bool reopen : {false, true}) {
            ControlledBackend backend;
            MainWindow window(&backend);
            auto *address = window.findChild<QLineEdit *>("archivePath");
            address->setText(fixture.path());
            QTest::keyClick(address, Qt::Key_Return);
            auto *flat = action(window, "flatViewAction");
            QCOMPARE(flat->isChecked(), reopen);
            if (!reopen)
                flat->trigger();
            const auto descendantVisible = [&] {
                auto *view = table(window);
                for (int row = 0; row < view->rowCount(); ++row)
                    if (view->model()->index(row, 1).data().toString() == path)
                        return true;
                return false;
            };
            QTRY_VERIFY(descendantVisible());
            if (reopen) {
                flat->trigger();
                QTRY_VERIFY(!descendantVisible());
            }
        }
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("retained flat view"));
    }

    void drivesToggleCarriesSourceFlatModeIntoFilesystem_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<QString>("language");
        for (unsigned pane : {0u, 1u})
            for (const QString language : {QString("-"), QString("ru")})
                QTest::newRow(qPrintable(QString("%1-%2").arg(pane).arg(language)))
                    << pane << language;
    }
    void drivesToggleCarriesSourceFlatModeIntoFilesystem() {
        QFETCH(unsigned, pane);
        QFETCH(QString, language);
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(QDir(fixture.path()).mkdir(QStringLiteral("nested папка")));
        QFile file(fixture.filePath(QStringLiteral("nested папка/protected δ.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("flat state control"), qint64(18));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        if (!action(window, "twoPanelsAction")->isChecked())
            action(window, "twoPanelsAction")->trigger();
        auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *address = panel->findChild<QLineEdit *>("archivePath");
        auto *list = static_cast<FileTable *>(panel->findChild<FileTable *>("archiveTable"));
        address->setFocus();
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY(list->rowCount() > 0);
        action(window, "openRootAction")->trigger();
        QVERIFY(list->sourcePanel->IsFSDrivesFolder());
        auto *flat = action(window, "flatViewAction");
        QVERIFY(flat->isEnabled());
        QVERIFY(!list->sourcePanel->GetFlatMode());
        flat->trigger();
        QVERIFY(list->sourcePanel->GetFlatMode());
        auto *view = window.findChild<QMenu *>("viewMenu");
        QVERIFY(QMetaObject::invokeMethod(view, "aboutToShow", Qt::DirectConnection));
        QVERIFY(flat->isChecked());
        const auto inspect = [&](const QString &state) {
            for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
                window.resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*address, window));
                QVERIFY(UiInspection::fullyVisible(*list, window));
                QVERIFY(UiInspection::writeReport(window, QString("source-flat-%1-%2-%3")
                                                              .arg(QTest::currentDataTag(), state)
                                                              .arg(size.width())));
            }
        };
        inspect("drives");
        address->setFocus();
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY([&] {
            for (int row = 0; row < list->rowCount(); ++row)
                if (list->model()->index(row, 0).data().toString().contains("protected"))
                    return true;
            return false;
        }());
        QVERIFY(list->sourcePanel->GetFlatMode());
        inspect("filesystem-flat");
        flat->trigger();
        QTRY_VERIFY(!list->sourcePanel->GetFlatMode());
        QTRY_COMPARE(list->rowCount(), 1);
        inspect("filesystem-normal");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("flat state control"));
    }

    void flatSelectionUsesNamesInsteadOfParentPaths() {
        ControlledBackend backend;
        MainWindow window(&backend);
        openListedArchive(window, backend, "selection.zip",
                          {ArchiveEntry{"parent.txt/README", 1, 1},
                           ArchiveEntry{QStringLiteral("другая папка/README"), 2, 2},
                           ArchiveEntry{"parent.txt/note.txt", 3, 3},
                           ArchiveEntry{"parent.txt/child", 0, 0, {}, true}});
        auto *flat = action(window, "flatViewAction");
        if (!flat->isChecked())
            flat->trigger();
        auto *list = table(window);
        list->setCurrentCell(rowWithPath(window, "parent.txt/README"), 0);
        action(window, "selectTypeAction")->trigger();
        QCOMPARE(list->selectionModel()->selectedRows().size(), 2);
        QVERIFY(
            list->selectionModel()->isRowSelected(rowWithPath(window, "parent.txt/README"), {}));
        QVERIFY(list->selectionModel()->isRowSelected(
            rowWithPath(window, QStringLiteral("другая папка/README")), {}));
        action(window, "deselectAllAction")->trigger();
        widgetDialogs_.respond(window, [](QDialog &active) {
            auto *dialog = dynamic_cast<ComboDialog *>(&active);
            QVERIFY(dialog);
            dialog->findChild<QLineEdit *>()->setText("README");
            dialog->accept();
        });
        action(window, "selectMaskAction")->trigger();
        QCOMPARE(list->selectionModel()->selectedRows().size(), 2);
        QVERIFY(
            !list->selectionModel()->isRowSelected(rowWithPath(window, "parent.txt/note.txt"), {}));
        flat->trigger();
    }

    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void flatArchiveViewShowsNestedPathsAndKeepsOperationsExact() {
        ControlledBackend backend;
        MainWindow window(&backend);
        const QList<ArchiveEntry> entries = {
            ArchiveEntry{QStringLiteral("раздел один/common.txt"), 1, 1},
            ArchiveEntry{QStringLiteral("раздел два/common.txt"), 2, 2},
            ArchiveEntry{QStringLiteral("раздел один/deep/leaf.txt"), 3, 3},
            ArchiveEntry{QStringLiteral("пустая"), 0, 0, {}, true}};
        openListedArchive(window, backend, "flat.7z", entries);
        QCOMPARE(table(window)->rowCount(), 3);
        QAction *flat = action(window, "flatViewAction");
        QVERIFY(flat->isEnabled());
        table(window)->selectRow(rowWithPath(window, QStringLiteral("пустая")));
        flat->trigger();
        QVERIFY(flat->isChecked());
        QCOMPARE(table(window)->rowCount(), 7);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        const int first = rowWithPath(window, QStringLiteral("раздел один/common.txt"));
        const int second = rowWithPath(window, QStringLiteral("раздел два/common.txt"));
        QVERIFY(first >= 0 && second >= 0);
        QCOMPARE(table(window)->model()->index(first, 0).data().toString(),
                 QStringLiteral("раздел один/common.txt"));
        QCOMPARE(table(window)->model()->index(second, 0).data().toString(),
                 QStringLiteral("раздел два/common.txt"));
        QVERIFY(rowWithPath(window, QStringLiteral("раздел один/deep")) >= 0);
        table(window)->clearSelection();
        table(window)->selectRow(first);
        window.extractTo("/temporary/destination", true);
        QStringList selected;
        for (const QString &path : backend.lastSelected)
            selected.append(QDir::fromNativeSeparators(path));
        QCOMPARE(selected, QStringList{QStringLiteral("раздел один/common.txt")});
        backend.cancel();
        flat->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
        QVERIFY(!flat->isChecked());
    }

    void flatFilesystemViewListsNestedFilesAndKeepsModesSeparate() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString folder = temp.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QDir().mkpath(folder));
        const QString nested = QDir(folder).filePath(QStringLiteral("файл.txt"));
        QFile file(nested);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QAction *flat = action(window, "flatViewAction");
        QVERIFY(flat->isEnabled());
        QCOMPARE(table(window)->rowCount(), 1);
        flat->trigger();
        QVERIFY(flat->isChecked());
        QTRY_COMPARE(table(window)->rowCount(), 2);
        QTRY_VERIFY(table(window)->isEnabled());
        const int nestedRow = filesystemRow(window, QStringLiteral("папка с пробелом/файл.txt"));
        QVERIFY(nestedRow >= 0);
        QCOMPARE(table(window)->model()->index(nestedRow, 1).data().toString(),
                 QFileInfo(nested).canonicalFilePath());
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        table(window)->setFocus();
        table(window)->setCurrentCell(nestedRow, 0);
        table(window)->selectRow(nestedRow);
        QCOMPARE(table(window)->currentRow(), nestedRow);
        QString renameFailure;
        QTimer dismissFailure;
        widgetDialogs_.connectResponder(dismissFailure, &window, [&] {
            auto *modal = widgetDialogs_.activeDialog();
            if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                if (box->icon() == QMessageBox::Critical) {
                    renameFailure = box->text();
                    box->accept();
                }
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text() == ToQString(LangString(IDS_CLOSE)))
                    close->click();
            }
        });
        dismissFailure.start(5);
        action(window, "renameAction")->trigger();
        auto *editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(editor);
        QVERIFY(table(window)->isAncestorOf(editor));
        editor->setText(QStringLiteral("новое имя.txt"));
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(QFileInfo::exists(QDir(folder).filePath(QStringLiteral("новое имя.txt"))) ||
                    !renameFailure.isEmpty());
        dismissFailure.stop();
        QVERIFY2(renameFailure.isEmpty(), qPrintable(renameFailure));
        QVERIFY(QFileInfo::exists(QDir(folder).filePath(QStringLiteral("новое имя.txt"))));
        QVERIFY(!QFileInfo::exists(nested));
        QTRY_COMPARE(table(window)->rowCount(), 2);
        openListedArchive(window, backend, temp.filePath("other.7z"),
                          {ArchiveEntry{QStringLiteral("inner/archive.txt"), 1, 1}});
        QVERIFY(!flat->isChecked());
        QCOMPARE(table(window)->rowCount(), 1);
        action(window, "backAction")->trigger();
        QVERIFY(flat->isChecked());
        QTRY_COMPARE(table(window)->rowCount(), 3);
        QVERIFY(filesystemRow(window, QStringLiteral("папка с пробелом/новое имя.txt")) >= 0);
        QVERIFY(filesystemRow(window, "other.7z") >= 0);
        flat->trigger();
    }

    void flatFilesystemScanCanBeCancelledWhileListIsGrowing_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(980, 620);
        QTest::newRow("compact") << QSize(640, 480);
    }
    void flatFilesystemScanCanBeCancelledWhileListIsGrowing() {
        QFETCH(QSize, size);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QTemporaryDir other;
        QVERIFY(other.isValid());
        QFile otherFile(other.filePath("destination.txt"));
        QVERIFY(otherFile.open(QIODevice::WriteOnly));
        otherFile.close();
        const QString nested = temp.filePath("many");
        QVERIFY(QDir().mkpath(nested));
        for (int i = 0; i < 1500; ++i) {
            QFile file(QDir(nested).filePath(QStringLiteral("file-%1.txt").arg(i)));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "viewMode3Action")->trigger();
        action(window, "flatViewAction")->setChecked(false);
        window.resize(size);
        window.show();
        QCoreApplication::processEvents();
        bool nativeReportWritten = false;
        connect(window.statusBar(), &QStatusBar::messageChanged, &window,
                [&](const QString &message) {
                    if (!nativeReportWritten && message.startsWith(QtLangText(IDS_SCANNING)))
                        nativeReportWritten = UiInspection::writeReport(
                            window, QString("filesystem-native-scan-%1").arg(size.width()));
                });
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QAction *flat = action(window, "flatViewAction");
        flat->trigger();
        bool cancelled = false;
        bool partial = false;
        QTimer cancelTimer;
        cancelTimer.setInterval(0);
        connect(&cancelTimer, &QTimer::timeout, &window, [&] {
            if (table(window)->rowCount() <= 0)
                return;
            partial = table(window)->rowCount() < 1501 && !table(window)->isEnabled();
            QVERIFY(UiInspection::fullyVisible(*table(window), window));
            QVERIFY(UiInspection::writeReport(
                window, QString("filesystem-presentation-%1").arg(size.width())));
            flat->trigger();
            cancelled = true;
            cancelTimer.stop();
        });
        cancelTimer.start();
        QTRY_VERIFY(cancelled);
        QVERIFY(nativeReportWritten);
        QVERIFY(partial);
        QVERIFY(!flat->isChecked());
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(table(window)->isEnabled());

        flat->trigger();
        bool navigated = false;
        QTimer navigationTimer;
        navigationTimer.setInterval(0);
        connect(&navigationTimer, &QTimer::timeout, &window, [&] {
            if (table(window)->rowCount() <= 0)
                return;
            address->setText(other.path());
            QTest::keyClick(address, Qt::Key_Return);
            navigated = true;
            navigationTimer.stop();
        });
        navigationTimer.start();
        QTRY_VERIFY(navigated);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QCOMPARE(table(window)->model()->index(0, 0).data().toString(),
                 QStringLiteral("destination.txt"));
        QTRY_VERIFY(table(window)->isEnabled());
        QCoreApplication::processEvents();
        QCOMPARE(table(window)->rowCount(), 1);
        flat->setChecked(false);
    }

    void flatArchiveViewKeepsFolderNavigationAndRefresh() {
        ControlledBackend backend;
        MainWindow window(&backend);
        const QList<ArchiveEntry> entries = {
            ArchiveEntry{QStringLiteral("outer/inner/file.txt"), 1, 1},
            ArchiveEntry{QStringLiteral("outer/top.txt"), 2, 2}};
        openListedArchive(window, backend, "flat-navigation.zip", entries);
        QAction *flat = action(window, "flatViewAction");
        flat->trigger();
        openRow(window, rowWithPath(window, QStringLiteral("outer")));
        QCOMPARE(table(window)->rowCount(), 3);
        QVERIFY(rowWithPath(window, QStringLiteral("outer/inner/file.txt")) >= 0);
        QCOMPARE(table(window)
                     ->model()
                     ->index(rowWithPath(window, QStringLiteral("outer/top.txt")), 0)
                     .data()
                     .toString(),
                 QStringLiteral("top.txt"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList(entries); });
            QVERIFY(!flat->isEnabled());
        });
        action(window, "refreshAction")->trigger();
        QVERIFY(flat->isEnabled());
        QVERIFY(flat->isChecked());
        QCOMPARE(table(window)->rowCount(), 3);
        action(window, "upAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 4);
        flat->trigger();
    }
};

QTEST_MAIN(FlatViewTests)
#include "flat_view_tests.moc"
