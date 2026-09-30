#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "layout_inspection.h"
#include "qtprogresscontrols.h"
#include "window_state_support.h"
#include <QProgressBar>
#include <QScopeGuard>
#include <QTreeWidget>

class MainWindowArchiveStatesTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static QString messageText(const QTreeWidget *list) {
        QString text;
        for (int i = 0; i < list->topLevelItemCount(); ++i)
            text += list->topLevelItem(i)->text(1) + '\n';
        return text;
    }

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void extractionDestinationAndCancellation() {
        ControlledBackend backend;
        MainWindow window(&backend);
        openListedArchive(window, backend, "archive.zip",
                          {ArchiveEntry{QStringLiteral("папка/δ space.txt"), 3, 3}});
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        QTimer::singleShot(0, &window, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            auto *destination = dialog->findChild<QComboBox *>("extractDestination");
            QVERIFY(destination);
            dialog->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
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
            dialog->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
            destination->setCurrentText("/temporary/selected output");
            auto *password = dialog->findChild<QLineEdit *>("extractPassword");
            QVERIFY(password);
            password->setText("temporary secret");
            dialog->findChild<QComboBox *>("extractPathMode")->setCurrentIndex(1);
            dialog->findChild<QComboBox *>("extractOverwriteMode")->setCurrentIndex(2);
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
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QFile archive(temporary.filePath("missing.7z"));
        QVERIFY(archive.open(QIODevice::WriteOnly));
        archive.close();
        MainWindow window(&backend);
        QTimer timer;
        connect(&timer, &QTimer::timeout, &window, [] { dismissWarnings(); });
        timer.start(10);
        window.openArchive(archive.fileName());
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
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QVERIFY(!action(window, "backAction")->isEnabled());
        });
        QVERIFY(QMetaObject::invokeMethod(table(window), "cellDoubleClicked", Qt::DirectConnection,
                                          Q_ARG(int, archiveRow), Q_ARG(int, 0)));
        QCOMPARE(backend.listCalls, 1);
        QCOMPARE(backend.lastArchive, QFileInfo(archive.fileName()).canonicalFilePath());
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
        QVERIFY(backend.prepareArchiveFixture("cancel.7z"));
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [] {
            if (auto *question = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
                question && question->objectName() == "progressCancelQuestion")
                question->button(QMessageBox::Yes)->click();
        });
        responder.start(1);
        backend.queueListAction([&] {
            auto *progress = dynamic_cast<CQtProgressDialog *>(QApplication::activeModalWidget());
            if (!progress) {
                backend.cancel();
                QFAIL("Native archive open progress dialog is missing");
            }
            auto *button = progress->findChild<QPushButton *>("IDCANCEL");
            if (!button) {
                backend.cancel();
                QFAIL("Native archive open cancel control is missing");
            }
            button->click();
        });
        window.openArchive("cancel.7z");
        responder.stop();
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
        auto *log = window.findChild<QTreeWidget *>("IDL_MESSAGE");
        QVERIFY(log);
        QVERIFY(messageText(log).contains(QStringLiteral("Операция отменена.")));
        openListedArchive(window, backend, "after-cancel.zip", {});
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
    }

    void operationCancellationKeepsArchive() {
        ControlledBackend backend;
        MainWindow window(&backend);
        openListedArchive(window, backend, "opened.7z",
                          {ArchiveEntry{QStringLiteral("файл.txt"), 4, 4}});
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
        auto *progress =
            dynamic_cast<CProgressDialog *>(window.findChild<QDialog *>("progressDialog"));
        QVERIFY(progress);
        auto *button = progress->findChild<QPushButton *>("IDCANCEL");
        QVERIFY(button);
        QTimer::singleShot(0, &window, [] {
            auto *question = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            QVERIFY(question);
            QCOMPARE(question->objectName(), QStringLiteral("progressCancelQuestion"));
            question->button(QMessageBox::Yes)->click();
        });
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
        QVERIFY(backend.prepareArchiveFixture("protected.7z"));
        QTimer responder;
        connect(&responder, &QTimer::timeout, &window, [] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (dialog && dialog->objectName() == "passwordDialog")
                dialog->reject();
        });
        responder.start(1);
        backend.queueListAction([&] { emit backend.passwordRequired(); });
        window.openArchive("protected.7z");
        responder.stop();
        QCOMPARE(backend.cancelCalls, 1);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        openListedArchive(window, backend, "empty.zip", {});
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
    }

    void passwordReuseAndResetOnReopen() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(backend.prepareArchiveFixture("protected.7z"));
        const QString password = QStringLiteral("disposable-δ-password");
        QTimer responder;
        bool supplied = false;
        connect(&responder, &QTimer::timeout, &window, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "passwordDialog" || supplied)
                return;
            supplied = true;
            const auto finish = qScopeGuard([&] {
                dialog->accept();
                QTimer::singleShot(0, &backend, [&] {
                    emit backend.infoReady(
                        ArchiveInfo{QStringLiteral("7z"), 120, 0, 3, 1, 0, true});
                    backend.completeList({ArchiveEntry{QStringLiteral("δ file.txt"), 3, 3}});
                });
            });
            auto *edit = dialog->findChild<QLineEdit *>("passwordEdit");
            QVERIFY(edit);
            edit->setText(password);
            auto *show = dialog->findChild<QCheckBox *>("passwordShow");
            QVERIFY(show);
            if (!show->isChecked())
                show->click();
            QCOMPARE(edit->echoMode(), QLineEdit::Normal);
        });
        responder.start(1);
        backend.queueListAction([&] { emit backend.passwordRequired(); });
        window.openArchive("protected.7z");
        responder.stop();
        QVERIFY(supplied);
        QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        window.testArchive();
        QCOMPARE(backend.lastTestPassword, password);
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        backend.fail(QStringLiteral("Операция отменена."));
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        openListedArchive(window, backend, "empty.zip", {});
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
        QCOMPARE(encryption->count(), 1);
        dialog.findChild<QCheckBox *>("archiveEncryptHeaders")->setChecked(true);
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
        QVERIFY(backend.prepareArchiveFixture("first.7z"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QVERIFY(journal->isEnabled());
            auto *progress = dynamic_cast<CQtProgressDialog *>(QApplication::activeModalWidget());
            QVERIFY(progress);
            QCOMPARE(progress->windowModality(), Qt::ApplicationModal);
            backend.report(42, QStringLiteral("папка/имя с пробелом.txt"),
                           QStringLiteral("42% + папка/имя с пробелом.txt"));
            auto *bar = progress->findChild<QProgressBar *>("IDC_PROGRESS1");
            auto *name = progress->findChild<QLabel *>("IDT_PROGRESS_FILE_NAME");
            QVERIFY(bar && name);
            QTRY_COMPARE(bar->value(), 42);
            QTRY_VERIFY(name->text().contains(QStringLiteral("имя с пробелом.txt")));
            QVERIFY(name->text().contains(QStringLiteral("папка")));
        });
        window.openArchive("first.7z");
        journal->trigger();
        QCoreApplication::processEvents();
        auto *text = window.findChild<QTreeWidget *>("IDL_MESSAGE");
        QVERIFY(text);
        QVERIFY(messageText(text).contains(QStringLiteral("Открытие архива: ")));
        QVERIFY(messageText(text).contains(QStringLiteral("42% + папка/имя с пробелом.txt")));
        QVERIFY(messageText(text).contains(QStringLiteral("Результат: Архив открыт.")));
        auto *dialog = window.findChild<QDialog *>("operationMessagesDialog");
        QVERIFY(dialog);
        auto *close = dialog->findChild<QPushButton *>("IDOK");
        QVERIFY(close);
        for (const QSize size : {QSize(760, 450), QSize(640, 380)}) {
            dialog->resize(size);
            QCoreApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*text, *dialog));
            QVERIFY(UiInspection::fullyVisible(*close, *dialog));
            QVERIFY(UiInspection::separate(*text, *close, *dialog));
            QVERIFY(close->sizeHint().width() <= close->width());
            QVERIFY(close->sizeHint().height() <= close->height());
            QVERIFY(UiInspection::writeReport(
                *dialog, QStringLiteral("messages-%1x%2").arg(size.width()).arg(size.height())));
        }
        const QString screenshotDir = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
        if (!screenshotDir.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshotDir));
            const QString screenshot =
                screenshotDir + "/" + QSysInfo::productType() + "-journal.png";
            QVERIFY(dialog->grab().save(screenshot));
        }

        QVERIFY(backend.prepareArchiveFixture("second.zip"));
        QTimer warningResponder;
        connect(&warningResponder, &QTimer::timeout, &window, [] { dismissWarnings(); });
        warningResponder.start(1);
        backend.queueListAction([&] {
            backend.report(-1, {}, QStringLiteral("ERROR: повреждённый архив"));
            backend.fail(QStringLiteral("7zz завершился с кодом 2."));
        });
        window.openArchive("second.zip");
        warningResponder.stop();
        QVERIFY(messageText(text).contains(QStringLiteral("ERROR: повреждённый архив")));
        QVERIFY(messageText(text).contains(QStringLiteral("Ошибка: 7zz завершился с кодом 2.")));
        QVERIFY(!messageText(text).contains(QStringLiteral("42% +")));
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
        QHash<QString, bool> enabled;
        auto capture = [&] {
            QTimer::singleShot(0, [&] {
                auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                QVERIFY(menu);
                for (QAction *item : menu->actions()) {
                    commands << item->text();
                    enabled.insert(item->text(), item->isEnabled());
                }
                menu->close();
            });
        };
        capture();
        QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested", Qt::DirectConnection,
                                          Q_ARG(QPoint, QPoint(5, view->rowHeight(0) / 2))));
        QVERIFY(commands.contains("Переименовать…"));
        QVERIFY(commands.contains("Копировать"));
        QVERIFY(commands.contains("Создать папку…"));
        QVERIFY(enabled.value(QStringLiteral("Переименовать…")));
        QVERIFY(!commands.contains("Обновить"));
        QStringList properties;
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
            auto *list = dialog->findChild<QTreeWidget *>("propertiesList");
            QVERIFY(list);
            for (int row = 0; row < list->topLevelItemCount(); ++row)
                properties << list->topLevelItem(row)->text(1);
            dialog->reject();
        });
        action(window, "selectedPropertiesAction")->trigger();
        QVERIFY(properties.contains(QStringLiteral("имя с пробелом.txt")));
        QVERIFY(properties.contains(
            QDir::fromNativeSeparators(QFileInfo(temp.path()).canonicalFilePath() + '/')));
        commands.clear();
        view->clearSelection();
        capture();
        QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested", Qt::DirectConnection,
                                          Q_ARG(QPoint, QPoint(5, view->height() - 5))));
        QVERIFY(commands.contains("Переименовать…"));
        QVERIFY(!enabled.value(QStringLiteral("Переименовать…")));
        QVERIFY(commands.contains("Создать папку…"));
        QVERIFY(enabled.value(QStringLiteral("Создать папку…")));

        openListedArchive(window, backend, "menu.7z",
                          {ArchiveEntry{QStringLiteral("папка/файл.txt"), 1, 1}});
        commands.clear();
        capture();
        QVERIFY(QMetaObject::invokeMethod(view, "customContextMenuRequested", Qt::DirectConnection,
                                          Q_ARG(QPoint, QPoint(5, view->rowHeight(0) / 2))));
        QVERIFY(commands.contains("Извлечь выбранное…"));
        QVERIFY(commands.contains("Добавить в открытый архив…"));
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
        openListedArchive(window, backend, archive, {});
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
