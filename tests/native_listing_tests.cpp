#include "archive_reader_fixture.h"
#include "native_archive_fixture.h"
#include <QtEndian>

class NativeListingTests final : public ArchiveReaderFixture {
    Q_OBJECT
    const QString name_ = QStringLiteral("данные 空 with spaces.txt");
    const QByteArray payload_ = QByteArray("native listing payload\n").repeated(100);

    QString Archive(QTemporaryDir &dir, const QString &format, bool encrypted = false) {
        if (!dir.isValid() || !Put(dir.filePath(name_), payload_))
            return {};
        QStringList args{"a", "-t" + format};
        if (encrypted)
            args << "-ptest password" << "-mhe=on";
        args << "archive." + format << name_;
        return Make(dir.path(), args) ? dir.filePath("archive." + format) : QString();
    }

  private slots:
    void initTestCase() { InitializeRuntime(); }
    void init() {
        manager_.Release();
        backend_ = std::make_unique<ArchiveBackend>(nullptr, engine_);
    }
    void metadataMatchesOfficialListing_data() {
        QTest::addColumn<QString>("format");
        for (const QString &format : {"zip", "7z", "gzip", "bzip2", "xz"})
            QTest::newRow(qPrintable(format)) << format;
    }
    void metadataMatchesOfficialListing() {
        QFETCH(QString, format);
        QTemporaryDir dir;
        const QString path = Archive(dir, format);
        QVERIFY(!path.isEmpty());
        const QByteArray original = Bytes(path);
        QProcess cli;
        cli.start(engine_, {"l", "-slt", "-sns", "-sccUTF-8", "--", path});
        QVERIFY(cli.waitForFinished(30000));
        QCOMPARE(cli.exitCode(), 0);
        QString text = QString::fromUtf8(cli.readAllStandardOutput());
        text.replace("\r\n", "\n");
        QMap<QString, QString> reference;
        for (const auto &line : text.section("----------\n", 1).split('\n')) {
            const auto separator = line.indexOf(" = ");
            if (separator >= 0)
                reference.insert(line.left(separator), line.mid(separator + 3));
        }
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        QSignalSpy entries(backend_.get(), &ArchiveBackend::entriesReady);
        QSignalSpy info(backend_.get(), &ArchiveBackend::infoReady);
        backend_->list(path);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(info.size(), 1);
        const auto actual = qvariant_cast<QList<ArchiveEntry>>(entries[0][0]);
        QCOMPARE(actual.size(), 1);
        const auto &entry = actual.first();
        // Unicode-capable archive names remain exact. Compressed streams
        // retain their producer's encoding and CArc default-name semantics.
        if (format == "zip" || format == "7z")
            QCOMPARE(entry.path, name_);
        if (!reference.value("Path").isEmpty())
            QCOMPARE(entry.path, reference.value("Path"));
        for (const auto &field :
             {qMakePair(QStringLiteral("Size"), entry.sizeValue),
              qMakePair(QStringLiteral("Packed Size"), entry.packedSizeValue)}) {
            bool defined = false;
            const auto size = reference.value(field.first).toULongLong(&defined);
            QCOMPARE(field.second.has_value(), defined);
            if (defined)
                QCOMPARE(*field.second, size);
        }
        if (!reference.value("CRC").isEmpty()) {
            QVERIFY(entry.crc.has_value());
            QCOMPARE(*entry.crc, reference.value("CRC").toUInt(nullptr, 16));
        }
        QCOMPARE(entry.modified, reference.value("Modified"));
        QVERIFY(!entry.directory);
        for (const QString &key : {"Attributes", "Method", "Encrypted"})
            QCOMPARE(entry.properties.value(key), reference.value(key));
        QCOMPARE(Bytes(path), original);
    }
    void encryptedHeadersFailureCancelAndRetry_data() {
        QTest::addColumn<QString>("answer");
        for (const QString &answer : {"correct", "wrong", "cancel"})
            QTest::newRow(qPrintable(answer)) << answer;
    }
    void splitVolumeMetadataMatchesOfficialHeader() {
        QTemporaryDir dir;
        QVERIFY(Put(dir.filePath(name_), payload_));
        QVERIFY(Make(dir.path(), {"a", "-tzip", "-mx=0", "-v1k", "archive.zip", name_}));
        const QString path = dir.filePath("archive.zip.001");
        QProcess cli;
        cli.start(engine_, {"l", "-slt", "-sccUTF-8", "--", path});
        QVERIFY(cli.waitForFinished(30000));
        QCOMPARE(cli.exitCode(), 0);
        QString text = QString::fromUtf8(cli.readAllStandardOutput());
        text.replace("\r\n", "\n");
        QMap<QString, QString> header;
        for (const auto &line : text.section("----------\n", 0, 0).split('\n')) {
            const auto separator = line.indexOf(" = ");
            if (separator >= 0)
                header.insert(line.left(separator), line.mid(separator + 3));
        }
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        QSignalSpy info(backend_.get(), &ArchiveBackend::infoReady);
        backend_->list(path);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(info.size(), 1);
        const auto actual = qvariant_cast<ArchiveInfo>(info[0][0]);
        QVERIFY(header.value("Volumes").toInt() > 1);
        QCOMPARE(actual.volumeCount, header.value("Volumes").toInt());
        QCOMPARE(actual.totalPhysicalSize, header.value("Total Physical Size").toLongLong());
    }
    void encryptedHeadersFailureCancelAndRetry() {
        QFETCH(QString, answer);
        QTemporaryDir dir;
        const QString path = Archive(dir, "7z", true);
        QVERIFY(!path.isEmpty());
        const QByteArray original = Bytes(path);
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        QSignalSpy entries(backend_.get(), &ArchiveBackend::entriesReady);
        QSignalSpy info(backend_.get(), &ArchiveBackend::infoReady);
        const auto prompt = connect(backend_.get(), &ArchiveBackend::passwordRequired, this, [&] {
            if (answer == "cancel")
                backend_->cancel();
            else
                backend_->providePassword(answer == "correct" ? "test password" : "wrong");
        });
        backend_->list(path);
        QTRY_COMPARE(finished.size(), 1);
        disconnect(prompt);
        const bool success = answer == "correct";
        QCOMPARE(finished[0][0].toBool(), success);
        QCOMPARE(entries.size(), success ? 1 : 0);
        QCOMPARE(info.size(), success ? 1 : 0);
        QCOMPARE(backend_->hasListedSourceArchive(path), success);
        if (answer == "cancel")
            QCOMPARE(backend_->lastResult(), E_ABORT);
        finished.clear();
        backend_->list(path, "test password");
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(Bytes(path), original);
    }
    void cancelledListingDoesNotPublishSnapshot() {
        QTemporaryDir dir;
        const QString path = Archive(dir, "zip");
        QVERIFY(!path.isEmpty());
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        QSignalSpy entries(backend_.get(), &ArchiveBackend::entriesReady);
        QSignalSpy info(backend_.get(), &ArchiveBackend::infoReady);
        const auto pause = connect(backend_.get(), &ArchiveBackend::phaseStarted, this,
                                   [&] { QCOMPARE(backend_->setPaused(true), S_OK); });
        backend_->list(path);
        backend_->cancelAndWait();
        disconnect(pause);
        QCOMPARE(finished.size(), 1);
        QCOMPARE(backend_->lastResult(), E_ABORT);
        QVERIFY(entries.isEmpty());
        QVERIFY(info.isEmpty());
        QVERIFY(!backend_->hasListedSourceArchive(path));
        QVERIFY(!backend_->busy());
    }
    void untrustedNamesRemainLiteral_data() {
        QTest::addColumn<QByteArray>("name");
        QTest::newRow("parent traversal") << QByteArray("../outside.txt");
        QTest::newRow("listing delimiter") << QByteArray("bad\nPath = alias.txt");
    }
    void untrustedNamesRemainLiteral() {
        QFETCH(QByteArray, name);
        QTemporaryDir dir;
        const QByteArray placeholder(name.size(), 'x');
        QVERIFY(Put(dir.filePath(QString::fromLatin1(placeholder)), payload_));
        QVERIFY(Make(dir.path(), {"a", "-tzip", "unsafe.zip", QString::fromLatin1(placeholder)}));
        const QString path = dir.filePath("unsafe.zip");
        QByteArray image = Bytes(path);
        // Change only equal-length local/central header names in an official
        // ZIP fixture. Payload and its CRC remain valid.
        QCOMPARE(image.count(placeholder), 2);
        image.replace(placeholder, name);
        QVERIFY(Put(path, image));
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        QSignalSpy entries(backend_.get(), &ArchiveBackend::entriesReady);
        backend_->list(path);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(entries.size(), 1);
        const auto listed = qvariant_cast<QList<ArchiveEntry>>(entries[0][0]);
        QCOMPARE(listed.size(), 1);
        QCOMPARE(listed.first().path, QDir::toNativeSeparators(QString::fromLatin1(name)));
        finished.clear();
        backend_->extract(path, dir.filePath("output"));
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(Bytes(path), image);
        QVERIFY(!QFileInfo::exists(dir.filePath("output/alias.txt")));
        QVERIFY(!QFileInfo::exists(dir.filePath("outside.txt")));
    }
    void destructionJoinsListingPasswordWait() {
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
        service->list(path);
        QTRY_VERIFY(destroyed);
        QCOMPARE(Bytes(path), original);
    }
    void automaticContainerBlocksMutationsDespiteZipSuffix() {
        QTemporaryDir dir;
        const QString inner = Archive(dir, "zip");
        QVERIFY(!inner.isEmpty());
        const QString path = dir.filePath("disguised.zip");
        const auto zip = Bytes(inner);
        const quint32 sectors = quint32((zip.size() + 511) / 512);
        QByteArray disk((sectors + 1) * 512, '\0');
        disk[446 + 4] = 7;
        qToLittleEndian<quint32>(1, disk.data() + 446 + 8);
        qToLittleEndian<quint32>(sectors, disk.data() + 446 + 12);
        disk[510] = char(0x55);
        disk[511] = char(0xaa);
        memcpy(disk.data() + 512, zip.constData(), zip.size());
        QVERIFY(Put(path, disk));
        const QByteArray original = Bytes(path);
        QSignalSpy finished(backend_.get(), &ArchiveBackend::finished);
        QSignalSpy info(backend_.get(), &ArchiveBackend::infoReady);
        QSignalSpy entries(backend_.get(), &ArchiveBackend::entriesReady);
        // Select the partition table at the outer boundary; its single main
        // subfile is then detected as ZIP by the official recursive opener.
        backend_->openFolder(path, "*:r.MBR");
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(info.size(), 1);
        QVERIFY(qvariant_cast<ArchiveInfo>(info[0][0]).containerLevels >= 2);
        QSignalSpy catalogReady(backend_.get(), &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend_->discoverCodecCatalog());
        QTRY_COMPARE(catalogReady.size(), 1);
        QVERIFY(catalogReady[0][0].toBool());
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(backend_->openNativeArchive(path, "*:r.MBR", nullptr, reader), S_OK);
        QVERIFY(reader);
        QCOMPARE(backend_->setNativeArchiveProperties(reader, nullptr, nullptr, 0), E_NOTIMPL);
        QCOMPARE(backend_->updateNativeArchive(reader, nullptr, 0, nullptr), E_NOTIMPL);
        QCOMPARE(Bytes(path), original);
        reader.reset();
        QCOMPARE(backend_->openNativeArchive(inner, "zip", nullptr, reader), S_OK);
        QVERIFY(reader);
        const QString updatedPath = dir.filePath("updated.zip");
        CMyComPtr2_Create<IOutStream, COutFileStream> output;
        QVERIFY(output->Create_NEW(us2fs(updatedPath.toStdWString().c_str())));
        CMyComPtr<IArchiveUpdateCallback> callback =
            new NativeArchiveFixture::CWriter({{"renamed.txt", payload_}}, {});
        QCOMPARE(backend_->updateNativeArchive(reader, output, 1, callback), S_OK);
        QCOMPARE(output->Close(), S_OK);
        QVERIFY(Make(dir.path(), {"t", "--", updatedPath}));
        finished.clear();
        backend_->list(updatedPath);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(finished[0][0].toBool());
        const auto updated = qvariant_cast<QList<ArchiveEntry>>(entries.last()[0]);
        QCOMPARE(updated.size(), 1);
        QCOMPARE(updated.first().path, QStringLiteral("renamed.txt"));
        QCOMPARE(Bytes(path), original);
    }
};
QTEST_MAIN(NativeListingTests)
#include "native_listing_tests.moc"
