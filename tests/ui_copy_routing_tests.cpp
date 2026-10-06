#include "copydialog.h"
#include "window_state_support.h"
#include <QProcess>
#include <QStandardPaths>

class UiCopyRoutingTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static QByteArray read(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return file.readAll();
    }
    static void write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
    }
    static void createArchive(const QString &path, const QStringList &files) {
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(path, files);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
    }
    static int row(QTableWidget &view, const QString &name) {
        for (int i = 0; i < view.rowCount(); ++i)
            if (view.item(i, 0)->text().normalized(QString::NormalizationForm_C) ==
                name.normalized(QString::NormalizationForm_C))
                return i;
        return -1;
    }
    static QTableWidget *openPanel(MainWindow &window, unsigned index, const QString &path) {
        auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(index));
        auto *address = panel->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        return panel->findChild<QTableWidget *>("archiveTable");
    }
    static void submit(MainWindow &window, QTableWidget &sourceView, int secondRow, bool move,
                       bool &dialogSeen, QString &errors) {
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (dialog && dialog->isVisible() && !dialogSeen) {
                dialogSeen = true;
                sourceView.selectRow(secondRow);
                sourceView.setCurrentCell(secondRow, 0);
                dialog->accept();
            }
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    if (box->icon() == QMessageBox::Question)
                        box->button(QMessageBox::Yes)->click();
                    else {
                        errors += box->text();
                        box->accept();
                    }
                }
        });
        driver.start(1);
        QTest::keyClick(&sourceView, move ? Qt::Key_F6 : Qt::Key_F5);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void capturedItemsSurviveModalSelectionChange_data() {
        QTest::addColumn<bool>("sourceArchive");
        QTest::addColumn<bool>("targetArchive");
        QTest::addColumn<bool>("move");
        for (bool sourceArchive : {false, true})
            for (bool targetArchive : {false, true})
                for (bool move : {false, true})
                    QTest::newRow(qPrintable(QString("%1-to-%2-%3")
                                                 .arg(sourceArchive ? "archive" : "fs")
                                                 .arg(targetArchive ? "archive" : "fs")
                                                 .arg(move ? "move" : "copy")))
                        << sourceArchive << targetArchive << move;
    }
    void capturedItemsSurviveModalSelectionChange() {
        QFETCH(bool, sourceArchive);
        QFETCH(bool, targetArchive);
        QFETCH(bool, move);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("input");
        const QString output = fixture.filePath("output");
        QVERIFY(QDir().mkpath(input));
        QVERIFY(QDir().mkpath(output));
        const QString first = QStringLiteral("первый δ.txt");
        const QString second = QStringLiteral("второй 文件.txt");
        const QByteArray firstBytes("first\0control", 13);
        const QByteArray secondBytes("second control", 14);
        write(QDir(input).filePath(first), firstBytes);
        write(QDir(input).filePath(second), secondBytes);
        const QString source = sourceArchive ? fixture.filePath("source archive.zip") : input;
        const QString target = targetArchive ? fixture.filePath("target archive.zip") : output;
        if (sourceArchive)
            createArchive(source, {QDir(input).filePath(first), QDir(input).filePath(second)});
        const QString seed = fixture.filePath("seed.txt");
        write(seed, "seed control");
        if (targetArchive)
            createArchive(target, {seed});
        const QByteArray sourceBefore = sourceArchive ? read(source) : QByteArray();
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        QTableWidget *sourceView = openPanel(window, 0, source);
        QTRY_COMPARE(static_cast<FileTable *>(sourceView)->sourcePanel->GetParentFolders().Size(),
                     sourceArchive ? 1u : 0u);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_COMPARE(sourceView->rowCount(), 2);
        QTableWidget *targetView = openPanel(window, 1, target);
        QTRY_COMPARE(static_cast<FileTable *>(targetView)->sourcePanel->GetParentFolders().Size(),
                     targetArchive ? 1u : 0u);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_COMPARE(targetView->rowCount(), targetArchive ? 1 : 0);
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        const int firstRow = row(*sourceView, first);
        const int secondRow = row(*sourceView, second);
        QVERIFY(firstRow >= 0 && secondRow >= 0);
        sourceView->selectRow(firstRow);
        sourceView->setCurrentCell(firstRow, 0);
        sourceView->setFocus();
        QTRY_VERIFY(sourceView->hasFocus());
        bool dialogSeen = false;
        QString errors;
        submit(window, *sourceView, secondRow, move, dialogSeen, errors);
        QVERIFY(dialogSeen);
        QVERIFY2(errors.isEmpty(), qPrintable(errors));
        if (targetArchive) {
            const QString engine = QStandardPaths::findExecutable("7zz");
            QVERIFY(!engine.isEmpty());
            QProcess verifier;
            verifier.start(engine, {"x", "-y", "-o" + output, "--", target});
            QVERIFY(verifier.waitForFinished(10000));
            QCOMPARE(verifier.exitCode(), 0);
            QCOMPARE(read(QDir(output).filePath("seed.txt")), QByteArray("seed control"));
        }
        QCOMPARE(read(QDir(output).filePath(first)), firstBytes);
        QVERIFY(!QFileInfo::exists(QDir(output).filePath(second)));
        QTRY_VERIFY(row(*targetView, first) >= 0);
        QVERIFY(row(*targetView, second) < 0);
        QCOMPARE(read(QDir(input).filePath(second)), secondBytes);
        if (sourceArchive) {
            if (!move)
                QCOMPARE(read(source), sourceBefore);
            QTRY_COMPARE(sourceView->rowCount(), move ? 1 : 2);
            QVERIFY(row(*sourceView, second) >= 0);
            QCOMPARE(row(*sourceView, first) >= 0, !move);
        } else
            QCOMPARE(QFileInfo::exists(QDir(input).filePath(first)), !move);
    }
};

QTEST_MAIN(UiCopyRoutingTests)
#include "ui_copy_routing_tests.moc"
