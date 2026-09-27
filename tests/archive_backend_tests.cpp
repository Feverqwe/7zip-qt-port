#include "archivebackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QtTest>

#include <cstdio>

static void writeOutput(const QByteArray &text) {
    std::fwrite(text.constData(), 1, size_t(text.size()), stdout);
    std::fflush(stdout);
}

class ArchiveBackendTests : public QObject {
    Q_OBJECT

  private:
    static void writeFile(const QString &path, const QByteArray &data) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), data.size());
    }
    static void await(ArchiveBackend &backend, bool expected) {
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        if (spy.isEmpty())
            QVERIFY2(spy.wait(30000), "operation timed out");
        QCOMPARE(spy.takeFirst().at(0).toBool(), expected);
    }

  private slots:
    void roundTrip_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
    }
    void roundTrip() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.path() + "/имя с пробелом";
        QVERIFY(QDir().mkpath(source + "/вложено"));
        writeFile(source + "/обычный файл.txt", "hello");
        writeFile(source + "/вложено/δ space.txt", "nested");
        ArchiveBackend backend;
        const QString archive = temp.path() + "/archive." + format;
        backend.create(archive, {source});
        await(backend, true);
        QVERIFY(QFileInfo::exists(archive));

        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QVERIFY(entries.size() >= 4);
        QString nested;
        for (const auto &entry : entries) {
            if (entry.path.endsWith("δ space.txt")) {
                nested = entry.path;
                QCOMPARE(entry.size, qint64(6));
                QVERIFY(entry.packedSize >= 0);
                QVERIFY(!entry.modified.isEmpty());
            }
        }
        QVERIFY(!nested.isEmpty());

        const QString selected = temp.path() + "/selected";
        QVERIFY(QDir().mkpath(selected));
        backend.extract(archive, selected, {nested});
        await(backend, true);
        QVERIFY(QFileInfo::exists(selected + "/" + nested));
        QVERIFY(!QFileInfo::exists(selected + "/имя с пробелом/обычный файл.txt"));

        const QString all = temp.path() + "/all";
        QVERIFY(QDir().mkpath(all));
        backend.extract(archive, all);
        await(backend, true);
        QFile file(all + "/имя с пробелом/обычный файл.txt");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("hello"));
    }
    void invalidArchive() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.path() + "/broken.7z";
        writeFile(archive, "this is not an archive");
        ArchiveBackend backend;
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.list(archive);
        QVERIFY(spy.wait(30000));
        QVERIFY(!spy.takeFirst().at(0).toBool());
    }
    void missingExecutable() {
        ArchiveBackend backend(nullptr, "/no/such/7zz");
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.list("/no/such/archive.7z");
        QVERIFY(spy.wait(30000));
        const auto result = spy.takeFirst();
        QVERIFY(!result.at(0).toBool());
        QVERIFY(result.at(1).toString().contains("7zz"));
    }
    void unsafePath() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.path() + "/outside.txt";
        const QString archive = temp.path() + "/unsafe.zip";
        const QString destination = temp.path() + "/out";
        writeFile(source, "unsafe");
        QVERIFY(QDir().mkpath(destination));
        QProcess maker;
        maker.start("7zz", {"a", "-tzip", "-spf", archive, source});
        QVERIFY(maker.waitForFinished(30000));
        QCOMPARE(maker.exitCode(), 0);
        ArchiveBackend backend;
        backend.list(archive);
        await(backend, true);
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination);
        QCOMPARE(spy.size(), 1);
        QVERIFY(!spy.takeFirst().at(0).toBool());
        QVERIFY(QDir(destination).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty());
    }
    void cancellation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.path() + "/large.bin";
        writeFile(source, QByteArray(8 * 1024 * 1024, 'x'));
        ArchiveBackend backend;
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.create(temp.path() + "/cancel.7z", {source});
        QVERIFY(backend.busy());
        backend.cancel();
        QVERIFY(spy.wait(30000));
        const auto result = spy.takeFirst();
        QVERIFY(!result.at(0).toBool());
        QVERIFY(result.at(1).toString().contains("отменена"));
        QVERIFY(!backend.busy());
    }
    void fragmentedOutputAndError() {
        qputenv("SEVENZIP_TEST_HELPER", "fragment");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy progress(&backend, &ArchiveBackend::progress);
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list("fake.7z");
        QVERIFY(finished.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!finished.takeFirst().at(0).toBool());
        QVERIFY(!backend.busy());
        QCOMPARE(progress.size(), 1);
        QCOMPARE(progress.at(0).at(0).toInt(), 42);
        QCOMPARE(progress.at(0).at(1).toString(), QStringLiteral("папка/имя с пробелом.txt"));
        QCOMPARE(output.size(), 2);
        QCOMPARE(output.at(1).at(0).toString(), QStringLiteral("ERROR: incomplete tail"));
    }
    void largeListingStaysResponsive() {
        qputenv("SEVENZIP_TEST_HELPER", "large");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy entries(&backend, &ArchiveBackend::entriesReady);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        bool timerRan = false;
        backend.list("large.7z");
        QTimer::singleShot(10, &backend, [&] { timerRan = true; });
        QVERIFY(finished.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(timerRan);
        QVERIFY(finished.takeFirst().at(0).toBool());
        QCOMPARE(entries.size(), 1);
        QCOMPARE(qvariant_cast<QList<ArchiveEntry>>(entries.at(0).at(0)).size(), 3000);
    }
    void cancelledPartialOutput() {
        qputenv("SEVENZIP_TEST_HELPER", "cancel");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list("cancel.7z");
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        backend.cancel();
        QVERIFY(finished.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!finished.takeFirst().at(0).toBool());
        QVERIFY(!backend.busy());
        QVERIFY(output.at(0).at(0).toString().contains("started"));
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const QByteArray mode = qgetenv("SEVENZIP_TEST_HELPER");
    if (!mode.isEmpty()) {
        if (mode == "fragment") {
            writeOutput("  4");
            QThread::msleep(20);
            writeOutput("2%\b\b    \b\b+ папка/имя с пробелом.txt\r");
            writeOutput("ERROR: incomplete tail");
            return 2;
        }
        if (mode == "large") {
            for (int i = 0; i < 3000; ++i) {
                writeOutput("Path = file" + QByteArray::number(i) + ".txt\nSize = 1\n\n");
                if (i == 999)
                    QThread::msleep(100);
            }
            return 0;
        }
        writeOutput("started\npartial");
        QThread::msleep(30000);
        return 0;
    }
    ArchiveBackendTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "archive_backend_tests.moc"
