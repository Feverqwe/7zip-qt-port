#include "window_state_support.h"

class MainWindowArchiveStatesTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
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

QTEST_MAIN(MainWindowArchiveStatesTests)
#include "ui_archive_states_tests.moc"
