#include "mainwindow.h"

#include <QCryptographicHash>
#include <QFile>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>

class ChecksumTests : public QObject {
    Q_OBJECT

  private slots:
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

    void cancellationStopsHashProcess() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("input.txt");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("content");
        file.close();
        qputenv("SEVENZIP_HASH_TEST_HELPER", "1");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.hash({path}, "SHA256");
        qunsetenv("SEVENZIP_HASH_TEST_HELPER");
        QTRY_VERIFY(backend.busy());
        backend.cancel();
        QVERIFY(finished.wait(30000));
        QVERIFY(!finished.takeFirst().first().toBool());
        QVERIFY(!backend.busy());
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
        action->trigger();
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QTRY_VERIFY(window.findChild<QPlainTextEdit *>("operationJournalText"));
        auto *journal = window.findChild<QPlainTextEdit *>("operationJournalText");
        const QString digest = QString::fromLatin1(
            QCryptographicHash::hash("content", QCryptographicHash::Sha256).toHex());
        QVERIFY(journal->toPlainText().contains(digest));

        window.openArchive(archive);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QTRY_COMPARE(table->rowCount(), 1);
        table->selectRow(0);
        QVERIFY(action->isEnabled());
        action->trigger();
        QVERIFY(finished.wait(30000)); // Temporary extraction.
        QVERIFY(finished.takeFirst().first().toBool());
        if (finished.isEmpty())
            QVERIFY(finished.wait(30000)); // Hashing.
        QVERIFY(finished.takeFirst().first().toBool());
        QTRY_VERIFY(journal->toPlainText().contains(digest));
        QVERIFY(QDir::setCurrent(original));
    }
};

int main(int argc, char **argv) {
    if (qEnvironmentVariableIsSet("SEVENZIP_HASH_TEST_HELPER")) {
        QThread::sleep(30);
        return 0;
    }
    QApplication app(argc, argv);
    ChecksumTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "checksum_tests.moc"
