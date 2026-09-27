#include "mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QStatusBar>
#include <QSysInfo>
#include <QTableWidget>
#include <QToolBar>
#include <QtTest>

class MainWindowLayoutTests : public QObject {
    Q_OBJECT

  private slots:
    void layoutAtSizes_data() {
        QTest::addColumn<QSize>("windowSize");
        QTest::newRow("default") << QSize(980, 620);
        QTest::newRow("compact") << QSize(720, 480);
        QTest::newRow("small") << QSize(640, 400);
    }

    void layoutAtSizes() {
        QFETCH(QSize, windowSize);

        MainWindow window;
        auto *path = window.findChild<QLabel *>("archivePath");
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
        QVERIFY(path);
        QVERIFY(table);
        QVERIFY(toolbar);

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
        QCOMPARE(table->columnCount(), 5);

        const QRect content = window.centralWidget()->contentsRect();
        QVERIFY2(content.contains(path->geometry()), "Archive path escaped the content area");
        QVERIFY2(content.contains(table->geometry()), "File table escaped the content area");
        QVERIFY2(path->geometry().bottom() < table->geometry().top(),
                 "Archive path overlaps the file table");
        QVERIFY2(table->height() >= 120, "File table is too short to inspect archives");
        QVERIFY2(table->horizontalHeader()->sectionSize(0) > 0, "Name column is hidden");
        QVERIFY2(table->horizontalHeader()->sectionSize(1) > 0, "Path column is hidden");

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
};

QTEST_MAIN(MainWindowLayoutTests)
#include "ui_layout_tests.moc"
