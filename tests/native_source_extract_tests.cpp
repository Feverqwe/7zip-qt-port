#include "archive_reader_fixture.h"
#include "archivecodeccatalog.h"

class NativeSourceExtractTests final : public ArchiveReaderFixture {
    Q_OBJECT
    QTemporaryDir deployment_;
    const QString name_ = QStringLiteral("данные 空 with spaces.txt");
    const QByteArray payload_ =
        (QByteArray::fromHex("62696e61727900") + "Enter password:\n50% secret\xff").repeated(100);

    std::unique_ptr<ArchiveBackend> Service() {
#ifdef SEVENZIP_CODEC_MODULE
        auto service =
            std::make_unique<ArchiveBackend>(nullptr, deployment_.filePath("missing console"));
        return service;
#else
        return {};
#endif
    }
    QString Archive(QTemporaryDir &directory, const QString &format, bool encrypted = false) {
        if (!Put(directory.filePath(name_), payload_))
            return {};
        QStringList arguments{"a", "-t" + format, "-mx0"};
        if (encrypted) {
            arguments << "-ptest password";
            if (format == "7z")
                arguments << "-mhe=on";
        }
        arguments << "input." + format << name_;
        return Make(directory.path(), arguments) ? directory.filePath("input." + format)
                                                 : QString();
    }

