#include "fileviewerdialog.h"
#include "mainwindow.h"
#include "window_state_support.h"

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class ViewerBackend : public ControlledBackend {
  public:
    ViewerBackend() = default;
    void list(const QString &, const QString & = {}) override {
        QTimer::singleShot(0, this, [this] {
            ArchiveInfo info;
            info.format = QStringLiteral("7z");
            emit infoReady(info);
            QList<ArchiveEntry> entries = {
                ArchiveEntry{"folder/имя с пробелом.txt", 6, 6, {}, false, false}};
            entries.append(additionalEntries);
            emit entriesReady(entries);
            emit finished(true, "Архив открыт.");
        });
    }
    void extractSourceEntry(const QString &, const QString &entry, const QString &output,
                            const QString &, const QString &) override {
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
        QTimer::singleShot(0, this, [this] { completeExtraction(); });
    }
    void extract(const QString &, const QString &destination, const QStringList &selected,
                 const QString & = {}, const ExtractionOptions & = {}) override {
        extracted = selected;
        temporary = destination;
        if (cancelExtraction) {
            QTimer::singleShot(
                0, this, [this] { emit finished(false, QStringLiteral("Операция отменена.")); });
            return;
        }
        QDir(destination).mkpath("folder");
        QFile output(QDir(destination).filePath(selected.first()));
        if (!output.open(QIODevice::WriteOnly))
            return;
        output.write("hello\n");
        output.close();
        QTimer::singleShot(0, this, [this] { emit finished(true, "Извлечено."); });
    }
    void replaceEntry(const QString &, const QString &entry, const QString &root) override {
        replaced = entry;
        QFile file(QDir(root).filePath(entry));
        if (file.open(QIODevice::ReadOnly))
            replacement = file.readAll();
        QTimer::singleShot(0, this, [this] {
            emit finished(replacementOutcome == 0,
                          replacementOutcome == 2   ? QStringLiteral("Операция отменена.")
                          : replacementOutcome == 1 ? QStringLiteral("Fixture update failed.")
                                                    : QStringLiteral("Обновлено."));
        });
    }
    QStringList extracted;
    QString temporary;
    QString replaced;
    QByteArray replacement;
    bool cancelExtraction = false;
    QList<ArchiveEntry> additionalEntries;
    int replacementOutcome = 0;
};

class FileViewerTests : public QObject {
    Q_OBJECT

    static int rowWithPath(QTableWidget *table, const QString &path) {
        for (int row = 0; row < table->rowCount(); ++row)
            if (table->item(row, 0)->data(Qt::UserRole).toString() == path)
                return row;
        return -1;
    }

