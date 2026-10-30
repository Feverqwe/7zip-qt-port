#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "filemanager/support/window_state_support.h"
#include "filemanager/views/filetable.h"
#include "filemanager/window/mainwindow.h"
#include "localization/LangUtils.h"
#include "settings/RegistryUtils.h"
#include "ui/progress/ProgressDialog2.h"

#include <QAction>
#include <QFile>
#include <QLineEdit>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class ItemOpenBackend final : public ControlledBackend {
  public:
    void list(const QString &, const QString & = {}) override {
        QTimer::singleShot(0, this, [this] {
            ArchiveInfo info;
            info.format = QStringLiteral("7z");
            emit infoReady(info);
            emit entriesReady({ArchiveEntry{"folder/имя с пробелом.txt", 6, 6, {}, false, false}});
            emit finished(true, "Архив открыт.");
        });
    }
    void extractSourceEntry(const QString &, const QString &entry, const QString &output,
                            const QString &, const QString &, bool = false) override {
        extracted = {entry};
        temporary = QFileInfo(output).absolutePath();
        if (cancelExtraction) {
            QTimer::singleShot(0, this, [this] { cancel(); });
            return;
        }
        QFile file(output);
        if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            QTimer::singleShot(0, this, [this] { fail("Fixture stream creation failed."); });
            return;
        }
        file.write("hello\n");
        file.close();
        QTimer::singleShot(0, this, [this] {
            if (failExtraction)
                fail("Controlled extraction failure.");
            else
                completeExtraction();
        });
    }
    QStringList extracted;
    QString temporary;
    bool cancelExtraction = false;
    bool failExtraction = false;
};

class ArchiveItemOpenTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() {
        // These transport/error checks do not depend on installed desktop handlers.
        SaveConfirmUnknownFileOpen(false, qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
    void openFileFromFilesystemAndArchive() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString diskFile = dir.filePath(QStringLiteral("обычный файл.txt"));
        QFile disk(diskFile);
        QVERIFY(disk.open(QIODevice::WriteOnly));
        disk.write("disk");
        disk.close();
        QUrl opened;
        QString extractedDirectory;
        {
            ItemOpenBackend backend;
            MainWindow window(&backend, [&](const QUrl &url) {
                opened = url;
                return true;
            });
            window.show();
            auto *address = window.findChild<QLineEdit *>("archivePath");
            auto *table = window.findChild<FileTable *>("archiveTable");
            auto *open = window.findChild<QAction *>("openItemAction");
            QVERIFY(address && table && open);
            address->setText(dir.path());
            emit address->returnPressed();
            QTRY_COMPARE(table->rowCount(), 1);
            table->selectRow(0);
            QTRY_VERIFY(open->isEnabled());
            open->trigger();
            QCOMPARE(QFileInfo(opened.toLocalFile()).canonicalFilePath(),
                     QFileInfo(diskFile).canonicalFilePath());
            opened = QUrl();
            QVERIFY(backend.prepareArchiveFixture(dir.filePath("test.7z")));
            window.openArchive(dir.filePath("test.7z"));
            QTRY_COMPARE(table->rowCount(), 1);
            emit table->doubleClicked(table->model()->index(0, 0));
            QTRY_COMPARE(table->model()->index(0, 0).data().toString(),
                         QStringLiteral("имя с пробелом.txt"));
            table->selectRow(0);
            QTRY_VERIFY(open->isEnabled());
            open->trigger();
            QTRY_VERIFY(!opened.isEmpty());
            QCOMPARE(backend.extracted.size(), 1);
            QCOMPARE(QDir::fromNativeSeparators(backend.extracted.front()),
                     QStringLiteral("folder/имя с пробелом.txt"));
            // Native CopyTo owns the final file separately from transport scratch storage.
            extractedDirectory = QFileInfo(opened.toLocalFile()).absolutePath();
            QVERIFY(QFileInfo::exists(opened.toLocalFile()));
            QVERIFY(QFileInfo::exists(extractedDirectory));
        }
        QVERIFY(!QFileInfo::exists(extractedDirectory));
    }
    void cancelledArchiveOpenDoesNotLaunchFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ItemOpenBackend backend;
        backend.cancelExtraction = true;
        int launches = 0;
        MainWindow window(&backend, [&](const QUrl &) {
            ++launches;
            return true;
        });
        QVERIFY(backend.prepareArchiveFixture(dir.filePath("test.7z")));
        window.openArchive(dir.filePath("test.7z"));
        auto *table = window.findChild<FileTable *>("archiveTable");
        auto *open = window.findChild<QAction *>("openItemAction");
        QVERIFY(table && open);
        QTRY_COMPARE(table->rowCount(), 1);
        emit table->doubleClicked(table->model()->index(0, 0));
        QTRY_COMPARE(table->model()->index(0, 0).data().toString(),
                     QStringLiteral("имя с пробелом.txt"));
        table->selectRow(0);
        QTRY_VERIFY(open->isEnabled());
        open->trigger();
        QTRY_VERIFY(!backend.temporary.isEmpty());
        QTRY_VERIFY(!QFileInfo::exists(backend.temporary));
        QCOMPARE(launches, 0);
    }
    void failedArchiveOpenRemovesPartialOutputWithoutLaunching() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ItemOpenBackend backend;
        backend.failExtraction = true;
        int launches = 0;
        MainWindow window(&backend, [&](const QUrl &) {
            ++launches;
            return true;
        });
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *box = qobject_cast<QMessageBox *>(modal))
                box->accept();
            else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text() == ToQString(LangString(IDS_CLOSE)))
                    QTimer::singleShot(0, progress, [progress, close] {
                        if (QApplication::activeModalWidget() == progress)
                            close->click();
                    });
            }
        });
        responder.start(1);
        const QString archive = dir.filePath("failure.7z");
        QVERIFY(backend.prepareArchiveFixture(archive));
        window.openArchive(archive);
        auto *table = window.findChild<FileTable *>("archiveTable");
        auto *open = window.findChild<QAction *>("openItemAction");
        QVERIFY(table && open);
        QTRY_COMPARE(table->rowCount(), 1);
        emit table->doubleClicked(table->model()->index(0, 0));
        QTRY_COMPARE(table->model()->index(0, 0).data().toString(),
                     QStringLiteral("имя с пробелом.txt"));
        table->selectRow(0);
        QTRY_VERIFY(open->isEnabled());
        open->trigger();
        QTRY_VERIFY(!backend.temporary.isEmpty());
        QTRY_VERIFY(!QFileInfo::exists(backend.temporary));
        QCOMPARE(launches, 0);
        QVERIFY(QFileInfo::exists(archive));
        QCOMPARE(QFileInfo(archive).size(), qint64(0));
    }
};

QTEST_MAIN(ArchiveItemOpenTests)
#include "archive_item_open_tests.moc"
