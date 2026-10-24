#include "archive_reader_fixture.h"
#include "rar_memory_fixture.h"
#include <QTimer>

class NativeArchiveTestTests final : public ArchiveReaderFixture {
    Q_OBJECT
    QString Archive(QTemporaryDir &dir, const QString &format, bool encrypted = false) {
        if (!dir.isValid())
            return {};
        const QString name = QStringLiteral("данные 空 with spaces.txt");
        if (!Put(dir.filePath(name), QByteArray("native test payload\n").repeated(10000)))
            return {};
        QStringList args{"a", "-t" + format};
        if (encrypted) {
            args << "-ptest password";
            args << (format == "7z" ? "-mhe=on" : "-mem=AES256");
        }
        args << "archive.bin" << name;
        return Make(dir.path(), args) ? dir.filePath("archive.bin") : QString();
    }

  private slots:
    void initTestCase() { InitializeRuntime(); }
    void init() {
        manager_.Release();
        backend_ = std::make_unique<ArchiveBackend>(nullptr, engine_);
    }
    void successfulTestUsesOfficialHandler_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("listed");
        for (const QString &format : {"zip", "7z", "gzip"})
            for (bool listed : {false, true})
                QTest::newRow(qPrintable(format + (listed ? " listed" : " source")))
                    << format << listed;
    }
    void successfulTestUsesOfficialHandler() {
        QFETCH(QString, format);
        QFETCH(bool, listed);
        QTemporaryDir dir;
        const QString path = Archive(dir, format);
        QVERIFY(!path.isEmpty());
        const QByteArray original = Bytes(path);
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        if (listed) {
            backend_->list(path);
            QTRY_COMPARE(finished.size(), 1);
            QVERIFY(finished.takeFirst()[0].toBool());
        }
        QSignalSpy phases(backend_.get(), &ArchiveBackend::phaseStarted);
        if (listed)
            backend_->test(path);
        else
            backend_->testSourceArchive(path, {});
        QVERIFY(backend_->busy());
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(backend_->lastResult(), S_OK);
        QVERIFY(!backend_->busy());
        QCOMPARE(phases.size(), 1);
        QCOMPARE(Bytes(path), original);
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 2);
    }
    void encryptedPasswordAndCancellation_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("answer");
        for (const QString &format : {"zip", "7z"})
            for (const QString &answer :
                 {"correct", "wrong", "empty", "cancel", "correct preset", "wrong preset"})
                QTest::newRow(qPrintable(format + '-' + answer)) << format << answer;
    }
    void encryptedPasswordAndCancellation() {
        QFETCH(QString, format);
        QFETCH(QString, answer);
        QTemporaryDir dir;
        const QString path = Archive(dir, format, true);
        QVERIFY(!path.isEmpty());
        const QByteArray original = Bytes(path);
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        unsigned questions = 0;
        const auto prompt = connect(backend_.get(), &ArchiveBackend::passwordRequired, this, [&] {
            ++questions;
            if (answer == "cancel")
                backend_->cancel();
            else
                backend_->providePassword(answer == "correct" ? QStringLiteral("test password")
                                          : answer == "empty" ? QStringLiteral("")
                                                              : QStringLiteral("wrong password"));
        });
        const QString preset = answer == "correct preset" ? QStringLiteral("test password")
                               : answer == "wrong preset" ? QStringLiteral("wrong password")
                                                          : QString();
        backend_->testSourceArchive(path, preset);
        QTRY_COMPARE(finished.size(), 1);
        disconnect(prompt);
        QCOMPARE(questions, answer.endsWith(" preset") ? 0u : 1u);
        QCOMPARE(finished[0][0].toBool(), answer.startsWith("correct"));
        if (answer == "cancel")
            QCOMPARE(backend_->lastResult(), E_ABORT);
        else
            QCOMPARE(backend_->lastResult() == S_OK, answer.startsWith("correct"));
        QVERIFY(!backend_->busy());
        QCOMPARE(Bytes(path), original);
    }
    void corruptEntryDoesNotBecomeSuccessfulTest() {
        QTemporaryDir dir;
        const QString path = Archive(dir, "gzip");
        QVERIFY(!path.isEmpty());
        QByteArray image = Bytes(path);
        QVERIFY(image.size() > 8);
        image[image.size() - 8] = char(image.at(image.size() - 8) ^ 1);
        QVERIFY(Put(path, image));
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        QSignalSpy errors(backend_.get(), &ArchiveBackend::outputReceived);
        backend_->testSourceArchive(path, {});
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(backend_->lastResult(), E_FAIL);
        QVERIFY(!errors.isEmpty());
        QVERIFY(finished[0][1].toString().contains(errors[0][0].toString()));
        QCOMPARE(Bytes(path), image);
    }
    void storedRarUsesAlreadyAllowedMemory_data() {
        QTest::addColumn<unsigned>("dictionaryBits");
        QTest::newRow("2 MiB") << 4u;
        // Stored records use CopyCoder: the declared window triggers the
        // official memory request but does not allocate a dictionary.
        QTest::newRow("2 GiB declared stored window") << 14u;
    }
    void storedRarUsesAlreadyAllowedMemory() {
        QFETCH(unsigned, dictionaryBits);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("RAR 空 with spaces.rar"));
        const auto image = RarMemoryFixture::archive(0, RarMemoryFixture::name(), dictionaryBits);
        QVERIFY(Put(path, image));
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        backend_->testSourceArchive(path, {});
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(backend_->lastResult(), S_OK);
        QCOMPARE(Bytes(path), image);
    }
    void pausedWorkerCanBeCancelledAndJoined() {
        QTemporaryDir dir;
        const QString path = Archive(dir, "zip");
        QVERIFY(!path.isEmpty());
        const QByteArray original = Bytes(path);
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        const auto pause = connect(backend_.get(), &ArchiveBackend::phaseStarted, this,
                                   [&] { QCOMPARE(backend_->setPaused(true), S_OK); });
        backend_->testSourceArchive(path, {});
        QVERIFY(backend_->busy());
        QTest::qWait(30);
        QVERIFY(finished.isEmpty());
        backend_->cancelAndWait();
        disconnect(pause);
        QCOMPARE(finished.size(), 1);
        QCOMPARE(backend_->lastResult(), E_ABORT);
        QVERIFY(!backend_->busy());
        QTest::qWait(10);
        QCOMPARE(finished.size(), 1);
        finished.clear();
        backend_->testSourceArchive(path, {});
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(finished[0][0].toBool());
        QCOMPARE(backend_->lastResult(), S_OK);
        QCOMPARE(Bytes(path), original);
    }
    void destructionJoinsPasswordWait() {
        QTemporaryDir dir;
        const QString path = Archive(dir, "7z", true);
        QVERIFY(!path.isEmpty());
        const QByteArray original = Bytes(path);
        std::unique_ptr<ArchiveBackend> service(backend_->createIndependentBackend());
        bool destroyed = false;
        connect(service.get(), &ArchiveBackend::passwordRequired, this, [&] {
            service.reset();
            destroyed = true;
        });
        service->testSourceArchive(path, {});
        QTRY_VERIFY(destroyed);
        QCOMPARE(Bytes(path), original);
    }
    void launchReceiverCanDestroyService() {
        QTemporaryDir dir;
        const QString path = Archive(dir, "zip");
        QVERIFY(!path.isEmpty());
        const QByteArray original = Bytes(path);
        std::unique_ptr<ArchiveBackend> service(backend_->createIndependentBackend());
        connect(service.get(), &ArchiveBackend::phaseStarted, this, [&] { service.reset(); });
        service->testSourceArchive(path, {});
        QVERIFY(!service);
        QCOMPARE(Bytes(path), original);
    }
};
QTEST_MAIN(NativeArchiveTestTests)
#include "native_archive_test_tests.moc"
