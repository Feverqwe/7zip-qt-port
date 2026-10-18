#include "archivebackend.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <cstdio>
#include <memory>

namespace {
bool write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
int engineFixture(const QStringList &arguments) {
    const int separator = arguments.indexOf("--");
    if (separator < 0 || separator + 1 >= arguments.size())
        return 2;
    QFile file(arguments[separator + 1]);
    if (!file.open(QIODevice::ReadOnly))
        return 2;
    const QByteArray mode = file.readAll();
    if (mode == "password") {
        std::fputs("Enter password:", stdout);
        std::fflush(stdout);
        char password[128];
        if (!std::fgets(password, sizeof(password), stdin))
            return 2;
    }
    if (mode == "stream")
        std::fputs("10% old-first\n20% old-second\n", stdout);
    const QByteArray name = mode == "second" ? "second.txt" : "first.txt";
    const QByteArray listing =
        "Type = zip\nPhysical Size = 9\n----------\nPath = " + name + "\nSize = 9\n\n";
    std::fwrite(listing.constData(), 1, size_t(listing.size()), stdout);
    std::fflush(stdout);
    return 0;
}
} // namespace

class ProcessNotificationTests final : public QObject {
    Q_OBJECT
  private slots:
    void receiversMayReleaseBackend_data() {
        QTest::addColumn<QString>("signal");
        for (const char *name : {"output", "progress", "info", "entries", "password", "started",
                                 "finished", "start-failure", "overwrite"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void receiversMayReleaseBackend() {
        QFETCH(QString, signal);
        QTemporaryDir fixture;
        const QString archive = fixture.filePath(QStringLiteral("архив 空 with spaces.zip"));
        const QByteArray mode = signal == "password" ? "password" : "first";
        QVERIFY(write(archive, mode));
        auto backend = std::make_unique<ArchiveBackend>(
            nullptr, signal == "start-failure" ? fixture.filePath("missing-engine")
                                               : QCoreApplication::applicationFilePath());
        QPointer<ArchiveBackend> owner(backend.get());
        const auto release = [&] { backend.reset(); };
        if (signal == "overwrite") {
            QSignalSpy listed(backend.get(), &ArchiveBackend::finished);
            backend->list(archive);
            QTRY_COMPARE(listed.size(), 1);
            QVERIFY(listed.first().first().toBool());
            const QString output = fixture.filePath("output");
            QVERIFY(QDir().mkpath(output));
            QVERIFY(write(output + "/first.txt", "unchanged"));
            connect(backend.get(), &ArchiveBackend::overwriteRequired, this, release);
            ExtractionOptions options;
            options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
            backend->extract(archive, output, {}, {}, options);
            QVERIFY(owner.isNull());
            QFile original(output + "/first.txt");
            QVERIFY(original.open(QIODevice::ReadOnly));
            QCOMPARE(original.readAll(), QByteArray("unchanged"));
        } else {
            if (signal == "output")
                connect(backend.get(), &ArchiveBackend::outputReceived, this, release);
            else if (signal == "progress")
                connect(backend.get(), &ArchiveBackend::progress, this, release);
            else if (signal == "info")
                connect(backend.get(), &ArchiveBackend::infoReady, this, release);
            else if (signal == "entries")
                connect(backend.get(), &ArchiveBackend::entriesReady, this, release);
            else if (signal == "password")
                connect(backend.get(), &ArchiveBackend::passwordRequired, this, release);
            else if (signal == "started")
                connect(backend.get(), &ArchiveBackend::engineStarted, this, release);
            else
                connect(backend.get(), &ArchiveBackend::finished, this, release);
            backend->list(archive);
            QTRY_VERIFY_WITH_TIMEOUT(owner.isNull(), 5000);
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QFile original(archive);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), mode);
    }
    void listingPayloadSurvivesRestart_data() {
        QTest::addColumn<bool>("fromInfo");
        QTest::newRow("info-receiver") << true;
        QTest::newRow("first-entries-receiver") << false;
    }
    void listingPayloadSurvivesRestart() {
        QFETCH(bool, fromInfo);
        QTemporaryDir fixture;
        const QString first = fixture.filePath("first.zip"),
                      second = fixture.filePath("second.zip");
        QVERIFY(write(first, "first"));
        QVERIFY(write(second, "second"));
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        bool restarted = false;
        const auto restart = [&] {
            if (!restarted) {
                restarted = true;
                backend.list(second);
            }
        };
        if (fromInfo)
            connect(&backend, &ArchiveBackend::infoReady, this, restart);
        else
            connect(&backend, &ArchiveBackend::entriesReady, this, restart);
        QSignalSpy entries(&backend, &ArchiveBackend::entriesReady);
        QSignalSpy completed(&backend, &ArchiveBackend::finished);
        backend.list(first);
        QTRY_COMPARE(completed.size(), 2);
        QCOMPARE(entries.size(), 2);
        const auto original = qvariant_cast<QList<ArchiveEntry>>(entries[0][0]);
        const auto replacement = qvariant_cast<QList<ArchiveEntry>>(entries[1][0]);
        QCOMPARE(original.size(), 1);
        QCOMPARE(original[0].path, QString("first.txt"));
        QCOMPARE(replacement.size(), 1);
        QCOMPARE(replacement[0].path, QString("second.txt"));
        QVERIFY(completed[0][0].toBool());
        QVERIFY(completed[1][0].toBool());
        QVERIFY(!backend.busy());
    }
    void listingDoesNotOverwriteReentrantRequestResult() {
        QTemporaryDir fixture;
        const QString archive = fixture.filePath("first.zip");
        QVERIFY(write(archive, "first"));
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        connect(&backend, &ArchiveBackend::infoReady, this,
                [&] { backend.hashMethods({}, {"CRC32"}); });
        QSignalSpy completed(&backend, &ArchiveBackend::finished);
        backend.list(archive);
        QTRY_COMPARE(completed.size(), 2);
        QVERIFY(!completed[0][0].toBool());
        QVERIFY(completed[1][0].toBool());
        QCOMPARE(backend.lastResult(), HRESULT(E_INVALIDARG));
    }
    void outputCancellationRetiresOldBuffersAndProcess() {
        QTemporaryDir fixture;
        const QString first = fixture.filePath("first.zip"),
                      second = fixture.filePath("second.zip");
        QVERIFY(write(first, "stream"));
        QVERIFY(write(second, "second"));
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        bool cancelling = false, restarted = false;
        int firstLineCount = 0;
        QStringList staleProgress;
        connect(&backend, &ArchiveBackend::outputReceived, this, [&](const QString &line) {
            if (line != "10% old-first")
                return;
            ++firstLineCount;
            if (!cancelling) {
                cancelling = true;
                backend.cancelAndWait();
            }
        });
        connect(&backend, &ArchiveBackend::finished, this, [&](bool, const QString &) {
            if (!restarted) {
                restarted = true;
                backend.list(second);
            }
        });
        connect(&backend, &ArchiveBackend::progress, this, [&](int, const QString &detail) {
            if (restarted && detail.startsWith("old-"))
                staleProgress.append(detail);
        });
        QSignalSpy completed(&backend, &ArchiveBackend::finished);
        QSignalSpy entries(&backend, &ArchiveBackend::entriesReady);
        backend.list(first);
        QTRY_COMPARE(completed.size(), 2);
        QCOMPARE(firstLineCount, 1);
        QVERIFY(staleProgress.isEmpty());
        QVERIFY(!completed[0][0].toBool());
        QVERIFY(completed[1][0].toBool());
        QCOMPARE(entries.size(), 1);
        const auto listed = qvariant_cast<QList<ArchiveEntry>>(entries[0][0]);
        QCOMPARE(listed.size(), 1);
        QCOMPARE(listed[0].path, QString("second.txt"));
        QVERIFY(!backend.busy());
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() > 1 && app.arguments()[1] == "l")
        return engineFixture(app.arguments());
    ProcessNotificationTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "process_notification_tests.moc"
