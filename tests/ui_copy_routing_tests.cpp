#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "copydialog.h"
#include "layout_inspection.h"
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
    static QString inspectAndCancel(MainWindow &window, QTableWidget &view, bool move, bool menu,
                                    const QSize &size, const QString &tag) {
        bool seen = false;
        bool inspected = false;
        QString summary;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (!dialog)
                return;
            seen = true;
            dialog->resize(size);
            dialog->layout()->activate();
            summary = dialog->findChild<QPlainTextEdit *>("copySelectionInfo")
                          ->toPlainText()
                          .normalized(QString::NormalizationForm_C);
            inspected = UiInspection::writeReport(*dialog, tag);
            dialog->reject();
        });
        if (menu)
            action(window, move ? "moveAction" : "copyAction")->trigger();
        else
            QTest::keyClick(&view, move ? Qt::Key_F6 : Qt::Key_F5);
        QTest::qVerify(seen, "seen", "Copy dialog opened", __FILE__, __LINE__);
        QTest::qVerify(inspected, "inspected", "Widget JSON written", __FILE__, __LINE__);
        return summary;
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
    static void inspectCompletedPanels(MainWindow &window, QTableWidget &source,
                                       QTableWidget &target, const QString &tag) {
        for (const QSize size : {QSize(1100, 640), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(source, window));
            QVERIFY(UiInspection::fullyVisible(target, window));
            QVERIFY(UiInspection::writeReport(window, tag + '-' + QString::number(size.width())));
        }
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void sourceReloadPublishesNamespace() {
        ArchiveBackend backend;
        MainWindow window(&backend);
        action(window, "openRootAction")->trigger();
        auto *view = static_cast<FileTable *>(table(window));
        QVERIFY(view->sourcePanel->IsFSDrivesFolder());
        const int count = view->rowCount();
        QVERIFY(count > 0);
        view->setRowCount(0);
        view->sourcePanel->OnReload();
        QCOMPARE(view->rowCount(), count);
        QCOMPARE(
            view->item(0, 0)->text().normalized(QString::NormalizationForm_C),
            ToQString(view->sourcePanel->GetItemName(0)).normalized(QString::NormalizationForm_C));
    }
    void outsidePeerStillRefreshesDestination_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<bool>("large");
        QTest::addColumn<QString>("outcome");
        for (unsigned sourceIndex : {0u, 1u})
            for (bool large : {false, true})
                for (const char *outcome : {"copy", "move", "dialog-cancel", "overwrite-cancel"})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(sourceIndex).arg(large).arg(outcome)))
                        << sourceIndex << large << QString::fromLatin1(outcome);
    }
    void outsidePeerStillRefreshesDestination() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(bool, large);
        QFETCH(QString, outcome);
        const bool cancel = outcome == "dialog-cancel";
        const bool overwrite = outcome == "overwrite-cancel";
        const bool move = outcome == "move";
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("input");
        const QString peer = fixture.filePath("peer");
        const QString output = fixture.filePath("output");
        for (const QString &path : {input, peer, output})
            QVERIFY(QDir().mkpath(path));
        const QString name = QStringLiteral("payload δ.txt");
        write(QDir(input).filePath(name), "control bytes");
        write(QDir(peer).filePath("seed.txt"), "seed control");
        if (overwrite)
            write(QDir(output).filePath(name), "existing control");
        const int originalRows = large ? 301 : 1;
        for (int i = 1; i < originalRows; ++i)
            write(QDir(peer).filePath(QString("item-%1").arg(i)), "control");
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *sourceView = openPanel(window, sourceIndex, input);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        auto *peerView = openPanel(window, 1 - sourceIndex, peer);
        QTRY_COMPARE(peerView->rowCount(), originalRows);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        for (auto *view : {sourceView, peerView})
            static_cast<FileTable *>(view)->sourcePanel->Set_AutoRefresh_Mode(false);
        peerView->selectRow(row(*peerView, "seed.txt"));
        sourceView->selectRow(row(*sourceView, name));
        sourceView->setFocus();
        QTRY_VERIFY(sourceView->hasFocus());
        bool seen = false;
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            if (dialog && dialog->isVisible() && !seen) {
                seen = true;
                write(QDir(peer).filePath("new while guarded.txt"), "new control");
                dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(output + '/');
                cancel ? dialog->reject() : dialog->accept();
            }
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = dynamic_cast<COverwriteDialog *>(widget); box && box->isVisible())
                    box->findChild<QPushButton *>("overwriteCancel")->click();
        });
        driver.start(1);
        QTest::keyClick(sourceView, move ? Qt::Key_F6 : Qt::Key_F5);
        QVERIFY(seen);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_COMPARE_WITH_TIMEOUT(peerView->rowCount(), originalRows + (cancel ? 0 : 1), 1000);
        QCOMPARE(QFileInfo::exists(QDir(output).filePath(name)), !cancel);
        QCOMPARE(read(QDir(output).filePath(name)), cancel      ? QByteArray()
                                                    : overwrite ? QByteArray("existing control")
                                                                : QByteArray("control bytes"));
        QCOMPARE(read(QDir(input).filePath(name)),
                 move ? QByteArray() : QByteArray("control bytes"));
        QCOMPARE(peerView->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(
            peerView->item(peerView->selectionModel()->selectedRows().first().row(), 0)->text(),
            QString("seed.txt"));
        inspectCompletedPanels(
            window, *sourceView, *peerView,
            QString("copy-outside-peer-%1-%2-%3").arg(sourceIndex).arg(large).arg(outcome));
    }
    void unselectedItemsReachSourceSmartCopy_data() {
        QTest::addColumn<bool>("archive");
        QTest::addColumn<bool>("move");
        QTest::addColumn<bool>("menu");
        QTest::addColumn<QString>("language");
        QTest::addColumn<QSize>("size");
        for (bool archive : {false, true})
            for (bool move : {false, true})
                for (bool menu : {false, true})
                    for (const char *language : {"-", "ru"})
                        for (const QSize size : {QSize(440, 250), QSize(360, 200)})
                            QTest::newRow(qPrintable(QString("%1-%2-%3-%4-%5")
                                                         .arg(archive)
                                                         .arg(move)
                                                         .arg(menu)
                                                         .arg(language)
                                                         .arg(size.width())))
                                << archive << move << menu << QString::fromLatin1(language) << size;
    }
    void unselectedItemsReachSourceSmartCopy() {
        QFETCH(bool, archive);
        QFETCH(bool, move);
        QFETCH(bool, menu);
        QFETCH(QString, language);
        QFETCH(QSize, size);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("input");
        QVERIFY(QDir().mkpath(input));
        const QString first = QStringLiteral("первый δ.txt");
        const QString second = QStringLiteral("второй 文件.txt");
        const QByteArray bytes("unchanged\0control", 17);
        for (const QString &name : {first, second})
            write(QDir(input).filePath(name), bytes);
        const QString source = archive ? fixture.filePath("source archive.zip") : input;
        if (archive)
            createArchive(source, {QDir(input).filePath(first), QDir(input).filePath(second)});
        const QByteArray archiveBefore = archive ? read(source) : QByteArray();
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTableWidget *view = openPanel(window, 0, source);
        QTRY_COMPARE(static_cast<FileTable *>(view)->sourcePanel->GetParentFolders().Size(),
                     archive ? 1u : 0u);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_COMPARE(view->rowCount(), 2);
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        view->setCurrentCell(0, 0);
        view->clearSelection();
        static_cast<FileTable *>(view)->sourcePanel->KillSelection();
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        QVERIFY(view->selectionModel()->selectedRows().isEmpty());
        QVERIFY(action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "moveAction")->isEnabled());
        const QString summary = inspectAndCancel(window, *view, move, menu, size,
                                                 QString("copy-smart-%1-%2-%3-%4-%5")
                                                     .arg(archive)
                                                     .arg(move)
                                                     .arg(menu)
                                                     .arg(language)
                                                     .arg(size.width()));
        QVERIFY(summary.contains(first));
        QVERIFY(summary.contains(second));
        for (const QString &name : {first, second})
            QCOMPARE(read(QDir(input).filePath(name)), bytes);
        if (archive)
            QCOMPARE(read(source), archiveBefore);
        QCOMPARE(QDir(fixture.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot),
                 QStringList{"input"});
    }
    void capturedItemsSurviveModalSelectionChange_data() {
        QTest::addColumn<bool>("sourceArchive");
        QTest::addColumn<bool>("targetArchive");
        QTest::addColumn<bool>("move");
        QTest::addColumn<QString>("language");
        QTest::addColumn<unsigned>("sourceIndex");
        for (bool sourceArchive : {false, true})
            for (bool targetArchive : {false, true})
                for (bool move : {false, true})
                    for (const char *language : {"-", "ru"})
                        for (unsigned sourceIndex : {0u, 1u})
                            QTest::newRow(qPrintable(QString("%1-to-%2-%3-%4-%5")
                                                         .arg(sourceArchive ? "archive" : "fs")
                                                         .arg(targetArchive ? "archive" : "fs")
                                                         .arg(move ? "move" : "copy")
                                                         .arg(language)
                                                         .arg(sourceIndex)))
                                << sourceArchive << targetArchive << move
                                << QString::fromLatin1(language) << sourceIndex;
    }
    void capturedItemsSurviveModalSelectionChange() {
        QFETCH(bool, sourceArchive);
        QFETCH(bool, targetArchive);
        QFETCH(bool, move);
        QFETCH(QString, language);
        QFETCH(unsigned, sourceIndex);
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
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
        else
            write(QDir(output).filePath("seed.txt"), "seed control");
        const QByteArray sourceBefore = sourceArchive ? read(source) : QByteArray();
        ArchiveBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        QTableWidget *sourceView = openPanel(window, sourceIndex, source);
        QTRY_COMPARE(static_cast<FileTable *>(sourceView)->sourcePanel->GetParentFolders().Size(),
                     sourceArchive ? 1u : 0u);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_COMPARE(sourceView->rowCount(), 2);
        QTableWidget *targetView = openPanel(window, 1 - sourceIndex, target);
        QTRY_COMPARE(static_cast<FileTable *>(targetView)->sourcePanel->GetParentFolders().Size(),
                     targetArchive ? 1u : 0u);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QTRY_COMPARE(targetView->rowCount(), 1);
        targetView->selectRow(row(*targetView, "seed.txt"));
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
        QTRY_VERIFY_WITH_TIMEOUT(sourceView->selectionModel()->selectedRows().isEmpty(), 1000);
        // The offscreen plugin loses its active window on modal close. Restore
        // activation only; the source continuation must have remembered focus.
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTRY_VERIFY_WITH_TIMEOUT(sourceView->hasFocus(), 1000);
        QCOMPARE(QDir::cleanPath(QDir::fromNativeSeparators(window.windowTitle())),
                 QDir::cleanPath(QDir::fromNativeSeparators(source)));
        QVERIFY(sourceView->currentItem());
        QCOMPARE(sourceView->currentItem()->text().normalized(QString::NormalizationForm_C),
                 second);
        QCOMPARE(targetView->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(
            targetView->item(targetView->selectionModel()->selectedRows().first().row(), 0)->text(),
            QString("seed.txt"));
        QCOMPARE(read(QDir(output).filePath("seed.txt")), QByteArray("seed control"));
        inspectCompletedPanels(window, *sourceView, *targetView,
                               QString("copy-completed-%1-%2-%3-%4-%5")
                                   .arg(sourceArchive)
                                   .arg(targetArchive)
                                   .arg(move)
                                   .arg(language)
                                   .arg(sourceIndex));
    }
};

QTEST_MAIN(UiCopyRoutingTests)
#include "ui_copy_routing_tests.moc"
