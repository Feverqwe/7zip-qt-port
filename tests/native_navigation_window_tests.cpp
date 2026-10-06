#include "mainwindow.h"
#include <QAction>
#include <QFile>
#include <QLineEdit>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

class NativeNavigationWindowTests : public QObject {
    Q_OBJECT
  private slots:
    void addressRefreshAndParentFocus() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString name = QStringLiteral("папка space/внутри.txt");
        QVERIFY(QDir().mkpath(dir.filePath(QStringLiteral("папка space"))));
        QFile payload(dir.filePath(name));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QCOMPARE(payload.write("before"), 6);
        payload.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        const QString archive = dir.filePath("address.zip");
        backend.create(archive, {payload.fileName()});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *refresh = window.findChild<QAction *>("refreshAction");
        auto *parent = window.findChild<QAction *>("parentFolderAction");
        QVERIFY(table && address && refresh && parent);
        QCOMPARE(table->rowCount(), 1);
        // Native binding clamps a missing suffix to the deepest existing folder.
        address->setText(archive + QStringLiteral(" / missing/deeper"));
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(address->text(), archive);
        table->selectRow(0);
        QVERIFY(payload.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(payload.write("after refresh with a different size"), 35);
        payload.close();
        QProcess update;
        update.setWorkingDirectory(QFileInfo(payload.fileName()).absolutePath());
        const QString executable = QStandardPaths::findExecutable("7zz");
        QVERIFY(!executable.isEmpty());
        update.start(executable,
                     {"u", "-y", "--", archive, QFileInfo(payload.fileName()).fileName()});
        QVERIFY(update.waitForFinished(30000));
        QCOMPARE(update.exitCode(), 0);
        refresh->trigger();
        QCOMPARE(table->rowCount(), 1);
        // OnReload loads the bound proxy; physical changes require reopening.
        QCOMPARE(table->item(0, 2)->text(), QStringLiteral("6"));
        QCOMPARE(table->selectionModel()->selectedRows().size(), 1);
        parent->trigger();
        QCOMPARE(address->text(), QFileInfo(dir.path()).canonicalFilePath());
        QCOMPARE(table->item(table->currentRow(), 0)->text(), QStringLiteral("address.zip"));
        window.openArchive(archive);
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 2)->text(), QStringLiteral("35"));
    }
};
QTEST_MAIN(NativeNavigationWindowTests)
#include "native_navigation_window_tests.moc"
