#include "filemanager/support/window_state_support.h"
#include "localization/LangUtils.h"
#include "settings/RegistryUtils.h"

#include <QScrollBar>

namespace {
struct CurrentDirectoryGuard {
    const QString previous = QDir::currentPath();
    ~CurrentDirectoryGuard() { QDir::setCurrent(previous); }
};

bool writeFile(const QString &path, const QByteArray &data = "data") {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
} // namespace

class PanelAutoRefreshTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void init() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.remove(QStringLiteral("view/autoRefresh"));
        SaveRegLang(UString("-"), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }

    void detectsChangesAndPreservesSelectionAndScroll() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(temp.path()));
        for (int i = 0; i < 80; ++i)
            QVERIFY(
                writeFile(temp.filePath(QStringLiteral("file-%1.txt").arg(i, 3, 10, QChar('0')))));

        MainWindow window;
        window.resize(640, 400);
        window.show();
        auto *files = table(window);
        QCOMPARE(files->rowCount(), 80);
        const int selectedRow = filesystemRow(window, QStringLiteral("file-050.txt"));
        QVERIFY(selectedRow >= 0);
        files->setCurrentCell(selectedRow, 0);
        files->selectRow(selectedRow);
        const int topRow = filesystemRow(window, QStringLiteral("file-035.txt"));
        files->scrollTo(files->model()->index(topRow, 0), QAbstractItemView::PositionAtTop);
        const QString topPath = files->model()->index(files->rowAt(0), 1).data().toString();
        QVERIFY(files->verticalScrollBar()->value() > 0);

        QVERIFY(writeFile(temp.filePath(QStringLiteral("новый файл.txt"))));
        QTRY_COMPARE_WITH_TIMEOUT(files->rowCount(), 81, 3500);
        QVERIFY(files->model()->index(files->currentRow(), 0).isValid());
        QCOMPARE(files->model()->index(files->currentRow(), 0).data().toString(),
                 QStringLiteral("file-050.txt"));
        QVERIFY(files->selectionModel()->isRowSelected(files->currentRow(), QModelIndex()));
        QVERIFY(files->model()->index(files->rowAt(0), 1).isValid());
        QCOMPARE(files->model()->index(files->rowAt(0), 1).data().toString(), topPath);

        // Changing an existing file leaves the directory entry set unchanged.
        // Windows must observe this through the original native folder watcher.
        const QByteArray changed("changed contents");
        QVERIFY(writeFile(temp.filePath(QStringLiteral("file-050.txt")), changed));
        QTRY_COMPARE_WITH_TIMEOUT(
            files->model()->index(files->currentRow(), 2).data(Qt::UserRole + 2).toLongLong(),
            qint64(changed.size()), 3500);
        QCOMPARE(files->model()->index(files->currentRow(), 0).data().toString(),
                 QStringLiteral("file-050.txt"));
        QVERIFY(files->selectionModel()->isRowSelected(files->currentRow(), QModelIndex()));

        QVERIFY(QFile::remove(temp.filePath(QStringLiteral("новый файл.txt"))));
        QTRY_COMPARE_WITH_TIMEOUT(files->rowCount(), 80, 3500);
    }

    void togglePersistsAndGatesTimer() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(temp.path()));
        QVERIFY(writeFile(temp.filePath(QStringLiteral("start.txt"))));
        {
            MainWindow window;
            window.show();
            auto *toggle = action(window, "autoRefreshAction");
            QVERIFY(toggle->isChecked());
            toggle->trigger();
            QVERIFY(!toggle->isChecked());
            QVERIFY(writeFile(temp.filePath(QStringLiteral("later.txt"))));
            QTest::qWait(1250);
            QCOMPARE(table(window)->rowCount(), 1);
            action(window, "refreshAction")->trigger();
            QCOMPARE(table(window)->rowCount(), 2);
        }
        MainWindow reopened;
        QVERIFY(!action(reopened, "autoRefreshAction")->isChecked());
        action(reopened, "autoRefreshAction")->trigger();
        QVERIFY(action(reopened, "autoRefreshAction")->isChecked());
    }

    void missingFolderClearsStaleRowsAndRecovers() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString folder = temp.filePath(QStringLiteral("browse"));
        QVERIFY(QDir().mkpath(folder));
        QVERIFY(writeFile(folder + QStringLiteral("/old.txt")));
        CurrentDirectoryGuard directoryGuard;
        QVERIFY(QDir::setCurrent(folder));
        MainWindow window;
        window.show();
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(QDir::setCurrent(temp.path()));
        QVERIFY(QFile::remove(folder + QStringLiteral("/old.txt")));
        bool removed = false;
        QTRY_VERIFY_WITH_TIMEOUT(removed || (removed = QDir().rmdir(folder)), 2000);
        QTRY_COMPARE_WITH_TIMEOUT(table(window)->rowCount(), 0, 3500);
        QVERIFY(window.statusBar()->currentMessage().contains(QStringLiteral("Folder not found")));

        QVERIFY(QDir().mkpath(folder));
        QVERIFY(writeFile(folder + QStringLiteral("/new.txt")));
        QTRY_COMPARE_WITH_TIMEOUT(table(window)->rowCount(), 1, 3500);
        QCOMPARE(table(window)->model()->index(0, 0).data().toString(), QStringLiteral("new.txt"));
    }
};

QTEST_MAIN(PanelAutoRefreshTests)
#include "panel_auto_refresh_tests.moc"
