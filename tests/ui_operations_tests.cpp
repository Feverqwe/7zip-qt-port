#include "CPP/7zip/UI/FileManager/ComboDialog.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivecodeccatalog.h"
#include "fixtures/drag_event_position.h"
#include "qttrashservice.h"
#include "window_state_support.h"
#include <QProcess>
#include <QScopeGuard>
#include <QStandardPaths>

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
    void initTestCase() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        if (!settings.isEmpty())
            QFile::remove(settings);
    }

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

    void twoPanelFilesystemCopyRefreshesDestination() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(QDir(root.path()).mkdir(QStringLiteral("источник")));
        QVERIFY(QDir(root.path()).mkdir(QStringLiteral("назначение")));
        const QString sourceDir = root.filePath(QStringLiteral("источник"));
        const QString destinationDir = root.filePath(QStringLiteral("назначение"));
        QFile original(QDir(sourceDir).filePath(QStringLiteral("файл с пробелом.txt")));
        QVERIFY(original.open(QIODevice::WriteOnly));
        QCOMPARE(original.write("source bytes"), qint64(12));
        original.close();

        ControlledBackend backend;
        MainWindow window(&backend);
        auto *left = window.findChild<QWidget *>("filePanel0");
        QVERIFY(left);
        auto *leftPath = left->findChild<QLineEdit *>("archivePath");
        leftPath->setFocus();
        leftPath->setText(sourceDir);
        QTest::keyClick(leftPath, Qt::Key_Return);
        window.show();
        QCoreApplication::processEvents();
        auto *leftTable = left->findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(leftTable->rowCount() > 0);

        action(window, "twoPanelsAction")->trigger();
        const auto restorePanels = qScopeGuard([&] {
            if (action(window, "twoPanelsAction")->isChecked())
                action(window, "twoPanelsAction")->trigger();
        });
        auto *right = window.findChild<QWidget *>("filePanel1");
        QVERIFY(right);
        auto *rightTable = right->findChild<QTableWidget *>("archiveTable");
        QVERIFY(rightTable);
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        auto *rightPath = right->findChild<QLineEdit *>("archivePath");
        rightPath->setFocus();
        rightPath->setText(destinationDir);
        QTest::keyClick(rightPath, Qt::Key_Return);
        QTRY_COMPARE(QFileInfo(rightPath->text()).canonicalFilePath(),
                     QFileInfo(destinationDir).canonicalFilePath());
        QTRY_COMPARE(rightTable->rowCount(), 0);

        leftTable->selectRow(0);
        leftTable->setFocus();
        QCoreApplication::processEvents();
        QVERIFY(action(window, "copyAction")->isEnabled());
        QString dialogDefault;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            auto *destination = dialog->findChild<QComboBox *>("copyDestination");
            QVERIFY(destination);
            dialogDefault = destination->currentText();
            dialog->accept();
        });
        action(window, "copyAction")->trigger();
        QCOMPARE(QFileInfo(dialogDefault).canonicalFilePath(),
                 QFileInfo(destinationDir).canonicalFilePath());
        QTRY_VERIFY(action(window, "copyAction")->isEnabled());
        const QString copied = QDir(destinationDir).filePath(QStringLiteral("файл с пробелом.txt"));
        QFile result(copied);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("source bytes"));
        QTRY_COMPARE(rightTable->rowCount(), 1);
        QCOMPARE(rightTable->item(0, 0)->text(), QStringLiteral("файл с пробелом.txt"));
        QCOMPARE(leftTable->rowCount(), 1);
        QVERIFY(QFileInfo::exists(original.fileName()));
    }

    void twoPanelArchiveCopyRefreshesFilesystemDestination() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(QDir(root.path()).mkdir(QStringLiteral("назначение")));
        const QString destination = root.filePath(QStringLiteral("назначение"));
        const QString source = root.filePath(QStringLiteral("архивный файл.txt"));
        QFile input(source);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("archive bytes"), qint64(13));
        input.close();
        const QString movedSource = root.filePath(QStringLiteral("для перемещения.txt"));
        QFile moveInput(movedSource);
        QVERIFY(moveInput.open(QIODevice::WriteOnly));
        QCOMPARE(moveInput.write("move bytes"), qint64(10));
        moveInput.close();
        const QString archive = root.filePath("source.zip");
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source, movedSource});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());

        MainWindow window(&backend);
        window.openArchive(archive);
        window.show();
        auto *left = window.findChild<QWidget *>("filePanel0");
        QVERIFY(left);
        auto *leftTable = left->findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(leftTable->rowCount(), 2);
        action(window, "twoPanelsAction")->trigger();
        const auto restorePanels = qScopeGuard([&] {
            if (action(window, "twoPanelsAction")->isChecked())
                action(window, "twoPanelsAction")->trigger();
        });
        auto *right = window.findChild<QWidget *>("filePanel1");
        QVERIFY(right);
        auto *rightTable = right->findChild<QTableWidget *>("archiveTable");
        window.activateWindow();
        QApplication::processEvents();
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        auto *rightPath = right->findChild<QLineEdit *>("archivePath");
        rightPath->setText(destination);
        QTest::keyClick(rightPath, Qt::Key_Return);
        QTRY_COMPARE(rightTable->rowCount(), 0);

        const auto leftRow = [leftTable](const QString &name) {
            for (int row = 0; row < leftTable->rowCount(); ++row)
                if (leftTable->item(row, 0)->text().normalized(QString::NormalizationForm_C) ==
                    name.normalized(QString::NormalizationForm_C))
                    return row;
            return -1;
        };
        const int copyRow = leftRow(QFileInfo(source).fileName());
        QStringList leftNames;
        for (int row = 0; row < leftTable->rowCount(); ++row)
            leftNames << leftTable->item(row, 0)->text();
        QVERIFY2(copyRow >= 0, qPrintable(leftNames.join(QStringLiteral(", "))));
        leftTable->selectRow(copyRow);
        leftTable->setFocus();
        QCoreApplication::processEvents();
        QVERIFY(action(window, "copyAction")->isEnabled());
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            QCOMPARE(QFileInfo(dialog->destination()).canonicalFilePath(),
                     QFileInfo(destination).canonicalFilePath());
            dialog->accept();
        });
        action(window, "copyAction")->trigger();
        const QString copied = QDir(destination).filePath(QFileInfo(source).fileName());
        QFile result(copied);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("archive bytes"));
        QTRY_COMPARE(rightTable->rowCount(), 1);
        QCOMPARE(rightTable->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                 QFileInfo(source).fileName().normalized(QString::NormalizationForm_C));
        QCOMPARE(leftTable->rowCount(), 2);

        const int moveRow = leftRow(QFileInfo(movedSource).fileName());
        QVERIFY(moveRow >= 0);
        leftTable->selectRow(moveRow);
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            dialog->accept();
        });
        action(window, "moveAction")->trigger();
        QTRY_COMPARE(rightTable->rowCount(), 2);
        QFile moved(QDir(destination).filePath(QFileInfo(movedSource).fileName()));
        QVERIFY(moved.open(QIODevice::ReadOnly));
        QCOMPARE(moved.readAll(), QByteArray("move bytes"));
        QTRY_COMPARE(leftTable->rowCount(), 1);
        QCOMPARE(leftTable->item(0, 0)->text().normalized(QString::NormalizationForm_C),
                 QFileInfo(source).fileName().normalized(QString::NormalizationForm_C));
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
        QVERIFY(copyInfo.startsWith("Files: 1    ( 8 bytes )\n\n"));
        QVERIFY(QDir::fromNativeSeparators(copyInfo).contains(temp.path() + '/'));
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
        QVERIFY(!settings.value("CopyHistory").toStringList().isEmpty());
        QCOMPARE(settings.value("CopyHistory").toStringList().first(), destination);
        QFile copiedFile(copied);
        QVERIFY(copiedFile.open(QIODevice::ReadOnly));
        QCOMPARE(copiedFile.readAll(), QByteArray("original"));
        copiedFile.close();
        QVERIFY(table(window)->selectionModel()->selectedRows().isEmpty());
        table(window)->selectRow(filesystemRow(window, QStringLiteral("имя с пробелом.txt")));
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
        table(window)->selectRow(filesystemRow(window, QStringLiteral("имя с пробелом.txt")));
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
        QVERIFY(table(window)->selectionModel()->selectedRows().isEmpty());
        QVERIFY(QFileInfo::exists(source));
        QTimer moveErrorTimer;
        table(window)->selectRow(filesystemRow(window, QStringLiteral("имя с пробелом.txt")));
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
        QVERIFY(table(window)->selectionModel()->selectedRows().isEmpty());
        QVERIFY(QFileInfo::exists(source));
        QVERIFY(QFileInfo::exists(copied));

        window.show();
        QCoreApplication::processEvents();
        const int copiedRow = filesystemRow(window, QStringLiteral("имя с пробелом.txt"));
        table(window)->selectRow(copiedRow);
        window.activateWindow();
        QCoreApplication::processEvents();
        table(window)->setFocus();
        QTRY_VERIFY(table(window)->hasFocus());
        table(window)->item(copiedRow, 1)->setText(temp.filePath("forged-rename.txt"));
        action(window, "renameAction")->trigger();
        auto *editor = qobject_cast<QLineEdit *>(QApplication::focusWidget());
        QVERIFY(editor);
        editor->setText(QStringLiteral("новое имя.txt"));
        QTest::keyClick(editor, Qt::Key_Return);
        const QString renamed = temp.filePath(QStringLiteral("новое имя.txt"));
        QTRY_VERIFY(QFileInfo::exists(renamed));
        QVERIFY(!QFileInfo::exists(source));

        QTRY_VERIFY(filesystemRow(window, QStringLiteral("новое имя.txt")) >= 0);
        table(window)->selectRow(filesystemRow(window, QStringLiteral("новое имя.txt")));
        QTRY_VERIFY(!table(window)->selectionModel()->selectedRows().isEmpty());
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
        QTRY_VERIFY(filesystemRow(window, "move me.txt") >= 0);
        table(window)->selectRow(filesystemRow(window, "move me.txt"));
        QTRY_VERIFY(!table(window)->selectionModel()->selectedRows().isEmpty());
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

    void stage7ArchiveCopyAndDropCancellation() {
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
        window.show();
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(source)});
        // Real filesystem-to-Add modals use native factories and are covered
        // by source_drop_add; this controlled archive fixture has no real file.
        openListedArchive(window, backend, "items.zip",
                          {ArchiveEntry{QStringLiteral("nested/file.txt"), 4, 4}});
        const int row = filesystemRow(window, "nested");
        QVERIFY(row >= 0);
        table(window)->selectRow(row);
        QString copyInfo;
        QTimer::singleShot(0, &window, [&temp, &copyInfo] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible()) {
                    if (auto *destination = dialog->findChild<QComboBox *>("copyDestination")) {
                        copyInfo =
                            dialog->findChild<QPlainTextEdit *>("copySelectionInfo")->toPlainText();
                        destination->setCurrentText(temp.filePath("out"));
                        dialog->accept();
                    }
                }
            }
        });
        action(window, "copyAction")->trigger();
        QVERIFY(QDir::fromNativeSeparators(copyInfo).contains(QStringLiteral("nested/")));
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(backend.lastExtractionOptions.pathMode, ExtractionOptions::PathMode::CurrentPaths);
        QCOMPARE(backend.lastSelected.size(), 1);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastSelected.front()),
                 QStringLiteral("nested/file.txt"));
        QVERIFY(!action(window, "copyAction")->isEnabled());
        backend.cancel();
        QVERIFY(action(window, "copyAction")->isEnabled());
        QDragEnterEvent archiveEnter(DragEventPosition(QPoint(5, 5)), Qt::CopyAction, &mime,
                                     Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table(window)->viewport(), &archiveEnter);
        QDropEvent archiveDrop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton,
                               Qt::NoModifier);
        QTimer cancelAdd;
        cancelAdd.setInterval(5);
        bool cancelRequested = false;
        connect(&cancelAdd, &QTimer::timeout, &window, [&] {
            if (auto *question = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                cancelRequested = true;
                question->button(QMessageBox::No)->click();
            }
        });
        cancelAdd.start();
        QCoreApplication::sendEvent(table(window)->viewport(), &archiveDrop);
        cancelAdd.stop();
        QCOMPARE(backend.addCalls, 0);
        QVERIFY(cancelRequested);
        QVERIFY(action(window, "copyAction")->isEnabled());
    }

    void diskPanelCopiesIntoOppositeArchive() {
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine.isEmpty(), "7zz required for disk-to-archive fixture");
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const bool hadSettings = qEnvironmentVariableIsSet("SEVENZIP_SETTINGS_PATH");
        const QByteArray previousSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const auto restoreSettings = qScopeGuard([hadSettings, previousSettings] {
            if (hadSettings)
                qputenv("SEVENZIP_SETTINGS_PATH", previousSettings);
            else
                qunsetenv("SEVENZIP_SETTINGS_PATH");
        });
        qputenv("SEVENZIP_SETTINGS_PATH", fixture.filePath("transfer-settings.ini").toUtf8());
        const QString disk = fixture.filePath("disk");
        const QString seed = fixture.filePath("seed");
        QVERIFY(QDir().mkpath(disk));
        QVERIFY(QDir().mkpath(seed));
        const QString archive = fixture.filePath("target.zip");
        const QString source = QDir(disk).filePath(QStringLiteral("новый файл.txt"));
        QFile incoming(source);
        QVERIFY(incoming.open(QIODevice::WriteOnly));
        QCOMPARE(incoming.write("source bytes"), qint64(12));
        incoming.close();
        QFile keep(QDir(seed).filePath("keep.txt"));
        QVERIFY(keep.open(QIODevice::WriteOnly));
        QCOMPARE(keep.write("keep"), qint64(4));
        keep.close();
        QProcess create;
        create.setWorkingDirectory(seed);
        create.start(engine, {"a", "-tzip", "--", archive, "keep.txt"});
        QVERIFY(create.waitForFinished(10000));
        QCOMPARE(create.exitCode(), 0);

        ArchiveBackend backend(nullptr, engine);
        MainWindow window(&backend);
        window.show();
        auto *left = window.findChild<QWidget *>("filePanel0");
        auto *leftAddress = left->findChild<QLineEdit *>("archivePath");
        auto *leftTable = left->findChild<QTableWidget *>("archiveTable");
        leftAddress->setText(disk);
        QTest::keyClick(leftAddress, Qt::Key_Return);
        int sourceRow = -1;
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                     for (int row = 0; row < leftTable->rowCount(); ++row)
                                         if (leftTable->item(row, 0)->text() ==
                                             QStringLiteral("новый файл.txt")) {
                                             sourceRow = row;
                                             return true;
                                         }
                                     return false;
                                 })(),
                                 10000);
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        auto *right = window.findChild<QWidget *>("filePanel1");
        QVERIFY(right);
        auto *rightTable = right->findChild<QTableWidget *>("archiveTable");
        window.activateWindow();
        QApplication::processEvents();
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        window.openArchive(archive);
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                     for (int row = 0; row < rightTable->rowCount(); ++row)
                                         if (rightTable->item(row, 0)->text() == "keep.txt")
                                             return true;
                                     return false;
                                 })(),
                                 10000);
        window.activateWindow();
        QApplication::processEvents();
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        QVERIFY(sourceRow >= 0);
        leftTable->setFocus();
        leftTable->selectRow(sourceRow);
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            QCOMPARE(QDir::cleanPath(dialog->destination()), QDir::cleanPath(archive));
            dialog->accept();
        });
        action(window, "copyAction")->trigger();
        QFile copied(source);
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("source bytes"));
        const QString extracted = fixture.filePath("extracted");
        QProcess extract;
        extract.start(engine, {"x", "-y", "-o" + extracted, "--", archive});
        QVERIFY(extract.waitForFinished(10000));
        QCOMPARE(extract.exitCode(), 0);
        QFile archived(QDir(extracted).filePath(QStringLiteral("новый файл.txt")));
        QVERIFY(archived.open(QIODevice::ReadOnly));
        QCOMPARE(archived.readAll(), QByteArray("source bytes"));
        QTRY_VERIFY(rightTable->rowCount() > 1);

        const QString movedSource = QDir(disk).filePath(QStringLiteral("move me.txt"));
        QFile moving(movedSource);
        QVERIFY(moving.open(QIODevice::WriteOnly));
        QCOMPARE(moving.write("move bytes"), qint64(10));
        moving.close();
        action(window, "refreshAction")->trigger();
        int moveRow = -1;
        QTRY_VERIFY_WITH_TIMEOUT(([&] {
                                     for (int row = 0; row < leftTable->rowCount(); ++row)
                                         if (leftTable->item(row, 0)->text() == "move me.txt") {
                                             moveRow = row;
                                             return true;
                                         }
                                     return false;
                                 })(),
                                 10000);
        leftTable->selectRow(moveRow);
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            QCOMPARE(QDir::cleanPath(dialog->destination()), QDir::cleanPath(archive));
            dialog->accept();
        });
        action(window, "moveAction")->trigger();
        QVERIFY(!QFileInfo::exists(movedSource));
        const QString movedExtracted = fixture.filePath("moved-extracted");
        QProcess verifyMove;
        verifyMove.start(engine, {"x", "-y", "-o" + movedExtracted, "--", archive});
        QVERIFY(verifyMove.waitForFinished(10000));
        QCOMPARE(verifyMove.exitCode(), 0);
        QFile moved(QDir(movedExtracted).filePath("move me.txt"));
        QVERIFY(moved.open(QIODevice::ReadOnly));
        QCOMPARE(moved.readAll(), QByteArray("move bytes"));
        QTRY_VERIFY(rightTable->rowCount() > 2);
    }

    void archivePanelsCopyAndMoveThroughStaging_data() {
        QTest::addColumn<bool>("russian");
        QTest::newRow("en") << false;
        QTest::newRow("ru") << true;
    }
    void archivePanelsCopyAndMoveThroughStaging() {
        QFETCH(bool, russian);
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine.isEmpty(), "7zz required for archive transfer fixture");
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const bool hadSettings = qEnvironmentVariableIsSet("SEVENZIP_SETTINGS_PATH");
        const QByteArray previousSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const auto restoreSettings = qScopeGuard([hadSettings, previousSettings] {
            if (hadSettings)
                qputenv("SEVENZIP_SETTINGS_PATH", previousSettings);
            else
                qunsetenv("SEVENZIP_SETTINGS_PATH");
        });
        const QString settings = fixture.filePath("transfer-settings.ini");
        qputenv("SEVENZIP_SETTINGS_PATH", settings.toUtf8());
        SaveRegLang(UString(russian ? "ru" : "-"), settings);
        const auto restoreLanguage = qScopeGuard([settings] {
            SaveRegLang(UString("-"), settings);
            ReloadLang(settings);
        });
        const QString sourceDir = fixture.filePath("source");
        const QString targetDir = fixture.filePath("target");
        QVERIFY(QDir().mkpath(sourceDir));
        QVERIFY(QDir().mkpath(targetDir));
        const QString sourceArchive = fixture.filePath("source.zip");
        const QString targetArchive = fixture.filePath("target.zip");
        for (const QString &name : {QStringLiteral("copy me.txt"), QStringLiteral("move me.txt")}) {
            QFile file(QDir(sourceDir).filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(name.toUtf8()), qint64(name.toUtf8().size()));
        }
        QFile keep(QDir(targetDir).filePath("keep.txt"));
        QVERIFY(keep.open(QIODevice::WriteOnly));
        QCOMPARE(keep.write("keep"), qint64(4));
        keep.close();
        QProcess createSource;
        createSource.setWorkingDirectory(sourceDir);
        createSource.start(engine,
                           {"a", "-tzip", "--", sourceArchive, "copy me.txt", "move me.txt"});
        QVERIFY(createSource.waitForFinished(10000));
        QCOMPARE(createSource.exitCode(), 0);
        QProcess createTarget;
        createTarget.setWorkingDirectory(targetDir);
        createTarget.start(engine, {"a", "-tzip", "--", targetArchive, "keep.txt"});
        QVERIFY(createTarget.waitForFinished(10000));
        QCOMPARE(createTarget.exitCode(), 0);

        ArchiveBackend backend(nullptr, engine);
        MainWindow window(&backend);
        window.show();
        auto *left = window.findChild<QWidget *>("filePanel0");
        auto *leftTable = left->findChild<QTableWidget *>("archiveTable");
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        window.openArchive(sourceArchive);
        auto *right = window.findChild<QWidget *>("filePanel1");
        QVERIFY(right);
        auto *rightTable = right->findChild<QTableWidget *>("archiveTable");
        window.activateWindow();
        QApplication::processEvents();
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        window.openArchive(targetArchive);
        QTRY_VERIFY_WITH_TIMEOUT(
            rightTable->rowCount() == 1 && rightTable->item(0, 0)->text() == "keep.txt", 10000);
        window.activateWindow();
        QApplication::processEvents();
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        const auto rowFor = [leftTable](const QString &name) {
            for (int row = 0; row < leftTable->rowCount(); ++row)
                if (leftTable->item(row, 0)->text() == name)
                    return row;
            return -1;
        };
        QTRY_VERIFY_WITH_TIMEOUT(rowFor("copy me.txt") >= 0, 10000);
        leftTable->selectRow(rowFor("copy me.txt"));
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            QCOMPARE(QDir::cleanPath(dialog->destination()), QDir::cleanPath(targetArchive));
            dialog->accept();
        });
        action(window, "copyAction")->trigger();
        QCOMPARE(window.statusBar()->currentMessage(),
                 russian ? QStringLiteral("Объекты скопированы в архив.")
                         : QString("Items copied into the archive."));
        QProcess verifyCopy;
        const QString copyOutput = fixture.filePath("copy-output");
        verifyCopy.start(engine, {"x", "-y", "-o" + copyOutput, "--", targetArchive});
        QVERIFY(verifyCopy.waitForFinished(10000));
        QCOMPARE(verifyCopy.exitCode(), 0);
        QFile copied(QDir(copyOutput).filePath("copy me.txt"));
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("copy me.txt"));
        QTRY_VERIFY_WITH_TIMEOUT(rowFor("move me.txt") >= 0, 10000);
        leftTable->selectRow(rowFor("move me.txt"));
        QFile targetBefore(targetArchive);
        QVERIFY(targetBefore.open(QIODevice::ReadOnly));
        const QByteArray beforeCanceledMove = targetBefore.readAll();
        targetBefore.close();
        bool approveDelete = false;
        QTimer answerDelete;
        connect(&answerDelete, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    if (auto *answer =
                            box->button(approveDelete ? QMessageBox::Yes : QMessageBox::No))
                        answer->click();
        });
        answerDelete.start(1);
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            dialog->accept();
        });
        action(window, "moveAction")->trigger();
        QFile targetAfterCancel(targetArchive);
        QVERIFY(targetAfterCancel.open(QIODevice::ReadOnly));
        QCOMPARE(targetAfterCancel.readAll(), beforeCanceledMove);
        targetAfterCancel.close();
        QVERIFY(rowFor("move me.txt") >= 0);
        approveDelete = true;
        leftTable->selectRow(rowFor("move me.txt"));
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            dialog->accept();
        });
        action(window, "moveAction")->trigger();
        answerDelete.stop();
        QCOMPARE(window.statusBar()->currentMessage(),
                 russian ? QStringLiteral("Объекты перемещены между архивами.")
                         : QString("Items moved between archives."));
        const QString movedOutput = fixture.filePath("move-output");
        QProcess verifyMove;
        verifyMove.start(engine, {"x", "-y", "-o" + movedOutput, "--", targetArchive});
        QVERIFY(verifyMove.waitForFinished(10000));
        QCOMPARE(verifyMove.exitCode(), 0);
        QFile moved(QDir(movedOutput).filePath("move me.txt"));
        QVERIFY(moved.open(QIODevice::ReadOnly));
        QCOMPARE(moved.readAll(), QByteArray("move me.txt"));
        QProcess listSource;
        listSource.start(engine, {"l", "-slt", "--", sourceArchive});
        QVERIFY(listSource.waitForFinished(10000));
        QCOMPARE(listSource.exitCode(), 0);
        QVERIFY(!listSource.readAllStandardOutput().contains("Path = move me.txt"));
    }

    void protectedArchivePanelsCopyWithStoredPasswords() {
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine.isEmpty(), "7zz required for protected archive transfer fixture");
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const bool hadSettings = qEnvironmentVariableIsSet("SEVENZIP_SETTINGS_PATH");
        const QByteArray previousSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const auto restoreSettings = qScopeGuard([hadSettings, previousSettings] {
            if (hadSettings)
                qputenv("SEVENZIP_SETTINGS_PATH", previousSettings);
            else
                qunsetenv("SEVENZIP_SETTINGS_PATH");
        });
        qputenv("SEVENZIP_SETTINGS_PATH", fixture.filePath("transfer-settings.ini").toUtf8());
        const QString sourceDir = fixture.filePath("source");
        const QString targetDir = fixture.filePath("target");
        QVERIFY(QDir().mkpath(sourceDir));
        QVERIFY(QDir().mkpath(targetDir));
        const QString sourceArchive = fixture.filePath("source.7z");
        const QString targetArchive = fixture.filePath("target.7z");
        const QString name = QStringLiteral("секрет с пробелом.txt");
        QFile incoming(QDir(sourceDir).filePath(name));
        QVERIFY(incoming.open(QIODevice::WriteOnly));
        QCOMPARE(incoming.write("secret bytes"), qint64(12));
        incoming.close();
        QFile keep(QDir(targetDir).filePath("keep.txt"));
        QVERIFY(keep.open(QIODevice::WriteOnly));
        QCOMPARE(keep.write("keep"), qint64(4));
        keep.close();
        const QString password = QStringLiteral("fixture-only-password");
        QProcess createSource;
        createSource.setWorkingDirectory(sourceDir);
        createSource.start(engine,
                           {"a", "-t7z", "-p" + password, "-mhe=on", "--", sourceArchive, name});
        QVERIFY(createSource.waitForFinished(10000));
        QCOMPARE(createSource.exitCode(), 0);
        QProcess createTarget;
        createTarget.setWorkingDirectory(targetDir);
        createTarget.start(
            engine, {"a", "-t7z", "-p" + password, "-mhe=on", "--", targetArchive, "keep.txt"});
        QVERIFY(createTarget.waitForFinished(10000));
        QCOMPARE(createTarget.exitCode(), 0);

        ArchiveBackend backend(nullptr, engine);
        MainWindow window(&backend);
        int passwordPrompts = 0;
        QString unexpectedDialog;
        QTimer answerPassword;
        connect(&answerPassword, &QTimer::timeout, &window, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            if (auto *box = qobject_cast<QMessageBox *>(dialog)) {
                unexpectedDialog = box->text();
                box->accept();
                return;
            }
            if (dialog->objectName() != "passwordDialog")
                return;
            ++passwordPrompts;
            dialog->findChild<QLineEdit *>("passwordEdit")->setText(password);
            dialog->accept();
        });
        answerPassword.start(1);
        window.show();
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        auto *left = window.findChild<QWidget *>("filePanel0");
        auto *leftTable = left->findChild<QTableWidget *>("archiveTable");
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        window.openArchive(sourceArchive);
        auto *right = window.findChild<QWidget *>("filePanel1");
        auto *rightTable = right->findChild<QTableWidget *>("archiveTable");
        window.activateWindow();
        QApplication::processEvents();
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        window.openArchive(targetArchive);
        QTRY_VERIFY_WITH_TIMEOUT(
            rightTable->rowCount() == 1 && rightTable->item(0, 0)->text() == "keep.txt", 10000);
        window.activateWindow();
        QApplication::processEvents();
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        QCOMPARE(leftTable->rowCount(), 1);
        leftTable->selectRow(0);
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            QCOMPARE(QDir::cleanPath(dialog->destination()), QDir::cleanPath(targetArchive));
            dialog->accept();
        });
        action(window, "copyAction")->trigger();
        answerPassword.stop();
        QVERIFY2(unexpectedDialog.isEmpty(), qPrintable(unexpectedDialog));
        QVERIFY(passwordPrompts >= 2);
        QTRY_COMPARE(rightTable->rowCount(), 2);
        const QString output = fixture.filePath("output");
        QProcess extract;
        extract.start(engine, {"x", "-y", "-p" + password, "-o" + output, "--", targetArchive});
        QVERIFY(extract.waitForFinished(10000));
        QCOMPARE(extract.exitCode(), 0);
        QFile copied(QDir(output).filePath(name));
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("secret bytes"));
    }

    void nestedArchivePanelCannotWriteIntoOppositeArchive() {
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY2(!engine.isEmpty(), "7zz required for nested archive transfer fixture");
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const bool hadSettings = qEnvironmentVariableIsSet("SEVENZIP_SETTINGS_PATH");
        const QByteArray previousSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        const auto restoreSettings = qScopeGuard([hadSettings, previousSettings] {
            if (hadSettings)
                qputenv("SEVENZIP_SETTINGS_PATH", previousSettings);
            else
                qunsetenv("SEVENZIP_SETTINGS_PATH");
        });
        qputenv("SEVENZIP_SETTINGS_PATH", fixture.filePath("transfer-settings.ini").toUtf8());
        const QString innerDir = fixture.filePath("inner");
        const QString outerDir = fixture.filePath("outer");
        const QString targetDir = fixture.filePath("target");
        QVERIFY(QDir().mkpath(innerDir));
        QVERIFY(QDir().mkpath(outerDir));
        QVERIFY(QDir().mkpath(targetDir));
        const QString innerArchive = QDir(outerDir).filePath("inner.zip");
        const QString outerArchive = fixture.filePath("outer.zip");
        const QString targetArchive = fixture.filePath("target.zip");
        QFile payload(QDir(innerDir).filePath("payload.txt"));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QCOMPARE(payload.write("inside"), qint64(6));
        payload.close();
        QFile keep(QDir(targetDir).filePath("keep.txt"));
        QVERIFY(keep.open(QIODevice::WriteOnly));
        QCOMPARE(keep.write("keep"), qint64(4));
        keep.close();
        const auto create = [&](const QString &root, const QString &archive, const QString &entry) {
            QProcess process;
            process.setWorkingDirectory(root);
            process.start(engine, {"a", "-tzip", "--", archive, entry});
            return process.waitForFinished(10000) && process.exitCode() == 0;
        };
        QVERIFY(create(innerDir, innerArchive, "payload.txt"));
        QVERIFY(create(outerDir, outerArchive, "inner.zip"));
        QVERIFY(create(targetDir, targetArchive, "keep.txt"));
        QFile before(targetArchive);
        QVERIFY(before.open(QIODevice::ReadOnly));
        const QByteArray originalTarget = before.readAll();
        before.close();

        ArchiveBackend backend(nullptr, engine);
        MainWindow window(&backend);
        window.show();
        action(window, "twoPanelsAction")->trigger();
        QVERIFY(action(window, "twoPanelsAction")->isChecked());
        auto *left = window.findChild<QWidget *>("filePanel0");
        auto *leftTable = left->findChild<QTableWidget *>("archiveTable");
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        window.openArchive(outerArchive);
        QTRY_VERIFY(leftTable->rowCount() == 1 && leftTable->item(0, 0)->text() == "inner.zip");
        auto *right = window.findChild<QWidget *>("filePanel1");
        auto *rightTable = right->findChild<QTableWidget *>("archiveTable");
        window.activateWindow();
        QApplication::processEvents();
        rightTable->setFocus();
        QTRY_VERIFY(rightTable->hasFocus());
        window.openArchive(targetArchive);
        QTRY_VERIFY(rightTable->rowCount() == 1 && rightTable->item(0, 0)->text() == "keep.txt");
        window.activateWindow();
        QApplication::processEvents();
        leftTable->setFocus();
        QTRY_VERIFY(leftTable->hasFocus());
        leftTable->selectRow(0);
        action(window, "openInsideAction")->trigger();
        QTRY_VERIFY(leftTable->rowCount() == 1 && leftTable->item(0, 0)->text() == "payload.txt");
        leftTable->selectRow(0);
        QString warning;
        QTimer answerWarning;
        connect(&answerWarning, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    warning = box->text();
                    box->accept();
                }
        });
        answerWarning.start(1);
        bool copyPresented = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            const auto rejectUnanswered = qScopeGuard([&] {
                if (dialog->isVisible())
                    dialog->reject();
            });
            copyPresented = true;
            QCOMPARE(QDir::cleanPath(QDir::fromNativeSeparators(dialog->destination())),
                     QDir::cleanPath(targetArchive));
            dialog->accept();
        });
        QVERIFY(action(window, "copyAction")->isEnabled());
        action(window, "copyAction")->trigger();
        answerWarning.stop();
        QVERIFY(copyPresented);
        QCOMPARE(warning, QString("Transfer between these archives is unavailable."));
        QFile after(targetArchive);
        QVERIFY(after.open(QIODevice::ReadOnly));
        QCOMPARE(after.readAll(), originalTarget);
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
        QVERIFY(action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "moveAction")->isEnabled());
        const int extractCalls = backend.extractCalls;
        const int removeCalls = backend.removeCalls;
        action(window, "copyAction")->trigger();
        action(window, "moveAction")->trigger();
        QCOMPARE(backend.extractCalls, extractCalls);
        QCOMPARE(backend.removeCalls, removeCalls);
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(backend.prepareArchiveFixture("cancel.7z"));
        backend.queueListAction([&] { backend.cancel(); });
        window.openArchive("cancel.7z");
        QTRY_VERIFY(static_cast<FileTable *>(table(window))->sourcePanel->IsFSFolder());
        QTRY_VERIFY(action(window, "copyAction")->isEnabled());
        QCOMPARE(backend.extractCalls, extractCalls);
        QCOMPARE(backend.removeCalls, removeCalls);
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
        QVERIFY(moveInfo.startsWith("Folders: 1    ( 4 bytes )\n\n"));
        QVERIFY(moveInfo.endsWith(QStringLiteral("  папка") + QDir::separator()));
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(backend.lastSelected.size(), 1);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastSelected.front()),
                 QStringLiteral("папка/файл с пробелом.txt"));
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
                    const bool expectedText =
                        box->text().contains("Files extracted but not deleted");
                    box->accept();
                    QVERIFY(expectedText);
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
        QVERIFY(action(window, "moveAction")->isEnabled());
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

        const QString copyOutput = temp.filePath(QStringLiteral("F5 назначение"));
        QVERIFY(QDir().mkpath(copyOutput));
        table(window)->selectRow(filesystemRow(window, fileName));
        QString summary;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            summary = dialog->findChild<QPlainTextEdit *>("copySelectionInfo")->toPlainText();
            dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(copyOutput);
            dialog->accept();
        });
        action(window, "copyAction")->trigger();
        QVERIFY(summary.contains(fileName));
        QFile copied(QDir(copyOutput).filePath(fileName));
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("new bytes"));
        QVERIFY(!QFileInfo::exists(QDir(copyOutput).filePath(folderName + '/' + fileName)));

        const QString moveOutput = temp.filePath(QStringLiteral("F6 назначение"));
        QVERIFY(QDir().mkpath(moveOutput));
        QFile moveConflict(QDir(moveOutput).filePath(fileName));
        QVERIFY(moveConflict.open(QIODevice::WriteOnly));
        QCOMPARE(moveConflict.write("keep bytes"), qint64(10));
        moveConflict.close();
        QTimer cancelPrompt;
        connect(&cancelPrompt, &QTimer::timeout, &window, [&] {
            if (auto *dialog = window.findChild<CopyDialog *>("copyDialog");
                dialog && dialog->isVisible()) {
                dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(moveOutput);
                dialog->accept();
            }
            if (auto *dialog = dynamic_cast<COverwriteDialog *>(QApplication::activeModalWidget()))
                dialog->findChild<QPushButton *>("overwriteCancel")->click();
        });
        cancelPrompt.start(1);
        action(window, "moveAction")->trigger();
        cancelPrompt.stop();
        QVERIFY(moveConflict.open(QIODevice::ReadOnly));
        QCOMPARE(moveConflict.readAll(), QByteArray("keep bytes"));
        moveConflict.close();
        QVERIFY(!table(window)->findItems(fileName, Qt::MatchExactly).isEmpty());
        QVERIFY(QFile::remove(moveConflict.fileName()));
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CopyDialog *>("copyDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(moveOutput);
            dialog->accept();
        });
        action(window, "moveAction")->trigger();
        QFile moved(QDir(moveOutput).filePath(fileName));
        QVERIFY(moved.open(QIODevice::ReadOnly));
        QCOMPARE(moved.readAll(), QByteArray("new bytes"));
        QVERIFY(!QFileInfo::exists(QDir(moveOutput).filePath(folderName + '/' + fileName)));
        QTRY_VERIFY(table(window)->findItems(fileName, Qt::MatchExactly).isEmpty());
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
                                if (close && close->text().contains(QStringLiteral("Close")))
                                    close->click();
                            });
                        }
                    }
                    message->accept();
                }
            }
            auto *progress = window.findChild<QDialog *>("progressDialog");
            auto *close = progress ? progress->findChild<QPushButton *>("IDCANCEL") : nullptr;
            if (close && close->text().contains(QStringLiteral("Close"))) {
                sawFailure = true;
                if (!closeScheduled) {
                    closeScheduled = true;
                    QTimer::singleShot(50, &window, [&window] {
                        auto *progress = window.findChild<QDialog *>("progressDialog");
                        auto *close =
                            progress ? progress->findChild<QPushButton *>("IDCANCEL") : nullptr;
                        if (close && close->text().contains(QStringLiteral("Close")))
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
    void encryptedArchiveMutationReusesSourcePassword() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("секретный файл.txt"));
        const QString archive = temp.filePath("protected.7z");
        const QString password = QStringLiteral("fixture-only-password");
        QFile input(source);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("payload"), qint64(7));
        input.close();
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source}, password, true);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        MainWindow window(&backend);
        int passwordPrompts = 0;
        QTimer passwordAnswer;
        connect(&passwordAnswer, &QTimer::timeout, &window, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "passwordDialog")
                return;
            ++passwordPrompts;
            dialog->findChild<QLineEdit *>("passwordEdit")->setText(password);
            dialog->accept();
        });
        passwordAnswer.start(1);
        window.openArchive(archive);
        window.show();
        QCoreApplication::processEvents();
        QTRY_VERIFY(action(window, "createFolderAction")->isEnabled());
        QTimer::singleShot(0, &window, [&window] {
            auto *dialog = window.findChild<CComboDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")
                ->setCurrentText(QStringLiteral("закрытая папка"));
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
        QTRY_VERIFY(archiveRow(QStringLiteral("закрытая папка")) >= 0);
        table(window)->selectRow(archiveRow(QStringLiteral("секретный файл.txt")));
        QTRY_VERIFY(action(window, "renameAction")->isEnabled());
        action(window, "renameAction")->trigger();
        auto *editor = table(window)->findChild<QLineEdit *>();
        QVERIFY(editor);
        editor->setFocus();
        editor->setText(QStringLiteral("новый секрет.txt"));
        QTest::keyClick(editor, Qt::Key_Return);
        QTRY_VERIFY(archiveRow(QStringLiteral("новый секрет.txt")) >= 0);
        table(window)->selectRow(archiveRow(QStringLiteral("новый секрет.txt")));
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->button(QMessageBox::Yes)->click();
        });
        action(window, "deleteAction")->trigger();
        QTRY_VERIFY(archiveRow(QStringLiteral("новый секрет.txt")) < 0);
        QCOMPARE(passwordPrompts, 1);
        passwordAnswer.stop();
    }
    void addToOpenArchiveState() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("новый файл.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("data");
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        const QString archive = temp.filePath("existing.zip");
        QVERIFY(backend.prepareArchiveFixture(archive));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QVERIFY(!action(window, "toolbarAddAction")->isEnabled());
        });
        window.openArchive(archive);
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());

        bool dialogSeen = false;
        bool unsupported = false;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            if (auto *dialog =
                    qobject_cast<CreateArchiveDialog *>(QApplication::activeModalWidget())) {
                dialogSeen = true;
                dialog->reject();
            } else if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                unsupported = true;
                box->accept();
            }
        });
        answer.start(1);
        action(window, "toolbarAddAction")->trigger();
        QVERIFY(!dialogSeen);
        QVERIFY(unsupported);
        QCOMPARE(backend.addCalls, 0);
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
    }
    void createDialogAndStateTransitions() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Source Add requires the official native module");
