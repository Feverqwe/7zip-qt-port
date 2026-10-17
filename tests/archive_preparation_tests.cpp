#include "archivebackend.h"
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class ArchivePreparationTests final : public QObject {
    Q_OBJECT
  private slots:
    void joinedCancelBeforeWorkerLaunchReleasesPreparation() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath("source.zip");
        QFile input(archive);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("original"), qint64(8));
        input.close();
        const QString output = fixture.filePath("canceled.zip");
        ArchiveBackend backend(nullptr, "/missing/preparation-engine");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        connect(&backend, &ArchiveBackend::phaseStarted, &backend, &ArchiveBackend::cancelAndWait);
        backend.updateSourcePlan(archive, {}, output, "zip", {});
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(output));
        QApplication::processEvents();
        QCOMPARE(finished.size(), 1);
    }
    void joinedCancelThenRestartRejectsOldCompletion() {
        QTemporaryDir fixture;
        const QString archive = fixture.filePath("source.zip");
        QFile input(archive);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("original"), qint64(8));
        input.close();
        ArchiveBackend backend(nullptr, "/missing/preparation-engine");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        const QString canceledOutput = fixture.filePath("canceled.zip");
        backend.updateSourcePlan(archive, {}, canceledOutput, "zip", {});
        QCOMPARE(backend.setPaused(true), HRESULT(S_OK));
        backend.cancelAndWait();
        QCOMPARE(finished.count(), 1);
        QVERIFY(!finished.first().first().toBool());
        const QString output = fixture.filePath("next.zip");
        backend.updateSourcePlan(archive, {}, output, "zip", {});
        QTRY_COMPARE(finished.count(), 2);
        QVERIFY(finished.last().first().toBool());
        QVERIFY(!QFileInfo::exists(canceledOutput));
        QFile result(output);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("original"));
        QApplication::processEvents();
        QCOMPARE(finished.count(), 2);
        QVERIFY(!backend.busy());
    }
    void destructionJoinsPausedPreparationAndReleasesStage() {
        QTemporaryDir fixture;
        const QString archive = fixture.filePath("source.zip");
        QFile input(archive);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write(QByteArray(1024 * 1024, 'a')), qint64(1024 * 1024));
        input.close();
        const QString output = fixture.filePath("canceled.zip");
        auto backend = std::make_unique<ArchiveBackend>(nullptr, "/missing/preparation-engine");
        backend->updateSourcePlan(archive, {}, output, "zip", {});
        QCOMPARE(backend->setPaused(true), HRESULT(S_OK));
        backend.reset();
        QApplication::processEvents();
        QVERIFY(!QFileInfo::exists(output));
        QVERIFY(QFileInfo::exists(archive));
        QVERIFY(QDir(fixture.path())
                    .entryList({".sevenzip-preparation-*"}, QDir::Dirs | QDir::Hidden)
                    .isEmpty());
    }
    void preparedBytesOrPublicationFailure_data() {
        QTest::addColumn<int>("failure");
        QTest::newRow("success") << 0;
        QTest::newRow("colliding-output") << 1;
        QTest::newRow("unavailable-parent") << 2;
    }
    void preparedBytesOrPublicationFailure() {
        QFETCH(int, failure);
        QTemporaryDir fixture;
        const QString archive = fixture.filePath("source 空 with spaces.zip");
        const QString parent = failure == 2 ? fixture.filePath("occupied parent") : fixture.path();
        const QString output = QDir(parent).filePath("prepared 空.zip");
        const QByteArray bytes("prepared source bytes");
        QFile input(archive);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write(bytes), qint64(bytes.size()));
        input.close();
        if (failure == 2) {
            QFile occupied(parent);
            QVERIFY(occupied.open(QIODevice::WriteOnly));
        }
        ArchiveBackend backend(nullptr, "/missing/preparation-engine");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        bool collision = false;
        connect(&backend, &ArchiveBackend::progress, this, [&](int, const QString &) {
            if (failure != 1 || collision)
                return;
            QFile other(output);
            QVERIFY(other.open(QIODevice::WriteOnly | QIODevice::NewOnly));
            QCOMPARE(other.write("caller-owned"), qint64(12));
            collision = true;
        });
        backend.updateSourcePlan(archive, {}, output, "zip", {});
        QTRY_COMPARE(finished.count(), 1);
        QCOMPARE(finished.first().first().toBool(), failure == 0);
        QVERIFY(!backend.busy());
        QCOMPARE(started.count(), 0);
        QFile result(output);
        if (failure != 2) {
            QVERIFY(result.open(QIODevice::ReadOnly));
            QCOMPARE(result.readAll(), failure == 0 ? bytes : QByteArray("caller-owned"));
        } else
            QVERIFY(!result.exists());
        QVERIFY(input.open(QIODevice::ReadOnly));
        QCOMPARE(input.readAll(), bytes);
        QVERIFY(QDir(fixture.path())
                    .entryList({".sevenzip-preparation-*"}, QDir::Dirs | QDir::Hidden)
                    .isEmpty());
    }
    void cancelPausedPreparationBeforeEngineStart_data() {
        QTest::addColumn<bool>("sourcePlan");
        QTest::newRow("source-mutation") << false;
        QTest::newRow("source-plan") << true;
    }
    void cancelPausedPreparationBeforeEngineStart() {
        QFETCH(bool, sourcePlan);
        QTemporaryDir fixture;
        const QString archive = fixture.filePath("source 空 with spaces.zip");
        const QString output = fixture.filePath("prepared 空 with spaces.zip");
        QFile input(archive);
        QVERIFY(input.open(QIODevice::WriteOnly));
        const QByteArray bytes(8 * 1024 * 1024, 'a');
        QCOMPARE(input.write(bytes), qint64(bytes.size()));
        input.close();
        ArchiveBackend backend(nullptr, "/missing/preparation-engine");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        if (sourcePlan)
            backend.updateSourcePlan(archive, {}, output, "zip", {});
        else
            backend.updateSourceMutation(
                archive, output, "zip", ArchiveBackend::SourceMutation::Rename, {"old", "new"}, {});
        QVERIFY(backend.busy());
        QVERIFY(!QFileInfo::exists(output));
        QCOMPARE(backend.setPaused(true), HRESULT(S_OK));
        bool heartbeat = false;
        QTimer::singleShot(0, &backend, [&] {
            heartbeat = true;
            backend.cancel();
        });
        QTRY_COMPARE(finished.count(), 1);
        QVERIFY(heartbeat);
        QVERIFY(!finished.first().first().toBool());
        QCOMPARE(backend.lastResult(), HRESULT(E_ABORT));
        QVERIFY(!backend.busy());
        QCOMPARE(started.count(), 0);
        QVERIFY(!QFileInfo::exists(output));
        QVERIFY(input.open(QIODevice::ReadOnly));
        QCOMPARE(input.readAll(), bytes);
    }
};
QTEST_GUILESS_MAIN(ArchivePreparationTests)
#include "archive_preparation_tests.moc"