  private slots:
    void initTestCase() {
        InitializeRuntime();
#ifdef SEVENZIP_CODEC_MODULE
        QVERIFY(deployment_.isValid());
        const QString module = QStringLiteral(SEVENZIP_CODEC_MODULE);
        QVERIFY(QFile::copy(module, deployment_.filePath(QFileInfo(module).fileName())));
#endif
    }
    void selectedBytesWithoutCli_data() {
        QTest::addColumn<QString>("format");
        for (const QString &format : {"zip", "7z", "gzip", "bzip2", "xz"})
            QTest::newRow(qPrintable(format)) << format;
    }
    void selectedBytesWithoutCli() {
        QFETCH(QString, format);
        QTemporaryDir directory;
        const QString archive = Archive(directory, format);
        QVERIFY(!archive.isEmpty());
        const auto before = Bytes(archive);
        auto service = Service();
        QVERIFY(service);
        QSignalSpy done(service.get(), &ArchiveBackend::finished);
        QSignalSpy listed(service.get(), &ArchiveBackend::entriesReady);
        service->list(archive);
        QTRY_COMPARE(done.size(), 1);
        QVERIFY2(done[0][0].toBool(), qPrintable(done[0][1].toString()));
        const auto entries = qvariant_cast<QList<ArchiveEntry>>(listed[0][0]);
        QCOMPARE(entries.size(), 1);
        done.clear();
        QSignalSpy passwords(service.get(), &ArchiveBackend::passwordRequired);
        QSignalSpy messages(service.get(), &ArchiveBackend::outputReceived);
        const QString output = directory.filePath("private output.bin");
        service->extractSourceEntry(archive, entries[0].path, output, format, {});
        QTRY_COMPARE(done.size(), 1);
        QVERIFY2(done[0][0].toBool(), qPrintable(done[0][1].toString()));
        QCOMPARE(service->lastSourceExtractOperation(), NArchive::NExtract::NOperationResult::kOK);
        QCOMPARE(Bytes(output), payload_);
        QVERIFY(passwords.isEmpty());
        for (const auto &message : messages)
            QVERIFY(!message[0].toString().contains("secret"));
        QCOMPARE(Bytes(archive), before);
    }
    void checksumFailureRetainsUnverifiedBytes_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("explicitType");
        for (const QString &format : {"zip", "7z"})
            for (bool explicitType : {false, true})
                QTest::newRow(qPrintable(format + (explicitType ? " explicit" : " automatic")))
                    << format << explicitType;
    }
    void checksumFailureRetainsUnverifiedBytes() {
        QFETCH(QString, format);
        QFETCH(bool, explicitType);
        QTemporaryDir directory;
        const QString archive = Archive(directory, format);
        QVERIFY(!archive.isEmpty());
        QByteArray damaged = Bytes(archive);
        const auto offset = damaged.indexOf(payload_);
        QVERIFY(offset > 0);
        damaged[offset] = '!';
        QVERIFY(Put(archive, damaged));
        auto service = Service();
        QVERIFY(service);
        QSignalSpy done(service.get(), &ArchiveBackend::finished);
        const QString output = directory.filePath("unverified.bin");
        service->extractSourceEntry(archive, name_, output, explicitType ? format : QString(), {});
        QTRY_COMPARE(done.size(), 1);
        QVERIFY(!done[0][0].toBool());
        QCOMPARE(service->lastResult(), S_FALSE);
        QCOMPARE(service->lastSourceExtractOperation(),
                 NArchive::NExtract::NOperationResult::kCRCError);
        QCOMPARE(Bytes(output), QByteArray("!") + payload_.mid(1));
        QCOMPARE(Bytes(archive), damaged);
    }
    void passwordCancelAndRetry_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("answer");
        for (const QString &format : {"7z", "zip"})
            for (const QString &answer : {"correct", "wrong", "cancel"})
                QTest::newRow(qPrintable(format + '-' + answer)) << format << answer;
    }
    void passwordCancelAndRetry() {
        QFETCH(QString, format);
        QFETCH(QString, answer);
        QTemporaryDir directory;
        const QString archive = Archive(directory, format, true);
        QVERIFY(!archive.isEmpty());
        const auto before = Bytes(archive);
        auto service = Service();
        QVERIFY(service);
        const auto prompt = connect(service.get(), &ArchiveBackend::passwordRequired, this, [&] {
            if (answer == "cancel")
                service->cancel();
            else
                service->providePassword(answer == "correct" ? "test password" : "wrong");
        });
        QSignalSpy done(service.get(), &ArchiveBackend::finished);
        const QString output = directory.filePath("output.bin");
        service->extractSourceEntry(archive, name_, output, format, {});
        QTRY_COMPARE(done.size(), 1);
        disconnect(prompt);
        QCOMPARE(done[0][0].toBool(), answer == "correct");
        if (answer == "correct") {
            QCOMPARE(Bytes(output), payload_);
            QVERIFY(QFile::remove(output));
        } else {
            QVERIFY(!QFileInfo::exists(output));
            if (answer == "cancel")
                QCOMPARE(service->lastResult(), E_ABORT);
        }
        done.clear();
        service->extractSourceEntry(archive, name_, output, format, "test password");
        QTRY_COMPARE(done.size(), 1);
        QVERIFY2(done[0][0].toBool(), qPrintable(done[0][1].toString()));
        QCOMPARE(Bytes(output), payload_);
        QCOMPARE(Bytes(archive), before);
    }
    void cancelJoinAndInvalidSelectionLeaveNoOutput() {
        QTemporaryDir directory;
        const QString archive = Archive(directory, "zip");
        QVERIFY(!archive.isEmpty());
        auto service = Service();
        QVERIFY(service);
        QSignalSpy done(service.get(), &ArchiveBackend::finished);
        const auto pause = connect(service.get(), &ArchiveBackend::phaseStarted, this,
                                   [&] { QCOMPARE(service->setPaused(true), S_OK); });
        const QString output = directory.filePath("output.bin");
        service->extractSourceEntry(archive, name_, output, "zip", {});
        service->cancelAndWait();
        disconnect(pause);
        QCOMPARE(done.size(), 1);
        QCOMPARE(service->lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(output));
        QVERIFY(!service->busy());
        done.clear();
        service->extractSourceEntry(archive, "absent.txt", output, "zip", {});
        QTRY_COMPARE(done.size(), 1);
        QVERIFY(!done[0][0].toBool());
        QVERIFY(!QFileInfo::exists(output));
        QVERIFY(Put(output, "preserve existing file"));
        done.clear();
        service->extractSourceEntry(archive, name_, output, "zip", {});
        QCOMPARE(done.size(), 1);
        QVERIFY(!done[0][0].toBool());
        QCOMPARE(Bytes(output), QByteArray("preserve existing file"));
    }
    void literalNamesAndAmbiguousDuplicates_data() {
        QTest::addColumn<QByteArray>("member");
        QTest::addColumn<bool>("duplicate");
        QTest::newRow("literal wildcards") << QByteArray("@-wild?*.txt") << false;
        QTest::newRow("line breaks") << QByteArray("line\nbreak.txt") << false;
        QTest::newRow("duplicates") << QByteArray("same.txt") << true;
    }
    void literalNamesAndAmbiguousDuplicates() {
        QFETCH(QByteArray, member);
        QFETCH(bool, duplicate);
        QTemporaryDir directory;
        const QByteArray first(member.size(), 'a'), second(member.size(), 'b');
        QVERIFY(Put(directory.filePath(QString::fromLatin1(first)), payload_));
        QVERIFY(Put(directory.filePath(QString::fromLatin1(second)), "other payload"));
        QVERIFY(Make(directory.path(), {"a", "-tzip", "input.zip", QString::fromLatin1(first),
                                        QString::fromLatin1(second)}));
        const auto archive = directory.filePath("input.zip");
        auto image = Bytes(archive);
        QCOMPARE(image.count(first), 2);
        image.replace(first, member);
        if (duplicate) {
            QCOMPARE(image.count(second), 2);
            image.replace(second, member);
        }
        QVERIFY(Put(archive, image));
        auto service = Service();
        QVERIFY(service);
        QSignalSpy done(service.get(), &ArchiveBackend::finished);
        const auto output = directory.filePath("output.bin");
        service->extractSourceEntry(archive, QString::fromUtf8(member), output, "zip", {});
        QTRY_COMPARE(done.size(), 1);
        QCOMPARE(done[0][0].toBool(), !duplicate);
        if (duplicate)
            QVERIFY(!QFileInfo::exists(output));
        else
            QCOMPARE(Bytes(output), payload_);
        QCOMPARE(Bytes(archive), image);
    }
    void solidSelectionDoesNotConcatenateSiblings() {
        QTemporaryDir directory;
        QVERIFY(Put(directory.filePath("a.txt"), QByteArray(65536, 'a')));
        QVERIFY(Put(directory.filePath("b.txt"), payload_));
        QVERIFY(Make(directory.path(), {"a", "-t7z", "-ms=on", "input.7z", "a.txt", "b.txt"}));
        auto service = Service();
        QVERIFY(service);
        QSignalSpy done(service.get(), &ArchiveBackend::finished);
        const auto output = directory.filePath("output.bin");
        service->extractSourceEntry(directory.filePath("input.7z"), "b.txt", output, "7z", {});
        QTRY_COMPARE(done.size(), 1);
        QVERIFY2(done[0][0].toBool(), qPrintable(done[0][1].toString()));
        QCOMPARE(Bytes(output), payload_);
    }
    void openWarningPreservesBytesWithoutReportingCompleteSuccess() {
        QTemporaryDir directory;
        const auto archive = Archive(directory, "7z");
        QVERIFY(!archive.isEmpty());
        const auto image = Bytes(archive) + "nonzero trailing data";
        QVERIFY(Put(archive, image));
        auto service = Service();
        QVERIFY(service);
        QSignalSpy done(service.get(), &ArchiveBackend::finished);
        QSignalSpy messages(service.get(), &ArchiveBackend::outputReceived);
        const auto output = directory.filePath("output.bin");
        service->extractSourceEntry(archive, name_, output, "7z", {});
        QTRY_COMPARE(done.size(), 1);
        QVERIFY(!done[0][0].toBool());
        QCOMPARE(service->lastResult(), S_FALSE);
        QCOMPARE(service->lastSourceExtractOperation(),
                 NArchive::NExtract::NOperationResult::kDataAfterEnd);
        QVERIFY(!messages.isEmpty());
        QCOMPARE(Bytes(output), payload_);
        QCOMPARE(Bytes(archive), image);
    }
    void destructionJoinsPasswordWait_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("opening password") << QString("7z");
        QTest::newRow("decoding password") << QString("zip");
    }
    void destructionJoinsPasswordWait() {
        QFETCH(QString, format);
        QTemporaryDir directory;
        const auto archive = Archive(directory, format, true);
        QVERIFY(!archive.isEmpty());
        auto service = Service();
        QVERIFY(service);
        const auto before = Bytes(archive);
        bool destroyed = false;
        connect(service.get(), &ArchiveBackend::passwordRequired, this, [&] {
            service.reset();
            destroyed = true;
        });
        const auto output = directory.filePath("output.bin");
        service->extractSourceEntry(archive, name_, output, format, {});
        QTRY_VERIFY(destroyed);
        QVERIFY(!QFileInfo::exists(output));
        QCOMPARE(Bytes(archive), before);
    }
};
QTEST_MAIN(NativeSourceExtractTests)
#include "native_source_extract_tests.moc"
