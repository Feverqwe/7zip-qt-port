#include "7zip/UI/FileManager/resource.h"
#include "layout_inspection.h"
#include "window_state_support.h"

class RootNavigationTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
#ifndef _WIN32
    void networkNamespaceOpensWithoutRemoteServices() {
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *view = table(window);
        action(window, "openRootAction")->trigger();
        int networkRow = -1;
        for (int row = 0; row < view->rowCount(); ++row)
            if (view->item(row, 0)->data(Qt::UserRole + 6).toUInt() == 2u)
                networkRow = row;
        QVERIFY(networkRow >= 0);
        view->selectRow(networkRow);
        action(window, "openItemAction")->trigger();
        QVERIFY(!address->text().isEmpty());
        QVERIFY(!action(window, "createFolderAction")->isEnabled());
        QVERIFY(action(window, "backAction")->isEnabled());
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            window.show();
            QCoreApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::writeReport(
                window, QSysInfo::productType() +
                            QStringLiteral("-network-%1x%2").arg(size.width()).arg(size.height())));
        }
        action(window, "backAction")->trigger();
        QCOMPARE(view->rowCount(), 3);
    }

    void mountedVolumesOpenThroughNativeRoot() {
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *view = table(window);
        action(window, "openRootAction")->trigger();
        int mountsRow = -1;
        for (int row = 0; row < view->rowCount(); ++row)
            if (view->item(row, 0)->data(Qt::UserRole + 6).toUInt() == 1u)
                mountsRow = row;
        QVERIFY(mountsRow >= 0);
        view->selectRow(mountsRow);
        action(window, "openItemAction")->trigger();
        QVERIFY(window.windowTitle().contains(QStringLiteral("Тома")));
        QVERIFY(view->rowCount() > 0);
        QVERIFY(!action(window, "createFolderAction")->isEnabled());
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            window.show();
            QCoreApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::writeReport(
                window, QSysInfo::productType() +
                            QStringLiteral("-mounts-%1x%2").arg(size.width()).arg(size.height())));
        }
        view->selectRow(0);
        action(window, "openItemAction")->trigger();
        QVERIFY(QFileInfo(address->text()).isDir());
        action(window, "backAction")->trigger();
        QVERIFY(view->rowCount() > 0);
    }
#endif
    void sourceRootRendersAndRoundTripsThroughComputer() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *view = table(window);
        QVERIFY(address && view);
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *root = action(window, "openRootAction");
        QCOMPARE(root->property("nativeCommandID").toUInt(), unsigned(IDM_OPEN_ROOT_FOLDER));
        root->trigger();
#ifdef _WIN32
        QCOMPARE(view->rowCount(), 4);
#else
        QCOMPARE(view->rowCount(), 3);
#endif
        int computerRow = -1;
        for (int row = 0; row < view->rowCount(); ++row)
            if (view->item(row, 0)->data(Qt::UserRole + 6).toUInt() == 0u)
                computerRow = row;
        QVERIFY(computerRow >= 0);
        QVERIFY(!view->item(computerRow, 0)->data(Qt::UserRole + 3).toString().isEmpty());
        QVERIFY(address->text().isEmpty());
        QCOMPARE(window.windowTitle(), QStringLiteral("7-Zip"));
        QVERIFY(!action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "createFileAction")->isEnabled());
        QVERIFY(!action(window, "flatViewAction")->isEnabled());
        action(window, "refreshAction")->trigger();
        QCOMPARE(view->rowCount(),
#ifdef _WIN32
                 4);
#else
                 3);
#endif
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            window.show();
            QCoreApplication::processEvents();
            auto *toolbar = window.findChild<QToolBar *>("archiveToolbar");
            QVERIFY(toolbar);
            QVERIFY(UiInspection::fullyVisible(*toolbar, window));
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::fullyVisible(*window.statusBar(), window));
            QVERIFY(UiInspection::writeReport(
                window, QSysInfo::productType() +
                            QStringLiteral("-root-%1x%2").arg(size.width()).arg(size.height())));
        }
        view->selectRow(computerRow);
        QVERIFY(action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "openOutsideAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        action(window, "openItemAction")->trigger();
#ifndef _WIN32
        QCOMPARE(address->text(), QStringLiteral("/"));
        action(window, "parentFolderAction")->trigger();
        QVERIFY(address->text().isEmpty());
        view->selectRow(computerRow);
        action(window, "openItemAction")->trigger();
        QCOMPARE(address->text(), QStringLiteral("/"));
#else
        QVERIFY(!address->text().isEmpty());
#endif
        action(window, "backAction")->trigger();
        QVERIFY(address->text().isEmpty());
        QVERIFY(view->rowCount() > 0);
        action(window, "backAction")->trigger();
        QCOMPARE(address->text(), QFileInfo(fixture.path()).canonicalFilePath());
    }

    void keyboardRootAndTypedRelativeAddressUseNativeBinding() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(QDir().mkpath(fixture.filePath(QStringLiteral("папка с пробелом"))));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *view = table(window);
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        address->setText(QStringLiteral("папка с пробелом"));
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(
            address->text(),
            QFileInfo(fixture.filePath(QStringLiteral("папка с пробелом"))).canonicalFilePath());
        view->setFocus();
        QTest::keyClick(view, Qt::Key_Backslash);
        QVERIFY(view->rowCount() > 0);
        QVERIFY(address->text().isEmpty());
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(address->text(), QFileInfo(fixture.path()).canonicalFilePath());
    }

    void busyArchiveCannotEnterRootThroughStaleAction() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        const QString archive = fixture.filePath(QStringLiteral("архив space.zip"));
        openListedArchive(window, backend, archive, {ArchiveEntry{"item.txt", 1, 1}});
        auto *root = action(window, "openRootAction");
        QVERIFY(root->isEnabled());
        action(window, "testArchiveAction")->trigger();
        QVERIFY(!root->isEnabled());
        root->setEnabled(true);
        root->trigger();
        QCOMPARE(table(window)->rowCount(), 1);
        QCOMPARE(backend.testCalls, 1);
        backend.cancel();
        root->trigger();
        QVERIFY(table(window)->rowCount() > 0);
        QVERIFY(window.findChild<QLineEdit *>("archivePath")->text().isEmpty());
    }
};

QTEST_MAIN(RootNavigationTests)
#include "root_navigation_tests.moc"
