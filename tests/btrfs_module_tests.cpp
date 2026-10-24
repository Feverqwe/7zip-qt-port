#include "btrfs_native_fixture.h"

class BtrfsModuleTests final : public QObject {
    Q_OBJECT
    QLibrary module_{deployedModulePath(QString::fromUtf8(SEVENZIP_BTRFS_MODULE))};
    CMyComPtr<IInArchive> archive_;
    HRESULT open(QByteArray data) {
        CMyComPtr<IInStream> input = new Input(std::move(data));
        return archive_->Open(input, nullptr, nullptr);
    }
    UInt32 index(const QString &path = name) {
        UInt32 count = 0;
        archive_->GetNumberOfItems(&count);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant prop;
            if (archive_->GetProperty(i, kpidPath, &prop) == S_OK && prop.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(prop.bstrVal)) == path)
                return i;
        }
        return UInt32(-1);
    }
    void createArchive() {
        archive_.Release();
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && create);
        NWindows::NCOM::CPropVariant clsid;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
        GUID guid;
        memcpy(&guid, clsid.bstrVal, sizeof(guid));
        QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
    }
  private slots:
    void initTestCase() { createArchive(); }
    void linuxImages_data() {
        QTest::addColumn<QString>("variant");
        const auto images = manifest()["images"].toObject();
        QVERIFY(!images.isEmpty());
        for (auto it = images.begin(); it != images.end(); ++it)
            QTest::newRow(qPrintable(it.key())) << it.key();
    }
    void linuxImages() {
        QFETCH(QString, variant);
        QCOMPARE(open(image(variant)), S_OK);
        const auto files = manifest()["images"]
                               .toObject()[variant]
                               .toObject()["expected"]
                               .toObject()["files"]
                               .toObject();
        for (auto it = files.begin(); it != files.end(); ++it) {
            const UInt32 item = index(it.key());
            QVERIFY2(item != UInt32(-1), qPrintable(it.key()));
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(&item, 1, 0, callback), S_OK);
            QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(callback->output->bytes.size(), it->toObject()["size"].toInteger());
            QCOMPARE(QString::fromLatin1(QCryptographicHash::hash(callback->output->bytes,
                                                                  QCryptographicHash::Sha256)
                                             .toHex()),
                     it->toObject()["sha256"].toString());
        }
        NWindows::NCOM::CPropVariant inode1, inode2, root1, root2;
        QCOMPARE(archive_->GetProperty(index("subvolume/same-inode.txt"), kpidINode, &inode1),
                 S_OK);
        QCOMPARE(archive_->GetProperty(index("snapshot/same-inode.txt"), kpidINode, &inode2), S_OK);
        QCOMPARE(archive_->GetProperty(index("subvolume/same-inode.txt"), kpidStreamId, &root1),
                 S_OK);
        QCOMPARE(archive_->GetProperty(index("snapshot/same-inode.txt"), kpidStreamId, &root2),
                 S_OK);
        QCOMPARE(inode1.uhVal.QuadPart, inode2.uhVal.QuadPart);
        QVERIFY(root1.uhVal.QuadPart != root2.uhVal.QuadPart);
        NWindows::NCOM::CPropVariant link;
        QCOMPARE(archive_->GetProperty(index("symlink.txt"), kpidSymLink, &link), S_OK);
        QCOMPARE(link.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(link.bstrVal), name);
    }
    void errorsCancellationAndReuse() {
        auto *input = new Input(image());
        CMyComPtr<IInStream> source = input;
        input->fail = true;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), E_FAIL);
        input->fail = false;
        QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        const UInt32 item = index();
        QVERIFY(item != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), E_ABORT);
        callback->cancel = false;
        callback->cancelAfterOutput = true;
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), E_ABORT);
        QVERIFY(!callback->output->bytes.isEmpty());
        callback->cancelAfterOutput = false;
        callback->output->bytes.clear();
        input->fail = true;
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), E_FAIL);
        input->fail = false;
        callback->output->fail = true;
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), E_FAIL);
        callback->output->fail = false;
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), S_OK);
        QCOMPARE(callback->output->bytes, payload());
    }
    void unsupportedFeatures_data() {
        QTest::addColumn<quint64>("feature");
        QTest::addColumn<QString>("reason");
        QTest::newRow("zoned") << (quint64(1) << 12) << QString("zoned");
        QTest::newRow("extent-tree-v2") << (quint64(1) << 13) << QString("extent-tree-v2");
        QTest::newRow("stripe-tree") << (quint64(1) << 14) << QString("RAID stripe tree");
        QTest::newRow("remap-tree") << (quint64(1) << 17) << QString("remap tree");
        QTest::newRow("unknown") << (quint64(1) << 63) << QString("unknown incompatible feature");
    }
    void unsupportedFeatures() {
        QFETCH(quint64, feature);
        QFETCH(QString, reason);
        QCOMPARE(open(incompatibleImage(feature)), S_FALSE);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        NWindows::NCOM::CPropVariant flags, error;
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QCOMPARE(flags.vt, VARTYPE(VT_UI4));
        QVERIFY(flags.ulVal & kpv_ErrorFlags_UnsupportedFeature);
        QVERIFY(!(flags.ulVal & kpv_ErrorFlags_IsNotArc));
        QCOMPARE(archive_->GetArchiveProperty(kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        QVERIFY(QString::fromWCharArray(error.bstrVal).contains(reason));
        QCOMPARE(open(image()), S_OK);
        QCOMPARE(archive_->GetArchiveProperty(kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
    }
    void checksumFailureIsNotSuccessfulExtraction() {
        BtrfsFixture::Reader reference;
        reference.bytes = image();
        QCOMPARE(reference.Open(), 0);
        QMap<QString, btrfs_file_info> items;
        QCOMPARE(BtrfsFixture::walk(reference, items), 0);
        const auto info = items[name];
        int error = 0;
        auto file = BtrfsFixture::open(reference.volume.get(), info.root, info.ino, error);
        QCOMPARE(error, 0);
        reference.reads.clear();
        QByteArray buffer(reference.info.sectorsize, '\0');
        size_t processed = 0;
        QCOMPARE(btrfs_pread_file(file.get(), 0, buffer.data(), buffer.size(), &processed), 0);
        uint64_t physical = UINT64_MAX;
        for (const auto &read : reference.reads)
            if (read.size == reference.info.sectorsize) {
                physical = read.offset;
                break;
            }
        QVERIFY(physical < uint64_t(reference.bytes.size()));
        reference.bytes[qsizetype(physical + 17)] ^= char(1);
        QCOMPARE(open(reference.bytes), S_OK);
        const UInt32 item = index();
        QVERIFY(item != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kCRCError));
        QVERIFY(callback->output->bytes.isEmpty());
    }
    void retainedStreamsAndAttributes() {
        QCOMPARE(open(image("zstd")), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> original, shared, attribute;
        QCOMPARE(provider->GetStream(index(), &original), S_OK);
        QCOMPARE(provider->GetStream(index("shared.bin"), &shared), S_OK);
        const UInt32 attr = index(name + ":$XATTR.user.comment");
        QVERIFY(attr != UInt32(-1));
        QCOMPARE(provider->GetStream(attr, &attribute), S_OK);
        CMyComPtr<IInStream> first, second;
        QCOMPARE(original.QueryInterface(IID_IInStream, &first), S_OK);
        QCOMPARE(shared.QueryInterface(IID_IInStream, &second), S_OK);
        QCOMPARE(archive_->Close(), S_OK);
        QCOMPARE(open(image("plain")), S_OK);
        provider.Release();
        archive_.Release();
        auto expectedShared = payload();
        memset(expectedShared.data(), 0, 4096);
        for (qint64 offset : {qint64(131060), qint64(0), qint64(399990)}) {
            for (const auto &pair :
                 {qMakePair(first, payload()), qMakePair(second, expectedShared)}) {
                QCOMPARE(pair.first->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
                char buffer[37];
                UInt32 processed = 0;
                QCOMPARE(pair.first->Read(buffer, sizeof(buffer), &processed), S_OK);
                QCOMPARE(QByteArray(buffer, processed), pair.second.mid(offset, sizeof(buffer)));
            }
        }
        char buffer[64];
        UInt32 processed = 0;
        QCOMPARE(attribute->Read(buffer, sizeof(buffer), &processed), S_OK);
        QCOMPARE(QByteArray(buffer, processed), QByteArray("binary\0value\xff", 13));
        createArchive();
    }
    void backendDiscoveryAndExtraction() {
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
#ifdef Q_OS_WIN
        const QString engineName = "7z.exe", coreName = "7z.dll";
#else
        const QString engineName = "7z", coreName = "7z.so";
#endif
        const QDir prepared(qEnvironmentVariable("SEVENZIP_BTRFS_TEST_RUNTIME",
                                                 QString::fromUtf8(SEVENZIP_BTRFS_RUNTIME)));
        const QString engine = runtime.filePath(engineName), core = runtime.filePath(coreName);
        QVERIFY(QFile::copy(prepared.filePath(engineName), engine));
        QVERIFY(QFile::copy(prepared.filePath(coreName), core));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        const QString module = deployedModulePath(QString::fromUtf8(SEVENZIP_BTRFS_MODULE));
        QVERIFY(QFile::copy(module, runtime.filePath("Formats/" + QFileInfo(module).fileName())));
        const QString mbr = deployedModulePath(QString::fromUtf8(SEVENZIP_MBR_MODULE));
        QVERIFY(QFile::copy(mbr, runtime.filePath("Formats/" + QFileInfo(mbr).fileName())));
        const QString path = runtime.filePath(QString::fromUtf8("образ 空.img"));
        QFile source(path);
        const auto data = image("zstd");
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(data), data.size());
        source.close();
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QVERIFY(backend.codecCatalog()->formatIndex("Btrfs") >= 0);
        QVERIFY(!backend.codecCatalog()->canUpdate("Btrfs"));
        {
            CMyComPtr<IInStream> input = new Input(incompatibleImage(quint64(1) << 14));
            std::shared_ptr<ArchiveNativeOpen> reader;
            QString error;
            COpenUi ui;
            QCOMPARE(OpenDeviceFilesystem(backend.codecCatalog(), input, "unsupported.img", &ui,
                                          reader, &error),
                     S_FALSE);
            QVERIFY(!reader);
            QVERIFY2(error.contains("RAID stripe tree"), qPrintable(error));
        }
        for (bool partitioned : {false, true}) {
            QByteArray disk = data;
            const UInt64 offset = partitioned ? 1024 * 1024 : 0;
            if (partitioned) {
                disk.prepend(QByteArray(qsizetype(offset), '\0'));
                auto *entry = reinterpret_cast<Byte *>(disk.data()) + 446;
                entry[4] = 0x83;
                qToLittleEndian(UInt32(offset / 4096), entry + 8);
                qToLittleEndian(UInt32(data.size() / 4096), entry + 12);
                disk[510] = 0x55;
                disk[511] = char(0xaa);
            }
            auto *physical = new CDeviceInput(disk);
            CMyComPtr<IInStream> raw = physical, direct;
            QCOMPARE(DeviceInput(raw, UInt64(disk.size()), &direct), S_OK);
            ArchiveDevice device(backend.codecCatalog());
            COpenUi ui;
            QCOMPARE(device.inspect(direct, UInt64(disk.size()), "USB диск 空", &ui), S_OK);
            QCOMPARE(device.partitions().size(), 1);
            QCOMPARE(device.partitions()[0].offset, offset);
            std::shared_ptr<ArchiveNativeOpen> reader;
            QString error;
            QCOMPARE(device.openPartition(0, &ui, reader, &error), S_OK);
            QCOMPARE(reader->resolvedOpenType(), QString("Btrfs"));
            UInt32 count = 0, selected = 0;
            QCOMPARE(reader->arc().Archive->GetNumberOfItems(&count), S_OK);
            for (; selected < count; ++selected) {
                NWindows::NCOM::CPropVariant value;
                QCOMPARE(reader->arc().Archive->GetProperty(selected, kpidPath, &value), S_OK);
                if (value.vt == VT_BSTR &&
                    QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal)) == name)
                    break;
            }
            QVERIFY(selected < count);
            auto *output = new CExtractUi;
            CMyComPtr<IArchiveExtractCallback> callback = output;
            QCOMPARE(reader->arc().Archive->Extract(&selected, 1, 0, callback), S_OK);
            QCOMPARE(output->ItemResult, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(output->bytes(), payload());
            reader.reset();
            ui.Cancel = true;
            QCOMPARE(device.openPartition(0, &ui, reader), E_ABORT);
            QVERIFY(!reader);
            QVERIFY(physical->FarthestRead <= UInt64(disk.size()));
        }
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QVERIFY(std::any_of(entries.cbegin(), entries.cend(),
                            [](const ArchiveEntry &entry) { return entry.path == name; }));
        finished.clear();
        backend.test(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        backend.extract(path, runtime.filePath("output"), {name});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile extracted(runtime.filePath("output/" + name));
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), payload());
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), data);
        source.close();
        QVERIFY(QDir(runtime.path()).mkdir("multi"));
        const auto members = BtrfsFixture::devices("raid0");
        QVERIFY(members.size() >= 2);
        for (auto it = members.begin(); it != members.end(); ++it) {
            QFile member(runtime.filePath(QString("multi/devid-%1.img").arg(it.key())));
            QVERIFY(member.open(QIODevice::WriteOnly));
            QCOMPARE(member.write(it.value()), it.value().size());
        }
        // Open a secondary member through the real backend volume callback.
        const QString selected = runtime.filePath("multi/devid-2.img");
        finished.clear();
        backend.list(selected);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QStringList selectedFiles{name, "subvolume/same-inode.txt",
                                        "snapshot/same-inode.txt"};
        backend.extract(selected, runtime.filePath("multi-output"), selectedFiles);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        const auto expected = BtrfsFixture::multiManifest()["profiles"]
                                  .toObject()["raid0"]
                                  .toObject()["expected"]
                                  .toObject()["files"]
                                  .toObject();
        for (const auto &path : selectedFiles) {
            QFile file(runtime.filePath("multi-output/" + path));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(
                QString::fromLatin1(
                    QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex()),
                expected[path].toObject()["sha256"].toString());
        }
        for (auto it = members.begin(); it != members.end(); ++it) {
            QFile member(runtime.filePath(QString("multi/devid-%1.img").arg(it.key())));
            QVERIFY(member.open(QIODevice::ReadOnly));
            QCOMPARE(member.readAll(), it.value());
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(BtrfsModuleTests)
#include "btrfs_module_tests.moc"
