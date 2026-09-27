#include "fileviewerdialog.h"
#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class ViewerBackend : public ArchiveBackend {
  public:
    ViewerBackend() : ArchiveBackend(nullptr, "/missing/7zz") {}
    void list(const QString &, const QString & = {}) override {
        QTimer::singleShot(0, this, [this] {
            emit entriesReady({ArchiveEntry{"folder/имя с пробелом.txt", 6, 6, {}, false, false}});
            emit finished(true, "Архив открыт.");
        });
    }
    void extract(const QString &, const QString &destination, const QStringList &selected,
                 const QString & = {}) override {
        extracted = selected;
        temporary = destination;
        QDir(destination).mkpath("folder");
        QFile output(QDir(destination).filePath(selected.first()));
        if (!output.open(QIODevice::WriteOnly))
            return;
        output.write("hello\n");
        output.close();
        QTimer::singleShot(0, this, [this] { emit finished(true, "Извлечено."); });
    }
    QStringList extracted;
    QString temporary;
};

class FileViewerTests : public QObject {
    Q_OBJECT

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
        QVERIFY(!edit->isEnabled());

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
