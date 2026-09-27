#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QLineEdit>
#include <QSettings>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class ControlledBackend : public ArchiveBackend {
  public:
    ControlledBackend() : ArchiveBackend(nullptr, "/missing/7zz") {}
    bool busy() const override { return busy_; }
    void list(const QString &, const QString & = {}) override { busy_ = true; }
    void extract(const QString &, const QString &, const QStringList &selected,
                 const QString & = {}, const ExtractionOptions & = {}) override {
        lastSelected = selected;
        busy_ = true;
    }
    void cancel() override {
        busy_ = false;
        emit finished(false, QStringLiteral("Операция отменена."));
    }
    void completeList(const QList<ArchiveEntry> &entries) {
        busy_ = false;
        emit entriesReady(entries);
        emit finished(true, QStringLiteral("Архив открыт."));
    }
    QStringList lastSelected;

  private:
    bool busy_ = false;
};

class FlatViewTests : public QObject {
    Q_OBJECT

    static QAction *action(MainWindow &window, const char *name) {
        auto *result = window.findChild<QAction *>(name);
        Q_ASSERT(result);
        return result;
    }
    static QTableWidget *table(MainWindow &window) {
        auto *result = window.findChild<QTableWidget *>("archiveTable");
        Q_ASSERT(result);
        return result;
    }
    static int rowWithPath(MainWindow &window, const QString &path) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 0)->data(Qt::UserRole).toString() == path)
                return row;
        }
        return -1;
    }
    static int filesystemRow(MainWindow &window, const QString &name) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 0)->text() == name)
                return row;
        }
        return -1;
    }
    static void openRow(MainWindow &window, int row) {
        QVERIFY(row >= 0);
        QVERIFY(QMetaObject::invokeMethod(table(window), "cellDoubleClicked", Qt::DirectConnection,
                                          Q_ARG(int, row), Q_ARG(int, 0)));
    }

  private slots:
    void initTestCase() {
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.remove("view/flatDisk");
        settings.remove("view/flatArchive");
    }

    void flatArchiveViewShowsNestedPathsAndKeepsOperationsExact() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("flat.7z");
        const QList<ArchiveEntry> entries = {
            ArchiveEntry{QStringLiteral("раздел один/common.txt"), 1, 1},
            ArchiveEntry{QStringLiteral("раздел два/common.txt"), 2, 2},
            ArchiveEntry{QStringLiteral("раздел один/deep/leaf.txt"), 3, 3},
            ArchiveEntry{QStringLiteral("пустая"), 0, 0, {}, true}};
        backend.completeList(entries);
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
        QCOMPARE(table(window)->item(first, 0)->text(), QStringLiteral("раздел один/common.txt"));
        QCOMPARE(table(window)->item(second, 0)->text(), QStringLiteral("раздел два/common.txt"));
        QVERIFY(rowWithPath(window, QStringLiteral("раздел один/deep")) >= 0);
        table(window)->clearSelection();
        table(window)->selectRow(first);
        window.extractTo("/temporary/destination", true);
        QCOMPARE(backend.lastSelected, QStringList{QStringLiteral("раздел один/common.txt")});
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
        QCOMPARE(table(window)->item(nestedRow, 1)->text(), QFileInfo(nested).canonicalFilePath());
        table(window)->selectRow(nestedRow);
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QInputDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->setTextValue(QStringLiteral("новое имя.txt"));
                    dialog->accept();
                }
            }
        });
        action(window, "renameAction")->trigger();
        QVERIFY(QFileInfo::exists(QDir(folder).filePath(QStringLiteral("новое имя.txt"))));
        QVERIFY(!QFileInfo::exists(nested));
        QTRY_COMPARE(table(window)->rowCount(), 2);
        window.openArchive(temp.filePath("other.7z"));
        backend.completeList({ArchiveEntry{QStringLiteral("inner/archive.txt"), 1, 1}});
        QVERIFY(!flat->isChecked());
        QCOMPARE(table(window)->rowCount(), 1);
        action(window, "backAction")->trigger();
        QVERIFY(flat->isChecked());
        QTRY_COMPARE(table(window)->rowCount(), 2);
        flat->trigger();
    }

    void flatFilesystemScanCanBeCancelledWhileListIsGrowing() {
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
            flat->trigger();
            cancelled = true;
            cancelTimer.stop();
        });
        cancelTimer.start();
        QTRY_VERIFY(cancelled);
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
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("destination.txt"));
        QTRY_VERIFY(table(window)->isEnabled());
        QCoreApplication::processEvents();
        QCOMPARE(table(window)->rowCount(), 1);
    }

    void flatArchiveViewKeepsFolderNavigationAndRefresh() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("flat-navigation.zip");
        const QList<ArchiveEntry> entries = {
            ArchiveEntry{QStringLiteral("outer/inner/file.txt"), 1, 1},
            ArchiveEntry{QStringLiteral("outer/top.txt"), 2, 2}};
        backend.completeList(entries);
        QAction *flat = action(window, "flatViewAction");
        flat->trigger();
        openRow(window, rowWithPath(window, QStringLiteral("outer")));
        QCOMPARE(table(window)->rowCount(), 3);
        QVERIFY(rowWithPath(window, QStringLiteral("outer/inner/file.txt")) >= 0);
        QCOMPARE(
            table(window)->item(rowWithPath(window, QStringLiteral("outer/top.txt")), 0)->text(),
            QStringLiteral("top.txt"));
        action(window, "refreshAction")->trigger();
        QVERIFY(!flat->isEnabled());
        backend.completeList(entries);
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
