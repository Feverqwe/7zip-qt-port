#include "archivebackend.h"
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class WorkerProgressTests final : public QObject {
    Q_OBJECT
  private slots:
    void hashBurstKeepsResultsAndBoundsGuiDelivery() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        constexpr int files = 2048;
        for (int i = 0; i < files; ++i) {
            QFile file(fixture.filePath(QStringLiteral("имя с пробелом %1.txt").arg(i)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("123456789"), qint64(9));
        }
        ArchiveBackend backend;
        QSignalSpy completed(&backend, &ArchiveBackend::finished);
        QSignalSpy progress(&backend, &ArchiveBackend::progress);
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QElapsedTimer elapsed;
        elapsed.start();
        backend.hashMethods({fixture.path()}, {"CRC32"});
        // Simulate a temporarily occupied GUI. Native callbacks must not leave
        // one queued event per file waiting to be delivered afterwards.
        QTest::qSleep(200);
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 15000);
        QVERIFY(completed.first().first().toBool());
        QCOMPARE(backend.lastResult(), HRESULT(S_OK));
        int digests = 0;
        for (const auto &line : output)
            digests += line.first().toString().startsWith("CRC32: CBF43926  ");
        QCOMPARE(digests, files);
        const qint64 deliveryLimit = elapsed.elapsed() / 20 + 4;
        QVERIFY2(progress.count() <= deliveryLimit,
                 qPrintable(QString("%1 deliveries in %2 ms; limit %3")
                                .arg(progress.count())
                                .arg(elapsed.elapsed())
                                .arg(deliveryLimit)));
        const int delivered = progress.count();
        QTest::qWait(50);
        QCOMPARE(progress.count(), delivered);
    }
    void joinedHashCancelThenRestartDropsOldProgressAndCompletion() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("input.txt");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("123456789"), qint64(9));
        file.close();
        ArchiveBackend backend;
        QSignalSpy completed(&backend, &ArchiveBackend::finished);
        backend.hashMethods({path}, {"SHA256"});
        QCOMPARE(backend.setPaused(true), HRESULT(S_OK));
        backend.cancelAndWait();
        QCOMPARE(completed.count(), 1);
        QCOMPARE(backend.lastResult(), HRESULT(E_ABORT));
        backend.hashMethods({path}, {"CRC32"});
        QTRY_COMPARE(completed.count(), 2);
        QVERIFY(completed.last().first().toBool());
        QTest::qWait(50);
        QCOMPARE(completed.count(), 2);
        QVERIFY(!backend.busy());
    }
};
QTEST_GUILESS_MAIN(WorkerProgressTests)
#include "worker_progress_tests.moc"
