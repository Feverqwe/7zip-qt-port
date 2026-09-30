#include "CPP/7zip/UI/FileManager/ComboDialog.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "qttrashservice.h"
#include "window_state_support.h"
#include <QScopeGuard>

namespace {
class FakeTrashService final : public IQtTrashService {
  public:
    bool MoveToTrash(const QString &path) override {
        moved << path;
        if (failAfter >= 0 && moved.size() > failAfter)
            return false;
        return QFile::remove(path);
    }
    QStringList moved;
    int failAfter = -1;
};
} // namespace

class MainWindowOperationsTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void inlineRenameCollisionRestoresSelection() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        for (const QString &name : {QStringLiteral("исходный.txt"), QStringLiteral("занято.txt")}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(name.toUtf8()), qint64(name.toUtf8().size()));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        window.show();
        QCoreApplication::processEvents();
        table(window)->selectRow(filesystemRow(window, QStringLiteral("исходный.txt")));
        action(window, "renameAction")->trigger();
        auto *editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(editor);
        editor->setText(QStringLiteral("занято.txt"));
        QTimer warningTimer;
        bool answeredWarning = false;
        connect(&warningTimer, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    answeredWarning = true;
                    box->accept();
                }
        });
        warningTimer.start(1);
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(answeredWarning);
        warningTimer.stop();
        QFile occupied(temp.filePath(QStringLiteral("занято.txt")));
        QVERIFY(occupied.open(QIODevice::ReadOnly));
        QCOMPARE(occupied.readAll(), QStringLiteral("занято.txt").toUtf8());
        QVERIFY(QFileInfo::exists(temp.filePath(QStringLiteral("исходный.txt"))));
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("исходный.txt"));
    }

    void deleteRoutesTrashAndShiftPermanent() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        for (const QString &name : {QStringLiteral("в корзину.txt"), QStringLiteral("навсегда.txt"),
                                    QStringLiteral("первый.txt"), QStringLiteral("второй.txt")}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("keep"), qint64(4));
        }
        ControlledBackend backend;
        FakeTrashService trash;
        MainWindow window(&backend, {}, &trash);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        window.show();
        QCoreApplication::processEvents();

        QString confirmation;
        bool approve = true;
        QTimer dialogTimer;
        connect(&dialogTimer, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                auto *box = qobject_cast<QMessageBox *>(widget);
                if (!box || !box->isVisible())
                    continue;
                if (box->button(QMessageBox::Yes)) {
                    confirmation = box->text();
                    box->button(approve ? QMessageBox::Yes : QMessageBox::No)->click();
                } else
                    box->accept();
            }
        });

        table(window)->selectRow(filesystemRow(window, QStringLiteral("в корзину.txt")));
        dialogTimer.start(1);
        action(window, "deleteAction")->trigger();
        dialogTimer.stop();
        QVERIFY(confirmation.contains(QStringLiteral("в корзину.txt")));
        QVERIFY(!QFileInfo::exists(temp.filePath(QStringLiteral("в корзину.txt"))));
        QCOMPARE(trash.moved.size(), 1);

        QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("папка для отказа")));
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        table(window)->selectRow(filesystemRow(window, QStringLiteral("папка для отказа")));
        approve = false;
        confirmation.clear();
        dialogTimer.start(1);
        action(window, "deleteAction")->trigger();
        dialogTimer.stop();
        QVERIFY(confirmation.contains(QStringLiteral("папка для отказа")));
        QVERIFY(QDir(temp.filePath(QStringLiteral("папка для отказа"))).exists());
        approve = true;

        table(window)->selectRow(filesystemRow(window, QStringLiteral("навсегда.txt")));
        table(window)->setFocus();
        confirmation.clear();
        dialogTimer.start(1);
        QTest::keyClick(table(window), Qt::Key_Delete, Qt::ShiftModifier);
        dialogTimer.stop();
        QVERIFY(confirmation.contains(QStringLiteral("навсегда.txt")));
        QVERIFY(!QFileInfo::exists(temp.filePath(QStringLiteral("навсегда.txt"))));
        QCOMPARE(trash.moved.size(), 1);
        QTest::keyRelease(table(window), Qt::Key_Shift);

        trash.failAfter = 2;
        table(window)->setSelectionMode(QAbstractItemView::ExtendedSelection);
        const int firstRow = filesystemRow(window, QStringLiteral("первый.txt"));
        const int secondRow = filesystemRow(window, QStringLiteral("второй.txt"));
        QVERIFY(firstRow >= 0);
        QVERIFY(secondRow >= 0);
        QVERIFY(firstRow != secondRow);
        QItemSelection selection;
        selection.select(table(window)->model()->index(firstRow, 0),
                         table(window)->model()->index(firstRow, table(window)->columnCount() - 1));
        selection.select(
            table(window)->model()->index(secondRow, 0),
            table(window)->model()->index(secondRow, table(window)->columnCount() - 1));
        table(window)->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        confirmation.clear();
        dialogTimer.start(1);
        action(window, "deleteAction")->trigger();
        dialogTimer.stop();
        QVERIFY(!confirmation.isEmpty());
        QVERIFY(confirmation.contains(QLatin1Char('2')));
        QVERIFY2(QFileInfo::exists(temp.filePath(QStringLiteral("первый.txt"))) !=
                     QFileInfo::exists(temp.filePath(QStringLiteral("второй.txt"))),
                 qPrintable(QStringLiteral("first=%1 second=%2 moved=%3")
                                .arg(QFileInfo::exists(temp.filePath(QStringLiteral("первый.txt"))))
                                .arg(QFileInfo::exists(temp.filePath(QStringLiteral("второй.txt"))))
                                .arg(trash.moved.size())));
    }

    void sourceFilesystemCreateAndInlineRename() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        window.show();
        QCoreApplication::processEvents();

        const auto answer = [&window](const char *actionName, const QString &name, bool accept) {
            QTimer::singleShot(0, &window, [&window, name, accept] {
                auto *dialog = window.findChild<CComboDialog *>("comboDialog");
                QVERIFY(dialog);
                if (accept) {
                    dialog->findChild<QComboBox *>("comboValue")->setCurrentText(name);
                    dialog->accept();
                } else
                    dialog->reject();
            });
            action(window, actionName)->trigger();
        };
        answer("createFolderAction", QStringLiteral("папка с пробелом/child"), true);
        QVERIFY(QDir(temp.filePath(QStringLiteral("папка с пробелом/child"))).exists());
        answer("createFileAction", QStringLiteral("папка с пробелом/child/файл.txt"), true);
        QVERIFY(
            QFileInfo::exists(temp.filePath(QStringLiteral("папка с пробелом/child/файл.txt"))));
        QTimer warningTimer;
        connect(&warningTimer, &QTimer::timeout, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
        });
        warningTimer.start(1);
        answer("createFileAction", QStringLiteral("папка с пробелом/child/файл.txt"), true);
        warningTimer.stop();
        QVERIFY(
            QFileInfo::exists(temp.filePath(QStringLiteral("папка с пробелом/child/файл.txt"))));
        answer("createFolderAction", QStringLiteral("отмена"), false);
        QVERIFY(!QFileInfo::exists(temp.filePath(QStringLiteral("отмена"))));
    }

    void inlineRenameCancel_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<int>("width");
        for (int width : {700, 400})
            for (int mode = 0; mode < 4; ++mode)
                QTest::newRow(qPrintable(QStringLiteral("mode-%1-width-%2").arg(mode).arg(width)))
                    << mode << width;
    }

    void inlineRenameCancel() {
        QFETCH(int, mode);
        QFETCH(int, width);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = temp.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("папка с пробелом")));
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        window.resize(width, 380);
        window.show();
        QCoreApplication::processEvents();
        action(window, qPrintable(QStringLiteral("viewMode%1Action").arg(mode)))->trigger();
        table(window)->selectRow(filesystemRow(window, QStringLiteral("папка с пробелом")));
        action(window, "renameAction")->trigger();
        auto *editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(editor);
        editor->setText(QStringLiteral("переименовано"));
        QTest::keyClick(editor, Qt::Key_Escape);
        QVERIFY(QDir(path).exists());
        QVERIFY(!QFileInfo::exists(temp.filePath(QStringLiteral("переименовано"))));
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
        FakeTrashService trash;
        MainWindow window(&backend, {}, &trash);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        const int row = filesystemRow(window, QStringLiteral("имя с пробелом.txt"));
        QVERIFY(row >= 0);
        table(window)->selectRow(row);
        QVERIFY(action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "moveAction")->isEnabled());
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QTRY_VERIFY(action(window, "renameAction")->isEnabled());
        const QScopedPointer<QMimeData> outbound(
            table(window)->model()->mimeData(table(window)->selectionModel()->selectedIndexes()));
        QVERIFY(outbound->hasUrls());
        QCOMPARE(QFileInfo(outbound->urls().first().toLocalFile()).canonicalFilePath(),
                 QFileInfo(source).canonicalFilePath());

        table(window)->item(row, 1)->setText(temp.filePath("forged-selection.txt"));

        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget))
                    dialog->reject();
            }
        });
        action(window, "moveAction")->trigger();
        QVERIFY(QFileInfo::exists(source));

        const QString destination = temp.filePath("destination");
        QString copyInfo;
        QTimer::singleShot(0, &window, [destination, &copyInfo] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget)) {
                    copyInfo =
                        dialog->findChild<QPlainTextEdit *>("copySelectionInfo")->toPlainText();
                    dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
                    dialog->accept();
                }
            }
        });
        action(window, "copyAction")->trigger();
        QVERIFY(copyInfo.startsWith("Файлов: 1    ( 8 байт )\n\n"));
        QVERIFY(copyInfo.contains(temp.path() + '/'));
        QVERIFY(copyInfo.endsWith(QStringLiteral("  имя с пробелом.txt")));
        const QString copied = QDir(destination).filePath(QStringLiteral("имя с пробелом.txt"));
        QVERIFY(!action(window, "copyAction")->isEnabled());
        auto *copyProgress =
            dynamic_cast<CProgressDialog *>(window.findChild<QDialog *>("progressDialog"));
        QVERIFY(copyProgress);
        QVERIFY(!copyProgress->findChild<QPushButton *>("IDB_PAUSE")->isEnabled());
        QVERIFY(copyProgress->findChild<QPushButton *>("IDB_PROGRESS_BACKGROUND")->isEnabled());
        QTRY_VERIFY(action(window, "copyAction")->isEnabled());
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        QCOMPARE(settings.value("CopyHistory").toStringList().first(), destination);
        QFile copiedFile(copied);
        QVERIFY(copiedFile.open(QIODevice::ReadOnly));
        QCOMPARE(copiedFile.readAll(), QByteArray("original"));
        copiedFile.close();
        QTimer warningTimer;
        bool renameAnswered = false;
        QString finalMessage;
        connect(&warningTimer, &QTimer::timeout, &window,
                [destination, &renameAnswered, &finalMessage] {
                    for (QWidget *widget : QApplication::topLevelWidgets()) {
                        if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                            dialog && dialog->isVisible()) {
                            dialog->findChild<QComboBox *>("copyDestination")
                                ->setCurrentText(destination);
                            dialog->accept();
                        }
                        if (auto *box = qobject_cast<QMessageBox *>(widget);
                            box && box->isVisible()) {
                            finalMessage = box->text() + box->informativeText();
                            box->accept();
                        }
                        if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                            dialog && dialog->isVisible()) {
                            renameAnswered = true;
                            dialog->findChild<QPushButton *>("overwriteRename")->click();
                        }
                    }
                });
        warningTimer.start(1);
        action(window, "copyAction")->trigger();
        QTRY_VERIFY(action(window, "copyAction")->isEnabled());
        warningTimer.stop();
        QVERIFY(renameAnswered);
        QVERIFY(QFileInfo::exists(source));
        QVERIFY(QFileInfo::exists(copied));
        QVERIFY2(
            QFileInfo::exists(QDir(destination).filePath(QStringLiteral("имя с пробелом_1.txt"))),
            qPrintable(
                finalMessage + " :: " +
                QDir(destination).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).join(',')));
        QTimer cancelTimer;
        bool cancelAnswered = false;
        connect(&cancelTimer, &QTimer::timeout, &window, [destination, &cancelAnswered] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(destination);
                    dialog->accept();
                }
                if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    cancelAnswered = true;
                    dialog->findChild<QPushButton *>("overwriteCancel")->click();
                }
            }
        });
        cancelTimer.start(1);
        action(window, "copyAction")->trigger();
        QTRY_VERIFY(action(window, "copyAction")->isEnabled());
        cancelTimer.stop();
        QVERIFY(cancelAnswered);
        QVERIFY(QFileInfo::exists(source));
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
                if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                    dialog && dialog->isVisible())
                    dialog->findChild<QPushButton *>("overwriteNo")->click();
            }
        });
        moveErrorTimer.start(1);
        action(window, "moveAction")->trigger();
        QTRY_VERIFY(action(window, "moveAction")->isEnabled());
        moveErrorTimer.stop();
        QVERIFY(QFileInfo::exists(source));
        QVERIFY(QFileInfo::exists(copied));

        window.show();
        QCoreApplication::processEvents();
        const int copiedRow = filesystemRow(window, QStringLiteral("имя с пробелом.txt"));
        table(window)->selectRow(copiedRow);
        table(window)->item(copiedRow, 1)->setText(temp.filePath("forged-rename.txt"));
        action(window, "renameAction")->trigger();
        auto *editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(editor);
        editor->setText(QStringLiteral("новое имя.txt"));
        QTest::keyClick(editor, Qt::Key_Return);
        const QString renamed = temp.filePath(QStringLiteral("новое имя.txt"));
        QTRY_VERIFY(QFileInfo::exists(renamed));
        QVERIFY(!QFileInfo::exists(source));

        table(window)->selectRow(filesystemRow(window, QStringLiteral("новое имя.txt")));
        table(window)
            ->item(table(window)->selectionModel()->selectedRows().first().row(), 1)
            ->setText(temp.filePath("forged-delete.txt"));
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::No)->click();
            }
        });
        action(window, "deleteAction")->trigger();
        QVERIFY(QFileInfo::exists(renamed));
        QTimer deleteDialogTimer;
        connect(&deleteDialogTimer, &QTimer::timeout, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    if (auto *yes = box->button(QMessageBox::Yes))
                        yes->click();
                    else
                        box->accept();
                }
            }
        });
        deleteDialogTimer.start(1);
        action(window, "deleteAction")->trigger();
        deleteDialogTimer.stop();
        QTRY_VERIFY(action(window, "createFolderAction")->isEnabled());
        QVERIFY(!QFileInfo::exists(renamed));
        QCOMPARE(trash.moved.size(), 1);
        QCOMPARE(QFileInfo(trash.moved.first()).canonicalPath(),
                 QFileInfo(renamed).canonicalPath());
        QCOMPARE(QFileInfo(trash.moved.first()).fileName(), QFileInfo(renamed).fileName());
        QVERIFY(QFileInfo::exists(copied));

        QFile movable(temp.filePath("move me.txt"));
        QVERIFY(movable.open(QIODevice::WriteOnly));
        movable.write("move");
        movable.close();
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        table(window)->selectRow(filesystemRow(window, "move me.txt"));
        table(window)
            ->item(table(window)->selectionModel()->selectedRows().first().row(), 1)
            ->setText(temp.filePath("forged-move.txt"));
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

        openListedArchive(window, backend, "items.zip",
                          {ArchiveEntry{QStringLiteral("nested/file.txt"), 4, 4}});
        const int row = filesystemRow(window, "nested");
        QVERIFY(row >= 0);
        table(window)->selectRow(row);
        QTimer::singleShot(0, &window, [&temp] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible()) {
                    if (auto *destination = dialog->findChild<QComboBox *>("extractDestination")) {
                        dialog->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
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
        QTimer cancelAdd;
        cancelAdd.setInterval(5);
        bool cancelRequested = false;
        connect(&cancelAdd, &QTimer::timeout, &window, [&] {
            if (auto *question = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                if (question->objectName() == "progressCancelQuestion")
                    question->button(QMessageBox::Yes)->click();
                return;
            }
            if (backend.addCalls == 1 && !cancelRequested) {
                cancelRequested = true;
                QVERIFY(!action(window, "copyAction")->isEnabled());
                auto *progress =
                    dynamic_cast<CProgressDialog *>(window.findChild<QDialog *>("progressDialog"));
                QVERIFY(progress);
                QTimer::singleShot(0, progress, [progress] { progress->reject(); });
            }
        });
        cancelAdd.start();
        QCoreApplication::sendEvent(table(window)->viewport(), &archiveDrop);
        cancelAdd.stop();
        QCOMPARE(backend.addCalls, 1);
        QVERIFY(cancelRequested);
        QVERIFY(action(window, "copyAction")->isEnabled());
    }

    void stage7ArchiveCommandStates() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(backend.prepareArchiveFixture("first.7z"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard(
                [&] { backend.completeList({ArchiveEntry{QStringLiteral("file.txt"), 4, 4}}); });
            QVERIFY(!action(window, "copyAction")->isEnabled());
            QVERIFY(!action(window, "moveAction")->isEnabled());
        });
        window.openArchive("first.7z");
        table(window)->selectRow(0);
        QVERIFY(action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "moveAction")->isEnabled());
        QVERIFY(action(window, "deleteAction")->isEnabled());
        QVERIFY(action(window, "renameAction")->isEnabled());
        QVERIFY(backend.prepareArchiveFixture("second.zip"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QVERIFY(!action(window, "copyAction")->isEnabled());
        });
        window.openArchive("second.zip");
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(backend.prepareArchiveFixture("cancel.7z"));
        backend.queueListAction([&] { backend.cancel(); });
        window.openArchive("cancel.7z");
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
        openListedArchive(window, backend, temp.filePath("items.7z"),
                          {ArchiveEntry{QStringLiteral("папка/файл с пробелом.txt"), 4, 4}});
        table(window)->selectRow(filesystemRow(window, QStringLiteral("папка")));
        QString moveInfo;
        const auto submit = [&window, &destination, &moveInfo] {
            QTimer::singleShot(0, &window, [&destination, &moveInfo] {
                for (QWidget *widget : QApplication::topLevelWidgets()) {
                    if (auto *dialog = qobject_cast<CopyDialog *>(widget);
                        dialog && dialog->isVisible()) {
                        moveInfo =
                            dialog->findChild<QPlainTextEdit *>("copySelectionInfo")->toPlainText();
                        dialog->findChild<QComboBox *>("copyDestination")
                            ->setCurrentText(destination);
                        dialog->accept();
                    }
                }
            });
            action(window, "moveAction")->trigger();
        };
        submit();
        QVERIFY(moveInfo.startsWith("Папок: 1    ( 4 байт )\n\n"));
        QVERIFY(moveInfo.endsWith(QStringLiteral("  папка/")));
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
        backend.queueList({});
        backend.completeAdd();
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(!action(window, "moveAction")->isEnabled());
    }
    void extractAskUsesSourceOverwriteDialog() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString source = temp.filePath(QStringLiteral("архивный файл.txt"));
        QFile incoming(source);
        QVERIFY(incoming.open(QIODevice::WriteOnly));
        QCOMPARE(incoming.write("archive"), qint64(7));
        incoming.close();
        const QString archive = temp.filePath("ask.7z");
        const QString target = temp.filePath(QStringLiteral("destination/архивный файл.txt"));
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        finished.clear();
        QFile old(target);
        QVERIFY(old.open(QIODevice::WriteOnly));
        QCOMPARE(old.write("existing"), qint64(8));
        old.close();
        QSignalSpy preflight(&backend, &ArchiveBackend::overwriteRequired);
        QSignalSpy batchOpen(&backend, &ArchiveBackend::entriesReady);
        bool answered = false;
        QByteArray choice("overwriteYes");
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    answered = true;
                    QVERIFY(!dialog->findChild<QLabel *>("overwriteOldFile")->text().isEmpty());
                    dialog->findChild<QPushButton *>(choice.constData())->click();
                }
            }
        });
        answer.start(1);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        window.extractTo(temp.filePath("destination"), false, {}, options);
        QVERIFY(finished.size() >= 1);
        QVERIFY(answered);
        QCOMPARE(batchOpen.size(), 1);
        QCOMPARE(preflight.size(), 0); // The source callback owns this prompt.
        QVERIFY(finished.last().first().toBool());
        QFile result(target);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("archive"));
        result.close();

        QVERIFY(old.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(old.write("preserved"), qint64(9));
        old.close();
        choice = "overwriteNo";
        answered = false;
        table(window)->selectRow(0);
        window.extractTo(temp.filePath("destination"), true, {}, options);
        QVERIFY(answered);
        QVERIFY(old.open(QIODevice::ReadOnly));
        QCOMPARE(old.readAll(), QByteArray("preserved"));
        old.close();

        choice = "overwriteRename";
        answered = false;
        window.extractTo(temp.filePath("destination"), true, {}, options);
        QVERIFY(answered);
        QStringList files =
            QDir(temp.filePath("destination")).entryList(QDir::Files | QDir::NoDotAndDotDot);
        QCOMPARE(files.size(), 2);
        QVERIFY(files.removeOne(QStringLiteral("архивный файл.txt")));
        const QString renamed = files.first();
        QFile renamedFile(temp.filePath("destination/" + renamed));
        QVERIFY(renamedFile.open(QIODevice::ReadOnly));
        QCOMPARE(renamedFile.readAll(), QByteArray("archive"));

        choice = "overwriteCancel";
        answered = false;
        window.extractTo(temp.filePath("destination"), true, {}, options);
        answer.stop();
        QVERIFY(answered);
        QCOMPARE(preflight.size(), 0);
        QVERIFY(old.open(QIODevice::ReadOnly));
        QCOMPARE(old.readAll(), QByteArray("preserved"));
    }

    void sourceNoPathsAskFlattensSelectedFolder() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("folder with space"));
        const QString name = QStringLiteral("δ name.txt");
        QVERIFY(QDir().mkpath(source));
        QFile input(QDir(source).filePath(name));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("new bytes"), qint64(9));
        input.close();
        const QString archive = temp.filePath("flat.zip");
        const QString output = temp.filePath("output");
        QVERIFY(QDir().mkpath(output));
        QFile occupied(QDir(output).filePath(name));
        QVERIFY(occupied.open(QIODevice::WriteOnly));
        QCOMPARE(occupied.write("old bytes"), qint64(9));
        occupied.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.openArchive(archive);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        QSignalSpy preflight(&backend, &ArchiveBackend::overwriteRequired);
        bool answered = false;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    answered = true;
                    dialog->findChild<QPushButton *>("overwriteYes")->click();
                }
            }
        });
        answer.start(1);
        ExtractionOptions options;
        options.pathMode = ExtractionOptions::PathMode::NoPaths;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        window.extractTo(output, true, {}, options);
        answer.stop();
        QVERIFY(answered);
        QCOMPARE(preflight.size(), 0);
        QVERIFY(occupied.open(QIODevice::ReadOnly));
        QCOMPARE(occupied.readAll(), QByteArray("new bytes"));
        // The source callback retains the explicit archive directory as empty.
        const QDir explicitDirectory(QDir(output).filePath("folder with space"));
        QVERIFY(explicitDirectory.exists());
        QVERIFY(explicitDirectory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());
    }

    void encryptedAskUsesSourceCallback_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }

    void nestedFolderAskUsesSourceRelativePath() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString folderName = QStringLiteral("внутренняя папка");
        const QString fileName = QStringLiteral("δ name.txt");
        const QString source = temp.filePath(folderName);
        const QString archive = temp.filePath("nested.zip");
        const QString output = temp.filePath("output");
        const QString target = QDir(output).filePath(fileName);
        QVERIFY(QDir().mkpath(source));
        QVERIFY(QDir().mkpath(output));
        QFile input(QDir(source).filePath(fileName));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("new bytes"), qint64(9));
        input.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.openArchive(archive);
        QTRY_VERIFY(table(window)->rowCount() >= 1);
        int folderRow = -1;
        for (int row = 0; row < table(window)->rowCount(); ++row)
            if (table(window)->item(row, 0)->text() == folderName)
                folderRow = row;
        QVERIFY(folderRow >= 0);
        table(window)->selectRow(folderRow);
        action(window, "openItemAction")->trigger();
        QTRY_VERIFY(!table(window)->findItems(fileName, Qt::MatchExactly).isEmpty());
        QFile occupied(target);
        QVERIFY(occupied.open(QIODevice::WriteOnly));
        QCOMPARE(occupied.write("old bytes"), qint64(9));
        occupied.close();
        QSignalSpy preflight(&backend, &ArchiveBackend::overwriteRequired);
        bool answered = false;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    answered = true;
                    dialog->findChild<QPushButton *>("overwriteYes")->click();
                }
            }
        });
        answer.start(1);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        window.extractTo(output, false, {}, options);
        answer.stop();
        QVERIFY(answered);
        QCOMPARE(preflight.size(), 0);
        QVERIFY(occupied.open(QIODevice::ReadOnly));
        QCOMPARE(occupied.readAll(), QByteArray("new bytes"));
        QVERIFY(!QFileInfo::exists(QDir(output).filePath(folderName + '/' + fileName)));
    }

    void encryptedAskUsesSourceCallback() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("секрет с пробелом.txt"));
        const QString archive = temp.filePath("encrypted." + format);
        const QString output = temp.filePath("output");
        const QString target = QDir(output).filePath(QFileInfo(source).fileName());
        const QString password = QStringLiteral("test-only-password");
        QVERIFY(QDir().mkpath(output));
        QFile input(source);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("new secret"), qint64(10));
        input.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source}, password);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        QTimer openPassword;
        connect(&openPassword, &QTimer::timeout, &window, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "passwordDialog")
                return;
            auto *edit = dialog->findChild<QLineEdit *>("passwordEdit");
            QVERIFY(edit);
            edit->setText(password);
            dialog->accept();
        });
        openPassword.start(1);
        window.openArchive(archive);
        openPassword.stop();
        QTRY_COMPARE(table(window)->rowCount(), 1);
        finished.clear();
        QFile occupied(target);
        QVERIFY(occupied.open(QIODevice::WriteOnly));
        QCOMPARE(occupied.write("old secret"), qint64(10));
        occupied.close();
        QSignalSpy preflight(&backend, &ArchiveBackend::overwriteRequired);
        bool answered = false;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = dynamic_cast<COverwriteDialog *>(widget);
                    dialog && dialog->isVisible()) {
                    answered = true;
                    dialog->findChild<QPushButton *>("overwriteYes")->click();
                }
            }
        });
        answer.start(1);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        bool sawFailure = false;
        bool closeScheduled = false;
        QTimer dismissFailure;
        connect(&dismissFailure, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (!widget->isVisible())
                    continue;
                if (auto *message = qobject_cast<QMessageBox *>(widget)) {
                    if (message->objectName() == "progressFinalMessage") {
                        sawFailure = true;
                        if (!closeScheduled) {
                            closeScheduled = true;
                            QTimer::singleShot(50, &window, [&window] {
                                auto *progress = window.findChild<QDialog *>("progressDialog");
                                auto *close = progress
                                                  ? progress->findChild<QPushButton *>("IDCANCEL")
                                                  : nullptr;
                                if (close && close->text().contains(QStringLiteral("Закрыть")))
                                    close->click();
                            });
                        }
                    }
                    message->accept();
                }
            }
            auto *progress = window.findChild<QDialog *>("progressDialog");
            auto *close = progress ? progress->findChild<QPushButton *>("IDCANCEL") : nullptr;
            if (close && close->text().contains(QStringLiteral("Закрыть"))) {
                sawFailure = true;
                if (!closeScheduled) {
                    closeScheduled = true;
                    QTimer::singleShot(50, &window, [&window] {
                        auto *progress = window.findChild<QDialog *>("progressDialog");
                        auto *close =
                            progress ? progress->findChild<QPushButton *>("IDCANCEL") : nullptr;
                        if (close && close->text().contains(QStringLiteral("Закрыть")))
                            close->click();
                    });
                }
            }
        });
        dismissFailure.start(1);
        window.extractTo(output, false, QStringLiteral("wrong-password"), options);
        dismissFailure.stop();
        QVERIFY(sawFailure);
        QVERIFY(!answered);
        QVERIFY(occupied.open(QIODevice::ReadOnly));
        QCOMPARE(occupied.readAll(), QByteArray("old secret"));
        occupied.close();
        window.extractTo(output, false, password, options);
        answer.stop();
        QVERIFY(answered);
        QCOMPARE(preflight.size(), 0);
        QVERIFY(occupied.open(QIODevice::ReadOnly));
        QCOMPARE(occupied.readAll(), QByteArray("new secret"));
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
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("исходный файл.txt"));
        QFile input(source);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("payload"), qint64(7));
        input.close();
        const QString archive = temp.filePath("mutate.7z");
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        window.openArchive(archive);
        window.show();
        QCoreApplication::processEvents();
        QTRY_VERIFY(action(window, "createFolderAction")->isEnabled());
        QTimer::singleShot(0, &window, [&window] {
            auto *dialog = window.findChild<CComboDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")
                ->setCurrentText(QStringLiteral("новая папка"));
            dialog->accept();
        });
        action(window, "createFolderAction")->trigger();
        const auto archiveRow = [&window](const QString &name) {
            for (int row = 0; row < table(window)->rowCount(); ++row)
                if (table(window)->item(row, 0)->text().normalized(QString::NormalizationForm_C) ==
                    name.normalized(QString::NormalizationForm_C))
                    return row;
            return -1;
        };
        QTRY_VERIFY(archiveRow(QStringLiteral("новая папка")) >= 0);
        table(window)->selectRow(archiveRow(QStringLiteral("исходный файл.txt")));
        QTRY_VERIFY(action(window, "renameAction")->isEnabled());
        action(window, "renameAction")->trigger();
        auto *editor = table(window)->findChild<QLineEdit *>();
        QVERIFY(editor);
        editor->setFocus();
        editor->setText(QStringLiteral("новое имя.txt"));
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(archiveRow(QStringLiteral("новое имя.txt")) >= 0);
        table(window)->selectRow(archiveRow(QStringLiteral("новое имя.txt")));
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::No)->click();
        });
        action(window, "deleteAction")->trigger();
        QVERIFY(archiveRow(QStringLiteral("новое имя.txt")) >= 0);
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::Yes)->click();
        });
        action(window, "deleteAction")->trigger();
        QTRY_VERIFY(archiveRow(QStringLiteral("новое имя.txt")) < 0);
        QVERIFY(archiveRow(QStringLiteral("новая папка")) >= 0);
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
        QVERIFY(backend.prepareArchiveFixture(archive));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QVERIFY(!action(window, "addArchiveAction")->isEnabled());
            QVERIFY(!action(window, "toolbarAddAction")->isEnabled());
        });
        window.openArchive(archive);
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());

        auto choose = [&](int outcome) {
            const int expectedCalls = backend.addCalls + 1;
            bool completed = false;
            QTimer finish;
            finish.setInterval(5);
            connect(&finish, &QTimer::timeout, &window, [&] {
                if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    if (box->objectName() == "progressCancelQuestion")
                        box->button(QMessageBox::Yes)->click();
                    else if (box->objectName() == "progressFinalMessage")
                        box->button(QMessageBox::Ok)->click();
                    return;
                }
                if (backend.addCalls != expectedCalls || completed)
                    return;
                completed = true;
                QVERIFY(!action(window, "addArchiveAction")->isEnabled());
                QVERIFY(!action(window, "openArchiveAction")->isEnabled());
                QVERIFY(!table(window)->isEnabled());
                auto *progress =
                    dynamic_cast<CProgressDialog *>(window.findChild<QDialog *>("progressDialog"));
                QVERIFY(progress);
                if (outcome == 0)
                    QTimer::singleShot(0, progress, [progress] { progress->reject(); });
                else if (outcome == 1)
                    backend.fail(QStringLiteral("Ошибка добавления."));
                else
                    backend.completeAdd();
            });
            finish.start();
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
            QVERIFY(completed);
        };
        choose(0);
        QCOMPARE(backend.addCalls, 1);
        QCOMPARE(backend.lastAddTarget, archive);
        QCOMPARE(backend.lastAddSources.first().normalized(QString::NormalizationForm_C),
                 canonicalSource.normalized(QString::NormalizationForm_C));
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        choose(1);
        QCOMPARE(backend.addCalls, 2);
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] {
                backend.completeList({{QStringLiteral("новый файл.txt"), 4, 4, {}, false, false}});
            });
            QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        });
        choose(2);
        QCOMPARE(backend.addCalls, 3);
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(action(window, "addArchiveAction")->isEnabled());
        QCOMPARE(table(window)->rowCount(), 1);
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QVERIFY(!action(window, "addArchiveAction")->isEnabled());
        });
        window.openArchive(archive);
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
                dialog->findChild<QLineEdit *>("archiveName")->setText("new archive.zip");
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
        openListedArchive(window, backend, temp.filePath("empty.7z"), {});
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        action(window, "backAction")->trigger();
        QVERIFY(QFile::remove(temp.filePath("empty.7z")));
        action(window, "refreshAction")->trigger();
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
