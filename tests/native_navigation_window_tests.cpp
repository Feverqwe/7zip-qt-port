#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "layout_inspection.h"
#include "mainwindow.h"
#include "window_state_support.h"
#include <QAction>
#include <QFile>
#include <QLineEdit>
#include <QProcess>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

class NativeNavigationWindowTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static void openFilesystemPanel(MainWindow &window, unsigned pane, const QString &path) {
        auto *panel = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        QVERIFY(panel);
        auto *address = panel->findChild<QLineEdit *>("archivePath");
        auto *list = panel->findChild<QTableWidget *>("archiveTable");
        QVERIFY(address && list);
        address->setFocus();
        QTRY_VERIFY(address->hasFocus());
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_VERIFY(list->isEnabled());
        QTRY_COMPARE(list->rowCount(), 1);
        // Finish the source address-Enter focus delivery before starting a
        // different command or explicitly choosing address focus.
        QTRY_VERIFY(
            panel->findChild<QStackedWidget *>("fileViewStack")->currentWidget()->hasFocus());
    }

  private slots:
    void init() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QVERIFY(!settings.isEmpty());
        QFile::remove(settings);
        SaveRegLang(UString("-"), settings);
        ReloadLang(settings);
    }
    void filesystemParentKeepsKeyboardFocus_data() {
        QTest::addColumn<QString>("language");
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("addressFocus");
        for (const QString &lang : {QString("-"), QString("ru")})
            for (unsigned pane : {0u, 1u})
                for (int mode : {0, 1, 2, 3})
                    for (bool address : {false, true})
                        QTest::newRow(qPrintable(QString("%1-%2-%3-%4")
                                                     .arg(lang)
                                                     .arg(pane)
                                                     .arg(mode)
                                                     .arg(address ? "address" : "list")))
                            << lang << pane << mode << address;
    }
    void filesystemParentKeepsKeyboardFocus() {
        QFETCH(QString, language);
        QFETCH(unsigned, pane);
        QFETCH(int, mode);
        QFETCH(bool, addressFocus);
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString childName = QStringLiteral("папка space 密碼");
        const QString child = fixture.filePath(childName);
        QVERIFY(QDir().mkpath(child));
        QFile payload(QDir(child).filePath(QStringLiteral("protected файл.txt")));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        const QByteArray bytes("parent keyboard bytes");
        QCOMPARE(payload.write(bytes), qint64(bytes.size()));
        payload.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        if (!action(window, "twoPanelsAction")->isChecked())
            action(window, "twoPanelsAction")->trigger();
        for (unsigned index : {1 - pane, pane}) {
            openFilesystemPanel(window, index, child);
            QVERIFY(!QTest::currentTestFailed());
        }
        auto *owner = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *peer = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        auto *table = owner->findChild<QTableWidget *>("archiveTable");
        auto *address = owner->findChild<QLineEdit *>("archivePath");
        auto *peerAddress = peer->findChild<QLineEdit *>("archivePath");
        const QString draft = QStringLiteral("peer черновик 密碼");
        peerAddress->setText(draft);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        auto *view = owner->findChild<QStackedWidget *>("fileViewStack")->currentWidget();
        QWidget *const control = addressFocus ? static_cast<QWidget *>(address) : view;
        control->setFocus();
        QTRY_VERIFY(control->hasFocus());
        // Editable QComboBox uses a focus proxy for its line editor. Preserve
        // the actual public focus owner, rather than assuming the child owns it.
        QWidget *const expected = window.focusWidget();
        QVERIFY(expected);
        if (addressFocus)
            action(window, "parentFolderAction")->trigger();
        else
            QTest::keyClick(view, Qt::Key_Backspace);
        QTRY_COMPARE(address->text(), QFileInfo(fixture.path()).canonicalFilePath());
        QTRY_VERIFY(table->isEnabled());
        QTRY_VERIFY(table->currentRow() >= 0);
        QTRY_COMPARE(table->item(table->currentRow(), 0)->text(), childName);
        QTRY_COMPARE(window.focusWidget(), expected);
        QCOMPARE(peerAddress->text(), draft);
        QCOMPARE(peer->findChild<QTableWidget *>("archiveTable")->rowCount(), 1);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QTRY_COMPARE(window.focusWidget(), expected);
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("parent-focus-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        QVERIFY(payload.open(QIODevice::ReadOnly));
        QCOMPARE(payload.readAll(), bytes);
        QCOMPARE(backend.extractCalls, 0);
    }

    void deferredParentRespectsNewerInteraction_data() {
        QTest::addColumn<int>("interaction");
        QTest::newRow("restore-list") << 0;
        QTest::newRow("address-draft") << 1;
        QTest::newRow("peer-focus") << 2;
    }
    void deferredParentRespectsNewerInteraction() {
        QFETCH(int, interaction);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString child = fixture.filePath(QStringLiteral("child папка"));
        QVERIFY(QDir().mkpath(child));
        QFile payload(QDir(child).filePath("protected.txt"));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QCOMPARE(payload.write("protected"), qint64(9));
        payload.close();
        // Cross the existing filesystem publication threshold with one bounded
        // fixture, rather than expanding a large archive/performance matrix.
        for (unsigned index = 0; index < 257; ++index) {
            QFile file(fixture.filePath(QString("entry-%1.txt").arg(index)));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        if (!action(window, "twoPanelsAction")->isChecked())
            action(window, "twoPanelsAction")->trigger();
        for (unsigned pane : {1u, 0u}) {
            openFilesystemPanel(window, pane, child);
            QVERIFY(!QTest::currentTestFailed());
        }
        auto *owner = window.findChild<QWidget *>("filePanel0");
        auto *peer = window.findChild<QWidget *>("filePanel1");
        auto *list = owner->findChild<QTableWidget *>("archiveTable");
        auto *address = owner->findChild<QLineEdit *>("archivePath");
        auto *peerAddress = peer->findChild<QLineEdit *>("archivePath");
        list->setFocus();
        QTRY_VERIFY(list->hasFocus());
        QTest::keyClick(list, Qt::Key_Backspace);
        QVERIFY(!list->isEnabled());
        QWidget *expected = list;
        const QString draft = "unsubmitted parent draft";
        if (interaction) {
            expected = interaction == 1 ? address : peerAddress;
            QTest::mouseClick(expected, Qt::LeftButton);
            QTRY_VERIFY(expected->hasFocus());
            expected = window.focusWidget();
            QVERIFY(expected);
            if (interaction == 1) {
                address->selectAll();
                QTest::keyClicks(address, draft);
            }
        }
        QTRY_VERIFY(list->isEnabled());
        QTRY_COMPARE(list->rowCount(), 258);
        QTRY_COMPARE(window.focusWidget(), expected);
        if (interaction == 1)
            QCOMPARE(address->text(), draft);
        QCOMPARE(peerAddress->text(), QFileInfo(child).canonicalFilePath());
        QCOMPARE(peer->findChild<QTableWidget *>("archiveTable")->rowCount(), 1);
        QVERIFY(payload.open(QIODevice::ReadOnly));
        QCOMPARE(payload.readAll(), QByteArray("protected"));
        QCOMPARE(backend.extractCalls, 0);
    }

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
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "viewMode3Action")->trigger();
        table->setFocus();
        QTRY_VERIFY(table->hasFocus());
        parent->trigger();
        QCOMPARE(address->text(), QFileInfo(dir.path()).canonicalFilePath());
        QCOMPARE(table->item(table->currentRow(), 0)->text(), QStringLiteral("address.zip"));
        QTRY_VERIFY(table->hasFocus());
        window.openArchive(archive);
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(table->item(0, 2)->text(), QStringLiteral("35"));
    }
};
QTEST_MAIN(NativeNavigationWindowTests)
#include "native_navigation_window_tests.moc"
