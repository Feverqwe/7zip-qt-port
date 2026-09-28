#include "window_state_support.h"

class MainWindowOperationsTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
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
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QTRY_VERIFY(action(window, "copyAction")->isEnabled());
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
        QTRY_VERIFY(action(window, "copyAction")->isEnabled());
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
        QTRY_VERIFY(action(window, "moveAction")->isEnabled());
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
        QTRY_VERIFY(QFileInfo::exists(QDir(destination).filePath("move me.txt")));
        QVERIFY(!QFileInfo::exists(movable.fileName()));
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
        QTRY_VERIFY(QFileInfo::exists(temp.filePath(QStringLiteral("in/drop ü.txt"))));
        QTRY_VERIFY(table(window)->isEnabled());

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
        QVERIFY(action(window, "moveAction")->isEnabled());
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
    void moveSelectedOutOfArchive() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString destination = temp.filePath("destination");
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive(temp.filePath("items.7z"));
        backend.completeList({ArchiveEntry{QStringLiteral("папка/файл с пробелом.txt"), 4, 4}});
        table(window)->selectRow(filesystemRow(window, QStringLiteral("папка")));
        const auto submit = [&window, &destination] {
            QTimer::singleShot(0, &window, [&destination] {
                for (QWidget *widget : QApplication::topLevelWidgets()) {
                    if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                        dialog && dialog->isVisible()) {
                        dialog->findChild<QComboBox *>("copyDestination")
                            ->setCurrentText(destination);
                        dialog->accept();
                    }
                }
            });
            action(window, "moveAction")->trigger();
        };
        submit();
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(backend.lastSelected, QStringList({QStringLiteral("папка/файл с пробелом.txt")}));
        QCOMPARE(backend.removeCalls, 0);
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
            }
        });
        backend.fail("Извлечение не удалось.");
        QCOMPARE(backend.removeCalls, 0);
        QVERIFY(action(window, "moveAction")->isEnabled());

        submit();
        QCOMPARE(backend.extractCalls, 2);
        backend.cancel();
        QCOMPARE(backend.removeCalls, 0);
        QVERIFY(action(window, "moveAction")->isEnabled());

        submit();
        QCOMPARE(backend.extractCalls, 3);
        backend.completeAdd();
        QCOMPARE(backend.removeCalls, 1);
        QCOMPARE(backend.lastRemoved, backend.lastSelected);
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    QVERIFY(box->text().contains("Файлы извлечены, но не удалены"));
                    box->accept();
                }
            }
        });
        backend.fail("Удаление не удалось.");
        QCOMPARE(backend.listCalls, 1);
        QVERIFY(action(window, "moveAction")->isEnabled());

        submit();
        QCOMPARE(backend.extractCalls, 4);
        backend.completeAdd();
        QCOMPARE(backend.removeCalls, 2);
        backend.completeAdd();
        QCOMPARE(backend.listCalls, 2);
        backend.completeList({});
        QVERIFY(!action(window, "moveAction")->isEnabled());
    }
    void movesRealArchiveEntryToDirectory() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString source = temp.filePath(QStringLiteral("файл с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("archive move"), 12);
        file.close();
        const QString archive = temp.filePath("move.7z");
        const QString destination = temp.filePath("destination");
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QTRY_VERIFY(action(window, "moveAction")->isEnabled());
        QTimer::singleShot(0, &window, [&destination] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
                    dialog->accept();
                }
            }
        });
        action(window, "moveAction")->trigger();
        const QString extracted = QDir(destination).filePath(QFileInfo(source).fileName());
        QTRY_VERIFY(QFileInfo::exists(extracted));
        QTRY_COMPARE(table(window)->rowCount(), 0);
        QFile extractedFile(extracted);
        QVERIFY(extractedFile.open(QIODevice::ReadOnly));
        QCOMPARE(extractedFile.readAll(), QByteArray("archive move"));
    }
    void stage7ArchiveMutations() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("mutate.7z");
        backend.completeList({ArchiveEntry{QStringLiteral("папка/file.txt"), 4, 4}});
        table(window)->selectRow(filesystemRow(window, QStringLiteral("папка")));
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QVERIFY(action(window, "renameAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QInputDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->setTextValue(QStringLiteral("новая папка"));
                    dialog->accept();
                }
            }
        });
        action(window, "renameAction")->trigger();
        QCOMPARE(backend.lastRenameOld, QStringLiteral("папка"));
        QCOMPARE(backend.lastRenameNew, QStringLiteral("новая папка"));
        backend.cancel();
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
        QCOMPARE(backend.renameCalls, 2);
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
                auto *threads = dialog->findChild<QComboBox *>("compressionThreads");
                threads->setCurrentIndex(threads->findData(2u));
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
};

QTEST_MAIN(MainWindowOperationsTests)
#include "ui_operations_tests.moc"
