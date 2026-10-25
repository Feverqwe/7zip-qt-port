#include "archivenativeopen.h"
#include "archivereadonlyview.h"
#include "dot_path_cpio_fixture.h"
#include "native_archive_fixture.h"
#include "qtarchivefileresource.h"
#include "qtarchivereadoperation.h"
#include <QTemporaryDir>
#include <QtTest>
#include <future>

namespace {
template <typename Function> auto Worker(Function work) {
    auto pending = std::async(std::launch::async, std::move(work));
    while (pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        QTest::qWait(1);
    return pending.get();
}
QByteArray Payload() {
    QByteArray data(200000, '\0');
    for (qsizetype i = 0; i < data.size(); ++i)
        data[i] = char(i * 31 + i / 137);
    return data;
}
HRESULT Open(const QString &path, const QString &format, CMyComPtr<IUnknown> &owner,
             std::shared_ptr<ArchiveNativeOpen> &reader) {
    RINOK(ArchiveBackend::openNativeArchive(NativeArchiveFixture::Catalog(), path, format, nullptr,
                                            reader, nullptr))
    if (!reader)
        return E_FAIL;
    auto *resource = new QtArchiveFileResource(path);
    owner = resource;
    resource->AttachNativeOpen(reader);
    resource->UseNativeMetadata();
    return S_OK;
}
quint64 FileId(const std::shared_ptr<ArchiveReadOnlyView> &view, const QString &name) {
    for (const auto &entry : view->entries())
        if (!entry.Directory && entry.Name == name)
            return entry.Id;
    return 0;
}
} // namespace

class ArchiveReadOnlyViewTests final : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        const auto catalog = NativeArchiveFixture::Catalog();
        QVERIFY2(catalog->result() == S_OK, qPrintable(catalog->error()));
    }
    void retainedRanges_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("direct tar stream") << QStringLiteral("tar");
        QTest::newRow("decoded zip cache") << QStringLiteral("zip");
    }
    void currentDirectoryEntries_data() {
        QTest::addColumn<QByteArray>("root");
        QTest::newRow("current directory") << QByteArray(".");
        QTest::newRow("repeated current directory") << QByteArray("./.");
    }
    void currentDirectoryEntries() {
        QFETCH(QByteArray, root);
        QTemporaryDir fixture;
        const QString path = fixture.filePath("Payload.cpio");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const auto bytes = DotPathCpioFixture(root);
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
        file.close();
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, "cpio", resource, reader); }), S_OK);
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString error;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(resource, 4096, {}, view, error);
                 }),
                 S_OK);
        QCOMPARE(view->entries().size(), 4);
        const auto id = FileId(view, QStringLiteral("имя 空.txt"));
        QVERIFY(id);
        const auto &entry = view->entries().at(qsizetype(id - 1));
        const auto &parent = view->entries().at(qsizetype(entry.Parent - 1));
        QCOMPARE(parent.Name, QStringLiteral("ProxyBridge.app"));
        QCOMPARE(parent.Parent, quint64(1));
        const auto result = Worker([&] { return view->read(id, 0, 1024); });
        QCOMPARE(result.Result, S_OK);
        QCOMPARE(result.Data, QByteArray("payload bytes"));
        const auto sidecar =
            Worker([&] { return view->read(FileId(view, "._ProxyBridge.app"), 0, 1024); });
        QCOMPARE(sidecar.Result, S_OK);
        QCOMPARE(sidecar.Data, QByteArray("sidecar bytes"));
    }
    void retainedRanges() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("выбранный 空 файл.bin");
        const auto payload = Payload();
        const QString path = fixture.filePath("source." + format);
        NativeArchiveFixture::Item item{"folder/" + name, payload};
        FILETIME modified;
        QVERIFY(NWindows::NTime::UnixTime64_To_FileTime(1700000000, modified));
        item.Modified = modified;
        QVERIFY(NativeArchiveFixture::Write(path, {item}, format));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, format, resource, reader); }), S_OK);
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString error;
        int confirmations = 0;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(
                         resource, payload.size(), {}, view, error,
                         [&](const QString &path, quint64 bytes) {
                             ++confirmations;
                             return path == "folder/" + name && bytes == quint64(payload.size());
                         });
                 }),
                 S_OK);
        const auto id = FileId(view, name);
        QVERIFY(id);
        for (const auto &entry : view->entries())
            if (entry.Id == id)
                QCOMPARE(entry.Modified, quint64(FILETIME_To_UInt64(modified)));
        resource.Release();
        reader.reset();
        for (const auto offset : {quint64(170001), quint64(7), quint64(199700)}) {
            const auto result = Worker([&] { return view->read(id, offset, 299); });
            QCOMPARE(result.Result, S_OK);
            QCOMPARE(result.Data, payload.mid(qsizetype(offset), 299));
        }
        const auto eof = Worker([&] { return view->read(id, payload.size(), 1024); });
        QCOMPARE(eof.Result, S_OK);
        QVERIFY(eof.Data.isEmpty());
        QCOMPARE(confirmations, format == "zip" ? 1 : 0);
        if (format == "zip") {
#ifdef Q_OS_UNIX
            // Windows read-locks the retained source; path truncation can only
            // simulate source loss on these hosts.
            QFile changed(path);
            QVERIFY(changed.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QCOMPARE(changed.write("damaged"), qint64(7));
            changed.close();
#endif
            const auto cached = Worker([&] { return view->read(id, 80000, 4096); });
            QCOMPARE(cached.Result, S_OK);
            QCOMPARE(cached.Data, payload.mid(80000, 4096));
            QCOMPARE(confirmations, 1);
        }
        view->cancel();
        const auto cancelled = Worker([&] { return view->read(id, 0, 1024); });
        QCOMPARE(cancelled.Result, E_ABORT);
        QVERIFY(cancelled.Data.isEmpty());
    }
    void cacheQuotaDoesNotGateDirectStreams_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("readable");
        QTest::newRow("direct") << QStringLiteral("tar") << true;
        QTest::newRow("requires decode") << QStringLiteral("zip") << false;
    }
    void panelGenerationReplacementDoesNotChangeMount_data() { retainedRanges_data(); }
    void panelGenerationReplacementDoesNotChangeMount() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        const auto payload = Payload();
        const QString original = fixture.filePath("original." + format);
        const QString replacement = fixture.filePath("replacement." + format);
        QVERIFY(NativeArchiveFixture::Write(original, {{"data.bin", payload}}, format));
        QVERIFY(NativeArchiveFixture::Write(
            replacement, {{"data.bin", QByteArray(payload.size(), 'x')}}, format));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(original, format, resource, reader); }), S_OK);
        auto view = ArchiveBackend::retainReadOnlyView(
            resource, payload.size(), {}, [](const QString &, quint64) { return true; });
        std::shared_ptr<ArchiveNativeOpen> next;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::openNativeArchive(NativeArchiveFixture::Catalog(),
                                                              replacement, format, nullptr, next,
                                                              nullptr);
                 }),
                 S_OK);
        auto *panelResource = dynamic_cast<QtArchiveFileResource *>(resource.Interface());
        panelResource->CloseNativeOpen();
        panelResource->AttachNativeOpen(next);
        panelResource->UseNativeMetadata();
        reader.reset();
        QString error;
        QCOMPARE(Worker([&] { return ArchiveBackend::prepareReadOnlyView(view, error); }), S_OK);
        const auto result =
            Worker([&] { return view->read(FileId(view, "data.bin"), 17000, 4096); });
        QCOMPARE(result.Result, S_OK);
        QCOMPARE(result.Data, payload.mid(17000, 4096));
    }
    void cacheQuotaDoesNotGateDirectStreams() {
        QFETCH(QString, format);
        QFETCH(bool, readable);
        QTemporaryDir fixture;
        const auto payload = Payload();
        const QString path = fixture.filePath("source." + format);
        QVERIFY(NativeArchiveFixture::Write(path, {{"data.bin", payload}}, format));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, format, resource, reader); }), S_OK);
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString error;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(resource, 1024, {}, view, error);
                 }),
                 S_OK);
        const auto id = FileId(view, "data.bin");
        QVERIFY(id);
        const auto result = Worker([&] { return view->read(id, 172003, 4096); });
        if (readable) {
            QCOMPARE(result.Result, S_OK);
            QCOMPARE(result.Data, payload.mid(172003, 4096));
        } else {
            QCOMPARE(result.Result, E_OUTOFMEMORY);
            QVERIFY(result.Data.isEmpty());
            QVERIFY(!result.Error.isEmpty());
        }
    }
    void extractionRequiresExplicitConfirmation() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("source.zip");
        QVERIFY(NativeArchiveFixture::Write(path, {{"data.bin", Payload()}}, "zip"));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, "zip", resource, reader); }), S_OK);
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString error;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(resource, 1000000, {}, view, error);
                 }),
                 S_OK);
        auto denied = Worker([&] { return view->read(FileId(view, "data.bin"), 0, 1024); });
        QCOMPARE(denied.Result, E_ABORT);
        QCOMPARE(denied.Operation, Int32(-1));
        QVERIFY(denied.Data.isEmpty());
        int confirmations = 0;
        bool accept = false;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(resource, 1000000, {}, view, error,
                                                               [&](const QString &, quint64) {
                                                                   ++confirmations;
                                                                   return accept;
                                                               });
                 }),
                 S_OK);
        const auto id = FileId(view, "data.bin");
        denied = Worker([&] { return view->read(id, 0, 1024); });
        QCOMPARE(denied.Result, E_ABORT);
        QCOMPARE(confirmations, 1);
        accept = true;
        const auto allowed = Worker([&] { return view->read(id, 0, 1024); });
        QCOMPARE(allowed.Result, S_OK);
        QCOMPARE(allowed.Data, Payload().left(1024));
        QCOMPARE(confirmations, 2);
        const auto cached = Worker([&] { return view->read(id, 8000, 1024); });
        QCOMPARE(cached.Result, S_OK);
        QCOMPARE(cached.Data, Payload().mid(8000, 1024));
        QCOMPARE(confirmations, 2);
    }
    void retainedExtractionHonorsOutputLimit() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("source.zip");
        QVERIFY(NativeArchiveFixture::Write(path, {{"data.bin", Payload()}}, "zip"));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, "zip", resource, reader); }), S_OK);
        const QString output = fixture.filePath("bounded.bin");
        QtArchiveReadOperation operation(reader, {});
        operation.Output.NativeIndex = 0;
        operation.Output.Path = output;
        operation.Output.Limit = 1024;
        operation.start();
        QVERIFY(operation.wait(10000));
        QVERIFY(operation.Result != S_OK);
        QVERIFY(operation.OutputClosed);
        QVERIFY(QFileInfo(output).size() <= 1024);
    }
    void unverifiedOutputRequiresSeparateConsent() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("damaged.zip");
        const auto payload = Payload();
        QVERIFY(NativeArchiveFixture::Write(path, {{"data.bin", payload}, {"good.bin", payload}},
                                            "zip", {}, true, true));
        QFile archive(path);
        QVERIFY(archive.open(QIODevice::ReadWrite));
        const auto stored = archive.readAll().indexOf(payload);
        QVERIFY(stored > 0);
        QVERIFY(archive.seek(stored));
        const char damaged = char(quint8(payload[0]) ^ 0x55);
        QCOMPARE(archive.write(&damaged, 1), qint64(1));
        archive.close();
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, "zip", resource, reader); }), S_OK);
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString error;
        bool allow = false;
        int decisions = 0;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(
                         resource, payload.size() * 2, {}, view, error,
                         [](const QString &, quint64) { return true; },
                         [&](const QString &name, quint64 bytes, Int32 operation) {
                             ++decisions;
                             return allow && name == "data.bin" &&
                                    bytes == quint64(payload.size()) &&
                                    operation == NArchive::NExtract::NOperationResult::kCRCError;
                         });
                 }),
                 S_OK);
        const auto id = FileId(view, "data.bin");
        const auto denied = Worker([&] { return view->read(id, 0, 1024); });
        QVERIFY(denied.Result != S_OK);
        QVERIFY(denied.Data.isEmpty());
        QCOMPARE(decisions, 1);
        allow = true;
        const auto unverified = Worker([&] { return view->read(id, 0, 1024); });
        QCOMPARE(unverified.Result, S_OK);
        QCOMPARE(unverified.Operation, NArchive::NExtract::NOperationResult::kCRCError);
        QVERIFY(!unverified.Error.isEmpty());
        QCOMPARE(unverified.Data, QByteArray(1, damaged) + payload.mid(1, 1023));
        allow = false;
        const auto cached = Worker([&] { return view->read(id, 10000, 1024); });
        QCOMPARE(cached.Result, S_OK);
        QCOMPARE(cached.Operation, unverified.Operation);
        QVERIFY(!cached.Error.isEmpty());
        QCOMPARE(cached.Data, payload.mid(10000, 1024));
        const auto good = Worker([&] { return view->read(FileId(view, "good.bin"), 0, 1024); });
        QCOMPARE(good.Result, S_OK);
        QCOMPARE(good.Operation, NArchive::NExtract::NOperationResult::kOK);
        QVERIFY(good.Error.isEmpty());
        QCOMPARE(decisions, 2);
    }
    void evictedEntryRequiresFreshExtractionConsent() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("source.zip");
        const auto payload = Payload();
        QVERIFY(NativeArchiveFixture::Write(path, {{"first.bin", payload}, {"second.bin", payload}},
                                            "zip"));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, "zip", resource, reader); }), S_OK);
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString error;
        QStringList authorized;
        bool allow = true;
        QCOMPARE(Worker([&] {
                     return ArchiveBackend::createReadOnlyView(resource, payload.size(), {}, view,
                                                               error,
                                                               [&](const QString &entry, quint64) {
                                                                   authorized.append(entry);
                                                                   return allow;
                                                               });
                 }),
                 S_OK);
        for (const auto &name : {QStringLiteral("first.bin"), QStringLiteral("second.bin")}) {
            const auto result = Worker([&] { return view->read(FileId(view, name), 5000, 4096); });
            QCOMPARE(result.Result, S_OK);
            QCOMPARE(result.Data, payload.mid(5000, 4096));
        }
        allow = false;
        const auto denied = Worker([&] { return view->read(FileId(view, "first.bin"), 0, 4096); });
        QCOMPARE(denied.Result, E_ABORT);
        QVERIFY(denied.Data.isEmpty());
        QCOMPARE(authorized, QStringList({"first.bin", "second.bin", "first.bin"}));
        const auto cached = Worker([&] { return view->read(FileId(view, "second.bin"), 0, 4096); });
        QCOMPARE(cached.Result, S_OK);
        QCOMPARE(cached.Data, payload.left(4096));
        QCOMPARE(authorized.size(), 3);
    }
    void rejectsUnsafeOrAmbiguousNamespace_data() {
        QTest::addColumn<QStringList>("names");
        QTest::newRow("parent traversal") << QStringList{"../outside.bin"};
        QTest::newRow("parent after current directory") << QStringList{"./../outside.bin"};
        QTest::newRow("current directory collision") << QStringList{"a/./data.bin", "a/data.bin"};
        QTest::newRow("absolute current directory") << QStringList{"/./outside.bin"};
        QTest::newRow("case collision") << QStringList{"DATA.bin", "data.bin"};
        QTest::newRow("stream syntax") << QStringList{"data:stream.bin"};
    }
    void rejectsUnsafeOrAmbiguousNamespace() {
        QFETCH(QStringList, names);
        QTemporaryDir fixture;
        const QString path = fixture.filePath("unsafe.zip");
        QList<NativeArchiveFixture::Item> items;
        for (const auto &name : names)
            items.append({name, "x"});
        QVERIFY(NativeArchiveFixture::Write(path, items, "zip"));
        CMyComPtr<IUnknown> resource;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(Worker([&] { return Open(path, "zip", resource, reader); }), S_OK);
        std::shared_ptr<ArchiveReadOnlyView> view;
        QString error;
        QVERIFY(Worker([&] {
                    return ArchiveBackend::createReadOnlyView(resource, 4096, {}, view, error);
                }) != S_OK);
        QVERIFY(!view);
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo::exists(fixture.filePath("outside.bin")));
    }
};

QTEST_MAIN(ArchiveReadOnlyViewTests)
#include "archive_readonly_view_tests.moc"
