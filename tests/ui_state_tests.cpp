#include "mainwindow.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopedPointer>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QSysInfo>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QtTest>

class ControlledBackend : public ArchiveBackend {
  public:
    ControlledBackend() : ArchiveBackend(nullptr, "/missing/7zz") {}
    bool busy() const override { return busy_; }
    void list(const QString &archive, const QString & = {}) override {
        ++listCalls;
        lastArchive = archive;
        busy_ = true;
    }
    void extract(const QString &, const QString &, const QStringList &selected,
                 const QString &password = {}, const ExtractionOptions &options = {}) override {
        ++extractCalls;
        lastSelected = selected;
        lastExtractPassword = password;
        lastExtractionOptions = options;
        busy_ = true;
    }
    void test(const QString &, const QString &password = {}) override {
        ++testCalls;
        lastTestPassword = password;
        busy_ = true;
    }
    void create(const QString &target, const QStringList &sources, const QString & = {},
                bool = false, const CompressionOptions &options = {}) override {
        ++createCalls;
        lastCreateTarget = target;
        lastCreateSources = sources;
        lastCompressionOptions = options;
        busy_ = true;
    }
    void add(const QString &target, const QStringList &sources) override {
        ++addCalls;
        lastAddTarget = target;
        lastAddSources = sources;
        busy_ = true;
    }
    void remove(const QString &, const QStringList &paths) override {
        ++removeCalls;
        lastRemoved = paths;
        busy_ = true;
    }
    void renameEntry(const QString &, const QString &oldPath, const QString &newPath) override {
        ++renameCalls;
        lastRenameOld = oldPath;
        lastRenameNew = newPath;
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
    void fail(const QString &message) {
        busy_ = false;
        emit finished(false, message);
    }
    void completeCreate() {
        busy_ = false;
        emit finished(true, QStringLiteral("Архив создан."));
    }
    void completeAdd() {
        busy_ = false;
        emit finished(true, QStringLiteral("Файлы добавлены."));
    }
    void report(int percent, const QString &detail, const QString &line) {
        emit outputReceived(line);
        emit progress(percent, detail);
    }
    int listCalls = 0;
    int extractCalls = 0;
    int testCalls = 0;
    int createCalls = 0;
    int addCalls = 0;
    int removeCalls = 0;
    int renameCalls = 0;
    int cancelCalls = 0;
    CompressionOptions lastCompressionOptions;
    QString lastArchive;
    QStringList lastSelected;
    QString lastExtractPassword;
    ExtractionOptions lastExtractionOptions;
    QString lastTestPassword;
    QString lastCreateTarget;
    QStringList lastCreateSources;
    QString lastAddTarget;
    QStringList lastAddSources;
    QStringList lastRemoved;
    QString lastRenameOld;
    QString lastRenameNew;

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
    static int rowWithPath(MainWindow &window, const QString &path) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 0)->data(Qt::UserRole).toString() == path)
                return row;
        }
        return -1;
    }
    static void openRow(MainWindow &window, int row) {
        QVERIFY(row >= 0);
        QVERIFY(QMetaObject::invokeMethod(table(window), "cellDoubleClicked", Qt::DirectConnection,
                                          Q_ARG(int, row), Q_ARG(int, 0)));
    }
    static void dismissWarnings() {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (auto *box = qobject_cast<QMessageBox *>(widget))
                box->accept();
        }
    }
    static void pressSelectAll(QTableWidget *view) {
        const QKeyCombination key = QKeySequence(QKeySequence::SelectAll)[0];
        QTest::keyClick(view, key.key(), key.keyboardModifiers());
    }
    static int filesystemRow(MainWindow &window, const QString &name) {
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 0)->text() == name)
                return row;
        }
        return -1;
    }

  private slots:
    void numericNameSortMatchesFileManager() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        for (const QString &name : {"10.txt", "2.txt", "1.txt"}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *view = table(window);
        QCOMPARE(view->item(0, 0)->text(), QStringLiteral("1.txt"));
        QCOMPARE(view->item(1, 0)->text(), QStringLiteral("2.txt"));
        QCOMPARE(view->item(2, 0)->text(), QStringLiteral("10.txt"));

        window.openArchive(temp.filePath("test.7z"));
        backend.completeList({ArchiveEntry{"10.txt", 10, 10}, ArchiveEntry{"2.txt", 2, 2},
                              ArchiveEntry{"1.txt", 1, 1}});
        QCOMPARE(view->item(0, 0)->text(), QStringLiteral("1.txt"));
        QCOMPARE(view->item(1, 0)->text(), QStringLiteral("2.txt"));
        QCOMPARE(view->item(2, 0)->text(), QStringLiteral("10.txt"));
    }

    void viewSortCommandsReorderArchiveAndSurviveRefresh() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive(temp.filePath("sort.7z"));
        const QList<ArchiveEntry> entries = {ArchiveEntry{"z.bin", 30, 1, "2024-01-01"},
                                             ArchiveEntry{"a.txt", 10, 1, "2023-01-01"},
                                             ArchiveEntry{"c.bin", 20, 1, "2022-01-01"}};
        backend.completeList(entries);
        auto names = [&] {
            QStringList result;
            for (int row = 0; row < table(window)->rowCount(); ++row)
                result << table(window)->item(row, 0)->text();
            return result;
        };
        QCOMPARE(names(), (QStringList{"a.txt", "c.bin", "z.bin"}));
        action(window, "sort1Action")->trigger();
        QCOMPARE(names(), (QStringList{"c.bin", "z.bin", "a.txt"}));
        action(window, "sort2Action")->trigger();
        QCOMPARE(names(), (QStringList{"c.bin", "a.txt", "z.bin"}));
        action(window, "sort3Action")->trigger();
        QCOMPARE(names(), (QStringList{"a.txt", "c.bin", "z.bin"}));
        action(window, "sort4Action")->trigger();
        QCOMPARE(names(), (QStringList{"z.bin", "a.txt", "c.bin"}));
        action(window, "refreshAction")->trigger();
        backend.completeList(entries);
        QCOMPARE(names(), (QStringList{"z.bin", "a.txt", "c.bin"}));
        QVERIFY(action(window, "sort4Action")->isChecked());
    }

    void createFileDialogCreatesUnicodeFileAndCancelLeavesDirectoryUntouched() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        const QString name = QStringLiteral("новый файл.txt");
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setEditText(name);
            dialog->accept();
        });
        action(window, "createFileAction")->trigger();
        QVERIFY(QFileInfo::exists(temp.filePath(name)));
        QCOMPARE(filesystemRow(window, name) >= 0, true);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        {
            QFile existing(temp.filePath(name));
            QVERIFY(existing.open(QIODevice::WriteOnly));
            QCOMPARE(existing.write("protected"), 9);
        }
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setEditText("cancelled.txt");
            dialog->reject();
        });
        action(window, "createFileAction")->trigger();
        QVERIFY(!QFileInfo::exists(temp.filePath("cancelled.txt")));
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setEditText(name);
            dialog->accept();
            QTimer::singleShot(0, [] {
                for (QWidget *widget : QApplication::topLevelWidgets()) {
                    if (auto *box = qobject_cast<QMessageBox *>(widget))
                        box->accept();
                }
            });
        });
        action(window, "createFileAction")->trigger();
        QFile existing(temp.filePath(name));
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("protected"));
    }

    void maskAndTypeSelectionFollowFileManager() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        for (const QString &name : {QStringLiteral("Alpha.txt"), QStringLiteral("beta.TXT"),
                                    QStringLiteral("image.png")}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto applyMask = [&](const char *actionName, const QString &mask) {
            QTimer::singleShot(0, [&] {
                auto *dialog = window.findChild<QDialog *>("comboDialog");
                QVERIFY(dialog);
                dialog->findChild<QComboBox *>("comboValue")->setEditText(mask);
                dialog->accept();
            });
            action(window, actionName)->trigger();
        };
        applyMask("selectMaskAction", "*.txt");
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        applyMask("deselectMaskAction", "alpha.*");
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        table(window)->selectionModel()->setCurrentIndex(
            table(window)->model()->index(filesystemRow(window, "image.png"), 0),
            QItemSelectionModel::NoUpdate);
        action(window, "selectTypeAction")->trigger();
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        action(window, "deselectTypeAction")->trigger();
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);

        window.openArchive(temp.filePath("test.7z"));
        backend.completeList({ArchiveEntry{"first.txt", 1, 1}, ArchiveEntry{"second.TXT", 1, 1},
                              ArchiveEntry{"other.bin", 1, 1}});
        applyMask("selectMaskAction", "*.txt");
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        QVERIFY(!action(window, "createFileAction")->isEnabled());
    }

    void foldersHistoryDialogPersistsAndDeletesEntries() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString first = temp.filePath("first folder");
        const QString second = temp.filePath(QStringLiteral("вторая папка"));
        QVERIFY(QDir().mkpath(first));
        QVERIFY(QDir().mkpath(second));
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(first);
        QTest::keyClick(address, Qt::Key_Return);
        address->setText(second);
        QTest::keyClick(address, Qt::Key_Return);
        const QString canonicalFirst = QFileInfo(first).canonicalFilePath();
        const QString canonicalSecond = QFileInfo(second).canonicalFilePath();
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("foldersHistoryDialog");
            QVERIFY(dialog);
            auto *list = dialog->findChild<QListWidget *>("foldersHistoryList");
            QCOMPARE(list->item(0)->text(), canonicalSecond);
            QCOMPARE(list->item(1)->text(), canonicalFirst);
            list->setCurrentRow(0);
            dialog->findChild<QPushButton *>("removeHistoryEntryButton")->click();
            list->setCurrentRow(0);
            dialog->accept();
        });
        action(window, "foldersHistoryAction")->trigger();
        QCOMPARE(address->text(), canonicalFirst);
        MainWindow reopened(&backend);
        auto *combo = reopened.findChild<QComboBox *>("pathCombo");
        QVERIFY(combo->findText(canonicalFirst) >= 0);
        QCOMPARE(combo->findText(canonicalSecond), -1);
    }

    void favoritesFollowFoldersAndArchivePaths() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString first = temp.filePath(QStringLiteral("папка с пробелом"));
        const QString second = temp.filePath("other");
        QVERIFY(QDir().mkpath(first));
        QVERIFY(QDir().mkpath(second));
        const QString archive = temp.filePath(QStringLiteral("архив с пробелом.7z"));
        QFile file(archive);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();

        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *view = table(window);
        address->setText(first);
        QTest::keyClick(address, Qt::Key_Return);
        QTest::keyClick(view, Qt::Key_1, Qt::AltModifier | Qt::ShiftModifier);
        QCOMPARE(action(window, "openBookmark1Action")->isEnabled(), true);
        address->setText(second);
        QTest::keyClick(address, Qt::Key_Return);
        QTest::keyClick(view, Qt::Key_1, Qt::AltModifier);
        QCOMPARE(address->text(), QFileInfo(first).canonicalFilePath());

        window.openArchive(archive);
        QVERIFY(!action(window, "openBookmark1Action")->isEnabled());
        backend.completeList({ArchiveEntry{QStringLiteral("внутри/файл.txt"), 1, 1}});
        openRow(window, rowWithPath(window, QStringLiteral("внутри")));
        action(window, "saveBookmark2Action")->trigger();
        action(window, "openBookmark1Action")->trigger();
        QCOMPARE(address->text(), QFileInfo(first).canonicalFilePath());
        action(window, "openBookmark2Action")->trigger();
        QCOMPARE(backend.listCalls, 2);
        backend.completeList({ArchiveEntry{QStringLiteral("внутри/файл.txt"), 1, 1}});
        QVERIFY(address->text().endsWith(QStringLiteral(" / внутри")));
        QCOMPARE(view->rowCount(), 1);

        MainWindow reopened;
        QVERIFY(action(reopened, "openBookmark1Action")->isEnabled());
        QVERIFY(action(reopened, "openBookmark2Action")->isEnabled());
        QFile::remove(archive);
        action(reopened, "openBookmark2Action")->trigger();
        QCOMPARE(reopened.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
    }

    void mainWindowFolderAndSelectionCommands() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString first = temp.filePath(QStringLiteral("файл с пробелом.txt"));
        QFile file(first);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(table(window)->rowCount(), 1);
        auto *createFolder = action(window, "createFolderAction");
        auto *refresh = action(window, "refreshAction");
        QVERIFY(createFolder->isEnabled());
        QVERIFY(refresh->isEnabled());

        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QInputDialog *>(widget))
                    dialog->reject();
            }
        });
        createFolder->trigger();
        QCOMPARE(table(window)->rowCount(), 1);
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QInputDialog *>(widget)) {
                    dialog->setTextValue(QStringLiteral("папка с пробелом"));
                    dialog->accept();
                }
            }
        });
        createFolder->trigger();
        const QString folder = temp.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QFileInfo(folder).isDir());
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("папка с пробелом"));

        action(window, "invertSelectionAction")->trigger();
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("файл с пробелом.txt"));
        action(window, "deselectAllAction")->trigger();
        QVERIFY(table(window)->selectionModel()->selectedRows().isEmpty());
        table(window)->selectRow(filesystemRow(window, QStringLiteral("файл с пробелом.txt")));
        const QString later = temp.filePath("later.txt");
        QFile newFile(later);
        QVERIFY(newFile.open(QIODevice::WriteOnly));
        newFile.close();
        refresh->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("файл с пробелом.txt"));
        QCOMPARE(backend.listCalls, 0);
    }

    void refreshArchiveKeepsFolderAndSelection() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("example.7z");
        ArchiveEntry first;
        first.path = QStringLiteral("папка с пробелом/первый.txt");
        backend.completeList({first});
        openRow(window, rowWithPath(window, QStringLiteral("папка с пробелом")));
        QCOMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        action(window, "refreshAction")->trigger();
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(!action(window, "refreshAction")->isEnabled());
        QVERIFY(!action(window, "createFolderAction")->isEnabled());
        action(window, "refreshAction")->trigger();
        QCOMPARE(backend.listCalls, 2);
        ArchiveEntry second;
        second.path = QStringLiteral("папка с пробелом/второй.txt");
        backend.completeList({first, second});
        QCOMPARE(table(window)->rowCount(), 2);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("первый.txt"));
        QVERIFY(window.findChild<QLineEdit *>("archivePath")
                    ->text()
                    .endsWith(QStringLiteral(" / папка с пробелом")));
        QVERIFY(action(window, "refreshAction")->isEnabled());
        action(window, "refreshAction")->trigger();
        backend.completeList({});
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!window.findChild<QLineEdit *>("archivePath")
                     ->text()
                     .endsWith(QStringLiteral(" / папка с пробелом")));
        action(window, "refreshAction")->trigger();
        backend.cancel();
        QVERIFY(action(window, "createFolderAction")->isEnabled());
        QVERIFY(action(window, "refreshAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());

        window.openArchive("example.7z");
        backend.completeList({first});
        action(window, "refreshAction")->trigger();
        QTimer::singleShot(0, &window, [] { dismissWarnings(); });
        backend.fail(QStringLiteral("Не удалось перечитать архив."));
        QVERIFY(action(window, "createFolderAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
    }

    void stage7FilesystemCommands() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString source = temp.filePath(QStringLiteral("имя с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("original"), 8);
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        const int row = filesystemRow(window, QStringLiteral("имя с пробелом.txt"));
        QVERIFY(row >= 0);
        table(window)->selectRow(row);
        QVERIFY(action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "moveAction")->isEnabled());
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QVERIFY(action(window, "renameAction")->isEnabled());
        const QScopedPointer<QMimeData> outbound(
            table(window)->model()->mimeData(table(window)->selectionModel()->selectedIndexes()));
        QVERIFY(outbound->hasUrls());
        QCOMPARE(QFileInfo(outbound->urls().first().toLocalFile()).canonicalFilePath(),
                 QFileInfo(source).canonicalFilePath());

        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget))
                    dialog->reject();
            }
        });
        action(window, "moveAction")->trigger();
        QVERIFY(QFileInfo::exists(source));

        const QString destination = temp.filePath("destination");
        QTimer::singleShot(0, &window, [destination] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget)) {
                    dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
                    dialog->accept();
                }
            }
        });
        action(window, "copyAction")->trigger();
        const QString copied = QDir(destination).filePath(QStringLiteral("имя с пробелом.txt"));
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        QCOMPARE(settings.value("copy/history").toStringList().first(), destination);
        QFile copiedFile(copied);
        QVERIFY(copiedFile.open(QIODevice::ReadOnly));
        QCOMPARE(copiedFile.readAll(), QByteArray("original"));
        copiedFile.close();
        QTimer warningTimer;
        connect(&warningTimer, &QTimer::timeout, &window, [destination] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
                    dialog->accept();
                }
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
            }
        });
        warningTimer.start(1);
        action(window, "copyAction")->trigger();
        warningTimer.stop();
        QVERIFY(QFileInfo::exists(source));
        QVERIFY(QFileInfo::exists(copied));
        QTimer moveErrorTimer;
        connect(&moveErrorTimer, &QTimer::timeout, &window, [destination] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
                    dialog->accept();
                }
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
            }
        });
        moveErrorTimer.start(1);
        action(window, "moveAction")->trigger();
        moveErrorTimer.stop();
        QVERIFY(QFileInfo::exists(source));
        QVERIFY(QFileInfo::exists(copied));

        const int copiedRow = filesystemRow(window, QStringLiteral("имя с пробелом.txt"));
        table(window)->selectRow(copiedRow);
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QInputDialog *>(widget)) {
                    dialog->setTextValue(QStringLiteral("новое имя.txt"));
                    dialog->accept();
                }
            }
        });
        action(window, "renameAction")->trigger();
        const QString renamed = temp.filePath(QStringLiteral("новое имя.txt"));
        QVERIFY(QFileInfo::exists(renamed));
        QVERIFY(!QFileInfo::exists(source));

        table(window)->selectRow(filesystemRow(window, QStringLiteral("новое имя.txt")));
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::No)->click();
            }
        });
        action(window, "deleteAction")->trigger();
        QVERIFY(QFileInfo::exists(renamed));
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::Yes)->click();
            }
        });
        action(window, "deleteAction")->trigger();
        QVERIFY(!QFileInfo::exists(renamed));
        QVERIFY(QFileInfo::exists(copied));

        QFile movable(temp.filePath("move me.txt"));
        QVERIFY(movable.open(QIODevice::WriteOnly));
        movable.write("move");
        movable.close();
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        table(window)->selectRow(filesystemRow(window, "move me.txt"));
        QTimer::singleShot(0, &window, [destination] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
                    dialog->accept();
                }
            }
        });
        action(window, "moveAction")->trigger();
        QVERIFY(!QFileInfo::exists(movable.fileName()));
        QVERIFY(QFileInfo::exists(QDir(destination).filePath("move me.txt")));
    }

    void stage7FilesystemDropAndArchiveCopy() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("in"));
        QVERIFY(QDir(temp.path()).mkdir("out"));
        const QString source = temp.filePath(QStringLiteral("drop ü.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("dropped");
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.filePath("in"));
        QTest::keyClick(address, Qt::Key_Return);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(source)});
        QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table(window)->viewport(), &enter);
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table(window)->viewport(), &drop);
        QVERIFY(QFileInfo::exists(temp.filePath(QStringLiteral("in/drop ü.txt"))));

        window.openArchive("items.zip");
        backend.completeList({ArchiveEntry{QStringLiteral("nested/file.txt"), 4, 4}});
        const int row = filesystemRow(window, "nested");
        QVERIFY(row >= 0);
        table(window)->selectRow(row);
        QTimer::singleShot(0, &window, [&temp] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible()) {
                    if (auto *destination = dialog->findChild<QComboBox *>("extractDestination")) {
                        destination->setCurrentText(temp.filePath("out"));
                        dialog->accept();
                    }
                }
            }
        });
        action(window, "copyAction")->trigger();
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(backend.lastSelected, QStringList({QStringLiteral("nested/file.txt")}));
        QVERIFY(!action(window, "copyAction")->isEnabled());
        backend.cancel();
        QVERIFY(action(window, "copyAction")->isEnabled());
        QDragEnterEvent archiveEnter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton,
                                     Qt::NoModifier);
        QCoreApplication::sendEvent(table(window)->viewport(), &archiveEnter);
        QDropEvent archiveDrop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton,
                               Qt::NoModifier);
        QCoreApplication::sendEvent(table(window)->viewport(), &archiveDrop);
        QCOMPARE(backend.addCalls, 1);
        QVERIFY(!action(window, "copyAction")->isEnabled());
        backend.cancel();
    }

    void stage7ArchiveCommandStates() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("first.7z");
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        backend.completeList({ArchiveEntry{QStringLiteral("file.txt"), 4, 4}});
        table(window)->selectRow(0);
        QVERIFY(action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QVERIFY(action(window, "renameAction")->isEnabled());
        window.openArchive("second.zip");
        QVERIFY(!action(window, "copyAction")->isEnabled());
        backend.completeList({});
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        window.openArchive("cancel.7z");
        backend.cancel();
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
    }
    void stage7ArchiveMutations() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("mutate.7z");
        backend.completeList({ArchiveEntry{QStringLiteral("папка/file.txt"), 4, 4}});
        table(window)->selectRow(filesystemRow(window, QStringLiteral("папка")));
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::No)->click();
            }
        });
        action(window, "deleteAction")->trigger();
        QCOMPARE(backend.removeCalls, 0);
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::Yes)->click();
            }
        });
        action(window, "deleteAction")->trigger();
        QCOMPARE(backend.removeCalls, 1);
        QCOMPARE(backend.lastRemoved, QStringList({QStringLiteral("папка/file.txt")}));
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        backend.cancel();
        QVERIFY(action(window, "deleteAction")->isEnabled());
        table(window)->selectRow(filesystemRow(window, QStringLiteral("папка")));
        openRow(window, filesystemRow(window, QStringLiteral("папка")));
        table(window)->selectRow(filesystemRow(window, QStringLiteral("file.txt")));
        QVERIFY(action(window, "renameAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QInputDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->setTextValue(QStringLiteral("новый.txt"));
                    dialog->accept();
                }
            }
        });
        action(window, "renameAction")->trigger();
        QCOMPARE(backend.renameCalls, 1);
        QCOMPARE(backend.lastRenameOld, QStringLiteral("папка/file.txt"));
        QCOMPARE(backend.lastRenameNew, QStringLiteral("папка/новый.txt"));
        backend.completeAdd();
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(!action(window, "renameAction")->isEnabled());
        backend.completeList({ArchiveEntry{QStringLiteral("папка/новый.txt"), 4, 4}});
        QVERIFY(!action(window, "renameAction")->isEnabled());
        openRow(window, filesystemRow(window, QStringLiteral("папка")));
        table(window)->selectRow(filesystemRow(window, QStringLiteral("новый.txt")));
        QVERIFY(action(window, "renameAction")->isEnabled());
    }
    void addToOpenArchiveState() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("новый файл.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("data");
        file.close();
        const QString canonicalSource = QFileInfo(source).canonicalFilePath();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        const QString archive = temp.filePath("existing.zip");
        window.openArchive(archive);
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        QVERIFY(!action(window, "toolbarAddAction")->isEnabled());
        backend.completeList({});
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());

        auto choose = [&] {
            QTimer::singleShot(0, &window, [&] {
                auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
                QVERIFY(dialog);
                QVERIFY(
                    dialog->findChild<QLabel *>("archiveTargetPreview")->text().contains(archive));
                auto *available = dialog->findChild<QListWidget *>("availableSources");
                for (int row = 0; row < available->count(); ++row)
                    if (available->item(row)
                            ->data(Qt::UserRole)
                            .toString()
                            .normalized(QString::NormalizationForm_C) ==
                        canonicalSource.normalized(QString::NormalizationForm_C))
                        available->item(row)->setSelected(true);
                dialog->findChild<QPushButton *>("selectSourcesButton")->click();
                if (dialog->findChild<QListWidget *>("selectedSources")->count() != 1) {
                    dialog->reject();
                    return;
                }
                const QString screenshotDir = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
                if (!screenshotDir.isEmpty()) {
                    QDir().mkpath(screenshotDir);
                    dialog->grab().save(screenshotDir + "/" + QSysInfo::productType() + "-add.png");
                }
                dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            });
            action(window, "toolbarAddAction")->trigger();
        };
        choose();
        QCOMPARE(backend.addCalls, 1);
        QCOMPARE(backend.lastAddTarget, archive);
        QCOMPARE(backend.lastAddSources.first().normalized(QString::NormalizationForm_C),
                 canonicalSource.normalized(QString::NormalizationForm_C));
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!table(window)->isEnabled());
        backend.cancel();
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        choose();
        QCOMPARE(backend.addCalls, 2);
        backend.fail(QStringLiteral("Операция отменена."));
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        choose();
        QCOMPARE(backend.addCalls, 3);
        backend.completeAdd();
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        backend.completeList({{QStringLiteral("новый файл.txt"), 4, 4, {}, false, false}});
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QCOMPARE(table(window)->rowCount(), 1);
        window.openArchive(archive);
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        backend.completeList({});
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
    }
    void createDialogAndStateTransitions() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString file =
            QFileInfo(temp.filePath(QStringLiteral("файл с пробелом.txt"))).absoluteFilePath();
        QFile source(file);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("data");
        source.close();
        const QString folder = temp.filePath(QStringLiteral("другой каталог/папка Unicode"));
        QVERIFY(QDir().mkpath(folder));
        const QString canonicalFile = QFileInfo(file).canonicalFilePath();
        const QString canonicalFolder = QFileInfo(folder).canonicalFilePath();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 1)->text() == canonicalFile)
                table(window)->selectRow(row);
        }

        int initialSelectionCount = -1;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
            QVERIFY(dialog);
            initialSelectionCount = dialog->findChild<QListWidget *>("selectedSources")->count();
            dialog->reject();
        });
        action(window, "toolbarAddAction")->trigger();
        QCOMPARE(backend.createCalls, 0);
        QCOMPARE(initialSelectionCount, 1);

        auto chooseSources = [&] {
            QTimer::singleShot(0, &window, [&] {
                auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
                QVERIFY(dialog);
                auto *available = dialog->findChild<QListWidget *>("availableSources");
                for (int row = 0; row < available->count(); ++row) {
                    const QString path = available->item(row)->data(Qt::UserRole).toString();
                    if (path == canonicalFile)
                        available->item(row)->setSelected(true);
                }
                auto *add = dialog->findChild<QPushButton *>("selectSourcesButton");
                add->click();
                auto *sourceDirectory = dialog->findChild<QLineEdit *>("sourceDirectory");
                sourceDirectory->setText(QFileInfo(canonicalFolder).absolutePath());
                QTest::keyClick(sourceDirectory, Qt::Key_Return);
                available = dialog->findChild<QListWidget *>("availableSources");
                for (int row = 0; row < available->count(); ++row) {
                    if (available->item(row)->data(Qt::UserRole).toString() == canonicalFolder)
                        available->item(row)->setSelected(true);
                }
                add->click();
                if (dialog->findChild<QListWidget *>("selectedSources")->count() != 2) {
                    dialog->reject();
                    return;
                }
                dialog->findChild<QLineEdit *>("archiveName")->setText("new archive");
                dialog->findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
                dialog->findChild<QComboBox *>("compressionLevel")->setCurrentIndex(5);
                dialog->findChild<QComboBox *>("compressionMethod")->setCurrentText("BZip2");
                dialog->findChild<QSpinBox *>("compressionThreads")->setValue(2);
                if (!dialog->findChild<QLabel *>("archiveTargetPreview")
                         ->text()
                         .endsWith("new archive.zip")) {
                    dialog->reject();
                    return;
                }
                const QString screenshotDir = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
                if (!screenshotDir.isEmpty()) {
                    QDir().mkpath(screenshotDir);
                    dialog->grab().save(screenshotDir + "/" + QSysInfo::productType() +
                                        "-create.png");
                }
                dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            });
            action(window, "toolbarAddAction")->trigger();
        };
        chooseSources();
        QCOMPARE(backend.createCalls, 1);
        QCOMPARE(backend.lastCompressionOptions.level, 9);
        QCOMPARE(backend.lastCompressionOptions.method, QStringLiteral("BZip2"));
        QCOMPARE(backend.lastCompressionOptions.threads, 2);
        QCOMPARE(backend.lastCreateTarget,
                 QDir(QFileInfo(temp.path()).canonicalFilePath()).filePath("new archive.zip"));
        QCOMPARE(backend.lastCreateSources.size(), 2);
        QVERIFY(backend.lastCreateSources.contains(canonicalFile));
        QVERIFY(backend.lastCreateSources.contains(canonicalFolder));
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!table(window)->isEnabled());
        backend.cancel();
        QVERIFY(action(window, "createArchiveAction")->isEnabled());

        chooseSources();
        QCOMPARE(backend.createCalls, 2);
        backend.fail(QStringLiteral("Операция отменена."));
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        window.openArchive("empty.7z");
        backend.completeList({});
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        action(window, "backAction")->trigger();
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        chooseSources();
        QCOMPARE(backend.createCalls, 3);
        backend.fail(QStringLiteral("Операция отменена."));
        chooseSources();
        QCOMPARE(backend.createCalls, 4);
        QFile created(backend.lastCreateTarget);
        QVERIFY(created.open(QIODevice::WriteOnly));
        created.close();
        backend.completeCreate();
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        QCOMPARE(table(window)->rowCount(), 3);
        QVERIFY(table(window)->findItems("new archive.zip", Qt::MatchExactly).size() > 0);
    }
    void keyboardNavigationAndCommandState() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("папка с пробелом")));
        QFile archive(temp.filePath(QStringLiteral("архив с пробелом.zip")));
        QVERIFY(archive.open(QIODevice::WriteOnly));
        archive.close();
        QFile plain(temp.filePath("plain.txt"));
        QVERIFY(plain.open(QIODevice::WriteOnly));
        plain.close();
        ControlledBackend backend;
        QUrl opened;
        MainWindow window(&backend, [&](const QUrl &url) {
            opened = url;
            return true;
        });
        window.show();
        auto *view = table(window);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        view->setFocus();
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QCOMPARE(action(window, "openArchiveAction")->shortcut(), QKeySequence::Open);
        QVERIFY(action(window, "selectAllAction")->isEnabled());
        pressSelectAll(view);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 3);

        for (int row = 0; row < view->rowCount(); ++row) {
            if (view->item(row, 0)->text() == "plain.txt")
                view->setCurrentCell(row, 0);
        }
        QVERIFY(action(window, "openItemAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Return);
        QCOMPARE(QFileInfo(opened.toLocalFile()).canonicalFilePath(),
                 QFileInfo(plain.fileName()).canonicalFilePath());
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());
        for (int row = 0; row < view->rowCount(); ++row) {
            if (view->item(row, 0)->text() == QStringLiteral("папка с пробелом"))
                view->setCurrentCell(row, 0);
        }
        QVERIFY(action(window, "openItemAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Return);
        QCOMPARE(address->text(),
                 QFileInfo(temp.filePath(QStringLiteral("папка с пробелом"))).canonicalFilePath());
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());

        for (int row = 0; row < view->rowCount(); ++row) {
            if (view->item(row, 0)->text() == QStringLiteral("архив с пробелом.zip"))
                view->setCurrentCell(row, 0);
        }
        QTest::keyClick(view, Qt::Key_Return);
        QCOMPARE(backend.listCalls, 1);
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        backend.completeList({ArchiveEntry{QStringLiteral("папка/файл.txt"), 1, 1},
                              ArchiveEntry{QStringLiteral("другой.txt"), 2, 2}});
        pressSelectAll(view);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        QCOMPARE(action(window, "extractSelectedAction")->isEnabled(), true);
        view->setCurrentCell(rowWithPath(window, QStringLiteral("папка")), 0);
        QTest::keyClick(view, Qt::Key_Return);
        QVERIFY(address->text().endsWith(QStringLiteral(" / папка")));
        QCOMPARE(view->rowCount(), 1);
        view->setCurrentCell(0, 0);
        QVERIFY(action(window, "openItemAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(view->rowCount(), 2);
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());

        window.openArchive(archive.fileName());
        backend.completeList({});
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());
    }

    void archiveTreeSortSelectionAndEmptyFolders() {
        ControlledBackend backend;
        MainWindow window(&backend);
        const QString longName = QString(150, QLatin1Char('x')) + QStringLiteral(" δ.txt");
        window.openArchive("tree.7z");
        backend.completeList({ArchiveEntry{QStringLiteral("пустая папка"), 0, 0, {}, true},
                              ArchiveEntry{QStringLiteral("папка с пробелом/глубже/10.txt"), 10, 8},
                              ArchiveEntry{QStringLiteral("папка с пробелом/глубже/2.txt"), 2, 2},
                              ArchiveEntry{QStringLiteral("папка с пробелом/") + longName, 20, 5},
                              ArchiveEntry{QStringLiteral("root.txt"), 1, 1}});
        QCOMPARE(table(window)->rowCount(), 3);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "upAction")->isEnabled());
        const int empty = rowWithPath(window, QStringLiteral("пустая папка"));
        const int rootFile = rowWithPath(window, QStringLiteral("root.txt"));
        QVERIFY(empty >= 0);
        QVERIFY(rootFile >= 0);
        table(window)->selectionModel()->select(table(window)->model()->index(empty, 0),
                                                QItemSelectionModel::Select |
                                                    QItemSelectionModel::Rows);
        table(window)->selectionModel()->select(table(window)->model()->index(rootFile, 0),
                                                QItemSelectionModel::Select |
                                                    QItemSelectionModel::Rows);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        window.extractTo("/temporary/destination", true);
        QVERIFY(backend.lastSelected.contains(QStringLiteral("пустая папка")));
        QVERIFY(backend.lastSelected.contains(QStringLiteral("root.txt")));
        backend.cancel();
        const int folder = rowWithPath(window, QStringLiteral("папка с пробелом"));
        QVERIFY(folder >= 0);
        QVERIFY(table(window)->item(folder, 0)->data(Qt::UserRole + 1).toBool());
        openRow(window, folder);
        QCOMPARE(table(window)->rowCount(), 2);
        QVERIFY(action(window, "upAction")->isEnabled());
        QVERIFY(window.findChild<QLineEdit *>("archivePath")
                    ->text()
                    .endsWith(QStringLiteral(" / папка с пробелом")));
        const QString screenshotDir = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
        if (!screenshotDir.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshotDir));
            for (const QSize size : {QSize(980, 620), QSize(720, 480), QSize(640, 400)}) {
                window.resize(size);
                window.show();
                QCoreApplication::processEvents();
                const QString imagePath =
                    screenshotDir + "/" + QSysInfo::productType() +
                    QStringLiteral("-archive-%1x%2.png").arg(size.width()).arg(size.height());
                QVERIFY(window.grab().save(imagePath));
            }
        }
        const int file = rowWithPath(window, QStringLiteral("папка с пробелом/") + longName);
        QVERIFY(file >= 0);
        openRow(window, file);
        QCOMPARE(backend.lastSelected,
                 QStringList({QStringLiteral("папка с пробелом/") + longName}));
        backend.cancel();
        QCOMPARE(table(window)->rowCount(), 2);
        QCOMPARE(rowWithPath(window, QStringLiteral("папка с пробелом/") + longName), file);
        openRow(window, rowWithPath(window, QStringLiteral("папка с пробелом/глубже")));
        QCOMPARE(table(window)->rowCount(), 2);
        table(window)->sortItems(2, Qt::DescendingOrder);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("10.txt"));
        QCOMPARE(table(window)->item(1, 0)->text(), QStringLiteral("2.txt"));
        table(window)->selectRow(1);
        QVERIFY(action(window, "extractSelectedAction")->isEnabled());
        window.extractTo("/temporary/destination", true);
        QCOMPARE(backend.lastSelected,
                 QStringList{QStringLiteral("папка с пробелом/глубже/2.txt")});
        backend.cancel();
        action(window, "upAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 2);
        action(window, "backAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
        openRow(window, rowWithPath(window, QStringLiteral("пустая папка")));
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        action(window, "upAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
    }

    void reopenFailureAndCancellationAfterFolderNavigation() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("one.zip");
        backend.completeList({ArchiveEntry{QStringLiteral("nested/file.txt"), 1, 1}});
        openRow(window, rowWithPath(window, QStringLiteral("nested")));
        window.openArchive("two.7z");
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!action(window, "upAction")->isEnabled());
        backend.cancel();
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        window.openArchive("three.zip");
        backend.completeList({});
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!action(window, "upAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        window.openArchive("failed.7z");
        QTimer::singleShot(0, &window, [] { dismissWarnings(); });
        backend.fail(QStringLiteral("7zz не запустился."));
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        QVERIFY(!action(window, "upAction")->isEnabled());
    }

    void reopeningAndEmptyArchive() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "archiveInfoAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());

        window.openArchive("first.7z");
        QCOMPARE(backend.listCalls, 1);
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        window.openArchive("ignored.zip");
        QCOMPARE(backend.listCalls, 1);
        backend.completeList({ArchiveEntry{QStringLiteral("имя с пробелом.txt"), 4, 4}});
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "archiveInfoAction")->isEnabled());
        QVERIFY(action(window, "selectAllAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        table(window)->selectRow(0);
        QVERIFY(action(window, "extractSelectedAction")->isEnabled());

        window.openArchive("second.zip");
        QCOMPARE(backend.listCalls, 2);
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        backend.completeList({});
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo("second.zip").absoluteFilePath());
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "backAction")->isEnabled());
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "archiveInfoAction")->isEnabled());
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Архив открыт."));
    }

    void testAndInfoStates() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("first.7z");
        emit backend.infoReady(ArchiveInfo{QStringLiteral("7z"), 120, 3, 1, 0});
        backend.completeList({ArchiveEntry{QStringLiteral("δ space.txt"), 3, 7}});
        action(window, "archiveInfoAction")->trigger();
        auto *info = window.findChild<QLabel *>("archiveInfoText");
        QVERIFY(info);
        QVERIFY(info->text().contains(QStringLiteral("Формат: 7z")));
        QVERIFY(info->text().contains(QStringLiteral("Размер архива: 120 байт")));
        action(window, "testArchiveAction")->trigger();
        QCOMPARE(backend.testCalls, 1);
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "archiveInfoAction")->isEnabled());
        window.testArchive();
        QCOMPARE(backend.testCalls, 1);
        backend.cancel();
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        window.openArchive("empty.zip");
        QVERIFY(!info->isVisible());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        backend.completeList({});
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        action(window, "archiveInfoAction")->trigger();
        QLabel *emptyInfo = nullptr;
        for (QDialog *dialog : window.findChildren<QDialog *>("archiveInfoDialog")) {
            if (dialog->isVisible())
                emptyInfo = dialog->findChild<QLabel *>("archiveInfoText");
        }
        QVERIFY(emptyInfo);
        QVERIFY(emptyInfo->text().contains(QStringLiteral("Формат: не указан 7zz")));
        action(window, "testArchiveAction")->trigger();
        QCOMPARE(backend.testCalls, 2);
        backend.fail(QStringLiteral("Операция отменена."));
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
    }

    void extractionDestinationAndCancellation() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("archive.zip");
        backend.completeList({ArchiveEntry{QStringLiteral("папка/δ space.txt"), 3, 3}});
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            auto *destination = dialog->findChild<QComboBox *>("extractDestination");
            QVERIFY(destination);
            destination->setCurrentText("/temporary/selected output");
            dialog->reject();
        });
        action(window, "extractArchiveAction")->trigger();
        QCOMPARE(backend.extractCalls, 0);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            auto *destination = dialog->findChild<QComboBox *>("extractDestination");
            QVERIFY(destination);
            destination->setCurrentText("/temporary/selected output");
            auto *password = dialog->findChild<QLineEdit *>("extractPassword");
            QVERIFY(password);
            password->setText("temporary secret");
            dialog->findChild<QComboBox *>("extractPathMode")->setCurrentIndex(1);
            dialog->findChild<QComboBox *>("extractOverwriteMode")->setCurrentIndex(1);
            dialog->accept();
        });
        action(window, "extractArchiveAction")->trigger();
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(backend.lastExtractPassword, QString("temporary secret"));
        QCOMPARE(backend.lastExtractionOptions.pathMode, ExtractionOptions::PathMode::NoPaths);
        QCOMPARE(backend.lastExtractionOptions.overwriteMode,
                 ExtractionOptions::OverwriteMode::SkipExisting);
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        backend.cancel();
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
    }

    void launchFailure() {
        ArchiveBackend backend(nullptr, "/definitely/missing/7zz");
        MainWindow window(&backend);
        QTimer timer;
        connect(&timer, &QTimer::timeout, &window, [] { dismissWarnings(); });
        timer.start(10);
        window.openArchive("missing.7z");
        QTRY_VERIFY_WITH_TIMEOUT(window.statusBar()->currentMessage().contains("7zz"), 3000);
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        QVERIFY(!action(window, "backAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QCOMPARE(action(window, "selectAllAction")->isEnabled(), table(window)->rowCount() > 0);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "archiveInfoAction")->isEnabled());
        QVERIFY(!backend.busy());
    }

    void filesystemNavigationAndArchiveReturn() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("папка с пробелом")));
        QFile archive(temp.filePath(QStringLiteral("тест архив.zip")));
        QVERIFY(archive.open(QIODevice::WriteOnly));
        archive.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        QVERIFY(address);
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());
        QCOMPARE(table(window)->rowCount(), 2);
        QVERIFY(action(window, "backAction")->isEnabled());
        int folderRow = -1;
        int archiveRow = -1;
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            const QString name = table(window)->item(row, 0)->text();
            if (name == QStringLiteral("папка с пробелом"))
                folderRow = row;
            if (name == QStringLiteral("тест архив.zip"))
                archiveRow = row;
        }
        QVERIFY(folderRow >= 0);
        QVERIFY(archiveRow >= 0);
        QVERIFY(QMetaObject::invokeMethod(table(window), "cellDoubleClicked", Qt::DirectConnection,
                                          Q_ARG(int, folderRow), Q_ARG(int, 0)));
        QCOMPARE(address->text(),
                 QFileInfo(temp.filePath(QStringLiteral("папка с пробелом"))).canonicalFilePath());
        QCOMPARE(table(window)->rowCount(), 0);
        action(window, "backAction")->trigger();
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());
        QVERIFY(QMetaObject::invokeMethod(table(window), "cellDoubleClicked", Qt::DirectConnection,
                                          Q_ARG(int, archiveRow), Q_ARG(int, 0)));
        QCOMPARE(backend.listCalls, 1);
        QCOMPARE(backend.lastArchive, QFileInfo(archive.fileName()).canonicalFilePath());
        QVERIFY(!action(window, "backAction")->isEnabled());
        backend.completeList({});
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "backAction")->isEnabled());
        action(window, "backAction")->trigger();
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());
        QCOMPARE(table(window)->rowCount(), 2);
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
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
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QCOMPARE(action(window, "selectAllAction")->isEnabled(), table(window)->rowCount() > 0);
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Операция отменена."));
        action(window, "operationJournalAction")->trigger();
        auto *log = window.findChild<QPlainTextEdit *>("operationJournalText");
        QVERIFY(log);
        QVERIFY(log->toPlainText().contains(QStringLiteral("Операция отменена.")));
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
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
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
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QVERIFY(action(window, "selectAllAction")->isEnabled());
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Операция отменена."));
    }

    void passwordPromptCancellationAndReopen() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("protected.7z");
        QTimer::singleShot(0, &window, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->reject();
        });
        emit backend.passwordRequired();
        QCOMPARE(backend.cancelCalls, 1);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        window.openArchive("empty.zip");
        QCOMPARE(backend.listCalls, 2);
        backend.completeList({});
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
    }

    void passwordReuseAndResetOnReopen() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("protected.7z");
        const QString password = QStringLiteral("disposable-δ-password");
        QTimer::singleShot(0, &window, [&password] {
            auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->setTextValue(password);
            dialog->accept();
        });
        emit backend.passwordRequired();
        emit backend.infoReady(ArchiveInfo{QStringLiteral("7z"), 120, 3, 1, 0, true});
        backend.completeList({ArchiveEntry{QStringLiteral("δ file.txt"), 3, 3}});
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        window.testArchive();
        QCOMPARE(backend.lastTestPassword, password);
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        backend.fail(QStringLiteral("Операция отменена."));
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        window.openArchive("empty.zip");
        backend.completeList({});
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        window.testArchive();
        QVERIFY(backend.lastTestPassword.isEmpty());
    }

    void createDialogEncryptionChoices() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("δ file.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        CreateArchiveDialog dialog(temp.path(), {source});
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        auto *encryption = dialog.findChild<QComboBox *>("archiveEncryption");
        auto *password = dialog.findChild<QLineEdit *>("archivePassword");
        QVERIFY(format);
        QVERIFY(encryption);
        QVERIFY(password);
        QCOMPARE(password->echoMode(), QLineEdit::Password);
        QCOMPARE(encryption->count(), 3);
        encryption->setCurrentIndex(2);
        password->setText(QStringLiteral("disposable-test-password"));
        QCOMPARE(dialog.password(), QStringLiteral("disposable-test-password"));
        QVERIFY(dialog.encryptHeaders());
        format->setCurrentText("zip");
        QCOMPARE(encryption->count(), 2);
        encryption->setCurrentIndex(1);
        QCOMPARE(dialog.password(), QStringLiteral("disposable-test-password"));
        QVERIFY(!dialog.encryptHeaders());
    }

    void progressAndLastJournal() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *journal = action(window, "operationJournalAction");
        QVERIFY(!journal->isEnabled());
        window.openArchive("first.7z");
        QVERIFY(journal->isEnabled());
        auto *progress = window.findChildren<QProgressDialog *>().last();
        QCOMPARE(progress->windowModality(), Qt::NonModal);
        backend.report(42, QStringLiteral("папка/имя с пробелом.txt"),
                       QStringLiteral("42% + папка/имя с пробелом.txt"));
        QCOMPARE(progress->value(), 42);
        QVERIFY(progress->labelText().contains(QStringLiteral("папка/имя с пробелом.txt")));
        backend.completeList({});
        journal->trigger();
        QCoreApplication::processEvents();
        auto *text = window.findChild<QPlainTextEdit *>("operationJournalText");
        QVERIFY(text);
        QVERIFY(text->toPlainText().contains(QStringLiteral("Открытие архива: ")));
        QVERIFY(text->toPlainText().contains(QStringLiteral("42% + папка/имя с пробелом.txt")));
        QVERIFY(text->toPlainText().contains(QStringLiteral("Результат: Архив открыт.")));
        auto *dialog = window.findChild<QDialog *>("operationJournalDialog");
        QVERIFY(dialog);
        const QString screenshotDir = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
        if (!screenshotDir.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshotDir));
            const QString screenshot =
                screenshotDir + "/" + QSysInfo::productType() + "-journal.png";
            QVERIFY(dialog->grab().save(screenshot));
        }

        window.openArchive("second.zip");
        backend.report(-1, {}, QStringLiteral("ERROR: повреждённый архив"));
        QTimer::singleShot(0, &window, [] { dismissWarnings(); });
        backend.fail(QStringLiteral("7zz завершился с кодом 2."));
        QVERIFY(text->toPlainText().contains(QStringLiteral("ERROR: повреждённый архив")));
        QVERIFY(text->toPlainText().contains(QStringLiteral("Ошибка: 7zz завершился с кодом 2.")));
        QVERIFY(!text->toPlainText().contains(QStringLiteral("42% +")));
        QVERIFY(journal->isEnabled());
    }

    void contextMenuFollowsSelectionAndLocation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QFile file(temp.filePath(QStringLiteral("имя с пробелом.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        window.findChild<QLineEdit *>("archivePath")->setText(temp.path());
        QTest::keyClick(window.findChild<QLineEdit *>("archivePath"), Qt::Key_Return);
        auto *view = table(window);
        QStringList commands;
        auto capture = [&] {
            QTimer::singleShot(0, [&] {
                auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                QVERIFY(menu);
                for (QAction *item : menu->actions())
                    commands << item->text();
                menu->close();
            });
        };
        capture();
        QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested", Qt::DirectConnection,
                                          Q_ARG(QPoint, QPoint(5, view->rowHeight(0) / 2))));
        QVERIFY(commands.contains("Переименовать…"));
        QVERIFY(commands.contains("Копировать"));
        QVERIFY(commands.contains("Создать папку…"));
        QVERIFY(commands.contains("Обновить"));
        QStringList properties;
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
            for (QLabel *label : dialog->findChildren<QLabel *>())
                properties << label->text();
            dialog->reject();
        });
        action(window, "selectedPropertiesAction")->trigger();
        QVERIFY(properties.contains(QStringLiteral("имя с пробелом.txt")));
        QVERIFY(properties.contains(QFileInfo(file.fileName()).canonicalFilePath()));
        commands.clear();
        view->clearSelection();
        capture();
        QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested", Qt::DirectConnection,
                                          Q_ARG(QPoint, QPoint(5, view->height() - 5))));
        QVERIFY(!commands.contains("Переименовать…"));
        QVERIFY(commands.contains("Создать папку…"));
        QVERIFY(commands.contains("Обновить"));

        window.openArchive("menu.7z");
        backend.completeList({ArchiveEntry{QStringLiteral("папка/файл.txt"), 1, 1}});
        commands.clear();
        capture();
        QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested", Qt::DirectConnection,
                                          Q_ARG(QPoint, QPoint(5, view->rowHeight(0) / 2))));
        QVERIFY(commands.contains("Извлечь выбранное…"));
        QVERIFY(commands.contains("Добавить в открытый архив…"));
        QVERIFY(!commands.contains("Создать папку…"));
        QVERIFY(commands.contains("Свойства…"));
    }

    void addressComboKeepsVisitedPaths() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString folder = temp.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QDir().mkdir(folder));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *combo = window.findChild<QComboBox *>("pathCombo");
        QVERIFY(address && combo);
        address->setText(folder);
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(address->text(), QFileInfo(folder).canonicalFilePath());
        QVERIFY(combo->findText(QFileInfo(QDir::currentPath()).canonicalFilePath()) >= 0);
        const QString archive = temp.filePath("archive.7z");
        window.openArchive(archive);
        backend.completeList({});
        QVERIFY(combo->findText(QFileInfo(archive).absoluteFilePath()) >= 0);
        const int folderIndex = combo->findText(QFileInfo(folder).canonicalFilePath());
        QVERIFY(folderIndex >= 0);
        QVERIFY(QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection,
                                          Q_ARG(int, folderIndex)));
        QCOMPARE(address->text(), QFileInfo(folder).canonicalFilePath());
    }
};

QTEST_MAIN(MainWindowStateTests)
#include "ui_state_tests.moc"
