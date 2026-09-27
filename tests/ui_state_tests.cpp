#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QStatusBar>
#include <QSysInfo>
#include <QTableWidget>
#include <QTemporaryDir>
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
    void extract(const QString &, const QString &, const QStringList &selected) override {
        ++extractCalls;
        lastSelected = selected;
        busy_ = true;
    }
    void test(const QString &) override {
        ++testCalls;
        busy_ = true;
    }
    void create(const QString &target, const QStringList &sources) override {
        ++createCalls;
        lastCreateTarget = target;
        lastCreateSources = sources;
        busy_ = true;
    }
    void add(const QString &target, const QStringList &sources) override {
        ++addCalls;
        lastAddTarget = target;
        lastAddSources = sources;
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
    int cancelCalls = 0;
    QString lastArchive;
    QStringList lastSelected;
    QString lastCreateTarget;
    QStringList lastCreateSources;
    QString lastAddTarget;
    QStringList lastAddSources;

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

  private slots:
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
        const QString archive = temp.filePath("existing.zip");
        window.openArchive(archive);
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        backend.completeList({});
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
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
            action(window, "addArchiveAction")->trigger();
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
        action(window, "createArchiveAction")->trigger();
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
            action(window, "createArchiveAction")->trigger();
        };
        chooseSources();
        QCOMPARE(backend.createCalls, 1);
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
        MainWindow window(&backend);
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
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Return);
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
        QVERIFY(!action(window, "openItemAction")->isEnabled());
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
            auto *destination = dialog->findChild<QLineEdit *>("extractDestination");
            QVERIFY(destination);
            destination->setText("/temporary/selected output");
            dialog->reject();
        });
        action(window, "extractArchiveAction")->trigger();
        QCOMPARE(backend.extractCalls, 0);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            auto *destination = dialog->findChild<QLineEdit *>("extractDestination");
            QVERIFY(destination);
            destination->setText("/temporary/selected output");
            dialog->accept();
        });
        action(window, "extractArchiveAction")->trigger();
        QCOMPARE(backend.extractCalls, 1);
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
};

QTEST_MAIN(MainWindowStateTests)
#include "ui_state_tests.moc"
