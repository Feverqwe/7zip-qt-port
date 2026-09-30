#include "mainwindow.h"

#include <QCryptographicHash>
#include <QFile>
#include <QSignalSpy>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTreeWidget>
#include <QtTest>

class ChecksumTests : public QObject {
    Q_OBJECT

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
        QTRY_VERIFY(window.findChild<QTreeWidget *>("IDL_MESSAGE"));
        auto *journal = window.findChild<QTreeWidget *>("IDL_MESSAGE");
        const QString digest = QString::fromLatin1(
            QCryptographicHash::hash("content", QCryptographicHash::Sha256).toHex());
        QVERIFY(messageText(journal).contains(digest));

        window.openArchive(archive);
        QCOMPARE(finished.size(), 1); // Source modal opening has already joined its worker.
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
        QTRY_VERIFY(messageText(journal).contains(digest));
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
