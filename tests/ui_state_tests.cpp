#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QtTest>

class ControlledBackend : public ArchiveBackend {
  public:
    ControlledBackend() : ArchiveBackend(nullptr, "/missing/7zz") {}
    bool busy() const override { return busy_; }
    void list(const QString &archive) override {
        ++listCalls;
        lastArchive = archive;
        busy_ = true;
    }
    void extract(const QString &, const QString &, const QStringList &) override {
        ++extractCalls;
        busy_ = true;
    }
    void create(const QString &, const QStringList &) override {
        ++createCalls;
        busy_ = true;
    }
    void cancel() override {
        ++cancelCalls;
        busy_ = false;
        emit finished(false, QStringLiteral("Операция отменена."));
    }
    void completeList(const QList<ArchiveEntry> &entries) {
        busy_ = false;
        emit entriesReady(entries);
        emit finished(true, QStringLiteral("Архив открыт."));
    }
    int listCalls = 0;
    int extractCalls = 0;
    int createCalls = 0;
    int cancelCalls = 0;
    QString lastArchive;

  private:
    bool busy_ = false;
};

class MainWindowStateTests : public QObject {
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
    static void dismissWarnings() {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (auto *box = qobject_cast<QMessageBox *>(widget))
                box->accept();
        }
    }

  private slots:
    void reopeningAndEmptyArchive() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());

        window.openArchive("first.7z");
        QCOMPARE(backend.listCalls, 1);
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        window.openArchive("ignored.zip");
        QCOMPARE(backend.listCalls, 1);
        backend.completeList({ArchiveEntry{QStringLiteral("имя с пробелом.txt"), 4, 4}});
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        table(window)->selectRow(0);
        QVERIFY(action(window, "extractSelectedAction")->isEnabled());

        window.openArchive("second.zip");
        QCOMPARE(backend.listCalls, 2);
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        backend.completeList({});
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Архив открыт."));
    }

    void launchFailure() {
        ArchiveBackend backend(nullptr, "/definitely/missing/7zz");
        MainWindow window(&backend);
        QTimer timer;
        connect(&timer, &QTimer::timeout, &window, [] { dismissWarnings(); });
        timer.start(10);
        window.openArchive("missing.7z");
        QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->currentMessage().contains("7zz"), 3000);
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!backend.busy());
    }

    void cancellation() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("cancel.7z");
        auto *progress = window.findChildren<QProgressDialog *>().last();
        QVERIFY(progress);
        progress->show();
        auto *button = progress->findChild<QPushButton *>();
        QVERIFY(button);
        button->click();
        QCOMPARE(backend.cancelCalls, 1);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Операция отменена."));
        window.openArchive("after-cancel.zip");
        QCOMPARE(backend.listCalls, 2);
        backend.completeList({});
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
    }

    void operationCancellationKeepsArchive() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("opened.7z");
        backend.completeList({ArchiveEntry{QStringLiteral("файл.txt"), 4, 4}});
        QCoreApplication::processEvents();
        window.extractTo("/temporary/destination");
        QCOMPARE(backend.extractCalls, 1);
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        window.openArchive("ignored.zip");
        window.extractTo("/temporary/destination");
        QCOMPARE(backend.listCalls, 1);
        QCOMPARE(backend.extractCalls, 1);
        auto *progress = window.findChildren<QProgressDialog *>().last();
        QVERIFY(progress);
        progress->show();
        auto *button = progress->findChild<QPushButton *>();
        QVERIFY(button);
        button->click();
        QCOMPARE(backend.cancelCalls, 1);
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Операция отменена."));
    }
};

QTEST_MAIN(MainWindowStateTests)
#include "ui_state_tests.moc"