  private slots:
    void editorSavesUtf8AndRejectsBinary() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("имя с пробелом.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("hello\n");
        file.close();

        FileViewerDialog editor(path, true);
        auto *text = editor.findChild<QPlainTextEdit *>("fileViewerText");
        auto *save = editor.findChild<QPushButton *>("fileViewerSave");
        QVERIFY(text && save);
        QCOMPARE(text->toPlainText(), QString("hello\n"));
        text->selectAll();
        text->insertPlainText(QStringLiteral("Привет мир\n"));
        save->click();
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QStringLiteral("Привет мир\n").toUtf8());
        file.close();

        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(QByteArray("a\0b", 3));
        file.close();
        FileViewerDialog binary(path, true);
        QVERIFY(binary.findChild<QPlainTextEdit *>("fileViewerText")->isReadOnly());
        QVERIFY(!binary.findChild<QPushButton *>("fileViewerSave")->isEnabled());
    }

    void archiveViewExtractsOnlyFocusedFileAndCleansUp() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ViewerBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(backend.prepareArchiveFixture(dir.filePath("test.7z")));
        window.openArchive(dir.filePath("test.7z"));
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *view = window.findChild<QAction *>("viewFileAction");
        auto *edit = window.findChild<QAction *>("editFileAction");
        QVERIFY(table && view && edit);
        QTRY_COMPARE(table->rowCount(), 1);
        QTRY_VERIFY(table->item(0, 0)->data(Qt::UserRole + 1).toBool());
        QTimer::singleShot(0, &window, [&] { table->cellDoubleClicked(0, 0); });
        QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("имя с пробелом.txt"));
        table->selectRow(0);
        QTRY_VERIFY(view->isEnabled());
        QVERIFY(edit->isEnabled());

        table->item(0, 0)->setData(Qt::UserRole, QString("forged/view-only.txt"));
        table->item(0, 0)->setData(Qt::UserRole + 1, true);

        bool displayed = false;
        QTimer::singleShot(0, &window, [&] {
            QTimer::singleShot(30, &window, [&] {
                auto *dialog = window.findChild<FileViewerDialog *>("fileViewerDialog");
                if (dialog) {
                    displayed =
                        dialog->findChild<QPlainTextEdit *>("fileViewerText")->toPlainText() ==
                        "hello\n";
                    dialog->close();
                }
            });
        });
        view->trigger();
        QTRY_VERIFY(displayed);
        QCOMPARE(backend.extracted, QStringList({"folder/имя с пробелом.txt"}));
        QTRY_VERIFY(!QFileInfo::exists(backend.temporary));
    }

    void archiveEditWritesBackSavedText_data() {
        QTest::addColumn<int>("outcome");
        QTest::newRow("success") << 0;
        QTest::newRow("failure") << 1;
        QTest::newRow("cancel") << 2;
    }

    void archiveEditWritesBackSavedText() {
        QFETCH(int, outcome);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ViewerBackend backend;
        backend.additionalEntries = {{"folder/z current.txt", 1, 1}};
        backend.replacementOutcome = outcome;
        MainWindow window(&backend);
        window.show();
        QVERIFY(backend.prepareArchiveFixture(dir.filePath("test.7z")));
        window.openArchive(dir.filePath("test.7z"));
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *edit = window.findChild<QAction *>("editFileAction");
        QVERIFY(table && edit);
        QTRY_COMPARE(table->rowCount(), 1);
        emit table->cellDoubleClicked(0, 0);
        QTRY_COMPARE(table->rowCount(), 2);
        const int editedRow = rowWithPath(table, QStringLiteral("folder/имя с пробелом.txt"));
        const int focusedRow = rowWithPath(table, "folder/z current.txt");
        QVERIFY(editedRow >= 0 && focusedRow >= 0);
        table->selectRow(editedRow);
        table->setCurrentCell(focusedRow, 0, QItemSelectionModel::NoUpdate);
        QTRY_VERIFY(edit->isEnabled());
        const auto editedIndex = table->item(editedRow, 0)->data(Qt::UserRole + 6);
        table->item(editedRow, 0)->setData(Qt::UserRole, QString("forged/edit-only.txt"));
        table->item(editedRow, 0)->setData(Qt::UserRole + 1, true);
        QTimer answer;
        connect(&answer, &QTimer::timeout, &window, [&] {
            if (auto *question = window.findChild<QMessageBox *>()) {
                if (question->isVisible())
                    question
                        ->button(question->standardButtons().testFlag(QMessageBox::Yes)
                                     ? QMessageBox::Yes
                                     : QMessageBox::Ok)
                        ->click();
            }
        });
        answer.start(10);
        QTimer::singleShot(30, &window, [&] {
            auto *dialog = window.findChild<FileViewerDialog *>("fileViewerDialog");
            QVERIFY(dialog);
            auto *text = dialog->findChild<QPlainTextEdit *>("fileViewerText");
            text->selectAll();
            text->insertPlainText(QStringLiteral("новый текст\n"));
            dialog->findChild<QPushButton *>("fileViewerSave")->click();
            dialog->accept();
        });
        edit->trigger();
        QTRY_COMPARE(backend.replaced, QStringLiteral("folder/имя с пробелом.txt"));
        QTRY_VERIFY(edit->isEnabled());
        answer.stop();
        QCOMPARE(backend.replacement, QStringLiteral("новый текст\n").toUtf8());
        QTRY_VERIFY(!QFileInfo::exists(backend.temporary));
        QTRY_COMPARE(table->rowCount(), 2);
        QTRY_COMPARE(table->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table->item(table->currentRow(), 0)->data(Qt::UserRole).toString(),
                 QString("folder/z current.txt"));
        QCOMPARE(table->item(table->selectionModel()->selectedRows()[0].row(), 0)
                     ->data(Qt::UserRole + 6),
                 editedIndex);
        QVERIFY(!table->selectionModel()->isRowSelected(table->currentRow(), {}));
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
            ViewerBackend backend;
            MainWindow window(&backend, [&](const QUrl &url) {
                opened = url;
                return true;
            });
            window.show();
            auto *address = window.findChild<QLineEdit *>("archivePath");
            auto *table = window.findChild<QTableWidget *>("archiveTable");
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
            emit table->cellDoubleClicked(0, 0);
            QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("имя с пробелом.txt"));
            table->selectRow(0);
            QTRY_VERIFY(open->isEnabled());
            open->trigger();
            QTRY_VERIFY(!opened.isEmpty());
            QCOMPARE(backend.extracted, QStringList({"folder/имя с пробелом.txt"}));
            // Native CopyTo owns the final file separately from the transport
            // stream's scratch directory, which is released on completion.
            extractedDirectory = QFileInfo(opened.toLocalFile()).absolutePath();
            QVERIFY(QFileInfo::exists(opened.toLocalFile()));
            QVERIFY(QFileInfo::exists(extractedDirectory));
        }
        QVERIFY(!QFileInfo::exists(extractedDirectory));
    }

    void cancelledArchiveOpenDoesNotLaunchFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ViewerBackend backend;
        backend.cancelExtraction = true;
        int launches = 0;
        MainWindow window(&backend, [&](const QUrl &) {
            ++launches;
            return true;
        });
        QVERIFY(backend.prepareArchiveFixture(dir.filePath("test.7z")));
        window.openArchive(dir.filePath("test.7z"));
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *open = window.findChild<QAction *>("openItemAction");
        QVERIFY(table && open);
        QTRY_COMPARE(table->rowCount(), 1);
        emit table->cellDoubleClicked(0, 0);
        QTRY_COMPARE(table->item(0, 0)->text(), QStringLiteral("имя с пробелом.txt"));
        table->selectRow(0);
        QTRY_VERIFY(open->isEnabled());
        open->trigger();
        QTRY_VERIFY(!backend.temporary.isEmpty());
        QTRY_VERIFY(!QFileInfo::exists(backend.temporary));
        QCOMPARE(launches, 0);
    }

    void filesystemEditUsesFileMenuAction() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("правка.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("before\n");
        file.close();

        MainWindow window;
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *edit = window.findChild<QAction *>("editFileAction");
        QVERIFY(address && table && edit);
        address->setText(dir.path());
        emit address->returnPressed();
        QTRY_COMPARE(table->rowCount(), 1);
        table->selectRow(0);
        QTRY_VERIFY(edit->isEnabled());

        table->item(0, 1)->setText(dir.filePath("forged-editor.txt"));
        table->item(0, 0)->setData(Qt::UserRole + 1, true);

        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<FileViewerDialog *>("fileViewerDialog");
            QVERIFY(dialog);
            auto *text = dialog->findChild<QPlainTextEdit *>("fileViewerText");
            text->selectAll();
            text->insertPlainText(QStringLiteral("после\n"));
            dialog->findChild<QPushButton *>("fileViewerSave")->click();
            dialog->close();
        });
        edit->trigger();
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QStringLiteral("после\n").toUtf8());
    }
};

QTEST_MAIN(FileViewerTests)
#include "file_viewer_tests.moc"