#endif
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString file =
            QFileInfo(temp.filePath(QStringLiteral("файл с пробелом.txt"))).absoluteFilePath();
        QFile source(file);
        QVERIFY(source.open(QIODevice::WriteOnly));
        source.write("data");
        source.close();
        const QString folder = temp.filePath(QStringLiteral("папка Unicode"));
        QVERIFY(QDir().mkpath(folder));
        const QString canonicalFile = QFileInfo(file).canonicalFilePath();
        const QString canonicalFolder = QFileInfo(folder).canonicalFilePath();
        ControlledBackend backend;
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog probe(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(
            backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), probe.version()),
            S_OK);
#endif
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        for (int row = 0; row < table(window)->rowCount(); ++row) {
            if (table(window)->item(row, 1)->text() == canonicalFile)
                table(window)->selectRow(row);
        }

        int initialSelectionCount = -1;
        QStringList addErrors;
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            if (auto *dialog =
                    qobject_cast<CreateArchiveDialog *>(QApplication::activeModalWidget())) {
                initialSelectionCount = dialog->sources().size();
                dialog->reject();
            } else if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                addErrors.append(box->text());
                box->accept();
            }
        });
        answer.start(1);
        action(window, "toolbarAddAction")->trigger();
        answer.stop();
        QVERIFY2(addErrors.isEmpty(), qPrintable(addErrors.join('\n')));
        QCOMPARE(backend.createCalls, 0);
        QCOMPARE(initialSelectionCount, 1);

        auto chooseSources = [&] {
            table(window)->clearSelection();
            for (int row = 0; row < table(window)->rowCount(); ++row) {
                const QString path = table(window)->item(row, 1)->text();
                if (path == canonicalFile || path == canonicalFolder)
                    table(window)->selectionModel()->select(table(window)->model()->index(row, 0),
                                                            QItemSelectionModel::Select |
                                                                QItemSelectionModel::Rows);
            }
            QTimer::singleShot(0, &window, [&] {
                auto *dialog = window.findChild<CreateArchiveDialog *>("createArchiveDialog");
                QVERIFY(dialog);
                const auto reject = qScopeGuard([&] { dialog->reject(); });
                QVERIFY(!dialog->findChild<QListWidget *>("availableSources"));
                if (dialog->sources().size() != 2) {
                    dialog->reject();
                    return;
                }
                QStringList sourcePaths = dialog->sources();
                for (QString &path : sourcePaths)
                    path = path.normalized(QString::NormalizationForm_C);
                QVERIFY(
                    sourcePaths.contains(canonicalFile.normalized(QString::NormalizationForm_C)));
                QVERIFY(
                    sourcePaths.contains(canonicalFolder.normalized(QString::NormalizationForm_C)));
                dialog->reject();
            });
            action(window, "toolbarAddAction")->trigger();
        };
        chooseSources();
        // Source UpdateGUI owns the modal worker; Add must not use the
        // independently written asynchronous create transport. Actual native
        // success/error/Cancel are covered by shell_update_window.
        QCOMPARE(backend.createCalls, 0);
        QVERIFY(table(window)->isEnabled());
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        QCOMPARE(table(window)->rowCount(), 2);
    }
};

QTEST_MAIN(MainWindowOperationsTests)
#include "ui_operations_tests.moc"
