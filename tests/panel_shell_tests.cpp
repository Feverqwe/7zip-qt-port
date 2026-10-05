#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "layout_inspection.h"
#include "window_state_support.h"

#include <QMenuBar>
#include <QScopeGuard>

namespace {
struct CurrentDirectoryGuard {
    const QString path = QDir::currentPath();
    ~CurrentDirectoryGuard() { QDir::setCurrent(path); }
};

QLabel *status(QWidget *panel, const char *name) { return panel->findChild<QLabel *>(name); }
} // namespace

class PanelShellTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void init() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString("-"), settings);
    }

    void perPanelStatusAndSourceContextFollowFocus() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CurrentDirectoryGuard guard;
        const QString left = fixture.filePath("left");
        const QString right = fixture.filePath("right space");
        QVERIFY(QDir().mkpath(left));
        QVERIFY(QDir().mkpath(right));
        QFile leftFile(QDir(left).filePath(QStringLiteral("левый.txt")));
        QVERIFY(leftFile.open(QIODevice::WriteOnly));
        QCOMPARE(leftFile.write("1234567"), 7);
        leftFile.close();
        QFile rightFile(QDir(right).filePath(QStringLiteral("right.txt")));
        QVERIFY(rightFile.open(QIODevice::WriteOnly));
        QCOMPARE(rightFile.write("123456789012"), 12);
        rightFile.close();
        QVERIFY(QDir::setCurrent(left));
        const QString settingsPath = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SavePanelPath(1, ToUString(right), settingsPath);
        QSettings settings(settingsPath, QSettings::IniFormat);
        settings.setValue("FM/ShowSystemMenu", true);
        settings.setValue("FM/ShowRealFileIcons", true);
        settings.sync();
        g_Ext_to_Icon_Map.Clear();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *leftPanel = window.findChild<QWidget *>("filePanel0");
        QVERIFY(leftPanel);
        auto *leftTable = leftPanel->findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(status(leftPanel, "selectedItemsStatus")->text(),
                     QStringLiteral("0 / 1 object(s) selected"));
        QCOMPARE(leftTable->rowCount(), 1);
        QVERIFY(!leftTable->item(0, 0)->icon().isNull());
        QVERIFY(g_Ext_to_Icon_Map._extMap_Normal.Size() > 0);
        leftTable->selectRow(0);
        leftTable->setCurrentCell(0, 0);
        QTRY_COMPARE(status(leftPanel, "selectedItemsStatus")->text(),
                     QStringLiteral("1 / 1 object(s) selected"));
        QCOMPARE(status(leftPanel, "selectedSizeStatus")->text(), QStringLiteral("7"));
        QCOMPARE(status(leftPanel, "focusedSizeStatus")->text(), QStringLiteral("7"));
        QVERIFY(!status(leftPanel, "focusedDateStatus")->text().isEmpty());

        QTimer::singleShot(0, &window, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            const auto close = qScopeGuard([&] { menu->close(); });
            QCOMPARE(menu->objectName(), QStringLiteral("panelContextMenu"));
            QCOMPARE(menu->actions().first()->objectName(), QStringLiteral("systemContextGroup"));
#ifdef Q_OS_WIN
            QVERIFY(menu->actions().first()->isEnabled());
            QVERIFY(menu->actions().first()->menu());
            QVERIFY(!menu->actions().first()->menu()->actions().isEmpty());
#else
            QVERIFY(!menu->actions().first()->isEnabled());
#endif
            QCOMPARE(menu->actions().at(1)->objectName(), QStringLiteral("sevenZipContextGroup"));
            QVERIFY(!menu->actions().at(1)->isEnabled());
            QVERIFY(menu->actions().contains(window.findChild<QAction *>("openItemAction")));
            QVERIFY(menu->actions().contains(window.findChild<QAction *>("renameAction")));
            QVERIFY(menu->actions().indexOf(window.findChild<QAction *>("openItemAction")) <
                    menu->actions().indexOf(window.findChild<QAction *>("renameAction")));
            QVERIFY(menu->actions().indexOf(window.findChild<QAction *>("renameAction")) <
                    menu->actions().indexOf(window.findChild<QAction *>("createFolderAction")));
            QVERIFY(!menu->actions().contains(window.findChild<QAction *>("exitAction")));
            QVERIFY(UiInspection::writeReport(*menu, QStringLiteral("panel-context-disk")));
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(
            leftTable, "customContextMenuRequested", Qt::DirectConnection,
            Q_ARG(QPoint, leftTable->visualItemRect(leftTable->item(0, 0)).center())));
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            QVERIFY(menu->actions().contains(window.findChild<QAction *>("openItemAction")));
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(leftTable, "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(-1, -1))));

        window.findChild<QAction *>("twoPanelsAction")->trigger();
        auto *rightPanel = window.findChild<QWidget *>("filePanel1");
        QVERIFY(rightPanel && rightPanel->isVisible());
        auto *rightTable = rightPanel->findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(status(rightPanel, "selectedItemsStatus")->text(),
                     QStringLiteral("0 / 1 object(s) selected"));
        rightTable->selectRow(0);
        rightTable->setCurrentCell(0, 0);
        rightTable->setFocus();
        QTRY_COMPARE(status(rightPanel, "selectedItemsStatus")->text(),
                     QStringLiteral("1 / 1 object(s) selected"));
        QCOMPARE(status(rightPanel, "selectedSizeStatus")->text(), QStringLiteral("12"));
        QCOMPARE(status(leftPanel, "selectedSizeStatus")->text(), QStringLiteral("7"));
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            for (QWidget *panel : {leftPanel, rightPanel}) {
                auto *bar = panel->findChild<QStatusBar *>("panelStatusBar");
                QVERIFY(bar);
                QVERIFY(UiInspection::fullyVisible(*bar, window));
                for (const char *name : {"selectedItemsStatus", "selectedSizeStatus",
                                         "focusedSizeStatus", "focusedDateStatus"})
                    QVERIFY(UiInspection::fullyVisible(*status(panel, name), window));
            }
            QVERIFY(UiInspection::writeReport(
                window, QStringLiteral("panel-shell-%1x%2").arg(size.width()).arg(size.height())));
        }
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            QVERIFY(UiInspection::writeReport(*menu, QStringLiteral("panel-context-disk-640")));
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(
            rightTable, "customContextMenuRequested", Qt::DirectConnection,
            Q_ARG(QPoint, rightTable->visualItemRect(rightTable->item(0, 0)).center())));
        QCOMPARE(status(leftPanel, "selectedSizeStatus")->text(), QStringLiteral("7"));
    }

    void sourceIconCacheUsesExtensionsAndSplitFamily() {
        CExtToIconMap cache;
        const int text = cache.GetIconIndex(FILE_ATTRIBUTE_ARCHIVE, L"file.txt");
        QCOMPARE(cache.GetIconIndex(FILE_ATTRIBUTE_ARCHIVE, L"another.TXT"), text);
        const int split = cache.GetIconIndex(FILE_ATTRIBUTE_ARCHIVE, L"file.001");
        QCOMPARE(cache.GetIconIndex(FILE_ATTRIBUTE_ARCHIVE, L"file.002"), split);
        const int folder = cache.GetIconIndex(FILE_ATTRIBUTE_DIRECTORY, L"folder");
        QCOMPARE(cache.GetIconIndex(FILE_ATTRIBUTE_DIRECTORY, L"another"), folder);
        QVERIFY(!cache.GetIcon(FILE_ATTRIBUTE_ARCHIVE, QStringLiteral("file.txt")).isNull());
        cache.Clear();
        QVERIFY(!cache.GetIcon(FILE_ATTRIBUTE_DIRECTORY, QStringLiteral("folder")).isNull());
    }

    void archiveContextKeepsSourceActionsAndArchiveWorkflows() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        ControlledBackend backend;
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.setValue("FM/ShowRealFileIcons", true);
        settings.sync();
        g_Ext_to_Icon_Map.Clear();
        MainWindow window(&backend);
        window.show();
        openListedArchive(window, backend, fixture.filePath("with space.zip"),
                          {ArchiveEntry{QStringLiteral("имя.txt"), 7, 5}});
        auto *files = window.findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(files->rowCount() > 0);
        int fileRow = -1;
        for (int row = 0; row < files->rowCount(); ++row)
            if (files->item(row, 0)->text() == QStringLiteral("имя.txt"))
                fileRow = row;
        QVERIFY(fileRow >= 0);
        QVERIFY(g_Ext_to_Icon_Map._extMap_Normal.Size() > 0);
        files->selectRow(fileRow);
        files->setCurrentCell(fileRow, 0);
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            QVERIFY(menu->actions().contains(window.findChild<QAction *>("openItemAction")));
            QVERIFY(menu->actions().contains(window.findChild<QAction *>("copyAction")));
            QVERIFY(!menu->actions().contains(window.findChild<QAction *>("extractArchiveAction")));
            QVERIFY(!window.findChild<QAction *>("addArchiveAction"));
            QVERIFY(!menu->findChild<QAction *>("systemContextGroup"));
            QVERIFY(!menu->actions().contains(window.findChild<QAction *>("exitAction")));
            QVERIFY(UiInspection::writeReport(*menu, QStringLiteral("panel-context-archive")));
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(
            files, "customContextMenuRequested", Qt::DirectConnection,
            Q_ARG(QPoint, files->visualItemRect(files->item(fileRow, 0)).center())));
        window.resize(640, 400);
        QApplication::processEvents();
        auto *bar = window.findChild<QStatusBar *>("panelStatusBar");
        QVERIFY(bar);
        QVERIFY(UiInspection::fullyVisible(*bar, window));
        QVERIFY(UiInspection::writeReport(window, QStringLiteral("panel-shell-archive-640")));
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            QVERIFY(UiInspection::writeReport(*menu, QStringLiteral("panel-context-archive-640")));
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(
            files, "customContextMenuRequested", Qt::DirectConnection,
            Q_ARG(QPoint, files->visualItemRect(files->item(fileRow, 0)).center())));
    }
};

QTEST_MAIN(PanelShellTests)
#include "panel_shell_tests.moc"
