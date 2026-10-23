#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "fixtures/widget_dialogs.h"
#include "mainwindow.h"
#include <QLineEdit>

#include <QCryptographicHash>
#include <QFile>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QTreeWidget>
#include <QtTest>

class ChecksumTests : public QObject {
    Q_OBJECT
    WidgetDialogFixture widgetDialogs_;

    static QString messageText(const QTreeWidget *list) {
        QString text;
        for (int i = 0; i < list->topLevelItemCount(); ++i)
            text += list->topLevelItem(i)->text(1) + '\n';
        return text;
    }

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void backendHashesFileAndRejectsInvalidMethod() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("hello"), qint64(5));
        file.close();
        ArchiveBackend backend;
        QString output;
        connect(&backend, &ArchiveBackend::outputReceived, this,
                [&](const QString &line) { output += line + '\n'; });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.hash({path}, "SHA256");
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QVERIFY(output.contains(QString::fromLatin1(
            QCryptographicHash::hash("hello", QCryptographicHash::Sha256).toHex())));
        QVERIFY(output.contains(QStringLiteral("данные с пробелом.txt")));
        backend.hash({path}, "unsupported");
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished.takeFirst().first().toBool());
    }

    void cancellationStopsSourceHashWorker() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("input.txt");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("content");
        file.close();
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy engineStarted(&backend, &ArchiveBackend::engineStarted);
        backend.hash({path}, "SHA256");
        QVERIFY(backend.busy());
        QCOMPARE(backend.setPaused(true), S_OK);
        backend.cancel();
        QVERIFY(finished.wait(30000));
        QVERIFY(!finished.takeFirst().first().toBool());
        QVERIFY(!backend.busy());
        QCOMPARE(backend.lastResult(), E_ABORT);
        QCOMPARE(engineStarted.size(), 0);
    }

    void filesystemAndArchiveMenusHashSelectedFile() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString original = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temporary.path()));
        const QString path = temporary.filePath(QStringLiteral("файл с пробелом.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("content"), qint64(7));
        file.close();
        ArchiveBackend backend;
        const QString archive = temporary.filePath("sample.7z");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {path});
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());

        MainWindow window(&backend);
        window.show();
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("hashSHA256Action");
        QVERIFY(table && action);
        QVERIFY(!action->isEnabled());
        for (int row = 0; row < table->rowCount(); ++row) {
            if (table->item(row, 0)->text() == QFileInfo(path).fileName()) {
                table->selectRow(row);
                break;
            }
        }
        QVERIFY(action->isEnabled());
        const QString digest = QString::fromLatin1(
            QCryptographicHash::hash("content", QCryptographicHash::Sha256).toHex());
        QString results;
        QTimer closeResults;
        connect(&closeResults, &QTimer::timeout, this, [&] {
            auto *dialog = qobject_cast<CListViewDialog *>(widgetDialogs_.activeDialog());
            if (!dialog)
                return;
            results = messageText(dialog->findChild<QTreeWidget *>("propertiesList"));
            dialog->accept();
        });
        closeResults.start(10);
        action->trigger(); // Source modal scan/hash/results has already joined its worker.
        QVERIFY(results.contains(digest));
        QCOMPARE(finished.size(), 0); // No process hashing or temporary extraction for FS.

        window.openArchive(archive);
        QCOMPARE(finished.size(), 1); // Source modal opening has already joined its worker.
        QVERIFY(finished.takeFirst().first().toBool());
        QTRY_COMPARE(table->rowCount(), 1);
        table->selectRow(0);
        QVERIFY(action->isEnabled());
        results.clear();
        action->trigger();
        QVERIFY(results.contains(digest));
        QCOMPARE(finished.size(), 1); // Engine stream transport; no second CLI hash job.
        QVERIFY(finished.takeFirst().first().toBool());
        closeResults.stop();
        QVERIFY(QDir::setCurrent(original));
    }

    void archiveHashErrorsAndPasswordCancellation_data() {
        QTest::addColumn<QString>("outcome");
        for (const char *outcome : {"encrypted", "password", "cancel", "crc"})
            QTest::newRow(outcome) << QString::fromLatin1(outcome);
    }
    void archiveHashErrorsAndPasswordCancellation() {
        QFETCH(QString, outcome);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString inputPath = temporary.filePath(QStringLiteral("имя с пробелом.txt"));
        const QByteArray input = "hash workflow fixture 123456789";
        QFile file(inputPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(input), qint64(input.size()));
        file.close();
        ArchiveBackend backend;
        const QString archive = temporary.filePath("sample.zip");
        CompressionOptions options;
        options.format = "zip";
        options.Level = 0;
        const QString password = outcome == "crc" ? QString() : QStringLiteral("fixture password");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {inputPath}, password, false, options);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        if (outcome == "crc") {
            QFile bytes(archive);
            QVERIFY(bytes.open(QIODevice::ReadOnly));
            QByteArray data = bytes.readAll();
            bytes.close();
            const int position = data.indexOf(input);
            QVERIFY(position >= 0);
            data[position] = char(data[position] ^ 1);
            QVERIFY(bytes.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QCOMPARE(bytes.write(data), qint64(data.size()));
        }
        MainWindow window(&backend);
        window.show();
        window.openArchive(archive);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(table->rowCount(), 1);
        table->selectRow(0);
        QString results;
        QStringList errors;
        bool passwordAsked = false;
        QTimer responder;
        widgetDialogs_.connectResponder(responder, this, [&] {
            QWidget *modal = widgetDialogs_.activeDialog();
            if (auto *dialog = dynamic_cast<CPasswordDialog *>(modal)) {
                passwordAsked = true;
                if (outcome == "cancel")
                    dialog->reject();
                else {
                    dialog->findChild<QLineEdit *>("passwordEdit")
                        ->setText(outcome == "password" ? "wrong" : password);
                    dialog->accept();
                }
            } else if (auto *dialog = qobject_cast<CListViewDialog *>(modal)) {
                results = messageText(dialog->findChild<QTreeWidget *>("propertiesList"));
                dialog->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                errors.append(box->text());
                box->accept();
            } else if (auto *dialog = dynamic_cast<CProgressDialog *>(modal)) {
                auto *pause = dialog->findChild<QPushButton *>("IDB_PAUSE");
                if (pause && !pause->isVisible()) {
                    FOR_VECTOR(i, dialog->Sync.Messages)
                    errors.append(ToQString(dialog->Sync.Messages[i]));
                    const UString &message = dialog->Sync.FinalMessage.ErrorMessage.Message;
                    if (!message.IsEmpty())
                        errors.append(ToQString(message));
                    dialog->findChild<QPushButton *>("IDCANCEL")->click();
                }
            }
        });
        responder.start(1);
        window.findChild<QAction *>("hashSHA256Action")->trigger();
        responder.stop();
        QCOMPARE(passwordAsked, outcome != "crc");
        if (outcome == "encrypted") {
            QVERIFY(errors.isEmpty());
            QVERIFY(results.contains(QString::fromLatin1(
                QCryptographicHash::hash(input, QCryptographicHash::Sha256).toHex())));
        } else {
            QVERIFY(results.isEmpty());
            QCOMPARE(errors.isEmpty(), outcome == "cancel");
        }
        QVERIFY(!backend.busy());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), input);
    }
};

QTEST_MAIN(ChecksumTests)
#include "checksum_tests.moc"
